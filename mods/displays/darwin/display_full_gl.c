#include <tek/teklib.h>
#include <tek/exec.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/imgproc.h>
#include <tek/debug.h>

#include <tek/mod/imgproc.h>
#include <tek/mod/displayhandler.h>

#include <math.h>
#include <string.h>

#include <Carbon/Carbon.h>
#include <AGL/AGL.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/glu.h>

#define kEventClassTeklib   'tekl'
#define kEventTeklibRedraw	1

#define MOD_VERSION		0
#define MOD_REVISION	1

/* standard font used for textout */
#define DEFAULT_DARWIN_FONT_NAME "\pMonaco"
#define DEFAULT_DARWIN_FONT_SIZE 14

/* structure needed for allocpen */
typedef struct
{
	GLfloat r,g,b;
} GLPen;

typedef struct
{
	TINT	w,h;
	TINT	ckeycol;
	TINT	alphaval;
	TBOOL	changed;
	TUINT8* pixelBuffer;
	GLuint  gltexname;
} offmap;

typedef struct _TModDisplay
{
	TMODL	module;                                           /* module header */
	TAPTR	exec;
	TAPTR	util;
	TAPTR	imgp;
	
	TINT	minwidth,maxwidth,minheight,maxheight,mindepth,maxdepth;
	TINT	defaultwidth,defaultheight,defaultdepth;

	TUINT msgcode;
	TDISKEY key;
	TDISMOUSEPOS mmove;
	TDISMBUTTON mbutton;
	TDISRECT drect;
	
	TINT8*	windowname;
	TINT	width, height,bytesperrow;
	TBOOL	dblbuf,resize;
	TINT	window_xpos,window_ypos;
	TBOOL	window_ready;
	TINT	bufferdepth,bufferpixelsize;
	TUINT	bufferformat;

	TINT 	font_w,font_h;
	TINT	bmwidth, bmheight;

	TDISPEN*	theDrawPen;

	TINT		keyqual;

	TBOOL		hiddenCursor;

	/* AGL variables */
	AGLContext		gldisplayContext;
	AGLContext		offscreenContext;			/* used to render into an offscreen bitmap */
	GLPen			theGlDrawPen;
	GLuint			fontList;
	
	CFDictionaryRef currentMode;
	
	TINT			numdismodes;
	TDISMODE*		modelist;
	
	/* ----- */
	TBOOL	sysfontset;
	TINT	sysfontysize;

	TBOOL	cursorset;
	TUINT8* keytranstable;

	TINT    ptrmode;
	TBOOL	deltamouse;
	TBOOL	vsync,smoothscale;
	TBOOL   fullscreen;
} TMOD_DISPLAY;

/* module prototypes */
static TCALLBACK TMOD_DISPLAY *mod_open(TMOD_DISPLAY *mod, TAPTR selftask, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_DISPLAY *display, TAPTR selftask);

TMODAPI TBOOL display_create			(TMOD_DISPLAY *display,TSTRPTR title,TINT x, TINT y, TINT w, TINT h, TINT d, TUINT flags);
TMODAPI TVOID display_destroy			(TMOD_DISPLAY *display);
TMODAPI TVOID display_getproperties		(TMOD_DISPLAY *display, TDISPROPS *props);
TMODAPI TVOID display_getcaps			(TMOD_DISPLAY *display,TDISCAPS *caps);
TMODAPI TINT display_getmodelist		(TMOD_DISPLAY *display,TDISMODE **modelist);
TMODAPI TVOID display_waitmsg			(TMOD_DISPLAY *display);
TMODAPI TBOOL display_getmsg			(TMOD_DISPLAY *display,TDISMSG *dismsg);
TMODAPI TVOID display_setattrs			(TMOD_DISPLAY *display, TTAGITEM *tags);
TMODAPI TVOID display_setpointermode	(TMOD_DISPLAY *display,TINT mode);
TMODAPI TVOID display_setpointerpos		(TMOD_DISPLAY *display, TINT x,TINT y);
TMODAPI TVOID display_flush				(TMOD_DISPLAY *display);
TMODAPI TBOOL display_allocpen			(TMOD_DISPLAY *display,TDISPEN *pen);
TMODAPI TVOID display_freepen			(TMOD_DISPLAY *display,TDISPEN *pen);
TMODAPI TVOID display_setdpen			(TMOD_DISPLAY *display,TDISPEN *pen);
TMODAPI TBOOL display_setpalette		(TMOD_DISPLAY *display,TIMGARGBCOLOR *pal,TINT sp,TINT sd,TINT numentries);
TMODAPI TBOOL display_allocbitmap		(TMOD_DISPLAY *display,TDISBITMAP *bitmap,TINT width,TINT height,TINT flags);
TMODAPI TVOID display_freebitmap		(TMOD_DISPLAY *display,TDISBITMAP *bitmap);

TMODAPI TVOID display_describe_dis		(TMOD_DISPLAY *display,TDISDESCRIPTOR *desc);
TMODAPI TVOID display_describe_bm		(TMOD_DISPLAY *display,TDISBITMAP *bm,TDISDESCRIPTOR *desc);
TMODAPI TBOOL display_lock_dis			(TMOD_DISPLAY *display,TIMGPICTURE *img);
TMODAPI TBOOL display_lock_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm,TIMGPICTURE *img);
TMODAPI TVOID display_unlock_dis		(TMOD_DISPLAY *display);
TMODAPI TVOID display_unlock_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm);
TMODAPI TBOOL display_begin_dis			(TMOD_DISPLAY *display);
TMODAPI TBOOL display_begin_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm);
TMODAPI TVOID display_end_dis			(TMOD_DISPLAY *display);
TMODAPI TVOID display_end_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm);

TMODAPI TVOID display_textout_dis       (TMOD_DISPLAY *display,TINT8 *text,TINT row,TINT column);
TMODAPI TVOID display_textout_bm        (TMOD_DISPLAY *display,TDISBITMAP *bm,TINT8 *text,TINT row,TINT column);

TMODAPI TVOID display_blit				(TMOD_DISPLAY *display,TDISBITMAP *bm,TDBLITOPS *bops);
TMODAPI TVOID display_putimage_dis		(TMOD_DISPLAY *display,TIMGPICTURE *img,TDISRECT *src,TDISRECT *dst);
TMODAPI TVOID display_putimage_bm		(TMOD_DISPLAY *display,TDISBITMAP *bm,TIMGPICTURE *img,TDISRECT *src,TDISRECT *dst);
TMODAPI TVOID display_putscaleimage_dis	(TMOD_DISPLAY *display,TIMGPICTURE *img,TDISRECT *src,TDISRECT *dst);
TMODAPI TVOID display_putscaleimage_bm	(TMOD_DISPLAY *display,TDISBITMAP *bm,TIMGPICTURE *img,TDISRECT *src,TDISRECT *dst);

TMODAPI TVOID display_fill_dis			(TMOD_DISPLAY *display);
TMODAPI TVOID display_fill_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm);
TMODAPI TVOID display_plot_dis			(TMOD_DISPLAY *display,TINT x,TINT y);
TMODAPI TVOID display_plot_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT x,TINT y);
TMODAPI TVOID display_line_dis			(TMOD_DISPLAY *display,TINT sx,TINT sy,TINT dx,TINT dy);
TMODAPI TVOID display_line_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT sx,TINT sy,TINT dx,TINT dy);
TMODAPI TVOID display_box_dis			(TMOD_DISPLAY *display,TINT sx,TINT sy,TINT w,TINT h);
TMODAPI TVOID display_box_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT sx,TINT sy,TINT w,TINT h);
TMODAPI TVOID display_boxf_dis			(TMOD_DISPLAY *display,TINT sx,TINT sy,TINT w,TINT h);
TMODAPI TVOID display_boxf_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT sx,TINT sy,TINT w,TINT h);
TMODAPI TVOID display_poly_dis			(TMOD_DISPLAY *display,TINT numpoints,TINT *points);
TMODAPI TVOID display_poly_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT numpoints,TINT *points);
TMODAPI TVOID display_polyf_dis			(TMOD_DISPLAY *display,TINT numpoints,TINT *points);
TMODAPI TVOID display_polyf_bm			(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT numpoints,TINT *points);
TMODAPI TVOID display_ellipse_dis		(TMOD_DISPLAY *display,TINT x,TINT y,TINT rx,TINT ry);
TMODAPI TVOID display_ellipse_bm		(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT x,TINT y,TINT rx,TINT ry);
TMODAPI TVOID display_ellipsef_dis		(TMOD_DISPLAY *display,TINT x,TINT y,TINT rx,TINT ry);
TMODAPI TVOID display_ellipsef_bm		(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT x,TINT y,TINT rx,TINT ry);

