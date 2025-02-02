#ifndef _TEK_ANSICALL_DISPLAYHANDLER_H
#define _TEK_ANSICALL_DISPLAYHANDLER_H

/*
**	$Id: displayhandler.h,v 1.2 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/displayhandler.h - displayhandler module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TDisFindDisplay(disp,tags) \
	(*(((TMODCALL THNDL *(**)(TAPTR,TTAGITEM *))(disp))[-1]))(disp,tags)

#define TDisGetDisplayProperties(disp,dishost,props) \
	(*(((TMODCALL TVOID(**)(TAPTR,THNDL *,TDISPROPS *))(disp))[-2]))(disp,dishost,props)

#define TDisGetModeList(disp,a,b) \
	(*(((TMODCALL TINT(**)(TAPTR,THNDL *,TDISMODE **))(disp))[-3]))(disp,a,b)

#define TDisGetBestMode(disp,dishost,w,h,d) \
	(*(((TMODCALL TDISMODE *(**)(TAPTR,THNDL *,TINT,TINT,TINT))(disp))[-4]))(disp,dishost,w,h,d)

#define TDisCreateView(disp,dishost,tags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,THNDL *,TTAGITEM *))(disp))[-5]))(disp,dishost,tags)

#define TDisSimpleCreateView(disp,title,w,h,d,gl,fulls,dbl,resize) \
	(*(((TMODCALL THNDL *(**)(TAPTR,TSTRPTR,TINT,TINT,TINT,TBOOL,TBOOL,TBOOL,TBOOL))(disp))[-6]))(disp,title,w,h,d,gl,fulls,dbl,resize)

#define TDisGetDisplayCaps(disp,a,b) \
	(*(((TMODCALL TVOID(**)(TAPTR,THNDL *,TDISCAPS *))(disp))[-7]))(disp,a,b)

#define TDisWaitMsg(disp,dishost,tags) \
	(*(((TMODCALL TVOID(**)(TAPTR,THNDL *,TTAGITEM *))(disp))[-20]))(disp,dishost,tags)

#define TDisGetMsg(disp,dishost,msg) \
	(*(((TMODCALL TBOOL(**)(TAPTR,THNDL *,TDISMSG *))(disp))[-21]))(disp,dishost,msg)

#define TDisSetAttrs(disp,dishost,tags) \
	(*(((TMODCALL TVOID(**)(TAPTR,THNDL *,TTAGITEM *))(disp))[-22]))(disp,dishost,tags)

#define TDisFlush(disp,dishost) \
	(*(((TMODCALL TVOID(**)(TAPTR,THNDL *))(disp))[-24]))(disp,dishost)

#define TDisAllocPen(disp,dishost,color) \
	(*(((TMODCALL THNDL *(**)(TAPTR,THNDL *,TUINT))(disp))[-25]))(disp,dishost,color)

#define TDisSetDPen(disp,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,THNDL *))(disp))[-26]))(disp,pen)

#define TDisSetPalette(disp,dishost,pal,sp,sd,numentries) \
	(*(((TMODCALL TVOID(**)(TAPTR,THNDL *,TIMGARGBCOLOR *,TINT,TINT,TINT))(disp))[-30]))(disp,dishost,pal,sp,sd,numentries)

#define TDisAllocBitmap(disp,dishost,w,h,flags) \
	(*(((TMODCALL THNDL *(**)(TAPTR,THNDL *,TINT,TINT,TINT))(disp))[-31]))(disp,dishost,w,h,flags)

#define TDisDescribe(disp,handle,desc) \
	(*(((TMODCALL TBOOL(**)(TAPTR,THNDL *,TDISDESCRIPTOR *))(disp))[-40]))(disp,handle,desc)

#define TDisLock(disp,handle,img) \
	(*(((TMODCALL TBOOL(**)(TAPTR,THNDL *,TIMGPICTURE *))(disp))[-41]))(disp,handle,img)

#define TDisUnlock(disp) \
	(*(((TMODCALL TVOID(**)(TAPTR))(disp))[-42]))(disp)

#define TDisBegin(disp,handle) \
	(*(((TMODCALL TBOOL(**)(TAPTR,THNDL *))(disp))[-43]))(disp,handle)

#define TDisEnd(disp) \
	(*(((TMODCALL TVOID(**)(TAPTR))(disp))[-44]))(disp)

#define TDisBlit(disp,bmhndl,tags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,THNDL *,TTAGITEM *))(disp))[-50]))(disp,bmhndl,tags)

#define TDisTextout(disp,text,row,column) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT8 *,TINT,TINT))(disp))[-51]))(disp,text,row,column)

#define TDisPutImage(disp,img,tags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TIMGPICTURE *,TTAGITEM *))(disp))[-59]))(disp,img,tags)

#define TDisFill(disp) \
	(*(((TMODCALL TVOID(**)(TAPTR))(disp))[-61]))(disp)

#define TDisPlot(disp,x,y) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT))(disp))[-62]))(disp,x,y)

#define TDisLine(disp,sx,sy,ex,ey) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(disp))[-63]))(disp,sx,sy,ex,ey)

#define TDisBox(disp,sx,sy,w,h) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(disp))[-64]))(disp,sx,sy,w,h)

#define TDisBoxf(disp,sx,sy,w,h) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(disp))[-65]))(disp,sx,sy,w,h)

#define TDisPoly(disp,num,points) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT *))(disp))[-66]))(disp,num,points)

#define TDisPolyf(disp,num,points) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT *))(disp))[-67]))(disp,num,points)

#define TDisEllipse(disp,x,y,rx,ry) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(disp))[-68]))(disp,x,y,rx,ry)

#define TDisEllipsef(disp,x,y,rx,ry) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(disp))[-69]))(disp,x,y,rx,ry)

#define TDisMovePixels(disp,sx,sy,dx,dy,w,h) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,TINT))(disp))[-70]))(disp,sx,sy,dx,dy,w,h)

#endif /* _TEK_ANSICALL_DISPLAYHANDLER_H */
