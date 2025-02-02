
/*
**	$Id: datatype_codec.c,v 1.2 2005/07/16 09:45:12 tmueller Exp $
**	teklib/mods/datatypes/targa/datatype_codec.c - Targa datatype
**
**	currently supports only 24 and 32 bit, mode 2 and 10.
**	see: http://astronomy.swin.edu.au/~pbourke/dataformats/tga/
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>

#include <tek/mod/datatypehandler.h>
#include <tek/mod/datatype_codec.h>
#include <tek/mod/imgproc.h>

#define MOD_VERSION		0
#define MOD_REVISION	1

typedef struct
{
	TUINT8 tt_IDLen;				/* 0 */
	TUINT8 tt_ColorMapType;			/* 1 */
	TUINT8 tt_ImageType;			/* 2 */
	TUINT8 tt_FirstEntryIndex[2];	/* 3, 4 */
	TUINT16 tt_ColorMapLength;		/* 5, 6 */ 
	TUINT8 tt_ColorMapEntrySize;	/* 7 */
	TUINT8 tt_XOrigin[2];			/* 8, 9 */
	TUINT8 tt_YOrigin[2];			/* 10, 11 */
	TUINT16 tt_ImageWidth;			/* 12, 13 */
	TUINT16 tt_ImageHeight;			/* 14, 15 */
	TUINT8 tt_PixelDepth;			/* 16 */
	TUINT8 tt_ImageDescriptor;		/* 17 */

} targa_t;

typedef struct
{
	struct THandle dti_Handle;
	targa_t dti_Header;
	TINT dti_ImageSize;
	TINT dti_BytesPerPixel;
	TINT dti_BytesPerRow;
	TUINT8 *dti_TempRow;

} dtimage_t;

typedef struct
{
	TMODL module;					/* Module header */
	TAPTR lock;						/* Modbase lock */
	TINT refcount;					/* Reference counter */
	
	TAPTR mmu;
	TAPTR util;
	TAPTR io;

	TAPTR fp;	
	TINT rate;
	TINT fmt;
	TINT chans;
	TTAGITEM attr[11];
	dtimage_t *image;

} DTCODEC;

static TCALLBACK DTCODEC *mod_open(DTCODEC *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(DTCODEC *dtcodec, TAPTR task);
static TCALLBACK TVOID mod_destroy(DTCODEC *mod);

static TMODAPI TTAGITEM *dtcodec_getattrs(DTCODEC *dtcodec);
static TMODAPI TINT dtcodec_domethod(DTCODEC *dtcodec, TTAGITEM *tags);

static TBOOL dtcodec_open(DTCODEC *mod, TAPTR fp);

#define TExecBase TGetExecBase(mod)
#define TIOBase mod->io
#define TUtilBase mod->util

/*****************************************************************************/
/*
**	module init/exit
*/

TMODENTRY TUINT
tek_init_datatype_codec_targa(TAPTR task, DTCODEC *mod, TUINT16 version,
	TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * 2;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(DTCODEC);		/* positive size */

		return 0;
	}
	
	mod->mmu = TNULL;
	mod->lock = TCreateLock(TNULL);
	if (mod->lock)
	{
		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;

		mod->module.tmd_DestroyFunc = (TDFUNC) mod_destroy;	
		mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;

		((TAPTR *) mod)[-1] = (TAPTR) dtcodec_getattrs;
		((TAPTR *) mod)[-2] = (TAPTR) dtcodec_domethod;
		
		return TTRUE;
	}
	
	return 0;
}

static TCALLBACK TVOID 
mod_destroy(DTCODEC *mod)
{
	TDestroy(mod->lock);
	TDestroy(mod->mmu);
}

/*****************************************************************************/
/*
**	open/close instance
*/

