#ifndef _TEK_INLINE_UNISTRING_H
#define _TEK_INLINE_UNISTRING_H

/*
**	$Id: unistring.h,v 1.4 2005/09/13 02:44:54 tmueller Exp $
**	teklib/tek/inline/unistring.h - unistring inline calls
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/proto/unistring.h>

extern TAPTR TUStrBase;

#define TAllocArray(flags) \
	(*(((TMODCALL TUString(**)(TAPTR,TUINT))(TUStrBase))[-13]))(TUStrBase,flags)

#define TFreeArray(a) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUString))(TUStrBase))[-14]))(TUStrBase,a)

#define TInsertArray(a,ptr) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR))(TUStrBase))[-15]))(TUStrBase,a,ptr)

#define TRemoveArray(a,ptr) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR))(TUStrBase))[-16]))(TUStrBase,a,ptr)

#define TSeekArray(a,mode,offs) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TINT))(TUStrBase))[-17]))(TUStrBase,a,mode,offs)

#define TGetArray(a,ptr) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR))(TUStrBase))[-18]))(TUStrBase,a,ptr)

#define TSetArray(a,ptr) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR))(TUStrBase))[-19]))(TUStrBase,a,ptr)

#define TLengthArray(a) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString))(TUStrBase))[-20]))(TUStrBase,a)

#define TMapArray(a,offs,len) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TUString,TINT,TINT))(TUStrBase))[-21]))(TUStrBase,a,offs,len)

#define TRenderArray(a,ptr,offs,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR,TINT,TINT))(TUStrBase))[-22]))(TUStrBase,a,ptr,offs,len)

#define TChangeArray(a,flags) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUINT))(TUStrBase))[-23]))(TUStrBase,a,flags)

#define TDupArray(a) \
	(*(((TMODCALL TUString(**)(TAPTR,TUString))(TUStrBase))[-24]))(TUStrBase,a)

#define TCopyArray(a,b) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString))(TUStrBase))[-25]))(TUStrBase,a,b)

#define TTruncArray(a) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString))(TUStrBase))[-26]))(TUStrBase,a)

#define TMoveArray(src,dst) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUString,TUString))(TUStrBase))[-27]))(TUStrBase,src,dst)

#define TAllocString(cstr) \
	(*(((TMODCALL TUString(**)(TAPTR,TSTRPTR))(TUStrBase))[-28]))(TUStrBase,cstr)

#define TFreeString(s) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUString))(TUStrBase))[-29]))(TUStrBase,s)

#define TInsCharString(s,pos,c) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TWCHAR))(TUStrBase))[-30]))(TUStrBase,s,pos,c)

#define TRemCharString(s,pos) \
	(*(((TMODCALL TWCHAR(**)(TAPTR,TUString,TINT))(TUStrBase))[-31]))(TUStrBase,s,pos)

#define TLengthString(s) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString))(TUStrBase))[-32]))(TUStrBase,s)

#define TMapString(s,offs,len,mode) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TUString,TINT,TINT,TUINT))(TUStrBase))[-33]))(TUStrBase,s,offs,len,mode)

#define TRenderString(s,ptr,offs,len,mode) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR,TINT,TINT,TUINT))(TUStrBase))[-34]))(TUStrBase,s,ptr,offs,len,mode)

#define TSetCharString(s,pos,c) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TWCHAR))(TUStrBase))[-35]))(TUStrBase,s,pos,c)

#define TGetCharString(s,pos) \
	(*(((TMODCALL TWCHAR(**)(TAPTR,TUString,TINT))(TUStrBase))[-36]))(TUStrBase,s,pos)

#define TDupString(s,pos,len) \
	(*(((TMODCALL TUString(**)(TAPTR,TUString,TINT,TINT))(TUStrBase))[-37]))(TUStrBase,s,pos,len)

#define TCopyString(s,d) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString))(TUStrBase))[-38]))(TUStrBase,s,d)

#define TInsertString(d,dpos,s,spos,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TUString,TINT,TINT))(TUStrBase))[-39]))(TUStrBase,d,dpos,s,spos,len)

#define TInsertStrNString(s,pos,ptr,len,mode) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TAPTR,TINT,TUINT))(TUStrBase))[-40]))(TUStrBase,s,pos,ptr,len,mode)

#define TEncodeUTF8String(s) \
	(*(((TMODCALL TUString(**)(TAPTR,TUString))(TUStrBase))[-41]))(TUStrBase,s)

#define TInsertUTF8String(s,pos,utf8) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TUINT8 *))(TUStrBase))[-42]))(TUStrBase,s,pos,utf8)

#define TCmpNString(s1,s2,p1,p2,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString,TINT,TINT,TINT))(TUStrBase))[-43]))(TUStrBase,s1,s2,p1,p2,len)

#define TCropString(s,pos,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TINT))(TUStrBase))[-44]))(TUStrBase,s,pos,len)

#define TTransformString(s,pos,len,mode) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TINT,TUINT))(TUStrBase))[-45]))(TUStrBase,s,pos,len,mode)

#define TTokenizeString(pat,flags) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUINT))(TUStrBase))[-46]))(TUStrBase,pat,flags)

#define TMatchString(pat,str) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString))(TUStrBase))[-47]))(TUStrBase,pat,str)

#define TMoveString(src,dst) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUString,TUString))(TUStrBase))[-48]))(TUStrBase,src,dst)

#define TAddPartString(path,part) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString))(TUStrBase))[-49]))(TUStrBase,path,part)

#define TIsAlnum(c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(TUStrBase))[-50]))(TUStrBase,c)

#define TIsAlpha(c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(TUStrBase))[-51]))(TUStrBase,c)

#define TIsCntrl(c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(TUStrBase))[-52]))(TUStrBase,c)

#define TIsGraph(c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(TUStrBase))[-53]))(TUStrBase,c)

#define TIsLower(c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(TUStrBase))[-54]))(TUStrBase,c)

#define TIsPrint(c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(TUStrBase))[-55]))(TUStrBase,c)

#define TIsPunct(c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(TUStrBase))[-56]))(TUStrBase,c)

#define TIsSpace(c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(TUStrBase))[-57]))(TUStrBase,c)

#define TIsUpper(c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(TUStrBase))[-58]))(TUStrBase,c)

#define TToLower(c) \
	(*(((TMODCALL TWCHAR(**)(TAPTR,TWCHAR))(TUStrBase))[-59]))(TUStrBase,c)

#define TToUpper(c) \
	(*(((TMODCALL TWCHAR(**)(TAPTR,TWCHAR))(TUStrBase))[-60]))(TUStrBase,c)

#define TFindPatString(s,p,pos,len,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString,TINT,TINT,TTAGITEM *))(TUStrBase))[-61]))(TUStrBase,s,p,pos,len,tags)

#define TFindString(s,p,pos,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString,TINT,TINT))(TUStrBase))[-62]))(TUStrBase,s,p,pos,len)

#endif /* _TEK_INLINE_UNISTRING_H */
