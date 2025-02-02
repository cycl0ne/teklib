
#ifndef _TEK_MOD_PS2_GS_H
#define _TEK_MOD_PS2_GS_H

/*
**  $Id: gs.h,v 1.1 2005/09/18 12:33:39 tmueller Exp $
**	teklib/tek/mod/ps/gs.h - GS defines and macros
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/ps2/memory.h>
#include <tek/mod/ps2/gif.h>

/***********************************************************************************************
	defines and structures 
 ***********************************************************************************************/

#define CTX1_W				320
#define CTX1_H				256
#define CTX1_PSM			GS_PSMCT32

#define GS_CTX1				0x1
#define GS_CTX2				0x2

/* GS output mode	*/
#define	GS_INTERLACE		1
#define	GS_NO_INTERLACE		0

#define GS_FRAME			1
#define GS_FIELD			2

/* GS pixel storage formats */
#define	GS_PSMCT32			0x00
#define	GS_PSMCT24			0x01
#define GS_PSMCT16			0x02
#define GS_PSMCT16S			0x0a
#define	GS_PSGPU24			0x12
#define	GS_PSMT8			0x13
#define	GS_PSMT4			0x14
#define	GS_PSMT8H			0x1b
#define	GS_PSMT4HL			0x24
#define	GS_PSMT4HH			0x2c
#define	GS_PSMZ32			0x30
#define	GS_PSMZ24			0x31
#define	GS_PSMZ16			0x32
#define	GS_PSMZ16S			0x3a

/* GS Depth Test Methods	*/
#define	GS_ZTEST_NEVER		0x0
#define	GS_ZTEST_ALWAYS		0x1
#define	GS_ZTEST_GEQUAL		0x2
#define	GS_ZTEST_GREATER	0x3

/* GS Texture Color Component */
#define GS_TEX_RGB			0x0
#define GS_TEX_RGBA			0x1

/* GS Texture Function */
#define GS_TEX_MODULATE		0x0
#define GS_TEX_DECAL		0x1
#define GS_TEX_HIGHLIGHT	0x2
#define GS_TEX_HIGHLIGHT2	0x3

#define LMA_NUMPAGES		(0x400000/8192)
#define GS_LMZBUF			0x1
#define GS_LMFBUF			0x2

typedef enum 
{
	PAL_256_256_32=0,
	PAL_320_256_32,
	PAL_384_256_32,
	PAL_512_256_32,
	PAL_640_256_32
} GSvmode;

struct LMAlloc
{
	TUINT8 lma_AllocPages[LMA_NUMPAGES];
};

/* texture allocator */

#define TXA_PSMCT32		0x10ff	/* X X X X  */
#define TXA_PSMCT24		0x203f	/*   X X X  */
#define TXA_PSMCT16		0x31ff	/* X X X X  */
#define TXA_PSMT8		0x40ff	/* X X X X  */
#define TXA_PSMT8H		0x50c0	/* X        */
#define TXA_PSMT4		0x61ff	/* xxxxxxxx */
#define TXA_PSMT4HH		0x7080	/* x        */
#define TXA_PSMT4HL		0x8040	/*  x       */

#define TXA_CONFMASK	0x0100	/* block configuration mask */

struct TXAlloc
{
	TINT txa_PWidth;			/* Texture buffer width [number of pages] */
	TINT txa_PHeight;			/* Texture buffer height [number of pages] */
	TUINT8 *txa_PAlloc;			/* Reserve pw * ph * 32 bytes */
};

typedef struct
{
	GSvmode gss_vmode;
	TINT gss_inter;					/* interlace/ nointerlace 	*/
	TINT gss_ffmd;					/* field/frame				*/	
	TINT gss_ctx;					/* contexts used			*/
	struct LMAlloc gss_lmalloc;		/* local memory allocator	*/
} GSscreen;

typedef struct
{
	TINT gsd_dx;
	TINT gsd_dy;
	TINT gsd_magh;
	TINT gsd_magv;
	TINT gsd_width;
	TINT gsd_height;
	TINT gsd_depth;
	TINT gsd_xcenter;
	TINT gsd_ycenter;
	TINT gsd_xoffset;
	TINT gsd_yoffset;
} GSdisplay;

