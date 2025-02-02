
/*
**	display_fb_api.c - Framebuffer display driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "display_fb_mod.h"

/*****************************************************************************/

LOCAL void
fb_openvisual(FBDISPLAY *mod, struct TVFBRequest *req)
{
	TTAGITEM *tags = req->tvr_Op.OpenWindow.Tags;
	FBWINDOW *v;
	req->tvr_Op.OpenWindow.Window = TNULL;
	v = TExecAlloc0(mod->fbd_ExecBase, mod->fbd_MemMgr, sizeof(FBWINDOW));
	if (v)
	{
		TIMSG *imsg;
		TINT width = (TINT) TGetTag(tags, TVisual_Width, mod->fbd_Width);
		TINT height = (TINT) TGetTag(tags, TVisual_Height, mod->fbd_Height);
		v->fbv_Modulo = mod->fbd_Modulo + (mod->fbd_Width - width);
		v->fbv_InputMask = (TUINT) TGetTag(tags, TVisual_EventMask, 0);

		if (TISLISTEMPTY(&mod->fbd_VisualList))
		{
			/* Is root window */
			v->fbv_WinRect[0] = 0;
			v->fbv_WinRect[1] = 0;

			/* Open rendering instance: */
			if (mod->fbd_RndDevice)
			{
				mod->fbd_RndRequest->tvr_Req.io_Device = mod->fbd_RndDevice;
				mod->fbd_RndRequest->tvr_Req.io_Command = TVCMD_OPENWINDOW;
				mod->fbd_RndRequest->tvr_Req.io_ReplyPort = mod->fbd_RndRPort;
				mod->fbd_RndRequest->tvr_Op.OpenWindow.Window = TNULL;
				/* forward input message port to sub module: */
				mod->fbd_RndRequest->tvr_Op.OpenWindow.IMsgPort =
					req->tvr_Op.OpenWindow.IMsgPort;
				mod->fbd_RndRequest->tvr_Op.OpenWindow.Tags = tags;
				TExecDoIO(mod->fbd_ExecBase, &mod->fbd_RndRequest->tvr_Req);
				mod->fbd_RndInstance =
					mod->fbd_RndRequest->tvr_Op.OpenWindow.Window;
			}
		}
		else
		{
			/* Not root window: */
			v->fbv_WinRect[0] = (TINT) TGetTag(tags, TVisual_WinLeft, 0);
			v->fbv_WinRect[1] = (TINT) TGetTag(tags, TVisual_WinTop, 0);
		}

		v->fbv_WinRect[2] = v->fbv_WinRect[0] + width - 1;
		v->fbv_WinRect[3] = v->fbv_WinRect[1] + height - 1;
		v->fbv_PixelPerLine = width + v->fbv_Modulo;

		v->fbv_ClipRect[0] = v->fbv_WinRect[0];
		v->fbv_ClipRect[1] = v->fbv_WinRect[1];
		v->fbv_ClipRect[2] = v->fbv_WinRect[2];
		v->fbv_ClipRect[3] = v->fbv_WinRect[3];

		v->fbv_IMsgPort = req->tvr_Op.OpenWindow.IMsgPort;

		TInitList(&v->fbv_IMsgQueue);

		TInitList(&v->penlist);
		v->bgpen = TVPEN_UNDEFINED;
		v->fgpen = TVPEN_UNDEFINED;

		v->fbv_BufPtr = mod->fbd_BufPtr;

		TExecLock(mod->fbd_ExecBase, mod->fbd_Lock);

		/* init default font */
		v->curfont = mod->fbd_FontManager.deffont;
		mod->fbd_FontManager.defref++;

		/* add window on top of window stack: */
		TAddHead(&mod->fbd_VisualList, &v->fbv_Node);

		TExecUnlock(mod->fbd_ExecBase, mod->fbd_Lock);

		/* Reply instance: */
		req->tvr_Op.OpenWindow.Window = v;

		/* send refresh message: */

		if (fb_getimsg(mod, v, &imsg, TITYPE_REFRESH))
		{
			imsg->timsg_X = 0;
			imsg->timsg_Y = 0;
			imsg->timsg_Width = width;
			imsg->timsg_Height = height;
			TAddTail(&v->fbv_IMsgQueue, &imsg->timsg_Node);
		}
	}
}

