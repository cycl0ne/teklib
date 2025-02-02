
#ifndef _TEK_PROTO_PS2_H
#define _TEK_PROTO_PS2_H

/*
**	$Id: ps2.h,v 1.1 2005/09/18 12:33:39 tmueller Exp $
**	teklib/tek/proto/ps2.h - PS2 module prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/ps2.h>
#include <tek/ansicall/ps2.h>

extern TMODENTRY TUINT
tek_init_ps2common(TAPTR, struct TModule *, TUINT16, TTAGITEM *);

#endif /* _TEK_PROTO_PS2_H */
