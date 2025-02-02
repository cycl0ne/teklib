
/*
**	$Id: visual_host.c,v 1.5 2005/09/13 02:43:36 tmueller Exp $
**	teklib/mods/visual/x11/visual_host.c - X11 implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Adler et al. See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include "visual_mod.h"

#include <sys/time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <sys/shm.h>
#include <X11/extensions/XShm.h>

/*****************************************************************************/

typedef struct
{
	TINT winwidth, winheight;
	TINT fontwidth, fontheight;
	TINT textwidth, textheight;
	TSTRPTR title;

	TUINT flags;
	TUINT pixfmt;
	TINT depth, bpp;

	Display *display;
	int screen;
	Visual *visual;
	Window window;
	XFontStruct *font;
	XTextProperty title_prop;
	Colormap colormap;
	GC gc;

	TINT shm, shmevent;
	XShmSegmentInfo shminfo;
	TBOOL shmpending;

	Atom atom_wm_delete_win;

	TINT keyqual;
	TINT mousex, mousey;
	TUINT base_mask;
	TUINT eventmask;
	
	TLIST imsgpool;
	
	TVPEN bgpen, fgpen;

	int fd_display;	
	int fd_sigpipe_read;
	int fd_sigpipe_write;
	int fd_max;
	
	XImage *image;
	char *tempbuf;
	int imw, imh;

} VISUAL;

#define DEFFONTNAME			"-misc-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*"

#define PIXFMT_UNDEFINED	0
#define PIXFMT_RGB			1
#define PIXFMT_RBG			2
#define PIXFMT_BRG			3
#define PIXFMT_BGR			4
#define PIXFMT_GRB			5
#define PIXFMT_GBR			6

#define TVISF_SWAPBYTEORDER	0x00000001

static TBOOL shm_available;

/*****************************************************************************/

static TVOID
setinputmask(TMOD_VIS *mod, VISUAL *v, TUINT eventmask)
{
	TUINT x11_mask = 0;

	if (eventmask & TITYPE_REFRESH)
		x11_mask |= StructureNotifyMask | ExposureMask;
	if (eventmask & TITYPE_MOUSEOVER)
		x11_mask |= LeaveWindowMask | EnterWindowMask;
	if (eventmask & TITYPE_FOCUS)
		x11_mask |= FocusChangeMask;
	if (eventmask & TITYPE_NEWSIZE)
		x11_mask |= StructureNotifyMask;
	if (eventmask & TITYPE_COOKEDKEY)
		x11_mask |= KeyPressMask | KeyReleaseMask;
	if (eventmask & TITYPE_MOUSEMOVE)
		x11_mask |= PointerMotionMask | OwnerGrabButtonMask |
			ButtonMotionMask | ButtonPressMask | ButtonReleaseMask;
	if (eventmask & TITYPE_MOUSEBUTTON)
		x11_mask |= ButtonPressMask | ButtonReleaseMask | OwnerGrabButtonMask;

	v->eventmask = eventmask;

	XSelectInput(v->display, v->window, v->base_mask | x11_mask);
	XFlush(v->display);
}

/*****************************************************************************/

static TVPEN
allocpen(TMOD_VIS *mod, VISUAL *v, TUINT rgb)
{
	XColor color;
	color.red = ((rgb >> 16) & 0xff) << 8;
	color.green = ((rgb >> 8) & 0xff) << 8;
	color.blue = (rgb & 0xff) << 8;
	color.flags = DoRed | DoGreen | DoBlue;
	if (!XAllocColor(v->display, v->colormap, &color))
	{
		return (TVPEN) 0xffffffff;
	}
	return (TVPEN) color.pixel;
}

/*****************************************************************************/

static TVOID
freepen(TMOD_VIS *mod, VISUAL *v, TVPEN pen)
{
	unsigned long color = (unsigned long) pen;
	XFreeColors(v->display, v->colormap, &color, 1, 0);
}

/*****************************************************************************/

static TVOID
setbgpen(TMOD_VIS *mod, VISUAL *v, TVPEN pen)
{
	if (pen != v->bgpen)
	{
		XGCValues gcv;
		gcv.background = (long) pen;
		XChangeGC(v->display, v->gc, GCBackground, &gcv);
		v->bgpen = pen;
	}
}

/*****************************************************************************/

