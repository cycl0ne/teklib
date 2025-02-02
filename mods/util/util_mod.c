
/*
**	$Id: util_mod.c,v 1.18 2005/09/13 02:42:48 tmueller Exp $
**	teklib/mods/util/util_mod.c - Utility module implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "util_mod.h"

static const TAPTR mod_vectors[MOD_NUMVECTORS];
static TCALLBACK TVOID util_moddestroy(TMOD_UTIL *util);

/*****************************************************************************/
/*
**	Module initializations
*/

TMODENTRY TUINT
tek_init_util(TAPTR task, TMOD_UTIL *util, TUINT16 version, TTAGITEM *tags)
{
	if (util == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * MOD_NUMVECTORS;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_UTIL);				/* positive size */

		return 0;
	}
	
	if (util_initargs(util))
	{
		TTIME ttime;
		TUINT8 endiancheck[4] = { 0x11, 0x22, 0x33, 0x44 };

		util->tmu_AtomIMods = TExecLockAtom(TExecBase, "sys.imods", 
			TATOMF_NAME | TATOMF_SHARED);
	
		util->tmu_HALBase = TExecGetHALBase(TExecBase);
		THALGetSysTime(util->tmu_HALBase, &ttime);
		util->tmu_RandomSeed = ttime.ttm_USec;
		
		util->tmu_BigEndian = (*((TUINT *) &endiancheck) == 0x11223344);
	
		util->tmu_Module.tmd_Version = MOD_VERSION;
		util->tmu_Module.tmd_Revision = MOD_REVISION;
		util->tmu_Module.tmd_DestroyFunc = (TDFUNC) util_moddestroy;
		util->tmu_Module.tmd_Flags |= TMODF_EXTENDED;
	
		/* put module vectors in front */
		TInitVectors(util, (TAPTR *) mod_vectors, MOD_NUMVECTORS);
	
		if (util->tmu_BigEndian)
		{
			((TAPTR *) util)[-31-8] = (TAPTR) util_htonlb;
			((TAPTR *) util)[-32-8] = (TAPTR) util_htonsb;
		}
	
		return TTRUE;
	}

	return 0;
}

static TCALLBACK TVOID
util_moddestroy(TMOD_UTIL *util)
{
	TExecUnlockAtom(TExecBase, util->tmu_AtomIMods, TATOMF_KEEP);
	util_freeargs(util);
}

static const TAPTR mod_vectors[MOD_NUMVECTORS] =
{
	(TAPTR) TNULL,				/* reserved */
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,

	(TAPTR) util_getargc,
	(TAPTR) util_getargv,
	(TAPTR) util_setreturn,
	(TAPTR) util_getunique,
	(TAPTR) util_getrandobs,	/* as of v4 */
	(TAPTR) util_setrandobs,	/* as of v4 */

	(TAPTR) util_heapsort,
	(TAPTR) util_seeknode,
	(TAPTR) util_insertsorted,
	(TAPTR) util_findsorted,

	(TAPTR) util_isbigendian,
	(TAPTR) util_bswap16,
	(TAPTR) util_bswap32,

	(TAPTR) util_strlen,
	(TAPTR) util_strcpy,
	(TAPTR) util_strncpy,
	(TAPTR) util_strcat,
	(TAPTR) util_strncat,

	(TAPTR) util_strcmp,
	(TAPTR) util_strncmp,
	(TAPTR) util_strcasecmp,
	(TAPTR) util_strncasecmp,

	(TAPTR) util_strstr,
	(TAPTR) util_strchr,
	(TAPTR) util_strrchr,

	(TAPTR) util_strdup,
	(TAPTR) util_strndup,

	(TAPTR) util_strtoi,
	(TAPTR) util_getrand,		/* v4 */

	(TAPTR) util_parseargv,

	(TAPTR) util_htonl,
	(TAPTR) util_htons,

	(TAPTR) util_strtod,

	(TAPTR) util_qsort,
	(TAPTR) util_getmodules,
	
	(TAPTR) util_parseargs,
	(TAPTR) util_getargs,
};

/*****************************************************************************/
/*
**	id = util_getunique(util, extended)
**	Return (relativey) unique ID
*/

