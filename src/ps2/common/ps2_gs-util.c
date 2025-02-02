/*
**	$Id: ps2_gs-util.c,v 1.1 2007/05/19 13:57:57 fschulze Exp $
**	teklib/mods/ps2/common/ps2_gs.c - GS utils
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
*/

#include <tek/inline/ps2.h>
#include <tek/mod/ps2/gif.h>
#include <kernel.h>		/* FIXME */
#include "ps2_mod.h"

struct cbdata
{
	TINT fbp;
	TINT ctx;
	GSinfo *gsinfo;
	TMOD_PS2 *ps2base;
};

static TVOID setvisiblebuf(struct cbdata *cbd);

/***********************************************************************************************
  GS utility functions
 ***********************************************************************************************/

EXPORT TVOID
gs_vsync(TMOD_PS2 *TPS2Base)
{
	*(TUINT volatile *)GS_CSR = *(TUINT volatile *)GS_CSR & 0x8;
	while(!(*(TUINT volatile *)GS_CSR & 0x8))	;
}

#define GSP_ERR(s)					\
({									\
	if (timeout == 0)				\
	{								\
		TDBPRINTF(99, ("%s\n", s));	\
		return -1;					\
	}								\
})

EXPORT TINT
gs_syncPath(TMOD_PS2 *TPS2Base, TINT mode)
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
gs_enableZBuf(TMOD_PS2 *TPS2Base, TINT ctx)
{
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	GScontext *cont;
	GSdisplay *disp;

	if (ctx & GS_CTX1)
	{	
		TQWDATA *d;
		
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
		
		g_setReg(GS_ZBUF_1, GS_SETREG_ZBUF_1(cont->gsc_zbp, cont->gsc_zpsm, 0));
	
		/* clear zBuffer */
		g_setReg(GS_TEST_1, GS_SETREG_TEST_1(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));
	
		d = d_alloc(DMC_GIF, 4, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x6, 0, 3, 0x551);
		d[1] = GIF_SET_RGBA(0, 0, 0, 0);
		d[2] = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width, disp->gsd_yoffset+disp->gsd_height,cont->gsc_dclear);

		g_setReg(GS_TEST_1, GS_SETREG_TEST_1(0, 0, 0, 0, 0, 0, 1, cont->gsc_dfunc));
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d;
		
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;
		
		g_setReg(GS_ZBUF_2, GS_SETREG_ZBUF_2(cont->gsc_zbp, cont->gsc_zpsm, 0));
	
		/* clear zBuffer */
		g_setReg(GS_TEST_2, GS_SETREG_TEST_2(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));

		d = d_alloc(DMC_GIF, 4, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x206, 0, 3, 0x551);
		d[1] = GIF_SET_RGBA(0, 0, 0, 0);
		d[2] = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width, disp->gsd_yoffset+disp->gsd_height,cont->gsc_dclear);

		g_setReg(GS_TEST_2, GS_SETREG_TEST_2(0, 0, 0, 0, 0, 0, 1, cont->gsc_dfunc));
	}
}

EXPORT TVOID 
gs_clearScreen(TMOD_PS2 *TPS2Base, TINT ctx)
{
	GScontext *cont;
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
		
	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		TUINT64 test_1 = 0;
		
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
		
		test_1 = g_getReg(GS_TEST_1);
		g_setReg(GS_TEST_1, GS_SETREG_TEST_1(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));
		
		d = d_alloc(DMC_GIF, 4, TNULL, TNULL);
		
		d[0] 		 = GIF_SET_GIFTAG(1, 1, 1, 0x6, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)&cont->gsc_clear;
		d[2] 		 = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] 		 = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width +1,
							  disp->gsd_yoffset+disp->gsd_height+1, cont->gsc_dclear);
		
		g_setReg(GS_TEST_1, test_1);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d;
		TUINT64 test_2 = 0;
				
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;

		test_2 = g_getReg(GS_TEST_2);
		g_setReg(GS_TEST_2, GS_SETREG_TEST_2(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));
		
		d = d_alloc(DMC_GIF, 4, TNULL, TNULL);
		
		d[0] 		 = GIF_SET_GIFTAG(1, 1, 1, 0x206, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)&cont->gsc_clear;
		d[2] 		 = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] 		 = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width +1,
							  disp->gsd_yoffset+disp->gsd_height+1, cont->gsc_dclear);

		g_setReg(GS_TEST_2, test_2);
	}
}

