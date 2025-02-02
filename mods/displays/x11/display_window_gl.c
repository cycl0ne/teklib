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

#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <math.h>
#include <string.h>

#define MOD_VERSION             0
#define MOD_REVISION    1

/* some definitions taken from glext.h */
#define GL_UNSIGNED_BYTE_3_3_2            0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_INT_8_8_8_8           0x8035
#define GL_UNSIGNED_INT_10_10_10_2        0x8036
#define GL_UNSIGNED_BYTE_2_3_3_REV        0x8362
#define GL_UNSIGNED_SHORT_5_6_5           0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV       0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV     0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV     0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#define GL_BGR                            0x80E0
#define GL_BGRA                           0x80E1

/*#ifndef GLX_SGI_swap_control
#define GLX_SGI_swap_control 1
typedef int ( * PFNGLXSWAPINTERVALSGIPROC) (int interval);
#endif
PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI;
void *glXGetProcAddressARB (const GLubyte *procName);
*/

/* structure needed for allocbitmap */
typedef struct
{
	XImage* theXImage;
	Pixmap theXPixmap;
	GC theXPixmapGC;
	XShmSegmentInfo theShminfo;
	GLuint gltexname;
	TINT w,h;
	TINT ckeycol;
	TINT alphaval;
	TBOOL changed;
}offscreenBitmap;

/* structure needed for allocpen */
typedef struct
{
	GLfloat r,g,b;
} GLPen;

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

	TINT    ptrmode;
	TBOOL	deltamouse;
	TBOOL	vsync,smoothscale;

	TINT keyqual;

	TINT    mouseoff_x,mouseoff_y;
	TUINT8 *keytranstable;

	/* opengl stuff */
	GLuint	fontlist;
	GLPen 	theGlDrawPen;
	TINT	numsysglyphs;

	/*-------------------- x11 typical variables -------------------------*/
	Display* theXDisplay;
	TINT theXScreen;
	Drawable theXWindow;
	Visual* theXVisual;
	Cursor hiddenCursor,busyCursor;
	Atom wmdeleteatom;
	GLXContext	lockContext, drawContext;
	TDISPEN *theDrawPen;

	XFontStruct *theXFont;
	XF86VidModeModeInfo deskMode;
	Colormap theXColormap;

	XImage* theXImage[2];
	Pixmap theXPixmap[2];
	GC theXPixmapGC[2];
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
TVOID Dismod_PutImage(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst);
TVOID Dismod_ProcessMessage(TMOD_DISMOD *dismod,TDISMSG *dismsg);
TVOID Dismod_ProcessEvents(TMOD_DISMOD *dismod,XEvent *event);
TVOID Dismod_MakeKeytransTable(TMOD_DISMOD *dismod);

/* global module init stuff */
#include "../modinit.h"

/* OpenGL drawing routines */
#include "../gldrawdisplay.h"

/* XLib drawing routines */
#include "xlibdrawbitmap.h"

/* standard message callback */
#include "xcommon.h"

/**************************************************************************
	tek_init
 **************************************************************************/
