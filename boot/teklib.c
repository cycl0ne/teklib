
/*
**	$Id: teklib.c,v 1.7 2005/09/13 02:41:00 tmueller Exp $
**	boot/teklib.c - Implementation of linklib functions
**
**	The library functions operate on public exec structures, do not
**	depend on private data or functions, and constitute the most
**	conservative and immutable part of TEKlib.
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/teklib.h>
#include <tek/proto/exec.h>

/*****************************************************************************/
/*
**	TInitList(list)
**	Prepare list header
*/

TLIBAPI TVOID
TInitList(struct TList *list)
{
	list->tlh_TailPred = (struct TNode *) list;
	list->tlh_Tail = TNULL;
	list->tlh_Head = (struct TNode *) &list->tlh_Tail;
}

/*****************************************************************************/
/*
**	TAddHead(list, node)
**	Add a node at the head of a list
*/

TLIBAPI TVOID
TAddHead(struct TList *list, struct TNode *node)
{
	struct TNode *temp = list->tlh_Head;
	list->tlh_Head = node;
	node->tln_Succ = temp;
	node->tln_Pred = (struct TNode *) &list->tlh_Head;
	temp->tln_Pred = node;
}

/*****************************************************************************/
/*
**	TAddTail(list, node)
**	Add a node at the tail of a list
*/

TLIBAPI TVOID
TAddTail(struct TList *list, struct TNode *node)
{
	struct TNode *temp = list->tlh_TailPred;
	list->tlh_TailPred = node;
	node->tln_Succ = (struct TNode *) &list->tlh_Tail;
	node->tln_Pred = temp;
	temp->tln_Succ = node;
}

/*****************************************************************************/
/*
**	node = TRemHead(list)
**	Unlink and return a list's first node
*/