static TCALLBACK DTCODEC *
mod_open(DTCODEC *mod, TAPTR task, TTAGITEM *tags)
{
	DTCODEC *inst = TNULL;
	TAPTR fp = (TAPTR) TGetTag(tags, TDCTAG_FILEHANDLE, TNULL);
	TBOOL success = TFALSE;
	
	if (fp)
	{
		TLock(mod->lock);

		if (mod->refcount == 0)
		{
			mod->util = TOpenModule("util", 0, TNULL);
			mod->io = TOpenModule("io", 0, TNULL);
		}

		if (mod->util && mod->io)
		{
			inst = TNewInstance(mod, mod->module.tmd_PosSize,
				mod->module.tmd_NegSize);
		}

		if (inst) success = dtcodec_open(inst, fp);

		if (inst && success)
		{
			mod->refcount++;
		}
		else
		{
			TCloseModule(mod->io);
			TCloseModule(mod->util);
			TFreeInstance(inst);
			inst = TNULL;
		}

		TUnlock(mod->lock);
	}

	return inst;
}

static TCALLBACK TVOID 
mod_close(DTCODEC *inst, TAPTR task)
{
	DTCODEC *mod = (DTCODEC *) inst->module.tmd_ModSuper;

	TLock(mod->lock);

	TDestroy(inst->image);

	TFreeInstance(inst);

	if (--mod->refcount == 0)
	{
		TCloseModule(mod->io);
		TCloseModule(mod->util);
	}

	TUnlock(mod->lock);
}

/*****************************************************************************/

static TCALLBACK TVOID destroyimg(dtimage_t *img)
{
	DTCODEC *mod = img->dti_Handle.thn_Data;
	TFree(img->dti_TempRow);
	TFree(img);
}

static TBOOL dtcodec_open(DTCODEC *mod, TAPTR fp)
{
	dtimage_t *img;
	TUINT8 buf[18];
	
	img = TAlloc(TNULL, sizeof(dtimage_t));
	if (img == TNULL) return TFALSE;

	img->dti_Handle.thn_Data = mod;
	img->dti_Handle.thn_DestroyFunc = (TDFUNC) destroyimg;
	img->dti_TempRow = TNULL;

	if (TFRead(fp, buf, 18) == 18)
	{
		targa_t *hd = &img->dti_Header;

		hd->tt_IDLen = buf[0];
		hd->tt_ColorMapType = buf[1];
		hd->tt_ImageType = buf[2];
		hd->tt_ColorMapEntrySize = buf[7];
		hd->tt_ColorMapLength = (TINT16) buf[5] + (((TINT16) buf[6]) << 8);
		hd->tt_PixelDepth = buf[16];
		hd->tt_ImageWidth = (TINT16) buf[12] + (((TINT16) buf[13]) << 8);
		hd->tt_ImageHeight = (TINT16) buf[14] + (((TINT16) buf[15]) << 8);
		hd->tt_ImageDescriptor = buf[17];
	
		for (;;)
		{
			TUINT format;
			TINT bpp;

			if (hd->tt_ImageType != 2 && hd->tt_ImageType != 10) break;
			
			if (hd->tt_PixelDepth == 24)
			{
				format = IMGFMT_B8G8R8;
				bpp = 3;
			}
			else if (hd->tt_PixelDepth == 32)
			{
				if (TIsBigEndian())
					format = IMGFMT_B8G8R8A8;
				else
					format = IMGFMT_A8R8G8B8;
				bpp = 4;
			}
			else break;

			img->dti_ImageSize = hd->tt_ImageHeight * hd->tt_ImageWidth * bpp;
		
			/* skip image ID field */
			if (hd->tt_IDLen)
			{
				if (TSeek(fp, hd->tt_IDLen, TNULL, TFPOS_CURRENT) == 0xffffffff) break;
			}

			/* skip colormap */			
			if (hd->tt_ColorMapType != 0)
			{
			    if (TSeek(fp, hd->tt_ColorMapEntrySize / 8 * hd->tt_ColorMapLength, 
				    	TNULL, TFPOS_CURRENT) == 0xffffffff) break;
			}
			
			img->dti_BytesPerPixel = bpp;
			img->dti_BytesPerRow = bpp * hd->tt_ImageWidth;
			
			if ((hd->tt_ImageDescriptor & 0x20) == 0)
			{
				/* must flip */
				img->dti_TempRow = TAlloc(TNULL, img->dti_BytesPerRow);
				if (img->dti_TempRow == TNULL) break;
			}

			mod->attr[0].tti_Tag = TDTAG_CLASS;
			mod->attr[0].tti_Value = DTCLASS_PIC;
			mod->attr[1].tti_Tag = TDTAG_FULLNAME;
			mod->attr[1].tti_Value = (TTAG) "Targa picture";
			mod->attr[2].tti_Tag = TDTAG_NUMSUBCLASSES;
			mod->attr[2].tti_Value = 0;
			mod->attr[3].tti_Tag = TDTAG_PICWIDTH;
			mod->attr[3].tti_Value = hd->tt_ImageWidth;
			mod->attr[4].tti_Tag = TDTAG_PICHEIGHT;
			mod->attr[4].tti_Value = hd->tt_ImageHeight;
			mod->attr[5].tti_Tag = TDTAG_PICDEPTH;
			mod->attr[5].tti_Value = hd->tt_PixelDepth;
			mod->attr[6].tti_Tag = TDTAG_PICBYTESPERROW;
			mod->attr[6].tti_Value = img->dti_BytesPerRow;
			mod->attr[7].tti_Tag = TDTAG_PICPIXELWIDTH;
			mod->attr[7].tti_Value = 1;
			mod->attr[8].tti_Tag = TDTAG_PICPIXELHEIGHT;
			mod->attr[8].tti_Value = 1;
			mod->attr[9].tti_Tag = TDTAG_PICFORMAT;
			mod->attr[9].tti_Value = format;
			mod->attr[10].tti_Tag = TTAG_DONE;
					
			mod->fp = fp;
			mod->image = img;

			return TTRUE;
		}
	}
	TDestroy(img);
	return TFALSE;
}

