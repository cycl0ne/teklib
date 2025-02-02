#include "imgproc.h"
#include <tek/proto/imgproc.h>
#include "imgp_makros.h"

TVOID IMGScaleSmooth_Xbig(TMOD_IMGPROC *imgp,
						  TINT srcfmt, TINT8 *srcdata, TINT srcwidth, TINT srcheight, TINT srcx, TINT srcy, TINT srcbytesperrow,
						  TINT dstfmt, TINT8 *dstdata, TINT dstwidth, TINT dstheight, TINT dstx, TINT dsty, TINT dstbytesperrow);

TVOID IMGScaleSmooth_Ybig(TMOD_IMGPROC *imgp,
						  TINT srcfmt, TINT8 *srcdata, TINT srcwidth, TINT srcheight, TINT srcx, TINT srcy, TINT srcbytesperrow,
						  TINT dstfmt, TINT8 *dstdata, TINT dstwidth, TINT dstheight, TINT dstx, TINT dsty, TINT dstbytesperrow);

TVOID IMGScaleSmooth_Xsmall(TMOD_IMGPROC *imgp,
							TINT srcfmt, TINT8 *srcdata, TINT srcwidth, TINT srcheight, TINT srcx, TINT srcy, TINT srcbytesperrow,
						    TINT dstfmt, TINT8 *dstdata, TINT dstwidth, TINT dstheight, TINT dstx, TINT dsty, TINT dstbytesperrow);

TVOID IMGScaleSmooth_Ysmall(TMOD_IMGPROC *imgp,
							TINT srcfmt, TINT8 *srcdata, TINT srcwidth, TINT srcheight, TINT srcx, TINT srcy, TINT srcbytesperrow,
						    TINT dstfmt, TINT8 *dstdata, TINT dstwidth, TINT dstheight, TINT dstx, TINT dsty, TINT dstbytesperrow);

