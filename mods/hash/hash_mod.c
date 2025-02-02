
/*
**	hash module
**	$Id: hash_mod.c,v 1.6 2005/09/07 23:59:04 tmueller Exp $
**
**	opening an instance of this module instantiates a hash object.
**	mod_open acts as the constructor, to which all arguments are
**	passed in tag items, e.g.
**
**	TTAGITEM tags[2] = { THash_Type, THASHTYPE_STRING, TTAG_DONE };
**
**	hash = TOpenModule("hash", version, tags);
**	if (hash)
**	{
**		THashPut(hash...);
**		THashGet(hash...);
**		etc.
**		TCloseModule(hash);
**	}
*/

/*****************************************************************************/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/mod/hash.h>

#define MOD_VERSION		0
#define MOD_REVISION	9
#define MOD_NUMVECTORS	15

#define BUCKET_MINSIZE	11
#define BUCKET_MAXSIZE	13845163

#define BUCKET_MINLOAD	0.3f
#define BUCKET_MAXLOAD	3.0f

#define HASHF_VALID		1
#define HASHF_FROZEN	2
#define HASHF_LISTED	4

/*****************************************************************************/

typedef struct TModHash
{
	struct TModule module;

	/* module instance data */

	TAPTR pool;

	THASHNODE **buckets;

	THASHNODE **(*lookupfunc)(struct TModHash *hash, TTAG key, TUINT *hvalp);

	THASHFUNC hashfunc;
	TCMPFUNC cmpfunc;

	TUINT type;
	TUINT flags;
	TUINT numnodes;
	TUINT numbuckets;

	TAPTR userdata;	
	TLIST *list;

} TMOD_HASH;

#define TExecBase		TGetExecBase(hash)

/*****************************************************************************/

static TCALLBACK TMOD_HASH *mod_open(TMOD_HASH *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_HASH *hash, TAPTR task);

static TMODAPI TBOOL hashget(TMOD_HASH *hash, TTAG key, TAPTR valp);
static TMODAPI TBOOL hashput(TMOD_HASH *hash, TTAG key, TTAG value);
static TMODAPI TBOOL hashremove(TMOD_HASH *hash, TTAG key);
static TMODAPI TBOOL hashvalid(TMOD_HASH *hash, TBOOL reset);
static TMODAPI TBOOL hashfreeze(TMOD_HASH *hash, TBOOL freeze);
static TMODAPI TUINT hashtolist(TMOD_HASH *hash, struct THashNode **);
static TMODAPI TVOID hashfreenode(TMOD_HASH *hash, struct THashNode *node);

/*****************************************************************************/
/*
**	Function vector table
*/

static const TAPTR hash_vectors[MOD_NUMVECTORS] =
{
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,

	(TAPTR) hashget,
	(TAPTR) hashput,
	(TAPTR) hashremove,
	(TAPTR) hashvalid,
	(TAPTR) hashfreeze,
	(TAPTR) hashtolist,
	(TAPTR) hashfreenode,
};

/*****************************************************************************/

TMODENTRY TUINT 
tek_init_hash(TAPTR task, TMOD_HASH *mod, TUINT16 version, TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * MOD_NUMVECTORS;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_HASH);				/* positive size */

		return 0;
	}

	/* put module vectors in front */
	
	TInitVectors(mod, hash_vectors, MOD_NUMVECTORS);

	/* init */

	mod->module.tmd_Version = MOD_VERSION;
	mod->module.tmd_Revision = MOD_REVISION;
	mod->module.tmd_Flags |= TMODF_EXTENDED;

	/* this module creates instances. place instance
	** open/close functions into the module structure. */

	mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
	mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;

	return TTRUE;
}

/*****************************************************************************/

static const TUINT primes[] =
{ 11, 19, 37, 73, 109, 163, 251, 367, 557, 823, 1237, 1861, 2777,
4177, 6247, 9371, 14057, 21089, 31627, 47431, 71143, 106721, 160073,
240101, 360163, 540217, 810343, 1215497, 1823231, 2734867, 4102283,
6153409, 9230113, 13845163 };

