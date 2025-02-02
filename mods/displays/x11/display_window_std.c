#include <tek/teklib.h>
#include <tek/exec.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/imgproc.h>
#include <tek/debug.h>

#include <tek/mod/imgproc.h>
#include <tek/mod/displayhandler.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>

#include <math.h>
#include <string.h>

#define MOD_VERSION             0
#define MOD_REVISION    1

/* structure needed for allocbitmap */
typedef struct
{
	XImage* theXImage;
	Pixmap theXPixmap;
	GC theXPixmapGC;
	XShmSegmentInfo theShminfo;
}offscreenBitmap;

/* standard font used for textout */
#define DEFAULT_X11_FONT_NAME   "-misc-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*"

typedef struct _TModDismod
{
	TMODL module;                                           /* module header */
	TAPTR exec;
	TAPTR util;
	TAPTR imgp;

	TINT minwidth,maxwidth,minheight,maxheight,mindepth,maxdepth;
	TINT defaultwidth,defaultheight,defaultdepth;

	TUINT msgcode;
	TDISKEY key;
	TDISMOUSEPOS mmove;
	TDISMBUTTON mbutton;
	TDISRECT drect;

	TINT8   *windowname;
	TINT    width, height;
	TBOOL   dblbuf,resize;
	TINT    window_xpos,window_ypos;
	TBOOL   window_ready;
	TINT    bufferdepth,bufferpixelsize;
	TUINT   bufferformat;
	TINT    workbuf;

	TINT font_w,font_h;
	TINT bmwidth, bmheight;

	TDISPEN *theDrawPen;
	TUINT pencolor;

	TINT    ptrmode;
	TBOOL	deltamouse;
	TBOOL	vsync,smoothscale;

	TINT keyqual;

	TINT    mouseoff_x,mouseoff_y;
	TUINT8 *keytranstable;

	/*-------------------- x11 typical variables -------------------------*/
	Display* theXDisplay;
	TINT theXScreen;
	Drawable theXWindow;
	Visual* theXVisual;
	Cursor hiddenCursor,busyCursor;
	Atom wmdeleteatom;

	XFontStruct *theXFont;
	XF86VidModeModeInfo deskMode;
	GC theXWindowGC;
	Colormap theXColormap;

	XImage* theXImage[2];
	Pixmap theXPixmap[2];
	GC theXPixmapGC[2];
	XShmSegmentInfo theShminfo[2];
	XPoint polybuf[257];

	Cursor theCursor;

	offscreenBitmap *theBitmap;
	TBOOL waitmessage;
	TBOOL mousemoved;
	TBOOL newmsgloop;
	/*--------------------------------------------------------------------*/

	TBOOL fullscreen;

} TMOD_DISMOD;

/* private prototypes */
TBOOL Dismod_ReadProperties( TMOD_DISMOD *dismod );
TBOOL Dismod_CreateWindow(TMOD_DISMOD *dismod);
TBOOL Dismod_CreateDirectBitmap(TMOD_DISMOD *dismod);
TVOID Dismod_DestroyDirectBitmap( TMOD_DISMOD *dismod );
TVOID Dismod_ParseBufferFormat( TMOD_DISMOD *dismod );
TVOID Dismod_ProcessMessage(TMOD_DISMOD *dismod,TDISMSG *dismsg);
TVOID Dismod_ProcessEvents(TMOD_DISMOD *dismod,XEvent *event);
TVOID Dismod_MakeKeytransTable(TMOD_DISMOD *dismod);

/* global module init stuff */
#include "../modinit.h"

/* XLib drawing routines */
#include "xlibdrawdisplay.h"
#include "xlibdrawbitmap.h"

/* standard message callback */
#include "xcommon.h"

/**************************************************************************
	tek_init
 **************************************************************************/
TMODENTRY TUINT tek_init_display_window_std(TAPTR selftask, TMOD_DISMOD *mod, TUINT16 version, TTAGITEM *tags)
{
	return Dismod_InitMod(selftask,mod,version,tags);
}

