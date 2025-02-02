
/*
**	$Id: exec_memory.c,v 1.11 2005/09/13 02:41:58 tmueller Exp $
**	teklib/mods/exec/exec_memory.c - Exec memory management
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include "exec_mod.h"

/*****************************************************************************/
/*
**	mmu = exec_CreateMMU(exec, allocator, mmutype, tags)
**	Create a memory manager
*/

static TCALLBACK TVOID
destroymmu(struct TMemManager *mmu)
{
	if (mmu->tmm_DestroyFunc) (*mmu->tmm_DestroyFunc)(mmu);
	exec_Free(mmu->tmm_Handle.tmo_ModBase, mmu);
}

static TCALLBACK TVOID
destroymmu_and_allocator(struct TMemManager *mmu)
{
	if (mmu->tmm_DestroyFunc) (*mmu->tmm_DestroyFunc)(mmu);
	TDESTROY(mmu->tmm_Allocator);
	exec_Free(mmu->tmm_Handle.tmo_ModBase, mmu);
}

static TCALLBACK TVOID
destroymmu_and_free(struct TMemManager *mmu)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	if (mmu->tmm_DestroyFunc) (*mmu->tmm_DestroyFunc)(mmu);
	exec_Free(exec, mmu->tmm_Allocator);
	exec_Free(exec, mmu);
}

