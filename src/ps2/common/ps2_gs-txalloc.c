
/*
**	$Id: ps2_gs-txalloc.c,v 1.1 2007/05/19 13:57:57 fschulze Exp $
**	teklib/mods/ps2/common/ps2_txalloc.c - Texture allocator
**
**	Writtem by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/ps2/gs.h>
#include "ps2_mod.h"

#define FINAL			/* define for final version (no colors) */

/*****************************************************************************/

static const TUINT8 bconfig[2][32] =
{
	/* PSMCT32, PSMCT24, PSMT8 */
	{
		0,1,4,5,16,17,20,21,
		2,3,6,7,18,19,22,23,
		8,9,12,13,24,25,28,29,
		10,11,14,15,26,27,30,31,
	},

	/* PSMCT16, PSMCT4 */
	{
		0,2,8,10,
		1,3,9,11,
		4,6,12,14,
		5,7,13,15,
		16,18,24,26,
		17,19,25,27,
		20,22,28,30,
		21,23,29,31,
	},
};

static TINT 
blkidx(TINT tw, TINT x, TINT y, TUINT fmt)
{
	if (fmt & TXA_CONFMASK)
	{
		TINT pi = y / 8 * tw + x / 4;
		return pi * 32 + bconfig[1][(y & 7) * 4 + (x & 3)];
	}
	else
	{
		TINT pi = y / 4 * tw + x / 8;
		return pi * 32 + bconfig[0][(y & 3) * 8 + (x & 7)];
	}
}

static TBOOL
txa_match(struct TXAlloc *t, TINT aw, TINT ah, TUINT fmt, TINT *px, TINT *py)
{
	TINT pw = (fmt & TXA_CONFMASK) ? 4 : 8;	/* pagewidth in blocks */
	TINT ph = (fmt & TXA_CONFMASK) ? 8 : 4;	/* pageheight in blocks */
	TINT y, x, yy, xx, cont;
	
	TUINT bfmt = fmt & 0xff;

	for (y = 0; y <= t->txa_PHeight * ph - ah; ++y)
	{
		for (x = 0; x <= t->txa_PWidth * pw - aw; ++x)
		{
			if ((t->txa_PAlloc[blkidx(t->txa_PWidth, x, y, fmt)] & bfmt) == 0)
			{
				TINT bpi = blkidx(t->txa_PWidth, x, y, fmt);

				cont = 0;

				/* first block ok */
				if (aw > 1)
				{
					for (xx = 1; !cont && (xx < aw); ++xx)
						if (t->txa_PAlloc[blkidx(t->txa_PWidth, xx, 0, fmt) + bpi] & bfmt)
							cont = 1;
					if (cont) continue;
				}

				/* first row ok */
				if (ah > 1)
				{
					for (yy = 1; !cont && (yy < ah); ++yy)
						for (xx = 0; !cont && (xx < aw); ++xx)
							if (t->txa_PAlloc[blkidx(t->txa_PWidth, xx, yy, fmt) + bpi] & bfmt)
								cont = 1;
					if (cont) continue;
				}

				/* have rect */
				*px = x;
				*py = y;
				return TTRUE;
			}
		}
	}

	return TFALSE;
}

static TVOID 
txa_mark(struct TXAlloc *t, TINT x, TINT y, TINT w, TINT h, TUINT fmt, TUINT8 clear, TUINT8 set)
{
	TINT xx, yy;
	TINT bpi = blkidx(t->txa_PWidth, x, y, fmt);

	for (yy = 0; yy < h; ++yy)
	{
		for (xx = 0; xx < w; ++xx)
		{
			t->txa_PAlloc[blkidx(t->txa_PWidth, xx, yy, fmt) + bpi] &= ~clear;
			t->txa_PAlloc[blkidx(t->txa_PWidth, xx, yy, fmt) + bpi] |= set;
		}
	}
}

static TVOID 
txa_markinv(struct TXAlloc *t, TINT bpi, TINT w, TINT h, TUINT fmt, TUINT8 clear, TUINT8 set)
{
	TINT xx, yy;

	for (yy = 0; yy < h; ++yy)
	{
		for (xx = 0; xx < w; ++xx)
		{
			t->txa_PAlloc[blkidx(t->txa_PWidth, xx, yy, fmt) + bpi] &= ~clear;
			t->txa_PAlloc[blkidx(t->txa_PWidth, xx, yy, fmt) + bpi] |= set;
		}
	}
}

