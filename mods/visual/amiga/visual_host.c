
/*
**	$Id: visual_host.c,v 1.6 2005/09/13 02:43:36 tmueller Exp $
**	teklib/mods/visual/amiga/visual_host.c - Amiga implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <string.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/mod/hal.h>
#include <tek/mod/exec.h>
#include <tek/mod/amiga/hal.h>
#include "visual_mod.h"

#include <intuition/intuition.h>
#include <libraries/cybergraphics.h>
#include <graphics/scale.h>
#include <exec/memory.h>
#include <guigfx/guigfx.h>
#include <exec/execbase.h>
#include <clib/alib_protos.h>

#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>
#include <proto/diskfont.h>
#include <proto/gadtools.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/guigfx.h>
#include <proto/keymap.h>

#define SysBase 		*((struct ExecBase **) 4L)
#define IntuitionBase	v->intuitionbase
#define GfxBase 		v->gfxbase
#define GuiGFXBase 		v->guigfxbase
#define GadToolsBase	v->gadtoolsbase
#define KeymapBase		v->keymapbase

/*****************************************************************************/

typedef struct
{
	TINT winwidth, winheight;
	TINT innerwidth, innerheight;
	TSTRPTR title;

	TMOD_VIS *mod;		/* backptr */

	APTR intuitionbase;
	APTR gfxbase;
	APTR guigfxbase;
	APTR gadtoolsbase;
	APTR keymapbase;
	
	struct Screen *screen;
	struct Window *window;
	WORD altwinpos[4];
	TBOOL active;

	TUINT pending;
	TLIST imsgpool;
	ULONG idcmpmask;
	TUINT eventmask;
	struct InputEvent keyevent;
	char keybuf[16];
	TUINT qualifier;
	TINT mousex, mousey;
	
	BYTE wakesig;
	TINT wakecount;
	
	ULONG amigapentab[256];
	ULONG bgpen, fgpen;

	APTR drawhandle;				/* guigfx drawhandle */
	APTR ddh;						/* guigfx directdrawhandle */
	TINT16 ddhw, ddhh;				/* directdrawhandle dimensions */

	TAPTR rasbuffer;
	struct TmpRas *oldtmpras;
	struct TmpRas tmpras;
	struct AreaInfo areainfo;
	TAPTR areabuf;
	TINT areasize;
	struct AreaInfo *oldareainfo;
	
} VISUAL;

/*****************************************************************************/

static TVOID
setinputmask(TMOD_VIS *mod, VISUAL *v, TUINT eventmask)
{
	ULONG idcmp = IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW |
		IDCMP_REFRESHWINDOW | IDCMP_NEWSIZE;

	if (eventmask & TITYPE_CLOSE) idcmp |= IDCMP_CLOSEWINDOW;
	if (eventmask & TITYPE_COOKEDKEY) idcmp |= IDCMP_RAWKEY;
	if (eventmask & TITYPE_MOUSEMOVE) idcmp |= IDCMP_MOUSEMOVE;
	if (eventmask & TITYPE_MOUSEBUTTON) idcmp |= IDCMP_MOUSEBUTTONS;
	if (idcmp != v->idcmpmask)
	{
		ModifyIDCMP(v->window, idcmp);
		v->idcmpmask = idcmp;
	}
	v->eventmask = eventmask;
}

/*****************************************************************************/

static TVOID
updatewindowparameters(TMOD_VIS *vis, VISUAL *v)
{
	v->winwidth = v->window->Width;
	v->winheight = v->window->Height;
	v->innerwidth = v->winwidth - v->window->BorderLeft -
		v->window->BorderRight;
	v->innerheight = v->winheight - v->window->BorderTop - 
		v->window->BorderBottom;
}

/*****************************************************************************/

