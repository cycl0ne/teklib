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

#include <windows.h>
#include <time.h>
#include <gl/gl.h>
#include <gl/glu.h>

#include "glfixup.h"

#define DIRECTINPUT_VERSION 0x0300
#include <dinput.h>
#include <math.h>

#define INSTALLABLE_DRIVER_TYPE_MASK  (PFD_GENERIC_ACCELERATED|PFD_GENERIC_FORMAT)

typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval); 
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

/* structure needed for allocpen */
typedef struct
{
	GLfloat r,g,b;
} GLPen;

/* win32 typical structure needed for allocbitmap */
typedef struct
{
	HDC hdc;
	HBITMAP hbm;
	TAPTR bmbits;
	GLuint gltexname;
	TINT w,h;
	TINT ckeycol;
	TINT alphaval;
	TBOOL changed;
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

	TINT    ptrmode;
	TBOOL	deltamouse;
	TBOOL	vsync,smoothscale;

	TIMGARGBCOLOR *theDrawPen;

	TINT8   *windowname;
	TINT    width, height,bytesperrow;
	TBOOL   dblbuf,resize;
	TINT    window_xpos,window_ypos;
	TBOOL   window_ready;
	TINT    bufferdepth,bufferpixelsize;
	TUINT   bufferformat;
	TINT    workbuf;

	TINT numdismodes;
	TDISMODE *modelist;

	TINT bmwidth, bmheight;
	TINT font_w, font_h;

	TINT keyqual;

	/* opengl stuff */
	GLuint	fontlist;
	GLPen	theGlDrawPen;

	/* win32 typical variables */
	TBOOL isWin9x;
	HWND hwnd;
	RECT window_rect;
	LPDIRECTINPUT                   di;
	LPDIRECTINPUTDEVICE             dikbDevice, dimouseDevice;
	TUINT8 keystate[256], keymap[256];
	TBOOL newmsgloop;
	DIMOUSESTATE mousestate;

	HDC bitmapHDC;
	HPEN theDrawPenWin32;
	HPEN theOldDrawPenWin32;
	HBRUSH theFillBrushWin32;
	HBRUSH theEmptyBrushWin32;
	HBRUSH theOldDrawBrushWin32;

	RECT                                    m_rcScreenRect;     // Screen rect for window
	RECT                                    m_rcViewportRect;   // Offscreen rect for VPort
	ATOM    wclass;
	HCURSOR mousepointer;
	TINT    frame_x, frame_y, title_y;
	TINT width_offset, height_offset;
	HDC		hdc;
	HGLRC	lockhrc,drawhrc;

	struct myBitmapInfo
	{
		BITMAPV4HEADER bmiHeader;
		RGBQUAD bmiColors[256];

	} bitmapinfo;

	offscreenBitmap *theBitmap;
	TBOOL fullscreen;
	TINT keycount;

} TMOD_DISMOD;

/* private prototypes */
TBOOL Dismod_ReadProperties( TMOD_DISMOD *dismod );
TBOOL Dismod_CreateWindow(TMOD_DISMOD *dismod);
TVOID Dismod_DestroyWindow(TMOD_DISMOD *dismod);
TBOOL Dismod_CreateOpenGl(TMOD_DISMOD *dismod);
TVOID Dismod_DestroyOpenGl(TMOD_DISMOD *dismod);
TVOID Dismod_PutImageDis(TMOD_DISMOD *dismod, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst);
TVOID Dismod_PutImageBm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst);
TVOID Dismod_ProcessMessage(TMOD_DISMOD *dismod,TDISMSG *dismsg);
LRESULT CALLBACK Dismod_WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

/* global module init stuff */
#include "../modinit.h"

/* OpenGL drawing routines */
#include "../gldrawdisplay.h"

/* GDI drawing routines */
#include "gdidrawbitmap.h"

/* standard message callback */
#include "win32common.h"

