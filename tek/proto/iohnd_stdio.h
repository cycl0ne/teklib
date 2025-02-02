#ifndef _TEK_PROTO_IOHND_STDIO_H
#define _TEK_PROTO_IOHND_STDIO_H

/*
**	$Id: iohnd_stdio.h,v 1.1 2006/08/30 20:21:38 tmueller Exp $
**	teklib/tek/proto/iohnd_stdio.h - standard I/O handler prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/io.h>

extern TMODENTRY TUINT
tek_init_iohnd_stdio(struct TTask *, struct TModule *, TUINT16, TTAGITEM *);

#endif /* _TEK_PROTO_IOHND_STDIO_H */
