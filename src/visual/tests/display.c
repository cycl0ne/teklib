
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
#include <tek/proto/visual.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TVisualBase;

/*****************************************************************************/

#if 0


	TAPTR dh = TVisualQueryDisplays(vis, dtags);
	if (dh)
	{
		TTAGITEM *dtags;
		while ((dtags = TVisualGetNextDisplay(vis, dh)))
			printf("%s\n", (TSTRPTR) TGetTag(dtags, TVisual_DisplayName, TNULL));

		dtags = TVisualGetNextDisplay(vis, dh);

		if (dtags)
		{
				display = TVisualOpenDisplay(vis, dtags);
		}

		TDestroy(dh);
	}

	TVisualQueryDisplays(vis, tags);
	TVisualGetNextDisplay(vis, dh);
	TVisualOpenDisplay(vis, tags);
	TVisualCloseDisplay(vis, display);



void fonttest(TAPTR v, TVPEN *pentab)
{
	TINT fh = 0;
	TAPTR cfont = TNULL;
	TAPTR dfont = TNULL;
	TSTRPTR buf = "äöü ABCDEFG.^'!$%&/()=?`|";
	TTAGITEM ftags[6];
	static TBOOL init_done = TFALSE;

	if (!init_done)
	{
		TAPTR fq = TNULL;
		TTAGITEM *ctags = TNULL;

		/* open default font in 20px */
		ftags[0].tti_Tag = TVisual_FontPxSize;
		ftags[0].tti_Value = (TTAG) 20;
		ftags[1].tti_Tag = TTAG_DONE;
		dfont = TVisualOpenFont(v, ftags);

#if 1
		/* query fonts */
		ftags[0].tti_Tag = TVisual_FontName;
		ftags[0].tti_Value = (TTAG) "utopia,arial";
		ftags[1].tti_Tag = TVisual_FontPxSize;
		ftags[1].tti_Value = (TTAG) 48;

		ftags[2].tti_Tag = TVisual_FontScaleable;
		ftags[2].tti_Value = (TTAG) TTRUE;

		ftags[3].tti_Tag = TVisual_FontNumResults;
		ftags[3].tti_Value = (TTAG) 1024;
		ftags[4].tti_Tag = TTAG_DONE;

		fq = TVisualQueryFonts(v, ftags);
		if (fq)
		{
			while((ctags = TVisualGetNextFont(v, fq)))
			{
				printf("%s\n", (TSTRPTR)TGetTag(ctags, TVisual_FontName, (TTAG)"???"));
			}

			ctags = TVisualGetNextFont(v, fq);

			if (ctags)
			{
				/* open custom font */
				cfont = TVisualOpenFont(v, ctags);
			}

			/* destroy font handle */
			TDestroy(fq);
		}
#else
		/* open custom font */
		ftags[0].tti_Tag = TVisual_FontName;
		ftags[0].tti_Value = (TTAG) "arial";
		ftags[1].tti_Tag = TVisual_FontPxSize;
		ftags[1].tti_Value = (TTAG) 57;
		ftags[2].tti_Tag = TVisual_FontItalic;
		ftags[2].tti_Value = (TTAG) TFALSE;
		ftags[3].tti_Tag = TVisual_FontBold;
		ftags[3].tti_Value = (TTAG) TTRUE;
		ftags[4].tti_Tag = TTAG_DONE;
		cfont = TVisualOpenFont(v, ftags);
#endif

		if (dfont && cfont)
		{
			TUINT ul = 0;
			TTAGITEM tags[4];

			tags[0].tti_Tag = TVisual_FontHeight;
			tags[0].tti_Value = (TTAG) &fh;
			tags[1].tti_Tag = TVisual_FontUlPosition;
			tags[1].tti_Value = (TTAG) &ul;
			tags[2].tti_Tag = TTAG_DONE;

			TVisualGetFontAttrs(v, cfont, tags);
			printf("UL pos: %d\n", ul);

			TVisualSetFont(v, cfont);
			TVisualText(v, 10, 10, buf, TStrLen(buf), pentab[0], pentab[3]/*TVPEN_UNDEFINED*/);
			printf("size of text: %d\n", TVisualTextSize(v, cfont, buf));
			TVisualSetFont(v, dfont);
			TVisualCloseFont(v, cfont);
		}

		init_done = TTRUE;
	}

	{
		TTAGITEM stags[2];
		TVPEN penarray[6];
		TINT a1[6] = {
			0,   300,
			300,   0,
			300, 600,
		};

		TINT a2[8] = {
			600, 200,
			300, 200,
			600, 400,
			300, 400,
		};

		TINT fan[12] = {
			340, 230,
			240, 130,
			440, 130,
			440, 330,
			240, 330,
			240, 130,
		};

		penarray[0] = pentab[0];
		penarray[1] = pentab[0];
		penarray[2] = pentab[0];
		penarray[3] = pentab[3];
		penarray[4] = pentab[0];
		penarray[5] = pentab[3];

		stags[0].tti_Tag = TVisual_PenArray;
		stags[0].tti_Value = (TTAG) &penarray;
		stags[1].tti_Tag = TTAG_DONE;

		TVisualDrawStrip(v, a1, 3, stags);
		TVisualDrawStrip(v, a2, 4, stags);

		TVisualDrawFan(v, fan, 6, stags);
	}

	TVisualText(v, 10, 10+fh, buf, TStrLen(buf), pentab[0], TVPEN_UNDEFINED);
}
#endif

