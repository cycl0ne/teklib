
/*
**	$Id: visual_host.c,v 1.2 2005/10/07 12:22:06 fschulze Exp $
**	teklib/mods/visual/ps2/visual_host.c - PS2 implementation
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/time.h>
#include <tek/proto/util.h>
#include <tek/inline/ps2.h>

#include <tek/proto/hal.h>		/* HAL module interface */
#include <tek/mod/hal.h>		/* HAL private stuff */
#include <tek/mod/ps2/hal.h>	/* HAL private stuff */

#include "visual_mod.h"

#include <stdlib.h>
#include <kernel.h>

#include "font.c"

/*****************************************************************************/

typedef struct
{
	gs_rgbaq_packed fgpen;
	gs_rgbaq_packed bgpen;
	
	struct TPS2ModBase *ps2base;

	GSimage font;
	GSimage backbuf;
	
	TINT scale;
	TFLOAT scalef;
	
	TSTRPTR title;
	TINT reqwidth, reqheight;	
	TINT winwidth, winheight;
	TINT fontwidth, fontheight;
	TINT textwidth, textheight;
	
	TINT wakecount;
	TUINT eventmask;
	
	TLIST imsgpool;
	char *tempbuf;
	
} VISUAL;

//extern TUINT8 font0[];
//extern TUINT8 fontchartab[];

#define FTWIDTH		512
#define FTHEIGHT	8

#define TPS2Base v->ps2base

/*****************************************************************************/

static TVOID
setinputmask(TMOD_VIS *mod, VISUAL *v, TUINT eventmask)
{
	if (eventmask & TITYPE_REFRESH)
		;
	if (eventmask & TITYPE_MOUSEOVER)
		;
	if (eventmask & TITYPE_FOCUS)
		;
	if (eventmask & TITYPE_NEWSIZE)
		;
	if (eventmask & TITYPE_COOKEDKEY)
		;
	if (eventmask & TITYPE_MOUSEMOVE)
		;
	if (eventmask & TITYPE_MOUSEBUTTON)
		;

	v->eventmask = eventmask;

}

/*****************************************************************************/

static TVPEN
allocpen(TMOD_VIS *mod, VISUAL *v, TUINT rgb)
{
	return (TVPEN)((TUINT *)rgb);
}

/*****************************************************************************/

static TVOID
freepen(TMOD_VIS *mod, VISUAL *v, TVPEN pen)
{

}

/*****************************************************************************/

static TVOID
setbgpen(TMOD_VIS *mod, VISUAL *v, TVPEN pen)
{
	v->bgpen.R = ((TUINT)pen >> 16) & 0xff;
	v->bgpen.G = ((TUINT)pen >> 8)  & 0xff;
	v->bgpen.B = ((TUINT)pen)       & 0xff;
}

/*****************************************************************************/

static TVOID
setfgpen(TMOD_VIS *mod, VISUAL *v, TVPEN pen)
{
	v->fgpen.R = ((TUINT)pen >> 16) & 0xff;
	v->fgpen.G = ((TUINT)pen >> 8)  & 0xff;
	v->fgpen.B = ((TUINT)pen)       & 0xff;
}

/*****************************************************************************/

static TVOID
frect(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	g_drawFRect(GS_CTX2, rect[0], rect[1], rect[2], rect[3], &v->fgpen);
}

/*****************************************************************************/

static TVOID
line(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	g_drawLine(GS_CTX2, rect[0], rect[1], rect[2], rect[3], &v->fgpen);
}

/*****************************************************************************/

static TVOID
rect(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	g_drawRect(GS_CTX2, rect[0], rect[1], rect[2], rect[3], &v->fgpen);	
}

/*****************************************************************************/

static TVOID
plot(TMOD_VIS *mod, VISUAL *v, TINT x, TINT y)
{
	g_drawPoint(GS_CTX2, x, y, &v->fgpen);
}

/*****************************************************************************/

