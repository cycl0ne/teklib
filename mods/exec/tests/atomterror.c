
/*
**	atomterror.c
**	torture test for shared/exclusive use of a list,
**	which is protected with a named atom.
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TTimeBase;
TAPTR TimeReq;
TINT seed = 123;

struct global
{
	TLIST list;
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

TTASKENTRY TVOID subfunc(TAPTR task)
{
	struct global *g;
	TNODE *n, *t;
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
			t = TSeekNode(TFirstNode(&g->list), r);

			if (mode != TATOMF_SHARED)
			{
				/* if the lock was exclusive, insert or remove a node here */

				if (((seed = TGetRand(seed)) % 2))
				{
					n = TAlloc(g->taskmmu, sizeof(TNODE));
					if (n)
					{
						if (t)
						{
							TInsert(&g->list, n, t);
						}
						else
						{
							TAddTail(&g->list, n);
						}
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
					{
						n = TRemHead(&g->list);
					}

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


/* 
**	create a named atom, i.e. a global resource for the 
**	data structure, and then spawn some threads of the above.
*/

#define NUMTASKS 70

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TTimeBase = TOpenModule("time", 0, TNULL);

	if (TUtilBase && TTimeBase)
	{
		TimeReq = TAllocTimeRequest(TNULL);
		if (TimeReq)
		{
			struct global g;
			TTIME t0, t1;
			TAPTR tasks[NUMTASKS];
			TINT i;
			TAPTR listatom;

			listatom = TLockAtom("list.atom", TATOMF_NAME | TATOMF_CREATE);
			if (listatom)
			{
				TFLOAT s;
				TQueryTime(TimeReq, &t0);

				g.taskmmu = TGetTaskMMU(task);
				g.numnodes = 0;
				g.maxnodes = 0;
				
				TInitList(&g.list);
			
				/* associate datapointer to the atom, and unlock. */
				TSetAtomData(listatom, (TTAG) &g);
				TUnlockAtom(listatom, TATOMF_KEEP);
			
				/* create threads */	
				for (i = 0; i < NUMTASKS; ++i)
				{
					tasks[i] = TCreateTask(subfunc, TNULL, TNULL);
				}
				
				/* destroy threads */	
				for (i = 0; i < NUMTASKS; ++i)
				{
					TDestroy(tasks[i]);
				}
		
				/* destroy atom */	
				TLockAtom(listatom, TATOMF_DESTROY);

				TQueryTime(TimeReq, &t1);
				TSubTime(&t1, &t0);
				s = (TFLOAT) t1.ttm_USec / 1000000 + t1.ttm_Sec;
				printf("all done. time elapsed: %.3fs\n", s);
			
			} else printf("*** failed to create atom\n");

			TFreeTimeRequest(TimeReq);
		
		} else printf("*** failed to get timerequest\n");
		
	} else printf("*** failed to open modules\n");
	
	TCloseModule(TTimeBase);
	TCloseModule(TUtilBase);
}
