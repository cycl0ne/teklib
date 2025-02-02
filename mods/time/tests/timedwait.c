
/*
**	torture test for waiting with timeout,
**	and signal integrity
*/

#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TTimeBase;
TINT seed = 123;

typedef struct
{
	TAPTR testtask;
	TAPTR tasks[16];
	TUINT signals[16];

} test;

TTASKENTRY TVOID torturetask(TAPTR task)
{
	TAPTR treq = TAllocTimeRequest(TNULL);
	if (treq)
	{
		test *t = TGetTaskData(task);
		TTIME wait = {0, 0};
		TUINT sigs, r;
	
		do
		{
			r = (seed = TGetRand(seed)) % 16;
			TSignal(t->testtask, t->signals[r]);
			wait.ttm_USec = (seed = TGetRand(seed)) % 1000;
			sigs = TWaitTime(treq, &wait, TTASK_SIG_ABORT);
	
		} while (!(sigs & TTASK_SIG_ABORT));

		TFreeTimeRequest(treq);
	}
}

TTASKENTRY TVOID testtask(TAPTR task)
{
	TAPTR treq = TAllocTimeRequest(TNULL);
	if (treq)
	{
		test t;
	
		TTAGITEM tasktags[2];
		TTIME time = { 0, 0 };
		TUINT sigs;
		TINT i;

		t.testtask = task;

		tasktags[0].tti_Tag = TTask_UserData;
		tasktags[0].tti_Value = (TTAG) &t;
		tasktags[1].tti_Tag = TTAG_DONE;
	
		for (i = 0; i < 16; ++i)
		{
			t.signals[i] = TAllocSignal(0);
			if (!t.signals[i]) tdbfatal(99);
		}
	
		for (i = 0; i < 16; ++i)
		{
			t.tasks[i] = TCreateTask(torturetask, TNULL, tasktags);
			if (!t.tasks[i]) tdbfatal(99);
		}
	
		do
		{
			time.ttm_USec = (seed = TGetRand(seed)) % 1000;
			sigs = TWaitTime(treq, &time, TTASK_SIG_ABORT);
	
		} while (!(sigs & TTASK_SIG_ABORT));
	
		for (i = 0; i < 16; ++i)
		{
			TSignal(t.tasks[i], TTASK_SIG_ABORT);
			TDestroy(t.tasks[i]);
		}
	
		for (i = 0; i < 16; ++i)
		{
			TFreeSignal(t.signals[i]);
		}
		
		TFreeTimeRequest(treq);
	}
}


#define NUMTESTS	12

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TTimeBase = TOpenModule("time", 0, TNULL);
	if (TUtilBase && TTimeBase)
	{
		TAPTR TimeReq = TAllocTimeRequest(TNULL);
		if (TimeReq)
		{
			TTIME tdelay;
			TAPTR tests[NUMTESTS];
			TINT i;
			
			for (i = 0; i < NUMTESTS; ++i)
			{
				tests[i] = TCreateTask(testtask, TNULL, TNULL);
			}
		
			tdelay.ttm_Sec = 0;
			tdelay.ttm_USec = 300000;
			TDelay(TimeReq, &tdelay);
			
			for (i = 0; i < NUMTESTS; ++i)
			{
				TSignal(tests[i], TTASK_SIG_ABORT);
				TDestroy(tests[i]);
			}

			TFreeTimeRequest(TimeReq);
			
			printf("all done\n");

		} else printf("*** failed to get timerequest\n");

	} else printf("*** could not open modules\n");

	TCloseModule(TTimeBase);
	TCloseModule(TUtilBase);
}
