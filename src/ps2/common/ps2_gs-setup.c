/*
**	$Id: ps2_gs-setup.c,v 1.1 2007/05/19 13:57:57 fschulze Exp $
**	teklib/mods/ps2/common/ps2_gs.c - GS setup
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**  Local Memory allocator written by Timm S. Mueller
*/

#include <tek/inline/ps2.h>
#include <tek/mod/ps2/gif.h>
#include <kernel.h>		/* FIXME */
#include "ps2_mod.h"

static TVOID gs_setpmode(TMOD_PS2 * TPS2Base, TINT ctx);
static TINT  gs_getZBits(TMOD_PS2 * TPS2Base, TINT zpsm);
static TVOID gs_initlmalloc(TMOD_PS2 * TPS2Base, struct LMAlloc *lma);
static TINT  gs_lmalloc(TMOD_PS2 * TPS2Base, struct LMAlloc *lma, TINT numpages);
static TINT  gs_lmfree(TMOD_PS2 * TPS2Base, struct LMAlloc *lma, TINT startpage, TINT numpages);

/***********************************************************************************************
  GS Setup
 ***********************************************************************************************/

static TVOID
gs_setpmode(TMOD_PS2 *TPS2Base, TINT ctx)
{
	switch (ctx)
	{
		case GS_CTX1:
			GS_SET_PMODE(
				1,			/* Read Circuit 1 ON  */
				0,			/* Read Circuit 2 OFF */
				1,			/* ALP Register value */
				0,			/* Alpha Value of Read Circuit 1 for Output selection */
				1,			/* Blend Alpha with background color */
				0xff		/* Fixed Alpha Value = 1.0 */
			);

			break;

		case GS_CTX2:
			GS_SET_PMODE(
				0,			/* Read Circuit 1 OFF */
				1,			/* Read Circuit 2 ON  */
				1,			/* ALP Register value */
				1,			/* Alpha Value of Read Circuit 2 for Output selection */
				0,			/* Blend Alpha with the output of Read Circuit 2 */
				0xff		/* Fixed Alpha Value = 1.0 */
			);

			break;

		case GS_CTX1 | GS_CTX2:
			GS_SET_PMODE(
				1,			/* Read Circuit 1 ON */
				1,			/* Read Circuit 2 ON */
				0,			/* Alpha Value of Read Circuit 1 for Alpha Blending	*/
				1,			/* Alpha Value of Read Circuit 2 for Output selection */
				0,			/* Blend Alpha with the Output of Read Circuit 2 */
				0xff		/* Fixed Alpha Value = 1.0 */
			);

			break;

		default:
			*(TINT *) 0 = 0;
	}

}

EXPORT TVOID
gs_initScreen(TMOD_PS2 *TPS2Base, TUINT16 mode, TUINT16 inter, TUINT16 ffmd)
{
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;

	dma_reset();

	GS_RESET();
	EE_SYNC();

	GsPutIMR(0xff00);
	SetGsCrt(inter, mode, ffmd);

	gsinfo->gsi_screen.gss_vmode = mode;
	gsinfo->gsi_screen.gss_inter = inter;
	gsinfo->gsi_screen.gss_ffmd = ffmd;
	gsinfo->gsi_screen.gss_ctx = 0;
	gs_initlmalloc(TPS2Base, &gsinfo->gsi_screen.gss_lmalloc);
}

