
/*
**	$Id: unistring_array.c,v 1.27 2005/09/13 02:42:42 tmueller Exp $
**	mods/unistring/unistring_array.c - Array storage class
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include "unistring_mod.h"

typedef struct { char foo[8]; } blk64;		/* 64bit element block */
typedef struct { char foo[16]; } blk128;	/* 128bit element block */

/*****************************************************************************/
/*
**	Alloc/Free index
*/

static TUINT
get2n(TUINT x)
{
	TUINT y = 1;
	if (x == 0) return 0;
	while (y < x) y <<= 1;
	return y;
}

static TVOID
initarray(TMOD_US *mod, TAHEAD *arr, TUINT elementsize)
{
	TInitList(&arr->tah_List);

	arr->tah_Flags = TDSTRF_VALID;
	arr->tah_Length = 0;

	arr->tah_Cursor.tac_RelPos = 0;
	arr->tah_Cursor.tac_AbsPos = 0;
	arr->tah_Cursor.tac_Node = &arr->tah_First;

	arr->tah_ElementSize = elementsize;			/* 1<<n bytes */
	arr->tah_NodeSize = mod->us_InitNodeSize;
		/* suggested nodesize from instance */

	arr->tah_First.tan_AllocLength = sizeof(arr->tah_Buffer) >> elementsize;
	arr->tah_First.tan_UsedLength = 0;
	TAddTail(&arr->tah_List, &arr->tah_First.tan_Node);
}

LOCAL TUINT
getelementsize(TUINT flags)
{
	TUINT newels = 2;
	switch (flags & 0xf0)
	{
		case TASIZE_TTAGITEM:
			newels = 3;		/* min. 8 bytes */
		case TASIZE_TAPTR:
		case TASIZE_TFLOAT:
		case TASIZE_TDOUBLE:
			while ((1 << newels) < sizeof(TTAGITEM)) newels++;
			break;

		default:
			newels = flags & 0xf;
	}
	return newels;
}

static TINT
allocindex(TMOD_US *mod, TUINT flags)
{
	TUINT elementsize = getelementsize(flags);
	if (elementsize <= 4)
	{
		TAHEAD *newa = TExecAllocPool(TExecBase, mod->us_Pool, sizeof(TAHEAD));
		if (newa)
		{
			#ifdef TDEBUG
				mod->us_AllocCount++;
			#endif
		
			if (mod->us_NumFree > 0)
			{
				TINT i, num = mod->us_NumTotal;
				TINT idx = mod->us_LastFreed;
				for (i = 0; i < num; ++i, ++idx)
				{
					idx %= num;
					if (mod->us_Array[idx] == TNULL)
					{
						mod->us_LastFreed = idx + 1;
						mod->us_NumFree--;
						mod->us_Array[idx] = newa;
						initarray(mod, newa, elementsize);
						return idx;
					}
				}
			}
			else
			{
				TINT idx = mod->us_NumTotal + 1;
				TINT newsize = get2n(idx * sizeof(TAHEAD *));
				TAHEAD **newptr;
				
				newptr = TExecReallocPool(TExecBase, mod->us_Pool,
					mod->us_Array, mod->us_ArraySize, newsize);

				if (newptr)
				{
					mod->us_Array = newptr;
					mod->us_ArraySize = newsize;
					mod->us_NumTotal = idx;
					idx--;
					newptr[idx] = newa;
					initarray(mod, newa, elementsize);
					return idx;
				}
			}

			TExecFreePool(TExecBase, mod->us_Pool, newa, sizeof(TAHEAD));
			#ifdef TDEBUG
				mod->us_AllocCount--;
			#endif
		}
	}

	return -1;
}

static TVOID
freehead(TMOD_US *mod, TINT idx)
{
	TExecFreePool(TExecBase, mod->us_Pool, mod->us_Array[idx], sizeof(TAHEAD));
	mod->us_Array[idx] = TNULL;
	#ifdef TDEBUG
		mod->us_AllocCount--;
	#endif
}

static TVOID
freeindex(TMOD_US *mod, TINT idx)
{
	TINT max = mod->us_NumTotal - 1;
	TINT numfree = mod->us_NumFree;

	if (idx == max)
	{
		TINT temp;
		
		for (temp = idx; temp > 0; --temp)
		{
			if (mod->us_Array[temp - 1]) break;
			max--;
			numfree--;
		}
		
		temp = get2n(max * sizeof(TAHEAD *));
		
		mod->us_Array = TExecReallocPool(TExecBase, mod->us_Pool,
			mod->us_Array, mod->us_ArraySize, temp);

		mod->us_ArraySize = temp;
		mod->us_NumTotal = max;
	}
	else
	{
		numfree++;
	}
	
	mod->us_NumFree = numfree;
	mod->us_LastFreed = idx;
}

/*****************************************************************************/
/* 
**	Array access helpers
*/

static TAHEAD *
_array_valid(TMOD_US *mod, TUString idx)
{
	if (idx >= 0 && idx < mod->us_NumTotal)
	{
		TAHEAD *arr = mod->us_Array[idx];
		if (arr->tah_Flags & TDSTRF_VALID) return arr;
	}
	return TNULL;
}