/**************************************************************************
	tek_init
 **************************************************************************/
TMODENTRY TUINT tek_init_display_full_gl(TAPTR selftask, TMOD_DISMOD *mod, TUINT16 version, TTAGITEM *tags)
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
	dismod->window_xpos=x;
	dismod->window_ypos=y;

	dismod->fullscreen=TTRUE;

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
		if(Dismod_CreateOpenGl(dismod))
		{
			TExecFillMem(dismod->exec,dismod->keystate,256,0);
			dismod->keyqual=0;

			dismod->ptrmode     = TDISPTR_NORMAL;
			dismod->deltamouse	 = TFALSE;
			dismod->vsync		 = TFALSE;
			dismod->smoothscale = TFALSE;
			dismod->newmsgloop = TTRUE;

			return TTRUE;
		}
	}
	return TFALSE;
}

/**************************************************************************
	dismod_destroy
 **************************************************************************/
TMODAPI TVOID dismod_destroy(TMOD_DISMOD *dismod)
{
	Dismod_DestroyOpenGl(dismod);
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
	props->dispclass=TDISCLASS_OPENGL;
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
	caps->minbmwidth=1;
	caps->minbmheight=1;
	caps->maxbmwidth=256;
	caps->maxbmheight=256;

	if(dismod->window_ready)
	{
		if(wglMakeCurrent(dismod->hdc, dismod->drawhrc))
		{
			GLint val;

			glGetIntegerv(GL_MAX_TEXTURE_SIZE,&val);
			caps->maxbmwidth=val/4;
			caps->maxbmheight=val/4;

			wglMakeCurrent(dismod->hdc,NULL);
		}
	}

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

	if(wglSwapIntervalEXT)
	{
		if(dismod->vsync)
		{
			if(wglMakeCurrent(dismod->hdc, dismod->drawhrc))
			{
				wglSwapIntervalEXT(1);
				wglMakeCurrent(dismod->hdc,NULL);
			}
		}
		else
		{
			if(wglMakeCurrent(dismod->hdc, dismod->drawhrc))
			{
				wglSwapIntervalEXT(0);
				wglMakeCurrent(dismod->hdc,NULL);
			}
		}
	}
}

/**************************************************************************
	dismod_flush
 **************************************************************************/
TMODAPI TVOID dismod_flush(TMOD_DISMOD *dismod)
{
	if(dismod->dblbuf)
		SwapBuffers(dismod->hdc);
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

	if(dismod->bitmapHDC)
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

		dismod->theFillBrushWin32=CreateSolidBrush(RGB(pen->color.r,pen->color.g,pen->color.b));
	}
	else
	{		
		dismod->theGlDrawPen.r=(GLfloat)pen->color.r / 255.0f;
		dismod->theGlDrawPen.g=(GLfloat)pen->color.g / 255.0f;
		dismod->theGlDrawPen.b=(GLfloat)pen->color.b / 255.0f;
	}
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
	BITMAPINFO bmi;
	HDC hdc;

	if(dismod->window_ready)
	{
		offscreenBitmap *bm=TExecAlloc0(dismod->exec,TNULL,sizeof(offscreenBitmap));
	
		bm->w=(TINT)pow(2,(TINT)(log(width)/log(2.0)));
		if(bm->w<width)
			bm->w+=bm->w;

		bm->h=(TINT)pow(2,(TINT)(log(height)/log(2.0)));
		if(bm->h<height)
			bm->h+=bm->h;

		TExecFillMem(dismod->exec,&bmi,sizeof(BITMAPINFO),0);
		bmi.bmiHeader.biSize		=sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth		= bm->w;
		bmi.bmiHeader.biHeight		= -bm->h;
		bmi.bmiHeader.biPlanes		= 1;
		bmi.bmiHeader.biBitCount	= 32;
		bmi.bmiHeader.biCompression	= BI_RGB;

		hdc=GetDC(dismod->hwnd);

		bm->hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bm->bmbits, NULL, (DWORD)0);

		ReleaseDC(dismod->hwnd, hdc);

		bm->hdc = CreateCompatibleDC(NULL);
		if (!bm->hdc)
		{
			DeleteObject(bm->hbm);
			return TFALSE;
		}

		bm->changed=TTRUE;
		bm->ckeycol=-1;
		bm->alphaval=-1;

		SelectObject(bm->hdc, bm->hbm);

		bitmap->hostdata=(TAPTR)bm;
		bitmap->image.width=width;
		bitmap->image.height=height;
		bitmap->image.depth=32;
		bitmap->image.format=IMGFMT_A8R8G8B8;
		bitmap->image.bytesperrow=bm->w*4;

		if(wglMakeCurrent(dismod->hdc, dismod->drawhrc))
		{
			glGenTextures( 1, &bm->gltexname);
			wglMakeCurrent(dismod->hdc,NULL);
		}
		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	dismod_freebitmap
 **************************************************************************/