TLIBAPI struct TNode *
TRemHead(struct TList *list)
{
	struct TNode *temp = list->tlh_Head;
	if (temp->tln_Succ)
	{
		list->tlh_Head = temp->tln_Succ;
		temp->tln_Succ->tln_Pred = (struct TNode *) &list->tlh_Head;
		return temp;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	node = TRemTail(list)
**	Unlink and return a list's last node
*/

TLIBAPI struct TNode *
TRemTail(struct TList *list)
{
	struct TNode *temp = list->tlh_TailPred;
	if (temp->tln_Pred)
	{
		list->tlh_TailPred = temp->tln_Pred;
		temp->tln_Pred->tln_Succ = (struct TNode *) &list->tlh_Tail;
		return temp;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	TRemove(node)
**	Unlink node from a list
*/

TLIBAPI TVOID
TRemove(struct TNode *node)
{
	struct TNode *temp = node->tln_Succ;
	node->tln_Pred->tln_Succ = temp;
	temp->tln_Pred = node->tln_Pred;
}

/*****************************************************************************/
/*
**	TNodeUp(node)
**	Move a node one position up in a list
*/

TLIBAPI TVOID 
TNodeUp(struct TNode *node)
{
	struct TNode *temp = node->tln_Pred;
	if (temp->tln_Pred)
	{	
		temp->tln_Pred->tln_Succ = node;
		node->tln_Pred = temp->tln_Pred;
		temp->tln_Succ = node->tln_Succ;
		temp->tln_Pred = node;
		node->tln_Succ->tln_Pred = temp;
		node->tln_Succ = temp;
	}
}

/*****************************************************************************/
/*
**	TInsert(list, node, prednode)
**	Insert node after prednode
*/

TLIBAPI TVOID
TInsert(struct TList *list, struct TNode *node, struct TNode *prednode)
{
	if (list)
	{
		if (prednode)
		{
			struct TNode *temp = prednode->tln_Succ;
			if (temp)
			{
				node->tln_Succ = temp;
				node->tln_Pred = prednode;
				temp->tln_Pred = node;
				prednode->tln_Succ = node;
			}
			else
			{
				node->tln_Succ = prednode;
				temp = prednode->tln_Pred;
				node->tln_Pred = temp;
				prednode->tln_Pred = node;
				temp->tln_Succ = node;
			}
		}
		else
		{
			TAddHead(list, node);
		}
	}
}

/*****************************************************************************/
/*
**	TDestroy(handle)
**	Invoke destructor on a handle
*/

TLIBAPI TVOID
TDestroy(TAPTR handle)
{
	if (handle)
	{
		TDFUNC dfunc = ((struct THandle *) handle)->thn_DestroyFunc;
		if (dfunc) (*dfunc)(handle);
	}
}

/*****************************************************************************/
/*
**	TDestroyList(list)
**	Unlink and invoke destructor on handles in a list
*/

TLIBAPI TVOID 
TDestroyList(struct TList *list)
{
	struct TNode *nextnode, *node = list->tlh_Head;
	while ((nextnode = node->tln_Succ))
	{
		TDFUNC dfunc = ((struct THandle *) node)->thn_DestroyFunc;
		TRemove(node);
		if (dfunc) (*dfunc)(node);
		node = nextnode;
	}
}

/*****************************************************************************/
/*
**	modinst = TNewInstance(mod, possize, negsize)
**	Get module instance copy
*/

TLIBAPI TAPTR
TNewInstance(TAPTR mod, TUINT possize, TUINT negsize)
{
	TAPTR exec = TGetExecBase(mod);
	TAPTR inst = TExecAlloc(exec, TNULL, possize + negsize);
	if (inst)
	{
		TUINT size = TMIN(((struct TModule *) mod)->tmd_NegSize, negsize);
		inst = (TINT8 *) inst + negsize;
		if (size > 0)
		{
			TExecCopyMem(exec, (TINT8 *) mod - size, (TINT8 *) inst - size,
				size);
		}
		size = TMIN(((struct TModule *) mod)->tmd_PosSize, possize);
		TExecCopyMem(exec, mod, inst, size);
		((struct TModule *) inst)->tmd_PosSize = possize;
		((struct TModule *) inst)->tmd_NegSize = negsize;
		((struct TModule *) inst)->tmd_InitTask = TExecFindTask(exec, TNULL);
	}
	return inst;
}

/*****************************************************************************/
/*
**	TFreeInstance(mod)
**	Free module instance
*/

TLIBAPI TVOID
TFreeInstance(TAPTR mod)
{
	TAPTR exec = TGetExecBase(mod);
	TExecFree(exec, (TINT8 *) mod - ((struct TModule *) mod)->tmd_NegSize);
}

/*****************************************************************************/
/*
**	TInitVectors(mod, vectors, num)
**	Init module vectors
*/

TLIBAPI TVOID
TInitVectors(TAPTR mod, const TAPTR *vectors, TUINT numv)
{
	TAPTR *vecp = (TAPTR *) mod;
	while (numv--)
	{
		*(--vecp) = *vectors++;
	}
}

/*****************************************************************************/
/*
**	complete = TForEachTag(taglist, func, data)
**	TBOOL foreachfunc(TAPTR data, TTAG tag)
*/

TLIBAPI TBOOL
TForEachTag(struct TTagItem *taglist, TTAGFOREACHFUNC func, TAPTR data)
{
	TBOOL complete = TTRUE;

	while (taglist && complete)
	{
		switch ((TUINT) taglist->tti_Tag)
		{
			case TTAG_DONE:
				goto done;
			
			case TTAG_MORE:
				taglist = (struct TTagItem *) taglist->tti_Value;
				break;
				
			case TTAG_SKIP:
				taglist += 1 + (TINT) taglist->tti_Value;
				break;
									
			case TTAG_GOSUB:
				complete = TForEachTag((struct TTagItem *) taglist->tti_Value,
					func, data);
				taglist++;
				break;

			default:
				complete = (*func)(data, taglist);

			case TTAG_IGNORE:
				taglist++;
				break;
		}
	}

done:
	return complete;
}

/*****************************************************************************/
/*
**	tag = TGetTag(taglist, tag, defvalue)
**	Get tag value
*/

TLIBAPI TTAG
TGetTag(struct TTagItem *taglist, TUINT tag, TTAG defvalue)
{
	TUINT listtag;
	while (taglist)
	{
		listtag = taglist->tti_Tag;
		switch (listtag)
		{
			case TTAG_DONE:
				return defvalue;
			
			case TTAG_MORE:
				taglist = (struct TTagItem *) taglist->tti_Value;
				break;
				
			case TTAG_SKIP:
				taglist += 1 + (TINT) taglist->tti_Value;
				break;
									
			case TTAG_GOSUB:
			{
				TTAG res = TGetTag((struct TTagItem *) taglist->tti_Value, tag, 
					defvalue);
				if (res != defvalue) return res;
				taglist++;
				break;
			}

			default:
				if (tag == listtag) return taglist->tti_Value;

			case TTAG_IGNORE:
				taglist++;
				break;
		}
	}
	return defvalue;
}

/*****************************************************************************/
/*
**	handle = TFindHandle(list, name)
**	Find named handle
*/

TLIBAPI struct THandle *
TFindHandle(struct TList *list, TSTRPTR name)
{
	struct TNode *nnode, *node;
	for (node = list->tlh_Head; (nnode = node->tln_Succ); node = nnode)
	{
		TSTRPTR s1 = (TSTRPTR) ((struct THandle *) node)->thn_Data;
		if (s1 && name)
		{
			TSTRPTR s2 = name;
			TINT8 a;
			while ((a = *s1++) == *s2++)
			{
				if (a == 0)
				{
					return (struct THandle *) node;
				}
			}
		}
		else if (s1 == TNULL && name == TNULL)
		{
			return (struct THandle *) node;
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: teklib.c,v $
**	Revision 1.7  2005/09/13 02:41:00  tmueller
**	updated copyright reference
**	
**	Revision 1.6  2004/04/18 13:57:53  tmueller
**	arguments in parseargv, atomdata, gettag changed from TAPTR to TTAG
**	
**	Revision 1.5  2004/02/07 04:58:15  tmueller
**	Time support functions (add, sub, cmp) removed from the link library
**	
**	Revision 1.4  2004/01/31 11:30:52  tmueller
**	Minor optimizations to TDestroy() and TDestroyList()
**	
**	Revision 1.3  2003/12/12 15:02:17  tmueller
**	Added and fixed some comments
**	
**	Revision 1.2  2003/12/12 03:46:20  tmueller
**	Return values do no longer try to emulate the crippled behavior of macros
**	
**	Revision 1.1.1.1  2003/12/11 07:18:19  tmueller
**	Krypton import
*/
