#ifndef _TEK_UTIL_MOD_H
#define _TEK_UTIL_MOD_H

/*
**	$Id: util_mod.h,v 1.2 2006/09/10 14:39:46 tmueller Exp $
**	teklib/src/util/util_mod.h - Utility module definitions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/mod/util.h>
#include <tek/proto/exec.h>
#include <tek/proto/hal.h>
#include <tek/mod/time.h>

/*****************************************************************************/

#define UTIL_VERSION	7
#define UTIL_REVISION	0
#define UTIL_NUMVECTORS	61

/*****************************************************************************/

#ifndef LOCAL
#define LOCAL	TMODINTERN
#endif

#ifndef EXPORT
#define EXPORT	TMODAPI
#endif

/*****************************************************************************/
/*
**	Util module base structure (private)
*/

struct TUtilBase
{
	/* Module header: */
	struct TModule tmu_Module;
	/* Exec module base pointer: */
	struct TExecBase *tmu_ExecBase;
	/* HAL module base pointer: */
	struct THALBase *tmu_HALBase;
	/* Argv atom: */
	struct TAtom *tmu_AtomArgV;
	/* Internal startup modules atom: */
	struct TAtom *tmu_AtomIMods;
	/* Arguments string: */
	TSTRPTR tmu_Arguments;
	/* Arguments vector: */
	TSTRPTR *tmu_ArgV;
	/* Argument number: */
	TINT tmu_ArgC;
	/* Is big endian architecture: */
	TBOOL tmu_BigEndian;
};

/*****************************************************************************/
/*
**	Hash structure (private)
*/

struct THash
{
	struct THandle handle;
	struct THashNode **buckets;
	struct THashNode **(*lookupfunc)(struct THash *hash, TTAG key,
		TUINT *hvalp);
	struct THook *hook;
	struct TList *list;
	TSIZE numnodes;
	TUINT type;
	TUINT numbuckets;
	TINT primeidx;
};

/*****************************************************************************/

/* Argument parsing */

LOCAL TBOOL util_initargs(struct TUtilBase *util);
LOCAL void util_freeargs(struct TUtilBase *util);
EXPORT TAPTR util_parseargv(TAPTR util, TSTRPTR template, TSTRPTR *argv,
	TTAG *array);
EXPORT TBOOL util_setreturn(struct TUtilBase *util, TINT r);
EXPORT TINT util_getargc(struct TUtilBase *util);
EXPORT TSTRPTR *util_getargv(struct TUtilBase *util);
EXPORT TAPTR util_parseargs(TAPTR util, TSTRPTR template, TSTRPTR args,
	TTAG *array);
EXPORT TSTRPTR util_getargs(struct TUtilBase *util);

/* System and misc. functions */

EXPORT TUINT util_getrand(struct TUtilBase *util, TUINT seed);
EXPORT TBOOL util_isbigendian(struct TUtilBase *util);
EXPORT void util_bswap16(struct TUtilBase *util, TUINT16 *val);
EXPORT void util_bswap32(struct TUtilBase *util, TUINT32 *val);
EXPORT TUINT util_htonl(TAPTR util, TUINT n);
EXPORT TUINT16 util_htons(TAPTR util, TUINT16 n);
EXPORT TUINT util_htonlb(TAPTR util, TUINT n);
EXPORT TUINT16 util_htonsb(TAPTR util, TUINT16 n);
EXPORT TINT util_getmodules(struct TUtilBase *util, TSTRPTR name,
	struct TList *list, struct TTagItem *tags);

/* Searching and sorting */

EXPORT TBOOL util_heapsort(struct TUtilBase *util, TTAG *refarray,
	TSIZE length, struct THook *hook);
EXPORT struct TNode *util_seeknode(struct TUtilBase *util, struct TNode *node,
	TINTPTR steps);

/* String functions */

EXPORT TSTRPTR util_strdup(struct TUtilBase *util, TAPTR mmu, TSTRPTR s);
EXPORT TSTRPTR util_strndup(struct TUtilBase *util, TAPTR mmu, TSTRPTR s,
	TSIZE maxlen);
/*EXPORT TINT util_strtod(struct TUtilBase *util, TSTRPTR nptr, TDOUBLE *valp);*/

/* Hash functions */

EXPORT struct THash *util_createhash(struct TUtilBase *mod, TTAGITEM *tags);
EXPORT TBOOL util_puthash(struct TUtilBase *mod, struct THash *hash, TTAG key,
	TTAG value);
EXPORT TBOOL util_gethash(struct TUtilBase *mod, struct THash *hash, TTAG key,
	TTAG *valp);
EXPORT TBOOL util_remhash(struct TUtilBase *mod, struct THash *hash, TTAG key);
EXPORT TUINT util_hashtolist(struct TUtilBase *mod, struct THash *hash,
	struct TList *list);
EXPORT void util_hashunlist(struct TUtilBase *mod, struct THash *hash);

/* Date functions */

EXPORT TBOOL util_isleapyear(struct TUtilBase *mod, TUINT y);
EXPORT TBOOL util_isvaliddate(struct TUtilBase *mod, TUINT d, TUINT m, TUINT y);
EXPORT TBOOL util_ydaytodm(struct TUtilBase *mod, TUINT n, TUINT y, TUINT *pd, TUINT *pm);
EXPORT TUINT util_dmytoyday(struct TUtilBase *mod, TUINT d, TUINT m, TUINT y);
EXPORT TUINT util_mytoday(struct TUtilBase *mod, TUINT m, TUINT y);
EXPORT void util_datetodmy(struct TUtilBase *mod, TDATE *td, TUINT *pD, TUINT *pM, TUINT *pY, TTIME *pT);
EXPORT TUINT util_getweekday(struct TUtilBase *mod, TUINT d, TUINT m, TUINT y);
EXPORT TUINT util_getweeknumber(struct TUtilBase *mod, TUINT d, TUINT m, TUINT y);
EXPORT TBOOL util_packdate(struct TUtilBase *mod, struct TDateBox *db, TDATE *td);
EXPORT void util_unpackdate(struct TUtilBase *mod, TDATE *td, struct TDateBox *db, TUINT16 rf);

#endif /* _TEK_UTIL_MOD_H */