typedef struct
{
	gs_rgbaq_packed gsc_clear;		// 0x00
	GSdisplay gsc_disp;				// 0x10
	TINT gsc_psm;					// 0x3c
	TINT gsc_zpsm;					// 0x40
	TINT gsc_zbits;					// 0x44
	TINT gsc_fbp0;					// 0x48
	TINT gsc_fbp1;					// 0x4c
	TINT gsc_zbp;					// 0x50
	TINT gsc_dfunc;					// 0x54
	TINT gsc_dclear;				// 0x58
	TINT gsc_pad;					// 0x5c
									// 0x60
} GScontext;

typedef struct
{
	TINT gst_tbp;
	struct TXAlloc gst_txalloc;
} GStexenv;

typedef struct
{
	GScontext gsi_ctx1;
	GScontext gsi_ctx2;
	GSscreen gsi_screen;
	GStexenv gsi_texenv;
} GSinfo;


typedef struct
{
	TINT w;
	TINT h;
	TINT psm;
	TINT tbp;
	TINT tbw;
	TINT pad[3];
	TAPTR data;
} GSimage;

typedef struct
{
	TUINT16 ntsc_pal;
	TUINT16 width;
	TUINT16 height;
	TUINT16 psm;
	TUINT16 bpp;
	TUINT16 magh;
} vmode_t ALIGN16;

/***********************************************************************************************
	macros 
 ***********************************************************************************************/

#define G_GETWIDTH(ctx)													\
({																		\
	TINT __w;															\
	if (ctx == GS_CTX1)	__w = TPS2Base->ps2_GSInfo->gsi_ctx1.gsc_disp.gsd_width;		\
	else				__w = TPS2Base->ps2_GSInfo->gsi_ctx2.gsc_disp.gsd_width;		\
	__w;																\
})

#define G_GETHEIGHT(ctx)												\
({																		\
	TINT __h;															\
	if (ctx == GS_CTX1)	__h = TPS2Base->ps2_GSInfo->gsi_ctx1.gsc_disp.gsd_height;		\
	else				__h = TPS2Base->ps2_GSInfo->gsi_ctx2.gsc_disp.gsd_height;		\
	__h;																\
})

#define G_GETDEPTH(ctx)													\
({																		\
	TINT __d;															\
	if (ctx == GS_CTX1)	__d = TPS2Base->ps2_GSInfo->gsi_ctx1.gsc_disp.gsd_depth;		\
	else				__d = TPS2Base->ps2_GSInfo->gsi_ctx2.gsc_disp.gsd_depth;		\
	__d;																\
})

#define G_GETXC(ctx)													\
({																		\
	TINT __xc;															\
	if (ctx == GS_CTX1)	__xc = TPS2Base->ps2_GSInfo->gsi_ctx1.gsc_disp.gsd_xcenter;	\
	else				__xc = TPS2Base->ps2_GSInfo->gsi_ctx2.gsc_disp.gsd_xcenter;	\
	__xc;																\
})

#define G_GETYC(ctx)													\
({																		\
	TINT __yc;															\
	if (ctx == GS_CTX1)	__yc = TPS2Base->ps2_GSInfo->gsi_ctx1.gsc_disp.gsd_ycenter;	\
	else				__yc = TPS2Base->ps2_GSInfo->gsi_ctx2.gsc_disp.gsd_ycenter;	\
	__yc;																\
})

#define G_GETXOFFS(ctx)													\
({																		\
	TINT __xo;															\
	if (ctx == GS_CTX1)	__xo = TPS2Base->ps2_GSInfo->gsi_ctx1.gsc_disp.gsd_xoffset;	\
	else				__xo = TPS2Base->ps2_GSInfo->gsi_ctx2.gsc_disp.gsd_xoffset;	\
	__xo;																\
})

#define G_GETYOFFS(ctx)													\
({																		\
	TINT __yo;															\
	if (ctx == GS_CTX1)	__yo = TPS2Base->ps2_GSInfo->gsi_ctx1.gsc_disp.gsd_yoffset;	\
	else				__yo = TPS2Base->ps2_GSInfo->gsi_ctx2.gsc_disp.gsd_yoffset;	\
	__yo;																\
})

#define G_GETZBITS(ctx)													\
({																		\
	TINT __zb;															\
	if (ctx == GS_CTX1)	__zb = TPS2Base->ps2_GSInfo->gsi_ctx1.gsc_zbits;				\
	else				__zb = TPS2Base->ps2_GSInfo->gsi_ctx2.gsc_zbits;				\
	__zb;																\
})

