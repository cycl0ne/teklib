
/*
**	display_fb_mod.c - Framebuffer display driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <unistd.h>
#include <fcntl.h>

#include "display_fb_mod.h"

static void fb_taskfunc(struct TTask *task);
static TBOOL fb_initinstance(struct TTask *task);
static void fb_exitinstance(FBDISPLAY *inst);
static TAPTR fb_modopen(FBDISPLAY *mod, TTAGITEM *tags);
static void fb_modclose(FBDISPLAY *mod);
static TMODAPI void fb_beginio(FBDISPLAY *mod,
	struct TVFBRequest *req);
static TMODAPI TINT fb_abortio(FBDISPLAY *mod,
	struct TVFBRequest *req);
static TMODAPI struct TVFBRequest *fb_allocreq(FBDISPLAY *mod);
static TMODAPI void fb_freereq(FBDISPLAY *mod,
	struct TVFBRequest *req);

static const TMFPTR
fb_vectors[FB_DISPLAY_NUMVECTORS] =
{
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) fb_beginio,
	(TMFPTR) fb_abortio,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) fb_allocreq,
	(TMFPTR) fb_freereq,
};

static void
fb_destroy(FBDISPLAY *mod)
{
	TDBPRINTF(TDB_TRACE,("Module destroy...\n"));
	TDestroy((struct THandle *) mod->fbd_Lock);
	FT_Done_FreeType(mod->fbd_FTLibrary);
}

static THOOKENTRY TTAG
fb_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	FBDISPLAY *mod = (FBDISPLAY *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			fb_destroy(mod);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) fb_modopen(mod, obj);
		case TMSG_CLOSEMODULE:
			fb_modclose(obj);
			break;
		case TMSG_INITTASK:
			return fb_initinstance(obj);
		case TMSG_RUNTASK:
			fb_taskfunc(obj);
			break;
	}
	return 0;
}

TMODENTRY TUINT
tek_init_display_rawfb(struct TTask *task, struct TModule *vis, TUINT16 version,
	TTAGITEM *tags)
{
	FBDISPLAY *mod = (FBDISPLAY *) vis;

	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * FB_DISPLAY_NUMVECTORS;

		if (version <= FB_DISPLAY_VERSION)
			return sizeof(FBDISPLAY);

		return 0;
	}

	TDBPRINTF(TDB_TRACE,("Module init...\n"));

	if (FT_Init_FreeType(&mod->fbd_FTLibrary) == 0)
	{
		for (;;)
		{
			mod->fbd_ExecBase = TGetExecBase(mod);
			mod->fbd_Lock = TExecCreateLock(mod->fbd_ExecBase, TNULL);
			if (mod->fbd_Lock == TNULL) break;

			mod->fbd_Module.tmd_Version = FB_DISPLAY_VERSION;
			mod->fbd_Module.tmd_Revision = FB_DISPLAY_REVISION;
			mod->fbd_Module.tmd_Handle.thn_Hook.thk_Entry = fb_dispatch;
			mod->fbd_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;
			TInitVectors(&mod->fbd_Module, fb_vectors, FB_DISPLAY_NUMVECTORS);
			return TTRUE;
		}
		fb_destroy(mod);
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	Module open/close
*/

static TAPTR fb_modopen(FBDISPLAY *mod, TTAGITEM *tags)
{
	TBOOL success = TFALSE;
	TExecLock(mod->fbd_ExecBase, mod->fbd_Lock);
	if (mod->fbd_RefCount == 0)
		success = fb_init(mod, tags);
	if (success)
		mod->fbd_RefCount++;
	TExecUnlock(mod->fbd_ExecBase, mod->fbd_Lock);
	if (success)
		return mod;
	return TNULL;
}

static void
fb_modclose(FBDISPLAY *mod)
{
	TDBPRINTF(TDB_TRACE,("Device close\n"));
	TExecLock(mod->fbd_ExecBase, mod->fbd_Lock);
	if (--mod->fbd_RefCount == 0)
		fb_exit(mod);
	TExecUnlock(mod->fbd_ExecBase, mod->fbd_Lock);
}

/*****************************************************************************/
/*
**	BeginIO/AbortIO
*/

static TMODAPI void
fb_beginio(FBDISPLAY *mod, struct TVFBRequest *req)
{
	TExecPutMsg(mod->fbd_ExecBase, mod->fbd_CmdPort,
		req->tvr_Req.io_ReplyPort, req);
}

static TMODAPI TINT
fb_abortio(FBDISPLAY *mod, struct TVFBRequest *req)
{
	return -1;
}

/*****************************************************************************/
/*
**	AllocReq/FreeReq
*/

static TMODAPI struct TVFBRequest *
fb_allocreq(FBDISPLAY *mod)
{
	struct TVFBRequest *req = TExecAllocMsg(mod->fbd_ExecBase,
		sizeof(struct TVFBRequest));
	if (req)
		req->tvr_Req.io_Device = (struct TModule *) mod;
	return req;
}

