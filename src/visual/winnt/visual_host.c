
/*
**	$Id: visual_host.c,v 1.1 2006/08/25 21:23:42 tmueller Exp $
**	teklib/mods/visual/win32/visual_host.c - Win32 implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	TODO: register/unregister classatom only once
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include "visual_mod.h"
#include <windows.h>

/*****************************************************************************/

typedef struct
{
	TMOD_VIS *mod;		/* backptr */

	TINT winwidth, winheight;
	TINT fontwidth, fontheight;
	TINT textwidth, textheight;
	TSTRPTR title;

	HINSTANCE hinst;
	ATOM classatom;
	WNDCLASSEX wcx;
	HWND hwnd;

	HDC dc;
	COLORREF bgcolor, fgcolor;
	HPEN curpen;
	HBRUSH curbrush;

	TINT numpoints;
	POINT *points;

	TUINT eventmask;

	BITMAPINFOHEADER bih;

	TINT mousex, mousey;
	TUINT qualifier;
	TINT8 keystate[256];

	TLIST imsgpool;

	TINT wakecount;

} VISUAL;

#define CLASSNAME	"tek_visual_class"
#define FLIPRGB(rgb) ((((rgb)<<16)&0xff0000)|((rgb)&0xff00)|(((rgb)>>16)&0xff))

/*****************************************************************************/

static void
setbgpen(TMOD_VIS *mod, VISUAL *v, TVPEN pen)
{
	COLORREF rgb = (COLORREF) pen;
	if (rgb != v->bgcolor)
	{
		v->bgcolor = rgb;
		SetBkColor(v->dc, FLIPRGB(rgb));
	}
}

/*****************************************************************************/

static void
setfgpen(TMOD_VIS *mod, VISUAL *v, TVPEN pen)
{
	COLORREF rgb = (COLORREF) pen;
	if (rgb != v->fgcolor)
	{
		HBRUSH prevbrush;
		HPEN prevpen;

		v->fgcolor = rgb;
		rgb = FLIPRGB(rgb);

		v->curbrush = CreateSolidBrush(rgb);
		prevbrush = SelectObject(v->dc, v->curbrush);
		DeleteObject(prevbrush);

		v->curpen = CreatePen(PS_SOLID, 0, rgb);
		prevpen = SelectObject(v->dc, v->curpen);
		DeleteObject(prevpen);

		SetTextColor(v->dc, rgb);
	}
}

/*****************************************************************************/

static void
frect(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	RECT r;
	r.left = rect[0];
	r.top = rect[1];
	r.right = rect[2]+rect[0];
	r.bottom = rect[3]+rect[1];
	FillRect(v->dc, &r, v->curbrush);
}

/*****************************************************************************/

static void
line(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	MoveToEx(v->dc, rect[0], rect[1], NULL);
	LineTo(v->dc, rect[2], rect[3]);
}

/*****************************************************************************/

static void
rect(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	HBRUSH tmpbrush = SelectObject(v->dc, GetStockObject(NULL_BRUSH));
	Rectangle(v->dc, rect[0], rect[1], rect[0] + rect[2], rect[1] + rect[3]);
	SelectObject(v->dc, tmpbrush);
}

/*****************************************************************************/

static void
plot(TMOD_VIS *mod, VISUAL *v, TINT x, TINT y)
{
	SetPixel(v->dc, x, y, v->fgcolor);
}

/*****************************************************************************/

static void
clear(TMOD_VIS *mod, VISUAL *v)
{
	RECT r;
	r.left = 0;
	r.top = 0;
	r.right = v->winwidth - 1;
	r.bottom = v->winheight - 1;
	FillRect(v->dc, &r, v->curbrush);
}

/*****************************************************************************/

static void
drawtext(TMOD_VIS *mod, VISUAL *v, TINT x, TINT y, TSTRPTR text, TINT len)
{
	TextOut(v->dc, x, y, text, len);
}

/*****************************************************************************/

static void
fpoly(TMOD_VIS *mod, VISUAL *v, TINT16 *array, TINT num)
{
	if (v->numpoints != num)
	{
		TExecFree(TExecBase, v->points);
		v->points = TExecAlloc(TExecBase, mod->mmu, sizeof(POINT) * num);
		if (v->points) v->numpoints = num;
	}

	if (v->points)
	{
		TINT i;
		for (i = 0; i < num; ++i)
		{
			v->points[i].x = (LONG) *array++;
			v->points[i].y = (LONG) *array++;
		}
		Polygon(v->dc, v->points, num);
	}
}

/*****************************************************************************/

static void
scroll(TMOD_VIS *mod, VISUAL *v, TINT *srect)
{
	RECT r;
	r.left = srect[0];
	r.top = srect[1];
	r.right = srect[0]+srect[2]-1;
	r.bottom = srect[1]+srect[3]-1;
	ScrollDC(v->dc,	srect[4],srect[5], &r, &r, NULL, &r);
}