TMODENTRY TUINT tek_init_display_window_gl(TAPTR selftask, TMOD_DISMOD *mod, TUINT16 version, TTAGITEM *tags)
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
		if(dismod->fontlist)
		{
			if(dismod_begin_dis(dismod))
			{
				glDeleteLists(dismod->fontlist,256);
				dismod_end_dis(dismod);
			}
		}

		XAutoRepeatOn(dismod->theXDisplay);

		if (dismod->drawContext)
			glXDestroyContext(dismod->theXDisplay, dismod->drawContext);

		if (dismod->lockContext)
			glXDestroyContext(dismod->theXDisplay, dismod->lockContext);

		if(dismod->theXColormap)
			XFreeColormap(dismod->theXDisplay,dismod->theXColormap);

		if(dismod->theXFont)
			XUnloadFont(dismod->theXDisplay, dismod->theXFont->fid);

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
	props->dispclass=TDISCLASS_OPENGL;
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
	caps->minbmwidth=1;
	caps->minbmheight=1;
	caps->maxbmwidth=256;
	caps->maxbmheight=256;

	if(dismod->window_ready)
	{
		if(glXMakeCurrent(dismod->theXDisplay, dismod->theXWindow, dismod->drawContext))
		{
			GLint val;

			glGetIntegerv(GL_MAX_TEXTURE_SIZE,&val);
			caps->maxbmwidth=val/4;
			caps->maxbmheight=val/4;

			glXMakeCurrent(dismod->theXDisplay, None, NULL);
		}
	}

	caps->blitscale=TTRUE;
	caps->blitalpha=TTRUE;
	caps->blitckey=TTRUE;

	caps->canconvertdisplay=TTRUE;
	caps->canconvertscaledisplay=TTRUE;
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
/*
	if(glXSwapIntervalSGI)
	{
		if(dismod->vsync)
		{
			if(glXMakeCurrent(dismod->theXDisplay, dismod->theXWindow, dismod->drawContext))
			{
				glXSwapIntervalSGI(1);
				glXMakeCurrent(dismod->theXDisplay, None, NULL);
			}
		}
		else
		{
			if(glXMakeCurrent(dismod->theXDisplay, dismod->theXWindow, dismod->drawContext))
			{
				glXSwapIntervalSGI(0);
				glXMakeCurrent(dismod->theXDisplay, None, NULL);
			}
		}
	}*/
}

/**************************************************************************
	dismod_flush
 **************************************************************************/
