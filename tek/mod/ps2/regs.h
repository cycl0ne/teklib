
#ifndef _PS2_HW_REGS_H
#define _PS2_HW_REGS_H

/*
**	$Id: regs.h,v 1.2 2005/09/18 11:27:22 tmueller Exp $
**	teklib/mods/hal/ps2/regs.h - GP/COP0/COP1 registers with symbolic names
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

/* general purpose registers (32 bit) */

#define	zero		$0	/* reg0 is hardwired to zero */
#define	at			$1	/* reserved for assembly (temporary pseudo instr.) */
#define v0			$2	/* statements and results of functions */
#define v1			$3
#define a0			$4	/* function or subroutine arguments 1-4	*/
#define a1			$5
#define a2			$6
#define a3			$7
#define t0			$8	/* temporary registers, will not be saved when 	*/
#define t1			$9	/* calling a subroutine							*/
#define t2			$10
#define t3			$11
#define	t4			$12
#define t5			$13
#define t6			$14
#define t7			$15
#define t8			$24
#define t9			$25
#define s0			$16	/* save registers, will be restored after calling 	*/
#define s1			$17	/* a subroutine										*/
#define s2			$18
#define s3			$19
#define s4			$20
#define s5			$21
#define s6			$22
#define s7			$23
#define k0			$26	/* reserved for kernel */
#define k1			$27
#define	gp			$28	/* global pointer to static memory (.sdata section)	*/
#define sp			$29	/* stack pointer */
#define	fp			$30	/* frame pointer */
#define ra			$31	/* return adress */

/* COP0 registers (r5900 specific) 					*/
/* $7 && $17 - $22 && $26/$27 && $31 are undefined	*/ 

#define	Index		$0
#define	Random		$1
#define	EntryLo0	$2
#define	EntryLo1	$3
#define	Context		$4
#define	PageMask	$5
#define	Wired		$6
#define	BadVaddr	$8	/* set by address errors							*/
#define	Count		$9	/* timer together with $11							*/
#define	EntryHi		$10
#define	Compare		$11	/* timer											*/
#define Status		$12	/* status register									*/
#define	Cause		$13	/* cause of exception/interrupt						*/
#define	EPC			$14	/* exception PC										*/
#define	PRId		$15	/* processor ID										*/
#define	Config		$16	/* CPU setup parameters								*/
#define	BadPAddr	$23	/* bad phys address									*/
#define	Debug		$24	/* debug functions									*/
#define	Perf		$25	/* performance counter and control reg				*/
#define	TagLo		$28	/* cache manipulation								*/
#define	TagHi		$29	/* ...												*/
#define	ErrorEPC	$30	/* error exception PC								*/

/* COP1 - floating point registers (32 bit)	*/
/* 	  control registers still missing		*/

#define	f0			$f0	 
#define	f1			$f1
#define	f2			$f2
#define	f3			$f3
#define	f4			$f4	
#define	f5			$f5
#define	f6			$f6
#define	f7			$f7
#define	f8			$f8
#define	f9			$f9
#define	f10			$f10
#define	f11			$f11
#define	f12			$f12
#define	f13			$f13
#define	f14			$f14
#define	f15			$f15
#define	f16			$f16
#define	f17			$f17
#define	f18			$f18
#define	f19			$f19
#define	f20			$f20
#define	f21			$f21
#define	f22			$f22
#define	f23			$f23
#define	f24			$f24
#define	f25			$f25
#define	f26			$f26
#define	f27			$f27
#define	f28			$f28
#define	f29			$f29
#define	f30			$f30
#define	f31			$f31

/* COP2 - floating point registers (128 bit) */

#define	vf0			$vf0
#define	vf1			$vf1
#define	vf2			$vf2
#define	vf3			$vf3
#define	vf4			$vf4
#define	vf5			$vf5
#define	vf6			$vf6
#define	vf7			$vf7
#define	vf8			$vf8
#define	vf9			$vf9
#define	vf10		$vf10
#define	vf11		$vf11
#define	vf12		$vf12
#define	vf13		$vf13
#define	vf14		$vf14
#define	vf15		$vf15
#define	vf16		$vf16
#define	vf17		$vf17
#define	vf18		$vf18
#define	vf19		$vf19
#define	vf20		$vf20
#define	vf21		$vf21
#define	vf22		$vf22
#define	vf23		$vf23
#define	vf24		$vf24
#define	vf25		$vf25
#define	vf26		$vf26
#define	vf27		$vf27
#define	vf28		$vf28
#define	vf29		$vf29
#define	vf30		$vf30
#define	vf31		$vf31


/*****************************************************************************/
/*
**	Revision History
**	$Log: regs.h,v $
**	Revision 1.2  2005/09/18 11:27:22  tmueller
**	added authors
**
**	Revision 1.1  2005/03/13 20:01:39  fschulze
**	added
**
*/

#endif /* _PS2_HW_REGS_H */

