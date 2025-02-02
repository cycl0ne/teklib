
/*
**	$Id: visual_mod.c,v 1.9 2005/09/13 02:43:36 tmueller Exp $
**	teklib/mods/visual/visual_mod.c - Visual device
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	This is an experimental implementation of drawables, using an
**	async I/O device approach.
*/

#include "visual_mod.h"

static TCALLBACK TVOID mod_destroy(TMOD_VIS *mod);
static TCALLBACK TMOD_VIS *mod_open(TMOD_VIS *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_VIS *mod, TAPTR task);

static TTASKENTRY TBOOL vis_taskinit(TAPTR task);
static TTASKENTRY TVOID vis_task(TAPTR task);

static const TAPTR mod_vectors[MOD_NUMVECTORS];

/*****************************************************************************/
/*
**	module init/exit
*/

TMODENTRY TUINT 
tek_init_visual(TAPTR task, TMOD_VIS *mod, TUINT16 version, TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * MOD_NUMVECTORS;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_VIS);	/* positive size */

		return 0;
	}

	mod->lock = TCreateLock(TNULL);
	if (mod->lock)
	{
		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;
		mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;
		mod->module.tmd_DestroyFunc = (TDFUNC) mod_destroy;
		mod->module.tmd_Flags |= TMODF_EXTENDED;

		TInitVectors(mod, mod_vectors, MOD_NUMVECTORS);

		return TTRUE;
	}

	TDestroy(mod->lock);
	return 0;
}

static TCALLBACK TVOID 
mod_destroy(TMOD_VIS *mod)
{
	TDestroy(mod->mmu);
	TDestroy(mod->lock);
}

/*****************************************************************************/

static const TAPTR mod_vectors[MOD_NUMVECTORS] =
{
	vis_beginio,
	vis_abortio,
	TNULL,			/* reserved */
	TNULL,
	TNULL,
	TNULL,
	TNULL,
	TNULL,
	
	tv_getport,
	tv_allocpen,
	tv_frect,
	tv_clear,
	tv_drawrgb,
	tv_flush,
	tv_freepen,
	tv_rect,
	tv_line,
	tv_linearray,
	tv_plot,
	tv_scroll,
	tv_text,
	tv_flusharea,
	tv_setinput,
	tv_getattrs,
	tv_attach,
	tv_fpoly,
};

/*****************************************************************************/
/*
**	attach/detach
*/

static TBOOL
vis_attach(TMOD_VIS *mod)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_ATTACH;
	TDoIO((struct TIORequest *) req);
	vis_ungetreq(mod, req);
	return TTRUE;
}

static TVOID
vis_detach(TMOD_VIS *mod)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_DETACH;
	TDoIO((struct TIORequest *) req);
	vis_ungetreq(mod, req);
}

/*****************************************************************************/
/*
**	module open/close
*/

static TCALLBACK TMOD_VIS *
mod_open(TMOD_VIS *mod, TAPTR task, TTAGITEM *tags)
{
	TMOD_VIS *inst, *attach;

	attach = (TMOD_VIS *) TGetTag(tags, TVisual_Attach, TNULL);
	if (attach) mod = attach;		/* instance to attach to */

	inst = TNewInstance(mod, mod->module.tmd_PosSize, mod->module.tmd_NegSize);
	if (inst)
	{
		TBOOL success = TFALSE;

		inst->utilbase = TOpenModule("util", 0, TNULL);
		inst->timebase = TOpenModule("time", 0, TNULL);
		if (inst->utilbase && inst->timebase)
		{
			inst->timereq = TTimeAllocTimeRequest(inst->timebase, TNULL);
			inst->userport = TCreatePort(TNULL);
			inst->cmdrport = TCreatePort(TNULL);
			if (inst->timereq && inst->userport && inst->cmdrport)
			{
				inst->numrequests = 0;
				TInitList(&inst->freelist);
				TInitList(&inst->waitlist);

				if (attach)
				{
					success = inst->isclient = vis_attach(inst);
				}
				else
				{
					inst->inittags = tags;
					inst->tasktags[0].tti_Tag = TTask_UserData;
					inst->tasktags[0].tti_Value = (TTAG) inst;
					inst->tasktags[1].tti_Tag = TTAG_DONE;
					inst->task = TCreateTask((TTASKFUNC) vis_task,
						(TINITFUNC) vis_taskinit, inst->tasktags);

					if (inst->task)
					{
						inst->cmdport = TGetUserPort(inst->task);
						inst->cmdportsignal = TGetPortSignal(inst->cmdport);
						tv_setinput(inst, 0, TITYPE_CLOSE);
						success = TTRUE;
					}
				}
			}
		}

		if (!success)
		{
			TDestroy(inst->cmdrport);
			TDestroy(inst->userport);
			if (inst->timebase)
			{
				TTimeFreeTimeRequest(inst->timebase, inst->timereq);
				TCloseModule(inst->timebase);
			}
			TCloseModule(inst->utilbase);
			TFreeInstance(inst);
			inst = TNULL;
		}
	}

	return inst;
}

