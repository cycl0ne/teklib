#ifndef _TEK_INLINE_PS2_H
#define _TEK_INLINE_PS2_H

/*
**	$Id: ps2.h,v 1.6 2007/05/19 14:04:32 fschulze Exp $
**	teklib/tek/inline/ps2.h - ps2 inline calls
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/proto/ps2.h>

extern struct TPS2ModBase *TPS2Base;

/*****************************************************************************/

#define u_dumpreg(reg)						TPS2DebugDumpReg(TPS2Base,reg)
#define u_hexdump(s,data,qwc)				TPS2DebugHexDump(TPS2Base,s,data,qwc)
#define d_initManager(chn,sz)				TPS2DMAInit(TPS2Base,chn,sz)
#define d_alloc(chn,qwc,cb,udata)			TPS2DMAAlloc(TPS2Base,chn,qwc,cb,udata)
#define	d_allocCall(chn,dmadata,cb,udata)	TPS2DMAAllocCall(TPS2Base,chn,dmadata,cb,udata)
#define d_allocspr(chn,qwc,cb,udata)		TPS2DMAAllocSPR(TPS2Base,chn,qwc,cb,udata)
#define d_commit(chn)						TPS2DMACommit(TPS2Base,chn)
#define g_setReg(reg,data)					TPS2GSSetReg(TPS2Base,reg,data)
#define g_vsync()							TPS2GSVSync(TPS2Base)
#define g_enableZBuf(ctx)					TPS2GSEnableZBuf(TPS2Base,ctx)
#define g_clearScreen(ctx)					TPS2GSClearScreen(TPS2Base,ctx)
#define g_clearScreenAlpha(ctx)				TPS2GSClearScreenAlpha(TPS2Base,ctx)
#define g_flipBuffer(ctx,fb)				TPS2GSFlipBuffer(TPS2Base,ctx,fb)
#define g_setClearColor(ctx,rgba)			TPS2GSSetClearColor(TPS2Base,ctx,rgba)
#define g_setDepthFunc(ctx,dfunc)			TPS2GSSetDepthFunc(TPS2Base,ctx,dfunc)
#define g_setDepthClear(ctx,clear)			TPS2GSSetDepthClear(TPS2Base,ctx,clear)
#define g_initImage(img,w,h,psm,data)		TPS2GSInitImage(TPS2Base,img,w,h,psm,data)
#define g_loadImage(img)					TPS2GSLoadImage(TPS2Base,img)
#define g_freeImage(img)					TPS2GSFreeImage(TPS2Base,img)
#define g_initScreen(mode,inter,ffmd)		TPS2GSInitScreen(TPS2Base,mode,inter,ffmd)
#define g_initDisplay(ctx,dx,dy,magh,magv,w,h,d,xc,yc)	TPS2GSInitDisplay(TPS2Base,ctx,dx,dy,magh,magv,w,h,d,xc,yc)
#define g_initFramebuffer(ctx,fbp0,fbp1,psm)	TPS2GSInitFramebuffer(TPS2Base,ctx,fbp0,fbp1,psm)
#define g_enableContext(ctx,onoff)			TPS2GSEnableContext(TPS2Base,ctx,onoff)
#define g_initZBuf(ctx,zbp,zpsm,dfunc,dclear)	TPS2GSInitZBuf(TPS2Base,ctx,zbp,zpsm,dfunc,dclear)
#define g_initTexEnv(tbp,tw,th)				TPS2GSInitTexEnv(TPS2Base,tbp,tw,th)
#define g_initTexReg(ctx,tcc,tfx,gsimage)	TPS2GSInitTexReg(TPS2Base,ctx,tcc,tfx,gsimage)
#define g_init(mode,inter,ffmd)				TPS2GSInit(TPS2Base,mode,inter,ffmd)
#define g_allocMem(ctx,type,psm)			TPS2GSAllocMem(TPS2Base,ctx,type,psm)
#define g_freeMem(ctx,type,page)			TPS2GSFreeMem(TPS2Base,ctx,type,page)
#define g_drawTRect(ctx,x,y,w,h,img)		TPS2DrawTRect(TPS2Base,ctx,x,y,w,h,img)
#define g_drawFRect(ctx,x,y,w,h,rgb)		TPS2DrawFRect(TPS2Base,ctx,x,y,w,h,rgb)
#define g_drawRect(ctx,x,y,w,h,rgb)			TPS2DrawRect(TPS2Base,ctx,x,y,w,h,rgb)
#define g_drawLine(ctx,x1,y1,x2,y2,rgb)		TPS2DrawLine(TPS2Base,ctx,x1,y1,x2,y2,rgb)
#define g_drawPoint(ctx,x,y,rgb)			TPS2DrawPoint(TPS2Base,ctx,x,y,rgb)
#define g_drawTRectUV(ctx,x0,y0,x1,y1,u0,v0,u1,v1,img)	TPS2DrawTRectUV(TPS2Base,ctx,x0,y0,x1,y1,u0,v0,u1,v1,img)
#define g_drawTPointUV(ctx,x,y,u,v,img)		TPS2DrawTPointUV(TPS2Base,ctx,x,y,u,v,img)
#define g_syncPath(mode)					TPS2GSSyncPath(TPS2Base,mode)
#define g_getImage(img)						TPS2GSGetImage(TPS2Base,img)
#define g_setActiveFb(ctx,fbp)				TPS2GSSetActiveFb(TPS2Base,ctx,fbp)
#define g_getReg(reg)						TPS2GSGetReg(TPS2Base,reg)
#define GS_SET_CSR(signal,finish,hsint,vsint,edwint,flush,reset,nfield,field,fifo,rev,id)	\
											TPS2GSSetCsr(TPS2Base,signal,finish,hsint,vsint,edwint,flush,reset,nfield, \
															field,fifo,rev,id)
#define GS_SET_PMODE(en1,en2,mmod,amod,slbg,alp)	TPS2GSSetPmode(TPS2Base,en1,en2,mmod,amod,slbg,alp)
#define GS_SET_SMODE2(inter,ffmd,dpms)		TPS2GSSetSmode2(TPS2Base,inter,ffmd,dpms)
#define GS_SET_DISPFB1(fbp,fbw,psm,dbx,dby)	TPS2GSSetDispfb1(TPS2Base,fbp,fbw,psm,dbx,dby)
#define GS_SET_DISPLAY1(dx,dy,magh,magv,dw,dh)	TPS2GSSetDisplay1(TPS2Base,dx,dy,magh,magv,dw,dh)
#define GS_SET_DISPFB2(fbp,fbw,psm,dbx,dby)	TPS2GSSetDispfb2(TPS2Base,fbp,fbw,psm,dbx,dby)
#define GS_SET_DISPLAY2(dx,dy,magh,magv,dw,dh)	TPS2GSSetDisplay2(TPS2Base,dx,dy,magh,magv,dw,dh)
#define GS_SET_BGCOLOR(r,g,b)				TPS2GSSetBgcolor(TPS2Base,r,g,b)
#define g_setRegCb(reg,data,cbfunc,cbdata)	TPS2GSSetRegCb(TPS2Base,reg,data,cbfunc,cbdata)
#define g_setVisibleFb(ctx,fbp)				TPS2GSSetVisibleFb(TPS2Base,ctx,fbp)

#endif /* _TEK_INLINE_PS2_H */
