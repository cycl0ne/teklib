
/*
**	$Id: host.c,v 1.3 2005/04/01 18:36:22 tmueller Exp $
**	boot/amiga/host.c - Amiga/MorphOS startup implementation
*/

#include "init.h"

#include <string.h>
#include <exec/memory.h>
#include <exec/exec.h>
#include <exec/libraries.h>
#include <proto/exec.h>

#define SysBase *((struct ExecBase **) 4L)

struct ModHandle
{
	TAPTR entry;
	TUINT type;
};

#define TYPE_DLL	0
#define TYPE_LIB	1

/*****************************************************************************/
/*
**	lookup internal module
*/

static struct TInitModule *
lookupmodule(TSTRPTR modname, TTAGITEM *tags)
{
	struct TInitModule *imod = 
		(struct TInitModule *) TGetTag(tags, TExecBase_ModInit, TNULL);
	if (imod)
	{
		while (imod->tinm_Name)
		{
			if (!strcmp(imod->tinm_Name, modname))
			{
				return imod;
			}
			imod++;
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	host init/exit
*/

TAPTR 
boot_init(TTAGITEM *tags)
{
	return (TAPTR) 1;
}

TVOID
boot_exit(TAPTR boot)
{
}

/*****************************************************************************/
/*
**	host alloc/free
*/

TAPTR 
boot_alloc(TAPTR boot, TUINT size)
{
	return AllocVec(size, MEMF_ANY);
}

TVOID boot_free(TAPTR boot, TAPTR mem, TUINT size)
{
	FreeVec(mem);
}

TVOID
boot_freevec(TAPTR boot, TAPTR mem)
{
	FreeVec(mem);
}

/*****************************************************************************/
/*
**	determine TEKlib global system directory
*/

TSTRPTR
boot_getsysdir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR sysdir;
	TSTRPTR s;
	TINT l;
	
	s = (TSTRPTR) TGetTag(tags, TExecBase_SysDir, (TTAG) TEKHOST_SYSDIR);
	l = strlen(s);

	sysdir = boot_alloc(boot, l + 1);
	if (sysdir)
	{
		strcpy(sysdir, s);
	}
	return sysdir;
}

/*****************************************************************************/
/*
**	determine TEKlib global module directory
*/

TSTRPTR
boot_getmoddir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR sysdir;
	TSTRPTR s;
	TINT l;
	
	s = (TSTRPTR) TGetTag(tags, TExecBase_ModDir, (TTAG) TEKHOST_MODDIR);
	l = strlen(s);

	sysdir = boot_alloc(boot, l + 1);
	if (sysdir)
	{
		strcpy(sysdir, s);
	}
	return sysdir;
}

/*****************************************************************************/
/*
**	determine the path to the application, which will
**	later resolve to "PROGDIR:" in teklib semantics.
*/

TSTRPTR
boot_getprogdir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR progdir;
	TSTRPTR s;
	TINT l;
	
	s = (TSTRPTR) TGetTag(tags, TExecBase_ProgDir, (TTAG) TEKHOST_PROGDIR);
	l = strlen(s);

	progdir = boot_alloc(boot, l + 1);
	if (progdir)
	{
		strcpy(progdir, s);
	}
	return progdir;
}

/*****************************************************************************/
/*
**	load a module. first try progdir/modname, then moddir
*/

TAPTR
boot_loadmodule(TAPTR boot, TSTRPTR progdir, TSTRPTR moddir, TSTRPTR modname,
	TTAGITEM *tags)
{
	TAPTR halmod = TNULL;
	TINT len1, len2, len3, len4;
	TSTRPTR t;
	struct TInitModule *imod;
	struct ModHandle *handle;

	handle = boot_alloc(boot, sizeof(struct ModHandle));
	if (!handle) return TNULL;

	imod = lookupmodule(modname, tags);
	if (imod)
	{
		handle->entry = (TAPTR) imod->tinm_InitFunc;
		handle->type = TYPE_LIB;
		return handle;
	}
	
	len1 = progdir? strlen(progdir) : 0;
	len2 = moddir? strlen(moddir) : 0;
	len3 = strlen(modname);
	
	/* + mod/ + .mod + \0 */
	len4 = TMAX(len1, len2) + len3 + 4 + TEKHOST_EXTLEN + 1;
	t = boot_alloc(boot, len4);
	if (t)
	{
		if (progdir) strcpy(t, progdir);
		strcpy(t + len1, "mod/");
		strcpy(t + len1 + 4, modname);
		strcpy(t + len1 + 4 + len3, TEKHOST_EXTSTR);

		tdbprintf1(2,"trying OpenLibrary %s\n", t);
		halmod = OpenLibrary(t, 0);
		if (!halmod)
		{
			if (moddir) strcpy(t, moddir);
			strcpy(t + len2, modname);
			strcpy(t + len2 + len3, TEKHOST_EXTSTR);

			tdbprintf1(2,"trying OpenLibrary %s\n", t);
			halmod = OpenLibrary(t, 0);
			if (!halmod) tdbprintf1(20,"OpenLibrary %s failed\n", modname);
		}
		boot_free(boot, t, len4);
	}

	if (halmod)
	{
		handle->entry = halmod;
		handle->type = TYPE_DLL;
	}
	else
	{
		boot_free(boot, handle, sizeof(struct ModHandle));
		handle = TNULL;
	}

	return handle;
}

/*****************************************************************************/
/*
**	close module
*/

TVOID
boot_closemodule(TAPTR boot, TAPTR halmod)
{
	struct ModHandle *handle = halmod;
	if (handle->type == TYPE_DLL) CloseLibrary(handle->entry);
	boot_free(boot, handle, 0);		/* size is dummy here */
}

/*****************************************************************************/
/*
**	get module entry
*/

TAPTR
boot_getentry(TAPTR boot, TAPTR halmod, TSTRPTR name)
{
	return (TAPTR) 1;		/* "success" */
}

/*****************************************************************************/
/*
**	call module
*/

TUINT
boot_callmod(TAPTR boot, TAPTR halmod, TAPTR entry, TAPTR task, TAPTR mod, TUINT16 version,
	TTAGITEM *tags)
{
	struct ModHandle *handle = halmod;
	if (handle->type == TYPE_DLL)
	{
		TAPTR ModBase = handle->entry;
		return tek_mod_enter(task, mod, version, tags);
	}
	else
	{
		return (*((TMODINITFUNC) handle->entry))(task, mod, version, tags);
	}
}

#undef SysBase

/*****************************************************************************/
/*
**	Revision History
**	$Log: host.c,v $
**	Revision 1.3  2005/04/01 18:36:22  tmueller
**	A boot-specific object handle is now passed to all boot functions and later
**	handed over to the HAL module, where it helps to abstract from HW resources
**	
**	Revision 1.2  2004/04/18 13:57:53  tmueller
**	arguments in parseargv, atomdata, gettag changed from TAPTR to TTAG
**	
**	Revision 1.1.1.1  2003/12/11 07:18:22  tmueller
**	Krypton import
**	
**	Revision 1.6  2003/10/28 21:23:25  tmueller
**	Standard headers added, cleanup (formatting and style)
**	
**	Revision 1.5  2003/10/23 11:37:35  dtrompetter
**	includes resorted
**	
**	Revision 1.4  2003/10/12 19:19:09  tmueller
**	TInitModule fields have been prefixed with tinm_
**	
**	Revision 1.3  2003/09/17 16:48:36  tmueller
**	Modules linked statically to the framework are fully supported again
**	
**	Revision 1.2  2003/03/08 21:49:49  tmueller
**	The boot libraries now respect the path defines TEKHOST_...DIR
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/
