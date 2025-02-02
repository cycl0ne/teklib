
#ifndef _TEK_MOD_DISPLAYHANDLER_H
#define _TEK_MOD_DISPLAYHANDLER_H

/*#include <tek/exec.h>*/
#include <tek/mod/imgproc.h>

#define DISPLAYHANDLER_VERSION  1

typedef struct
{
	TINT x,y;
	TINT width,height;
}TDISRECT;

typedef struct
{
	TSTRPTR name;
	TINT    version;
	TINT    priority;
	TINT    dispmode,dispclass;
	TINT    minwidth,maxwidth,minheight,maxheight,mindepth,maxdepth;
	TINT    defaultwidth,defaultheight,defaultdepth;
} TDISPROPS;

typedef struct
{
	TINT    minbmwidth,minbmheight,maxbmwidth,maxbmheight;
	TBOOL   blitscale,blitalpha,blitckey;
	TBOOL   canconvertdisplay,canconvertscaledisplay;
	TBOOL   canconvertbitmap,canconvertscalebitmap;
	TBOOL   candrawbitmap;
} TDISCAPS;

typedef struct
{
	TINT	x,y;
	TINT	width,height;
	TINT	depth,format,bytesperrow;
}TDISDESCRIPTOR;

typedef struct
{
	TINT code;
	TINT qualifier;
}TDISKEY;

typedef struct
{
	TINT x, y;
}TDISMOUSEPOS;

typedef struct
{
	TUINT code;
}TDISMBUTTON;

typedef struct
{
	TUINT code;
	TUINT8 data[32];
}TDISMSG;

typedef struct
{
	TINT width,height,depth;
}TDISMODE;

/* tags for displayhandler_finddisplay */
#define TDISMUSTHAVE_CLASS                      TTAG_USER+1
#define TDISMUSTNOTHAVE_CLASS           TTAG_USER+2
#define TDISMUSTHAVE_MODE                       TTAG_USER+3
#define TDISMUSTNOTHAVE_MODE            TTAG_USER+4
#define TDISMUSTHAVE_COLORS                     TTAG_USER+5
#define TDISMUSTNOTHAVE_COLORS          TTAG_USER+6
#define TDIS_MINWIDTH                           TTAG_USER+7
#define TDIS_MINHEIGHT                          TTAG_USER+8

/* flags for class */
#define TDISCLASS_STANDARD                      0x00000001
#define TDISCLASS_OPENGL                        0x00000002

/* flags for mode */
#define TDISMODE_WINDOW                         0x00000001
#define TDISMODE_FULLSCREEN                     0x00000002

/* flags for colors */
#define TDISCOLORS_CLUT                         0x00000001
#define TDISCOLORS_TRUECOLOR            0x00000002

/* tags for displayhandler_createview */
#define TDISC_WIDTH                                     TTAG_USER+1
#define TDISC_HEIGHT                            TTAG_USER+2
#define TDISC_DEPTH                                     TTAG_USER+3
#define TDISC_TITLE                                     TTAG_USER+4
#define TDISC_XPOS                                      TTAG_USER+5
#define TDISC_YPOS                                      TTAG_USER+6
#define TDISC_FLAGS                                     TTAG_USER+7

/* flags (bit-wise) for createview */
#define TDISCF_DOUBLEBUFFER                     0x00000001
#define TDISCF_RESIZEABLE                       0x00000002

/* flags (bit-wise) for allocbitmap */
#define TDISCF_SCALEBLIT                        0x00000001
#define TDISCF_CKEYBLIT                         0x00000002
#define TDISCF_CALPHABLIT                       0x00000004

/* messages for getmessage */
#define TDISMSG_CLOSE                           0x00000001
#define TDISMSG_RESIZE                          0x00000002
#define TDISMSG_REDRAW                          0x00000004
#define TDISMSG_ACTIVATED                       0x00000008
#define TDISMSG_DEACTIVATED                     0x00000010
#define TDISMSG_ICONIC                          0x00000020
#define TDISMSG_MOVE                            0x00000040

