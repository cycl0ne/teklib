/**************************************************************************
	dismod_textout_dis
 **************************************************************************/
TMODAPI TVOID dismod_textout_dis(TMOD_DISMOD *dismod, TINT8 *text, TINT row, TINT column)
{
    BYTE drawmode=dismod->theDrawRPort->DrawMode;
    SetDrMd(dismod->theDrawRPort,JAM1);
    Move(dismod->theDrawRPort,column*dismod->font_w,row*dismod->font_h+dismod->font_h);
    Text(dismod->theDrawRPort,text,TUtilStrLen(dismod->util,text));
    SetDrMd(dismod->theDrawRPort,drawmode);
}

/**************************************************************************
	dismod_fill_dis
 **************************************************************************/
TMODAPI TVOID dismod_fill_dis(TMOD_DISMOD *dismod)
{
    RectFill(dismod->theDrawRPort,0,0,dismod->width-1,dismod->height-1);
}

/**************************************************************************
	dismod_plot_dis
 **************************************************************************/
TMODAPI TVOID dismod_plot_dis(TMOD_DISMOD *dismod,TINT x, TINT y)
{
    WritePixel(dismod->theDrawRPort,x,y);
}

/**************************************************************************
	dismod_line_dis
 **************************************************************************/
TMODAPI TVOID dismod_line_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT dx, TINT dy)
{
    Move(dismod->theDrawRPort,sx,sy);
    Draw(dismod->theDrawRPort,dx,dy);
}

/**************************************************************************
	dismod_box_dis
 **************************************************************************/
TMODAPI TVOID dismod_box_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT w, TINT h)
{
    Move(dismod->theDrawRPort,sx,sy);
    Draw(dismod->theDrawRPort,sx+w-1,sy);
    Draw(dismod->theDrawRPort,sx+w-1,sy+h-1);
    Draw(dismod->theDrawRPort,sx,sy+h-1);
    Draw(dismod->theDrawRPort,sx,sy);
}

/**************************************************************************
	dismod_boxf_dis
 **************************************************************************/
TMODAPI TVOID dismod_boxf_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT w, TINT h)
{
    RectFill(dismod->theDrawRPort,
	     sx,sy,
	     sx+w-1,sy+h-1);
}

/**************************************************************************
	dismod_poly_dis
 **************************************************************************/
TMODAPI TVOID dismod_poly_dis(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
    TINT i;

    Move(dismod->theDrawRPort,points[0],points[1]);
    for(i=1;i<numpoints;i++)
    {
	Draw(dismod->theDrawRPort,points[i*2],points[i*2+1]);
    }
    Draw(dismod->theDrawRPort,points[0],points[1]);
}

/**************************************************************************
	dismod_polyf_dis
 **************************************************************************/
TMODAPI TVOID dismod_polyf_dis(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
    TINT i;

    AreaMove(dismod->theDrawRPort,points[0],points[1]);
    for(i=1;i<numpoints;i++)
    {
	AreaDraw(dismod->theDrawRPort,points[i*2],points[i*2+1]);
    }
    AreaDraw(dismod->theDrawRPort,points[0],points[1]);
    AreaEnd(dismod->theDrawRPort);
}

/**************************************************************************
	dismod_ellipse_dis
 **************************************************************************/
TMODAPI TVOID dismod_ellipse_dis(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
    DrawEllipse(dismod->theDrawRPort,
		x,y,
		rx,ry);
}

/**************************************************************************
	dismod_ellipsef_dis
 **************************************************************************/
TMODAPI TVOID dismod_ellipsef_dis(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
    AreaEllipse(dismod->theDrawRPort,
		x,y,
		rx,ry);

    AreaEnd(dismod->theDrawRPort);
}

/**************************************************************************
	dismod_movepixels_dis
 **************************************************************************/
TMODAPI TVOID dismod_movepixels_dis(TMOD_DISMOD *dismod,TINT sx,TINT sy, TINT dx, TINT dy, TINT w, TINT h)
{
    BltBitMapRastPort(dismod->theDrawRPort->BitMap,
		      sx,sy,
		      dismod->theDrawRPort,
		      dx,dy,
		      w,h,
		      ABNC | ABC);
}

