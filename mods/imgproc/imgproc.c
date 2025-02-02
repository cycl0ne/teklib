
#include "imgproc.h"
#include <tek/teklib.h>

#define MOD_VERSION 	0
#define MOD_REVISION	1

/*
**	module prototypes
*/
TMODAPI TBOOL TImgDoMethod(TMOD_IMGPROC *imgp, TIMGPICTURE *src, TIMGPICTURE *dst, TINT method, TTAGITEM *taglist);
TMODAPI TINT  TImgBytesPerPel(TMOD_IMGPROC *imgp, TINT format);
TMODAPI TUINT TImgColToFmt(TMOD_IMGPROC *imgp, TIMGARGBCOLOR *col, TINT format);
TMODAPI TVOID TImgFill(TMOD_IMGPROC *imgp, TIMGPICTURE *pic,TIMGARGBCOLOR *col);
TMODAPI TVOID TImgPlot(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT x, TINT y, TIMGARGBCOLOR *col);
TMODAPI TVOID TImgLine(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT sx, TINT sy, TINT ex, TINT ey,TIMGARGBCOLOR *col);
TMODAPI TVOID TImgBox(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT sx, TINT sy, TINT w, TINT h,TIMGARGBCOLOR *col);
TMODAPI TVOID TImgBoxf(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT sx, TINT sy, TINT w, TINT h,TIMGARGBCOLOR *col);
TMODAPI TVOID TImgEllipse(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry,TIMGARGBCOLOR *col);
TMODAPI TVOID TImgEllipsef(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry,TIMGARGBCOLOR *col);
TMODAPI TVOID TImgPoly(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT numpoints, TINT *points, TIMGARGBCOLOR *col);
TMODAPI TVOID TImgPolyf(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT numpoints, TINT *points, TIMGARGBCOLOR *col);
TMODAPI TIMGPICTURE* TImgAllocBitmap(TMOD_IMGPROC *imgp, TINT width, TINT height, TINT format);
TMODAPI TVOID TImgFreeBitmap(TMOD_IMGPROC *imgp, TIMGPICTURE *p);
TMODAPI TVOID TImgPoint(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT x, TINT y, TIMGARGBCOLOR *col);

static TCALLBACK TMOD_IMGPROC *mod_open(TMOD_IMGPROC *imgp, TAPTR selftask, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_IMGPROC *imgp, TAPTR selftask);

/*
**	private prototypes
*/
TVOID IMGP_Clip(TMOD_IMGPROC *imgp,TINT method);
TBOOL IMGP_Convert(TMOD_IMGPROC *imgp);


/*
**	tek_init_<modname>
**	all initializations that are not instance specific.
*/
TMODENTRY TUINT tek_init_imgproc(TAPTR selftask, TMOD_IMGPROC *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)					/* first call */
		{
			if (version <= MOD_VERSION)			/* version check */
			{
				return sizeof(TMOD_IMGPROC);	/* return module positive size */
			}
		}
		else									/* second call */
		{
			return sizeof(TAPTR) * 16;			/* return module negative size */
		}
	}
	else										/* third call */
	{
		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;

		/* this module has instances. place instance
		** open/close functions into the module structure. */
		mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;

		/* put module vectors in front */
		((TAPTR *) mod)[-1 ] = (TAPTR) TImgDoMethod;
		((TAPTR *) mod)[-2 ] = (TAPTR) TImgBytesPerPel;
		((TAPTR *) mod)[-3 ] = (TAPTR) TImgColToFmt;
		((TAPTR *) mod)[-4 ] = (TAPTR) TImgFill;
		((TAPTR *) mod)[-5 ] = (TAPTR) TImgPlot;
		((TAPTR *) mod)[-6 ] = (TAPTR) TImgLine;
		((TAPTR *) mod)[-7 ] = (TAPTR) TImgBox;
		((TAPTR *) mod)[-8 ] = (TAPTR) TImgBoxf;
		((TAPTR *) mod)[-9 ] = (TAPTR) TImgEllipse;
		((TAPTR *) mod)[-10] = (TAPTR) TImgEllipsef;
		((TAPTR *) mod)[-11] = (TAPTR) TImgPoly;
		((TAPTR *) mod)[-12] = (TAPTR) TImgPolyf;
		((TAPTR *) mod)[-13] = (TAPTR) TImgAllocBitmap;
		((TAPTR *) mod)[-14] = (TAPTR) TImgFreeBitmap;
		((TAPTR *) mod)[-15] = (TAPTR) TImgPoint;

		return TTRUE;
	}
	return 0;
}