static TBOOL 
strequal(TSTRPTR s1, TSTRPTR s2)
{
	if (s1 && s2)
	{
		TINT8 a;
		while ((a = *s1++) == *s2++)
		{
			if (a == 0) return TTRUE;
		}
	}
	else if (!s1 && !s2)
	{
		return TTRUE;
	}

	return TFALSE;
}

/*****************************************************************************/

static THASHNODE **
lookup_string(TMOD_HASH *hash, TTAG key, TUINT *hvalp)
{
	THASHNODE **bucket;
	TUINT hval;
	TSTRPTR s = (TSTRPTR) key;
	TUINT g, c;

	hval = 0;
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
		if (strequal((TSTRPTR) (*bucket)->thn_Key, (TSTRPTR) key)) break;
		bucket = (THASHNODE **) &(*bucket)->thn_Next;
	}

	*hvalp = hval;
	return bucket;
}

static THASHNODE **
lookup_int(TMOD_HASH *hash, TTAG key, TUINT *hvalp)
{
	THASHNODE **bucket;
	TUINT hval;

	hval = (TUINT) (TUINTPTR) key;
	bucket = &hash->buckets[hval % hash->numbuckets];
	while (*bucket)
	{
		if ((*bucket)->thn_Key == key) break;
		bucket = (THASHNODE **) &(*bucket)->thn_Next;
	}

	*hvalp = hval;
	return bucket;
}

static THASHNODE **
lookup_custom(TMOD_HASH *hash, TTAG key, TUINT *hvalp)
{
	THASHNODE **bucket;
	TUINT hval;

	hval = (*hash->hashfunc)(hash->userdata, key);
	bucket = &hash->buckets[hval % hash->numbuckets];
	while (*bucket)
	{
		if ((*hash->cmpfunc)(hash->userdata, (*bucket)->thn_Key, key)) break;
		bucket = (THASHNODE **) &(*bucket)->thn_Next;
	}

	*hvalp = hval;
	return bucket;
}

static TVOID 
hashresize(TMOD_HASH *hash)
{
	TUINT i;
	TUINT newnumbuckets;
	THASHNODE **newbuckets, *node, *next;
	TUINT hashvalindex;
	
	TFLOAT load = (TFLOAT) hash->numnodes / hash->numbuckets;

	if ((load > BUCKET_MINLOAD || hash->numbuckets <= BUCKET_MINSIZE) &&
		(load < BUCKET_MAXLOAD || hash->numbuckets >= BUCKET_MAXSIZE))
	{
		return;
	}
	
	newnumbuckets = BUCKET_MAXSIZE;

	for (i = 0; i < sizeof(primes) / sizeof(TUINT); i++)
	{
		if (primes[i] > hash->numnodes)
		{
			newnumbuckets = primes[i];
			break;
		}
	}

	newbuckets = TAllocPool(hash->pool, sizeof(THASHNODE *) * newnumbuckets);
	if (newbuckets)
	{
		TFillMem(newbuckets, sizeof(THASHNODE *) * newnumbuckets, 0);
		for (i = 0; i < hash->numbuckets; ++i)
		{
			node = hash->buckets[i];
			while (node)
			{
				hashvalindex = node->thn_HashVal % newnumbuckets;
				next = (THASHNODE *) node->thn_Next;
				node->thn_Next = newbuckets[hashvalindex];
				newbuckets[hashvalindex] = node;
				node = next;
			}
		}

		TFreePool(hash->pool, hash->buckets,
			sizeof(THASHNODE *) * hash->numbuckets);
		hash->buckets = newbuckets;
		hash->numbuckets = newnumbuckets;
	}
}

static TVOID 
hashunlist(TMOD_HASH *hash)
{
	TUINT i;
	THASHNODE **bucket;
	TNODE *nextnode, *node = hash->list->tlh_Head;

	for (i = 0; i < hash->numbuckets; ++i)
	{
		hash->buckets[i] = TNULL;
	}
	
	while ((nextnode = node->tln_Succ))
	{
		TRemove(node);
		bucket = &hash->buckets[((THASHNODE *)node)->thn_HashVal % 
			hash->numbuckets];
		node->tln_Succ = (TNODE *) *bucket;
		*bucket = (THASHNODE *) node;
		node = nextnode;
	}

	hash->flags &= ~HASHF_LISTED;
	hash->list = TNULL;
}