/*****************************************************************************/

LOCAL void
fb_closevisual(FBDISPLAY *mod, struct TVFBRequest *req)
{
	struct FBPen *pen;
	struct Region *A, *B;
	FBWINDOW *v = req->tvr_Op.CloseWindow.Window;
	if (v == TNULL) return;

	TDBPRINTF(TDB_INFO,("Visual close\n"));

	/* traverse window stack; refresh B where A and B overlap ; A = A - B */

	A = fb_region_new(mod, v->fbv_WinRect);
	if (A)
	{
		struct TNode *next, *node = mod->fbd_VisualList.tlh_Head;
		TBOOL success = TTRUE;
		TBOOL below = TFALSE;
		for (; success && !fb_region_isempty(mod, A) &&
			(next = node->tln_Succ); node = next)
		{
			FBWINDOW *bv = (FBWINDOW *) node;

			if (!below)
			{
				if (bv == v)
					below = TTRUE;
				else
					/* above: subtract current from window to be closed: */
					success = fb_region_subrect(mod, A, bv->fbv_WinRect);
				continue;
			}

			if (bv->fbv_InputMask & TITYPE_REFRESH)
			{
				success = TFALSE;
				B = fb_region_new(mod, bv->fbv_WinRect);
				if (B)
				{
					if (fb_region_andregion(mod, B, A))
					{
						struct TNode *next, *node = B->rg_List.tlh_Head;
						for (; (next = node->tln_Succ); node = next)
						{
							TIMSG *imsg;
							struct RectNode *r = (struct RectNode *) node;
							if (fb_getimsg(mod, bv, &imsg, TITYPE_REFRESH))
							{
								imsg->timsg_X = r->rn_Rect[0];
								imsg->timsg_Y = r->rn_Rect[1];
								imsg->timsg_Width =
									r->rn_Rect[2] - r->rn_Rect[0] + 1;
								imsg->timsg_Height =
									r->rn_Rect[3] - r->rn_Rect[1] + 1;
								imsg->timsg_X -= bv->fbv_WinRect[0];
								imsg->timsg_Y -= bv->fbv_WinRect[1];
								TAddTail(&bv->fbv_IMsgQueue,
									&imsg->timsg_Node);
							}
						}
						success = TTRUE;
					}
					fb_region_destroy(mod, B);
				}
			}

			if (success)
				success = fb_region_subrect(mod, A, bv->fbv_WinRect);
		}

		fb_region_destroy(mod, A);
		if (!success)
			TDBPRINTF(20,("ERROR\n"));
	}


	TRemove(&v->fbv_Node);

	while ((pen = (struct FBPen *) TRemHead(&v->penlist)))
	{
		/* free pens */
		TRemove(&pen->node);
		TExecFree(mod->fbd_ExecBase, pen);
	}

	TExecLock(mod->fbd_ExecBase, mod->fbd_Lock);
	mod->fbd_FontManager.defref--;
	TExecUnlock(mod->fbd_ExecBase, mod->fbd_Lock);

	if (TISLISTEMPTY(&mod->fbd_VisualList) && mod->fbd_RndDevice)
	{
		/* last window: */
		mod->fbd_RndRequest->tvr_Req.io_Command = TVCMD_CLOSEWINDOW;
		mod->fbd_RndRequest->tvr_Op.CloseWindow.Window =
			mod->fbd_RndInstance;
		TExecDoIO(mod->fbd_ExecBase, &mod->fbd_RndRequest->tvr_Req);
	}

	TExecFree(mod->fbd_ExecBase, v);
}

/*****************************************************************************/

LOCAL void
fb_setinput(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.SetInput.Window;
	TUINT oldmask = mod->fbd_InputMask;
	struct TNode *node, *next;
	TUINT newmask = 0;

	req->tvr_Op.SetInput.OldMask = v->fbv_InputMask;
	v->fbv_InputMask = req->tvr_Op.SetInput.Mask;
	node = mod->fbd_VisualList.tlh_Head;
	for (; (next = node->tln_Succ); node = next)
		newmask |= v->fbv_InputMask;
	mod->fbd_InputMask = newmask;
	if (newmask != oldmask && mod->fbd_RndDevice)
	{
		mod->fbd_RndRequest->tvr_Req.io_Command = TVCMD_SETINPUT;
		mod->fbd_RndRequest->tvr_Op.SetInput.Window = mod->fbd_RndInstance;
		mod->fbd_RndRequest->tvr_Op.SetInput.Mask = req->tvr_Op.SetInput.Mask;
		TExecDoIO(mod->fbd_ExecBase, &mod->fbd_RndRequest->tvr_Req);
	}
}

/*****************************************************************************/

LOCAL void
fb_flush(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.Flush.Window;
	if (mod->fbd_RndDevice)
	{
		TINT ww = v->fbv_WinRect[2] - v->fbv_WinRect[0] + 1;
		TINT wh = v->fbv_WinRect[3] - v->fbv_WinRect[1] + 1;
		TINT tw = ww + v->fbv_Modulo;
		TINT x0 = req->tvr_Op.Flush.Rect[0];
		TINT y0 = req->tvr_Op.Flush.Rect[1];
		TINT x1 = req->tvr_Op.Flush.Rect[2];
		TINT y1 = req->tvr_Op.Flush.Rect[3];
		TINT w = x1 - x0 + 1;
		TINT h = y1 - y0 + 1;
		if (req->tvr_Op.Flush.Rect[0] < 0)
		{
			x0 = 0;
			y0 = 0;
			w = ww;
			h = wh;
		}
		mod->fbd_RndRequest->tvr_Req.io_Command = TVCMD_DRAWBUFFER;
		mod->fbd_RndRequest->tvr_Op.DrawBuffer.Window = mod->fbd_RndInstance;
		mod->fbd_RndRequest->tvr_Op.DrawBuffer.RRect[0] = x0;
		mod->fbd_RndRequest->tvr_Op.DrawBuffer.RRect[1] = y0;
		mod->fbd_RndRequest->tvr_Op.DrawBuffer.RRect[2] = w;
		mod->fbd_RndRequest->tvr_Op.DrawBuffer.RRect[3] = h;
		mod->fbd_RndRequest->tvr_Op.DrawBuffer.Buf =
			mod->fbd_BufPtr + (y0 * tw + x0) * 4;
		mod->fbd_RndRequest->tvr_Op.DrawBuffer.TotWidth = ww + v->fbv_Modulo;
		mod->fbd_RndRequest->tvr_Op.DrawBuffer.Tags = TNULL;
		TExecDoIO(mod->fbd_ExecBase, &mod->fbd_RndRequest->tvr_Req);
	}
}

/*****************************************************************************/

static void
setbgpen(FBDISPLAY *mod, FBWINDOW *v, TVPEN pen)
{
	if (pen != v->bgpen && pen != TVPEN_UNDEFINED)
		v->bgpen = pen;
}

static TVPEN
setfgpen(FBDISPLAY *mod, FBWINDOW *v, TVPEN pen)
{
	TVPEN oldpen = v->fgpen;
	if (pen != oldpen && pen != TVPEN_UNDEFINED)
	{
		v->fgpen = pen;
		if (oldpen == TVPEN_UNDEFINED) oldpen = pen;
	}
	return oldpen;
}

/*****************************************************************************/

LOCAL void
fb_allocpen(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.AllocPen.Window;
	TUINT rgb = req->tvr_Op.AllocPen.RGB;
	struct FBPen *pen = TExecAlloc(mod->fbd_ExecBase,
		mod->fbd_MemMgr, sizeof(struct FBPen));
	if (pen)
	{
		pen->rgb = rgb;
		TAddTail(&v->penlist, &pen->node);
		req->tvr_Op.AllocPen.Pen = (TVPEN) pen;
		return;
	}
	req->tvr_Op.AllocPen.Pen = TVPEN_UNDEFINED;
}

/*****************************************************************************/

LOCAL void
fb_freepen(FBDISPLAY *mod, struct TVFBRequest *req)
{
	struct FBPen *pen = (struct FBPen *) req->tvr_Op.FreePen.Pen;
	TRemove(&pen->node);
	TExecFree(mod->fbd_ExecBase, pen);
}

/*****************************************************************************/

LOCAL void
fb_frect(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.FRect.Window;
	TINT rect[4];
	struct FBPen *pen = (struct FBPen *) req->tvr_Op.FRect.Pen;
	setfgpen(mod, v, req->tvr_Op.FRect.Pen);
	rect[0] = req->tvr_Op.FRect.Rect[0] + v->fbv_WinRect[0];
	rect[1] = req->tvr_Op.FRect.Rect[1] + v->fbv_WinRect[1];
	rect[2] = req->tvr_Op.FRect.Rect[2];
	rect[3] = req->tvr_Op.FRect.Rect[3];
	fbp_drawfrect(v, rect, pen);
}

/*****************************************************************************/

LOCAL void
fb_line(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.Line.Window;
	struct FBPen *pen = (struct FBPen *) req->tvr_Op.Line.Pen;
	setfgpen(mod, v, req->tvr_Op.Line.Pen);
	fbp_drawline(v, req->tvr_Op.Line.Rect, pen);
}

/*****************************************************************************/

LOCAL void
fb_rect(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.Rect.Window;
	struct FBPen *pen = (struct FBPen *) req->tvr_Op.Rect.Pen;
	setfgpen(mod, v, req->tvr_Op.Rect.Pen);
	fbp_drawrect(v, req->tvr_Op.Rect.Rect, pen);
}

/*****************************************************************************/

LOCAL void
fb_plot(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.Plot.Window;
	TUINT x = req->tvr_Op.Plot.Rect[0];
	TUINT y = req->tvr_Op.Plot.Rect[1];
	struct FBPen *pen = (struct FBPen *) req->tvr_Op.Plot.Pen;
	setfgpen(mod, v, req->tvr_Op.Plot.Pen);
	fbp_drawpoint(v, x, y, pen);
}

/*****************************************************************************/

LOCAL void
fb_drawstrip(FBDISPLAY *mod, struct TVFBRequest *req)
{
	TINT i, x0, y0, x1, y1, x2, y2;
	FBWINDOW *v = req->tvr_Op.Strip.Window;
	TINT *array = req->tvr_Op.Strip.Array;
	TINT num = req->tvr_Op.Strip.Num;
	TTAGITEM *tags = req->tvr_Op.Strip.Tags;
	TVPEN pen = (TVPEN) TGetTag(tags, TVisual_Pen, TVPEN_UNDEFINED);
	TVPEN *penarray = (TVPEN *) TGetTag(tags, TVisual_PenArray, TNULL);

	if (num < 3) return;

	if (penarray)
		setfgpen(mod, v, penarray[2]);
	else
		setfgpen(mod, v, pen);

	x0 = array[0];
	y0 = array[1];
	x1 = array[2];
	y1 = array[3];
	x2 = array[4];
	y2 = array[5];

	fbp_drawtriangle(v, x0, y0, x1, y1, x2, y2, (struct FBPen *) v->fgpen);

	for (i = 3; i < num; i++)
	{
		x0 = x1;
		y0 = y1;
		x1 = x2;
		y1 = y2;
		x2 = array[i*2];
		y2 = array[i*2+1];

		if (penarray)
			setfgpen(mod, v, penarray[i]);

		fbp_drawtriangle(v, x0, y0, x1, y1, x2, y2,
			(struct FBPen *) v->fgpen);
	}
}

/*****************************************************************************/

LOCAL void
fb_drawfan(FBDISPLAY *mod, struct TVFBRequest *req)
{
	TINT i, x0, y0, x1, y1, x2, y2;
	FBWINDOW *v = req->tvr_Op.Fan.Window;
	TINT *array = req->tvr_Op.Fan.Array;
	TINT num = req->tvr_Op.Fan.Num;
	TTAGITEM *tags = req->tvr_Op.Fan.Tags;
	TVPEN pen = (TVPEN) TGetTag(tags, TVisual_Pen, TVPEN_UNDEFINED);
	TVPEN *penarray = (TVPEN *) TGetTag(tags, TVisual_PenArray, TNULL);

	if (num < 3) return;

	if (penarray)
		setfgpen(mod, v, penarray[2]);
	else
		setfgpen(mod, v, pen);

	x0 = array[0];
	y0 = array[1];
	x1 = array[2];
	y1 = array[3];
	x2 = array[4];
	y2 = array[5];

	fbp_drawtriangle(v, x0, y0, x1, y1, x2, y2, (struct FBPen *) v->fgpen);

	for (i = 3; i < num; i++)
	{
		x1 = x2;
		y1 = y2;
		x2 = array[i*2];
		y2 = array[i*2+1];

		if (penarray)
			setfgpen(mod, v, penarray[i]);

		fbp_drawtriangle(v, x0, y0, x1, y1, x2, y2,
			(struct FBPen *) v->fgpen);
	}
}

/*****************************************************************************/

LOCAL void
fb_copyarea(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.CopyArea.Window;
	TINT dx = req->tvr_Op.CopyArea.DestX;
	TINT dy = req->tvr_Op.CopyArea.DestY;

	fbp_copyarea(v, req->tvr_Op.CopyArea.Rect, dx, dy);
}

/*****************************************************************************/

LOCAL void
fb_setcliprect(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.ClipRect.Window;
	v->fbv_ClipRect[0] = req->tvr_Op.ClipRect.Rect[0];
	v->fbv_ClipRect[1] = req->tvr_Op.ClipRect.Rect[1];
	v->fbv_ClipRect[2] = v->fbv_ClipRect[0] + req->tvr_Op.ClipRect.Rect[2] - 1;
	v->fbv_ClipRect[3] = v->fbv_ClipRect[1] + req->tvr_Op.ClipRect.Rect[3] - 1;
}

/*****************************************************************************/

LOCAL void
fb_unsetcliprect(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.ClipRect.Window;
	v->fbv_ClipRect[0] = v->fbv_WinRect[0];
	v->fbv_ClipRect[1] = v->fbv_WinRect[1];
	v->fbv_ClipRect[2] = v->fbv_WinRect[2];
	v->fbv_ClipRect[3] = v->fbv_WinRect[3];
}

/*****************************************************************************/

LOCAL void
fb_clear(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.Clear.Window;
	TINT ww = v->fbv_WinRect[2] - v->fbv_WinRect[0] + 1;
	TINT wh = v->fbv_WinRect[3] - v->fbv_WinRect[1] + 1;
	TINT rect[4] = { 0, 0, ww, wh };
	struct FBPen *pen = (struct FBPen *) req->tvr_Op.Clear.Pen;
	setfgpen(mod, v, req->tvr_Op.Clear.Pen);
	fbp_drawfrect(v, rect, pen);
}

/*****************************************************************************/

LOCAL void
fb_drawbuffer(FBDISPLAY *mod, struct TVFBRequest *req)
{
	FBWINDOW *v = req->tvr_Op.DrawBuffer.Window;
	TAPTR buf = req->tvr_Op.DrawBuffer.Buf;
	TINT totw = req->tvr_Op.DrawBuffer.TotWidth;

	fbp_drawbuffer(v, (TUINT8 *)buf, req->tvr_Op.DrawBuffer.RRect, totw);
}

/*****************************************************************************/