#define G_GETFMT(ctx)														\
({																			\
	TINT __fmt;																\
	if (ctx == GS_CTX1)	__fmt = TPS2Base->ps2_GSInfo->gsi_ctx1.gsc_psm;		\
	else				__fmt = TPS2Base->ps2_GSInfo->gsi_ctx2.gsc_psm;		\
	__fmt;																	\
})

#define G_GETCOL(r,g,b,a)		\
({								\
	gs_rgbaq_packed __rgba[1];	\
	__rgba[0].R = r;			\
	__rgba[0].G = g;			\
	__rgba[0].B = b;			\
	__rgba[0].A = a;			\
	__rgba;						\
})

#define	G_GETINTER()	TPS2Base->ps2_GSInfo->gsi_screen.gss_inter

#define GS_SET_CSR(signal,finish,hsint,vsint,edwint,flush,reset,nfield,field,fifo,rev,id) \
	(*(volatile TUINT64 *)(GS_CSR) = 	\
	((TUINT64)(signal)	<< 0)	| 		\
	((TUINT64)(finish)	<< 1)	| 		\
	((TUINT64)(hsint)	<< 2)	| 		\
	((TUINT64)(vsint)	<< 3)	| 		\
	((TUINT64)(edwint)	<< 4)	| 		\
	((TUINT64)(flush)	<< 8)	| 		\
	((TUINT64)(reset)	<< 9)	| 		\
	((TUINT64)(nfield)	<< 12)	| 		\
	((TUINT64)(field)	<< 13)	| 		\
	((TUINT64)(fifo)	<< 14)	| 		\
	((TUINT64)(rev)		<< 16)	| 		\
	((TUINT64)(id)		<< 24))

#define GS_RESET() \
	(*(volatile TUINT64 *)(GS_CSR) = ((TUINT64)(1) << 9))

#define GS_SET_PMODE(en1,en2,mmod,amod,slbg,alp) \
	(*(volatile TUINT64 *)(GS_PMODE) = 	\
	((TUINT64)(en1)	<< 0) 	| 			\
	((TUINT64)(en2)	<< 1) 	| 			\
	((TUINT64)(001)	<< 2) 	| 			\
	((TUINT64)(mmod)<< 5) 	| 			\
	((TUINT64)(amod)<< 6) 	| 			\
	((TUINT64)(slbg)<< 7) 	| 			\
	((TUINT64)(alp) << 8))

#define GS_SET_SMODE2(int,ffmd,dpms) 	\
	(*(volatile TUINT64 *)(GS_SMODE2) = \
	((TUINT64)(int)	<< 0)	| 			\
	((TUINT64)(ffmd)<< 1)	|			\
	((TUINT64)(dpms)<< 2))

#define GS_SET_DISPFB1(fbp,fbw,psm,dbx,dby) \
	(*(volatile TUINT64 *)(GS_DISPFB1) = \
	((TUINT64)(fbp)	<< 0)	| 			\
	((TUINT64)(fbw)	<< 9)	| 			\
	((TUINT64)(psm)	<< 15)	| 			\
	((TUINT64)(dbx)	<< 32)	| 			\
	((TUINT64)(dby)	<< 43))

#define GS_SET_DISPLAY1(dx,dy,magh,magv,dw,dh) \
	(*(volatile TUINT64 *)(GS_DISPLAY1)= \
	((TUINT64)(dx)	<< 0)	| 			\
	((TUINT64)(dy)	<< 12)	| 			\
	((TUINT64)(magh)<< 23)	| 			\
	((TUINT64)(magv)<< 27)	| 			\
	((TUINT64)(dw)	<< 32)	| 			\
	((TUINT64)(dh)	<< 44))

#define GS_SET_DISPFB2(fbp,fbw,psm,dbx,dby) \
	(*(volatile TUINT64 *)(GS_DISPFB2) = \
	((TUINT64)(fbp)	<< 0)	|	 		\
	((TUINT64)(fbw)	<< 9)	| 			\
	((TUINT64)(psm)	<< 15)	| 			\
	((TUINT64)(dbx)	<< 32)	| 			\
	((TUINT64)(dby)	<< 43))

#define GS_SET_DISPLAY2(dx,dy,magh,magv,dw,dh) \
	(*(volatile TUINT64 *)(GS_DISPLAY2)= \
	((TUINT64)(dx)	<< 0)	| 			\
	((TUINT64)(dy)	<< 12)	| 			\
	((TUINT64)(magh)<< 23)	| 			\
	((TUINT64)(magv)<< 27)	| 			\
	((TUINT64)(dw)	<< 32)	| 			\
	((TUINT64)(dh)	<< 44))

