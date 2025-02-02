#ifndef _TEK_INLINE_UTIL_H
#define _TEK_INLINE_UTIL_H

/*
**	$Id: util.h,v 1.8 2005/09/13 02:44:54 tmueller Exp $
**	teklib/tek/inline/util.h - util inline calls
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/proto/util.h>

extern TAPTR TUtilBase;

#define TGetArgC() \
	(*(((TMODCALL TINT(**)(TAPTR))(TUtilBase))[-9]))(TUtilBase)

#define TGetArgV() \
	(*(((TMODCALL TSTRPTR *(**)(TAPTR))(TUtilBase))[-10]))(TUtilBase)

#define TSetRetVal(retval) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TINT))(TUtilBase))[-11]))(TUtilBase,retval)

#define TGetUniqueID(ext) \
	(*(((TMODCALL TUINT(**)(TAPTR,TAPTR))(TUtilBase))[-12]))(TUtilBase,ext)

#define TGetRandObs() \
	(*(((TMODCALL TINT(**)(TAPTR))(TUtilBase))[-13]))(TUtilBase)

#define TSetRandObs(seed) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT))(TUtilBase))[-14]))(TUtilBase,seed)

#define THeapSort(data,ref,len,cmp) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR,TTAG *,TUINT,TCMPFUNC))(TUtilBase))[-15]))(TUtilBase,data,ref,len,cmp)

#define TSeekNode(node,steps) \
	(*(((TMODCALL TNODE *(**)(TAPTR,TNODE *,TINT))(TUtilBase))[-16]))(TUtilBase,node,steps)

#define TInsertSorted(list,numentries,nnode,cmp,data) \
	(*(((TMODCALL TVOID(**)(TAPTR,TLIST *,TUINT,TNODE *,TCMPFUNC,TAPTR))(TUtilBase))[-17]))(TUtilBase,list,numentries,nnode,cmp,data)

#define TFindSorted(list,numentries,find,data) \
	(*(((TMODCALL TNODE *(**)(TAPTR,TLIST *,TUINT,TFINDFUNC,TAPTR))(TUtilBase))[-18]))(TUtilBase,list,numentries,find,data)

#define TIsBigEndian() \
	(*(((TMODCALL TBOOL(**)(TAPTR))(TUtilBase))[-19]))(TUtilBase)

#define TBSwap16(valp) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT16 *))(TUtilBase))[-20]))(TUtilBase,valp)

#define TBSwap32(valp) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT *))(TUtilBase))[-21]))(TUtilBase,valp)

#define TStrLen(str) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR))(TUtilBase))[-22]))(TUtilBase,str)

#define TStrCpy(dst,src) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR))(TUtilBase))[-23]))(TUtilBase,dst,src)

#define TStrNCpy(dst,src,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR,TINT))(TUtilBase))[-24]))(TUtilBase,dst,src,len)

#define TStrCat(dst,src) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR))(TUtilBase))[-25]))(TUtilBase,dst,src)

#define TStrNCat(dst,src,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR,TINT))(TUtilBase))[-26]))(TUtilBase,dst,src,len)

#define TStrCmp(s1,s2) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR))(TUtilBase))[-27]))(TUtilBase,s1,s2)

#define TStrNCmp(s1,s2,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR,TINT))(TUtilBase))[-28]))(TUtilBase,s1,s2,len)

#define TStrCaseCmp(s1,s2) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR))(TUtilBase))[-29]))(TUtilBase,s1,s2)

#define TStrNCaseCmp(s1,s2,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR,TINT))(TUtilBase))[-30]))(TUtilBase,s1,s2,len)

#define TStrStr(s1,s2) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR))(TUtilBase))[-31]))(TUtilBase,s1,s2)

#define TStrChr(str,chr) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TINT))(TUtilBase))[-32]))(TUtilBase,str,chr)

#define TStrRChr(str,chr) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TINT))(TUtilBase))[-33]))(TUtilBase,str,chr)

#define TStrDup(mmu,str) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TAPTR,TSTRPTR))(TUtilBase))[-34]))(TUtilBase,mmu,str)

#define TStrNDup(mmu,str,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TAPTR,TSTRPTR,TINT))(TUtilBase))[-35]))(TUtilBase,mmu,str,len)

#define TStrToI(str,valp) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TINT *))(TUtilBase))[-36]))(TUtilBase,str,valp)

#define TGetRand(seed) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(TUtilBase))[-37]))(TUtilBase,seed)

#define TParseArgV(tmpl,argv,argarray) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TSTRPTR *,TTAG *))(TUtilBase))[-38]))(TUtilBase,tmpl,argv,argarray)

#define THToNL(val) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(TUtilBase))[-39]))(TUtilBase,val)

#define THToNS(val) \
	(*(((TMODCALL TUINT16(**)(TAPTR,TUINT16))(TUtilBase))[-40]))(TUtilBase,val)

#define TStrToD(str,valp) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TDOUBLE *))(TUtilBase))[-41]))(TUtilBase,str,valp)

#define TQSort(array,num,size,cmp,udata) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR,TINT,TINT,TCMPFUNC,TAPTR))(TUtilBase))[-42]))(TUtilBase,array,num,size,cmp,udata)

#define TGetModules(prefix,list,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TLIST *,TTAGITEM *))(TUtilBase))[-43]))(TUtilBase,prefix,list,tags)

#define TParseArgs(template,argstring,argarray) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TSTRPTR,TTAG *))(TUtilBase))[-44]))(TUtilBase,template,argstring,argarray)

#define TGetArgs() \
	(*(((TMODCALL TSTRPTR(**)(TAPTR))(TUtilBase))[-45]))(TUtilBase)

#endif /* _TEK_INLINE_UTIL_H */
