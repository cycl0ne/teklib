
/*
**	$Id: util_searchsort.c,v 1.3 2005/09/13 02:42:48 tmueller Exp $
**	teklib/mods/util/util_searchsort.c - Searching and sorting utilities
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "util_mod.h"

/*****************************************************************************/
/*
**	success = util_heapsort(utilbase, data, refarray, length, cmpfunc)
**		TINT  (*cmpfunc)(TAPTR data, TTAG ref1, TTAG ref2)
**	Heapsort via reference array:
**		- can sort any kind of data structure
**		- no recursion, no stack impact
**		- performance insensitive to initial array order
*/

EXPORT TBOOL
util_heapsort(TMOD_UTIL *util, TAPTR data, TTAG *refarray, TUINT length, 
	TCMPFUNC cmpfunc)
{
	TUINT indx, k, j, half, limit;
	TTAG temp;
	
	if (refarray && cmpfunc && length > 1)
	{
		indx = (length >> 1) - 1;
		do
		{
			k = indx;
			temp = refarray[k];
			limit = length - 1;
			half = length >> 1;
			while (k < half)
			{
				j = k + k + 1;
				if ((j < limit) && ((*cmpfunc)(data, refarray[j + 1], 
					refarray[j]) > 0))
				{
					++j;
				}
				if ((*cmpfunc)(data, temp, refarray[j]) >= 0)
				{
					break;
				}
				refarray[k] = refarray[j];
				k = j;
			}
			refarray[k] = temp;
		} while (indx-- != 0);
	
		while (--length > 0)
		{
			temp = refarray[0];
			refarray[0] = refarray[length];
			refarray[length] = temp;
			k = 0;
			temp = refarray[k];
			limit = length - 1;
			half = length >> 1;
			while (k < half)
			{
				j = k + k + 1;
				if ((j < limit) && ((*cmpfunc)(data, refarray[j + 1], 
					refarray[j]) > 0))
				{
					++j;
				}
				if ((*cmpfunc)(data, temp, refarray[j]) >= 0)
				{
					break;
				}
				refarray[k] = refarray[j];
				k = j;
			}
			refarray[k] = temp;
		}
		return TTRUE;
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	node = util_seeknode(utilbase, node, steps)
**	Starting at a given node, seek in a list by a given number of steps, and
**	return the node reached,  or TNULL if seeked past end or before start
*/

EXPORT TNODE *
util_seeknode(TMOD_UTIL *util, TNODE *node, TINT steps)
{
	if (node)
	{
		TNODE *nextnode;

		if (!node->tln_Succ) return TNULL;
		if (!node->tln_Pred) return TNULL;
		
		if (steps > 0)
		{
			while ((nextnode = node->tln_Succ))
			{
				if (steps-- == 0)
				{
					return node;
				}
				node = nextnode;
			}
			return TNULL;
		}
		else if (steps < 0)
		{
			while ((nextnode = node->tln_Pred))
			{
				if (steps++ == 0)
				{
					return node;
				}
				node = nextnode;
			}
			return TNULL;
		}
	}

	return node;
}

/*****************************************************************************/
/*
**	util_insertsorted(util, list, numentries, newnode, cmpfunc, userdata)
**	Insert into a sorted list, according to comparision function,
**	implemented with binary search
*/

EXPORT TVOID
util_insertsorted(TMOD_UTIL *util, TLIST *list, TUINT numentries, 
	TNODE *newnode, TCMPFUNC cmpfunc, TAPTR userdata)
{
	if (numentries == 0)
	{
		TAddTail(list, newnode);
	}
	else
	{
		TNODE *currentnode = list->tlh_Head;
		TUINT newindex;
		TUINT currentindex = 0;
		TUINT lowerindex = 0;
		TUINT upperindex = numentries - 1;

		for(;;)
		{
			newindex = lowerindex + (upperindex - lowerindex) / 2;
			currentnode = util_seeknode(util, currentnode, newindex - 
				currentindex);
			currentindex = newindex;

			if ((*cmpfunc)(userdata, (TTAG) newnode, (TTAG) currentnode) > 0)
			{
				if (lowerindex == upperindex)
				{
					TInsert(list, newnode, currentnode);
					break;
				}
				else
				{
					lowerindex = currentindex + 1;
				}
			}
			else
			{
				if (lowerindex == upperindex)
				{
					if (currentindex == 0)
					{
						TAddHead(list, newnode);
					}
					else
					{
						currentnode = currentnode->tln_Pred;
						TInsert(list, newnode, currentnode);
					}
					break;
				}
				else
				{
					upperindex = currentindex;
				}
			}
		}
	}
}

/*****************************************************************************/
/*
**	node = util_findsorted(util, list, numentries, findfunc, userdata)
**	Find a node in a sorted list, according to comparison function,
**	implemented with binary search.
*/

EXPORT TNODE *
util_findsorted(TMOD_UTIL *util, TLIST *list, TUINT numentries, 
	TFINDFUNC findfunc, TAPTR userdata)
{
	if (numentries == 0)
	{
		return TNULL;
	}
	else
	{
		TNODE *currentnode = list->tlh_Head;
		TUINT newindex;
		TUINT currentindex = 0;
		TUINT lowerindex = 0;
		TUINT upperindex = numentries - 1;
		TINT cmpresult;

		for(;;)
		{
			newindex = lowerindex + (upperindex - lowerindex) / 2;
			currentnode = util_seeknode(util, currentnode, newindex -
				currentindex);
			currentindex = newindex;

			cmpresult = (*findfunc)(userdata, (TTAG) currentnode);
			
			if (cmpresult == 0)
			{
				return currentnode;
			}
			else if (cmpresult > 0)
			{
				if (lowerindex == upperindex)
				{
					return TNULL;
				}
				else
				{
					lowerindex = currentindex + 1;
				}
			}
			else
			{
				if (lowerindex == upperindex)
				{
					return TNULL;
				}
				else
				{
					upperindex = currentindex;
				}
			}
		}
	}
}

/*****************************************************************************/
/*
**	success = util_qsort(util, array, num, size, compar)
*/

TVOID
qsortmain(TAPTR exec, TAPTR array, TINT l, TINT r, TINT size, 
	TCMPFUNC compar, TAPTR tmparray, TAPTR udata)
{
	TINT m,e,k;

	if (l >= r)
	    return;
	else if(l==r-1)
	{
		if(compar(udata, (TTAG)((TUINT8*)array+l*size),
			(TTAG)((TUINT8*)array+r*size))>0)
		{
			TExecCopyMem(exec,(TUINT8*)array+r*size,tmparray,size);
			TExecCopyMem(exec,(TUINT8*)array+l*size,(TUINT8*)array+r*size,
				size);
			TExecCopyMem(exec,tmparray,(TUINT8*)array+l*size,size);
		}
		return;
	}

	m=(l + r)>>1;
	TExecCopyMem(exec,(TUINT8*)array+l*size,tmparray,size);
	TExecCopyMem(exec,(TUINT8*)array+m*size,(TUINT8*)array+l*size,size);
	TExecCopyMem(exec,tmparray,(TUINT8*)array+m*size,size);

	e=l;
    for(k=l+1;k<=r;k++)
	{
		if(compar(udata, (TTAG)((TUINT8*)array+k*size),
			(TTAG)((TUINT8*)array+l*size))<0)
		{
			e++;
			TExecCopyMem(exec,(TUINT8*)array+e*size,tmparray,size);
			TExecCopyMem(exec,(TUINT8*)array+k*size,(TUINT8*)array+e*size,
				size);
			TExecCopyMem(exec,tmparray,(TUINT8*)array+k*size,size);
		}
	}

	TExecCopyMem(exec,(TUINT8*)array+l*size,tmparray,size);
	TExecCopyMem(exec,(TUINT8*)array+e*size,(TUINT8*)array+l*size,size);
	TExecCopyMem(exec,tmparray,(TUINT8*)array+e*size,size);

	qsortmain(exec, (TUINT8*)array, l, e-1, size, compar, tmparray, udata);
	qsortmain(exec, (TUINT8*)array, e+1, r, size, compar, tmparray, udata);
}

EXPORT TBOOL
util_qsort(TMOD_UTIL *util, TAPTR array, TINT num, TINT size, TCMPFUNC compar,
	TAPTR udata)
{
	if(array && *compar && num>=2 && size>0)
	{
		TAPTR exec = TGetExecBase(util);
		TAPTR tmparray=TExecAlloc(exec,TNULL,size);
		if(tmparray)
		{
			qsortmain(exec, array, 0, num-1, size, compar, tmparray, udata);
			TExecFree(exec,tmparray);
			return TTRUE;
		}
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: util_searchsort.c,v $
**	Revision 1.3  2005/09/13 02:42:48  tmueller
**	updated copyright reference
**	
**	Revision 1.2  2005/06/29 09:09:15  tmueller
**	changed types of TCMPFUNC, TFINDFUNC, HeapSort refarray
**	
**	Revision 1.1  2004/07/04 21:42:12  tmueller
**	The utility module grew too large -- now splitted over several files
**	
*/