#define GS_SET_BGCOLOR(r,g,b) 			\
	(*(volatile TUINT64 *)(GS_BGCOLOR)= \
	((TUINT64)(r)	<< 0)	| 			\
	((TUINT64)(g)	<< 8)	| 			\
	((TUINT64)(b)	<< 16))

/* privileged registers	numbers	*/
#define	GS_PRIV_PMODE		0x00
#define	GS_PRIV_SMODE1		0x01
#define	GS_PRIV_SMODE2		0x02
#define	GS_PRIV_SRFSH		0x03
#define	GS_PRIV_SYNCH1		0x04
#define	GS_PRIV_SYNCH2		0x05
#define	GS_PRIV_SYNCV		0x06
#define	GS_PRIV_DISPFB1		0x07
#define	GS_PRIV_DISPLAY1	0x08
#define	GS_PRIV_DISPFB2		0x09
#define	GS_PRIV_DISPLAY2	0x0a
#define	GS_PRIV_EXTBUF		0x0b
#define	GS_PRIV_EXTDATA		0x0c
#define	GS_PRIV_EXTWRITE	0x0d
#define	GS_PRIV_BGCOLOR		0x0e
#define	GS_PRIV_CSR			0x40
#define	GS_PRIV_IMR			0x41
#define	GS_PRIV_BUSDIR		0x44
#define	GS_PRIV_SIGLBLID	0x48
#define	GS_PRIV_SYSCNT		0x4f

/* general purpose registers numbers */

#define GS_PRIM				0x00
#define GS_RGBAQ			0x01
#define GS_ST				0x02
#define GS_UV				0x03
#define GS_XYZF2			0x04
#define GS_XYZ2				0x05
#define GS_TEX0_1			0x06
#define GS_TEX0_2			0x07
#define GS_CLAMP_1			0x08
#define GS_CLAMP_2			0x09
#define GS_FOG				0x0a
#define GS_XYZF3			0x0c
#define GS_XYZ3				0x0d
#define GS_TEX1_1			0x14
#define GS_TEX1_2			0x15
#define GS_TEX2_1			0x16
#define GS_TEX2_2			0x17
#define GS_XYOFFSET_1		0x18
#define GS_XYOFFSET_2		0x19
#define GS_PRMODECONT		0x1a
#define GS_PRMODE			0x1b
#define GS_TEXCLUT			0x1c
#define GS_SCANMSK			0x22
#define GS_MIPTBP1_1		0x34
#define GS_MIPTBP1_2		0x35
#define GS_MIPTBP2_1		0x36
#define GS_MIPTBP2_2		0x37
#define GS_TEXA				0x3b
#define GS_FOGCOL			0x3d
#define GS_TEXFLUSH			0x3f
#define GS_SCISSOR_1		0x40
#define GS_SCISSOR_2		0x41
#define GS_ALPHA_1			0x42
#define GS_ALPHA_2			0x43
#define GS_DIMX				0x44
#define GS_DTHE				0x45
#define GS_COLCLAMP			0x46
#define GS_TEST_1			0x47
#define GS_TEST_2			0x48
#define GS_PABE				0x49
#define GS_FBA_1			0x4a
#define GS_FBA_2			0x4b
#define GS_FRAME_1			0x4c
#define GS_FRAME_2			0x4d
#define GS_ZBUF_1			0x4e
#define GS_ZBUF_2			0x4f
#define GS_BITBLTBUF		0x50
#define GS_TRXPOS			0x51
#define GS_TRXREG			0x52
#define GS_TRXDIR			0x53
#define GS_HWREG			0x54
#define GS_SIGNAL			0x60
#define GS_FINISH			0x61
#define GS_LABEL			0x62

/* GS setreg macros 	*/

#define GS_SETREG_ALPHA_1	GS_SET_ALPHA
#define GS_SETREG_ALPHA_2	GS_SET_ALPHA
#define GS_SETREG_ALPHA(a, b, c, d, fix)\
	((TUINT64)(a)      		| 			\
	((TUINT64)(b) 	<< 2) 	|			\
	((TUINT64)(c)	<< 4) 	| 			\
	((TUINT64)(d) 	<< 6) 	| 			\
	((TUINT64)(fix) << 32))