static TANODE *
getcnode(TACURSOR *cursor)
{
	TANODE *cnode = cursor->tac_Node;
	TINT cul = cnode->tan_UsedLength;
	if (cursor->tac_RelPos == cul)
	{
		/* at end of previous node */
		if (cnode->tan_AllocLength != cul)
		{
			cnode = (TANODE *) cnode->tan_Node.tln_Succ;
		}
	}
	return cnode;
}

static TBOOL
testlinear(TACURSOR *cpos1, TACURSOR *cpos2)
{
	return (TBOOL) (getcnode(cpos1) == getcnode(cpos2));
}

static TAPTR
getarrayptr(TACURSOR *cursor, TINT elementsize)
{
	TANODE *cnode = cursor->tac_Node;
	TINT cpos = cursor->tac_RelPos;
	TINT cul = cnode->tan_UsedLength;
	if (cpos == cul)
	{
		/* at end of previous node */
		if (cnode->tan_AllocLength != cul)
		{
			cnode = (TANODE *) cnode->tan_Node.tln_Succ;
			cpos = 0;
		}
	}
	return (TAPTR) (((TUINT8 *) (cnode + 1)) + (cpos << elementsize));
}

static TVOID
readvalue(TANODE *cnode, TINT cpos, TAPTR ptr, TINT elementsize)
{
	if (ptr)
	{
		switch (elementsize)
		{
			case 0:	*((TUINT8 *) ptr) = ((TUINT8 *) (cnode + 1))[cpos];
					break;
			case 1:	*((TUINT16 *) ptr) = ((TUINT16 *) (cnode + 1))[cpos];
					break;
			case 2:	*((TUINT *) ptr) = ((TUINT *) (cnode + 1))[cpos];
					break;
			case 3:	*((blk64 *) ptr) = ((blk64 *) (cnode + 1))[cpos];
					break;
			case 4:	*((blk128 *) ptr) = ((blk128 *) (cnode + 1))[cpos];
					break;
		}
	}
}

static TVOID
writevalue(TANODE *cnode, TINT cpos, TAPTR ptr, TINT elementsize)
{
	if (ptr)
	{
		switch (elementsize)
		{
			case 0:	((TUINT8 *) (cnode + 1))[cpos] = *((TUINT8 *) ptr);
					break;
			case 1:	((TUINT16 *) (cnode + 1))[cpos] = *((TUINT16 *) ptr);
					break;
			case 2:	((TUINT *) (cnode + 1))[cpos] = *((TUINT *) ptr);
					break;
			case 3:	((blk64 *) (cnode + 1))[cpos] = *((blk64 *) ptr);
					break;
			case 4:	((blk128 *) (cnode + 1))[cpos] = *((blk128 *) ptr);
					break;
		}
	}
}

static TAPTR
getelementptr(TANODE *cnode, TINT cpos, TINT elementsize)
{
	switch (elementsize)
	{
		default:
		case 0:	return ((TUINT8 *) (cnode + 1)) + cpos;
		case 1:	return ((TUINT16 *) (cnode + 1)) + cpos;
		case 2:	return ((TUINT *) (cnode + 1)) + cpos;
		case 3:	return ((blk64 *) (cnode + 1)) + cpos;
		case 4:	return ((blk128 *) (cnode + 1)) + cpos;
	}
}

static TVOID
freearray(TMOD_US *mod, TAHEAD *arr)
{
	TANODE *nnode, *node = (TANODE *) arr->tah_List.tlh_Head;
	for (; (nnode = (TANODE *) node->tan_Node.tln_Succ); node = nnode)
	{
		array_freenode(mod, arr, node);
	}	
}

static TVOID
copyelement(TANODE *snode, TINT spos, TANODE *dnode, TINT dpos, TINT els)
{
	readvalue(snode, spos, getelementptr(dnode, dpos, els), els);
}

/*
**	node seeking routine - handles the following rules:
**
**	|_____|			cpos == uselen == alloclen (=> end, no successor)
**	      ^
**	|_____| |_____|	cpos == 0 (=> predecessor is full, or have no predecessor)
**	        ^
**	|___|_| |__|__|	cpos == uselen != alloclen (=> have successor)
**	    ^
*/

