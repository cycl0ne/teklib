
#ifndef _TEK_MOD_UNISTRING_H
#define _TEK_MOD_UNISTRING_H

/*
**	$Id: unistring.h,v 1.12 2005/09/13 02:45:09 tmueller Exp $
**	teklib/tek/mod/unistring.h - Unistring module definitions
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>

typedef TINT TWCHAR;			/* 31bit wide character */
typedef TINT TUString;			/* Dynamic String/Array Index */

#define TINVALID_WCHAR	(-1)	/* Not a valid wide character */
#define TINVALID_ARRAY	(-1)	/* Not a valid dynamic array */
#define TINVALID_STRING	(-1)	/* Not a valid dynamic string */

/*****************************************************************************/
/* 
**	Element sizes
*/

#define TASIZE_7BIT		0x0100	/* Element size is 7bit (TUINT8) */
#define TASIZE_8BIT		0x0000	/* Element size is 8bit (TUINT8) */
#define TASIZE_16BIT	0x0001	/* Element size is 16bit (TUINT16) */
#define TASIZE_32BIT	0x0002	/* Element size is 32bit (TUINT) */
#define TASIZE_64BIT	0x0003	/* Element size is 64bit (8 bytes) */
#define TASIZE_128BIT	0x0004	/* Element size is 128bit (16 bytes) */

#define TASIZE_TAPTR	0x0010	/* Element size is sizeof(TAPTR) */
#define TASIZE_TFLOAT	0x0020	/* Element size is sizeof(TFLOAT) */
#define TASIZE_TDOUBLE	0x0040	/* Element size is sizeof(TDOUBLE) */
#define TASIZE_TTAGITEM	0x0080	/* Element size is sizeof(TTAGITEM) */

/*****************************************************************************/
/* 
**	Transformations
*/

#define TSTRF_UPPER		0x0001
#define TSTRF_LOWER		0x0002

#define TSTRF_NOLOSS	0x0010

/*****************************************************************************/
/*
**	TUString_Local (TBOOL)
**		By default, opens to the unistring module return a module base that
**		operates on a global memory pool and ID space. If set to TTRUE, this
**		tag causes TExecOpenModule() to create a local module instance with
**		a private memory pool and ID space. Local pools are slightly faster,
**		and all objects are automatically freed when the instance is being
**		closed. Do not use this tag if you want to pass strings between
**		different openers of the module. Default: TFALSE
**
**	TUString_FragSize (TINT)
**		Initial size of fragments for newly allocated strings, in number of
**		elements. Only taken into account for local instances. Default: 64
*/

#define TUString_Local			(TTAG_USER + 0x2100)
#define TUString_FragSize		(TTAG_USER + 0x2101)

/*****************************************************************************/
/* 
**	Tags for TUStrFindPatString()
*/

#define	TUStrFind_PlainSearch	(TTAG_USER + 0x2110)
#define	TUStrFind_MatchBegin	(TTAG_USER + 0x2111)
#define	TUStrFind_MatchEnd		(TTAG_USER + 0x2112)
#define	TUStrFind_MatchString	(TTAG_USER + 0x2113)
#define	TUStrFind_NumCaptures	(TTAG_USER + 0x2114)
#define	TUStrFind_CaptureBegin	(TTAG_USER + 0x2115)
#define	TUStrFind_CaptureEnd	(TTAG_USER + 0x2116)
#define	TUStrFind_CaptureString	(TTAG_USER + 0x2117)

#endif
