/*
**	$Id: rplasma.c,v 1.4 2007/05/19 14:14:29 fschulze Exp $
**	teklib/mods/ps2/tests/rplasma.c - effect from 'sohn des nichts' demo
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdlib.h>
#include <math.h>
#include <kernel.h>

#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/ps2.h>

#include <tek/mod/ps2/vif.h>

#include "imgload.h"

TAPTR TExecBase, TUtilBase, TIOBase, TDthBase, TImgpBase;
struct TPS2ModBase *TPS2Base;

/***********************************************************************
	default set of rplasma parameters
***********************************************************************/
#define RP_CTX		GS_CTX2
#define RP_WIDTH	G_GETWIDTH(RP_CTX)
#define RP_HEIGHT	G_GETHEIGHT(RP_CTX)
#define RP_XOFF		0
#define RP_YOFF		0
#define RP_SYM		7
#define RP_SC		16000
#define RP_ASP		"0.0"
#define RP_ABE		TFALSE

#define RP_SPEED	"0.05"
#define RP_RSPEED	"0.006"

#define RP_TEX		"ps2io:host/mods/ps2/tests/electric.tga"
/**********************************************************************/

TVOID MakeVuPacket(TQWDATA *chain);

#define FOGMIN		5
#define FDIST 		40

struct RPlasma
{
	TINT rp_Width;
	TINT rp_Height;
	TINT rp_Xoffset;
	TINT rp_Yoffset;
	TINT rp_Context;
	TINT rp_Symmetry;
	TINT rp_Scale;
	TFLOAT rp_Pos;
	TFLOAT rp_Rot;
	TFLOAT rp_Speed;
	TFLOAT rp_RotSpeed;
	TFLOAT rp_Asp;
	TQWDATA *rp_Chain;
	TBOOL rp_Blend;
};

struct RPlasma *rp;

#define ARG_TEMPLATE \
"-tex=TEXTURE/K,-sym=SYMMETRY/K/N,-sp=SPEED/K,-rsp=ROTSPEED/K,-sc=SCALE/K/N," \
"-pw=PWIDTH/K/N,-ph=PHEIGHT/K/N,-xoff=XOFFSET/K/N,-yoff=YOFFSET/K/N,-ctx=CONTEXT/K/N," \
"-asp=ASPECT/K,-abe=BLEND/S,-h=HELP/S"

enum { ARG_TEX, ARG_SYM, ARG_SPEED, ARG_RSPEED, ARG_SC, ARG_WIDTH, ARG_HEIGHT,
       ARG_XOFF, ARG_YOFF, ARG_CTX, ARG_ASP, ARG_ABE, ARG_HELP, ARG_NUM };

TTAG args[ARG_NUM];			/* arguments array */

/**********************************************************************
	VU symbols
***********************************************************************/
extern	TQWDATA		My_dma_start __attribute__((section(".vudata")));

/**********************************************************************/

TVOID MakeVuPacket(TQWDATA *chain)
{
	My_dma_start.ui32[1] = (TUINT32)chain;		/* set next pointer */

	FlushCache(0);

	DMA_WAIT((TUINT32 *)D1_CHCR);
	DMA_SET_QWC ((TUINT32 *)D1_QWC, 0);

	DMA_SET_MADR((TUINT32 *)D1_MADR, &My_dma_start, 0);
	DMA_SET_TADR((TUINT32 *)D1_TADR, &My_dma_start, 0);
	DMA_SET_CHCR((TUINT32 *)D1_CHCR, 1, 1, 0, 0, 0, 1, 0);
}

TINT calcuv(TQWDATA *uv, TINT x, TINT y, struct RPlasma *rp)
{
	TINT scrw = rp->rp_Width;
	TINT scrh = rp->rp_Height;
	TFLOAT f, at, r, xx, v;

	y -= scrh / 2;
	x -= scrw / 2;

	xx = (TFLOAT)x / rp->rp_Asp;

	if (xx == 0)
	{
		if (y > 0) 	at =  TPI / 2;
		else		at = -TPI / 2;
	}
	else
	{
		at = atan2f(y, xx);
	}

	r = (sqrtf(xx * xx + y * y));
	f = r*(1/(sqrtf(scrw/4 * scrw/4 + scrh/2 * scrh/2)/255))+FDIST;
	if (f > 255) f = 255;

	v = sinf(at * rp->rp_Symmetry);
	uv[0].fs32[0] = rp->rp_Scale / (r * (v * 0.5 + 2));
	uv[0].fs32[1] = v;

	return f;
}

