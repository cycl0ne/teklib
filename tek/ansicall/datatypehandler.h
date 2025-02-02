#ifndef _TEK_ANSICALL_DATATYPEHANDLER_H
#define _TEK_ANSICALL_DATATYPEHANDLER_H

/*
**	$Id: datatypehandler.h,v 1.2 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/datatypehandler.h - datatypehandler module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TDthOpen(dth,tags) \
	(*(((TMODCALL THNDL *(**)(TAPTR,TTAGITEM *))(dth))[-1]))(dth,tags)

#define TDthGetAttrs(dth,handle) \
	(*(((TMODCALL TTAGITEM *(**)(TAPTR,THNDL *))(dth))[-2]))(dth,handle)

#define TDthDoMethod(dth,handle,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,THNDL *,TTAGITEM *))(dth))[-3]))(dth,handle,tags)

#define TDthListDatatypes(dth,filtertags) \
	(*(((TMODCALL TLIST *(**)(TAPTR,TTAGITEM *))(dth))[-4]))(dth,filtertags)

#define TDthSimpleLoadPicture(dth,filename,pic) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TIMGPICTURE *))(dth))[-5]))(dth,filename,pic)

#endif /* _TEK_ANSICALL_DATATYPEHANDLER_H */
