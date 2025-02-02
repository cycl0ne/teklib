#ifndef _TEK_ANSICALL_UNISTRING_H
#define _TEK_ANSICALL_UNISTRING_H

/*
**	$Id: unistring.h,v 1.3 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/unistring.h - unistring module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TUStrAllocArray(unistring,flags) \
	(*(((TMODCALL TUString(**)(TAPTR,TUINT))(unistring))[-13]))(unistring,flags)

#define TUStrFreeArray(unistring,a) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUString))(unistring))[-14]))(unistring,a)

#define TUStrInsertArray(unistring,a,ptr) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR))(unistring))[-15]))(unistring,a,ptr)

#define TUStrRemoveArray(unistring,a,ptr) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR))(unistring))[-16]))(unistring,a,ptr)

#define TUStrSeekArray(unistring,a,mode,offs) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TINT))(unistring))[-17]))(unistring,a,mode,offs)

#define TUStrGetArray(unistring,a,ptr) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR))(unistring))[-18]))(unistring,a,ptr)

#define TUStrSetArray(unistring,a,ptr) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR))(unistring))[-19]))(unistring,a,ptr)

#define TUStrLengthArray(unistring,a) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString))(unistring))[-20]))(unistring,a)

#define TUStrMapArray(unistring,a,offs,len) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TUString,TINT,TINT))(unistring))[-21]))(unistring,a,offs,len)

#define TUStrRenderArray(unistring,a,ptr,offs,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR,TINT,TINT))(unistring))[-22]))(unistring,a,ptr,offs,len)

#define TUStrChangeArray(unistring,a,flags) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUINT))(unistring))[-23]))(unistring,a,flags)

#define TUStrDupArray(unistring,a) \
	(*(((TMODCALL TUString(**)(TAPTR,TUString))(unistring))[-24]))(unistring,a)

#define TUStrCopyArray(unistring,a,b) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString))(unistring))[-25]))(unistring,a,b)

#define TUStrTruncArray(unistring,a) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString))(unistring))[-26]))(unistring,a)

#define TUStrMoveArray(unistring,src,dst) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUString,TUString))(unistring))[-27]))(unistring,src,dst)

#define TUStrAllocString(unistring,cstr) \
	(*(((TMODCALL TUString(**)(TAPTR,TSTRPTR))(unistring))[-28]))(unistring,cstr)

#define TUStrFreeString(unistring,s) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUString))(unistring))[-29]))(unistring,s)

#define TUStrInsCharString(unistring,s,pos,c) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TWCHAR))(unistring))[-30]))(unistring,s,pos,c)

#define TUStrRemCharString(unistring,s,pos) \
	(*(((TMODCALL TWCHAR(**)(TAPTR,TUString,TINT))(unistring))[-31]))(unistring,s,pos)

#define TUStrLengthString(unistring,s) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString))(unistring))[-32]))(unistring,s)

#define TUStrMapString(unistring,s,offs,len,mode) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TUString,TINT,TINT,TUINT))(unistring))[-33]))(unistring,s,offs,len,mode)

#define TUStrRenderString(unistring,s,ptr,offs,len,mode) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TAPTR,TINT,TINT,TUINT))(unistring))[-34]))(unistring,s,ptr,offs,len,mode)

#define TUStrSetCharString(unistring,s,pos,c) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TWCHAR))(unistring))[-35]))(unistring,s,pos,c)

#define TUStrGetCharString(unistring,s,pos) \
	(*(((TMODCALL TWCHAR(**)(TAPTR,TUString,TINT))(unistring))[-36]))(unistring,s,pos)

#define TUStrDupString(unistring,s,pos,len) \
	(*(((TMODCALL TUString(**)(TAPTR,TUString,TINT,TINT))(unistring))[-37]))(unistring,s,pos,len)

#define TUStrCopyString(unistring,s,d) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString))(unistring))[-38]))(unistring,s,d)

#define TUStrInsertString(unistring,d,dpos,s,spos,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TUString,TINT,TINT))(unistring))[-39]))(unistring,d,dpos,s,spos,len)

#define TUStrInsertStrNString(unistring,s,pos,ptr,len,mode) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TAPTR,TINT,TUINT))(unistring))[-40]))(unistring,s,pos,ptr,len,mode)

#define TUStrEncodeUTF8String(unistring,s) \
	(*(((TMODCALL TUString(**)(TAPTR,TUString))(unistring))[-41]))(unistring,s)

#define TUStrInsertUTF8String(unistring,s,pos,utf8) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TUINT8 *))(unistring))[-42]))(unistring,s,pos,utf8)

#define TUStrCmpNString(unistring,s1,s2,p1,p2,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString,TINT,TINT,TINT))(unistring))[-43]))(unistring,s1,s2,p1,p2,len)

#define TUStrCropString(unistring,s,pos,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TINT))(unistring))[-44]))(unistring,s,pos,len)

#define TUStrTransformString(unistring,s,pos,len,mode) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TINT,TINT,TUINT))(unistring))[-45]))(unistring,s,pos,len,mode)

#define TUStrTokenizeString(unistring,pat,flags) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUINT))(unistring))[-46]))(unistring,pat,flags)

#define TUStrMatchString(unistring,pat,str) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString))(unistring))[-47]))(unistring,pat,str)

#define TUStrMoveString(unistring,src,dst) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUString,TUString))(unistring))[-48]))(unistring,src,dst)

#define TUStrAddPartString(unistring,path,part) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString))(unistring))[-49]))(unistring,path,part)

#define TUStrIsAlnum(unistring,c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(unistring))[-50]))(unistring,c)

#define TUStrIsAlpha(unistring,c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(unistring))[-51]))(unistring,c)

#define TUStrIsCntrl(unistring,c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(unistring))[-52]))(unistring,c)

#define TUStrIsGraph(unistring,c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(unistring))[-53]))(unistring,c)

#define TUStrIsLower(unistring,c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(unistring))[-54]))(unistring,c)

#define TUStrIsPrint(unistring,c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(unistring))[-55]))(unistring,c)

#define TUStrIsPunct(unistring,c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(unistring))[-56]))(unistring,c)

#define TUStrIsSpace(unistring,c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(unistring))[-57]))(unistring,c)

#define TUStrIsUpper(unistring,c) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TWCHAR))(unistring))[-58]))(unistring,c)

#define TUStrToLower(unistring,c) \
	(*(((TMODCALL TWCHAR(**)(TAPTR,TWCHAR))(unistring))[-59]))(unistring,c)

#define TUStrToUpper(unistring,c) \
	(*(((TMODCALL TWCHAR(**)(TAPTR,TWCHAR))(unistring))[-60]))(unistring,c)

#define TUStrFindPatString(unistring,s,p,pos,len,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString,TINT,TINT,TTAGITEM *))(unistring))[-61]))(unistring,s,p,pos,len,tags)

#define TUStrFindString(unistring,s,p,pos,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TUString,TUString,TINT,TINT))(unistring))[-62]))(unistring,s,p,pos,len)

#endif /* _TEK_ANSICALL_UNISTRING_H */