EXPORT struct TMemManager *
exec_CreateMMU(TEXECBASE *exec, TAPTR allocator, TUINT mmutype,
	struct TTagItem *tags)
{
	struct TMemManager *mmu = exec_AllocMMU(exec, TNULL,
		sizeof(struct TMemManager));
	if (mmu)
	{
		TDFUNC destructor = TNULL;
		TBOOL success = TTRUE;
		TAPTR staticmem = TNULL;

		if ((mmutype & TMMUT_Pooled) && allocator == TNULL)
		{
			/* supported:
			** create a MMU based on an internal pooled allocator */
			
			allocator = exec_CreatePool(exec, tags);
			destructor = (TDFUNC) destroymmu_and_allocator;
		}
		else if (mmutype & TMMUT_Static)
		{
			/*	supported:
			**	create a MMU based on a static memory block */

			TUINT blocksize = (TUINT) TGetTag(tags, TMem_StaticSize, 0);

			success = TFALSE;
			if (blocksize > sizeof(union TMemHead))
			{
				if (allocator == TNULL)
				{
					allocator = staticmem = exec_AllocMMU(exec, TNULL,
						blocksize);
					if (allocator)
					{
						destructor = (TDFUNC) destroymmu_and_free;
						success = TTRUE;
					}
				}
				else
				{
					destructor = (TDFUNC) destroymmu;
					success = TTRUE;
				}

				if (success)
				{
					TUINT flags = TGetTag(tags, TMem_LowFrag, TFALSE) ? 
						TMEMHF_LOWFRAG : TMEMHF_NONE;
					TAPTR block = (TAPTR) 
						(((union TMemHead *) allocator) + 1);
					blocksize -= sizeof(union TMemHead);
					success =
						exec_initmemhead(allocator, block, blocksize, flags,
							0);
					if (!success) tdbprintf(20,"static allocator failed\n");
				}
			}
		}
		else
		{
			destructor = (TDFUNC) destroymmu;
		}

		if (success)
		{	
			if (exec_initmmu(exec, mmu, allocator, mmutype, tags))
			{
				/* Overwrite destructor. The one provided by exec_initmmu()
				** doesn't know how to free the memory manager. */
	
				mmu->tmm_Handle.tmo_DestroyFunc = destructor;
				return mmu;
			}
		}

		exec_Free(exec, staticmem);
		exec_Free(exec, mmu);
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	exec_CopyMem(exec, from, to, size)
**	Copy memory
*/

EXPORT TVOID
exec_CopyMem(TEXECBASE *exec, TAPTR from, TAPTR to, TUINT numbytes)
{
	THALCopyMem(exec->texb_HALBase, from, to, numbytes);
}

/*****************************************************************************/
/*
**	exec_FillMem(exec, dest, numbytes, fillval)
**	Fill memory
*/

EXPORT TVOID
exec_FillMem(TEXECBASE *exec, TAPTR dest, TUINT len, TUINT8 fillval)
{
	if (((TUINTPTR) dest | len) & 3)
	{
		THALFillMem(exec->texb_HALBase, dest, len, fillval);
	}
	else
	{
		TUINT f32;
		f32 = fillval;
		f32 = (f32 << 8) | f32;
		f32 = (f32 << 16) | f32;
		exec_FillMem32(exec, dest, len, f32);
	}
}

/*****************************************************************************/
/*
**	exec_FillMem32(exec, dest, numbytes, fillval)
**	Fill memory, 32bit aligned
*/

EXPORT TVOID
exec_FillMem32(TEXECBASE *exec, TUINT *dest, TUINT len, TUINT fill)
{
	TUINT len8;
	len >>= 2;
	len8 = (len >> 3) + 1;
	
	switch (len & 7)
	{
		do
		{
					*dest++ = fill;
			case 7:	*dest++ = fill;
			case 6:	*dest++ = fill;
			case 5:	*dest++ = fill;
			case 4:	*dest++ = fill;
			case 3:	*dest++ = fill;
			case 2:	*dest++ = fill;
			case 1:	*dest++ = fill;
			case 0:	len8--;
	
		} while (len8);
	}
}

/*****************************************************************************/
/*
**	mem = exec_AllocMMU(exec, mmu, size)
**	Allocate memory via memory manager
*/

EXPORT TAPTR
exec_AllocMMU(TEXECBASE *exec, struct TMemManager *mmu, TUINT size)
{
	union TMMUInfo *mem = TNULL;
	if (size)
	{
		if (mmu == TNULL) mmu = &exec->texb_BaseMMU;
	
		mem = (*mmu->tmm_Alloc)(mmu, size + sizeof(union TMMUInfo));

		if (mem)
		{
			mem->tmu_Node.tmu_UserSize = size;
			mem->tmu_Node.tmu_MMU = mmu;
			mem++;
		}
		else tdbprintf1(5, "alloc failed size %d\n", size);
	}
	else tdbprintf(5, "called with size=0\n");
	
	return (TAPTR) mem;
}

/*****************************************************************************/
/*
**	mem = exec_AllocMMU0(exec, mmu, size)
**	Allocate memory via memory manager, zero'ed out
*/

EXPORT TAPTR
exec_AllocMMU0(TEXECBASE *exec, struct TMemManager *mmu, TUINT size)
{
	TAPTR mem = exec_AllocMMU(exec, mmu, size);
	if (mem) exec_FillMem(exec, mem, size, 0);
	return mem;
}

/*****************************************************************************/
/*
**	mem = exec_Realloc(exec, mem, newsize)
**	Reallocate an existing memory manager allocation. Note that it is not
**	possible to allocate a fresh block of memory with this function.
*/

EXPORT TAPTR
exec_Realloc(TEXECBASE *exec, TAPTR mem, TUINT newsize)
{
	union TMMUInfo *newmem = TNULL;
	if (mem)
	{
		union TMMUInfo *mmuinfo = (union TMMUInfo *) mem - 1;
		struct TMemManager *mmu = mmuinfo->tmu_Node.tmu_MMU;

		if (mmu == TNULL) mmu = &exec->texb_BaseMMU;

		if (newsize)
		{
			TUINT oldsize = mmuinfo->tmu_Node.tmu_UserSize;
			if (oldsize == newsize) return mem;
			newmem = (*mmu->tmm_Realloc)(mmu, (TINT8 *) mmuinfo, 
				oldsize + sizeof(union TMMUInfo), 
				newsize + sizeof(union TMMUInfo));

			if (newmem)
			{
				newmem->tmu_Node.tmu_UserSize = newsize;
				newmem++;
			}
		}
		else
		{
			(*mmu->tmm_Free)(mmu, (TINT8 *) mmuinfo, 
				mmuinfo->tmu_Node.tmu_UserSize + sizeof(union TMMUInfo));
		}
	}
	return newmem;
}

/*****************************************************************************/
/*
**	exec_Free(exec, mem)
**	Return memory allocated from a memory manager
*/

EXPORT TVOID
exec_Free(TEXECBASE *exec, TAPTR mem)
{
	if (mem)
	{
		union TMMUInfo *mmuinfo = (union TMMUInfo *) mem - 1;
		struct TMemManager *mmu = mmuinfo->tmu_Node.tmu_MMU;
		if (mmu == TNULL) mmu = &exec->texb_BaseMMU;
		(*mmu->tmm_Free)(mmu, (TINT8 *) mmuinfo, 
			mmuinfo->tmu_Node.tmu_UserSize + sizeof(union TMMUInfo));
	}
}

/*****************************************************************************/
/*
**	success = exec_initmemhead(mh, mem, size, flags, bytealign)
**	Init memheader
*/

LOCAL TBOOL
exec_initmemhead(union TMemHead *mh, TAPTR mem, TUINT size, TUINT flags,
	TINT bytealign)
{
	TUINT align = sizeof(TAPTR);
	bytealign = TMAX(bytealign, sizeof(union TMemNode));
	while (align < bytealign) align <<= 1;
	if (size >= align)
	{
		--align;
		size &= ~align;

		mh->tmh_Node.tmh_Mem = mem;
		mh->tmh_Node.tmh_MemEnd = ((TINT8 *) mem) + size;
		mh->tmh_Node.tmh_Free = size;
		mh->tmh_Node.tmh_Align = align;
		mh->tmh_Node.tmh_Flags = flags;

		mh->tmh_Node.tmh_FreeList = (union TMemNode *) mem;
		((union TMemNode *) mem)->tmn_Node.tmn_Next = TNULL;
		((union TMemNode *) mem)->tmn_Node.tmn_Size = size;

		return TTRUE;
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	mem = exec_staticalloc(memhead, size)
**	Allocate from a static memheader
*/

LOCAL TAPTR
exec_staticalloc(union TMemHead *mh, TUINT size)
{
	size = (size + mh->tmh_Node.tmh_Align) & ~mh->tmh_Node.tmh_Align;

	if (mh->tmh_Node.tmh_Free >= size)
	{
		union TMemNode **mnp, *mn, **x;
	
		mnp = &mh->tmh_Node.tmh_FreeList;
		x = TNULL;

		if (mh->tmh_Node.tmh_Flags & TMEMHF_LOWFRAG)
		{
			/* bestfit strategy */

			TUINT bestsize = 0xffffffff;
			while ((mn = *mnp))
			{
				if (mn->tmn_Node.tmn_Size == size)
				{
exactfit:			*mnp = mn->tmn_Node.tmn_Next;
					mh->tmh_Node.tmh_Free -= size;
					return (TAPTR) mn;
				}
				else if (mn->tmn_Node.tmn_Size > size)
				{
					if (mn->tmn_Node.tmn_Size < bestsize)
					{
						bestsize = mn->tmn_Node.tmn_Size;
						x = mnp;
					}
				}
				mnp = &mn->tmn_Node.tmn_Next;
			}
		}
		else
		{
			/* firstfit strategy */

			while ((mn = *mnp))
			{
				if (mn->tmn_Node.tmn_Size == size)
				{
					goto exactfit;
				}
				else if (mn->tmn_Node.tmn_Size > size)
				{
					x = mnp;
					break;
				}
				mnp = &mn->tmn_Node.tmn_Next;
			}
		}
		
		if (x)
		{
			mn = *x;
			*x = (union TMemNode *) ((TINT8 *) mn + size);
			(*x)->tmn_Node.tmn_Next = mn->tmn_Node.tmn_Next;
			(*x)->tmn_Node.tmn_Size = mn->tmn_Node.tmn_Size - size; 
			mh->tmh_Node.tmh_Free -= size;
			return (TAPTR) mn;
		}
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	exec_staticfree(mh, mem, size)
**	Return memory to a static allocator
*/

LOCAL TVOID
exec_staticfree(union TMemHead *mh, TAPTR mem, TUINT size)
{
	union TMemNode **mnp, *mn, *pmn;
	size = (size + mh->tmh_Node.tmh_Align) & ~mh->tmh_Node.tmh_Align;

	mh->tmh_Node.tmh_Free += size;
	mnp = &mh->tmh_Node.tmh_FreeList;
	pmn = TNULL;
	
	while ((mn = *mnp))
	{
		if ((TINT8 *) mem < (TINT8 *) mn) break;
		pmn = mn;
		mnp = &mn->tmn_Node.tmn_Next;
	}

	if (mn && ((TINT8 *) mem + size == (TINT8 *) mn))		
	{
		size += mn->tmn_Node.tmn_Size;
		mn = mn->tmn_Node.tmn_Next;		/* concatenate with following free node */
	}
	
	if (pmn && ((TINT8 *) pmn + pmn->tmn_Node.tmn_Size == (TINT8 *) mem))
	{
		size += pmn->tmn_Node.tmn_Size;	/* concatenate with previous free node */
		mem = pmn;
	}
	
	*mnp = (union TMemNode *) mem;
	((union TMemNode *) mem)->tmn_Node.tmn_Next = mn;
	((union TMemNode *) mem)->tmn_Node.tmn_Size = size;
}

/*****************************************************************************/
/*
**	newmem = exec_staticrealloc(hal, mh, oldmem, oldsize, newsize)
**	Realloc static memheader allocation
*/

LOCAL TAPTR
exec_staticrealloc(TAPTR hal, union TMemHead *mh,  TAPTR oldmem,
	TUINT oldsize, TUINT newsize)
{
	TAPTR newmem;
	union TMemNode **mnp, *mn, *mend;
	TINT diffsize;

	oldsize = (oldsize + mh->tmh_Node.tmh_Align) & ~mh->tmh_Node.tmh_Align;
	newsize = (newsize + mh->tmh_Node.tmh_Align) & ~mh->tmh_Node.tmh_Align;

	if (newsize == oldsize) return oldmem;

	/* end of old allocation */
	mend = (union TMemNode *) (((TINT8 *) oldmem) + oldsize);		
	mnp = &mh->tmh_Node.tmh_FreeList;

scan:	

	mn = *mnp;
	if (mn == TNULL) goto notfound;
	if (mn < mend)
	{
		mnp = &mn->tmn_Node.tmn_Next;
		goto scan;
	}
	
	if (newsize > oldsize)
	{
		/* grow allocation */
		if (mn == mend)
		{
			/* there is a free node at end */
			diffsize = newsize - oldsize;
			if (mn->tmn_Node.tmn_Size == diffsize)
			{
				/* exact match: swallow free node */
				*mnp = mn->tmn_Node.tmn_Next;
				mh->tmh_Node.tmh_Free -= diffsize;
				return oldmem;
			}
			else if (mn->tmn_Node.tmn_Size > diffsize)
			{
				/* free node is larger: move free node */
				mend = (union TMemNode *) (((TINT8 *) mend) + diffsize);
				*mnp = mend;
				mend->tmn_Node.tmn_Next = mn->tmn_Node.tmn_Next;
				mend->tmn_Node.tmn_Size = mn->tmn_Node.tmn_Size - diffsize;
				mh->tmh_Node.tmh_Free -= diffsize;
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
			mend = (union TMemNode *) (((TINT8 *) mend) - diffsize);
			*mnp = mend;
			mend->tmn_Node.tmn_Next = mn->tmn_Node.tmn_Next;
			mend->tmn_Node.tmn_Size = mn->tmn_Node.tmn_Size + diffsize;
		}
		else
		{
			/* add new free node */
			mend = (union TMemNode *) (((TINT8 *) mend) - diffsize);
			*mnp = mend;
			mend->tmn_Node.tmn_Next = mn;
			mend->tmn_Node.tmn_Size = diffsize;
		}
		mh->tmh_Node.tmh_Free += diffsize;
		return oldmem;
	}

notfound:

	newmem = exec_staticalloc(mh, newsize);
	if (newmem)
	{
		THALCopyMem(hal, oldmem, newmem, TMIN(oldsize, newsize));
		exec_staticfree(mh, oldmem, oldsize);
	}
	
	return newmem;
}

/*****************************************************************************/
/*
**	size = exec_GetSize(exec, allocation)
**	Get size of an allocation made from a memory manager
*/

EXPORT TUINT
exec_GetSize(TEXECBASE *exec, TAPTR mem)
{
	TUINT size = 0;
	if (mem) size = ((union TMMUInfo *) mem - 1)->tmu_Node.tmu_UserSize;
	return size;
}

/*****************************************************************************/
/*
**	mmu = exec_GetMMU(exec, allocation)
**	Get the memory manager an allocation was made from
*/

EXPORT TAPTR
exec_GetMMU(TEXECBASE *exec, TAPTR mem)
{
	TAPTR mmu = TNULL;
	if (mem) mmu = ((union TMMUInfo *) mem - 1)->tmu_Node.tmu_MMU;
	return mmu;
}

/*****************************************************************************/
/*
**	pool = exec_CreatePool(exec, tags)
**	Create memory pool
*/

static TCALLBACK TVOID
destroypool(struct TMemPool *pool)
{
	TEXECBASE *exec = TGetExecBase(pool);
	union TMemHead *node = (union TMemHead *) pool->tpl_List.tlh_Head;
	struct TNode *nnode;
	if (pool->tpl_Flags & TMEMHF_FREE)
	{
		while ((nnode = ((struct TNode *) node)->tln_Succ))
		{
			exec_Free(exec, node);
			node = (union TMemHead *) nnode;
		}
	}
	exec_Free(exec, pool);
}

EXPORT TAPTR
exec_CreatePool(TEXECBASE *exec, struct TTagItem *tags)
{
	TUINT fixedsize = TGetTag(tags, TPool_StaticSize, 0);
	if (fixedsize)
	{
		struct TMemPool *pool = exec_AllocMMU(exec, TNULL,
			sizeof(struct TMemPool));
		if (pool)
		{
			union TMemHead *fixed = (TAPTR) TGetTag(tags, TPool_Static, TNULL);
			pool->tpl_Flags = TMEMHF_FIXED;
			if (fixed == TNULL)
			{
				TAPTR mmu = (TAPTR) TGetTag(tags, TPool_MMU, TNULL);
				fixed = exec_AllocMMU(exec, mmu, fixedsize);
				pool->tpl_Flags |= TMEMHF_FREE;
			}
			
			if (fixed)
			{
				struct TNode *tempn;
	
				pool->tpl_Align = sizeof(union TMemNode) - 1;
				pool->tpl_Flags |= ((TBOOL) TGetTag(tags, TMem_LowFrag, 
					(TTAG) TFALSE)) ? TMEMHF_LOWFRAG : TMEMHF_NONE;
			
				pool->tpl_Handle.tmo_ModBase = exec;
				pool->tpl_Handle.tmo_DestroyFunc = (TDFUNC) destroypool;
		
				TINITLIST(&pool->tpl_List);
				exec_initmemhead(fixed, fixed + 1,
					fixedsize - sizeof(union TMemHead),
					pool->tpl_Flags, pool->tpl_Align + 1);
				TADDHEAD(&pool->tpl_List, (struct TNode *) fixed, tempn);
			}
			else
			{
				exec_Free(exec, pool);
				pool = TNULL;
			}
		}
		return pool;
	}
	else
	{
		TUINT pudsize = (TUINT) TGetTag(tags, TPool_PudSize, (TTAG) 1024);
		TUINT thressize = (TUINT) TGetTag(tags, TPool_ThresSize, (TTAG) 256);
		if (pudsize >= thressize)
		{
			struct TMemPool *pool = exec_AllocMMU(exec, TNULL,
				sizeof(struct TMemPool));
			if (pool)
			{
				pool->tpl_Align = sizeof(union TMemNode) - 1;
				pool->tpl_PudSize = pudsize;
				pool->tpl_ThresSize = thressize;
	
				pool->tpl_Flags = TMEMHF_FREE;
				pool->tpl_Flags |= ((TBOOL) TGetTag(tags,
					TPool_AutoAdapt, (TTAG) TTRUE)) ? TMEMHF_AUTO : TMEMHF_NONE;
				pool->tpl_Flags |= ((TBOOL) TGetTag(tags, TMem_LowFrag, 
					(TTAG) TFALSE)) ? TMEMHF_LOWFRAG : TMEMHF_NONE;
		
				pool->tpl_Handle.tmo_ModBase = exec;
				pool->tpl_Handle.tmo_DestroyFunc = (TDFUNC) destroypool;
				pool->tpl_MMU = (TAPTR) TGetTag(tags, TPool_MMU, TNULL);
	
				TINITLIST(&pool->tpl_List);
			}
			return pool;
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	mem = exec_AllocPool(exec, pool, size)
**	Alloc memory from a pool
*/

EXPORT TAPTR
exec_AllocPool(TEXECBASE *exec, struct TMemPool *pool, TUINT size)
{
	if (size && pool)
	{
		union TMemHead *node;
		
		if (pool->tpl_Flags & TMEMHF_FIXED)
		{
			node = (union TMemHead *) pool->tpl_List.tlh_Head;
			return exec_staticalloc(node, size);
		}
		
		if (pool->tpl_Flags & TMEMHF_AUTO)
		{
			/* auto-adapt pool parameters */
		
			TINT p, t;
			
			t = pool->tpl_ThresSize;
			/* tend to 4x typical size */ 
			t += (((TINT)size) - (t >> 2)) >> 2;
			p = pool->tpl_PudSize;
			/* tend to 8x thressize */
			p += (t - (p >> 3)) >> 3;
			if (t > p) p = t;
			
			pool->tpl_ThresSize = t;
			pool->tpl_PudSize = p;

			/*tdbprintf3(20,"size: %d - puds: %d - thress: %d\n", 
				size, pool->pudsize, pool->thressize);*/
		}

		if (size <= ((pool->tpl_ThresSize + pool->tpl_Align) & 
			~pool->tpl_Align))
		{
			/* regular pool allocation */

			TAPTR mem;
			struct TNode *tempnode;
			TUINT pudsize;

			node = (union TMemHead *) pool->tpl_List.tlh_Head;
			while ((tempnode = ((struct TNode *) node)->tln_Succ))
			{
				if (node->tmh_Node.tmh_Flags & TMEMHF_LARGE) break;
				mem = exec_staticalloc(node, size);
				if (mem) return mem;
				node = (union TMemHead *) tempnode;
			}		

			pudsize = (pool->tpl_PudSize + pool->tpl_Align) & ~pool->tpl_Align;
			node = exec_AllocMMU(exec, pool->tpl_MMU,
				sizeof(union TMemHead) + pudsize);
			if (node)
			{
				struct TNode *tempn;
				exec_initmemhead(node, node + 1, pudsize, pool->tpl_Flags,
					pool->tpl_Align + 1);
				TADDHEAD(&pool->tpl_List, (struct TNode *) node, tempn);
				return exec_staticalloc(node, size);
			}
		}
		else
		{
			/* large allocation */

			size = (size + pool->tpl_Align) & ~pool->tpl_Align;
			node = exec_AllocMMU(exec, pool->tpl_MMU,
				sizeof(union TMemHead) + size);
			if (node)
			{
				struct TNode *tempn;
				exec_initmemhead(node, node + 1, size,
					pool->tpl_Flags | TMEMHF_LARGE, pool->tpl_Align + 1);
				TADDTAIL(&pool->tpl_List, (struct TNode *) node, tempn);
				return exec_staticalloc(node, size);
			}
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	exec_FreePool(exec, pool, mem, size)
**	Return memory to a pool
*/

EXPORT TVOID
exec_FreePool(TEXECBASE *exec, struct TMemPool *pool, TINT8 *mem, TUINT size)
{
	if ((mem == TNULL) != (size == 0))
		exec_panic(exec, "TExecFreePool(): Invalid allocation and size");

	if (mem)
	{
		union TMemHead *node = (union TMemHead *) pool->tpl_List.tlh_Head;
		if (pool->tpl_Flags & TMEMHF_FIXED)
		{
			exec_staticfree(node, mem, size);
			return;
		}
		else
		{
			struct TNode *temp;
			TINT8 *memend = mem + size;
			while ((temp = ((struct TNode *) node)->tln_Succ))
			{
				if (mem >= node->tmh_Node.tmh_Mem && memend <= node->tmh_Node.tmh_MemEnd)
				{
					exec_staticfree(node, mem, size);
					if (node->tmh_Node.tmh_Free == node->tmh_Node.tmh_MemEnd - node->tmh_Node.tmh_Mem)
					{
						/* flush puddle */
						TREMOVE((struct TNode *) node);
						exec_Free(exec, node);
					}
					else
					{
						/* recently used node moves up */
						TNodeUp((struct TNode *) node);
					}
					return;
				}
				node = (union TMemHead *) temp;
			}
			
			exec_panic(exec, "TExecFreePool(): Allocation not pool member");
		}
	}
}

/*****************************************************************************/
/*
**	newmem = exec_ReallocPool(exec, pool, oldmem, oldsize, newsize)
**	Realloc pool allocation
*/

EXPORT TAPTR 
exec_ReallocPool(TEXECBASE *exec, struct TMemPool *pool, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	if (oldmem && oldsize)
	{
		struct TNode *tempnode;
		union TMemHead *node;
		TINT8 *memend;

		if (newsize == 0)
		{
			exec_FreePool(exec, pool, oldmem, oldsize);
			return TNULL;
		}

		node = (union TMemHead *) pool->tpl_List.tlh_Head;

		if (pool->tpl_Flags & TMEMHF_FIXED)
		{
			return exec_staticrealloc(exec->texb_HALBase, node, oldmem,
				oldsize, newsize);
		}

		memend = oldmem + oldsize;

		while ((tempnode = ((struct TNode *) node)->tln_Succ))
		{
			if (oldmem >= node->tmh_Node.tmh_Mem && memend <= node->tmh_Node.tmh_MemEnd)
			{
				TAPTR newmem;
	
				newmem = exec_staticrealloc(exec->texb_HALBase, node, oldmem,
					oldsize, newsize);
				if (newmem)
				{
					if (node->tmh_Node.tmh_Flags & TMEMHF_LARGE)
					{
						struct TNode *tempn;
						/* this is no longer a large node */
						TREMOVE((struct TNode *) node);
						TADDHEAD(&pool->tpl_List, (struct TNode *) node,
							tempn);
						node->tmh_Node.tmh_Flags &= ~TMEMHF_LARGE;
					}
					return newmem;
				}
	
				newmem = exec_AllocPool(exec, pool, newsize);
				if (newmem)
				{
					THALCopyMem(exec->texb_HALBase, oldmem, newmem,
						TMIN(oldsize, newsize));
					exec_staticfree(node, oldmem, oldsize);
					if (node->tmh_Node.tmh_Free == node->tmh_Node.tmh_MemEnd - node->tmh_Node.tmh_Mem)
					{
						/* flush puddle */
						TREMOVE((struct TNode *) node);
						exec_Free(exec, node);
					}
					else
					{
						/* recently used node moves up */
						TNodeUp((struct TNode *) node);
					}
				}
				return newmem;
			}
			node = (union TMemHead *) tempnode;
		}
	}
	else if ((oldmem == TNULL) && (oldsize == 0))
	{
		return exec_AllocPool(exec, pool, newsize);
	}

	exec_panic(exec, "TExecReallocPool(): Invalid allocation and size");
	return TNULL;
}

/*****************************************************************************/
/*
**	debug allocator (tasksafe, tracking)
**
**	        |-deadsize-|                     |-deadsize-|
**	                   |-------- size -------|
**	 _______ __________ _________ ___________ __________
**	| TNODE | ABADCAFE | MMUInfo | userspace | DEADBEEF |
**	^                  ^
**	|                  this is returned, therefore we cannot 
**	|                  catch violations of the MMUInfo header.
**	track header
**
*/

#define DEADSIZE	256		/* in 32bit words */

static TBOOL 
mmu_dbcheck(TINT8 *mem, TUINT size)
{
	TBOOL clean = TTRUE;
	TINT i;
	TUINT *p;

	p = (TUINT *) (mem - DEADSIZE);
	for (i = 0; i < DEADSIZE>>2; ++i)
	{
		if (*p++ != 0xABADCAFE)
		{
			tdbprintf3(20,"memory before %08x, size %08x, offset -%08x hit\n",
				(TUINT)(TUINTPTR) mem, size, DEADSIZE - (i<<2));
			clean = TFALSE;
			break;
		}
	}	

	p = (TUINT *) (mem + size);
	for (i = 0; i < DEADSIZE>>2; ++i)
	{
		if (*p++ != 0xDEADBEEF)
		{
			tdbprintf3(20,"memory behind %08x, size %08x, offset %08x hit\n", 
				(TUINT)(TUINTPTR) mem, size, (i<<2));
			clean = TFALSE;
			break;
		}
	}
	return clean;
}

static TCALLBACK TAPTR
mmu_dballoc(struct TMemManager *mmu, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *mem;
	
	size = (size + 3) & ~3;

	exec_Lock(exec, &mmu->tmm_Lock);
	mem = exec_AllocMMU(exec, mmu->tmm_Allocator, size + DEADSIZE * 2 + 
		sizeof(TNODE));
	if (mem)
	{
		TUINT *p, i;
		struct TNode *tempn;

		p = (TUINT *) (mem + sizeof(TNODE));
		for (i = 0; i < (DEADSIZE>>2); ++i) *p++ = 0xABADCAFE;

		p = (TUINT *) (mem + sizeof(TNODE) + DEADSIZE);
		for (i = 0; i < (DEADSIZE>>2); ++i)
		{
			*p = 0xaac65539 + (TUINTPTR) p;
			p++;
		}

		p = (TUINT *) (mem + sizeof(TNODE) + DEADSIZE + size);
		for (i = 0; i < (DEADSIZE>>2); ++i) *p++ = 0xDEADBEEF;
		
		TADDHEAD(&mmu->tmm_TrackList, (TNODE *) mem, tempn);
		mem += sizeof(TNODE) + DEADSIZE;
	}
	exec_Unlock(exec, &mmu->tmm_Lock);
	return mem;
}

static TCALLBACK TAPTR 
mmu_dbrealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *newmem;

	oldsize = (oldsize + 3) & ~3;
	newsize = (newsize + 3) & ~3;

	mmu_dbcheck(oldmem, oldsize);

	exec_Lock(exec, &mmu->tmm_Lock);

	TREMOVE((TNODE *) (oldmem - DEADSIZE - sizeof(TNODE)));

	newmem = exec_Realloc(exec, oldmem - DEADSIZE - sizeof(TNODE), 
		newsize + DEADSIZE + DEADSIZE + sizeof(TNODE));

	if (newmem)
	{
		TUINT *p, i;
		struct TNode *tempn;
		
		if (newsize > oldsize)
		{
			p = (TUINT *) (newmem + sizeof(TNODE) + oldsize);
			for (i = 0; i < (newsize - oldsize) >> 2; ++i) *p++ = 0x55555555;
		}

		p = (TUINT *) (newmem + sizeof(TNODE));
		for (i = 0; i < (DEADSIZE>>2); ++i) *p++ = 0xABADCAFE;

		p = (TUINT *) (newmem + sizeof(TNODE) + DEADSIZE + newsize);
		for (i = 0; i < (DEADSIZE>>2); ++i) *p++ = 0xDEADBEEF;
	
		TADDHEAD(&mmu->tmm_TrackList, (TNODE *) newmem, tempn);
		newmem += sizeof(TNODE) + DEADSIZE;
	}

	exec_Unlock(exec, &mmu->tmm_Lock);
	return newmem;
}	

static TCALLBACK TVOID 
mmu_dbfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	size = (size + 3) & ~3;
	mmu_dbcheck(mem, size);
	exec_Lock(exec, &mmu->tmm_Lock);
	TREMOVE((TNODE *) (mem - DEADSIZE - sizeof(TNODE)));
	exec_Free(exec, mem - DEADSIZE - sizeof(TNODE));
	exec_Unlock(exec, &mmu->tmm_Lock);
}

static TCALLBACK TVOID
mmu_dbdestroy(struct TMemManager *mmu)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TNODE *nextnode, *node = mmu->tmm_TrackList.tlh_Head;
	TINT numfreed = 0;
	TUINT size;

	while ((nextnode = node->tln_Succ))
	{
		TINT8 *inner = ((TINT8 *) (node + 1)) + DEADSIZE;
		size = ((union TMMUInfo *) inner)->tmu_Node.tmu_UserSize;
		size = (size + 3) & ~3;
		size += sizeof(union TMMUInfo);
		mmu_dbcheck(inner, size);
		exec_Free(exec, node);
		numfreed++;
		node = nextnode;
	}
	TDESTROY(&mmu->tmm_Lock);

	if (numfreed)
		tdbprintf1(10, "freed %d pending allocations\n",
			numfreed);
}

/*****************************************************************************/
/*
**	TNULL message allocator
**	Note that this kind of allocator is using execbase->texb_Lock, not
**	the lock supplied with the MMU. There may be no self-context available
**	when a message MMU is used to allocate the initial task structures
**	during the Exec setup.
*/

static TCALLBACK TAPTR 
mmu_knmsgalloc(struct TMemManager *mmu, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TAPTR hal = exec->texb_HALBase;
	TINT8 *mem;
	THALLock(hal, &exec->texb_Lock);
	mem = THALAlloc(hal, size + sizeof(TNODE) + sizeof(struct TMessage));
	if (mem)
	{
		struct TNode *tempn;
		TADDTAIL(&mmu->tmm_TrackList, (TNODE *) mem, tempn);
		THALUnlock(hal, &exec->texb_Lock);
		((struct TMessage *) (mem + sizeof(TNODE)))->tmsg_Flags =
			TMSG_STATUS_FAILED;
		return (TAPTR) (mem + sizeof(TNODE) + sizeof(struct TMessage));
	}
	THALUnlock(hal, &exec->texb_Lock);
	return TNULL;
}

static TCALLBACK TVOID 
mmu_knmsgfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TAPTR hal = exec->texb_HALBase;
	THALLock(hal, &exec->texb_Lock);
	TREMOVE((TNODE *) (mem - sizeof(TNODE) - sizeof(struct TMessage)));
	THALFree(hal, mem - sizeof(TNODE) - sizeof(struct TMessage),
		size + sizeof(TNODE) + sizeof(struct TMessage));
	THALUnlock(hal, &exec->texb_Lock);
}

static TCALLBACK TVOID 
mmu_knmsgdestroy(struct TMemManager *mmu)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TAPTR hal = exec->texb_HALBase;
	TNODE *nextnode, *node = mmu->tmm_TrackList.tlh_Head;
	TINT numfreed = 0;
	while ((nextnode = node->tln_Succ))
	{
		THALFree(hal, node, ((union TMMUInfo *) ((TINT8 *) (node + 1) + 
			sizeof(struct TMessage)))->tmu_Node.tmu_UserSize + sizeof(TNODE) +
			sizeof(struct TMessage) + sizeof(union TMMUInfo));
		numfreed++;
		node = nextnode;
	}
	if (numfreed)
		tdbprintf1(10, "freed %d pending allocations\n",
			numfreed);
}

/*****************************************************************************/
/*
**	static memheader allocator
*/

static TCALLBACK TAPTR
mmu_staticalloc(struct TMemManager *mmu, TUINT size)
{
	return exec_staticalloc(mmu->tmm_Allocator, size);
}

static TCALLBACK TVOID
mmu_staticfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	exec_staticfree(mmu->tmm_Allocator, mem, size);
}

static TCALLBACK TAPTR 
mmu_staticrealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	return exec_staticrealloc(exec->texb_HALBase, mmu->tmm_Allocator, oldmem,
		oldsize, newsize);
}	

/*****************************************************************************/
/*
**	pooled allocator
*/

static TCALLBACK TAPTR
mmu_poolalloc(struct TMemManager *mmu, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	return exec_AllocPool(exec, mmu->tmm_Allocator, size);
}

static TCALLBACK TVOID
mmu_poolfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	exec_FreePool(exec, mmu->tmm_Allocator, mem, size);
}

static TCALLBACK TAPTR 
mmu_poolrealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	return exec_ReallocPool(exec, mmu->tmm_Allocator, oldmem,
		oldsize, newsize);
}	

/*****************************************************************************/
/*
**	pooled+tasksafe allocator
*/

static TCALLBACK TAPTR
mmu_pooltaskalloc(struct TMemManager *mmu, TUINT size)
{	
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *mem;
	exec_Lock(exec, &mmu->tmm_Lock);
	mem = exec_AllocPool(exec, mmu->tmm_Allocator, size);
	exec_Unlock(exec, &mmu->tmm_Lock);
	return mem;
}

static TCALLBACK TVOID
mmu_pooltaskfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	exec_Lock(exec, &mmu->tmm_Lock);	
	exec_FreePool(exec, mmu->tmm_Allocator, mem, size);
	exec_Unlock(exec, &mmu->tmm_Lock);
}

static TCALLBACK TAPTR
mmu_pooltaskrealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *newmem;
	exec_Lock(exec, &mmu->tmm_Lock);
	newmem = exec_ReallocPool(exec, mmu->tmm_Allocator, oldmem, oldsize,
		newsize);
	exec_Unlock(exec, &mmu->tmm_Lock);
	return newmem;
}	

static TCALLBACK TVOID 
mmu_pooltaskdestroy(struct TMemManager *mmu)
{
	TDESTROY(&mmu->tmm_Lock);
}

/*****************************************************************************/
/*
**	static memheader allocator, task-safe
*/

static TCALLBACK TAPTR
mmu_statictaskalloc(struct TMemManager *mmu, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *mem;	
	exec_Lock(exec, &mmu->tmm_Lock);
	mem = exec_staticalloc(mmu->tmm_Allocator, size);
	exec_Unlock(exec, &mmu->tmm_Lock);
	return mem;
}

static TCALLBACK TVOID
mmu_statictaskfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	exec_Lock(exec, &mmu->tmm_Lock);
	exec_staticfree(mmu->tmm_Allocator, mem, size);
	exec_Unlock(exec, &mmu->tmm_Lock);
}

static TCALLBACK TAPTR
mmu_statictaskrealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *newmem;
	exec_Lock(exec, &mmu->tmm_Lock);
	newmem = exec_staticrealloc(exec->texb_HALBase, mmu->tmm_Allocator,
		oldmem, oldsize, newsize);
	exec_Unlock(exec, &mmu->tmm_Lock);
	return newmem;
}	

