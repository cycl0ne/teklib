/*
**	$Id: host.c,v 1.3 2005/04/01 18:36:22 tmueller Exp $
**	boot/unix/host.c - Unix implementation of startup functions
*/

#include "init.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/param.h>

#ifdef PATH_MAX
#define MAX_PATH_LEN	PATH_MAX
#else
#define MAX_PATH_LEN	MAXPATHLEN
#endif

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
	TAPTR mem = malloc(size);
	return mem;
}

TVOID
boot_free(TAPTR boot, TAPTR mem, TUINT size)
{
	if (mem) free(mem);
}

TVOID
boot_freevec(TAPTR boot, TAPTR mem)
{
	if (mem) free(mem);
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
	TSTRPTR progdir = TNULL;
	TSTRPTR *argv;
	TINT argc;

	progdir = (TSTRPTR) TGetTag(tags, TExecBase_ProgDir, TEKHOST_PROGDIR);
	if (progdir)
	{
		TINT l = strlen(progdir);
		TSTRPTR p = boot_alloc(boot, l + 1);
		if (p)
		{
			strcpy(p, progdir);
			return p;
		}
		return TNULL;
	}
	
	argc = (TINT) TGetTag(tags, TExecBase_ArgC, 0);
	argv = (TSTRPTR *) TGetTag(tags, TExecBase_ArgV, TNULL);
	
	if (argc >= 1 && argv)
	{
		TSTRPTR olddir = boot_alloc(boot, MAX_PATH_LEN);
		if (olddir)
		{
			if (getcwd(olddir, MAX_PATH_LEN))
			{
				progdir = boot_alloc(boot, MAX_PATH_LEN + 1);
				if (progdir)
				{
					TBOOL success = TFALSE;
					TINT l = 0;
					TSTRPTR s = argv[0];
					TINT8 c;
	
					while (*s) { s++; l++; }
					if (l > 0)
					{
						success = TTRUE;
						while ((c = *(--s))) { if (c == '/') break; l--; }
						if (l > 0)
						{
							TSTRPTR d, pathpart = boot_alloc(boot, l + 1);
							success = TFALSE;
							if (pathpart)
							{
								s = argv[0];
								d = pathpart;
								while (l--) *d++ = *s++;
								*d = 0;
								success = (chdir(pathpart) == 0);
								boot_freevec(boot, pathpart);
							}
						}
					}
					
					if (success)
						success = (getcwd(progdir, MAX_PATH_LEN) != TNULL);
					
					if (!(chdir(olddir) == 0)) success = TFALSE;
	
					if (success)
					{
						strcat(progdir, "/");
					}
					else
					{
						boot_freevec(boot, progdir);
						progdir = TNULL;
					}
				}
			}
			boot_freevec(boot, olddir);
		}
	}
	return progdir;
}

/*****************************************************************************/
/*
**	load a module. first try progdir/modname, then moddir
*/

#ifdef TSYS_DARWIN

#include <mach-o/dyld.h>

static TAPTR
getmodule(TSTRPTR modpath)
{
	return (TAPTR) NSAddImage(modpath, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
}

static TAPTR
getsymaddr(struct mach_header *mod, TSTRPTR name)
{
	TAPTR sym = TNULL;
	TSTRPTR name2 = malloc(strlen(name) + 2);
	if (name2)
	{
		strcpy(name2, "_");
		strcat(name2, name);
		if (NSIsSymbolNameDefinedInImage(mod, name2))
		{
			NSSymbol *nssym = NSLookupSymbolInImage(mod, name2,
				NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
				NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
			sym = NSAddressOfSymbol(nssym);
		}
		free(name2);
	}
	return sym;
}

static TVOID
closemodule()
{
	/* is there no equivalent to dlclose()? */
}

#else

#include <dlfcn.h>

static TAPTR
getmodule(TSTRPTR modpath)
{
	return dlopen(modpath, RTLD_NOW);
}

static TAPTR
getsymaddr(TAPTR mod, TSTRPTR name)
{
	return dlsym(mod, name);
}

static TVOID
closemodule(TAPTR mod)
{
	dlclose(mod);
}

#endif

/*****************************************************************************/

TAPTR 
boot_loadmodule(TAPTR boot, TSTRPTR progdir, TSTRPTR moddir, TSTRPTR modname,
	TTAGITEM *tags)
{
	TAPTR knmod = TNULL;
	TINT len1, len2, len3;
	TSTRPTR t;
	struct TInitModule *imod;
	struct ModHandle *handle;

	handle = boot_alloc(boot, sizeof(struct ModHandle));
	if (!handle) return TNULL;

	imod = lookupmodule(modname, tags);
	if (imod)
	{
		handle->entry = imod->tinm_InitFunc;
		handle->type = TYPE_LIB;
		return handle;
	}
	
	len1 = progdir? strlen(progdir) : 0;
	len2 = moddir? strlen(moddir) : 0;
	len3 = strlen(modname);
	
	/* + mod/ + .so + \0 */
	t = boot_alloc(boot, TMAX(len1, len2) + len3 + 4 + TEKHOST_EXTLEN + 1);
	if (t)
	{
		if (progdir) strcpy(t, progdir);
		strcpy(t + len1, "mod/");
		strcpy(t + len1 + 4, modname);
		strcpy(t + len1 + 4 + len3, TEKHOST_EXTSTR);

		tdbprintf1(2,"trying modopen %s\n", t);
		knmod = getmodule(t);
		if (!knmod)
		{
			tdbprintf1(5,"modopen %s failed\n", modname);

			if (moddir) strcpy(t, moddir);
			strcpy(t + len2, modname);
			strcpy(t + len2 + len3, TEKHOST_EXTSTR);

			tdbprintf1(2,"trying modopen %s\n", t);
			knmod = getmodule(t);
			if (!knmod) tdbprintf1(5,"modopen %s failed\n", modname);
		}
		boot_freevec(boot, t);
	}

	if (knmod)
	{
		handle->entry = knmod;
		handle->type = TYPE_DLL;
	}
	else
	{
		boot_freevec(boot, handle);
		handle = TNULL;
	}

	return handle;
}

/*****************************************************************************/
/*
**	close module
*/

TVOID
boot_closemodule(TAPTR boot, TAPTR knmod)
{
	struct ModHandle *handle = knmod;
	if (handle->type == TYPE_DLL) closemodule(handle->entry);
	boot_freevec(boot, handle);
}

/*****************************************************************************/
/*
**	get module entry
*/

TAPTR
boot_getentry(TAPTR boot, TAPTR knmod, TSTRPTR name)
{
	struct ModHandle *handle = knmod;
	if (handle->type == TYPE_DLL) return getsymaddr(handle->entry, name);
	return handle->entry;
}

/*****************************************************************************/
/*
**	call module
*/

TUINT
boot_callmod(TAPTR boot, TAPTR ModBase, TAPTR entry, TAPTR task, TAPTR mod,
	TUINT16 version, TTAGITEM *tags)
{
	return (*((TMODINITFUNC) entry))(task, mod, version, tags);
}

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
**	Revision 1.5  2003/10/28 08:47:07  tmueller
**	Darwin and Posix builds share the same HAL backend now.
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