static TVPEN
setfgpen(TMOD_VIS *mod, VISUAL *v, TVPEN pen)
{
	TVPEN oldpen = v->fgpen;
	if (pen != oldpen)
	{
		XGCValues gcv;
		gcv.foreground = (long) pen;
		XChangeGC(v->display, v->gc, GCForeground, &gcv);
		v->fgpen = pen;
		if (oldpen == (TVPEN) 0xffffffff) oldpen = pen;
	}
	return oldpen;
}

/*****************************************************************************/

static TVOID
frect(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	XFillRectangle(v->display, v->window, v->gc,
		rect[0], rect[1], rect[2], rect[3]);
}

/*****************************************************************************/

static TVOID
line(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	XDrawLine(v->display, v->window, v->gc,
		rect[0], rect[1], rect[2], rect[3]);
}

/*****************************************************************************/

static TVOID
rect(TMOD_VIS *mod, VISUAL *v, TINT *rect)
{
	XDrawRectangle(v->display, v->window, v->gc,
		rect[0], rect[1], rect[2], rect[3]);
}

/*****************************************************************************/

static TVOID
plot(TMOD_VIS *mod, VISUAL *v, TINT x, TINT y)
{
	XDrawPoint(v->display, v->window, v->gc, x, y);
}

/*****************************************************************************/

static TVOID
clear(TMOD_VIS *mod, VISUAL *v)
{
	XFillRectangle(v->display, v->window, v->gc,
		0, 0, v->winwidth, v->winheight);
}

/*****************************************************************************/

static TVOID
drawtext(TMOD_VIS *mod, VISUAL *v, TINT x, TINT y, TSTRPTR text, TINT len)
{
	XDrawImageString(v->display, v->window, v->gc,
		v->fontwidth * x, v->fontheight * y + v->font->ascent,
			(char *) text, len);
}

/*****************************************************************************/

static TVOID
fpoly(TMOD_VIS *mod, VISUAL *v, TINT16 *array, TINT num)
{
	XFillPolygon(v->display, v->window, v->gc, (XPoint *) array, num, 
		Complex, CoordModeOrigin);
}

/*****************************************************************************/

static TVOID
scroll(TMOD_VIS *mod, VISUAL *v, TINT *srect)
{
	XCopyArea(v->display, v->window, v->window, v->gc,
		srect[0], srect[1], srect[2], srect[3], -srect[4], -srect[5]);
}

/*****************************************************************************/

static int
shm_errhandler(Display *d, XErrorEvent *evt)
{
	tdbprintf(10,"Remote display - fallback to normal XPutImage\n");
	shm_available = TFALSE;
	return 0;
}