static TCALLBACK TVOID
mmu_statictaskdestroy(struct TMemManager *mmu)
{
	TDESTROY(&mmu->tmm_Lock);
}

/*****************************************************************************/
/*
**	tasksafe MMU allocator
*/

static TCALLBACK TAPTR
mmu_taskalloc(struct TMemManager *mmu, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *mem;
	exec_Lock(exec, &mmu->tmm_Lock);
	mem = exec_AllocMMU(exec, mmu->tmm_Allocator, size);
	exec_Unlock(exec, &mmu->tmm_Lock);
	return mem;
}

static TCALLBACK TAPTR
mmu_taskrealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *newmem;
	exec_Lock(exec, &mmu->tmm_Lock);
	newmem = exec_Realloc(exec, oldmem, newsize);
	exec_Unlock(exec, &mmu->tmm_Lock);
	return newmem;
}	

static TCALLBACK TVOID
mmu_taskfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	exec_Lock(exec, &mmu->tmm_Lock);
	exec_Free(exec, mem);
	exec_Unlock(exec, &mmu->tmm_Lock);
}

static TCALLBACK TVOID
mmu_taskdestroy(struct TMemManager *mmu)
{
	TDESTROY(&mmu->tmm_Lock);
}

/*****************************************************************************/
/*
**	tracking MMU allocator
*/

