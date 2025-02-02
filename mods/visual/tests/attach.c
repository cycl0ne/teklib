
/*
**	examples/attach.c
**	torture test for the visual attach/detach logic
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/proto/visual.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TINT seed = 123;

TTASKENTRY TBOOL childinit(TAPTR task)
{
	TAPTR clientvis = TNULL;
	TAPTR atom = TLockAtom("global.visual", TATOMF_NAME);
	if (atom)
	{
		TAPTR visual = (TAPTR) TGetAtomData(atom);
		clientvis = TVisualAttach(visual, TNULL);
		if (clientvis)
		{
			TSetTaskData(task, clientvis);
		}
		TUnlockAtom(atom, 0);
	}
	return (TBOOL) (clientvis ? TTRUE : TFALSE);
}


TTASKENTRY TVOID child(TAPTR task)
{
	TAPTR clientvis = TGetTaskData(task);
	TINT i = 0;

	while (i < 150)
	{
		TVPEN p = TVisualAllocPen(clientvis, (seed = TGetRand(seed)) % 256);
		TVisualClear(clientvis, p);
		TVisualFlush(clientvis);
		i++;
	}

	TCloseModule(clientvis);
}


#define NUMTASKS 50

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TAPTR atom;

	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (!TUtilBase) return;

	atom = TLockAtom("global.visual", TATOMF_CREATE | TATOMF_NAME);
	if (atom)
	{
		TAPTR tasks[NUMTASKS];
		TINT i;
		TTAGITEM tags[3];
		TAPTR visual;

		tags[0].tti_Tag = TVisual_PixWidth; tags[0].tti_Value = (TTAG) 150;
		tags[1].tti_Tag = TVisual_PixHeight; tags[1].tti_Value = (TTAG) 150;
		tags[2].tti_Tag = TTAG_DONE;

		visual = TOpenModule("visual", 0, tags);
		if (visual)
		{
			TSetAtomData(atom, (TTAG) visual);

			TUnlockAtom(atom, TATOMF_KEEP);

			if (visual)
			{
				for (i = 0; i < NUMTASKS; ++i)
				{
					tasks[i] = TCreateTask(child, childinit, TNULL);
				}

				TCloseModule(visual);

				for (i = 0; i < NUMTASKS; ++i)
				{
					TDestroy(tasks[i]);
				}
			}

			TLockAtom(atom, TATOMF_DESTROY);
		}
		else printf("*** could not open visual\n");
	}

	TCloseModule(TUtilBase);
}
