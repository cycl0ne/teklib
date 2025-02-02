#ifndef _TEK_PROTO_ASTRO_H
#define _TEK_PROTO_ASTRO_H

/*
**	$Id: astro.h,v 1.1.1.1 2006/08/20 22:15:26 tmueller Exp $
**	teklib/tek/proto/astro.h - Astro module prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/astro.h>
#include <tek/stdcall/astro.h>

extern TMODENTRY TUINT
tek_init_astro(struct TTask *, struct TModule *, TUINT16, TTAGITEM *);

#endif /* _TEK_PROTO_ASTRO_H */