TVOID genchain(struct RPlasma *rp)
{
	TFLOAT f;
	TINT qwc = 0;
	TINT y, x, i, j = 0, k = 0, l = 0;
	TINT xoff = G_GETXOFFS(rp->rp_Context) + rp->rp_Xoffset;
	TINT yoff = G_GETYOFFS(rp->rp_Context) + rp->rp_Yoffset;
	TQWDATA *chain = rp->rp_Chain;

	/* DMAcnt */
	chain[qwc  ].ui32[0] = 0x10000002;
	chain[qwc  ].ui32[1] = 0x00000000;
	chain[qwc++].ul64[1] = 0x0000000000000000UL;

	/* VIFcodes */
	chain[qwc  ].ui32[0] = NOP;
	chain[qwc  ].ui32[1] = NOP;
	chain[qwc  ].ui32[2] = MSKPATH3(1);
	chain[qwc++].ui32[3] = UNPACK(0, 0, 0, 1, V4_32); /* addr, usn, flg, num, vnvl */

	/* pos, rot, SYMMETRY, SCALE */
	chain[qwc  ].ui32[0] = 0;
	chain[qwc  ].ui32[1] = 0;
	chain[qwc  ].fs32[2] = (TFLOAT)1/2*rp->rp_Height/(TPI * 2);
	chain[qwc++].fs32[3] = (TFLOAT)rp->rp_Scale;

	for (y = 0; y < rp->rp_Height; y++)
	{
		/* DMAcnt */
		chain[qwc  ].ui32[0] = 0x10000000+((32*2+3)*(rp->rp_Width/32));
		chain[qwc  ].ui32[1] = 0;
		chain[qwc++].ul64[1] = 0x0000000000000000UL;

		for (x = 0; x < rp->rp_Width/32; x++)
		{
			/* VIFcodes */
			chain[qwc  ].ui32[0] = NOP;
			chain[qwc  ].ui32[1] = NOP;
			chain[qwc  ].ui32[2] = NOP;
			chain[qwc++].ui32[3] = UNPACK(0, 0, 1, (32*2+1), V4_32); /* addr, usn, flg, num, vnvl */

			/* GIFtag */
			chain[qwc  ].ui32[0] = 0x00008020;

			if (rp->rp_Context == GS_CTX1)
			{
				if (rp->rp_Blend)	chain[qwc  ].ui32[1] = 0x20b84000;
				else				chain[qwc  ].ui32[1] = 0x20984000;
			}
			else
			{
				if (rp->rp_Blend)	chain[qwc  ].ui32[1] = 0x21b84000;
				else				chain[qwc  ].ui32[1] = 0x21984000;
			}

			chain[qwc  ].ui32[2] = 0x00000043;
			chain[qwc++].ui32[3] = 0x00000000;

			for (i = 0; i < 32; i++, j++, k++)
			{
				f = calcuv(&chain[qwc], k, l, rp);
				chain[qwc  ].ui32[2] = 0;
				chain[qwc++].ui32[3] = 0;

				/* XYZF */
				chain[qwc++] = GIF_SET_XYZF(xoff+k, yoff+l, 1.0, (TINT)f);
			}

			/* VIFcodes */
			chain[qwc  ].ui32[0] = FLUSH;
			chain[qwc  ].ui32[1] = MSCNT;
			chain[qwc  ].ui32[2] = NOP;
			chain[qwc++].ui32[3] = NOP;
		}

		l++;
		j = 0;
		k = 0;
	}

	/* DMAend */
	chain[qwc  ].ui32[0] = 0x70000001;
	chain[qwc  ].ui32[1] = 0x0;
	chain[qwc++].ul64[1] = 0x0000000000000000UL;

	chain[qwc  ].ui32[0] = FLUSH;
	chain[qwc  ].ui32[1] = MARK(0x42);
	chain[qwc  ].ui32[2] = MSKPATH3(0);
	chain[qwc  ].ui32[3] = NOP;
}

TVOID set_rpspeed(TINT num, TFLOAT speed)
{
	rp[num].rp_Speed = speed;
}

