/*
**	$Id: imgload.h,v 1.1 2005/10/05 22:11:26 fschulze Exp $
*/

#ifndef _IMAGELOAD_H
#define _IMAGELOAD_H

#include <tek/inline/ps2.h>

GSimage u_loadImage(TSTRPTR fname, TINT halpha);
TVOID u_freeImage(GSimage *gs);

#endif /* _IMAGELOAD_H */

