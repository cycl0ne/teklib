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

#define DIRECTDRAW_VERSION 0x0300
#define DIRECTINPUT_VERSION 0x0300
#include <windows.h>
#include <time.h>
#include <ddraw.h>
#include <dinput.h>
#include <math.h>

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
	TINT    width, height,bytesperrow,depth;
	TBOOL   dblbuf;
	TINT    window_xpos,window_ypos;
	TBOOL   window_ready;
	TINT    bufferdepth,bufferpixelsize;
	TUINT   bufferformat;
	TINT    workbuf;

	TINT numdismodes;
	TDISMODE *modelist;

	TINT bmwidth, bmheight;
	TINT font_w, font_h;

	TINT    ptrmode;
	TBOOL	deltamouse;
	TBOOL	vsync,smoothscale;

	TIMGARGBCOLOR *theDrawPen;

	TINT keyqual;

	/* win32 typical variables */
	TBOOL isWin9x;
	HWND hwnd;
	RECT window_rect;
	LPDIRECTINPUT                   di; 
	LPDIRECTINPUTDEVICE             dikbDevice, dimouseDevice;
	TUINT8 keystate[256], keymap[256];
	TBOOL newmsgloop;
	DIMOUSESTATE mousestate;

	LPDIRECTDRAW                    dd;
	LPDIRECTDRAWSURFACE             FrontBuffer;
	LPDIRECTDRAWSURFACE             DismodBuffer[2];
	RECT                            m_rcScreenRect;     // Screen rect for window
	RECT							m_rcViewportRect;   // Offscreen rect for VPort
	IDirectDrawPalette     *ddpal;
	PALETTEENTRY                    ape[256];
	ATOM    wclass;
	HFONT AppFont;
	HCURSOR mousepointer;
	HDC dismodHDC,bitmapHDC;
	HPEN theDrawPenWin32;
	HPEN theOldDrawPenWin32;
	HBRUSH theFillBrushWin32;
	HBRUSH theEmptyBrushWin32;
	HBRUSH theOldDrawBrushWin32;
	
	struct myBitmapInfo
	{
		BITMAPV4HEADER bmiHeader;
		RGBQUAD bmiColors[256];

	} bitmapinfo;

	TBOOL fullscreen;
	TINT keycount;

} TMOD_DISMOD;

/* private prototypes */
TBOOL Dismod_ReadProperties( TMOD_DISMOD *dismod );
TBOOL Dismod_CreateWindow(TMOD_DISMOD *dismod);
TVOID Dismod_DestroyWindow(TMOD_DISMOD *dismod);
TBOOL Dismod_CreateDirectDraw(TMOD_DISMOD *dismod);
TVOID Dismod_DestroyDirectDraw(TMOD_DISMOD *dismod);
TBOOL Dismod_CreateDirectDrawBuffers( TMOD_DISMOD *dismod );
TVOID Dismod_DestroyDirectDrawBuffers( TMOD_DISMOD *dismod );
TVOID Dismod_ParseBufferFormat( TMOD_DISMOD *dismod );
TVOID Dismod_RestoreSurfaces( TMOD_DISMOD *dismod );
TVOID Dismod_PutImage(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst);
TVOID Dismod_ProcessMessage(TMOD_DISMOD *dismod,TDISMSG *dismsg);
LRESULT CALLBACK Dismod_WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
HRESULT WINAPI Dismod_ModeListCallback(LPDDSURFACEDESC pdds, LPVOID lParam);

/* global module init stuff */
#include "../modinit.h"

/* GDI drawing routines */
#include "gdidrawdisplay.h"
#include "gdidrawbitmap.h"

/* standard message callback */
#include "win32common.h"

/**************************************************************************
	tek_init
 **************************************************************************/
TMODENTRY TUINT tek_init_display_full_std(TAPTR selftask, TMOD_DISMOD *mod, TUINT16 version, TTAGITEM *tags)
{
	return Dismod_InitMod(selftask,mod,version,tags);
}

/**************************************************************************
	dismod_create
 **************************************************************************/
TMODAPI TBOOL dismod_create(TMOD_DISMOD *dismod, TSTRPTR title, TINT x, TINT y, TINT w, TINT h, TINT d, TUINT flags)
{
	dismod->width=w;
	dismod->height=h;
	dismod->depth=d;

	dismod->fullscreen=TTRUE;

	if((flags & TDISCF_DOUBLEBUFFER)==TDISCF_DOUBLEBUFFER)
		dismod->dblbuf=TTRUE;
	else
		dismod->dblbuf=TFALSE;

	dismod->windowname=TExecAlloc0(dismod->exec,TNULL,TUtilStrLen(dismod->util,title)+1);
	strcpy(dismod->windowname,title);

	if(Dismod_CreateWindow(dismod))
	{
		if(Dismod_CreateDirectDraw(dismod))
		{
			if(Dismod_CreateDirectDrawBuffers(dismod))
			{
				TExecFillMem(dismod->exec,dismod->keystate,256,0);
				dismod->keyqual=0;

				dismod->ptrmode     = TDISPTR_NORMAL;
				dismod->deltamouse	 = TFALSE;
				dismod->vsync		 = TFALSE;
				dismod->smoothscale = TFALSE;
				dismod->newmsgloop = TTRUE;

				Dismod_ParseBufferFormat(dismod);
				dismod->theEmptyBrushWin32=GetStockObject(NULL_BRUSH);

				return TTRUE;
			}
		}
	}
	return TFALSE;
}

/**************************************************************************
	dismod_destroy
 **************************************************************************/
TMODAPI TVOID dismod_destroy(TMOD_DISMOD *dismod)
{
	if(dismod->AppFont)
	{
		DeleteObject(dismod->AppFont);
		dismod->AppFont=NULL;
	}

	Dismod_DestroyDirectDrawBuffers(dismod);
	Dismod_DestroyDirectDraw(dismod);
	Dismod_DestroyWindow(dismod);

	if(dismod->modelist)
	{
		TExecFree(dismod->exec,dismod->modelist);
		dismod->modelist=TNULL;
	}

	if(dismod->windowname)
	{
		TExecFree(dismod->exec,dismod->windowname);
		dismod->windowname=TNULL;
	}
}