LOCAL TVOID
vis_exit(TMOD_VIS *mod)
{
	VISUAL *v = mod->hostspecific;
	if (v)
	{
		ULONG i;
		struct TNode *imsg;

		if (v->rasbuffer)
		{
			v->window->RPort->AreaInfo = v->oldareainfo;
			if (v->areasize) FreeMem(v->areabuf, v->areasize * 5);
			v->window->RPort->TmpRas = v->oldtmpras;
			FreeMem(v->rasbuffer, 512);
		}

		DeleteDirectDrawHandle(v->ddh);
		ReleaseDrawHandle(v->drawhandle);
	
		for (i = 0; i < 256; ++i)
		{
			while (v->amigapentab[i]--)
			{
				ReleasePen(v->screen->ViewPort.ColorMap, i);
			}
		}

		if (v->window) CloseWindow(v->window);
		if (v->screen) UnlockPubScreen(NULL, v->screen);
		TExecFree(TExecBase, v->title);

		while ((imsg = TRemHead(&v->imsgpool))) TExecFree(TExecBase, imsg);

		if (v->wakesig) FreeSignal(v->wakesig);

		CloseLibrary(GfxBase);
		CloseLibrary(IntuitionBase);
		CloseLibrary(GadToolsBase);
		CloseLibrary(GuiGFXBase);
		CloseLibrary(KeymapBase);

		TExecFree(TExecBase, v);
		mod->hostspecific = TNULL;
	}
}

/*****************************************************************************/