/* private prototypes */
TBOOL Display_ReadProperties( TMOD_DISPLAY *display );
TVOID Display_ParseBufferFormat( TMOD_DISPLAY *display );
TVOID Display_MakeKeytransTable(TMOD_DISPLAY *display);
TVOID Display_PutImage( TMOD_DISPLAY* display, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst);
TVOID Display_SetStrokeFillColors( TMOD_DISPLAY *display, CGContextRef con );
TVOID Display_ConvertCoords( TMOD_DISPLAY *display, CGRect *srcRect, CGRect *dstRect );		// need to convert coordinates (different origins eg. upper-left/lower-left)
TVOID Display_ProcessMessage(TMOD_DISPLAY *display,TDISMSG *dismsg);

TVOID processEvents( EventRef theEvent, TAPTR mod );
TVOID processKeyboardEvents( EventRef theEvent, TAPTR mod );
TVOID processMouseEvents( EventRef theEvent, TAPTR mod );

pascal void Display_CreateUpdateEvent(EventLoopTimerRef theTimer, void* userData);
TVOID Display_InstallDrawTimer( TMOD_DISPLAY *display );
TVOID Display_DrawTimerProc ( EventLoopTimerRef inTimer, void *inUserData );

/**************************************************************************
	tek_init
 **************************************************************************/
TMODENTRY TUINT tek_init_display_full_gl(TAPTR selftask, TMOD_DISPLAY *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)					/* first call */
		{
			if (version <= MOD_VERSION)			/* version check */
			{
				return sizeof(TMOD_DISPLAY);	/* return module positive size */
			}
		}
		else									/* second call */
		{
			return sizeof(TAPTR) * 87;			/* return module negative size */
		}
	}
	else										/* third call */
	{
		mod->exec = TGetExecBase( mod );

		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;

		/* this module has instances. place instance
		** open/close functions into the module structure. */
		mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;

		/* put module vectors in front */
		((TAPTR *) mod)[-1 ] = (TAPTR) display_create;
		((TAPTR *) mod)[-2 ] = (TAPTR) display_destroy;
		((TAPTR *) mod)[-3 ] = (TAPTR) display_getproperties;
		((TAPTR *) mod)[-4 ] = (TAPTR) display_getmodelist;
		((TAPTR *) mod)[-5 ] = (TAPTR) display_waitmsg;
		((TAPTR *) mod)[-6 ] = (TAPTR) display_getmsg;

		((TAPTR *) mod)[-7 ] = (TAPTR) display_setattrs;
		((TAPTR *) mod)[-9 ] = (TAPTR) display_flush;
		((TAPTR *) mod)[-10] = (TAPTR) display_getcaps;

		((TAPTR *) mod)[-20] = (TAPTR) display_allocpen;
		((TAPTR *) mod)[-21] = (TAPTR) display_freepen;
		((TAPTR *) mod)[-22] = (TAPTR) display_setdpen;
		((TAPTR *) mod)[-23] = (TAPTR) display_setpalette;
		((TAPTR *) mod)[-24] = (TAPTR) display_allocbitmap;
		((TAPTR *) mod)[-25] = (TAPTR) display_freebitmap;

		((TAPTR *) mod)[-30] = (TAPTR) display_describe_dis;
		((TAPTR *) mod)[-31] = (TAPTR) display_describe_bm;
		((TAPTR *) mod)[-32] = (TAPTR) display_lock_dis;
		((TAPTR *) mod)[-33] = (TAPTR) display_lock_bm;
		((TAPTR *) mod)[-34] = (TAPTR) display_unlock_dis;
		((TAPTR *) mod)[-35] = (TAPTR) display_unlock_bm;
		((TAPTR *) mod)[-36] = (TAPTR) display_begin_dis;
		((TAPTR *) mod)[-37] = (TAPTR) display_begin_bm;
		((TAPTR *) mod)[-38] = (TAPTR) display_end_dis;
		((TAPTR *) mod)[-39] = (TAPTR) display_end_bm;

		((TAPTR *) mod)[-40] = (TAPTR) display_blit;

		((TAPTR *) mod)[-50] = (TAPTR) display_textout_dis;
		((TAPTR *) mod)[-51] = (TAPTR) display_textout_bm;

		((TAPTR *) mod)[-61] = (TAPTR) display_putimage_dis;
		((TAPTR *) mod)[-62] = (TAPTR) display_putimage_bm;
		((TAPTR *) mod)[-63] = (TAPTR) display_putscaleimage_dis;
		((TAPTR *) mod)[-64] = (TAPTR) display_putscaleimage_bm;

		((TAPTR *) mod)[-70] = (TAPTR) display_fill_dis;
		((TAPTR *) mod)[-71] = (TAPTR) display_fill_bm;
		((TAPTR *) mod)[-72] = (TAPTR) display_plot_dis;
		((TAPTR *) mod)[-73] = (TAPTR) display_plot_bm;
		((TAPTR *) mod)[-74] = (TAPTR) display_line_dis;
		((TAPTR *) mod)[-75] = (TAPTR) display_line_bm;
		((TAPTR *) mod)[-76] = (TAPTR) display_box_dis;
		((TAPTR *) mod)[-77] = (TAPTR) display_box_bm;
		((TAPTR *) mod)[-78] = (TAPTR) display_boxf_dis;
		((TAPTR *) mod)[-79] = (TAPTR) display_boxf_bm;
		((TAPTR *) mod)[-80] = (TAPTR) display_poly_dis;
		((TAPTR *) mod)[-81] = (TAPTR) display_poly_bm;
		((TAPTR *) mod)[-82] = (TAPTR) display_polyf_dis;
		((TAPTR *) mod)[-83] = (TAPTR) display_polyf_bm;
		((TAPTR *) mod)[-84] = (TAPTR) display_ellipse_dis;
		((TAPTR *) mod)[-85] = (TAPTR) display_ellipse_bm;
		((TAPTR *) mod)[-86] = (TAPTR) display_ellipsef_dis;
		((TAPTR *) mod)[-87] = (TAPTR) display_ellipsef_bm;

		return TTRUE;
	}

	return 0;
}

/**************************************************************************
	open instance
 **************************************************************************/
static TCALLBACK TMOD_DISPLAY *mod_open(TMOD_DISPLAY *display, TAPTR selftask, TTAGITEM *tags)
{
	display = TNewInstance(display, display->module.tmd_PosSize, display->module.tmd_NegSize);

	if (!display)
		return TNULL;

	display->util = TExecOpenModule(display->exec, "util", 0, TNULL);
	display->imgp = TExecOpenModule(display->exec, "imgproc", 0, TNULL);

	display->hiddenCursor = TFALSE;

	if(!Display_ReadProperties(display))
	{
		TExecCloseModule(display->exec, display->imgp);
		TExecCloseModule(display->exec, display->util);
		TFreeInstance(display);

		return TNULL;
	}

	display->window_ready=TFALSE;

	/* sauber wieder aufraeumen, wenn irgendwas schief lief */

	return display;
}

/**************************************************************************
	close instance
 **************************************************************************/
static TCALLBACK TVOID mod_close(TMOD_DISPLAY *display, TAPTR selftask)
{
	display_destroy(display);
	TExecCloseModule(display->exec, display->imgp);
	TExecCloseModule(display->exec, display->util);
	TFreeInstance(display);
}

/**************************************************************************
	display_create
		- create a standard window
		- ALWAYS doublebuffered to ensure clipping
 **************************************************************************/