static TVOID
clear(TMOD_VIS *mod, VISUAL *v)
{
	g_drawFRect(GS_CTX2, 0, 0, v->winwidth, v->winheight, &v->fgpen);
}

/*****************************************************************************/

static TVOID
drawtext(TMOD_VIS *mod, VISUAL *v, TINT x, TINT y, TSTRPTR text, TINT len)
{
	/* FIXME: use pens */
	
	TINT i, posx = x * v->fontwidth, posy = y * v->fontheight;

	for (i = 0; i < len; i++)
	{
		int tx = fontchartab[(int)text[i]] * v->fontwidth;
		
		if (text[i] != ' ')
		{
			g_drawTRectUV(GS_CTX2, posx,posy, posx+v->textwidth,posy+v->textheight,
				tx,0,tx+v->fontwidth,v->fontheight, &v->font);
		}
		
		posx += v->textwidth;
		
		if (posx >= v->winwidth)
		{
			posy += v->fontheight;
			posx = x * v->fontwidth;
		}
		
		if (posy >= v->winheight)
		{
			posy = y * v->fontheight;
			posx = x * v->fontwidth;
		}
	}
}

/*****************************************************************************/

static TVOID
fpoly(TMOD_VIS *mod, VISUAL *v, TINT16 *array, TINT num)
{
#if 0
	XFillPolygon(v->display, v->window, v->gc, (XPoint *) array, num, 
		Complex, CoordModeOrigin);
#endif
}

/*****************************************************************************/

static TVOID
scroll(TMOD_VIS *mod, VISUAL *v, TINT *srect)
{
#if 0
	XCopyArea(v->display, v->window, v->window, v->gc,
		srect[0], srect[1], srect[2], srect[3], -srect[4], -srect[5]);
#endif
}

/*****************************************************************************/

static TVOID
drawrgb(TMOD_VIS *mod, VISUAL *v, TUINT *buf, TINT *rrect)
{
	TUINT y, x;
	GSimage img;
	TUINT *p;
	
	g_initImage(&img, rrect[2], rrect[3], GS_PSMCT32, buf);
	
	p = img.data;
	
	for (y = 0; y < rrect[3]; ++y)
	{
		for (x = 0; x < rrect[2]; ++x)
		{
			TUINT x = *p;
			*p++ = ((x & 0xff0000) >> 16) | (x & 0x00ff00) | ((x & 0x0000ff) << 16) | 0x80000000;
		}
	}
	
	g_loadImage(&img);
	g_drawTRect(GS_CTX2, rrect[0], rrect[1], rrect[2], rrect[3], &img);
	g_freeImage(&img);
}

/*****************************************************************************/

TVOID g_drawtRect1(VISUAL *v, TINT x, TINT y, TINT w, TINT h, GSimage *gsimage)
{
	TINT dx, dy;
	TINT tw, th;
	GSdisplay *disp;
	TINT tbw;
	TQWDATA *d;
	GSinfo *gsinfo = TPS2Base->ps2_GSInfo;
	
	tbw = gsimage->w/64;
		
	tw = u_ld(gsimage->w);
	th = u_ld(gsimage->h);
		
	disp = &gsinfo->gsi_ctx1.gsc_disp;
	dx = disp->gsd_xoffset;
	dy = disp->gsd_yoffset;
		
	g_setReg(GS_TEX0_1, GS_SETREG_TEX0(gsimage->tbp,tbw,gsimage->psm,tw,th,1,1,0,0,0,0,0));
	
	d = d_alloc(DMC_GIF, 5, TNULL, TNULL);
		
	d[0] = GIF_SET_GIFTAG(1, 1, 1, 0x156, 0, 4, 0x5353);
	d[1] = GIF_SET_UV(0,0);
	d[2] = GIF_SET_XYZ(dx+x,dy+y,1);
	d[3] = GIF_SET_UV(v->reqwidth-1,v->reqheight-1);
	d[4] = GIF_SET_XYZ(dx+x+w, dy+y+h, 1);
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
			*((TINT *) item->tti_Value) = v->reqwidth;
			break;
		case TVisual_PixHeight:
			*((TINT *) item->tti_Value) = v->reqheight;
			break;
		case TVisual_TextWidth:
			*((TINT *) item->tti_Value) = v->textwidth;
			break;
		case TVisual_TextHeight:
			*((TINT *) item->tti_Value) = v->textheight;
			break;
		case TVisual_FontWidth:
			*((TINT *) item->tti_Value) = v->fontwidth;
			break;
		case TVisual_FontHeight:
			*((TINT *) item->tti_Value) = v->fontheight;
			break;
	}
	data->num++;
	return TTRUE;
}