static TCALLBACK TAPTR 
mmu_trackalloc(struct TMemManager *mmu, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *mem = exec_AllocMMU(exec, mmu->tmm_Allocator, size + sizeof(TNODE));
	if (mem)
	{
		struct TNode *tempn;
		TADDHEAD(&mmu->tmm_TrackList, (TNODE *) mem, tempn);
		return (TAPTR) (mem + sizeof(TNODE));
	}
	return TNULL;
}

static TCALLBACK TAPTR 
mmu_trackrealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *newmem;
	TREMOVE((TNODE *) (oldmem - sizeof(TNODE)));
	newmem = exec_Realloc(exec, oldmem - sizeof(TNODE),
		newsize + sizeof(TNODE));
	if (newmem)
	{
		struct TNode *tempn;
		TADDHEAD(&mmu->tmm_TrackList, (TNODE *) newmem, tempn);
		newmem += sizeof(TNODE);
	}
	return (TAPTR) newmem;
}	

static TCALLBACK TVOID
mmu_trackfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TREMOVE((TNODE *) (mem - sizeof(TNODE)));
	exec_Free(exec, mem - sizeof(TNODE));
}

static TCALLBACK
TVOID mmu_trackdestroy(struct TMemManager *mmu)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TNODE *nextnode, *node = mmu->tmm_TrackList.tlh_Head;
	TINT numfreed = 0;
	while ((nextnode = node->tln_Succ))
	{
		exec_Free(exec, node);
		numfreed++;
		node = nextnode;
	}
	if (numfreed)
		tdbprintf1(10, "freed %d pending allocations\n",
			numfreed);
}

