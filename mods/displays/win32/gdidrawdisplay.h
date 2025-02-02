/**************************************************************************
	dismod_textout_dis
 **************************************************************************/
TMODAPI TVOID dismod_textout_dis(TMOD_DISMOD *dismod, TINT8 *text, TINT row, TINT column)
{
	SetBkMode(dismod->dismodHDC, TRANSPARENT);
	SetTextColor(dismod->dismodHDC, RGB(dismod->theDrawPen->r,dismod->theDrawPen->g,dismod->theDrawPen->b));
	TextOut(dismod->dismodHDC, column*dismod->font_w, row*dismod->font_h, (LPCTSTR)text, TUtilStrLen(dismod->util,text));
}

/**************************************************************************
	dismod_fill_dis
 **************************************************************************/
TMODAPI TVOID dismod_fill_dis(TMOD_DISMOD *dismod)
{
	RECT r;

	r.top=0;
	r.left=0;
	r.right=dismod->width;
	r.bottom=dismod->height;

	FillRect(dismod->dismodHDC,&r,dismod->theFillBrushWin32);
}

/**************************************************************************
	dismod_plot_dis
 **************************************************************************/
TMODAPI TVOID dismod_plot_dis(TMOD_DISMOD *dismod,TINT x, TINT y)
{
	SetPixel(dismod->dismodHDC,x,y,RGB(dismod->theDrawPen->r,dismod->theDrawPen->g,dismod->theDrawPen->b));
}

/**************************************************************************
	dismod_line_dis
 **************************************************************************/
TMODAPI TVOID dismod_line_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT dx, TINT dy)
{
	MoveToEx(dismod->dismodHDC,sx,sy,TNULL);
	LineTo(dismod->dismodHDC,dx,dy);
}

/**************************************************************************
	dismod_box_dis
 **************************************************************************/
TMODAPI TVOID dismod_box_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT w, TINT h)
{
	SelectObject(dismod->dismodHDC,dismod->theEmptyBrushWin32);
	Rectangle(dismod->dismodHDC,sx,sy,sx+w,sy+h);
}

/**************************************************************************
	dismod_boxf_dis
 **************************************************************************/
TMODAPI TVOID dismod_boxf_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT w, TINT h)
{
	SelectObject(dismod->dismodHDC,dismod->theFillBrushWin32);
	Rectangle(dismod->dismodHDC,sx,sy,sx+w,sy+h);
}

/**************************************************************************
	dismod_poly_dis
 **************************************************************************/
TMODAPI TVOID dismod_poly_dis(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
	SelectObject(dismod->dismodHDC,dismod->theEmptyBrushWin32);
	Polygon(dismod->dismodHDC,(POINT*)points,numpoints);
}

/**************************************************************************
	dismod_polyf_dis
 **************************************************************************/
TMODAPI TVOID dismod_polyf_dis(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
	SelectObject(dismod->dismodHDC,dismod->theFillBrushWin32);
	Polygon(dismod->dismodHDC,(POINT*)points,numpoints);
}

/**************************************************************************
	dismod_ellipse_dis
 **************************************************************************/
TMODAPI TVOID dismod_ellipse_dis(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
	SelectObject(dismod->dismodHDC,dismod->theEmptyBrushWin32);
	Ellipse(dismod->dismodHDC,x-rx,y-ry,x+rx,y+ry);
}

/**************************************************************************
	dismod_ellipsef_dis
 **************************************************************************/
TMODAPI TVOID dismod_ellipsef_dis(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
	SelectObject(dismod->dismodHDC,dismod->theFillBrushWin32);
	Ellipse(dismod->dismodHDC,x-rx,y-ry,x+rx,y+ry);
}

/**************************************************************************
	dismod_movepixels_dis
 **************************************************************************/
TMODAPI TVOID dismod_movepixels_dis(TMOD_DISMOD *dismod,TINT sx,TINT sy, TINT dx, TINT dy, TINT w, TINT h)
{
	RECT sr,cr;
	HRGN rgn;

	rgn=CreateRectRgn(dx,dy,w,h);

	sr.left=sx;
	sr.top=sy;
	sr.right=sx+w;
	sr.bottom=sy+h;

	cr.left=0;
	cr.top=0;
	cr.right=dismod->width;
	cr.bottom=dismod->height;

	ScrollDC(dismod->dismodHDC,dx-sx,dy-sy,&sr,&cr,rgn,NULL);
	DeleteObject(rgn);
}

