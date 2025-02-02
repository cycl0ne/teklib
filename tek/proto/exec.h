
#ifndef _TEK_PROTO_EXEC_H
#define _TEK_PROTO_EXEC_H

/*
**	$Id: exec.h,v 1.6 2005/09/13 02:45:20 tmueller Exp $
**	teklib/tek/proto/exec.h - Exec module prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/ansicall/exec.h>

extern TMODENTRY TUINT
tek_init_exec(TAPTR, struct TModule *, TUINT16, TTAGITEM *);

#endif /* _TEK_PROTO_EXEC_H */