/*****************************************************************************/
/*
**	filled = getnextevent(visual, newimsg)
**	get next input event from visual object
**	and fill it into the supplied TIMSG structure.
*/

static TBOOL 
getnextevent(TMOD_VIS *mod, TIMSG *newimsg)
{
	return TFALSE;
}


/*****************************************************************************/

LOCAL TBOOL
vis_init(TMOD_VIS *mod)
{
	VISUAL *v = TExecAlloc0(TExecBase, mod->mmu, sizeof(VISUAL));
	if (v == TNULL) return TFALSE;
	mod->hostspecific = v;
	TInitList(&v->imsgpool);
	
	v->ps2base = TExecOpenModule(TExecBase, "ps2common", 0, TNULL);
	if (v->ps2base)
	{
		TUINT t, n = 0; 
		TINT fbp1_0, fbp2_0;
		
		v->winwidth = (TINT) TGetTag(mod->inittags, TVisual_PixWidth, (TTAG) 512);
		v->winheight = (TINT) TGetTag(mod->inittags, TVisual_PixHeight, (TTAG) 256);
		
		printf("winwidth requested:  %d\n", v->winwidth);
		printf("winheight requested: %d\n", v->winheight);
		
		v->reqwidth = v->winwidth;
		v->reqheight = v->winheight;
		
		while ((t = 1 << n) < v->winwidth) n++;
		v->winwidth = t;
		n = 0;
		while ((t = 1 << n) < v->winheight) n++;
		v->winheight = t;
		
		printf("winwidth corrected:  %d\n", v->winwidth);
		printf("winheight corrected: %d\n", v->winheight);
		
		g_initScreen(PAL_640_256_32, GS_INTERLACE, GS_FRAME);
		g_initDisplay(GS_CTX1, 700, 37, 4, 1, 640, 256, 32, 2048, 2048);
		g_initDisplay(GS_CTX2, 700, 37, 4, 1, v->winwidth, v->winheight, 32, 2048, 2048);
		
		fbp2_0 = g_allocMem(GS_CTX2, GS_LMFBUF, GS_PSMCT32);
		fbp1_0 = g_allocMem(GS_CTX1, GS_LMFBUF, GS_PSMCT32);
		g_initContext(GS_CTX1, fbp1_0, fbp1_0, GS_PSMCT32);
		g_initContext(GS_CTX2, fbp2_0, fbp2_0, GS_PSMCT32);
		
		g_initTexEnv(0, 10, 10);
		g_setReg(GS_ALPHA_1, GS_SETREG_ALPHA(0,0,0,0,0));
		g_setReg(GS_ALPHA_2, GS_SETREG_ALPHA(0,1,0,1,0));
		g_setReg(GS_TEX1_1, GS_SETREG_TEX1(0,0,1,1,0,0,0));
		g_setReg(GS_TEX1_2, GS_SETREG_TEX1(0,0,1,1,0,0,0));
		
		g_clearScreen(GS_CTX1 | GS_CTX2);
		
		g_initImage(&v->font, FTWIDTH, FTHEIGHT, GS_PSMCT32, font0);
		g_loadImage(&v->font);
	
		d_commit(DMC_GIF);
						
		v->fontwidth = 8; v->fontheight = 8;
		v->textwidth = 8; v->textheight = 8;
			
		/* backbuffer setup */
		
		g_enableContext(GS_CTX2,0);
		v->backbuf.w = v->winwidth;
		v->backbuf.h = v->winheight;
		v->backbuf.psm = GS_PSMCT32;
		v->backbuf.tbp = fbp2_0 * 2048 / 64;

	printf("*** init ok\n");
		return TTRUE;
	}
	
	vis_exit(mod);
	return TFALSE;
}