LOCAL TBOOL 
vis_init(TMOD_VIS *mod)
{
	VISUAL *v = TExecAlloc0(TExecBase, mod->mmu, sizeof(VISUAL));
	if (v == TNULL) return TFALSE;
	mod->hostspecific = v;
	
	for (;;)
	{
		TINT viswidth, visheight, visleft, vistop, winx, winy;
		struct TagItem tags[20], *tp = tags;
		TINT borderwidth, borderheight;
		TSTRPTR s;
		TUINT modeID;

		TInitList(&v->imsgpool);
		v->mod = mod;	/* backptr */

		v->wakesig = AllocSignal(-1);
		if (v->wakesig == -1) break;

		GfxBase = OpenLibrary("graphics.library", 0);
		if (GfxBase == NULL) break;
		IntuitionBase = OpenLibrary("intuition.library", 0);
		if (IntuitionBase == NULL) break;
		GadToolsBase = OpenLibrary("gadtools.library", 0);
		if (GadToolsBase == NULL) break;
		GuiGFXBase = OpenLibrary("guigfx.library", 16);
		if (GuiGFXBase == NULL) break;
		KeymapBase = OpenLibrary("keymap.library", 0);
		if (KeymapBase == NULL) break;
		
		s = (TSTRPTR) TGetTag(mod->inittags, 
			TVisual_Title, (TTAG) "TEKlib visual");
		v->title = TExecAlloc(TExecBase, mod->mmu, strlen(s) + 1);
		if (v->title == TNULL) break;
		strcpy(v->title, s);		

		/* get screen info */

		v->screen = LockPubScreen(NULL);
		if (v->screen == TNULL) break;		

		viswidth = v->screen->Width;
		visheight = v->screen->Height;
		modeID = GetVPModeID(&v->screen->ViewPort);
		if (modeID)
		{
			DisplayInfoHandle dih = FindDisplayInfo(modeID);
			if (dih)
			{
				struct DimensionInfo di;
				if (GetDisplayInfoData(dih, (UBYTE *) &di, sizeof(di),
					DTAG_DIMS, modeID))
				{
					viswidth = di.TxtOScan.MaxX - di.TxtOScan.MinX + 1;
					visheight = di.TxtOScan.MaxY - di.TxtOScan.MinY + 1;
				}
			}
		}

		visleft = -v->screen->ViewPort.DxOffset;
		vistop = -v->screen->ViewPort.DyOffset;
	
		/* window center */

		winx = (viswidth >> 1) - v->screen->ViewPort.DxOffset;
		winy = (visheight >> 1) - v->screen->ViewPort.DyOffset;

		/* by default, don't let window obscure the screen's title bar */
		if (vistop < v->screen->BarHeight + 1)
		{
			vistop += v->screen->BarHeight + 1;
			visheight -= v->screen->BarHeight + 1;
			winy += (v->screen->BarHeight + 1) / 2;
		}

		/* get window size, by default 2/3 of visible screen */

		v->winwidth = (TINT) TGetTag(mod->inittags, 
			TVisual_PixWidth, (TTAG) (viswidth * 2 / 3));
		v->winheight = (TINT) TGetTag(mod->inittags, 
			TVisual_PixHeight, (TTAG) (visheight * 2 / 3));
		if (v->winwidth <= 0 || v->winheight <= 0) break;

		borderwidth = v->screen->WBorLeft + v->screen->WBorRight;
		borderheight = v->screen->WBorRight + v->screen->Font->ta_YSize +
			v->screen->WBorBottom + 1;
		borderheight += 8;	/* WFLG_SIZEBBOTTOM default */

		tp->ti_Tag = WA_InnerWidth; tp->ti_Data = v->winwidth; tp++;
		tp->ti_Tag = WA_InnerHeight; tp->ti_Data = v->winheight; tp++;

		winx -= (v->winwidth + borderwidth) / 2;
		winy -= (v->winheight + borderheight) / 2;

		tp->ti_Tag = WA_Left; tp->ti_Data = winx; tp++;
		tp->ti_Tag = WA_Top; tp->ti_Data = winy; tp++;

		tp->ti_Tag = WA_PubScreen; tp->ti_Data = (ULONG) v->screen; tp++;
		tp->ti_Tag = WA_NewLookMenus; tp->ti_Data = TRUE; tp++;
		tp->ti_Tag = WA_RMBTrap; tp->ti_Data = TRUE; tp++;
		tp->ti_Tag = WA_Title; tp->ti_Data = (ULONG) v->title; tp++;
		
		tp->ti_Tag = WA_Flags; tp->ti_Data = WFLG_DRAGBAR | 
			WFLG_GIMMEZEROZERO | WFLG_DEPTHGADGET | WFLG_ACTIVATE | 
			WFLG_CLOSEGADGET | WFLG_SIZEBBOTTOM | WFLG_SIZEGADGET |
			WFLG_NOCAREREFRESH | WFLG_REPORTMOUSE;
		tp++;

		tp->ti_Tag = WA_MinWidth; tp->ti_Data = borderwidth + 1; tp++;
		tp->ti_Tag = WA_MinHeight; tp->ti_Data = borderheight + 1; tp++;
		tp->ti_Tag = WA_MaxWidth; tp->ti_Data = viswidth; tp++;
		tp->ti_Tag = WA_MaxHeight; tp->ti_Data = visheight; tp++;

		v->altwinpos[0] = visleft;
		v->altwinpos[1] = vistop;
		v->altwinpos[2] = viswidth;
		v->altwinpos[3] = visheight;
		tp->ti_Tag = WA_Zoom; tp->ti_Data = (ULONG) v->altwinpos; tp++;
	
		//tp->ti_Tag = WA_IDCMP; tp->ti_Data = (ULONG) v->idcmpmask; tp++;
		tp->ti_Tag = TAG_DONE;
		
		v->window = OpenWindowTagList(NULL, tags);
		if (v->window == TNULL) break;

		setinputmask(mod, v, 0);
		
		updatewindowparameters(mod, v);
		
		SetABPenDrMd(v->window->RPort, 1, 0, JAM2);
		ActivateWindow(v->window);

		v->keyevent.ie_Class = IECLASS_RAWKEY;
		v->keyevent.ie_SubClass = 0;
		v->active = TTRUE;

		return TTRUE;
	}
	
	vis_exit(mod);
	return TFALSE;
}

