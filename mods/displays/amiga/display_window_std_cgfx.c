#define MOD_VERSION     1
#define MOD_REVISION    0

#include <tek/teklib.h>
#include <tek/exec.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/imgproc.h>
#include <tek/debug.h>

#include <tek/mod/imgproc.h>
#include <tek/mod/displayhandler.h>

#include <intuition/intuition.h>
#include <intuition/pointerclass.h>

#ifdef TSYS_AMIGA
/* This is an outdated installation. Use the version below if this fails */
#include <cybergraphics/cybergraphics.h>
#include <libraries/cybergraphics.h>
#else
#include <cybergraphx/cybergraphics.h>
#endif

#include <graphics/gfx.h>
#include <graphics/scale.h>

#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>
#include <proto/exec.h>

typedef struct
{
    struct RastPort rport;
    struct TmpRas theTmpRas;
    TUINT8 *tmprasbuf;
}offscreenBitmap;

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
	TINT    bufferdepth;
	TUINT   bufferformat;
	TINT    workbuf;

	TINT font_w,font_h;
	TINT bmwidth, bmheight;

	TDISPEN *theDrawPen;
	TUINT pencolor;

	TINT    ptrmode;
	TBOOL   deltamouse;
	TBOOL   vsync,smoothscale;

	TINT keyqual;

	/*-------------------- AmigaOS typical variables -------------------------*/
	struct Library *cybergfxbase;
	struct Library *gfxbase;
	struct Library *intuitionbase;

	struct Screen *theScreen;
	struct Window *theWindow;
	struct RastPort *theRastPort;
	struct ViewPort *theViewPort;
	struct RastPort rport[2];
	struct TextFont *sysfont;
	offscreenBitmap *theBitmap;

	struct TmpRas theTmpRas;
	TUINT8 *tmprasbuf;
	struct AreaInfo theAreaInfo;
	TUINT8 *areabuf;

	struct RastPort *theDrawRPort;

	struct BitMap pointer_bitmap;
	void *MousePointer;
	TBOOL outside;

	TUINT *lockhandle;
	TUINT16 zoomrect[4];
	TINT borderwidth,borderheight;
	TBOOL waitmessage;
	TBOOL ldown,mdown,rdown;
	TBOOL mwheelup,mwheeldown;
	TUINT8 *keymap;
	/*--------------------------------------------------------------------*/

	TBOOL   sysfontset;
	TINT    sysfontysize;

	TINT    mouseoff_x,mouseoff_y;
	TBOOL   cursorset;
	TUINT8 *keytranstable;

	TBOOL fullscreen;

} TMOD_DISMOD;

/* private prototypes */
TBOOL Dismod_ReadProperties( TMOD_DISMOD *dismod );
TBOOL Dismod_CreateWindow(TMOD_DISMOD *dismod);
TBOOL Dismod_CreateDirectBitmap(TMOD_DISMOD *dismod);
TVOID Dismod_DestroyDirectBitmap( TMOD_DISMOD *dismod );
TVOID Dismod_ParseBufferFormat( TMOD_DISMOD *dismod );
TVOID Dismod_PutImage(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst);
TVOID Dismod_SetPointerMode(TMOD_DISMOD *dismod,TINT mode);
TVOID Dismod_ProcessMessage(TMOD_DISMOD *dismod,TDISMSG *dismsg);
TVOID Dismod_ProcessEvents(TMOD_DISMOD *dismod,struct IntuiMessage *msg);
TVOID Dismod_MakeKeytransTable(TMOD_DISMOD *dismod);

#define SysBase         *((struct ExecBase **) 4L)
#define GfxBase         dismod->gfxbase
#define IntuitionBase   dismod->intuitionbase
#define CyberGfxBase    dismod->cybergfxbase

/* global module init stuff */
#ifdef __SASC
#include "/modinit.h"
#else
#include "../modinit.h"
#endif

/* XLib drawing routines */
#include "aosdrawdisplay.h"
#include "aosdrawbitmap.h"

/* standard message callback */
#include "aoscommon.h"

/**************************************************************************
	tek_init
 **************************************************************************/
