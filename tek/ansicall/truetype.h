#ifndef _TEK_ANSICALL_TRUETYPE_H
#define _TEK_ANSICALL_TRUETYPE_H

/*
**	$Id: truetype.h,v 1.2 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/truetype.h - truetype module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define truetype_init(truetype,font,name) \
	(*(((TMODCALL TINT(**)(TAPTR,TTFONT *,TSTRPTR))(truetype))[-1]))(truetype,font,name)

#define truetype_close(truetype,font) \
	(*(((TMODCALL TVOID(**)(TAPTR,TTFONT *))(truetype))[-2]))(truetype,font)

#define truetype_getchar(truetype,font,char,vert) \
	(*(((TMODCALL TINT(**)(TAPTR,TTFONT *,TINT8,TTVERT *))(truetype))[-3]))(truetype,font,char,vert)

#define truetype_freevert(truetype,vert) \
	(*(((TMODCALL TVOID(**)(TAPTR,TTVERT *))(truetype))[-4]))(truetype,vert)

#define truetype_getstring(truetype,font,string,tstr) \
	(*(((TMODCALL TINT(**)(TAPTR,TTFONT *,TSTRPTR,TTSTR *))(truetype))[-5]))(truetype,font,string,tstr)

#define truetype_freetstr(truetype,tstr) \
	(*(((TMODCALL TVOID(**)(TAPTR,TTSTR *))(truetype))[-6]))(truetype,tstr)

#endif /* _TEK_ANSICALL_TRUETYPE_H */