/*****************************************************************************/
/*
**	tasksafe+tracking MMU allocator
*/

static TCALLBACK TAPTR 
mmu_tasktrackalloc(struct TMemManager *mmu, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *mem;
	exec_Lock(exec, &mmu->tmm_Lock);
	mem = exec_AllocMMU(exec, mmu->tmm_Allocator, size + sizeof(TNODE));
	if (mem)
	{
		struct TNode *tempn;
		TADDHEAD(&mmu->tmm_TrackList, (TNODE *) mem, tempn);
		mem += sizeof(TNODE);
	}
	exec_Unlock(exec, &mmu->tmm_Lock);
	return (TAPTR) mem;
}

static TCALLBACK TAPTR 
mmu_tasktrackrealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TINT8 *newmem;
	exec_Lock(exec, &mmu->tmm_Lock);
	TREMOVE((TNODE *) (oldmem - sizeof(TNODE)));
	newmem = exec_Realloc(exec, oldmem - sizeof(TNODE),
		newsize + sizeof(TNODE));
	if (newmem)
	{
		struct TNode *tempn;
		TADDHEAD(&mmu->tmm_TrackList, (TNODE *) newmem, tempn);
		newmem += sizeof(TNODE);
	}
	exec_Unlock(exec, &mmu->tmm_Lock);
	return (TAPTR) newmem;
}	

