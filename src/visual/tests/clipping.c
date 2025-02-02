
/*
**	$Id: clipping.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/visual/tests/clipping.c - triangle clipping test
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <tek/teklib.h>
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/proto/visual.h>
#include <tek/proto/display.h>
#include <tek/mod/display/fb.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TVisualBase;

#define WIN_WIDTH	(201*2)
#define WIN_HEIGHT	(201*2)

#define BUF_WIDTH	120
#define BUF_HEIGHT	120

/*****************************************************************************/

TAPTR device, window;

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
		req->tvr_Op.Flush.Rect[0] = -1;
		req->tvr_Op.Flush.Rect[1] = 10;
		req->tvr_Op.Flush.Rect[2] = WIN_WIDTH - 10;
		req->tvr_Op.Flush.Rect[3] = WIN_HEIGHT - 10;
		TDoIO(&req->tvr_Req);
		TDisplayFreeReq(device, (struct TVRequest *)req);
	}
}

/*****************************************************************************/
/*
**	Main Program
*/


TINT triangle[6] =
{
	WIN_WIDTH/4*3, 0,
	WIN_WIDTH/2, WIN_HEIGHT/2-1,
	WIN_WIDTH-1, WIN_HEIGHT/2-1,
};

void TEKMain(struct TTask *task)
{
	TINT count = 0;
	TINT clipw = WIN_WIDTH;
	TINT cliph = WIN_HEIGHT;

	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TVisualBase = TOpenModule("visual", 0, TNULL);
	if (TUtilBase && TVisualBase)
	{
		TTAGITEM vistags[3];
		TAPTR display;

		vistags[0].tti_Tag = TVisual_DisplayName;
		vistags[0].tti_Value = (TTAG) "display_rawfb";
		vistags[1].tti_Tag = TVisual_DriverName;
		vistags[1].tti_Value = (TTAG) "display_x11";
		vistags[2].tti_Tag = TTAG_DONE;

		display = TVisualOpenDisplay(TVisualBase, vistags);
		if (display)
		{
			TTAGITEM vistags[5];
			TAPTR v;

			vistags[0].tti_Tag = TVisual_Display;
			vistags[0].tti_Value = (TTAG) display;
			vistags[1].tti_Tag = TVisual_Width;
			vistags[1].tti_Value = (TTAG) WIN_WIDTH;
			vistags[2].tti_Tag = TVisual_Height;
			vistags[2].tti_Value = (TTAG) WIN_HEIGHT;
			vistags[3].tti_Tag = TVisual_Title;
			vistags[3].tti_Value = (TTAG) "Clipping Test";
			vistags[4].tti_Tag = TTAG_DONE;

			v = TVisualOpen(TVisualBase, vistags);

			if (v)
			{
				TIMSG *imsg;
				TBOOL abort = TFALSE;
				TBOOL refresh = TTRUE;
				TAPTR iport;
				TUINT i = 0;
				TVPEN pentab[5];
				TTAGITEM tags[3];
				TINT x = (WIN_WIDTH - clipw) / 2;
				TINT y = (WIN_HEIGHT - cliph) / 2;

				TUINT32 col, mybuf[BUF_WIDTH * BUF_HEIGHT];
				TUINT8 r = 0, g = 0, b = 0;
				TINT bx, by;

				/* testdata */
				for (by = 0; by < BUF_HEIGHT; by++)
				{
					r = g = b = 0;

					for (bx = 0; bx < BUF_WIDTH; bx++)
					{
						col = (r<<16) | (g<<8) | b;
						mybuf[by*BUF_WIDTH+bx] = col;

						r += 4;
						g += 12;
						b += 1;
					}
				}

				tags[0].tti_Tag = TVisual_Device;
				tags[0].tti_Value = (TTAG) &device;
				tags[1].tti_Tag = TVisual_Window;
				tags[1].tti_Value = (TTAG) &window;
				tags[2].tti_Tag = TTAG_DONE;
				TVisualGetAttrs(v, tags);

				flush(v);

				pentab[0] = TVisualAllocPen(v, 0x00ffff);
				pentab[1] = TVisualAllocPen(v, 0x000000);
				pentab[2] = TVisualAllocPen(v, 0xff00ff);
				pentab[3] = TVisualAllocPen(v, 0x00ff00);
				pentab[4] = TVisualAllocPen(v, 0xff0000);

				TVisualClear(v, pentab[1]);
				TVisualSetClipRect(v, 0, 0, clipw, cliph, TNULL);
				TVisualRect(v, 0, 0, clipw, cliph, pentab[4]);

				TVisualSetInput(v, TITYPE_NONE, TITYPE_CLOSE |
					TITYPE_COOKEDKEY | TITYPE_NEWSIZE | TITYPE_REFRESH);

				iport = TVisualGetPort(v);

				do
				{
				#if 0
					TWait(TGetPortSignal(iport));
				#else

					if (count % 0x100 == 0)
					{
						TINT i;

						for (i = 0; i < 6; i++)
							triangle[i] = rand();

						for (i = 0; i < 6; i+=2)
						{
							triangle[i] = triangle[i]/(RAND_MAX/(WIN_WIDTH))+WIN_WIDTH/2;
							triangle[i+1] = WIN_HEIGHT/2-triangle[i+1]/(RAND_MAX/(WIN_HEIGHT));

							/*triangle[i] = WIN_WIDTH*5-triangle[i]/(RAND_MAX/(WIN_WIDTH*10));
							triangle[i+1] = WIN_HEIGHT*5-triangle[i+1]/(RAND_MAX/(WIN_HEIGHT*10));*/
						}

						TVisualUnsetClipRect(v);
						TVisualClear(v, pentab[1]);
						TVisualSetClipRect(v, x, y, clipw, cliph, TNULL);
						refresh = TTRUE;
					}

					count++;
				#endif

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
							{
								if (imsg->timsg_Code == TKEYC_ESC)
									abort = TTRUE;
								if (imsg->timsg_Code == TKEYC_F1)
									refresh = TTRUE;

								if (imsg->timsg_Code == '+')
								{
									if (clipw + 1 < WIN_WIDTH && cliph + 1 < WIN_HEIGHT)
									{
										clipw+=2; cliph+=2;
										x = (WIN_WIDTH - clipw) / 2;
										y = (WIN_HEIGHT - cliph) / 2;

										TVisualUnsetClipRect(v);
										TVisualClear(v, pentab[1]);
										TVisualSetClipRect(v, x, y, clipw, cliph, TNULL);
										TVisualRect(v, x, y, clipw, cliph, pentab[4]);
										refresh = TTRUE;
									}
								}

								if (imsg->timsg_Code == '-')
								{
									if (clipw - 1 > 0 && cliph - 1 > 0)
									{
										clipw-=2; cliph-=2;
										x = (WIN_WIDTH - clipw) / 2;
										y = (WIN_HEIGHT - cliph) / 2;

										TVisualUnsetClipRect(v);
										TVisualClear(v, pentab[1]);
										TVisualSetClipRect(v, x, y, clipw, cliph, TNULL);
										TVisualRect(v, x, y, clipw, cliph, pentab[4]);
										refresh = TTRUE;
									}
								}

								if (imsg->timsg_Code == 'n')
								{
									TINT i;

									for (i = 0; i < 6; i++)
										triangle[i] = rand();

									for (i = 0; i < 6; i+=2)
									{
										triangle[i] = triangle[i]/(RAND_MAX/(WIN_WIDTH))+WIN_WIDTH/2;
										triangle[i+1] = WIN_HEIGHT/2-triangle[i+1]/(RAND_MAX/(WIN_HEIGHT));
									}

									TVisualUnsetClipRect(v);
									TVisualClear(v, pentab[1]);
									TVisualSetClipRect(v, x, y, clipw, cliph, TNULL);
									TVisualRect(v, x, y, clipw, cliph, pentab[4]);
									refresh = TTRUE;
								}

								break;
							}
						}
						TAckMsg(imsg);
					}

					/*TVisualSetClipRect(v, 20, 20, 160, 160, TNULL);
					TVisualRect(v, 20, 20, 160, 160, pentab[4]);*/
					TVisualRect(v, x, y, clipw, cliph, pentab[4]);

					if (refresh)
					{
						TTAGITEM stags[2];
						TINT x, y = 0;
						TINT w = WIN_WIDTH/2;
						TINT h = WIN_HEIGHT/2;

						/*TVisualFRect(v, -100, -100, 100, 100, pentab[3]);
						TVisualFRect(v, WIN_WIDTH+1, WIN_HEIGHT+1, 100, 100, pentab[3]);*/
						TVisualFRect(v, -50, -50+h, 100, 100, pentab[3]);
						TVisualFRect(v, -50, WIN_HEIGHT-50, 100, 100, pentab[3]);
						TVisualFRect(v, w-50, -50+h, 100, 100, pentab[3]);
						TVisualFRect(v, w-50, WIN_HEIGHT-50, 100, 100, pentab[3]);
						TVisualFRect(v, w/2-50, h/2-50+h, 100, 100, pentab[3]);

						stags[0].tti_Tag = TVisual_Pen;
						stags[0].tti_Value = (TTAG) pentab[0];
						stags[1].tti_Tag = TTAG_DONE;

      					TVisualDrawStrip(v, triangle, 3, stags);

						for (x = 0; x < w-1; x +=20)
							TVisualLine(v,  x, y, w-x,  h-y, pentab[2]);

						for (y = 0; y < h-1; y +=20)
							TVisualLine(v,  x, y, w-x,  h-y, pentab[2]);

						TVisualDrawBuffer(v, w, h, mybuf, BUF_WIDTH, BUF_HEIGHT,
							BUF_WIDTH, TNULL);

						TVisualCopyArea(v, w, h, BUF_WIDTH*2, BUF_HEIGHT*2,
							w+w-BUF_WIDTH, h+h-BUF_HEIGHT, TNULL);

      					flush(v);
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
}
