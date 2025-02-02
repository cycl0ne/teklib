
/*
**	$Id: ps2_gs.c,v 1.3 2005/10/07 12:22:06 fschulze Exp $
**	teklib/mods/ps2/common/ps2_gs.c - GS setup and primitive drawing
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	InitScreen function based on code by Tony Saveski <t_saveski at yahoo.com>
*/

#include <tek/mod/ps2/gif.h>
#include <kernel.h>		/* FIXME */
#include "ps2_mod.h"

static const vmode_t vmodes[] =
{
	{3, 256, 256, 0, 32, 10},	/* PAL_256_512_32 */
	{3, 320, 256, 0, 32, 8},	/* PAL_320_256_32 */
	{3, 384, 256, 0, 32, 7},	/* PAL_384_256_32 */
	{3, 512, 256, 0, 32, 5},	/* PAL_512_256_32 */
	{3, 640, 256, 0, 32, 4}		/* PAL_640_256_32 */
};

struct cbdata
{
	TINT fb;
	TINT ctx;
	GSinfo *gsinfo;
};

static TINT  getZBits(TINT zpsm);
static TVOID setpmode(TINT ctx);
static TVOID setvisiblebuf(struct cbdata *cbd);
static TVOID g_initlmalloc(TMOD_PS2 * mod, struct LMAlloc *lma);
static TINT  g_lmalloc(TMOD_PS2 * mod, struct LMAlloc *lma, TINT numpages);
static TINT  g_lmfree(TMOD_PS2 * mod, struct LMAlloc *lma, TINT startpage, TINT numpages);

/*----------------------------------------------------------------------------------------------*/
/***********************************************************************************************
  NEW gs functions
 ***********************************************************************************************/
/*----------------------------------------------------------------------------------------------*/

EXPORT TVOID 
g_setReg(TMOD_PS2 *mod, TINT reg, TUINT64 data)
{
	TQWDATA *d = d_alloc(mod, DMC_GIF, 2, TNULL, TNULL);

	d[0] 		 = GIF_SET_GIFTAG(1, 1, 0, 0, 0, 1, 0xe);
	d[1].ul64[1] = reg;
	d[1].ul64[0] = data;
}

EXPORT TVOID
g_vsync(TMOD_PS2 *mod)
{
	*(TUINT volatile *)GS_CSR = *(TUINT volatile *)GS_CSR & 0x8;
	while(!(*(TUINT volatile *)GS_CSR & 0x8))	;
}

#define GSP_ERR(s)			\
({							\
	if (timeout == 0)		\
	{						\
		printf("%s\n", s);	\
		return -1;			\
	}						\
})

EXPORT TINT
g_syncPath(TMOD_PS2 *mod, TINT mode)
{
	TUINT t = 0;
	TINT res = 0;

	if (mode == 0) /* blocking mode */
	{
		TINT timeout = 0x1000000;
		
		while (timeout && *(TUINT volatile *)D1_CHCR & 0x100)
			timeout--;		
		GSP_ERR("D1 channel not ready!");
		
		while (timeout && *(TUINT volatile *)D2_CHCR & 0x100)
			timeout--;
		GSP_ERR("D2 channel not ready!\n");
		
		while (timeout && *(TUINT volatile *)VIF1_STAT & 0x1f000003)
			timeout--;
		GSP_ERR("VIF1 not ready!");
		
		asm __volatile__("cfc2 %0, $29" : "=r" (t));
		while (timeout && t & 0x100)
		{
			asm __volatile__("cfc2 %0, $29" : "=r" (t));
			timeout--;
		}
		GSP_ERR("VU1 not ready!");

		while (timeout && *(TUINT volatile *)GIF_STAT & 0xc00)
			timeout--;
		GSP_ERR("GIF not ready!");		
	}
	else /* non-blocking mode */
	{
		if (*(TUINT volatile *)D1_CHCR & 0x100)
			res  = 0x1;
		
		if (*(TUINT volatile *)D2_CHCR & 0x100)
			res |= 0x2;
		
		if (*(TUINT volatile *)VIF1_STAT & 0x1f000003)
			res |= 0x4;
		
		asm __volatile__("cfc2 %0, $29" : "=r" (t));
		if (t & 0x100)
			res |= 0x8;
		
		if (*(TUINT volatile *)GIF_STAT & 0xc00)
			res |= 0x10;
	}
	
	return res;
}

EXPORT TVOID 
g_enableZBuf(TMOD_PS2 *mod, TINT ctx)
{
	GSinfo *gsinfo = mod->ps2_GSInfo;
	GScontext *cont;
	GSdisplay *disp;

	if (ctx & GS_CTX1)
	{	
		TQWDATA *d;
		
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
		
		g_setReg(mod, GS_ZBUF_1, GS_SETREG_ZBUF_1(cont->gsc_zbp, cont->gsc_zpsm, 0));
	
		/* clear zBuffer */
		g_setReg(mod, GS_TEST_1, GS_SETREG_TEST_1(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));
	
		d = d_alloc(mod, DMC_GIF, 4, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x6, 0, 3, 0x551);
		d[1] = GIF_SET_RGBA(0, 0, 0, 0);
		d[2] = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width, disp->gsd_yoffset+disp->gsd_height,cont->gsc_dclear);

		g_setReg(mod, GS_TEST_1, GS_SETREG_TEST_1(0, 0, 0, 0, 0, 0, 1, cont->gsc_dfunc));
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d;
		
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;
		
		g_setReg(mod, GS_ZBUF_2, GS_SETREG_ZBUF_2(cont->gsc_zbp, cont->gsc_zpsm, 0));
	
		/* clear zBuffer */
		g_setReg(mod, GS_TEST_2, GS_SETREG_TEST_2(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));

		d = d_alloc(mod, DMC_GIF, 4, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x206, 0, 3, 0x551);
		d[1] = GIF_SET_RGBA(0, 0, 0, 0);
		d[2] = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width, disp->gsd_yoffset+disp->gsd_height,cont->gsc_dclear);

		g_setReg(mod, GS_TEST_2, GS_SETREG_TEST_2(0, 0, 0, 0, 0, 0, 1, cont->gsc_dfunc));
	}
}

