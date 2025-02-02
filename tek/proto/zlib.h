
#ifndef _TEK_PROTO_ZLIB_H
#define _TEK_PROTO_ZLIB_H

/*
**	$Id: zlib.h,v 1.3 2005/09/13 02:45:20 tmueller Exp $
**	teklib/tek/proto/zlib.h - ZLib module prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/mod/zlib/zlib.h>
#include <tek/ansicall/zlib.h>

extern TMODENTRY TUINT
tek_init_zlib(TAPTR, struct TModule *, TUINT16, TTAGITEM *);

#endif /* _TEK_PROTO_ZLIB_H */
