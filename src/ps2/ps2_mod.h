#ifndef _TEK_MOD_PS2_MOD_H
#define _TEK_MOD_PS2_MOD_H

/*
**	$Id: ps2_mod.h,v 1.7 2007/05/19 14:12:20 fschulze Exp $
**	teklib/mods/ps2/ps2_mod.h - PS2 module internal definitions
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/exec.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/debug.h>
#include <tek/teklib.h>

#include <tek/mod/ps2.h>
#include <tek/mod/ps2/dma.h>
#include <tek/mod/ps2/gs.h>
#include <tek/mod/ps2/type3d.h>

#define MOD_VERSION			0
#define MOD_REVISION		1
#define MOD_NUMVECTORS		76

/*****************************************************************************/

#define DMA_MAXCALLS	256

struct DMACallBack
{
	DMACB dcb_Func;
	TAPTR dcb_UserData;
};

struct DMAManager
{
	struct DMACallBack dma_AllocCallstack[DMA_MAXCALLS];
	struct DMACallBack dma_BusyCallstack[DMA_MAXCALLS];
	TAPTR dma_MemPool;
	TAPTR dma_MemPoolSPR;
	TQWDATA *dma_AllocChain;
	struct DMACallBack *dma_AllocCallptr;
	TQWDATA *dma_AllocLastTagptr;
	TQWDATA *dma_BusyChain;
	struct DMACallBack *dma_BusyCallptr;
	TUINT dma_Channel;
	volatile TBOOL dma_Ready;
	volatile TUINT dma_mode;
};

/*****************************************************************************/

typedef struct TPS2ModBase TMOD_PS2;

struct TPS2ModPrivate
{
	GSinfo ps2_GSInfo;
	TAPTR ps2_Lock;				/* module base lock */
	TUINT ps2_RefCount;			/* module reference counter */
	TAPTR ps2_UtilBase;			/* utility module base */
	TAPTR ps2_HALBase;			/* HAL module base */
	TAPTR ps2_MemManager;		/* module's memory manager */
	struct DMAManager ps2_DMAManager[DMC_GIF + 1];
};

#define TExecBase TGetExecBase(TPS2Base)
#define TUtilBase ((struct TPS2ModPrivate *) TPS2Base->ps2_Private)->ps2_UtilBase
#define THALBase ((struct TPS2ModPrivate *) TPS2Base->ps2_Private)->ps2_HALBase

/*****************************************************************************/
/*
**	module API
*/

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

#ifndef LOCAL
#define LOCAL
#endif

/*****************************************************************************/

LOCAL TVOID dma_reset(TVOID);
LOCAL TINT txa_alloc(struct TXAlloc *t, TUINT fmt, TINT w, TINT h);
LOCAL TVOID txa_free(struct TXAlloc *t, TUINT fmt, TINT w, TINT h, TINT b);

EXPORT TVOID util_dumpreg(TMOD_PS2 *TPS2Base, TUINT regdesc);
EXPORT TVOID util_hexdump(TMOD_PS2 *TPS2Base, TSTRPTR s, TQWDATA *data, TINT qwc);

EXPORT TQWDATA *dma_alloc(TMOD_PS2 *TPS2Base, TUINT chn, TUINT qwc, DMACB cb, TAPTR udata);
EXPORT TQWDATA *dma_allocCall(TMOD_PS2 *TPS2Base, TUINT chn, TQWDATA* dmadata, DMACB cb, TAPTR udata);

EXPORT TVOID dma_commit(TMOD_PS2 *TPS2Base, TUINT channel);
EXPORT TVOID dma_initManager(TMOD_PS2 *TPS2Base, TUINT channel, TUINT size);
EXPORT TQWDATA *dma_allocspr(TMOD_PS2 *TPS2Base, TUINT chn, TUINT qwc, DMACB cb, TAPTR udata);