TMODAPI TVOID dismod_freebitmap(TMOD_DISMOD *dismod, TDISBITMAP *bitmap)
{
	offscreenBitmap *bm=(offscreenBitmap*)bitmap->hostdata;

	if(wglMakeCurrent(dismod->hdc, dismod->drawhrc))
	{
		glDeleteTextures( 1, &bm->gltexname);
		wglMakeCurrent(dismod->hdc,NULL);
	}
	if(bm->hdc) DeleteDC(bm->hdc);
	if(bm->hbm) DeleteObject(bm->hbm);
	TExecFree(dismod->exec,bm);
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
		if(wglMakeCurrent(dismod->hdc, dismod->lockhrc))
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

		img->bytesperrow=bm->image.bytesperrow;
		img->width=bm->image.width;
		img->height=bm->image.height;
		img->depth=bm->image.depth;
		img->format=bm->image.format;
		img->data=dismod->theBitmap->bmbits;

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
		wglMakeCurrent(dismod->hdc,NULL);
	}
}

/**************************************************************************
	dismod_unlock_bm
 **************************************************************************/
TMODAPI TVOID dismod_unlock_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
	dismod->theBitmap->changed=TTRUE;
	dismod->theBitmap=TNULL;
}

/**************************************************************************
	dismod_begin_dis
 **************************************************************************/
TMODAPI TBOOL dismod_begin_dis(TMOD_DISMOD *dismod)
{
	if(dismod->window_ready && dismod->hdc)
	{
		if(wglMakeCurrent(dismod->hdc, dismod->drawhrc))
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
		dismod->bitmapHDC=dismod->theBitmap->hdc;

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
		wglMakeCurrent(dismod->hdc,NULL);
	}
}

/**************************************************************************
	dismod_end_bm
 **************************************************************************/
TMODAPI TVOID dismod_end_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm)
{
	dismod->theBitmap->changed=TTRUE;
	GdiFlush();
	dismod->bitmapHDC=TNULL;
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

		if(wglMakeCurrent(dismod->hdc, dismod->drawhrc))
		{
			GLfloat sx,sy,ex,ey;

			glEnable(GL_TEXTURE_2D);
			glBindTexture( GL_TEXTURE_2D,bitmap->gltexname );

			if(bops->ckey)
			{
				TINT keycol=(bops->ckey_val.r << 16) | (bops->ckey_val.g << 8 ) | bops->ckey_val.b;
				if(keycol != bitmap->ckeycol)
				{
					TINT x,y;
					TINT col,scol;
					TUINT *d;
					TUINT8 *data=(TUINT8*)bitmap->bmbits;
					
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
								*d=col | 0x02000000;;

							d++;
						}
						data += bm->image.bytesperrow;
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
					TUINT8 *data=(TUINT8*)bitmap->bmbits;
					TUINT a=bops->calpha_val<<24;

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
						data += bm->image.bytesperrow;
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
				glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, bitmap->w, bitmap->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, bitmap->bmbits );
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
			wglMakeCurrent(dismod->hdc,NULL);
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
		Dismod_PutImageDis(dismod,img,src,dst);
	}
}

