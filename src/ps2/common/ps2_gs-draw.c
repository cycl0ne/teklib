/*
**	$Id: ps2_gs-draw.c,v 1.1 2007/05/19 13:57:57 fschulze Exp $
**	teklib/mods/ps2/common/ps2_gs.c - GS primitive drawing
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
  primitive drawing function
 ***********************************************************************************************/

EXPORT TVOID 
gs_drawTRect(TMOD_PS2 *TPS2Base, TINT ctx, TINT x, TINT y, TINT w, TINT h, GSimage *gsimage)
{
	TINT dx, dy;
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		dx = disp->gsd_xoffset;
		dy = disp->gsd_yoffset;
		
		g_initTexReg(GS_CTX1, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
		
		d = d_alloc(DMC_GIF, 5, TNULL, TNULL);
		
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
		
		g_initTexReg(GS_CTX2, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
		
		d = d_alloc(DMC_GIF, 5, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x356, 0, 4, 0x5353);
		d[1] = GIF_SET_UV(0,0);
		d[2] = GIF_SET_XYZ(dx+x,dy+y,1);
		d[3] = GIF_SET_UV(gsimage->w,gsimage->h);
		d[4] = GIF_SET_XYZ(dx+x+w, dy+y+h, 1);
	}
}

EXPORT TVOID 
gs_drawFRect(TMOD_PS2 *TPS2Base, TINT ctx, TINT x, TINT y, TINT w, TINT h, gs_rgbaq_packed *rgb)
{
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;

	if (ctx & GS_CTX1)
	{
		TQWDATA *d = d_alloc(DMC_GIF, 4, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x6, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x, disp->gsd_yoffset+y, 1);
		d[3]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y+h, 1);
	}

	if (ctx & GS_CTX2)
	{
		TQWDATA *d = d_alloc(DMC_GIF, 4, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x206, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x, disp->gsd_yoffset+y, 1);
		d[3]         = GIF_SET_XYZ(disp->gsd_xoffset+x+w, disp->gsd_yoffset+y+h, 1);
	}
}

EXPORT TVOID 
gs_drawRect(TMOD_PS2 *TPS2Base, TINT ctx, TINT x, TINT y, TINT w, TINT h, gs_rgbaq_packed *rgb)
{
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;

	if (ctx & GS_CTX1)
	{
		TQWDATA *d = d_alloc(DMC_GIF, 10, TNULL, TNULL);
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
		TQWDATA *d = d_alloc(DMC_GIF, 10, TNULL, TNULL);
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
gs_drawLine(TMOD_PS2 *TPS2Base, TINT ctx, TINT x1, TINT y1, TINT x2, TINT y2, gs_rgbaq_packed *rgb)
{
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d = d_alloc(DMC_GIF, 4, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x1, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x1, disp->gsd_yoffset+y1, 1);
		d[3]         = GIF_SET_XYZ(disp->gsd_xoffset+x2, disp->gsd_yoffset+y2, 1);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d = d_alloc(DMC_GIF, 4, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x201, 0, 3, 0x551);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x1, disp->gsd_yoffset+y1, 1);
		d[3]         = GIF_SET_XYZ(disp->gsd_xoffset+x2, disp->gsd_yoffset+y2, 1);
	}
}

EXPORT TVOID 
gs_drawPoint(TMOD_PS2 *TPS2Base, TINT ctx, TINT x, TINT y, gs_rgbaq_packed *rgb)
{
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d = d_alloc(DMC_GIF, 3, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x0, 0, 2, 0x51);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x, disp->gsd_yoffset+y, 1);
	}
	
	if (ctx & GS_CTX2)
	{
		TQWDATA *d = d_alloc(DMC_GIF, 3, TNULL, TNULL);
		disp = &gsinfo->gsi_ctx2.gsc_disp;
		
		d[0]         = GIF_SET_GIFTAG(1, 1, 1, 0x200, 0, 2, 0x51);
		d[1].ul128   = *(TUINT128 *)rgb;
		d[2]         = GIF_SET_XYZ(disp->gsd_xoffset+x, disp->gsd_yoffset+y, 1);
	}
}

EXPORT TVOID 
gs_drawTRectUV(TMOD_PS2 *TPS2Base, TINT ctx, TINT x0, TINT y0, TINT x1, TINT y1, TINT u0, TINT v0, TINT u1, TINT v1, GSimage *gsimage)
{
	TINT dx, dy;
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;

	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		dx = disp->gsd_xoffset;
		dy = disp->gsd_yoffset;
		
		g_initTexReg(GS_CTX1, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
	
		d = d_alloc(DMC_GIF, 5, TNULL, TNULL);
		
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
		
		g_initTexReg(GS_CTX2, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
	
		d = d_alloc(DMC_GIF, 5, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x356, 0, 4, 0x5353);
		d[1] = GIF_SET_UV(u0, v0);
		d[2] = GIF_SET_XYZ(dx + x0, dy + y0, 1);
		d[3] = GIF_SET_UV(u1, v1);
		d[4] = GIF_SET_XYZ(dx + x1, dy + y1, 1);
	}
}


EXPORT TVOID 
gs_drawTPointUV(TMOD_PS2 *TPS2Base, TINT ctx, TINT x, TINT y, TINT u, TINT v, GSimage *gsimage)
{
	TINT dx, dy;
	GSdisplay *disp;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
	if (ctx & GS_CTX1)
	{
		TQWDATA *d;
		disp = &gsinfo->gsi_ctx1.gsc_disp;
		dx = disp->gsd_xoffset;
		dy = disp->gsd_yoffset;
		
		g_initTexReg(GS_CTX1, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
	
		d = d_alloc(DMC_GIF, 3, TNULL, TNULL);
		
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
		
		g_initTexReg(GS_CTX2, GS_TEX_RGBA, GS_TEX_DECAL, gsimage);
	
		d = d_alloc(DMC_GIF, 3, TNULL, TNULL);
		
		d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x310, 0, 2, 0x53);
		d[1] = GIF_SET_UV(u, v);
		d[2] = GIF_SET_XYZ(dx + x, dy + y, 1);
	}
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_gs-draw.c,v $
**	Revision 1.1  2007/05/19 13:57:57  fschulze
**	restructered ps2_gs.c
**
**	
*/
