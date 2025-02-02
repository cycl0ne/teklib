#ifndef _TEK_MOD_DISPLAY_FB_H
#define _TEK_MOD_DISPLAY_FB_H

#include <tek/mod/visual.h>

/* Command codes: */
#define TVCMD_FLUSH		TVCMD_EXTENDED + 1

struct TVFBRequest
{
	struct TIORequest tvr_Req;
	union
	{
		struct { TAPTR Window; TTAGITEM *Tags; TAPTR IMsgPort; } OpenWindow;
		struct { TAPTR Window; } CloseWindow;
		struct { TAPTR Font; TTAGITEM *Tags; } OpenFont;
		struct { TAPTR Font; } CloseFont;
		struct { TAPTR Font; TTAGITEM *Tags; TUINT Num; } GetFontAttrs;
		struct { TAPTR Font; TSTRPTR Text; TINT Width; } TextSize;
		struct { TAPTR Handle; TTAGITEM *Tags; } QueryFonts;
		struct { TAPTR Handle; TTAGITEM *Attrs; } GetNextFont;
		struct { TAPTR Window; TUINT Mask; TUINT OldMask; } SetInput;
		struct { TAPTR Window; TTAGITEM *Tags; TUINT Num; } GetAttrs;
		struct { TAPTR Window; TTAGITEM *Tags; TUINT Num; } SetAttrs;
		struct { TAPTR Window; TUINT RGB; TVPEN Pen; } AllocPen;
		struct { TAPTR Window; TVPEN Pen; } FreePen;
		struct { TAPTR Font; TAPTR Window;  } SetFont;
		struct { TAPTR Window; TVPEN Pen; } Clear;
		struct { TAPTR Window; TINT Rect[4]; TVPEN Pen; } Rect;
		struct { TAPTR Window; TINT Rect[4]; TVPEN Pen; } FRect;
		struct { TAPTR Window; TINT Rect[4]; TVPEN Pen; } Line;
		struct { TAPTR Window; TINT Rect[2]; TVPEN Pen; } Plot;
		struct { TAPTR Window; TINT X, Y; TSTRPTR Text; TINT Length;
			TVPEN FgPen, BgPen; } Text;
		struct { TAPTR Window; TINT *Array; TINT Num; TTAGITEM *Tags; } Strip;
		struct { TAPTR Window; TTAGITEM *Tags; } DrawTags;
		struct { TAPTR Window; TINT *Array; TINT Num; TTAGITEM *Tags; } Fan;
		struct { TAPTR Window; TINT Rect[4]; TINT DestX; TINT DestY;
			TTAGITEM *Tags; } CopyArea;
		struct { TAPTR Window; TINT Rect[4]; TTAGITEM *Tags; } ClipRect;
		struct { TAPTR Window; TINT RRect[4]; TAPTR Buf; TINT TotWidth;
			TTAGITEM *Tags; } DrawBuffer;
		struct { TAPTR Window; TINT Rect[4]; } Flush;
	} tvr_Op;
};

#endif
