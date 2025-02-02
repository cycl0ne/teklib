#include "imgproc.h"

/***********************************************************************
	Boxf 8 bit
 ***********************************************************************/
TVOID IMGBoxf_8(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col)
{
	TINT i,j;
	TUINT8* dst=(TUINT8*)pic->data + y*pic->bytesperrow + x;
	TINT lineoff=pic->bytesperrow;

	for(i=0;i<h;i++)
	{
		for(j=0;j<w;j++)
		{
			dst[j]=col;
		}
		dst+=lineoff;
	}
}

/***********************************************************************
	Boxf 16 bit
 ***********************************************************************/
TVOID IMGBoxf_16(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col)
{
	TINT i,j;
	TUINT16* dst=(TUINT16*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
	TINT lineoff=pic->bytesperrow>>1;

	for(i=0;i<h;i++)
	{
		for(j=0;j<w;j++)
		{
			dst[j]=col;
		}
		dst+=lineoff;
	}
}

/***********************************************************************
	Boxf 24 bit
 ***********************************************************************/
TVOID IMGBoxf_24(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT8 r, TUINT8 g, TUINT8 b)
{
	TINT i,j;
	TUINT8* dst=(TUINT8*)pic->data + y*pic->bytesperrow + x*3;
	TINT lineoff=pic->bytesperrow;

	for(i=0;i<h;i++)
	{
		for(j=0;j<w;j++)
		{
			dst[j*3]=r;
			dst[j*3+1]=g;
			dst[j*3+2]=b;
		}
		dst+=lineoff;
	}
}

/***********************************************************************
	Boxf 32 bit
 ***********************************************************************/
TVOID IMGBoxf_32(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col)
{
	TINT i,j;
	TUINT* dst=(TUINT*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
	TINT lineoff=pic->bytesperrow>>2;

	for(i=0;i<h;i++)
	{
		for(j=0;j<w;j++)
		{
			dst[j]=col;
		}
		dst+=lineoff;
	}
}