LOCAL TINT
__array_seek(TAHEAD *arr, TACURSOR *cursor, TINT steps)
{
	TINT abspos = cursor->tac_AbsPos;
	if (steps != 0)
	{
		TANODE *nnode, *cnode = cursor->tac_Node;
		TINT cpos = cursor->tac_RelPos;

		if (steps > 0)
		{
			TINT cul = cnode->tan_UsedLength;
	
			abspos += steps;
			if (abspos > arr->tah_Length) abspos = arr->tah_Length;
	
			cpos += steps;
	
			while (cpos > cul || (cpos == cul && cpos == cnode->tan_AllocLength))
			{
				nnode = (TANODE *) cnode->tan_Node.tln_Succ;
				if (nnode->tan_Node.tln_Succ == TNULL)
				{
					cpos = cul;
					break;
				}
				cpos -= cul;
				cnode = nnode;
				cul = cnode->tan_UsedLength;
			}
		}
		else
		{
			abspos += steps;
			if (abspos < 0) abspos = 0;
	
			steps = -steps;		
			nnode = (TANODE *) cnode->tan_Node.tln_Pred;
	
			while (nnode->tan_Node.tln_Pred && 
				(cpos < steps || (cpos == steps && 
					nnode->tan_UsedLength != nnode->tan_AllocLength)))
			{
				steps -= cpos;
				cnode = nnode;
				nnode = (TANODE *) cnode->tan_Node.tln_Pred;
				cpos = cnode->tan_UsedLength;
			}
		
			cpos -= steps;
			if (cpos < 0) cpos = 0;
		}

		cursor->tac_Node = cnode;
		cursor->tac_RelPos = cpos;
		cursor->tac_AbsPos = abspos;
	}
	return abspos;
}

/*****************************************************************************/
/* 
**	Alloc/free fragment nodes
*/

EXPORT TANODE *
_array_allocnode(TMOD_US *mod, TINT len, TINT elementsize)
{
	TINT bytes = (len << elementsize) + sizeof(TANODE);
	TANODE *node = TExecAllocPool(TExecBase, mod->us_Pool, bytes);
	if (node)
	{
		#ifdef TDEBUG
			mod->us_AllocCount++;
		#endif
		node->tan_AllocLength = len;
	}
	return node;
}

EXPORT TVOID
_array_freenode(TMOD_US *mod, TAHEAD *arr, TANODE *node)
{
	/* the internal node must not be freed */
	if (node != &arr->tah_First)
	{
		TExecFreePool(TExecBase, mod->us_Pool, node, 
			(node->tan_AllocLength << arr->tah_ElementSize) + sizeof(TANODE));
		#ifdef TDEBUG
			mod->us_AllocCount--;
		#endif
	}
}

EXPORT TANODE *
_array_allocnode_tasksafe(TMOD_US *mod, TINT len, TINT elementsize)
{
	TINT bytes = (len << elementsize) + sizeof(TANODE);
	TANODE *node;

	TExecLock(TExecBase, mod->us_Lock);
	node = TExecAllocPool(TExecBase, mod->us_Pool, bytes);
	TExecUnlock(TExecBase, mod->us_Lock);

	if (node)
	{
		#ifdef TDEBUG
			mod->us_AllocCount++;
		#endif
		node->tan_AllocLength = len;
	}

	return node;
}

EXPORT TVOID
_array_freenode_tasksafe(TMOD_US *mod, TAHEAD *arr, TANODE *node)
{
	/* the internal node must not be freed */
	if (node != &arr->tah_First)
	{
		TExecLock(TExecBase, mod->us_Lock);

		TExecFreePool(TExecBase, mod->us_Pool, node, 
			(node->tan_AllocLength << arr->tah_ElementSize) + sizeof(TANODE));

		TExecUnlock(TExecBase, mod->us_Lock);

		#ifdef TDEBUG
			mod->us_AllocCount--;
		#endif
	}
}

/*****************************************************************************/

EXPORT TUString
_array_alloc(TMOD_US *mod, TUINT flags)
{
	return allocindex(mod, flags);
}

EXPORT TVOID
_array_free(TMOD_US *mod, TUString idx)
{
	if (idx >= 0 && idx < mod->us_NumTotal)
	{
		TAHEAD *arr = mod->us_Array[idx];
		if (!(arr->tah_Flags & TDSTRF_FREE))
		{
			freearray(mod, arr);
			freehead(mod, idx);
			freeindex(mod, idx);
		}
	}
}

EXPORT TUString
_array_alloc_tasksafe(TMOD_US *mod, TUINT flags)
{
	TUString idx;
	TExecLock(TExecBase, mod->us_Lock);
	idx = allocindex(mod, flags);
	TExecUnlock(TExecBase, mod->us_Lock);
	return idx;
}

EXPORT TVOID
_array_free_tasksafe(TMOD_US *mod, TUString idx)
{
	if (idx >= 0 && idx < mod->us_NumTotal)
	{
		TAHEAD *arr = mod->us_Array[idx];
		if (!(arr->tah_Flags & TDSTRF_FREE))
		{
			TExecLock(TExecBase, mod->us_Lock);
			freearray(mod, arr);
			freehead(mod, idx);
			freeindex(mod, idx);
			TExecUnlock(TExecBase, mod->us_Lock);
		}
	}
}

EXPORT TVOID
_array_move(TMOD_US *mod, TUString srcidx, TUString dstidx)
{
	freearray(mod, mod->us_Array[dstidx]);
	freehead(mod, dstidx);
	mod->us_Array[dstidx] = mod->us_Array[srcidx];
	freeindex(mod, srcidx);
}

EXPORT TVOID
_array_move_tasksafe(TMOD_US *mod, TUString srcidx, TUString dstidx)
{
	TExecLock(TExecBase, mod->us_Lock);
	_array_move(mod, srcidx, dstidx);
	TExecUnlock(TExecBase, mod->us_Lock);
}