static TVOID
drawrgb(TMOD_VIS *mod, VISUAL *v, TUINT *buf, TINT *rrect)
{
	int w = rrect[2];
	int h = rrect[3];
	int totw = rrect[4];

	while (v->shmpending) vis_wait(mod, 0);

	if (w != v->imw || h != v->imh)
	{
		if (shm_available && v->shm)
		{
			if (v->image)
			{
				XShmDetach(v->display, &v->shminfo);
				XDestroyImage(v->image);
				shmdt(v->shminfo.shmaddr);
				shmctl(v->shminfo.shmid, IPC_RMID, 0);
				v->image = TNULL;
			}

			v->image = XShmCreateImage(v->display, v->visual, v->depth,
				ZPixmap, TNULL, &v->shminfo, w, h);
			if (v->image)
			{
				v->shminfo.shmid = shmget(IPC_PRIVATE, 
					v->image->bytes_per_line * v->image->height, 
						IPC_CREAT|0777);
				if (v->shminfo.shmid != -1)
				{
					v->shminfo.shmaddr = v->image->data =
						shmat(v->shminfo.shmid, 0, 0);
					if (v->shminfo.shmaddr)
					{
						XErrorHandler oldhnd;
						v->shminfo.readOnly = False;
						XSync(v->display, 0);
						oldhnd = XSetErrorHandler(shm_errhandler);
						XShmAttach(v->display, &v->shminfo);
						XSync(v->display, 0);
						XSetErrorHandler(oldhnd);
						if (shm_available)
						{
							v->imw = w;
							v->imh = h;
						}
					}
				}
			}
		}
		else
		{
			TBOOL okay = TTRUE;

			if (v->image)
			{
				v->image->data = NULL;
				XDestroyImage(v->image);
				v->image = NULL;
			}

			if (v->tempbuf)
			{
				TExecFree(TExecBase, v->tempbuf);
				v->tempbuf = NULL;
			}

			if (!(((v->depth << 9) + (v->pixfmt << 1) + 
				(v->flags & TVISF_SWAPBYTEORDER)) == (24 << 9) +
					(PIXFMT_RGB << 1) + 0))
			{
				/* formats differ, need tempbuf */
				v->tempbuf = TExecAlloc(TExecBase, mod->mmu,
					w * h * (v->bpp << 3));
				if (v->tempbuf == TNULL) okay = TFALSE;
			}

			if (okay)
			{
				v->image = XCreateImage(v->display, v->visual, v->depth,
					ZPixmap, 0, v->tempbuf ? v->tempbuf : (char *) buf,
						w, h, v->bpp << 3, 0);

				if (v->image)
				{
					v->imw = w;
					v->imh = h;
				}
			}
		}
	}
	

	if (v->image)
	{
		int xx, yy;
		TUINT p;
		TUINT *sp = buf;

		switch ((v->depth << 9) + (v->pixfmt << 1) +
			(v->flags & TVISF_SWAPBYTEORDER))
		{		
			case (24 << 9) + (PIXFMT_RGB << 1) + 0:
				if (v->shm)
				{
					TExecCopyMem(TExecBase, buf, v->image->data, w*h*4);
				}
				else
				{
					v->image->data = (char *) buf;
				}
				break;


			case (15 << 9) + (PIXFMT_RGB << 1) + 0:
			{
				TUINT16 *dp = (TUINT16 *) 
					(v->tempbuf ? v->tempbuf : v->image->data);
				
				for (yy = 0; yy < h; ++yy)
				{
					for (xx = 0; xx < w; xx++)
					{
						p = sp[xx];
						dp[xx] = ((p & 0xf80000) >> 9) | 
							((p & 0x00f800) >> 6) |
							((p & 0x0000f8) >> 3);

					}				
					sp += totw;
					dp += totw;
				}
				break;
			}

			case (15 << 9) + (PIXFMT_RGB << 1) + 1:
			{
				TUINT16 *dp = (TUINT16 *) 
					(v->tempbuf ? v->tempbuf : v->image->data);
				
				for (yy = 0; yy < h; ++yy)
				{
					for (xx = 0; xx < w; xx++)
					{
						p = sp[xx];

						/*		24->15 bit, host-swapped
						**		........rrrrrrrrGGggggggbbbbbbbb
						** ->	................gggbbbbb0rrrrrGG */
					
						dp[xx] = ((p & 0xf80000) >> 17) |
							((p & 0x00c000) >> 14) |
							((p & 0x003800) << 2) |
							((p & 0x0000f8) << 5);
					}
					sp += totw;
					dp += totw;
				}
				break;
			}

			case (16 << 9) + (PIXFMT_RGB << 1) + 0:
			{
				TUINT16 *dp = (TUINT16 *)
					(v->tempbuf ? v->tempbuf : v->image->data);
				for (yy = 0; yy < h; ++yy)
				{
					for (xx = 0; xx < w; xx++)
					{
						p = sp[xx];
						dp[xx] = ((p & 0xf80000) >> 8) | 
							((p & 0x00fc00) >> 5) |
							((p & 0x0000f8) >> 3);

					}				
					sp += totw;
					dp = (TUINT16 *)
						(((char*) dp) + v->image->bytes_per_line);
				}
				break;
			}

			case (16 << 9) + (PIXFMT_RGB << 1) + 1:
			{
				TUINT16 *dp = (TUINT16 *)
					(v->tempbuf ? v->tempbuf : v->image->data);
				for (yy = 0; yy < h; ++yy)
				{
					for (xx = 0; xx < w; xx++)
					{
						p = sp[xx];
						
						/*		24->16 bit, host-swapped
						**		........rrrrrrrrGGGgggggbbbbbbbb
						** ->	................gggbbbbbrrrrrGGG */

						dp[xx] = ((p & 0xf80000) >> 16) |
							((p & 0x0000f8) << 5) |
							((p & 0x00e000) >> 13) |
							((p & 0x001c00) << 3);
					}
					sp += totw;
					dp = (TUINT16 *)
						(((char*) dp) + v->image->bytes_per_line);
				}
				break;
			}

			case (24 << 9) + (PIXFMT_RGB << 1) + 1:
			{
				TUINT *dp = (TUINT *)
					(v->tempbuf ? v->tempbuf : v->image->data);

				for (yy = 0; yy < h; ++yy)
				{
					for (xx = 0; xx < w; xx++)
					{
						p = sp[xx];
						
						/*	24->24 bit, host-swapped */
						dp[xx] = ((p & 0x00ff0000) >> 8) |
							((p & 0x0000ff00) << 8) |
							((p & 0x000000ff) << 24);
					}
					sp += totw;
					dp += totw;
				}
				break;
			}
		}

		if (shm_available && v->shm)
		{
			XShmPutImage(v->display, v->window, v->gc, v->image, 0, 0, 
				rrect[0], rrect[1], w, h, 1);
			v->shmpending = TTRUE;
		}
		else
		{
			XPutImage(v->display, v->window, v->gc, v->image, 0, 0,
				rrect[0], rrect[1], w, h);
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
**	filled = getnextevent(visual, newimsg)
**	get next input event from visual object
**	and fill it into the supplied TIMSG structure.
*/

static TBOOL 
getnextevent(TMOD_VIS *mod, TIMSG *newimsg)
{	
	VISUAL *v = mod->hostspecific;
	XEvent ev;
	TBOOL resizepending = TFALSE;

	KeySym keysym;
	XComposeStatus compose;
	char buffer[10];

	while ((XPending(v->display)) > 0)
	{
		XNextEvent(v->display, &ev);

		if (v->shmpending && ev.type == v->shmevent)
		{
			v->shmpending = TFALSE;
			continue;
		}

		newimsg->timsg_Qualifier = v->keyqual;
		newimsg->timsg_MouseX = v->mousex;
		newimsg->timsg_MouseY = v->mousey;

		switch(ev.type)
		{
			case EnterNotify:
			case LeaveNotify:
				if (v->eventmask & TITYPE_MOUSEOVER)
				{
					newimsg->timsg_Type = TITYPE_MOUSEOVER;
					newimsg->timsg_Code = (ev.type == EnterNotify);
					return TTRUE;
				}
				break;

			case MapNotify:
			case Expose:
				if (v->eventmask & TITYPE_REFRESH)
				{
					newimsg->timsg_Type = TITYPE_REFRESH;
					return TTRUE;
				}
				break;

			case FocusIn:
			case FocusOut:
				if (v->eventmask & TITYPE_FOCUS)
				{
					newimsg->timsg_Type = TITYPE_FOCUS;
					newimsg->timsg_Code = (ev.type == FocusIn);
					return TTRUE;
				}
				break;

			case ConfigureNotify:
				newimsg->timsg_MouseX =
					v->mousex = (TINT) ((XConfigureEvent *)&ev)->x;
				newimsg->timsg_MouseY =
					v->mousey = (TINT) ((XConfigureEvent *)&ev)->y;
				resizepending = TTRUE;
				v->winwidth = ((XConfigureEvent *) &ev)->width;
				v->winheight = ((XConfigureEvent *) &ev)->height;
				v->textwidth = v->winwidth / v->fontwidth;
				v->textheight = v->winheight / v->fontheight;
				break;

			case MotionNotify:
				newimsg->timsg_MouseX = v->mousex =
					(TINT) ((XMotionEvent *) &ev)->x;
				newimsg->timsg_MouseY = v->mousey =
					(TINT) ((XMotionEvent *) &ev)->y;
				if (v->eventmask & TITYPE_MOUSEMOVE)
				{
					newimsg->timsg_Type = TITYPE_MOUSEMOVE;
					return TTRUE;
				}
				break;

			case ButtonRelease:
			case ButtonPress:
				newimsg->timsg_MouseX = v->mousex =
					(TINT) ((XButtonEvent *) &ev)->x;
				newimsg->timsg_MouseY = v->mousey =
					(TINT) ((XButtonEvent *) &ev)->y;
				if (v->eventmask & TITYPE_MOUSEBUTTON)
				{
					unsigned int button;
					newimsg->timsg_Type = TITYPE_MOUSEBUTTON;

					button = ((XButtonEvent *)&ev)->button;

					if (ev.type == ButtonPress)
					{
						switch(button)
						{
							case Button1:
								newimsg->timsg_Code = TMBCODE_LEFTDOWN;
								break;
							case Button2:
								newimsg->timsg_Code = TMBCODE_MIDDLEDOWN;
								break;
							case Button3:
								newimsg->timsg_Code = TMBCODE_RIGHTDOWN;
								break;
						}
					}
					else
					{
						switch(button)
						{
							case Button1:
								newimsg->timsg_Code = TMBCODE_LEFTUP;
								break;
							case Button2:
								newimsg->timsg_Code = TMBCODE_MIDDLEUP;
								break;
							case Button3:
								newimsg->timsg_Code = TMBCODE_RIGHTUP;
								break;
						}
					}
					return TTRUE;
				}
				break;

			case KeyRelease:
				newimsg->timsg_MouseX = v->mousex =
					(TINT) ((XKeyEvent *) &ev)->x;
				newimsg->timsg_MouseY = v->mousey =
					(TINT) ((XKeyEvent *) &ev)->y;

				XLookupString((XKeyEvent *)&ev, buffer, 10, &keysym, &compose);

				switch (keysym)
				{
					case XK_Shift_L:
						v->keyqual &= ~TKEYQ_LSHIFT;
						break;
					case XK_Shift_R:
						v->keyqual &= ~TKEYQ_RSHIFT;
						break;
					case XK_Control_L:
						v->keyqual &= ~TKEYQ_LCTRL;
						break;
					case XK_Control_R:
						v->keyqual &= ~TKEYQ_RCTRL;
						break;
					case XK_Alt_L:
						v->keyqual &= ~TKEYQ_LALT;
						break;
					case XK_Alt_R:
						v->keyqual &= ~TKEYQ_RALT;
						break;
				}
				break;

			case KeyPress:
			{
				TBOOL qual = TTRUE;
				newimsg->timsg_MouseX = v->mousex =
					(TINT) ((XKeyEvent *) &ev)->x;
				newimsg->timsg_MouseY = v->mousey =
					(TINT) ((XKeyEvent *) &ev)->y;

				XLookupString((XKeyEvent *) &ev, buffer, 10, &keysym,
					&compose);

				switch (keysym)
				{
					case XK_Shift_L:
						v->keyqual |= TKEYQ_LSHIFT;
						break;
					case XK_Shift_R:
						v->keyqual |= TKEYQ_RSHIFT;
						break;
					case XK_Control_L:
						v->keyqual |= TKEYQ_LCTRL;
						break;
					case XK_Control_R:
						v->keyqual |= TKEYQ_RCTRL;
						break;
					case XK_Alt_L:
						v->keyqual |= TKEYQ_LALT;
						break;
					case XK_Alt_R:
						v->keyqual |= TKEYQ_RALT;
						break;
					default:
						qual = TFALSE;
				}

				if (v->eventmask & TITYPE_COOKEDKEY)
				{
					TBOOL newkey = TFALSE;

					if (qual)
					{
						newimsg->timsg_Qualifier = v->keyqual;
					}

					if (keysym >= XK_F1 && keysym <= XK_F12)
					{
						newimsg->timsg_Code = (TUINT)
							(keysym - XK_F1) + TKEYC_F1;
						newkey = TTRUE;
					}
					else if (keysym < 256)
					{
						newimsg->timsg_Code = keysym;	/* cooked ASCII code */
						newkey = TTRUE;
					}
					else if (keysym >= XK_KP_0 && keysym <= XK_KP_9)
					{
						newimsg->timsg_Code = (TUINT) (keysym - XK_KP_0) + 48;
						newimsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
						newkey = TTRUE;
					}
					else
					{
						newkey = TTRUE;
						switch(keysym)
						{
							case XK_Left:
								newimsg->timsg_Code = TKEYC_CRSRLEFT;
								break;
							case XK_Right:
								newimsg->timsg_Code = TKEYC_CRSRRIGHT;
								break;
							case XK_Up:
								newimsg->timsg_Code = TKEYC_CRSRUP;
								break;
							case XK_Down:
								newimsg->timsg_Code = TKEYC_CRSRDOWN;
								break;

							case XK_Escape:
								newimsg->timsg_Code = TKEYC_ESC;
								break;
							case XK_Delete:
								newimsg->timsg_Code = TKEYC_DEL;
								break;
							case XK_BackSpace:
								newimsg->timsg_Code = TKEYC_BCKSPC;
								break;
							case XK_Tab:
								newimsg->timsg_Code = TKEYC_TAB;
								break;
							case XK_Return:
								newimsg->timsg_Code = TKEYC_ENTER;
								break;

							case XK_Help:
								newimsg->timsg_Code = TKEYC_HELP;
								break;
							case XK_Insert:
								newimsg->timsg_Code = TKEYC_INSERT;
								break;
							case XK_Page_Up:
								newimsg->timsg_Code = TKEYC_PAGEUP;
								break;
							case XK_Page_Down:
								newimsg->timsg_Code = TKEYC_PAGEDOWN;
								break;
							case XK_Begin:
								newimsg->timsg_Code = TKEYC_POSONE;
								break;
							case XK_End:
								newimsg->timsg_Code = TKEYC_POSEND;
								break;
							case XK_Print:
								newimsg->timsg_Code = TKEYC_PRINT;
								break;
							case XK_Scroll_Lock:
								newimsg->timsg_Code = TKEYC_SCROLL;
								break;
							case XK_Pause:
								newimsg->timsg_Code = TKEYC_PAUSE;
								break;
							case XK_KP_Enter:
								newimsg->timsg_Code = TKEYC_ENTER;
								newimsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
								break;
							case XK_KP_Decimal:
								newimsg->timsg_Code = '.';
								newimsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
								break;
							case XK_KP_Add:
								newimsg->timsg_Code = '+';
								newimsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
								break;
							case XK_KP_Subtract:
								newimsg->timsg_Code = '-';
								newimsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
								break;
							case XK_KP_Multiply:
								newimsg->timsg_Code = '*';
								newimsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
								break;
							case XK_KP_Divide:
								newimsg->timsg_Code = '/';
								newimsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
								break;
							default:
								newkey = TFALSE;
						}
					}

					if (!newkey && qual)
					{
						newimsg->timsg_Code = TKEYC_NONE;
						newkey = TTRUE;
					}

					if (newkey)
					{
						newimsg->timsg_Type = TITYPE_COOKEDKEY;
						return TTRUE;
					}
					else
					{
						return TFALSE;
					}
				}
				break;
			}

			case ClientMessage:
				if (((XClientMessageEvent *) &ev)->data.l[0] ==
					v->atom_wm_delete_win)
				{
					newimsg->timsg_Type = TITYPE_CLOSE;
					return TTRUE;
				}
				break;

			default:
				tdbprintf(5,"event not handled\n");
				return TFALSE;
		}
	}

	if (resizepending)
	{
		if (v->eventmask & TITYPE_NEWSIZE)
		{
			newimsg->timsg_Type = TITYPE_NEWSIZE;
			return TTRUE;
		}
	}

	return TFALSE;
}

/*****************************************************************************/

static TBOOL
getprops(TMOD_VIS *mod, VISUAL *v)
{
	XVisualInfo xvi, *xvir;
	int max, maxn = 0, i, num, clas;
	int major, minor;

	v->depth = DefaultDepth(v->display, v->screen);

	xvi.screen = v->screen;
	xvir = XGetVisualInfo(v->display, VisualScreenMask, &xvi, &num);
	if (xvir)
	{
		max = 0;
		for (i = 0; i < num; i++)
		{
			if (xvir[i].depth > max)
			{
				max = xvir[i].depth;		/* max depth supported */
			}
	    }
	    
		if (max > 8)
		{
			v->depth = max;
			clas = -1;
			maxn = -1;
			for (i = 0; i < num; i++)
			{
				if (xvir[i].depth == v->depth)
				{
					if ((xvir[i].class > clas) && 
						(xvir[i].class != DirectColor))
					{
						maxn = i;
						clas = xvir[i].class;
					}
				}
			}
		}

		if (maxn >= 0)
		{
			unsigned long rmsk, gmsk, bmsk;
			
			v->visual = xvir[maxn].visual;
			rmsk = xvir[maxn].red_mask;
			gmsk = xvir[maxn].green_mask;
			bmsk = xvir[maxn].blue_mask;
			
			if ((rmsk > gmsk) && (gmsk > bmsk))
			{
				v->pixfmt = PIXFMT_RGB;
			}
			else if ((rmsk > bmsk) && (bmsk > gmsk))
			{
				v->pixfmt = PIXFMT_RBG;
			}
			else if ((bmsk > rmsk) && (rmsk > gmsk))
			{
				v->pixfmt = PIXFMT_BRG;
			}
			else if ((bmsk > gmsk) && (gmsk > rmsk))
			{
				v->pixfmt = PIXFMT_BGR;
			}
			else if ((gmsk > rmsk) && (rmsk > bmsk))
			{
				v->pixfmt = PIXFMT_GRB;
			}
			else if ((gmsk > bmsk) && (bmsk > rmsk))
			{
				v->pixfmt = PIXFMT_GBR;
			}
			else
			{
				v->pixfmt = PIXFMT_UNDEFINED;
			}
		}

		XFree(xvir);
	}

	if (v->depth == 16)
	{
		xvi.visual = v->visual;
		xvi.visualid = XVisualIDFromVisual(v->visual);
		xvir = XGetVisualInfo(v->display, VisualIDMask, &xvi, &num);
		if (xvir)
		{
			if (xvir->red_mask != 0xf800)
			{
				v->depth = 15;
			}
			XFree(xvir);
		}
	}

	switch (v->depth)
	{
		case 15:
		case 16:
			v->bpp = 2;
			break;
		case 24:
		case 32:
			v->bpp = 4;
			break;
	}

	XShmQueryVersion(v->display, &major, &minor, &v->shm);
	if (v->shm) v->shmevent = XShmGetEventBase(v->display) + ShmCompletion;

	return TTRUE;
}

/*****************************************************************************/

LOCAL TBOOL
vis_init(TMOD_VIS *mod)
{
	VISUAL *v = TExecAlloc0(TExecBase, mod->mmu, sizeof(VISUAL));
	if (v == TNULL) return TFALSE;
	mod->hostspecific = v;
	TInitList(&v->imsgpool);

	for (;;)
	{
		XSetWindowAttributes swa;
		TUINT swa_mask;
		XGCValues gcv;
		TUINT gcv_mask;
		int pipefd[2];
		
		v->fd_sigpipe_read = -1;
		v->fd_sigpipe_write = -1;

		v->display = XOpenDisplay(TNULL);
		if (v->display == TNULL) break;

		v->fd_display = ConnectionNumber(v->display);
		if (pipe(pipefd) != 0) break;
		v->fd_sigpipe_read = pipefd[0];
		v->fd_sigpipe_write = pipefd[1];
		v->fd_max = TMAX(v->fd_sigpipe_read, v->fd_display) + 1;

		v->screen = DefaultScreen(v->display);
		v->visual = DefaultVisual(v->display, v->screen);

		v->winwidth = 
			(TINT) TGetTag(mod->inittags, TVisual_PixWidth, (TTAG) 600);
		v->winheight =
			(TINT) TGetTag(mod->inittags, TVisual_PixHeight, (TTAG) 400);
		if (v->winwidth <= 0 || v->winheight <= 0) break;
		
		v->title = (TSTRPTR) TGetTag(mod->inittags, TVisual_Title, (TTAG) "TEKlib visual");

		v->flags = (ImageByteOrder(v->display) !=
			(TUtilIsBigEndian(TUtilBase) ? MSBFirst : LSBFirst)) ?
				TVISF_SWAPBYTEORDER : 0;		

		if (getprops(mod, v) == TFALSE) break;

		v->font = XLoadQueryFont(v->display, DEFFONTNAME);
		if (v->font == TNULL) break;

		v->fontwidth = XTextWidth(v->font, " ", 1);
		v->fontheight = v->font->ascent + v->font->descent;
		if (v->fontwidth <= 0 || v->fontheight <= 0) break;

		v->colormap = XCreateColormap(v->display, 
			RootWindow(v->display, v->screen), v->visual, AllocNone);
		if (v->colormap == TNULL) break;

		swa_mask = CWColormap | CWEventMask;
		swa.colormap = v->colormap;
		swa.event_mask = StructureNotifyMask;

		v->window = XCreateWindow(v->display,
			RootWindow(v->display, v->screen), 0, 0, v->winwidth, v->winheight,
			0, CopyFromParent, CopyFromParent, CopyFromParent,
			swa_mask, &swa);
		if (v->window == TNULL) break;

		XStringListToTextProperty((char **) &v->title, 1, &v->title_prop);
		XSetWMProperties(v->display, v->window, &v->title_prop, 
			NULL, NULL, 0, NULL, NULL, NULL);

		v->atom_wm_delete_win = XInternAtom(v->display,
			"WM_DELETE_WINDOW", True);
		XSetWMProtocols(v->display, v->window, &v->atom_wm_delete_win, 1);

		gcv.font = v->font->fid;
		gcv.function = GXcopy;
		gcv.fill_style = FillSolid;
		gcv.graphics_exposures = False;
		gcv_mask = GCFont | GCFunction | GCFillStyle | GCGraphicsExposures;
			
		v->gc = XCreateGC(v->display, v->window, gcv_mask, &gcv);
		XCopyGC(v->display, XDefaultGC(v->display, v->screen), 
			GCForeground | GCBackground, v->gc);

		XMapWindow(v->display, v->window);

		for (;;)
		{
			XEvent ev;
			XNextEvent(v->display, &ev);
			if (ev.type == MapNotify) break;
		}

		v->base_mask = swa.event_mask;
		v->textwidth = v->winwidth / v->fontwidth;
		v->textheight = v->winheight / v->fontheight;
		v->bgpen = (TVPEN) 0xffffffff;
		v->fgpen = (TVPEN) 0xffffffff;

		shm_available = TTRUE;
		/* will be reverted on first attempt if untrue */

		return TTRUE;
	}

	vis_exit(mod);
	return TFALSE;
}

/*****************************************************************************/

LOCAL TVOID
vis_exit(TMOD_VIS *mod)
{
	VISUAL *v = mod->hostspecific;
	if (v)
	{
		struct TNode *imsg;
		while ((imsg = TRemHead(&v->imsgpool))) TExecFree(TExecBase, imsg);

		TExecFree(TExecBase, v->tempbuf);
		if (v->window) XUnmapWindow(v->display, v->window);
		if (v->gc) XFreeGC(v->display, v->gc);
		if (v->window) XDestroyWindow(v->display, v->window);
		if (v->colormap) XFreeColormap(v->display, v->colormap);
		if (v->font) XFreeFont(v->display, v->font);
		if (v->display) XCloseDisplay(v->display);

		if (v->fd_sigpipe_read != -1)
		{
			close(v->fd_sigpipe_read);
			close(v->fd_sigpipe_write);
		}
		
		TExecFree(TExecBase, v);
		mod->hostspecific = TNULL;
	}
}

/*****************************************************************************/

LOCAL TUINT
vis_wait(TMOD_VIS *mod, TUINT waitsig)
{
	VISUAL *v = mod->hostspecific;
	fd_set rset;

	FD_ZERO(&rset);
	FD_SET(v->fd_display, &rset);
	FD_SET(v->fd_sigpipe_read, &rset);

	if (select(v->fd_max, &rset, NULL, NULL, NULL) > 0)
	{
		if (FD_ISSET(v->fd_sigpipe_read, &rset))
		{
			char sig;
			read(v->fd_sigpipe_read, &sig, 1);
		}
	}

	for (;;)
	{
		TIMSG *imsg = (TIMSG *) TRemHead(&v->imsgpool);
		if (imsg == TNULL)
		{
			imsg = TExecAllocMsg(TExecBase, sizeof(TIMSG));
			if (imsg == TNULL) break;
		}
		if (getnextevent(mod, imsg) == TFALSE)
		{
			TAddTail(&v->imsgpool, (struct TNode *) imsg);
			break;
		}

		vis_sendimsg(mod, imsg);
	}

	return TExecSetSignal(TExecBase, 0, waitsig);
}

/*****************************************************************************/

LOCAL TVOID
vis_wake(TMOD_VIS *mod)
{
	VISUAL *v = mod->hostspecific;
	char sig = 1;
	write(v->fd_sigpipe_write, &sig, 1);
}

/*****************************************************************************/

LOCAL TVOID
vis_docmd(TMOD_VIS *mod, TVREQ *req)
{
	VISUAL *v = mod->hostspecific;
	switch (req->vis_Req.io_Command)
	{
		case TVCMD_FLUSH:
			XFlush(v->display);
			break;

		case TVCMD_SETINPUT:
			setinputmask(mod, v, req->vis_Op.SetInput.Mask);
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
**	Revision 1.5  2005/09/13 02:43:36  tmueller
**	updated copyright reference
**	
**	Revision 1.4  2004/04/18 14:18:00  tmueller
**	TTAG changed to TUINTPTR; atomdata, parseargv changed from TAPTR to TTAG
**	
**	Revision 1.3  2004/02/15 19:43:19  tmueller
**	Typecast to (char **) added in interaction with X function
**	
**	Revision 1.2  2004/01/13 02:22:43  tmueller
**	New visual backend implementations added. They are no longer sub-modules.
**	
**	Revision 1.2  2003/12/13 14:12:56  tmueller
*/
