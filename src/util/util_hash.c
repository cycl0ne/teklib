
/*
**	teklib/mods/util/util_hash.c - Hash functions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "util_mod.h"
#include <tek/mod/exec.h>

/*****************************************************************************/

static const TUINT util_primes[] =
{
	11, 19, 37, 73, 109, 163, 251, 367, 557, 823, 1237, 1861, 2777,
 	4177, 6247, 9371, 14057, 21089, 31627, 47431, 71143, 106721, 160073,
 	240101, 360163, 540217, 810343, 1215497, 1823231, 2734867, 4102283,
 	6153409, 9230113, 13845163
};

#define HASH_NUMPRIMES	(sizeof(util_primes) / sizeof(TUINT))
#define HASH_MINLOAD	0
#define HASH_MAXLOAD	3
#define HASH_MINSIZE	11
#define HASH_MAXSIZE	13845163

/*****************************************************************************/

static void
util_resize(struct TUtilBase *mod, struct THash *hash)
{
	TINT numbuckets = hash->numbuckets;
	TSIZE load = hash->numnodes / numbuckets;
	TINT pi = hash->primeidx;

	if (numbuckets < HASH_MAXSIZE && load >= HASH_MAXLOAD)
		for (; pi < (TINT) HASH_NUMPRIMES - 1 &&
			util_primes[pi + 1] < hash->numnodes; ++pi);
	else if (numbuckets > HASH_MINSIZE && load <= HASH_MINLOAD)
		for (; pi >= 0 && util_primes[pi] >= hash->numnodes; --pi);

	if (pi != hash->primeidx)
	{
		struct THashNode **newbuckets;
		int newnumbuckets = util_primes[pi];
		newbuckets = TExecAlloc0(mod->tmu_ExecBase, TNULL,
			sizeof(struct THashNode) * newnumbuckets);
		if (newbuckets)
		{
			TUINT i;
			for (i = 0; i < hash->numbuckets; ++i)
			{
				struct THashNode *next, *node = hash->buckets[i];
				while (node)
				{
					int hashidx = node->thn_HashValue % newnumbuckets;
					next = (struct THashNode *) node->thn_Node.tln_Succ;
					node->thn_Node.tln_Succ =
						(struct TNode *) newbuckets[hashidx];
					newbuckets[hashidx] = node;
					node = next;
				}
			}

			TExecFree(mod->tmu_ExecBase, hash->buckets);
			hash->buckets = newbuckets;
			hash->numbuckets = newnumbuckets;
			hash->primeidx = pi;
		}
	}
}

/*****************************************************************************/

static TBOOL
util_strequal(TSTRPTR s1, TSTRPTR s2)
{
	if (s1 && s2)
	{
		TINT a;
		while ((a = *s1++) == *s2++)
			if (a == 0)
				return TTRUE;
	}
	else if (!s1 && !s2)
		return TTRUE;

	return TFALSE;
}

/*****************************************************************************/

static struct THashNode **
util_lookupstring(struct THash *hash, TTAG key, TUINT *hvalp)
{
	struct THashNode **bucket;
	TUINT hval = 0;
	char *s = (char *) key;
	TUINT g;
	int c;

	while ((c = *s++))
	{
		hval = (hval << 4) + c;
		if ((g = hval & 0xf0000000))
		{
			hval ^= g >> 24;
			hval ^= g;
		}
	}

	bucket = &hash->buckets[hval % hash->numbuckets];
	while (*bucket)
	{
		if (util_strequal((TSTRPTR) (*bucket)->thn_Key, (TSTRPTR) key))
			break;
		bucket = (TAPTR) &(*bucket)->thn_Node.tln_Succ;
	}

	*hvalp = hval;
	return bucket;
}

/*****************************************************************************/

