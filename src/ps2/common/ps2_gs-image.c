/*
**	$Id: ps2_gs-image.c,v 1.1 2007/05/19 13:57:57 fschulze Exp $
**	teklib/mods/ps2/common/ps2_gs.c - GS texture handling
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
*/

#include <tek/inline/ps2.h>
#include <tek/mod/ps2/gif.h>
#include <kernel.h>
#include "ps2_mod.h"

/***********************************************************************************************
  GS Image related functions
 ***********************************************************************************************/

EXPORT TINT
gs_initImage(TMOD_PS2 *TPS2Base, GSimage *gsimage, TINT w, TINT h, TINT psm, TAPTR data)
{
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	GStexenv *tenv = &gsinfo->gsi_texenv;
	TINT blk = txa_alloc(&tenv->gst_txalloc, psm, w, h);
	if (blk >= 0)
	{
		gsimage->w = w;
		gsimage->h = h;
		gsimage->psm = psm;
		gsimage->tbp = tenv->gst_tbp+blk;
		gsimage->tbw = tenv->gst_txalloc.txa_PWidth;
		gsimage->data = data;
	}
	return blk;
}

EXPORT TVOID 
gs_loadImage(TMOD_PS2 *TPS2Base, GSimage *gsimage)
{
	TQWDATA *d;
	TINT qwc, i;
	TAPTR tptr = gsimage->data;
	TINT bytes = gsimage->w * gsimage->h;
	
	/* see page 24 of GS User's Manual v5.0 */
	switch (gsimage->psm)
	{
		case GS_PSMCT32:
		case GS_PSMZ32:
			bytes <<= 2;
			break;
		
		case GS_PSMCT24:
		case GS_PSMZ24:
			bytes *= 3;
			break;
		
		case GS_PSMCT16:
		case GS_PSMCT16S:
		case GS_PSMZ16:
		case GS_PSMZ16S:
			bytes <<= 1;
			break;
	
		case GS_PSMT8:
		case GS_PSMT8H:
			break;
			
		case GS_PSMT4:
		case GS_PSMT4HH:
		case GS_PSMT4HL:
			bytes >>= 1;
			break;
					
		default:
			break;
	}

	qwc = (bytes + 15) >> 4;
	
	g_setReg(GS_BITBLTBUF, GS_SETREG_BITBLTBUF(0,0,0,gsimage->tbp,gsimage->tbw,gsimage->psm));
	g_setReg(GS_TRXPOS, GS_SETREG_TRXPOS(0,0,0,0,0));
	g_setReg(GS_TRXREG, GS_SETREG_TRXREG(gsimage->w, gsimage->h));
	g_setReg(GS_TRXDIR, GS_SETREG_TRXDIR(0));

	d = d_alloc(DMC_GIF, qwc+1, TNULL, TNULL);
	d[0].ui32[0] = 0x00008000+qwc;
	d[0].ui32[1] = 0x08000000;
	d[0].ui32[2] = 0x00000000;
	d[0].ui32[3] = 0x00000000;

	for (i = 0; i < qwc; i++)
	{
		d[i+1].ul128 = *(TUINT128 *)tptr;
		tptr += 16;
	}
				
	g_setReg(GS_TEXFLUSH, 0x42);
}

EXPORT TVOID
gs_getImage(TMOD_PS2 *TPS2Base, GSimage *img)
{
	TINT qwc;
	TINT bytes = img->w * img->h;

	switch (img->psm)
	{
		case GS_PSMCT32:
		case GS_PSMZ32:
			bytes <<= 2;
			break;
		
		case GS_PSMCT24:
		case GS_PSMZ24:
			bytes *= 3;
			break;
		
		case GS_PSMCT16:
		case GS_PSMCT16S:
		case GS_PSMZ16:
		case GS_PSMZ16S:
			bytes <<= 1;
			break;
	
		case GS_PSMT8:
		case GS_PSMT8H:
			break;
		
		case GS_PSMT4:	
		case GS_PSMT4HH:
		case GS_PSMT4HL:
			*(TINT *) 0 = 0;	/* not supported */
					
		default:
			break;
	}
	
	qwc = (bytes + 15) >> 4;
	
	if (img->data == TNULL)
		img->data = TAlloc(TNULL, (qwc << 4));

	/****************************************************************************
		Transmission from Local Buffer to Host

		1. FINISH = 1
		2. while (CSR.finish != 1) ;
		3. CSR.finish = 0
		4. transmission parameters
			- BITBLTBUF (base ptr, buffer width, pixel storage format)
			- TRXPOS (offset, pixel transmission direction)
			- TRXREG (width, height of transmission area)
			- TRXDIR (direction of transmission, causes start of transmission)
		5. BUSDIR.dir = 1
		6. dma read data
		7. BUSDIR.dir = 0
	****************************************************************************/
	
	g_setReg(GS_BITBLTBUF, GS_SETREG_BITBLTBUF(img->tbp,img->tbw,img->psm, 0, 0, 0));
	g_setReg(GS_TRXPOS, GS_SETREG_TRXPOS(0,0,0,0,0));
	g_setReg(GS_TRXREG, GS_SETREG_TRXREG(img->w, img->h));
	g_setReg(GS_FINISH, 1);
	g_setReg(GS_TRXDIR, GS_SETREG_TRXDIR(1));
	
	d_commit(DMC_GIF);
	d_commit(DMC_GIF);

	while(!(*(TUINT volatile *)GS_CSR & 0x2))	;
	*(TUINT volatile *)GS_CSR = *(TUINT volatile *)GS_CSR & 0x0;

	g_syncPath(0);
	
	*(TUINT volatile *)VIF1_STAT = 0x800000;
	*(TUINT volatile *)GS_BUSDIR = 0x1;

	DMA_SET_QWC ((TUINT *)D1_QWC, qwc);
	DMA_SET_MADR((TUINT *)D1_MADR, img->data, 0);
	DMA_SET_CHCR((TUINT *)D1_CHCR, 0, 0, 0, 0, 0, 1, 0);
	DMA_WAIT((TUINT *)D1_CHCR);
	
	g_syncPath(0);
	
	*(TUINT volatile *)VIF1_STAT = 0x0;
	*(TUINT volatile *)GS_BUSDIR = 0x0;
	
	FlushCache(0);
}

EXPORT TVOID 
gs_freeImage(TMOD_PS2 *TPS2Base, GSimage *gsimage)
{
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	GStexenv *tenv = &gsinfo->gsi_texenv;
	txa_free(&tenv->gst_txalloc, gsimage->psm, gsimage->w, gsimage->h, gsimage->tbp);
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_gs-image.c,v $
**	Revision 1.1  2007/05/19 13:57:57  fschulze
**	restructered ps2_gs.c
**
**	
*/
