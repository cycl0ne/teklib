#ifndef _TEK_DISPLAY_FB_MOD_H
#define _TEK_DISPLAY_FB_MOD_H

/*
**	display_fb_mod.h - Framebuffer display driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftglyph.h>

#include <tek/debug.h>
#include <tek/exec.h>
#include <tek/teklib.h>

#include <tek/proto/exec.h>
#include <tek/mod/display/fb.h>

/*****************************************************************************/

#define FB_DISPLAY_VERSION      1
#define FB_DISPLAY_REVISION     0
#define FB_DISPLAY_NUMVECTORS   10

#define FB_DEF_RENDER_DEVICE    "display_x11"
#define FB_DEF_WIDTH            600
#define FB_DEF_HEIGHT           400

#ifndef LOCAL
#define LOCAL
#endif

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

/*****************************************************************************/
/*
**	Fonts
*/

#ifndef FNT_DEFDIR
#define	FNT_DEFDIR          TEKHOST_SYSDIR "fonts/"
#endif

#define FNT_DEFNAME         "VeraMono"
#define FNT_DEFPXSIZE       14

#define	FNT_WILDCARD        "*"

#define FNTQUERY_NUMATTR	(5+1)
#define	FNTQUERY_UNDEFINED	-1

#define FNT_ITALIC			0x1
#define	FNT_BOLD			0x2
#define FNT_UNDERLINE		0x4

#define FNT_MATCH_NAME		0x01
#define FNT_MATCH_SIZE		0x02
#define FNT_MATCH_SLANT		0x04
#define	FNT_MATCH_WEIGHT	0x08
#define	FNT_MATCH_SCALE		0x10
/* all mandatory properties: */
#define FNT_MATCH_ALL		0x0f

#define MAX_GLYPHS 256

struct FontManager
{
	/* list of opened fonts */
	struct TList openfonts;
	/* pointer to default font */
	TAPTR deffont;
	/* count of references to default font */
	TINT defref;

	/*********************************************/
	/* string based font preparation and caching */

	/* glyph images */
	FT_Glyph glyphs[MAX_GLYPHS];
	/* glyph positions */
	FT_Vector pos[MAX_GLYPHS];
	/* glyph count of current string */
	FT_Int num_glyphs;
};

struct FontNode
{
	struct THandle handle;
	FT_Face face;
	TUINT attr;
	TUINT pxsize;
	TINT ascent;
	TINT descent;
	TINT height;
};

struct FontQueryNode
{
	struct TNode node;
	TTAGITEM tags[FNTQUERY_NUMATTR];
};

struct FontQueryHandle
{
	struct THandle handle;
	struct TList reslist;
	struct TNode **nptr;
};

/*****************************************************************************/
/*
**	UTF8 support
*/

#define RAWFB_UTF8_BUFSIZE 4096

struct utf8reader
{
	/* character reader callback: */
	int (*readchar)(struct utf8reader *);
	/* reader state: */
	int accu, numa, min, bufc;
	/* userdata to reader */
	void *udata;
};

LOCAL int readutf8(struct utf8reader *rd);
LOCAL unsigned char *encodeutf8(unsigned char *buf, int c);

/*****************************************************************************/

typedef struct
{
	/* Module header: */
	struct TModule fbd_Module;
	/* Exec module base ptr: */
	struct TExecBase *fbd_ExecBase;
	/* Locking for module base: */
	struct TLock *fbd_Lock;
	/* Number of module opens: */
	TUINT fbd_RefCount;
	/* Task: */
	struct TTask *fbd_Task;
	/* Command message port: */
	struct TMsgPort *fbd_CmdPort;
	/* Command message port signal: */
	TUINT fbd_CmdPortSignal;
	/* Sub rendering device (optional): */
	TAPTR fbd_RndDevice;
	/* Replyport for render requests: */
	struct TMsgPort *fbd_RndRPort;
	/* Render device instance: */
	TAPTR fbd_RndInstance;
	/* Render request: */
	struct TVFBRequest *fbd_RndRequest;

	/* Device open tags: */
	TTAGITEM *fbd_OpenTags;

	/* pooled input messages: */
	struct TList fbd_IMsgPool;
	/* list of all visuals: */
	struct TList fbd_VisualList;
	/* Module global memory manager (thread safe): */
	struct TMemManager *fbd_MemMgr;

	TUINT8 *fbd_BufPtr;
	TBOOL fbd_BufferOwner;
	TUINT fbd_InputMask;

	TINT fbd_Width;
	TINT fbd_Height;
	TINT fbd_Modulo;
	TINT fbd_BytesPerPixel;
	TINT fbd_BytesPerLine;

	FT_Library fbd_FTLibrary;
	struct FontManager fbd_FontManager;

	TINT fbd_MouseX;
	TINT fbd_MouseY;
	TINT fbd_KeyQual;

	TUINT32 fbd_unicodebuffer[RAWFB_UTF8_BUFSIZE];

} FBDISPLAY;

