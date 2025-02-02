
/*
**	$Id: audio_mod.c,v 1.5 2005/09/13 02:41:44 tmueller Exp $
**	teklib/mods/audio/audio_mod.c - Simple asynchronous audio device
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	Only TIOCMD_WRITE is currently supported, but with great flexibility;
**	the mixer handles any number of sounds of any format and rate at a time.
**
**	TODO:
**		- synchronized writes ("NULL" requests)
**		- playpos notification
*/

#include "audio_mod.h"

static TCALLBACK TVOID mod_destroy(TMOD_DEV *mod);
static TCALLBACK TIOMSG *mod_open(TMOD_DEV *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_DEV *mod, TAPTR task);

static TMODAPI TVOID aud_beginio(TMOD_DEV *mod, TIOMSG *msg);
static TMODAPI TINT aud_abortio(TMOD_DEV *mod, TIOMSG *msg);

static TTASKENTRY TVOID aud_task(TAPTR task);

/*****************************************************************************/
/*
**	module init/exit
*/

TMODENTRY TUINT 
tek_init_audio(TAPTR task, TMOD_DEV *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)					/* first call */
		{
			if (version <= MOD_VERSION)			/* version check */
			{
				return sizeof(TMOD_DEV);		/* return module positive size */
			}
		}
		else									/* second call */
		{
			return sizeof(TAPTR) * 2;
		}
	}
	else										/* third call */
	{
		mod->exec = TGetExecBase(mod);
		mod->lock = TExecCreateLock(TExecBase, TNULL);
		mod->mixerlock = TExecCreateLock(TExecBase, TNULL);
		if (mod->lock && mod->mixerlock)
		{		
			#ifdef TDEBUG
			mod->mmu = TExecCreateMMU(TExecBase, TNULL, TMMUT_Debug, TNULL);
			#endif

			mod->module.tmd_Version = MOD_VERSION;
			mod->module.tmd_Revision = MOD_REVISION;
			mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
			mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;
			mod->module.tmd_DestroyFunc = (TDFUNC) mod_destroy;

			((TAPTR *) mod)[-1] = (TAPTR) aud_beginio;
			((TAPTR *) mod)[-2] = (TAPTR) aud_abortio;

			return TTRUE;
		}
		TDestroy(mod->mixerlock);
		TDestroy(mod->lock);
	}

	return 0;
}

static TCALLBACK TVOID 
mod_destroy(TMOD_DEV *mod)
{
	TDestroy(mod->mmu);
	TDestroy(mod->mixerlock);
	TDestroy(mod->lock);
}

/*****************************************************************************/
/*
**	module open/close
*/

static TCALLBACK TIOMSG *
mod_open(TMOD_DEV *mod, TAPTR task, TTAGITEM *tags)
{
	TIOMSG *msg;

	msg = TExecAllocMsg0(TExecBase, sizeof(TIOMSG));
	if (msg)
	{
		TExecLock(TExecBase, mod->lock);
		
		if (!mod->task)
		{			
			mod->inittags = tags;
			mod->tasktags[0].tti_Tag = TTask_UserData;
			mod->tasktags[0].tti_Value = (TTAG) mod;
			mod->tasktags[1].tti_Tag = TTAG_DONE;
			mod->task = TExecCreateTask(TExecBase, (TTASKFUNC) aud_task,
				(TINITFUNC) aud_init, mod->tasktags);
		}

		if (mod->task)
		{
			mod->refcount++;
			
			/* Insert I/O request defaults. */

			msg->audio_Std.io_Req.io_Device = (struct TModule *) mod;
			msg->audio_Tags = mod->attributes;
			msg->audio_Format = mod->mixfmt;
			msg->audio_Rate = mod->mixfreq;
			msg->audio_Volume = 0x00010000;
		}
		else
		{	
			TExecFree(TExecBase, msg);
			msg = TNULL;
		}

		TExecUnlock(TExecBase, mod->lock);
	}

	return msg;
}

static TCALLBACK TVOID 
mod_close(TMOD_DEV *mod, TAPTR task)
{
	TExecLock(TExecBase, mod->lock);

	if (mod->task)
	{
		if (--mod->refcount == 0)
		{
			TExecSignal(TExecBase, mod->task, TTASK_SIG_ABORT);
			TDestroy(mod->task);
			mod->task = TNULL;
		}
	}

	TExecUnlock(TExecBase, mod->lock);
}

/*****************************************************************************/
/*
**	aud_replymsg(mod, msg)
**	Reply including up-to-date msg->io_Actual
*/

static TVOID
aud_replymsg(TMOD_DEV *mod, TIOMSG *msg)
{
	msg->audio_Std.io_Actual = (TUINT) (msg->audio_Pos * msg->audio_BytesPS);
	TExecReplyMsg(TExecBase, msg);
}

/*****************************************************************************/
/*
**	aud_docmd
*/