TMODAPI TBOOL display_create(TMOD_DISPLAY *display, TSTRPTR title, TINT x, TINT y, TINT w, TINT h, TINT d, TUINT flags)
{
	boolean_t		modeMatch;
	CFDictionaryRef bestMode;
	AGLPixelFormat  pixelFormat;
	short			fontID = 0;
	GLint			attrList[13];
	GLint			swapInval = 1;
	TINT			r1,g1,b1,r2,g2,b2;
	
	InitCursor( );
	
	/* choose attributes and create AGLContext */

	if((flags & TDISCF_DOUBLEBUFFER)==TDISCF_DOUBLEBUFFER)
		display->dblbuf=TTRUE;
	else
		display->dblbuf=TFALSE;

	display->width  = w;
	display->height = h;
	
	if (display->defaultdepth > 16)
	{
		r1=8;	g1=8;	b1=8;
		r2=5;	g2=6;	b2=5;
	}
	else
	{
		r1=5;	g1=6;	b1=5;
		r2=8;	g2=8;	b2=8;
	}

	attrList[0] = AGL_FULLSCREEN;
	attrList[1] = AGL_RGBA;

	attrList[2] = AGL_RED_SIZE;
	attrList[4] = AGL_GREEN_SIZE;
	attrList[6] = AGL_BLUE_SIZE;
	attrList[8] = AGL_DEPTH_SIZE;

	if (display->dblbuf)
	{
		attrList[10] = AGL_DOUBLEBUFFER;
		attrList[11] = AGL_NONE;
	}
	else
		attrList[10] = AGL_NONE;

	attrList[3] = r1;
	attrList[5] = g1;
	attrList[7] = b1;
	attrList[9] = 24;
	
	pixelFormat = aglChoosePixelFormat (TNULL, 0, attrList);

	if (pixelFormat==NULL)
	{
		attrList[3] = r2;
		attrList[5] = g2;
		attrList[7] = b2;
		attrList[9] = 24;
		pixelFormat = aglChoosePixelFormat (TNULL, 0, attrList);
	}

	if(pixelFormat==NULL)
	{
		attrList[3]=0;
		attrList[5]=0;
		attrList[7]=0;
		attrList[9]=16;
		pixelFormat = aglChoosePixelFormat (TNULL, 0, attrList);
	}

	if(pixelFormat==NULL)
	{
		printf("no pixelformat found\n");
		return TFALSE;
	}

	display->gldisplayContext = aglCreateContext (pixelFormat, NULL);
	aglDestroyPixelFormat (pixelFormat);

	/* create offscreen context for bitmap rendering */
	attrList[0]=AGL_OFFSCREEN;
	attrList[2]=AGL_NONE;

	pixelFormat = aglChoosePixelFormat(NULL, 0, attrList);
	
	if(pixelFormat == NULL)
		return TFALSE;

	display->offscreenContext = aglCreateContext(pixelFormat, NULL);
	aglDestroyPixelFormat(pixelFormat);

	/* context creation done... setup screen and environment */
	
	if (!display->gldisplayContext || !display->offscreenContext)											// at last no context ->> give up and return
		return TFALSE;

	aglSetInteger (display->gldisplayContext, AGL_SWAP_INTERVAL, &swapInval);
	
	/* now checkout the actual configuration and set the displaybuffer setting according to */
	
	aglGetInteger (display->gldisplayContext, AGL_RED_SIZE, (GLint*) &r1);
	aglGetInteger (display->gldisplayContext, AGL_GREEN_SIZE, (GLint*) &g1);
	aglGetInteger (display->gldisplayContext, AGL_BLUE_SIZE, (GLint*) &b1);


	if(r1==5 && g1==5 && b1==5)
	{
		display->bufferformat=IMGFMT_R5G5B5;
		display->bufferdepth=16;
	}
	if(r1==5 && g1==6 && b1==5)
	{
		display->bufferformat=IMGFMT_R5G6B5;
		display->bufferdepth=16;
	}
	else
	{
		display->bufferformat=IMGFMT_R8G8B8A8;
		display->bufferdepth=32;
	}

	/* setup font */
	
	GetFNum (DEFAULT_DARWIN_FONT_NAME,  &fontID);

	display->fontList = glGenLists(256);
	aglUseFont( display->gldisplayContext, fontID, normal, DEFAULT_DARWIN_FONT_SIZE, 0, 256, (long) display->fontList);
	aglUseFont( display->offscreenContext, fontID, normal, DEFAULT_DARWIN_FONT_SIZE, 0, 256, (long) display->fontList);

	display->font_w = DEFAULT_DARWIN_FONT_SIZE;
	display->font_h = DEFAULT_DARWIN_FONT_SIZE;
	
	/* save current mode and switch to fullscreen */
	
	display->currentMode = CGDisplayCurrentMode(kCGDirectMainDisplay);
			 
	bestMode = CGDisplayBestModeForParameters(kCGDirectMainDisplay, display->bufferdepth, display->width, display->height, &modeMatch);

	if (! modeMatch)
	{
		tdbprintf(10, "could not find appropriate display\n");
		return TFALSE;
	}

	CGCaptureAllDisplays( );
		
	CGDisplaySwitchToMode(kCGDirectMainDisplay, bestMode);
	aglSetFullScreen(display->gldisplayContext, display->width, display->height, 100, 0);		// last two are hertz and output-device

	Display_MakeKeytransTable(display);
	
	display->window_ready = TTRUE;
	display->ptrmode      = TDISPTR_NORMAL;
	display->deltamouse	  = TFALSE;
	display->vsync		  = TFALSE;
	display->smoothscale  = TFALSE;

	return TTRUE;
}


/**************************************************************************
	display_destroy
 **************************************************************************/
TMODAPI TVOID display_destroy(TMOD_DISPLAY *display)
{
	if (display->window_ready)
	{
		TExecFree(display->exec, display->modelist);
		TExecFree(display->exec, display->keytranstable);

		aglSetCurrentContext(NULL);
		
		if (display->gldisplayContext)
			aglDestroyContext(display->gldisplayContext);
		
		if (display->offscreenContext)
			aglDestroyContext(display->offscreenContext);
		
		CGDisplaySwitchToMode(kCGDirectMainDisplay, display->currentMode);
		
		CGReleaseAllDisplays( );
	}
}

/**************************************************************************
	display_getproperties
 **************************************************************************/
TMODAPI TVOID display_getproperties(TMOD_DISPLAY *display, TDISPROPS *props)
{
	props->version=DISPLAYHANDLER_VERSION;
	props->priority=0;
	props->dispclass=TDISCLASS_OPENGL;
	props->dispmode=TDISMODE_FULLSCREEN;
	props->minwidth=display->minwidth;
	props->maxwidth=display->maxwidth;
	props->minheight=display->minheight;
	props->maxheight=display->maxheight;
	props->mindepth=display->mindepth;
	props->maxdepth=display->maxdepth;
	props->defaultwidth=display->defaultwidth;
	props->defaultheight=display->defaultheight;
	props->defaultdepth=display->defaultdepth;
}

/**************************************************************************
	display_getcaps
 **************************************************************************/
TMODAPI TVOID display_getcaps(TMOD_DISPLAY *display, TDISCAPS *caps)
{
	caps->minbmwidth=16;
	caps->minbmheight=16;
	caps->maxbmwidth=32768;
	caps->maxbmheight=32768;
	caps->blitscale=TFALSE;
	caps->blitalpha=TFALSE;
	caps->blitckey=TFALSE;

	caps->canconvertdisplay=TTRUE;
	caps->canconvertscaledisplay=TTRUE;
	caps->canconvertbitmap=TFALSE;
	caps->canconvertscalebitmap=TFALSE;
	caps->candrawbitmap=TFALSE;
}

/**************************************************************************
	display_getmodelist
 **************************************************************************/
TMODAPI TINT display_getmodelist(TMOD_DISPLAY *display, TDISMODE **modelist)
{
	*modelist=display->modelist;
	return display->numdismodes;
}

/**************************************************************************
	display_waitmessage
 **************************************************************************/
TMODAPI TVOID display_waitmsg(TMOD_DISPLAY *display)
{
	EventRef		theEvent;
#if 0
	EventTypeSpec   eventTypes[] =					/* this is maybe useful in later versions.... we will see */
	{
		{kEventClassTeklib, kEventTeklibRedraw},
					
		{kEventClassApplication, kEventAppQuit},
					
		{kEventClassMouse, kEventMouseDown},
		{kEventClassMouse, kEventMouseUp},
		{kEventClassMouse, kEventMouseMoved},
		{kEventClassMouse, kEventMouseDragged},
		{kEventClassMouse, kEventMouseWheelMoved},
		{kEventClassMouse, 23},				// kEventControlTrackingAreaEnter
		{kEventClassMouse, 24},				// kEventControlTrackingAreaLeave

		{kEventClassKeyboard, kEventRawKeyDown},
		{kEventClassKeyboard, kEventRawKeyUp},
	};
#endif

	if (display->window_ready)
	{
		ReceiveNextEvent(0, TNULL, kEventDurationForever, TTRUE, &theEvent);
		display->msgcode=0;
		processEvents( theEvent, display );
	}
}

/**************************************************************************
	display_getmsg
 **************************************************************************/
TMODAPI TBOOL display_getmsg(TMOD_DISPLAY *display,TDISMSG *dismsg)
{
	EventRef		theEvent;

	if(display->msgcode)
	{
		dismsg->code=display->msgcode;
		Display_ProcessMessage(display,dismsg);
		display->msgcode=0;
		return TTRUE;
	}
	
	while( ReceiveNextEvent(0, TNULL,kEventDurationNoWait,TTRUE, &theEvent) == noErr )
	{
		processEvents( theEvent, display );

		if(display->msgcode)
		{
			dismsg->code=display->msgcode;
			Display_ProcessMessage(display,dismsg);
			display->msgcode=0;
			return TTRUE;
		}
	}

	return TFALSE;
}

/**************************************************************************
	display_setpointermode
 **************************************************************************/
