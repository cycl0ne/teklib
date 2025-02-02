
#ifndef _TEK_DEBUG_H
#define _TEK_DEBUG_H

/*
**	$Id: debug.h,v 1.2 2005/09/13 02:43:58 tmueller Exp $
**	tek/debug.h - Debug macros
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/type.h>

#ifdef TDEBUG
	#define	tdbprintf(l,x)				{if ((l) >= TDEBUG) platform_dbprintf(l,x);}
	#define	tdbprintf1(l,x,a)			{if ((l) >= TDEBUG) platform_dbprintf1(l,x,a);}
	#define	tdbprintf2(l,x,a,b)			{if ((l) >= TDEBUG) platform_dbprintf2(l,x,a,b);}
	#define	tdbprintf3(l,x,a,b,c)		{if ((l) >= TDEBUG) platform_dbprintf3(l,x,a,b,c);}
	#define	tdbprintf4(l,x,a,b,c,d)		{if ((l) >= TDEBUG) platform_dbprintf4(l,x,a,b,c,d);}
	#define	tdbprintf5(l,x,a,b,c,d,e)	{if ((l) >= TDEBUG) platform_dbprintf5(l,x,a,b,c,d,e);}
	#define	tdbprintf6(l,x,a,b,c,d,e,f)	{if ((l) >= TDEBUG) platform_dbprintf6(l,x,a,b,c,d,e,f);}
	#define tdbfatal(l)					{if ((l) >= TDEBUG) platform_fatal(l);}
#else
	#define	tdbprintf(l,x)
	#define	tdbprintf1(l,x,a)
	#define	tdbprintf2(l,x,a,b)
	#define	tdbprintf3(l,x,a,b,c)
	#define	tdbprintf4(l,x,a,b,c,d)
	#define	tdbprintf5(l,x,a,b,c,d,e)
	#define	tdbprintf6(l,x,a,b,c,d,e,f)
	#define tdbfatal(l)
#endif

/*****************************************************************************/
/*
**	Revision History
**	$Log: debug.h,v $
**	Revision 1.2  2005/09/13 02:43:58  tmueller
**	updated copyright reference
**	
**	Revision 1.1.1.1  2003/12/11 07:17:42  tmueller
**	Krypton import
**	
**	Revision 1.2  2003/10/19 03:38:18  tmueller
**	Changed exec structure field names: Node, List, Handle, Module, ModuleEntry
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
