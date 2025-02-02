#include "imgproc.h"

TVOID IMGPolyf_preparepoly(TINT numpoints, TINT *points, TINT *polyfbuf)
{
	TINT i;
	TINT cy,ny,maxy;
	TINT sp,cp;
	TINT ax,ay,ex,ey,dx,dy;
	TINT y;

	TINT *linebuf=polyfbuf+2;

	/* seek for top-left and lowest point */
	sp=0;
	cy=points[1];
	maxy=-100000;
	for(i=1;i<numpoints;i++)
	{
		ny=points[i*2+1];

		if(cy>ny)
		{
			sp=i;
			cy=ny;
		}
		else if(cy==ny)
		{
			if(points[sp*2]<points[i*2])
			{
				sp=i;
				cy=ny;
			}
		}

		if(ny>maxy)
			maxy=ny;
	}

	polyfbuf[0]=points[sp*2+1];
	polyfbuf[1]=maxy;

	/* fill up linebuffer with x-coords */
	ax=points[sp*2]<<16;
	ay=points[sp*2+1];
	cp=sp+1;
	if(cp==numpoints)
		cp=0;

	for(i=0;i<numpoints;i++)
	{
		ex=points[cp*2]<<16;
		ey=points[cp*2+1];

		dy=ey-ay;
		if(dy)
		{
			dx=(ex-ax)/dy;

			if(ay<ey)
			{
				for(y=ay;y<=ey;y++)
				{
					linebuf[y*2]=(ax>>16);
					ax+=dx;
				}
			}
			else if(ay>ey)
			{
				for(y=ay;y>=ey;y--)
				{
					linebuf[y*2+1]=(ax>>16);
					ax-=dx;
				}
			}
		}
		ax=ex;
		ay=ey;

		cp++;
		if(cp==numpoints)
			cp=0;
	}
}

/***********************************************************************
	Polyf 8 bit
 ***********************************************************************/
TVOID IMGPolyf_8(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT col, TINT *polyfbuf)
{
	TINT i,x,sx,ex,sy,ey,lineoff;
	TUINT8 *screen;
	TINT *linebuf=polyfbuf+2;

	IMGPolyf_preparepoly(numpoints,points,polyfbuf);

	sy=polyfbuf[0];
	ey=polyfbuf[1];

	lineoff=pic->bytesperrow;
	screen=(TUINT8*)pic->data+sy*pic->bytesperrow;

	i=sy+((ey-sy)>>1);
	sx=linebuf[i*2];
	ex=linebuf[i*2+1];

	if(sx<ex)
	{
		for(i=sy;i<=ey;i++)
		{
			sx=linebuf[i*2];
			ex=linebuf[i*2+1];
			for(x=sx;x<=ex;x++)
			{
				screen[x]=col;
			}
			screen+=lineoff;
		}
	}
	else
	{
		for(i=sy;i<=ey;i++)
		{
			sx=linebuf[i*2+1];
			ex=linebuf[i*2];
			for(x=sx;x<=ex;x++)
			{
				screen[x]=col;
			}
			screen+=lineoff;
		}
	}
}

/***********************************************************************
	Polyf 16 bit
 ***********************************************************************/
TVOID IMGPolyf_16(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT col, TINT *polyfbuf)
{
	TINT i,x,sx,ex,sy,ey,lineoff;
	TUINT16 *screen;
	TINT *linebuf=polyfbuf+2;

	IMGPolyf_preparepoly(numpoints,points,polyfbuf);

	sy=polyfbuf[0];
	ey=polyfbuf[1];

	lineoff=pic->bytesperrow>>1;
	screen=(TUINT16*)((TUINT8*)pic->data+sy*pic->bytesperrow);

	i=sy+((ey-sy)>>1);
	sx=linebuf[i*2];
	ex=linebuf[i*2+1];

	if(sx<ex)
	{
		for(i=sy;i<=ey;i++)
		{
			sx=linebuf[i*2];
			ex=linebuf[i*2+1];
			for(x=sx;x<=ex;x++)
			{
				screen[x]=col;
			}
			screen+=lineoff;
		}
	}
	else
	{
		for(i=sy;i<=ey;i++)
		{
			sx=linebuf[i*2+1];
			ex=linebuf[i*2];
			for(x=sx;x<=ex;x++)
			{
				screen[x]=col;
			}
			screen+=lineoff;
		}
	}
}

/***********************************************************************
	Polyf 24 bit
 ***********************************************************************/
TVOID IMGPolyf_24(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT8 r, TUINT8 g, TUINT8 b, TINT *polyfbuf)
{
	TINT i,x,sx,ex,sy,ey,lineoff;
	TUINT8 *screen;
	TINT *linebuf=polyfbuf+2;

	IMGPolyf_preparepoly(numpoints,points,polyfbuf);

	sy=polyfbuf[0];
	ey=polyfbuf[1];

	lineoff=pic->bytesperrow;
	screen=(TUINT8*)pic->data+sy*pic->bytesperrow;

	i=sy+((ey-sy)>>1);
	sx=linebuf[i*2];
	ex=linebuf[i*2+1];

	if(sx<ex)
	{
		for(i=sy;i<=ey;i++)
		{
			sx=linebuf[i*2];
			ex=linebuf[i*2+1];
			for(x=sx;x<=ex;x++)
			{
				screen[x*3]=r;
				screen[x*3+1]=g;
				screen[x*3+2]=b;
			}
			screen+=lineoff;
		}
	}
	else
	{
		for(i=sy;i<=ey;i++)
		{
			sx=linebuf[i*2+1];
			ex=linebuf[i*2];
			for(x=sx;x<=ex;x++)
			{
				screen[x*3]=r;
				screen[x*3+1]=g;
				screen[x*3+2]=b;
			}
			screen+=lineoff;
		}
	}
}

/***********************************************************************
	Polyf 32 bit
 ***********************************************************************/
TVOID IMGPolyf_32(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT col, TINT *polyfbuf)
{
	TINT i,x,sx,ex,sy,ey,lineoff;
	TUINT *screen;
	TINT *linebuf=polyfbuf+2;

	IMGPolyf_preparepoly(numpoints,points,polyfbuf);

	sy=polyfbuf[0];
	ey=polyfbuf[1];

	lineoff=pic->bytesperrow>>2;
	screen=(TUINT*)((TUINT8*)pic->data+sy*pic->bytesperrow);

	i=sy+((ey-sy)>>1);
	sx=linebuf[i*2];
	ex=linebuf[i*2+1];

	if(sx<ex)
	{
		for(i=sy;i<=ey;i++)
		{
			sx=linebuf[i*2];
			ex=linebuf[i*2+1];
			for(x=sx;x<=ex;x++)
			{
				screen[x]=col;
			}
			screen+=lineoff;
		}
	}
	else
	{
		for(i=sy;i<=ey;i++)
		{
			sx=linebuf[i*2+1];
			ex=linebuf[i*2];
			for(x=sx;x<=ex;x++)
			{
				screen[x]=col;
			}
			screen+=lineoff;
		}
	}
}
