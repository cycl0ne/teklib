
/*
**	$Id: poolmem.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/visual/tests/poolmem.c - Visual module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <stdlib.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>
#include <tek/proto/visual.h>
/* need access to exec private structures: */
#include <tek/mod/exec.h>

#define NUMSLOTS	200
#define MAXSIZE		300

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TTimeBase;
struct TTask *taskmmu;
struct TTimeRequest *treq;
TUINT seed = 123;

typedef struct
{
	struct THandle handle;
	TAPTR v;
	TVPEN pen[4];
} WINDOW;

static THOOKENTRY TTAG
destroywin(struct THook *hook, TAPTR obj, TTAG msg)
{
	WINDOW *w = obj;
	TCloseModule(w->v);
	TFree(w);
	return 0;
}

static TAPTR
openwin(struct TTask *task)
{
	WINDOW *w = TAlloc0(taskmmu, sizeof(WINDOW));
	if (w)
	{
		TTAGITEM wintags[4];
		wintags[0].tti_Tag = TVisual_Title;
		wintags[0].tti_Value = (TTAG) "Pooled Memory Visualization";
		wintags[1].tti_Tag = TVisual_PixWidth;
		wintags[1].tti_Value = (TTAG) 700;
		wintags[2].tti_Tag = TVisual_PixHeight;
		wintags[2].tti_Value = (TTAG) 500;
		wintags[3].tti_Tag = TTAG_DONE;
		w->handle.thn_Hook.thk_Entry = destroywin;
		w->v = TOpenModule("visual", 0, wintags);
		if (w->v)
		{
			w->pen[0] = TVisualAllocPen(w->v, 0x000000);
			w->pen[1] = TVisualAllocPen(w->v, 0x880000);
			w->pen[2] = TVisualAllocPen(w->v, 0x00cc00);
			w->pen[3] = TVisualAllocPen(w->v, 0xffffff);
			TVisualClear(w->v, w->pen[0]);
			TVisualSetInput(w->v, TITYPE_NONE, TITYPE_CLOSE | TITYPE_COOKEDKEY);
			return w;
		}
		TFree(w);
	}
	return TNULL;
}

static TBOOL
drawpools(WINDOW *win, struct TMemPool *p)
{
	struct TNode *n, *nn;
	TINT x,y,w,h;
	TINT xx1,yy1,xx2,yy2;
	union TMemHead *mh;
	union TMemNode **mnp, *mn;
	TBOOL abort = TFALSE;
	TIMSG *imsg;
	TINT poolsize = 0;
	TINT poolfree = 0;
	TTIME time;
	TCHR text[50];

	time.ttm_Sec = 0;
	time.ttm_USec = 20000;

	TVisualClear(win->v, win->pen[0]);

	w = 32;
	x = 10;
	y = 10;

	n = p->tpl_List.tlh_Head;
	while ((nn = n->tln_Succ))
	{
		mh = (union TMemHead *) n;
		mnp = &mh->tmh_Node.tmh_FreeList;
		h = (mh->tmh_Node.tmh_MemEnd - mh->tmh_Node.tmh_Mem)/w;

		TVisualFRect(win->v, x, y, w, h, win->pen[1]);

		poolsize += (mh->tmh_Node.tmh_MemEnd - mh->tmh_Node.tmh_Mem);
		poolfree += mh->tmh_Node.tmh_Free;

		while ((mn = *mnp))
		{
			yy1 = (TINT)((TINT8*)mn - (TINT8*)mh->tmh_Node.tmh_Mem) / w;
			xx1 = (TINT)((TINT8*)mn - (TINT8*)mh->tmh_Node.tmh_Mem) % w;

			yy2 = ((TINT)((TINT8*)mn - (TINT8*)mh->tmh_Node.tmh_Mem) +
				mn->tmn_Node.tmn_Size - 1) / w;
			xx2 = ((TINT)((TINT8*)mn - (TINT8*)mh->tmh_Node.tmh_Mem) +
				mn->tmn_Node.tmn_Size - 1) % w;

			if (yy1 == yy2)
				TVisualLine(win->v, x+xx1, y+yy1, x+xx2, y+yy1, win->pen[2]);
			else
			{
				TVisualLine(win->v, x+xx1, y+yy1, x+w-1, y+yy1, win->pen[2]);
				if (yy2-yy1 > 1)
					TVisualFRect(win->v, x, y+yy1+1, w, yy2-yy1-1, win->pen[2]);
				TVisualLine(win->v, x, y+yy2, x+xx2, y+yy2, win->pen[2]);
			}
			mnp = &mn->tmn_Node.tmn_Next;
		}

		x += w + 8;
		n = nn;
	}

	sprintf(text, "load: %d%%", 100-(100 * poolfree / poolsize));
	TVisualText(win->v, 0,0, text, TUtilStrLen(TUtilBase, text),
		win->pen[3], TVPEN_UNDEFINED);

	TWaitTime(treq, &time, 0);

	while ((imsg = (TIMSG *) TGetMsg(TVisualGetPort(win->v))))
	{
		switch (imsg->timsg_Type)
		{
			case TITYPE_CLOSE:
				abort = TTRUE;
				break;
			case TITYPE_COOKEDKEY:
				abort = (imsg->timsg_Code == TKEYC_ESC);
				break;
		}
		TAckMsg(imsg);
	}

	return abort;
}

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TTimeBase = TOpenModule("time", 0, TNULL);
	taskmmu = TGetTaskMemManager(task);
	if (TUtilBase && TTimeBase)
	{
		treq = TAllocTimeRequest(TNULL);
		if (treq)
		{
			WINDOW *w = openwin(task);
			if (w)
			{
				TAPTR pool = TCreatePool(TNULL);
				if (pool)
				{
					TAPTR *slots = TAlloc(taskmmu, sizeof(TAPTR) * NUMSLOTS);
					TUINT *sizes = TAlloc(taskmmu, sizeof(TUINT) * NUMSLOTS);

					if (slots && sizes)
					{
						TINT slot, size;
						TINT i = 0;

						TFillMem(slots, sizeof(TAPTR) * NUMSLOTS, 0);
						TFillMem(sizes, sizeof(TUINT) * NUMSLOTS, 0);

						for (;;)
						{
							slot = (seed = TGetRand(seed)) % NUMSLOTS;
							if ((seed = TGetRand(seed)) % 10)
							{
								/* "typical" allocation */
								size = ((seed = TGetRand(seed)) % MAXSIZE) + 1;
							}
							else
							{
								/* "peak" allocation */
								size = ((seed = TGetRand(seed)) % (MAXSIZE*5))+1;
							}

							if (slots[slot])
							{
								if (!((seed = TGetRand(seed)) % 3))
								{
									TFreePool(pool, slots[slot], sizes[slot]);
									slots[slot] = TNULL;
									sizes[slot] = 0;
								}
								else
								{
									TAPTR newmem = TReallocPool(pool,
										slots[slot], sizes[slot], size);
									if (newmem)
									{
										slots[slot] = newmem;
										sizes[slot] = size;
									}
								}
							}
							else
							{
								slots[slot] = TAllocPool(pool, size);
								sizes[slot] = size;
							}

							if (++i % 32 == 0)
								if (drawpools(w, pool))
									break;
						}
					}

					TFree(slots);
					TFree(sizes);

					TDestroy(pool);
				}
				TDestroy(w);
			}
			TFreeTimeRequest(treq);
		}
	}
	TCloseModule(TTimeBase);
	TCloseModule(TUtilBase);
}
