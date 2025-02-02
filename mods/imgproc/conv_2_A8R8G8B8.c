#include "imgproc.h"
#include "imgp_makros.h"

TVOID IMGConv_CLUT_2_A8R8G8B8(TMOD_IMGPROC *imgp);
TVOID IMGConv_R5G5B5_2_A8R8G8B8(TMOD_IMGPROC *imgp);
TVOID IMGConv_R5G6B5_2_A8R8G8B8(TMOD_IMGPROC *imgp);
TVOID IMGConv_R8G8B8_2_A8R8G8B8(TMOD_IMGPROC *imgp);
TVOID IMGConv_A8R8G8B8_2_A8R8G8B8(TMOD_IMGPROC *imgp);
TVOID IMGConv_R8G8B8A8_2_A8R8G8B8(TMOD_IMGPROC *imgp);
TVOID IMGConv_B8G8R8_2_A8R8G8B8(TMOD_IMGPROC *imgp);
TVOID IMGConv_B8G8R8A8_2_A8R8G8B8(TMOD_IMGPROC *imgp);

/**************************************************************************
	IMGConv_2_A8R8G8B8 Main Routine
 **************************************************************************/
TVOID IMGConv_2_A8R8G8B8(TMOD_IMGPROC *imgp)
{
	switch(imgp->srcfmt)
	{
		case IMGFMT_CLUT:
			IMGConv_CLUT_2_A8R8G8B8(imgp);
		break;

		case IMGFMT_R5G5B5:
			IMGConv_R5G5B5_2_A8R8G8B8(imgp);
		break;
		
		case IMGFMT_R5G6B5:
			IMGConv_R5G6B5_2_A8R8G8B8(imgp);
		break;
		
		case IMGFMT_R8G8B8:
			IMGConv_R8G8B8_2_A8R8G8B8(imgp);
		break;
		
		case IMGFMT_A8R8G8B8:
			IMGConv_A8R8G8B8_2_A8R8G8B8(imgp);
		break;
		
		case IMGFMT_R8G8B8A8:
			IMGConv_R8G8B8A8_2_A8R8G8B8(imgp);
		break;

		case IMGFMT_B8G8R8:
			IMGConv_B8G8R8_2_A8R8G8B8(imgp);
		break;
		
		case IMGFMT_B8G8R8A8:
			IMGConv_B8G8R8A8_2_A8R8G8B8(imgp);
		break;
	}
}

/**************************************************************************
	IMGConv_CLUT_2_A8R8G8B8
 **************************************************************************/
TVOID IMGConv_CLUT_2_A8R8G8B8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 paloffset;
	TUINT8 a,r,g,b;
	TUINT8* src;
	TUINT *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_8bit
		imgp_dst_for_32bit
		for(x=0;x<imgp->width;x++)
		{
			paloffset=*src++;

			a=imgp->srcpalette[paloffset].a;
			r=imgp->srcpalette[paloffset].r;
			g=imgp->srcpalette[paloffset].g;
			b=imgp->srcpalette[paloffset].b;

			*dst++=(a<<24)|(r<<16)|(g<<8)|b;
		}
	}
}

/**************************************************************************
	IMGConv_R5G5B5_2_A8R8G8B8
 **************************************************************************/
TVOID IMGConv_R5G5B5_2_A8R8G8B8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT16 col;
	TUINT8 a,r,g,b;
	TUINT16 *src;
	TUINT *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_16bit
		imgp_dst_for_32bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			a=(col & 0x8000)>>8;
			r=(col & 0x7c00)>>7;
			g=(col & 0x03e0)>>2;
			b=(col & 0x001f)<<3;

			*dst++=(a<<24)|(r<<16)|(g<<8)|b;
		}
	}
}

/**************************************************************************
	IMGConv_R5G6B5_2_A8R8G8B8
 **************************************************************************/
TVOID IMGConv_R5G6B5_2_A8R8G8B8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT16 col;
	TUINT8 r,g,b;
	TUINT16 *src;
	TUINT *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_16bit
		imgp_dst_for_32bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			r=(col & 0xf800)>>8;
			g=(col & 0x07e0)>>3;
			b=(col & 0x001f)<<3;

			*dst++=(r<<16)|(g<<8)|b;
		}
	}
}

/**************************************************************************
	IMGConv_R8G8B8_2_A8R8G8B8
 **************************************************************************/
TVOID IMGConv_R8G8B8_2_A8R8G8B8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 r,g,b;

	TUINT8 *src;
	TUINT *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_24bit
		imgp_dst_for_32bit
		for(x=0;x<imgp->width;x++)
		{
			r=*src++;
			g=*src++;
			b=*src++;

			*dst++=(r<<16)|(g<<8)|b;
		}
	}
}

/**************************************************************************
	IMGConv_A8R8G8B8_2_A8R8G8B8
 **************************************************************************/
TVOID IMGConv_A8R8G8B8_2_A8R8G8B8(TMOD_IMGPROC *imgp)
{
	TINT y;
	TUINT *src, *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_32bit
		imgp_dst_for_32bit
		TExecCopyMem(imgp->exec,src,dst,imgp->width*4);
	}
}

/**************************************************************************
	IMGConv_R8G8B8A8_2_A8R8G8B8
 **************************************************************************/
TVOID IMGConv_R8G8B8A8_2_A8R8G8B8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 a,r,g,b;

	TUINT col;
	TUINT *src;
	
	TUINT *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_32bit
		imgp_dst_for_32bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			r=(col & 0xff000000)>>24;
			g=(col & 0x00ff0000)>>16;
			b=(col & 0x0000ff00)>>8;
			a=(col & 0x000000ff);

			*dst++=(a<<24)|(r<<16)|(g<<8)|b;
		}
	}
}

/**************************************************************************
	IMGConv_B8G8R8_2_A8R8G8B8
 **************************************************************************/
TVOID IMGConv_B8G8R8_2_A8R8G8B8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 r,g,b;
	TUINT8* src;
	TUINT *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_24bit
		imgp_dst_for_32bit
		for(x=0;x<imgp->width;x++)
		{
			b=*src++;
			g=*src++;
			r=*src++;

			*dst++=(r<<16)|(g<<8)|b;
		}
	}
}

/**************************************************************************
	IMGConv_B8G8R8A8_2_A8R8G8B8
 **************************************************************************/
TVOID IMGConv_B8G8R8A8_2_A8R8G8B8(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT col;
	TUINT8 a,r,g,b;
	TUINT *src;
	TUINT *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_32bit
		imgp_dst_for_32bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			b=(col & 0xff000000)>>24;
			g=(col & 0x00ff0000)>>16;
			r=(col & 0x0000ff00)>>8;
			a=(col & 0x000000ff);

			*dst++=(a<<24)|(r<<16)|(g<<8)|b;
		}
	}
}

