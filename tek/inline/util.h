#ifndef _TEK_INLINE_UTIL_H
#define _TEK_INLINE_UTIL_H

/*
**	teklib/tek/inline/util.h - util inline calls
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/proto/util.h>

#define TGetArgC() \
	(*(((TMODCALL TINT(**)(TAPTR))(TUtilBase))[-9]))(TUtilBase)

#define TGetArgV() \
	(*(((TMODCALL TSTRPTR *(**)(TAPTR))(TUtilBase))[-10]))(TUtilBase)

#define TSetRetVal(retval) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TINT))(TUtilBase))[-11]))(TUtilBase,retval)

#define THeapSort(refarray,len,cmphook) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR,TSIZE,struct THook *))(TUtilBase))[-12]))(TUtilBase,refarray,len,cmphook)

#define TSeekList(node,steps) \
	(*(((TMODCALL struct TNode *(**)(TAPTR,struct TNode *,TINTPTR))(TUtilBase))[-13]))(TUtilBase,node,steps)

#define TIsBigEndian() \
	(*(((TMODCALL TBOOL(**)(TAPTR))(TUtilBase))[-14]))(TUtilBase)

#define TBSwap16(valp) \
	(*(((TMODCALL void(**)(TAPTR,TUINT16 *))(TUtilBase))[-15]))(TUtilBase,valp)

#define TBSwap32(valp) \
	(*(((TMODCALL void(**)(TAPTR,TUINT *))(TUtilBase))[-16]))(TUtilBase,valp)

#define TStrDup(mm,str) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,struct TMemManager *,TSTRPTR))(TUtilBase))[-17]))(TUtilBase,mm,str)

#define TStrNDup(mm,str,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,struct TMemManager *,TSTRPTR,TSIZE))(TUtilBase))[-18]))(TUtilBase,mm,str,len)

#define TGetRand(seed) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(TUtilBase))[-19]))(TUtilBase,seed)

#define TParseArgV(tmpl,argv,argarray) \
	(*(((TMODCALL struct THandle *(**)(TAPTR,TSTRPTR,TSTRPTR *,TTAG *))(TUtilBase))[-20]))(TUtilBase,tmpl,argv,argarray)

#define THToNL(val) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(TUtilBase))[-21]))(TUtilBase,val)

#define THToNS(val) \
	(*(((TMODCALL TUINT16(**)(TAPTR,TUINT16))(TUtilBase))[-22]))(TUtilBase,val)

#define TGetModules(prefix,list,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,struct TList *,TTAGITEM *))(TUtilBase))[-23]))(TUtilBase,prefix,list,tags)

#define TParseArgs(template,argstring,argarray) \
	(*(((TMODCALL struct THandle *(**)(TAPTR,TSTRPTR,TSTRPTR,TTAG *))(TUtilBase))[-24]))(TUtilBase,template,argstring,argarray)

#define TGetArgs() \
	(*(((TMODCALL TSTRPTR(**)(TAPTR))(TUtilBase))[-25]))(TUtilBase)

#define TCreateHash(tags) \
	(*(((TMODCALL struct THash *(**)(TAPTR,TTAGITEM *))(TUtilBase))[-26]))(TUtilBase,tags)

#define TPutHash(a,key,value) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct THash *,TTAG,TTAG))(TUtilBase))[-27]))(TUtilBase,a,key,value)

#define TGetHash(a,key,valuep) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct THash *,TTAG,TTAG *))(TUtilBase))[-28]))(TUtilBase,a,key,valuep)

#define TRemHash(a,key) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct THash *,TTAG))(TUtilBase))[-29]))(TUtilBase,a,key)

#define THashToList(a,list) \
	(*(((TMODCALL TUINT(**)(TAPTR,struct THash *,struct TList *))(TUtilBase))[-30]))(TUtilBase,a,list)

#define THashUnList(a) \
	(*(((TMODCALL void(**)(TAPTR,struct THash *))(TUtilBase))[-31]))(TUtilBase,a)

#define TIsLeapYear(y) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TUINT))(TUtilBase))[-32]))(TUtilBase,y)

#define TIsValidDate(d,m,y) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TUINT,TUINT,TUINT))(TUtilBase))[-33]))(TUtilBase,d,m,y)

#define TYDayToDM(yday,y,pd,pm) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TUINT,TUINT,TUINT *,TUINT *))(TUtilBase))[-34]))(TUtilBase,yday,y,pd,pm)

#define TDMYToYDay(d,m,y) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT,TUINT))(TUtilBase))[-35]))(TUtilBase,d,m,y)

#define TMYToDay(m,y) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT))(TUtilBase))[-36]))(TUtilBase,m,y)

#define TDateToDMY(td,pD,pM,pY,pT) \
	(*(((TMODCALL void(**)(TAPTR,TDATE *,TUINT *,TUINT *,TUINT *,TTIME *))(TUtilBase))[-37]))(TUtilBase,td,pD,pM,pY,pT)

#define TGetWeekDay(d,m,y) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT,TUINT))(TUtilBase))[-38]))(TUtilBase,d,m,y)

#define TGetWeekNumber(d,m,y) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT,TUINT))(TUtilBase))[-39]))(TUtilBase,d,m,y)

#define TPackDate(db,td) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct TDateBox *,TDATE *))(TUtilBase))[-40]))(TUtilBase,db,td)

#define TUnpackDate(td,db,rf) \
	(*(((TMODCALL void(**)(TAPTR,TDATE *,struct TDateBox *,TUINT16))(TUtilBase))[-41]))(TUtilBase,td,db,rf)

#endif /* _TEK_INLINE_UTIL_H */
