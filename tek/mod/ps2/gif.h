
#ifndef _TEK_MOD_PS2_GIF_H
#define _TEK_MOD_PS2_GIF_H

/*
**	$Id: gif.h,v 1.1 2005/09/18 12:33:39 tmueller Exp $
**	teklib/tek/mod/ps2/gif.h - GIF packed formats and macros
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/ps2/memory.h>

typedef struct
{
	TUINT32 NLOOP: 	15 PACKED;	/* repeat count						*/
	TUINT32 EOP:   	 1 PACKED;	/* end of packet					*/
	TUINT32 PAD:   	30 PACKED;
	TUINT32 PRE:   	 1 PACKED;	/* PRIM field enable on/off			*/
	TUINT32 PRI:	11 PACKED;	/* data for prim reg of GS			*/
	TUINT32 FLG:   	 2 PACKED;	/* data format packed/reglist/image	*/
	TUINT32 NREG: 	 4 PACKED;	/* number of reg descriptors		*/
	TUINT64 REGS: 	64 PACKED;	/* reg descriptors */
} gs_giftag ALIGN16;

/* GS datatypes		*/
typedef struct
{
	TUINT32 PRIM:	3 PACKED;	/* drawing primitive				*/
	TUINT32 IIP:	1 PACKED;	/* shading flat/gouraud				*/
	TUINT32 TME:	1 PACKED;	/* texture mapping  on/off			*/
	TUINT32 FGE:	1 PACKED;	/* fogging on/off					*/
	TUINT32 ABE:	1 PACKED;	/* alpha blending on/off			*/
	TUINT32 AA1:	1 PACKED;	/* 1 pass antialiasing on/off		*/
	TUINT32 FST:	1 PACKED;	/* texture coord UV/STQ				*/
	TUINT32 CTXT:	1 PACKED;	/* context 1/2						*/
	TUINT32 FIX:	1 PACKED;	/* fragment value ctl unfixed/fixed	*/
	TUINT32 pad0:	5 PACKED;
} gs_primitiv PACKED;

typedef struct
{
	TUINT64 R:	 	 8 	PACKED;
	TUINT64 pad0:	24 	PACKED;
	TUINT64 G:	 	 8	PACKED;
	TUINT64 pad1:	24 	PACKED;
	TUINT64 B:	 	 8 	PACKED;
	TUINT64 pad2:	24 	PACKED;
	TUINT64 A:	 	 8 	PACKED;
	TUINT64 pad3:	24 	PACKED;
} gs_rgbaq_packed PACKED;

typedef struct
{
	TUINT64 U:	 	14 	PACKED;
	TUINT64 pad0:	18 	PACKED;
	TUINT64 V:	 	14	PACKED;
	TUINT64 pad1:	18 	PACKED;
	TUINT64 pad2:	64 	PACKED;
} gs_uv_packed PACKED;

typedef struct
{
	TUINT64 X: 		16  PACKED;
	TUINT64 pad0: 	16  PACKED;
	TUINT64 Y: 		16  PACKED;
	TUINT64 pad1: 	16  PACKED;
	TUINT64 Z: 		32  PACKED;
	TUINT64 pad2: 	15  PACKED;
	TUINT64 ADC: 	 1  PACKED;
	TUINT64 pad3: 	16  PACKED;
} gs_xyz_packed PACKED;

typedef struct
{
	TUINT64 X: 		16  PACKED;
	TUINT64 pad0: 	16  PACKED;
	TUINT64 Y: 		16  PACKED;
	TUINT64 pad1: 	20  PACKED;
	TUINT64 Z: 		24  PACKED;
	TUINT64 pad2: 	 8  PACKED;
	TUINT64 F:		 8	PACKED;
	TUINT64 pad3:	 3	PACKED;
	TUINT64 ADC: 	 1  PACKED;
	TUINT64 pad4: 	16  PACKED;
} gs_xyzf_packed PACKED;

/*****************************************************************************/

#define GIF_SET_GIFTAG(nloop,eop,pre,pri,flag,nreg,regs) \
({ \
	TQWDATA __giftag; \
	((gs_giftag *) &__giftag)->NLOOP = nloop; \
	((gs_giftag *) &__giftag)->EOP = eop; \
	((gs_giftag *) &__giftag)->PRE = pre; \
	((gs_giftag *) &__giftag)->PRI = pri; \
	((gs_giftag *) &__giftag)->FLG = flag; \
	((gs_giftag *) &__giftag)->NREG = nreg; \
	((gs_giftag *) &__giftag)->REGS = regs; \
	__giftag; \
})

#define GIF_SET_PRIM(pri,iip,tme,fge,abe,aa1,fst,ctxt,fix) \
({ \
	TUINT16 __gsprim;	\
	((gs_primitiv *) &__gsprim)->PRIM = pri; \
	((gs_primitiv *) &__gsprim)->IIP = iip; \
	((gs_primitiv *) &__gsprim)->TME = tme; \
	((gs_primitiv *) &__gsprim)->FGE = fge; \
	((gs_primitiv *) &__gsprim)->ABE = abe; \
	((gs_primitiv *) &__gsprim)->AA1 = aa1; \
	((gs_primitiv *) &__gsprim)->FST = fst; \
	((gs_primitiv *) &__gsprim)->CTXT = ctxt; \
	((gs_primitiv *) &__gsprim)->FIX = fix; \
	__gsprim; \
})

#define GIF_SET_RGBA(r,g,b,a) \
({ \
	TQWDATA __rgba; \
	((gs_rgbaq_packed *) &__rgba)->R = r; \
	((gs_rgbaq_packed *) &__rgba)->G = g; \
	((gs_rgbaq_packed *) &__rgba)->B = b; \
	((gs_rgbaq_packed *) &__rgba)->A = a; \
	__rgba; \
})

#define GIF_SET_UV(u,v) \
({ \
	TQWDATA __uv; \
	((gs_uv_packed *) &__uv)->U = (u) << 4; \
	((gs_uv_packed *) &__uv)->V = (v) << 4; \
	__uv; \
})

#define GIF_SET_XYZ(x,y,z) \
({ \
	TQWDATA __xyz; \
	((gs_xyz_packed *) &__xyz)->X = (x) << 4; \
	((gs_xyz_packed *) &__xyz)->Y = (y) << 4; \
	((gs_xyz_packed *) &__xyz)->Z = (z); \
	__xyz; \
})

#define GIF_SET_XYZF(x,y,z,f) \
({ \
	TQWDATA __xyzf; \
	((gs_xyzf_packed *) &__xyzf)->X = (x) << 4; \
	((gs_xyzf_packed *) &__xyzf)->Y = (y) << 4; \
	((gs_xyzf_packed *) &__xyzf)->Z = (z); \
	((gs_xyzf_packed *) &__xyzf)->F = (f); \
	__xyzf; \
})

/*****************************************************************************/
/*
**	Revision History
**	$Log: gif.h,v $
**	Revision 1.1  2005/09/18 12:33:39  tmueller
**	added
**	
**
*/

#endif /* _TEK_MOD_PS2_GIF_H */
