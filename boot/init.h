
#ifndef _TEK_INIT_H
#define _TEK_INIT_H

/*
**	$Id: init.h,v 1.4 2005/09/13 02:41:00 tmueller Exp $
**	teklib/boot/init.h - Startup library definitions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/mod/hal.h>

/*****************************************************************************/
/*
**	Initdata. This packet will be attached to
**	exectask's userdata during the lifetime of apptask.
*/

typedef struct
{
	TAPTR boot;						/* inital boot handle */
	TAPTR halhalmod;				/* host-specific ptr to hal module */
	TAPTR halexecmod;				/* host-specific ptr to exec module */
	TAPTR halentry;					/* hal entry function */
	TAPTR execentry;				/* exec entry function */
	TAPTR halbase;					/* hal module base */
	struct TModule *execbase;		/* exec module base */
	TSTRPTR progdir;				/* PROGDIR: */
	TSTRPTR sysdir;					/* SYS: */
	TSTRPTR moddir;					/* system-wide module directory */
	struct THALObject execthread;	/* host-specific execbase thread */
	struct THALObject initevent;	/* signals apptask that exec is running */
	struct TTask *exectask;			/* execbase task handle */
	THNDL *apptask;					/* application task handle */
	TAPTR iotask;					/* io task handle */
	TDFUNC orgapptaskdestructor;	/* org. destructor for app task */
	TTAGITEM haltags[7];			/* attributes for HAL init */
	TTAGITEM exectags[3];			/* attributes for Exec init */
	TAPTR atom_argv;				/* named atom holding the argv vector */
	TAPTR atom_retvalp;				/* named atom holding a ptr to retvalue */
	TAPTR atom_unique;				/* named atom holding a ptr to unique ID */
	TAPTR atom_imods;				/* named atom holding initmodules */
	TAPTR atom_args;				/* named atom holding argument string */
	TAPTR atom_progname;			/* named atom holding progname */
	TUINT uniqueID;					/* unique ID */

} INITDATA;

/*****************************************************************************/
/*
**	User entrypoint
*/

extern TTASKENTRY TVOID TEKMain(TAPTR task);

/*****************************************************************************/
/*
**	Host-specific prototypes
*/

TAPTR boot_init(TTAGITEM *tags);
TVOID boot_exit(TAPTR handle);
TAPTR boot_alloc(TAPTR handle, TUINT size);
TVOID boot_free(TAPTR handle, TAPTR mem, TUINT size);
TVOID boot_freevec(TAPTR handle, TAPTR mem);
TSTRPTR boot_getsysdir(TAPTR handle, TTAGITEM *tags);
TSTRPTR boot_getmoddir(TAPTR handle, TTAGITEM *tags);
TSTRPTR boot_getprogdir(TAPTR handle, TTAGITEM *tags);
TAPTR boot_loadmodule(TAPTR handle, TSTRPTR progdir, TSTRPTR moddir,
	TSTRPTR modname, TTAGITEM *tags);
TVOID boot_closemodule(TAPTR handle, TAPTR halmod);
TAPTR boot_getentry(TAPTR handle, TAPTR halmod, TSTRPTR name);
TUINT boot_callmod(TAPTR handle, TAPTR ModBase, TAPTR entry, TAPTR task,
	TAPTR mod, TUINT16 version, TTAGITEM *tags);

/*****************************************************************************/
/*
**	Revision History
**	$Log: init.h,v $
**	Revision 1.4  2005/09/13 02:41:00  tmueller
**	updated copyright reference
**	
**	Revision 1.3  2005/04/01 18:36:22  tmueller
**	A boot-specific object handle is now passed to all boot functions and later
**	handed over to the HAL module, where it helps to abstract from HW resources
**	
**	Revision 1.2  2004/07/04 17:16:14  tmueller
**	added sys.arguments and sys.progname atoms
**	
**	Revision 1.1.1.1  2003/12/11 07:18:21  tmueller
**	Krypton import
**	
**	Revision 1.3  2003/10/28 21:23:25  tmueller
**	Standard headers added, cleanup (formatting and style)
**	
**	Revision 1.2  2003/09/17 16:49:24  tmueller
**	boot_loadmodule prototype changed
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.2  2003/02/02 03:47:50  tmueller
**	Initialization of application-global named atoms (argv, retvalp, uniqueid)
**	now takes place in the boot library, no longer in the exec module
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
