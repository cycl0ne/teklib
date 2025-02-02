#ifndef _TEK_ANSICALL_DATATYPE_IDENT_H
#define _TEK_ANSICALL_DATATYPE_IDENT_H

/*
**	$Id: datatype_ident.h,v 1.2 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/datatype_ident.h - datatype_ident module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define datatype_ident_getidentdata(datatype_ident,a) \
	(*(((TMODCALL TVOID(**)(TAPTR,DTIdentifyData*))(datatype_ident))[-1]))(datatype_ident,a)

#endif /* _TEK_ANSICALL_DATATYPE_IDENT_H */
