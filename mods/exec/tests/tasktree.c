
/*
**	tasktree.c
**	create a tree of child tasks
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>

#define CHILDSPERNODE	5
#define MAXTASKS		200
#define CHILDSIGDONE	0x80000000

struct TModule *TExecBase;

TTASKENTRY TVOID childfunc(TAPTR task)
{
	TAPTR atom = TExecLockAtom(TExecBase, "global.atom", TATOMF_NAME);
	if (atom)
	{
		TUINT *taskcount = (TUINT *) TExecGetAtomData(TExecBase, atom);
		TAPTR childs[CHILDSPERNODE];
		TUINT i;
			
		for (i = 0; i < CHILDSPERNODE; ++i)
		{	
			if (*taskcount < MAXTASKS)
			{
				childs[i] = TExecCreateTask(TExecBase, childfunc, TNULL, TNULL);
				(*taskcount)++;
			}
			else
			{
				childs[i] = TNULL;
			}
		}
		
		if (*taskcount >= MAXTASKS)
		{
			TAPTR maintask = TExecFindTask(TExecBase, TTASKNAME_ENTRY);
			TExecSignal(TExecBase, maintask, CHILDSIGDONE);
		}
		
		TExecUnlockAtom(TExecBase, atom, 0);
		
		TExecWait(TExecBase, TTASK_SIG_ABORT);
		
		for (i = 0; i < CHILDSPERNODE; ++i)
		{
			if (childs[i])
			{
				TExecSignal(TExecBase, childs[i], TTASK_SIG_ABORT);
				TDestroy(childs[i]);
			}
		}
	}
}


TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TAPTR subtask, atom;
	TExecBase = TGetExecBase(task);
	atom = TExecLockAtom(TExecBase, "global.atom", TATOMF_CREATE | TATOMF_NAME);
	if (atom)
	{
		TUINT taskcount = 1;

		TExecSetAtomData(TExecBase, atom, (TTAG) &taskcount);

		TExecUnlockAtom(TExecBase, atom, TATOMF_KEEP);
	
		printf("creating a tree of %d tasks, with %d childs per node\n", MAXTASKS, CHILDSPERNODE);
	
		subtask = TExecCreateTask(TExecBase, childfunc, TNULL, TNULL);
		if (subtask)
		{
			TExecWait(TExecBase, CHILDSIGDONE);
			TExecSignal(TExecBase, subtask, TTASK_SIG_ABORT);
			TDestroy(subtask);
		}

		TExecLockAtom(TExecBase, atom, TATOMF_DESTROY);
	
		printf("done. tasks created: %d\n", taskcount);
	}
}