TMODAPI TVOID display_setpointermode(TMOD_DISPLAY *display, TINT mode)
{
	switch(mode)
	{
		case TDISPTR_NORMAL:
			if ( display->hiddenCursor )
				ShowCursor( );
			
			QDDisplayWaitCursor( FALSE );
			display->hiddenCursor = TFALSE;
		break;

		case TDISPTR_BUSY:
			if ( display->hiddenCursor )
				ShowCursor( );
			
			QDDisplayWaitCursor( TRUE );
			display->hiddenCursor = TFALSE;
		break;

		case TDISPTR_INVISIBLE:
			if(!display->hiddenCursor)
			{
				HideCursor( );
				display->hiddenCursor = TTRUE;
			}
		break;
	}
}

/**************************************************************************
	display_setpointerpos
 **************************************************************************/
TMODAPI TVOID display_setpointerpos(TMOD_DISPLAY *display, TINT x, TINT y)
{
	CGPoint point = { x, y };
	CGDisplayMoveCursorToPoint( kCGDirectMainDisplay, point );
}

/**************************************************************************
	display_setattrs
 **************************************************************************/
TMODAPI TVOID display_setattrs(TMOD_DISPLAY *display, TTAGITEM *tags)
{
	display->ptrmode     = (TINT)TGetTag(tags,TDISTAG_POINTERMODE,(TTAG)TDISPTR_NORMAL);
	display->deltamouse	 = (TINT)TGetTag(tags,TDISTAG_DELTAMOUSE,(TTAG)TFALSE);
	display->vsync		 = (TINT)TGetTag(tags,TDISTAG_VSYNCHINT,(TTAG)TFALSE);
	display->smoothscale = (TINT)TGetTag(tags,TDISTAG_SMOOTHHINT,(TTAG)TFALSE);

	switch(display->ptrmode)
	{
		case TDISPTR_NORMAL:
			if ( display->hiddenCursor )
				ShowCursor( );
			
			QDDisplayWaitCursor( FALSE );
			display->hiddenCursor = TFALSE;
			break;

		case TDISPTR_BUSY:
			if ( display->hiddenCursor )
				ShowCursor( );
			
			QDDisplayWaitCursor( TRUE );
			display->hiddenCursor = TFALSE;
			break;

		case TDISPTR_INVISIBLE:
			if(!display->hiddenCursor)
			{
				HideCursor( );
				display->hiddenCursor = TTRUE;
			}
			break;
	}
}

/**************************************************************************
	display_flush
 **************************************************************************/
TMODAPI TVOID display_flush(TMOD_DISPLAY *display)
{
	if(display->window_ready)
		aglSwapBuffers(display->gldisplayContext);
}

/**************************************************************************
	display_allocpen
 **************************************************************************/
TMODAPI TBOOL display_allocpen(TMOD_DISPLAY *display, TDISPEN *pen)
{
	/* do nothing. all requiered data is arranged in the displayhandler function */
	return TTRUE;
}

/**************************************************************************
	display_freepen
 **************************************************************************/
TMODAPI TVOID display_freepen(TMOD_DISPLAY *display, TDISPEN *pen)
{
	/* do nothing. all requiered data was arranged in the displayhandler */
}

/**************************************************************************
	display_setdpen
 **************************************************************************/
TMODAPI TVOID display_setdpen(TMOD_DISPLAY *display, TDISPEN *pen)
{
	display->theGlDrawPen.r=(GLfloat)pen->color.r / 255.0f;
	display->theGlDrawPen.g=(GLfloat)pen->color.g / 255.0f;
	display->theGlDrawPen.b=(GLfloat)pen->color.b / 255.0f;
}

/**************************************************************************
	display_setpalette
 **************************************************************************/
TMODAPI TBOOL display_setpalette(TMOD_DISPLAY *display, TIMGARGBCOLOR *pal, TINT sp, TINT sd, TINT numentries)
{
	/* its a no-op in windowed environments! */
	return TFALSE;
}

/**************************************************************************
	display_allocbitmap
 **************************************************************************/
TMODAPI TBOOL display_allocbitmap(TMOD_DISPLAY *display, TDISBITMAP *bitmap, TINT width, TINT height, TINT flags)
{
	if (display->window_ready)
	{
		offmap* bm = TNULL;
		
		bitmap->image.width			= width;
		bitmap->image.height		= height;
		bitmap->image.bytesperrow   = width * display->bufferdepth;
		bitmap->image.depth			= display->bufferdepth;
		bitmap->image.format		= display->bufferformat;
		
		/* plain pixelbuffer to assign with aglSetOffscreen() when locking() or beginning() */
		bitmap->hostdata			= TExecAlloc0(display->exec, TNULL, sizeof(offmap));
		
		bm = (offmap*)bitmap->hostdata;
		
		bm->pixelBuffer	= TExecAlloc0(display->exec, TNULL, sizeof(TUINT8) * width * height * display->bufferdepth);

		bm->w = (TINT) pow(2, (TINT)(log(width) / log(2.0) ));
		
		if(bm->w < width)
			bm->w += bm->w;

		bm->h = (TINT) pow(2, (TINT)(log(height) / log(2.0) ));
		
		if(bm->h < height)
			bm->h += bm->h;
		
		if(aglSetCurrentContext(display->gldisplayContext))
		{
			glGenTextures(1, &bm->gltexname);
			aglSetCurrentContext(NULL);
			
			bm->changed = TTRUE;
			
			return TTRUE;
		}
	}

	return TFALSE;
}

/**************************************************************************
	display_freebitmap
 **************************************************************************/
TMODAPI TVOID display_freebitmap(TMOD_DISPLAY *display, TDISBITMAP *bitmap)
{
	if (display->window_ready)
	{
		if(bitmap->hostdata)
		{
			if(aglSetCurrentContext(display->gldisplayContext))
			{
				glDeleteTextures( 1, &((offmap*)bitmap->hostdata)->gltexname);
				aglSetCurrentContext(NULL);
			}

			TExecFree(display->exec, ((offmap*)bitmap->hostdata)->pixelBuffer);
			TExecFree(display->exec, bitmap->hostdata);
		}
	}
}

/**************************************************************************
	display_describe_dis
 **************************************************************************/
TMODAPI TVOID display_describe_dis(TMOD_DISPLAY *display, TDISDESCRIPTOR *desc)
{
	desc->x				= display->window_xpos;
	desc->y				= display->window_ypos;
	desc->width			= display->width;
	desc->height		= display->height;
	desc->depth			= display->bufferdepth;
	desc->format		= display->bufferformat;
	desc->bytesperrow   = display->bytesperrow;
}

/**************************************************************************
	display_describe_bm
 **************************************************************************/
TMODAPI TVOID display_describe_bm(TMOD_DISPLAY *display, TDISBITMAP *bm, TDISDESCRIPTOR *desc)
{
	desc->x				= 0;
	desc->y				= 0;
	desc->width			= bm->image.width;
	desc->height		= bm->image.height;
	desc->depth			= bm->image.depth;
	desc->format		= bm->image.format;
	desc->bytesperrow   = bm->image.bytesperrow;
}

/**************************************************************************
	display_lock_dis
	
	set pointer in <img> that all actions going to screenbuffer
	
 **************************************************************************/
TMODAPI TBOOL display_lock_dis(TMOD_DISPLAY *display, TIMGPICTURE *img)
{
	if(display->window_ready)
	{
		if (aglSetCurrentContext(display->gldisplayContext))
		{
			img->bytesperrow=0;
			img->width = display->width;
			img->height = display->height;
			img->depth = display->bufferdepth;
			img->format = display->bufferformat;
			img->data=TNULL;

			return TTRUE;
		}
	}
	
	return TFALSE;
}

/**************************************************************************
	display_lock_bm
	
	fill <img> structure with data according to <bm> to enable direct drawing into the pixelbuffer
	of <bm>. remember that <bm->hostdata> is a bitmap context.
	
 **************************************************************************/
TMODAPI TBOOL display_lock_bm(TMOD_DISPLAY *display, TDISBITMAP *bm, TIMGPICTURE *img)
{
	if(display->window_ready)
	{
		img->bytesperrow = bm->image.bytesperrow;
		img->width		 = bm->image.width;
		img->height		 = bm->image.height;
		img->depth		 = bm->image.depth;
		img->format		 = bm->image.format;
		img->data		 = ((offmap*)bm->hostdata)->pixelBuffer;
			return TTRUE;
	}

	return TFALSE;
}

/**************************************************************************
	display_unlock_dis
 **************************************************************************/
TMODAPI TVOID display_unlock_dis(TMOD_DISPLAY *display)
{
	aglSetCurrentContext(NULL);
}

/**************************************************************************
	display_unlock_bm
 **************************************************************************/
TMODAPI TVOID display_unlock_bm(TMOD_DISPLAY *display, TDISBITMAP *bm)
{
	((offmap*)bm->hostdata)->changed = TTRUE;
}