/*****************************************************************************/
/*
**	Main Program
*/

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TVisualBase = TOpenModule("visual", 0, TNULL);
	if (TUtilBase && TVisualBase)
	{
		TTAGITEM vistags[2];
		TAPTR display;

		vistags[0].tti_Tag = TVisual_DisplayName;
		vistags[0].tti_Value = (TTAG) "display_windows";
		vistags[1].tti_Tag = TTAG_DONE;

		display = TVisualOpenDisplay(TVisualBase, vistags);
		if (display)
		{
			TTAGITEM vistags[5];
			TAPTR v;

			vistags[0].tti_Tag = TVisual_Display;
			vistags[0].tti_Value = (TTAG) display;
			vistags[1].tti_Tag = TVisual_Width;
			vistags[1].tti_Value = (TTAG) 680;
			vistags[2].tti_Tag = TVisual_Height;
			vistags[2].tti_Value = (TTAG) 460;
			vistags[3].tti_Tag = TVisual_Title;
			vistags[3].tti_Value = (TTAG) "Display Test";
			vistags[4].tti_Tag = TTAG_DONE;

			v = TVisualOpen(TVisualBase, vistags);

			if (v)
			{
				TIMSG *imsg;
				TBOOL abort = TFALSE;
				TBOOL refresh = TTRUE;
				TAPTR iport;
				TUINT i = 0;
				TVPEN pentab[4];

				pentab[0] = TVisualAllocPen(v, 0xffffff);
				pentab[1] = TVisualAllocPen(v, 0x000000);
				pentab[2] = TVisualAllocPen(v, 0xff00ff);
				pentab[3] = TVisualAllocPen(v, 0xff0000);

				TVisualClear(v, pentab[1]);

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
								refresh = TTRUE;
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

					if (refresh)
					{
						TVisualClear(v, pentab[1]);
						refresh = TFALSE;
					}

				} while (!abort);

				for (i = 0; i < sizeof(pentab) / sizeof(TVPEN); ++i)
					TVisualFreePen(v, pentab[i]);

				TVisualClose(TVisualBase, v);
			}

			TVisualCloseDisplay(TVisualBase, display);
		}
	}

	TCloseModule(TVisualBase);
	TCloseModule(TUtilBase);

	printf("done.\n");
}