TBOOL IMGScale_Smooth(TMOD_IMGPROC *imgp)
{
	TUINT8* buffer=TNULL;

	if(imgp->scalewidth != imgp->width && imgp->scaleheight != imgp->height)
	{
		if(imgp->scalewidth<imgp->scaleheight)
		{
			TINT bpr=imgp->scalewidth*TImgBytesPerPel(imgp,IMGFMT_A8R8G8B8);
			buffer=TExecAlloc(imgp->exec,TNULL,bpr*imgp->srcheight);
			if(!buffer)
				return TFALSE;
			else
			{
				if(imgp->width<imgp->scalewidth)
				{
					IMGScaleSmooth_Xbig(imgp,
										imgp->srcfmt,imgp->srcdata,imgp->width,imgp->height,imgp->srcx, imgp->srcy,imgp->srcbytesperrow,
										IMGFMT_A8R8G8B8,buffer,imgp->scalewidth,imgp->srcheight,0,0,bpr);
				}
				else
				{
					IMGScaleSmooth_Xsmall(imgp,
										  imgp->srcfmt,imgp->srcdata,imgp->width,imgp->height,imgp->srcx, imgp->srcy,imgp->srcbytesperrow,
										  IMGFMT_A8R8G8B8,buffer,imgp->scalewidth,imgp->height,0,0,bpr);
				}

				if(imgp->height<imgp->scaleheight)
				{
					IMGScaleSmooth_Ybig(imgp,
										IMGFMT_A8R8G8B8,buffer,imgp->scalewidth,imgp->height,0,0,bpr,
										imgp->dstfmt,imgp->dstdata,imgp->scalewidth,imgp->scaleheight,imgp->dstx,imgp->dsty,imgp->dstbytesperrow);
				}
				else
				{
					IMGScaleSmooth_Ysmall(imgp,
										  IMGFMT_A8R8G8B8,buffer,imgp->scalewidth,imgp->height,0,0,bpr,
										  imgp->dstfmt,imgp->dstdata,imgp->scalewidth,imgp->scaleheight,imgp->dstx,imgp->dsty,imgp->dstbytesperrow);
				}
			}
		}
		else
		{
			TINT bpr=imgp->width*TImgBytesPerPel(imgp,IMGFMT_A8R8G8B8);
			buffer=TExecAlloc(imgp->exec,TNULL,bpr*imgp->scaleheight);
			if(!buffer)
				return TFALSE;
			else
			{
				if(imgp->height<imgp->scaleheight)
				{
					IMGScaleSmooth_Ybig(imgp,
										imgp->srcfmt,imgp->srcdata,imgp->width,imgp->height,imgp->srcx,imgp->srcy,imgp->srcbytesperrow,
										IMGFMT_A8R8G8B8,buffer,imgp->width,imgp->scaleheight,0,0,bpr);
				}
				else
				{
					IMGScaleSmooth_Ysmall(imgp,
										  imgp->srcfmt,imgp->srcdata,imgp->width,imgp->height,imgp->srcx,imgp->srcy,imgp->srcbytesperrow,
										  IMGFMT_A8R8G8B8,buffer,imgp->width,imgp->scaleheight,0,0,bpr);
				}

				if(imgp->width<imgp->scalewidth)
				{
					IMGScaleSmooth_Xbig(imgp,
										IMGFMT_A8R8G8B8,buffer,imgp->width,imgp->scaleheight,0,0,bpr,
										imgp->dstfmt,imgp->dstdata,imgp->scalewidth,imgp->scaleheight,imgp->dstx,imgp->dsty,imgp->dstbytesperrow);
				}
				else
				{
					IMGScaleSmooth_Xsmall(imgp,
										  IMGFMT_A8R8G8B8,buffer,imgp->width,imgp->scaleheight,0,0,bpr,
										  imgp->dstfmt,imgp->dstdata,imgp->scalewidth,imgp->scaleheight,imgp->dstx,imgp->dsty,imgp->dstbytesperrow);
				}
			}
		}
	}
	else
	{
		if(imgp->width<imgp->scalewidth)
		{
			IMGScaleSmooth_Xbig(imgp,
								imgp->srcfmt,imgp->srcdata,imgp->width,imgp->height,imgp->srcx,imgp->srcy,imgp->srcbytesperrow,
								imgp->dstfmt,imgp->dstdata,imgp->scalewidth,imgp->scaleheight,imgp->dstx,imgp->dsty,imgp->dstbytesperrow);
		}
		else if(imgp->width>imgp->scalewidth)
		{
			IMGScaleSmooth_Xsmall(imgp,
								  imgp->srcfmt,imgp->srcdata,imgp->width,imgp->height,imgp->srcx,imgp->srcy,imgp->srcbytesperrow,
								  imgp->dstfmt,imgp->dstdata,imgp->scalewidth,imgp->scaleheight,imgp->dstx,imgp->dsty,imgp->dstbytesperrow);
		}
		else if(imgp->height<imgp->scaleheight)
		{
			IMGScaleSmooth_Ybig(imgp,
								imgp->srcfmt,imgp->srcdata,imgp->width,imgp->height,imgp->srcx,imgp->srcy,imgp->srcbytesperrow,
								imgp->dstfmt,imgp->dstdata,imgp->scalewidth,imgp->scaleheight,imgp->dstx,imgp->dsty,imgp->dstbytesperrow);
		}
		else if(imgp->height>imgp->scaleheight)
		{
			IMGScaleSmooth_Ysmall(imgp,
								  imgp->srcfmt,imgp->srcdata,imgp->width,imgp->height,imgp->srcx,imgp->srcy,imgp->srcbytesperrow,
								  imgp->dstfmt,imgp->dstdata,imgp->scalewidth,imgp->scaleheight,imgp->dstx,imgp->dsty,imgp->dstbytesperrow);
		}
	}

	if(buffer)
		TExecFree(imgp->exec,buffer);

	return TTRUE;
}