/*****************************************************************************/

static TVPEN
allocpen(TMOD_VIS *mod, VISUAL *v, TUINT rgb)
{
	ULONG r, g, b;
	LONG amigapen;

	r = (rgb & 0xff0000); r = r | (r << 8); r = r | (r >> 16);
	g = (rgb & 0x00ff00); g = g | (g >> 8); g = g | (g << 16);
	b = (rgb & 0x0000ff); b = b | (b << 8); b = b | (b << 16);

	amigapen = ObtainBestPenA(v->screen->ViewPort.ColorMap, r,g,b, NULL);
	v->amigapentab[amigapen]++;
	return (TVPEN) amigapen;
}

/*****************************************************************************/

static TVOID
freepen(TMOD_VIS *mod, VISUAL *v, TVPEN pen)
{
	LONG amigapen = (LONG) pen;
	if (amigapen >= 0)
	{
		ReleasePen(v->screen->ViewPort.ColorMap, amigapen);
		v->amigapentab[amigapen]--;
	}
}

/*****************************************************************************/

static TVOID
setbgpen(TMOD_VIS *mod, VISUAL *v, TVPEN p)
{
	ULONG pen = (ULONG) p;
	if (pen != v->bgpen)
	{
		SetBPen(v->window->RPort, pen);
		v->bgpen = pen;
	}
}

/*****************************************************************************/

static TVOID
setfgpen(TMOD_VIS *mod, VISUAL *v, TVPEN p)
{
	ULONG pen = (ULONG) p;
	if (pen != v->fgpen)
	{
		SetAPen(v->window->RPort, pen);
		v->fgpen = pen;
	}
}

/*****************************************************************************/

static TVOID
frect(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	RectFill(v->window->RPort, (LONG) rect[0], (LONG) rect[1], 
		(LONG) rect[0] + rect[2] - 1, (LONG) rect[1] + rect[3] - 1);
}

/*****************************************************************************/

static TVOID
line(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	Move(v->window->RPort, (LONG) rect[0], (LONG) rect[1]);
	Draw(v->window->RPort, (LONG) rect[2], (LONG) rect[3]);
}

/*****************************************************************************/

static TVOID
rect(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	struct RastPort *rp = v->window->RPort;
	Move(rp, (LONG) rect[0], (LONG) rect[1]);
	Draw(rp, (LONG) rect[0] + rect[2] - 1, (LONG) rect[1]);
	Draw(rp, (LONG) rect[0] + rect[2] - 1, (LONG) rect[1] + rect[3] - 1);
	Draw(rp, (LONG) rect[0], (LONG) rect[1] + rect[3] - 1);
	Draw(rp, (LONG) rect[0], (LONG) rect[1]);
}

/*****************************************************************************/

static TVOID
plot(TMOD_VIS *mod, VISUAL *v, TINT x, TINT y)
{
	Move(v->window->RPort, (LONG) x, (LONG) y);
	Draw(v->window->RPort, (LONG) x, (LONG) y);
}

/*****************************************************************************/

static TVOID
clear(TMOD_VIS *mod, VISUAL *v)
{
	RectFill(v->window->RPort, 0, 0, v->innerwidth - 1, v->innerheight - 1);
}

/*****************************************************************************/

static TVOID
drawtext(TMOD_VIS *mod, VISUAL *v, TINT x, TINT y, TSTRPTR text, TINT len)
{
	struct RastPort *rp = v->window->RPort;
	Move(rp, (LONG) x * rp->TxWidth, (LONG) y * rp->TxHeight + rp->TxBaseline);
	Text(rp, text, (ULONG) len);
}

/*****************************************************************************/