EXPORT TVOID 
g_clearScreen(TMOD_PS2 *mod, TINT ctx)
{
	GScontext *cont;
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;
		
	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
		
		if (cont->gsc_dfunc != GS_ZTEST_ALWAYS && cont->gsc_dfunc != GS_ZTEST_NEVER)
			g_setReg(mod, GS_TEST_1, GS_SETREG_TEST_1(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));
	
		d = d_alloc(mod, DMC_GIF, 4, TNULL, TNULL);
		
		d[0] 		 = GIF_SET_GIFTAG(1, 1, 1, 0x6, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)&cont->gsc_clear;
		d[2] 		 = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] 		 = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width +1,
							  disp->gsd_yoffset+disp->gsd_height+1, cont->gsc_dclear);
		
		if (cont->gsc_dfunc != GS_ZTEST_ALWAYS && cont->gsc_dfunc != GS_ZTEST_NEVER)
			g_setReg(mod, GS_TEST_1, GS_SETREG_TEST_1(0, 0, 0, 0, 0, 0, 1, cont->gsc_dfunc));
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d;
				
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;

		if (cont->gsc_dfunc != GS_ZTEST_ALWAYS && cont->gsc_dfunc != GS_ZTEST_NEVER)
			g_setReg(mod, GS_TEST_2, GS_SETREG_TEST_2(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));
	
		d = d_alloc(mod, DMC_GIF, 4, TNULL, TNULL);
		
		d[0] 		 = GIF_SET_GIFTAG(1, 1, 1, 0x206, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)&cont->gsc_clear;
		d[2] 		 = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] 		 = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width +1,
							  disp->gsd_yoffset+disp->gsd_height+1, cont->gsc_dclear);

		if (cont->gsc_dfunc != GS_ZTEST_ALWAYS && cont->gsc_dfunc != GS_ZTEST_NEVER)
			g_setReg(mod, GS_TEST_2, GS_SETREG_TEST_2(0, 0, 0, 0, 0, 0, 1, cont->gsc_dfunc));
	}
}

EXPORT TVOID
g_clearScreenAlpha(TMOD_PS2 *mod, TINT ctx)
{
	GScontext *cont;
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
		
		if (cont->gsc_dfunc != GS_ZTEST_ALWAYS && cont->gsc_dfunc != GS_ZTEST_NEVER)
			g_setReg(mod, GS_TEST_1, GS_SETREG_TEST_1(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));
	
		d = d_alloc(mod, DMC_GIF, 4, TNULL, TNULL);
		
		d[0] 		 = GIF_SET_GIFTAG(1, 1, 1, 0x46, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)&cont->gsc_clear;
		d[2] 		 = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] 		 = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width +1,
							  disp->gsd_yoffset+disp->gsd_height+1, cont->gsc_dclear);
		
		if (cont->gsc_dfunc != GS_ZTEST_ALWAYS && cont->gsc_dfunc != GS_ZTEST_NEVER)
			g_setReg(mod, GS_TEST_1, GS_SETREG_TEST_1(0, 0, 0, 0, 0, 0, 1, cont->gsc_dfunc));
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d;
		
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;
		
		if (cont->gsc_dfunc != GS_ZTEST_ALWAYS && cont->gsc_dfunc != GS_ZTEST_NEVER)
			g_setReg(mod, GS_TEST_2, GS_SETREG_TEST_2(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));
	
		d = d_alloc(mod, DMC_GIF, 4, TNULL, TNULL);
		
		d[0] 		 = GIF_SET_GIFTAG(1, 1, 1, 0x246, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)&cont->gsc_clear;
		d[2] 		 = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] 		 = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width +1,
							  disp->gsd_yoffset+disp->gsd_height+1, cont->gsc_dclear);

		if (cont->gsc_dfunc != GS_ZTEST_ALWAYS && cont->gsc_dfunc != GS_ZTEST_NEVER)
			g_setReg(mod, GS_TEST_2, GS_SETREG_TEST_2(0, 0, 0, 0, 0, 0, 1, cont->gsc_dfunc));
	}
}

static TVOID
setvisiblebuf(struct cbdata *cbd)
{
	TINT fbp;
	GScontext *cont;
	GSdisplay *disp;
	GSinfo *gsinfo = cbd->gsinfo;
	
	if (cbd->ctx & GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
		
		/* set visible buffer in DISPFB_1 (modify base ptr) */
		if (cbd->fb & 1)	fbp = cont->gsc_fbp0;
		else				fbp = cont->gsc_fbp1;
		GS_SET_DISPFB1(fbp, disp->gsd_width/64, cont->gsc_psm, 0, 0);
	}
	
	if (cbd->ctx & GS_CTX2)
	{
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;
		
		/* set visible buffer in DISPFB_2 (modify base ptr) */
		if (cbd->fb & 1)	fbp = cont->gsc_fbp0;
		else				fbp = cont->gsc_fbp1;
		GS_SET_DISPFB2(fbp, disp->gsd_width/64, cont->gsc_psm, 0, 0);
	}
}