TVOID set_rprotspeed(TINT num, TFLOAT rotspeed)
{
	rp[num].rp_RotSpeed = rotspeed;
}

TVOID init_rplasma_part(TINT rp_count)
{
	/* global init */

	rp = (struct RPlasma *) TAlloc0(TNULL, sizeof(struct RPlasma) * rp_count);

	g_setReg(GS_FOGCOL, GS_SETREG_FOGCOL(0x00, 0x55, 0x44));
	d_commit(DMC_GIF);
	d_commit(DMC_GIF);
}

TVOID free_rplasma_part(TINT rp_num)
{
	TINT i;

	for (i = 0; i < rp_num; i++)
		TFree(rp[i].rp_Chain);

	TFree(rp);
}

TVOID init_rplasma(TINT num, TTAG *args)
{
	TDOUBLE asp;

	rp[num].rp_Context = *(TUINT *)args[ARG_CTX];
	rp[num].rp_Width = *(TUINT *)args[ARG_WIDTH];
	rp[num].rp_Height = *(TUINT *)args[ARG_HEIGHT];
	rp[num].rp_Xoffset = *(TUINT *)args[ARG_XOFF];
	rp[num].rp_Yoffset = *(TUINT *)args[ARG_YOFF];
	rp[num].rp_Symmetry = *(TUINT *)args[ARG_SYM];
	rp[num].rp_Scale = *(TUINT *)args[ARG_SC];
	rp[num].rp_Blend = args[ARG_ABE];

	TStrToD((TSTRPTR)args[ARG_ASP], &asp);
	rp[num].rp_Asp = asp;

	if (!rp[num].rp_Asp)
		rp[num].rp_Asp = rp[num].rp_Width / rp[num].rp_Height;

	rp[num].rp_Chain = (TQWDATA *) TAlloc(TNULL, ((((32*2+3)*(rp[num].rp_Width/32))
										*rp[num].rp_Height)+rp[num].rp_Height+5)*16);
	genchain(&rp[num]);
}

TVOID free_rplasma(TINT num)
{
	TFree(rp[num].rp_Chain);
	rp[num].rp_Chain = TNULL;
}

TVOID do_rplasma(TINT num, TFLOAT delta, GSimage *tex)
{
	g_initTexReg(rp[num].rp_Context, GS_TEX_RGBA, GS_TEX_DECAL, tex);
	d_commit(DMC_GIF);
	d_commit(DMC_GIF);

	rp[num].rp_Chain[2].fs32[0] = rp[num].rp_Pos;
	rp[num].rp_Chain[2].fs32[1] = rp[num].rp_Rot;

	rp[num].rp_Pos += rp[num].rp_Speed + delta;
	rp[num].rp_Rot += rp[num].rp_RotSpeed*rp[num].rp_Symmetry;

	MakeVuPacket(rp[num].rp_Chain);
}

/*****************************************************************************/