EXPORT TVOID gs_setReg(TMOD_PS2 *TPS2Base, TINT reg, TUINT64 data);
EXPORT TVOID gs_vsync(TMOD_PS2 *TPS2Base);
EXPORT TVOID gs_enableZBuf(TMOD_PS2 *TPS2Base, TINT ctx);
EXPORT TVOID gs_clearScreen(TMOD_PS2 *TPS2Base, TINT ctx);
EXPORT TVOID gs_clearScreenAlpha(TMOD_PS2 *TPS2Base, TINT ctx);
EXPORT TVOID gs_flipBuffer(TMOD_PS2 *TPS2Base, TINT ctx, TINT fb);
EXPORT TVOID gs_setClearColor(TMOD_PS2 *TPS2Base, TINT ctx, gs_rgbaq_packed *rgba);
EXPORT TVOID gs_setDepthFunc(TMOD_PS2 *TPS2Base, TINT ctx, TINT dfunc);
EXPORT TVOID gs_setDepthClear(TMOD_PS2 *TPS2Base, TINT ctx, TINT clear);
EXPORT TINT  gs_initImage(TMOD_PS2 *TPS2Base, GSimage *gsimage, TINT w, TINT h, TINT psm, TAPTR data);
EXPORT TVOID gs_loadImage(TMOD_PS2 *TPS2Base, GSimage *gsimage);
EXPORT TVOID gs_freeImage(TMOD_PS2 *TPS2Base, GSimage *gsimage);
EXPORT TVOID gs_initScreen(TMOD_PS2 *TPS2Base, TUINT16 mode, TUINT16 inter, TUINT16 ffmd);
EXPORT TVOID gs_initDisplay(TMOD_PS2 *TPS2Base, TINT ctx, TINT dx, TINT dy, TINT magh, TINT magv, TINT w, TINT h, TINT d, TINT xc, TINT yc);
EXPORT TVOID gs_initFramebuffer(TMOD_PS2 *TPS2Base, TINT ctx, TINT fbp0, TINT fbp1, TINT psm);
EXPORT TVOID gs_enableContext(TMOD_PS2 *TPS2Base, TINT ctx, TBOOL onoff);
EXPORT TVOID gs_initZBuf(TMOD_PS2 *TPS2Base, TINT ctx, TINT zbp, TINT zpsm, TINT dfunc, TINT dclear);
EXPORT TVOID gs_initTexEnv(TMOD_PS2 *TPS2Base, TINT tbp, TINT tw, TINT th);
EXPORT TVOID gs_initTexReg(TMOD_PS2 *TPS2Base, TINT ctx, TINT tcc, TINT tfx, GSimage *gsimage);
EXPORT TVOID gs_init(TMOD_PS2 *TPS2Base, TINT mode, TINT inter, TINT ffmd);
EXPORT TINT  gs_allocMem(TMOD_PS2 *TPS2Base, TINT ctx, TINT memtype, TINT psm);
EXPORT TVOID gs_freeMem(TMOD_PS2 *TPS2Base, TINT ctx, TINT memtype, TINT startpage);
EXPORT TVOID gs_drawTRect(TMOD_PS2 *TPS2Base, TINT ctx, TINT x, TINT y, TINT w, TINT h, GSimage *gsimage);
EXPORT TVOID gs_drawFRect(TMOD_PS2 *TPS2Base, TINT ctx, TINT x, TINT y, TINT w, TINT h, gs_rgbaq_packed *rgb);
EXPORT TVOID gs_drawRect(TMOD_PS2 *TPS2Base, TINT ctx, TINT x, TINT y, TINT w, TINT h, gs_rgbaq_packed *rgb);
EXPORT TVOID gs_drawLine(TMOD_PS2 *TPS2Base, TINT ctx, TINT x1, TINT y1, TINT x2, TINT y2, gs_rgbaq_packed *rgb);
EXPORT TVOID gs_drawPoint(TMOD_PS2 *TPS2Base, TINT ctx, TINT x, TINT y, gs_rgbaq_packed *rgb);
EXPORT TVOID gs_drawTRectUV(TMOD_PS2 *TPS2Base, TINT ctx, TINT x0, TINT y0, TINT x1, TINT y1, TINT u0, TINT v0, TINT u1, TINT v1, GSimage *gsimage);
EXPORT TVOID gs_drawTPointUV(TMOD_PS2 *TPS2Base, TINT ctx, TINT x, TINT y, TINT u, TINT v, GSimage *gsimage);
EXPORT TINT  gs_syncPath(TMOD_PS2 *TPS2Base, TINT mode);
EXPORT TVOID gs_getImage(TMOD_PS2 *TPS2Base, GSimage *img);
EXPORT TVOID gs_setActiveFb(TMOD_PS2 *TPS2Base, TINT ctx, TINT fbp);
EXPORT TUINT64 gs_getReg(TMOD_PS2 *TPS2Base, TINT reg);
EXPORT TVOID gs_set_csr(TMOD_PS2 *TPS2Base, TINT signal, TINT finish, TINT hsint, TINT vsint, TINT edwint,
						TINT flush, TINT reset, TINT nfield, TINT field, TINT fifo, TINT rev, TINT id);