TMODENTRY TUINT tek_init_display_window_std_cgfx(TAPTR selftask, TMOD_DISMOD *mod, TUINT16 version, TTAGITEM *tags)
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
    TExecCopyMem(dismod->exec,title,dismod->windowname,TUtilStrLen(dismod->util,title));

    if(Dismod_CreateWindow(dismod))
    {
	    Dismod_MakeKeytransTable(dismod);

	    dismod->ptrmode     = TDISPTR_NORMAL;
	    dismod->deltamouse      = TFALSE;
	    dismod->vsync           = TFALSE;
	    dismod->smoothscale = TFALSE;

	    InitBitMap(&dismod->pointer_bitmap,0,0,0);
	    dismod->MousePointer=(void*)NewObject(NULL,"pointerclass",POINTERA_BitMap,(int)&dismod->pointer_bitmap,TAG_DONE);

	    dismod->keymap=TExecAlloc0(dismod->exec,TNULL,256);
	    return TTRUE;
    }
    return TFALSE;
}


/**************************************************************************
	dismod_destroy
 **************************************************************************/
TMODAPI TVOID dismod_destroy(TMOD_DISMOD *dismod)
{
    if(dismod->keymap)
	    TExecFree(dismod->exec,dismod->keymap), dismod->keymap=TNULL;

    if(dismod->areabuf)
	TExecFree(dismod->exec,dismod->areabuf), dismod->areabuf=TNULL;

    if(dismod->MousePointer)
	DisposeObject(dismod->MousePointer), dismod->MousePointer=TNULL;

    if(dismod->keytranstable)
	    TExecFree(dismod->exec,dismod->keytranstable), dismod->keytranstable=TNULL;

    if(dismod->windowname)
	TExecFree(dismod->exec,dismod->windowname), dismod->windowname=TNULL;

    Dismod_DestroyDirectBitmap(dismod);

    if(dismod->theWindow)
	CloseWindow(dismod->theWindow), dismod->theWindow=TNULL;

    if (IntuitionBase) CloseLibrary( (struct Library *)IntuitionBase ), IntuitionBase=NULL;
    if (GfxBase) CloseLibrary(( struct Library *)GfxBase), GfxBase=NULL;
    if (CyberGfxBase) CloseLibrary(( struct Library *)CyberGfxBase), CyberGfxBase=NULL;
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
	caps->maxbmwidth=1024;
	caps->maxbmheight=1024;

	caps->blitscale=TTRUE;
	caps->blitalpha=TFALSE;
	caps->blitckey=TFALSE;

	caps->canconvertdisplay=TTRUE;
	caps->canconvertscaledisplay=TFALSE;
	caps->canconvertbitmap=TTRUE;
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
    TBOOL dm = (TINT)TGetTag(tags,TDISTAG_DELTAMOUSE,(TTAG)dismod->deltamouse);
    TINT ptr = (TINT)TGetTag(tags,TDISTAG_POINTERMODE,(TTAG)dismod->ptrmode);
    dismod->vsync       = (TINT)TGetTag(tags,TDISTAG_VSYNCHINT,(TTAG)dismod->vsync);
    dismod->smoothscale = (TINT)TGetTag(tags,TDISTAG_SMOOTHHINT,(TTAG)dismod->smoothscale);

    dismod->ptrmode=ptr;

    if(dm!=dismod->deltamouse)
    {
	if(dm)
	{
	    ModifyIDCMP( dismod->theWindow,
			 REFRESHWINDOW     |
			 CHANGEWINDOW      |
			 ACTIVEWINDOW      |
			 INACTIVEWINDOW    |
			 RAWKEY            |
			 CLOSEWINDOW       |
			 MOUSEMOVE         |
			 MOUSEBUTTONS      |
			 DELTAMOVE
		       );

	    ptr=TDISPTR_INVISIBLE;
	}
	else
	{
	    ModifyIDCMP( dismod->theWindow,
			 REFRESHWINDOW     |
			 CHANGEWINDOW      |
			 ACTIVEWINDOW      |
			 INACTIVEWINDOW    |
			 RAWKEY            |
			 CLOSEWINDOW       |
			 MOUSEMOVE         |
			 MOUSEBUTTONS
		       );
	}
	dismod->deltamouse=dm;
    }

    Dismod_SetPointerMode(dismod,ptr);
}

/**************************************************************************
	dismod_flush
 **************************************************************************/
