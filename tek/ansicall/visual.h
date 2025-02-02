#ifndef _TEK_ANSICALL_VISUAL_H
#define _TEK_ANSICALL_VISUAL_H

/*
**	$Id: visual.h,v 1.3 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/visual.h - visual module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TVisualGetPort(visual) \
	(*(((TMODCALL TAPTR(**)(TAPTR))(visual))[-9]))(visual)

#define TVisualAllocPen(visual,rgb) \
	(*(((TMODCALL TVPEN(**)(TAPTR,TUINT))(visual))[-10]))(visual,rgb)

#define TVisualFRect(visual,x,y,w,h,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TVPEN))(visual))[-11]))(visual,x,y,w,h,pen)

#define TVisualClear(visual,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,TVPEN))(visual))[-12]))(visual,pen)

#define TVisualDrawRGB(visual,x,y,buf,w,h,totw) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TUINT *,TINT,TINT,TINT))(visual))[-13]))(visual,x,y,buf,w,h,totw)

#define TVisualFlush(visual) \
	(*(((TMODCALL TVOID(**)(TAPTR))(visual))[-14]))(visual)

#define TVisualFreePen(visual,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,TVPEN))(visual))[-15]))(visual,pen)

#define TVisualRect(visual,x,y,w,h,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TVPEN))(visual))[-16]))(visual,x,y,w,h,pen)

#define TVisualLine(visual,x1,y1,x2,y2,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TVPEN))(visual))[-17]))(visual,x1,y1,x2,y2,pen)

#define TVisualLineArray(visual,array,num,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT16 *,TINT,TVPEN))(visual))[-18]))(visual,array,num,pen)

#define TVisualPlot(visual,x,y,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TVPEN))(visual))[-19]))(visual,x,y,pen)

#define TVisualScroll(visual,x,y,w,h,dx,dy) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,TINT))(visual))[-20]))(visual,x,y,w,h,dx,dy)

#define TVisualText(visual,x,y,text,len,bg,fg) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TSTRPTR,TINT,TVPEN,TVPEN))(visual))[-21]))(visual,x,y,text,len,bg,fg)

#define TVisualFlushArea(visual,x,y,w,h) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(visual))[-22]))(visual,x,y,w,h)

#define TVisualSetInput(visual,clearflags,setflags) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT))(visual))[-23]))(visual,clearflags,setflags)

#define TVisualGetAttrs(visual,tags) \
	(*(((TMODCALL TUINT(**)(TAPTR,TTAGITEM *))(visual))[-24]))(visual,tags)

#define TVisualAttach(visual,tags) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TTAGITEM *))(visual))[-25]))(visual,tags)

#define TVisualFPoly(visual,array,num,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT16 *,TINT,TVPEN))(visual))[-26]))(visual,array,num,pen)

#endif /* _TEK_ANSICALL_VISUAL_H */