typedef struct
{
	struct TNode fbv_Node;
	TINT fbv_Modulo;

	/* Window extents: */
	TINT fbv_WinRect[4];

	/* Clipping boundaries: */
	TINT fbv_ClipRect[4];

	TINT fbv_PixelPerLine;

	/* Buffer pointer to upper left edge of visual: */
	TUINT8 *fbv_BufPtr;

	TVPEN bgpen, fgpen;

	/* list of allocated pens: */
	struct TList penlist;

	/* current active font */
	TAPTR curfont;

	/* List of queued input messages to be sent: */
	struct TList fbv_IMsgQueue;
	/* Destination message port for input messages: */
	TAPTR fbv_IMsgPort;

	TUINT fbv_InputMask;

} FBWINDOW;

struct FBPen
{
	struct TNode node;
	TUINT32 rgb;
};

struct attrdata
{
	FBDISPLAY *mod;
	FBWINDOW *v;
	TAPTR font;
	TINT num;
};

/*****************************************************************************/
/*
**	Framebuffer drawing primitives
*/

LOCAL void fbp_drawpoint(FBWINDOW *v, TINT x, TINT y, struct FBPen *pen);
LOCAL void fbp_drawfrect(FBWINDOW *v, TINT rect[4], struct FBPen *pen);
LOCAL void fbp_drawrect(FBWINDOW *v, TINT rect[4], struct FBPen *pen);
LOCAL void fbp_drawline(FBWINDOW *v, TINT rect[4], struct FBPen *pen);
LOCAL void fbp_drawtriangle(FBWINDOW *v, TINT x0, TINT y0, TINT x1, TINT y1,
	TINT x2, TINT y2, struct FBPen *pen);
LOCAL void fbp_drawbuffer(FBWINDOW *v, TUINT8 *buf, TINT rect[4], TINT totw);
LOCAL void fbp_copyarea(FBWINDOW *v, TINT rect[4], TINT xd, TINT yd);

/*****************************************************************************/

LOCAL void WritePixel(FBWINDOW *v, TINT x, TINT y, struct FBPen *pen);
LOCAL TUINT32 GetPixel(FBWINDOW *v, TINT x, TINT y);

/*****************************************************************************/
/*
**	Region management
*/

struct Region
{
	struct TList rg_List;
};

struct RectNode
{
	struct TNode rn_Node;
	TINT rn_Rect[4];
};

LOCAL struct Region *fb_region_new(FBDISPLAY *mod, TINT s[]);
LOCAL void fb_region_destroy(FBDISPLAY *mod, struct Region *region);
LOCAL TBOOL fb_region_overlap(FBDISPLAY *mod, struct Region *region,
	TINT s[]);
LOCAL TBOOL fb_region_subrect(FBDISPLAY *mod, struct Region *region,
	TINT s[]);
LOCAL TBOOL fb_region_subregion(FBDISPLAY *mod, struct Region *dregion,
	struct Region *sregion);
LOCAL TBOOL fb_region_andrect(FBDISPLAY *mod, struct Region *region,
	TINT s[]);
LOCAL TBOOL fb_region_andregion(FBDISPLAY *mod, struct Region *dregion,
	struct Region *sregion);
LOCAL TBOOL fb_region_isempty(FBDISPLAY *mod, struct Region *region);

/*****************************************************************************/

LOCAL TBOOL fb_init(FBDISPLAY *mod, TTAGITEM *tags);
LOCAL TBOOL fb_getimsg(FBDISPLAY *mod, FBWINDOW *v, TIMSG **msgptr,
	TUINT type);
LOCAL void fb_sendimessages(FBDISPLAY *mod, TBOOL do_interval);

LOCAL void fb_exit(FBDISPLAY *mod);
LOCAL void fb_openvisual(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_closevisual(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_setinput(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_allocpen(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_freepen(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_frect(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_rect(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_line(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_plot(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_drawstrip(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_clear(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_getattrs(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_setattrs(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_drawtext(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_openfont(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_getfontattrs(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_textsize(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_setfont(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_closefont(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_queryfonts(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_getnextfont(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_drawtags(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_drawfan(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_copyarea(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_setcliprect(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_unsetcliprect(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_drawbuffer(FBDISPLAY *mod, struct TVFBRequest *req);
LOCAL void fb_flush(FBDISPLAY *mod, struct TVFBRequest *req);

LOCAL TAPTR fb_hostopenfont(FBDISPLAY *mod, TTAGITEM *tags);
LOCAL void fb_hostclosefont(FBDISPLAY *mod, TAPTR font);
LOCAL void fb_hostsetfont(FBDISPLAY *mod, FBWINDOW *v, TAPTR font);
LOCAL TTAGITEM *fb_hostgetnextfont(FBDISPLAY *mod, TAPTR fqhandle);
LOCAL TINT fb_hosttextsize(FBDISPLAY *mod, TAPTR font, TSTRPTR text, TINT len);
LOCAL TVOID fb_hostdrawtext(FBDISPLAY *mod, FBWINDOW *v, TSTRPTR text,
	TINT len, TUINT posx, TUINT posy, TVPEN fgpen);
LOCAL THOOKENTRY TTAG fb_hostgetfattrfunc(struct THook *hook, TAPTR obj,
	TTAG msg);
LOCAL TAPTR fb_hostqueryfonts(FBDISPLAY *mod, TTAGITEM *tags);
LOCAL TUINT32* fb_utf8tounicode(FBDISPLAY *mod, TSTRPTR utf8string, TINT len,
	TINT *bytelen);

#endif /* _TEK_DISPLAY_FB_MOD_H */