/**************************************************************************
	dismod_create
		- create a standard window
		- ALWAYS doublebuffered to ensure clipping
 **************************************************************************/
TMODAPI TBOOL dismod_create(TMOD_DISMOD *dismod, TSTRPTR title, TINT x, TINT y, TINT w, TINT h, TINT d, TUINT flags)
{
	dismod->width=w;
	dismod->height=h;
	dismod->window_xpos=x;
	dismod->window_ypos=y;
	dismod->workbuf=0;

	dismod->fullscreen=TFALSE;

	if((flags & TDISCF_DOUBLEBUFFER)==TDISCF_DOUBLEBUFFER)
		dismod->dblbuf=TTRUE;
	else
		dismod->dblbuf=TFALSE;

	if((flags & TDISCF_RESIZEABLE)==TDISCF_RESIZEABLE)
		dismod->resize=TTRUE;
	else
		dismod->resize=TFALSE;

	dismod->windowname=TExecAlloc0(dismod->exec,TNULL,TUtilStrLen(dismod->util,title)+1);
	strcpy(dismod->windowname,title);

	if(Dismod_CreateWindow(dismod))
	{
		Dismod_MakeKeytransTable(dismod);

		dismod->ptrmode     = TDISPTR_NORMAL;
		dismod->deltamouse	 = TFALSE;
		dismod->vsync		 = TFALSE;
		dismod->smoothscale = TFALSE;

		Dismod_SetMousePtrMode(dismod,dismod->ptrmode);

		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	dismod_destroy
 **************************************************************************/
TMODAPI TVOID dismod_destroy(TMOD_DISMOD *dismod)
{
	if(dismod->theXDisplay)
	{
		XAutoRepeatOn(dismod->theXDisplay);
		Dismod_DestroyDirectBitmap(dismod);

		if(dismod->theXColormap)
			XFreeColormap(dismod->theXDisplay,dismod->theXColormap);

		if(dismod->theXFont)
			XUnloadFont(dismod->theXDisplay, dismod->theXFont->fid);

		if(dismod->theXWindowGC)
			XFreeGC(dismod->theXDisplay, dismod->theXWindowGC);

		if(dismod->theXWindow)
			XDestroyWindow(dismod->theXDisplay,dismod->theXWindow);

		XCloseDisplay(dismod->theXDisplay);
	}

	if(dismod->windowname)
		TExecFree(dismod->exec,dismod->windowname);

	if(dismod->keytranstable)
		TExecFree(dismod->exec,dismod->keytranstable);
}

/**************************************************************************
	dismod_getproperties
 **************************************************************************/
TMODAPI TVOID dismod_getproperties(TMOD_DISMOD *dismod, TDISPROPS *props)
{
	props->version=DISPLAYHANDLER_VERSION;
	props->priority=0;
	props->dispclass=TDISCLASS_STANDARD;
	props->dispmode=TDISMODE_WINDOW;
	props->minwidth=dismod->minwidth;
	props->maxwidth=dismod->maxwidth;
	props->minheight=dismod->minheight;
	props->maxheight=dismod->maxheight;
	props->mindepth=dismod->mindepth;
	props->maxdepth=dismod->maxdepth;
	props->defaultwidth=dismod->defaultwidth;
	props->defaultheight=dismod->defaultheight;
	props->defaultdepth=dismod->defaultdepth;
}

/**************************************************************************
	dismod_getcaps
 **************************************************************************/
TMODAPI TVOID dismod_getcaps(TMOD_DISMOD *dismod, TDISCAPS *caps)
{
	caps->minbmwidth=16;
	caps->minbmheight=16;
	caps->maxbmwidth=32768;
	caps->maxbmheight=32768;

	caps->blitscale=TFALSE;
	caps->blitalpha=TFALSE;
	caps->blitckey=TTRUE;

	caps->canconvertdisplay=TFALSE;
	caps->canconvertscaledisplay=TFALSE;
	caps->canconvertbitmap=TFALSE;
	caps->canconvertscalebitmap=TFALSE;
	caps->candrawbitmap=TTRUE;
}

/**************************************************************************
	dismod_getmodelist
 **************************************************************************/
TMODAPI TINT dismod_getmodelist(TMOD_DISMOD *dismod, TDISMODE **modelist)
{
	*modelist=TNULL;
	return TNULL;
}

/**************************************************************************
	dismod_setattrs
 **************************************************************************/
TMODAPI TVOID dismod_setattrs(TMOD_DISMOD *dismod, TTAGITEM *tags)
{
	TBOOL dm 			 = (TINT)TGetTag(tags,TDISTAG_DELTAMOUSE,(TTAG)dismod->deltamouse);
	TINT ptr 			 = (TINT)TGetTag(tags,TDISTAG_POINTERMODE,(TTAG)dismod->ptrmode);
	dismod->vsync		 = (TINT)TGetTag(tags,TDISTAG_VSYNCHINT,(TTAG)dismod->vsync);
	dismod->smoothscale = (TINT)TGetTag(tags,TDISTAG_SMOOTHHINT,(TTAG)dismod->smoothscale);

	dismod->ptrmode=ptr;

	if(dm!=dismod->deltamouse)
	{
		if(dm)
		{
			XWarpPointer(dismod->theXDisplay,
						 0,
						 dismod->theXWindow,
						 0,0,0,0,
						 dismod->width/2,dismod->height/2);

			dismod->mousemoved=TTRUE;
			ptr=TDISPTR_INVISIBLE;
		}
		dismod->deltamouse=dm;
	}

	Dismod_SetMousePtrMode(dismod,ptr);
}

/**************************************************************************
	dismod_flush
 **************************************************************************/
TMODAPI TVOID dismod_flush(TMOD_DISMOD *dismod)
{
	if(dismod->window_ready)
	{
		XShmPutImage(dismod->theXDisplay,
			     dismod->theXWindow,
			     dismod->theXWindowGC,
			     dismod->theXImage[dismod->workbuf],
			     0,0,0,0,
			     dismod->width,dismod->height,
			     False);

		if(dismod->dblbuf)
			dismod->workbuf=1 - dismod->workbuf;
	}
}

/**************************************************************************
	dismod_allocpen
 **************************************************************************/
TMODAPI TBOOL dismod_allocpen(TMOD_DISMOD *dismod, TDISPEN *pen)
{
	if(dismod->window_ready)
	{
		XColor *xcolor=TExecAlloc0(dismod->exec,TNULL,sizeof(XColor));

		xcolor->red =   pen->color.r << 8;
		xcolor->green = pen->color.g << 8;
		xcolor->blue =  pen->color.b << 8;
		xcolor->flags = DoRed | DoGreen | DoBlue;

		if (!XAllocColor(dismod->theXDisplay, dismod->theXColormap, xcolor))
		{
			TExecFree(dismod->exec,xcolor);
			return TFALSE;;
		}
		pen->hostdata=(TAPTR)xcolor;
		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	dismod_freepen
 **************************************************************************/
TMODAPI TVOID dismod_freepen(TMOD_DISMOD *dismod, TDISPEN *pen)
{
	if(dismod->window_ready)
	{
		XColor *xcolor=(XColor*)pen->hostdata;
		if(dismod->theDrawPen==pen)	dismod->theDrawPen=TNULL;
		XFreeColors(dismod->theXDisplay, dismod->theXColormap, &xcolor->pixel, 1, 0);
		TExecFree(dismod->exec,xcolor);
	}
}

/**************************************************************************
	dismod_setdpen
 **************************************************************************/
TMODAPI TVOID dismod_setdpen(TMOD_DISMOD *dismod, TDISPEN *pen)
{
	XColor *xcolor=(XColor*)pen->hostdata;
	dismod->theDrawPen=pen;
	if(!dismod->theBitmap)
		XSetForeground(dismod->theXDisplay, dismod->theXPixmapGC[dismod->workbuf], xcolor->pixel);
	else
		XSetForeground(dismod->theXDisplay, dismod->theBitmap->theXPixmapGC, xcolor->pixel);
}

/**************************************************************************
	dismod_setpalette
 **************************************************************************/
TMODAPI TBOOL dismod_setpalette(TMOD_DISMOD *dismod, TIMGARGBCOLOR *pal, TINT sp, TINT sd, TINT numentries)
{
	/* its a no-op in windowed environments! */
	return TFALSE;
}

/**************************************************************************
	dismod_allocbitmap
 **************************************************************************/
TMODAPI TBOOL dismod_allocbitmap(TMOD_DISMOD *dismod, TDISBITMAP *bitmap, TINT width, TINT height, TINT flags)
{
	if(dismod->window_ready)
	{
		offscreenBitmap *bm=TExecAlloc0(dismod->exec,TNULL,sizeof(offscreenBitmap));

		XSync(dismod->theXDisplay,False);

		bm->theXImage = XShmCreateImage(dismod->theXDisplay, dismod->theXVisual,
										dismod->theXImage[0]->depth, ZPixmap, NULL,
										&bm->theShminfo,
										width, height);

		bm->theShminfo.shmid = shmget(  IPC_PRIVATE,
										bm->theXImage->bytes_per_line * bm->theXImage->height,
										IPC_CREAT|0666);

		bm->theShminfo.shmaddr = (TINT8 *)shmat(bm->theShminfo.shmid, 0, 0);

		bm->theXImage->data = bm->theShminfo.shmaddr;
		bm->theShminfo.readOnly = False;
		XShmAttach(dismod->theXDisplay, &bm->theShminfo);

		bm->theXPixmap=XShmCreatePixmap(dismod->theXDisplay, dismod->theXWindow,
										bm->theShminfo.shmaddr, &bm->theShminfo,
										width,height,dismod->theXImage[0]->depth);

		bm->theXPixmapGC = XCreateGC(dismod->theXDisplay,bm->theXPixmap,0,NULL);

		XSync(dismod->theXDisplay,False);

		bitmap->hostdata=(TAPTR)bm;
		bitmap->image.width=bm->theXImage->width;
		bitmap->image.height=bm->theXImage->height;
		bitmap->image.depth=dismod->bufferdepth;
		bitmap->image.format=dismod->bufferformat;
		bitmap->image.bytesperrow=bm->theXImage->bytes_per_line;

		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	dismod_freebitmap
 **************************************************************************/
TMODAPI TVOID dismod_freebitmap(TMOD_DISMOD *dismod, TDISBITMAP *bitmap)
{
	offscreenBitmap *bm;

	XSync(dismod->theXDisplay,False);

	bm=(offscreenBitmap*)bitmap->hostdata;

	if(bm->theXImage)
	{
		if(bm->theXPixmapGC)
			XFreeGC(dismod->theXDisplay, bm->theXPixmapGC);

		XShmDetach(dismod->theXDisplay, &bm->theShminfo);
		XFreePixmap (dismod->theXDisplay,bm->theXPixmap);
		XDestroyImage (bm->theXImage);
		shmdt(bm->theShminfo.shmaddr);
		shmctl(bm->theShminfo.shmid, IPC_RMID, 0);
	}
	TExecFree(dismod->exec,bm);
	XSync(dismod->theXDisplay,False);
}

/**************************************************************************
	dismod_describe_dis
 **************************************************************************/
TMODAPI TVOID dismod_describe_dis(TMOD_DISMOD *dismod, TDISDESCRIPTOR *desc)
{
	if(dismod->window_ready)
	{
		desc->x=dismod->window_xpos;
		desc->y=dismod->window_ypos;
		desc->width=dismod->theXImage[dismod->workbuf]->width;
		desc->height=dismod->theXImage[dismod->workbuf]->height;
		desc->depth=dismod->bufferdepth;
		desc->format=dismod->bufferformat;
		desc->bytesperrow=dismod->theXImage[dismod->workbuf]->bytes_per_line;
	}
}

/**************************************************************************
	dismod_describe_bm
 **************************************************************************/
TMODAPI TVOID dismod_describe_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TDISDESCRIPTOR *desc)
{
	if(dismod->window_ready)
	{
		offscreenBitmap *bitmap=(offscreenBitmap*)bm->hostdata;

		desc->x=0;
		desc->y=0;
		desc->width=bitmap->theXImage->width;
		desc->height=bitmap->theXImage->height;
		desc->depth=dismod->bufferdepth;
		desc->format=dismod->bufferformat;
		desc->bytesperrow=bitmap->theXImage->bytes_per_line;
	}
}

/**************************************************************************
	dismod_lock_dis
 **************************************************************************/
TMODAPI TBOOL dismod_lock_dis(TMOD_DISMOD *dismod, TIMGPICTURE *img)
{
	if(dismod->window_ready)
	{
		XSync(dismod->theXDisplay,False);

		img->bytesperrow=dismod->theXImage[dismod->workbuf]->bytes_per_line;
		img->width=dismod->theXImage[dismod->workbuf]->width;
		img->height=dismod->theXImage[dismod->workbuf]->height;
		img->depth=dismod->bufferdepth;
		img->format=dismod->bufferformat;
		img->data=(TUINT8*)dismod->theXImage[dismod->workbuf]->data;
		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	dismod_lock_bm
 **************************************************************************/
TMODAPI TBOOL dismod_lock_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img)
{
	if(dismod->window_ready)
	{
		dismod->theBitmap=(offscreenBitmap*)bm->hostdata;
		XSync(dismod->theXDisplay,False);

		img->bytesperrow=dismod->theBitmap->theXImage->bytes_per_line;
		img->width=dismod->theBitmap->theXImage->width;
		img->height=dismod->theBitmap->theXImage->height;
		img->depth=dismod->bufferdepth;
		img->format=dismod->bufferformat;
		img->data=(TUINT8*)dismod->theBitmap->theXImage->data;
		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	dismod_unlock_dis
 **************************************************************************/
TMODAPI TVOID dismod_unlock_dis(TMOD_DISMOD *dismod)
{
	XSync(dismod->theXDisplay,False);
}

/**************************************************************************
	dismod_unlock_bm
 **************************************************************************/
TMODAPI TVOID dismod_unlock_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
	XSync(dismod->theXDisplay,False);
	dismod->theBitmap=TNULL;
}

/**************************************************************************
	dismod_begin_dis
 **************************************************************************/
TMODAPI TBOOL dismod_begin_dis(TMOD_DISMOD *dismod)
{
	if(dismod->window_ready)
	{
		XSync(dismod->theXDisplay,False);
		if(dismod->theDrawPen)
		{
			XColor *xcolor=(XColor*)dismod->theDrawPen->hostdata;
			XSetForeground(dismod->theXDisplay, dismod->theXPixmapGC[dismod->workbuf], xcolor->pixel);
		}
		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	dismod_begin_bm
 **************************************************************************/
TMODAPI TBOOL dismod_begin_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
	if(dismod->window_ready)
	{
		dismod->theBitmap=(offscreenBitmap*)bm->hostdata;
		XSync(dismod->theXDisplay,False);
		if(dismod->theDrawPen)
		{
			XColor *xcolor=(XColor*)dismod->theDrawPen->hostdata;
			XSetForeground(dismod->theXDisplay,dismod->theBitmap->theXPixmapGC,xcolor->pixel);
		}
		dismod->bmwidth=bm->image.width;
		dismod->bmheight=bm->image.height;
		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	dismod_end_dis
 **************************************************************************/
TMODAPI TVOID dismod_end_dis(TMOD_DISMOD *dismod)
{
	if(dismod->window_ready)
		XSync(dismod->theXDisplay,False);
}

/**************************************************************************
	dismod_end_bm
 **************************************************************************/
TMODAPI TVOID dismod_end_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
	if(dismod->window_ready)
	{
		XSync(dismod->theXDisplay,False);
		dismod->theBitmap=TNULL;
	}
}

/**************************************************************************
	dismod_blit
 **************************************************************************/
TMODAPI TVOID dismod_blit(TMOD_DISMOD *dismod, TDISBITMAP *bm,TDBLITOPS *bops)
{
	if(dismod->window_ready)
	{
		offscreenBitmap *bitmap=(offscreenBitmap*)bm->hostdata;

		XShmPutImage(dismod->theXDisplay,
			     dismod->theXPixmap[dismod->workbuf],
			     //dismod->theXPixmapGC[dismod->workbuf],
				 bitmap->theXPixmapGC,
			     bitmap->theXImage,
			     bops->src.x,bops->src.y,
			     bops->dst.x,bops->dst.y,
			     bops->src.width,bops->src.height,
			     False);
	}
}

/**************************************************************************
	dismod_putimage_dis
 **************************************************************************/
TMODAPI TVOID dismod_putimage_dis(TMOD_DISMOD *dismod, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	/* emulated via dismodhandler */
}

/**************************************************************************
	dismod_putimage_bm
 **************************************************************************/
TMODAPI TVOID dismod_putimage_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	/* emulated via dismodhandler */
}

/**************************************************************************
	dismod_putscaleimage_dis
 **************************************************************************/
TMODAPI TVOID dismod_putscaleimage_dis(TMOD_DISMOD *dismod, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	/* emulated via dismodhandler */
}

/**************************************************************************
	dismod_putscaleimage_bm
 **************************************************************************/
TMODAPI TVOID dismod_putscaleimage_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	/* emulated via dismodhandler */
}



/**************************************************************************
 **************************************************************************

  private routines

 **************************************************************************
 **************************************************************************/

/**************************************************************************
	ReadProperties
 **************************************************************************/
TBOOL Dismod_ReadProperties( TMOD_DISMOD *dismod )
{
	TINT d;

	dismod->theXDisplay = XOpenDisplay(0);
	if(dismod->theXDisplay)
	{
		dismod->theXScreen = DefaultScreen(dismod->theXDisplay);
		dismod->theXVisual = DefaultVisual(dismod->theXDisplay,dismod->theXScreen);

		dismod->theXWindow=0;

		d=DefaultDepth(dismod->theXDisplay,dismod->theXScreen);
		if(d>=15)
		{
			dismod->mindepth=15;
			dismod->maxdepth=32;
			dismod->defaultdepth=d;

			dismod->minwidth=64;
			dismod->minheight=64;
			dismod->maxwidth=DisplayWidth(dismod->theXDisplay,dismod->theXScreen);
			dismod->maxheight=DisplayHeight(dismod->theXDisplay,dismod->theXScreen);

			dismod->defaultwidth=dismod->maxwidth/2;
			dismod->defaultheight=dismod->maxheight/2;

			return TTRUE;
		}
	}

	XCloseDisplay(dismod->theXDisplay);
	return TFALSE;
}

/**************************************************************************
	CreateWindow
 **************************************************************************/
TBOOL Dismod_CreateWindow(TMOD_DISMOD *dismod)
{
	XSetWindowAttributes attr;
	XSizeHints    size_hints;

	dismod->theXColormap = XCreateColormap( dismod->theXDisplay,
						 RootWindow(dismod->theXDisplay,dismod->theXScreen),
						 dismod->theXVisual,
						 AllocNone);

	attr.colormap = dismod->theXColormap;
	attr.border_pixel = 0;

	attr.event_mask = ExposureMask |
			  KeyPressMask | KeyReleaseMask |
			  ButtonPressMask | ButtonReleaseMask |
			  PointerMotionMask |
			  StructureNotifyMask | VisibilityChangeMask;

	if(dismod->defaultdepth<=8)
	{
		tdbprintf(10,"dismod: for windowed mode your desktop must have 15bit or more!\n");
		return TFALSE;
	}

	dismod->theXWindow = XCreateWindow( dismod->theXDisplay,
					    RootWindow(dismod->theXDisplay, dismod->theXScreen),
					    dismod->window_xpos, dismod->window_ypos, dismod->width, dismod->height,
					    0, CopyFromParent, InputOutput,
					    dismod->theXVisual,
					    CWBorderPixel | CWColormap | CWEventMask,
					    &attr);

	if(!dismod->theXWindow)
	{
		tdbprintf(10,"dismod: couldn't open window\n");
		return TFALSE;
	}

	if(dismod->resize)
	{
		size_hints.flags = (PSize | PMinSize);
		size_hints.min_width = dismod->minwidth;
		size_hints.min_height = dismod->minheight;
		size_hints.base_width = dismod->width;
		size_hints.base_height = dismod->height;
	}
	else
	{
		size_hints.flags = (PSize | PMinSize | PMaxSize);
		size_hints.base_width = size_hints.min_width = size_hints.max_width=  dismod->width;
		size_hints.base_height= size_hints.min_height= size_hints.max_height= dismod->height;
	}
	XSetWMNormalHints (dismod->theXDisplay, dismod->theXWindow, &size_hints);
	dismod->wmdeleteatom = XInternAtom(dismod->theXDisplay, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(dismod->theXDisplay, dismod->theXWindow, &dismod->wmdeleteatom, 1);
	XSetStandardProperties(dismod->theXDisplay, dismod->theXWindow,dismod->windowname,dismod->windowname, None, NULL, 0, NULL);
	XMapRaised(dismod->theXDisplay, dismod->theXWindow);

	dismod->theXWindowGC = XCreateGC(dismod->theXDisplay,dismod->theXWindow,0,NULL);

	dismod->theXFont = XLoadQueryFont(dismod->theXDisplay, DEFAULT_X11_FONT_NAME);
	if(dismod->theXFont)
	{
		dismod->font_w = XTextWidth(dismod->theXFont, " ", 1);
		dismod->font_h = dismod->theXFont->ascent + dismod->theXFont->descent;
	}

	if(!Dismod_CreateDirectBitmap(dismod))
	{
		tdbprintf(10,"dismod: couldn't create backbuffer bitmap(s)\n");
		return TFALSE;
	}

	Dismod_ParseBufferFormat(dismod);
	XAutoRepeatOff(dismod->theXDisplay);
	dismod->window_ready=TTRUE;
	return TTRUE;
}

/********************************************************************************
	Display CreateDirectBitmap
 ********************************************************************************/
TBOOL Dismod_CreateDirectBitmap(TMOD_DISMOD *dismod)
{
	TINT i=0;
	TINT numbufs=1;

	XSync(dismod->theXDisplay,False);

	if(dismod->dblbuf)
		numbufs=2;

	if(!XShmQueryExtension(dismod->theXDisplay))
		return TFALSE;

	while(i<numbufs)
	{
		dismod->theXImage[i] = XShmCreateImage(dismod->theXDisplay, dismod->theXVisual,
												dismod->defaultdepth, ZPixmap, NULL,
												&dismod->theShminfo[i],
												dismod->width, dismod->height);

		dismod->theShminfo[i].shmid = shmget(	IPC_PRIVATE,
												dismod->theXImage[i]->bytes_per_line * dismod->theXImage[i]->height,
												IPC_CREAT|0666);

		dismod->theShminfo[i].shmaddr = (TINT8 *)shmat(dismod->theShminfo[i].shmid, 0, 0);

		dismod->theXImage[i]->data = dismod->theShminfo[i].shmaddr;
		dismod->theShminfo[i].readOnly = False;
		XShmAttach(dismod->theXDisplay, &dismod->theShminfo[i]);

		dismod->theXPixmap[i]=XShmCreatePixmap(dismod->theXDisplay, dismod->theXWindow,
												dismod->theShminfo[i].shmaddr, &dismod->theShminfo[i],
												dismod->width,dismod->height,dismod->defaultdepth);

		dismod->theXPixmapGC[i] = XCreateGC(dismod->theXDisplay,dismod->theXPixmap[i],0,NULL);

		i++;
	}
	XSync(dismod->theXDisplay,False);

	return TTRUE;
}

/********************************************************************************
	Display DestroyDirectBitmap
 ********************************************************************************/
TVOID Dismod_DestroyDirectBitmap( TMOD_DISMOD *dismod )
{
	TINT i=0;
	TINT numbufs=1;

	XSync(dismod->theXDisplay,False);

	if(dismod->dblbuf)
		numbufs=2;

	while(i<numbufs)
	{
		if(dismod->theXImage[i])
		{
			if(dismod->theXPixmapGC[i])
			{
				XFreeGC(dismod->theXDisplay, dismod->theXPixmapGC[i]);
				dismod->theXPixmapGC[i]=TNULL;
			}

			XShmDetach(dismod->theXDisplay, &dismod->theShminfo[i]);
			XFreePixmap (dismod->theXDisplay,dismod->theXPixmap[i]);
			XDestroyImage (dismod->theXImage[i]);
			shmdt(dismod->theShminfo[i].shmaddr);
			shmctl(dismod->theShminfo[i].shmid, IPC_RMID, 0);

			dismod->theXImage[i]=TNULL;
		}
		i++;
	}
	XSync(dismod->theXDisplay,False);
}

/**************************************************************************
  ParseBufferFormat
 **************************************************************************/
TVOID Dismod_ParseBufferFormat( TMOD_DISMOD *dismod )
{
	TUINT rm,gm,bm;

	rm=dismod->theXImage[0]->red_mask;
	gm=dismod->theXImage[0]->green_mask;
	bm=dismod->theXImage[0]->blue_mask;

	dismod->bufferdepth=dismod->theXImage[0]->bits_per_pixel;

	switch(dismod->bufferdepth)
	{
		case 16:
			if( rm==0x00007c00 && gm==0x000003e0 && bm==0x0000001f )
				dismod->bufferformat=IMGFMT_R5G5B5;
			else if(rm==0x0000f800 && gm==0x000007e0 && bm==0x0000001f )
				dismod->bufferformat=IMGFMT_R5G6B5;
		break;

		case 24:
			if( rm==0x00ff0000 && gm==0x0000ff00 && bm==0x000000ff )
				dismod->bufferformat=IMGFMT_R8G8B8;
			else if( rm==0x000000ff && gm==0x0000ff00 && bm==0x00ff0000 )
				dismod->bufferformat=IMGFMT_B8G8R8;
		break;

		case 32:
			if( rm==0x00ff0000 && gm==0x0000ff00 && bm==0x000000ff )
				dismod->bufferformat=IMGFMT_A8R8G8B8;
			else if( rm==0x0000ff00 && gm==0x00ff0000 && bm==0xff000000 )
				dismod->bufferformat=IMGFMT_B8G8R8A8;
			else if( rm==0xff000000 && gm==0x00ff0000 && bm==0x0000ff00 )
				dismod->bufferformat=IMGFMT_R8G8B8A8;
		break;
	}
}

/**************************************************************************
  Display ProcessMessage
 **************************************************************************/
TVOID Dismod_ProcessMessage(TMOD_DISMOD *dismod,TDISMSG *dismsg)
{
	switch (dismsg->code)
	{
		case TDISMSG_RESIZE:
			Dismod_DestroyDirectBitmap(dismod);
			Dismod_CreateDirectBitmap(dismod);
			TExecCopyMem(dismod->exec,&dismod->drect,dismsg->data,sizeof(TDISRECT));
		break;

	 	case TDISMSG_MOVE:
			TExecCopyMem(dismod->exec,&dismod->drect,dismsg->data,sizeof(TDISRECT));
		break;

		case TDISMSG_KEYDOWN: case TDISMSG_KEYUP:
			TExecCopyMem(dismod->exec,&dismod->key,dismsg->data,sizeof(TDISKEY));
		break;

		case TDISMSG_MOUSEMOVE:
			TExecCopyMem(dismod->exec,&dismod->mmove,dismsg->data,sizeof(TDISMOUSEPOS));
		break;

		case TDISMSG_MBUTTONDOWN: case TDISMSG_MBUTTONUP:
			TExecCopyMem(dismod->exec,&dismod->mbutton,dismsg->data,sizeof(TDISMBUTTON));
		break;

	}
}

