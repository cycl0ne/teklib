
#ifndef _UTIL_MOD_H
#define _UTIL_MOD_H

/*
**	$Id: util_mod.h,v 1.7 2005/09/13 02:42:48 tmueller Exp $
**	teklib/mods/util/util_mod.h - Utility module definitions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <math.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/mod/util.h>
#include <tek/proto/exec.h>
#include <tek/proto/hal.h>

/*****************************************************************************/

#ifndef EXPORT
#define EXPORT	TMODAPI
#endif

#ifndef LOCAL
#define LOCAL
#endif

#define MOD_VERSION		5
#define MOD_REVISION	0
#define MOD_NUMVECTORS	45

#define TExecBase		TGetExecBase(util)

typedef struct
{
	struct TModule tmu_Module;
	TAPTR tmu_HALBase;
	TINT tmu_RandomSeed;
	TAPTR tmu_AtomArgV;
	struct TInitModule *tmu_AtomIMods;
	TBOOL tmu_BigEndian;

	TSTRPTR tmu_Arguments;
	TSTRPTR *tmu_ArgV;
	TINT tmu_ArgC;

} TMOD_UTIL;

/*****************************************************************************/

/* Argument parsing */
LOCAL TBOOL util_initargs(TMOD_UTIL *util);
LOCAL TVOID util_freeargs(TMOD_UTIL *util);
EXPORT TAPTR util_parseargv(TAPTR util, TSTRPTR template, TSTRPTR *argv,
	TTAG *array);
EXPORT TBOOL util_setreturn(TMOD_UTIL *util, TINT r);
EXPORT TINT util_getargc(TMOD_UTIL *util);
EXPORT TSTRPTR *util_getargv(TMOD_UTIL *util);
EXPORT TAPTR util_parseargs(TAPTR util, TSTRPTR template, TSTRPTR args,
	TTAG *array);
EXPORT TSTRPTR util_getargs(TMOD_UTIL *util);

/* System and misc. functions */
EXPORT TUINT util_getunique(TMOD_UTIL *util, TAPTR ext);
EXPORT TINT util_getrandobs(TMOD_UTIL *util);
EXPORT TVOID util_setrandobs(TMOD_UTIL *util, TINT seed);
EXPORT TINT util_getrand(TMOD_UTIL *util, TINT seed);
EXPORT TBOOL util_isbigendian(TMOD_UTIL *util);
EXPORT TVOID util_bswap16(TMOD_UTIL *util, TUINT16 *val);
EXPORT TVOID util_bswap32(TMOD_UTIL *util, TUINT32 *val);
EXPORT TUINT util_htonl(TAPTR util, TUINT n);
EXPORT TUINT16 util_htons(TAPTR util, TUINT16 n);
EXPORT TUINT util_htonlb(TAPTR util, TUINT n);
EXPORT TUINT16 util_htonsb(TAPTR util, TUINT16 n);
EXPORT TINT util_getmodules(TMOD_UTIL *util, TSTRPTR name, struct TList *list,
	struct TTagItem *tags);

/* Searching and sorting */
EXPORT TBOOL util_heapsort(TMOD_UTIL *util, TAPTR data, TTAG *refarray,
	TUINT length, TCMPFUNC cmpfunc);
EXPORT TNODE *util_seeknode(TMOD_UTIL *util, TNODE *node, TINT steps);
EXPORT TVOID util_insertsorted(TMOD_UTIL *util, TLIST *list, TUINT numentries,
	TNODE *newnode, TCMPFUNC cmp, TAPTR userdata);
EXPORT TNODE *util_findsorted(TMOD_UTIL *util, TLIST *list, TUINT numentries,
	TFINDFUNC findfunc, TAPTR userdata);
EXPORT TBOOL util_qsort(TMOD_UTIL *util, TAPTR array, TINT num, TINT size,
	TCMPFUNC compar, TAPTR data);

/* String functions */
EXPORT TINT util_strlen(TMOD_UTIL *util, TSTRPTR s);
EXPORT TSTRPTR util_strcpy(TMOD_UTIL *util, TSTRPTR d, TSTRPTR s);
EXPORT TSTRPTR util_strncpy(TMOD_UTIL *util, TSTRPTR d, TSTRPTR s, TINT maxl);
EXPORT TSTRPTR util_strcat(TMOD_UTIL *util, TSTRPTR d, TSTRPTR s);
EXPORT TSTRPTR util_strncat(TMOD_UTIL *util, TSTRPTR d, TSTRPTR s, TINT maxl);
EXPORT TINT util_strcmp(TMOD_UTIL *util, TSTRPTR s1, TSTRPTR s2);
EXPORT TINT util_strncmp(TMOD_UTIL *util, TSTRPTR s1, TSTRPTR s2, TINT count);
EXPORT TINT util_strcasecmp(TMOD_UTIL *util, TSTRPTR s1, TSTRPTR s2);
EXPORT TINT util_strncasecmp(TMOD_UTIL *util, TSTRPTR s1, TSTRPTR s2,
	TINT count);
EXPORT TSTRPTR util_strstr(TMOD_UTIL *util, TSTRPTR s1, TSTRPTR s2);
EXPORT TSTRPTR util_strchr(TMOD_UTIL *util, TSTRPTR s, TINT c);
EXPORT TSTRPTR util_strrchr(TMOD_UTIL *util, TSTRPTR s, TINT c);
EXPORT TSTRPTR util_strdup(TMOD_UTIL *util, TAPTR mmu, TSTRPTR s);
EXPORT TSTRPTR util_strndup(TMOD_UTIL *util, TAPTR mmu, TSTRPTR s,
	TINT maxlen);
EXPORT TINT util_strtoi(TAPTR util, TSTRPTR s, TINT *valp);
EXPORT TINT util_strtod(TMOD_UTIL *util, TSTRPTR nptr, TDOUBLE *valp);

/*****************************************************************************/
/*
**	Revision History
**	$Log: util_mod.h,v $
**	Revision 1.7  2005/09/13 02:42:48  tmueller
**	updated copyright reference
**	
**	Revision 1.6  2005/09/08 03:30:00  tmueller
**	strchr/strrchr char argument changed to TINT
**	
**	Revision 1.5  2005/09/08 00:04:27  tmueller
**	API extended; strchr and strcasecmp optimized
**	
**	Revision 1.4  2005/07/07 01:44:41  tmueller
**	changed TGetRand(), TSetRand() is obsolete
**	
**	Revision 1.3  2005/06/29 09:09:15  tmueller
**	changed types of TCMPFUNC, TFINDFUNC, HeapSort refarray
**	
**	Revision 1.2  2004/07/05 21:33:43  tmueller
**	added TUtilGetArgs() and TUtilParseArgs()
**	
**	Revision 1.1  2004/07/04 21:42:12  tmueller
**	The utility module grew too large -- now splitted over several files
*/

#endif
