#include "imgproc.h"
#include <math.h>

/***********************************************************************
	Ellipse 8 bit
 ***********************************************************************/
TVOID IMGEllipse_8(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col)
{
	TINT ex,ey,oldx,i;
	TFLOAT r1q,r2q;

	TUINT8 *dst1=(TUINT8*)pic->data+y*pic->bytesperrow+x;
	TUINT8 *dst2=dst1;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	oldx=rx;

	for(ey=0;ey<=ry;ey++)
	{
		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));
		
		if(ex<oldx-1)
		{
			for(i=ex;i<oldx;i++)
			{
				dst1[i]=col;
				dst2[i]=col;
				dst1[-i]=col;
				dst2[-i]=col;
			}
		}
		else
		{
			dst1[ex]=col;
			dst2[ex]=col;
			dst1[-ex]=col;
			dst2[-ex]=col;
		}
		dst1+=pic->bytesperrow;
		dst2-=pic->bytesperrow;
		oldx=ex;
	}
}

/***********************************************************************
	Ellipse 16 bit
 ***********************************************************************/
TVOID IMGEllipse_16(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col)
{
	TINT ex,ey,oldx,i;
	TFLOAT r1q,r2q;

	TUINT16 *dst1=(TUINT16*)((TUINT8*)pic->data+y*pic->bytesperrow)+x;
	TUINT16 *dst2=dst1;
	TINT linewid=pic->bytesperrow>>1;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	oldx=rx;

	for(ey=0;ey<=ry;ey++)
	{
		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));
		
		if(ex<oldx)
		{
			for(i=ex;i<oldx;i++)
			{
				dst1[i]=col;
				dst2[i]=col;
				dst1[-i]=col;
				dst2[-i]=col;
			}
		}
		else
		{
			dst1[ex]=col;
			dst2[ex]=col;
			dst1[-ex]=col;
			dst2[-ex]=col;
		}
		dst1+=linewid;
		dst2-=linewid;
		oldx=ex;
	}
}

/***********************************************************************
	Ellipse 24 bit
 ***********************************************************************/
TVOID IMGEllipse_24(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT8 r, TUINT8 g, TUINT8 b)
{
	TINT ex,ey,oldx,i,j;
	TFLOAT r1q,r2q;

	TUINT8 *dst1=(TUINT8*)pic->data+y*pic->bytesperrow+x*3;
	TUINT8 *dst2=dst1;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	oldx=rx;

	for(ey=0;ey<=ry;ey++)
	{
		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));
		
		if(ex<oldx-1)
		{
			for(i=ex;i<oldx;i++)
			{
				j=i*3;
				dst1[j]=r;	dst1[j+1]=g;	dst1[j+2]=b;
				dst2[j]=r;	dst2[j+1]=g;	dst2[j+2]=b;
				dst1[-j]=r;	dst1[-j+1]=g;	dst1[-j+2]=b;
				dst2[-j]=r;	dst2[-j+1]=g;	dst2[-j+2]=b;
			}
		}
		else
		{
			j=ex*3;
			dst1[j]=r;	dst1[j+1]=g;	dst1[j+2]=b;
			dst2[j]=r;	dst2[j+1]=g;	dst2[j+2]=b;
			dst1[-j]=r;	dst1[-j+1]=g;	dst1[-j+2]=b;
			dst2[-j]=r;	dst2[-j+1]=g;	dst2[-j+2]=b;
		}
		dst1+=pic->bytesperrow;
		dst2-=pic->bytesperrow;
		oldx=ex;
	}
}

/***********************************************************************
	Ellipse 32 bit
 ***********************************************************************/
TVOID IMGEllipse_32(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col)
{
	TINT ex,ey,oldx,i;
	TFLOAT r1q,r2q;

	TUINT *dst1=(TUINT*)((TUINT8*)pic->data+y*pic->bytesperrow)+x;
	TUINT *dst2=dst1;
	TINT linewid=pic->bytesperrow>>2;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	oldx=rx;

	for(ey=0;ey<=ry;ey++)
	{
		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));
		
		if(ex<oldx-1)
		{
			for(i=ex;i<oldx;i++)
			{
				dst1[i]=col;
				dst2[i]=col;
				dst1[-i]=col;
				dst2[-i]=col;
			}
		}
		else
		{
			dst1[ex]=col;
			dst2[ex]=col;
			dst1[-ex]=col;
			dst2[-ex]=col;
		}
		dst1+=linewid;
		dst2-=linewid;
		oldx=ex;
	}
}