/*****************************************************************************/
/* 
**	pos/err = array_get(mod, arr, ptr)
**	get element at current cursor position
*/

LOCAL TVOID
__array_get(TACURSOR *cursor, TINT elementsize, TAPTR data)
{
	TANODE *cnode = cursor->tac_Node;
	TINT cpos = cursor->tac_RelPos;
	if (cpos == cnode->tan_UsedLength)
	{
		/* at end of previous node */
		cnode = (TANODE *) cnode->tan_Node.tln_Succ;
		cpos = 0;
	}
	readvalue(cnode, cpos, data, elementsize);
}

LOCAL TVOID
_array_get(TAHEAD *arr, TAPTR data)
{
	__array_get(&arr->tah_Cursor, arr->tah_ElementSize, data);
}

EXPORT TINT
array_get(TMOD_US *mod, TUString idx, TAPTR data)
{
	TAHEAD *arr = _array_valid(mod, idx);
	if (arr && arr->tah_Cursor.tac_AbsPos < arr->tah_Length)
	{
		_array_get(arr, data);
		return arr->tah_Cursor.tac_AbsPos;
	}
	return -1;
}

/*****************************************************************************/
/*
**	pos/err = array_set(mod, arr, ptr)
**	set element at current cursor position
*/

LOCAL TVOID
_array_set(TAHEAD *arr, TAPTR data)
{
	TANODE *cnode = arr->tah_Cursor.tac_Node;
	TINT cpos = arr->tah_Cursor.tac_RelPos;
	if (cpos == cnode->tan_UsedLength)
	{
		/* at end of previous node */
		cnode = (TANODE *) cnode->tan_Node.tln_Succ;
		cpos = 0;
	}
	writevalue(cnode, cpos, data, arr->tah_ElementSize);
}

EXPORT TINT
array_set(TMOD_US *mod, TUString idx, TAPTR data)
{
	TAHEAD *arr = _array_valid(mod, idx);
	if (arr && arr->tah_Cursor.tac_AbsPos < arr->tah_Length)
	{
		_array_set(arr, data);
		return arr->tah_Cursor.tac_AbsPos;
	}
	return -1;
}

/*****************************************************************************/
/* 
**	err/len = array_length(mod, arr)
*/

EXPORT TINT
array_length(TMOD_US *mod, TUString idx)
{
	TAHEAD *arr = _array_valid(mod, idx);
	if (arr) return arr->tah_Length;
	return -1;
}

/*****************************************************************************/
/* 
**	newpos/err = array_ins(mod, arr, data)
**	insert data at the current cursor position. returns
**	the new cursor position, or -1 in case of an error
*/

LOCAL TINT
_array_ins(TMOD_US *mod, TAHEAD *arr, TAPTR data)
{
	TANODE *cnode = arr->tah_Cursor.tac_Node;
	TINT cal = cnode->tan_AllocLength;
	TINT cpos = arr->tah_Cursor.tac_RelPos;
	TINT els = arr->tah_ElementSize;
		
	if (cpos == cal)
	{
		/* At the end of the last node, which is full */

		cnode = array_allocnode(mod, arr->tah_NodeSize, els);
		if (cnode == TNULL)
		{
			arr->tah_Flags &= ~TDSTRF_VALID;
			return -1;
		}

		TAddTail(&arr->tah_List, &cnode->tan_Node);
		cnode->tan_UsedLength = 1;

		arr->tah_Cursor.tac_Node = cnode;
		cpos = 0;

		/* write element */
		writevalue(cnode, cpos, data, els);
	}
	else
	{
		TANODE *nnode = (TANODE *) cnode->tan_Node.tln_Succ;
		TINT cul = cnode->tan_UsedLength;
		TINT i;
	
		if (cul == cal)
		{
			/* Current node is full. */
			
			if (nnode->tan_Node.tln_Succ && 
				nnode->tan_UsedLength < nnode->tan_AllocLength)
			{
				/* Successing node is not full */

				for (i = nnode->tan_UsedLength; i > 0; --i)
				{
					copyelement(nnode, i - 1, nnode, i, els);
				}

				nnode->tan_UsedLength++;
			}
			else
			{
				/* No successor, or it is full. Need a new node. */
	
				nnode = array_allocnode(mod, arr->tah_NodeSize, els);
				if (nnode == TNULL)
				{
					arr->tah_Flags &= ~TDSTRF_VALID;
					return -1;
				}

				TInsert(&arr->tah_List, &nnode->tan_Node, &cnode->tan_Node);
				nnode->tan_UsedLength = 1;
			}
	
			cal--;
			copyelement(cnode, cal, nnode, 0, els);

			for (i = cal; i > cpos; --i)
			{
				copyelement(cnode, i - 1, cnode, i, els);
			}
		}
		else
		{
			/* Current node is not full */

			for (i = cul; i > cpos; --i)
			{
				copyelement(cnode, i - 1, cnode, i, els);
			}

			cnode->tan_UsedLength = cul + 1;
			cal--;
		}

		/* write element */
		writevalue(cnode, cpos, data, els);
	
		if (cpos == cal && nnode->tan_Node.tln_Succ)
		{
			cpos = -1;	/* wrap cursor if there is a next node */
			arr->tah_Cursor.tac_Node = nnode;
		}
	}

	arr->tah_Cursor.tac_RelPos = cpos + 1;
	arr->tah_Length++;
	return ++arr->tah_Cursor.tac_AbsPos;
}