EXPORT TVOID
g_setActiveFb(TMOD_PS2 *mod, TINT ctx, TINT fbp)
{
	GScontext *cont;
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	
	if (ctx == GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;

		/* set active buffer in FRAME_1 (modify base ptr) */
		g_setReg(mod, GS_FRAME_1, GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0));
	}
	else
	{
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;

		/* set active buffer in FRAME_2 (modify base ptr) */
		g_setReg(mod, GS_FRAME_2, GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0));
	}
}

EXPORT TVOID 
g_flipBuffer(TMOD_PS2 *mod, TINT ctx, TINT fb)
{
	int fbp;
	GScontext *cont;
	GSdisplay *disp;
	static struct cbdata cbd;						/* FIXME */
	GSinfo *gsinfo = mod->ps2_GSInfo;
	
	cbd.fb = fb;
	cbd.ctx = ctx;
	cbd.gsinfo = gsinfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		
		if (ctx & GS_CTX2)	d = d_alloc(mod, DMC_GIF, 2, TNULL, TNULL);
		else				d = d_alloc(mod, DMC_GIF, 2, (DMACB) setvisiblebuf, (TAPTR) &cbd);
		
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
			
		/* set active buffer in FRAME_1 (modify base ptr) */
		if (fb & 1)	fbp = cont->gsc_fbp1;		/* !!! */
		else		fbp = cont->gsc_fbp0;		/* !!! */
	
		d[0] 		 = GIF_SET_GIFTAG(1, 1, 0, 0, 0, 1, 0xe);
		d[1].ul64[1] = GS_FRAME_1;
		d[1].ul64[0] = GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d = d_alloc(mod, DMC_GIF, 2, (DMACB) setvisiblebuf, (TAPTR) &cbd);
		
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;
			
		/* set active buffer in FRAME_2 (modify base ptr) */
		if (fb & 1)	fbp = cont->gsc_fbp1;		/* !!! */
		else		fbp = cont->gsc_fbp0;		/* !!! */
	
		d[0] 		 = GIF_SET_GIFTAG(1, 1, 0, 0, 0, 1, 0xe);
		d[1].ul64[1] = GS_FRAME_2;
		d[1].ul64[0] = GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0);
	}
}

TVOID flipBuffer(TMOD_PS2 *mod, TINT ctx, TINT fb)
{
	int fbp;
	GScontext *cont;
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;

	if (ctx & GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
		
		/* set visible buffer in DISPFB_1 (modify base ptr) */
		if (fb & 1)	fbp = cont->gsc_fbp1;
		else 		fbp = cont->gsc_fbp0;
		GS_SET_DISPFB1(fbp, disp->gsd_width/64, cont->gsc_psm, 0, 0);
		
		/* set active buffer in FRAME_1 (modify base ptr) */
		if (fb & 1)	fbp = cont->gsc_fbp0;
		else		fbp = cont->gsc_fbp1;
		g_setReg(mod, GS_FRAME_1, GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0));
	}
	
	if (ctx & GS_CTX2)
	{
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;
		
		/* set visible buffer in DISPFB_2 (modify base ptr) */
		if (fb & 1)	fbp = cont->gsc_fbp1;
		else 		fbp = cont->gsc_fbp0;
		GS_SET_DISPFB2(fbp, disp->gsd_width/64, cont->gsc_psm, 0, 0);
		
		/* set active buffer in FRAME_2 (modify base ptr) */
		if (fb & 1)	fbp = cont->gsc_fbp0;
		else		fbp = cont->gsc_fbp1;
		g_setReg(mod, GS_FRAME_2, GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0));
	}
}

EXPORT TVOID
g_setClearColor(TMOD_PS2 *mod, TINT ctx, gs_rgbaq_packed *rgba)
{
	GScontext *cont;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		cont->gsc_clear = *(gs_rgbaq_packed *)rgba;
	}
	
	if (ctx & GS_CTX2)
	{
		cont = &gsinfo->gsi_ctx2;
		cont->gsc_clear = *(gs_rgbaq_packed *)rgba;
	}
}

EXPORT TVOID 
g_setDepthFunc(TMOD_PS2 *mod, TINT ctx, TINT dfunc)
{
	GScontext *cont;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		cont->gsc_dfunc = dfunc;
	}
	
	if (ctx & GS_CTX2)
	{
		cont = &gsinfo->gsi_ctx2;
		cont->gsc_dfunc = dfunc;
	}
}

EXPORT TVOID
g_setDepthClear(TMOD_PS2 *mod, TINT ctx, TINT clear)
{
	GScontext *cont;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		cont->gsc_dclear = clear;
	}
	
	if (ctx & GS_CTX2)
	{
		cont = &gsinfo->gsi_ctx2;
		cont->gsc_dclear = clear;
	}
}

/***********************************************************************************************
  GS Image related functions
 ***********************************************************************************************/

