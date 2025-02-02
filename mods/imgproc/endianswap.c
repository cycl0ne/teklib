#include "imgproc.h"

TVOID IMGEndianSwap_16bit(TMOD_IMGPROC *imgp);
TVOID IMGEndianSwap_32bit(TMOD_IMGPROC *imgp);

/**************************************************************************
	IMGEndianSwap Main Routine
 **************************************************************************/
TVOID IMGEndianSwap(TMOD_IMGPROC *imgp)
{
	switch(imgp->srcfmt)
	{
		case IMGFMT_R5G5B5: case IMGFMT_R5G6B5:
			IMGEndianSwap_16bit(imgp);
		break;
	
		case IMGFMT_A8R8G8B8: case IMGFMT_R8G8B8A8: case IMGFMT_B8G8R8A8:
			IMGEndianSwap_32bit(imgp);
		break;
	}
}

TVOID IMGEndianSwap_16bit(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT16 *src,*dst;

	for( y=0; y<imgp->height; y++)
	{
		src=(TUINT16*)(imgp->srcdata + (y+imgp->srcy)*imgp->srcbytesperrow) + imgp->srcx;
		dst=(TUINT16*)(imgp->dstdata + (y+imgp->dsty)*imgp->dstbytesperrow) + imgp->dstx;

		for( x=0; x<imgp->width; x++)
		{
			dst[x]=(src[x]<<8) | (src[x]>>8);
		}
	}
}

TVOID IMGEndianSwap_32bit(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT *src,*dst;

	for( y=0; y<imgp->height; y++)
	{
		src=(TUINT*)(imgp->srcdata + (y+imgp->srcy)*imgp->srcbytesperrow) + imgp->srcx;
		dst=(TUINT*)(imgp->dstdata + (y+imgp->dsty)*imgp->dstbytesperrow) + imgp->dstx;

		for( x=0; x<imgp->width; x++)
		{
			dst[x]=	(src[x]<<24) | 
					((src[x] & 0x0000ff00)<<8) |
					((src[x] & 0x00ff0000)>>8) |
					(src[x]>>24);
		}
	}
}
