/**************************************************************************
	dismod_textout_bm
 **************************************************************************/
TMODAPI TVOID dismod_textout_bm(TMOD_DISMOD *dismod, TINT8 *text, TINT row, TINT column)
{
	SetBkMode(dismod->bitmapHDC, TRANSPARENT);
	SetTextColor(dismod->bitmapHDC, RGB(dismod->theDrawPen->r,dismod->theDrawPen->g,dismod->theDrawPen->b));
	TextOut(dismod->bitmapHDC, column*dismod->font_w, row*dismod->font_h, (LPCTSTR)text, TUtilStrLen(dismod->util,text));
}

/**************************************************************************
	dismod_fill_bm
 **************************************************************************/
TMODAPI TVOID dismod_fill_bm(TMOD_DISMOD *dismod)
{
	RECT r;

	r.top=0;
	r.left=0;
	r.right=dismod->bmwidth;
	r.bottom=dismod->bmheight;

	FillRect(dismod->bitmapHDC,&r,dismod->theFillBrushWin32);
}

/**************************************************************************
	dismod_plot_bm
 **************************************************************************/
TMODAPI TVOID dismod_plot_bm(TMOD_DISMOD *dismod,TINT x, TINT y)
{
	SetPixel(dismod->bitmapHDC,x,y,RGB(dismod->theDrawPen->r,dismod->theDrawPen->g,dismod->theDrawPen->b));
}

/**************************************************************************
	dismod_line_bm
 **************************************************************************/
TMODAPI TVOID dismod_line_bm(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT dx, TINT dy)
{
	MoveToEx(dismod->bitmapHDC,sx,sy,TNULL);
	LineTo(dismod->bitmapHDC,dx,dy);
}

/**************************************************************************
	dismod_box_bm
 **************************************************************************/
TMODAPI TVOID dismod_box_bm(TMOD_DISMOD *dismod, TINT sx, TINT sy, TINT w, TINT h)
{
	SelectObject(dismod->bitmapHDC,dismod->theEmptyBrushWin32);
	Rectangle(dismod->bitmapHDC,sx,sy,sx+w,sy+h);
}

/**************************************************************************
	dismod_boxf_bm
 **************************************************************************/
TMODAPI TVOID dismod_boxf_bm(TMOD_DISMOD *dismod, TINT sx, TINT sy, TINT w, TINT h)
{
	SelectObject(dismod->bitmapHDC,dismod->theFillBrushWin32);
	Rectangle(dismod->bitmapHDC,sx,sy,sx+w,sy+h);
}

/**************************************************************************
	dismod_poly_bm
 **************************************************************************/
TMODAPI TVOID dismod_poly_bm(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
	SelectObject(dismod->bitmapHDC,dismod->theEmptyBrushWin32);
	Polygon(dismod->bitmapHDC,(POINT*)points,numpoints);
}

/**************************************************************************
	dismod_polyf_bm
 **************************************************************************/
TMODAPI TVOID dismod_polyf_bm(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
	SelectObject(dismod->bitmapHDC,dismod->theFillBrushWin32);
	Polygon(dismod->bitmapHDC,(POINT*)points,numpoints);
}

/**************************************************************************
	dismod_ellipse_bm
 **************************************************************************/
TMODAPI TVOID dismod_ellipse_bm(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
	SelectObject(dismod->bitmapHDC,dismod->theEmptyBrushWin32);
	Ellipse(dismod->bitmapHDC,x-rx,y-ry,x+rx,y+ry);
}

/**************************************************************************
	dismod_ellipsef_bm
 **************************************************************************/
TMODAPI TVOID dismod_ellipsef_bm(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
	SelectObject(dismod->bitmapHDC,dismod->theFillBrushWin32);
	Ellipse(dismod->bitmapHDC,x-rx,y-ry,x+rx,y+ry);
}

/**************************************************************************
	dismod_movepixels_bm
 **************************************************************************/
TMODAPI TVOID dismod_movepixels_bm(TMOD_DISMOD *dismod,TINT sx,TINT sy, TINT dx, TINT dy, TINT w, TINT h)
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

	ScrollDC(dismod->bitmapHDC,dx-sx,dy-sy,&sr,&cr,rgn,NULL);
	DeleteObject(rgn);
}