static TCALLBACK TVOID 
mmu_tasktrackfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	exec_Lock(exec, &mmu->tmm_Lock);
	TREMOVE((TNODE *) (mem - sizeof(TNODE)));
	exec_Free(exec, mem - sizeof(TNODE));
	exec_Unlock(exec, &mmu->tmm_Lock);
}

static TCALLBACK TVOID
mmu_tasktrackdestroy(struct TMemManager *mmu)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TNODE *nextnode, *node = mmu->tmm_TrackList.tlh_Head;
	TINT numfreed = 0;
	while ((nextnode = node->tln_Succ))
	{
		exec_Free(exec, node);
		numfreed++;
		node = nextnode;
	}
	TDESTROY(&mmu->tmm_Lock);
	if (numfreed)
		tdbprintf1(10, "freed %d pending allocations\n", 
			numfreed);
}

/*****************************************************************************/
/*
**	TNULL tasksafe+tracking allocator
*/

static TCALLBACK TAPTR 
mmu_kntasktrackalloc(struct TMemManager *mmu, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	struct TNode *mem;
	exec_Lock(exec, &mmu->tmm_Lock);
	mem = exec_AllocMMU(exec, &exec->texb_BaseMMU, size + sizeof(TNODE));
	if (mem)
	{
		struct TNode *tempn;
		TADDHEAD(&mmu->tmm_TrackList, mem, tempn);
		mem++;
	}
	exec_Unlock(exec, &mmu->tmm_Lock);
	return (TAPTR) mem;
}