#define TDISMSG_KEYDOWN                         0x00001000
#define TDISMSG_KEYUP                           0x00002000
#define TDISMSG_MOUSEMOVE                       0x00004000
#define TDISMSG_MBUTTONDOWN                     0x00008000
#define TDISMSG_MBUTTONUP                       0x00010000
#define TDISMSG_MWHEELDOWN                      0x00020000
#define TDISMSG_MWHEELUP                        0x00040000
#define TDISMSG_MWHEELLEFT                      0x00080000      /* not supported yet :( */
#define TDISMSG_MWHEELRIGHT                     0x00100000      /*         ""           */

/* codes for mbuttondown/up messages */
#define TDISMB_LBUTTON                          0x00000001
#define TDISMB_MBUTTON                          0x00000002
#define TDISMB_RBUTTON                          0x00000004
#define TDISMB_E1BUTTON                         0x00000008
#define TDISMB_E2BUTTON                         0x00000010
#define TDISMB_E3BUTTON                         0x00000020
#define TDISMB_E4BUTTON                         0x00000040
#define TDISMB_E5BUTTON                         0x00000080
#define TDISMB_E6BUTTON                         0x00000100

/* tags for dis_setattrs */
#define TDISTAG_POINTERMODE						TTAG_USER+1
#define TDISTAG_DELTAMOUSE						TTAG_USER+2
#define TDISTAG_VSYNCHINT						TTAG_USER+64
#define TDISTAG_SMOOTHHINT						TTAG_USER+65

/* modes for tag setpointer */
#define TDISPTR_NORMAL                          0x00000001
#define TDISPTR_BUSY                            0x00000002
#define TDISPTR_INVISIBLE                       0x00000003

/* tags for pictodisplay, blit */
#define TDISB_SRCX                              TTAG_USER+1
#define TDISB_SRCY                              TTAG_USER+2
#define TDISB_SRCWIDTH                          TTAG_USER+3
#define TDISB_SRCHEIGHT                         TTAG_USER+4
#define TDISB_DSTX                              TTAG_USER+5
#define TDISB_DSTY                              TTAG_USER+6
#define TDISB_DSTWIDTH                          TTAG_USER+7
#define TDISB_DSTHEIGHT                         TTAG_USER+8
/* these are ONLY for blitting !!! */
#define TDISB_CKEY                              TTAG_USER+9
#define TDISB_CALPHA                            TTAG_USER+10

/* type flags for bitmaps and displays */
#define TDIS_DISPLAY    0x00000001
#define TDIS_BITMAP     0x00000002

/* Keycodes */
#define TDISKEY_ESCAPE           0x01
#define TDISKEY_F1               0x3B
#define TDISKEY_F2               0x3C
#define TDISKEY_F3               0x3D
#define TDISKEY_F4               0x3E
#define TDISKEY_F5               0x3F
#define TDISKEY_F6               0x40
#define TDISKEY_F7               0x41
#define TDISKEY_F8               0x42
#define TDISKEY_F9               0x43
#define TDISKEY_F10              0x44
#define TDISKEY_F11              0x57
#define TDISKEY_F12              0x58

#define TDISKEY_GRAVE            0x29
#define TDISKEY_1                0x02
#define TDISKEY_2                0x03
#define TDISKEY_3                0x04
#define TDISKEY_4                0x05
#define TDISKEY_5                0x06
#define TDISKEY_6                0x07
#define TDISKEY_7                0x08
#define TDISKEY_8                0x09
#define TDISKEY_9                0x0A
#define TDISKEY_0                0x0B
#define TDISKEY_MINUS            0x0C
#define TDISKEY_EQUALS           0x0D
#define TDISKEY_BACKSPACE        0x0E

#define TDISKEY_TAB              0x0F
#define TDISKEY_q                0x10
#define TDISKEY_w                0x11
#define TDISKEY_e                0x12
#define TDISKEY_r                0x13
#define TDISKEY_t                0x14
#define TDISKEY_y                0x15
#define TDISKEY_u                0x16
#define TDISKEY_i                0x17
#define TDISKEY_o                0x18
#define TDISKEY_p                0x19
#define TDISKEY_LEFTBRACKET      0x1A
#define TDISKEY_RIGHTBRACKET     0x1B
#define TDISKEY_RETURN           0x1C

