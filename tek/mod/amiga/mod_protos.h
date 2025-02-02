
#ifndef _TEK_MOD_CLIB_H
#define _TEK_MOD_CLIB_H

/*
**	clib prototype for tek_init
**	this is required for fd2pragma, if you want to create the library
**	call pragma/inline for your favourite compiler. you can find the
**	result in tek/config/amiga.h
*/

#ifndef  EXEC_TYPES_H
#include <exec/types.h>
#endif

ULONG tek_mod_enter(APTR self, APTR mod, UWORD version, APTR foo);

#endif
