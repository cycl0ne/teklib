
#include <tek/exec.h>
#include <tek/proto/exec.h>
#include <tek/mod/imgproc.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
**	imgproc module
*/
typedef struct _TModImgProc
{
	TMODL module;						/* module header */
	TAPTR exec;

	TINT srcfmt,srcx,srcy,srcwidth,srcheight,srcdepth,srcbytesperrow;
	TINT dstfmt,dstx,dsty,dstwidth,dstheight,dstdepth,dstbytesperrow;
	TINT width,height;
	TUINT8 *srcdata;
	TIMGARGBCOLOR *srcpalette;
	TUINT8 *dstdata;
	TIMGARGBCOLOR *dstpalette;
	
	TINT scalewidth,scaleheight,scalemethod;

	TUINT8 *tmpbuffer;

	TINT oldpolyf_h;
	TINT *polyfbuffer;

} TMOD_IMGPROC;

TVOID IMGConv_PLANAR_2_CHUNKY(TMOD_IMGPROC *imgp);
TVOID IMGConv_PLANARINT_2_CHUNKY(TMOD_IMGPROC *imgp);
TVOID IMGConv_2_R5G5B5(TMOD_IMGPROC *imgp);
TVOID IMGConv_2_R5G6B5(TMOD_IMGPROC *imgp);
TVOID IMGConv_2_R8G8B8(TMOD_IMGPROC *imgp);
TVOID IMGConv_2_A8R8G8B8(TMOD_IMGPROC *imgp);
TVOID IMGConv_2_R8G8B8A8(TMOD_IMGPROC *imgp);
TVOID IMGConv_2_B8G8R8(TMOD_IMGPROC *imgp);
TVOID IMGConv_2_B8G8R8A8(TMOD_IMGPROC *imgp);

TVOID IMGScale_Hard(TMOD_IMGPROC *imgp);

TBOOL IMGScale_Smooth(TMOD_IMGPROC *imgp);

TVOID IMGBlitNormal(TMOD_IMGPROC *imgp);
TVOID IMGEndianSwap(TMOD_IMGPROC *imgp);


TVOID IMGFill_8(TIMGPICTURE *pic, TUINT col);
TVOID IMGFill_16(TIMGPICTURE *pic, TUINT col);
TVOID IMGFill_24(TIMGPICTURE *pic, TUINT8 r, TUINT8 g, TUINT8 b);
TVOID IMGFill_32(TIMGPICTURE *pic, TUINT col);


TVOID IMGLine_8(TIMGPICTURE *pic, TINT x_a, TINT y_a, TINT x_e, TINT y_e, TUINT col);
TVOID IMGLine_16(TIMGPICTURE *pic, TINT x_a, TINT y_a, TINT x_e, TINT y_e, TUINT col);
TVOID IMGLine_24(TIMGPICTURE *pic, TINT x_a, TINT y_a, TINT x_e, TINT y_e, TUINT8 r, TUINT8 g, TUINT8 b);
TVOID IMGLine_32(TIMGPICTURE *pic, TINT x_a, TINT y_a, TINT x_e, TINT y_e, TUINT col);

TVOID IMGBox_8(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col);
TVOID IMGBox_16(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col);
TVOID IMGBox_24(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT8 r, TUINT8 g, TUINT8 b);
TVOID IMGBox_32(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col);

TVOID IMGBoxf_8(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col);
TVOID IMGBoxf_16(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col);
TVOID IMGBoxf_24(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT8 r, TUINT8 g, TUINT8 b);
TVOID IMGBoxf_32(TIMGPICTURE *pic, TINT x, TINT y, TINT w, TINT h, TUINT col);

TVOID IMGEllipse_8(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col);
TVOID IMGEllipse_16(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col);
TVOID IMGEllipse_24(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT8 r, TUINT8 g, TUINT8 b);
TVOID IMGEllipse_32(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col);

TVOID IMGEllipsef_8(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col);
TVOID IMGEllipsef_16(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col);
TVOID IMGEllipsef_24(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT8 r, TUINT8 g, TUINT8 b);
TVOID IMGEllipsef_32(TIMGPICTURE *pic, TINT x, TINT y, TINT rx, TINT ry, TUINT col);

TVOID IMGPoly_8(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT col);
TVOID IMGPoly_16(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT col);
TVOID IMGPoly_24(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT8 r, TUINT8 g, TUINT8 b);
TVOID IMGPoly_32(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT col);

TVOID IMGPolyf_8(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT col, TINT *polyfbuf);
TVOID IMGPolyf_16(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT col, TINT *polyfbuf);
TVOID IMGPolyf_24(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT8 r, TUINT8 g, TUINT8 b, TINT *polyfbuf);
TVOID IMGPolyf_32(TIMGPICTURE *pic, TINT numpoints, TINT *points, TUINT col, TINT *polyfbuf);
