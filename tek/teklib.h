
#ifndef _TEK_LIB_H
#define _TEK_LIB_H

/*
**	$Id: teklib.h,v 1.5 2005/09/13 02:44:06 tmueller Exp $
**	teklib/tek/teklib.h - Link library functions for bootstrapping
**	and for operating on elementary, public data structures
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

/*****************************************************************************/

#include <tek/exec.h>
#include <tek/mod/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef TCALLBACK TBOOL (*TTAGFOREACHFUNC)(TAPTR data, TTAGITEM *item);

extern TLIBAPI TAPTR TEKCreate(TTAGITEM *tags);
extern TLIBAPI TVOID TDestroy(TAPTR handle);
extern TLIBAPI TVOID TDestroyList(TLIST *list);
extern TLIBAPI TAPTR TNewInstance(TAPTR mod, TUINT possize, TUINT negsize);
extern TLIBAPI TVOID TFreeInstance(TAPTR mod);
extern TLIBAPI TVOID TInitVectors(TAPTR mod, const TAPTR *vectors, TUINT numv);
extern TLIBAPI TTAG TGetTag(TTAGITEM *taglist, TUINT tag, TTAG defvalue);
extern TLIBAPI TVOID TInitList(TLIST *list);
extern TLIBAPI TVOID TAddHead(TLIST *list, TNODE *node);
extern TLIBAPI TVOID TAddTail(TLIST *list, TNODE *node);
extern TLIBAPI TNODE *TRemHead(TLIST *list);
extern TLIBAPI TNODE *TRemTail(TLIST *list);
extern TLIBAPI TVOID TRemove(TNODE *node);
extern TLIBAPI TVOID TNodeUp(TNODE *node);
extern TLIBAPI TVOID TInsert(TLIST *list, TNODE *node, TNODE *prednode);
extern TLIBAPI TBOOL TForEachTag(TTAGITEM *taglist, TTAGFOREACHFUNC func,
	TAPTR data);
extern TLIBAPI struct THandle *TFindHandle(struct TList *list, TSTRPTR s2);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

/*
**	Revision History
**	$Log: teklib.h,v $
**	Revision 1.5  2005/09/13 02:44:06  tmueller
**	updated copyright reference
**	
**	Revision 1.4  2005/05/27 19:45:04  tmueller
**	added author to header
**	
**	Revision 1.3  2004/02/07 04:58:15  tmueller
**	Time support functions (add, sub, cmp) removed from the link library
**	
**	Revision 1.2  2003/12/12 03:46:20  tmueller
**	Return values do no longer try to emulate the crippled behavior of macros
**	
**	Revision 1.1.1.1  2003/12/11 07:17:42  tmueller
**	Krypton import
*/

#endif
