
/*
**	$Id: sorttest.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/util/tests/sorttest.c - Util module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

TAPTR TExecBase;
TAPTR TUtilBase;

#define NUMNODES 10
#define addnode(l, h, n) (h)->thn_Name = (n), TAddTail(l, &(h)->thn_Node)

/*****************************************************************************/
/*
**	Demonstrate alphabetical sorting of named handles in a list
*/

static TTAG THOOKENTRY
cmpfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct THandle **op = obj;
	return TStrCmp(op[0]->thn_Name, op[1]->thn_Name);
}

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		TINT i;
		struct THook sorthook;
		struct TNode *next, *node;
		struct THandle *refarray[NUMNODES], **rp = refarray;

		/* Setup list of handles to be sorted alphabetically: */

		struct THandle handles[NUMNODES];
		struct TList list;
		TInitList(&list);

		addnode(&list, &handles[0], "eins");
		addnode(&list, &handles[1], "zwei");
		addnode(&list, &handles[2], "drei");
		addnode(&list, &handles[3], "vier");
		addnode(&list, &handles[4], "fuenf");
		addnode(&list, &handles[5], "sechs");
		addnode(&list, &handles[6], "sieben");
		addnode(&list, &handles[7], "acht");
		addnode(&list, &handles[8], "neun");
		addnode(&list, &handles[9], "zehn");

		/* Setup reference array: */

		for (node = list.tlh_Head; (next = node->tln_Succ); node = next)
			*rp++ = (struct THandle *) node;

		/* Init callback hook for comparison: */

		TInitHook(&sorthook, cmpfunc, TNULL);

		/* Sort: */

		THeapSort((TAPTR) refarray, NUMNODES, &sorthook);

		/* The list can be accessed in sorted order now using the
		reference array. But it may be reordered also: */

		for (i = 0; i < NUMNODES; ++i)
		{
			TRemove((struct TNode *) refarray[i]);
			TAddTail(&list, (struct TNode *) refarray[i]);
		}

		/* Iterate list (now in alphabetical order): */

		for (node = list.tlh_Head; (next = node->tln_Succ); node = next)
			puts(((struct THandle *) node)->thn_Name);

		TCloseModule(TUtilBase);
	}
}