/**************************************************************************
	dismod_getproperties
 **************************************************************************/
TMODAPI TVOID dismod_getproperties(TMOD_DISMOD *dismod, TDISPROPS *props)
{
	props->version=DISPLAYHANDLER_VERSION;
	props->priority=0;
	props->dispclass=TDISCLASS_STANDARD;
	props->dispmode=TDISMODE_FULLSCREEN;
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

	caps->blitscale=TTRUE;
	caps->blitalpha=TTRUE;
	caps->blitckey=TTRUE;

	caps->canconvertdisplay=TTRUE;
	caps->canconvertscaledisplay=TTRUE;
	caps->canconvertbitmap=TTRUE;
	caps->canconvertscalebitmap=TTRUE;
	caps->candrawbitmap=TTRUE;
}

/**************************************************************************
	dismod_getmodelist
 **************************************************************************/
TMODAPI TINT dismod_getmodelist(TMOD_DISMOD *dismod, TDISMODE **modelist)
{
	*modelist=dismod->modelist;
	return dismod->numdismodes;
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
			Dismod_CreateDeltaMouse(dismod);
			ptr = TDISPTR_INVISIBLE;
		}
		else
			Dismod_DestroyDeltaMouse(dismod);

		dismod->deltamouse=dm;
	}

	switch(ptr)
	{
		case TDISPTR_NORMAL:
			dismod->mousepointer=LoadCursor(NULL,IDC_ARROW);
		break;

		case TDISPTR_BUSY:
			dismod->mousepointer=LoadCursor(NULL,IDC_WAIT);
		break;
		
		case TDISPTR_INVISIBLE:
			dismod->mousepointer=0;
		break;
	}
	SetCursor(dismod->mousepointer);
}

/**************************************************************************
	dismod_flush
 **************************************************************************/
TMODAPI TVOID dismod_flush(TMOD_DISMOD *dismod)
{
	if(dismod->vsync)
		IDirectDraw_WaitForVerticalBlank(dismod->dd,DDWAITVB_BLOCKBEGIN,NULL);

	IDirectDrawSurface_BltFast(dismod->FrontBuffer,0,0,dismod->DismodBuffer[dismod->workbuf], NULL, DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT );
	
	if(dismod->dblbuf)
		dismod->workbuf=1 - dismod->workbuf;
}

/**************************************************************************
	dismod_allocpen
 **************************************************************************/
TMODAPI TBOOL dismod_allocpen(TMOD_DISMOD *dismod, TDISPEN *pen)
{
	HPEN *hpen=TExecAlloc0(dismod->exec,TNULL,sizeof(HPEN));
	*hpen = CreatePen(PS_SOLID, 1, RGB(pen->color.r, pen->color.g, pen->color.b));
	pen->hostdata=(TAPTR)hpen;
	return TTRUE;
}

/**************************************************************************
	dismod_freepen
 **************************************************************************/
TMODAPI TVOID dismod_freepen(TMOD_DISMOD *dismod, TDISPEN *pen)
{
	HPEN *hpen=(HPEN*)pen->hostdata;
    DeleteObject(*hpen);
	TExecFree(dismod->exec,hpen);
}

/**************************************************************************
	dismod_setdpen
 **************************************************************************/
TMODAPI TVOID dismod_setdpen(TMOD_DISMOD *dismod, TDISPEN *pen)
{
	HPEN *hpen=(HPEN*)pen->hostdata;

	dismod->theDrawPen=&pen->color;
	dismod->theDrawPenWin32=*hpen;

	if(dismod->dismodHDC)
	{
		if(!dismod->theOldDrawPenWin32)
			dismod->theOldDrawPenWin32=SelectObject(dismod->dismodHDC,dismod->theDrawPenWin32);
		else
			SelectObject(dismod->dismodHDC,dismod->theDrawPenWin32);

		if(!dismod->theOldDrawBrushWin32)
			dismod->theOldDrawBrushWin32=SelectObject(dismod->dismodHDC,dismod->theEmptyBrushWin32);
		else
			SelectObject(dismod->dismodHDC,dismod->theEmptyBrushWin32);

		if(dismod->theFillBrushWin32)
			DeleteObject(dismod->theFillBrushWin32);

		dismod->theFillBrushWin32=CreateSolidBrush(RGB(pen->color.r, pen->color.g, pen->color.b));
	}
	else if(dismod->bitmapHDC)
	{
		if(!dismod->theOldDrawPenWin32)
			dismod->theOldDrawPenWin32=SelectObject(dismod->bitmapHDC,dismod->theDrawPenWin32);
		else
			SelectObject(dismod->bitmapHDC,dismod->theDrawPenWin32);

		if(!dismod->theOldDrawBrushWin32)
			dismod->theOldDrawBrushWin32=SelectObject(dismod->bitmapHDC,dismod->theEmptyBrushWin32);
		else
			SelectObject(dismod->bitmapHDC,dismod->theEmptyBrushWin32);

		if(dismod->theFillBrushWin32)
			DeleteObject(dismod->theFillBrushWin32);

		dismod->theFillBrushWin32=CreateSolidBrush(RGB(pen->color.r, pen->color.g, pen->color.b));
	}
}

/**************************************************************************
	dismod_setpalette
 **************************************************************************/