#define GS_SETREG_BITBLTBUF(sbp, sbw, spsm, dbp, dbw, dpsm) \
	((TUINT64)(sbp)         | 			\
	((TUINT64)(sbw)  << 16) | 			\
	((TUINT64)(spsm) << 24) | 			\
	((TUINT64)(dbp)  << 32) | 			\
	((TUINT64)(dbw)  << 48) | 			\
	((TUINT64)(dpsm) << 56))

#define GS_SETREG_CLAMP_1	GS_SET_CLAMP
#define GS_SETREG_CLAMP_2	GS_SET_CLAMP
#define GS_SETREG_CLAMP(wms, wmt, minu, maxu, minv, maxv) \
	((TUINT64)(wms)         |			\
	((TUINT64)(wmt)  <<  2)	| 			\
	((TUINT64)(minu) <<  4) | 			\
	((TUINT64)(maxu) << 14) | 			\
	((TUINT64)(minv) << 24) | 			\
	((TUINT64)(maxv) << 34))

#define GS_SETREG_COLCLAMP(clamp) ((TUINT64)(clamp))

#define GS_SETREG_DIMX(dm00, dm01, dm02, dm03, dm10, dm11, dm12, dm13, 	\
			dm20, dm21, dm22, dm23, dm30, dm31, dm32, dm33) 			\
	((TUINT64)(dm00)        |			\
	((TUINT64)(dm01) << 4) 	| 			\
	((TUINT64)(dm02) << 8)  |			\
	((TUINT64)(dm03) << 12) | 			\
	((TUINT64)(dm10) << 16) |			\
	((TUINT64)(dm11) << 20)	| 			\
	((TUINT64)(dm12) << 24) |			\
	((TUINT64)(dm13) << 28) | 			\
	((TUINT64)(dm20) << 32) |			\
	((TUINT64)(dm21) << 36) | 			\
	((TUINT64)(dm22) << 40)	|			\
	((TUINT64)(dm23) << 44) | 			\
	((TUINT64)(dm30) << 48) |			\
	((TUINT64)(dm31) << 52) | 			\
	((TUINT64)(dm32) << 56) |			\
	((TUINT64)(dm33) << 60))

#define GS_SETREG_DTHE(dthe) ((TUINT64)(dthe))

#define GS_SETREG_FBA_1	GS_SETREG_FBA
#define GS_SETREG_FBA_2	GS_SETREG_FBA
#define GS_SETREG_FBA(fba) ((TUINT64)(fba))

#define GS_SETREG_FOG(f) ((TUINT64)(f) << 56)

#define GS_SETREG_FOGCOL(fcr, fcg, fcb) \
	((TUINT64)(fcr) 		|			\
	((TUINT64)(fcg) << 8) 	|			\
	((TUINT64)(fcb) << 16))

#define GS_SETREG_FRAME_1	GS_SETREG_FRAME
#define GS_SETREG_FRAME_2	GS_SETREG_FRAME
#define GS_SETREG_FRAME(fbp, fbw, psm, fbmask) \
	((TUINT64)(fbp)        	|			\
	((TUINT64)(fbw) << 16) 	| 			\
	((TUINT64)(psm) << 24) 	|			\
	((TUINT64)(fbmask) << 32))

#define GS_SETREG_LABEL(id, idmsk) 		\
	((TUINT64)(id) 			|			\
	((TUINT64)(idmsk) << 32))

#define GS_SETREG_MIPTBP1_1	GS_SETREG_MIPTBP1
#define GS_SETREG_MIPTBP1_2	GS_SETREG_MIPTBP1
#define GS_SETREG_MIPTBP1(tbp1, tbw1, tbp2, tbw2, tbp3, tbw3) \
	((TUINT64)(tbp1)        |			\
	((TUINT64)(tbw1) << 14) | 			\
	((TUINT64)(tbp2) << 20) |			\
	((TUINT64)(tbw2) << 34) | 			\
	((TUINT64)(tbp3) << 40) |			\
	((TUINT64)(tbw3) << 54))

#define GS_SETREG_MIPTBP2_1	GS_SETREG_MIPTBP2
#define GS_SETREG_MIPTBP2_2	GS_SETREG_MIPTBP2
#define GS_SETREG_MIPTBP2(tbp4, tbw4, tbp5, tbw5, tbp6, tbw6) \
	((TUINT64)(tbp4)        | 			\
	((TUINT64)(tbw4) << 14) | 			\
	((TUINT64)(tbp5) << 20) | 			\
	((TUINT64)(tbw5) << 34) | 			\
	((TUINT64)(tbp6) << 40) |			\
	((TUINT64)(tbw6) << 54))

