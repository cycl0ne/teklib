#ifndef _TEK_MOD_PS2_EEKERNEL_H
#define _TEK_MOD_PS2_EEKERNEL_H

/*
**	$Id: eekernel.h,v 1.1 2007/04/21 14:56:36 fschulze Exp $
**	teklib/tek/mod/ps2/eekernel.h - missing ps2lib kernel functions
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/type.h>

#define ExitHandler()	__asm__ volatile("sync.l\n; ei\n")

#define SYSCALL(num)		\
	asm __volatile__(		\
		"li $3,%0\n"		\
		"syscall\n"			\
		:: "i" (num))

TINT AddDmacHandler2(TINT chan, TINT (*handler)(TINT ch, TAPTR udata, TAPTR addr), TINT next, TAPTR arg);

/*****************************************************************************/
/*
**	Revision History
**	$Log: eekernel.h,v $
**	Revision 1.1  2007/04/21 14:56:36  fschulze
**	added
**
**	
*/

#endif	/* _TEK_MOD_PS2_EEKERNEL_H */
