#include "imgproc.h"
#include "imgp_makros.h"

TVOID IMGConv_CLUT_2_B8G8R8(TMOD_IMGPROC *imgp);
TVOID IMGConv_R5G5B5_2_B8G8R8(TMOD_IMGPROC *imgp);
TVOID IMGConv_R5G6B5_2_B8G8R8(TMOD_IMGPROC *imgp);
TVOID IMGConv_R8G8B8_2_B8G8R8(TMOD_IMGPROC *imgp);
TVOID IMGConv_A8R8G8B8_2_B8G8R8(TMOD_IMGPROC *imgp);
TVOID IMGConv_R8G8B8A8_2_B8G8R8(TMOD_IMGPROC *imgp);
TVOID IMGConv_B8G8R8_2_B8G8R8(TMOD_IMGPROC *imgp);
TVOID IMGConv_B8G8R8A8_2_B8G8R8(TMOD_IMGPROC *imgp);

/**************************************************************************
	IMGConv_2_B8G8R8 Main Routine
 **************************************************************************/
TVOID IMGConv_2_B8G8R8(TMOD_IMGPROC *imgp)
{
	switch(imgp->srcfmt)
	{
		case IMGFMT_CLUT:
			IMGConv_CLUT_2_B8G8R8(imgp);
		break;

		case IMGFMT_R5G5B5:
			IMGConv_R5G5B5_2_B8G8R8(imgp);
		break;
		
		case IMGFMT_R5G6B5:
			IMGConv_R5G6B5_2_B8G8R8(imgp);
		break;
		
		case IMGFMT_R8G8B8:
			IMGConv_R8G8B8_2_B8G8R8(imgp);
		break;
		
		case IMGFMT_A8R8G8B8:
			IMGConv_A8R8G8B8_2_B8G8R8(imgp);
		break;
		
		case IMGFMT_R8G8B8A8:
			IMGConv_R8G8B8A8_2_B8G8R8(imgp);
		break;

		case IMGFMT_B8G8R8:
			IMGConv_B8G8R8_2_B8G8R8(imgp);
		break;
		
		case IMGFMT_B8G8R8A8:
			IMGConv_B8G8R8A8_2_B8G8R8(imgp);
		break;
	}
}

/**************************************************************************
	IMGConv_CLUT_2_B8G8R8
 **************************************************************************/
TVOID IMGConv_CLUT_2_B8G8R8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 paloffset;
	TUINT8 *src;
	TUINT8 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_8bit
		imgp_dst_for_24bit
		for(x=0;x<imgp->width;x++)
		{
			paloffset=*src++;

			*dst++=imgp->srcpalette[paloffset].b;
			*dst++=imgp->srcpalette[paloffset].g;
			*dst++=imgp->srcpalette[paloffset].r;
		}
	}
}

/**************************************************************************
	IMGConv_R5G5B5_2_B8G8R8
 **************************************************************************/
TVOID IMGConv_R5G5B5_2_B8G8R8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT16 col;
	TUINT16 *src;
	TUINT8 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_16bit
		imgp_dst_for_24bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			*dst++=(col & 0x001f)<<3;
			*dst++=(col & 0x03e0)>>2;
			*dst++=(col & 0x7c00)>>7;
		}
	}
}

/**************************************************************************
	IMGConv_R5G6B5_2_B8G8R8
 **************************************************************************/
TVOID IMGConv_R5G6B5_2_B8G8R8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT16 col;
	TUINT16 *src;
	TUINT8 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_16bit
		imgp_dst_for_24bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			*dst++=(col & 0x001f)<<3;
			*dst++=(col & 0x07e0)>>3;
			*dst++=(col & 0xf800)>>8;
		}
	}
}

/**************************************************************************
	IMGConv_R8G8B8_2_B8G8R8
 **************************************************************************/
TVOID IMGConv_R8G8B8_2_B8G8R8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 r,g,b;
	TUINT8 *src;
	TUINT8 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_24bit
		imgp_dst_for_24bit
		for(x=0;x<imgp->width;x++)
		{
			r=*src++;
			g=*src++;
			b=*src++;

			*dst++=b;
			*dst++=g;
			*dst++=r;
		}
	}
}

/**************************************************************************
	IMGConv_A8R8G8B8_2_B8G8R8
 **************************************************************************/
TVOID IMGConv_A8R8G8B8_2_B8G8R8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT col;
	TUINT *src;
	TUINT8 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_32bit
		imgp_dst_for_24bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			*dst++=(col & 0x000000ff);
			*dst++=(col & 0x0000ff00)>>8;
			*dst++=(col & 0x00ff0000)>>16;
		}
	}
}

/**************************************************************************
	IMGConv_R8G8B8A8_2_B8G8R8
 **************************************************************************/
TVOID IMGConv_R8G8B8A8_2_B8G8R8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT col;
	TUINT *src;
	TUINT8 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_32bit
		imgp_dst_for_24bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			*dst++=(col & 0x0000ff00)>>8;
			*dst++=(col & 0x00ff0000)>>16;
			*dst++=(col & 0xff000000)>>24;
		}
	}
}

/**************************************************************************
	IMGConv_B8G8R8_2_B8G8R8
 **************************************************************************/
TVOID IMGConv_B8G8R8_2_B8G8R8(TMOD_IMGPROC *imgp)
{
	TINT y;
	TUINT8 *src, *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_24bit
		imgp_dst_for_24bit
		TExecCopyMem(imgp->exec,src,dst,imgp->width*3);
	}
}

/**************************************************************************
	IMGConv_B8G8R8A8_2_B8G8R8
 **************************************************************************/
TVOID IMGConv_B8G8R8A8_2_B8G8R8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT col;
	TUINT *src;
	TUINT8 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_32bit
		imgp_dst_for_24bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			*dst++=(col & 0xff000000)>>24;
			*dst++=(col & 0x00ff0000)>>16;
			*dst++=(col & 0x0000ff00)>>8;
		}
	}
}