/*****************************************************************************/

static TMODAPI TBOOL
hashput(TMOD_HASH *hash, TTAG key, TTAG value)
{
	if (hash->flags & HASHF_VALID)
	{
		THASHNODE **bucket;
		TUINT hval;

		if (hash->flags & HASHF_LISTED)
		{
			hashunlist(hash);
		}

		bucket = (*hash->lookupfunc)(hash, key, &hval);
		if (*bucket)
		{
			/* key exists - overwrite */

			(*bucket)->thn_Key = key;
			(*bucket)->thn_Value = value;
			(*bucket)->thn_HashVal = hval;

			return TTRUE;
		}
		else
		{
			/* key does not exist - create new node */
			
			THASHNODE *newnode = TAllocPool(hash->pool, sizeof(THASHNODE));
			if (newnode)
			{
				newnode->thn_Next = TNULL;
				newnode->thn_Key = key;
				newnode->thn_Value = value;
				newnode->thn_HashVal = hval;
			
				*bucket = newnode;
				hash->numnodes++;

				if (!(hash->flags & HASHF_FROZEN))
				{
					hashresize(hash);
				}

				return TTRUE;
			}

			/* mark hash as corrupt */
			hash->flags &= ~HASHF_VALID;
		}
	}

	return TFALSE;
}

static TMODAPI TBOOL
hashget(TMOD_HASH *hash, TTAG key, TAPTR valp)
{
	if (hash->flags & HASHF_VALID)
	{
		THASHNODE **bucket;
		TUINT hval;

		if (hash->flags & HASHF_LISTED)
		{
			hashunlist(hash);
		}

		bucket = (*hash->lookupfunc)(hash, key, &hval);
		if (*bucket)
		{
			if (valp)
			{
				*(TTAG *) valp = (*bucket)->thn_Value;
			}
			return TTRUE;
		}
	}
	
	return TFALSE;
}

static TMODAPI TBOOL
hashremove(TMOD_HASH *hash, TTAG key)
{
	if (hash->flags & HASHF_VALID)
	{
		THASHNODE **bucket;
		TUINT hval;
		
		if (hash->flags & HASHF_LISTED)
		{
			hashunlist(hash);
		}
	
		bucket = (*hash->lookupfunc)(hash, key, &hval);
		if (*bucket)
		{
			THASHNODE *node = *bucket;
	
			*bucket = (THASHNODE *) node->thn_Next;
			
			TFreePool(hash->pool, node, sizeof(THASHNODE));
			
			hash->numnodes--;
	
			if (!(hash->flags & HASHF_FROZEN))
			{
				hashresize(hash);
			}
			
			return TTRUE;
		}
	}

	return TFALSE;
}

static TMODAPI TBOOL
hashvalid(TMOD_HASH *hash, TBOOL reset)
{
	TBOOL valid = (hash->flags & HASHF_VALID) ? TTRUE: TFALSE;
	if (reset) hash->flags |= HASHF_VALID;
	return valid;
}

static TMODAPI TBOOL
hashfreeze(TMOD_HASH *hash, TBOOL freeze)
{
	TBOOL frozen = (hash->flags & HASHF_FROZEN) ? TTRUE: TFALSE;
	hash->flags &= ~HASHF_FROZEN;
	if (freeze) hash->flags |= HASHF_FROZEN;
	return frozen;
}

static TMODAPI TUINT 
hashtolist(TMOD_HASH *hash, struct THashNode **list)
{
#if 0
	if (list)
	{
		if (hash->numnodes)
		{
			TUINT i;
			THASHNODE *node, *next;
	
			/*TInitList(list);*/
	
			for (i = 0; i < hash->numbuckets; ++i)
			{
				for (node = hash->buckets[i]; node; node = next)
				{
					next = (THASHNODE *) node->thn_Next;
					TAddTail(list, (TNODE *) node);
				}
			}

			hash->list = list;		
			hash->flags |= HASHF_LISTED;
			return hash->numnodes;
		}
	}
#endif
	return 0;
}