/**************************************************************************
	display_begin_dis
	
	do everything necessary to draw into the display context. such as assigning theDrawContext
	
 **************************************************************************/
TMODAPI TBOOL display_begin_dis(TMOD_DISPLAY *display)
{
	if(display->window_ready)
	{
		if(aglSetCurrentContext(display->gldisplayContext))
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

			if(display->dblbuf)
				glDrawBuffer(GL_BACK);
			else
				glDrawBuffer(GL_FRONT);

			glPointSize(1);
			glLineWidth(1);

			glViewport(0,0,(GLsizei)display->width,(GLsizei)display->height);

			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();
			gluOrtho2D(0,display->width,display->height,0);

			glMatrixMode( GL_MODELVIEW );
			glLoadIdentity();

			return TTRUE;
		}
	}
	return TFALSE;
}

/**************************************************************************
	display_begin_bm
 **************************************************************************/
TMODAPI TBOOL display_begin_bm(TMOD_DISPLAY *display, TDISBITMAP *bm)
{
	if(display->window_ready)
	{
		if (!bm->hostdata)
			return TFALSE;
		
		if (aglSetOffScreen(display->offscreenContext, bm->image.width, bm->image.height, bm->image.bytesperrow, ((offmap*)bm->hostdata)->pixelBuffer) == GL_FALSE)
			return TFALSE;
		
		if(aglSetCurrentContext(display->offscreenContext))
		{
			glEnable( GL_TEXTURE_2D);

			glPointSize(1);
			glLineWidth(1);

			glViewport(0,0,(GLsizei)bm->image.width,(GLsizei)bm->image.height);

			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();
			gluOrtho2D(0,bm->image.width,bm->image.height,0);

			glMatrixMode( GL_MODELVIEW );
			glLoadIdentity();

			((offmap*)bm->hostdata)->changed = TTRUE;
			
			return TTRUE;
		}
	}

	return TFALSE;
}

/**************************************************************************
	display_end_dis
 **************************************************************************/
TMODAPI TVOID display_end_dis(TMOD_DISPLAY *display)
{
	if(display->window_ready)
	{
		glFlush();
		glFinish();
		aglSetCurrentContext(NULL);
	}
}

/**************************************************************************
	display_end_bm
 **************************************************************************/
TMODAPI TVOID display_end_bm(TMOD_DISPLAY *display, TDISBITMAP *bm)
{
	if(display->window_ready)
	{
		glFlush();
		glFinish();
		aglSetDrawable(display->offscreenContext, NULL);
		aglSetCurrentContext(NULL);

		((offmap*)bm->hostdata)->changed = TTRUE;
	}
}

/**************************************************************************
	display textout
 **************************************************************************/
TMODAPI TVOID display_textout_dis(TMOD_DISPLAY *display, TINT8 *text, TINT row, TINT column)
{
	glListBase(display->fontList);
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glPushAttrib(GL_LIST_BIT);
	glRasterPos2d(column*display->font_w,(row+1)*display->font_h);
	
	
	glCallLists( TUtilStrLen(display->util,text), GL_UNSIGNED_BYTE,(GLubyte *)text);
	glPopAttrib();
}

TMODAPI TVOID display_textout_bm(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT8 *text,TINT row,TINT column)
{
	glListBase(display->fontList);
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glPushAttrib(GL_LIST_BIT);
	glRasterPos2d(column*display->font_w,(row+1)*display->font_h);
	
	glCallLists( TUtilStrLen(display->util,text), GL_UNSIGNED_BYTE,(GLubyte *)text);
	glPopAttrib();
}


/**************************************************************************
	display_blit
 **************************************************************************/
TMODAPI TVOID display_blit(TMOD_DISPLAY *display, TDISBITMAP *bm,TDBLITOPS *bops)
{
	if(display->window_ready)
	{
		offmap *bitmap=(offmap*)bm->hostdata;

		if(aglSetCurrentContext(display->gldisplayContext))
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
				bmimg.data=(TUINT8*)bitmap->pixelBuffer;
				
				bufimg=TImgAllocBitmap(display->imgp,bitmap->w,bitmap->h,IMGFMT_A8R8G8B8);
				if(!bufimg)
				{
					aglSetCurrentContext(NULL);
					return;
				}

				TImgDoMethod(display->imgp,&bmimg,bufimg,IMGMT_CONVERT,TNULL);
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
						bmimg.data=(TUINT8*)bitmap->pixelBuffer;

						bufimg=TImgAllocBitmap(display->imgp,bitmap->w,bitmap->h,IMGFMT_A8R8G8B8);
						if(!bufimg)
						{
							aglSetCurrentContext(NULL);
							return;
						}

						TImgDoMethod(display->imgp,&bmimg,bufimg,IMGMT_CONVERT,TNULL);
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
						bmimg.data=(TUINT8*)bitmap->pixelBuffer;

						bufimg=TImgAllocBitmap(display->imgp,bitmap->w,bitmap->h,IMGFMT_A8R8G8B8);
						if(!bufimg)
						{
							aglSetCurrentContext(NULL);
							return;
						}

						TImgDoMethod(display->imgp,&bmimg,bufimg,IMGMT_CONVERT,TNULL);
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
				TImgFreeBitmap(display->imgp,bufimg);
				bitmap->changed=TFALSE;
			}

			if(display->smoothscale)
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
			glVertex2i(bops->dst.x+bops->dst.width,bops->dst.y+bm->image.height);

			glTexCoord2f(sx,ey);
			glVertex2i(bops->dst.x,bops->dst.y+bm->image.height);

			glEnd();

			glDisable(GL_TEXTURE_2D);
			glDisable(GL_ALPHA_TEST);
			glDisable(GL_BLEND);

			aglSetCurrentContext(NULL);
		}
	}
}

/**************************************************************************
	display_putimage_dis
 **************************************************************************/
TMODAPI TVOID display_putimage_dis(TMOD_DISPLAY *display, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	if ( display->window_ready )
	{
		Display_PutImage( display, TNULL, img, src, dst );
	}
}

/**************************************************************************
	display_putimage_bm
 **************************************************************************/
TMODAPI TVOID display_putimage_bm(TMOD_DISPLAY *display, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
}

/**************************************************************************
	display_putscaleimage_dis
 **************************************************************************/
TMODAPI TVOID display_putscaleimage_dis(TMOD_DISPLAY *display, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	if ( display->window_ready )
	{
		Display_PutImage( display, TNULL, img, src, dst );
	}
}

/**************************************************************************
	display_putscaleimage_bm
 **************************************************************************/
TMODAPI TVOID display_putscaleimage_bm(TMOD_DISPLAY *display, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
}

/**************************************************************************
	display_fill_dis
 **************************************************************************/
TMODAPI TVOID display_fill_dis(TMOD_DISPLAY *display)
{
	glClearColor(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b,1);
	glClear(GL_COLOR_BUFFER_BIT);
}

/**************************************************************************
	display_fill_bm
 **************************************************************************/
TMODAPI TVOID display_fill_bm(TMOD_DISPLAY *display, TDISBITMAP *bm)
{
	glClearColor(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b,1);
	glClear(GL_COLOR_BUFFER_BIT);
}

/**************************************************************************
	display_plot_dis
 **************************************************************************/
TMODAPI TVOID display_plot_dis(TMOD_DISPLAY *display,TINT x, TINT y)
{
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_POINTS);
	glVertex2i(x,y);
	glEnd();
}

/**************************************************************************
	display_plot_bm
 **************************************************************************/
TMODAPI TVOID display_plot_bm(TMOD_DISPLAY *display, TDISBITMAP *bm,TINT x, TINT y)
{
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_POINTS);
	glVertex2i(x,y);
	glEnd();
}

/**************************************************************************
	display_line_dis
 **************************************************************************/
TMODAPI TVOID display_line_dis(TMOD_DISPLAY *display,TINT sx, TINT sy, TINT dx, TINT dy)
{
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_LINES);
	glVertex2i(sx,sy);
	glVertex2i(dx,dy);
	glEnd();
}

/**************************************************************************
	display_line_bm
 **************************************************************************/
TMODAPI TVOID display_line_bm(TMOD_DISPLAY *display, TDISBITMAP *bm,TINT sx, TINT sy, TINT dx, TINT dy)
{
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_LINES);
	glVertex2i(sx,sy);
	glVertex2i(dx,dy);
	glEnd();
}

/**************************************************************************
	display_box_dis
 **************************************************************************/
TMODAPI TVOID display_box_dis(TMOD_DISPLAY *display,TINT sx, TINT sy, TINT w, TINT h)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_QUADS);
	glVertex2i(sx,sy);
	glVertex2i(sx+w,sy);
	glVertex2i(sx+w,sy+h);
	glVertex2i(sx,sy+h);
	glEnd();
}