GSimage loadTexture(TSTRPTR fname, TINT halpha)
{
	GSimage img;

	img = u_loadImage(fname, 1);
	g_initImage(&img, img.w, img.h, GS_PSMCT32, img.data);
	g_loadImage(&img);
	return img;
}

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TIOBase = TOpenModule("io", 0, TNULL);
	TPS2Base = TOpenModule("ps2common", 0, TNULL);
	TDthBase = TOpenModule("datatypehandler", 0, TNULL);
	TImgpBase = TOpenModule("imgproc", 0, TNULL);

	if (TExecBase && TUtilBase && TIOBase && TPS2Base && TDthBase && TImgpBase)
	{
		TSTRPTR *argv = TGetArgV();
		TAPTR arghandle;
		TINT sym = RP_SYM;
		TINT sc = RP_SC;
		TINT xoff = RP_XOFF;
		TINT yoff = RP_YOFF;
		TINT ctx = RP_CTX;
		TINT w = 0, h = 0;
		TDOUBLE speed, rspeed;

		args[ARG_TEX] = (TTAG) RP_TEX;
		args[ARG_SYM] = (TTAG) &sym;
		args[ARG_SPEED] = (TTAG) RP_SPEED;
		args[ARG_RSPEED] = (TTAG) RP_RSPEED;
		args[ARG_SC] = (TTAG) &sc;
		args[ARG_WIDTH] = (TTAG) &w;
		args[ARG_HEIGHT] = (TTAG) &h;
		args[ARG_XOFF] = (TTAG) &xoff;
		args[ARG_YOFF] = (TTAG) &yoff;
		args[ARG_CTX] = (TTAG) &ctx;
		args[ARG_ASP] = (TTAG) RP_ASP;
		args[ARG_ABE] = RP_ABE;
		args[ARG_HELP] = TFALSE;

		arghandle = TParseArgV(ARG_TEMPLATE, argv + 1, args);
		if (!arghandle || args[ARG_HELP])
		{
			printf("Usage: rplasma %s\n", ARG_TEMPLATE);
			printf("-tex=TEXTURE/K     Texture file [default: %s]\n", RP_TEX);
			printf("-sym=SYMMETRY/N/K  Plasma symmetry [default: %d]\n", RP_SYM);
			printf("-sp=SPEED/K        Plasma speed [default: %s]\n", RP_SPEED);
			printf("-rsp=RSPEED/K      Plasma rotation speed [default: %s]\n", RP_RSPEED);
			printf("-sc=SCALE/N/K      Plasma scale factor [default: %d]\n", RP_SC);
			printf("-pw=PWIDTH/N/K     Plasma width [default: %d]\n", RP_WIDTH);
			printf("                   (must be a multiple of 32)\n");
			printf("-ph=PHEIGHT/N/K    Plasma height [default: %d]\n", RP_HEIGHT);
			printf("-xoff=XOFFSET/N/K  Plasma position x [default: %d]\n", RP_XOFF);
			printf("-yoff=YOFFSET/N/K  Plasma position y [default: %d]\n", RP_YOFF);
			printf("-ctx=CONTEXT/N/K   Draw context [default: %d]\n", RP_CTX);
			printf("-asp=ASPECT/K      Aspect ratio [default: %s]\n", RP_ASP);
			printf("-abe=BLEND/S       Enable alpha blending [default: %s]\n", RP_ABE ? "on" : "off");
			printf("                   (use a texture with alpha channel)\n");
			printf("-h=HELP/S          This help\n");
		}
		else
		{
			TINT frame = 0;
			GSimage texture[1];

			/* init display */
			g_init(GS_VMODE_PAL, GS_INTERLACE, GS_FRAME);

			if (!*(TINT *)args[ARG_WIDTH])
				*(TINT *)args[ARG_WIDTH] = G_GETWIDTH(*(TINT *)args[ARG_CTX]);
			if (!*(TINT *)args[ARG_HEIGHT])
				*(TINT *)args[ARG_HEIGHT] = G_GETHEIGHT(*(TINT *)args[ARG_CTX]);

			/* init texture */
			texture[0] = loadTexture((TSTRPTR)args[ARG_TEX], 1);
			g_setReg(GS_TEX1_2, GS_SETREG_TEX1(0,0,1,1,0,0,0));
			g_setReg(GS_TEX1_1, GS_SETREG_TEX1(0,0,1,1,0,0,0));
			d_commit(DMC_GIF);
			d_commit(DMC_GIF);

			/* init plasma */
			init_rplasma_part(1);
			init_rplasma(0, args);

			TStrToD((TSTRPTR)args[ARG_SPEED], &speed);
			TStrToD((TSTRPTR)args[ARG_RSPEED], &rspeed);

			set_rpspeed(0, speed);
			set_rprotspeed(0, rspeed);

			while (1)
			{
				g_flipBuffer(GS_CTX2, frame & 1);
				d_commit(DMC_GIF);
				d_commit(DMC_GIF);

				do_rplasma(0, 0, &texture[0]);
				++frame;
				g_vsync();
			}

			free_rplasma_part(1);
			TFree(texture[0].data);
			g_freeImage(&texture[0]);
		}
	}

	TCloseModule(TImgpBase);
	TCloseModule(TDthBase);
	TCloseModule(TPS2Base);
	TCloseModule(TIOBase);
	TCloseModule(TUtilBase);
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: rplasma.c,v $
**	Revision 1.4  2007/05/19 14:14:29  fschulze
**	adapted to new GS startup
**
**	Revision 1.3  2005/11/14 21:32:39  tmueller
**	added modbase declarations which are no longer implicit
**
**	Revision 1.2  2005/10/07 13:59:18  fschulze
**	removed unnecessary includes
**
**	Revision 1.1  2005/10/05 22:11:26  fschulze
**	added
**
**
*/