TMODAPI TVOID dismod_flush(TMOD_DISMOD *dismod)
{
	if(dismod->dblbuf)
	{
		glFinish();
		glXSwapBuffers(dismod->theXDisplay, dismod->theXWindow);
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

	if(dismod->theBitmap)
		XSetForeground(dismod->theXDisplay, dismod->theBitmap->theXPixmapGC, xcolor->pixel);

	dismod->theGlDrawPen.r=(GLfloat)pen->color.r / 255.0f;
	dismod->theGlDrawPen.g=(GLfloat)pen->color.g / 255.0f;
	dismod->theGlDrawPen.b=(GLfloat)pen->color.b / 255.0f;
}

/**************************************************************************
	dismod_setpalette
 **************************************************************************/
TMODAPI TBOOL dismod_setpalette(TMOD_DISMOD *dismod, TIMGARGBCOLOR *pal, TINT sp, TINT sd, TINT numentries)
{
	/* its a no-op in opengl environments! */
	return TFALSE;
}

/**************************************************************************
	dismod_allocbitmap
 **************************************************************************/
TMODAPI TBOOL dismod_allocbitmap(TMOD_DISMOD *dismod, TDISBITMAP *bitmap, TINT width, TINT height, TINT flags)
{
	if(dismod->window_ready)
	{
		TUINT rma,gma,bma;

		offscreenBitmap *bm=TExecAlloc0(dismod->exec,TNULL,sizeof(offscreenBitmap));

		bm->w=(TINT)pow(2,(TINT)(log(width)/log(2.0)));
		if(bm->w<width)
			bm->w+=bm->w;

		bm->h=(TINT)pow(2,(TINT)(log(height)/log(2.0)));
		if(bm->h<height)
			bm->h+=bm->h;

		XSync(dismod->theXDisplay,False);

		bm->theXImage = XShmCreateImage(dismod->theXDisplay, dismod->theXVisual,
										dismod->bufferdepth, ZPixmap, NULL,
										&bm->theShminfo,
										width, height);

		bm->theShminfo.shmid = shmget(  IPC_PRIVATE,
										bm->theXImage->bytes_per_line * bm->theXImage->height,
										IPC_CREAT|0777);

		bm->theShminfo.shmaddr = (TINT8 *)shmat(bm->theShminfo.shmid, 0, 0);

		bm->theXImage->data = bm->theShminfo.shmaddr;
		bm->theShminfo.readOnly = False;
		XShmAttach(dismod->theXDisplay, &bm->theShminfo);

		bm->theXPixmap=XShmCreatePixmap ( 	dismod->theXDisplay, dismod->theXWindow,
											bm->theShminfo.shmaddr, &bm->theShminfo,
											width,height,dismod->bufferdepth);

		bm->theXPixmapGC = XCreateGC(dismod->theXDisplay,bm->theXPixmap,0,NULL);

		XSync(dismod->theXDisplay,False);

		bm->changed=TTRUE;

		bitmap->hostdata=(TAPTR)bm;
		bitmap->image.width=width;
		bitmap->image.height=height;
		bitmap->image.bytesperrow=bm->theXImage->bytes_per_line;
		bitmap->image.depth=bm->theXImage->bits_per_pixel;

		rma=bm->theXImage->red_mask;
		gma=bm->theXImage->green_mask;
		bma=bm->theXImage->blue_mask;

		switch(bitmap->image.depth)
		{
			case 16:
				if( rma==0x00007c00 && gma==0x000003e0 && bma==0x0000001f )
					bitmap->image.format=IMGFMT_R5G5B5;
				else if(rma==0x0000f800 && gma==0x000007e0 && bma==0x0000001f )
					bitmap->image.format=IMGFMT_R5G6B5;
			break;

			case 24:
				if( rma==0x00ff0000 && gma==0x0000ff00 && bma==0x000000ff )
					bitmap->image.format=IMGFMT_R8G8B8;
				else if( rma==0x000000ff && gma==0x0000ff00 && bma==0x00ff0000 )
					bitmap->image.format=IMGFMT_B8G8R8;
			break;

			case 32:
				if( rma==0x00ff0000 && gma==0x0000ff00 && bma==0x000000ff )
					bitmap->image.format=IMGFMT_A8R8G8B8;
				else if( rma==0x0000ff00 && gma==0x00ff0000 && bma==0xff000000 )
					bitmap->image.format=IMGFMT_B8G8R8A8;
				else if( rma==0xff000000 && gma==0x00ff0000 && bma==0x0000ff00 )
					bitmap->image.format=IMGFMT_R8G8B8A8;
			break;
		}

		if(glXMakeCurrent(dismod->theXDisplay, dismod->theXWindow, dismod->drawContext))
		{
			glGenTextures( 1, &bm->gltexname);
			glXMakeCurrent(dismod->theXDisplay, None, NULL);
			bm->ckeycol=-1;
			bm->alphaval=-1;
			return TTRUE;
		}
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

	if(glXMakeCurrent(dismod->theXDisplay, dismod->theXWindow, dismod->drawContext))
	{
		glDeleteTextures( 1, &bm->gltexname);
		glXMakeCurrent(dismod->theXDisplay, None, NULL);
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
		desc->width=dismod->width;
		desc->height=dismod->height;
		desc->depth=dismod->bufferdepth;
		desc->format=dismod->bufferformat;
		desc->bytesperrow=0;
	}
}

/**************************************************************************
	dismod_describe_bm
 **************************************************************************/
TMODAPI TVOID dismod_describe_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TDISDESCRIPTOR *desc)
{
	if(dismod->window_ready)
	{
		desc->x=0;
		desc->y=0;
		desc->width=bm->image.width;
		desc->height=bm->image.height;
		desc->depth=bm->image.depth;
		desc->format=bm->image.format;
		desc->bytesperrow=bm->image.bytesperrow;
	}
}

/**************************************************************************
	dismod_lock_dis
 **************************************************************************/
TMODAPI TBOOL dismod_lock_dis(TMOD_DISMOD *dismod, TIMGPICTURE *img)
{
	if(dismod->window_ready)
	{
		if(glXMakeCurrent(dismod->theXDisplay, dismod->theXWindow, dismod->lockContext))
		{
			img->bytesperrow=0;
			img->width=dismod->width;
			img->height=dismod->height;
			img->depth=dismod->bufferdepth;
			img->format=dismod->bufferformat;
			img->data=TNULL;
			return TTRUE;
		}
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

		img->bytesperrow=bm->image.bytesperrow;
		img->width=bm->image.width;
		img->height=bm->image.height;
		img->depth=bm->image.depth;
		img->format=bm->image.format;
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
	if(dismod->window_ready)
	{
		glFlush();
		glFinish();
		glXMakeCurrent(dismod->theXDisplay, None, NULL);
	}
}

/**************************************************************************
	dismod_unlock_bm
 **************************************************************************/
TMODAPI TVOID dismod_unlock_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
	XSync(dismod->theXDisplay,False);
	dismod->theBitmap->changed=TTRUE;
	dismod->theBitmap=TNULL;
}

/**************************************************************************
	dismod_begin_dis
 **************************************************************************/
TMODAPI TBOOL dismod_begin_dis(TMOD_DISMOD *dismod)
{
	if(dismod->window_ready)
	{
		if(glXMakeCurrent(dismod->theXDisplay, dismod->theXWindow, dismod->drawContext))
		{
			glDisable( GL_LIGHTING );
			glDisable( GL_DEPTH_TEST );
			glDisable( GL_CULL_FACE );
			glDisable( GL_ALPHA_TEST );
			glDisable( GL_BLEND );
			glDisable( GL_DITHER );
			glDisable( GL_POLYGON_STIPPLE);
			glDisable( GL_LINE_STIPPLE);
			glDisable( GL_POLYGON_SMOOTH);
			glDisable( GL_LINE_SMOOTH);

			if(dismod->dblbuf)
				glDrawBuffer(GL_BACK);
			else
				glDrawBuffer(GL_FRONT);

			glPointSize(1);
			glLineWidth(1);

			glViewport(0,0,(GLsizei)dismod->width,(GLsizei)dismod->height);

			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();
			gluOrtho2D(0,dismod->width,dismod->height,0);

			glMatrixMode( GL_MODELVIEW );
			glLoadIdentity();

			return TTRUE;
		}
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
	{
		glFlush();
		glFinish();
		glXMakeCurrent(dismod->theXDisplay, None, NULL);
	}
}

/**************************************************************************
	dismod_end_bm
 **************************************************************************/
TMODAPI TVOID dismod_end_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
	XSync(dismod->theXDisplay,False);
	dismod->theBitmap->changed=TTRUE;
	dismod->theBitmap=TNULL;
}

/**************************************************************************
	dismod_blit
 **************************************************************************/
TMODAPI TVOID dismod_blit(TMOD_DISMOD *dismod, TDISBITMAP *bm,TDBLITOPS *bops)
{
	if(dismod->window_ready)
	{
		offscreenBitmap *bitmap=(offscreenBitmap*)bm->hostdata;

		if(glXMakeCurrent(dismod->theXDisplay, dismod->theXWindow, dismod->drawContext))
		{
			GLfloat sx,sy,ex,ey;
			TIMGPICTURE bmimg, *bufimg=TNULL;

			glEnable(GL_TEXTURE_2D);
			glBindTexture( GL_TEXTURE_2D,bitmap->gltexname );

			/* allocate and convert the bitmap into a temporary bitmap */
			if(bitmap->changed)
			{
				bmimg.bytesperrow=bm->image.bytesperrow;
				bmimg.width=bm->image.width;
				bmimg.height=bm->image.height;
				bmimg.depth=bm->image.depth;
				bmimg.format=bm->image.format;
				bmimg.data=(TUINT8*)bitmap->theXImage->data;

				bufimg=TImgAllocBitmap(dismod->imgp,bitmap->w,bitmap->h,IMGFMT_A8R8G8B8);
				if(!bufimg)
				{
					glXMakeCurrent(dismod->theXDisplay, None, NULL);
					return;
				}

				TImgDoMethod(dismod->imgp,&bmimg,bufimg,IMGMT_CONVERT,TNULL);
			}

			if(bops->ckey)
			{
				TINT keycol=(bops->ckey_val.r << 16) | (bops->ckey_val.g << 8 ) | bops->ckey_val.b;

				if(bm->image.format==IMGFMT_R5G5B5)
					keycol &= 0x00f8f8f8;
				else if(bm->image.format==IMGFMT_R5G6B5)
					keycol &= 0x00f8fcf8;
					
				if(keycol != bitmap->ckeycol)
				{
					TINT x,y;
					TINT col,scol;
					TUINT *d;
					TUINT8 *data;

					/* allocate and convert the bitmap into a temporary bitmap */
					if(!bufimg)
					{
						bmimg.bytesperrow=bm->image.bytesperrow;
						bmimg.width=bm->image.width;
						bmimg.height=bm->image.height;
						bmimg.depth=bm->image.depth;
						bmimg.format=bm->image.format;
						bmimg.data=(TUINT8*)bitmap->theXImage->data;

						bufimg=TImgAllocBitmap(dismod->imgp,bitmap->w,bitmap->h,IMGFMT_A8R8G8B8);
						if(!bufimg)
						{
							glXMakeCurrent(dismod->theXDisplay, None, NULL);
							return;
						}

						TImgDoMethod(dismod->imgp,&bmimg,bufimg,IMGMT_CONVERT,TNULL);
					}

					data=(TUINT8*)bufimg->data;
					for(y=0;y<bm->image.height;y++)
					{
						d=(TUINT*)data;
						for(x=0;x<bm->image.width;x++)
						{
							scol=*d;
							col=scol & 0x00ffffff;
							if(col==keycol)
								*d=col | 0x01000000;
							else if((scol & 0xff000000)==0x01000000)
								*d=col | 0x02000000;

							d++;
						}
						data += bufimg->bytesperrow;
					}
					bitmap->ckeycol=keycol;
					bitmap->changed=TTRUE;
				}
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_NOTEQUAL,1.0f/255.0f);
			}

			if(bops->calpha)
			{
				if(bops->calpha_val != bitmap->alphaval)
				{
					TINT x,y;
					TUINT scol;
					TUINT *d;
					TUINT8 *data;
					TUINT a;

					/* allocate and convert the bitmap into a temporary bitmap */
					if(!bufimg)
					{
						bmimg.bytesperrow=bm->image.bytesperrow;
						bmimg.width=bm->image.width;
						bmimg.height=bm->image.height;
						bmimg.depth=bm->image.depth;
						bmimg.format=bm->image.format;
						bmimg.data=(TUINT8*)bitmap->theXImage->data;

						bufimg=TImgAllocBitmap(dismod->imgp,bitmap->w,bitmap->h,IMGFMT_A8R8G8B8);
						if(!bufimg)
						{
							glXMakeCurrent(dismod->theXDisplay, None, NULL);
							return;
						}

						TImgDoMethod(dismod->imgp,&bmimg,bufimg,IMGMT_CONVERT,TNULL);
					}

					data=(TUINT8*)bufimg->data;
					a=bops->calpha_val<<24;
					for(y=0;y<bm->image.height;y++)
					{
						d=(TUINT*)data;
						for(x=0;x<bm->image.width;x++)
						{
							scol=*d;
							if((scol & 0xff000000) != 0x01000000)
								*d=(scol & 0x00ffffff) | a;

							d++;

						}
						data += bufimg->bytesperrow;
					}
					bitmap->alphaval=bops->calpha_val;
					bitmap->changed=TTRUE;
				}
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			}

			if(bitmap->changed)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, bitmap->w, bitmap->h, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bufimg->data );
				TImgFreeBitmap(dismod->imgp,bufimg);
				bitmap->changed=TFALSE;
			}

			if(dismod->smoothscale)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			}

			sx = (GLfloat)bops->src.x / (GLfloat)bitmap->w;
			ex = (GLfloat)(bops->src.x + bops->src.width) / (GLfloat)bitmap->w;
			sy = (GLfloat)bops->src.y / (GLfloat)bitmap->h;
			ey = (GLfloat)(bops->src.y + bops->src.height) / (GLfloat)bitmap->h;

			glColor4f(1.0f,1.0f,1.0f,1.0f);

			glBegin(GL_QUADS);

			glTexCoord2f(sx,sy);
			glVertex2i(bops->dst.x,bops->dst.y);

			glTexCoord2f(ex,sy);
			glVertex2i(bops->dst.x+bops->dst.width,bops->dst.y);

			glTexCoord2f(ex,ey);
			glVertex2i(bops->dst.x+bops->dst.width,bops->dst.y+bops->dst.height);
			
			glTexCoord2f(sx,ey);
			glVertex2i(bops->dst.x,bops->dst.y+bops->dst.height);

			glEnd();

			glDisable(GL_TEXTURE_2D);
			glDisable(GL_ALPHA_TEST);
			glDisable(GL_BLEND);

			glXMakeCurrent(dismod->theXDisplay, None, NULL);
		}
	}
}