#define GS_SETREG_PABE(pabe) ((TUINT64)(pabe))

#define GS_SETREG_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix) \
	((TUINT64)(prim)      	|			\
	((TUINT64)(iip) << 3) 	|			\
	((TUINT64)(tme) << 4) 	|			\
	((TUINT64)(fge) << 5) 	|			\
	((TUINT64)(abe) << 6)  	|			\
	((TUINT64)(aa1) << 7) 	| 			\
	((TUINT64)(fst) << 8) 	|			\
	((TUINT64)(ctxt) << 9) 	|			\
	((TUINT64)(fix) << 10))

#define GS_SETREG_PRMODE(iip, tme, fge, abe, aa1, fst, ctxt, fix) \
	(((TUINT64)(iip) << 3) 	|			\
	((TUINT64)(tme) << 4)  	| 			\
	((TUINT64)(fge) << 5) 	|			\
	((TUINT64)(abe) << 6) 	|			\
	((TUINT64)(aa1) << 7) 	| 			\
	((TUINT64)(fst) << 8) 	|			\
	((TUINT64)(ctxt) << 9) 	|			\
	((TUINT64)(fix) << 10))

#define GS_SETREG_PRMODECONT(ac) ((TUINT64)(ac))

#define GS_SETREG_RGBAQ(r, g, b, a, q) 	\
	((TUINT64)(r)        	|			\
	((TUINT64)(g) << 8) 	|			\
	((TUINT64)(b) << 16)	| 			\
	((TUINT64)(a) << 24) 	|			\
	((TUINT64)(q) << 32))

#define GS_SETREG_SCANMSK(msk) ((TUINT64)(msk))

#define GS_SETREG_SCISSOR_1	GS_SETREG_SCISSOR
#define GS_SETREG_SCISSOR_2	GS_SETREG_SCISSOR
#define GS_SETREG_SCISSOR(scax0, scax1, scay0, scay1) \
	((TUINT64)(scax0)        |			\
	((TUINT64)(scax1) << 16) | 			\
	((TUINT64)(scay0) << 32) |			\
	((TUINT64)(scay1) << 48))

#define GS_SETREG_SIGNAL(id, idmsk) ((TUINT64)(id) | ((TUINT64)(idmsk) << 32))
#define GS_SETREG_ST(s, t) 			((TUINT64)(s) |((TUINT64)(t) << 32))

#define GS_SETREG_TEST_1 GS_SETREG_TEST
#define GS_SETREG_TEST_2 GS_SETREG_TEST
#define GS_SETREG_TEST(ate, atst, aref, afail, date, datm, zte, ztst) \
	((TUINT64)(ate)         |			\
	((TUINT64)(atst) << 1) 	| 			\
	((TUINT64)(aref) << 4)  |			\
	((TUINT64)(afail) << 12)| 			\
	((TUINT64)(date) << 14)	|			\
	((TUINT64)(datm) << 15)	| 			\
	((TUINT64)(zte) << 16) 	|			\
	((TUINT64)(ztst) << 17))

#define GS_SETREG_TEX0_1	GS_SETREG_TEX0
#define GS_SETREG_TEX0_2	GS_SETREG_TEX0
#define GS_SETREG_TEX0(tbp, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld) \
	((TUINT64)(tbp)        	|			\
	((TUINT64)(tbw) << 14) 	|			\
	((TUINT64)(psm) << 20) 	|			\
	((TUINT64)(tw) << 26)	| 			\
	((TUINT64)(th) << 30)	|			\
	((TUINT64)(tcc) << 34)	| 			\
	((TUINT64)(tfx) << 35)	|			\
	((TUINT64)(cbp) << 37)	| 			\
	((TUINT64)(cpsm) << 51)	|			\
	((TUINT64)(csm) << 55) 	| 			\
	((TUINT64)(csa) << 56)	|			\
	((TUINT64)(cld) << 61))

