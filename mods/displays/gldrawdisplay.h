/* here are all routines needed for simple drawing
   to a OpenGL context
*/

/**************************************************************************
	dismod_textout_dis
 **************************************************************************/
TMODAPI TVOID dismod_textout_dis(TMOD_DISMOD *dismod, TINT8 *text, TINT row, TINT column)
{
	glListBase(dismod->fontlist);
	glColor3f(dismod->theGlDrawPen.r,dismod->theGlDrawPen.g,dismod->theGlDrawPen.b);
	glPushAttrib(GL_LIST_BIT);
	glRasterPos2d(column*dismod->font_w,(row+1)*dismod->font_h);
	glCallLists( TUtilStrLen(dismod->util,text), GL_UNSIGNED_BYTE,(GLubyte *)text);
	glPopAttrib();
}

/**************************************************************************
	dismod_fill_dis
 **************************************************************************/
TMODAPI TVOID dismod_fill_dis(TMOD_DISMOD *dismod)
{
	glClearColor(dismod->theGlDrawPen.r,dismod->theGlDrawPen.g,dismod->theGlDrawPen.b,0);
	glClear(GL_COLOR_BUFFER_BIT);
}

/**************************************************************************
	dismod_plot_dis
 **************************************************************************/
TMODAPI TVOID dismod_plot_dis(TMOD_DISMOD *dismod,TINT x, TINT y)
{
	glColor3f(dismod->theGlDrawPen.r,dismod->theGlDrawPen.g,dismod->theGlDrawPen.b);
	glBegin(GL_POINTS);
	glVertex2i(x,y);
	glEnd();
}

/**************************************************************************
	dismod_line_dis
 **************************************************************************/
TMODAPI TVOID dismod_line_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT dx, TINT dy)
{
	glColor3f(dismod->theGlDrawPen.r,dismod->theGlDrawPen.g,dismod->theGlDrawPen.b);
	glBegin(GL_LINES);
	glVertex2i(sx,sy);
	glVertex2i(dx,dy);
	glEnd();
}

/**************************************************************************
	dismod_box_dis
 **************************************************************************/
TMODAPI TVOID dismod_box_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT w, TINT h)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glColor3f(dismod->theGlDrawPen.r,dismod->theGlDrawPen.g,dismod->theGlDrawPen.b);
	glBegin(GL_QUADS);
	glVertex2i(sx,sy);
	glVertex2i(sx+w,sy);
	glVertex2i(sx+w,sy+h);
	glVertex2i(sx,sy+h);
	glEnd();
}

/**************************************************************************
	dismod_boxf_dis
 **************************************************************************/
TMODAPI TVOID dismod_boxf_dis(TMOD_DISMOD *dismod,TINT sx, TINT sy, TINT w, TINT h)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glColor3f(dismod->theGlDrawPen.r,dismod->theGlDrawPen.g,dismod->theGlDrawPen.b);
	glBegin(GL_QUADS);
	glVertex2i(sx,sy);
	glVertex2i(sx+w,sy);
	glVertex2i(sx+w,sy+h);
	glVertex2i(sx,sy+h);
	glEnd();
}

/**************************************************************************
	dismod_poly_dis
 **************************************************************************/
TMODAPI TVOID dismod_poly_dis(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glColor3f(dismod->theGlDrawPen.r,dismod->theGlDrawPen.g,dismod->theGlDrawPen.b);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2,GL_INT,0,points);
	glDrawArrays(GL_POLYGON,0,numpoints);
	glDisableClientState(GL_VERTEX_ARRAY);
}

/**************************************************************************
	dismod_polyf_dis
 **************************************************************************/
TMODAPI TVOID dismod_polyf_dis(TMOD_DISMOD *dismod,TINT numpoints,TINT *points)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glColor3f(dismod->theGlDrawPen.r,dismod->theGlDrawPen.g,dismod->theGlDrawPen.b);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2,GL_INT,0,points);
	glDrawArrays(GL_POLYGON,0,numpoints);
	glDisableClientState(GL_VERTEX_ARRAY);
}

/**************************************************************************
	dismod_ellipse_dis
 **************************************************************************/
TMODAPI TVOID dismod_ellipse_dis(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
	TINT ex,ey,oldx;
	TFLOAT r1q,r2q;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	oldx=rx;

	glColor3f(dismod->theGlDrawPen.r,dismod->theGlDrawPen.g,dismod->theGlDrawPen.b);
	glBegin(GL_LINES);

	for(ey=0;ey<=ry;ey++)
	{
		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));
		
		if(ex<oldx-1)
		{
			glEnd();
			glBegin(GL_LINES);
			glVertex2i(x-ex,  y-ey);
			glVertex2i(x-oldx,y-ey);
			glVertex2i(x+ex,  y-ey);
			glVertex2i(x+oldx,y-ey);
			glVertex2i(x-ex,  y+ey);
			glVertex2i(x-oldx,y+ey);
			glVertex2i(x+ex,  y+ey);
			glVertex2i(x+oldx,y+ey);
		}
		else
		{
			glEnd();
			glBegin(GL_POINTS);
			glVertex2i(x-ex,y-ey);
			glVertex2i(x+ex,y-ey);
			glVertex2i(x-ex,y+ey);
			glVertex2i(x+ex,y+ey);
		}
		oldx=ex;
	}
	glEnd();
}

/**************************************************************************
	dismod_ellipsef_dis
 **************************************************************************/
TMODAPI TVOID dismod_ellipsef_dis(TMOD_DISMOD *dismod,TINT x,TINT y, TINT rx, TINT ry)
{
	TINT ex,ey;
	TFLOAT r1q,r2q;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	glColor3f(dismod->theGlDrawPen.r,dismod->theGlDrawPen.g,dismod->theGlDrawPen.b);
	glBegin(GL_LINES);
	
	glVertex2i(x-rx,y);
	glVertex2i(x+rx,y);

	for(ey=1;ey<=ry;ey++)
	{
		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));
		
		glVertex2i(x-ex,y-ey);
		glVertex2i(x+ex,y-ey);

		glVertex2i(x-ex,y+ey);
		glVertex2i(x+ex,y+ey);
	}
	glEnd();
}

/**************************************************************************
	dismod_movepixels_dis
 **************************************************************************/
TMODAPI TVOID dismod_movepixels_dis(TMOD_DISMOD *dismod,TINT sx,TINT sy, TINT dx, TINT dy, TINT w, TINT h)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glRasterPos2d(dx,dy+h-1);
	glCopyPixels(sx,dismod->height-h-sy,w,h-1,GL_COLOR);
	glFlush();
}