/**************************************************************************
	display_box_bm
 **************************************************************************/
TMODAPI TVOID display_box_bm(TMOD_DISPLAY *display, TDISBITMAP *bm, TINT sx, TINT sy, TINT w, TINT h)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_QUADS);
	glVertex2i(sx,sy);
	glVertex2i(sx+w,sy);
	glVertex2i(sx+w,sy+h);
	glVertex2i(sx,sy+h);
	glEnd();
}

/**************************************************************************
	display_boxf_dis
 **************************************************************************/
TMODAPI TVOID display_boxf_dis(TMOD_DISPLAY *display,TINT sx, TINT sy, TINT w, TINT h)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_QUADS);
	glVertex2i(sx,sy);
	glVertex2i(sx+w,sy);
	glVertex2i(sx+w,sy+h);
	glVertex2i(sx,sy+h);
	glEnd();
}

/**************************************************************************
	display_boxf_bm
 **************************************************************************/
TMODAPI TVOID display_boxf_bm(TMOD_DISPLAY *display, TDISBITMAP *bm, TINT sx, TINT sy, TINT w, TINT h)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_QUADS);
	glVertex2i(sx,sy);
	glVertex2i(sx+w,sy);
	glVertex2i(sx+w,sy+h);
	glVertex2i(sx,sy+h);
	glEnd();
}

/**************************************************************************
	display_poly_dis
 **************************************************************************/
TMODAPI TVOID display_poly_dis(TMOD_DISPLAY *display,TINT numpoints,TINT *points)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2,GL_INT,0,points);
	glDrawArrays(GL_POLYGON,0,numpoints);
	glDisableClientState(GL_VERTEX_ARRAY);
}

/**************************************************************************
	display_poly_bm
 **************************************************************************/
TMODAPI TVOID display_poly_bm(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT numpoints,TINT *points)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2,GL_INT,0,points);
	glDrawArrays(GL_POLYGON,0,numpoints);
	glDisableClientState(GL_VERTEX_ARRAY);
}

/**************************************************************************
	display_polyf_dis
 **************************************************************************/
TMODAPI TVOID display_polyf_dis(TMOD_DISPLAY *display,TINT numpoints,TINT *points)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2,GL_INT,0,points);
	glDrawArrays(GL_POLYGON,0,numpoints);
	glDisableClientState(GL_VERTEX_ARRAY);
}

/**************************************************************************
	display_polyf_bm
 **************************************************************************/
TMODAPI TVOID display_polyf_bm(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT numpoints,TINT *points)
{
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2,GL_INT,0,points);
	glDrawArrays(GL_POLYGON,0,numpoints);
	glDisableClientState(GL_VERTEX_ARRAY);
}

/**************************************************************************
	display_ellipse_dis
 **************************************************************************/
TMODAPI TVOID display_ellipse_dis(TMOD_DISPLAY *display,TINT x,TINT y, TINT rx, TINT ry)
{
	TINT ex,ey,oldx;
	TFLOAT r1q,r2q;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	oldx=rx;

	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_LINES);

	for(ey=0;ey<=ry;ey++)
	{
		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));

		if(ex<oldx-1)
		{
			glEnd();
			glBegin(GL_LINES);
			glVertex2i(x-ex,  y-ey);
			glVertex2i(x-oldx,y-ey);
			glVertex2i(x+ex,  y-ey);
			glVertex2i(x+oldx,y-ey);
			glVertex2i(x-ex,  y+ey);
			glVertex2i(x-oldx,y+ey);
			glVertex2i(x+ex,  y+ey);
			glVertex2i(x+oldx,y+ey);
		}
		else
		{
			glEnd();
			glBegin(GL_POINTS);
			glVertex2i(x-ex,y-ey);
			glVertex2i(x+ex,y-ey);
			glVertex2i(x-ex,y+ey);
			glVertex2i(x+ex,y+ey);
		}
		oldx=ex;
	}
	glEnd();
}

/**************************************************************************
	display_ellipse_bm
 **************************************************************************/
TMODAPI TVOID display_ellipse_bm(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT x,TINT y, TINT rx, TINT ry)
{
	TINT ex,ey,oldx;
	TFLOAT r1q,r2q;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	oldx=rx;

	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_LINES);

	for(ey=0;ey<=ry;ey++)
	{
		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));

		if(ex<oldx-1)
		{
			glEnd();
			glBegin(GL_LINES);
			glVertex2i(x-ex,  y-ey);
			glVertex2i(x-oldx,y-ey);
			glVertex2i(x+ex,  y-ey);
			glVertex2i(x+oldx,y-ey);
			glVertex2i(x-ex,  y+ey);
			glVertex2i(x-oldx,y+ey);
			glVertex2i(x+ex,  y+ey);
			glVertex2i(x+oldx,y+ey);
		}
		else
		{
			glEnd();
			glBegin(GL_POINTS);
			glVertex2i(x-ex,y-ey);
			glVertex2i(x+ex,y-ey);
			glVertex2i(x-ex,y+ey);
			glVertex2i(x+ex,y+ey);
		}
		oldx=ex;
	}
	glEnd();
}

/**************************************************************************
	display_ellipsef_dis
 **************************************************************************/
TMODAPI TVOID display_ellipsef_dis(TMOD_DISPLAY *display,TINT x,TINT y, TINT rx, TINT ry)
{
	TINT ex,ey;
	TFLOAT r1q,r2q;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_LINES);

	glVertex2i(x-rx,y);
	glVertex2i(x+rx,y);

	for(ey=1;ey<=ry;ey++)
	{
		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));

		glVertex2i(x-ex,y-ey);
		glVertex2i(x+ex,y-ey);

		glVertex2i(x-ex,y+ey);
		glVertex2i(x+ex,y+ey);
	}
	glEnd();
}

/**************************************************************************
	display_ellipsef_bm
 **************************************************************************/
TMODAPI TVOID display_ellipsef_bm(TMOD_DISPLAY *display,TDISBITMAP *bm,TINT x,TINT y, TINT rx, TINT ry)
{
	TINT ex,ey;
	TFLOAT r1q,r2q;

	r1q=(TFLOAT)rx*rx;
	r2q=r1q/((TFLOAT)ry*ry);

	glColor3f(display->theGlDrawPen.r,display->theGlDrawPen.g,display->theGlDrawPen.b);
	glBegin(GL_LINES);

	glVertex2i(x-rx,y);
	glVertex2i(x+rx,y);

	for(ey=1;ey<=ry;ey++)
	{
		ex=(TINT)sqrt((TFLOAT)(r1q - ey*ey*r2q));

		glVertex2i(x-ex,y-ey);
		glVertex2i(x+ex,y-ey);

		glVertex2i(x-ex,y+ey);
		glVertex2i(x+ex,y+ey);
	}
	glEnd();
}



/**************************************************************************
 **************************************************************************

  private routines

 **************************************************************************
 **************************************************************************/

/**************************************************************************
	ReadProperties
 **************************************************************************/
TBOOL Display_ReadProperties( TMOD_DISPLAY *display )
{
	CFArrayRef		availModes;
	CFDictionaryRef displayModeDict;
	TINT	i;

	displayModeDict = CGDisplayCurrentMode(kCGDirectMainDisplay);
	
	if (displayModeDict)	/* query current configuration */
	{
		CFNumberRef	value;
		
		value = CFDictionaryGetValue (displayModeDict, kCGDisplayBitsPerPixel);
		CFNumberGetValue (value,  kCFNumberIntType, &display->defaultdepth);

		value = CFDictionaryGetValue (displayModeDict, kCGDisplayWidth);
		CFNumberGetValue (value,  kCFNumberIntType, &display->defaultwidth);
		
		value = CFDictionaryGetValue (displayModeDict, kCGDisplayHeight);
		CFNumberGetValue (value,  kCFNumberIntType, &display->defaultheight);
	}

	availModes = CGDisplayAvailableModes(kCGDirectMainDisplay);
	
	if (!availModes)
		return TFALSE;
	
	display->numdismodes = CFArrayGetCount(availModes);

	display->modelist=(TDISMODE*)TExecAlloc0(display->exec,display->modelist,display->numdismodes*sizeof(TDISMODE));
	
	for (i=0;i<display->numdismodes;i++)
	{
		TINT w,h,d;

		CFNumberGetValue( CFDictionaryGetValue( CFArrayGetValueAtIndex(availModes, i), kCGDisplayWidth), kCFNumberIntType, &w );
		CFNumberGetValue( CFDictionaryGetValue( CFArrayGetValueAtIndex(availModes, i), kCGDisplayHeight), kCFNumberIntType, &h );
		CFNumberGetValue( CFDictionaryGetValue( CFArrayGetValueAtIndex(availModes, i), kCGDisplayBitsPerPixel), kCFNumberIntType, &d );
		
		if (display->minwidth > w)
			display->minwidth = w;
		if (display->minheight > h)
			display->minheight = h;
		if (display->mindepth > d)
			display->mindepth = d;

		if (display->maxwidth < w)
			display->maxwidth = w;
		if (display->maxheight < h)
			display->maxheight = h;
		if (display->maxdepth < d)
			display->maxdepth = d;

		display->modelist[i].width  = w;
		display->modelist[i].height = h;
		display->modelist[i].depth  = d;
	}

	return TTRUE;
}

/**************************************************************************
  ParseBufferFormat
 **************************************************************************/
TVOID Display_ParseBufferFormat( TMOD_DISPLAY *display )
{
}

/********************************************************************************
	Display MakeKeytransTable
 ********************************************************************************/
TVOID Display_MakeKeytransTable(TMOD_DISPLAY *display)
{
	display->keytranstable=TExecAlloc0(display->exec,TNULL,256);

        display->keytranstable[0x1B] = TDISKEY_ESCAPE;
        display->keytranstable[0xBE] = TDISKEY_F1;
        display->keytranstable[0xBF] = TDISKEY_F2;
        display->keytranstable[0xC0] = TDISKEY_F3;
        display->keytranstable[0xC1] = TDISKEY_F4;
        display->keytranstable[0xC2] = TDISKEY_F5;
        display->keytranstable[0xC3] = TDISKEY_F6;
        display->keytranstable[0xC4] = TDISKEY_F7;
        display->keytranstable[0xC5] = TDISKEY_F8;
        display->keytranstable[0xC6] = TDISKEY_F9;
        display->keytranstable[0xC7] = TDISKEY_F10;
        display->keytranstable[0xC8] = TDISKEY_F11;
        display->keytranstable[0xC9] = TDISKEY_F12;

        display->keytranstable[0x5E] = TDISKEY_GRAVE;
        display->keytranstable[0x31] = TDISKEY_1;
        display->keytranstable[0x32] = TDISKEY_2;
        display->keytranstable[0x33] = TDISKEY_3;
        display->keytranstable[0x34] = TDISKEY_4;
        display->keytranstable[0x35] = TDISKEY_5;
        display->keytranstable[0x36] = TDISKEY_6;
        display->keytranstable[0x37] = TDISKEY_7;
        display->keytranstable[0x38] = TDISKEY_8;
        display->keytranstable[0x39] = TDISKEY_9;
        display->keytranstable[0x30] = TDISKEY_0;
        display->keytranstable[0xDF] = TDISKEY_MINUS;
        display->keytranstable[0x27] = TDISKEY_EQUALS;
        display->keytranstable[0x08] = TDISKEY_BACKSPACE;

		display->keytranstable[0x09] = TDISKEY_TAB;
        display->keytranstable[0x71] = TDISKEY_q;
        display->keytranstable[0x77] = TDISKEY_w;
        display->keytranstable[0x65] = TDISKEY_e;
        display->keytranstable[0x72] = TDISKEY_r;
        display->keytranstable[0x74] = TDISKEY_t;
        display->keytranstable[0x7A] = TDISKEY_y;
        display->keytranstable[0x75] = TDISKEY_u;
        display->keytranstable[0x69] = TDISKEY_i;
        display->keytranstable[0x6F] = TDISKEY_o;
        display->keytranstable[0x70] = TDISKEY_p;
        display->keytranstable[0xFC] = TDISKEY_LEFTBRACKET;
        display->keytranstable[0x2B] = TDISKEY_RIGHTBRACKET;
        display->keytranstable[0x0D] = TDISKEY_RETURN;

        display->keytranstable[0xE5] = TDISKEY_CAPSLOCK;
        display->keytranstable[0x61] = TDISKEY_a;
        display->keytranstable[0x73] = TDISKEY_s;
        display->keytranstable[0x64] = TDISKEY_d;
        display->keytranstable[0x66] = TDISKEY_f;
        display->keytranstable[0x67] = TDISKEY_g;
        display->keytranstable[0x68] = TDISKEY_h;
        display->keytranstable[0x6A] = TDISKEY_j;
        display->keytranstable[0x6B] = TDISKEY_k;
        display->keytranstable[0x6C] = TDISKEY_l;
        display->keytranstable[0xF6] = TDISKEY_SEMICOLON;
        display->keytranstable[0xE4] = TDISKEY_APOSTROPH;
        display->keytranstable[0x23] = TDISKEY_BACKSLASH;

        display->keytranstable[0xE1] = TDISKEY_LSHIFT;
        display->keytranstable[0x3C] = TDISKEY_EXTRA1;
        display->keytranstable[0x79] = TDISKEY_z;
        display->keytranstable[0x78] = TDISKEY_x;
        display->keytranstable[0x63] = TDISKEY_c;
        display->keytranstable[0x76] = TDISKEY_v;
        display->keytranstable[0x62] = TDISKEY_b;
        display->keytranstable[0x6E] = TDISKEY_n;
        display->keytranstable[0x6D] = TDISKEY_m;
        display->keytranstable[0x2C] = TDISKEY_COMMA;
        display->keytranstable[0x2E] = TDISKEY_PERIOD;
        display->keytranstable[0x2D] = TDISKEY_SLASH;
        display->keytranstable[0xE2] = TDISKEY_RSHIFT;

		display->keytranstable[0xE3] = TDISKEY_LCTRL;
        display->keytranstable[0xE9] = TDISKEY_LALT;
        display->keytranstable[0x20] = TDISKEY_SPACE;
        display->keytranstable[0x7E] = TDISKEY_RALT;
        display->keytranstable[0xE4] = TDISKEY_RCTRL;

        display->keytranstable[0x52] = TDISKEY_UP;
        display->keytranstable[0x51] = TDISKEY_LEFT;
        display->keytranstable[0x53] = TDISKEY_RIGHT;
        display->keytranstable[0x54] = TDISKEY_DOWN;

        display->keytranstable[0x61] = TDISKEY_PRINT;
        display->keytranstable[0x14] = TDISKEY_SCROLLOCK;
        display->keytranstable[0x13] = TDISKEY_PAUSE;

        display->keytranstable[0x63] = TDISKEY_INSERT;
        display->keytranstable[0x50] = TDISKEY_HOME;
        display->keytranstable[0x55] = TDISKEY_PAGEUP;
        display->keytranstable[0xFF] = TDISKEY_DELETE;
        display->keytranstable[0x57] = TDISKEY_END;
        display->keytranstable[0x56] = TDISKEY_PAGEDOWN;

        display->keytranstable[0x7F] = TDISKEY_NUMLOCK;
        display->keytranstable[0xAF] = TDISKEY_KP_DIVIDE;
        display->keytranstable[0xAA] = TDISKEY_KP_MULTIPLY;
        display->keytranstable[0xAD] = TDISKEY_KP_MINUS;
        display->keytranstable[0xAB] = TDISKEY_KP_PLUS;
        display->keytranstable[0x8D] = TDISKEY_KP_ENTER;

		display->keytranstable[0x95] = TDISKEY_KP7;
        display->keytranstable[0x97] = TDISKEY_KP8;
        display->keytranstable[0x9A] = TDISKEY_KP9;
        display->keytranstable[0x96] = TDISKEY_KP4;
        display->keytranstable[0x9D] = TDISKEY_KP5;
        display->keytranstable[0x98] = TDISKEY_KP6;
        display->keytranstable[0x9C] = TDISKEY_KP1;
        display->keytranstable[0x99] = TDISKEY_KP2;
        display->keytranstable[0x9B] = TDISKEY_KP3;
        display->keytranstable[0x9E] = TDISKEY_KP0;
        display->keytranstable[0x9F] = TDISKEY_KP_PERIOD;

        display->keytranstable[0x00] = TDISKEY_OEM1;
        display->keytranstable[0x02] = TDISKEY_OEM2;
        display->keytranstable[0x03] = TDISKEY_OEM3;
}

TVOID processEvents( EventRef theEvent, TAPTR mod )
{
	TUINT32			eventClass;
	TMOD_DISPLAY*	display = mod;
	EventTargetRef 	theTarget;

	theTarget = GetEventDispatcherTarget();

	eventClass = GetEventClass (theEvent);
	
	switch (eventClass)
	{
		case kEventClassKeyboard:
			processKeyboardEvents( theEvent, display );
			break;
		case kEventClassApplication:
			break;
		case kEventClassCommand:
			break;
		case kEventClassMenu:
			break;
		case kEventClassMouse:
			processMouseEvents( theEvent, display );
			break;
		case kEventClassAppleEvent:
			break;
		case kEventClassTeklib:
		{
			display->msgcode = TDISMSG_REDRAW;
			break;
		}
		
		default:
			break;
	}

	SendEventToEventTarget (theEvent, theTarget );
	ReleaseEvent(theEvent);
}