/*
**	open instance
*/
static TCALLBACK TMOD_IMGPROC *mod_open(TMOD_IMGPROC *imgp, TAPTR selftask, TTAGITEM *tags)
{
	imgp = TNewInstance(imgp, imgp->module.tmd_PosSize, imgp->module.tmd_NegSize);
	if(!imgp) return TNULL;

	imgp->exec = TGetExecBase(imgp);

	/* allocating the linebuffer needed by polyf */
	imgp->oldpolyf_h=512;
	imgp->polyfbuffer=(TINT*)TExecAlloc0(imgp->exec,TNULL,2*sizeof(TINT)+imgp->oldpolyf_h*2*sizeof(TINT));

	return imgp;
}

/*
**	close instance
*/
static TCALLBACK TVOID mod_close(TMOD_IMGPROC *imgp, TAPTR selftask)
{
	if(imgp->polyfbuffer)
	{
		TExecFree(imgp->exec,imgp->polyfbuffer);
		imgp->polyfbuffer=TNULL;
	}
	TFreeInstance(imgp);
}

/**************************************************************************
	TImgDoMethod
 **************************************************************************/
TMODAPI TBOOL TImgDoMethod(TMOD_IMGPROC *imgp, TIMGPICTURE *src, TIMGPICTURE *dst, TINT method, TTAGITEM *taglist)
{
	/* make a local copy of all data */
	imgp->srcdata=src->data;
	imgp->srcpalette=src->palette;
	imgp->srcfmt=src->format;
	imgp->srcwidth=src->width;
	imgp->srcheight=src->height;
	imgp->srcdepth=src->depth;
	imgp->srcbytesperrow=src->bytesperrow;

	imgp->dstdata=dst->data;
	imgp->dstpalette=dst->palette;
	imgp->dstfmt=dst->format;
	imgp->dstwidth=dst->width;
	imgp->dstheight=dst->height;
	imgp->dstdepth=dst->depth;
	imgp->dstbytesperrow=dst->bytesperrow;

	/* retrieve the standard method-tags */
	imgp->srcx=0;
	imgp->srcx=0;
	imgp->srcy=0;
	imgp->dstx=0;
	imgp->dsty=0;
	imgp->width=src->width - imgp->srcx;
	imgp->height=src->height - imgp->srcy;

	imgp->scalewidth=dst->width - imgp->dstx;
	imgp->scaleheight=dst->height - imgp->dsty;

	imgp->scalemethod=IMGSMT_HARD;

	if(taglist)
	{
		imgp->srcx=(TINT)TGetTag(taglist,IMGTAG_SRCX,(TTAG)0);
		imgp->srcy=(TINT)TGetTag(taglist,IMGTAG_SRCY,(TTAG)0);
		imgp->dstx=(TINT)TGetTag(taglist,IMGTAG_DSTX,(TTAG)0);
		imgp->dsty=(TINT)TGetTag(taglist,IMGTAG_DSTY,(TTAG)0);
		imgp->width=(TINT)TGetTag(taglist,IMGTAG_WIDTH,(TTAG)(src->width - imgp->srcx));
		imgp->height=(TINT)TGetTag(taglist,IMGTAG_HEIGHT,(TTAG)(src->height - imgp->srcy));

		/* only relevant for scaling */
		imgp->scalewidth=(TINT)TGetTag(taglist,IMGTAG_SCALEWIDTH,(TTAG)(dst->width - imgp->dstx));
		imgp->scaleheight=(TINT)TGetTag(taglist,IMGTAG_SCALEHEIGHT,(TTAG)(dst->height - imgp->dsty));

		imgp->scalemethod=(TINT)TGetTag(taglist,IMGTAG_SCALEMETHOD,(TTAG)IMGSMT_HARD);
	}

	/* go */
	IMGP_Clip(imgp,method);
	switch(method)
	{
		case IMGMT_CONVERT:
			return IMGP_Convert(imgp);
		break;

		case IMGMT_SCALE:
			if(imgp->scalewidth<=0 || imgp->scaleheight<=0)
				return TFALSE;
			else if(imgp->scalewidth==imgp->width && imgp->scaleheight==imgp->height)
			{
				if(imgp->srcfmt == imgp->dstfmt)
				{
					IMGBlitNormal(imgp);
					return TTRUE;
				}
				else
					return IMGP_Convert(imgp);
			}
			else
			{
				switch(imgp->scalemethod)
				{
					case IMGSMT_HARD:
						IMGScale_Hard(imgp);
						return TTRUE;
					break;

					case IMGSMT_SMOOTH:
						return IMGScale_Smooth(imgp);
					break;
				}
			}
		break;

		case IMGMT_ENDIANSWAP:
			IMGEndianSwap(imgp);
		break;

		case IMGMT_BLIT:
			IMGBlitNormal(imgp);
		break;
	}
	return TTRUE;
}