EXPORT TINT
array_ins(TMOD_US *mod, TUString idx, TAPTR data)
{
	TAHEAD *arr = _array_valid(mod, idx);
	if (arr) return _array_ins(mod, arr, data);
	return -1;
}

/*****************************************************************************/
/* 
**	pos/err = array_rem(mod, arr, data)
**	remove element from current cursor position. returns
**	the cursor position, or -1 in case of an error.
**	the removed element being inserted to the data ptr
*/

LOCAL TBOOL
_array_rem(TMOD_US *mod, TAHEAD *arr, TAPTR data)
{
	TANODE *cnode = arr->tah_Cursor.tac_Node;
	TANODE *nnode = (TANODE *) cnode->tan_Node.tln_Succ;
	TINT cul = cnode->tan_UsedLength;
	TINT cpos = arr->tah_Cursor.tac_RelPos;
	TINT els = arr->tah_ElementSize;
	TINT i;
	
	if (cpos == cul)
	{
		/* at the end of a node */
		if (nnode->tan_Node.tln_Succ)
		{
			/* have successor */

			TINT nul = nnode->tan_UsedLength - 1;

			/* the character being removed */
			readvalue(nnode, 0, data, els);

			if (nul == 0)
			{
				TRemove(&nnode->tan_Node);
				array_freenode(mod, arr, nnode);
			}
			else
			{
				for (i = 0; i < nul; ++i)
				{
					copyelement(nnode, i + 1, nnode, i, els);
				}
				nnode->tan_UsedLength = nul;
			}
			arr->tah_Length--;
		}
		else
		{
			/* at the end, nothing to do */
			return TFALSE;
		}
	}
	else
	{
		/* not at the end of a node */
		
		/* the character being removed */
		readvalue(cnode, cpos, data, els);

		if (--cul == 0)
		{
			/* last character in current node removed */

			TANODE *pnode = (TANODE *) cnode->tan_Node.tln_Pred;
			if (pnode->tan_Node.tln_Pred)
			{
				/* have predecessor */
				if (pnode->tan_UsedLength == pnode->tan_AllocLength)
				{
					/* if predecessor is full... */
					if (nnode->tan_Node.tln_Succ)
					{
						/* wrap cursor to successor, if there is one */
						goto successor;
					}
				}

				/* wrap cursor to predecessor */
				arr->tah_Cursor.tac_Node = pnode;
				arr->tah_Cursor.tac_RelPos = pnode->tan_UsedLength;
				goto cursorok;
			}

			if (nnode->tan_Node.tln_Succ)
			{
				/* wrap cursor to successor */

successor:		arr->tah_Cursor.tac_Node = nnode;
				arr->tah_Cursor.tac_RelPos = 0;
			}

cursorok:	TRemove(&cnode->tan_Node);
			array_freenode(mod, arr, cnode);
			
			if (TListEmpty(&arr->tah_List))
			{
				/* would be the last node. re-initialize static node */
				cnode = (TANODE *) &arr->tah_First.tan_Node;
				cnode->tan_UsedLength = 0;	
				arr->tah_Cursor.tac_RelPos = 0;
				arr->tah_Cursor.tac_Node = cnode;			
				TAddTail(&arr->tah_List, &cnode->tan_Node);
			}
		}
		else
		{
			for (i = cpos; i < cul; ++i)
			{
				copyelement(cnode, i + 1, cnode, i, els);
			}
			cnode->tan_UsedLength = cul;
		}

		arr->tah_Length--;
	}
	return TTRUE;
}

EXPORT TINT
array_rem(TMOD_US *mod, TUString idx, TAPTR data)
{
	TAHEAD *arr = _array_valid(mod, idx);
	if (arr)
	{
		_array_rem(mod, arr, data);
		return arr->tah_Cursor.tac_AbsPos;
	}
	return -1;
}

/*****************************************************************************/
/* 
**	pos/err = array_seek(mod, idx, mode, steps)
**	mode: 0 from current, -1 from end, 1 from start
*/

LOCAL TINT
_array_seek(TAHEAD *arr, TACURSOR *cursor, TINT mode, TINT steps)
{
	switch (mode)
	{
		default:
			return -1;

		case -1:
		{
			TANODE *cnode = (TANODE *) arr->tah_List.tlh_TailPred;
			cursor->tac_Node = cnode;
			cursor->tac_AbsPos = arr->tah_Length;
			cursor->tac_RelPos = cnode->tan_UsedLength;
			steps = -steps;
			break;
		}

		case 1:
			cursor->tac_Node = (TANODE *) arr->tah_List.tlh_Head;
			cursor->tac_AbsPos = 0;
			cursor->tac_RelPos = 0;
			break;

		case 0:
			break;
	}
	return __array_seek(arr, cursor, steps);
}

