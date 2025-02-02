
#ifndef _TEK_PROTO_STORAGEMANAGER_H
#define _TEK_PROTO_STORAGEMANAGER_H

/*
**	$Id: storagemanager.h,v 1.5 2005/09/13 02:45:20 tmueller Exp $
**	teklib/tek/proto/storagemanager.h - Storagemanager module prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/storagemanager.h>
#include <tek/ansicall/storagemanager.h>

extern TMODENTRY TUINT
tek_init_storagemanager(TAPTR, struct TModule *, TUINT16, TTAGITEM *);

#endif /* _TEK_PROTO_STORAGEMANAGER_H */
