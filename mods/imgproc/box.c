#include "imgproc.h"

/***********************************************************************
	Box 8 bit
 ***********************************************************************/
TVOID IMGBox_8(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col)
{
	TINT i;
	TUINT8* dst=(TUINT8*)pic->data + y*pic->bytesperrow + x;
	TINT lineoff=pic->bytesperrow;

	for(i=0;i<=w;i++)
		dst[i]=col;

	dst+=lineoff;

	for(i=1;i<h;i++)
	{
		dst[0]=col;
		dst[w]=col;
		dst+=lineoff;
	}

	for(i=0;i<=w;i++)
		dst[i]=col;
}

/***********************************************************************
	Box 16 bit
 ***********************************************************************/
TVOID IMGBox_16(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col)
{
	TINT i;
	TUINT16* dst=(TUINT16*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
	TINT lineoff=pic->bytesperrow>>1;

	for(i=0;i<=w;i++)
		dst[i]=col;

	dst+=lineoff;

	for(i=1;i<h;i++)
	{
		dst[0]=col;
		dst[w]=col;
		dst+=lineoff;
	}

	for(i=0;i<=w;i++)
		dst[i]=col;
}

/***********************************************************************
	Box 24 bit
 ***********************************************************************/
TVOID IMGBox_24(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT8 r, TUINT8 g, TUINT8 b)
{
	TINT i;
	TUINT8* dst=(TUINT8*)pic->data + y*pic->bytesperrow + x*3;
	TINT lineoff=pic->bytesperrow;

	for(i=0;i<=w;i++)
	{
		dst[i*3]=r;
		dst[i*3+1]=g;
		dst[i*3+2]=b;
	}

	dst+=lineoff;

	for(i=1;i<h;i++)
	{
		dst[0]=r;
		dst[1]=g;
		dst[2]=b;
		dst[w*3]=r;
		dst[w*3+1]=g;
		dst[w*3+2]=b;
		dst+=lineoff;
	}

	for(i=0;i<=w;i++)
	{
		dst[i*3]=r;
		dst[i*3+1]=g;
		dst[i*3+2]=b;
	}
}

/***********************************************************************
	Box 32 bit
 ***********************************************************************/
TVOID IMGBox_32(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col)
{
	TINT i;
	TUINT* dst=(TUINT*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
	TINT lineoff=pic->bytesperrow>>2;

	for(i=0;i<=w;i++)
		dst[i]=col;

	dst+=lineoff;

	for(i=1;i<h;i++)
	{
		dst[0]=col;
		dst[w]=col;
		dst+=lineoff;
	}

	for(i=0;i<=w;i++)
		dst[i]=col;
}