TVOID IMGScaleSmooth_Xbig(TMOD_IMGPROC *imgp,
						  TINT srcfmt, TINT8 *srcdata, TINT srcwidth, TINT srcheight, TINT srcx, TINT srcy, TINT srcbytesperrow,
						  TINT dstfmt, TINT8 *dstdata, TINT dstwidth, TINT dstheight, TINT dstx, TINT dsty, TINT dstbytesperrow)
{
	TINT sx,sy,dest_x,x,subpix_xint;
	TINT aw,rw,gw,bw;
	TINT da,dr,dg,db;
	TINT delta_xrez, delta_xint;
	TFLOAT delta_x;
	TIMGARGBCOLOR col1,col2;

	TINT srcbpel=TImgBytesPerPel(imgp,srcfmt);
	TINT dstbpel=TImgBytesPerPel(imgp,dstfmt);

	TUINT8* src=srcdata+srcy*srcbytesperrow+srcx*srcbpel;
	TUINT8* dst=dstdata+dsty*dstbytesperrow+dstx*dstbpel;

	delta_x=(TFLOAT)dstwidth/(srcwidth-1);
	delta_xrez=(TINT)(65536.0f/delta_x);
	delta_xint=(TINT)(65536.0f*delta_x);
	subpix_xint=65536-(delta_xint & 0x0000ffff);

	for(sy=0;sy<srcheight;sy++)
	{
		IMG_GetPixel((TAPTR)(src),srcfmt, dstfmt, imgp->srcpalette, &col1);

		dest_x=0;
		for(sx=0;sx<srcwidth-1;sx++)
		{
			IMG_GetPixel((TAPTR)(src+(sx+1)*srcbpel),srcfmt, dstfmt, imgp->srcpalette, &col2);

			da = (col2.a-col1.a)*delta_xrez;
			dr = (col2.r-col1.r)*delta_xrez;
			dg = (col2.g-col1.g)*delta_xrez;
			db = (col2.b-col1.b)*delta_xrez;

			aw=col1.a<<16;
			rw=col1.r<<16;
			gw=col1.g<<16;
			bw=col1.b<<16;

			for(x=dest_x;x<dest_x+delta_xint;x+=65536)
			{
				col1.a=aw>>16;
				col1.r=rw>>16;
				col1.g=gw>>16;
				col1.b=bw>>16;
				IMG_PutPixel((TAPTR)(dst+(x>>16)*dstbpel),srcfmt, dstfmt, &col1);

				aw += da;
				rw += dr;
				gw += dg;
				bw += db;
			}
			dest_x=x-subpix_xint;

			col1.a=col2.a;
			col1.r=col2.r;
			col1.g=col2.g;
			col1.b=col2.b;
		}
		src+=srcbytesperrow;
		dst+=dstbytesperrow;
	}
}

TVOID IMGScaleSmooth_Ybig(TMOD_IMGPROC *imgp,
						  TINT srcfmt, TINT8 *srcdata, TINT srcwidth, TINT srcheight, TINT srcx, TINT srcy, TINT srcbytesperrow,
						  TINT dstfmt, TINT8 *dstdata, TINT dstwidth, TINT dstheight, TINT dstx, TINT dsty, TINT dstbytesperrow)
{
	TINT sx,sy,dest_y,y,subpix_yint;
	TINT aw,rw,gw,bw;
	TINT da,dr,dg,db;
	TINT delta_yrez, delta_yint;
	TFLOAT delta_y;
	TIMGARGBCOLOR col1,col2;

	TINT srcbpel=TImgBytesPerPel(imgp,srcfmt);
	TINT dstbpel=TImgBytesPerPel(imgp,dstfmt);

	TUINT8* src=srcdata+srcy*srcbytesperrow+srcx*srcbpel;
	TUINT8* dst=dstdata+dsty*dstbytesperrow+dstx*dstbpel;

	delta_y=(TFLOAT)dstheight/(srcheight-1);
	delta_yrez=(TINT)(65536.0f/delta_y);
	delta_yint=(TINT)(65536.0f*delta_y);
	subpix_yint=65536-(delta_yint & 0x0000ffff);

	for(sx=0;sx<srcwidth;sx++)
	{
		IMG_GetPixel((TAPTR)(src+sx*srcbpel),srcfmt, dstfmt, imgp->srcpalette, &col1);

		dest_y=0;
		for(sy=0;sy<srcheight-1;sy++)
		{
			IMG_GetPixel((TAPTR)(src+(sy+1)*srcbytesperrow+sx*srcbpel),srcfmt, dstfmt, imgp->srcpalette, &col2);

			da = (col2.a-col1.a)*delta_yrez;
			dr = (col2.r-col1.r)*delta_yrez;
			dg = (col2.g-col1.g)*delta_yrez;
			db = (col2.b-col1.b)*delta_yrez;

			aw=col1.a<<16;
			rw=col1.r<<16;
			gw=col1.g<<16;
			bw=col1.b<<16;

			for(y=dest_y;y<dest_y+delta_yint;y+=65536)
			{
				col1.a=aw>>16;
				col1.r=rw>>16;
				col1.g=gw>>16;
				col1.b=bw>>16;

				IMG_PutPixel((TAPTR)(dst+(y>>16)*dstbytesperrow+sx*dstbpel),srcfmt, dstfmt, &col1);

				aw += da;
				rw += dr;
				gw += dg;
				bw += db;
			}
			dest_y=y-subpix_yint;

			col1.a=col2.a;
			col1.r=col2.r;
			col1.g=col2.g;
			col1.b=col2.b;
		}
	}
}

