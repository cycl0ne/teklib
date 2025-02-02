
/*
**	$Id: mouseover.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/visual/tests/mouseover.c - Visual module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/visual.h>

enum { BACK, DRED, LRED, CLICK, FOCUS, NUMPEN };


TBOOL mouseover_init(struct TTask *task)
{
	TAPTR TExecBase = TGetExecBase(task);
	TTAGITEM initags[4];
	TAPTR visual;

	initags[0].tti_Tag = TVisual_Title; initags[0].tti_Value = (TTAG) "MouseOver";
	initags[1].tti_Tag = TVisual_PixWidth; initags[1].tti_Value = (TTAG) 110;
	initags[2].tti_Tag = TVisual_PixHeight; initags[2].tti_Value = (TTAG) 110;
	initags[3].tti_Tag = TTAG_DONE;

	visual = TExecOpenModule(TExecBase, "visual", 0, initags);
	if (visual)
	{
		TExecSetTaskData(TExecBase, task, visual);
		return TTRUE;
	}

	return TFALSE;
}


void mouseover(struct TTask *task)
{
	TAPTR TExecBase = TGetExecBase(task);
	TAPTR visual = TExecGetTaskData(TExecBase, task);

	TTAGITEM sizetags[3];
	TVPEN pentab[NUMPEN];
	TBOOL over = TFALSE, newover = TFALSE, focus = TFALSE, newfocus = TFALSE;
	TBOOL abort = TFALSE, refresh = TTRUE, button = TFALSE;
	TAPTR iport;
	TIMSG *imsg;
	TUINT ww = 110;
	TUINT wh = 110;

	sizetags[0].tti_Tag = TVisual_PixWidth;
	sizetags[0].tti_Value = (TTAG) &ww;
	sizetags[1].tti_Tag = TVisual_PixHeight;
	sizetags[1].tti_Value = (TTAG) &wh;
	sizetags[2].tti_Tag = TTAG_DONE;

	pentab[BACK] = TVisualAllocPen(visual, 0x000000);
	pentab[DRED] = TVisualAllocPen(visual, 0x660000);
	pentab[LRED] = TVisualAllocPen(visual, 0xff4444);
	pentab[CLICK] = TVisualAllocPen(visual, 0xffcc44);
	pentab[FOCUS] = TVisualAllocPen(visual, 0x880000);

	TVisualClear(visual, pentab[BACK]);

	TVisualSetInput(visual, TITYPE_NONE, TITYPE_CLOSE | TITYPE_COOKEDKEY |
		TITYPE_NEWSIZE |
		TITYPE_MOUSEOVER | TITYPE_REFRESH | TITYPE_MOUSEBUTTON | TITYPE_FOCUS);

	iport = TVisualGetPort(visual);

	do
	{
		if (newfocus != focus || refresh)
		{
			focus = newfocus;
			TVisualClear(visual, focus? pentab[FOCUS] : pentab[BACK]);
			refresh = TTRUE;
		}
		if (newover != over || refresh)
		{
			over = newover;
			TVisualFRect(visual, 10, 10, ww - 20, wh - 20,
				over? (button? pentab[CLICK] : pentab[LRED]) : pentab[DRED]);
			refresh = TFALSE;
		}

		if (TExecWait(TExecBase, TExecGetPortSignal(TExecBase, iport) | TTASK_SIG_ABORT) & TTASK_SIG_ABORT) break;

		while ((imsg = (TIMSG *) TExecGetMsg(TExecBase, iport)))
		{
			switch (imsg->timsg_Type)
			{
				case TITYPE_FOCUS:
					newfocus = imsg->timsg_Code;
					break;

				case TITYPE_MOUSEBUTTON:
					switch (imsg->timsg_Code)
					{
						case TMBCODE_LEFTDOWN:
							button |= 1;
							refresh = TTRUE;
							break;
						case TMBCODE_LEFTUP:
							button &= ~1;
							refresh = TTRUE;
							break;
					}
					break;

				case TITYPE_MOUSEOVER:
					newover = imsg->timsg_Code;
					break;

				case TITYPE_CLOSE:
					abort = TTRUE;
					break;

				case TITYPE_COOKEDKEY:
					abort = (imsg->timsg_Code == TKEYC_ESC);
					break;

				case TITYPE_NEWSIZE:
				case TITYPE_REFRESH:
					/*TVisualClear(visual, pentab[BACK]);*/
					TVisualGetAttrs(visual, sizetags);
					refresh = TTRUE;
					break;
			}
			TExecAckMsg(TExecBase, imsg);
		}

	} while (!abort);

	TExecCloseModule(TExecBase, visual);
}


/*
**	create some window instances
*/

void TEKMain(struct TTask *task)
{
	TAPTR TExecBase = TGetExecBase(task);
	TAPTR TUtilBase = TExecOpenModule(TExecBase, "util", 0, TNULL);
	if (TUtilBase)
	{
		struct TTask *tasks[20];
		TINT i;
		TINT numwindows = 4;

		TSTRPTR *args = TUtilGetArgV(TUtilBase);

		if (args)
		{
			if (args[1]) TUtilStrToI(TUtilBase, args[1], &numwindows);
			numwindows = TCLAMP(1, numwindows, 20);
		}

		for (i = 0; i < numwindows; ++i)
			tasks[i] = TExecCreateTask(TExecBase, mouseover, mouseover_init, TNULL);

		for (i = 0; i < numwindows; ++i)
			if (tasks[i])
				TDestroy(tasks[i]);

		TExecCloseModule(TExecBase, TUtilBase);

	}
	else
		printf("could not open util module.\n");
}