TMODAPI TBOOL dismod_setpalette(TMOD_DISMOD *dismod, TIMGARGBCOLOR *pal, TINT sp, TINT sd, TINT numentries)
{
	if(dismod->window_ready && dismod->depth==8)
	{
		TINT i;
		for (i=0; i<numentries; i++)
		{
			dismod->ape[i+sd].peRed        =       pal[i+sp].r;
			dismod->ape[i+sd].peGreen      =       pal[i+sp].g;
			dismod->ape[i+sd].peBlue       =       pal[i+sp].b;
			dismod->ape[i+sd].peFlags      =       0;
		}
		IDirectDrawSurface_GetPalette(dismod->FrontBuffer, &dismod->ddpal);
		IDirectDrawPalette_SetEntries(dismod->ddpal,0,0,256,dismod->ape);
		IDirectDrawSurface_SetPalette(dismod->FrontBuffer, dismod->ddpal);
		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	dismod_allocbitmap
 **************************************************************************/
TMODAPI TBOOL dismod_allocbitmap(TMOD_DISMOD *dismod, TDISBITMAP *bitmap, TINT width, TINT height, TINT flags)
{
	HRESULT hr;
	DDSURFACEDESC ddsd;

	LPDIRECTDRAWSURFACE *surface=TExecAlloc0(dismod->exec,TNULL,sizeof(LPDIRECTDRAWSURFACE));

	ZeroMemory( &ddsd, sizeof(DDSURFACEDESC) );
	ddsd.dwSize                 = sizeof(DDSURFACEDESC);
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.dwFlags                            = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwWidth                            = width;
	ddsd.dwHeight                           = height;
	ddsd.ddsCaps.dwCaps                     = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

	if( FAILED( hr = IDirectDraw_CreateSurface( dismod->dd, &ddsd, surface, NULL ) ) )
		return TFALSE;

	bitmap->hostdata=(TAPTR)surface;

	TExecFillMem(dismod->exec, &ddsd,sizeof(ddsd),0);
	ddsd.dwSize = sizeof(ddsd);

	IDirectDrawSurface_GetSurfaceDesc(*surface,&ddsd);

	bitmap->image.width=ddsd.dwWidth;
	bitmap->image.height=ddsd.dwHeight;
	bitmap->image.depth=ddsd.ddpfPixelFormat.dwRGBBitCount;
	bitmap->image.format=dismod->bufferformat;
	bitmap->image.bytesperrow=ddsd.lPitch;

	return TTRUE;
}

/**************************************************************************
	dismod_freebitmap
 **************************************************************************/
TMODAPI TVOID dismod_freebitmap(TMOD_DISMOD *dismod, TDISBITMAP *bitmap)
{
	LPDIRECTDRAWSURFACE *surface=(LPDIRECTDRAWSURFACE*)bitmap->hostdata;
	if (*surface) IDirectDrawSurface_Release(*surface);
	TExecFree(dismod->exec,surface);
}

/**************************************************************************
	dismod_describe_dis
 **************************************************************************/
TMODAPI TVOID dismod_describe_dis(TMOD_DISMOD *dismod, TDISDESCRIPTOR *desc)
{
	DDSURFACEDESC ddsd;
	
	if(dismod->window_ready)
	{
		TExecFillMem(dismod->exec, &ddsd,sizeof(ddsd),0);
		ddsd.dwSize = sizeof(ddsd);

		Dismod_RestoreSurfaces(dismod);
		IDirectDrawSurface_GetSurfaceDesc(dismod->FrontBuffer,&ddsd);

		desc->x=0;
		desc->y=0;
		desc->width=ddsd.dwWidth;
		desc->height=ddsd.dwHeight;
		desc->depth=ddsd.ddpfPixelFormat.dwRGBBitCount;
		desc->format=dismod->bufferformat;
		desc->bytesperrow=ddsd.lPitch;
	}
}

/**************************************************************************
	dismod_describe_bm
 **************************************************************************/
TMODAPI TVOID dismod_describe_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TDISDESCRIPTOR *desc)
{
	DDSURFACEDESC ddsd;
	
	if(dismod->window_ready)
	{
		LPDIRECTDRAWSURFACE *surface=(LPDIRECTDRAWSURFACE*)bm->hostdata;

		if( IDirectDrawSurface_IsLost(*surface) )
			IDirectDrawSurface_Restore(*surface);

		TExecFillMem(dismod->exec, &ddsd,sizeof(ddsd),0);
		ddsd.dwSize = sizeof(ddsd);

		IDirectDrawSurface_GetSurfaceDesc(*surface,&ddsd);

		desc->x=0;
		desc->y=0;
		desc->width=ddsd.dwWidth;
		desc->height=ddsd.dwHeight;
		desc->depth=ddsd.ddpfPixelFormat.dwRGBBitCount;
		desc->format=dismod->bufferformat;
		desc->bytesperrow=ddsd.lPitch;
	}
}

/**************************************************************************
	dismod_lock_dis
 **************************************************************************/
TMODAPI TBOOL dismod_lock_dis(TMOD_DISMOD *dismod, TIMGPICTURE *img)
{
	DDSURFACEDESC ddsd;

	if(dismod->window_ready)
	{
		memset(&ddsd,0,sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		Dismod_RestoreSurfaces(dismod);

		if( dismod->DismodBuffer[dismod->workbuf] )
		{
			if (IDirectDrawSurface_Lock(dismod->DismodBuffer[dismod->workbuf], NULL, &ddsd, DDLOCK_WAIT, NULL) == DD_OK)
			{
				img->bytesperrow=ddsd.lPitch;
				img->width=ddsd.dwWidth;
				img->height=ddsd.dwHeight;
				img->depth=dismod->bufferdepth;
				img->format=dismod->bufferformat;
				img->data=((TUINT8*)ddsd.lpSurface);
				return TTRUE;
			}
		}
	}
	return TFALSE;
}

/**************************************************************************
	dismod_lock_bm
 **************************************************************************/
TMODAPI TBOOL dismod_lock_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img)
{
	DDSURFACEDESC ddsd;

	if(dismod->window_ready)
	{
		LPDIRECTDRAWSURFACE *surface=(LPDIRECTDRAWSURFACE*)bm->hostdata;

		if( IDirectDrawSurface_IsLost(*surface) )
			IDirectDrawSurface_Restore(*surface);

		memset(&ddsd,0,sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		if (IDirectDrawSurface_Lock(*surface, NULL, &ddsd, DDLOCK_WAIT, NULL) == DD_OK)
		{
			if(img)
			{
				img->bytesperrow=ddsd.lPitch;
				img->width=ddsd.dwWidth;
				img->height=ddsd.dwHeight;
				img->depth=dismod->bufferdepth;
				img->format=dismod->bufferformat;
				img->data=((TUINT8*)ddsd.lpSurface);
			}
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
		IDirectDrawSurface_Unlock(dismod->DismodBuffer[dismod->workbuf], NULL);
}

/**************************************************************************
	dismod_unlock_bm
 **************************************************************************/
TMODAPI TVOID dismod_unlock_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
	if(dismod->window_ready)
	{
		LPDIRECTDRAWSURFACE *surface=(LPDIRECTDRAWSURFACE*)bm->hostdata;
		IDirectDrawSurface_Unlock(*surface, NULL);
	}
}

/**************************************************************************
	dismod_begin_dis
 **************************************************************************/
TMODAPI TBOOL dismod_begin_dis(TMOD_DISMOD *dismod)
{
	if(dismod->window_ready && !dismod->dismodHDC)
	{
		Dismod_RestoreSurfaces(dismod);
		if(IDirectDrawSurface_GetDC(dismod->DismodBuffer[dismod->workbuf],&dismod->dismodHDC)==DD_OK)
		{
			if(dismod->theDrawPen)
			{
				dismod->theOldDrawPenWin32=SelectObject(dismod->dismodHDC,dismod->theDrawPenWin32);
				dismod->theOldDrawBrushWin32=SelectObject(dismod->dismodHDC,dismod->theEmptyBrushWin32);
				if(!dismod->theFillBrushWin32)
					dismod->theFillBrushWin32=CreateSolidBrush(RGB(dismod->theDrawPen->r,dismod->theDrawPen->g,dismod->theDrawPen->b));
			}
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
	if(dismod->window_ready && !dismod->bitmapHDC)
	{
		LPDIRECTDRAWSURFACE *surface=(LPDIRECTDRAWSURFACE*)bm->hostdata;

		if( IDirectDrawSurface_IsLost(*surface) )
			IDirectDrawSurface_Restore(*surface);

		if(IDirectDrawSurface_GetDC(*surface,&dismod->bitmapHDC)==DD_OK)
		{
			if(dismod->theDrawPen)
			{
				dismod->theOldDrawPenWin32=SelectObject(dismod->bitmapHDC,dismod->theDrawPenWin32);
				dismod->theOldDrawBrushWin32=SelectObject(dismod->bitmapHDC,dismod->theEmptyBrushWin32);
				if(!dismod->theFillBrushWin32)
					dismod->theFillBrushWin32=CreateSolidBrush(RGB(dismod->theDrawPen->r,dismod->theDrawPen->g,dismod->theDrawPen->b));
			}
			dismod->bmwidth=bm->image.width;
			dismod->bmheight=bm->image.height;
			return TTRUE;
		}
	}
	return TFALSE;
}

/**************************************************************************
	dismod_end_dis
 **************************************************************************/
TMODAPI TVOID dismod_end_dis(TMOD_DISMOD *dismod)
{
	if(dismod->window_ready && dismod->dismodHDC)
	{
		if(dismod->theDrawPen)
		{
			SelectObject(dismod->dismodHDC,dismod->theOldDrawBrushWin32);
			SelectObject(dismod->dismodHDC,dismod->theOldDrawPenWin32);
			dismod->theOldDrawBrushWin32=TNULL;
			dismod->theOldDrawPenWin32=TNULL;

			if(dismod->theFillBrushWin32)
			{
				DeleteObject(dismod->theFillBrushWin32);
				dismod->theFillBrushWin32=TNULL;
			}
		}
		dismod->dismodHDC=TNULL;
		IDirectDrawSurface_ReleaseDC(dismod->DismodBuffer[dismod->workbuf],dismod->dismodHDC);
		GdiFlush();
	}
}

/**************************************************************************
	dismod_end_bm
 **************************************************************************/
TMODAPI TVOID dismod_end_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
	if(dismod->window_ready && dismod->bitmapHDC)
	{
		LPDIRECTDRAWSURFACE *surface=(LPDIRECTDRAWSURFACE*)bm->hostdata;

		if( IDirectDrawSurface_IsLost(*surface) )
			IDirectDrawSurface_Restore(*surface);

		if(dismod->theDrawPen)
		{
			SelectObject(dismod->bitmapHDC,dismod->theOldDrawBrushWin32);
			SelectObject(dismod->bitmapHDC,dismod->theOldDrawPenWin32);
			dismod->theOldDrawBrushWin32=TNULL;
			dismod->theOldDrawPenWin32=TNULL;

			if(dismod->theFillBrushWin32)
			{
				DeleteObject(dismod->theFillBrushWin32);
				dismod->theFillBrushWin32=TNULL;
			}
		}
		dismod->bitmapHDC=TNULL;
		IDirectDrawSurface_ReleaseDC(*surface,dismod->bitmapHDC);
		GdiFlush();
	}
}

/**************************************************************************
	dismod_blit
 **************************************************************************/
TMODAPI TVOID dismod_blit(TMOD_DISMOD *dismod, TDISBITMAP *bm,TDBLITOPS *bops)
{
	DDBLTFX bltfx;
	RECT s,d;
	LPDIRECTDRAWSURFACE *surface=(LPDIRECTDRAWSURFACE*)bm->hostdata;
	TINT bltflags;

	if(dismod->window_ready)
	{
		Dismod_RestoreSurfaces(dismod);

		if( IDirectDrawSurface_IsLost(*surface) )
			IDirectDrawSurface_Restore(*surface);

		ZeroMemory( &bltfx, sizeof(DDBLTFX) );
		bltfx.dwSize=sizeof(DDBLTFX);

		s.left=bops->src.x;
		s.right=s.left+bops->src.width;
		s.top=bops->src.y;
		s.bottom=s.top+bops->src.height;

		d.left=bops->dst.x;
		d.right=d.left+bops->dst.width;
		d.top=bops->dst.y;
		d.bottom=d.top+bops->dst.height;

		bltflags=DDBLT_WAIT;

		if(bops->ckey)
		{
			TIMGARGBCOLOR imgpcol;
			TINT col;

			imgpcol.a=0;
			imgpcol.r=bops->ckey_val.r;
			imgpcol.g=bops->ckey_val.g;
			imgpcol.b=bops->ckey_val.b;

			col=TImgColToFmt(dismod->imgp,&imgpcol,dismod->bufferformat);

			bltfx.ddckSrcColorkey.dwColorSpaceLowValue=col;
			bltfx.ddckSrcColorkey.dwColorSpaceHighValue=col;
			bltflags |= DDBLT_KEYSRCOVERRIDE;
		}
		IDirectDrawSurface_Blt( dismod->DismodBuffer[dismod->workbuf], &d, *surface, &s, bltflags, &bltfx);
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
	if(dismod->window_ready)
	{
		Dismod_PutImage(dismod,bm,img,src,dst);
	}
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
	HDC hdc;
	OSVERSIONINFO osinfo;
	LPDIRECTDRAW dd;
	TINT i;

	ZeroMemory(&osinfo,sizeof(OSVERSIONINFO));
	osinfo.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&osinfo);
	if(osinfo.dwMajorVersion<5)
		dismod->isWin9x=TTRUE;
	else
		dismod->isWin9x=TFALSE;

	/* check if we have DDRAW, also needed for enumeration of dismodmodes */
	if(DirectDrawCreate( NULL, &dd, NULL ) != DD_OK)
		return TFALSE;

	/* list dismodmodes */
	dismod->numdismodes=0;
	dismod->modelist=TExecAlloc0(dismod->exec,TNULL,sizeof(TDISMODE));
	IDirectDraw_EnumDisplayModes(dd,0,NULL,dismod,Dismod_ModeListCallback);

	IDirectDraw_Release(dd);

	/* scan list for min & max values */
	dismod->mindepth=100;
	dismod->maxdepth=0;
	dismod->minwidth=100000;
	dismod->maxwidth=0;
	dismod->minheight=100000;
	dismod->maxheight=0;
	for(i=0;i<dismod->numdismodes;i++)
	{
		if(dismod->modelist[i].depth<dismod->mindepth)
			dismod->mindepth=dismod->modelist[i].depth;

		if(dismod->modelist[i].depth>dismod->maxdepth)
			dismod->maxdepth=dismod->modelist[i].depth;

		if(dismod->modelist[i].width<dismod->minwidth)
			dismod->minwidth=dismod->modelist[i].width;

		if(dismod->modelist[i].width>dismod->maxwidth)
			dismod->maxwidth=dismod->modelist[i].width;

		if(dismod->modelist[i].height<dismod->minheight)
			dismod->minheight=dismod->modelist[i].height;

		if(dismod->modelist[i].height>dismod->maxheight)
			dismod->maxheight=dismod->modelist[i].height;
	}
	dismod->defaultwidth=640;
	dismod->defaultheight=480;

	hdc=GetDC(NULL);
	dismod->defaultdepth=GetDeviceCaps(hdc, BITSPIXEL);
	ReleaseDC(NULL,hdc);

	// set some variables to default values
	dismod->window_ready=TFALSE;
	dismod->dismodHDC=TNULL;
	dismod->bitmapHDC=TNULL;

	return TTRUE;
}

/**************************************************************************
	CreateWindow
 **************************************************************************/
TBOOL Dismod_CreateWindow(TMOD_DISMOD *dismod)
{
	WNDCLASSEX cls;
	DWORD windowflags;
	char buf[20];
	TEXTMETRIC tm;
	HDC hdc;
	LARGE_INTEGER t;

	QueryPerformanceCounter(&t);
	_itoa(t.LowPart,buf,10);

	cls.cbSize                      = sizeof(WNDCLASSEX);
	cls.hCursor                     = NULL;
	cls.hIcon                       = NULL;
	cls.hIconSm                     = NULL;
	cls.lpszMenuName        = NULL;
	cls.lpszClassName       = buf;
	cls.hbrBackground       = (HBRUSH)GetStockObject(BLACK_BRUSH);
	cls.hInstance           = GetModuleHandle(NULL);
	cls.style                       = CS_HREDRAW | CS_VREDRAW;
	cls.lpfnWndProc         = (WNDPROC)Dismod_WndProc;
	cls.cbClsExtra          = 0;
	cls.cbWndExtra          = 0;

	dismod->wclass=RegisterClassEx(&cls);
	if(!dismod->wclass) return TFALSE;

	windowflags=WS_SYSMENU | WS_MINIMIZEBOX | WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	dismod->hwnd =CreateWindow(buf,
								dismod->windowname,
								WS_POPUP,
								0,0,
								dismod->width, dismod->height,
								NULL,
								NULL,
								NULL,
								NULL);

	if(!dismod->hwnd) return TFALSE;

	hdc=GetDC(dismod->hwnd);
	GetTextMetrics(hdc,&tm);
	ReleaseDC(dismod->hwnd,hdc);
	dismod->font_w=tm.tmAveCharWidth;
	dismod->font_h=tm.tmAscent;

	SetWindowLong(dismod->hwnd, GWL_USERDATA, (long)dismod);
	ShowWindow(dismod->hwnd,SW_SHOW);
	UpdateWindow(dismod->hwnd);
	SetForegroundWindow(dismod->hwnd);

	dismod->mousepointer=LoadCursor(NULL,IDC_ARROW);

	WaitMessage();
	dismod->window_ready=TTRUE;
	
	// create directinput for keyboard
	if( FAILED( DirectInputCreate(NULL, DIRECTINPUT_VERSION, &dismod->di, NULL )))
		return TFALSE;

	if( FAILED(IDirectInput_CreateDevice( dismod->di, &GUID_SysKeyboard,&dismod->dikbDevice, NULL)))
		return TFALSE;

	IDirectInputDevice_SetDataFormat(dismod->dikbDevice,&c_dfDIKeyboard);
	IDirectInputDevice_SetCooperativeLevel(dismod->dikbDevice,dismod->hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);         
	IDirectInputDevice_Acquire(dismod->dikbDevice);

	return TTRUE;
}

/**************************************************************************
	DestroyWindow
 **************************************************************************/
TVOID Dismod_DestroyWindow(TMOD_DISMOD *dismod)
{
	Dismod_DestroyDeltaMouse(dismod);

	if(dismod->dikbDevice)
	{
		IDirectInputDevice_Unacquire(dismod->dikbDevice); 
		IDirectInputDevice_Release(dismod->dikbDevice);
		dismod->dikbDevice=TNULL;
	}

	if(dismod->di)
	{
		IDirectInput_Release(dismod->di);
		dismod->di=TNULL;
	}

	if( dismod->hwnd )
	{
		SetWindowLong(dismod->hwnd, GWL_USERDATA, 0);
		DestroyWindow(dismod->hwnd);
		UnregisterClass((LPCTSTR) (TUINT32) dismod->wclass, NULL);
		dismod->hwnd = NULL;
		dismod->window_ready=TFALSE;
	}
}

/**************************************************************************
	CreateDirectDraw
 **************************************************************************/
TBOOL Dismod_CreateDirectDraw(TMOD_DISMOD *dismod)
{
	if( FAILED( DirectDrawCreate( NULL, &dismod->dd, NULL ) ) )
		return TFALSE;
	else
	{
		if(FAILED(IDirectDraw_SetCooperativeLevel(dismod->dd,dismod->hwnd,DDSCL_ALLOWREBOOT|DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN)))
			return TFALSE;
	}
	return TTRUE;
}

/**************************************************************************
	DestroyDirectDraw
 **************************************************************************/
TVOID Dismod_DestroyDirectDraw(TMOD_DISMOD *dismod)
{
	if(dismod->dd)
	{
		IDirectDraw_SetCooperativeLevel( dismod->dd, dismod->hwnd, DDSCL_NORMAL );
		IDirectDraw_RestoreDisplayMode(dismod->dd);
		IDirectDraw_Release(dismod->dd);
		dismod->dd=NULL;
	}
}

/**************************************************************************
	CreateDirectDrawBuffers
 **************************************************************************/
TBOOL Dismod_CreateDirectDrawBuffers( TMOD_DISMOD *dismod )
{
	HRESULT hr;

	DDSURFACEDESC ddsd;

	if (dismod->DismodBuffer[0]) IDirectDrawSurface_Release(dismod->DismodBuffer[0]), dismod->DismodBuffer[0] = NULL;
	if (dismod->DismodBuffer[1]) IDirectDrawSurface_Release(dismod->DismodBuffer[1]), dismod->DismodBuffer[1] = NULL;
	if (dismod->FrontBuffer) IDirectDrawSurface_Release(dismod->FrontBuffer), dismod->FrontBuffer = NULL;

	if( FAILED( IDirectDraw_SetDisplayMode(dismod->dd,dismod->width,dismod->height,dismod->depth)) )
		return TFALSE;

	ZeroMemory( &ddsd, sizeof(DDSURFACEDESC) );
	ddsd.dwSize                 = sizeof(DDSURFACEDESC);
	ddsd.dwFlags                = DDSD_CAPS;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.ddsCaps.dwCaps         = DDSCAPS_PRIMARYSURFACE;

	if( FAILED( hr = IDirectDraw_CreateSurface( dismod->dd, &ddsd, &dismod->FrontBuffer, NULL ) ) )
		return TFALSE;

	ddsd.dwFlags        = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwWidth        = dismod->width;
	ddsd.dwHeight       = dismod->height;
	ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN;

	if( FAILED( hr = IDirectDraw_CreateSurface( dismod->dd, &ddsd, &dismod->DismodBuffer[0], NULL ) ) )
		return TFALSE;

	if(dismod->dblbuf)
	{
		ddsd.dwFlags        = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
		ddsd.dwWidth        = dismod->width;
		ddsd.dwHeight       = dismod->height;
		ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN;

		if( FAILED( hr = IDirectDraw_CreateSurface( dismod->dd, &ddsd, &dismod->DismodBuffer[1], NULL ) ) )
			return TFALSE;
	}

	if(dismod->depth==8)
	{
		IDirectDraw_CreatePalette(dismod->dd, DDPCAPS_8BIT | DDPCAPS_ALLOW256, dismod->ape, &dismod->ddpal, NULL);
		IDirectDrawSurface_SetPalette( dismod->FrontBuffer, dismod->ddpal );
	}
	return TTRUE;
}

/**************************************************************************
	DestroyDirectDrawBuffers
 **************************************************************************/
TVOID Dismod_DestroyDirectDrawBuffers( TMOD_DISMOD *dismod )
{
	if(dismod->depth==8)
	{
		if(dismod->ddpal) IDirectDrawPalette_Release(dismod->ddpal), dismod->ddpal = NULL;
	}
	if (dismod->DismodBuffer[0]) IDirectDrawSurface_Release(dismod->DismodBuffer[0]), dismod->DismodBuffer[0] = NULL;
	if (dismod->DismodBuffer[1]) IDirectDrawSurface_Release(dismod->DismodBuffer[1]), dismod->DismodBuffer[1] = NULL;
	if (dismod->FrontBuffer) IDirectDrawSurface_Release(dismod->FrontBuffer), dismod->FrontBuffer = NULL;
}

/**************************************************************************
  ParseBufferFormat
 **************************************************************************/
TVOID Dismod_ParseBufferFormat( TMOD_DISMOD *dismod )
{
	DDPIXELFORMAT pf;

	TExecFillMem(dismod->exec, &pf,sizeof(pf),0);
	pf.dwSize = sizeof(pf);

	IDirectDrawSurface_GetPixelFormat(dismod->FrontBuffer,&pf);
	
	switch(pf.dwRGBBitCount)
	{
		case 8:
			dismod->bufferpixelsize=1;
			dismod->bufferdepth=8;
			dismod->bufferformat=IMGFMT_CLUT;
		break;

		case 16:
			if(pf.dwRBitMask==0x00007c00 && pf.dwGBitMask==0x000003e0 && pf.dwBBitMask==0x0000001f)
			{
				dismod->bufferdepth=15;
				dismod->bufferformat=IMGFMT_R5G5B5;
			}
			else if(pf.dwRBitMask==0x0000f800 && pf.dwGBitMask==0x000007e0 && pf.dwBBitMask==0x0000001f)
			{
				dismod->bufferdepth=16;
				dismod->bufferformat=IMGFMT_R5G6B5;
			}
			dismod->bufferpixelsize=2;
		break;

		case 24:
			if(pf.dwRBitMask==0x00ff0000 && pf.dwGBitMask==0x0000ff00 && pf.dwBBitMask==0x000000ff)
			{
				dismod->bufferdepth=24;
				dismod->bufferformat=IMGFMT_B8G8R8;
			}
			else if(pf.dwRBitMask==0x000000ff && pf.dwGBitMask==0x0000ff00 && pf.dwBBitMask==0x00ff0000)
			{
				dismod->bufferdepth=24;
				dismod->bufferformat=IMGFMT_R8G8B8;
			}
			dismod->bufferpixelsize=3;
		break;

		case 32:
			if(pf.dwRBitMask==0x00ff0000 && pf.dwGBitMask==0x0000ff00 && pf.dwBBitMask==0x000000ff)
			{
				dismod->bufferdepth=32;
				dismod->bufferformat=IMGFMT_A8R8G8B8;
			}
			else if(pf.dwRBitMask==0x0000ff00 && pf.dwGBitMask==0x00ff0000 && pf.dwBBitMask==0xff000000)
			{
				dismod->bufferdepth=32;
				dismod->bufferformat=IMGFMT_B8G8R8A8;
			}
			else if(pf.dwRBitMask==0xff000000 && pf.dwGBitMask==0x00ff0000 && pf.dwBBitMask==0x0000ff00)
			{
				dismod->bufferdepth=32;
				dismod->bufferformat=IMGFMT_R8G8B8A8;
			}
			dismod->bufferpixelsize=4;
		break;
	}
}

/**************************************************************************
  RestoreSurfaces
 **************************************************************************/
TVOID Dismod_RestoreSurfaces( TMOD_DISMOD *dismod )
{
	if(dismod->window_ready)
	{
		if( dismod->FrontBuffer )
		{
			if( IDirectDrawSurface_IsLost(dismod->FrontBuffer) )
				IDirectDrawSurface_Restore(dismod->FrontBuffer);
		}
		if( dismod->DismodBuffer[0] )
		{
			if( IDirectDrawSurface_IsLost(dismod->DismodBuffer[0]) )
				IDirectDrawSurface_Restore(dismod->DismodBuffer[0]);
		}
		if( dismod->DismodBuffer[1] )
		{
			if( IDirectDrawSurface_IsLost(dismod->DismodBuffer[1]) )
				IDirectDrawSurface_Restore(dismod->DismodBuffer[1]);
		}
	}
}

/**************************************************************************
	PutImage
 **************************************************************************/
TVOID Dismod_PutImage(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	TBOOL emul=TFALSE;

	if(img->format==IMGFMT_R8G8B8 || img->format==IMGFMT_B8G8R8 || img->format==IMGFMT_CLUT)
		emul=TTRUE;

	if(dismod->isWin9x)
	{
		if(img->format==IMGFMT_R8G8B8A8 || img->format==IMGFMT_B8G8R8A8)
			emul=TTRUE;
	}

	if(emul)
	{
		TIMGPICTURE bufimg;

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

		if(src->width==dst->width && src->height==dst->height)
		{
			TTAGITEM convtags[7];

			convtags[0].tti_Tag = IMGTAG_SRCX;      convtags[0].tti_Value = (TTAG)(src->x);
			convtags[1].tti_Tag = IMGTAG_SRCY;      convtags[1].tti_Value = (TTAG)(src->y);
			convtags[2].tti_Tag = IMGTAG_DSTX;      convtags[2].tti_Value = (TTAG)(dst->x);
			convtags[3].tti_Tag = IMGTAG_DSTY;      convtags[3].tti_Value = (TTAG)(dst->y);
			convtags[4].tti_Tag = IMGTAG_WIDTH;     convtags[4].tti_Value = (TTAG)(src->width);
			convtags[5].tti_Tag = IMGTAG_HEIGHT;    convtags[5].tti_Value = (TTAG)(src->height);
			convtags[6].tti_Tag = TTAG_DONE;

			TImgDoMethod(dismod->imgp,img,&bufimg,IMGMT_CONVERT,convtags);
		}
		else
		{
			TTAGITEM scaletags[10];

			scaletags[0].tti_Tag = IMGTAG_SRCX;                     scaletags[0].tti_Value = (TTAG)(src->x);
			scaletags[1].tti_Tag = IMGTAG_SRCY;                     scaletags[1].tti_Value = (TTAG)(src->y);
			scaletags[2].tti_Tag = IMGTAG_DSTX;                     scaletags[2].tti_Value = (TTAG)(dst->x);
			scaletags[3].tti_Tag = IMGTAG_DSTY;                     scaletags[3].tti_Value = (TTAG)(dst->y);
			scaletags[4].tti_Tag = IMGTAG_WIDTH;            scaletags[4].tti_Value = (TTAG)(src->width);
			scaletags[5].tti_Tag = IMGTAG_HEIGHT;           scaletags[5].tti_Value = (TTAG)(src->height);
			scaletags[6].tti_Tag = IMGTAG_SCALEWIDTH;       scaletags[6].tti_Value = (TTAG)(dst->width);
			scaletags[7].tti_Tag = IMGTAG_SCALEHEIGHT;      scaletags[7].tti_Value = (TTAG)(dst->height);
			scaletags[8].tti_Tag = IMGTAG_SCALEMETHOD;      scaletags[8].tti_Value = (TTAG)IMGSMT_HARD;
			scaletags[9].tti_Tag = TTAG_DONE;

			TImgDoMethod(dismod->imgp,img,&bufimg,IMGMT_SCALE,scaletags);
		}

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
		HDC hdc;

		dismod->bitmapinfo.bmiHeader.bV4Size = sizeof(BITMAPV4HEADER);
		dismod->bitmapinfo.bmiHeader.bV4Width = img->width;
		dismod->bitmapinfo.bmiHeader.bV4Height = -img->height;
		dismod->bitmapinfo.bmiHeader.bV4Planes = 1;
		dismod->bitmapinfo.bmiHeader.bV4BitCount = img->depth;
		dismod->bitmapinfo.bmiHeader.bV4SizeImage = 0;
		dismod->bitmapinfo.bmiHeader.bV4XPelsPerMeter = 1;
		dismod->bitmapinfo.bmiHeader.bV4YPelsPerMeter = 1;
		dismod->bitmapinfo.bmiHeader.bV4ClrUsed = 0;
		dismod->bitmapinfo.bmiHeader.bV4ClrImportant = 0;

		switch(img->format)
		{
			case IMGFMT_CLUT:
				if(img->palette)
				{
					TINT i;
			
					for(i=0;i<pow(2,img->depth);i++)
					{
						dismod->bitmapinfo.bmiColors[i].rgbRed=img->palette[i].r;
						dismod->bitmapinfo.bmiColors[i].rgbGreen=img->palette[i].g;
						dismod->bitmapinfo.bmiColors[i].rgbBlue=img->palette[i].b;
					}
					dismod->bitmapinfo.bmiHeader.bV4BitCount = 8;
					dismod->bitmapinfo.bmiHeader.bV4V4Compression = BI_RGB;
				}
			break;

			case IMGFMT_R5G5B5:
				dismod->bitmapinfo.bmiHeader.bV4V4Compression = BI_BITFIELDS;
				dismod->bitmapinfo.bmiHeader.bV4AlphaMask      = 0x00008000;
				dismod->bitmapinfo.bmiHeader.bV4RedMask        = 0x00007c00;
				dismod->bitmapinfo.bmiHeader.bV4GreenMask      = 0x000003e0;
				dismod->bitmapinfo.bmiHeader.bV4BlueMask       = 0x0000001f;
			break;

			case IMGFMT_R5G6B5:
				dismod->bitmapinfo.bmiHeader.bV4V4Compression = BI_BITFIELDS;
				dismod->bitmapinfo.bmiHeader.bV4RedMask        = 0x0000f800;
				dismod->bitmapinfo.bmiHeader.bV4GreenMask      = 0x000007e0;
				dismod->bitmapinfo.bmiHeader.bV4BlueMask       = 0x0000001f;
			break;

			//case IMGFMT_B8G8R8:
			//	dismod->bitmapinfo.bmiHeader.bV4V4Compression = BI_RGB;
			//break;

			case IMGFMT_A8R8G8B8:
				dismod->bitmapinfo.bmiHeader.bV4V4Compression = BI_RGB;
			break;

			case IMGFMT_R8G8B8A8:
				dismod->bitmapinfo.bmiHeader.bV4V4Compression = BI_BITFIELDS;
				dismod->bitmapinfo.bmiHeader.bV4RedMask        = 0xff000000;
				dismod->bitmapinfo.bmiHeader.bV4GreenMask      = 0x00ff0000;
				dismod->bitmapinfo.bmiHeader.bV4BlueMask       = 0x0000ff00;
				dismod->bitmapinfo.bmiHeader.bV4AlphaMask      = 0x00000000;
			break;

			case IMGFMT_B8G8R8A8:
				dismod->bitmapinfo.bmiHeader.bV4V4Compression = BI_BITFIELDS;
				dismod->bitmapinfo.bmiHeader.bV4BlueMask       = 0xff000000;
				dismod->bitmapinfo.bmiHeader.bV4GreenMask      = 0x00ff0000;
				dismod->bitmapinfo.bmiHeader.bV4RedMask        = 0x0000ff00;
				dismod->bitmapinfo.bmiHeader.bV4AlphaMask      = 0x000000ff;
			break;
		}

		if(bm)
		{
			hdc=dismod->bitmapHDC;
		}
		else
		{
			hdc=dismod->dismodHDC;
		}

		if(src->width==dst->width && src->height==dst->height)
		{
			SetDIBitsToDevice(      hdc,
								dst->x,dst->y,
								src->width,src->height,
								src->x,img->height-src->height-src->y,
								0,img->height,
								img->data,
								(BITMAPINFO*)&dismod->bitmapinfo.bmiHeader,
								DIB_RGB_COLORS);
		}
		else
		{
			StretchDIBits(  hdc,
							dst->x,dst->y,
							dst->width,dst->height,
							src->x,img->height-src->height-src->y,
							src->width,src->height,
							img->data,
							(BITMAPINFO*)&dismod->bitmapinfo.bmiHeader,
							DIB_RGB_COLORS,
							SRCCOPY);
		}
	}
}

/**************************************************************************
  Dismod ProcessMessage
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

/**************************************************************************
	Dismod_ModeListCallback
 **************************************************************************/
HRESULT WINAPI Dismod_ModeListCallback(LPDDSURFACEDESC pdds, LPVOID lParam)
{
	TMOD_DISMOD *dismod=(TMOD_DISMOD*)lParam;

	dismod->modelist=(TDISMODE*)TExecRealloc(dismod->exec,dismod->modelist,sizeof(TDISMODE)*(dismod->numdismodes+1));
    
	dismod->modelist[dismod->numdismodes].width = pdds->dwWidth;
    dismod->modelist[dismod->numdismodes].height = pdds->dwHeight;
    dismod->modelist[dismod->numdismodes].depth = pdds->ddpfPixelFormat.dwRGBBitCount;
	
	dismod->numdismodes++;

	return DDENUMRET_OK;
}