static THOOKENTRY TTAG
getattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct attrdata *data = hook->thk_Data;
	TTAGITEM *item = obj;
	FBWINDOW *v = data->v;

	switch (item->tti_Tag)
	{
		default:
			return TTRUE;
		case TVisual_Width:
			*((TINT *) item->tti_Value) =
				v->fbv_WinRect[2] - v->fbv_WinRect[0] + 1;
			break;
		case TVisual_Height:
			*((TINT *) item->tti_Value) =
				v->fbv_WinRect[3] - v->fbv_WinRect[1] + 1;
			break;
		case TVisual_WinLeft:
			*((TINT *) item->tti_Value) = v->fbv_WinRect[0];
			break;
		case TVisual_WinTop:
			*((TINT *) item->tti_Value) = v->fbv_WinRect[1];
			break;
		case TVisual_MinWidth:
			*((TINT *) item->tti_Value) =
				v->fbv_WinRect[2] - v->fbv_WinRect[0] + 1;
			break;
		case TVisual_MinHeight:
			*((TINT *) item->tti_Value) =
				v->fbv_WinRect[3] - v->fbv_WinRect[1] + 1;
			break;
		case TVisual_MaxWidth:
			*((TINT *) item->tti_Value) =
				v->fbv_WinRect[2] - v->fbv_WinRect[0] + 1;
			break;
		case TVisual_MaxHeight:
			*((TINT *) item->tti_Value) =
				v->fbv_WinRect[3] - v->fbv_WinRect[1] + 1;
			break;
		case TVisual_Device:
			*((TAPTR *) item->tti_Value) = data->mod;
			break;
		case TVisual_Window:
			*((TAPTR *) item->tti_Value) = v;
			break;
	}
	data->num++;
	return TTRUE;
}

LOCAL void
fb_getattrs(FBDISPLAY *mod, struct TVFBRequest *req)
{
	struct attrdata data;
	struct THook hook;

	data.v = req->tvr_Op.GetAttrs.Window;
	data.num = 0;
	data.mod = mod;
	TInitHook(&hook, getattrfunc, &data);

	TForEachTag(req->tvr_Op.GetAttrs.Tags, &hook);
	req->tvr_Op.GetAttrs.Num = data.num;
}

/*****************************************************************************/

static THOOKENTRY TTAG
setattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct attrdata *data = hook->thk_Data;
	TTAGITEM *item = obj;
	/*FBWINDOW *v = data->v;*/
	switch (item->tti_Tag)
	{
		default:
			return TTRUE;
	}
	data->num++;
	return TTRUE;
}

LOCAL void
fb_setattrs(FBDISPLAY *mod, struct TVFBRequest *req)
{
	struct attrdata data;
	struct THook hook;
	FBWINDOW *v = req->tvr_Op.SetAttrs.Window;

	data.v = v;
	data.num = 0;
	data.mod = mod;
	TInitHook(&hook, setattrfunc, &data);

	TForEachTag(req->tvr_Op.SetAttrs.Tags, &hook);
	req->tvr_Op.SetAttrs.Num = data.num;
}

/*****************************************************************************/

struct drawdata
{
	FBWINDOW *v;
	FBDISPLAY *mod;
	TINT x0, x1, y0, y1;
};

static THOOKENTRY TTAG
drawtagfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct drawdata *data = hook->thk_Data;
	TTAGITEM *item = obj;

	switch (item->tti_Tag)
	{
		case TVisualDraw_X0:
			data->x0 = item->tti_Value;
			break;
		case TVisualDraw_Y0:
			data->y0 = item->tti_Value;
			break;
		case TVisualDraw_X1:
			data->x1 = item->tti_Value;
			break;
		case TVisualDraw_Y1:
			data->y1 = item->tti_Value;
			break;
		case TVisualDraw_NewX:
			data->x0 = data->x1;
			data->x1 = item->tti_Value;
			break;
		case TVisualDraw_NewY:
			data->y0 = data->y1;
			data->y1 = item->tti_Value;
			break;
		case TVisualDraw_FgPen:
			setfgpen(data->mod, data->v, item->tti_Value);
			break;
		case TVisualDraw_BgPen:
			setbgpen(data->mod, data->v, item->tti_Value);
			break;
		case TVisualDraw_Command:
			switch (item->tti_Value)
			{
				case TVCMD_FRECT:
				{
					TINT r[] = { data->x0, data->y0, data->x1-data->x0, data->y1-data->y0 };
					struct FBPen *pen = (struct FBPen *) data->v->fgpen;
					fbp_drawfrect(data->v, r, pen);
					break;
				}
				case TVCMD_RECT:
				{
					TINT r[] = { data->x0, data->y0, data->x1-data->x0, data->y1-data->y0 };
					struct FBPen *pen = (struct FBPen *) data->v->fgpen;
					fbp_drawrect(data->v, r, pen);
					break;
				}
				case TVCMD_LINE:
				{
					TINT r[] = { data->x0, data->y0, data->x1, data->y1 };
					struct FBPen *pen = (struct FBPen *) data->v->fgpen;
					fbp_drawline(data->v, r, pen);
					break;
				}
			}

			break;
	}
	return TTRUE;
}

