#include "imgproc.h"

/**************************************************************************
	IMGConv_Planar_2_Chunky Main Routine
 **************************************************************************/
TVOID IMGConv_PLANAR_2_CHUNKY(TMOD_IMGPROC *imgp)
{
	TINT x,y,d;
	TUINT8 a,b,c;
	TUINT8 *src, *src2, *src3, *dst;
	TINT main_width,rest_width;
	TINT r1,r2;

	main_width=imgp->srcwidth/8;
	rest_width=imgp->srcwidth & 0x00000007;

	if(imgp->srcdepth<=8)
	{
		for(d=imgp->srcdepth-1;d>=0;d--)
		{
			for(y=0;y<imgp->srcheight;y++)
			{
				src=(TUINT8*)imgp->srcdata+imgp->srcheight*imgp->srcbytesperrow*d+y*imgp->srcbytesperrow;
				dst=(TUINT8*)imgp->dstdata+y*imgp->srcwidth;

				for(x=0;x<main_width;x++)
				{
					a=*src++;

					*dst++ |= (((a & 0x80)>>7)<<d);
					*dst++ |= (((a & 0x40)>>6)<<d);
					*dst++ |= (((a & 0x20)>>5)<<d);
					*dst++ |= (((a & 0x10)>>4)<<d);
					*dst++ |= (((a & 0x08)>>3)<<d);
					*dst++ |= (((a & 0x04)>>2)<<d);
					*dst++ |= (((a & 0x02)>>1)<<d);
					*dst++ |= (((a & 0x01)>>0)<<d);
				}

				if(rest_width>0)
				{
					r1=0x80;
					r2=0x07;
					a=*src++;

					for(x=0;x<rest_width;x++)
					{
						*dst++ |= (((a & r1)>>r2)<<d);
						r1=r1>>1;
						r2--;
					}
				}
			}
		}
	}
	else if(imgp->srcdepth==24)
	{
		for(d=7;d>=0;d--)
		{
			for(y=0;y<imgp->srcheight;y++)
			{
				src=(TUINT8*)imgp->srcdata + imgp->srcheight*imgp->srcbytesperrow*d + y*imgp->srcbytesperrow;
				src2=(TUINT8*)imgp->srcdata + imgp->srcheight*imgp->srcbytesperrow*(d+8) + y*imgp->srcbytesperrow;
				src3=(TUINT8*)imgp->srcdata + imgp->srcheight*imgp->srcbytesperrow*(d+16) + y*imgp->srcbytesperrow;
				dst=(TUINT8*)imgp->dstdata + y*imgp->dstbytesperrow;

				for(x=0;x<main_width;x++)
				{
					a=*src++;
					b=*src2++;
					c=*src3++;

					*dst++ |= (((a & 0x80)>>7)<<d);
					*dst++ |= (((b & 0x80)>>7)<<d);
					*dst++ |= (((c & 0x80)>>7)<<d);

					*dst++ |= (((a & 0x40)>>6)<<d);
					*dst++ |= (((b & 0x40)>>6)<<d);
					*dst++ |= (((c & 0x40)>>6)<<d);

					*dst++ |= (((a & 0x20)>>5)<<d);
					*dst++ |= (((b & 0x20)>>5)<<d);
					*dst++ |= (((c & 0x20)>>5)<<d);

					*dst++ |= (((a & 0x10)>>4)<<d);
					*dst++ |= (((b & 0x10)>>4)<<d);
					*dst++ |= (((c & 0x10)>>4)<<d);

					*dst++ |= (((a & 0x08)>>3)<<d);
					*dst++ |= (((b & 0x08)>>3)<<d);
					*dst++ |= (((c & 0x08)>>3)<<d);

					*dst++ |= (((a & 0x04)>>2)<<d);
					*dst++ |= (((b & 0x04)>>2)<<d);
					*dst++ |= (((c & 0x04)>>2)<<d);

					*dst++ |= (((a & 0x02)>>1)<<d);
					*dst++ |= (((b & 0x02)>>1)<<d);
					*dst++ |= (((c & 0x02)>>1)<<d);

					*dst++ |= (((a & 0x01)>>0)<<d);
					*dst++ |= (((b & 0x01)>>0)<<d);
					*dst++ |= (((c & 0x01)>>0)<<d);
				}

				if(rest_width>0)
				{
					r1=0x80;
					r2=0x07;
					a=*src++;
					b=*src2++;
					c=*src3++;

					for(x=0;x<rest_width;x++)
					{
						*dst++ |= (((a & r1)>>r2)<<d);
						*dst++ |= (((b & r1)>>r2)<<d);
						*dst++ |= (((c & r1)>>r2)<<d);
						r1=r1>>1;
						r2--;
					}
				}
			}
		}
	}
}
