#include "imgproc.h"
#include "imgp_makros.h"

TVOID IMGConv_CLUT_2_R5G5B5(TMOD_IMGPROC *imgp);
TVOID IMGConv_R5G5B5_2_R5G5B5(TMOD_IMGPROC *imgp);
TVOID IMGConv_R5G6B5_2_R5G5B5(TMOD_IMGPROC *imgp);
TVOID IMGConv_R8G8B8_2_R5G5B5(TMOD_IMGPROC *imgp);
TVOID IMGConv_A8R8G8B8_2_R5G5B5(TMOD_IMGPROC *imgp);
TVOID IMGConv_R8G8B8A8_2_R5G5B5(TMOD_IMGPROC *imgp);
TVOID IMGConv_B8G8R8_2_R5G5B5(TMOD_IMGPROC *imgp);
TVOID IMGConv_B8G8R8A8_2_R5G5B5(TMOD_IMGPROC *imgp);

/**************************************************************************
	IMGConv_2_R5G5B5 Main Routine
 **************************************************************************/
TVOID IMGConv_2_R5G5B5(TMOD_IMGPROC *imgp)
{
	switch(imgp->srcfmt)
	{
		case IMGFMT_CLUT:
			IMGConv_CLUT_2_R5G5B5(imgp);
		break;

		case IMGFMT_R5G5B5:
			IMGConv_R5G5B5_2_R5G5B5(imgp);
		break;

		case IMGFMT_R5G6B5:
			IMGConv_R5G6B5_2_R5G5B5(imgp);
		break;

		case IMGFMT_R8G8B8:
			IMGConv_R8G8B8_2_R5G5B5(imgp);
		break;

		case IMGFMT_A8R8G8B8:
			IMGConv_A8R8G8B8_2_R5G5B5(imgp);
		break;
		
		case IMGFMT_R8G8B8A8:
			IMGConv_R8G8B8A8_2_R5G5B5(imgp);
		break;

		case IMGFMT_B8G8R8:
			IMGConv_B8G8R8_2_R5G5B5(imgp);
		break;
		
		case IMGFMT_B8G8R8A8:
			IMGConv_B8G8R8A8_2_R5G5B5(imgp);
		break;
	}
}

/**************************************************************************
	IMGConv_CLUT_2_R5G5B5
 **************************************************************************/
TVOID IMGConv_CLUT_2_R5G5B5(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 a,r,g,b;
	TUINT8 paloffset;
	TUINT8 *src;
	TUINT16 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_8bit
		imgp_dst_for_16bit
		for(x=0;x<imgp->width;x++)
		{
			paloffset=*src++;
			
			a=imgp->srcpalette[paloffset].a & 0x80;
			r=imgp->srcpalette[paloffset].r & 0xf8;
			g=imgp->srcpalette[paloffset].g & 0xf8;
			b=imgp->srcpalette[paloffset].b & 0xf8;

			*dst++=(a<<8)|(r<<7)|(g<<2)|(b>>3);
		}
	}
}

/**************************************************************************
	IMGConv_R5G5B5_2_R5G5B5
 **************************************************************************/
TVOID IMGConv_R5G5B5_2_R5G5B5(TMOD_IMGPROC *imgp)
{
	TINT y;
	TUINT16 *src, *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_16bit
		imgp_dst_for_16bit
		TExecCopyMem(imgp->exec,src,dst,imgp->width*2);
	}
}

/**************************************************************************
	IMGConv_R5G6B5_2_R5G5B5
 **************************************************************************/
TVOID IMGConv_R5G6B5_2_R5G5B5(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT16 col;
	TUINT8 r,g,b;
	TUINT16 *src;
	TUINT16 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_16bit
		imgp_dst_for_16bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			r=(col & 0xf800)>>1;
			g=(col & 0x07c0)>>1;
			b=(col & 0x001f);

			*dst++=r|g|b;
		}
	}
}

/**************************************************************************
	IMGConv_R8G8B8_2_R5G5B5
 **************************************************************************/
TVOID IMGConv_R8G8B8_2_R5G5B5(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 r,g,b;
	TUINT8 *src;
	TUINT16 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_24bit
		imgp_dst_for_16bit
		for(x=0;x<imgp->width;x++)
		{
			r=*src++ & 0xf8;
			g=*src++ & 0xf8;
			b=*src++ & 0xf8;

			*dst++=(r<<7)|(g<<2)|(b>>3);
		}
	}
}

/**************************************************************************
	IMGConv_A8R8G8B8_2_R5G5B5
 **************************************************************************/
TVOID IMGConv_A8R8G8B8_2_R5G5B5(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT col;
	TUINT8 a,r,g,b;
	TUINT *src;
	TUINT16 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_32bit
		imgp_dst_for_16bit

		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			a=(col & 0x80000000)>>24;
			r=(col & 0x00f80000)>>16;
			g=(col & 0x0000f800)>>8;
			b=(col & 0x000000f8);

			*dst++=(a<<8)|(r<<7)|(g<<2)|(b>>3);
		}
	}
}

/**************************************************************************
	IMGConv_R8G8B8A8_2_R5G5B5
 **************************************************************************/
TVOID IMGConv_R8G8B8A8_2_R5G5B5(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT col;
	TUINT8 a,r,g,b;
	TUINT *src;
	TUINT16 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_32bit
		imgp_dst_for_16bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			r=(col & 0xf8000000)>>24;
			g=(col & 0x00f80000)>>16;
			b=(col & 0x0000f800)>>8;
			a=(col & 0x00000080);

			*dst++=(a<<8)|(r<<7)|(g<<2)|(b>>3);
		}
	}
}

/**************************************************************************
	IMGConv_B8G8R8_2_R5G5B5
 **************************************************************************/
TVOID IMGConv_B8G8R8_2_R5G5B5(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT8 r,g,b;
	TUINT8 *src;
	TUINT16 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_24bit
		imgp_dst_for_16bit
		for(x=0;x<imgp->width;x++)
		{
			b=*src++ & 0xf8;
			g=*src++ & 0xf8;
			r=*src++ & 0xf8;
			
			*dst++=(r<<7)|(g<<2)|(b>>3);
		}
	}
}

/**************************************************************************
	IMGConv_B8G8R8A8_2_R5G5B5
 **************************************************************************/
TVOID IMGConv_B8G8R8A8_2_R5G5B5(TMOD_IMGPROC *imgp)
{
	TINT x,y;
	TUINT col;
	TUINT8 a,r,g,b;
	TUINT *src;
	TUINT16 *dst;

	for(y=0;y<imgp->height;y++)
	{
		imgp_src_for_32bit
		imgp_dst_for_16bit
		for(x=0;x<imgp->width;x++)
		{
			col=*src++;

			b=(col & 0xf8000000)>>24;
			g=(col & 0x00f80000)>>16;
			r=(col & 0x0000f800)>>8;
			a=(col & 0x00000080);
			
			*dst++=(a<<8)|(r<<7)|(g<<2)|(b>>3);
		}
	}
}

