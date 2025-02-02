
/*
**	$Id: host.c,v 1.4 2005/09/18 11:27:22 tmueller Exp $
**	boot/ps2/host.c - PS2 implementation of startup functions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "init.h"
#include <tek/mod/ps2/hal.h>
#include <stdlib.h>
#include <string.h>

struct ModHandle
{
	TAPTR entry;
	TUINT type;
};

#define TYPE_DLL	0
#define TYPE_LIB	1

/*****************************************************************************/
/*
**	root allocator
*/

static TBOOL
initPool(struct HALMemHead *mh, TAPTR mem, TUINT size, TUINT flags,
	TINT alignsize)
{
	TUINT align = sizeof(TAPTR);
	while (align < alignsize) align <<= 1;
	if (size >= align)
	{
		--align;
		mem = (TAPTR) (((TUINT) mem + align) & ~align);
		size -= align;
		size &= ~align;

		mh->hmh_Mem = mem;
		mh->hmh_MemEnd = ((TINT8 *) mem) + size;
		mh->hmh_Free = size;
		mh->hmh_Align = align;
		mh->hmh_Flags = flags;

		mh->hmh_FreeList = (struct HALMemNode *) mem;
		((struct HALMemNode *) mem)->hmn_Next = TNULL;
		((struct HALMemNode *) mem)->hmn_Size = size;

		return TTRUE;
	}
	return TFALSE;
}

static TVOID 
availPool(TAPTR boot, TUINT *freep, TUINT *nump)
{
	struct HALMemHead *mh = ((struct HALPS2Boot *) boot)->hpb_MemHead;
	struct HALMemNode **mnp, *mn;
	mnp = &mh->hmh_FreeList;
	*freep = 0;
	*nump = 0;
	while ((mn = *mnp))
	{
		(*freep) += mn->hmn_Size;
		(*nump)++;
		mnp = &mn->hmn_Next;
	}
}

static TAPTR
int_alloc(struct HALMemHead *mh, TUINT size)
{
	size = (size + mh->hmh_Align) & ~mh->hmh_Align;

	if (mh->hmh_Free >= size)
	{
		struct HALMemNode **mnp, *mn, **x;

		mnp = &mh->hmh_FreeList;
		x = TNULL;

		/* firstfit strategy */

		while ((mn = *mnp))
		{
			if (mn->hmn_Size == size)
			{
				*mnp = mn->hmn_Next;
				mh->hmh_Free -= size;
				return (TAPTR) mn;
			}
			else if (mn->hmn_Size > size)
			{
				x = mnp;
				break;
			}
			mnp = &mn->hmn_Next;
		}

		if (x)
		{
			mn = *x;
			*x = (struct HALMemNode *) ((TINT8 *) mn + size);
			(*x)->hmn_Next = mn->hmn_Next;
			(*x)->hmn_Size = mn->hmn_Size - size;
			mh->hmh_Free -= size;
			return (TAPTR) mn;
		}
	}

	return TNULL;
}

TAPTR 
boot_alloc(TAPTR boot, TUINT size)
{
	struct HALMemHead *mh = ((struct HALPS2Boot *) boot)->hpb_MemHead;
	return int_alloc(mh, size);
}

static TVOID
int_free(struct HALMemHead *mh, TAPTR mem, TUINT size)
{
	if (mem)
	{
		struct HALMemNode **mnp, *mn, *pmn;
		size = (size + mh->hmh_Align) & ~mh->hmh_Align;

		mh->hmh_Free += size;
		mnp = &mh->hmh_FreeList;
		pmn = TNULL;

		while ((mn = *mnp))
		{
			if ((TINT8 *) mem < (TINT8 *) mn) break;
			pmn = mn;
			mnp = &mn->hmn_Next;
		}

		if (mn && ((TINT8 *) mem + size == (TINT8 *) mn))
		{
			size += mn->hmn_Size;
			mn = mn->hmn_Next;		/* concatenate with following free node */
		}

		if (pmn && ((TINT8 *) pmn + pmn->hmn_Size == (TINT8 *) mem))
		{
			size += pmn->hmn_Size;	/* concatenate with previous free node */
			mem = pmn;
		}

		*mnp = (struct HALMemNode *) mem;
		((struct HALMemNode *) mem)->hmn_Next = mn;
		((struct HALMemNode *) mem)->hmn_Size = size;
	}
}

