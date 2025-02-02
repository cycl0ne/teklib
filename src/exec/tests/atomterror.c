
/*
**	$Id: atomterror.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/exec/tests/atomterror.c - Exec module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/mod/time.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TUINT seed = 123;

struct global
{
	struct TList list;
	TAPTR taskmmu;
	TUINT numnodes;
	TUINT maxnodes;
};

/*
**	in this thread, rock over a list in either shared or exclusive mode
**	(depending on random numbers). if locked shared, the list is only
**	read, otherwise it is modified also. the atom must ensure list
**	integrity at any time.
*/

void subfunc(struct TTask *task)
{
	struct global *g;
	struct TNode *n, *t;
	TINT i, r;
	TUINT mode;
	TAPTR listatom;

	for (i = 0; i < 500; ++i)
	{
		mode = ((seed = TGetRand(seed)) % 2) * TATOMF_SHARED;

		listatom = TLockAtom("list.atom", TATOMF_NAME | mode);
		if (listatom)
		{
			g = (struct global *) TGetAtomData(listatom);

			/* seek over the list to a random position */

			r = g->numnodes? (seed = TGetRand(seed)) % g->numnodes : 0;
			t = TSeekList(TFIRSTNODE(&g->list), r);

			if (mode != TATOMF_SHARED)
			{
				/* if the lock was exclusive, insert or remove a node here */

				if (((seed = TGetRand(seed)) % 2))
				{
					n = TAlloc(g->taskmmu, sizeof(struct TNode));
					if (n)
					{
						if (t)
							TInsert(&g->list, n, t);
						else
							TAddTail(&g->list, n);
						g->numnodes++;
						if (g->numnodes > g->maxnodes) g->maxnodes = g->numnodes;
					}
				}
				else
				{
					/* unlink and free that node */

					if (t)
					{
						TRemove(t);
						n = t;
					}
					else
						n = TRemHead(&g->list);

					if (n)
					{
						TFree(n);
						g->numnodes--;
					}
				}
			}
			TUnlockAtom(listatom, TATOMF_KEEP);
		}
	}
}


static THOOKENTRY TTAG
task_dispatch(struct THook *hook, TAPTR task, TTAG msg)
{
	switch (msg)
	{
		case TMSG_INITTASK:
			return TTRUE;
		case TMSG_RUNTASK:
			subfunc(task);
			break;
	}
	return 0;
}


/*
**	create a named atom, i.e. a global resource for the
**	data structure, and then spawn some threads of the above.
*/

#define NUMTASKS 70

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		struct global g;
		TTIME t0, t1;
		struct TTask *tasks[NUMTASKS];
		TINT i;
		TAPTR listatom;

		listatom = TLockAtom("list.atom", TATOMF_NAME | TATOMF_CREATE);
		if (listatom)
		{
			TINT sec, usec;
			TFLOAT s;
			struct THook taskhook;
			TInitHook(&taskhook, task_dispatch, TNULL);

			TGetSystemTime(&t0);

			g.taskmmu = TGetTaskMemManager(task);
			g.numnodes = 0;
			g.maxnodes = 0;

			TInitList(&g.list);

			/* associate datapointer to the atom, and unlock. */
			TSetAtomData(listatom, (TTAG) &g);
			TUnlockAtom(listatom, TATOMF_KEEP);

			/* create threads */
			for (i = 0; i < NUMTASKS; ++i)
				tasks[i] = TCreateTask(&taskhook, TNULL);

			/* destroy threads */
			for (i = 0; i < NUMTASKS; ++i)
				TDestroy((struct THandle *) tasks[i]);

			/* destroy atom */
			TLockAtom(listatom, TATOMF_DESTROY);

			TGetSystemTime(&t1);
			TSubTime(&t1, &t0);
			TExtractTime(&t1, TNULL, &sec, &usec);
			s = (TFLOAT) sec + usec * 0.000001;
			printf("all done. time elapsed: %.3fs\n", s);

		}
		else
			printf("*** failed to create atom\n");
	}
	else
		printf("*** failed to open modules\n");

	TCloseModule(TUtilBase);
}