static TUINT getbwidth(TUINT fmt)
{
	switch (fmt)
	{
		case TXA_PSMCT32:	return 3;
		case TXA_PSMCT24:	return 3;
		case TXA_PSMCT16: 	return 4;
		case TXA_PSMT8:		return 4;
		case TXA_PSMT4:		return 5;
		case TXA_PSMT8H:	return 3;
		case TXA_PSMT4HL:	return 3;
		case TXA_PSMT4HH:	return 3;
	}
	*(int *) 0 = 0;
	return 5;
}

static TUINT getbheight(TUINT fmt)
{
	switch (fmt)
	{
		case TXA_PSMCT32:	return 3;
		case TXA_PSMCT24:	return 3;
		case TXA_PSMCT16: 	return 3;
		case TXA_PSMT8:		return 4;
		case TXA_PSMT4:		return 4;
		case TXA_PSMT8H:	return 3;
		case TXA_PSMT4HL:	return 3;
		case TXA_PSMT4HH:	return 3;
	}
	*(int *) 0 = 0;
	return 5;
}


static TUINT transfmt(TUINT fmt)
{
	switch (fmt)
	{
		case GS_PSMCT32:	return TXA_PSMCT32;
		case GS_PSMCT24:	return TXA_PSMCT24;
		case GS_PSMCT16:	return TXA_PSMCT16;
		case GS_PSMT8:		return TXA_PSMT8;			
		case GS_PSMT4:		return TXA_PSMT4;			
		case GS_PSMT8H:		return TXA_PSMT8H;	
		case GS_PSMT4HL:	return TXA_PSMT4HL;
		case GS_PSMT4HH:	return TXA_PSMT4HH;
	}
	*(int *) 0 = 0;
	return 5;
}

/*****************************************************************************/
/* 
**	success = txa_allocTexture(txstruct, format, width, height)
**
**	Reserve memory for a texture of the given size and format. Returns
**	a page and a block index.
**
**	INPUTS
**	    txstruct - initialized TXAlloc structure
**	    format   - texture format
**	    width    - width [pixels]
**	    height   - height [pixels]
**	RESULTS
**	    result   - block number
*/

LOCAL TINT
txa_alloc(struct TXAlloc *t, TUINT fmt, TINT w, TINT h)
{
	TUINT txfmt = transfmt(fmt);
	TINT block = -1;
	TINT bx, by;
	TINT bw = getbwidth(txfmt);
	TINT bh = getbheight(txfmt);

	w += (1<<bw)-1;
	w >>= bw;

	h += (1<<bh)-1;
	h >>= bh;
	
	if (txa_match(t, w, h, txfmt, &bx, &by))
	{

	#ifdef FINAL
		txa_mark(t, bx, by, w, h, txfmt, 0, txfmt & 0xff);
	#else
		txa_mark(t, bx, by, w, h, txfmt, col);
	#endif

		block = blkidx(t->txa_PWidth, bx, by, txfmt);
	}

	return block;
}

/*****************************************************************************/
/* 
**	txa_free(txstruct, format, width, height, block)
**
**	Free a texture of given size and format at the specified
**	page and block index.
**
**	INPUTS
**	    txstruct - initialized TXAlloc structure
**	    format   - texture format
**	    width    - width [pixels]
**	    height   - height [pixels]
**	    block    - block index
*/

LOCAL TVOID
txa_free(struct TXAlloc *t, TUINT fmt, TINT w, TINT h, TINT block)
{
	TUINT txfmt = transfmt(fmt);
	TINT bw = getbwidth(txfmt);
	TINT bh = getbheight(txfmt);

	w += (1<<bw)-1;
	w >>= bw;

	h += (1<<bh)-1;
	h >>= bh;

	txa_markinv(t, block, w, h, txfmt, txfmt & 0xff, 0);
}


/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_gs-txalloc.c,v $
**	Revision 1.1  2007/05/19 13:57:57  fschulze
**	restructered ps2_gs.c
**
**	Revision 1.1  2005/09/18 12:40:09  fschulze
**	added
**
**	
*/
