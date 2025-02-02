#ifndef _TEK_ANSICALL_DISPLAY_H
#define _TEK_ANSICALL_DISPLAY_H

/*
**	$Id: display.h,v 1.2 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/display.h - display module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define dismod_create(dsph,title,x,y,w,h,d,flags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TINT,TINT,TINT,TINT,TINT,TINT))(dsph))[-1]))(dsph,title,x,y,w,h,d,flags)

#define dismod_destroy(dsph) \
	(*(((TMODCALL TVOID(**)(TAPTR))(dsph))[-2]))(dsph)

#define dismod_getproperties(dsph,props) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISPROPS *))(dsph))[-3]))(dsph,props)

#define dismod_getmodelist(dsph,modelist) \
	(*(((TMODCALL TINT(**)(TAPTR,TDISMODE **))(dsph))[-4]))(dsph,modelist)

#define dismod_waitmsg(dsph) \
	(*(((TMODCALL TVOID(**)(TAPTR))(dsph))[-5]))(dsph)

#define dismod_getmsg(dsph,dismsg) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TDISMSG *))(dsph))[-6]))(dsph,dismsg)

#define dismod_setattrs(dsph,tags) \
	(*(((TMODCALL TVOID(**)(TAPTR,TTAGITEM *))(dsph))[-7]))(dsph,tags)

#define dismod_flush(dsph) \
	(*(((TMODCALL TVOID(**)(TAPTR))(dsph))[-9]))(dsph)

#define dismod_getcaps(dsph,caps) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISCAPS *))(dsph))[-10]))(dsph,caps)

#define dismod_allocpen(dsph,pen) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TDISPEN *))(dsph))[-20]))(dsph,pen)

#define dismod_freepen(dsph,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISPEN *))(dsph))[-21]))(dsph,pen)

#define dismod_setdpen(dsph,pen) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISPEN *))(dsph))[-22]))(dsph,pen)

#define dismod_setpalette(dsph,pal,sp,sd,numentries) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TIMGARGBCOLOR *,TINT,TINT,TINT))(dsph))[-23]))(dsph,pal,sp,sd,numentries)

#define dismod_allocbitmap(dsph,bitmap,w,h,flags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TDISBITMAP *,TINT,TINT,TINT))(dsph))[-24]))(dsph,bitmap,w,h,flags)

#define dismod_freebitmap(dsph,bitmap) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISBITMAP *))(dsph))[-25]))(dsph,bitmap)

#define dismod_describe_dis(dsph,desc) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISDESCRIPTOR *))(dsph))[-30]))(dsph,desc)

#define dismod_describe_bm(dsph,bm,desc) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISBITMAP *,TDISDESCRIPTOR *))(dsph))[-31]))(dsph,bm,desc)

#define dismod_lock_dis(dsph,img) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TIMGPICTURE *))(dsph))[-32]))(dsph,img)

#define dismod_lock_bm(dsph,bm,img) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TDISBITMAP *,TIMGPICTURE *))(dsph))[-33]))(dsph,bm,img)

#define dismod_unlock_dis(dsph) \
	(*(((TMODCALL TVOID(**)(TAPTR))(dsph))[-34]))(dsph)

#define dismod_unlock_bm(dsph,bm) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISBITMAP *))(dsph))[-35]))(dsph,bm)

#define dismod_begin_dis(dsph) \
	(*(((TMODCALL TBOOL(**)(TAPTR))(dsph))[-36]))(dsph)

#define dismod_begin_bm(dsph,bm) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TDISBITMAP *))(dsph))[-37]))(dsph,bm)

#define dismod_end_dis(dsph) \
	(*(((TMODCALL TVOID(**)(TAPTR))(dsph))[-38]))(dsph)

#define dismod_end_bm(dsph,bm) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISBITMAP *))(dsph))[-39]))(dsph,bm)

#define dismod_blit(dsph,bm,bops) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISBITMAP *,TDBLITOPS *))(dsph))[-40]))(dsph,bm,bops)

#define dismod_textout_dis(dsph,txt,row,column) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT8 *,TINT,TINT))(dsph))[-50]))(dsph,txt,row,column)

#define dismod_textout_bm(dsph,txt,row,column) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT8 *,TINT,TINT))(dsph))[-51]))(dsph,txt,row,column)

#define dismod_putimage_dis(dsph,img,src,dst) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TDISRECT *,TDISRECT *))(dsph))[-61]))(dsph,img,src,dst)

#define dismod_putimage_bm(dsph,bm,img,src,dst) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISBITMAP *,TIMGPICTURE *,TDISRECT *,TDISRECT *))(dsph))[-62]))(dsph,bm,img,src,dst)

#define dismod_putscaleimage_dis(dsph,img,src,dst) \
	(*(((TMODCALL TVOID(**)(TAPTR,TIMGPICTURE *,TDISRECT *,TDISRECT *))(dsph))[-63]))(dsph,img,src,dst)

#define dismod_putscaleimage_bm(dsph,bm,img,src,dst) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDISBITMAP *,TIMGPICTURE *,TDISRECT *,TDISRECT *))(dsph))[-64]))(dsph,bm,img,src,dst)

#define dismod_fill_dis(dsph) \
	(*(((TMODCALL TVOID(**)(TAPTR))(dsph))[-70]))(dsph)

#define dismod_fill_bm(dsph) \
	(*(((TMODCALL TVOID(**)(TAPTR))(dsph))[-71]))(dsph)

#define dismod_plot_dis(dsph,x,y) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT))(dsph))[-72]))(dsph,x,y)

#define dismod_plot_bm(dsph,x,y) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT))(dsph))[-73]))(dsph,x,y)

#define dismod_line_dis(dsph,sx,sy,dx,dy) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(dsph))[-74]))(dsph,sx,sy,dx,dy)

#define dismod_line_bm(dsph,sx,sy,dx,dy) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(dsph))[-75]))(dsph,sx,sy,dx,dy)

#define dismod_box_dis(dsph,sx,sy,w,h) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(dsph))[-76]))(dsph,sx,sy,w,h)

#define dismod_box_bm(dsph,sx,sy,w,h) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(dsph))[-77]))(dsph,sx,sy,w,h)

#define dismod_boxf_dis(dsph,sx,sy,w,h) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(dsph))[-78]))(dsph,sx,sy,w,h)

#define dismod_boxf_bm(dsph,sx,sy,w,h) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(dsph))[-79]))(dsph,sx,sy,w,h)

#define dismod_poly_dis(dsph,num,points) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT *))(dsph))[-80]))(dsph,num,points)

#define dismod_poly_bm(dsph,num,points) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT *))(dsph))[-81]))(dsph,num,points)

#define dismod_polyf_dis(dsph,num,points) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT *))(dsph))[-82]))(dsph,num,points)

#define dismod_polyf_bm(dsph,num,points) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT *))(dsph))[-83]))(dsph,num,points)

#define dismod_ellipse_dis(dsph,x,y,rx,ry) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(dsph))[-84]))(dsph,x,y,rx,ry)

#define dismod_ellipse_bm(dsph,x,y,rx,ry) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(dsph))[-85]))(dsph,x,y,rx,ry)

#define dismod_ellipsef_dis(dsph,x,y,rx,ry) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(dsph))[-86]))(dsph,x,y,rx,ry)

#define dismod_ellipsef_bm(dsph,x,y,rx,ry) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT))(dsph))[-87]))(dsph,x,y,rx,ry)

#define dismod_movepixels_dis(dsph,sx,sy,dx,dy,w,h) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,TINT))(dsph))[-88]))(dsph,sx,sy,dx,dy,w,h)

#define dismod_movepixels_bm(dsph,sx,sy,dx,dy,w,h) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,TINT))(dsph))[-89]))(dsph,sx,sy,dx,dy,w,h)

#endif /* _TEK_ANSICALL_DISPLAY_H */
