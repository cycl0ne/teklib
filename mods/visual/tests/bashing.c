
/*
**	$Id: bashing.c,v 1.7 2005/07/07 01:45:28 tmueller Exp $
**	apps/tests/bashing.c - Visual multibashing test
**	Demonstrates multiple threads drawing to a single window
**	at different framerates
*/

#include <math.h>
#include <stdio.h>
#include <tek/teklib.h>
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>
#include <tek/proto/visual.h>

TAPTR TExecBase;
TAPTR TTimeBase;
TAPTR TUtilBase;
TINT seed = 123;

/*****************************************************************************/

#define NUMLINES	40

struct efxdata
{
	TINT x, y;
	TVPEN pen, backpen, whitepen, blackpen;
	TAPTR visual;
	TINT framerate;
	struct TTimeRequest *treq;
};

/*****************************************************************************/
/* 
**	Thread init function
*/

TTASKENTRY TBOOL efxinitfunc(TAPTR task)
{
	TAPTR mmu = TGetTaskMMU(task);
	struct efxdata *initdata = TGetTaskData(task);
	struct efxdata *data = TAlloc(mmu, sizeof(struct efxdata));
	if (data)
	{
		TCopyMem(initdata, data, sizeof(struct efxdata));
		data->treq = TAllocTimeRequest(TNULL);
		if (data->treq)
		{
			data->visual = TVisualAttach(initdata->visual, TNULL);
			if (data->visual)
			{
				TSetTaskData(task, data);
				return TTRUE;
			}
			TFreeTimeRequest(data->treq);
		}
	}
	return TFALSE;
}

static TFLOAT frand(TVOID)
{
	TFLOAT f = (TFLOAT) (seed = TGetRand(seed));
	f /= 0x7fffffff;
	return f;
}

/*****************************************************************************/
/*
**	Effect thread
*/

TTASKENTRY TVOID efxfunc(TAPTR task)
{
	struct efxdata *data = TGetTaskData(task);
	TINT i, j;
	TFLOAT s[6], ss[6], ds[6], dss[6];
	TTIME t0, t1;
	TFLOAT difftime, fps = 1.0;
	TUINT signals;
	char buf[30];

	struct { TINT16 x, y; } xyarray[NUMLINES + 1];

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
		TFLOAT sec;
		TQueryTime(data->treq, &t0);

		TVisualFRect(data->visual, data->x, data->y, 200, 200, data->backpen);

		for (i = 0; i < 6; ++i) ss[i] = s[i];

		for (j = 0; j < NUMLINES + 1; ++j)
		{
			xyarray[j].x = (TINT) ((sin(ss[0]) + sin(ss[1]) + sin(ss[2])) *
				200 / 6 + 200 / 2 + data->x);
			xyarray[j].y = (TINT) ((sin(ss[3]) + sin(ss[4]) + sin(ss[5])) *
				200 / 6 + 200 / 2 + data->y);
			for (i = 0; i < 6; ++i)
			{
				ss[i] += dss[i];
				if (ss[i] > 2*TPI) ss[i] -= (TFLOAT) (2*TPI);
			}
		}

		for (i = 0; i < 6; ++i)
		{
			s[i] += ds[i];
			if (s[i] > 2*TPI) s[i] -= (TFLOAT) (2*TPI);
		}

		TVisualLineArray(data->visual, (TINT16 *) xyarray, NUMLINES + 1,
			data->pen);

		sprintf(buf, "FPS: %d/%d/%d%% ", (TINT) fps, data->framerate,
			(TINT) (fps * 100 / data->framerate));

		TVisualText(data->visual, (data->x + fw - 1) / fw, (data->y + fh - 1) / fh,
			buf, TUtilStrLen(TUtilBase, buf), data->blackpen,
			data->whitepen);

		TVisualFlushArea(data->visual, data->x, data->y,200,200);

		TQueryTime(data->treq, &t1);
		TSubTime(&t1, &t0);

		sec = (TFLOAT) t1.ttm_USec / 1000000 + t1.ttm_Sec;
		difftime = 1.0f / (TFLOAT) data->framerate - sec;

		if (difftime > 0)
		{
			t1.ttm_Sec = (TINT) difftime;
			t1.ttm_USec = (TINT) ((difftime - t1.ttm_Sec) * 1000000);

			if (t1.ttm_Sec || t1.ttm_USec)
				signals = TWaitTime(data->treq, &t1, TTASK_SIG_ABORT);
			else
				signals = 0;
		}
		else
		{
			signals = TSetSignal(0, TTASK_SIG_ABORT);
		}

		TQueryTime(data->treq, &t1);
		TSubTime(&t1, &t0);

		sec = (TFLOAT) t1.ttm_USec / 1000000 + t1.ttm_Sec;
		fps = 1.0f / sec;

	} while (!(signals & TTASK_SIG_ABORT));

	TFreeTimeRequest(data->treq);
	TCloseModule(data->visual);
	TFree(data);
}

/*****************************************************************************/
/* 
**	Main Program
*/

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TTimeBase = TOpenModule("time", 0, TNULL);
	if (TUtilBase && TTimeBase)
	{
		TAPTR v;
		TTAGITEM vistags[4];
	
		vistags[0].tti_Tag = TVisual_PixWidth;
		vistags[0].tti_Value = (TTAG) 680;
		vistags[1].tti_Tag = TVisual_PixHeight;
		vistags[1].tti_Value = (TTAG) 460;
		vistags[2].tti_Tag = TVisual_Title;
		vistags[2].tti_Value = (TTAG) "Visual multibashing";
		vistags[3].tti_Tag = TTAG_DONE;

		v = TOpenModule("visual", 0, vistags);
		if (v)
		{
			TIMSG *imsg;
			TBOOL abort = TFALSE;
			TAPTR iport;
	
			TINT x, y, i = 0;
			TVPEN pentab[8];
			TAPTR tasks[6] = {TNULL, TNULL, TNULL, TNULL, TNULL, TNULL};

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
					tasks[i] = TCreateTask(efxfunc,
						efxinitfunc, tasktags);
					i++;
				}
			}

			TVisualSetInput(v, TITYPE_NONE, TITYPE_CLOSE | TITYPE_COOKEDKEY |
				TITYPE_NEWSIZE | TITYPE_REFRESH);

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
							TVisualFlush(v);
							break;

						case TITYPE_CLOSE:
							abort = TTRUE;
							break;

						case TITYPE_COOKEDKEY:
							if (imsg->timsg_Code == TKEYC_ESC)
							{
								abort = TTRUE;
							}
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
					TDestroy(tasks[i]);
				}
			}

			for (i = 0; i < 8; ++i)
			{
				TVisualFreePen(v, pentab[i]);
			}

			TCloseModule(v);
		}
	}
	TCloseModule(TTimeBase);
	TCloseModule(TUtilBase);
	printf("all done\n");
}