static TMODAPI void
fb_freereq(FBDISPLAY *mod, struct TVFBRequest *req)
{
	TExecFree(mod->fbd_ExecBase, req);
}

/*****************************************************************************/
/*
**	Module init/exit
*/

LOCAL TBOOL
fb_init(FBDISPLAY *mod, TTAGITEM *tags)
{
	mod->fbd_OpenTags = tags;

	for (;;)
	{
		TTAGITEM tags[2];
		tags[0].tti_Tag = TTask_UserData;
		tags[0].tti_Value = (TTAG) mod;
		tags[1].tti_Tag = TTAG_DONE;
		mod->fbd_Task = TExecCreateTask(mod->fbd_ExecBase,
			&mod->fbd_Module.tmd_Handle.thn_Hook, tags);
		if (mod->fbd_Task == TNULL) break;

		mod->fbd_CmdPort =
			TExecGetUserPort(mod->fbd_ExecBase, mod->fbd_Task);
		mod->fbd_CmdPortSignal = TExecGetPortSignal(mod->fbd_ExecBase,
			mod->fbd_CmdPort);

		return TTRUE;
	}

	fb_exit(mod);
	return TFALSE;
}

LOCAL void
fb_exit(FBDISPLAY *mod)
{
	if (mod->fbd_Task)
	{
		TExecSignal(mod->fbd_ExecBase, mod->fbd_Task, TTASK_SIG_ABORT);
		TDestroy((struct THandle *) mod->fbd_Task);
	}
}

/*****************************************************************************/

static TBOOL fb_initinstance(struct TTask *task)
{
	FBDISPLAY *mod = TExecGetTaskData(TGetExecBase(task), task);

	for (;;)
	{
		TTAGITEM *opentags = mod->fbd_OpenTags;
		TTAGITEM ftags[3];
		TSTRPTR subname;

		mod->fbd_Width = TGetTag(opentags, TVisual_Width, FB_DEF_WIDTH);
		mod->fbd_Height = TGetTag(opentags, TVisual_Height, FB_DEF_HEIGHT);
		mod->fbd_Modulo = TGetTag(opentags, TVisual_Modulo, 0);

		mod->fbd_BytesPerPixel = 4;
		mod->fbd_BytesPerLine =
			(mod->fbd_Width + mod->fbd_Modulo) * mod->fbd_BytesPerPixel;

		mod->fbd_BufPtr = (TUINT8 *) TGetTag(opentags, TVisual_BufPtr, TNULL);
		if (mod->fbd_BufPtr == TNULL)
		{
			mod->fbd_BufPtr = TExecAlloc0(mod->fbd_ExecBase, mod->fbd_MemMgr,
				mod->fbd_BytesPerLine * mod->fbd_Height *
				mod->fbd_BytesPerPixel);
			if (mod->fbd_BufPtr == TNULL)
				break;
			/* we own the buffer: */
			mod->fbd_BufferOwner = TTRUE;
		}

		/* list of free input messages: */
		TInitList(&mod->fbd_IMsgPool);

		/* list of all open visuals: */
		TInitList(&mod->fbd_VisualList);

		/* init fontmanager and default font */
		TInitList(&mod->fbd_FontManager.openfonts);

		/* Open sub device, if one is requested: */
		subname = (TSTRPTR) TGetTag(opentags, TVisual_DriverName,
			(TTAG) FB_DEF_RENDER_DEVICE);
		if (subname)
		{
			mod->fbd_RndRPort = TExecCreatePort(mod->fbd_ExecBase, TNULL);
			if (mod->fbd_RndRPort == TNULL)
				break;
			mod->fbd_RndDevice = TExecOpenModule(mod->fbd_ExecBase,
				subname, 0, TNULL);
			if (mod->fbd_RndDevice == TNULL)
				break;
			mod->fbd_RndRequest = TExecAllocMsg(mod->fbd_ExecBase,
				sizeof(struct TVFBRequest));
			if (mod->fbd_RndRequest == TNULL)
				break;
		}

		ftags[0].tti_Tag = TVisual_FontName;
		ftags[0].tti_Value = (TTAG) FNT_DEFNAME;
		ftags[1].tti_Tag = TVisual_FontPxSize;
		ftags[1].tti_Value = (TTAG) FNT_DEFPXSIZE;
		ftags[2].tti_Tag = TTAG_DONE;
		mod->fbd_FontManager.deffont = fb_hostopenfont(mod, ftags);
		if (mod->fbd_FontManager.deffont == TNULL)
			break;

		TDBPRINTF(TDB_TRACE,("Instance init successful\n"));
		return TTRUE;
	}

	fb_exitinstance(mod);
	return TFALSE;
}

