/*
**	$Id: imgload.c,v 1.1.4.1 2006/02/26 10:38:36 tmueller Exp $
**	teklib/mods/ps2/tests/imgload.c
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdlib.h>
#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/io.h>
#include <tek/proto/imgproc.h>
#include <tek/proto/datatypehandler.h>

#include "imgload.h"

extern TAPTR TDthBase;
extern TAPTR TImgpBase;
extern TAPTR TIOBase;

/*****************************************************************************/
/* 
**	load, convert, scale images via datatypes
*/

static TVOID
FreeImage(TIMGPICTURE *img)
{
	TFree(img->palette);
 	TFree(img->data);
}

static TBOOL
Image2ARGB(TIMGPICTURE *img)
{
	if (img->format != IMGFMT_A8R8G8B8)
	{
		TIMGPICTURE temp = *img;
		temp.data = TAlloc(TNULL, temp.width * temp.height * 4);
		if (temp.data)
		{
			temp.palette = TNULL;
			temp.format = IMGFMT_A8R8G8B8;
			temp.depth = 32;
			temp.bytesperrow = temp.width * 4;

			if (TImgDoMethod(TImgpBase, img, &temp, IMGMT_CONVERT, TNULL))
			{
				FreeImage(img);
				*img = temp;
				return TTRUE;
			}
			else
			{
				TFree(temp.data);
			}
		}
		return TFALSE;
	}
	return TTRUE;
}

static TBOOL
ScaleImage(TIMGPICTURE *img, TINT w, TINT h)
{
	TBOOL success = TTRUE;
	if (img->width != w || img->height != h)
	{
		TIMGPICTURE temp = *img;
		success = TFALSE;
		temp.data = TAlloc(TNULL, w * h * sizeof(TUINT));
		if (temp.data)
		{
			TTAGITEM scaletags[2];
			scaletags[0].tti_Tag = IMGTAG_SCALEMETHOD;
			scaletags[0].tti_Value = (TTAG) IMGSMT_SMOOTH;
			scaletags[1].tti_Tag = TTAG_DONE;

			temp.width = w;
			temp.height = h;
			temp.bytesperrow = w * sizeof(TUINT);

			if (TImgDoMethod(TImgpBase, img, &temp, IMGMT_SCALE, scaletags))
			{
				FreeImage(img);
				*img = temp;
				success = TTRUE;
			}
			else
			{
				TFree(temp.data);
			}
		}
	}
	return success;
}

static TBOOL
LoadImage(TIMGPICTURE *img, TSTRPTR filename)
{
	TBOOL success = TFALSE;
	TAPTR fh = TOpenFile(filename, TFMODE_READONLY, TNULL);
	if (fh)
	{
		TTAGITEM opentags[2];
		TAPTR dtobject;

		opentags[0].tti_Tag = TDOPENTAG_FILEHANDLE;
		opentags[0].tti_Value = (TTAG) fh;
		opentags[1].tti_Tag = TTAG_DONE;
		
		dtobject = TDthOpen(TDthBase, opentags);
		if (dtobject)
		{
			TAPTR dttags = TDthGetAttrs(TDthBase, dtobject);
			if ((TINT) TGetTag(dttags, TDTAG_CLASS, 0) == DTCLASS_PIC)
			{
				img->width = (TINT) TGetTag(dttags, TDTAG_PICWIDTH, 0);
				img->height = (TINT) TGetTag(dttags, TDTAG_PICHEIGHT, 0);
				img->depth = (TINT) TGetTag(dttags, TDTAG_PICDEPTH, 0);
				img->format = (TINT) TGetTag(dttags, TDTAG_PICFORMAT, 0);
				img->bytesperrow = (TINT) TGetTag(dttags, TDTAG_PICBYTESPERROW, 0);
				img->palette = TNULL;
				
				img->data = TAlloc(TNULL, img->bytesperrow * img->height);
				if (img->data)
				{
					TTAGITEM loadtags[3];
					loadtags[0].tti_Tag = TDOTAG_GETDATA;
					loadtags[0].tti_Value = (TTAG) img->data;

					success = TTRUE;
					if (img->depth <= 8)
					{
						img->palette = TAlloc(TNULL, sizeof(TIMGARGBCOLOR) * 256);
						if (img->palette)
						{
							loadtags[1].tti_Tag = TDOTAG_GETPALETTE;
							loadtags[1].tti_Value = (TTAG) img->palette;
							loadtags[2].tti_Tag = TTAG_DONE;
						}
						else
							success = TFALSE;
					}
					else
						loadtags[1].tti_Tag = TTAG_DONE;
					
					if (success)
						success = TDthDoMethod(TDthBase, dtobject, loadtags);
				}

				if (!success)
					FreeImage(img);
			}
			TDestroy(dtobject);
		}
		TCloseFile(fh);
	}
	return success;
}

/*****************************************************************************/

TVOID 
u_freeImage(GSimage *gs)
{
	TFree(gs->data);
}

GSimage 
u_loadImage(TSTRPTR fname, TINT halpha)
{
	TIMGPICTURE dtimg;
	GSimage img;
	
	if (LoadImage(&dtimg, fname))
	{
		/* ensure ARGB */
		if (Image2ARGB(&dtimg))
		{
			TINT i;
			TINT dw = 1 << u_ld(dtimg.width);
			TINT dh = 1 << u_ld(dtimg.height);
			
			if (dw != dtimg.width || dh != dtimg.height)
			{
				//printf("scale %dx%d -> %dx%d\n", dtimg.width, dtimg.height, dw, dh);
				ScaleImage(&dtimg, dw, dh);
			}
				
			img.w = dtimg.width;
			img.h = dtimg.height;
			img.data = dtimg.data;

			/* shift alpha, convert ARGB (img) to ABGR (screen) */

			for (i = 0; i < img.w * img.h; ++i)
			{
				TUINT p = ((TUINT *)img.data)[i];
				((TUINT *)img.data)[i] =
					((p & 0xfe000000) >> halpha) |
					((p & 0x00ff0000) >> 16) |
					(p & 0x0000ff00) |
					((p & 0x000000ff) << 16);
			}

			return img;
		}
					
		FreeImage(&dtimg);
	}
	else
		printf("could not load '%s'\n", fname);

	img.w = 0;
	img.h = 0;
	img.data = TNULL;
	return img;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: imgload.c,v $
**	Revision 1.1.4.1  2006/02/26 10:38:36  tmueller
**	added missing TIOBase declaration
**	
**	Revision 1.1  2005/10/05 22:11:26  fschulze
**	added
**	
**	
*/

