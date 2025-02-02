
#ifndef _TEK_MOD_IMGPROC_H
#define _TEK_MOD_IMGPROC_H

/* picture formats */
#ifndef PICFORMATS

#define IMGFMT_PLANAR       0x00001001
#define IMGFMT_CLUT         0x00081002
#define IMGFMT_R5G5B5       0x000f2103
#define IMGFMT_R5G6B5       0x00102104
#define IMGFMT_R8G8B8       0x00182207
#define IMGFMT_B8G8R8       0x00182208
#define IMGFMT_A8R8G8B8     0x00206309
#define IMGFMT_R8G8B8A8     0x0020630a
#define IMGFMT_B8G8R8A8     0x0020630b

#define IMGFMTF_INDEXED     0x00001000  // flag: indexed color system
#define IMGFMTF_TRUECOLOR   0x00002000  // flag: full color space system
#define IMGFMTF_MASKED      0x00004000  // flag: alpha channel present
#define IMGFMTF_BYTESPERPIX 0x00000f00  // mask: bytes per pixel - 1
#define IMGFMTF_COLORRES    0x00ff0000  // mask: colorspace depth

#define PICFORMATS
#endif

typedef struct _TImgARGBColor
{
	TUINT8 a,r,g,b;

} TIMGARGBCOLOR;

typedef struct _TImgPicture
{
	TVOID *data;
	TIMGARGBCOLOR *palette;

	TINT width,height,depth,format,bytesperrow;

} TIMGPICTURE;

/* basic Tags */

#define IMGTAG_SCALESMOOTH              TTAG_USER+64

#define IMGTAG_METHOD					TTAG_USER+101
#define IMGTAG_SRCX						TTAG_USER+102
#define IMGTAG_SRCY						TTAG_USER+103
#define IMGTAG_DSTX						TTAG_USER+104
#define IMGTAG_DSTY						TTAG_USER+105

#define IMGTAG_WIDTH					TTAG_USER+106
#define IMGTAG_HEIGHT					TTAG_USER+107
#define IMGTAG_SCALEWIDTH				TTAG_USER+108
#define IMGTAG_SCALEHEIGHT				TTAG_USER+109
#define IMGTAG_SCALEMETHOD				TTAG_USER+110

#define IMGTAG_BLITMETHOD				TTAG_USER+122

/* methods */
#define IMGMT_CONVERT					0x00000001
#define IMGMT_SCALE						0x00000002
#define IMGMT_ENDIANSWAP				0x00000003
#define IMGMT_BLIT						0x00000004

/* scale methods */
#define IMGSMT_HARD						0x00000001
#define IMGSMT_SMOOTH					0x00000002


/*
**      definitions for imgproc
*/
#include <tek/exec.h>

#endif