TVOID IMGScaleSmooth_Xsmall(TMOD_IMGPROC *imgp,
							TINT srcfmt, TINT8 *srcdata, TINT srcwidth, TINT srcheight, TINT srcx, TINT srcy, TINT srcbytesperrow,
						    TINT dstfmt, TINT8 *dstdata, TINT dstwidth, TINT dstheight, TINT dstx, TINT dsty, TINT dstbytesperrow)
{
	TINT dx,dy,src_x,x,c,cc,subpix_xint;
	TINT delta_xint;
	TINT aw,rw,gw,bw;
	TFLOAT delta_x;
	TIMGARGBCOLOR col1;

	TINT srcbpel=TImgBytesPerPel(imgp,srcfmt);
	TINT dstbpel=TImgBytesPerPel(imgp,dstfmt);

	TUINT8* src=srcdata+srcy*srcbytesperrow+srcx*srcbpel;
	TUINT8* dst=dstdata+dsty*dstbytesperrow+dstx*dstbpel;

	delta_x=(TFLOAT)(srcwidth-1)/dstwidth;
	delta_xint=(TINT)(65536.0f*delta_x);

	subpix_xint=65536-(delta_xint & 0x0000ffff);

	for(dy=0;dy<dstheight;dy++)
	{
		src_x=0;
		for(dx=0;dx<dstwidth;dx++)
		{
			aw=0;
			rw=0;
			gw=0;
			bw=0;
			c=0;
			for(x=src_x;x<src_x+delta_xint;x+=65536)
			{
				IMG_GetPixel((TAPTR)(src+(x>>16)*srcbpel),srcfmt, dstfmt, imgp->srcpalette, &col1);
				aw+=col1.a;
				rw+=col1.r;
				gw+=col1.g;
				bw+=col1.b;
				c++;
			}
			src_x=x-subpix_xint;

			cc=65536/c;
			col1.a=(aw*cc)>>16;
			col1.r=(rw*cc)>>16;
			col1.g=(gw*cc)>>16;
			col1.b=(bw*cc)>>16;
			IMG_PutPixel((TAPTR)(dst+dx*dstbpel),srcfmt, dstfmt, &col1);
		}
		src+=srcbytesperrow;
		dst+=dstbytesperrow;
	}
}

TVOID IMGScaleSmooth_Ysmall(TMOD_IMGPROC *imgp,
							TINT srcfmt, TINT8 *srcdata, TINT srcwidth, TINT srcheight, TINT srcx, TINT srcy, TINT srcbytesperrow,
						    TINT dstfmt, TINT8 *dstdata, TINT dstwidth, TINT dstheight, TINT dstx, TINT dsty, TINT dstbytesperrow)
{
	TINT dx,dy,src_y,y,c,cc,subpix_yint;
	TINT delta_yint;
	TINT aw,rw,gw,bw;
	TFLOAT delta_y;
	TIMGARGBCOLOR col1;

	TINT srcbpel=TImgBytesPerPel(imgp,srcfmt);
	TINT dstbpel=TImgBytesPerPel(imgp,dstfmt);

	TUINT8* src=srcdata+srcy*srcbytesperrow+srcx*srcbpel;
	TUINT8* dst=dstdata+dsty*dstbytesperrow+dstx*dstbpel;

	delta_y=(TFLOAT)(srcheight-1)/dstheight;
	delta_yint=(TINT)(65536.0f*delta_y);
	subpix_yint=65536-(delta_yint & 0x0000ffff);

	for(dx=0;dx<dstwidth;dx++)
	{
		src_y=0;
		for(dy=0;dy<dstheight;dy++)
		{
			aw=0;
			rw=0;
			gw=0;
			bw=0;
			c=0;
			for(y=src_y;y<src_y+delta_yint;y+=65536)
			{
				IMG_GetPixel((TAPTR)(src+(y>>16)*srcbytesperrow+dx*srcbpel),srcfmt, dstfmt, imgp->srcpalette, &col1);
				aw+=col1.a;
				rw+=col1.r;
				gw+=col1.g;
				bw+=col1.b;
				c++;
			}
			src_y=y-subpix_yint;

			cc=65536/c;
			col1.a=(aw*cc)>>16;
			col1.r=(rw*cc)>>16;
			col1.g=(gw*cc)>>16;
			col1.b=(bw*cc)>>16;
			IMG_PutPixel((TAPTR)(dst+dy*dstbytesperrow+dx*dstbpel),srcfmt, dstfmt, &col1);
		}
	}
}
