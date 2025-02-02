
/*
**	teklib/src/visual/tests/fonts.c - Visual module test
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	and Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <math.h>
#include <stdio.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/proto/visual.h>
#include <tek/proto/display.h>
#include <tek/mod/display/fb.h>

#define WIN_WIDTH	600
#define WIN_HEIGHT	400

TAPTR TExecBase;
TAPTR TVisualBase;
TAPTR Visual;
TVPEN PenTab[4];
TAPTR Fonts[20];

TAPTR device, window;
const TSTRPTR pangram = "\"Fix, Schwyz!\" quäkt Jürgen blöd vom Paß";
/*****************************************************************************/

TVOID flush(TAPTR v)
{
	if (device)
	{
		/* send custom FLUSH command to visual's sub device */
		struct TVFBRequest *req;

		req = (struct TVFBRequest *) TDisplayAllocReq(device);
		req->tvr_Req.io_ReplyPort = TGetSyncPort(TNULL);
		req->tvr_Req.io_Command = TVCMD_FLUSH;
		req->tvr_Op.Flush.Window = window;
		req->tvr_Op.Flush.Rect[0] = 0;
		req->tvr_Op.Flush.Rect[1] = 0;
		req->tvr_Op.Flush.Rect[2] = WIN_WIDTH;
		req->tvr_Op.Flush.Rect[3] = WIN_HEIGHT;
		TDoIO(&req->tvr_Req);
		TDisplayFreeReq(device, (struct TVRequest *)req);
	}
}

/*****************************************************************************/

void fonttest(void)
{
	static TBOOL initialized = 0;
	TTAGITEM ftags[3];
	TAPTR fqh;
	int i = 0, j;

	if (!initialized)
	{
		ftags[0].tti_Tag = TVisual_FontName;
		ftags[0].tti_Value = (TTAG) "arial,utopia,decker";
		ftags[1].tti_Tag = TVisual_FontScaleable;
		ftags[1].tti_Value = (TTAG) TTRUE;
		ftags[2].tti_Tag = TTAG_DONE;

		printf("*********************************************************\n");
		fqh = TVisualQueryFonts(Visual, ftags);
		if (fqh)
		{
			TTAGITEM *qtags = TVisualGetNextFont(Visual, fqh);

			printf("qtags.name = %s\n", (TSTRPTR) TGetTag(qtags, TVisual_FontName, TNULL));
			printf("qtags.italic = %d\n", (TUINT) TGetTag(qtags, TVisual_FontItalic, TNULL));
			printf("qtags.bold = %d\n", (TUINT) TGetTag(qtags, TVisual_FontBold, TNULL));

			for (i = 6, j = 0; i < 36; i += 2, j++)
			{
				TTAGITEM fotags[2];
				fotags[0].tti_Tag = TVisual_FontPxSize;
				fotags[0].tti_Value = i;
				fotags[1].tti_Tag = TTAG_MORE;
				fotags[1].tti_Value = (TTAG) qtags;
				Fonts[j] = TVisualOpenFont(Visual, fotags);
			}

			TDestroy(fqh);
			initialized = TTRUE;
		}
		printf("*********************************************************\n");
	}

	if (initialized)
	{
		TINT y = 0;
		for (i = 6, j = 0; i < 36; i += 2, j++)
		{
			TVisualSetFont(Visual, Fonts[j]);
			TVisualText(Visual, 0, y, pangram, TStrLen(pangram),
				PenTab[0]);
			y += i;
		}
	}
}

/*****************************************************************************/
/*
**	Main Program
*/

void TEKMain(struct TTask *task)
{
	TAPTR display;
	TTAGITEM vistags[3];

	TExecBase = TGetExecBase(task);
	TVisualBase = TOpenModule("visual", 0, TNULL);
	if (TVisualBase == TNULL)
		return;

	vistags[0].tti_Tag = TVisual_DisplayName;
	vistags[0].tti_Value = (TTAG) "display_rawfb";
	vistags[1].tti_Tag = TVisual_DriverName;
	vistags[1].tti_Value = (TTAG) "display_x11";
	vistags[2].tti_Tag = TTAG_DONE;

	display = TVisualOpenDisplay(TVisualBase, vistags);
	if (display)
	{
		TTAGITEM vistags[5];

		vistags[0].tti_Tag = TVisual_Display;
		vistags[0].tti_Value = (TTAG) display;
		vistags[1].tti_Tag = TVisual_Width;
		vistags[1].tti_Value = (TTAG) WIN_WIDTH;
		vistags[2].tti_Tag = TVisual_Height;
		vistags[2].tti_Value = (TTAG) WIN_HEIGHT;
		vistags[3].tti_Tag = TVisual_Title;
		vistags[3].tti_Value = (TTAG) "Font Test";
		vistags[4].tti_Tag = TTAG_DONE;

		Visual = TVisualOpen(TVisualBase, vistags);
		if (Visual)
		{
			TTAGITEM tags[3];
			struct TMsgPort *iport = TVisualGetPort(Visual);
			TUINT isignal = TGetPortSignal(iport);
			TBOOL abort = TFALSE;
			TBOOL refresh = TTRUE;

			tags[0].tti_Tag = TVisual_Device;
			tags[0].tti_Value = (TTAG) &device;
			tags[1].tti_Tag = TVisual_Window;
			tags[1].tti_Value = (TTAG) &window;
			tags[2].tti_Tag = TTAG_DONE;
			TVisualGetAttrs(Visual, tags);

			flush(Visual);

			PenTab[0] = TVisualAllocPen(Visual, 0xffffff);
			PenTab[1] = TVisualAllocPen(Visual, 0x000000);
			PenTab[2] = TVisualAllocPen(Visual, 0xff00ff);
			PenTab[3] = TVisualAllocPen(Visual, 0xff0000);

			TVisualClear(Visual, PenTab[1]);

			TVisualSetInput(Visual, TITYPE_NONE, TITYPE_CLOSE |
				TITYPE_COOKEDKEY | TITYPE_NEWSIZE | TITYPE_REFRESH);

			do
			{
				TIMSG *imsg;

				if (refresh)
				{
					TVisualClear(Visual, PenTab[1]);
					fonttest();
					flush(Visual);
					refresh = TFALSE;
				}

				TWait(isignal);

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

			} while (!abort);

			TVisualClose(TVisualBase, Visual);
		}
	}

	TCloseModule(TVisualBase);
}
