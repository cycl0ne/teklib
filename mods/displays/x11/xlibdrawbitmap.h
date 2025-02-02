/**************************************************************************
	dismod_textout_bm
 **************************************************************************/
TMODAPI TVOID dismod_textout_bm(TMOD_DISMOD *dismod, TINT8 *text, TINT row, TINT column)
{
	XSetFont(dismod->theXDisplay,dismod->theBitmap->theXPixmapGC,dismod->theXFont->fid);
	if(dismod->window_ready)
	{
		XDrawString(dismod->theXDisplay,
					dismod->theBitmap->theXPixmap,
					dismod->theBitmap->theXPixmapGC,
					column*dismod->font_w,(row+1)*dismod->font_h,
					text,
					TUtilStrLen(dismod->util,text));
	}
}


/**************************************************************************
	dismod_fill_bm
 **************************************************************************/
TMODAPI TVOID dismod_fill_bm(TMOD_DISMOD *dismod)
{
	XFillRectangle( dismod->theXDisplay,
					dismod->theBitmap->theXPixmap,
					dismod->theBitmap->theXPixmapGC,
					0,0,
					dismod->bmwidth,dismod->bmheight);
}

/**************************************************************************
	dismod_plot_bm
 **************************************************************************/
TMODAPI TVOID dismod_plot_bm(TMOD_DISMOD *dismod,TINT x, TINT y)
{
	XDrawPoint( dismod->theXDisplay,
				dismod->theBitmap->theXPixmap,
				dismod->theBitmap->theXPixmapGC,
				x,y);
}

/**************************************************************************
	dismod_line_bm
 **************************************************************************/
TMODAPI TVOID dismod_line_bm(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT dx, TINT dy)
{
	XDrawLine(  dismod->theXDisplay,
				dismod->theBitmap->theXPixmap,
				dismod->theBitmap->theXPixmapGC,
				sx,sy,
				dx,dy);
}

/**************************************************************************
	dismod_box_bm
 **************************************************************************/
TMODAPI TVOID dismod_box_bm(TMOD_DISMOD *dismod, TINT sx, TINT sy, TINT w, TINT h)
{
	XDrawRectangle( dismod->theXDisplay,
					dismod->theBitmap->theXPixmap,
					dismod->theBitmap->theXPixmapGC,
					sx,sy,
					w,h);
}

/**************************************************************************
	dismod_boxf_bm
 **************************************************************************/
TMODAPI TVOID dismod_boxf_bm(TMOD_DISMOD *dismod, TINT sx, TINT sy, TINT w, TINT h)
{
	XFillRectangle( dismod->theXDisplay,
					dismod->theBitmap->theXPixmap,
					dismod->theBitmap->theXPixmapGC,
					sx,sy,
					w,h);
}

/**************************************************************************
	dismod_poly_bm
 **************************************************************************/
TMODAPI TVOID dismod_poly_bm(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
	TINT i;

	for(i=0;i<numpoints;i++)
	{
		dismod->polybuf[i].x = points[i*2];
		dismod->polybuf[i].y = points[i*2+1];
	}
	dismod->polybuf[numpoints].x = points[0];
	dismod->polybuf[numpoints].y = points[1];

	XDrawLines( dismod->theXDisplay,
				dismod->theBitmap->theXPixmap,
				dismod->theBitmap->theXPixmapGC,
				dismod->polybuf,
				numpoints+1,
				CoordModeOrigin);
}

/**************************************************************************
	dismod_polyf_bm
 **************************************************************************/
TMODAPI TVOID dismod_polyf_bm(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
	TINT i;

	for(i=0;i<numpoints;i++)
	{
		dismod->polybuf[i].x = points[i*2];
		dismod->polybuf[i].y = points[i*2+1];
	}

	XFillPolygon(dismod->theXDisplay,
				dismod->theBitmap->theXPixmap,
				dismod->theBitmap->theXPixmapGC,
				dismod->polybuf,
				numpoints,
				Complex,
				CoordModeOrigin);
}

/**************************************************************************
	dismod_ellipse_bm
 **************************************************************************/
TMODAPI TVOID dismod_ellipse_bm(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
	XDrawArc(dismod->theXDisplay,
			dismod->theBitmap->theXPixmap,
			dismod->theBitmap->theXPixmapGC,
			x-rx,y-ry,
			rx*2,ry*2,
			0,360*64);
}

/**************************************************************************
	dismod_ellipsef_bm
 **************************************************************************/
TMODAPI TVOID dismod_ellipsef_bm(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
	XFillArc(dismod->theXDisplay,
			dismod->theBitmap->theXPixmap,
			dismod->theBitmap->theXPixmapGC,
			x-rx,y-ry,
			rx*2,ry*2,
			0,360*64);
}

/**************************************************************************
	dismod_movepixels_bm
 **************************************************************************/
TMODAPI TVOID dismod_movepixels_bm(TMOD_DISMOD *dismod,TINT sx,TINT sy, TINT dx, TINT dy, TINT w, TINT h)
{
	XCopyArea(	dismod->theXDisplay,
				dismod->theBitmap->theXPixmap,
				dismod->theBitmap->theXPixmap,
				dismod->theBitmap->theXPixmapGC,
				sx,sy,
				w,h,
				dx,dy);
}
