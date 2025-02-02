
/*
**	$Id: io_all.c,v 1.2 2006/09/10 14:39:46 tmueller Exp $
**	teklib/src/io/io_all.c - Stub to build module from single source
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#ifndef EXPORT
#define EXPORT static TMODAPI
#endif

#ifndef LOCAL
#define LOCAL static
#endif

#ifndef TLIBAPI
#define TLIBAPI static
#endif

#include "../teklib/teklib.c"
#if defined(TDEBUG) && TDEBUG > 0
#include "../teklib/debug.c"
#endif

#include "io_mod.c"
#include "io_names.c"
#include "io_filelock.c"
#include "io_standard.c"