static TCALLBACK TAPTR
mmu_kntasktrackrealloc(struct TMemManager *mmu, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	struct TNode *newmem;
	oldmem -= sizeof(TNODE);
	exec_Lock(exec, &mmu->tmm_Lock);
	TREMOVE((TNODE *) oldmem);
	newmem = exec_Realloc(exec, oldmem, newsize + sizeof(TNODE));
	if (newmem)
	{
		struct TNode *tempn;
		TADDHEAD(&mmu->tmm_TrackList, newmem, tempn);
		newmem++;
	}
	exec_Unlock(exec, &mmu->tmm_Lock);
	return (TAPTR) newmem;
}

static TCALLBACK TVOID
mmu_kntasktrackfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	mem -= sizeof(TNODE);
	exec_Lock(exec, &mmu->tmm_Lock);
	TREMOVE((TNODE *) mem);
	exec_Free(exec, mem);
	exec_Unlock(exec, &mmu->tmm_Lock);
}

static TCALLBACK TVOID
mmu_kntasktrackdestroy(struct TMemManager *mmu)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	TNODE *nextnode, *node = mmu->tmm_TrackList.tlh_Head;
	TINT numfreed = 0;

	while ((nextnode = node->tln_Succ))
	{
		exec_Free(exec, node);
		numfreed++;
		node = nextnode;
	}

	TDESTROY(&mmu->tmm_Lock);
	
	#ifdef TDEBUG
	if (numfreed > 0)
		tdbprintf1(10, "freed %d pending allocations\n",
			numfreed);
	#endif
}

/*****************************************************************************/
/*
**	MMU-on-MMU allocator
*/

static TCALLBACK TAPTR
mmu_mmualloc(struct TMemManager *mmu, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	return exec_AllocMMU(exec, mmu->tmm_Allocator, size);
}

static TCALLBACK TAPTR
mmu_mmurealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	return exec_Realloc(exec, oldmem, newsize);
}	

static TCALLBACK TVOID
mmu_mmufree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TEXECBASE *exec = TGetExecBase(mmu);
	exec_Free(exec, mem);
}

/*****************************************************************************/
/*
**	TNULL allocator
*/

static TCALLBACK TAPTR
mmu_kernelalloc(struct TMemManager *mmu, TUINT size)
{
	TAPTR hal = ((TEXECBASE *) TGetExecBase(mmu))->texb_HALBase;
	TAPTR mem = THALAlloc(hal, size);
	return mem;
}

static TCALLBACK TVOID
mmu_kernelfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
	TAPTR hal = ((TEXECBASE *) TGetExecBase(mmu))->texb_HALBase;
	THALFree(hal, mem, size);
}

static TCALLBACK TAPTR
mmu_kernelrealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	TAPTR hal = ((TEXECBASE *) TGetExecBase(mmu))->texb_HALBase;
	return THALRealloc(hal, oldmem, oldsize, newsize);
}

/*****************************************************************************/
/*
**	void allocator
*/

static TCALLBACK TAPTR
voidalloc(struct TMemManager *mmu, TUINT size)
{
	return TNULL;
}

static TCALLBACK TAPTR 
voidrealloc(struct TMemManager *mmu, TINT8 *oldmem, TUINT oldsize,
	TUINT newsize)
{
	return TNULL;
}	

static TCALLBACK TVOID
voidfree(struct TMemManager *mmu, TINT8 *mem, TUINT size)
{
}

/*****************************************************************************/
/*
**	success = exec_initmmu(exec, mmu, allocator, mmutype, tags)
**	Initialize Memory Manager
*/

/*
**	generic mmu destroy function
*/

static TCALLBACK TVOID 
mmudestroyfunc(struct TMemManager *mmu)
{
	if (mmu->tmm_DestroyFunc) (*mmu->tmm_DestroyFunc)(mmu);		
}

