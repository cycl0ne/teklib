/**************************************************************************
	dismod_textout_dis
 **************************************************************************/
TMODAPI TVOID dismod_textout_dis(TMOD_DISMOD *dismod, TINT8 *text, TINT row, TINT column)
{
	XSetFont(dismod->theXDisplay,dismod->theXPixmapGC[dismod->workbuf],dismod->theXFont->fid);
	if(dismod->window_ready)
	{
		XDrawString(dismod->theXDisplay,
					dismod->theXPixmap[dismod->workbuf],
					dismod->theXPixmapGC[dismod->workbuf],
					column*dismod->font_w,(row+1)*dismod->font_h,
					text,
					TUtilStrLen(dismod->util,text));
	}
}

/**************************************************************************
	dismod_fill_dis
 **************************************************************************/
TMODAPI TVOID dismod_fill_dis(TMOD_DISMOD *dismod)
{
	XFillRectangle( dismod->theXDisplay,
					dismod->theXPixmap[dismod->workbuf],
					dismod->theXPixmapGC[dismod->workbuf],
					0,0,
					dismod->width,dismod->height);
}

/**************************************************************************
	dismod_plot_dis
 **************************************************************************/
TMODAPI TVOID dismod_plot_dis(TMOD_DISMOD *dismod,TINT x, TINT y)
{
	XDrawPoint( dismod->theXDisplay,
				dismod->theXPixmap[dismod->workbuf],
				dismod->theXPixmapGC[dismod->workbuf],
				x,y);
}

/**************************************************************************
	dismod_line_dis
 **************************************************************************/
TMODAPI TVOID dismod_line_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT dx, TINT dy)
{
	XDrawLine(  dismod->theXDisplay,
				dismod->theXPixmap[dismod->workbuf],
				dismod->theXPixmapGC[dismod->workbuf],
				sx,sy,
				dx,dy);
}

/**************************************************************************
	dismod_box_dis
 **************************************************************************/
TMODAPI TVOID dismod_box_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT w, TINT h)
{
	XDrawRectangle( dismod->theXDisplay,
					dismod->theXPixmap[dismod->workbuf],
					dismod->theXPixmapGC[dismod->workbuf],
					sx,sy,
					w,h);
}

/**************************************************************************
	dismod_boxf_dis
 **************************************************************************/
TMODAPI TVOID dismod_boxf_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT w, TINT h)
{
	XFillRectangle( dismod->theXDisplay,
					dismod->theXPixmap[dismod->workbuf],
					dismod->theXPixmapGC[dismod->workbuf],
					sx,sy,
					w,h);
}

/**************************************************************************
	dismod_poly_dis
 **************************************************************************/
TMODAPI TVOID dismod_poly_dis(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
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
				dismod->theXPixmap[dismod->workbuf],
				dismod->theXPixmapGC[dismod->workbuf],
				dismod->polybuf,
				numpoints+1,
				CoordModeOrigin);
}

/**************************************************************************
	dismod_polyf_dis
 **************************************************************************/
TMODAPI TVOID dismod_polyf_dis(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
	TINT i;

	for(i=0;i<numpoints;i++)
	{
		dismod->polybuf[i].x = points[i*2];
		dismod->polybuf[i].y = points[i*2+1];
	}

	XFillPolygon(dismod->theXDisplay,
				dismod->theXPixmap[dismod->workbuf],
				dismod->theXPixmapGC[dismod->workbuf],
				dismod->polybuf,
				numpoints,
				Complex,
				CoordModeOrigin);
}

/**************************************************************************
	dismod_ellipse_dis
 **************************************************************************/
TMODAPI TVOID dismod_ellipse_dis(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
	XDrawArc(dismod->theXDisplay,
			dismod->theXPixmap[dismod->workbuf],
			dismod->theXPixmapGC[dismod->workbuf],
			x-rx,y-ry,
			rx*2,ry*2,
			0,360*64);
}

/**************************************************************************
	dismod_ellipsef_dis
 **************************************************************************/
TMODAPI TVOID dismod_ellipsef_dis(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
	XFillArc(dismod->theXDisplay,
			dismod->theXPixmap[dismod->workbuf],
			dismod->theXPixmapGC[dismod->workbuf],
			x-rx,y-ry,
			rx*2,ry*2,
			0,360*64);
}

/**************************************************************************
	dismod_movepixels_dis
 **************************************************************************/
TMODAPI TVOID dismod_movepixels_dis(TMOD_DISMOD *dismod,TINT sx,TINT sy, TINT dx, TINT dy, TINT w, TINT h)
{
	XCopyArea(	dismod->theXDisplay,
				dismod->theXPixmap[dismod->workbuf],
				dismod->theXPixmap[dismod->workbuf],
				dismod->theXPixmapGC[dismod->workbuf],
				sx,sy,
				w,h,
				dx,dy);
}

