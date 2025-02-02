
#ifndef _TEK_MOD_ZLIB_MOD_H
#define _TEK_MOD_ZLIB_MOD_H

#include <tek/exec.h>

typedef struct				/* module base */
{
	struct TModule module;	/* module header */
	TAPTR mmu;				/* memory manager */

} ZMOD;

#define TExecBase	TGetExecBase(zlib)

#endif