/**************************************************************************
	dismod_putimage_bm
 **************************************************************************/
TMODAPI TVOID dismod_putimage_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	if(dismod->window_ready)
	{
		Dismod_PutImageBm(dismod,bm,img,src,dst);
	}
}

/**************************************************************************
	dismod_putscaleimage_dis
 **************************************************************************/
TMODAPI TVOID dismod_putscaleimage_dis(TMOD_DISMOD *dismod, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	if(dismod->window_ready)
	{
		Dismod_PutImageDis(dismod,img,src,dst);
	}
}

/**************************************************************************
	dismod_putscaleimage_bm
 **************************************************************************/
TMODAPI TVOID dismod_putscaleimage_bm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	if(dismod->window_ready)
	{
		Dismod_PutImageBm(dismod,bm,img,src,dst);
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
	TINT i,j;
	DEVMODE mode;

	ZeroMemory(&osinfo,sizeof(OSVERSIONINFO));
	osinfo.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&osinfo);
	if(osinfo.dwMajorVersion<5)
		dismod->isWin9x=TTRUE;
	else
		dismod->isWin9x=TFALSE;

	/* list dismodmodes */
	i=0;
	dismod->numdismodes=0;

    /* get number of modes */
	TExecFillMem(dismod->exec,&mode,sizeof(DEVMODE),0);
	mode.dmSize=sizeof(DEVMODE);
	mode.dmDriverExtra=0;
	while(EnumDisplaySettings( NULL, i, &mode ))
	{
		if(mode.dmBitsPerPel>=16)
			dismod->numdismodes++;

		TExecFillMem(dismod->exec,&mode,sizeof(DEVMODE),0);
		mode.dmSize=sizeof(DEVMODE);
		mode.dmDriverExtra=0;

		i++;
	}

    if(dismod->numdismodes==0)
		return TFALSE;

    /* get modes */
    dismod->modelist=TExecAlloc0(dismod->exec,TNULL,sizeof(TDISMODE)*dismod->numdismodes);
	i=0;
	j=0;
	while(EnumDisplaySettings( NULL, i, &mode ))
	{
		if(mode.dmBitsPerPel>=16)
		{
			dismod->modelist[j].width=mode.dmPelsWidth;
			dismod->modelist[j].height=mode.dmPelsHeight;
			dismod->modelist[j].depth=mode.dmBitsPerPel;
			j++;
		}
		i++;
	}

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
	
	dismod->defaultwidth=GetSystemMetrics(SM_CXSCREEN);
	dismod->defaultheight=GetSystemMetrics(SM_CYSCREEN);
	hdc=GetDC(NULL);
	dismod->defaultdepth=GetDeviceCaps(hdc, BITSPIXEL);
	ReleaseDC(NULL,hdc);

	// set some variables to default values
	dismod->window_ready=TFALSE;
	dismod->hdc=TNULL;

	return TTRUE;
}

/**************************************************************************
	CreateWindow
 **************************************************************************/