static TMODAPI TVOID
hashfreenode(TMOD_HASH *hash, struct THashNode *node)
{
}

static TCALLBACK TMOD_HASH *
mod_open(TMOD_HASH *hash, TAPTR task, TTAGITEM *tags)
{
	TUINT type;
	TTAGITEM pooltags[3] =
	{
		{ TPool_PudSize, sizeof(THASHNODE) * 256 },
		{ TPool_AutoAdapt, TFALSE },
		{ TTAG_DONE }
	};

	/* create instance copy */

	hash = TNewInstance(hash, 
		hash->module.tmd_PosSize, hash->module.tmd_NegSize);
	if (!hash) return TNULL;

	/* instance specific setup */

	hash->pool = TCreatePool(pooltags);
	if (hash->pool)
	{
		hash->lookupfunc = TNULL;
		type = (TUINT) TGetTag(tags, THash_Type, THASHTYPE_STRING);
		hash->userdata = (TAPTR) TGetTag(tags, THash_UserData, TNULL);
	
		switch (type)
		{
			case THASHTYPE_CUSTOM:
				hash->hashfunc = (THASHFUNC) TGetTag(tags, THash_HashFunc, TNULL);
				hash->cmpfunc = (TCMPFUNC) TGetTag(tags, THash_CmpFunc, TNULL);
				if (hash->hashfunc && hash->cmpfunc)
				{
					hash->lookupfunc = lookup_custom;
				}
				break;
			case THASHTYPE_INT:
			case THASHTYPE_PTR:
				hash->lookupfunc = lookup_int;
				break;
			case THASHTYPE_STRING:
				hash->lookupfunc = lookup_string;
				break;
		}
		
		if (hash->lookupfunc)
		{
			hash->buckets = TAllocPool(hash->pool,
				sizeof(THASHNODE *) * BUCKET_MINSIZE);
			if (hash->buckets)
			{
				TFillMem(hash->buckets,
					sizeof(THASHNODE *) * BUCKET_MINSIZE, 0);
				hash->type = type;
				hash->flags = HASHF_VALID;
				hash->numbuckets = BUCKET_MINSIZE;
				hash->numnodes = 0;
				return hash;
			}
		}
	}

	/* something went wrong */
	mod_close(hash, task);
	return TNULL;
}

static TCALLBACK TVOID 
mod_close(TMOD_HASH *hash, TAPTR task)
{
	TDestroy(hash->pool);
	TFreeInstance(hash);
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: hash_mod.c,v $
**	Revision 1.6  2005/09/07 23:59:04  tmueller
**	module interface extended; cleanup
**	
**	Revision 1.5  2005/06/29 09:11:25  tmueller
**	hash keys are now of type TTAG
**	
**	Revision 1.4  2005/06/29 07:14:36  tmueller
**	now using memory pools instead of mmus
**	
**	Revision 1.2  2004/04/18 14:14:05  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.1.1.1  2003/12/11 07:19:14  tmueller
**	Krypton import
**	
**	Revision 1.3  2003/10/19 03:38:18  tmueller
**	Changed exec structure field names: Node, List, Handle, Module, ModuleEntry
**	
**	Revision 1.2  2003/04/10 02:55:34  tmueller
**	Memory corruption in hash_resize fixed
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
**	
**	Revision 1.4  2002/10/11 11:32:27  bifat
**	exec API rework
**	
**	Revision 1.3  2002/09/20 20:56:03  bifat
**	corruption in hash_remove fixed
**	
**	Revision 1.2  2002/09/18 03:03:55  bifat
**	corruption in hash resize fixed.
**	
**	Revision 1.1.1.1  2002/08/17 15:28:29  bifat
**	urknall
**	
**	Revision 1.5  2002/07/21 04:38:26  bifat
**	hash module now supports tolist, user data added to user callbacks, und lots of polishment. doc added
**	
*/