/*****************************************************************************/
/*
**	dtcodec_getattrs()
*/

static TMODAPI TTAGITEM *
dtcodec_getattrs(DTCODEC *mod)
{
	if (mod->image) return mod->attr;
	return TNULL;
}

/*****************************************************************************/
/*
**	dtcodec_domethod()
*/

static TMODAPI TINT
dtcodec_domethod(DTCODEC *mod, TTAGITEM *tags)
{
	TINT success = TFALSE;
	dtimage_t *img = mod->image;
	TINT bpr = img->dti_BytesPerRow;
	TUINT8 *data = (TUINT8 *) TGetTag(tags, TDOTAG_GETDATA, TNULL);
	TINT y;

	if (img && data)
	{
		if (img->dti_Header.tt_ImageType == 2)	
		{
			TFlush(mod->fp);
			success = (TRead(mod->fp, data, img->dti_ImageSize) == img->dti_ImageSize);
		}
		else if (img->dti_Header.tt_ImageType == 10)
		{
			TINT bpp = img->dti_BytesPerPixel;
			TINT cb = 0;
			
			success = TTRUE;
			while (cb < img->dti_ImageSize)
			{
				TINT ch = TFGetC(mod->fp);
				if (ch < 128) /* raw chunk */
				{
					ch++;
					if (TFRead(mod->fp, data + cb, bpp * ch) == bpp * ch)
					{
						cb += bpp * ch;
						continue;
					}
				}
				else if (ch >= 0)
				{
					TUINT8 rgb[4];
					if (TFRead(mod->fp, rgb, bpp) == bpp)
					{
						TINT i, j;
						for (i = 0; i < ch - 127; i++)
						{
							for (j = 0; j < bpp; j++)
							{
								data[cb + j] = rgb[j];
							}
							cb += bpp;
						}
						continue;
					}
				}

				/* fallthrough */
				success = TFALSE;
				break;
			}
		}
			
		TFlush(mod->fp);
		
		if (success && img->dti_TempRow)
		{
			TUINT8 *p1 = (TUINT8 *) TGetTag(tags, TDOTAG_GETDATA, TNULL);
			TUINT8 *p2 = p1 + img->dti_ImageSize;
			
			for (y = 0; y < img->dti_Header.tt_ImageHeight / 2; ++y)
			{
				p2 -= bpr;
				TCopyMem(p2, img->dti_TempRow, bpr);
				TCopyMem(p1, p2, bpr);
				TCopyMem(img->dti_TempRow, p1, bpr);
				p1 += bpr;
			}
		}
	}
	
	return success;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: datatype_codec.c,v $
**	Revision 1.2  2005/07/16 09:45:12  tmueller
**	uncompressed targas are now loaded in a single read
**	
**	Revision 1.1  2005/07/13 23:29:48  tmueller
**	added targa datatype
**	
*/
