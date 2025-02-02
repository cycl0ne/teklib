#ifndef _TEK_PROTO_UTIL_H
#define _TEK_PROTO_UTIL_H

/*
**	$Id: util.h,v 1.1.1.1 2006/08/20 22:15:26 tmueller Exp $
**	teklib/tek/proto/util.h - Util module prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/util.h>
#include <tek/stdcall/util.h>

extern TMODENTRY TUINT
tek_init_util(struct TTask *, struct TModule *, TUINT16, TTAGITEM *);

#define TUtilNToHL(util,val)	TUtilHToNL(util,val)
#define TUtilNToHS(util,val)	TUtilHToNS(util,val)

#endif /* _TEK_PROTO_UTIL_H */
