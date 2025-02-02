
/*
**	$Id: util_searchsort.c,v 1.2 2006/09/10 14:39:32 tmueller Exp $
**	teklib/src/util/util_searchsort.c - Searching and sorting utilities
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "util_mod.h"

/*****************************************************************************/
/*
**	Heapsort via reference array:
**		- can sort any kind of data structure
**		- no recursion, no stack impact
**		- performance insensitive to initial array order
*/

EXPORT TBOOL
util_heapsort(struct TUtilBase *util, TTAG *refarray, TSIZE length,
	struct THook *hook)
{
	TSIZE i = length / 2 - 1;
	TTAG obj[2], tmp[2];

	if (!refarray || length < 2)
		return TFALSE;

	do
	{
		TSIZE k = i;
		tmp[0] = refarray[k];
		while (k < length / 2)
		{
			TSIZE j = k + k + 1;
			if (j < length - 1)
			{
				obj[0] = refarray[j + 1];
				obj[1] = refarray[j];
				if (TCALLHOOKPKT(hook, obj, TMSG_COMPAREKEYS) > 0)
					++j;
			}
			tmp[1] = refarray[j];
			if (TCALLHOOKPKT(hook, tmp, TMSG_COMPAREKEYS) >= 0)
				break;
			refarray[k] = refarray[j];
			k = j;
		}
		refarray[k] = tmp[0];
	} while (i-- > 0);

	while (--length > 0)
	{
		TSIZE k = 0;
		tmp[0] = refarray[0];
		refarray[0] = refarray[length];
		refarray[length] = tmp[0];
		tmp[0] = refarray[k];
		while (k < length / 2)
		{
			TSIZE j = k + k + 1;
			if (j < length - 1)
			{
				obj[0] = refarray[j + 1];
				obj[1] = refarray[j];
				if (TCALLHOOKPKT(hook, obj, TMSG_COMPAREKEYS) > 0)
					++j;
			}
			tmp[1] = refarray[j];
			if (TCALLHOOKPKT(hook, tmp, TMSG_COMPAREKEYS) >= 0)
				break;
			refarray[k] = refarray[j];
			k = j;
		}
		refarray[k] = tmp[0];
	}

	return TTRUE;
}

/*****************************************************************************/
/*
**	node = util_seeknode(utilbase, node, steps)
**	Starting at a given node, seek in a list by a given number of steps, and
**	return the node reached, or TNULL if seeked past end or before start
*/

EXPORT struct TNode *
util_seeknode(struct TUtilBase *util, struct TNode *node, TINTPTR steps)
{
	if (node)
	{
		struct TNode *nextnode;

		if (!node->tln_Succ) return TNULL;
		if (!node->tln_Pred) return TNULL;

		if (steps > 0)
		{
			while ((nextnode = node->tln_Succ))
			{
				if (steps-- == 0)
					return node;
				node = nextnode;
			}
			return TNULL;
		}
		else if (steps < 0)
		{
			while ((nextnode = node->tln_Pred))
			{
				if (steps++ == 0)
					return node;
				node = nextnode;
			}
			return TNULL;
		}
	}
	return node;
}
