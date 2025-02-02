
#ifndef _TEK_MOD_PS2_MOD_H
#define _TEK_MOD_PS2_MOD_H

/* 
**	$Id: ps2_mod.h,v 1.3 2005/10/07 12:22:06 fschulze Exp $
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
#define MOD_NUMVECTORS		64

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
	TAPTR ps2_MMU;				/* module's memory manager */
	struct DMAManager ps2_DMAManager[DMC_GIF + 1];
};

#define TExecBase TGetExecBase(mod)
#define TUtilBase ((struct TPS2ModPrivate *) mod->ps2_Private)->ps2_UtilBase
#define THALBase ((struct TPS2ModPrivate *) mod->ps2_Private)->ps2_HALBase

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

LOCAL TVOID d_reset(TVOID);
LOCAL TINT txa_alloc(struct TXAlloc *t, TUINT fmt, TINT w, TINT h);
LOCAL TVOID txa_free(struct TXAlloc *t, TUINT fmt, TINT w, TINT h, TINT b);

EXPORT TVOID u_dumpreg(TMOD_PS2 *mod, TUINT regdesc);
EXPORT TVOID u_hexdump(TMOD_PS2 *mod, TINT8 *s, TQWDATA *data, TINT qwc);

EXPORT TQWDATA *d_alloc(TMOD_PS2 *mod, TUINT chn, TUINT qwc, DMACB cb, TAPTR udata);
EXPORT TQWDATA *d_allocCall(TMOD_PS2 *mod, TUINT chn, TQWDATA* dmadata, DMACB cb, TAPTR udata);

EXPORT TVOID d_commit(TMOD_PS2 *mod, TUINT channel);
EXPORT TVOID d_initManager(TMOD_PS2 *mod, TUINT channel, TUINT size);
EXPORT TQWDATA *d_allocspr(TMOD_PS2 *mod, TUINT chn, TUINT qwc, DMACB cb, TAPTR udata);

EXPORT TVOID g_setReg(TMOD_PS2 *mod, TINT reg, TUINT64 data);
EXPORT TVOID g_vsync(TMOD_PS2 *mod);
EXPORT TVOID g_enableZBuf(TMOD_PS2 *mod, TINT ctx);
EXPORT TVOID g_clearScreen(TMOD_PS2 *mod, TINT ctx);
EXPORT TVOID g_clearScreenAlpha(TMOD_PS2 *mod, TINT ctx);
EXPORT TVOID g_flipBuffer(TMOD_PS2 *mod, TINT ctx, TINT fb);
EXPORT TVOID g_setClearColor(TMOD_PS2 *mod, TINT ctx, gs_rgbaq_packed *rgba);
EXPORT TVOID g_setDepthFunc(TMOD_PS2 *mod, TINT ctx, TINT dfunc);
EXPORT TVOID g_setDepthClear(TMOD_PS2 *mod, TINT ctx, TINT clear);
EXPORT TINT  g_initImage(TMOD_PS2 *mod, GSimage *gsimage, TINT w, TINT h, TINT psm, TAPTR data);
EXPORT TVOID g_loadImage(TMOD_PS2 *mod, GSimage *gsimage);
EXPORT TVOID g_freeImage(TMOD_PS2 *mod, GSimage *gsimage);
EXPORT TVOID g_initScreen(TMOD_PS2 *mod, GSvmode mode, TINT inter, TINT ffmd);
EXPORT TVOID g_initDisplay(TMOD_PS2 *mod, TINT ctx, TINT dx, TINT dy, TINT magh, TINT magv, TINT w, TINT h, TINT d, TINT xc, TINT yc);
EXPORT TVOID g_initContext(TMOD_PS2 *mod, TINT ctx, TINT fbp0, TINT fbp1, TINT psm);
EXPORT TVOID g_enableContext(TMOD_PS2 *mod, TINT ctx, TBOOL onoff);
EXPORT TVOID g_initZBuf(TMOD_PS2 *mod, TINT ctx, TINT zbp, TINT zpsm, TINT dfunc, TINT dclear);
EXPORT TVOID g_initTexEnv(TMOD_PS2 *mod, TINT tbp, TINT tw, TINT th);
EXPORT TVOID g_initTexReg(TMOD_PS2 *mod, TINT ctx, TINT tcc, TINT tfx, GSimage *gsimage);
EXPORT TVOID g_init(TMOD_PS2 *mod, GSvmode mode, TINT inter, TINT ffmd);
EXPORT TINT  g_allocMem(TMOD_PS2 *mod, TINT ctx, TINT memtype, TINT psm);
EXPORT TVOID g_freeMem(TMOD_PS2 *mod, TINT ctx, TINT memtype, TINT startpage);
EXPORT TVOID g_drawTRect(TMOD_PS2 *mod, TINT ctx, TINT x, TINT y, TINT w, TINT h, GSimage *gsimage);
EXPORT TVOID g_drawFRect(TMOD_PS2 *mod, TINT ctx, TINT x, TINT y, TINT w, TINT h, gs_rgbaq_packed *rgb);
EXPORT TVOID g_drawRect(TMOD_PS2 *mod, TINT ctx, TINT x, TINT y, TINT w, TINT h, gs_rgbaq_packed *rgb);
EXPORT TVOID g_drawLine(TMOD_PS2 *mod, TINT ctx, TINT x1, TINT y1, TINT x2, TINT y2, gs_rgbaq_packed *rgb);
EXPORT TVOID g_drawPoint(TMOD_PS2 *mod, TINT ctx, TINT x, TINT y, gs_rgbaq_packed *rgb);
EXPORT TVOID g_drawTRectUV(TMOD_PS2 *mod, TINT ctx, TINT x0, TINT y0, TINT x1, TINT y1, TINT u0, TINT v0, TINT u1, TINT v1, GSimage *gsimage);
EXPORT TVOID g_drawTPointUV(TMOD_PS2 *mod, TINT ctx, TINT x, TINT y, TINT u, TINT v, GSimage *gsimage);
EXPORT TINT  g_syncPath(TMOD_PS2 *mod, TINT mode);
EXPORT TVOID g_getImage(TMOD_PS2 *mod, GSimage *img);
EXPORT TVOID g_setActiveFb(TMOD_PS2 *mod, TINT ctx, TINT fbp);

/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_mod.h,v $
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
