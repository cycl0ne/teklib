#ifndef _TEK_STDCALL_UTIL_H
#define _TEK_STDCALL_UTIL_H

/*
**	teklib/tek/stdcall/util.h - util module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TUtilGetArgC(util) \
	(*(((TMODCALL TINT(**)(TAPTR))(util))[-9]))(util)

#define TUtilGetArgV(util) \
	(*(((TMODCALL TSTRPTR *(**)(TAPTR))(util))[-10]))(util)

#define TUtilSetRetVal(util,retval) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TINT))(util))[-11]))(util,retval)

#define TUtilHeapSort(util,refarray,len,cmphook) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR,TSIZE,struct THook *))(util))[-12]))(util,refarray,len,cmphook)

#define TUtilSeekList(util,node,steps) \
	(*(((TMODCALL struct TNode *(**)(TAPTR,struct TNode *,TINTPTR))(util))[-13]))(util,node,steps)

#define TUtilIsBigEndian(util) \
	(*(((TMODCALL TBOOL(**)(TAPTR))(util))[-14]))(util)

#define TUtilBSwap16(util,valp) \
	(*(((TMODCALL void(**)(TAPTR,TUINT16 *))(util))[-15]))(util,valp)

#define TUtilBSwap32(util,valp) \
	(*(((TMODCALL void(**)(TAPTR,TUINT *))(util))[-16]))(util,valp)

#define TUtilStrDup(util,mm,str) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,struct TMemManager *,TSTRPTR))(util))[-17]))(util,mm,str)

#define TUtilStrNDup(util,mm,str,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,struct TMemManager *,TSTRPTR,TSIZE))(util))[-18]))(util,mm,str,len)

#define TUtilGetRand(util,seed) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(util))[-19]))(util,seed)

#define TUtilParseArgV(util,tmpl,argv,argarray) \
	(*(((TMODCALL struct THandle *(**)(TAPTR,TSTRPTR,TSTRPTR *,TTAG *))(util))[-20]))(util,tmpl,argv,argarray)

#define TUtilHToNL(util,val) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(util))[-21]))(util,val)

#define TUtilHToNS(util,val) \
	(*(((TMODCALL TUINT16(**)(TAPTR,TUINT16))(util))[-22]))(util,val)

#define TUtilGetModules(util,prefix,list,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,struct TList *,TTAGITEM *))(util))[-23]))(util,prefix,list,tags)

#define TUtilParseArgs(util,template,argstring,argarray) \
	(*(((TMODCALL struct THandle *(**)(TAPTR,TSTRPTR,TSTRPTR,TTAG *))(util))[-24]))(util,template,argstring,argarray)

#define TUtilGetArgs(util) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR))(util))[-25]))(util)

#define TUtilCreateHash(util,tags) \
	(*(((TMODCALL struct THash *(**)(TAPTR,TTAGITEM *))(util))[-26]))(util,tags)

#define TUtilPutHash(util,a,key,value) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct THash *,TTAG,TTAG))(util))[-27]))(util,a,key,value)

#define TUtilGetHash(util,a,key,valuep) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct THash *,TTAG,TTAG *))(util))[-28]))(util,a,key,valuep)

#define TUtilRemHash(util,a,key) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct THash *,TTAG))(util))[-29]))(util,a,key)

#define TUtilHashToList(util,a,list) \
	(*(((TMODCALL TUINT(**)(TAPTR,struct THash *,struct TList *))(util))[-30]))(util,a,list)

#define TUtilHashUnList(util,a) \
	(*(((TMODCALL void(**)(TAPTR,struct THash *))(util))[-31]))(util,a)

#define TUtilIsLeapYear(util,y) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TUINT))(util))[-32]))(util,y)

#define TUtilIsValidDate(util,d,m,y) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TUINT,TUINT,TUINT))(util))[-33]))(util,d,m,y)

#define TUtilYDayToDM(util,yday,y,pd,pm) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TUINT,TUINT,TUINT *,TUINT *))(util))[-34]))(util,yday,y,pd,pm)

#define TUtilDMYToYDay(util,d,m,y) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT,TUINT))(util))[-35]))(util,d,m,y)

#define TUtilMYToDay(util,m,y) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT))(util))[-36]))(util,m,y)

#define TUtilDateToDMY(util,td,pD,pM,pY,pT) \
	(*(((TMODCALL void(**)(TAPTR,TDATE *,TUINT *,TUINT *,TUINT *,TTIME *))(util))[-37]))(util,td,pD,pM,pY,pT)

#define TUtilGetWeekDay(util,d,m,y) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT,TUINT))(util))[-38]))(util,d,m,y)

#define TUtilGetWeekNumber(util,d,m,y) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT,TUINT))(util))[-39]))(util,d,m,y)

#define TUtilPackDate(util,db,td) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct TDateBox *,TDATE *))(util))[-40]))(util,db,td)

#define TUtilUnpackDate(util,td,db,rf) \
	(*(((TMODCALL void(**)(TAPTR,TDATE *,struct TDateBox *,TUINT16))(util))[-41]))(util,td,db,rf)

#endif /* _TEK_STDCALL_UTIL_H */