TMODAPI TVOID dismod_flush(TMOD_DISMOD *dismod)
{
    if(dismod->window_ready)
    {
	BltBitMapRastPort( dismod->rport[dismod->workbuf].BitMap, 0,0, dismod->theRastPort,
			   0,0, (unsigned long)dismod->width, (unsigned long)dismod->height,
			   ABNC | ABC );

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
	LONG amipen=ObtainBestPen(dismod->theViewPort->ColorMap,
				  pen->color.r<<24,pen->color.g<<24,pen->color.b<<24,
				  OBP_Precision,PRECISION_EXACT,
				  TAG_DONE);

	pen->hostdata=(TAPTR)amipen;
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
	LONG amipen=(LONG)pen->hostdata;
	ReleasePen(dismod->theViewPort->ColorMap,amipen);
    }
}

/**************************************************************************
	dismod_setdpen
 **************************************************************************/
TMODAPI TVOID dismod_setdpen(TMOD_DISMOD *dismod, TDISPEN *pen)
{
    LONG amipen=(LONG)pen->hostdata;
    dismod->theDrawPen=pen;
    if(dismod->theBitmap)
	SetAPen(&dismod->theBitmap->rport,amipen);
    else
	SetAPen(&dismod->rport[dismod->workbuf],amipen);
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

	InitRastPort(&bm->rport);
	bm->rport.BitMap=AllocBitMap( (unsigned long)width,
				      (unsigned long)height,
				      (unsigned long)dismod->bufferdepth,
				      BMF_MINPLANES,
				      dismod->theRastPort->BitMap
				    );
	if(bm->rport.BitMap)
	{
	    bm->tmprasbuf=TExecAlloc0(dismod->exec,TNULL,(width*height/8)*dismod->bufferdepth);
	    InitTmpRas(&bm->theTmpRas,bm->tmprasbuf,(width*height/8)*dismod->bufferdepth);

	    bm->rport.TmpRas=&bm->theTmpRas;
	    bm->rport.AreaInfo=&dismod->theAreaInfo;

	    bitmap->hostdata=(TAPTR)bm;
	    bitmap->image.width=width;
	    bitmap->image.height=height;
	    bitmap->image.depth=dismod->bufferdepth;
	    bitmap->image.format=dismod->bufferformat;
	    bitmap->image.bytesperrow=GetCyberMapAttr(bm->rport.BitMap,CYBRMATTR_XMOD);

	    FillPixelArray(&bm->rport,0,0,width,height,0);

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
    offscreenBitmap *bm=(offscreenBitmap*)bitmap->hostdata;

    if(bm)
    {
	if(bm->rport.BitMap)
	    FreeBitMap(bm->rport.BitMap), bm->rport.BitMap=TNULL;

	if(bm->tmprasbuf)
	    TExecFree(dismod->exec,bm->tmprasbuf), bm->tmprasbuf=TNULL;

	TExecFree(dismod->exec,bm);
	bitmap->hostdata=TNULL;
    }
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
	desc->width=dismod->theWindow->Width;
	desc->height=dismod->theWindow->Height;
	desc->depth=dismod->bufferdepth;
	desc->format=dismod->bufferformat;
	desc->bytesperrow=GetCyberMapAttr(dismod->rport[dismod->workbuf].BitMap,CYBRMATTR_XMOD);
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
	desc->width=GetCyberMapAttr(bitmap->rport.BitMap,CYBRMATTR_WIDTH);
	desc->height=GetCyberMapAttr(bitmap->rport.BitMap,CYBRMATTR_HEIGHT);
	desc->depth=dismod->bufferdepth;
	desc->format=dismod->bufferformat;
	desc->bytesperrow=GetCyberMapAttr(bitmap->rport.BitMap,CYBRMATTR_XMOD);
    }
}

/**************************************************************************
	dismod_lock_dis
 **************************************************************************/
TMODAPI TBOOL dismod_lock_dis(TMOD_DISMOD *dismod, TIMGPICTURE *img)
{
    if(dismod->window_ready)
    {
	dismod->lockhandle = (TUINT*)LockBitMapTags( dismod->rport[dismod->workbuf].BitMap,
						      LBMI_WIDTH,       (TUINT)&img->width,
						      LBMI_HEIGHT,      (TUINT)&img->height,
						      LBMI_BYTESPERROW, (TUINT)&img->bytesperrow,
						      LBMI_BASEADDRESS, (TUINT)&img->data,
						      TAG_DONE);


	if(dismod->lockhandle)
	{
	    img->depth=dismod->bufferdepth;
	    img->format=dismod->bufferformat;
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

	dismod->lockhandle = (TUINT*)LockBitMapTags( dismod->theBitmap->rport.BitMap,
						      LBMI_WIDTH,      (TUINT)&img->width,
						      LBMI_HEIGHT,     (TUINT)&img->height,
						      LBMI_BYTESPERROW,(TUINT)&img->bytesperrow,
						      LBMI_BASEADDRESS,(TUINT)&img->data,
						      TAG_DONE);
	if(dismod->lockhandle)
	{
	    img->depth=dismod->bufferdepth;
	    img->format=dismod->bufferformat;
	    return TTRUE;
	}
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
	if(dismod->lockhandle)
	{
	    UnLockBitMap(dismod->lockhandle);
	    dismod->lockhandle=0;
	}
    }
}

/**************************************************************************
	dismod_unlock_bm
 **************************************************************************/
TMODAPI TVOID dismod_unlock_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
    if(dismod->window_ready)
    {
	if(dismod->lockhandle)
	{
	    UnLockBitMap(dismod->lockhandle);
	    dismod->lockhandle=0;
	    dismod->theBitmap=TNULL;
	}
    }
}

/**************************************************************************
	dismod_begin_dis
 **************************************************************************/