#define GS_SETREG_TEX1_1	GS_SETREG_TEX1
#define GS_SETREG_TEX1_2	GS_SETREG_TEX1
#define GS_SETREG_TEX1(lcm, mxl, mmag, mmin, mtba, l, k) \
	((TUINT64)(lcm)        	|			\
	((TUINT64)(mxl) << 2)  	| 			\
	((TUINT64)(mmag) << 5) 	|			\
	((TUINT64)(mmin) << 6) 	| 			\
	((TUINT64)(mtba) << 9) 	|			\
	((TUINT64)(l) << 19) 	| 			\
	((TUINT64)(k) << 32))

#define GS_SETREG_TEX2_1	GS_SETREG_TEX2
#define GS_SETREG_TEX2_2	GS_SETREG_TEX2
#define GS_SETREG_TEX2(psm, cbp, cpsm, csm, csa, cld) \
	(((TUINT64)(psm) << 20)	|			\
	((TUINT64)(cbp) << 37) 	| 			\
	((TUINT64)(cpsm) << 51)	|			\
	((TUINT64)(csm) << 55) 	|			\
	((TUINT64)(csa) << 56) 	|			\
	((TUINT64)(cld) << 61))

#define GS_SETREG_TEXA(ta0, aem, ta1) 	\
	((TUINT64)(ta0) 		|			\
	((TUINT64)(aem) << 15) 	|			\
	((TUINT64)(ta1) << 32))

#define GS_SETREG_TEXCLUT(cbw, cou, cov)\
	((TUINT64)(cbw) 		|			\
	((TUINT64)(cou) << 6) 	|			\
	((TUINT64)(cov) << 12))

#define GS_SETREG_TRXDIR(xdr) ((TUINT64)(xdr))

#define GS_SETREG_TRXPOS(ssax, ssay, dsax, dsay, dir) \
	((TUINT64)(ssax)        |			\
	((TUINT64)(ssay) << 16) | 			\
	((TUINT64)(dsax) << 32) |			\
	((TUINT64)(dsay) << 48) | 			\
	((TUINT64)(dir) << 59))

#define GS_SETREG_TRXREG(rrw, rrh) 	((TUINT64)(rrw) | ((TUINT64)(rrh) << 32))
#define GS_SETREG_UV(u, v) 			((TUINT64)(u) 	| ((TUINT64)(v) << 16))

#define GS_SETREG_XYOFFSET_1	GS_SETREG_XYOFFSET
#define GS_SETREG_XYOFFSET_2	GS_SETREG_XYOFFSET
#define GS_SETREG_XYOFFSET(ofx, ofy)((TUINT64)(ofx) | ((TUINT64)(ofy) << 32))

#define GS_SETREG_XYZ3 GS_SETREG_XYZ
#define GS_SETREG_XYZ2 GS_SETREG_XYZ
#define GS_SETREG_XYZ(x, y, z) 			\
	((TUINT64)(x) 			|			\
	((TUINT64)(y) << 16) 	| 			\
	((TUINT64)(z) << 32))

#define GS_SETREG_XYZF3 GS_SETREG_XYZF
#define GS_SETREG_XYZF2 GS_SETREG_XYZF
#define GS_SETREG_XYZF(x, y, z, f) 		\
	((TUINT64)(x) 			| 			\
	((TUINT64)(y) << 16) 	|			\
	((TUINT64)(z) << 32)	| 			\
	((TUINT64)(f) << 56))

#define GS_SETREG_ZBUF_1	GS_SETREG_ZBUF
#define GS_SETREG_ZBUF_2	GS_SETREG_ZBUF
#define GS_SETREG_ZBUF(zbp, psm, zmsk) 	\
	((TUINT64)(zbp) 		|			\
	((TUINT64)(psm) << 24) 	| 			\
	((TUINT64)(zmsk) << 32))

#define SetIMR()						\
__asm__ __volatile__(					\
	".set push\n"						\
	".text\n"							\
	".set 	noreorder\n"				\
	"li\t	$4,0x0000ff00\n"			\
	"ld\t	$2,0x12001000\n"			\
	"dsrl\t	$2,16\n"					\
	"andi\t	$2,0xff\n"					\
	"li\t	$3,0x71\n"					\
	"nop\n"								\
	"syscall\n"							\
	"nop\n"								\
	".set  	pop\n"						\
	::)

/*****************************************************************************/
/*
**	Revision History
**	$Log: gs.h,v $
**	Revision 1.1  2005/09/18 12:33:39  tmueller
**	added
**	
**	
*/

#endif /* _TEK_MOD_PS2_GS_H */