EXPORT TINT
array_seek(TMOD_US *mod, TUString idx, TINT mode, TINT steps)
{
	TAHEAD *arr = _array_valid(mod, idx);
	if (arr) return _array_seek(arr, &arr->tah_Cursor, mode, steps);
	return -1;
}

/*****************************************************************************/
/* 
**	ptr = array_map(mod, arr, ptr, offs, len)
**	render array into a block of memory supplied by the caller,
**	or return a pointer to a linear range of memory in the array
*/

static TBOOL
_array_break(TMOD_US *mod, TAHEAD *arr)
{
	TANODE *cnode = arr->tah_Cursor.tac_Node;
	TINT cpos = arr->tah_Cursor.tac_RelPos;
	TINT cul = cnode->tan_UsedLength;
	TINT len = cul - cpos;
	
	if (len && cpos)
	{
		TANODE *nnode = (TANODE *) cnode->tan_Node.tln_Succ;
		TINT els = arr->tah_ElementSize;
		TINT i;

		if (nnode->tan_Node.tln_Succ == TNULL)
		{
			/* have no successor, need a new node */
			nnode = array_allocnode(mod,
				get2n(TMAX(len, arr->tah_NodeSize)), els);
			if (nnode == TNULL)
			{
				arr->tah_Flags &= ~TDSTRF_VALID;
				return TFALSE;
			}
	
			for (i = 0; i < len; ++i)
			{
				copyelement(cnode, i + cpos, nnode, i, els);
			}
	
			nnode->tan_UsedLength = len;
			cnode->tan_UsedLength = cpos;
			TInsert(&arr->tah_List, &nnode->tan_Node, &cnode->tan_Node);
		}
		else
		{
			/* save old abspos */
			TINT abspos = arr->tah_Cursor.tac_AbsPos;
	
			/* pretend this node would be full */
			cnode->tan_UsedLength = cnode->tan_AllocLength;
		
			/* place cursor to start of following node */
			arr->tah_Cursor.tac_Node = nnode;
			arr->tah_Cursor.tac_RelPos = 0;
	
			/* insert chars into newly separated node */		
			for (i = cpos; i < cul; ++i)
			{
				if (_array_ins(mod, arr, getelementptr(cnode, i, els)) < 0)
				{
					arr->tah_Flags &= ~TDSTRF_VALID;
					return TFALSE;
				}
			}
		
			/* restore and correct values */
			cnode->tan_UsedLength = cpos;
			arr->tah_Cursor.tac_Node = cnode;
			arr->tah_Cursor.tac_RelPos = cpos;
			arr->tah_Cursor.tac_AbsPos = abspos;
			arr->tah_Length -= len;
		}
	}
	return TTRUE;
}

LOCAL TAPTR
_array_maplinear(TMOD_US *mod, TAHEAD *arr, TINT startpos, TINT len)
{
	TINT oldpos = arr->tah_Cursor.tac_AbsPos;
	TACURSOR sc = arr->tah_Cursor;
	TAPTR lptr = TNULL;

	if (__array_seek(arr, &sc, startpos - oldpos) == startpos)
	{
		TACURSOR ec = sc;

		if (__array_seek(arr, &ec, len - 1) == startpos + len - 1)
		{
			if (testlinear(&sc, &ec))
			{
				/* in linear range already */
				return getarrayptr(&sc, arr->tah_ElementSize);
			}
		}

		/* seek to start */
		arr->tah_Cursor = sc;

		/* cut node at start */
		if (_array_break(mod, arr))
		{
			/* sc = arr->tah_Cursor; - should not be required */

			/* seek to end */
			if (__array_seek(arr, &arr->tah_Cursor, len) == startpos + len)
			{
				TINT els = arr->tah_ElementSize;

				/* test if the cut produced a node in linear range */
				ec = arr->tah_Cursor;
				__array_seek(arr, &ec, -1);
				if (testlinear(&sc, &ec))
				{
					/* node in linear range after cutting */
					lptr = getarrayptr(&sc, els);
				}
				else
				{
					/* cut node at end */
					if (_array_break(mod, arr))
					{
						/* alloc new node */
						TANODE *nnode;
						TINT newlen = get2n(TMAX(arr->tah_NodeSize, len + 1));
				
					#if 0		
						/* this line sets a new, persistent size for nodes
						to be allocated for this string. questionable as a
						result from a mapping operation! */
						arr->tah_NodeSize = newlen;
					#endif

						nnode = array_allocnode(mod, newlen, els);
						if (nnode)
						{
							TINT i;
							TANODE *cnode, *cnext, *cend;
							TNODE *pnode;
							
							/* merge */

							arr->tah_Cursor = sc;
							for (i = 0; i < len; ++i)
							{
								_array_get(arr, getelementptr(nnode, i, els));
								__array_seek(arr, &arr->tah_Cursor, 1);
							}
							
							nnode->tan_UsedLength = len;

							for (i = len; i < newlen; ++i)
							{
								if (_array_rem(mod, arr, 
									getelementptr(nnode, i, els)) 
										== TFALSE) break;
								nnode->tan_UsedLength++;
								arr->tah_Length++;			
							}

							/* unlink old nodes */

							cnode = getcnode(&sc);
							pnode = cnode->tan_Node.tln_Pred;
							cend = getcnode(&arr->tah_Cursor);
							if (cend->tan_UsedLength == 
								cend->tan_AllocLength) cend = TNULL;

							while ((cnext = 
								(TANODE *) cnode->tan_Node.tln_Succ))
							{
								TRemove((TNODE *) cnode);
								array_freenode(mod, arr, cnode);
								if (cnext == cend) break;
								cnode = cnext;
							}
							
							/* link new node */
							
							TInsert(&arr->tah_List, (TNODE *) nnode, pnode);

							lptr = (nnode + 1);
						}
						else
						{
							arr->tah_Flags &= ~TDSTRF_VALID;
						}
					}
				}
			}
		}

		_array_seek(arr, &arr->tah_Cursor, 1, oldpos);
	}

	return lptr;
}

