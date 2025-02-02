#include "imgproc.h"

TVOID IMGBlitNormal_8bit(TMOD_IMGPROC *imgp);
TVOID IMGBlitNormal_16bit(TMOD_IMGPROC *imgp);
TVOID IMGBlitNormal_24bit(TMOD_IMGPROC *imgp);
TVOID IMGBlitNormal_32bit(TMOD_IMGPROC *imgp);

/**************************************************************************
	IMGBlitNormal Main Routine
 **************************************************************************/
TVOID IMGBlitNormal(TMOD_IMGPROC *imgp)
{
	switch(imgp->srcfmt)
	{
		case IMGFMT_CLUT:
			IMGBlitNormal_8bit(imgp);
		break;

		case IMGFMT_R5G5B5: case IMGFMT_R5G6B5:
			IMGBlitNormal_16bit(imgp);
		break;
	
		case IMGFMT_R8G8B8: case IMGFMT_B8G8R8:
			IMGBlitNormal_24bit(imgp);
		break;
		
		case IMGFMT_A8R8G8B8: case IMGFMT_R8G8B8A8: case IMGFMT_B8G8R8A8:
			IMGBlitNormal_32bit(imgp);
		break;
	}
}

TVOID IMGBlitNormal_8bit(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 *src,*dst;

	for( y=0; y<imgp->height; y++)
	{
		src=(TUINT8*)(imgp->srcdata + (y+imgp->srcy)*imgp->srcbytesperrow) + imgp->srcx;
		dst=(TUINT8*)(imgp->dstdata + (y+imgp->dsty)*imgp->dstbytesperrow) + imgp->dstx;

		for( x=0; x<imgp->width; x++)
		{
			dst[x]=src[x];
		}
	}
}

TVOID IMGBlitNormal_16bit(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT16 *src,*dst;

	for( y=0; y<imgp->height; y++)
	{
		src=(TUINT16*)(imgp->srcdata + (y+imgp->srcy)*imgp->srcbytesperrow) + imgp->srcx;
		dst=(TUINT16*)(imgp->dstdata + (y+imgp->dsty)*imgp->dstbytesperrow) + imgp->dstx;

		for( x=0; x<imgp->width; x++)
		{
			dst[x]=src[x];
		}
	}
}

TVOID IMGBlitNormal_24bit(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 *src,*dst;

	for( y=0; y<imgp->height; y++)
	{
		src=(TUINT8*)(imgp->srcdata + (y+imgp->srcy)*imgp->srcbytesperrow) + imgp->srcx*3;
		dst=(TUINT8*)(imgp->dstdata + (y+imgp->dsty)*imgp->dstbytesperrow) + imgp->dstx*3;

		for( x=0; x<imgp->width; x++)
		{
			*dst++=*src++;
			*dst++=*src++;
			*dst++=*src++;
		}
	}
}

TVOID IMGBlitNormal_32bit(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT *src,*dst;

	for( y=0; y<imgp->height; y++)
	{
		src=(TUINT*)(imgp->srcdata + (y+imgp->srcy)*imgp->srcbytesperrow) + imgp->srcx;
		dst=(TUINT*)(imgp->dstdata + (y+imgp->dsty)*imgp->dstbytesperrow) + imgp->dstx;

		for( x=0; x<imgp->width; x++)
		{
			dst[x]=src[x];
		}
	}
}