EXPORT TVOID gs_set_pmode(TMOD_PS2 *TPS2Base, TINT en1, TINT en2, TINT mmod, TINT amod, TINT slbg, TINT alp);
EXPORT TVOID gs_set_smode2(TMOD_PS2 *TPS2Base, TINT inter, TINT ffmd, TINT dpms);
EXPORT TVOID gs_set_dispfb1(TMOD_PS2 *TPS2Base, TINT fbp, TINT fbw, TINT psm, TINT dbx, TINT dby);
EXPORT TVOID gs_set_display1(TMOD_PS2 *TPS2Base, TINT dx, TINT dy, TINT magh, TINT magv, TINT dw, TINT dh);
EXPORT TVOID gs_set_dispfb2(TMOD_PS2 *TPS2Base, TINT fbp, TINT fbw, TINT psm, TINT dbx, TINT dby);
EXPORT TVOID gs_set_display2(TMOD_PS2 *TPS2Base, TINT dx, TINT dy, TINT magh, TINT magv, TINT dw, TINT dh);
EXPORT TVOID gs_set_bgcolor(TMOD_PS2 *TPS2Base, TINT r, TINT g, TINT b);
EXPORT TVOID gs_setRegCb(TMOD_PS2 *TPS2Base, TINT reg, TUINT64 data, DMACB cbfunc, TAPTR cbdata);
EXPORT TVOID gs_setVisibleFb(TMOD_PS2 *TPS2Base, TINT ctx, TINT fbp);


/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_mod.h,v $
**	Revision 1.7  2007/05/19 14:12:20  fschulze
**	- GSvmode is obsolete -> adapted gs_initScreen
**	- g_initContext renamed to g_initFramebuffer
**
**	Revision 1.6  2006/03/10 17:01:34  fschulze
**	added gs_setRegCb() and gs_setVisibleFb()
**
**	Revision 1.5  2006/02/24 15:45:18  fschulze
**	renamed modbase to TPS2Base; adapted to renamed ps2 common
**	functions; added protos for new gs functions
**
**	Revision 1.4  2005/11/20 17:31:41  tmueller
**	character types corrected
**
**	Revision 1.3  2005/10/07 12:22:06  fschulze
**	renamed some primitive drawing functions
**
**	Revision 1.2  2005/10/05 22:03:55  fschulze
**	added new gs functions: local mem downloads, syncing of paths and
**	activation of framebuffers; renamed setactivebuf to setvisiblebuf;
**	fixed interface definition of GSVSync
**
**	Revision 1.1  2005/09/18 12:38:11  fschulze
**	added
**
**
*/

#endif
