#include "imgproc.h"
#include <tek/proto/imgproc.h>
#include "imgp_makros.h"

/***********************************************************************
	Scale hard
 ***********************************************************************/
TVOID IMGScale_Hard(TMOD_IMGPROC *imgp)
{
	TINT delta_x,delta_y,sx,sy,dx,dy,w,x;
	TUINT8 *src, *dst;

	TIMGARGBCOLOR pixel;

	TINT srcbpel=TImgBytesPerPel(imgp,imgp->srcfmt);
	TINT dstbpel=TImgBytesPerPel(imgp,imgp->dstfmt);

	delta_y=(TINT)((TFLOAT)imgp->height*65536.0f/(TFLOAT)imgp->scaleheight);

	if(imgp->scalewidth > imgp->width)
	{
		delta_x=(TINT)((TFLOAT)imgp->scalewidth*65536.0f/(TFLOAT)imgp->width);

		sy=0;
		for( dy=0; dy<imgp->scaleheight; dy++)
		{
			src=(TUINT8*)imgp->srcdata + ((sy>>16)+imgp->srcy)*imgp->srcbytesperrow + imgp->srcx*srcbpel;
			dst=(TUINT8*)imgp->dstdata + (dy+imgp->dsty)*imgp->dstbytesperrow + imgp->dstx*dstbpel;

			dx=0;
			x=0;
			for( sx=0; sx<imgp->width; sx++)
			{
				IMG_GetPixel((TAPTR)(src+sx*srcbpel),imgp->srcfmt, imgp->dstfmt, imgp->srcpalette, &pixel);

				w=((dx+delta_x)>>16)-(dx>>16);
				while(w)
				{
					IMG_PutPixel((TAPTR)(dst+x*dstbpel),imgp->srcfmt, imgp->dstfmt, &pixel);
					x++;
					w--;
				}
				dx+=delta_x;
			}
			sy+=delta_y;
		}
	}
	else
	{
		delta_x=(TINT)((TFLOAT)imgp->width*65536.0f/(TFLOAT)imgp->scalewidth);

		sy=0;
		for( dy=0; dy<imgp->scaleheight; dy++)
		{
			src=(TUINT8*)imgp->srcdata + ((sy>>16)+imgp->srcy)*imgp->srcbytesperrow + imgp->srcx*srcbpel;
			dst=(TUINT8*)imgp->dstdata + (dy+imgp->dsty)*imgp->dstbytesperrow + imgp->dstx*dstbpel;

			sx=0;
			for( dx=0; dx<imgp->scalewidth; dx++)
			{
				IMG_GetPixel((TAPTR)(src+(sx>>16)*srcbpel),imgp->srcfmt, imgp->dstfmt, imgp->srcpalette, &pixel);
				IMG_PutPixel((TAPTR)(dst+dx*dstbpel),imgp->srcfmt, imgp->dstfmt, &pixel);

				sx+=delta_x;
			}
			sy+=delta_y;
		}
	}
}
