
/*
**	$Id: bashing.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/visual/tests/bashing.c - Visual module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <math.h>
#include <stdio.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/proto/visual.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TUINT seed = 123;

/*****************************************************************************/

#define NUMLINES	40

struct efxdata
{
	TINT x, y;
	TVPEN pen, backpen, whitepen, blackpen;
	TAPTR visual;
	TINT framerate;
	TTAGITEM drawtags[200];
};

/*****************************************************************************/
/*
**	Thread init function
*/

TBOOL efxinitfunc(struct TTask *task)
{
	TAPTR mmu = TGetTaskMemManager(task);
	struct efxdata *initdata = TGetTaskData(task);
	struct efxdata *data = TAlloc(mmu, sizeof(struct efxdata));
	if (data)
	{
		TCopyMem(initdata, data, sizeof(struct efxdata));
		data->visual = TVisualAttach(initdata->visual, TNULL);
		if (data->visual)
		{
			TSetTaskData(task, data);
			return TTRUE;
		}
	}
	return TFALSE;
}

static TFLOAT frand(void)
{
	union { TUINT i; TFLOAT f; } bits;
	bits.i = ((seed = TGetRand(seed)) & 0x007fffff) | 0x40000000;
	return bits.f - 3.0f;
}

/*****************************************************************************/
/*
**	Effect thread
*/

void efxfunc(struct TTask *task)
{
	struct efxdata *data = TGetTaskData(task);
	TINT i, j;
	TFLOAT s[6], ss[6], ds[6], dss[6];
	TTIME t0, t1;
	TFLOAT fps = 1.0;
	TINT diffus;
	TUINT signals;
	TCHR buf[30];

	TINT fw,fh;
	TTAGITEM tags[3];

	tags[0].tti_Tag = TVisual_FontWidth;
	tags[0].tti_Value = (TTAG) &fw;
	tags[1].tti_Tag = TVisual_FontHeight;
	tags[1].tti_Value = (TTAG) &fh;
	tags[2].tti_Tag = TTAG_DONE;

	TVisualGetAttrs(data->visual, tags);

	data->framerate = (seed = TGetRand(seed)) % 40 + 10;

	for (i = 0; i < 6; ++i)
	{
		s[i] = 0.0;
		ds[i] = frand() * 0.3f + 0.03f;
		dss[i] = frand() * 0.5f + 0.07f;
	}

	do
	{
		TINT is, ius;
		TFLOAT sec;
		TTAGITEM *tp;

		TGetSystemTime(&t0);
		tp = data->drawtags;

		tp->tti_Tag = TVisualDraw_FgPen;
		(tp++)->tti_Value = data->backpen;
		tp->tti_Tag = TVisualDraw_X0;
		(tp++)->tti_Value = data->x;
		tp->tti_Tag = TVisualDraw_Y0;
		(tp++)->tti_Value = data->y;
		tp->tti_Tag = TVisualDraw_X1;
		(tp++)->tti_Value = 200;
		tp->tti_Tag = TVisualDraw_Y1;
		(tp++)->tti_Value = 200;
		tp->tti_Tag = TVisualDraw_Command;
		(tp++)->tti_Value = TVCMD_FRECT;
		tp->tti_Tag = TVisualDraw_FgPen;
		(tp++)->tti_Value = data->pen;

		for (i = 0; i < 6; ++i) ss[i] = s[i];

		for (j = 0; j < NUMLINES + 1; ++j)
		{
			tp->tti_Tag = TVisualDraw_NewX;
			(tp++)->tti_Value = (TINT) ((sin(ss[0]) + sin(ss[1]) + sin(ss[2])) *
				200 / 6 + 200 / 2 + data->x);
			tp->tti_Tag = TVisualDraw_NewY;
			(tp++)->tti_Value = (TINT) ((sin(ss[3]) + sin(ss[4]) + sin(ss[5])) *
				200 / 6 + 200 / 2 + data->y);
			if (j > 0)
			{
				tp->tti_Tag = TVisualDraw_Command;
				(tp++)->tti_Value = TVCMD_LINE;
			}

			for (i = 0; i < 6; ++i)
			{
				ss[i] += dss[i];
				if (ss[i] > 2*TPI) ss[i] -= (TFLOAT) (2*TPI);
			}
		}

		tp->tti_Tag = TTAG_DONE;

		TVisualDrawTags(data->visual, data->drawtags);

		for (i = 0; i < 6; ++i)
		{
			s[i] += ds[i];
			if (s[i] > 2*TPI) s[i] -= (TFLOAT) (2*TPI);
		}

		sprintf(buf, "FPS: %d/%d/%d%% ", (TINT) fps, data->framerate,
			(TINT) (fps * 100 / data->framerate));

		TVisualText(data->visual, data->x, data->y,
			buf, TStrLen(buf), data->whitepen);

		/* calculate elapsed seconds */
		TGetSystemTime(&t1);
		TSubTime(&t1, &t0);
		TExtractTime(&t1, TNULL, &is, &ius);
		sec = is + (TFLOAT) ius * 0.000001;

		/* calculate wait time */
		diffus = (TFLOAT) 1000000 / data->framerate - sec;
		TCreateTime(&t1, 0, 0, diffus);
		signals = TWaitTime(&t1, TTASK_SIG_ABORT);

		/* measure effective framerate */
		TGetSystemTime(&t1);
		TSubTime(&t1, &t0);
		TExtractTime(&t1, TNULL, &is, &ius);
		sec = is + (TFLOAT) ius * 0.000001;

		fps = 1 / sec;

	} while (!(signals & TTASK_SIG_ABORT));

	TCloseModule(data->visual);
	TFree(data);
}