EXPORT TUINT
util_getunique(TMOD_UTIL *util, TAPTR ext)
{
	TUINT val;
	TUINT *p;
	TAPTR atom;

	atom = TExecLockAtom(TExecBase, "sys.uniqueid", TATOMF_NAME);
	if (!atom)
	{
		tdbprintf(99, "sys.uniqueid atom lock failed\n");
		tdbfatal(99);
	}
	p = (TUINT *) TExecGetAtomData(TExecBase, atom);
	val = (*p)++;

	TExecUnlockAtom(TExecBase, atom, TATOMF_KEEP);
	
	return val;
}

/*****************************************************************************/
/*
**	integer = util_getrand(utilbase)
**	Calculate pseudo random number
*/

EXPORT TINT 
util_getrand(TMOD_UTIL *util, TINT seed)
{
	TUINT lo, hi;

	lo = 16807 * (TINT) (seed & 0xffff);
	hi = 16807 * (TINT) ((TUINT) seed >> 16);
	lo += (hi & 0x7fff) << 16;
	if (lo > 2147483647)
	{
		lo &= 2147483647;
		++lo;
	}
	lo += hi >> 15;
	if (lo > 2147483647)
	{
		lo &= 2147483647;
		++lo;
	}

	return (TINT) lo;
}

/*****************************************************************************/
/*
**	integer = util_getrandobs(utilbase)
**	Calculate pseudo random number (obsolete)
*/

EXPORT TINT 
util_getrandobs(TMOD_UTIL *util)
{
	TINT r = util_getrand(util, util->tmu_RandomSeed);
	util->tmu_RandomSeed = r;
	return r;
}

/*****************************************************************************/
/*
**	util_setrandobs(utilbase, seed)
**	set random seed (obsolete)
*/

EXPORT TVOID 
util_setrandobs(TMOD_UTIL *util, TINT seed)
{
	util->tmu_RandomSeed = seed;
}

/*****************************************************************************/
/*
**	boolean = util_isbigendian(util)
**	Returns TTRUE, if system is big endian
*/

EXPORT TBOOL
util_isbigendian(TMOD_UTIL *util)
{
	return (TBOOL) util->tmu_BigEndian;
}

/*****************************************************************************/
/*
**	util_bswap16(util, value)
**	Swaps bytes of a short
*/

EXPORT TVOID
util_bswap16(TMOD_UTIL *util, TUINT16 *val)
{
	TUINT16 v;

	TUINT8* s=(TUINT8*)val;
	TUINT8* d=(TUINT8*)&v;

	d[0] = s[1];
	d[1] = s[0];

	*val=v;
}

/*****************************************************************************/
/*
**	util_bswap32(util, value)
**	Swaps bytes of a long
*/

EXPORT TVOID
util_bswap32(TMOD_UTIL *util, TUINT32 *val)
{
	TUINT v;

	TUINT8* s=(TUINT8*)val;
	TUINT8* d=(TUINT8*)&v;

	d[0] = s[3];
	d[1] = s[2];
	d[2] = s[1];
	d[3] = s[0];

	*val=v;
}

/*****************************************************************************/
/*
**	endianess functions
*/

EXPORT TUINT 
util_htonl(TAPTR util, TUINT n)
{
	util_bswap32(util, &n);
	return n;
}

EXPORT TUINT16
util_htons(TAPTR util, TUINT16 n)
{
	util_bswap16(util, &n);
	return n;
}

EXPORT TUINT
util_htonlb(TAPTR util, TUINT n)
{
	return n;
}

EXPORT TUINT16
util_htonsb(TAPTR util, TUINT16 n)
{
	return n;
}

/*****************************************************************************/
/*
**	numentries = util_getmodules(util, path, list, tags)
**	Get a list of modules that are available to the system. This includes
**	modules in the filesystem as well as those linked in statically.
**	returns -1 on error, else number of entries found
*/

static TCALLBACK TVOID 
destroymodnode(struct TModuleEntry *entry)
{
	TAPTR exec = TGetExecBase(entry);
	TExecFree(exec, entry);
}

struct scandata
{
	TAPTR exec;
	struct TList *list;
};

static TCALLBACK TBOOL
scanfunc(struct scandata *sd, TSTRPTR name, TINT len)
{
	struct TModuleEntry *ne;
	TAPTR exec = sd->exec;
	ne = TExecAlloc(exec, TNULL, sizeof(struct TModuleEntry) + len + 1);
	if (ne)
	{
		TExecCopyMem(exec, name, (TAPTR)(ne + 1), len);
		ne->tme_Handle.tmo_Name = (TSTRPTR)(ne + 1);
		ne->tme_Handle.tmo_Name[len] = 0;
		TAddTail(sd->list, (struct TNode *) ne);
		return TTRUE;
	}
	return TFALSE;
}

