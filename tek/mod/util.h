
#ifndef _TEK_MOD_UTIL_H
#define _TEK_MOD_UTIL_H

/*
**	$Id: util.h,v 1.4 2005/09/13 02:45:09 tmueller Exp $
**	teklib/tek/mod/util.h - Utility module types and definitions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/type.h>

/*****************************************************************************/
/* 
**	Callback functions
*/

typedef TCALLBACK TINT (*TCMPFUNC)(TAPTR userdata, TTAG arg1, TTAG arg2);
typedef TCALLBACK TINT (*TFINDFUNC)(TAPTR userdata, TTAG ref);

/*****************************************************************************/
/* 
**	Module entry node, as returned by TUtilGetModules()
*/

struct TModuleEntry
{
	struct TModObject tme_Handle;	/* Object handle */
	TTAGITEM *tme_Tags;				/* Reserved for future extensions */
};

/*****************************************************************************/
/*
**	Revision History
**	$Log: util.h,v $
**	Revision 1.4  2005/09/13 02:45:09  tmueller
**	updated copyright reference
**	
**	Revision 1.3  2005/06/29 09:09:15  tmueller
**	changed types of TCMPFUNC, TFINDFUNC, HeapSort refarray
**	
**	Revision 1.2  2003/12/12 03:50:15  tmueller
**	Email address in headers fixed
**	
**	Revision 1.1.1.1  2003/12/11 07:17:50  tmueller
**	Krypton import
**	
**	Revision 1.4  2003/10/30 20:07:09  dtrompetter
**	added util_qsort
**	
**	Revision 1.3  2003/10/22 03:17:10  tmueller
**	Removed util_itoa and util_atoi, made util_strol and util_strtod public,
**	documented them, cleaned up util module documentation
**	
**	Revision 1.2  2003/10/22 03:14:48  tmueller
**	mods/util/util_mod.c
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif

