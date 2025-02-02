
/*
**	$Id: visual_api.c,v 1.5 2005/09/13 02:43:36 tmueller Exp $
**	teklib/mods/visual/visual_api.c - Visual API functions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "visual_mod.h"

/*****************************************************************************/

EXPORT TAPTR 
tv_getport(TMOD_VIS *mod)
{
	return mod->userport;
}

/*****************************************************************************/

static TVOID
doasync(TMOD_VIS *mod, TVREQ *req)
{
	TPutIO((struct TIORequest *) req);
	TAddTail(&mod->waitlist, (struct TNode *) req);
}

/*****************************************************************************/

EXPORT TVPEN
tv_allocpen(TMOD_VIS *mod, TUINT rgb)
{
	TVPEN pen;
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_ALLOCPEN;
	req->vis_Op.AllocPen.RGB = rgb;
	TDoIO((struct TIORequest *) req);
	pen = req->vis_Op.AllocPen.Pen;
	vis_ungetreq(mod, req);
	return pen;
}

/*****************************************************************************/

EXPORT TVOID
tv_frect(TMOD_VIS *mod, TINT x, TINT y, TINT w, TINT h, TVPEN pen)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_FRECT;
	req->vis_Op.FRect.Pen = pen;
	req->vis_Op.FRect.Rect[0] = x;
	req->vis_Op.FRect.Rect[1] = y;
	req->vis_Op.FRect.Rect[2] = w;
	req->vis_Op.FRect.Rect[3] = h;
	doasync(mod, req);
}

/*****************************************************************************/

EXPORT TVOID
tv_clear(TMOD_VIS *mod, TVPEN pen)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_CLEAR;
	req->vis_Op.Clear.Pen = pen;
	doasync(mod, req);
}

/*****************************************************************************/

EXPORT TVOID
tv_drawrgb(TMOD_VIS *mod, TINT x, TINT y, TUINT *buf, TINT w, TINT h, TINT totw)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_DRAWRGB;
	req->vis_Op.DrawRGB.Buf = buf;
	req->vis_Op.DrawRGB.RRect[0] = x;
	req->vis_Op.DrawRGB.RRect[1] = y;
	req->vis_Op.DrawRGB.RRect[2] = w;
	req->vis_Op.DrawRGB.RRect[3] = h;
	req->vis_Op.DrawRGB.RRect[4] = totw;
	TDoIO((struct TIORequest *) req);
	vis_ungetreq(mod, req);
}

/*****************************************************************************/

EXPORT TVOID
tv_flush(TMOD_VIS *mod)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_FLUSH;
	TDoIO((struct TIORequest *) req);
	vis_ungetreq(mod, req);
}

/*****************************************************************************/

EXPORT TVOID
tv_freepen(TMOD_VIS *mod, TVPEN pen)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_FREEPEN;
	req->vis_Op.FreePen.Pen = pen;
	doasync(mod, req);
}

/*****************************************************************************/

EXPORT TVOID
tv_rect(TMOD_VIS *mod, TINT x, TINT y, TINT w, TINT h, TVPEN pen)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_RECT;
	req->vis_Op.Rect.Pen = pen;
	req->vis_Op.Rect.Rect[0] = x;
	req->vis_Op.Rect.Rect[1] = y;
	req->vis_Op.Rect.Rect[2] = w;
	req->vis_Op.Rect.Rect[3] = h;
	doasync(mod, req);
}

/*****************************************************************************/

EXPORT TVOID
tv_line(TMOD_VIS *mod, TINT x1, TINT y1, TINT x2, TINT y2, TVPEN pen)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_LINE;
	req->vis_Op.Line.Pen = pen;
	req->vis_Op.Line.Rect[0] = x1;
	req->vis_Op.Line.Rect[1] = y1;
	req->vis_Op.Line.Rect[2] = x2;
	req->vis_Op.Line.Rect[3] = y2;
	doasync(mod, req);
}

/*****************************************************************************/

EXPORT TVOID
tv_linearray(TMOD_VIS *mod, TINT16 *array, TINT n, TVPEN pen)
{
	TINT x0, y0, x1, y1;

	x0 = *array++;
	y0 = *array++;

	while (--n)
	{
		x1 = *array++;
		y1 = *array++;
		tv_line(mod, x0,y0, x1,y1, pen);
		x0 = x1;
		y0 = y1;
	}
}

/*****************************************************************************/