#define TDISKEY_CAPSLOCK         0x3A
#define TDISKEY_a                0x1E
#define TDISKEY_s                0x1F
#define TDISKEY_d                0x20
#define TDISKEY_f                0x21
#define TDISKEY_g                0x22
#define TDISKEY_h                0x23
#define TDISKEY_j                0x24
#define TDISKEY_k                0x25
#define TDISKEY_l                0x26
#define TDISKEY_SEMICOLON        0x27
#define TDISKEY_APOSTROPH        0x28
#define TDISKEY_BACKSLASH        0x2B

#define TDISKEY_LSHIFT           0x2A
#define TDISKEY_z                0x2C
#define TDISKEY_x                0x2D
#define TDISKEY_c                0x2E
#define TDISKEY_v                0x2F
#define TDISKEY_b                0x30
#define TDISKEY_n                0x31
#define TDISKEY_m                0x32
#define TDISKEY_COMMA            0x33
#define TDISKEY_PERIOD           0x34
#define TDISKEY_SLASH            0x35
#define TDISKEY_RSHIFT           0x36

#define TDISKEY_LCTRL            0x1D
#define TDISKEY_LALT             0x38
#define TDISKEY_SPACE            0x39
#define TDISKEY_RALT             0xB8
#define TDISKEY_RCTRL            0x9D

#define TDISKEY_UP               0xC8
#define TDISKEY_LEFT             0xCB
#define TDISKEY_RIGHT            0xCD
#define TDISKEY_DOWN             0xD0

#define TDISKEY_PRINT            0xFF
#define TDISKEY_SCROLLOCK        0x46
#define TDISKEY_PAUSE            0xC5

#define TDISKEY_INSERT           0xD2
#define TDISKEY_HOME             0xC7
#define TDISKEY_PAGEUP           0xC9
#define TDISKEY_DELETE           0xD3
#define TDISKEY_END              0xCF
#define TDISKEY_PAGEDOWN         0xD1

#define TDISKEY_NUMLOCK          0x45
#define TDISKEY_KP_DIVIDE        0xB5
#define TDISKEY_KP_MULTIPLY      0x37
#define TDISKEY_KP_MINUS         0x4A
#define TDISKEY_KP_PLUS          0x4E
#define TDISKEY_KP_ENTER         0x9C

#define TDISKEY_KP7              0x47
#define TDISKEY_KP8              0x48
#define TDISKEY_KP9              0x49
#define TDISKEY_KP4              0x4B
#define TDISKEY_KP5              0x4C
#define TDISKEY_KP6              0x4D
#define TDISKEY_KP1              0x4F
#define TDISKEY_KP2              0x50
#define TDISKEY_KP3              0x51
#define TDISKEY_KP0              0x52
#define TDISKEY_KP_PERIOD        0x53

#define TDISKEY_EXTRA1           0x56
#define TDISKEY_EXTRA2           0x60
#define TDISKEY_EXTRA3           0x61

#define TDISKEY_OEM1             0xDB
#define TDISKEY_OEM2             0xDC
#define TDISKEY_OEM3             0xDD

#define TDISKEYQUAL_SHIFT       0x01
#define TDISKEYQUAL_ALT         0x02
#define TDISKEYQUAL_CTRL        0x04


/* private structures used internally between displayhandler and display hosts */
typedef struct
{
	THNDL *display;
	TIMGARGBCOLOR color;
	TAPTR hostdata;
} TDISPEN;

typedef struct
{
	TINT type;
	TAPTR displayhandler;
	TAPTR modul;
	TDISPROPS props;
	TDISCAPS caps;
	TDISDESCRIPTOR desc;
	TINT numdismodes;
	TDISMODE *modelist;
	TLIST pens_list;
	TLIST bitmaps_list;
	TDISPEN drawpen;
	TIMGARGBCOLOR *palette;
} TDISMODULE;

typedef struct
{
	TINT type;
	TUINT flags;
	THNDL *display;
	TIMGPICTURE image;
	TAPTR hostdata;
} TDISBITMAP;

typedef struct
{
	TDISRECT src,dst;
	TBOOL ckey, calpha;
	TIMGARGBCOLOR ckey_val;
	TINT calpha_val;
}TDBLITOPS;

#if 0
/*
**      convenience macros
*/

#define displayhandler_new(exec,tags)                                   TOpenModule(exec, "displayhandler", 0, tags)
#define displayhandler_destroy(exec,displayhandler)             TCloseModule(exec, displayhandler)
#endif

#endif

