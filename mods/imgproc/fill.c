#include "imgproc.h"

/***********************************************************************
	Fill 8 bit
 ***********************************************************************/
TVOID IMGFill_8(TIMGPICTURE *pic, TUINT col)
{
	TINT x,y;
	TUINT8* dst;

	for(y=0;y<pic->height;y++)
	{
		dst=(TUINT8*)pic->data+y*pic->bytesperrow;
		for(x=0;x<pic->width;x++)
		{
			*dst++=col;
		}
	}
}

/***********************************************************************
	Fill 16 bit
 ***********************************************************************/
TVOID IMGFill_16(TIMGPICTURE *pic, TUINT col)
{
	TINT x,y;
	TUINT16* dst;

	for(y=0;y<pic->height;y++)
	{
		dst=(TUINT16*)((TUINT8*)pic->data+y*pic->bytesperrow);
		for(x=0;x<pic->width;x++)
		{
			*dst++=col;
		}
	}
}

/***********************************************************************
	Fill 24 bit
 ***********************************************************************/
TVOID IMGFill_24(TIMGPICTURE *pic, TUINT8 r, TUINT8 g, TUINT8 b)
{
	TINT x,y;
	TUINT8* dst;

	for(y=0;y<pic->height;y++)
	{
		dst=(TUINT8*)pic->data+y*pic->bytesperrow;
		for(x=0;x<pic->width;x++)
		{
			*dst++=r;
			*dst++=g;
			*dst++=b;
		}
	}
}

/***********************************************************************
	Fill 32 bit
 ***********************************************************************/
TVOID IMGFill_32(TIMGPICTURE *pic, TUINT col)
{
	TINT x,y;
	TUINT* dst;

	for(y=0;y<pic->height;y++)
	{
		dst=(TUINT*)((TUINT8*)pic->data+y*pic->bytesperrow);
		for(x=0;x<pic->width;x++)
		{
			*dst++=col;
		}
	}
}
