
/*
**	$Id: host.c,v 1.3 2004/09/09 19:34:50 tmueller Exp $
**	boot/darwin/host.c - Darwin-specific implementation
**	of startup functions
*/

#include "init.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mach-o/dyld.h>
/*#include <dlfcn.h>*/
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/param.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFBase.h>

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

/**************************************************************************
**
**	lookup internal module
*/

static struct TInitModule *lookupmodule(TSTRPTR modname, TTAGITEM *tags)
{
	struct TInitModule *imod = (TTAG) TGetTag(tags, TExecBase_ModInit, TNULL);
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


/**************************************************************************
**
**	host alloc/free
*/

TAPTR boot_alloc(TUINT size)
{
	return malloc(size);
}

TVOID boot_free(TAPTR mem)
{
	if (mem) free(mem);
}


/**************************************************************************
**
**	determine TEKlib global system directory
*/

TSTRPTR boot_getsysdir(TTAGITEM *tags)
{
	TSTRPTR sysdir;
	TSTRPTR s;
	TINT l;
	
	s = (TSTRPTR) TGetTag(tags, TExecBase_SysDir, TEKHOST_SYSDIR);
	l = strlen(s);

	sysdir = boot_alloc(l + 1);
	if (sysdir)
	{
		strcpy(sysdir, s);
	}
	return sysdir;
}


/**************************************************************************
**
**	determine TEKlib global module directory
**
**	searching in default framework locations for tek.framework
**	
**	~/Library/Frameworks
**	/Library/Frameworks
**	/Network/Library/Frameworks
**	/System/Library/Frameworks
*/

TSTRPTR boot_getmoddir(TTAGITEM *tags)
{
	TSTRPTR sysdir = TNULL;

	if (access ("/System/Library/Frameworks/tek.framework/mod/", R_OK) == 0)
	{
		sysdir = boot_alloc (128);
		if (sysdir)
			strcpy (sysdir, "/System/Library/Frameworks/tek.framework/mod/");
	}
	
	if (access ("/Network/Library/Frameworks/tek.framework/mod/", R_OK) == 0)
	{
		sysdir = boot_alloc (128);
		if (sysdir)
			strcpy (sysdir, "/Network/Library/Frameworks/tek.framework/mod/");
	}
	
	if (access ("/Library/Frameworks/tek.framework/mod/", R_OK) == 0)
	{
		sysdir = boot_alloc (128);
		if (sysdir)
			strcpy (sysdir, "/Library/Frameworks/tek.framework/mod/");
	}

	if (access ("~/Library/Frameworks/tek.framework/mod/", R_OK) == 0)
	{
		sysdir = boot_alloc (128);
		if (sysdir)
			strcpy (sysdir, "~/Library/Frameworks/tek.framework/mod/");
	}
	
	return sysdir;
}


/**************************************************************************
**
**	determine the path to the application, which will
**	later resolve to "PROGDIR:" in teklib semantics.
*/

TSTRPTR boot_getprogdir(TTAGITEM *tags)
{
	TSTRPTR progdir = TNULL;
	TSTRPTR *argv;
	TINT argc;

	progdir = (TSTRPTR) TGetTag(tags, TExecBase_ProgDir, TEKHOST_PROGDIR);
	if (progdir)
	{
		TINT l = strlen(progdir);
		TSTRPTR p = boot_alloc(l + 1);
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
		TSTRPTR olddir = boot_alloc(MAX_PATH_LEN);
		if (olddir)
		{
			if (getcwd(olddir, MAX_PATH_LEN))
			{
				progdir = boot_alloc(MAX_PATH_LEN + 1);
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
							TSTRPTR d, pathpart = boot_alloc(l + 1);
							success = TFALSE;
							if (pathpart)
							{
								s = argv[0];
								d = pathpart;
								while (l--) *d++ = *s++;
								*d = 0;
								success = (chdir(pathpart) == 0);
								boot_free(pathpart);
							}
						}
					}
					
					if (success) success = (getcwd(progdir, MAX_PATH_LEN) != TNULL);
					
					if (!(chdir(olddir) == 0)) success = TFALSE;
	
					if (success)
					{
						strcat(progdir, "/");
					}
					else
					{
						boot_free(progdir);
						progdir = TNULL;
					}
				}
			}
			boot_free(olddir);
		}
	}
	return progdir;
}


/**************************************************************************
**
**	load a module. first try progdir/modname, then moddir
*/

