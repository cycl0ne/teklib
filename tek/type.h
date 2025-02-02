
#ifndef _TEK_TYPE_H
#define	_TEK_TYPE_H

/*
**	$Id: type.h,v 1.6 2005/09/13 02:44:06 tmueller Exp $
**	teklib/tek/type.h - Basic types, constants, macros
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/config/config.h>

/*****************************************************************************/
/* 
**	Type aliases
*/

typedef TINT32				TINT;		/* 32bit signed integer */
typedef TUINT32				TUINT;		/* 32bit unsigned integer */
typedef TUINT				TBOOL;		/* Boolean */
typedef TUINTPTR			TTAG;		/* Tag type; integers and pointers */
typedef TUINTPTR			TSIZE;		/* size_t type */

#ifdef __cplusplus
typedef char *				TSTRPTR;
#else
typedef TUINT8 *			TSTRPTR;
#endif

/*****************************************************************************/
/*
**	TagItem - key, value pair
*/

struct TTagItem
{
	TUINT tti_Tag;			/* Tag identifier */
	TTAG tti_Value;			/* Tag value item */
};

typedef struct TTagItem TTAGITEM;

#define TTAG_USER			0x80000000	/* User tag item */
#define TTAG_DONE			0x00000000	/* Taglist ends with this item */
#define TTAG_IGNORE			0x00000001	/* This item is being ignored */
#define TTAG_MORE			0x00000002	/* List continues at tti_Value */
#define TTAG_SKIP			0x00000003	/* Skip this plus tti_Value items */
#define TTAG_GOSUB			0x00000004	/* Traverse sublist in tti_Value */

/*****************************************************************************/
/*
**	Constants
*/

#define TNULL				0
#define TTRUE				1
#define TFALSE				0
#define TPI					(3.14159265358979323846)

/*****************************************************************************/
/*
**	Macros
*/

#define TABS(a)				((a)>0?(a):-(a))
#define TMIN(a,b)			((a)<(b)?(a):(b))
#define TMAX(a,b)			((a)>(b)?(a):(b))
#define TCLAMP(min,x,max)	((x)>(max)?(max):((x)<(min)?(min):(x)))

/*****************************************************************************/
/*
**	Revision History
**	$Log: type.h,v $
**	Revision 1.6  2005/09/13 02:44:06  tmueller
**	updated copyright reference
**	
**	Revision 1.5  2005/05/27 19:45:04  tmueller
**	added author to header
**	
**	Revision 1.4  2004/04/18 14:31:35  tmueller
**	Type definition of TTAG changed from TAPTR to TUINTPTR
**	
**	Revision 1.3  2004/02/19 05:11:27  tmueller
**	Fixed TSTRPTR definition for C++
**	
**	Revision 1.2  2004/02/15 19:47:00  tmueller
**	TSTRPTR is now TUINT8*, i.e. it changed from signed to unsigned!
**	
**	Revision 1.1.1.1  2003/12/11 07:17:45  tmueller
**	Krypton import
**	
**	Revision 1.6  2003/10/20 14:27:28  tmueller
**	Field names in tek/type.h changed: TTime, TDate, TTagItem
**	
**	Revision 1.5  2003/10/19 03:38:18  tmueller
**	Changed exec structure field names: Node, List, Handle, Module, ModuleEntry
**	
**	Revision 1.4  2003/10/18 21:47:35  tmueller
**	Cleanup of formatting and style
**	
**	Revision 1.3  2003/09/29 12:11:19  tmueller
**	Added struct equiv. for TDate, TTagItem, TTime
**	
**	Revision 1.2  2003/09/17 16:51:38  tmueller
**	(TTAG) casts removed
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
