
/*
**	$Id: timedwait.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/time/tests/timedwait.c - Time module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

#define MAXTESTS	12
#define MAXTASKS	16

#define TMPL "-t=NUMTASKS/N,-n=NUMTESTS/N,-r=NUMRUNS/N,-v=VERBOSE/S,-h=HELP/S"
enum { ARG_NUMTASKS, ARG_NUMTESTS, ARG_NUMRUNS, ARG_VERBOSE, ARG_HELP, ARG_NUM };

TTAG args[ARG_NUM];
TINT numtasks = 16;
TINT numtests = 12;
TINT numruns = 1;

TUINT seed = 123;

TAPTR TExecBase;
TAPTR TUtilBase;

typedef struct
{
	TAPTR testtask;
	struct TTask *tasks[MAXTASKS];
	TUINT signals[MAXTASKS];

} test;

void torturefunc(struct TTask *task)
{
	test *t = TGetTaskData(task);
	TTIME wait = { 0 };
	TUINT sigs, r;

	do
	{
		r = (seed = TGetRand(seed)) % numtasks;
		TSignal(t->testtask, t->signals[r]);
		TCreateTime(&wait, 0, 0, (seed = TGetRand(seed)) % 1000);
		sigs = TWaitTime(&wait, TTASK_SIG_ABORT);

	} while (!(sigs & TTASK_SIG_ABORT));
}

static THOOKENTRY TTAG
torture_dispatch(struct THook *hook, TAPTR task, TTAG msg)
{
	switch (msg)
	{
		case TMSG_INITTASK:
			return TTRUE;
		case TMSG_RUNTASK:
			torturefunc(task);
			break;
	}
	return 0;
}

void testfunc(struct TTask *task)
{
	test t;
	struct THook taskhook;
	TTAGITEM tasktags[2];
	TTIME time = { 0 };
	TUINT sigs;
	TINT i;

	TInitHook(&taskhook, torture_dispatch, TNULL);

	t.testtask = task;

	tasktags[0].tti_Tag = TTask_UserData;
	tasktags[0].tti_Value = (TTAG) &t;
	tasktags[1].tti_Tag = TTAG_DONE;

	for (i = 0; i < numtasks; ++i)
		t.signals[i] = TAllocSignal(0);

	for (i = 0; i < numtasks; ++i)
		t.tasks[i] = TCreateTask(&taskhook, tasktags);

	do
	{
		TCreateTime(&time, 0, 0, (seed = TGetRand(seed)) % 1000);
		sigs = TWaitTime(&time, TTASK_SIG_ABORT);

	} while (!(sigs & TTASK_SIG_ABORT));

	for (i = 0; i < numtasks; ++i)
	{
		TSignal(t.tasks[i], TTASK_SIG_ABORT);
		TDestroy((struct THandle *) t.tasks[i]);
	}

	for (i = 0; i < numtasks; ++i)
		TFreeSignal(t.signals[i]);

}

static THOOKENTRY TTAG
test_dispatch(struct THook *hook, TAPTR task, TTAG msg)
{
	switch (msg)
	{
		case TMSG_INITTASK:
			return TTRUE;
		case TMSG_RUNTASK:
			testfunc(task);
			break;
	}
	return 0;
}

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		TAPTR argh;
		args[ARG_NUMTASKS] = (TTAG) &numtasks;
		args[ARG_NUMTESTS] = (TTAG) &numtests;
		args[ARG_NUMRUNS] = (TTAG) &numruns;
		args[ARG_VERBOSE] = (TTAG) TFALSE;
		args[ARG_HELP] = (TTAG) TFALSE;
		argh = TParseArgV(TMPL, TGetArgV() + 1, args);
		if (argh && !args[ARG_HELP])
		{
			TAPTR TimeReq = TAllocTimeRequest(TNULL);
			if (TimeReq)
			{
				TTIME tdelay;
				TAPTR tests[MAXTESTS];
				TINT i, r;
				struct THook taskhook;
				TInitHook(&taskhook, test_dispatch, TNULL);

				#if 0
				TTIME t;
				GetSystemTime(TimeReq, &t);
				TExtractTime(&t, 0, 0, (TINT *) &seed);
				#else
				seed = 11111;
				#endif

				numtasks = *(TINT *) args[ARG_NUMTASKS];
				numtests = *(TINT *) args[ARG_NUMTESTS];
				numruns = *(TINT *) args[ARG_NUMRUNS];

				if (args[ARG_VERBOSE])
				{
					printf("numtasks: %d\n", numtasks);
					printf("numtests: %d\n", numtests);
					printf("numruns: %d\n", numruns);
				}

				for (r = 0; r < numruns; ++r)
				{
					if (args[ARG_VERBOSE])
						printf("run %d\n", r + 1);

					for (i = 0; i < numtests; ++i)
						tests[i] = TCreateTask(&taskhook, TNULL);

					TCreateTime(&tdelay, 0, 0, 50000);
					TWaitTime(&tdelay, 0);

					for (i = 0; i < numtests; ++i)
					{
						TSignal(tests[i], TTASK_SIG_ABORT);
						TDestroy(tests[i]);
					}
				}

				TFreeTimeRequest(TimeReq);
			}
			else
				printf("*** failed to get timerequest\n");
		}
		else
			printf("usage: %s\n", TMPL);
		TDestroy(argh);
	}
	else
		printf("*** could not open modules\n");

	TCloseModule(TUtilBase);
}