static TVOID
aud_docmd(TMOD_DEV *mod, TIOMSG *msg)
{
	TIOMSG *prev = msg->audio_Link;
	msg->audio_Std.io_Req.io_Error = 0;
	
	switch (msg->audio_Std.io_Req.io_Command)
	{
		default:

			msg->audio_Std.io_Req.io_Error = TIOERR_UNKNOWN_COMMAND;
			tdbprintf(10,"unknown I/O request\n");
			TExecReplyMsg(TExecBase, msg);
			break;

		case TIOCMD_WRITE:

			msg->audio_Std.io_Actual = 0;
			msg->audio_Pos = 0;
			msg->audio_Delta = (TFLOAT) msg->audio_Rate / mod->mixfreq;
			msg->audio_NumChan = TADIO_GETNUMCHANNELS(msg->audio_Format);
			msg->audio_BytesPC = TADIO_GETBYTESPERCHAN(msg->audio_Format);
			msg->audio_BytesPS = msg->audio_NumChan * msg->audio_BytesPC;
			msg->audio_Length = msg->audio_Std.io_Length / msg->audio_BytesPS;

			if (prev)
			{
				if (prev->audio_Flags & TAUDIOF_ABORTED)
				{
					/* links to a message that has been aborted already */
					aud_replymsg(mod, msg);
					break;
				}
			
				/* link to already existing channel */
				prev->audio_Next = msg;
				msg->audio_Flags |= TAUDIOF_QUEUED;
			}
			else
			{
				/* this message constitutes a new channel */
				TAddTail(&mod->chanlist, (TNODE *) msg);
				msg->audio_Flags |= TAUDIOF_ACTIVE;
			}

			break;
	}
}

/*****************************************************************************/
/*
**	aud_mix()
*/

static TVOID
aud_mix(TMOD_DEV *mod)
{
	TINT numsmp, pos;
	TBOOL endfrag = TFALSE;

	numsmp = mod->playcursor - mod->writecursor;
	if (numsmp <= 0) numsmp += mod->mixbufsize;
	if (numsmp == 0) return;
	
	for (pos = mod->writecursor; !endfrag && pos < mod->writecursor + numsmp; ++pos)
	{
		TNODE *nextnode, *node = mod->chanlist.tlh_Head;
		TUINT spos;
		TAPTR sptr;
		TINT s, sl = 0, sr = 0;

		for (; (nextnode = node->tln_Succ); node = nextnode)
		{
			TIOMSG *msg = (TIOMSG *) node;

			spos = (TUINT) msg->audio_Pos;
			if (spos >= msg->audio_Length)
			{
				msg->audio_Flags &= ~TAUDIOF_ACTIVE;

				msg = msg->audio_Next;
				if (msg)
				{
					if (!(msg->audio_Flags & TAUDIOF_QUEUED))
					{
						/* Must wait for arrival of follow-up node */
						goto skip;
					}
					TAddHead(&mod->chanlist, (TNODE *) msg);
					msg->audio_Flags &= ~TAUDIOF_QUEUED;
					msg->audio_Flags |= TAUDIOF_ACTIVE;
					spos = (TUINT) msg->audio_Pos;
				}

				TRemove(node);
				aud_replymsg(mod, (TIOMSG *) node);

				if (!msg) continue;
				endfrag = TTRUE;
			}

			sptr = msg->audio_Std.io_Data;
			switch (msg->audio_Format)
			{
				case TADIOFMT_U8M:
					s = (TINT) ((TUINT8 *) sptr)[spos];			// 8
					s -= 128;
					s *= msg->audio_Volume;						// 24
					sl += s;
					sr += s;
					break;

				case TADIOFMT_U8S:
					spos <<= 1;
					s = (TINT) ((TUINT8 *) sptr)[spos];			// 8
					s -= 128;
					s *= msg->audio_Volume;						// 24
					sl += s;
					s = (TINT) ((TUINT8 *) sptr)[spos + 1];
					s -= 128;
					s *= msg->audio_Volume;						// 24
					sr += s;
					break;

				case TADIOFMT_S8M:
					s = (TINT) ((TINT8 *) sptr)[spos];			// 8
					s *= msg->audio_Volume;						// 24
					sl += s;
					sr += s;
					break;

				case TADIOFMT_S8S:
					spos <<= 1;
					s = (TINT) ((TINT8 *) sptr)[spos];			// 8
					s *= msg->audio_Volume;						// 24
					sl += s;
					s = (TINT) ((TINT8 *) sptr)[spos + 1];
					s *= msg->audio_Volume;						// 24
					sr += s;
					break;

				case TADIOFMT_S16M:
					s = (TINT) ((TINT16 *) sptr)[spos];			// 16
					s *= msg->audio_Volume >> 8;				// 24
					sl += s;
					sr += s;
					break;

				case TADIOFMT_S16S:
					spos <<= 1;
					s = (TINT) ((TINT16 *) sptr)[spos];			// 16
					s *= msg->audio_Volume >> 8;				// 24
					sl += s;
					s = (TINT) ((TINT16 *) sptr)[spos + 1];		// 16
					s *= msg->audio_Volume >> 8;				// 24
					sr += s;
					break;
			}
			
			msg->audio_Pos += msg->audio_Delta;
		}

		if (sl < -8388608) sl = -8388608; else if (sl > 8388607) sl = 8388607;
		if (sr < -8388608) sr = -8388608; else if (sr > 8388607) sr = 8388607;

		s = pos % mod->mixbufsize;
		switch (mod->mixfmt)
		{
			case TADIOFMT_U8M:
				((TUINT8 *) mod->mixbuf)[s] = ((sl + sr) >> 17) + 128;
				break;

			case TADIOFMT_U8S:
				s <<= 1;
				((TUINT8 *) mod->mixbuf)[s] = (sl >> 16) + 128;
				((TUINT8 *) mod->mixbuf)[s + 1] = (sr >> 16) + 128;
				break;

			case TADIOFMT_S8M:
				((TINT8 *) mod->mixbuf)[s] = (sl + sr) >> 17;
				break;

			case TADIOFMT_S8S:
				s <<= 1;
				((TINT8 *) mod->mixbuf)[s] = sl >> 16;
				((TINT8 *) mod->mixbuf)[s + 1] = sr >> 16;
				break;

			case TADIOFMT_S16M:
				((TINT16 *) mod->mixbuf)[s] = (sl + sr) >> 9;
				break;

			case TADIOFMT_S16S:
				s <<= 1;
				((TINT16 *) mod->mixbuf)[s] = sl >> 8;
				((TINT16 *) mod->mixbuf)[s + 1] = sr >> 8;
				break;
		}
	}

skip:

	mod->writecursor = pos % mod->mixbufsize;
}