/**************************************************************************
	TImgBytesPerPel
 **************************************************************************/
TMODAPI TINT TImgBytesPerPel(TMOD_IMGPROC *imgp, TINT format)
{
	switch(format)
	{
		case IMGFMT_CLUT:
			return 1;
		break;

		case IMGFMT_R5G5B5: case IMGFMT_R5G6B5:
			return 2;
		break;

		case IMGFMT_R8G8B8: case IMGFMT_B8G8R8:
			return 3;
		break;

		case IMGFMT_A8R8G8B8: case IMGFMT_R8G8B8A8:	case IMGFMT_B8G8R8A8:
			return 4;
		break;
	}
	return 0;
}

/**************************************************************************
	TImgColToFmt
 **************************************************************************/
TMODAPI TUINT TImgColToFmt(TMOD_IMGPROC *imgp, TIMGARGBCOLOR *col, TINT format)
{
	switch(format)
	{
		case IMGFMT_CLUT:
			return (TUINT)col->b;
		break;

		case IMGFMT_R5G5B5:
			return (TUINT)(((col->a & 0x80)<<8) | ((col->r & 0xf8)<<7) | ((col->g & 0xf8) << 2) | ((col->b & 0xf8) >> 3));
		break;

		case IMGFMT_R5G6B5:
			return (TUINT)(((col->r & 0xf8)<<8) | ((col->g & 0xfc) << 3) | ((col->b & 0xf8) >> 3));
		break;

		case IMGFMT_R8G8B8:
			return (TUINT)((col->r <<16) | (col->g << 8) | col->b);
		break;

		case IMGFMT_B8G8R8:
			return (TUINT)((col->b <<16) | (col->g << 8) | col->r);
		break;

		case IMGFMT_A8R8G8B8:
			return (TUINT)((col->a <<24) | (col->r <<16) | (col->g << 8) | col->b);
		break;

		case IMGFMT_R8G8B8A8:
			return (TUINT)((col->r <<24) | (col->g <<16) | (col->b << 8) | col->a);
		break;

		case IMGFMT_B8G8R8A8:
			return (TUINT)((col->b <<24) | (col->g <<16) | (col->r << 8) | col->a);
		break;
	}
	return 0;
}

/**************************************************************************
	TImgFill
 **************************************************************************/
