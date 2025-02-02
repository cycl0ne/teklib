#ifndef _TEK_MOD_UTIL_H
#define _TEK_MOD_UTIL_H

/*
**	tek/mod/util.h - Internal Util module types and structures
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	Do not depend on this include file, as it may break your code's
**	binary compatibility with newer versions of the Util module.
**	Normal applications require only tek/util.h
*/

#include <tek/exec.h>
#include <tek/mod/time.h>

/*****************************************************************************/
/*
**	Forward declarations. Actual internal structures of the Util module
**	can be found in <tek/mod/util.h>, which shouldn't be included in
**	code that is designed for upwards compatibility.
*/

/* Util module base structure: */
struct TUtilBase;

/* Hash structure: */
struct THash;

/*****************************************************************************/
/*
**	Module entry node, as returned by TGetModules()
*/

struct TModuleEntry
{
	/* Object handle: */
	struct THandle tme_Handle;
	/* Reserved for future extensions: */
	TTAGITEM *tme_Tags;
};

/*****************************************************************************/
/*
**	Hashes
*/

#define THash_Type			(TTAG_USER + 0x1000)
#define THash_Hook			(TTAG_USER + 0x1001)

/* Keys are strings (default): */
#define THASHTYPE_STRING		0
/* As above; internally manages copies of the key names: */
#define THASHTYPE_STRINGCOPY	1
/* Keys are values of type TTAG (integers, pointers, ...): */
#define THASHTYPE_VALUE			2
/* Keys are of a custom type; supply a Hook implementing
** the message types TMSG_CALCHASH32 and TMSG_COMPAREKEYS: */
#define THASHTYPE_CUSTOM		3

struct THashNode
{
	struct TNode thn_Node;
	TTAG thn_Key;
	TTAG thn_Value;
	TUINT thn_HashValue;
};

/*****************************************************************************/
/*
**	"Datebox" - Date/Time container
*/

struct TDateBox
{
	TUINT16 tdb_Fields;		/* 0  Fields, see below */
	TUINT16 tdb_Reserved1;	/* 2  Reserved for future use */
	TUINT tdb_Year;			/* 4  Year */
	TUINT16 tdb_YDay;		/* 8  Day of year 1...366 */
	TUINT16 tdb_Month;		/* 10 Month 1...12 */
	TUINT16 tdb_Week;		/* 12 Week of year 1...53 */
	TUINT16 tdb_WDay;		/* 14 Day of week 0 (sunday) ... 6 (saturday) */
	TUINT16 tdb_Day;		/* 16 Day of month 1...31 */
	TUINT16 tdb_Hour;		/* 18 Hour of day 0...23 */
	TUINT16 tdb_Minute;		/* 20 Minute of hour 0...59 */
	TUINT16 tdb_Sec;		/* 22 Second of minute 0...59 */
	TUINT tdb_USec;			/* 24 Microsecond of second 0... 999999 */
	TUINT tdb_Reserved2;	/* 28 Reserved for future extensions */
};							/* 32 bytes */

/*
**	Corresponding flags in datebox->tdb_Fields if value present
*/

#define TDB_YEAR		0x0001
#define TDB_YDAY		0x0002
#define TDB_MONTH		0x0004
#define TDB_WEEK		0x0008
#define TDB_WDAY		0x0010
#define TDB_DAY			0x0020
#define TDB_HOUR		0x0040
#define TDB_MINUTE		0x0080
#define TDB_SEC			0x0100
#define TDB_USEC		0x0200
#define TDB_ALL         0x03ff

#endif