/**************************************************************************
	dismod_putimage_dis
 **************************************************************************/
TMODAPI TVOID dismod_putimage_dis(TMOD_DISMOD *dismod, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	if(dismod->window_ready)
	{
		Dismod_PutImage(dismod,TNULL,img,src,dst);
	}
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
	if(dismod->window_ready)
	{
		Dismod_PutImage(dismod,TNULL,img,src,dst);
	}
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
			dismod->mindepth=16;
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
	XVisualInfo *vi=NULL;
	TINT attrList[12];
	TINT r1,g1,b1,r2,g2,b2;
	Font id;
	TUINT first, last;

	if(dismod->defaultdepth>16)
	{
		r1=8;	g1=8;	b1=8;
		r2=5;	g2=6;	b2=5;
	}
	else
	{
		r1=5;	g1=6;	b1=5;
		r2=8;	g2=8;	b2=8;
	}

	attrList[0]=GLX_RGBA;

	attrList[1]=GLX_RED_SIZE;
	attrList[3]=GLX_GREEN_SIZE;
	attrList[5]=GLX_BLUE_SIZE;
	attrList[7]=GLX_DEPTH_SIZE;

	if(dismod->dblbuf)
	{
		attrList[9]=GLX_DOUBLEBUFFER;
		attrList[10]=None;
	}
	else
		attrList[9]=None;

	attrList[2]=r1;
	attrList[4]=g1;
	attrList[6]=b1;
	attrList[8]=24;
	vi = glXChooseVisual(dismod->theXDisplay, dismod->theXScreen, attrList);

	if(vi==NULL)
	{
		attrList[2]=r2;
		attrList[4]=g2;
		attrList[6]=b2;
		attrList[8]=24;
		vi = glXChooseVisual(dismod->theXDisplay, dismod->theXScreen, attrList);
	}

	if(vi==NULL)
	{
		attrList[2]=0;
		attrList[4]=0;
		attrList[6]=0;
		attrList[8]=16;
		vi = glXChooseVisual(dismod->theXDisplay, dismod->theXScreen, attrList);
	}

	if(vi==NULL)
		return TFALSE;

	glXGetConfig(dismod->theXDisplay, vi, GLX_RED_SIZE, &r1);
	glXGetConfig(dismod->theXDisplay, vi, GLX_GREEN_SIZE, &g1);
	glXGetConfig(dismod->theXDisplay, vi, GLX_BLUE_SIZE, &b1);

	dismod->theXColormap = XCreateColormap(dismod->theXDisplay,
											RootWindow(dismod->theXDisplay, vi->screen),
											vi->visual,
											AllocNone);

	attr.colormap = dismod->theXColormap;
	attr.border_pixel = 0;

	attr.event_mask = ExposureMask |
			  KeyPressMask | KeyReleaseMask |
			  ButtonPressMask | ButtonReleaseMask |
			  PointerMotionMask |
			  StructureNotifyMask | VisibilityChangeMask;

	dismod->theXWindow = XCreateWindow(dismod->theXDisplay,
										RootWindow(dismod->theXDisplay, dismod->theXScreen),
										dismod->window_xpos, dismod->window_ypos, dismod->width, dismod->height,
										0, vi->depth, InputOutput,
										vi->visual,
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

	/* create both contexts */
	dismod->lockContext = glXCreateContext(dismod->theXDisplay, vi, 0, GL_TRUE);
	dismod->drawContext = glXCreateContext(dismod->theXDisplay, vi, 0, GL_TRUE);

	/* attach the created context */
	if(!glXMakeCurrent(dismod->theXDisplay, dismod->theXWindow, dismod->drawContext))
		return TFALSE;

	/* try to get the glXSwapIntervalSGI extension for vsync */
	//glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB("glXSwapIntervalSGI");

	/* fill up data for bufferformat */
	if(r1==5 && g1==5 && b1==5)
	{
		dismod->bufferformat=IMGFMT_R5G5B5;
		dismod->bufferdepth=16;
	}
	if(r1==5 && g1==6 && b1==5)
	{
		dismod->bufferformat=IMGFMT_R5G6B5;
		dismod->bufferdepth=16;
	}
	else
	{
		dismod->bufferformat=IMGFMT_R8G8B8A8;
		dismod->bufferdepth=32;
	}

	/* load a font and create a font list for opengl textout */
	dismod->theXFont = XLoadQueryFont(dismod->theXDisplay, DEFAULT_X11_FONT_NAME);
	if(!dismod->theXFont)
		return TFALSE;

	dismod->font_w = XTextWidth(dismod->theXFont, " ", 1);
	dismod->font_h = dismod->theXFont->ascent + dismod->theXFont->descent;

	id = dismod->theXFont->fid;
	first = dismod->theXFont->min_char_or_byte2;
	last = dismod->theXFont->max_char_or_byte2;

	dismod->fontlist=glGenLists( last+1 );
	if(glGetError())
		return TFALSE;

	dismod->numsysglyphs=last-first+1;
	glXUseXFont(id, first, dismod->numsysglyphs, dismod->fontlist+first);
	glXMakeCurrent(dismod->theXDisplay, None, NULL);

	/* disable keyboard autorepeat */
	XAutoRepeatOff(dismod->theXDisplay);
	dismod->window_ready=TTRUE;
	return TTRUE;
}

/**************************************************************************
	PutImage
 **************************************************************************/
TVOID Dismod_PutImage(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	GLint format,type;
	GLfloat rbuf[256],gbuf[256], bbuf[256];
	GLfloat constantAlpha = 1.0;
	TINT i;

	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

	format=GL_BGRA;
	type=GL_UNSIGNED_INT_8_8_8_8;

	switch(img->format)
	{
		case IMGFMT_CLUT:
			for(i=0;i<256;i++)
			{
				rbuf[i]=(GLfloat)img->palette[i].r / 255.0f;
				gbuf[i]=(GLfloat)img->palette[i].g / 255.0f;
				bbuf[i]=(GLfloat)img->palette[i].b / 255.0f;
			}
			glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,rbuf);
			glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,gbuf);
			glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,bbuf);
			glPixelMapfv(GL_PIXEL_MAP_I_TO_A,1,&constantAlpha);

			glPixelTransferf(GL_ALPHA_SCALE, 0.0);
			glPixelTransferf(GL_ALPHA_BIAS,  1.0);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelTransferi(GL_INDEX_SHIFT, 0);
			glPixelTransferi(GL_INDEX_OFFSET, 0);
			glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

			format=GL_COLOR_INDEX;
			type=GL_UNSIGNED_BYTE;
		break;

		case IMGFMT_R5G5B5:
			glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

			format=GL_BGRA;
			type=GL_UNSIGNED_SHORT_1_5_5_5_REV;
		break;

		case IMGFMT_R5G6B5:
			glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

			format=GL_RGB;
			type=GL_UNSIGNED_SHORT_5_6_5;
		break;

		case IMGFMT_B8G8R8:
			glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

			format=GL_BGR_EXT;
			type=GL_UNSIGNED_BYTE;
		break;

		case IMGFMT_R8G8B8:
			glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

			format=GL_RGB;
			type=GL_UNSIGNED_BYTE;
		break;

		case IMGFMT_A8R8G8B8:
			glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

			format=GL_BGRA;
			type=GL_UNSIGNED_BYTE;
		break;

		case IMGFMT_R8G8B8A8:
			glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

			format=GL_RGBA;
			type=GL_UNSIGNED_INT_8_8_8_8;
		break;

		case IMGFMT_B8G8R8A8:
			glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

			format=GL_BGRA;
			type=GL_UNSIGNED_INT_8_8_8_8;
		break;
	}

	glRasterPos2i(dst->x,dst->y);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,src->y);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,img->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,src->x);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelZoom((GLfloat)dst->width/(GLfloat)src->width,-(GLfloat)dst->height/(GLfloat)src->height);
	glDrawPixels(src->width,src->height,format,type,img->data);
	glPixelZoom(1,1);
	glPopClientAttrib();
}

/**************************************************************************
  Display ProcessMessage
 **************************************************************************/
TVOID Dismod_ProcessMessage(TMOD_DISMOD *dismod,TDISMSG *dismsg)
{
	switch (dismsg->code)
	{
		case TDISMSG_RESIZE: case TDISMSG_MOVE:
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

