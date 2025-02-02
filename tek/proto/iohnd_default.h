#ifndef _TEK_PROTO_IOHND_DEFAULT_H
#define _TEK_PROTO_IOHND_DEFAULT_H

/*
**	$Id: iohnd_default.h,v 1.1 2006/08/30 20:21:38 tmueller Exp $
**	teklib/tek/proto/iohnd_default.h - default I/O handler prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/io.h>

extern TMODENTRY TUINT
tek_init_iohnd_default(struct TTask *, struct TModule *, TUINT16, TTAGITEM *);

#endif /* _TEK_PROTO_IOHND_DEFAULT_H */