TMODAPI TVOID TImgFill(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TIMGARGBCOLOR *col)
{
	switch(pic->format)
	{
		case IMGFMT_CLUT:
			IMGFill_8(pic,col->b);
		break;

		case IMGFMT_R5G5B5:
			IMGFill_16(pic, TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R5G6B5:
			IMGFill_16(pic, TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8:
			IMGFill_24(pic,col->r,col->g,col->b);
		break;

		case IMGFMT_B8G8R8:
			IMGFill_24(pic,col->b,col->g,col->r);
		break;

		case IMGFMT_A8R8G8B8:
			IMGFill_32(pic,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8A8:
			IMGFill_32(pic,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_B8G8R8A8:
			IMGFill_32(pic,TImgColToFmt(imgp,col,pic->format));
		break;
	}
}

/**************************************************************************
	TImgPlot
 **************************************************************************/
TMODAPI TVOID TImgPlot(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT x, TINT y, TIMGARGBCOLOR *col)
{
	switch(pic->format)
	{
		case IMGFMT_CLUT:
		{
			TUINT8 *dst=(TUINT8*)pic->data + y*pic->bytesperrow + x;
			*dst=col->b;
		}
		break;

		case IMGFMT_R5G5B5:
		{
			TUINT16 *dst=(TUINT16*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
			*dst=TImgColToFmt(imgp,col,pic->format);
		}
		break;

		case IMGFMT_R5G6B5:
		{
			TUINT16 *dst=(TUINT16*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
			*dst=TImgColToFmt(imgp,col,pic->format);
		}
		break;

		case IMGFMT_R8G8B8:
		{
			TUINT8 *dst=(TUINT8*)pic->data + y*pic->bytesperrow + x*3;
			dst[0]=col->r;
			dst[1]=col->g;
			dst[2]=col->b;
		}
		break;

		case IMGFMT_B8G8R8:
		{
			TUINT8 *dst=(TUINT8*)pic->data + y*pic->bytesperrow + x*3;
			dst[0]=col->b;
			dst[1]=col->g;
			dst[2]=col->r;
		}
		break;

		case IMGFMT_A8R8G8B8:
		{
			TUINT *dst=(TUINT*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
			*dst=TImgColToFmt(imgp,col,pic->format);
		}
		break;

		case IMGFMT_R8G8B8A8:
		{
			TUINT *dst=(TUINT*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
			*dst=TImgColToFmt(imgp,col,pic->format);
		}
		break;

		case IMGFMT_B8G8R8A8:
		{
			TUINT *dst=(TUINT*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
			*dst=TImgColToFmt(imgp,col,pic->format);
		}
		break;
	}
}

/**************************************************************************
	TImgLine
 **************************************************************************/
TMODAPI TVOID TImgLine(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT sx, TINT sy, TINT ex, TINT ey,TIMGARGBCOLOR *col)
{
	switch(pic->format)
	{
		case IMGFMT_CLUT:
			IMGLine_8(pic,sx,sy,ex,ey,col->b);
		break;

		case IMGFMT_R5G5B5:
			IMGLine_16(pic,sx,sy,ex,ey,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R5G6B5:
			IMGLine_16(pic,sx,sy,ex,ey,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8:
			IMGLine_24(pic,sx,sy,ex,ey,col->r,col->g,col->b);
		break;

		case IMGFMT_B8G8R8:
			IMGLine_24(pic,sx,sy,ex,ey,col->b,col->g,col->r);
		break;

		case IMGFMT_A8R8G8B8:
			IMGLine_32(pic,sx,sy,ex,ey,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8A8:
			IMGLine_32(pic,sx,sy,ex,ey,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_B8G8R8A8:
			IMGLine_32(pic,sx,sy,ex,ey,TImgColToFmt(imgp,col,pic->format));
		break;
	}
}

/**************************************************************************
	TImgBox
 **************************************************************************/
TMODAPI TVOID TImgBox(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT sx, TINT sy, TINT w, TINT h,TIMGARGBCOLOR *col)
{
	switch(pic->format)
	{
		case IMGFMT_CLUT:
			IMGBox_8(pic,sx,sy,w,h,col->b);
		break;

		case IMGFMT_R5G5B5:
			IMGBox_16(pic,sx,sy,w,h, TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R5G6B5:
			IMGBox_16(pic,sx,sy,w,h, TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8:
			IMGBox_24(pic,sx,sy,w,h,col->r,col->g,col->b);
		break;

		case IMGFMT_B8G8R8:
			IMGBox_24(pic,sx,sy,w,h,col->b,col->g,col->r);
		break;

		case IMGFMT_A8R8G8B8:
			IMGBox_32(pic,sx,sy,w,h,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8A8:
			IMGBox_32(pic,sx,sy,w,h,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_B8G8R8A8:
			IMGBox_32(pic,sx,sy,w,h,TImgColToFmt(imgp,col,pic->format));
		break;
	}
}

/**************************************************************************
	TImgBoxf
 **************************************************************************/
TMODAPI TVOID TImgBoxf(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT sx, TINT sy, TINT w, TINT h,TIMGARGBCOLOR *col)
{
	switch(pic->format)
	{
		case IMGFMT_CLUT:
			IMGBoxf_8(pic,sx,sy,w,h,col->b);
		break;

		case IMGFMT_R5G5B5:
			IMGBoxf_16(pic,sx,sy,w,h, TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R5G6B5:
			IMGBoxf_16(pic,sx,sy,w,h, TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8:
			IMGBoxf_24(pic,sx,sy,w,h,col->r,col->g,col->b);
		break;

		case IMGFMT_B8G8R8:
			IMGBoxf_24(pic,sx,sy,w,h,col->b,col->g,col->r);
		break;

		case IMGFMT_A8R8G8B8:
			IMGBoxf_32(pic,sx,sy,w,h,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8A8:
			IMGBoxf_32(pic,sx,sy,w,h,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_B8G8R8A8:
			IMGBoxf_32(pic,sx,sy,w,h,TImgColToFmt(imgp,col,pic->format));
		break;
	}
}

/**************************************************************************
	TImgEllipse
 **************************************************************************/
TMODAPI TVOID TImgEllipse(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry,TIMGARGBCOLOR *col)
{
	switch(pic->format)
	{
		case IMGFMT_CLUT:
			IMGEllipse_8(pic,x,y,rx,ry,col->b);
		break;

		case IMGFMT_R5G5B5:
			IMGEllipse_16(pic,x,y,rx,ry, TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R5G6B5:
			IMGEllipse_16(pic,x,y,rx,ry, TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8:
			IMGEllipse_24(pic,x,y,rx,ry,col->r,col->g,col->b);
		break;

		case IMGFMT_B8G8R8:
			IMGEllipse_24(pic,x,y,rx,ry,col->b,col->g,col->r);
		break;

		case IMGFMT_A8R8G8B8:
			IMGEllipse_32(pic,x,y,rx,ry,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8A8:
			IMGEllipse_32(pic,x,y,rx,ry,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_B8G8R8A8:
			IMGEllipse_32(pic,x,y,rx,ry,TImgColToFmt(imgp,col,pic->format));
		break;
	}
}

/**************************************************************************
	TImgEllipse
 **************************************************************************/
TMODAPI TVOID TImgEllipsef(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry,TIMGARGBCOLOR *col)
{
	switch(pic->format)
	{
		case IMGFMT_CLUT:
			IMGEllipsef_8(pic,x,y,rx,ry,col->b);
		break;

		case IMGFMT_R5G5B5:
			IMGEllipsef_16(pic,x,y,rx,ry, TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R5G6B5:
			IMGEllipsef_16(pic,x,y,rx,ry, TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8:
			IMGEllipsef_24(pic,x,y,rx,ry,col->r,col->g,col->b);
		break;

		case IMGFMT_B8G8R8:
			IMGEllipsef_24(pic,x,y,rx,ry,col->b,col->g,col->r);
		break;

		case IMGFMT_A8R8G8B8:
			IMGEllipsef_32(pic,x,y,rx,ry,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8A8:
			IMGEllipsef_32(pic,x,y,rx,ry,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_B8G8R8A8:
			IMGEllipsef_32(pic,x,y,rx,ry,TImgColToFmt(imgp,col,pic->format));
		break;
	}
}

/**************************************************************************
	TImgPoly
 **************************************************************************/
TMODAPI TVOID TImgPoly(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT numpoints, TINT *points, TIMGARGBCOLOR *col)
{
	switch(pic->format)
	{
		case IMGFMT_CLUT:
			IMGPoly_8(pic,numpoints,points,col->b);
		break;

		case IMGFMT_R5G5B5:
			IMGPoly_16(pic,numpoints,points,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R5G6B5:
			IMGPoly_16(pic,numpoints,points,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8:
			IMGPoly_24(pic,numpoints,points,col->r,col->g,col->b);
		break;

		case IMGFMT_B8G8R8:
			IMGPoly_24(pic,numpoints,points,col->b,col->g,col->r);
		break;

		case IMGFMT_A8R8G8B8:
			IMGPoly_32(pic,numpoints,points,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_R8G8B8A8:
			IMGPoly_32(pic,numpoints,points,TImgColToFmt(imgp,col,pic->format));
		break;

		case IMGFMT_B8G8R8A8:
			IMGPoly_32(pic,numpoints,points,TImgColToFmt(imgp,col,pic->format));
		break;
	}
}

/**************************************************************************
	TImgPolyf
 **************************************************************************/
TMODAPI TVOID TImgPolyf(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT numpoints, TINT *points, TIMGARGBCOLOR *col)
{
	/* reallocate linebuffer if needed */
	if(pic->height>imgp->oldpolyf_h)
	{
		imgp->oldpolyf_h=pic->height;
		TExecFree(imgp->exec,imgp->polyfbuffer);
		imgp->polyfbuffer=(TINT*)TExecAlloc(imgp->exec,TNULL,2*sizeof(TINT)+imgp->oldpolyf_h*2*sizeof(TINT));
	}

	switch(pic->format)
	{
		case IMGFMT_CLUT:
			IMGPolyf_8(pic,numpoints,points,col->b,imgp->polyfbuffer);
		break;

		case IMGFMT_R5G5B5:
			IMGPolyf_16(pic,numpoints,points, TImgColToFmt(imgp,col,pic->format),imgp->polyfbuffer);
		break;

		case IMGFMT_R5G6B5:
			IMGPolyf_16(pic,numpoints,points, TImgColToFmt(imgp,col,pic->format),imgp->polyfbuffer);
		break;

		case IMGFMT_R8G8B8:
			IMGPolyf_24(pic,numpoints,points,col->r,col->g,col->b,imgp->polyfbuffer);
		break;

		case IMGFMT_B8G8R8:
			IMGPolyf_24(pic,numpoints,points,col->b,col->g,col->r,imgp->polyfbuffer);
		break;

		case IMGFMT_A8R8G8B8:
			IMGPolyf_32(pic,numpoints,points,TImgColToFmt(imgp,col,pic->format),imgp->polyfbuffer);
		break;

		case IMGFMT_R8G8B8A8:
			IMGPolyf_32(pic,numpoints,points,TImgColToFmt(imgp,col,pic->format),imgp->polyfbuffer);
		break;

		case IMGFMT_B8G8R8A8:
			IMGPolyf_32(pic,numpoints,points,TImgColToFmt(imgp,col,pic->format),imgp->polyfbuffer);
		break;
	}
}

/**************************************************************************
	TImgAllocBitmap
 **************************************************************************/
TMODAPI TIMGPICTURE* TImgAllocBitmap(TMOD_IMGPROC *imgp, TINT width, TINT height, TINT format)
{
	TIMGPICTURE *p=TNULL;

	p=TExecAlloc0(imgp->exec,TNULL,sizeof(TIMGPICTURE));
	if(!p) return TNULL;

	p->width=width;
	p->height=height;
	p->format=format;

	switch(format)
	{
		case IMGFMT_CLUT:
			p->depth=8;
			p->bytesperrow=width;
		break;

		case IMGFMT_R5G5B5:
			p->depth=16;
			p->bytesperrow=width*2;
		break;

		case IMGFMT_R5G6B5:
			p->depth=16;
			p->bytesperrow=width*2;
		break;

		case IMGFMT_R8G8B8:
			p->depth=24;
			p->bytesperrow=width*3;
		break;

		case IMGFMT_B8G8R8:
			p->depth=24;
			p->bytesperrow=width*3;
		break;

		case IMGFMT_A8R8G8B8:
			p->depth=32;
			p->bytesperrow=width*4;
		break;

		case IMGFMT_R8G8B8A8:
			p->depth=32;
			p->bytesperrow=width*4;
		break;

		case IMGFMT_B8G8R8A8:
			p->depth=32;
			p->bytesperrow=width*4;
		break;
	}

	p->data=TExecAlloc0(imgp->exec,TNULL,p->bytesperrow*p->height);
	if(!p->data)
	{
		TExecFree(imgp->exec,p);
		return TNULL;
	}

	if(p->format==IMGFMT_CLUT)
	{
		p->palette=TExecAlloc0(imgp->exec,TNULL,sizeof(TIMGARGBCOLOR)*256);
		if(!p->palette)
		{
			TExecFree(imgp->exec,p->data);
			TExecFree(imgp->exec,p);
			return TNULL;
		}
	}
	return p;
}

/**************************************************************************
	TImgFreeBitmap
 **************************************************************************/
TMODAPI TVOID TImgFreeBitmap(TMOD_IMGPROC *imgp, TIMGPICTURE *p)
{
	if(p)
	{
		if(p->data)
			TExecFree(imgp->exec,p->data);

		if(p->palette)
			TExecFree(imgp->exec,p->palette);

		TExecFree(imgp->exec,p);
	}
}

/**************************************************************************
	TImgPoint
 **************************************************************************/
TMODAPI TVOID TImgPoint(TMOD_IMGPROC *imgp, TIMGPICTURE *pic, TINT x, TINT y, TIMGARGBCOLOR *col)
{
	switch(pic->format)
	{
		case IMGFMT_CLUT:
		{
			TUINT8 *src=(TUINT8*)pic->data + y*pic->bytesperrow + x;
			col->a=0;
			col->r=0;
			col->g=0;
			col->b=*src;
		}
		break;

		case IMGFMT_R5G5B5:
		{
			TUINT16 *src=(TUINT16*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
			col->a = (*src & 0x8000)>>8;
			col->r = (*src & 0x7c00)>>7;
			col->g = (*src & 0x03e0)>>2;
			col->b = (*src & 0x001f)<<3;
		}
		break;

		case IMGFMT_R5G6B5:
		{
			TUINT16 *src=(TUINT16*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
			col->a = 0;
			col->r = (*src & 0xf800)>>8;
			col->g = (*src & 0x07e0)>>3;
			col->b = (*src & 0x001f)<<3;
		}
		break;

		case IMGFMT_R8G8B8:
		{
			TUINT8 *src=(TUINT8*)pic->data + y*pic->bytesperrow + x*3;
			col->a = 0;
			col->r = src[0];
			col->g = src[1];
			col->b = src[2];
		}
		break;

		case IMGFMT_B8G8R8:
		{
			TUINT8 *src=(TUINT8*)pic->data + y*pic->bytesperrow + x*3;
			col->a = 0;
			col->b = src[0];
			col->g = src[1];
			col->r = src[2];
		}
		break;

		case IMGFMT_A8R8G8B8:
		{
			TUINT *src=(TUINT*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
			col->a = (*src & 0xff000000)>>24;
			col->r = (*src & 0x00ff0000)>>16;
			col->g = (*src & 0x0000ff00)>>8;
			col->b = (*src & 0x000000ff);
		}
		break;

		case IMGFMT_R8G8B8A8:
		{
			TUINT *src=(TUINT*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
			col->r = (*src & 0xff000000)>>24;
			col->g = (*src & 0x00ff0000)>>16;
			col->b = (*src & 0x0000ff00)>>8;
			col->a = (*src & 0x000000ff);
		}
		break;

		case IMGFMT_B8G8R8A8:
		{
			TUINT *src=(TUINT*)((TUINT8*)pic->data + y*pic->bytesperrow) + x;
			col->b = (*src & 0xff000000)>>24;
			col->g = (*src & 0x00ff0000)>>16;
			col->r = (*src & 0x0000ff00)>>8;
			col->a = (*src & 0x000000ff);
		}
		break;
	}
}

/**************************************************************************
 **************************************************************************

	private routines

 **************************************************************************
 **************************************************************************/

/**************************************************************************
	IMGP_Clip
 **************************************************************************/
TVOID IMGP_Clip(TMOD_IMGPROC *imgp, TINT method)
{
	if(imgp->srcx+imgp->width > imgp->srcwidth)
		imgp->width = imgp->srcwidth-imgp->srcx;

	if(imgp->srcy+imgp->height > imgp->srcheight)
		imgp->height = imgp->srcheight-imgp->srcy;

	if(method==IMGMT_CONVERT)
	{
		if(imgp->dstx+imgp->width > imgp->dstwidth)
			imgp->width = imgp->dstwidth-imgp->dstx;

		if(imgp->dsty+imgp->height > imgp->dstheight)
			imgp->height = imgp->dstheight-imgp->dsty;
	}
}

/**************************************************************************
	IMGP_Convert
 **************************************************************************/
TBOOL IMGP_Convert(TMOD_IMGPROC *imgp)
{
	TBOOL done=TFALSE;

	if(imgp->srcfmt==IMGFMT_PLANAR)
	{
		/* for planar src only full conversions are allowed  */
		if(imgp->srcx!=0 || imgp->srcy!=0 || imgp->srcwidth!=imgp->dstwidth || imgp->srcheight!=imgp->dstheight)
			return TFALSE;

		/* convert to temporary chunky buffer, if needed */
		if(imgp->srcdepth<=8)
		{
			if(imgp->dstfmt!=IMGFMT_PLANAR && imgp->dstfmt!=IMGFMT_CLUT)
			{
				TUINT8 *swp;

				swp=imgp->dstdata;
				imgp->dstdata=(TUINT8*)TExecAlloc0(imgp->exec,TNULL,imgp->srcwidth*imgp->srcheight);

				IMGConv_PLANAR_2_CHUNKY(imgp);

				imgp->srcdata=imgp->dstdata;
				imgp->tmpbuffer=imgp->dstdata;
				imgp->dstdata=swp;

				imgp->srcfmt=IMGFMT_CLUT;
				imgp->srcbytesperrow=imgp->srcwidth;

				done=TFALSE;
			}
			else if(imgp->dstfmt==IMGFMT_CLUT)
			{
				IMGConv_PLANAR_2_CHUNKY(imgp);
				imgp->srcfmt=IMGFMT_CLUT;

				done=TTRUE;
			}
		}
		else if(imgp->srcdepth==24)
		{
			if(imgp->dstfmt!=IMGFMT_PLANAR && imgp->dstfmt!=IMGFMT_R8G8B8)
			{
				TUINT8 *swp;

				swp=imgp->dstdata;
				imgp->dstdata=(TUINT8*)TExecAlloc0(imgp->exec,NULL,imgp->srcwidth*3*imgp->srcheight);

				IMGConv_PLANAR_2_CHUNKY(imgp);

				imgp->srcdata=imgp->dstdata;
				imgp->tmpbuffer=imgp->dstdata;
				imgp->dstdata=swp;

				imgp->srcfmt=IMGFMT_R8G8B8;
				imgp->srcbytesperrow=imgp->srcwidth*3;

				done=TTRUE;
			}
			else if(imgp->dstfmt==IMGFMT_R8G8B8)
			{
				IMGConv_PLANAR_2_CHUNKY(imgp);
				imgp->srcfmt=IMGFMT_R8G8B8;

				done=TTRUE;
			}
		}
	}

	/* ------------------------------------------------------------------
			convert
	   ------------------------------------------------------------------ */
	if(!done)
	{
		switch(imgp->dstfmt)
		{
			case IMGFMT_PLANAR:
			break;

			case IMGFMT_CLUT:
			break;

			case IMGFMT_R5G5B5:
				IMGConv_2_R5G5B5(imgp);
			break;

			case IMGFMT_R5G6B5:
				IMGConv_2_R5G6B5(imgp);
			break;

			case IMGFMT_R8G8B8:
				IMGConv_2_R8G8B8(imgp);
			break;

			case IMGFMT_A8R8G8B8:
				IMGConv_2_A8R8G8B8(imgp);
			break;

			case IMGFMT_R8G8B8A8:
				IMGConv_2_R8G8B8A8(imgp);
			break;

			case IMGFMT_B8G8R8:
				IMGConv_2_B8G8R8(imgp);
			break;

			case IMGFMT_B8G8R8A8:
				IMGConv_2_B8G8R8A8(imgp);
			break;
		}
	}

	if(imgp->tmpbuffer)
	{
		TExecFree(imgp->exec,imgp->tmpbuffer);
		imgp->tmpbuffer=TNULL;
	}
	return TTRUE;
}