static THOOKENTRY TTAG
task_dispatch(struct THook *hook, TAPTR task, TTAG msg)
{
	switch (msg)
	{
		case TMSG_INITTASK:
			return efxinitfunc(task);
		case TMSG_RUNTASK:
			efxfunc(task);
			break;
	}
	return 0;
}

/*****************************************************************************/
/*
**	Main Program
*/

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);

	TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		TAPTR vismod = TOpenModule("visual", 0, TNULL);
		if (vismod)
		{
			TAPTR v;
			TTAGITEM vistags[4];

			vistags[0].tti_Tag = TVisual_Width;
			vistags[0].tti_Value = (TTAG) 680;
			vistags[1].tti_Tag = TVisual_Height;
			vistags[1].tti_Value = (TTAG) 460;
			vistags[2].tti_Tag = TVisual_Title;
			vistags[2].tti_Value = (TTAG) "Visual multibashing";
			vistags[3].tti_Tag = TTAG_DONE;

			v = TVisualOpen(vismod, vistags);
			if (v)
			{
				TIMSG *imsg;
				TBOOL abort = TFALSE;
				TAPTR iport;
				TINT x, y, i = 0;
				TVPEN pentab[8];
				struct TTask *tasks[6] = {TNULL, TNULL, TNULL, TNULL, TNULL, TNULL};
				struct THook taskhook;

				TInitHook(&taskhook, task_dispatch, TNULL);

				pentab[0] = TVisualAllocPen(v, 0xffffff);
				pentab[1] = TVisualAllocPen(v, 0xff00ff);
				pentab[2] = TVisualAllocPen(v, 0xff0000);
				pentab[3] = TVisualAllocPen(v, 0x0000ff);
				pentab[4] = TVisualAllocPen(v, 0x00ff00);
				pentab[5] = TVisualAllocPen(v, 0x00ffff);
				pentab[6] = TVisualAllocPen(v, 0x112233);
				pentab[7] = TVisualAllocPen(v, 0x000000);

				TVisualClear(v, pentab[7]);

				for (y = 0; y < 2; ++y)
				{
					for (x = 0; x < 3; ++x)
					{
						TTAGITEM tasktags[2];
						struct efxdata init;
						init.x = 20 + x * 220;
						init.y = 20 + y * 220;
						init.pen = pentab[i];
						init.backpen = pentab[6];
						init.whitepen = pentab[0];
						init.blackpen = pentab[7];
						init.visual = v;
						tasktags[0].tti_Tag = TTask_UserData;
						tasktags[0].tti_Value = (TTAG) &init;
						tasktags[1].tti_Tag = TTAG_DONE;
						tasks[i] = TCreateTask(&taskhook, tasktags);
						i++;
					}
				}

				TVisualSetInput(v, TITYPE_NONE, TITYPE_CLOSE |
					TITYPE_COOKEDKEY | TITYPE_NEWSIZE | TITYPE_REFRESH);

				iport = TVisualGetPort(v);

				do
				{
					TWait(TGetPortSignal(iport));

					while ((imsg = (TIMSG *) TGetMsg(iport)))
					{
						switch (imsg->timsg_Type)
						{
							case TITYPE_REFRESH:
							case TITYPE_NEWSIZE:
								TVisualClear(v, pentab[7]);
								break;

							case TITYPE_CLOSE:
								abort = TTRUE;
								break;

							case TITYPE_COOKEDKEY:
								if (imsg->timsg_Code == TKEYC_ESC)
									abort = TTRUE;
								break;
						}
						TAckMsg(imsg);
					}

				} while (!abort);

				for (i = 0; i < 6; ++i)
				{
					if (tasks[i])
					{
						TSignal(tasks[i], TTASK_SIG_ABORT);
						TDestroy((struct THandle *) tasks[i]);
					}
				}

				for (i = 0; i < 8; ++i)
					TVisualFreePen(v, pentab[i]);

				TVisualClose(vismod, v);
			}

			TCloseModule(vismod);
		}
	}

	TCloseModule(TUtilBase);
}
