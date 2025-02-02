#ifndef _TEK_ANSICALL_UTIL_H
#define _TEK_ANSICALL_UTIL_H

/*
**	$Id: util.h,v 1.7 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/util.h - util module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TUtilGetArgC(util) \
	(*(((TMODCALL TINT(**)(TAPTR))(util))[-9]))(util)

#define TUtilGetArgV(util) \
	(*(((TMODCALL TSTRPTR *(**)(TAPTR))(util))[-10]))(util)

#define TUtilSetRetVal(util,retval) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TINT))(util))[-11]))(util,retval)

#define TUtilGetUniqueID(util,ext) \
	(*(((TMODCALL TUINT(**)(TAPTR,TAPTR))(util))[-12]))(util,ext)

#define TUtilGetRandObs(util) \
	(*(((TMODCALL TINT(**)(TAPTR))(util))[-13]))(util)

#define TUtilSetRandObs(util,seed) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT))(util))[-14]))(util,seed)

#define TUtilHeapSort(util,data,ref,len,cmp) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR,TTAG *,TUINT,TCMPFUNC))(util))[-15]))(util,data,ref,len,cmp)

#define TUtilSeekNode(util,node,steps) \
	(*(((TMODCALL TNODE *(**)(TAPTR,TNODE *,TINT))(util))[-16]))(util,node,steps)

#define TUtilInsertSorted(util,list,numentries,nnode,cmp,data) \
	(*(((TMODCALL TVOID(**)(TAPTR,TLIST *,TUINT,TNODE *,TCMPFUNC,TAPTR))(util))[-17]))(util,list,numentries,nnode,cmp,data)

#define TUtilFindSorted(util,list,numentries,find,data) \
	(*(((TMODCALL TNODE *(**)(TAPTR,TLIST *,TUINT,TFINDFUNC,TAPTR))(util))[-18]))(util,list,numentries,find,data)

#define TUtilIsBigEndian(util) \
	(*(((TMODCALL TBOOL(**)(TAPTR))(util))[-19]))(util)

#define TUtilBSwap16(util,valp) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT16 *))(util))[-20]))(util,valp)

#define TUtilBSwap32(util,valp) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT *))(util))[-21]))(util,valp)

#define TUtilStrLen(util,str) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR))(util))[-22]))(util,str)

#define TUtilStrCpy(util,dst,src) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR))(util))[-23]))(util,dst,src)

#define TUtilStrNCpy(util,dst,src,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR,TINT))(util))[-24]))(util,dst,src,len)

#define TUtilStrCat(util,dst,src) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR))(util))[-25]))(util,dst,src)

#define TUtilStrNCat(util,dst,src,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR,TINT))(util))[-26]))(util,dst,src,len)

#define TUtilStrCmp(util,s1,s2) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR))(util))[-27]))(util,s1,s2)

#define TUtilStrNCmp(util,s1,s2,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR,TINT))(util))[-28]))(util,s1,s2,len)

#define TUtilStrCaseCmp(util,s1,s2) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR))(util))[-29]))(util,s1,s2)

#define TUtilStrNCaseCmp(util,s1,s2,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR,TINT))(util))[-30]))(util,s1,s2,len)

#define TUtilStrStr(util,s1,s2) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR))(util))[-31]))(util,s1,s2)

#define TUtilStrChr(util,str,chr) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TINT))(util))[-32]))(util,str,chr)

#define TUtilStrRChr(util,str,chr) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TINT))(util))[-33]))(util,str,chr)

#define TUtilStrDup(util,mmu,str) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TAPTR,TSTRPTR))(util))[-34]))(util,mmu,str)

#define TUtilStrNDup(util,mmu,str,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TAPTR,TSTRPTR,TINT))(util))[-35]))(util,mmu,str,len)

#define TUtilStrToI(util,str,valp) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TINT *))(util))[-36]))(util,str,valp)

#define TUtilGetRand(util,seed) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(util))[-37]))(util,seed)

#define TUtilParseArgV(util,tmpl,argv,argarray) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TSTRPTR *,TTAG *))(util))[-38]))(util,tmpl,argv,argarray)

#define TUtilHToNL(util,val) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(util))[-39]))(util,val)

#define TUtilHToNS(util,val) \
	(*(((TMODCALL TUINT16(**)(TAPTR,TUINT16))(util))[-40]))(util,val)

#define TUtilStrToD(util,str,valp) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TDOUBLE *))(util))[-41]))(util,str,valp)

#define TUtilQSort(util,array,num,size,cmp,udata) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR,TINT,TINT,TCMPFUNC,TAPTR))(util))[-42]))(util,array,num,size,cmp,udata)

#define TUtilGetModules(util,prefix,list,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TLIST *,TTAGITEM *))(util))[-43]))(util,prefix,list,tags)

#define TUtilParseArgs(util,template,argstring,argarray) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TSTRPTR,TTAG *))(util))[-44]))(util,template,argstring,argarray)

#define TUtilGetArgs(util) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR))(util))[-45]))(util)

#endif /* _TEK_ANSICALL_UTIL_H */
