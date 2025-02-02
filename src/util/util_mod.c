
/*
**	$Id: util_mod.c,v 1.6 2006/11/11 14:19:10 tmueller Exp $
**	teklib/src/util/util_mod.c - Utility module implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "util_mod.h"
#include <tek/mod/hal.h>

static const TMFPTR util_vectors[UTIL_NUMVECTORS];
static THOOKENTRY TTAG util_dispatch(struct THook *hook, TAPTR obj, TTAG msg);

/*****************************************************************************/
/*
**	Module initializations
*/

TMODENTRY TUINT
tek_init_util(struct TTask *task, struct TModule *mod, TUINT16 version,
	TTAGITEM *tags)
{
	struct TUtilBase *util = (struct TUtilBase *) mod;
	TAPTR exec;

	if (util == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * UTIL_NUMVECTORS; /* negative size */

		if (version <= UTIL_VERSION)
			return sizeof(struct TUtilBase); /* positive size */

		return 0;
	}

	exec = TGetExecBase(util);
	util->tmu_ExecBase = exec;

	if (util_initargs(util))
	{
		TUINT8 endiancheck[4] = { 0x11, 0x22, 0x33, 0x44 };

		util->tmu_AtomIMods = TExecLockAtom(exec, "sys.imods",
			TATOMF_NAME | TATOMF_SHARED);

		util->tmu_HALBase = TExecGetHALBase(exec);

		util->tmu_BigEndian = (*((TUINT *) &endiancheck) == 0x11223344);

		util->tmu_Module.tmd_Version = UTIL_VERSION;
		util->tmu_Module.tmd_Revision = UTIL_REVISION;
		util->tmu_Module.tmd_Handle.thn_Hook.thk_Entry = util_dispatch;

		util->tmu_Module.tmd_Flags = TMODF_VECTORTABLE;

		/* put module vectors in front */
		TInitVectors(&util->tmu_Module, util_vectors, UTIL_NUMVECTORS);

		if (util->tmu_BigEndian)
		{
			((TMFPTR *) util)[-35] = (TMFPTR) util_htonlb;
			((TMFPTR *) util)[-36] = (TMFPTR) util_htonsb;
		}

		return TTRUE;
	}

	return 0;
}

static THOOKENTRY TTAG
util_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TUtilBase *util = obj;
		TAPTR exec = TGetExecBase(util);
		TExecUnlockAtom(exec, util->tmu_AtomIMods, TATOMF_KEEP);
		util_freeargs(util);
	}
	return 0;
}

static const TMFPTR
util_vectors[UTIL_NUMVECTORS] =
{
	(TMFPTR) TNULL,				/* reserved */
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) util_getargc,
	(TMFPTR) util_getargv,
	(TMFPTR) util_setreturn,

	(TMFPTR) util_heapsort,
	(TMFPTR) util_seeknode,

	(TMFPTR) util_isbigendian,
	(TMFPTR) util_bswap16,
	(TMFPTR) util_bswap32,

	(TMFPTR) util_strdup,
	(TMFPTR) util_strndup,

	(TMFPTR) util_getrand,

	(TMFPTR) util_parseargv,

	(TMFPTR) util_htonl,
	(TMFPTR) util_htons,

	(TMFPTR) util_getmodules,

	(TMFPTR) util_parseargs,
	(TMFPTR) util_getargs,

	(TMFPTR) util_createhash,
	(TMFPTR) util_puthash,
	(TMFPTR) util_gethash,
	(TMFPTR) util_remhash,
	(TMFPTR) util_hashtolist,
	(TMFPTR) util_hashunlist,

	(TMFPTR) util_isleapyear,
	(TMFPTR) util_isvaliddate,
	(TMFPTR) util_ydaytodm,
	(TMFPTR) util_dmytoyday,
	(TMFPTR) util_mytoday,
	(TMFPTR) util_datetodmy,
	(TMFPTR) util_getweekday,
	(TMFPTR) util_getweeknumber,
	(TMFPTR) util_packdate,
	(TMFPTR) util_unpackdate,
};

/*****************************************************************************/
/*
**	integer = util_getrand(utilbase)
**	Calculate pseudo random number
*/

