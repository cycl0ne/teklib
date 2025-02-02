
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
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>
#include <tek/proto/visual.h>

TAPTR TExecBase;
TAPTR TTimeBase;
TAPTR TUtilBase;
TAPTR vismod;

/*****************************************************************************/

#define NUMPENS		4

/*****************************************************************************/

void fonttest(TAPTR v, TVPEN *pentab)
{
	TINT fh = 0;
	TAPTR cfont = TNULL;
	TAPTR dfont = TNULL;
	TSTRPTR buf = "hallo";
	TTAGITEM ftags[6];
	static TBOOL init_done = TFALSE;

	if (!init_done)
	{
		TAPTR fq = TNULL;
		TTAGITEM *ctags = TNULL;

		dfont = TVisualOpenFont(v, TNULL);

		/* open custom font */
		ftags[0].tti_Tag = TVisual_FontName;
		ftags[0].tti_Value = (TTAG) "utopia";
		ftags[1].tti_Tag = TVisual_FontPxSize;
		ftags[1].tti_Value = (TTAG) 32;
		ftags[2].tti_Tag = TTAG_DONE;
		cfont = TVisualOpenFont(v, ftags);

		if (dfont && cfont)
		{
			TVisualSetFont(v, cfont);

			printf("size of text: %d\n", TVisualTextSize(v, cfont, "hallo"));

// 			TVisualSetFont(v, dfont);
// 			TVisualCloseFont(v, cfont);
		}

		init_done = TTRUE;
	}

	TVisualText(v, 30, 30, "hallo", 5, pentab[0], TVPEN_UNDEFINED);
}

/*****************************************************************************/
/*
**	Main Program
*/

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TTimeBase = TOpenModule("time", 0, TNULL);

	if (TUtilBase && TTimeBase)
	{
		vismod = TOpenModule("visual", 0, TNULL);
		if (vismod)
		{
			TAPTR v;
			TTAGITEM vistags[4];

			vistags[0].tti_Tag = TVisual_PixWidth;
			vistags[0].tti_Value = (TTAG) 680;
			vistags[1].tti_Tag = TVisual_PixHeight;
			vistags[1].tti_Value = (TTAG) 460;
			vistags[2].tti_Tag = TVisual_Title;
			vistags[2].tti_Value = (TTAG) "Font Test";
			vistags[3].tti_Tag = TTAG_DONE;

			v = TVisualOpen(vismod, vistags);

			if (v)
			{
				TIMSG *imsg;
				TBOOL abort = TFALSE;
				TAPTR iport;
				TINT i = 0;
				TVPEN pentab[NUMPENS];

				pentab[0] = TVisualAllocPen(v, 0xffffff);
				pentab[1] = TVisualAllocPen(v, 0x000000);
				pentab[2] = TVisualAllocPen(v, 0xff00ff);
				pentab[3] = TVisualAllocPen(v, 0xff0000);

				TVisualClear(v, pentab[1]);

				fonttest(v, pentab);

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
								TVisualClear(v, pentab[1]);
								fonttest(v, pentab);
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

				for (i = 0; i < NUMPENS; ++i)
					TVisualFreePen(v, pentab[i]);

				TVisualClose(vismod, v);
			}

			TCloseModule(vismod);
		}
	}

	TCloseModule(TTimeBase);
	TCloseModule(TUtilBase);
}