EXPORT TINT
util_getmodules(TMOD_UTIL *util, TSTRPTR path, struct TList *list,
	struct TTagItem *tags)
{
	struct TList templist;
	struct TNode *nextnode, *node;
	TBOOL success = TTRUE;
	struct TInitModule *imods = (struct TInitModule *)
		TExecGetAtomData(TExecBase, util->tmu_AtomIMods);
	struct TModuleEntry *ne;
	TINT l, pl;
	TSTRPTR p;
	TINT numentries = -1;
	
	if (list == TNULL) return 0;

	TInitList(&templist);

	/* determine path length */

	if (path)
	{
		p = path;
		while (*p++);
		pl = p - path - 1;
	}
	else
	{
		pl = 0;
	}

	/* scan internal modules */
	if (imods)
	{
		while ((p = imods->tinm_Name))
		{
			tdbprintf1(5,"scan internal module: %s\n", p);
			l = 0;
			if (pl)
			{
				do
				{
					if (*p != path[l]) goto nopath;
					p++;
					l++;
				} while (l < pl);
			}
			l++;
			while (*p++) l++;
	
			ne = TExecAlloc(TExecBase, TNULL, sizeof(struct TModuleEntry) + l);
			if (ne)
			{
				TExecCopyMem(TExecBase, imods->tinm_Name, (TAPTR)(ne + 1), l);
				ne->tme_Handle.tmo_Name = (TSTRPTR)(ne + 1);
				TAddTail(&templist, (struct TNode *) ne);
			}
			else
			{
				success = TFALSE;
				break;
			}
	nopath:	imods++;
		}
	}

	/* scan modules from HAL */
	if (success)
	{
		struct scandata sd;
		sd.exec = TExecBase;
		sd.list = &templist;
		success = THALScanModules(util->tmu_HALBase,
			path ? path : (TSTRPTR) "", 
			(TCALLBACK TBOOL (*)(TAPTR, TSTRPTR, TINT)) scanfunc, &sd);
	}

	node = templist.tlh_Head;
	if (success)
	{
		struct TModuleEntry *e;
		struct TNode *nn, *n;
		struct TList *insertlist;

		numentries = 0;

		/* insert destructor to all nodes, insert to user list */
		while ((nextnode = node->tln_Succ))
		{
			e = (struct TModuleEntry *) node;
			e->tme_Handle.tmo_ModBase = TExecBase;
			e->tme_Handle.tmo_DestroyFunc = (TDFUNC) destroymodnode;

			n = list->tlh_Head;
			insertlist = list;
			while ((nn = n->tln_Succ))
			{
				if (!util_strcmp(util, e->tme_Handle.tmo_Name,
					((struct TModuleEntry *) n)->tme_Handle.tmo_Name))
				{
					insertlist = TNULL;		/* double node */
					break;
				}
				n = nn;
			}
			if (insertlist)
			{
				TRemove(node);
				e->tme_Tags = TNULL;
				TAddTail(insertlist, node);
				numentries++;
			}
			node = nextnode;			
		}
			
		/* delete remaining (double) nodes */
		TDestroyList(&templist);
	}
	else
	{
		/* free all nodes */
		
		while ((nextnode = node->tln_Succ))
		{
			TExecFree(TExecBase, node);
			node = nextnode;
		}
	}
	
	return numentries;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: util_mod.c,v $
**	Revision 1.18  2005/09/13 02:42:48  tmueller
**	updated copyright reference
**	
**	Revision 1.17  2005/09/08 03:27:53  tmueller
**	module vectors got b0rked on big endian - fixed
**	
**	Revision 1.16  2005/09/08 00:04:27  tmueller
**	API extended; strchr and strcasecmp optimized
**	
**	Revision 1.15  2005/07/07 01:44:41  tmueller
**	changed TGetRand(), TSetRand() is obsolete
**	
**	Revision 1.14  2005/03/21 11:11:43  fschulze
**	added typecast in mod scanner gets rid of warning
**	
**	Revision 1.13  2005/03/14 10:57:16  tmueller
**	TUtilGetModules(): Internal module scanning was broken - fixed
**	
**	Revision 1.12  2004/07/05 21:33:42  tmueller
**	added TUtilGetArgs() and TUtilParseArgs()
**	
**	Revision 1.11  2004/07/04 21:42:12  tmueller
**	The utility module grew too large -- now splitted over several files
**	
*/
