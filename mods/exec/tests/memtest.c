
/*
**	torture test:
**	multitasked allocations from a static memory manager
*/

#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

TAPTR TExecBase;
TINT seed = 123;

TTASKENTRY TVOID subfunc(TAPTR task)
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
					tdbprintf(99,"memory corrupt!\n");
					tdbfatal(99);
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


#define NUMTASKS 100

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TAPTR mmu;
	TAPTR tasks[NUMTASKS];
	TINT i;
	TTAGITEM tasktags[2];
	TTAGITEM memtags[3];
	TExecBase = TGetExecBase(task);

	/* create a task-safe memory manager in a static block of memory */

	memtags[0].tti_Tag = TMem_StaticSize;
	memtags[0].tti_Value = (TTAG) 200000;
	memtags[1].tti_Tag = TMem_LowFrag;
	memtags[1].tti_Value = (TTAG) TFALSE;
	memtags[2].tti_Tag = TTAG_DONE;

	mmu = TCreateMMU(TNULL, TMMUT_Static | TMMUT_TaskSafe, memtags);

	/* run test tasks */

	tasktags[0].tti_Tag = TTask_UserData;
	tasktags[0].tti_Value = (TTAG) mmu;
	tasktags[1].tti_Tag = TTAG_DONE;
	
	for (i = 0; i < NUMTASKS; ++i)
	{
		tasks[i] = TCreateTask(subfunc, TNULL, tasktags);
	}
	
	printf("tasks running\n");

	for (i = 0; i < NUMTASKS; ++i)
	{
		TDestroy(tasks[i]);
	}

	printf("all done\n");
	
	TDestroy(mmu);
}

