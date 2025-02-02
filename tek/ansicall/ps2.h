#ifndef _TEK_ANSICALL_PS2_H
#define _TEK_ANSICALL_PS2_H

/*
**	$Id: ps2.h,v 1.2 2005/10/05 22:03:55 fschulze Exp $
**	teklib/tek/ansicall/ps2.h - ps2 module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TPS2DebugDumpReg(ps2,regdesc) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUINT))(ps2))[-9]))(ps2,regdesc)

#define TPS2DebugHexDump(ps2,s,data,qwc) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT8 *,TQWDATA *,TINT))(ps2))[-10]))(ps2,s,data,qwc)

#define TPS2DMAInit(ps2,channel,size) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUINT,TUINT))(ps2))[-15]))(ps2,channel,size)

#define TPS2DMAAlloc(ps2,chn,qwc,cb,udata) \
	(*(((TMODCALL TQWDATA *(**)(TAPTR,TUINT,TUINT,DMACB,TAPTR))(ps2))[-16]))(ps2,chn,qwc,cb,udata)

#define TPS2DMACommit(ps2,channel) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUINT))(ps2))[-17]))(ps2,channel)

#define TPS2DMAAllocSPR(ps2,chn,qwc,cb,udata) \
	(*(((TMODCALL TQWDATA *(**)(TAPTR,TUINT,TUINT,DMACB,TAPTR))(ps2))[-18]))(ps2,chn,qwc,cb,udata)

#define TPS2DMAAllocCall(ps2,chn,dmadata,cb,udata) \
	(*(((TMODCALL TQWDATA *(**)(TAPTR,TUINT,TQWDATA*,DMACB,TAPTR))(ps2))[-19]))(ps2,chn,dmadata,cb,udata)

#define TPS2GSSetReg(ps2,reg,data) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TUINT64))(ps2))[-25]))(ps2,reg,data)

#define TPS2GSVSync(ps2) \
	(*(((TMODCALL TVOID(**)(TAPTR))(ps2))[-26]))(ps2)

#define TPS2GSEnableZBuf(ps2,ctx) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT))(ps2))[-27]))(ps2,ctx)

#define TPS2GSClearScreen(ps2,ctx) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT))(ps2))[-28]))(ps2,ctx)

#define TPS2GSClearScreenAlpha(ps2,ctx) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT))(ps2))[-29]))(ps2,ctx)

#define TPS2GSFlipBuffer(ps2,ctx,fb) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT))(ps2))[-30]))(ps2,ctx,fb)

#define TPS2GSSetClearColor(ps2,ctx,rgba) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,gs_rgbaq_packed *))(ps2))[-31]))(ps2,ctx,rgba)

#define TPS2GSSetDepthFunc(ps2,ctx,dfunc) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT))(ps2))[-32]))(ps2,ctx,dfunc)

#define TPS2GSSetDepthClear(ps2,ctx,clear) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT))(ps2))[-33]))(ps2,ctx,clear)

#define TPS2GSInitImage(ps2,gsimage,w,h,psm,data) \
	(*(((TMODCALL TINT(**)(TAPTR,GSimage *,TINT,TINT,TINT,TAPTR))(ps2))[-34]))(ps2,gsimage,w,h,psm,data)

#define TPS2GSLoadImage(ps2,gsimage) \
	(*(((TMODCALL TVOID(**)(TAPTR,GSimage *))(ps2))[-35]))(ps2,gsimage)

#define TPS2GSFreeImage(ps2,gsimage) \
	(*(((TMODCALL TVOID(**)(TAPTR,GSimage *))(ps2))[-36]))(ps2,gsimage)

#define TPS2GSInitScreen(ps2,mode,inter,ffmd) \
	(*(((TMODCALL TVOID(**)(TAPTR,GSvmode,TINT,TINT))(ps2))[-37]))(ps2,mode,inter,ffmd)

#define TPS2GSInitDisplay(ps2,ctx,dx,dy,magh,magv,w,h,d,xc,yc) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,TINT,TINT,TINT,TINT,TINT))(ps2))[-38]))(ps2,ctx,dx,dy,magh,magv,w,h,d,xc,yc)

#define TPS2GSInitContext(ps2,ctx,fbp0,fbp1,psm) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(ps2))[-39]))(ps2,ctx,fbp0,fbp1,psm)

#define TPS2GSEnableContext(ps2,ctx,onoff) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TBOOL))(ps2))[-40]))(ps2,ctx,onoff)

#define TPS2GSInitZBuf(ps2,ctx,zbp,zpsm,dfunc,dclear) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT))(ps2))[-41]))(ps2,ctx,zbp,zpsm,dfunc,dclear)

#define TPS2GSInitTexEnv(ps2,tbp,tw,th) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT))(ps2))[-42]))(ps2,tbp,tw,th)

#define TPS2GSInitTexReg(ps2,ctx,tcc,tfx,gsimage) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,GSimage *))(ps2))[-43]))(ps2,ctx,tcc,tfx,gsimage)

#define TPS2GSInit(ps2,mode,inter,ffmd) \
	(*(((TMODCALL TVOID(**)(TAPTR,GSvmode,TINT,TINT))(ps2))[-44]))(ps2,mode,inter,ffmd)

#define TPS2GSAllocMem(ps2,ctx,memtype,psm) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TINT,TINT))(ps2))[-45]))(ps2,ctx,memtype,psm)

#define TPS2GSFreeMem(ps2,ctx,memtype,startpage) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT))(ps2))[-46]))(ps2,ctx,memtype,startpage)

#define TPS2DrawTRect(ps2,ctx,x,y,w,h,gsimage) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,GSimage *))(ps2))[-47]))(ps2,ctx,x,y,w,h,gsimage)

#define TPS2DrawFRect(ps2,ctx,x,y,w,h,rgb) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,gs_rgbaq_packed *))(ps2))[-48]))(ps2,ctx,x,y,w,h,rgb)

#define TPS2DrawRect(ps2,ctx,x,y,w,h,rgb) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,gs_rgbaq_packed *))(ps2))[-49]))(ps2,ctx,x,y,w,h,rgb)

#define TPS2DrawLine(ps2,ctx,x1,y1,x2,y2,rgb) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,gs_rgbaq_packed *))(ps2))[-50]))(ps2,ctx,x1,y1,x2,y2,rgb)

#define TPS2DrawPoint(ps2,ctx,x,y,rgb) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,gs_rgbaq_packed *))(ps2))[-51]))(ps2,ctx,x,y,rgb)

#define TPS2DrawTRectUV(ps2,ctx,x0,y0,x1,y1,u0,v0,u1,v1,gsimage) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,TINT,TINT,TINT,TINT,GSimage *))(ps2))[-52]))(ps2,ctx,x0,y0,x1,y1,u0,v0,u1,v1,gsimage)

#define TPS2DrawTPointUV(ps2,ctx,x,y,u,v,gsimage) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,GSimage *))(ps2))[-53]))(ps2,ctx,x,y,u,v,gsimage)

#define TPS2GSSyncPath(ps2,mode) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(ps2))[-54]))(ps2,mode)

#define TPS2GSGetImage(ps2,img) \
	(*(((TMODCALL TVOID(**)(TAPTR,GSimage *))(ps2))[-55]))(ps2,img)

#define TPS2GSSetActiveFb(ps2,ctx,fbp) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT))(ps2))[-56]))(ps2,ctx,fbp)

#endif /* _TEK_ANSICALL_PS2_H */