EXPORT TINT
g_initImage(TMOD_PS2 *mod, GSimage *gsimage, TINT w, TINT h, TINT psm, TAPTR data)
{
	GSinfo *gsinfo = mod->ps2_GSInfo;
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
g_loadImage(TMOD_PS2 *mod, GSimage *gsimage)
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
	
	g_setReg(mod, GS_BITBLTBUF, GS_SETREG_BITBLTBUF(0,0,0,gsimage->tbp,gsimage->tbw,gsimage->psm));
	g_setReg(mod, GS_TRXPOS, 	GS_SETREG_TRXPOS(0,0,0,0,0));
	g_setReg(mod, GS_TRXREG, 	GS_SETREG_TRXREG(gsimage->w, gsimage->h));
	g_setReg(mod, GS_TRXDIR, 	GS_SETREG_TRXDIR(0));

	d = d_alloc(mod, DMC_GIF, qwc+1, TNULL, TNULL);
	d[0].ui32[0] = 0x00008000+qwc;
	d[0].ui32[1] = 0x08000000;
	d[0].ui32[2] = 0x00000000;
	d[0].ui32[3] = 0x00000000;

	for (i = 0; i < qwc; i++)
	{
		d[i+1].ul128 = *(TUINT128 *)tptr;
		tptr += 16;
	}
				
	g_setReg(mod, GS_TEXFLUSH, 0x42);
}

EXPORT TVOID
g_getImage(TMOD_PS2 *mod, GSimage *img)
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
	
	g_setReg(mod, GS_BITBLTBUF, GS_SETREG_BITBLTBUF(img->tbp,img->tbw,img->psm, 0, 0, 0));
	g_setReg(mod, GS_TRXPOS, 	GS_SETREG_TRXPOS(0,0,0,0,0));
	g_setReg(mod, GS_TRXREG, 	GS_SETREG_TRXREG(img->w, img->h));
	g_setReg(mod, GS_FINISH, 	1);
	g_setReg(mod, GS_TRXDIR, 	GS_SETREG_TRXDIR(1));
	
	d_commit(mod, DMC_GIF);
	d_commit(mod, DMC_GIF);

	while(!(*(TUINT volatile *)GS_CSR & 0x2))	;
	*(TUINT volatile *)GS_CSR = *(TUINT volatile *)GS_CSR & 0x0;

	g_syncPath(mod, 0);
	
	*(TUINT volatile *)VIF1_STAT = 0x800000;
	*(TUINT volatile *)GS_BUSDIR = 0x1;

	DMA_SET_QWC ((TUINT *)D1_QWC, qwc);
	DMA_SET_MADR((TUINT *)D1_MADR, img->data, 0);
	DMA_SET_CHCR((TUINT *)D1_CHCR, 0, 0, 0, 0, 0, 1, 0);
	DMA_WAIT((TUINT *)D1_CHCR);
	
	g_syncPath(mod, 0);
	
	*(TUINT volatile *)VIF1_STAT = 0x0;
	*(TUINT volatile *)GS_BUSDIR = 0x0;
	
	FlushCache(0);
}

EXPORT TVOID 
g_freeImage(TMOD_PS2 *mod, GSimage *gsimage)
{
	GSinfo *gsinfo = mod->ps2_GSInfo;
	GStexenv *tenv = &gsinfo->gsi_texenv;
	txa_free(&tenv->gst_txalloc, gsimage->psm, gsimage->w, gsimage->h, gsimage->tbp);
}

/***********************************************************************************************
  GS Setup
 ***********************************************************************************************/

static TVOID 
setpmode(TINT ctx)
{
	switch (ctx)
	{
		case GS_CTX1:
			GS_SET_PMODE(
				1,			/* Read Circuit 1 ON 									*/
				0,			/* Read Circuit 2 OFF									*/
				1,			/* ALP Register value									*/
				0,			/* Alpha Value of Read Circuit 1 for Output selection	*/
				1,			/* Blend Alpha with background color					*/
				0xff		/* Fixed Alpha Value = 1.0								*/
			);
			
			break;
			
		case GS_CTX2:
			GS_SET_PMODE(
				0,			/* Read Circuit 1 OFF 									*/
				1,			/* Read Circuit 2 ON									*/
				1,			/* ALP Register value									*/
				1,			/* Alpha Value of Read Circuit 2 for Output selection	*/
				0,			/* Blend Alpha with the output of Read Circuit 2		*/
				0xff		/* Fixed Alpha Value = 1.0								*/
			);

			break;
			
		case GS_CTX1 | GS_CTX2:
			GS_SET_PMODE(
				1,			/* Read Circuit 1 ON 									*/
				1,			/* Read Circuit 2 ON									*/
				0,			/* Alpha Value of Read Circuit 1 for Alpha Blending		*/
				1,			/* Alpha Value of Read Circuit 2 for Output selection	*/
				0,			/* Blend Alpha with the Output of Read Circuit 2		*/
				0xff		/* Fixed Alpha Value = 1.0								*/
			);
		
			break;
		
		default:
			*(TINT *) 0 = 0;
	}

}

EXPORT TVOID 
g_initScreen(TMOD_PS2 *mod, GSvmode mode, TINT inter, TINT ffmd)
{
	GSinfo *gsinfo = mod->ps2_GSInfo;
	const vmode_t *v = &(vmodes[mode]);

	d_reset();

	GS_RESET();
	EE_SYNC();

	GsPutIMR(0xff00);
	SetGsCrt(inter, v->ntsc_pal, ffmd);
	
	gsinfo->gsi_screen.gss_vmode = mode;
	gsinfo->gsi_screen.gss_inter = inter;
	gsinfo->gsi_screen.gss_ffmd = ffmd;
	gsinfo->gsi_screen.gss_ctx = 0;
	g_initlmalloc(mod, &gsinfo->gsi_screen.gss_lmalloc);
}

