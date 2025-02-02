
/*
**	$Id: memtest.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/exec/tests/memtest.c - Exec module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

TAPTR TExecBase;
TUINT seed = 123;

void subfunc(struct TTask *task)
{
	TAPTR mmu = TGetTaskData(task);
	TAPTR TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		TINT i, r, s;
		TUINT *allocs[100];
		TFillMem(allocs, 100 * sizeof(TUINT *), 0);

		for (i = 0; i < 1000; ++i)
		{
			r = (seed = TGetRand(seed)) % 100;
			if (allocs[r])
			{
				if (*(allocs[r]) != (TUINT)r)	/* check validity */
				{
					TDBPRINTF(20,("memory corrupt!\n"));
					TDBFATAL();
				}
				TFree(allocs[r]);
				allocs[r] = TNULL;
			}
			else
			{
				s = (seed = TGetRand(seed)) % 100;
				allocs[r] = TAlloc(mmu, s);
				if (allocs[r]) *(allocs[r]) = r;
			}
		}
		TCloseModule(TUtilBase);
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


#define NUMTASKS 100

void TEKMain(struct TTask *task)
{
	TAPTR mmu;
	struct TTask *tasks[NUMTASKS];
	TINT i;
	TTAGITEM tasktags[2];
	TTAGITEM memtags[3];
	struct THook taskhook;
	TInitHook(&taskhook, task_dispatch, TNULL);

	TExecBase = TGetExecBase(task);

	/* create a task-safe memory manager in a static block of memory */

	memtags[0].tti_Tag = TMem_StaticSize;
	memtags[0].tti_Value = (TTAG) 200000;
	memtags[1].tti_Tag = TMem_LowFrag;
	memtags[1].tti_Value = (TTAG) TFALSE;
	memtags[2].tti_Tag = TTAG_DONE;

	mmu = TCreateMemManager(TNULL, TMMT_Static | TMMT_TaskSafe, memtags);

	/* run test tasks */

	tasktags[0].tti_Tag = TTask_UserData;
	tasktags[0].tti_Value = (TTAG) mmu;
	tasktags[1].tti_Tag = TTAG_DONE;

	for (i = 0; i < NUMTASKS; ++i)
		tasks[i] = TCreateTask(&taskhook, tasktags);

	printf("tasks running\n");

	for (i = 0; i < NUMTASKS; ++i)
		TDestroy((struct THandle *) tasks[i]);

	printf("all done\n");

	TDestroy(mmu);
}