static struct THashNode **
util_lookupcustom(struct THash *hash, TTAG key, TUINT *hvalp)
{
	struct THashNode **bucket;
	TUINT hval;
	TTAG obj[2];
	obj[1] = key;

	hval = (TUINT) TCALLHOOKPKT(hash->hook, (TAPTR) key, TMSG_CALCHASH32);
	bucket = &hash->buckets[hval % hash->numbuckets];
	while (*bucket)
	{
		obj[0] = (*bucket)->thn_Key;
		if (TCALLHOOKPKT(hash->hook, obj, TMSG_COMPAREKEYS))
			break;
		bucket = (TAPTR) &(*bucket)->thn_Node.tln_Succ;
	}

	*hvalp = hval;
	return bucket;
}

/*****************************************************************************/

static struct THashNode **
util_lookupvalue(struct THash *hash, TTAG key, TUINT *hvalp)
{
	TUINT hval = (TUINT) (TUINTPTR) key;
	struct THashNode **bucket = &hash->buckets[hval % hash->numbuckets];
	while (*bucket)
	{
		if ((*bucket)->thn_Key == key)
			break;
		bucket = (TAPTR) &(*bucket)->thn_Node.tln_Succ;
	}

	*hvalp = hval;
	return bucket;
}

/*****************************************************************************/

EXPORT TBOOL util_puthash(struct TUtilBase *mod, struct THash *hash, TTAG key,
	TTAG value)
{
	struct THashNode **bucket, *newnode;
	TUINT hval;

	bucket = (*hash->lookupfunc)(hash, key, &hval);
	if (*bucket)
	{
		/* key exists - overwrite */
		(*bucket)->thn_Value = value;
		return 1;
	}

	/* key does not exist - create new node */
	newnode = TExecAlloc(mod->tmu_ExecBase, TNULL,
		sizeof(struct THashNode));
	if (newnode)
	{
		if (hash->type == THASHTYPE_STRINGCOPY)
		{
			TSIZE len = TStrLen((TSTRPTR) key) + 1;
			TSTRPTR newkey = TExecAlloc(mod->tmu_ExecBase, TNULL, len);
			if (newkey == TNULL)
			{
				TExecFree(mod->tmu_ExecBase, newnode);
				return TFALSE;
			}
			TExecCopyMem(mod->tmu_ExecBase, (TAPTR) key, newkey, len);
			key = (TTAG) newkey;
		}
		newnode->thn_Node.tln_Succ = TNULL;
		newnode->thn_Key = key;
		newnode->thn_Value = value;
		newnode->thn_HashValue = hval;
		*bucket = newnode;
		hash->numnodes++;
		util_resize(mod, hash);
		return TTRUE;
	}

	return TFALSE;
}

/*****************************************************************************/

EXPORT TBOOL util_gethash(struct TUtilBase *mod, struct THash *hash, TTAG key,
	TTAG *valp)
{
	TUINT hval;
	struct THashNode **bucket = (*hash->lookupfunc)(hash, key, &hval);
	if (*bucket)
	{
		if (valp)
			*valp = (*bucket)->thn_Value;
		return TTRUE;
	}
	return TFALSE;
}

/*****************************************************************************/

EXPORT TBOOL util_remhash(struct TUtilBase *mod, struct THash *hash, TTAG key)
{
	TUINT hval;
	struct THashNode **bucket = (*hash->lookupfunc)(hash, key, &hval);
	if (*bucket)
	{
		struct THashNode *node = *bucket;
		*bucket = (struct THashNode *) node->thn_Node.tln_Succ;
		if (hash->type == THASHTYPE_STRINGCOPY)
			TExecFree(mod->tmu_ExecBase, (void *) node->thn_Key);
		TExecFree(mod->tmu_ExecBase, node);
		hash->numnodes--;
		util_resize(mod, hash);
		return TTRUE;
	}
	return TFALSE;
}

/*****************************************************************************/