EXPORT TVOID
gs_initDisplay(TMOD_PS2 *TPS2Base, TINT ctx, TINT dx, TINT dy, TINT magh, TINT magv,
	TINT w, TINT h, TINT d, TINT xc, TINT yc)
{
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	TINT inter = gsinfo->gsi_screen.gss_inter;
	TINT ffmd = gsinfo->gsi_screen.gss_ffmd;

	if (ctx == GS_CTX1)
	{
		disp = &gsinfo->gsi_ctx1.gsc_disp;

		GS_SET_DISPLAY1(
			dx,					/* X position in the display area (in VCK units)	*/
			inter ? dy*2 : dy,	/* Y position in the display area (in Raster units)	*/
			magh-1,				/* Horizontal Magnification - 1	*/
			magv-1,				/* Vertical Magnification = 1x	*/
			w*magh-1,			/* Display area width - 1 (in VCK units) (Width*HMag-1)	*/
			ffmd ? h*2-1 : h-1	/* Display area height - 1 (in pixels)	  (Height-1)	*/
		);

		g_setReg(GS_XYOFFSET_1, GS_SETREG_XYOFFSET((xc - w/2)<<4, (yc - h/2)<<4));
		g_setReg(GS_SCISSOR_1, GS_SETREG_SCISSOR(0, w - 1, 0, h - 1));
	}
	else
	{
		disp = &gsinfo->gsi_ctx2.gsc_disp;

		GS_SET_DISPLAY2(
			dx,					/* X position in the display area (in VCK units)	*/
			inter ? dy*2 : dy,	/* Y position in the display area (in Raster units)	*/
			magh-1,				/* Horizontal Magnification - 1	*/
			magv-1,				/* Vertical Magnification = 1x	*/
			w*magh-1,			/* Display area width - 1 (in VCK units) (Width*HMag-1)	*/
			ffmd ? h*2-1 : h-1	/* Display area height - 1 (in pixels)	  (Height-1)	*/
		);

		g_setReg(GS_XYOFFSET_2, GS_SETREG_XYOFFSET((xc - w/2)<<4, (yc - h/2)<<4));
		g_setReg(GS_SCISSOR_2, GS_SETREG_SCISSOR(0, w - 1, 0, h - 1));
	}

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
gs_initFramebuffer(TMOD_PS2 *TPS2Base, TINT ctx, TINT fbp0, TINT fbp1, TINT psm)
{
	TINT w;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	GScontext *cont;

	if (ctx == GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		w = cont->gsc_disp.gsd_width;

		GS_SET_DISPFB1(
			fbp1,		/* Frame Buffer base pointer */
			w/64,		/* Buffer Width (Address/64) */
			psm,		/* Pixel Storage Format	*/
			0,			/* Upper Left X in Buffer = 0 */
			0			/* Upper Left Y in Buffer = 0 */
		);

		g_setReg(GS_FRAME_1, GS_SETREG_FRAME(fbp0, w/64, psm, 0));
	}
	else
	{
		cont = &gsinfo->gsi_ctx2;
		w = cont->gsc_disp.gsd_width;

		GS_SET_DISPFB2(
			fbp1,		/* Frame Buffer base pointer */
			w/64,		/* Buffer Width (Address/64) */
			psm,		/* Pixel Storage Format	*/
			0,			/* Upper Left X in Buffer = 0 */
			0			/* Upper Left Y in Buffer = 0 */
		);

		g_setReg(GS_FRAME_2, GS_SETREG_FRAME(fbp0, w/64, psm, 0));
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
gs_enableContext(TMOD_PS2 *TPS2Base, TINT ctx, TBOOL onoff)
{
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
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
	gs_setpmode(TPS2Base, gsinfo->gsi_screen.gss_ctx);
}

static TINT gs_getZBits(TMOD_PS2 *TPS2Base, TINT zpsm)
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
gs_initZBuf(TMOD_PS2 *TPS2Base, TINT ctx, TINT zbp, TINT zpsm, TINT dfunc, TINT dclear)
{
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	GScontext *cont;

	if (ctx == GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;

		cont->gsc_zpsm = zpsm;
		cont->gsc_zbits = gs_getZBits(TPS2Base, zpsm);
		cont->gsc_zbp = zbp;
		cont->gsc_dfunc = dfunc;
		cont->gsc_dclear = dclear;

		g_enableZBuf(GS_CTX1);
	}
	else
	{
		cont = &gsinfo->gsi_ctx2;

		cont->gsc_zpsm = zpsm;
		cont->gsc_zbits = gs_getZBits(TPS2Base, zpsm);
		cont->gsc_zbp = zbp;
		cont->gsc_dfunc = dfunc;
		cont->gsc_dclear = dclear;

		g_enableZBuf(GS_CTX2);
	}
}

EXPORT TVOID
gs_initTexEnv(TMOD_PS2 *TPS2Base, TINT tbp, TINT tw, TINT th)
{
	struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	GStexenv *tenv = &gsinfo->gsi_texenv;

	tenv->gst_tbp = tbp;
	g_setReg(GS_TEXA, GS_SETREG_TEXA(0xff,0,0xff));
	g_setReg(GS_PRMODECONT, GS_SETREG_PRMODECONT(1));

	/* init texture allocator */
	tenv->gst_txalloc.txa_PWidth = tw;
	tenv->gst_txalloc.txa_PHeight = th;
	tenv->gst_txalloc.txa_PAlloc = (TUINT8 *) TAlloc(priv->ps2_MM, tw*th*32);
	TFillMem(tenv->gst_txalloc.txa_PAlloc, tw*th*32, 0x0);
}

EXPORT TVOID
gs_initTexReg(TMOD_PS2 *TPS2Base, TINT ctx, TINT tcc, TINT tfx, GSimage *gsimage)
{
	TINT tw, th;

	tw = u_ld(gsimage->w);
	th = u_ld(gsimage->h);

	if (ctx == GS_CTX1)
		g_setReg(GS_TEX0_1, GS_SETREG_TEX0(gsimage->tbp,gsimage->tbw,gsimage->psm,tw,th,tcc,tfx,0,0,0,0,0));

	if (ctx == GS_CTX2)
		g_setReg(GS_TEX0_2, GS_SETREG_TEX0(gsimage->tbp,gsimage->tbw,gsimage->psm,tw,th,tcc,tfx,0,0,0,0,0));
}

EXPORT TVOID
gs_init(TMOD_PS2 *TPS2Base, TINT mode, TINT inter, TINT ffmd)
{
	TINT fbp1_0, fbp2_0, fbp2_1, zbp2;

	g_initScreen(mode, inter, ffmd);

	g_initDisplay(GS_CTX1, 652+160, 37, 7, 1, 320, 256, 32, 2048, 2048);
	g_initDisplay(GS_CTX2, 692, 37, 4, 1, 640, 256, 32, 2048, 2048);

	zbp2   = g_allocMem(GS_CTX2, GS_LMZBUF, GS_PSMZ24);
	fbp2_1 = g_allocMem(GS_CTX2, GS_LMFBUF, GS_PSMCT32);
	fbp2_0 = g_allocMem(GS_CTX2, GS_LMFBUF, GS_PSMCT32);
	fbp1_0 = g_allocMem(GS_CTX1, GS_LMFBUF, GS_PSMCT32);

	TDBPRINTF(2, ("fbp1_0: %d\n", fbp1_0));
	TDBPRINTF(2, ("fbp2_0: %d\n", fbp2_0));
	TDBPRINTF(2, ("fbp2_1: %d\n", fbp2_1));
	TDBPRINTF(2, ("zbp2:   %d\n", zbp2));

	g_initFramebuffer(GS_CTX1, fbp1_0, fbp1_0, GS_PSMCT32);
	g_initFramebuffer(GS_CTX2, fbp2_0, fbp2_1, GS_PSMCT32);

	g_initTexEnv(0, 16, 20);
	g_initZBuf(GS_CTX2, zbp2, GS_PSMZ24, GS_ZTEST_ALWAYS, 0);

	g_setReg(GS_ALPHA_1, GS_SETREG_ALPHA(0,0,0,0,0));
	g_setReg(GS_ALPHA_2, GS_SETREG_ALPHA(0,1,0,1,0));

	g_setClearColor(GS_CTX2 | GS_CTX1, G_GETCOL(0,0,0,0));

	g_clearScreen(GS_CTX1 | GS_CTX2);

	g_flipBuffer(GS_CTX2, 1);
	d_commit(DMC_GIF);
	g_clearScreen(GS_CTX1 | GS_CTX2);
	d_commit(DMC_GIF);
	d_commit(DMC_GIF);

	g_enableContext(GS_CTX2 | GS_CTX1, TTRUE);
}

/***********************************************************************************************
	local memory allocator (backwards)
 ***********************************************************************************************/

static TVOID gs_initlmalloc(TMOD_PS2 *TPS2Base, struct LMAlloc *lma)
{
	TFillMem(lma->lma_AllocPages, LMA_NUMPAGES, 0);
}

static TINT gs_lmalloc(TMOD_PS2 *TPS2Base, struct LMAlloc *lma, TINT numpages)
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

static TINT gs_lmfree(TMOD_PS2 *TPS2Base, struct LMAlloc *lma, TINT startpage, TINT numpages)
{
	TFillMem(lma->lma_AllocPages + startpage, numpages, 0);
}

EXPORT TINT
gs_allocMem(TMOD_PS2 *TPS2Base, TINT ctx, TINT memtype, TINT psm)
{
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
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

	return(gs_lmalloc(TPS2Base, &gsinfo->gsi_screen.gss_lmalloc, np));
}

EXPORT TVOID
gs_freeMem(TMOD_PS2 *TPS2Base, TINT ctx, TINT memtype, TINT startpage)
{
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
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

	gs_lmfree(TPS2Base, &gsinfo->gsi_screen.gss_lmalloc, startpage, np);
}


/***********************************************************************************************
  GS register related functions
 ***********************************************************************************************/

EXPORT TVOID
gs_setReg(TMOD_PS2 *TPS2Base, TINT reg, TUINT64 data)
{
	TQWDATA *d = d_alloc(DMC_GIF, 2, TNULL, TNULL);

	TPS2Base->ps2_GSInfo->gsi_regs[(reg & 0xff00)>>8] = data;

	d[0] 		 = GIF_SET_GIFTAG(1, 1, 0, 0, 0, 1, 0xe);
	d[1].ul64[1] = reg & 0xff;
	d[1].ul64[0] = data;
}

EXPORT TVOID
gs_setRegCb(TMOD_PS2 *TPS2Base, TINT reg, TUINT64 data, DMACB cbfunc, TAPTR cbdata)
{
	TQWDATA *d = d_alloc(DMC_GIF, 2, cbfunc, cbdata);

	TPS2Base->ps2_GSInfo->gsi_regs[(reg & 0xff00)>>8] = data;

	d[0] 		 = GIF_SET_GIFTAG(1, 1, 0, 0, 0, 1, 0xe);
	d[1].ul64[1] = reg & 0xff;
	d[1].ul64[0] = data;
}

EXPORT TUINT64
gs_getReg(TMOD_PS2 *TPS2Base, TINT reg)
{
	return TPS2Base->ps2_GSInfo->gsi_regs[(reg & 0xff00)>>8];
}

EXPORT TVOID
gs_set_csr(TMOD_PS2 *TPS2Base, TINT signal, TINT finish, TINT hsint, TINT vsint, TINT edwint,
			TINT flush, TINT reset, TINT nfield, TINT field, TINT fifo, TINT rev, TINT id)
{
	*(volatile TUINT64 *)(GS_CSR) =
		GS_SETREG_CSR(signal,finish,hsint,vsint,edwint,flush,reset,nfield,field,fifo,rev,id);
}

EXPORT TVOID
gs_set_pmode(TMOD_PS2 *TPS2Base, TINT en1, TINT en2, TINT mmod, TINT amod, TINT slbg, TINT alp)
{
	TUINT64 *regsave = &TPS2Base->ps2_GSInfo->gsi_regs[(GS_PRIV_PMODE & 0xff00)>>8];
	*regsave = GS_SETREG_PMODE(en1,en2,mmod,amod,slbg,alp);
	*(volatile TUINT64 *)(GS_PMODE) = *(TUINT64 *)regsave;
}

EXPORT TVOID
gs_set_smode2(TMOD_PS2 *TPS2Base, TINT inter, TINT ffmd, TINT dpms)
{
	TUINT64 *regsave = &TPS2Base->ps2_GSInfo->gsi_regs[(GS_PRIV_SMODE2 & 0xff00)>>8];
	*regsave = GS_SETREG_SMODE2(inter,ffmd,dpms);
	*(volatile TUINT64 *)(GS_SMODE2) = *(TUINT64 *)regsave;
}

EXPORT TVOID
gs_set_dispfb1(TMOD_PS2 *TPS2Base, TINT fbp, TINT fbw, TINT psm, TINT dbx, TINT dby)
{
	TUINT64 *regsave = &TPS2Base->ps2_GSInfo->gsi_regs[(GS_PRIV_DISPFB1 & 0xff00)>>8];
	*regsave = GS_SETREG_DISPFB1(fbp,fbw,psm,dbx,dby);
	*(volatile TUINT64 *)(GS_DISPFB1) = *(TUINT64 *)regsave;
}

EXPORT TVOID
gs_set_display1(TMOD_PS2 *TPS2Base, TINT dx, TINT dy, TINT magh, TINT magv, TINT dw, TINT dh)
{
	TUINT64 *regsave = &TPS2Base->ps2_GSInfo->gsi_regs[(GS_PRIV_DISPLAY1 & 0xff00)>>8];
	*regsave = GS_SETREG_DISPLAY1(dx,dy,magh,magv,dw,dh);
	*(volatile TUINT64 *)(GS_DISPLAY1) = *(TUINT64 *)regsave;
}

EXPORT TVOID
gs_set_dispfb2(TMOD_PS2 *TPS2Base, TINT fbp, TINT fbw, TINT psm, TINT dbx, TINT dby)
{
	TUINT64 *regsave = &TPS2Base->ps2_GSInfo->gsi_regs[(GS_PRIV_DISPFB2 & 0xff00)>>8];
	*regsave = GS_SETREG_DISPFB2(fbp,fbw,psm,dbx,dby);
	*(volatile TUINT64 *)(GS_DISPFB2) = *(TUINT64 *)regsave;
}

EXPORT TVOID
gs_set_display2(TMOD_PS2 *TPS2Base, TINT dx, TINT dy, TINT magh, TINT magv, TINT dw, TINT dh)
{
	TUINT64 *regsave = &TPS2Base->ps2_GSInfo->gsi_regs[(GS_PRIV_DISPLAY2 & 0xff00)>>8];
	*regsave = GS_SETREG_DISPLAY2(dx,dy,magh,magv,dw,dh);
	*(volatile TUINT64 *)(GS_DISPLAY2) = *(TUINT64 *)regsave;
}

EXPORT TVOID
gs_set_bgcolor(TMOD_PS2 *TPS2Base, TINT r, TINT g, TINT b)
{
	TUINT64 *regsave = &TPS2Base->ps2_GSInfo->gsi_regs[(GS_PRIV_BGCOLOR & 0xff00)>>8];
	*regsave = GS_SETREG_BGCOLOR(r,g,b);
	*(volatile TUINT64 *)(GS_BGCOLOR)= *(TUINT64 *)regsave;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_gs-setup.c,v $
**	Revision 1.1  2007/05/19 13:57:57  fschulze
**	restructered ps2_gs.c
**
**
*/
