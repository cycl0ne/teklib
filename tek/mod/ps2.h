#ifndef _TEK_MOD_PS2_H
#define _TEK_MOD_PS2_H

/*
**	$Id: ps2.h,v 1.2 2006/03/26 14:28:35 fschulze Exp $
**  tek/mod/ps.h - ps2 macros and definitions
*/

#include <tek/exec.h>
#include <tek/mod/ps2/gs.h>
#include <tek/mod/ps2/dma.h>
#include <tek/mod/ps2/type3d.h>

struct TPS2ModBase
{
	struct TModule ps2_Module;	/* module header */
	TAPTR ps2_Private;			/* module-internal data */
	GSinfo *ps2_GSInfo;
	TBOOL ps2_DMADebug;
};

typedef TVOID(*DMACB)(TAPTR userdata);


/* 			 2^c = b
 * 		log _2 b = c
 *
 * while(b) b>>=1; c++;
 */

#define u_ld(b) ({ int __i = 0, __j = b; while (__j >>= 1) __i++; __i; })

#endif /* _TEK_MOD_PS2_H */