EXPORT TVOID 
g_initDisplay(TMOD_PS2 *mod, TINT ctx, TINT dx, TINT dy, TINT magh, TINT magv,
	TINT w, TINT h, TINT d, TINT xc, TINT yc)
{
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	TINT inter = gsinfo->gsi_screen.gss_inter;
	
	if (ctx == GS_CTX1)
	{
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		gsinfo->gsi_screen.gss_ctx |= GS_CTX1;
		
		GS_SET_DISPLAY1(
			dx,					/* X position in the display area (in VCK units)		*/
			inter ? dy*2 : dy,	/* Y position in the display area (in Raster units)		*/
			magh-1,				/* Horizontal Magnification - 1							*/
			magv-1,				/* Vertical Magnification = 1x							*/
			w*magh-1,			/* Display area width - 1 (in VCK units) (Width*HMag-1)	*/
			inter ? h*2-1 : h-1	/* Display area height - 1 (in pixels)	  (Height-1)	*/
		);
		
		g_setReg(mod, GS_XYOFFSET_1, GS_SETREG_XYOFFSET((xc - w/2)<<4, (yc - h/2)<<4));
		g_setReg(mod, GS_SCISSOR_1, GS_SETREG_SCISSOR(0, w - 1, 0, h - 1));
	}
	else
	{
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		gsinfo->gsi_screen.gss_ctx |= GS_CTX2;
		
		GS_SET_DISPLAY2(
			dx,					/* X position in the display area (in VCK units)		*/
			inter ? dy*2 : dy,	/* Y position in the display area (in Raster units)		*/
			magh-1,				/* Horizontal Magnification - 1							*/
			magv-1,				/* Vertical Magnification = 1x							*/
			w*magh-1,			/* Display area width - 1 (in VCK units) (Width*HMag-1)	*/
			inter ? h*2-1 : h-1	/* Display area height - 1 (in pixels)	  (Height-1)	*/	
		);
		
		g_setReg(mod, GS_XYOFFSET_2, GS_SETREG_XYOFFSET((xc - w/2)<<4, (yc - h/2)<<4));
		g_setReg(mod, GS_SCISSOR_2, GS_SETREG_SCISSOR(0, w - 1, 0, h - 1));
	}

	// testweise in g_init()
	// setpmode(gsinfo->gsi_screen.gss_ctx);
	
	disp->gsd_dx = dx;
	disp->gsd_dy = dy;
	disp->gsd_magh = magh;
	disp->gsd_magv = magv;
	disp->gsd_width = w;
	disp->gsd_height = h;
	disp->gsd_depth = d;
	disp->gsd_xcenter = xc;	
	disp->gsd_ycenter = yc;
	disp->gsd_xoffset = xc - w/2;
	disp->gsd_yoffset = yc - h/2;
}

EXPORT TVOID 
g_initContext(TMOD_PS2 *mod, TINT ctx, TINT fbp0, TINT fbp1, TINT psm)
{
	TINT w;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	GScontext *cont;
	
	if (ctx == GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		w = cont->gsc_disp.gsd_width;
			
		GS_SET_DISPFB1(
			fbp1,		/* Frame Buffer base pointer	*/
			w/64,		/* Buffer Width (Address/64)	*/
			psm,		/* Pixel Storage Format			*/
			0,			/* Upper Left X in Buffer = 0	*/
			0			/* Upper Left Y in Buffer = 0	*/
		);
		
		g_setReg(mod, GS_FRAME_1, GS_SETREG_FRAME(fbp0, w/64, psm, 0));
	}
	else
	{
		cont = &gsinfo->gsi_ctx2;
		w = cont->gsc_disp.gsd_width;
			
		GS_SET_DISPFB2(
			fbp1,		/* Frame Buffer base pointer	*/
			w/64,		/* Buffer Width (Address/64)	*/
			psm,		/* Pixel Storage Format			*/
			0,			/* Upper Left X in Buffer = 0	*/
			0			/* Upper Left Y in Buffer = 0	*/
		);
		
		g_setReg(mod, GS_FRAME_2, GS_SETREG_FRAME(fbp0, w/64, psm, 0));
	}
	
	cont->gsc_psm = psm;	
	cont->gsc_fbp0 = fbp0;
	cont->gsc_fbp1 = fbp1;

	cont->gsc_zpsm = 0;
	cont->gsc_zbits = 0;
	cont->gsc_zbp = 0;
	cont->gsc_dfunc = 0;
	cont->gsc_dclear = 0;
	TFillMem(&cont->gsc_clear, 16, 0);
}

EXPORT TVOID 
g_enableContext(TMOD_PS2 *mod, TINT ctx, TBOOL onoff)
{
	GSinfo *gsinfo = mod->ps2_GSInfo;
	if (onoff)
	{
		if (ctx & GS_CTX1)	gsinfo->gsi_screen.gss_ctx |= GS_CTX1;
		if (ctx & GS_CTX2)	gsinfo->gsi_screen.gss_ctx |= GS_CTX2;
	}
	else
	{
		if (ctx & GS_CTX1)	gsinfo->gsi_screen.gss_ctx &= ~GS_CTX1;
		if (ctx & GS_CTX2)	gsinfo->gsi_screen.gss_ctx &= ~GS_CTX2;
	}
	setpmode(gsinfo->gsi_screen.gss_ctx);
}

static TINT getZBits(TINT zpsm)
{
	switch(zpsm)
	{
		case GS_PSMZ32:		return 32;
		case GS_PSMZ24:		return 24;
		case GS_PSMZ16:		return 16;
		case GS_PSMZ16S:	return 16;
	}
	
	*(TINT *) 0 = 0;
	return 0;
}