LOCAL void
fb_drawtags(FBDISPLAY *mod, struct TVFBRequest *req)
{
	struct THook hook;
	struct drawdata data;
	data.v = req->tvr_Op.DrawTags.Window;
	data.mod = mod;

	TInitHook(&hook, drawtagfunc, &data);
	TForEachTag(req->tvr_Op.DrawTags.Tags, &hook);
}

/*****************************************************************************/

LOCAL void
fb_drawtext(FBDISPLAY *mod, struct TVFBRequest *req)
{
	fb_hostdrawtext(mod, req->tvr_Op.Text.Window, req->tvr_Op.Text.Text,
		req->tvr_Op.Text.Length, req->tvr_Op.Text.X, req->tvr_Op.Text.Y,
		req->tvr_Op.Text.FgPen);
}

/*****************************************************************************/

LOCAL void
fb_setfont(FBDISPLAY *mod, struct TVFBRequest *req)
{
	fb_hostsetfont(mod, req->tvr_Op.SetFont.Window,
		req->tvr_Op.SetFont.Font);
}

/*****************************************************************************/

LOCAL void
fb_openfont(FBDISPLAY *mod, struct TVFBRequest *req)
{
	req->tvr_Op.OpenFont.Font =
		fb_hostopenfont(mod, req->tvr_Op.OpenFont.Tags);
}

/*****************************************************************************/

LOCAL void
fb_textsize(FBDISPLAY *mod, struct TVFBRequest *req)
{
	req->tvr_Op.TextSize.Width =
		fb_hosttextsize(mod, req->tvr_Op.TextSize.Font,
			req->tvr_Op.TextSize.Text, strlen(req->tvr_Op.TextSize.Text)); /* FIXME: strlen <-> UTF8 */
}

/*****************************************************************************/

LOCAL void
fb_getfontattrs(FBDISPLAY *mod, struct TVFBRequest *req)
{
	struct attrdata data;
	struct THook hook;

	data.mod = mod;
	data.font = req->tvr_Op.GetFontAttrs.Font;
	data.num = 0;
	TInitHook(&hook, fb_hostgetfattrfunc, &data);

	TForEachTag(req->tvr_Op.GetFontAttrs.Tags, &hook);
	req->tvr_Op.GetFontAttrs.Num = data.num;
}

/*****************************************************************************/

LOCAL void
fb_closefont(FBDISPLAY *mod, struct TVFBRequest *req)
{
	fb_hostclosefont(mod, req->tvr_Op.CloseFont.Font);
}

/*****************************************************************************/

LOCAL void
fb_queryfonts(FBDISPLAY *mod, struct TVFBRequest *req)
{
	req->tvr_Op.QueryFonts.Handle =
		fb_hostqueryfonts(mod, req->tvr_Op.QueryFonts.Tags);
}

/*****************************************************************************/

LOCAL void
fb_getnextfont(FBDISPLAY *mod, struct TVFBRequest *req)
{
	req->tvr_Op.GetNextFont.Attrs =
		fb_hostgetnextfont(mod, req->tvr_Op.GetNextFont.Handle);
}
