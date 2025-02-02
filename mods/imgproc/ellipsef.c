#include "imgproc.h"
#include <math.h>

/***********************************************************************
	Ellipse filled 8 bit
 ***********************************************************************/
TVOID IMGEllipsef_8(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col)
{
	TINT ex,ey,i;
	TFLOAT r1q,r2q;

	TUINT8 *dst1=(TUINT8*)((TUINT8*)pic->data+y*pic->bytesperrow)+x;
	TUINT8 *dst2=dst1;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	for(i=-rx;i<=rx;i++)
		dst1[i]=col;

	for(ey=1;ey<=ry;ey++)
	{
		dst1+=pic->bytesperrow;
		dst2-=pic->bytesperrow;

		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));
		
		for(i=-ex;i<=ex;i++)
		{
			dst1[i]=col;
			dst2[i]=col;
		}
	}
}

/***********************************************************************
	Ellipse filled 16 bit
 ***********************************************************************/
TVOID IMGEllipsef_16(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col)
{
	TINT ex,ey,i;
	TFLOAT r1q,r2q;

	TUINT16 *dst1=(TUINT16*)((TUINT8*)pic->data+y*pic->bytesperrow)+x;
	TUINT16 *dst2=dst1;
	TINT linewid=pic->bytesperrow>>1;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	for(i=-rx;i<=rx;i++)
		dst1[i]=col;

	for(ey=1;ey<=ry;ey++)
	{
		dst1+=linewid;
		dst2-=linewid;

		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));
		
		for(i=-ex;i<=ex;i++)
		{
			dst1[i]=col;
			dst2[i]=col;
		}
	}
}

/***********************************************************************
	Ellipse filled 24 bit
 ***********************************************************************/
TVOID IMGEllipsef_24(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT8 r, TUINT8 g, TUINT8 b)
{
	TINT ex,ey,i,j;
	TFLOAT r1q,r2q;

	TUINT8 *dst1=(TUINT8*)pic->data+y*pic->bytesperrow+x*3;
	TUINT8 *dst2=dst1;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	for(i=-rx;i<=rx;i++)
	{	
		j=i*3;
		dst1[j]=r;	dst1[j+1]=g;	dst1[j+2]=b;
	}
	
	for(ey=1;ey<=ry;ey++)
	{
		dst1+=pic->bytesperrow;
		dst2-=pic->bytesperrow;

		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));
		
		for(i=-ex;i<=ex;i++)
		{	
			j=i*3;
			dst1[j]=r;	dst1[j+1]=g;	dst1[j+2]=b;
			dst2[j]=r;	dst2[j+1]=g;	dst2[j+2]=b;
		}
	}
}

/***********************************************************************
	Ellipse filled 32 bit
 ***********************************************************************/
TVOID IMGEllipsef_32(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col)
{
	TINT ex,ey,i;
	TFLOAT r1q,r2q;

	TUINT *dst1=(TUINT*)((TUINT8*)pic->data+y*pic->bytesperrow)+x;
	TUINT *dst2=dst1;
	TINT linewid=pic->bytesperrow>>2;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	for(i=-rx;i<=rx;i++)
		dst1[i]=col;

	for(ey=1;ey<=ry;ey++)
	{
		dst1+=linewid;
		dst2-=linewid;

		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));
		
		for(i=-ex;i<=ex;i++)
		{
			dst1[i]=col;
			dst2[i]=col;
		}
	}
}