static TBOOL 
initarea(TMOD_VIS *mod, VISUAL *v, TINT numpoly)
{
	if (numpoly != v->areasize)
	{
		numpoly = (numpoly + 15) & ~15;
		if (!v->rasbuffer)
		{
			v->rasbuffer = AllocMem(512, MEMF_CHIP);
			if (v->rasbuffer)
			{
				v->oldtmpras = v->window->RPort->TmpRas;
				v->oldareainfo = v->window->RPort->AreaInfo;
				InitTmpRas(&v->tmpras, v->rasbuffer, 512);
				v->window->RPort->TmpRas = &v->tmpras;			
			}
		}

		if (v->rasbuffer)
		{
			if (v->areabuf) FreeMem(v->areabuf, v->areasize * 5);
			v->areabuf = AllocMem((long)(numpoly * 5), MEMF_ANY);
			if (v->areabuf)
			{
				v->areasize = numpoly;
				InitArea(&v->areainfo, v->areabuf, numpoly);
				v->window->RPort->AreaInfo = &v->areainfo;	
			}
		}
		else
		{
			v->areasize = 0;
		}
	}
	
	return (TBOOL) (numpoly == v->areasize);
}

static TVOID
fpoly(TMOD_VIS *mod, VISUAL *v, TINT16 *array, TINT num)
{
	if (initarea(mod, v, num + 1))
	{
		TINT i;
		TINT16 x, y;
		x = *array++;
		y = *array++;			
		AreaMove(v->window->RPort, x, y);
		for (i = 1; i < num; ++i)
		{
			AreaDraw(v->window->RPort, *array++, *array++);
		}
		AreaDraw(v->window->RPort, x, y);
		AreaEnd(v->window->RPort);
	}
}

/*****************************************************************************/

static TVOID
scroll(TMOD_VIS *mod, VISUAL *v, TINT *srect)
{
	ScrollRaster(v->window->RPort, 
		(LONG) srect[4], (LONG) srect[5], (LONG) srect[0], (LONG) srect[1],
		(LONG) srect[0] + srect[2] - 1, (LONG) srect[1] + srect[3] - 1);
}

/*****************************************************************************/

static TVOID
drawrgb(TMOD_VIS *mod, VISUAL *v, TUINT *buf, TINT *rrect)
{
	TINT w = rrect[2];
	TINT h = rrect[3];

	if (!v->drawhandle)
	{
		v->drawhandle = ObtainDrawHandleA(NULL, v->window->RPort,
			v->screen->ViewPort.ColorMap, TNULL);
	}
	
	if (v->drawhandle)
	{
		if (v->ddh)
		{
			if (w != v->ddhw || h != v->ddhh)
			{
				DeleteDirectDrawHandle(v->ddh);
				v->ddh = TNULL;
			}
		}
	
		if (!v->ddh)
		{
			v->ddh = CreateDirectDrawHandleA(v->drawhandle, w, h, w, h, TNULL);
			v->ddhw = w;
			v->ddhh = h;
		}
		
		if (v->ddh)
		{
			struct TagItem tags[] = {
				{ GGFX_SourceWidth, 0 },
				{ TAG_DONE, TAG_DONE }
			};
			tags[0].ti_Data = rrect[4];
			DirectDrawTrueColorA(v->ddh, 
				(ULONG *) buf, rrect[0], rrect[1], tags);
		}
	}
}

/*****************************************************************************/

struct getattrdata
{
	TMOD_VIS *mod;
	VISUAL *v;
	TINT num;				
};

static TCALLBACK TBOOL
getattrfunc(struct getattrdata *data, TTAGITEM *item)
{
	VISUAL *v = data->v;
	switch (item->tti_Tag)
	{
		default:
			return TTRUE;
		case TVisual_PixWidth:
			*((TINT *) item->tti_Value) = v->innerwidth;
			break;
		case TVisual_PixHeight:
			*((TINT *) item->tti_Value) = v->innerheight;
			break;
		case TVisual_TextWidth:
			*((TINT *) item->tti_Value) =
				v->innerwidth / v->window->RPort->TxWidth;
			break;
		case TVisual_TextHeight:
			*((TINT *) item->tti_Value) =
				v->innerheight / v->window->RPort->TxHeight;
			break;
		case TVisual_FontWidth:
			*((TINT *) item->tti_Value) = v->window->RPort->TxWidth;
			break;
		case TVisual_FontHeight:
			*((TINT *) item->tti_Value) = v->window->RPort->TxHeight;
			break;
	}
	data->num++;
	return TTRUE;
}