EXPORT TVOID
tv_plot(TMOD_VIS *mod, TINT x, TINT y, TVPEN pen)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_PLOT;
	req->vis_Op.Plot.Pen = pen;
	req->vis_Op.Plot.X = x;
	req->vis_Op.Plot.Y = y;
	doasync(mod, req);
}

/*****************************************************************************/

EXPORT TVOID
tv_scroll(TMOD_VIS *mod, TINT x, TINT y, TINT w, TINT h, TINT dx, TINT dy)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_SCROLL;
	req->vis_Op.Scroll.SRect[0] = x;
	req->vis_Op.Scroll.SRect[1] = y;
	req->vis_Op.Scroll.SRect[2] = w;
	req->vis_Op.Scroll.SRect[3] = h;
	req->vis_Op.Scroll.SRect[4] = dx;
	req->vis_Op.Scroll.SRect[5] = dy;
	doasync(mod, req);
}

/*****************************************************************************/

EXPORT TVOID
tv_text(TMOD_VIS *mod, TINT x, TINT y, TSTRPTR s, TUINT l, TVPEN bg, TVPEN fg)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_TEXT;
	req->vis_Op.Text.BGPen = bg;
	req->vis_Op.Text.FGPen = fg;
	req->vis_Op.Text.X = x;
	req->vis_Op.Text.Y = y;
	req->vis_Op.Text.Text = s;
	req->vis_Op.Text.Length = l;
	TDoIO((struct TIORequest *) req);
	vis_ungetreq(mod, req);
}

/*****************************************************************************/

EXPORT TVOID
tv_flusharea(TMOD_VIS *mod, TINT x, TINT y, TINT w, TINT h)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_FLUSH;
	TDoIO((struct TIORequest *) req);
	vis_ungetreq(mod, req);
}

/*****************************************************************************/

EXPORT TUINT
tv_setinput(TMOD_VIS *mod, TUINT cmask, TUINT smask)
{
	TUINT oldmask = mod->inputmask;
	TUINT newmask = (oldmask & ~cmask) | smask;
	if (newmask != oldmask)
	{
		TVREQ *req = vis_getreq(mod);
		req->vis_Req.io_Command = TVCMD_SETINPUT;
		req->vis_Op.SetInput.Mask = newmask;
		TDoIO((struct TIORequest *) req);
		vis_ungetreq(mod, req);
		mod->inputmask = newmask;
	}
	return newmask;
}

/*****************************************************************************/

EXPORT TUINT
tv_getattrs(TMOD_VIS *mod, TTAGITEM *tags)
{
	TUINT num;
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_GETATTRS;
	req->vis_Op.GetAttrs.Tags = tags;
	TDoIO((struct TIORequest *) req);
	num = req->vis_Op.GetAttrs.Num;
	vis_ungetreq(mod, req);
	return num;
}

/*****************************************************************************/

EXPORT TAPTR
tv_attach(TMOD_VIS *mod, TTAGITEM *usertags)
{
	TTAGITEM vtags[2];
	vtags[0].tti_Tag = TVisual_Attach;
	vtags[0].tti_Value = (TTAG) mod;	/* the instance to attach to */
	vtags[1].tti_Tag = TTAG_MORE;
	vtags[1].tti_Value = (TTAG) usertags;

	/* Reference to self with an additional attachment tag */
	return TOpenModule("visual", 0, vtags);
}

/*****************************************************************************/

EXPORT TVOID
tv_fpoly(TMOD_VIS *mod, TINT16 *array, TINT num, TVPEN pen)
{
	TVREQ *req = vis_getreq(mod);
	req->vis_Req.io_Command = TVCMD_FPOLY;
	req->vis_Op.FPoly.Array = array;
	req->vis_Op.FPoly.Num = num;
	req->vis_Op.FPoly.Pen = pen;
	TDoIO((struct TIORequest *) req);
	vis_ungetreq(mod, req);
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: visual_api.c,v $
**	Revision 1.5  2005/09/13 02:43:36  tmueller
**	updated copyright reference
**	
**	Revision 1.4  2005/09/11 01:27:57  tmueller
**	cosmetic
**	
**	Revision 1.3  2004/04/18 14:18:00  tmueller
**	TTAG changed to TUINTPTR; atomdata, parseargv changed from TAPTR to TTAG
**	
**	Revision 1.2  2004/01/13 02:19:40  tmueller
**	Reimplemented as a fully-featured, asynchronous Exec I/O device
**	
**	Revision 1.2  2003/12/13 14:12:56  tmueller
*/
