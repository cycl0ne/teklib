
/*
**	$Id: host.c,v 1.3 2005/04/01 18:36:22 tmueller Exp $
**	boot/win32/host.c - Win32 startup implementation
*/

#include <boot/init.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winreg.h>
#include <process.h>

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
**	Get common directory
*/

static TSTRPTR
getcommondir(TSTRPTR extra)
{
	HKEY key;
	TSTRPTR cfd = TNULL;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"Software\\Microsoft\\Windows\\CurrentVersion\\",
			0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		LONG len = 0;
		if (RegQueryValueEx(key, "CommonFilesDir", NULL, NULL, NULL, &len) ==
			ERROR_SUCCESS)
		{
			if (len > 0)
			{
				cfd = malloc(len + (extra? strlen(extra) : 0) + 1);
				if (cfd)
				{
					if (RegQueryValueEx(key, "CommonFilesDir", NULL, NULL,
						cfd, &len) == ERROR_SUCCESS)
					{
						if (cfd[len - 2] != '\\') strcat(cfd, "\\");
						if (extra) strcat(cfd, extra);
					}
					else
					{
						free(cfd);
						cfd = TNULL;
					}
				}
			}
		}
		RegCloseKey(key);
	}
	return cfd;
}

/*****************************************************************************/

TAPTR
boot_init(TTAGITEM *tags)
{
	return (TAPTR) 1;
}

TVOID
boot_exit(TAPTR handle)
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

	s = (TSTRPTR) TGetTag(tags, TExecBase_SysDir, TNULL);
	if (s)
	{
		sysdir = boot_alloc(boot, strlen(s) + 1);
		if (sysdir)
		{
			strcpy(sysdir, s);
		}
	}
	else
	{
		sysdir = getcommondir("tek\\");
	}

	tdbprintf1(2,"got sysdir: %s\n", sysdir);
	return sysdir;
}

/*****************************************************************************/
/*
**	determine TEKlib global module directory
*/

TSTRPTR
boot_getmoddir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR moddir;
	TSTRPTR s;

	s = (TSTRPTR) TGetTag(tags, TExecBase_ModDir, TNULL);
	if (s)
	{
		moddir = boot_alloc(boot, strlen(s) + 1);
		if (moddir)
		{
			strcpy(moddir, s);
		}
	}
	else
	{
		moddir = getcommondir("tek\\mod\\");
	}

	tdbprintf1(2,"got moddir: %s\n", moddir);
	return moddir;
}

/*****************************************************************************/
/*
**	determine the path to the application, which will
**	later resolve to "PROGDIR:"
*/

TSTRPTR
boot_getprogdir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR progdir;
	TSTRPTR s;
	s = (TSTRPTR) TGetTag(tags, TExecBase_ProgDir, TNULL);
	if (s)
	{
		progdir = boot_alloc(boot, strlen(s) + 1);
		if (progdir)
		{
			strcpy(progdir, s);
		}
	}
	else
	{
		progdir = malloc(MAX_PATH + 1);
		if (progdir)
		{
			if (GetModuleFileName(NULL, progdir, MAX_PATH + 1))
			{
				char *p = progdir;
				while (*p++);
				while (*(--p) != '\\');
				p[1] = 0;
			}
		}
	}

	tdbprintf1(2,"got progdir: %s\n", progdir);
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
	
	/* + mod\ + .dll + \0 */
	t = boot_alloc(boot, TMAX(len1, len2) + len3 + 4 + 4 + 1);
	if (t)
	{
		if (progdir) strcpy(t, progdir);
		strcpy(t + len1, "mod\\");
		strcpy(t + len1 + 4, modname);
		strcpy(t + len1 + 4 + len3, ".dll");

		tdbprintf1(2,"trying LoadLibrary %s\n", t);
		
		knmod = LoadLibraryEx(t, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (!knmod)
		{
			tdbprintf1(2,"LoadLibrary %s failed\n", modname);
	
			if (moddir) strcpy(t, moddir);
			strcpy(t + len2, modname);
			strcpy(t + len2 + len3, ".dll");

			tdbprintf1(2,"trying LoadLibrary %s\n", t);
			knmod = LoadLibraryEx(t, 0, LOAD_WITH_ALTERED_SEARCH_PATH);

			if (!knmod) tdbprintf1(5,"LoadLibrary %s failed\n", modname);
		}
		boot_free(boot, t, 0);	/* dummy size */
	}

	if (knmod)
	{
		handle->entry = knmod;
		handle->type = TYPE_DLL;
	}
	else
	{
		boot_free(boot, handle, 0);	/* dummy size */
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
	if (handle->type == TYPE_DLL) FreeLibrary(handle->entry);
	boot_free(boot, handle, 0);	/* dummy size */
}

/*****************************************************************************/
/*
**	get module entry
*/

TAPTR
boot_getentry(TAPTR boot, TAPTR knmod, TSTRPTR name)
{
	TAPTR initfunc;
	struct ModHandle *handle = knmod;
	if (handle->type == TYPE_LIB) return handle->entry;
	tdbprintf1(2,"symbol lookup: %s\n", name);
	initfunc = GetProcAddress(handle->entry, name);
	if (!initfunc)
	{
		TSTRPTR t = malloc(strlen(name) + 2);
		if (t)
		{
			strcpy(t, "_");
			strcat(t, name);
			tdbprintf1(2,"symbol lookup: %s\n", t);
			initfunc = GetProcAddress(handle->entry, t);
			free(t);
		}
	}
	return initfunc;
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
**	Revision 1.1.1.1  2003/12/11 07:18:24  tmueller
**	Krypton import
**	
**	Revision 1.5  2003/10/28 21:23:25  tmueller
**	Standard headers added, cleanup (formatting and style)
**	
**	Revision 1.4  2003/10/12 19:19:09  tmueller
**	TInitModule fields have been prefixed with tinm_
**	
**	Revision 1.3  2003/10/12 16:38:26  tschwinger
**	
**	Support for mingw32 (windows gcc) compiler added.
**	
**	Revision 1.2  2003/09/17 16:48:36  tmueller
**	Modules linked statically to the framework are fully supported again
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.2  2003/01/09 03:56:56  dante
**	*** empty log message ***
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/