TAPTR boot_loadmodule(TSTRPTR progdir, TSTRPTR moddir, TSTRPTR modname, TTAGITEM *tags)
{
	TAPTR knmod = TNULL;
	TINT len1, len2, len3;
	TSTRPTR t;
	struct TInitModule *imod;
	struct ModHandle *handle;

	handle = boot_alloc(sizeof(struct ModHandle));
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
	
	t = boot_alloc(TMAX(len1, len2) + len3 + 4 + 6 + 1);		/* + mod/ + .dylib + \0 */
	if (t)
	{
		if (progdir) strcpy(t, progdir);
		strcpy(t + len1, "mod/");
		strcpy(t + len1 + 4, modname);
		strcpy(t + len1 + 4 + len3, ".dylib");

		tdbprintf1(2,"trying NSAddImage %s\n", t);
		knmod = (TAPTR) NSAddImage(t, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
		
		if (!knmod)
		{
			tdbprintf1(5,"NSAddImage %s failed.\n", modname);

			if (moddir) strcpy(t, moddir);
			strcpy(t + len2, modname);
			strcpy(t + len2 + len3, ".dylib");

			tdbprintf1(2,"trying NSAddImage %s\n", t);
			knmod = (TAPTR) NSAddImage(t, NSADDIMAGE_OPTION_RETURN_ON_ERROR);

			if (!knmod) tdbprintf1(5,"NSAddImage %s failed.\n", modname);
		}
		boot_free(t);
	}

	if (knmod)
	{
		handle->entry = knmod;
		handle->type = TYPE_DLL;
	}
	else
	{
		boot_free(handle);
		handle = TNULL;
	}

	return handle;
}


/**************************************************************************
**
**	close module
*/

TVOID boot_closemodule(TAPTR knmod)
{

//	it seems that there is now way to unload dynamic libaries
//	unless it is a bundle
//	dlclose(knmod);

	boot_free(knmod);
}


/**************************************************************************
**
**	get module entry
*/

TAPTR boot_getentry(TAPTR knmod, TSTRPTR name)
{
	struct ModHandle *handle = knmod;
	static char	symname[1024];
	NSSymbol *nssym = 0;
	if (handle->type == TYPE_LIB) return handle->entry;
	
	snprintf (symname,1024,"_%s",name);
	
	if (NSIsSymbolNameDefinedInImage((struct mach_header *) handle->entry, symname))
	{
		nssym = NSLookupSymbolInImage((struct mach_header *) handle->entry,
						  symname, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND
								| NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
	}
	else
		tdbprintf1(5,"could not find symbol: %s\n", symname);

	return NSAddressOfSymbol(nssym);
}

/**************************************************************************
**
**	call module
*/

TUINT boot_callmod(TAPTR ModBase, TAPTR entry, TAPTR task, TAPTR mod, TUINT16 version, TTAGITEM *tags)
{
	return (*((TMODINITFUNC) entry))(task, mod, version, tags);
}

/*
**	Revision History
**	$Log: host.c,v $
**	Revision 1.3  2004/09/09 19:34:50  tmueller
**	fixed compiler glitches resulting from TTAG conversion
**	
**	Revision 1.2  2004/01/10 13:04:32  mtaubert
**	This is the initial version of the Mac OS X display modules.
**	
**	The current implementation contains only OpenGL-based display modules.
**	
**	Also, there is a new build target in the top makefile called darwin_install,
**	which will build a Mac OS X framework and installs it with the help of
**	sudo into /Library/Frameworks/.
**	
**	The search paths for the darwin platform has changed too.
**	There are a few locations from where the modules tried to be loaded from,
**	such as /Library/Frameworks/tek.framework/mod/,
**	/System/Library/Frameworks/... or ~/Library/Frameworks. At last the
**	application directory should contain the modules.
**	
**	Revision 1.1.1.1  2003/12/11 07:18:25  tmueller
**	Krypton import
**	
**	Revision 1.5  2003/10/12 19:19:09  tmueller
**	TInitModule fields have been prefixed with tinm_
**	
**	Revision 1.4  2003/09/17 16:48:36  tmueller
**	Modules linked statically to the framework are fully supported again
**	
**	Revision 1.2  2003/03/08 21:49:49  tmueller
**	The boot libraries now respect the path defines TEKHOST_...DIR
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.3  2003/02/13 18:27:03  cybin
**	darwin port now compiles with -DTSYS_DARWIN and uses correct
**	host and platform values.
**	
**	added files for io module. should be the same like posix32.
**	
**	Revision 1.2  2003/01/31 15:58:40  cybin
**	
**	darwin related changes:
**	
**	removed dependency to libdl (dlcompat).
**	
**	it now uses the native nsaddimage() and nslookupsymbolinimage() to
**	resolve symbols. but there are restriction with the current
**	implementation:
**	
**	 - it works _only_ with libraries linked with -dynamiclib not bundles/frameworks
**	 - the symbolname is limited to 1023 letter to prevent an allocation
**	
**	Revision 1.1  2003/01/28 19:00:29  cybin
**	added darwin specific hal module and boot/
**	
**	shared libs for OSX use .dylib as suffix
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
**	
*/