EXPORT TUINT
util_getrand(struct TUtilBase *util, TUINT seed)
{
#if 0
	return seed * 1664525 + 1;
#else
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
#endif
}

/*****************************************************************************/
/*
**	boolean = util_isbigendian(util)
**	Returns TTRUE, if system is big endian
*/

EXPORT TBOOL
util_isbigendian(struct TUtilBase *util)
{
	return (TBOOL) util->tmu_BigEndian;
}

/*****************************************************************************/
/*
**	util_bswap16(util, value)
**	Swaps bytes of a short
*/

EXPORT void
util_bswap16(struct TUtilBase *util, TUINT16 *val)
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

EXPORT void
util_bswap32(struct TUtilBase *util, TUINT32 *val)
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

struct util_scanhookdata
{
	TAPTR exec;
	struct TList *list;
};

static THOOKENTRY TTAG
util_destroymodnode(struct THook *hook, TAPTR obj, TTAG msg)
{
	TAPTR exec = TGetExecBase(obj);
	TDBASSERT(20, msg == 0);
	TExecFree(exec, obj);
	return 0;
}

static THOOKENTRY TTAG
util_scanfunc(struct THook *hook, TAPTR obj, TTAG m)
{
	struct THALScanModMsg *msg = (struct THALScanModMsg *) m;
	struct util_scanhookdata *sd = hook->thk_Data;
	TAPTR exec = sd->exec;
	TINT len = msg->tsmm_Length;
	struct TModuleEntry *ne;
	ne = TExecAlloc(exec, TNULL, sizeof(struct TModuleEntry) + len + 1);
	if (ne)
	{
		TExecCopyMem(exec, msg->tsmm_Name, (TAPTR)(ne + 1), len);
		ne->tme_Handle.thn_Name = (TSTRPTR)(ne + 1);
		ne->tme_Handle.thn_Name[len] = 0;
		TAddTail(sd->list, (struct TNode *) ne);
		return TTRUE;
	}
	return TFALSE;
}

EXPORT TINT
util_getmodules(struct TUtilBase *util, TSTRPTR path, struct TList *list,
	struct TTagItem *tags)
{
	TAPTR exec = TGetExecBase(util);
	struct TList templist;
	struct TNode *nextnode, *node;
	TBOOL success = TTRUE;
	struct TInitModule *imods = (struct TInitModule *)
		TExecGetAtomData(exec, util->tmu_AtomIMods);
	struct TModuleEntry *ne;
	TINT l, pl;
	TSTRPTR p;
	TINT numentries = -1;

	if (list == TNULL)
		return 0;

	TINITLIST(&templist);

	/* determine path length */

	if (path)
	{
		p = path;
		while (*p++);
		pl = p - path - 1;
	}
	else
		pl = 0;

	/* scan internal modules */
	if (imods)
	{
		while ((p = imods->tinm_Name))
		{
			TDBPRINTF(5,("scan internal module: %s\n", p));
			l = 0;
			if (pl)
			{
				do
				{
					if (*p != path[l])
						goto nopath;
					p++;
					l++;
				} while (l < pl);
			}
			l++;
			while (*p++) l++;

			ne = TExecAlloc(exec, TNULL, sizeof(struct TModuleEntry) + l);
			if (ne)
			{
				TExecCopyMem(exec, imods->tinm_Name, (TAPTR)(ne + 1), l);
				ne->tme_Handle.thn_Name = (TSTRPTR)(ne + 1);
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
		struct THook scanhook;
		struct util_scanhookdata sd;

		sd.exec = exec;
		sd.list = &templist;
		scanhook.thk_Entry = util_scanfunc;
		scanhook.thk_Data = &sd;
		success = THALScanModules(util->tmu_HALBase,
			path ? path : (TSTRPTR) "", &scanhook);
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
			e->tme_Handle.thn_Owner = exec;
			e->tme_Handle.thn_Hook.thk_Entry = util_destroymodnode;

			n = list->tlh_Head;
			insertlist = list;
			while ((nn = n->tln_Succ))
			{
				if (!TStrCmp(e->tme_Handle.thn_Name,
					((struct TModuleEntry *) n)->tme_Handle.thn_Name))
				{
					/* double node */
					insertlist = TNULL;
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
			TExecFree(exec, node);
			node = nextnode;
		}
	}

	return numentries;
}