TBOOL Dismod_CreateWindow(TMOD_DISMOD *dismod)
{
	WNDCLASSEX cls;
	char buf[20];
	TEXTMETRIC tm;
	TINT i,j;
	TBOOL res;
	struct _devicemodeA  mode;
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
	cls.style               = CS_HREDRAW | CS_VREDRAW;
	cls.lpfnWndProc         = (WNDPROC)Dismod_WndProc;
	cls.cbClsExtra          = 0;
	cls.cbWndExtra          = 0;

	dismod->wclass=RegisterClassEx(&cls);
	if(!dismod->wclass) return TFALSE;

	
	i=0;
	j=0;
	res=TFALSE;
	while( (EnumDisplaySettings( NULL, i, &mode )) && !res && j<dismod->numdismodes)
	{
		if( (int)mode.dmPelsWidth == dismod->width && 
			(int)mode.dmPelsHeight == dismod->height &&
			(int)mode.dmBitsPerPel >= 16)
		{
			res=TTRUE;
		}
		else
			i++;

		if((int)mode.dmBitsPerPel>=16)
			j++;
	}

	if( res )
	{
		mode.dmFields=DM_PELSWIDTH | DM_PELSHEIGHT;;
		ChangeDisplaySettings( &mode, CDS_FULLSCREEN);
	}

	dismod->hwnd = CreateWindowEx( WS_EX_TOPMOST,
									buf,
									dismod->windowname,
									WS_POPUP | WS_VISIBLE,
									0, 0,
									dismod->width,dismod->height,
									0,
									0,
									NULL,
									0);


	if(!dismod->hwnd) return TFALSE;

	dismod->hdc=GetDC(dismod->hwnd);

	GetTextMetrics(dismod->hdc,&tm);
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
		ChangeDisplaySettings(NULL, 0);
		ReleaseDC(dismod->hwnd,dismod->hdc);
		SetWindowLong(dismod->hwnd, GWL_USERDATA, 0);
		DestroyWindow( dismod->hwnd );
		UnregisterClass((LPCTSTR)(TUINT32)dismod->wclass,NULL);
		dismod->hwnd=NULL;
		dismod->window_ready=TFALSE;
	}
}

/**************************************************************************
	CreateOpenGl
 **************************************************************************/
