
#ifndef _TEK_MOD_DSTRING_H_
#define _TEK_MOD_DSTRING_H_ 1

/*
**	tek/mod/dstring.h
*/

#include <tek/exec.h>

typedef struct
{	union
	{
		TUINT8 buf[12];		/* local buffer */
		TSTRPTR ptr;		/* or ptr to allocation */
	} s;
	TINT uselen;			/* bytes in use. negative if invalid. */
} TDSTR;					/* should be 16 bytes on 32bit arch */


#define _ds_arrayvalid(a)	((a)->uselen >= 0)
#define _ds_arraylocal(a)	((a)->uselen <= sizeof((a)->s.buf))
#define _ds_arraylen(a)		(a)->uselen
#define _ds_arrayptr(a)		(_ds_arrayvalid(a) ? (_ds_arraylocal(a) ? ((a)->s.buf) : ((a)->s.ptr)) : TNULL)

#endif