TVOID
boot_free(TAPTR boot, TAPTR mem, TUINT size)
{
	struct HALMemHead *mh = ((struct HALPS2Boot *) boot)->hpb_MemHead;
	int_free(mh, mem, size);
}

static TAPTR
boot_realloc(TAPTR boot, TAPTR oldmem, TUINT oldsize, TUINT newsize)
{
	struct HALMemHead *mh = ((struct HALPS2Boot *) boot)->hpb_MemHead;
	TAPTR newmem;
	struct HALMemNode **mnp, *mn, *mend;
	TINT diffsize;

	oldsize = (oldsize + mh->hmh_Align) & ~mh->hmh_Align;
	newsize = (newsize + mh->hmh_Align) & ~mh->hmh_Align;

	if (newsize == oldsize) return oldmem;

	/* end of old allocation */
	mend = (struct HALMemNode *) (((TINT8 *) oldmem) + oldsize);		
	mnp = &mh->hmh_FreeList;

scan:

	mn = *mnp;
	if (mn == TNULL) goto notfound;
	if (mn < mend)
	{
		mnp = &mn->hmn_Next;
		goto scan;
	}
	
	if (newsize > oldsize)
	{
		/* grow allocation */
		if (mn == mend)
		{
			/* there is a free node at end */
			diffsize = newsize - oldsize;
			if (mn->hmn_Size == diffsize)
			{
				/* exact match: swallow free node */
				*mnp = mn->hmn_Next;
				mh->hmh_Free -= diffsize;
				return oldmem;
			}
			else if (mn->hmn_Size > diffsize)
			{
				/* free node is larger: move free node */
				mend = (struct HALMemNode *) (((TINT8 *) mend) + diffsize);
				*mnp = mend;
				mend->hmn_Next = mn->hmn_Next;
				mend->hmn_Size = mn->hmn_Size - diffsize;
				mh->hmh_Free -= diffsize;
				return oldmem;
			}
			/* else not enough space */
		}
		/* else no free node at end */
	}
	else
	{
		/* shrink allocation */
		diffsize = oldsize - newsize;
		if (mn == mend)
		{
			/* merge with following free node */
			mend = (struct HALMemNode *) (((TINT8 *) mend) - diffsize);
			*mnp = mend;
			mend->hmn_Next = mn->hmn_Next;
			mend->hmn_Size = mn->hmn_Size + diffsize;
		}
		else
		{
			/* add new free node */
			mend = (struct HALMemNode *) (((TINT8 *) mend) - diffsize);
			*mnp = mend;
			mend->hmn_Next = mn;
			mend->hmn_Size = diffsize;
		}
		mh->hmh_Free += diffsize;
		return oldmem;
	}

notfound:

	newmem = int_alloc(mh, newsize);
	if (newmem)
	{
		memcpy(newmem, oldmem, TMIN(oldsize, newsize));
		int_free(mh, oldmem, oldsize);
	}

	return newmem;
}

TVOID
boot_freevec(TAPTR boot, TAPTR mem)
{
	TUINT *p = mem;
	p--;
	boot_free(boot, p, *p);
}

static TAPTR
boot_allocvec(TAPTR boot, TUINT size)
{
	TUINT *mem;
	size += sizeof(TUINT);
	mem = boot_alloc(boot, size);
	if (mem) *mem++ = size;
	return mem;
}

/*****************************************************************************/
/*
**	lookup internal module
*/