EXPORT TVOID 
g_initZBuf(TMOD_PS2 *mod, TINT ctx, TINT zbp, TINT zpsm, TINT dfunc, TINT dclear)
{
	GSinfo *gsinfo = mod->ps2_GSInfo;
	GScontext *cont;
	
	if (ctx == GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		
		cont->gsc_zpsm = zpsm;
		cont->gsc_zbits = getZBits(zpsm);
		cont->gsc_zbp = zbp;
		cont->gsc_dfunc = dfunc;
		cont->gsc_dclear = dclear;
		
		g_enableZBuf(mod, GS_CTX1);
	}
	else
	{
		cont = &gsinfo->gsi_ctx2;
		
		cont->gsc_zpsm = zpsm;
		cont->gsc_zbits = getZBits(zpsm);
		cont->gsc_zbp = zbp;
		cont->gsc_dfunc = dfunc;
		cont->gsc_dclear = dclear;
		
		g_enableZBuf(mod, GS_CTX2);
	}
}

EXPORT TVOID 
g_initTexEnv(TMOD_PS2 *mod, TINT tbp, TINT tw, TINT th)
{
	struct TPS2ModPrivate *priv = mod->ps2_Private;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	GStexenv *tenv = &gsinfo->gsi_texenv;
	
	tenv->gst_tbp = tbp;
	g_setReg(mod, GS_TEXA, GS_SETREG_TEXA(0xff,0,0xff));
	g_setReg(mod, GS_PRMODECONT, GS_SETREG_PRMODECONT(1));
	
	/* init texture allocator */
	tenv->gst_txalloc.txa_PWidth = tw;
	tenv->gst_txalloc.txa_PHeight = th;
	tenv->gst_txalloc.txa_PAlloc = (TUINT8 *) TAlloc(priv->ps2_MMU, tw*th*32);
	TFillMem(tenv->gst_txalloc.txa_PAlloc, tw*th*32, 0x0);
}

EXPORT TVOID
g_initTexReg(TMOD_PS2 *mod, TINT ctx, TINT tcc, TINT tfx, GSimage *gsimage)
{
	TINT tw, th;
	
	tw = u_ld(gsimage->w);
	th = u_ld(gsimage->h);

	if (ctx == GS_CTX1)
		g_setReg(mod, GS_TEX0_1, GS_SETREG_TEX0(gsimage->tbp,gsimage->tbw,gsimage->psm,tw,th,tcc,tfx,0,0,0,0,0));
	
	if (ctx == GS_CTX2)
		g_setReg(mod, GS_TEX0_2, GS_SETREG_TEX0(gsimage->tbp,gsimage->tbw,gsimage->psm,tw,th,tcc,tfx,0,0,0,0,0));
}

EXPORT TVOID 
g_init(TMOD_PS2 *mod, GSvmode mode, TINT inter, TINT ffmd)
{
	GSinfo *gsinfo = mod->ps2_GSInfo;
	TINT fbp1_0, fbp2_0, fbp2_1, zbp2;
	
	g_initScreen(mod, mode, inter, ffmd);
	
//	g_initDisplay(mod, GS_CTX1, 800, 38, 7, 1, CTX1_W, CTX1_H, 32, 2048, 2048);
	g_initDisplay(mod, GS_CTX1, 652+160, 37, 7, 1, 320, 256, 32, 2048, 2048);
//	g_initDisplay(mod, GS_CTX1, 652+40, 37, 4, 1, 640, 256, 32, 2048, 2048);
	g_initDisplay(mod, GS_CTX2, 652+40, 37, 4, 1, 640, 256, 32, 2048, 2048);
	
	zbp2   = g_allocMem(mod, GS_CTX2, GS_LMZBUF, GS_PSMZ16S);
	fbp2_1 = g_allocMem(mod, GS_CTX2, GS_LMFBUF, GS_PSMCT32);
	fbp2_0 = g_allocMem(mod, GS_CTX2, GS_LMFBUF, GS_PSMCT32);
	fbp1_0 = g_allocMem(mod, GS_CTX1, GS_LMFBUF, GS_PSMCT32);
	
	tdbprintf1(5, "fbp1_0: %d\n", fbp1_0);
	tdbprintf1(5, "fbp2_0: %d\n", fbp2_0);
	tdbprintf1(5, "fbp2_1: %d\n", fbp2_1);
	tdbprintf1(5, "zbp2:   %d\n", zbp2);
	
	g_initContext(mod, GS_CTX1, fbp1_0, fbp1_0, GS_PSMCT32);
	g_initContext(mod, GS_CTX2, fbp2_0, fbp2_1, GS_PSMCT32); /* zbp: 200 tpb: 11520 */
	
	g_initTexEnv(mod, 0, 16, 20);
	g_initZBuf(mod, GS_CTX2, zbp2, GS_PSMZ16S, GS_ZTEST_ALWAYS, 0);
	
	g_setReg(mod, GS_ALPHA_1, GS_SETREG_ALPHA(0,0,0,0,0));
	g_setReg(mod, GS_ALPHA_2, GS_SETREG_ALPHA(0,1,0,1,0));

	g_setClearColor(mod, GS_CTX2 | GS_CTX1, G_GETCOL(0,0,0,0));
	
	g_clearScreen(mod, GS_CTX1 | GS_CTX2);

	g_flipBuffer(mod, GS_CTX2, 1);
	d_commit(mod, DMC_GIF);
	g_clearScreen(mod, GS_CTX1 | GS_CTX2);
	d_commit(mod, DMC_GIF);
	d_commit(mod, DMC_GIF);

	setpmode(gsinfo->gsi_screen.gss_ctx);	
}

/***********************************************************************************************
	local memory allocator (backwards)
 ***********************************************************************************************/