TBOOL Dismod_CreateOpenGl(TMOD_DISMOD *dismod)
{
	PIXELFORMATDESCRIPTOR p, pfd;
	TINT pID;
	TINT d1,d2;

	if(dismod->defaultdepth>16)
	{
		d1=32;
		d2=16;
	}
	else
	{
		d1=16;
		d2=32;
	}

	TExecFillMem(dismod->exec,&p,sizeof(PIXELFORMATDESCRIPTOR),0);
	p.nSize=sizeof(PIXELFORMATDESCRIPTOR);
	p.nVersion=1;
	p.iLayerType=PFD_MAIN_PLANE;
	p.dwFlags = PFD_DRAW_TO_WINDOW |
				PFD_SUPPORT_OPENGL |
				PFD_GENERIC_ACCELERATED;
	p.iPixelType=PFD_TYPE_RGBA;

	if(dismod->dblbuf)
		p.dwFlags |= PFD_DOUBLEBUFFER;

	p.cColorBits=d1;
	p.cDepthBits=24;
	pID = ChoosePixelFormat(dismod->hdc,&p);
	DescribePixelFormat(dismod->hdc, pID,sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	if( 0!=(INSTALLABLE_DRIVER_TYPE_MASK & pfd.dwFlags ) && 
		INSTALLABLE_DRIVER_TYPE_MASK != (INSTALLABLE_DRIVER_TYPE_MASK & pfd.dwFlags ) )
	{
		p.cColorBits=d2;
		p.cDepthBits=24;
		pID = ChoosePixelFormat(dismod->hdc,&p);
		DescribePixelFormat(dismod->hdc, pID,sizeof(PIXELFORMATDESCRIPTOR),&pfd);
	}

	if( 0!=(INSTALLABLE_DRIVER_TYPE_MASK & pfd.dwFlags ) && 
		INSTALLABLE_DRIVER_TYPE_MASK != (INSTALLABLE_DRIVER_TYPE_MASK & pfd.dwFlags ) )
	{
		p.cColorBits=d1;
		p.cDepthBits=16;
		pID = ChoosePixelFormat(dismod->hdc,&p);
		DescribePixelFormat(dismod->hdc, pID,sizeof(PIXELFORMATDESCRIPTOR),&pfd);
	}

	if(pID)
	{
		SetPixelFormat(dismod->hdc, pID, &pfd);
		dismod->lockhrc=wglCreateContext(dismod->hdc);
		dismod->drawhrc=wglCreateContext(dismod->hdc);
		dismod->bufferdepth=pfd.cColorBits;

		if(wglMakeCurrent(dismod->hdc, dismod->drawhrc))
		{
			dismod->fontlist=glGenLists( 256 );
			wglUseFontBitmaps(dismod->hdc,0,255,dismod->fontlist);

			wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");

			switch(dismod->bufferdepth)
			{
				case 16:
					if(pfd.cRedBits==5 && pfd.cGreenBits==5 && pfd.cBlueBits==5)
						dismod->bufferformat=IMGFMT_R5G5B5;
					else if(pfd.cRedBits==5 && pfd.cGreenBits==6 && pfd.cBlueBits==5)
						dismod->bufferformat=IMGFMT_R5G6B5;
				break;

				case 32:
					if(pfd.cRedShift==16 && pfd.cGreenShift==8 && pfd.cBlueShift==0)
						dismod->bufferformat=IMGFMT_R8G8B8A8;
				break;
			}
	
			wglMakeCurrent(dismod->hdc,NULL);
			if(dismod->bufferformat)
				return TTRUE;
		}
	}
	return TFALSE;
}

/**************************************************************************
	DestroyOpenGL
 **************************************************************************/
TVOID Dismod_DestroyOpenGl(TMOD_DISMOD *dismod)
{
	if(dismod_begin_dis(dismod))
	{
		glDeleteLists(dismod->fontlist,256);
		dismod_end_dis(dismod);
	}
	if(dismod->lockhrc) wglDeleteContext( dismod->lockhrc ), dismod->lockhrc=NULL;
	if(dismod->drawhrc) wglDeleteContext( dismod->drawhrc ), dismod->drawhrc=NULL;
}

/**************************************************************************
	PutImage
 **************************************************************************/
TVOID Dismod_PutImageDis(TMOD_DISMOD *dismod, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
{
	GLint format = 0,type = 0;
	GLfloat rbuf[256],gbuf[256], bbuf[256];
	GLfloat constantAlpha = 1.0;
	TINT i;

	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

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

			format=GL_BGR;
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

	glFinish();
}

/**************************************************************************
	PutImage to Bitmap
 **************************************************************************/
TVOID Dismod_PutImageBm(TMOD_DISMOD *dismod, TDISBITMAP *bm, TIMGPICTURE *img,TDISRECT *src, TDISRECT *dst)
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

		dismod_end_bm(dismod,bm);
		if(!(dismod_lock_bm(dismod,bm,&bufimg)))
			return;

		if(src->width==dst->width && src->height==dst->height)
		{
			TTAGITEM convtags[5];

			convtags[0].tti_Tag = IMGTAG_SRCX;      convtags[0].tti_Value = (TTAG)(src->x);
			convtags[1].tti_Tag = IMGTAG_SRCY;      convtags[1].tti_Value = (TTAG)(src->y);
			convtags[2].tti_Tag = IMGTAG_DSTX;      convtags[2].tti_Value = (TTAG)(dst->x);
			convtags[3].tti_Tag = IMGTAG_DSTY;      convtags[3].tti_Value = (TTAG)(dst->y);
			convtags[4].tti_Tag = TTAG_DONE;

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

		dismod_unlock_bm(dismod,bm);
		dismod_begin_bm(dismod,bm);
	}
	else
	{
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

			case IMGFMT_B8G8R8:
				dismod->bitmapinfo.bmiHeader.bV4V4Compression = BI_RGB;
			break;

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

		if(src->width==dst->width && src->height==dst->height)
		{
			SetDIBitsToDevice(  dismod->bitmapHDC,
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
			SetStretchBltMode(dismod->bitmapHDC,COLORONCOLOR);
			StretchDIBits(  dismod->bitmapHDC,
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

