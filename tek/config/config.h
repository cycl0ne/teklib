
#ifndef _TEK_CONFIG_H
#define	_TEK_CONFIG_H

/*
**	$Id: config.h,v 1.6 2005/09/13 02:44:34 tmueller Exp $
**	teklib/tek/config/config.h - Platform and compiler specific
**	See copyright notice in teklib/COPYRIGHT
*/

#ifdef TSYS_POSIX
	#include <tek/config/posix.h>
#endif

#ifdef TSYS_AMIGA
	#include <tek/config/amiga.h>
#endif

#ifdef TSYS_MORPHOS
	#include <tek/config/amiga.h>
#endif

#ifdef TSYS_INTENT
	#include <tek/config/intent.h>
#endif

#ifdef TSYS_WIN32
	#include <tek/config/win32.h>
#endif

#ifdef TSYS_DARWIN
	#include <tek/config/posix.h>
#endif

#ifdef TSYS_PS2
	#include <tek/config/ps2.h>
#endif

/*****************************************************************************/
/*
**	Module interface
**
**	- Depending on your module build, you may eventually want to override
**	  TMODAPI to "static". module functions are not exported symbolically
**	  in TEKlib.
**
**	- Depending on your host, you may have to override TMODENTRY with things
**	  like __declspec(dllexport). See config/platforms.h for examples.
**
**	- Depending on your preferred calling conventions, override TMODCALL with
**	  things like __stdargs, __fastcall etc. Some platforms allow or require
**	  to differentiate between register and stack-based calling. When porting
**	  TEKlib to a new platform, please try to define the calling conventions
**	  for the entire platform, not only for a single compiler.
*/

#ifndef TMODAPI
#define TMODAPI
#endif

#ifndef TLIBAPI
#define TLIBAPI
#endif

#ifndef TMODCALL
#define TMODCALL
#endif

#ifndef TMODENTRY
#define TMODENTRY
#endif

/*****************************************************************************/
/* 
**	Callbacks and task entries
**
**	In config/yourplatform.h, override these with your preferred
**	platform-specific calling conventions. See also the annotations
**	for the module interface above. Stack/register calling conventions
**	may be an issue here.
*/

#ifndef TCALLBACK
#define TCALLBACK
#endif

#ifndef TTASKENTRY
#define TTASKENTRY
#endif

/*****************************************************************************/
/* 
**	Inline
**	Override with __inline etc. where available
*/

#ifndef TINLINE
#define TINLINE
#endif

/*****************************************************************************/
/*
**	Revision History
**	$Log: config.h,v $
**	Revision 1.6  2005/09/13 02:44:34  tmueller
**	updated copyright reference
**	
**	Revision 1.5  2005/01/29 22:27:35  tmueller
**	TSYS_POSIX32 renamed to TSYS_POSIX
**	
**	Revision 1.4  2004/08/10 22:05:05  fschulze
**	added #ifdef for TSYS_PS2
**	
**	Revision 1.3  2004/02/07 04:56:01  tmueller
**	Added TINLINE definition
**	
**	Revision 1.2  2003/12/17 15:23:09  mlukat
**	changed Elate to intent
**	
**	Revision 1.1.1.1  2003/12/11 07:18:07  tmueller
**	Krypton import
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.2  2003/02/13 18:27:06  cybin
**	darwin port now compiles with -DTSYS_DARWIN and uses correct
**	host and platform values.
**	
**	added files for io module. should be the same like posix32.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