/*****************************************************************************/
/* 
**	return value indicates whether this command
**	is a cooperation point for message processing
*/

LOCAL TVOID
vis_docmd(TMOD_VIS *mod, TVREQ *req)
{
	VISUAL *v = mod->hostspecific;
	switch (req->vis_Req.io_Command)
	{
		case TVCMD_FLUSH:
			break;

		case TVCMD_ALLOCPEN:
			req->vis_Op.AllocPen.Pen =	
				allocpen(mod, v, req->vis_Op.AllocPen.RGB);
			break;

		case TVCMD_FREEPEN:
			freepen(mod, v, req->vis_Op.FreePen.Pen);
			break;

		case TVCMD_SETINPUT:
			setinputmask(mod, v, req->vis_Op.SetInput.Mask);
			break;

		case TVCMD_CLEAR:
			setfgpen(mod, v, req->vis_Op.Clear.Pen);
			clear(mod, v);
			break;

		case TVCMD_FRECT:
			setfgpen(mod, v, req->vis_Op.FRect.Pen);
			frect(mod, v, req->vis_Op.FRect.Rect);
			break;

		case TVCMD_RECT:
			setfgpen(mod, v, req->vis_Op.Rect.Pen);
			rect(mod, v, req->vis_Op.Rect.Rect);
			break;

		case TVCMD_LINE:
			setfgpen(mod, v, req->vis_Op.Line.Pen);
			line(mod, v, req->vis_Op.Line.Rect);
			break;

		case TVCMD_PLOT:
			setfgpen(mod, v, req->vis_Op.Plot.Pen);
			plot(mod, v, req->vis_Op.Plot.X, req->vis_Op.Plot.Y);
			break;

		case TVCMD_GETATTRS:
		{					
			struct getattrdata data;
			data.mod = mod;
			data.v = v;
			data.num = 0;
			TForEachTag(req->vis_Op.GetAttrs.Tags, 
				(TTAGFOREACHFUNC) getattrfunc, &data);
			req->vis_Op.GetAttrs.Num = data.num;
			break;
		}			

		case TVCMD_TEXT:
			setbgpen(mod, v, req->vis_Op.Text.BGPen);
			setfgpen(mod, v, req->vis_Op.Text.FGPen);
			drawtext(mod, v, req->vis_Op.Text.X, req->vis_Op.Text.Y,
				req->vis_Op.Text.Text, req->vis_Op.Text.Length);
			break;

		case TVCMD_DRAWRGB:
			drawrgb(mod, v, req->vis_Op.DrawRGB.Buf, 
				req->vis_Op.DrawRGB.RRect);
			break;
			
		case TVCMD_FPOLY:
			setfgpen(mod, v, req->vis_Op.FPoly.Pen);
			fpoly(mod, v, req->vis_Op.FPoly.Array, req->vis_Op.FPoly.Num);
			break;

		case TVCMD_SCROLL:
			scroll(mod, v, req->vis_Op.Scroll.SRect);
			break;
	}
}

/*****************************************************************************/

LOCAL TVOID
vis_wake(TMOD_VIS *mod)
{
	VISUAL *v = mod->hostspecific;
	Forbid();
	v->wakecount++;
	Signal(v->window->UserPort->mp_SigTask, 1L << v->wakesig);
	Permit();
}

/*****************************************************************************/