static void
fb_exitinstance(FBDISPLAY *mod)
{
	struct TNode *imsg, *node, *next;

	/* free pooled input messages: */
	while ((imsg = TRemHead(&mod->fbd_IMsgPool)))
		TExecFree(mod->fbd_ExecBase, imsg);

	/* free queued input messages in all open visuals: */
	node = mod->fbd_VisualList.tlh_Head;
	for (; (next = node->tln_Succ); node = next)
	{
		FBWINDOW *v = (FBWINDOW *) node;

		/* unset active font in all open visuals */
		v->curfont = TNULL;

		while ((imsg = TRemHead(&v->fbv_IMsgQueue)))
			TExecFree(mod->fbd_ExecBase, imsg);
	}

	/* force closing of default font */
	mod->fbd_FontManager.defref = 0;

	/* close all fonts */
	node = mod->fbd_FontManager.openfonts.tlh_Head;
	for (; (next = node->tln_Succ); node = next)
		fb_hostclosefont(mod, (TAPTR) node);

	if (mod->fbd_BufferOwner)
		TExecFree(mod->fbd_ExecBase, mod->fbd_BufPtr);
	TExecFree(mod->fbd_ExecBase, mod->fbd_RndRequest);
	if (mod->fbd_RndDevice)
		TExecCloseModule(mod->fbd_ExecBase, mod->fbd_RndDevice);
	if (mod->fbd_RndRPort)
		TDestroy((struct THandle *) mod->fbd_RndRPort);
}

/*****************************************************************************/

static void
fb_docmd(FBDISPLAY *mod, struct TVFBRequest *req)
{
	switch (req->tvr_Req.io_Command)
	{
		case TVCMD_OPENWINDOW:
			fb_openvisual(mod, req);
			break;
		case TVCMD_CLOSEWINDOW:
			fb_closevisual(mod, req);
			break;
		case TVCMD_OPENFONT:
			fb_openfont(mod, req);
			break;
		case TVCMD_CLOSEFONT:
			fb_closefont(mod, req);
			break;
		case TVCMD_GETFONTATTRS:
			fb_getfontattrs(mod, req);
			break;
		case TVCMD_TEXTSIZE:
			fb_textsize(mod, req);
			break;
		case TVCMD_QUERYFONTS:
			fb_queryfonts(mod, req);
			break;
		case TVCMD_GETNEXTFONT:
			fb_getnextfont(mod, req);
			break;
		case TVCMD_SETINPUT:
			fb_setinput(mod, req);
			break;
		case TVCMD_GETATTRS:
			fb_getattrs(mod, req);
			break;
		case TVCMD_SETATTRS:
			fb_setattrs(mod, req);
			break;
		case TVCMD_ALLOCPEN:
			fb_allocpen(mod, req);
			break;
		case TVCMD_FREEPEN:
			fb_freepen(mod, req);
			break;
		case TVCMD_SETFONT:
			fb_setfont(mod, req);
			break;
		case TVCMD_CLEAR:
			fb_clear(mod, req);
			break;
		case TVCMD_RECT:
			fb_rect(mod, req);
			break;
		case TVCMD_FRECT:
			fb_frect(mod, req);
			break;
		case TVCMD_LINE:
			fb_line(mod, req);
			break;
		case TVCMD_PLOT:
			fb_plot(mod, req);
			break;
		case TVCMD_TEXT:
			fb_drawtext(mod, req);
			break;
		case TVCMD_DRAWSTRIP:
			fb_drawstrip(mod, req);
			break;
		case TVCMD_DRAWTAGS:
			fb_drawtags(mod, req);
			break;
		case TVCMD_DRAWFAN:
			fb_drawfan(mod, req);
			break;
		case TVCMD_COPYAREA:
			fb_copyarea(mod, req);
			break;
		case TVCMD_SETCLIPRECT:
			fb_setcliprect(mod, req);
			break;
		case TVCMD_UNSETCLIPRECT:
			fb_unsetcliprect(mod, req);
			break;
		case TVCMD_DRAWBUFFER:
			fb_drawbuffer(mod, req);
			break;
		case TVCMD_FLUSH:
			fb_flush(mod, req);
			break;
		default:
			TDBPRINTF(TDB_ERROR,("Unknown command code: %d\n",
			req->tvr_Req.io_Command));
	}
}

