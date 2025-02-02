
/*
**	$Id: util_all.c,v 1.2 2006/09/10 14:39:46 tmueller Exp $
**	teklib/src/util/util_all.c - Stub to build module from single source
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

#include "util_mod.c"
#include "util_args.c"
#include "util_string.c"
#include "util_searchsort.c"
#include "util_hash.c"
#include "util_time.c"