EXPORT TAPTR
array_map(TMOD_US *mod, TUString idx, TINT offs, TINT len)
{
	TAHEAD *arr = _array_valid(mod, idx);
	if (arr && len > 0)
	{
		return _array_maplinear(mod, arr, offs, len);
	}
	return TNULL;
}

/*****************************************************************************/

LOCAL TINT
_array_render(TMOD_US *mod, TAHEAD *arr, TUINT8 *dest, TINT startpos,
	TINT len, TUINT flags)
{
	if (startpos + len <= arr->tah_Length)
	{
		TACURSOR sc = arr->tah_Cursor;
		TINT oldpos = sc.tac_AbsPos;
		
		if (__array_seek(arr, &sc, startpos - oldpos) == startpos)
		{
			TUINT newels = getelementsize(flags);
			TUINT8 temp[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
			TINT oldels = arr->tah_ElementSize;
			TUINT8 *src;
			TINT i;

			for (i = 0; i < len; ++i)
			{
				src = convertelement(mod, getarrayptr(&sc, oldels), 
					temp, oldels, newels);
					
				switch (newels)
				{
					case 0:	*((TUINT8 *) dest) = *((TUINT8 *) src);
							break;
					case 1:	*((TUINT16 *) dest) = *((TUINT16 *) src);
							break;
					case 2:	*((TUINT *) dest) = *((TUINT *) src);
							break;
					case 3:	*((blk64 *) dest) = *((blk64 *) src);
							break;
					case 4:	*((blk128 *) dest) = *((blk128 *) src);
							break;
				}

				dest += 1 << newels;
				__array_seek(arr, &sc, 1);
			}

			return 0;
		}
	}
	return -1;
}

EXPORT TINT
array_render(TMOD_US *mod, TUString idx, TUINT8 *dest, TINT offs, TINT len)
{
	TAHEAD *arr = _array_valid(mod, idx);
	if (arr && len > 0 && dest)
	{
		return _array_render(mod, arr, dest, offs, len, arr->tah_ElementSize);
	}
	return -1;
}

/*****************************************************************************/
/* 
**	len/err = array_change(mod, idx, flags)
**	Change array's element size. Conversion maintains native endianness,
**	e.g. when changing the element size from 1 to 2, padding with zero
**	- behind the element on little endian: x -> x0
**	- before the element on big endian:    x -> 0x
*/

LOCAL TINT
_array_setsize(TMOD_US *mod, TAHEAD *arr, TUINT newels)
{
	TUINT oldels = arr->tah_ElementSize;
	TINT len = arr->tah_Length;
	if (oldels != newels)
	{
		TUINT8 *ptr = TExecAlloc(TExecBase, mod->us_MMU, len << newels);
		if (ptr)
		{
			TUINT8 temp[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
			TINT cpos = arr->tah_Cursor.tac_AbsPos;
			TUINT8 *p = ptr;
			TINT ofs = 1 << oldels;
			TINT i;
			
			len = arr->tah_Length;
			_array_render(mod, arr, ptr, 0, len, oldels);
			freearray(mod, arr);
			initarray(mod, arr, newels);
			
			for (i = 0; i < len; ++i)
			{
				if (_array_ins(mod, arr, 
					convertelement(mod, p, temp, oldels, newels)) < 0)
				{
					len = -1;
					break;
				}
				p += ofs;
			}

			TExecFree(TExecBase, ptr);
			
			if (len >= 0)
			{
				_array_seek(arr, &arr->tah_Cursor, 1, cpos);
			}
		}
		else
		{
			len = -1;
		}
	}
	return len;
}

EXPORT TINT
array_change(TMOD_US *mod, TUString idx, TUINT flags)
{
	TAHEAD *arr = _array_valid(mod, idx);
	TUINT newels = getelementsize(flags);
	if (arr && newels <= 4) return _array_setsize(mod, arr, newels);
	return -1;
}

/*****************************************************************************/
/* 
**	newidx = array_dup(mod, idx)
**	create duplicate
*/

static TINT
_array_insertcopy(TMOD_US *mod, TAHEAD *srcarr, TAHEAD *dstarr)
{
	TINT i, len = srcarr->tah_Length;
	TINT els = srcarr->tah_ElementSize;
	TACURSOR sc;

	_array_seek(srcarr, &sc, 1, 0);
	for (i = 0; i < len; ++i)
	{
		if (_array_ins(mod, dstarr, getarrayptr(&sc, els)) < 0) return -1;
		__array_seek(srcarr, &sc, 1);
	}
	_array_seek(dstarr, &dstarr->tah_Cursor, 1, srcarr->tah_Cursor.tac_AbsPos);
	return len;
}

LOCAL TUString
_array_dup(TMOD_US *mod, TAHEAD *srcarr)
{
	TINT els = srcarr->tah_ElementSize;
	TUString newidx = array_alloc(mod, els);
	if (newidx >= 0)
	{
		TAHEAD *newarr = mod->us_Array[newidx];
		if (_array_insertcopy(mod, srcarr, newarr) == TFALSE)
		{
			array_free(mod, newidx);
			newidx = -1;
		}
	}
	return newidx;
}

EXPORT TUString
array_dup(TMOD_US *mod, TUString idx)
{
	TAHEAD *arr = _array_valid(mod, idx);
	if (arr) return _array_dup(mod, arr);
	return TINVALID_STRING;
}

/*****************************************************************************/
/* 
**	len = array_copy(mod, src, dst)
**	copy array
*/

EXPORT TINT
array_copy(TMOD_US *mod, TUString srcidx, TUString dstidx)
{
	TAHEAD *srcarr = _array_valid(mod, srcidx);
	TAHEAD *dstarr = _array_valid(mod, dstidx);
	if (srcarr && dstarr)
	{
		freearray(mod, dstarr);
		initarray(mod, dstarr, srcarr->tah_ElementSize);
		return _array_insertcopy(mod, srcarr, dstarr);
	}
	return -1;
}

/*****************************************************************************/
/* 
**	length = array_trunc(mod, str)
**	truncate at current cursor position
*/

EXPORT TINT
array_trunc(TMOD_US *mod, TUString idx)
{
	TINT length = -1;
	TAHEAD *arr = _array_valid(mod, idx);
	if (arr)
	{
		TACURSOR *cursor = &arr->tah_Cursor;
		TANODE *cnode = cursor->tac_Node, *nnode;

		cnode->tan_UsedLength = cursor->tac_RelPos;
		arr->tah_Length = cursor->tac_AbsPos;
		cnode = (TANODE *) cnode->tan_Node.tln_Succ;
		while ((nnode = (TANODE *) cnode->tan_Node.tln_Succ))
		{
			TRemove((struct TNode *) cnode);
			array_freenode(mod, arr, cnode);
			cnode = nnode;
		}
	}
	return length;
}

/*****************************************************************************/
/*****************************************************************************/

#ifdef TDEBUG
#ifdef TSYS_POSIX

static TVOID
_array_debugprint(TAHEAD *arr)
{
	TANODE *nnode, *node = (TANODE *) arr->tah_List.tlh_Head;
	TANODE *cnode = arr->tah_Cursor.tac_Node;
	
	printf("%03d %03d : ", arr->tah_Length, arr->tah_Cursor.tac_AbsPos);
	
	for (; (nnode = (TANODE *) node->tan_Node.tln_Succ); node = nnode)
	{
		TUINT8 *cp = (TUINT8 *) (node + 1);
		TINT i;
		for (i = 0; i <= node->tan_AllocLength; ++i)
		{
			if (node == cnode && i == arr->tah_Cursor.tac_RelPos)
			{
				if (i == cnode->tan_AllocLength)
				{
					printf("%c[7m|%c[27m", 27, 27);
				}
				else if (i >= cnode->tan_UsedLength)
				{
					printf("%c[7m.%c[27m", 27, 27);
				}
				else
				{
					printf("%c[7m%c%c[27m", 27, cp[i], 27);
				}
			}
			else if (i < node->tan_UsedLength)
			{
				printf("%c", cp[i]);
			}
			else if (i == node->tan_AllocLength)
			{
				printf("|");
			}
			else
			{
				printf(".");
			}
		}
	}
	printf("\n");
}

#endif
#endif

EXPORT TVOID
array_debugprint(TMOD_US *mod, TUString idx)
{
#ifdef TDEBUG
#ifdef TSYS_POSIX
	_array_debugprint(mod->us_Array[idx]);
#endif
#endif
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: unistring_array.c,v $
**	Revision 1.27  2005/09/13 02:42:42  tmueller
**	updated copyright reference
**	
**	Revision 1.26  2005/01/29 22:27:55  tmueller
**	TSYS_POSIX32 renamed to TSYS_POSIX
**	
**	Revision 1.25  2004/08/01 11:31:52  tmueller
**	removed lots of history garbage
**	
*/
