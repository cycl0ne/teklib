
/*
**	$Id: tasktree.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/exec/tests/tasktree.c - Exec module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>

#define CHILDSPERNODE	5
#define MAXTASKS		200
#define CHILDSIGDONE	0x80000000

void childfunc(struct TTask *task);

struct TExecBase *TExecBase;
struct THook TaskHook;


static THOOKENTRY TTAG
task_dispatch(struct THook *hook, TAPTR task, TTAG msg)
{
	switch (msg)
	{
		case TMSG_INITTASK:
			return TTRUE;
		case TMSG_RUNTASK:
			childfunc(task);
			break;
	}
	return 0;
}


void childfunc(struct TTask *task)
{
	TAPTR atom = TLockAtom("global.atom", TATOMF_NAME);
	if (atom)
	{
		TUINT *taskcount = (TUINT *) TGetAtomData(atom);
		TAPTR childs[CHILDSPERNODE];
		TUINT i;

		for (i = 0; i < CHILDSPERNODE; ++i)
		{
			if (*taskcount < MAXTASKS)
			{
				childs[i] = TCreateTask(&TaskHook, TNULL);
				(*taskcount)++;
			}
			else
				childs[i] = TNULL;
		}

		if (*taskcount >= MAXTASKS)
		{
			TAPTR maintask = TFindTask(TTASKNAME_ENTRY);
			TSignal(maintask, CHILDSIGDONE);
		}

		TUnlockAtom(atom, 0);

		TWait(TTASK_SIG_ABORT);

		for (i = 0; i < CHILDSPERNODE; ++i)
		{
			if (childs[i])
			{
				TSignal(childs[i], TTASK_SIG_ABORT);
				TDestroy(childs[i]);
			}
		}
	}
}


void TEKMain(struct TTask *task)
{
	TInitHook(&TaskHook, task_dispatch, TNULL);
	TAPTR subtask, atom;
	TExecBase = TGetExecBase(task);
	atom = TLockAtom("global.atom", TATOMF_CREATE | TATOMF_NAME);
	if (atom)
	{
		TUINT taskcount = 1;

		TSetAtomData(atom, (TTAG) &taskcount);

		TUnlockAtom(atom, TATOMF_KEEP);

		printf("creating a tree of %d tasks, with %d childs per node\n",
			MAXTASKS, CHILDSPERNODE);

		subtask = TCreateTask(&TaskHook, TNULL);
		if (subtask)
		{
			TWait(CHILDSIGDONE);
			TSignal(subtask, TTASK_SIG_ABORT);
			TDestroy(subtask);
		}

		TLockAtom(atom, TATOMF_DESTROY);

		printf("done. tasks created: %d\n", taskcount);
	}
}