EXPORT TVOID
gs_clearScreenAlpha(TMOD_PS2 *TPS2Base, TINT ctx)
{
	GScontext *cont;
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		TUINT64 test_1 = 0;
		
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
		
		test_1 = g_getReg(GS_TEST_1);
		g_setReg(GS_TEST_1, GS_SETREG_TEST_1(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));
		
		d = d_alloc(DMC_GIF, 4, TNULL, TNULL);
		
		d[0] 		 = GIF_SET_GIFTAG(1, 1, 1, 0x46, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)&cont->gsc_clear;
		d[2] 		 = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] 		 = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width +1,
							  disp->gsd_yoffset+disp->gsd_height+1, cont->gsc_dclear);
		
		g_setReg(GS_TEST_1, test_1);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d;
		TUINT64 test_2 = 0;
				
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;
		
		test_2 = g_getReg(GS_TEST_2);
		g_setReg(GS_TEST_2, GS_SETREG_TEST_2(0, 0, 0, 0, 0, 0, 1, GS_ZTEST_ALWAYS));
		
		d = d_alloc(DMC_GIF, 4, TNULL, TNULL);
		
		d[0] 		 = GIF_SET_GIFTAG(1, 1, 1, 0x246, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)&cont->gsc_clear;
		d[2] 		 = GIF_SET_XYZ(disp->gsd_xoffset, disp->gsd_yoffset, cont->gsc_dclear);
		d[3] 		 = GIF_SET_XYZ(disp->gsd_xoffset+disp->gsd_width +1,
							  disp->gsd_yoffset+disp->gsd_height+1, cont->gsc_dclear);

		g_setReg(GS_TEST_2, test_2);
	}
}

EXPORT TVOID
gs_setVisibleFb(TMOD_PS2 *TPS2Base, TINT ctx, TINT fbp)
{
	GScontext *cont;
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;

	if (ctx & GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
		
		/* set visible buffer in DISPFB_1 (modify base ptr) */
		GS_SET_DISPFB1(fbp, disp->gsd_width/64, cont->gsc_psm, 0, 0);
	}
	
	if (ctx & GS_CTX2)
	{
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;
		
		/* set visible buffer in DISPFB_2 (modify base ptr) */
		GS_SET_DISPFB2(fbp, disp->gsd_width/64, cont->gsc_psm, 0, 0);
	}
}

EXPORT TVOID
gs_setActiveFb(TMOD_PS2 *TPS2Base, TINT ctx, TINT fbp)
{
	GScontext *cont;
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
	if (ctx == GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;

		/* set active buffer in FRAME_1 (modify base ptr) */
		g_setReg(GS_FRAME_1, GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0));
	}
	else
	{
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;

		/* set active buffer in FRAME_2 (modify base ptr) */
		g_setReg(GS_FRAME_2, GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0));
	}
}

static TVOID
setvisiblebuf(struct cbdata *cbd)
{
	TMOD_PS2 *TPS2Base = cbd->ps2base;
	gs_setVisibleFb(TPS2Base, cbd->ctx, cbd->fbp);
}

EXPORT TVOID
gs_flipBuffer(TMOD_PS2 *TPS2Base, TINT ctx, TINT fb)
{
	int fbp = 0;
	GScontext *cont;
	GSdisplay *disp;
	static struct cbdata cbd;						/* FIXME */
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
	cbd.ctx = ctx;
	cbd.gsinfo = gsinfo;
	cbd.ps2base = TPS2Base;
	
	if (ctx & GS_CTX1)
	{
		cont = &gsinfo->gsi_ctx1;
		disp = &cont->gsc_disp;
		
		if (fb & 1)	fbp = cont->gsc_fbp1;
		else		fbp = cont->gsc_fbp0;
		
		/* set active buffer in FRAME_1 (modify base ptr) */
		if (ctx & GS_CTX2)
		{
			g_setReg(GS_FRAME_1, GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0));
		}
		else
		{
			g_setRegCb(GS_FRAME_1, GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0), 
						(DMACB) setvisiblebuf, (TAPTR) &cbd);
		}
	}
	
	if (ctx & GS_CTX2)
	{
		cont = &gsinfo->gsi_ctx2;
		disp = &cont->gsc_disp;

		if (fb & 1)	fbp = cont->gsc_fbp1;
		else		fbp = cont->gsc_fbp0;

		/* set active buffer in FRAME_2 (modify base ptr) */		
		g_setRegCb(GS_FRAME_2, GS_SETREG_FRAME(fbp, disp->gsd_width/64, cont->gsc_psm, 0), 
					(DMACB) setvisiblebuf, (TAPTR) &cbd);
	}
	
	cbd.fbp = fbp;
}

EXPORT TVOID
gs_setClearColor(TMOD_PS2 *TPS2Base, TINT ctx, gs_rgbaq_packed *rgba)
{
	GScontext *cont;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
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
gs_setDepthFunc(TMOD_PS2 *TPS2Base, TINT ctx, TINT dfunc)
{
	GScontext *cont;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
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
gs_setDepthClear(TMOD_PS2 *TPS2Base, TINT ctx, TINT clear)
{
	GScontext *cont;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
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

/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_gs-util.c,v $
**	Revision 1.1  2007/05/19 13:57:57  fschulze
**	restructered ps2_gs.c
**
**	
*/