static TVOID g_initlmalloc(TMOD_PS2 *mod, struct LMAlloc *lma)
{
	TFillMem(lma->lma_AllocPages, LMA_NUMPAGES, 0);
}

static TINT g_lmalloc(TMOD_PS2 *mod, struct LMAlloc *lma, TINT numpages)
{
	TINT x, f = 0;
	for (x = LMA_NUMPAGES - 1; x >= 0; x--)
	{
		if (lma->lma_AllocPages[x] == 0)
		{
			if (++f == numpages)
			{
				TFillMem(lma->lma_AllocPages + x, f, 1);
				return x;
			}
		}
		else
		{
			f = 0;
		}
	}
	return -1;
}

static TINT g_lmfree(TMOD_PS2 *mod, struct LMAlloc *lma, TINT startpage, TINT numpages)
{
	TFillMem(lma->lma_AllocPages + startpage, numpages, 0);
}

EXPORT TINT 
g_allocMem(TMOD_PS2 *mod, TINT ctx, TINT memtype, TINT psm)
{
	GSinfo *gsinfo = mod->ps2_GSInfo;
	TINT np;
	GSdisplay *disp;
	
	if (ctx == GS_CTX1)	disp = &gsinfo->gsi_ctx1.gsc_disp;
	else				disp = &gsinfo->gsi_ctx2.gsc_disp;

	switch (memtype)
	{
		default:
			*(TINT *) 0 = 0;

		case GS_LMZBUF:
			np = (disp->gsd_width * disp->gsd_height) / (psm & 0x2 ? 4096 : 2048);
			break;
	
		case GS_LMFBUF:
			np = (disp->gsd_width * disp->gsd_height) / (psm & 0x2 ? 4096 : 2048);
			break;
	}

	return(g_lmalloc(mod, &gsinfo->gsi_screen.gss_lmalloc, np));
}

EXPORT TVOID 
g_freeMem(TMOD_PS2 *mod, TINT ctx, TINT memtype, TINT startpage)
{
	GSinfo *gsinfo = mod->ps2_GSInfo;
	TINT np;
	GSdisplay *disp;
	GScontext *cont;
	
	if (ctx == GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		disp = &gsinfo->gsi_ctx1.gsc_disp;
	}
	else
	{
		cont = &gsinfo->gsi_ctx2;
		disp = &gsinfo->gsi_ctx2.gsc_disp;
	}
	
	switch (memtype)
	{
		default:
			*(TINT *) 0 = 0;

		case GS_LMZBUF:
			np = (disp->gsd_width * disp->gsd_height) / (cont->gsc_zpsm & 0x2 ? 4096 : 2048);
			break;
	
		case GS_LMFBUF:
			np = (disp->gsd_width * disp->gsd_height) / (cont->gsc_psm & 0x2 ? 4096 : 2048);
			break;
	}

	g_lmfree(mod, &gsinfo->gsi_screen.gss_lmalloc, startpage, np);
}

/***********************************************************************************************
  primitive drawing function
 ***********************************************************************************************/

EXPORT TVOID 
g_drawTRect(TMOD_PS2 *mod, TINT ctx, TINT x, TINT y, TINT w, TINT h, GSimage *gsimage)
{
	TINT dx, dy;
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		dx = disp->gsd_xoffset;
		dy = disp->gsd_yoffset;
		
		g_initTexReg(mod, GS_CTX1, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
		
		d = d_alloc(mod, DMC_GIF, 5, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x156, 0, 4, 0x5353);
		d[1] = GIF_SET_UV(0,0);
		d[2] = GIF_SET_XYZ(dx+x,dy+y,1);
		d[3] = GIF_SET_UV(gsimage->w,gsimage->h);
		d[4] = GIF_SET_XYZ(dx+x+w, dy+y+h, 1);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d;
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		dx = disp->gsd_xoffset;
		dy = disp->gsd_yoffset;
		
		g_initTexReg(mod, GS_CTX2, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
		
		d = d_alloc(mod, DMC_GIF, 5, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x356, 0, 4, 0x5353);
		d[1] = GIF_SET_UV(0,0);
		d[2] = GIF_SET_XYZ(dx+x,dy+y,1);
		d[3] = GIF_SET_UV(gsimage->w,gsimage->h);
		d[4] = GIF_SET_XYZ(dx+x+w, dy+y+h, 1);
	}
}

EXPORT TVOID 
g_drawFRect(TMOD_PS2 *mod, TINT ctx, TINT x, TINT y, TINT w, TINT h, gs_rgbaq_packed *rgb)
{
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;

	if (ctx & GS_CTX1)
	{
		TQWDATA *d = d_alloc(mod, DMC_GIF, 4, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x6, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x, disp->gsd_yoffset+y, 1);
		d[3]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y+h, 1);
	}

	if (ctx & GS_CTX2)
	{
		TQWDATA *d = d_alloc(mod, DMC_GIF, 4, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x206, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x, disp->gsd_yoffset+y, 1);
		d[3]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y+h, 1);
	}
}