static TIMSG *
getimsg(TMOD_VIS *mod, VISUAL *v)
{
	TIMSG *imsg = (TIMSG *) TFirstNode(&v->imsgpool);
	if (imsg == TNULL)
	{
		imsg = TExecAllocMsg(TExecBase, sizeof(TIMSG));
		TAddTail(&v->imsgpool, (struct TNode *) imsg);
	}
	if (imsg)
	{
		imsg->timsg_Type = TITYPE_NONE;
		imsg->timsg_Code = 0;
	}
	return imsg;
}

static TVOID
sendimsg(TMOD_VIS *mod, VISUAL *v, TIMSG *imsg)
{
	if (imsg->timsg_Type & v->eventmask)
	{
		TRemove((struct TNode *) imsg);
		imsg->timsg_Qualifier = v->qualifier;
		imsg->timsg_MouseX = v->mousex;
		imsg->timsg_MouseY = v->mousey;
		vis_sendimsg(mod, imsg);
	}
}

static TVOID 
getmsgprops(VISUAL *v, struct IntuiMessage *amsg, TIMSG *imsg)
{
	TUINT tquali = 0;
	TUINT aquali = amsg->Qualifier;

	if (aquali & IEQUALIFIER_LSHIFT) tquali |= TKEYQ_LSHIFT;		
	if (aquali & IEQUALIFIER_RSHIFT) tquali |= TKEYQ_RSHIFT;		
	if (aquali & IEQUALIFIER_CONTROL) tquali |= TKEYQ_LCTRL;
	if (aquali & IEQUALIFIER_LALT) tquali |= TKEYQ_LALT;
	if (aquali & IEQUALIFIER_RALT) tquali |= TKEYQ_RALT;
	if (aquali & IEQUALIFIER_LCOMMAND) tquali |= TKEYQ_LPROP;
	if (aquali & IEQUALIFIER_RCOMMAND) tquali |= TKEYQ_RPROP;
	if (aquali & IEQUALIFIER_NUMERICPAD) tquali |= TKEYQ_NUMBLOCK;

	v->qualifier = tquali;
	v->mousex = amsg->MouseX - v->window->BorderLeft;
	v->mousey = amsg->MouseY - v->window->BorderTop;
}