static void fb_taskfunc(struct TTask *task)
{
	FBDISPLAY *inst = TExecGetTaskData(TGetExecBase(task), task);
	struct TVFBRequest *req;
	TUINT sig;

	/* interval time: 1/50s: */
	TTIME intt = { 20000 };
	/* next absolute time to send interval message: */
	TTIME nextt;
	TTIME waitt, nowt;

	TExecGetSystemTime(inst->fbd_ExecBase, &nextt);
	TAddTime(&nextt, &intt);

	TDBPRINTF(TDB_INFO,("Device instance running\n"));

	do
	{
		TBOOL do_interval = TFALSE;

		while ((req = TExecGetMsg(inst->fbd_ExecBase, inst->fbd_CmdPort)))
		{
			fb_docmd(inst, req);
			TExecReplyMsg(inst->fbd_ExecBase, req);
		}

		/* calculate new delta to wait: */
		TExecGetSystemTime(inst->fbd_ExecBase, &nowt);
		waitt = nextt;
		TSubTime(&waitt, &nowt);

		TExecWaitTime(inst->fbd_ExecBase, &waitt, inst->fbd_CmdPortSignal);

		#if 0
		/* wait for input, signal fd and timeout: */
		if (select(inst->fbd_FDMax + 1, &rset, NULL, NULL, &tv) > 0)
		{
			if (FD_ISSET(inst->fbd_FDSigRead, &rset))
				/* consume one signal: */
				read(inst->fbd_FDSigRead, buf, 1);
		}
		#endif

		/* check if time interval has expired: */
		TExecGetSystemTime(inst->fbd_ExecBase, &nowt);
		if (TCmpTime(&nowt, &nextt) > 0)
		{
			/* expired; send interval: */
			do_interval = TTRUE;
			TAddTime(&nextt, &intt);
			if (TCmpTime(&nowt, &nextt) >= 0)
			{
				/* nexttime expired already; create new time from now: */
				nextt = nowt;
				TAddTime(&nextt, &intt);
			}
		}

		/* process input messages: */
// 		fb_processevent(inst);

		/* send out input messages to owners: */
		fb_sendimessages(inst, do_interval);

		/* get signal state: */
		sig = TExecSetSignal(inst->fbd_ExecBase, 0, TTASK_SIG_ABORT);

	} while (!(sig & TTASK_SIG_ABORT));

	TDBPRINTF(TDB_INFO,("Device instance closedown\n"));
	fb_exitinstance(inst);
}

LOCAL TBOOL fb_getimsg(FBDISPLAY *mod, FBWINDOW *v, TIMSG **msgptr, TUINT type)
{
	TIMSG *msg = (TIMSG *) TRemHead(&mod->fbd_IMsgPool);
	if (msg == TNULL)
		msg = TExecAllocMsg0(mod->fbd_ExecBase, sizeof(TIMSG));
	if (msg)
	{
		msg->timsg_Type = type;
		msg->timsg_Qualifier = mod->fbd_KeyQual;
		msg->timsg_MouseX = mod->fbd_MouseX;
		msg->timsg_MouseY = mod->fbd_MouseY;
		TExecGetSystemTime(mod->fbd_ExecBase, &msg->timsg_TimeStamp);
		*msgptr = msg;
		return TTRUE;
	}
	*msgptr = TNULL;
	return TFALSE;
}

LOCAL void fb_sendimessages(FBDISPLAY *mod, TBOOL do_interval)
{
	struct TNode *next, *node = mod->fbd_VisualList.tlh_Head;
	for (; (next = node->tln_Succ); node = next)
	{
		FBWINDOW *v = (FBWINDOW *) node;
		TIMSG *imsg;

		if (do_interval && (v->fbv_InputMask & TITYPE_INTERVAL) &&
			fb_getimsg(mod, v, &imsg, TITYPE_INTERVAL))
			TExecPutMsg(mod->fbd_ExecBase, v->fbv_IMsgPort, TNULL, imsg);

		while ((imsg = (TIMSG *) TRemHead(&v->fbv_IMsgQueue)))
			TExecPutMsg(mod->fbd_ExecBase, v->fbv_IMsgPort, TNULL, imsg);
	}
}

/*****************************************************************************/
/*
**	convert an utf8 encoded string to unicode
*/

struct readstringdata
{
	const unsigned char *src;
	size_t srclen;
};

static int readstring(struct utf8reader *rd)
{
	struct readstringdata *ud = rd->udata;
	if (ud->srclen == 0)
		return -1;
	ud->srclen--;
	return *ud->src++;
}

LOCAL TUINT32* fb_utf8tounicode(FBDISPLAY *mod, TSTRPTR utf8string, TINT len,
	TINT *bytelen)
{
	struct utf8reader rd;
	struct readstringdata rs;
	TUINT32 *unicode = mod->fbd_unicodebuffer;
	TINT i = 0;
	TINT c;

	rs.src = (unsigned char *) utf8string;
	rs.srclen = len;

	rd.readchar = readstring;
	rd.accu = 0;
	rd.numa = 0;
	rd.bufc = -1;
	rd.udata = &rs;

	while (i < RAWFB_UTF8_BUFSIZE - 1 && (c = readutf8(&rd)) >= 0)
	{
		unicode[i++] = c;
	}

	if (bytelen)
		*bytelen = i;

	return unicode;
}