TVOID processKeyboardEvents( EventRef theEvent, TAPTR mod )
{
	TUINT32			eventKind;
	TMOD_DISPLAY*	display = mod;

	eventKind = GetEventKind (theEvent);

	switch( eventKind )
	{
		case kEventRawKeyDown:
		{
			EventRecord record;
			TUINT	keysym = 0;
			TUINT8	key;
			
			ConvertEventRefToEventRecord(theEvent, &record);

			keysym = record.message & charCodeMask;

			if(keysym == 0xff20)
				key = display->keytranstable[0x02];
			else if(keysym == 0xff67)
				key = display->keytranstable[0x03];
			else
				key = display->keytranstable[((TUINT8)(keysym)) & 0x000000ff];

			if(key == TDISKEY_LSHIFT || key == TDISKEY_RSHIFT)
			{
				display->keyqual |= TDISKEYQUAL_SHIFT;
				display->key.qualifier = 0;
			}
			else if(key == TDISKEY_LALT || key == TDISKEY_RALT)
			{
				display->keyqual |= TDISKEYQUAL_ALT;
				display->key.qualifier = 0;
			}
			else if(key==TDISKEY_LCTRL || key==TDISKEY_RCTRL)
			{
				display->keyqual |= TDISKEYQUAL_CTRL;
				display->key.qualifier = 0;
			}
			else
				display->key.qualifier=display->keyqual;
			
			display->key.code = key;
			display->msgcode = TDISMSG_KEYDOWN;
			break;
		}
		case kEventRawKeyUp:
		{
			EventRecord record;
			TUINT	keysym = 0;
			TUINT8	key;
			
			ConvertEventRefToEventRecord(theEvent, &record);

			keysym = record.message & charCodeMask;

			if(keysym == 0xff20)
				key = display->keytranstable[0x02];
			else if(keysym == 0xff67)
				key = display->keytranstable[0x03];
			else
				key = display->keytranstable[((TUINT8)(keysym)) & 0x000000ff];

			if(key==TDISKEY_LSHIFT || key==TDISKEY_RSHIFT)
			{
				display->keyqual |= TDISKEYQUAL_SHIFT;
				display->key.qualifier = 0;
			}
			else if(key==TDISKEY_LALT || key==TDISKEY_RALT)
			{
				display->keyqual |= TDISKEYQUAL_ALT;
				display->key.qualifier = 0;
			}
			else if(key==TDISKEY_LCTRL || key==TDISKEY_RCTRL)
			{
				display->keyqual |= TDISKEYQUAL_CTRL;
				display->key.qualifier  = 0;
			}
			else
				display->key.qualifier=display->keyqual;

			display->key.code = key;
			display->msgcode = TDISMSG_KEYUP;

			break;
		}
		
		default:
			break;
	}

}

TVOID processMouseEvents( EventRef theEvent, TAPTR mod )
{
	TUINT32			eventKind;
	TMOD_DISPLAY*	display = mod;

	eventKind = GetEventKind (theEvent);

	switch (eventKind)
	{
		case kEventMouseDown:
		{
			TUINT16	button;
			GetEventParameter (theEvent, kEventParamMouseButton, typeMouseButton, TNULL, sizeof(TUINT16), TNULL, &button);
	
			if (button == kEventMouseButtonPrimary)
			{
				display->mbutton.code = TDISMB_LBUTTON;

				display->msgcode = TDISMSG_MBUTTONDOWN;
			}
			if (button == kEventMouseButtonSecondary)
			{
				display->mbutton.code = TDISMB_MBUTTON;

				display->msgcode = TDISMSG_MBUTTONDOWN;
			}
			if (button == kEventMouseButtonTertiary)
			{
				display->mbutton.code = TDISMB_RBUTTON;

				display->msgcode = TDISMSG_MBUTTONDOWN;
			}
			break;
		}
		case kEventMouseUp:
		{
			TUINT16	button;
			GetEventParameter (theEvent, kEventParamMouseButton,
						typeMouseButton, TNULL, sizeof(TUINT16), TNULL, &button);
			if (button == kEventMouseButtonPrimary)
			{
				display->mbutton.code = TDISMB_LBUTTON;

				display->msgcode = TDISMSG_MBUTTONUP;
			}
			if (button == kEventMouseButtonSecondary)
			{
				display->mbutton.code = TDISMB_MBUTTON;

				display->msgcode = TDISMSG_MBUTTONUP;
			}
			if (button == kEventMouseButtonTertiary)
			{
				display->mbutton.code = TDISMB_RBUTTON;

				display->msgcode = TDISMSG_MBUTTONUP;
			}
			break;
		}
		case kEventMouseDragged:
		case kEventMouseMoved:
		{
			Point mouseLoc;
			GetMouse (&mouseLoc);
			
			ShowCursor();
			
			display->mmove.x = mouseLoc.h < 0 || mouseLoc.v < 0 || mouseLoc.h > display->width || mouseLoc.v > display->height ? 0 : mouseLoc.h;
			display->mmove.y = mouseLoc.h < 0 || mouseLoc.v < 0 || mouseLoc.h > display->width || mouseLoc.v > display->height ? 0 : mouseLoc.v;			
			
			display->msgcode = TDISMSG_MOUSEMOVE;
			break;
		}
		default:
			break;
	}
}

TVOID Display_PutImage( TMOD_DISPLAY* display, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
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
			type=GL_UNSIGNED_INT_8_8_8_8_REV ;
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

	glEnable(GL_SMOOTH);
	glShadeModel(GL_SMOOTH);

	glRasterPos2i(dst->x,dst->y);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,src->y);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,img->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,src->x);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelZoom((GLfloat)dst->width/(GLfloat)src->width,-(GLfloat)dst->height/(GLfloat)src->height);
	glDrawPixels(src->width,src->height,format,type,img->data);

	glPopClientAttrib();
}

TVOID Display_SetStrokeFillColors( TMOD_DISPLAY *display, CGContextRef con )
{
	if ( display->theDrawPen )
	{
		CGContextSetRGBFillColor ( con, (float) ( (float) display->theDrawPen->color.r / (float) 255),
										(float) ( (float) display->theDrawPen->color.g / (float) 255),
										(float) ( (float) display->theDrawPen->color.b / (float) 255),
										(float) ( (float) ( 255 - display->theDrawPen->color.a) / (float) 255));
										
		CGContextSetRGBStrokeColor ( con, (float) ( (float) display->theDrawPen->color.r / (float) 255),
										  (float) ( (float) display->theDrawPen->color.g / (float) 255),
										  (float) ( (float) display->theDrawPen->color.b / (float) 255),
										  (float) ( (float) ( 255 - display->theDrawPen->color.a) / (float) 255));
	}
}

TVOID Display_InstallDrawTimer( TMOD_DISPLAY *display )
{
    EventLoopTimerRef timer;
    
    InstallEventLoopTimer( GetCurrentEventLoop(), 0, kEventDurationSecond, NewEventLoopTimerUPP( Display_DrawTimerProc ), display, &timer );
//    InstallEventLoopTimer( GetCurrentEventLoop(), 0, kEventDurationMillisecond, NewEventLoopTimerUPP( Display_DrawTimerProc ), display, &timer );
}

void Display_DrawTimerProc ( EventLoopTimerRef inTimer, void *inUserData )
{
	EventRef		theEvent;
	EventTargetRef 	theTarget;
	EventQueueRef   theQueue;

	theQueue = GetMainEventQueue();
	theTarget = GetEventDispatcherTarget();

	if ( CreateEvent( TNULL, kEventClassTeklib, kEventTeklibRedraw, 0, kEventAttributeNone, &theEvent ) == noErr )
		PostEventToQueue( theQueue, theEvent, kEventPriorityHigh );
}

/**************************************************************************
  Display ProcessMessage
 **************************************************************************/
TVOID Display_ProcessMessage(TMOD_DISPLAY *display,TDISMSG *dismsg)
{
	switch (dismsg->code)
	{
		case TDISMSG_RESIZE: case TDISMSG_MOVE:
			TExecCopyMem(display->exec,&display->drect,dismsg->data,sizeof(TDISRECT));
		break;

		case TDISMSG_KEYDOWN: case TDISMSG_KEYUP:
			TExecCopyMem(display->exec,&display->key,dismsg->data,sizeof(TDISKEY));
		break;

		case TDISMSG_MOUSEMOVE:
			TExecCopyMem(display->exec,&display->mmove,dismsg->data,sizeof(TDISMOUSEPOS));
		break;

		case TDISMSG_MBUTTONDOWN: case TDISMSG_MBUTTONUP:
			TExecCopyMem(display->exec,&display->mbutton,dismsg->data,sizeof(TDISMBUTTON));
		break;

	}
}