/*****************************************************************************/
/*
**	handler task
*/

static TTASKENTRY TVOID
aud_task(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_DEV *mod = TExecGetTaskData(exec, task);

	TAPTR port = TExecGetUserPort(exec, task);
	TUINT portsig = TExecGetPortSignal(exec, port);
	TIOMSG *msg;
	TUINT sigs;

	do
	{
		aud_write(mod);

		TExecLock(exec, mod->mixerlock);
	
		while ((msg = TExecGetMsg(exec, port))) aud_docmd(mod, msg);
		aud_mix(mod);

		TExecUnlock(exec, mod->mixerlock);

		sigs = aud_wait(mod, portsig | TTASK_SIG_ABORT);
		
	} while (!(sigs & TTASK_SIG_ABORT));

	aud_exit(mod);
}

/*****************************************************************************/
/*
**	beginio(msg->io_Device, msg)
**	Calls to this device are always asynchronous
*/

static TMODAPI TVOID
aud_beginio(TMOD_DEV *mod, TIOMSG *msg)
{
	TAPTR replyport = msg->audio_Std.io_Req.io_ReplyPort;
	TExecPutMsg(TExecBase, mod->port, replyport, msg);
}

/*****************************************************************************/
/*
**	abortio(msg->io_Device, msg)
**
**	We MUST provide a TAbortIO() mechanism, because this may be the only
**	way to break doublebuffer chains; they wait for each other and may
**	refuse to go away by themselves under unhappy multitasking conditions
*/

static TMODAPI TINT
aud_abortio(TMOD_DEV *mod, TIOMSG *msg)
{
	TINT error = -1;

	TExecLock(TExecBase, mod->mixerlock);
	
	msg->audio_Flags |= TAUDIOF_ABORTED;	/* prevent further chaining */

	if (msg->audio_Flags & TAUDIOF_ACTIVE)
	{
		TRemove((TNODE *) msg);
		msg->audio_Flags &= ~TAUDIOF_ACTIVE;
		aud_replymsg(mod, msg);
		error = 0;
	}
	else if (msg->audio_Flags & TAUDIOF_QUEUED)
	{
		msg->audio_Flags &= ~TAUDIOF_QUEUED;
		aud_replymsg(mod, msg);
		error = 0;
	}

	TExecUnlock(TExecBase, mod->mixerlock);
	
	return error;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: audio_mod.c,v $
**	Revision 1.5  2005/09/13 02:41:44  tmueller
**	updated copyright reference
**	
**	Revision 1.4  2004/06/04 00:15:01  tmueller
**	Fixed some minor quirks that produced warnings
**	
**	Revision 1.3  2004/02/07 05:01:54  tmueller
**	Cleanup of history entries
**	
**	Revision 1.2  2003/12/13 14:12:56  tmueller
**	Minor sourcecode cleanup
**	
**	Revision 1.1.1.1  2003/12/11 07:19:56  tmueller
**	Krypton import
*/
