
#ifndef _TEK_MOD_PS2_VIF_H
#define _TEK_MOD_PS2_VIF_H

/*
**	$Id: vif.h,v 1.1 2005/09/18 12:33:39 tmueller Exp $
**	teklib/tek/mod/ps2/vif.h - VIF related macros and constants
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/ps2/memory.h>

#define S_32	0x0
#define S_16	0x1
#define S_8		0x2
#define V2_32	0x4
#define V2_16	0x5
#define V2_8	0x6
#define V3_32	0x8
#define V3_16	0x9
#define V3_8	0xa
#define V4_32	0xc
#define V4_16	0xd
#define V4_8	0xe
#define V4_5	0xf

#define	NOP					(TUINT)(0x00000000)
#define	STCYCL(cl, wl)		(TUINT)(0x01000000 | (((TUINT)wl  & 0xff) <<  8) | ((TUINT)cl   & 0xff))
#define	OFFSET(offs, num)	(TUINT)(0x02000000 | (((TUINT)num & 0xff) << 16) | ((TUINT)offs & 0x3ff))
#define BASE(base, num)		(TUINT)(0x03000000 | (((TUINT)num & 0xff) << 16) | ((TUINT)base & 0x3ff))
#define ITOP(addr, num)		(TUINT)(0x04000000 | (((TUINT)num & 0xff) << 16) | ((TUINT)addr & 0x3ff))
#define STMOD(mode)			(TUINT)(0x05000000 | ((TUINT)mode & 0x3)
#define MSKPATH3(mask)		(TUINT)(0x06000000 | ((TUINT)mask & 0x1) << 15)
#define	MARK(mark)			(TUINT)(0x07000000 | ((TUINT)mark & 0xffff))
#define	FLUSHE				(TUINT)(0x10000000)
#define	FLUSH				(TUINT)(0x11000000)
#define FLUSHA				(TUINT)(0x13000000)
#define	MSCAL (addr)		(TUINT)(0x14000000 | ((TUINT)addr  & 0xffff))
#define MSCALF(addr)		(TUINT)(0x16000000 | ((TUINT)addr  & 0xffff))
#define MSCNT				(TUINT)(0x17000000)
#define	STMASK				(TUINT)(0x20000000)
#define	STROW				(TUINT)(0x30000000)
#define STCOL				(TUINT)(0x31000000)
#define	MPG(addr, num)		(TUINT)(0x4a000000 | (((TUINT)num  & 0xff) << 16) | ((TUINT)addr & 0xffff))
#define DIRECT	(size)		(TUINT)(0x50000000 | ((TUINT)size  & 0xffff))
#define DIRECTHL(size)		(TUINT)(0x51000000 | ((TUINT)size  & 0xffff))
#define UNPACK(addr, usn, flg, num, vnvl)	(TUINT)(0x60000000 			|	\
											(((TUINT)vnvl & 0xf) << 24) |	\
											(((TUINT)num & 0xff) << 16) |	\
											(((TUINT)flg & 0x01) << 15) | 	\
											(((TUINT)usn & 0x01) << 14) |	\
											((TUINT)addr & 0x3ff))

#define	VU1_WAIT()	while(*(volatile TUINT32 *)VIF1_STAT & 0x7) ;

#define getvu0stats(t)	\
__asm__ __volatile__(	\
	"cfc2.i\t %0,$29\n"	\
	:"=r" (t):);

/*****************************************************************************/
/*
**	Revision History
**	$Log: vif.h,v $
**	Revision 1.1  2005/09/18 12:33:39  tmueller
**	added
**	
**
*/

#endif /* _TEK_MOD_PS2_VIF_H */
