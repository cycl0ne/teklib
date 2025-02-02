
#ifndef _GLOBAL_H
#define _GLOBAL_H

#include "parse_tmkmf.h"

#define ARG_TEMPLATE	\
"-f=FROM,-c=CONTEXT/K,-r=RECURSE/S,-b=BUILDDIR/K,-q=QUIET/S,-m=MAKEDIR/K,-h=HELP/S"
#define ARG_FROM		0
#define ARG_CONTEXT		1
#define ARG_RECURSE		2
#define ARG_BUILDDIR	3
#define ARG_QUIET		4
#define ARG_MAKEDIR		5
#define ARG_HELP		6
#define ARG_NUM			7

/*****************************************************************************/
/*
**	global	
*/

extern TAPTR TExecBase;
extern TAPTR TUtilBase;
extern TAPTR TIOBase;
extern TAPTR TDStrBase;
extern TAPTR TUStrBase;
extern TAPTR MMU;
extern TTAG args[ARG_NUM];

extern TBOOL docontext(TSTRPTR fname, TSTRPTR context);

#endif