LOCAL TUINT
vis_wait(TMOD_VIS *mod, TUINT waitsig)
{
	VISUAL *v = mod->hostspecific;
	struct IntuiMessage *amsg;
	TIMSG *imsg;

	Forbid();
	if (v->wakecount > 0)
	{
		v->wakecount--;
		Permit();
	}
	else
	{
		Permit();
		Wait((1L << v->window->UserPort->mp_SigBit) | (1L << v->wakesig));
	}

	while ((amsg = (struct IntuiMessage *) GetMsg(v->window->UserPort)))
	{
		imsg = getimsg(mod, v);
		if (imsg)
		{
			getmsgprops(v, amsg, imsg);
		
			switch (amsg->Class)
			{
				case ACTIVEWINDOW:
					imsg->timsg_Type = TITYPE_FOCUS;
					imsg->timsg_Code = 1;
					break;
				case INACTIVEWINDOW:
					imsg->timsg_Type = TITYPE_FOCUS;
					break;
				case CLOSEWINDOW:
					imsg->timsg_Type = TITYPE_CLOSE;
					break;
				case NEWSIZE:
					updatewindowparameters(mod, v);
					v->pending |= TITYPE_REFRESH | TITYPE_NEWSIZE;
					break;
				case REFRESHWINDOW:
					v->pending |= TITYPE_REFRESH;
					break;
				case MOUSEBUTTONS:
					imsg->timsg_Type = TITYPE_MOUSEBUTTON;
					switch (amsg->Code)
					{
						default:
							imsg->timsg_Type = TITYPE_NONE;
							break;

						case SELECTUP:
							imsg->timsg_Code = TMBCODE_LEFTUP;
							break;
						case SELECTDOWN:
							imsg->timsg_Code = TMBCODE_LEFTDOWN;
							break;
						case MENUUP:
							imsg->timsg_Code = TMBCODE_RIGHTUP;
							break;
						case MENUDOWN:
							imsg->timsg_Code = TMBCODE_RIGHTDOWN;
							break;
					}
					break;

				case MOUSEMOVE:
					imsg->timsg_Type = TITYPE_MOUSEMOVE;
					break;

				case RAWKEY:
				{
					TUINT newcode;
					if (amsg->Code >= 80 && amsg->Code <= 89)
					{
						newcode = TKEYC_F1 + amsg->Code - 80;
					}
					else
					{
						switch (amsg->Code)
						{
							case 65:
								newcode = TKEYC_BCKSPC;
								break;
							case 66:
								newcode = TKEYC_TAB;
								break;
							case 68:
								newcode = TKEYC_ENTER;
								break;
							case 69:
								newcode = TKEYC_ESC;
								break;
							case 70:
								newcode = TKEYC_DEL;
								break;
							case 76:
								newcode = TKEYC_CRSRUP;
								break;
							case 77:
								newcode = TKEYC_CRSRDOWN;
								break;
							case 78:
								newcode = TKEYC_CRSRRIGHT;
								break;
							case 79:
								newcode = TKEYC_CRSRLEFT;
								break;
							case 95:
								newcode = TKEYC_HELP;
								break;
							default:
							{
								LONG len;
	
								/* map out all qualifiers except shift: */
								v->keyevent.ie_Qualifier = amsg->Qualifier & 
									(IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT);
								v->keyevent.ie_Code = amsg->Code;
				
								if ((len = MapRawKey(&v->keyevent, v->keybuf, 
									9, NULL)) == 1)
								{
									v->keybuf[len] = 0;
									newcode = (TUINT) v->keybuf[0];
								}
								else
								{
									newcode = 0;
								}
							}
						}
					}
	
					if (newcode)
					{				
						imsg->timsg_Type = TITYPE_COOKEDKEY;
						imsg->timsg_Code = newcode;
					}

					break;
				}
			}
			
			sendimsg(mod, v, imsg);
		}

		ReplyMsg((struct Message *) amsg);
	}
	
	while (v->pending)
	{
		imsg = getimsg(mod, v);
		if (imsg == TNULL) break;
		
		if (v->pending & TITYPE_REFRESH)
		{
			v->pending &= ~TITYPE_REFRESH;
			imsg->timsg_Type = TITYPE_REFRESH;
			sendimsg(mod, v, imsg);
			continue;
		}

		if (v->pending & TITYPE_NEWSIZE)
		{
			v->pending &= ~TITYPE_NEWSIZE;
			imsg->timsg_Type = TITYPE_NEWSIZE;
			sendimsg(mod, v, imsg);
			continue;
		}
	}
	

	return TExecSetSignal(TExecBase, 0, waitsig);
}

#undef SysBase
#undef IntuitionBase
#undef GfxBase
#undef GuiGFXBase
#undef GadToolsBase
#undef KeymapBase

/*****************************************************************************/
/*
**	Revision History
**	$Log: visual_host.c,v $
**	Revision 1.6  2005/09/13 02:43:36  tmueller
**	updated copyright reference
**	
**	Revision 1.5  2005/09/08 00:05:20  tmueller
**	cosmetic
**	
**	Revision 1.4  2004/04/18 14:18:00  tmueller
**	TTAG changed to TUINTPTR; atomdata, parseargv changed from TAPTR to TTAG
**	
**	Revision 1.3  2004/01/18 03:05:36  tmueller
**	Fixed tracking of TITYPE_MOUSEMOVE, qualifiers, mouse position
**	
**	Revision 1.2  2004/01/13 02:22:43  tmueller
**	New visual backend implementations added. They are no longer sub-modules.
**	
**	Revision 1.2  2003/12/13 14:12:56  tmueller
*/