/*****************************************************************************/

LOCAL TVOID
vis_exit(TMOD_VIS *mod)
{
	/* FIXME: will crash when called from vis_init */
	
	VISUAL *v = mod->hostspecific;
	printf("%s\n", __FUNCTION__);
	if (v)
	{
		struct TNode *imsg;
		while ((imsg = TRemHead(&v->imsgpool))) TExecFree(TExecBase, imsg);

		TExecFree(TExecBase, v->tempbuf);		
		TExecCloseModule(TExecBase, v->ps2base);
		TExecFree(TExecBase, v);
		mod->hostspecific = TNULL;
	}
}

/*****************************************************************************/

LOCAL TUINT
vis_wait(TMOD_VIS *mod, TUINT waitsig)
{
	VISUAL *v = mod->hostspecific;
	
	THALLock(TExecGetHALBase(TExecBase), TNULL);
	if (v->wakecount > 0)
	{
		v->wakecount--;
		THALUnlock(TExecGetHALBase(TExecBase), TNULL);
	}
	else
	{
		THALUnlock(TExecGetHALBase(TExecBase), TNULL);
		SleepThread();
	}
	
	return TExecSetSignal(TExecBase, 0, waitsig);
}

/*****************************************************************************/

LOCAL TVOID
vis_wake(TMOD_VIS *mod)
{
	VISUAL *v = mod->hostspecific;
	struct HALPS2Thread *thread = THALGetObject(mod->task, struct HALPS2Thread);

	THALLock(TExecGetHALBase(TExecBase), TNULL);
	v->wakecount++;
	THALUnlock(TExecGetHALBase(TExecBase), TNULL);

	WakeupThread(thread->hpt_TID);
}

/*****************************************************************************/

void blafasel(int x, int *dummy)
{
	int i;
	
	for (i = 0; i < x; ++i)
		*dummy = i+i;
}

/*****************************************************************************/

LOCAL TVOID
vis_docmd(TMOD_VIS *mod, TVREQ *req)
{
	VISUAL *v = mod->hostspecific;
	
	switch (req->vis_Req.io_Command)
	{
		case TVCMD_FLUSH:
			g_drawtRect1(v, 0, 0, 640, 256, &v->backbuf);
   			d_commit(DMC_GIF);   			
			break;

		case TVCMD_SETINPUT:
			break;

		case TVCMD_CLEAR:
			setfgpen(mod, v, req->vis_Op.Clear.Pen);
			clear(mod, v);
			break;

		case TVCMD_DRAWRGB:
			drawrgb(mod, v, req->vis_Op.DrawRGB.Buf,
				req->vis_Op.DrawRGB.RRect);
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

		case TVCMD_ALLOCPEN:
			req->vis_Op.AllocPen.Pen =	
				allocpen(mod, v, req->vis_Op.AllocPen.RGB);
			break;

		case TVCMD_FREEPEN:
			freepen(mod, v, req->vis_Op.FreePen.Pen);
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

		case TVCMD_TEXT:
			setbgpen(mod, v, req->vis_Op.Text.BGPen);
			setfgpen(mod, v, req->vis_Op.Text.FGPen);
			drawtext(mod, v, req->vis_Op.Text.X, req->vis_Op.Text.Y,
				req->vis_Op.Text.Text, req->vis_Op.Text.Length);
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
/*
**	Revision History
**	$Log: visual_host.c,v $
**	Revision 1.2  2005/10/07 12:22:06  fschulze
**	renamed some primitive drawing functions
**	
**	Revision 1.1  2005/09/18 12:49:21  fschulze
**	added
**	
*/