EXPORT TVOID 
g_drawRect(TMOD_PS2 *mod, TINT ctx, TINT x, TINT y, TINT w, TINT h, gs_rgbaq_packed *rgb)
{
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;

	if (ctx & GS_CTX1)
	{
		TQWDATA *d = d_alloc(mod, DMC_GIF, 10, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		
		d[0].ui32[0] = 0x00008001;
		d[0].ui32[1] = 0x9000c000;
		d[0].ui32[2] = 0x55555551;
		d[0].ui32[3] = 0x00000005;
	
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x,   disp->gsd_yoffset+y,   1);
		d[3]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y,   1);
		d[4]         = GIF_SET_XYZ(disp->gsd_xoffset+x,   disp->gsd_yoffset+y,   1);
		d[5]         = GIF_SET_XYZ(disp->gsd_xoffset+x,   disp->gsd_yoffset+y+h, 1);
		d[6]         = GIF_SET_XYZ(disp->gsd_xoffset+x,   disp->gsd_yoffset+y+h, 1);
		d[7]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y+h, 1);
		d[8]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y,   1);
		d[9]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y+h, 1);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d = d_alloc(mod, DMC_GIF, 10, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		
		d[0].ui32[0] = 0x00008001;
		d[0].ui32[1] = 0x9100c000;
		d[0].ui32[2] = 0x55555551;
		d[0].ui32[3] = 0x00000005;
	
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x,   disp->gsd_yoffset+y,   1);
		d[3]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y,   1);
		d[4]         = GIF_SET_XYZ(disp->gsd_xoffset+x,   disp->gsd_yoffset+y,   1);
		d[5]         = GIF_SET_XYZ(disp->gsd_xoffset+x,   disp->gsd_yoffset+y+h, 1);
		d[6]         = GIF_SET_XYZ(disp->gsd_xoffset+x,   disp->gsd_yoffset+y+h, 1);
		d[7]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y+h, 1);
		d[8]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y,   1);
		d[9]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y+h, 1);
	}
}

EXPORT TVOID 
g_drawLine(TMOD_PS2 *mod, TINT ctx, TINT x1, TINT y1, TINT x2, TINT y2, gs_rgbaq_packed *rgb)
{
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d = d_alloc(mod, DMC_GIF, 4, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x1, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x1, disp->gsd_yoffset+y1, 1);
		d[3]         = GIF_SET_XYZ(disp->gsd_xoffset+x2, disp->gsd_yoffset+y2, 1);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d = d_alloc(mod, DMC_GIF, 4, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x201, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x1, disp->gsd_yoffset+y1, 1);
		d[3]         = GIF_SET_XYZ(disp->gsd_xoffset+x2, disp->gsd_yoffset+y2, 1);
	}
}

EXPORT TVOID 
g_drawPoint(TMOD_PS2 *mod, TINT ctx, TINT x, TINT y, gs_rgbaq_packed *rgb)
{
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d = d_alloc(mod, DMC_GIF, 3, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x0, 0, 2, 0x51);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x, disp->gsd_yoffset+y, 1);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d = d_alloc(mod, DMC_GIF, 3, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x200, 0, 2, 0x51);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x, disp->gsd_yoffset+y, 1);
	}
}

EXPORT TVOID 
g_drawTRectUV(TMOD_PS2 *mod, TINT ctx, TINT x0, TINT y0, TINT x1, TINT y1, TINT u0, TINT v0, TINT u1, TINT v1, GSimage *gsimage)
{
	TINT dx, dy;
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;

	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		dx = disp->gsd_xoffset;
		dy = disp->gsd_yoffset;
		
		g_initTexReg(mod, GS_CTX1, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
	
		d = d_alloc(mod, DMC_GIF, 5, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x156, 0, 4, 0x5353);
		d[1] = GIF_SET_UV(u0, v0);
		d[2] = GIF_SET_XYZ(dx + x0, dy + y0, 1);
		d[3] = GIF_SET_UV(u1, v1);
		d[4] = GIF_SET_XYZ(dx + x1, dy + y1, 1);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d;
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		dx = disp->gsd_xoffset;
		dy = disp->gsd_yoffset;
		
		g_initTexReg(mod, GS_CTX2, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
	
		d = d_alloc(mod, DMC_GIF, 5, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x356, 0, 4, 0x5353);
		d[1] = GIF_SET_UV(u0, v0);
		d[2] = GIF_SET_XYZ(dx + x0, dy + y0, 1);
		d[3] = GIF_SET_UV(u1, v1);
		d[4] = GIF_SET_XYZ(dx + x1, dy + y1, 1);
	}
}


EXPORT TVOID 
g_drawTPointUV(TMOD_PS2 *mod, TINT ctx, TINT x, TINT y, TINT u, TINT v, GSimage *gsimage)
{
	TINT dx, dy;
	GSdisplay *disp;
	GSinfo *gsinfo = mod->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		dx = disp->gsd_xoffset;
		dy = disp->gsd_yoffset;
		
		g_initTexReg(mod, GS_CTX1, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
	
		d = d_alloc(mod, DMC_GIF, 3, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x150, 0, 2, 0x53);
		d[1] = GIF_SET_UV(u, v);
		d[2] = GIF_SET_XYZ(dx + x, dy + y, 1);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d;
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		dx = disp->gsd_xoffset;
		dy = disp->gsd_yoffset;
		
		g_initTexReg(mod, GS_CTX2, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
	
		d = d_alloc(mod, DMC_GIF, 3, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x310, 0, 2, 0x53);
		d[1] = GIF_SET_UV(u, v);
		d[2] = GIF_SET_XYZ(dx + x, dy + y, 1);
	}
}


/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_gs.c,v $
**	Revision 1.3  2005/10/07 12:22:06  fschulze
**	renamed some primitive drawing functions
**	
**	Revision 1.2  2005/10/05 22:03:55  fschulze
**	added new gs functions: local mem downloads, syncing of paths and
**	activation of framebuffers; renamed setactivebuf to setvisiblebuf;
**	fixed interface definition of GSVSync
**	
**	Revision 1.1  2005/09/18 12:40:09  fschulze
**	added
**	
**	
*/