/*****************************************************************************/

static void
drawrgb(TMOD_VIS *mod, VISUAL *v, TUINT *buf, TINT *rrect)
{
	v->bih.biWidth = rrect[4];
	v->bih.biHeight = -rrect[3];
	SetDIBitsToDevice(v->dc,
		rrect[0],rrect[1],
		rrect[2],rrect[3],
		0,0,
		0,rrect[3]-1,
		buf,
		(const void*) &v->bih,
		DIB_RGB_COLORS);
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
			*((TINT *) item->tti_Value) = v->winwidth;
			break;
		case TVisual_PixHeight:
			*((TINT *) item->tti_Value) = v->winheight;
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
**	return value indicates whether this command
**	is a cooperation point for message processing
*/

LOCAL void
vis_docmd(TMOD_VIS *mod, TVREQ *req)
{
	VISUAL *v = mod->hostspecific;
	switch (req->vis_Req.io_Command)
	{
		case TVCMD_FLUSH:
			GdiFlush();
			break;

		case TVCMD_ALLOCPEN:
			req->vis_Op.AllocPen.Pen = (TVPEN) req->vis_Op.AllocPen.RGB;
			break;

		case TVCMD_FREEPEN:
			break;

		case TVCMD_SETINPUT:
			v->eventmask = req->vis_Op.SetInput.Mask;
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

static TBOOL
getqualifier(TMOD_VIS *mod, VISUAL *v)
{
	TUINT quali = TKEYQ_NONE;
	TBOOL newquali;

	GetKeyboardState(v->keystate);
	if (v->keystate[VK_LSHIFT] < 0) quali |= TKEYQ_LSHIFT;
	if (v->keystate[VK_RSHIFT] < 0) quali |= TKEYQ_RSHIFT;
	if (v->keystate[VK_LCONTROL] < 0) quali |= TKEYQ_LCTRL;
	if (v->keystate[VK_RCONTROL] < 0) quali |= TKEYQ_RCTRL;
	if (v->keystate[VK_LMENU] < 0) quali |= TKEYQ_LALT;
	if (v->keystate[VK_RMENU] < 0) quali |= TKEYQ_RALT;
	if (v->keystate[VK_NUMLOCK] & 1) quali |= TKEYQ_NUMBLOCK;

	newquali = (v->qualifier != quali);
	v->qualifier = quali;

	return newquali;
}

static LRESULT CALLBACK
vis_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	VISUAL *v = (VISUAL *) GetWindowLong(hwnd, GWL_USERDATA);
	TMOD_VIS *mod;
	TIMSG *imsg;

	for (;;)
	{
		if (v == TNULL) break;
		mod = v->mod;

		imsg = (TIMSG *) TFIRSTNODE(&v->imsgpool);
		if (imsg == TNULL)
		{
			imsg = TExecAllocMsg(TExecBase, sizeof(TIMSG));
			if (imsg == TNULL) break;
			TAddTail(&v->imsgpool, (struct TNode *) imsg);
		}

		imsg->timsg_Type = TITYPE_NONE;

		switch (uMsg)
		{
			case WM_CLOSE:
				imsg->timsg_Type = TITYPE_CLOSE;
				break;

			case WM_PAINT:
				imsg->timsg_Type = TITYPE_REFRESH;
				break;

			case WM_ACTIVATE:
				imsg->timsg_Type = TITYPE_FOCUS;
				imsg->timsg_Code = (LOWORD(wParam) != WA_INACTIVE);
				break;

			case WM_SIZE:
				v->winwidth = LOWORD(lParam);
				v->winheight = HIWORD(lParam);
				imsg->timsg_Type = TITYPE_NEWSIZE;
				break;

			case WM_MOUSEMOVE:
			{
				POINT scrpos;
				TINT x = v->mousex = LOWORD(lParam);
				TINT y = v->mousey = HIWORD(lParam);

				if (v->eventmask & TITYPE_MOUSEOVER)
				{
					if (GetCapture() != v->hwnd)
					{
						SetCapture(v->hwnd);
						imsg->timsg_Type = TITYPE_MOUSEOVER;
						imsg->timsg_Code = 1;
						break;
					}

					GetCursorPos(&scrpos);

					if ((WindowFromPoint(scrpos) != v->hwnd) || (x < 0) ||
						(y < 0) || (x >= v->winwidth) || (y >= v->winheight))
					{
						ReleaseCapture();
						imsg->timsg_Type = TITYPE_MOUSEOVER;
						imsg->timsg_Code = 0;
						break;
					}
				}

				imsg->timsg_Type = TITYPE_MOUSEMOVE;
				break;
			}

			case WM_LBUTTONDOWN:
				if (GetActiveWindow() != v->hwnd)
				{
					/* cause WM_ACTIVATE to show up */
					SetActiveWindow(v->hwnd);
					return 0;
				}
				imsg->timsg_Type = TITYPE_MOUSEBUTTON;
				imsg->timsg_Code = TMBCODE_LEFTDOWN;
				break;
			case WM_LBUTTONUP:
				imsg->timsg_Type = TITYPE_MOUSEBUTTON;
				imsg->timsg_Code = TMBCODE_LEFTUP;
				break;
			case WM_RBUTTONDOWN:
				imsg->timsg_Type = TITYPE_MOUSEBUTTON;
				imsg->timsg_Code = TMBCODE_RIGHTDOWN;
				break;
			case WM_RBUTTONUP:
				imsg->timsg_Type = TITYPE_MOUSEBUTTON;
				imsg->timsg_Code = TMBCODE_RIGHTUP;
				break;
			case WM_MBUTTONDOWN:
				imsg->timsg_Type = TITYPE_MOUSEBUTTON;
				imsg->timsg_Code = TMBCODE_MIDDLEDOWN;
				break;
			case WM_MBUTTONUP:
				imsg->timsg_Type = TITYPE_MOUSEBUTTON;
				imsg->timsg_Code = TMBCODE_MIDDLEUP;
				break;

			case WM_KEYUP:
			case WM_SYSKEYUP:
				if (getqualifier(mod, v))
				{
					imsg->timsg_Type = TITYPE_COOKEDKEY;
				}
				break;

			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				TBOOL newqual = getqualifier(mod, v);
				imsg->timsg_Type = TITYPE_COOKEDKEY;
				switch (wParam)
				{
					default:
						if (newqual)
						{
							imsg->timsg_Code = TKEYC_NONE;	/* send qualifier alone */
						}
						else
						{
							imsg->timsg_Type = TITYPE_NONE;	/* nothing to send */
						}
						break;

					case VK_DELETE:	imsg->timsg_Code = TKEYC_DEL;
									break;
					case VK_LEFT:	imsg->timsg_Code = TKEYC_CRSRLEFT;
									break;
					case VK_UP:		imsg->timsg_Code = TKEYC_CRSRUP;
									break;
					case VK_RIGHT:	imsg->timsg_Code = TKEYC_CRSRRIGHT;
									break;
					case VK_DOWN:	imsg->timsg_Code = TKEYC_CRSRDOWN;
									break;
					case VK_F1: case VK_F2: case VK_F3: case VK_F4:
					case VK_F5: case VK_F6: case VK_F7: case VK_F8:
					case VK_F9: case VK_F10: case VK_F11: case VK_F12:
						imsg->timsg_Code = (TUINT) (wParam - VK_F1) + TKEYC_F1;
						break;
				}
				break;
			}

			case WM_CHAR:
				imsg->timsg_Type = TITYPE_COOKEDKEY;
				switch (wParam)
				{
					default:	imsg->timsg_Code = wParam;
								break;
					case 8:		imsg->timsg_Code = TKEYC_BCKSPC;
								break;
					case 27:	imsg->timsg_Code = TKEYC_ESC;
								break;
					case 9:		imsg->timsg_Code = TKEYC_TAB;
								break;
					case 13:	imsg->timsg_Code = TKEYC_RETURN;
								break;
				}
				break;

			case WM_USER:
				v->wakecount++;
				break;
		}

		if (imsg->timsg_Type & v->eventmask)
		{
			TRemove((struct TNode *) imsg);
			imsg->timsg_Qualifier = v->qualifier;
			imsg->timsg_MouseX = v->mousex;
			imsg->timsg_MouseY = v->mousey;
			vis_sendimsg(mod, imsg);
		}

		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/*****************************************************************************/

LOCAL void
vis_exit(TMOD_VIS *mod)
{
	VISUAL *v = mod->hostspecific;
	if (v)
	{
		struct TNode *imsg;

		if (v->dc) ReleaseDC(v->hwnd, v->dc);
		if (v->curpen) DeleteObject(v->curpen);
		if (v->curbrush) DeleteObject(v->curbrush);
		if (v->hwnd) DestroyWindow(v->hwnd);
		if (v->classatom) UnregisterClass(CLASSNAME, v->hinst);

		while ((imsg = TRemHead(&v->imsgpool))) TExecFree(TExecBase, imsg);

		TExecFree(TExecBase, v->points);

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
		RECT wrect;
		TEXTMETRIC tm;
		TAPTR atom;

		v->hinst = GetModuleHandle(NULL);
		if (v->hinst == TNULL) break;

		TInitList(&v->imsgpool);

		v->wcx.cbSize = sizeof(v->wcx);
		v->wcx.style = CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS;
		v->wcx.lpfnWndProc = vis_wndproc;
		v->wcx.cbClsExtra = 0;
		v->wcx.cbWndExtra = 0;
		v->wcx.hInstance = v->hinst;
		v->wcx.hIcon = LoadIcon(NULL,IDI_APPLICATION);
		v->wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
		v->wcx.hbrBackground = GetStockObject(BLACK_BRUSH);
		v->wcx.lpszMenuName = NULL;
		v->wcx.lpszClassName = CLASSNAME;
		v->wcx.hIconSm = NULL;
		v->classatom = RegisterClassEx(&v->wcx);
	//	if (v->classatom == TNULL) break;

		v->winwidth =
			(TINT) TGetTag(mod->inittags, TVisual_PixWidth, (TTAG) 600);
		v->winheight =
			(TINT) TGetTag(mod->inittags, TVisual_PixHeight, (TTAG) 400);
		if (v->winwidth <= 0 || v->winheight <= 0) break;

		v->title = (TSTRPTR)
			TGetTag(mod->inittags, TVisual_Title, (TTAG) "TEKlib visual");

		wrect.left = 0;
		wrect.top = 0;
		wrect.right = v->winwidth - 1;
		wrect.bottom = v->winheight - 1;
		AdjustWindowRectEx(&wrect, WS_OVERLAPPEDWINDOW, FALSE, 0);

		v->hwnd = CreateWindowEx(0, CLASSNAME, v->title,
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			wrect.right - wrect.left + 1, wrect.bottom - wrect.top + 1,
			(HWND) NULL, (HMENU) NULL, v->hinst, (LPVOID) NULL);
		if (v->hwnd == TNULL) break;

		/* place visual as userdata in the window */
		v->mod = mod;	/* backptr */
		SetWindowLong(v->hwnd, GWL_USERDATA, (long) v);

		v->dc = GetDC(v->hwnd);

		SelectObject(v->dc, GetStockObject(ANSI_FIXED_FONT));
		GetTextMetrics(v->dc, &tm);
		v->fontwidth = tm.tmMaxCharWidth;
		v->fontheight = tm.tmHeight;

		v->fgcolor = 0xffffffff;
		v->bgcolor = 0xffffffff;

	    ShowWindow(v->hwnd, SW_SHOWNORMAL);
		UpdateWindow(v->hwnd);

		/* init bitmapinfo header */
		v->bih.biSize = sizeof(BITMAPINFOHEADER);
		v->bih.biPlanes = 1;
		v->bih.biBitCount = 32;
		v->bih.biCompression = BI_RGB;
		v->bih.biSizeImage = 0;
		v->bih.biXPelsPerMeter = 1;
		v->bih.biYPelsPerMeter = 1;
		v->bih.biClrUsed = 0;
		v->bih.biClrImportant = 0;

		/* announce a "win32.hwnd" atom in the application */
		atom = TExecLockAtom(TExecBase, "win32.hwnd",
			TATOMF_CREATE | TATOMF_NAME | TATOMF_TRY);
		if (atom)
		{
			TExecSetAtomData(TExecBase, atom, (TTAG) v->hwnd);
			TExecUnlockAtom(TExecBase, atom, TATOMF_KEEP);
		}

		return TTRUE;
	}

	vis_exit(mod);
	return TFALSE;
}

/*****************************************************************************/

LOCAL void
vis_wake(TMOD_VIS *mod)
{
	VISUAL *v = mod->hostspecific;
	PostMessage(v->hwnd, WM_USER, 0, 0);
}

/*****************************************************************************/

LOCAL TUINT
vis_wait(TMOD_VIS *mod, TUINT waitsig)
{
	VISUAL *v = mod->hostspecific;
	MSG msg;

	if (v->wakecount)
	{
		v->wakecount--;
	}
	else
	{
		WaitMessage();
	}

	while (PeekMessage(&msg, v->hwnd, 0,0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return TExecSetSignal(TExecBase, 0, waitsig);
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: visual_host.c,v $
**	Revision 1.1  2006/08/25 21:23:42  tmueller
**	added WinNT files
**
**	Revision 1.7  2006/08/19 11:34:33  tmueller
**	added interval timer, setattrs, cleanup
**
**	Revision 1.1  2006/05/25 22:46:49  tmueller
**	added
**
**	Revision 1.5  2005/09/13 02:43:36  tmueller
**	updated copyright reference
**
**	Revision 1.4  2004/04/18 14:18:00  tmueller
**	TTAG changed to TUINTPTR; atomdata, parseargv changed from TAPTR to TTAG
**
**	Revision 1.3  2004/01/18 17:17:10  dtrompetter
**	some changes
**
**	Revision 1.2  2004/01/13 02:22:43  tmueller
**	New visual backend implementations added. They are no longer sub-modules.
**
**	Revision 1.2  2003/12/13 14:12:56  tmueller
*/
#warning unreviewed
