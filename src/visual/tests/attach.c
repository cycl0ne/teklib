
/*
**	$Id: attach.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/visual/tests/attach.c - Visual module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/proto/visual.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TVisualBase;
TUINT seed = 123;


TBOOL childinit(struct TTask *task)
{
	TAPTR clientvis = TNULL;
	TAPTR atom = TLockAtom("global.visual", TATOMF_NAME);
	if (atom)
	{
		TAPTR visual = (TAPTR) TGetAtomData(atom);
		clientvis = TVisualAttach(visual, TNULL);
		if (clientvis)
			TSetTaskData(task, clientvis);
		TUnlockAtom(atom, 0);
	}
	return (TBOOL) (clientvis ? TTRUE : TFALSE);
}


void child(struct TTask *task)
{
	TAPTR clientvis = TGetTaskData(task);
	TINT i = 0;

	while (i < 150)
	{
		TVPEN p = TVisualAllocPen(clientvis, (seed = TGetRand(seed)) % 256);
		TVisualClear(clientvis, p);
		i++;
	}

	TCloseModule(clientvis);
}


static THOOKENTRY TTAG
task_dispatch(struct THook *hook, TAPTR task, TTAG msg)
{
	switch (msg)
	{
		case TMSG_INITTASK:
			return childinit(task);
		case TMSG_RUNTASK:
			child(task);
			break;
	}
	return 0;
}


#define NUMTASKS 50

void TEKMain(struct TTask *task)
{
	TAPTR atom;

	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (!TUtilBase) return;

	atom = TLockAtom("global.visual", TATOMF_CREATE | TATOMF_NAME);
	if (atom)
	{
		struct TTask *tasks[NUMTASKS];
		TVisualBase = TOpenModule("visual", 0, TNULL);
		if (TVisualBase)
		{
			TTAGITEM tags[3];
			TAPTR visual;

			tags[0].tti_Tag = TVisual_Width;
			tags[0].tti_Value = (TTAG) 150;
			tags[1].tti_Tag = TVisual_Height;
			tags[1].tti_Value = (TTAG) 150;
			tags[2].tti_Tag = TTAG_DONE;

			visual = TVisualOpen(TVisualBase, tags);
			if (visual)
			{
				struct THook taskhook;
				TInitHook(&taskhook, task_dispatch, TNULL);

				TSetAtomData(atom, (TTAG) visual);

				TUnlockAtom(atom, TATOMF_KEEP);

				if (visual)
				{
					TINT i;
					for (i = 0; i < NUMTASKS; ++i)
						tasks[i] = TCreateTask(&taskhook, TNULL);
					for (i = 0; i < NUMTASKS; ++i)
						TDestroy((struct THandle *) tasks[i]);
					TVisualClose(TVisualBase, visual);
				}
			}

			TCloseModule(TVisualBase);
		}
		else
			printf("*** could not open visual\n");

		TLockAtom(atom, TATOMF_DESTROY);
	}

	TCloseModule(TUtilBase);
}