static struct TInitModule *
lookupmodule(TSTRPTR modname, TTAGITEM *tags)
{
	struct TInitModule *imod = (TAPTR) TGetTag(tags, TExecBase_ModInit, TNULL);
	if (imod)
	{
		while (imod->tinm_Name)
		{
			if (!strcmp(imod->tinm_Name, modname))
			{
				return imod;
			}
			imod++;
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	host init/exit
*/

#define MEMALIGN	15

TAPTR 
boot_init(TTAGITEM *tags)
{
	TUINT membase = (TUINT) TGetTag(tags, TExecBase_MemBase, TNULL);
	TUINT memsize = (TUINT) TGetTag(tags, TExecBase_MemSize, 0);
	if (membase && memsize > (MEMALIGN + 1) * 2)
	{
		TUINT headsize;
		memsize -= membase - ((membase + MEMALIGN) & ~MEMALIGN);
		memsize &= ~MEMALIGN;
		membase = (membase + MEMALIGN) & ~MEMALIGN;
		headsize = (sizeof(struct HALMemHead) + MEMALIGN) & ~MEMALIGN;
		if (initPool((struct HALMemHead *) membase,
			(TAPTR) (membase + headsize), memsize - headsize, 0, MEMALIGN + 1))
		{
			struct HALPS2Boot *hpb = 
				int_alloc((TAPTR) membase, sizeof(struct HALPS2Boot));	
			if (hpb)
			{
				hpb->hpb_MemHead = (TAPTR) membase;
				hpb->hpb_Alloc = boot_alloc;
				hpb->hpb_Free = boot_free;
				hpb->hpb_Realloc = boot_realloc;				
				return hpb;
			}
		}
	}	
	return TNULL;
}

TVOID
boot_exit(TAPTR boot)
{
}

/*****************************************************************************/
/*
**	determine TEKlib global system directory
*/

TSTRPTR 
boot_getsysdir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR s = boot_allocvec(boot, 1);
	if (s) *s = 0;
	return s;
}

/*****************************************************************************/
/*
**	determine TEKlib global module directory
*/

TSTRPTR 
boot_getmoddir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR s = boot_allocvec(boot, 1);
	if (s) *s = 0;
	return s;
}

/*****************************************************************************/
/*
**	determine the path to the application, which will
**	later resolve to "PROGDIR:" in teklib semantics.
*/

TSTRPTR
boot_getprogdir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR s = boot_allocvec(boot, 1);
	if (s) *s = 0;
	return s;
}

/*****************************************************************************/

TAPTR 
boot_loadmodule(TAPTR boot, TSTRPTR progdir, TSTRPTR moddir, TSTRPTR modname,
	TTAGITEM *tags)
{
	struct TInitModule *imod;
	struct ModHandle *handle;

	handle = boot_alloc(boot, sizeof(struct ModHandle));
	if (!handle) return TNULL;

	imod = lookupmodule(modname, tags);
	if (imod)
	{
		handle->entry = imod->tinm_InitFunc;
		handle->type = TYPE_LIB;
		return handle;
	}
	
	return TNULL;
}

/*****************************************************************************/
/*
**	close module
*/

TVOID
boot_closemodule(TAPTR boot, TAPTR knmod)
{
	struct ModHandle *handle = knmod;
	boot_free(boot, handle, sizeof(struct ModHandle));
}

/*****************************************************************************/
/*
**	get module entry
*/

TAPTR
boot_getentry(TAPTR boot, TAPTR knmod, TSTRPTR name)
{
	struct ModHandle *handle = knmod;
	return handle->entry;
}

/*****************************************************************************/
/*
**	call module
*/

TUINT
boot_callmod(TAPTR boot, TAPTR ModBase, TAPTR entry, TAPTR task, TAPTR mod,
	TUINT16 version, TTAGITEM *tags)
{
	return (*((TMODINITFUNC) entry))(task, mod, version, tags);
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: host.c,v $
**	Revision 1.4  2005/09/18 11:27:22  tmueller
**	added authors
**	
**	Revision 1.3  2005/04/01 22:13:05  fschulze
**	Fixed lethal errors in boot_realloc that originated from copy'n'paste
**	
**	Revision 1.2  2005/04/01 18:36:22  tmueller
**	A boot-specific object handle is now passed to all boot functions and later
**	handed over to the HAL module, where it helps to abstract from HW resources
**	
**	Revision 1.1  2005/03/13 20:01:39  fschulze
**	added
**	
**
*/
