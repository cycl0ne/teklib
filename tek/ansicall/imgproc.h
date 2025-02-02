#ifndef _TEK_ANSICALL_IMGPROC_H
#define _TEK_ANSICALL_IMGPROC_H

/*
**	$Id: imgproc.h,v 1.2 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/imgproc.h - imgproc module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TImgDoMethod(imgproc,src,dst,method,tags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TIMGPICTURE *,TIMGPICTURE *,TINT,TTAGITEM *))(imgproc))[-1]))(imgproc,src,dst,method,tags)

#define TImgBytesPerPel(imgproc,format) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(imgproc))[-2]))(imgproc,format)

#define TImgColToFmt(imgproc,col,format) \
	(*(((TMODCALL TUINT(**)(TAPTR,TIMGARGBCOLOR *,TINT))(imgproc))[-3]))(imgproc,col,format)

#define TImgFill(imgproc,pic,col) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TIMGARGBCOLOR *))(imgproc))[-4]))(imgproc,pic,col)

#define TImgPlot(imgproc,pic,x,y,col) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TINT,TINT,TIMGARGBCOLOR *))(imgproc))[-5]))(imgproc,pic,x,y,col)

#define TImgLine(imgproc,pic,sx,sy,ex,ey,col) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TINT,TINT,TINT,TINT,TIMGARGBCOLOR *))(imgproc))[-6]))(imgproc,pic,sx,sy,ex,ey,col)

#define TImgBox(imgproc,pic,sx,sy,w,h,col) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TINT,TINT,TINT,TINT,TIMGARGBCOLOR *))(imgproc))[-7]))(imgproc,pic,sx,sy,w,h,col)

#define TImgBoxf(imgproc,pic,sx,sy,w,h,col) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TINT,TINT,TINT,TINT,TIMGARGBCOLOR *))(imgproc))[-8]))(imgproc,pic,sx,sy,w,h,col)

#define TImgEllipse(imgproc,pic,x,y,rx,ry,col) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TINT,TINT,TINT,TINT,TIMGARGBCOLOR *))(imgproc))[-9]))(imgproc,pic,x,y,rx,ry,col)

#define TImgEllipsef(imgproc,pic,x,y,rx,ry,col) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TINT,TINT,TINT,TINT,TIMGARGBCOLOR *))(imgproc))[-10]))(imgproc,pic,x,y,rx,ry,col)

#define TImgPoly(imgproc,pic,numpoints,points,col) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TINT,TINT *,TIMGARGBCOLOR *))(imgproc))[-11]))(imgproc,pic,numpoints,points,col)

#define TImgPolyf(imgproc,pic,numpoints,points,col) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TINT,TINT *,TIMGARGBCOLOR *))(imgproc))[-12]))(imgproc,pic,numpoints,points,col)

#define TImgAllocBitmap(imgproc,w,h,format) \
	(*(((TMODCALL TIMGPICTURE *(**)(TAPTR,TINT,TINT,TINT))(imgproc))[-13]))(imgproc,w,h,format)

#define TImgFreeBitmap(imgproc,pic) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *))(imgproc))[-14]))(imgproc,pic)

#define TImgPoint(imgproc,pic,x,y,col) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TINT,TINT,TIMGARGBCOLOR *))(imgproc))[-15]))(imgproc,pic,x,y,col)

#endif /* _TEK_ANSICALL_IMGPROC_H */