TMODAPI TBOOL dismod_begin_dis(TMOD_DISMOD *dismod)
{
    if(dismod->window_ready)
    {
	dismod->theDrawRPort=&dismod->rport[dismod->workbuf];
	if(dismod->theDrawPen)
	{
	    LONG amipen=(LONG)dismod->theDrawPen->hostdata;
	    SetAPen(dismod->theDrawRPort,amipen);
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
	dismod->theDrawRPort=&dismod->theBitmap->rport;
	if(dismod->theDrawPen)
	{
	    LONG amipen=(LONG)dismod->theDrawPen->hostdata;
	    SetAPen(dismod->theDrawRPort,amipen);
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
    dismod->theDrawRPort=TNULL;
}

/**************************************************************************
	dismod_end_bm
 **************************************************************************/
TMODAPI TVOID dismod_end_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
    dismod->theDrawRPort=TNULL;
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

	if(bops->src.width==bops->dst.width && bops->src.height==bops->dst.height)
	{
	    BltBitMap(bitmap->rport.BitMap,
		      bops->src.x,bops->src.y,
		      dismod->rport[dismod->workbuf].BitMap,
		      bops->dst.x,bops->dst.y,
		      bops->src.width,bops->src.height,
		      ABNC|ABC,
		      0xffffffff,
		      TNULL);
	}
	else
	{
	    struct BitScaleArgs bs;

	    bs.bsa_SrcX=bops->src.x;
	    bs.bsa_SrcY=bops->src.y;
	    bs.bsa_SrcWidth=bops->src.width;
	    bs.bsa_SrcHeight=bops->src.height;
	    bs.bsa_DestX=bops->dst.x;
	    bs.bsa_DestY=bops->dst.y;
	    bs.bsa_SrcBitMap=bitmap->rport.BitMap;
	    bs.bsa_DestBitMap=dismod->rport[dismod->workbuf].BitMap;

	    bs.bsa_XSrcFactor=bops->src.width;
	    bs.bsa_YSrcFactor=bops->src.height;
	    bs.bsa_XDestFactor=bops->dst.width;
	    bs.bsa_YDestFactor=bops->dst.height;

	    bs.bsa_DestWidth=0;
	    bs.bsa_DestHeight=0;
	    bs.bsa_Flags=0;
	    bs.bsa_XDDA=0;
	    bs.bsa_YDDA=0;
	    bs.bsa_Reserved1=0;
	    bs.bsa_Reserved2=0;

	    BitMapScale(&bs);
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
    if(dismod->window_ready)
    {
	Dismod_PutImage(dismod,bm,img,src,dst);
    }
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
    TUINT modeid;
    struct Screen *theScreen;
    TINT d;
    TUINT8 buf[128];

    struct DimensionInfo *di=(struct DimensionInfo *)buf;

    CyberGfxBase=NULL;
    GfxBase=NULL;
    IntuitionBase=NULL;

    CyberGfxBase = OpenLibrary((UBYTE *)"cybergraphics.library",2);
    GfxBase = OpenLibrary((UBYTE *)"graphics.library", 39L);
    IntuitionBase = OpenLibrary((UBYTE *)"intuition.library", 39L);

    if(CyberGfxBase && GfxBase && IntuitionBase)
    {
	theScreen=LockPubScreen( NULL );
	UnlockPubScreen( NULL, theScreen );

	modeid=GetVPModeID((struct ViewPort *)&theScreen->ViewPort);
	if(IsCyberModeID((ULONG)modeid))
	{
	    d=GetCyberIDAttr(CYBRIDATTR_BPPIX,(ULONG)modeid)*8;
	    if(d>=15)
	    {
		GetDisplayInfoData(NULL,buf,128,DTAG_DIMS,(ULONG)modeid);
		dismod->zoomrect[0]=0;
		dismod->zoomrect[1]=0;
		dismod->zoomrect[2]=di->Nominal.MaxX - di->Nominal.MinX +1;
		dismod->zoomrect[3]=di->Nominal.MaxY - di->Nominal.MinY +1;

		dismod->mindepth=15;
		dismod->maxdepth=32;
		dismod->defaultdepth=d;

		dismod->minwidth=64;
		dismod->minheight=64;
		dismod->maxwidth=theScreen->Width;
		dismod->maxheight=theScreen->Height;

		dismod->defaultwidth=dismod->maxwidth/2;
		dismod->defaultheight=dismod->maxheight/2;

		return TTRUE;
	    }
	}
    }
    if (IntuitionBase) CloseLibrary( (struct Library *)IntuitionBase ), IntuitionBase=NULL;
    if (GfxBase) CloseLibrary(( struct Library *)GfxBase), GfxBase=NULL;
    if (CyberGfxBase) CloseLibrary(( struct Library *)CyberGfxBase), CyberGfxBase=NULL;
    return TFALSE;
}

/**************************************************************************
	CreateWindow
 **************************************************************************/
TBOOL Dismod_CreateWindow(TMOD_DISMOD *dismod)
{
    struct TagItem wtags[23];

    dismod->theScreen=LockPubScreen(NULL);

    wtags[ 0].ti_Tag=WA_Left;            wtags[ 0].ti_Data=dismod->window_xpos;
    wtags[ 1].ti_Tag=WA_Top;             wtags[ 1].ti_Data=dismod->window_ypos;
    wtags[ 2].ti_Tag=WA_InnerWidth;      wtags[ 2].ti_Data=dismod->width;
    wtags[ 3].ti_Tag=WA_InnerHeight;     wtags[ 3].ti_Data=dismod->height;
    wtags[ 4].ti_Tag=WA_Title;           wtags[ 4].ti_Data=(TINT)dismod->windowname;
    wtags[ 5].ti_Tag=WA_Activate;        wtags[ 5].ti_Data=TRUE;
    wtags[ 6].ti_Tag=WA_DragBar;         wtags[ 6].ti_Data=TRUE;
    wtags[ 7].ti_Tag=WA_DepthGadget;     wtags[ 7].ti_Data=TRUE;
    wtags[ 8].ti_Tag=WA_CloseGadget;     wtags[ 8].ti_Data=TRUE;
    wtags[ 9].ti_Tag=WA_SimpleRefresh;   wtags[ 9].ti_Data=TRUE;
    wtags[10].ti_Tag=WA_RMBTrap;         wtags[10].ti_Data=TRUE;
    wtags[11].ti_Tag=WA_ReportMouse;     wtags[11].ti_Data=TRUE;
    wtags[12].ti_Tag=WA_GimmeZeroZero;   wtags[12].ti_Data=TRUE;
    wtags[13].ti_Tag=WA_IDCMP;           wtags[13].ti_Data=REFRESHWINDOW  |
							   CHANGEWINDOW   |
							   ACTIVEWINDOW   |
							   INACTIVEWINDOW |
							   RAWKEY         |
							   CLOSEWINDOW    |
							   MOUSEMOVE      |
							   MOUSEBUTTONS;

    wtags[14].ti_Tag=WA_PubScreen;      wtags[14].ti_Data=(TINT)dismod->theScreen;

    if(dismod->resize)
    {
	wtags[15].ti_Tag=WA_MinWidth;    wtags[15].ti_Data=dismod->minwidth;
	wtags[16].ti_Tag=WA_MinHeight;   wtags[16].ti_Data=dismod->minheight;
	wtags[17].ti_Tag=WA_MaxWidth;    wtags[17].ti_Data=~0;
	wtags[18].ti_Tag=WA_MaxHeight;   wtags[18].ti_Data=~0;
	wtags[19].ti_Tag=WA_SizeGadget;  wtags[19].ti_Data=TRUE;
	wtags[20].ti_Tag=WA_SizeBBottom; wtags[20].ti_Data=TRUE;
	wtags[21].ti_Tag=WA_Zoom;        wtags[21].ti_Data=(TINT)dismod->zoomrect;
	wtags[22].ti_Tag=TAG_DONE;
    }
    else
    {
	wtags[15].ti_Tag=WA_SizeGadget;  wtags[15].ti_Data=TFALSE;
	wtags[16].ti_Tag=TAG_DONE;
    }

    dismod->theWindow = OpenWindowTagList(NULL,wtags);

    UnlockPubScreen( NULL, dismod->theScreen );

    if(dismod->theWindow)
    {
	dismod->theRastPort = dismod->theWindow->RPort;
	dismod->theViewPort = &dismod->theScreen->ViewPort;

	dismod->borderwidth=dismod->theWindow->BorderLeft+dismod->theWindow->BorderRight;
	dismod->borderheight=dismod->theWindow->BorderTop+dismod->theWindow->BorderBottom;

	dismod->font_w=dismod->theRastPort->Font->tf_XSize;
	dismod->font_h=dismod->theRastPort->Font->tf_YSize;

	dismod->areabuf=TExecAlloc0(dismod->exec,TNULL,256*5+256);
	InitArea(&dismod->theAreaInfo,dismod->areabuf,256);

	if(Dismod_CreateDirectBitmap(dismod))
	{
	    Dismod_ParseBufferFormat(dismod);
	    dismod->ldown=TFALSE;
	    dismod->mdown=TFALSE;
	    dismod->rdown=TFALSE;
	    dismod->window_ready=TTRUE;
	    return TTRUE;
	}
    }
    return TFALSE;
}

/********************************************************************************
	Display CreateDirectBitmap
 ********************************************************************************/
TBOOL Dismod_CreateDirectBitmap(TMOD_DISMOD *dismod)
{
    TINT i=0;
    TINT numbufs=1;

    TUINT d=GetCyberMapAttr(dismod->theRastPort->BitMap,CYBRMATTR_DEPTH);

    dismod->tmprasbuf=TExecAlloc0(dismod->exec,TNULL,(dismod->theWindow->Width*dismod->theWindow->Height/8)*d);
    InitTmpRas(&dismod->theTmpRas,dismod->tmprasbuf,(dismod->theWindow->Width*dismod->theWindow->Height/8)*d);

    if(dismod->dblbuf)
	    numbufs=2;

    while(i<numbufs)
    {
	InitRastPort( &dismod->rport[i] );
	dismod->rport[i].BitMap=AllocBitMap( (ULONG)dismod->width,
					      (ULONG)dismod->height,
					      (ULONG)d,
					      BMF_MINPLANES,
					      dismod->theRastPort->BitMap
					    );
	if(!dismod->rport[i].BitMap)
	    return TFALSE;

	dismod->rport[i].TmpRas=&dismod->theTmpRas;
	dismod->rport[i].AreaInfo=&dismod->theAreaInfo;

	FillPixelArray(&dismod->rport[i],0,0,dismod->width,dismod->height,0);

	i++;
    }
    return TTRUE;
}

/********************************************************************************
	Display DestroyDirectBitmap
 ********************************************************************************/
TVOID Dismod_DestroyDirectBitmap( TMOD_DISMOD *dismod )
{
    TINT i=0;
    TINT numbufs=1;

    if(dismod->dblbuf)
	    numbufs=2;

    while(i<numbufs)
    {
	if(dismod->rport[i].BitMap)
	    FreeBitMap(dismod->rport[i].BitMap), dismod->rport[i].BitMap=TNULL;

	i++;
    }

    if(dismod->tmprasbuf)
	TExecFree(dismod->exec,dismod->tmprasbuf), dismod->tmprasbuf=TNULL;
}

/**************************************************************************
  ParseBufferFormat
 **************************************************************************/
TVOID Dismod_ParseBufferFormat( TMOD_DISMOD *dismod )
{
    TUINT format=GetCyberMapAttr(dismod->rport[0].BitMap,CYBRMATTR_PIXFMT);

    switch(format)
    {
	case PIXFMT_LUT8:
	    dismod->bufferformat=IMGFMT_CLUT;
	    dismod->bufferdepth=8;
	break;

	case PIXFMT_RGB15:
	    dismod->bufferformat=IMGFMT_R5G5B5;
	    dismod->bufferdepth=15;
	break;

	case PIXFMT_RGB16:
	    dismod->bufferformat=IMGFMT_R5G6B5;
	    dismod->bufferdepth=16;
	break;

	case PIXFMT_RGB24:
	    dismod->bufferformat=IMGFMT_R8G8B8;
	    dismod->bufferdepth=24;
	break;

	case PIXFMT_BGR24:
	    dismod->bufferformat=IMGFMT_B8G8R8;
	    dismod->bufferdepth=24;
	break;

	case PIXFMT_ARGB32:
	    dismod->bufferformat=IMGFMT_A8R8G8B8;
	    dismod->bufferdepth=32;
	break;

	case PIXFMT_BGRA32:
	    dismod->bufferformat=IMGFMT_B8G8R8A8;
	    dismod->bufferdepth=32;
	break;

	case PIXFMT_RGBA32:
	    dismod->bufferformat=IMGFMT_R8G8B8A8;
	    dismod->bufferdepth=32;
	break;
    }
}

/**************************************************************************
	PutImage
 **************************************************************************/
TVOID Dismod_PutImage(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
    if(img->format!=IMGFMT_R8G8B8 &&
       img->format!=IMGFMT_A8R8G8B8 &&
       img->format!=IMGFMT_R8G8B8A8 )
    {
	TIMGPICTURE bufimg;
	TTAGITEM convtags[5];

	if(img->format==IMGFMT_CLUT && !img->palette)
	    return;

	if(bm)
	{
	    dismod_end_bm(dismod,bm);
	    if(!(dismod_lock_bm(dismod,bm,&bufimg)))
		return;
	}
	else
	{
	    dismod_end_dis(dismod);
	    if(!(dismod_lock_dis(dismod,&bufimg)))
		return;
	}

	convtags[0].tti_Tag = IMGTAG_SRCX;      convtags[0].tti_Value = (TTAG)(src->x);
	convtags[1].tti_Tag = IMGTAG_SRCY;      convtags[1].tti_Value = (TTAG)(src->y);
	convtags[2].tti_Tag = IMGTAG_DSTX;      convtags[2].tti_Value = (TTAG)(dst->x);
	convtags[3].tti_Tag = IMGTAG_DSTY;      convtags[3].tti_Value = (TTAG)(dst->y);
	convtags[4].tti_Tag = TTAG_DONE;

	TImgDoMethod(dismod->imgp,img,&bufimg,IMGMT_CONVERT,convtags);

	if(bm)
	{
	    dismod_unlock_bm(dismod,bm);
	    dismod_begin_bm(dismod,bm);
	}
	else
	{
	    dismod_unlock_dis(dismod);
	    dismod_begin_dis(dismod);
	}
    }
    else
    {
	struct RastPort *rp;
	UBYTE cgfxformat = ~0;

	switch(img->format)
	{
	    case IMGFMT_R8G8B8:
		cgfxformat=RECTFMT_RGB;
	    break;

	    case IMGFMT_A8R8G8B8:
		cgfxformat=RECTFMT_ARGB;
	    break;

	    case IMGFMT_R8G8B8A8:
		cgfxformat=RECTFMT_RGBA;
	    break;
	}

	if(bm)
	{
	    rp=&dismod->theBitmap->rport;
	}
	else
	{
	    rp=&dismod->rport[dismod->workbuf];
	}

	WritePixelArray( img->data,
			 src->x,src->y,img->bytesperrow,
			 rp,
			 dst->x,dst->y,
			 src->width,src->height,
			 cgfxformat);
    }
}

/**************************************************************************
  Display SetPointerMode
 **************************************************************************/
TVOID Dismod_SetPointerMode(TMOD_DISMOD *dismod,TINT mode)
{
    switch(mode)
    {
	case TDISPTR_NORMAL:
	    SetWindowPointer( dismod->theWindow,
			      WA_Pointer,NULL,
			      WA_BusyPointer,FALSE,
			      TAG_DONE);
	break;

	case TDISPTR_BUSY:
	    SetWindowPointer( dismod->theWindow,
			      WA_Pointer,NULL,
			      WA_BusyPointer,TRUE,
			      TAG_DONE);
	break;

	case TDISPTR_INVISIBLE:
	    SetWindowPointer( dismod->theWindow,
			      WA_Pointer,(TUINT)dismod->MousePointer,
			      WA_BusyPointer,FALSE,
			      TAG_DONE);
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