LOCAL TUINT
util_hashtolist(struct TUtilBase *mod, struct THash *hash, struct TList *list)
{
	if (list && hash->numnodes > 0)
	{
		TUINT i;
		for (i = 0; i < hash->numbuckets; ++i)
		{
			struct THashNode *node, *next;
			for (node = hash->buckets[i]; node; node = next)
			{
				next = (struct THashNode *) node->thn_Node.tln_Succ;
				TAddTail(list, &node->thn_Node);
			}
		}
		hash->list = list;
		return hash->numnodes;
	}
	return 0;
}

/*****************************************************************************/

EXPORT void
util_hashunlist(struct TUtilBase *mod, struct THash *hash)
{
	if (hash->list)
	{
		TUINT i;
		struct THashNode **bucket;
		struct TNode *nextnode, *node = hash->list->tlh_Head;
		for (i = 0; i < hash->numbuckets; ++i)
			hash->buckets[i] = TNULL;
		while ((nextnode = node->tln_Succ))
		{
			TRemove(node);
			bucket = &hash->buckets[((struct THashNode *) node)->thn_HashValue %
				hash->numbuckets];
			node->tln_Succ = (struct TNode *) *bucket;
			*bucket = (struct THashNode *) node;
			node = nextnode;
		}
		hash->list = TNULL;
	}
}

/*****************************************************************************/

static THOOKENTRY TTAG util_destroyhash(struct THook *hook, TAPTR obj,
	TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct THash *hash = obj;
		struct TUtilBase *mod = hash->handle.thn_Owner;
		TUINT i;
		util_hashunlist(mod, hash);
		for (i = 0; i < hash->numbuckets; ++i)
		{
			struct THashNode *node, *nnode = hash->buckets[i];
			while ((node = nnode))
			{
				nnode = (struct THashNode *) node->thn_Node.tln_Succ;
				if (node)
				{
					if (hash->type == THASHTYPE_STRINGCOPY)
						TExecFree(mod->tmu_ExecBase, (TAPTR) node->thn_Key);
					TExecFree(mod->tmu_ExecBase, node);
				}
			}
		}
		TExecFree(mod->tmu_ExecBase, hash->buckets);
		TExecFree(mod->tmu_ExecBase, hash);
	}
	return 0;
}

/*****************************************************************************/

EXPORT struct THash *util_createhash(struct TUtilBase *mod, TTAGITEM *tags)
{
	struct THash *hash =
		TExecAlloc0(mod->tmu_ExecBase, TNULL, sizeof(struct THash));
	if (hash == TNULL)
		return TNULL;

	hash->type = (TUINT) TGetTag(tags, THash_Type, THASHTYPE_STRING);
	switch (hash->type)
	{
		default:
			return TNULL;
		case THASHTYPE_CUSTOM:
			hash->hook = (struct THook *) TGetTag(tags, THash_Hook, TNULL);
			if (hash->hook)
				hash->lookupfunc = util_lookupcustom;
			break;
		case THASHTYPE_VALUE:
			hash->lookupfunc = util_lookupvalue;
			break;
		case THASHTYPE_STRINGCOPY:
		case THASHTYPE_STRING:
			hash->lookupfunc = util_lookupstring;
			break;
			hash->lookupfunc = util_lookupstring;
			break;
	}

	if (hash->lookupfunc)
	{
		hash->buckets = TExecAlloc0(mod->tmu_ExecBase, TNULL,
			sizeof(struct THash) * HASH_MINSIZE);
		if (hash->buckets)
		{
			hash->numbuckets = HASH_MINSIZE;
			hash->lookupfunc = util_lookupstring;
			hash->numnodes = 0;
			hash->primeidx = 0;
			hash->list = TNULL;

			hash->handle.thn_Owner = mod;
			TInitHook(&hash->handle.thn_Hook, util_destroyhash, TNULL);

			return hash;
		}
	}

	TExecFree(mod->tmu_ExecBase, hash);
	return TNULL;
}
