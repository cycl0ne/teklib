
#ifndef _TEK_MOD_HAL_H
#define _TEK_MOD_HAL_H

/*
**	$Id: hal.h,v 1.2 2005/09/13 02:45:09 tmueller Exp $
**	teklib/tek/mod/hal.h - HAL module private
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/type.h>

/*****************************************************************************/
/* 
**	Access macros for the THALO ("generic HAL object") type.
**	Some platform-specific structures may fit into sizeof(THALO),
**	in which case an allocation can be avoided. The compiler should
**	optimize out the non-applicable case.
*/

#define THALNewObject(hal,obj,type) 	\
	(type *)((sizeof(THALO)>=sizeof(type))?(obj):hal_alloc(hal,sizeof(type)))

#define THALDestroyObject(hal,obj,type)	\
	if(sizeof(THALO)<sizeof(type))hal_free(hal,obj,sizeof(type))

#define THALSetObject(obj,type,obj2)	\
	if(sizeof(THALO)<sizeof(type))*((type **)obj)=obj2;

#define THALGetObject(obj,type)			\
	((sizeof(THALO)>=sizeof(type))?(type *)(obj):*((type **)(obj)))

/*****************************************************************************/
/*
**	Revision History
**	$Log: hal.h,v $
**	Revision 1.2  2005/09/13 02:45:09  tmueller
**	updated copyright reference
**	
**	Revision 1.1.1.1  2003/12/11 07:17:47  tmueller
**	Krypton import
**	
**	Revision 1.2  2003/10/28 08:54:42  tmueller
**	Reworked HAL-internal structures
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
