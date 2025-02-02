
#ifndef _TEK_PROTO_DSTRING_H
#define _TEK_PROTO_DSTRING_H

/*
**	$Id: dstring.h,v 1.5 2005/09/13 02:45:20 tmueller Exp $
**	teklib/tek/proto/dstring.h - Dynamic strings module interface
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/dstring.h>

extern TMODENTRY TUINT 
tek_init_dstring(TAPTR, struct TModule *, TUINT16, TTAGITEM *);

/*****************************************************************************/

#define ds_newarray(ds,s,len)			(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TINT))((TAPTR*)(ds))[-1]))(ds,s,len)
#define ds_delarray(ds,s)				(*((TMODCALL TVOID(*)(TAPTR,TDSTR*))((TAPTR*)(ds))[-2]))(ds,s)
#define ds_arraysetlen(ds,s,len)		(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TINT))((TAPTR*)(ds))[-3]))(ds,s,len)
#define ds_newstring(ds)				(*((TMODCALL TDSTR(*)(TAPTR))((TAPTR*)(ds))[-4]))(ds)
#define ds_newstringstr(ds,str)			(*((TMODCALL TDSTR(*)(TAPTR,TSTRPTR))((TAPTR*)(ds))[-5]))(ds,str)
#define _ds_stringdup(ds,str)			(*((TMODCALL TDSTR(*)(TAPTR,TDSTR*))((TAPTR*)(ds))[-6]))(ds,str)
#define _ds_stringcpy(ds,d,s)			(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TDSTR*))((TAPTR*)(ds))[-7]))(ds,d,s)
#define _ds_stringcpystr(ds,d,str)		(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TSTRPTR))((TAPTR*)(ds))[-8]))(ds,d,str)
#define _ds_stringncpy(ds,d,s,max)		(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TDSTR*,TINT))((TAPTR*)(ds))[-9]))(ds,d,s,max)
#define _ds_stringncpystr(ds,d,str,max)	(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TSTRPTR,TINT))((TAPTR*)(ds))[-10]))(ds,d,str,max)
#define _ds_stringcat(ds,d,s)			(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TDSTR*))((TAPTR*)(ds))[-11]))(ds,d,s)
#define _ds_stringcatstr(ds,d,str)		(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TSTRPTR))((TAPTR*)(ds))[-12]))(ds,d,str)
#define _ds_stringncat(ds,d,s,max)		(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TDSTR*,TINT))((TAPTR*)(ds))[-13]))(ds,d,s,max)
#define _ds_stringncatstr(ds,d,str,max)	(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TSTRPTR,TINT))((TAPTR*)(ds))[-14]))(ds,d,str,max)
#define _ds_stringcatchr(ds,d,c)		(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TINT8))((TAPTR*)(ds))[-15]))(ds,d,c)
#define _ds_stringtrunc(ds,s,max)		(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TINT))((TAPTR*)(ds))[-16]))(ds,s,max)
#define _ds_stringexpstr(ds,s,max,estr,elen)	(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TINT,TSTRPTR,TINT))((TAPTR*)(ds))[-17]))(ds,s,max,estr,elen)
#define _ds_stringexpchr(ds,s,max,c)	(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TINT,TINT8))((TAPTR*)(ds))[-18]))(ds,s,max,c)
#define _ds_stringexp(ds,s,max,es)		(*((TMODCALL TINT(*)(TAPTR,TDSTR*,TINT,TDSTR*))((TAPTR*)(ds))[-19]))(ds,s,max,es)
#define _ds_strcmp(ds,str1,str2)		(*((TMODCALL TINT(*)(TAPTR,TSTRPTR,TSTRPTR))((TAPTR*)(ds))[-20]))(ds,str1,str2)
#define _ds_strfind(ds,str1,str2)		(*((TMODCALL TINT(*)(TAPTR,TSTRPTR,TSTRPTR))((TAPTR*)(ds))[-21]))(ds,str1,str2)

/*****************************************************************************/

#define ds_stringdup(ds,s)				_ds_stringdup(ds,&(s))
#define ds_delstring(ds,s)				ds_delarray(ds,&(s))
#define ds_stringlen(ds,s)				(_ds_arraylen(&(s))-1)
#define ds_stringvalid(ds,s)			_ds_arrayvalid(&(s))
#define ds_string(ds,s)					_ds_arrayptr(&(s))

#define ds_stringcpy(ds,d,s)			_ds_stringcpy(ds,&(d),&(s))
#define ds_stringcpystr(ds,d,s)			_ds_stringcpystr(ds,&(d),s)
#define ds_stringncpy(ds,d,s,l)			_ds_stringncpy(ds,&(d),&(s),l)
#define ds_stringncpystr(ds,d,s,l)		_ds_stringncpystr(ds,&(d),s,l)

#define ds_stringcat(ds,s1,s2)			_ds_stringcat(ds,&(s1),&(s2))
#define ds_stringcatstr(ds,s1,s2)		_ds_stringcatstr(ds,&(s1),s2)
#define ds_stringncat(ds,s1,s2,l)		_ds_stringncat(ds,&(s1),&(s2),l)
#define ds_stringncatstr(ds,s1,s2,l)	_ds_stringncatstr(ds,&(s1),s2,l)
#define ds_stringcatchr(ds,s,c)			_ds_stringcatchr(ds,&(s),c)

#define ds_stringtrunc(ds,s,l)			_ds_stringtrunc(ds,&(s),l)
#define ds_stringexp(ds,s1,l,s2)		_ds_stringexp(ds,&(s1),l,&(s2))
#define ds_stringexpstrn(ds,s1,l,s2,l2)	_ds_stringexpstr(ds,&(s1),l,s2,l2)
#define ds_stringexpchr(ds,s1,l,c)		_ds_stringexpchr(ds,&(s1),l,c)

#define ds_stringcmp(ds,s1,s2)			_ds_strcmp(ds,_ds_arrayptr(&(s1)),_ds_arrayptr(&(s2)))
#define ds_stringcmpstr(ds,s1,s2)		_ds_strcmp(ds,_ds_arrayptr(&(s1)),s2)

#define ds_stringfind(ds,s1,s2,pos)		_ds_strfind(ds,_ds_arrayptr(&(s1))+(pos),_ds_arrayptr(&(s2)))
#define ds_stringfindstr(ds,s1,s2,pos)	_ds_strfind(ds,_ds_arrayptr(&(s1))+(pos),s2)

/*#define ds_strcmpstring(ds,s1,s2)		_ds_strcmp(ds,s1,_ds_arrayptr(&(s2)))*/
/*#define ds_strcmp(ds,s1,s2)				_ds_strcmp(ds,s1,s2)*/

/*****************************************************************************/
/*
**	Revision History
**	$Log: dstring.h,v $
**	Revision 1.5  2005/09/13 02:45:20  tmueller
**	updated copyright reference
**	
**	Revision 1.4  2004/07/16 20:34:16  tmueller
**	added positional argument to ds_strfind()
**	
**	Revision 1.3  2004/07/15 14:56:12  tmueller
**	added ds_stringfind()
**	
**	Revision 1.2  2004/07/05 21:35:25  tmueller
**	fixed ds_stringncat... and removed some protos
**	
**	Revision 1.1.1.1  2003/12/11 07:18:05  tmueller
**	Krypton import
**	
*/

#endif