static TCALLBACK TVOID
mod_close(TMOD_VIS *mod, TAPTR task)
{
	TVREQ *req;

	if (mod->isclient)
	{
		/* detach ourselves */
		vis_detach(mod);
		vis_wake(mod);
	}

	/* wait for and free queued packages in the waitlist */
	while ((req = (TVREQ *) TRemHead(&mod->waitlist)))
	{
		vis_wake(mod);
		TWaitIO((struct TIORequest *) req);
		TFree(req);
	}

	if (!mod->isclient)
	{
		/* close task */
		TSignal(mod->task, TTASK_SIG_ABORT);
		vis_wake(mod);
		TDestroy(mod->task);
	}

	/* free queued packages in the freelist */
	while ((req = (TVREQ *) TRemHead(&mod->freelist)))
	{
		TFree(req);
	}

	/* closedown */
	TDestroy(mod->cmdrport);
	TDestroy(mod->userport);
	TFreeTimeRequest(mod->timereq);
	TCloseModule(mod->timebase);
	TCloseModule(mod->utilbase);
	TFreeInstance(mod);
}

/*****************************************************************************/
/*
**	handler task
*/

static TTASKENTRY TBOOL
vis_taskinit(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_VIS *mod = TExecGetTaskData(exec, task);
	return vis_init(mod);
}

static TTASKENTRY TVOID
vis_task(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_VIS *mod = TExecGetTaskData(exec, task);

	TAPTR port = TGetUserPort(task);
	TUINT portsig = TGetPortSignal(port);
	TVREQ *req;
	TUINT sigs;
	TBOOL abort = TFALSE;
	TINT numclients = 0;	/* number of clients of this visual */

	do
	{
		TBOOL sync;

		while ((req = TGetMsg(port)))
		{
			switch (req->vis_Req.io_Command)
			{
				default:
					vis_docmd(mod, req);
					break;

				case TVCMD_ATTACH:
					numclients++;
					break;

				case TVCMD_DETACH:
					numclients--;
					break;
			}

			/* check if this is a synchronous request */
			sync = (req->vis_Req.io_Flags & TIOF_QUICK);

			/* reply request */			
			TReplyMsg(req);

			/* sync requests break the loop */
			if (sync) break;
		}

		sigs = vis_wait(mod, portsig | TTASK_SIG_ABORT);
		if (sigs & TTASK_SIG_ABORT) abort = TTRUE;

	} while (numclients || !abort);

	vis_exit(mod);
}

/*****************************************************************************/
/*
**	beginio(msg->io_Device, msg)
**	abortio(msg->io_Device, msg)
*/

EXPORT TVOID
vis_beginio(TMOD_VIS *mod, TVREQ *req)
{
	TAPTR rport = req->vis_Req.io_ReplyPort;

	TPutMsg(mod->cmdport, rport, req);

	if (req->vis_Req.io_Flags & TIOF_QUICK)
	{
		vis_wake(mod);
	}
}

EXPORT TINT
vis_abortio(TMOD_VIS *mod, TVREQ *msg)
{
	return -1;		/* not supported */
}

/*****************************************************************************/

LOCAL TVREQ *
vis_getreq(TMOD_VIS *mod)
{
	TVREQ *req;

	req = (TVREQ *) TRemHead(&mod->freelist);
	if (req) return req;
	
	req = (TVREQ *) TFirstNode(&mod->waitlist);
	if (req)
	{
		if (!TCheckIO((struct TIORequest *) req))
		{
			if (mod->numrequests < MAXREQPERINSTANCE)
			{
				TVREQ *nreq = TAllocMsg(sizeof(TVREQ));
				if (nreq)
				{
					/* initialize I/O request */
					nreq->vis_Req.io_Device = (struct TModule *) mod;
					nreq->vis_Req.io_ReplyPort = mod->cmdrport;
					mod->numrequests++;
					return nreq;
				}
			}

			vis_wake(mod);
		}

		TWaitIO((struct TIORequest *) req);
		TRemove((struct TNode *) req);
		return req;
	}

	req = TAllocMsg(sizeof(TVREQ));
	if (req)
	{
		/* initialize I/O request */
		req->vis_Req.io_Device = (struct TModule *) mod;
		req->vis_Req.io_ReplyPort = mod->cmdrport;
		mod->numrequests++;
		return req;
	}

	tdbfatal(99);

	return req;
}

LOCAL TVOID vis_ungetreq(TMOD_VIS *mod, TVREQ *req)
{
	TAddTail(&mod->waitlist, (struct TNode *) req);
}

/*****************************************************************************/

LOCAL TVOID
vis_sendimsg(TMOD_VIS *mod, TIMSG *imsg)
{
	TQueryTime(mod->timereq, &imsg->timsg_TimeStamp);
	TPutMsg(mod->userport, TNULL, imsg);
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: visual_mod.c,v $
**	Revision 1.9  2005/09/13 02:43:36  tmueller
**	updated copyright reference
**	
**	Revision 1.8  2005/09/11 01:48:26  tmueller
**	introduced bug during initialization - fixed
**	
**	Revision 1.7  2005/09/11 01:27:57  tmueller
**	cosmetic
**	
**	Revision 1.6  2005/09/08 00:05:00  tmueller
**	API extended
**	
**	Revision 1.5  2005/05/06 20:47:32  fschulze
**	Removed debug MMU; now using new-style function names from the Time module
**	
**	Revision 1.4  2004/04/18 14:18:00  tmueller
**	TTAG changed to TUINTPTR; atomdata, parseargv changed from TAPTR to TTAG
**	
**	Revision 1.3  2004/01/13 03:01:06  tmueller
**	Visuals under Windows (resize, drag) behaved sloppy again... Fixed.
**	
**	Revision 1.2  2004/01/13 02:19:40  tmueller
**	Reimplemented as a fully-featured, asynchronous Exec I/O device
**	
**	Revision 1.2  2003/12/13 14:12:56  tmueller
*/