LOCAL TBOOL 
exec_initmmu(TEXECBASE *exec, struct TMemManager *mmu, TAPTR allocator,
	TUINT mmutype, TTAGITEM *tags)
{
	exec_FillMem(exec, mmu, sizeof(struct TMemManager), 0);
	mmu->tmm_Handle.tmo_DestroyFunc = (TDFUNC) mmudestroyfunc;
	mmu->tmm_Type = mmutype;
	mmu->tmm_Handle.tmo_ModBase = exec;

	mmu->tmm_Allocator = allocator;
	
	switch (mmutype)
	{
		case TMMUT_Debug:
			if (exec_initlock(exec, &mmu->tmm_Lock))
			{
				TINITLIST(&mmu->tmm_TrackList);
				mmu->tmm_Alloc = mmu_dballoc;
				mmu->tmm_Free = mmu_dbfree;
				mmu->tmm_Realloc = mmu_dbrealloc;
				mmu->tmm_DestroyFunc = (TDFUNC) mmu_dbdestroy;
				return TTRUE;
			}
			break;

		case TMMUT_MMU:

			if (allocator)
			{
				/* MMU on top of another MMU - no additional functionality */
	
				mmu->tmm_Alloc = mmu_mmualloc;
				mmu->tmm_Free = mmu_mmufree;
				mmu->tmm_Realloc = mmu_mmurealloc;
				return TTRUE;
			}
			else
			{
				mmu->tmm_Alloc = mmu_kernelalloc;
				mmu->tmm_Free = mmu_kernelfree;
				mmu->tmm_Realloc = mmu_kernelrealloc;
				return TTRUE;
			}
			break;

		case TMMUT_Tracking:
	
			TINITLIST(&mmu->tmm_TrackList);
			if (allocator)
			{
				/* implement memory-tracking on top of another MMU */

				mmu->tmm_Alloc = mmu_trackalloc;
				mmu->tmm_Free = mmu_trackfree;
				mmu->tmm_Realloc = mmu_trackrealloc;
				mmu->tmm_DestroyFunc = (TDFUNC) mmu_trackdestroy;
				return TTRUE;
			}
			else
			{
				/* if tracking is requested for a TNULL allocator, we must
				** additionally provide locking for the tracklist, because
				** kernel allocators are implicitly tasksafe */

				if (exec_initlock(exec, &mmu->tmm_Lock))
				{
					mmu->tmm_Alloc = mmu_kntasktrackalloc;
					mmu->tmm_Free = mmu_kntasktrackfree;
					mmu->tmm_Realloc = mmu_kntasktrackrealloc;
					mmu->tmm_DestroyFunc = (TDFUNC) mmu_kntasktrackdestroy;
					return TTRUE;
				}
			}
			break;

		case TMMUT_TaskSafe:
		
			if (allocator)
			{
				/* implement task-safety on top of another MMU */

				if (exec_initlock(exec, &mmu->tmm_Lock))
				{
					mmu->tmm_Alloc = mmu_taskalloc;
					mmu->tmm_Free = mmu_taskfree;
					mmu->tmm_Realloc = mmu_taskrealloc;
					mmu->tmm_DestroyFunc = (TDFUNC) mmu_taskdestroy;
					return TTRUE;
				}
			}
			else
			{
				/* a TNULL MMU is task-safe by definition */

				mmu->tmm_Alloc = mmu_kernelalloc;
				mmu->tmm_Free = mmu_kernelfree;
				mmu->tmm_Realloc = mmu_kernelrealloc;
				return TTRUE;
			}
			break;

		case TMMUT_TaskSafe | TMMUT_Tracking:
	
			/* implement task-safety and tracking on top of another MMU */
	
			if (exec_initlock(exec, &mmu->tmm_Lock))
			{
				TINITLIST(&mmu->tmm_TrackList);
				if (allocator)
				{
					mmu->tmm_Alloc = mmu_tasktrackalloc;
					mmu->tmm_Free = mmu_tasktrackfree;
					mmu->tmm_Realloc = mmu_tasktrackrealloc;
					mmu->tmm_DestroyFunc = (TDFUNC) mmu_tasktrackdestroy;
				}
				else
				{
					mmu->tmm_Alloc = mmu_kntasktrackalloc;
					mmu->tmm_Free = mmu_kntasktrackfree;
					mmu->tmm_Realloc = mmu_kntasktrackrealloc;
					mmu->tmm_DestroyFunc = (TDFUNC) mmu_kntasktrackdestroy;
				}
				return TTRUE;
			}
			break;

		case TMMUT_Message:

			if (allocator == TNULL)		/* must be TNULL for now */
			{
				/* note that we use the execbase lock */
				TINITLIST(&mmu->tmm_TrackList);
				mmu->tmm_Alloc = mmu_knmsgalloc;
				mmu->tmm_Free = mmu_knmsgfree;
				/* messages cannot be reallocated */
				mmu->tmm_Realloc = voidrealloc;
				mmu->tmm_DestroyFunc = (TDFUNC) mmu_knmsgdestroy;
				return TTRUE;
			}
			break;

		case TMMUT_Static:

			/*	MMU on top of memheader */
			if (allocator)
			{	
				mmu->tmm_Alloc = mmu_staticalloc;
				mmu->tmm_Free = mmu_staticfree;
				mmu->tmm_Realloc = mmu_staticrealloc;
				return TTRUE;
			}
			break;

		case TMMUT_Static | TMMUT_TaskSafe:

			/*	MMU on top of memheader, task-safe */
			if (allocator)
			{	
				if (exec_initlock(exec, &mmu->tmm_Lock))
				{
					mmu->tmm_Alloc = mmu_statictaskalloc;
					mmu->tmm_Free = mmu_statictaskfree;
					mmu->tmm_Realloc = mmu_statictaskrealloc;
					mmu->tmm_DestroyFunc = (TDFUNC) mmu_statictaskdestroy;
					return TTRUE;
				}
			}
			break;

		case TMMUT_Pooled:

			/*	MMU on top of a pool */
			if (allocator)
			{	
				mmu->tmm_Alloc = mmu_poolalloc;
				mmu->tmm_Free = mmu_poolfree;
				mmu->tmm_Realloc = mmu_poolrealloc;
				return TTRUE;
			}
			break;

		case TMMUT_Pooled | TMMUT_TaskSafe:

			/*	MMU on top of a pool, task-safe */
			if (allocator)
			{
				if (exec_initlock(exec, &mmu->tmm_Lock))
				{
					mmu->tmm_Alloc = mmu_pooltaskalloc;
					mmu->tmm_Free = mmu_pooltaskfree;
					mmu->tmm_Realloc = mmu_pooltaskrealloc;
					mmu->tmm_DestroyFunc = (TDFUNC) mmu_pooltaskdestroy;
					return TTRUE;
				}
			}
			break;

		case TMMUT_Void:

			mmu->tmm_Allocator = TNULL;
			mmu->tmm_Type = TMMUT_Void;
			mmu->tmm_Alloc = voidalloc;
			mmu->tmm_Free = voidfree;
			mmu->tmm_Realloc = voidrealloc;
			return TTRUE;

	}

	/* as a fallback, initialize a void MMU that is incapable of allocating.
	** this allows safe usage of an MMU without checking the return value of
	** TInitMMU() */

	mmu->tmm_Allocator = TNULL;
	mmu->tmm_Type = TMMUT_Void;
	mmu->tmm_Alloc = voidalloc;
	mmu->tmm_Free = voidfree;
	mmu->tmm_Realloc = voidrealloc;

	return TFALSE;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: exec_memory.c,v $
**	Revision 1.11  2005/09/13 02:41:58  tmueller
**	updated copyright reference
**	
**	Revision 1.10  2005/01/30 06:21:38  tmueller
**	added fixed-size memory pools
**	
**	Revision 1.9  2005/01/29 23:07:21  tmueller
**	The premature introduction of TMem_Align has lead to misconceptions about
**	the depth of the problem at hand - tag interpretation has been removed
**	
**	Revision 1.8  2005/01/29 22:25:24  tmueller
**	added alignment scheme to TMemNode and TMMUInfo
**	
**	Revision 1.7  2005/01/21 23:52:03  tmueller
**	added alignment option to TExecCreatePool
**	
**	Revision 1.6  2005/01/21 11:41:31  tmueller
**	added alignment option to memheaders
**	
**	Revision 1.5  2004/07/05 21:31:33  tmueller
**	dead assignment removed and cosmetic
**	
**	Revision 1.4  2004/04/18 14:08:52  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.3  2004/01/24 14:56:42  tmueller
**	TExecReallocPool() didn't free an allocation and returned NULL when size=0
**	
**	Revision 1.2  2003/12/12 03:44:18  tmueller
**	Default pudsize for pools is now 1024 bytes
**	
**	Revision 1.1.1.1  2003/12/11 07:19:12  tmueller
**	Krypton import
**	
*/
