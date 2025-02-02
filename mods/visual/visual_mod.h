
#ifndef _TEK_MODS_VISUAL_MOD_H
#define _TEK_MODS_VISUAL_MOD_H

/*
**	$Id: visual_mod.h,v 1.5 2005/09/13 02:43:36 tmueller Exp $
**	teklib/mods/visual/visual_mod.h - Visual device
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/debug.h>
#include <tek/teklib.h>

#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>

#include <tek/mod/visual.h>

/*****************************************************************************/

#define MOD_VERSION			1
#define MOD_REVISION		0
#define MOD_NUMVECTORS		26

#define MAXREQPERINSTANCE	64

#ifndef LOCAL
#define LOCAL
#endif

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

#define TExecBase	TGetExecBase(mod)
#define TUtilBase	mod->utilbase
#define TTimeBase	mod->timebase

/*****************************************************************************/

typedef struct Visual
{
	struct TModule module;		/* Module header */

	TAPTR hostspecific;			/* Ptr to host-specific data */

	TAPTR utilbase;				/* Utility module base */
	TAPTR timebase;				/* Time module base */
	TAPTR timereq;				/* Time request */

	TAPTR task;					/* Device task */
	TAPTR cmdrport;				/* Reply port */
	TAPTR mmu;					/* Memory manager */
	TAPTR lock;					/* Locking for module base structure */

	TAPTR cmdport;				/* Message port for device commands */
	TUINT cmdportsignal;
	TAPTR userport;				/* User's input msg port */

	TUINT refcount;				/* Count of module opens */

	TTAGITEM *inittags;			/* Tags passed by the user */
	TTAGITEM tasktags[2];		/* Task init tags */

	TLIST freelist;				/* pool of free requests */
	TLIST waitlist;				/* pool of requests waiting */
	TINT numrequests;			/* number of requests allocated */

	TUINT inputmask;			/* Current set of input types to listen for */
	TBOOL isclient;				/* Indicate whether this instance is client */

} TMOD_VIS;

/*****************************************************************************/
/*
**	Visual I/O request
*/

typedef struct TVRequestInternal
{
	struct TIORequest vis_Req;
	union
	{
		struct { TVPEN Pen; TUINT RGB; } AllocPen;
		struct { TVPEN Pen; } FreePen;
		struct { TVPEN Pen; TINT Rect[4]; } FRect;
		struct { TVPEN Pen; TINT Rect[4]; } Rect;
		struct { TVPEN Pen; TINT Rect[4]; } Line;
		struct { TVPEN Pen; TINT X, Y; } Plot;
		struct { TUINT Mask; } SetInput;
		struct { TVPEN Pen; } Clear;
		struct { TUINT *Buf; TINT RRect[5]; } DrawRGB;
		struct { TTAGITEM *Tags; TINT Num; } GetAttrs;
		struct { TBOOL Success; } Attach;
		struct { TVPEN BGPen, FGPen; TINT X, Y, Length; TSTRPTR Text; } Text;
		struct { TVPEN Pen; TINT16 *Array; TINT Num; } FPoly;
		struct { TINT SRect[6]; } Scroll;

	} vis_Op;

} TVREQ;

#define TVCMD_ATTACH		0x1000
#define TVCMD_DETACH		0x1001
#define TVCMD_GETATTRS		0x1002
#define TVCMD_FLUSH			0x1003
#define TVCMD_SETINPUT		0x1004
#define TVCMD_ALLOCPEN		0x1005
#define TVCMD_FREEPEN		0x1006
#define TVCMD_CLEAR			0x1007
#define TVCMD_FRECT			0x1008
#define TVCMD_RECT			0x1009
#define TVCMD_LINE			0x100a
#define TVCMD_PLOT			0x100b
#define TVCMD_DRAWRGB		0x100c
#define TVCMD_TEXT			0x100d
#define TVCMD_FPOLY			0x100e
#define TVCMD_SCROLL		0x100f

/*****************************************************************************/

LOCAL TBOOL vis_init(TMOD_VIS *mod);
LOCAL TVOID vis_exit(TMOD_VIS *mod);
LOCAL TVOID vis_docmd(TMOD_VIS *mod, TVREQ *req);
LOCAL TUINT vis_wait(TMOD_VIS *mod, TUINT waitsig);
LOCAL TVOID vis_wake(TMOD_VIS *mod);

LOCAL TVREQ *vis_getreq(TMOD_VIS *mod);
LOCAL TVOID vis_ungetreq(TMOD_VIS *mod, TVREQ *req);
LOCAL TVOID vis_sendimsg(TMOD_VIS *mod, TIMSG *imsg);

EXPORT TVOID vis_beginio(TMOD_VIS *mod, TVREQ *msg);
EXPORT TINT vis_abortio(TMOD_VIS *mod, TVREQ *msg);

EXPORT TAPTR tv_getport(TMOD_VIS *mod);
EXPORT TVPEN tv_allocpen(TMOD_VIS *mod, TUINT rgb);
EXPORT TVOID tv_frect(TMOD_VIS *mod, TINT x, TINT y, TINT w, TINT h, TVPEN pen);
EXPORT TVOID tv_clear(TMOD_VIS *mod, TVPEN pen);
EXPORT TVOID tv_drawrgb(TMOD_VIS *mod, TINT x, TINT y, TUINT *buf, TINT w, TINT h, TINT totw);
EXPORT TVOID tv_flush(TMOD_VIS *mod);
EXPORT TVOID tv_freepen(TMOD_VIS *mod, TVPEN pen);
EXPORT TVOID tv_rect(TMOD_VIS *mod, TINT x, TINT y, TINT w, TINT h, TVPEN pen);
EXPORT TVOID tv_line(TMOD_VIS *mod, TINT x1, TINT y1, TINT x2, TINT y2, TVPEN pen);
EXPORT TVOID tv_linearray(TMOD_VIS *mod, TINT16 *array, TINT n, TVPEN pen);
EXPORT TVOID tv_plot(TMOD_VIS *mod, TINT x, TINT y, TVPEN pen);
EXPORT TVOID tv_scroll(TMOD_VIS *mod, TINT x, TINT y, TINT w, TINT h, TINT dx, TINT dy);
EXPORT TVOID tv_text(TMOD_VIS *mod, TINT x, TINT y, TSTRPTR t, TUINT l, TVPEN bg, TVPEN fg);
EXPORT TVOID tv_flusharea(TMOD_VIS *mod, TINT x, TINT y, TINT w, TINT h);
EXPORT TUINT tv_setinput(TMOD_VIS *mod, TUINT cmask, TUINT smask);
EXPORT TUINT tv_getattrs(TMOD_VIS *mod, TTAGITEM *tags);
EXPORT TAPTR tv_attach(TMOD_VIS *mod, TTAGITEM *tags);
EXPORT TVOID tv_fpoly(TMOD_VIS *mod, TINT16 *array, TINT num, TVPEN pen);

/*****************************************************************************/
/*
**	Revision History
**	$Log: visual_mod.h,v $
**	Revision 1.5  2005/09/13 02:43:36  tmueller
**	updated copyright reference
**	
**	Revision 1.4  2005/09/11 01:27:57  tmueller
**	cosmetic
**	
**	Revision 1.3  2005/09/08 00:05:05  tmueller
**	API extended
**	
**	Revision 1.2  2004/01/13 02:19:40  tmueller
**	Reimplemented as a fully-featured, asynchronous Exec I/O device
**	
*/

#endif
