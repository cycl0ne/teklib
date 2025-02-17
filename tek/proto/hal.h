
#ifndef _TEK_PROTO_HAL_H
#define _TEK_PROTO_HAL_H

/*
**	$Id: hal.h,v 1.5 2005/09/13 02:45:20 tmueller Exp $
**	teklib/tek/proto/hal.h - HAL module prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/ansicall/hal.h>

extern TMODENTRY TUINT
tek_init_hal(TAPTR, struct TModule *, TUINT16, TTAGITEM *);

#endif /* _TEK_PROTO_HAL_H */
