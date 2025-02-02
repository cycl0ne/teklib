
/*
**	$Header: /cvs/teklib/teklib/boot/intent/host.c,v 1.2 2004/01/05 10:55:45 mlukat Exp $
*/

#include "init.h"

#include <elate/kn.h>
#include <elate/tool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct ModHandle
{
	TAPTR entry;
	TUINT type;
};

#define TYPE_DLL 0
#define TYPE_LIB 1

/*****************************************************************************/
/*
**	lookup internal module
*/

static struct TInitModule *lookupmodule(TSTRPTR modname, TTAGITEM *tags)
{
	struct TInitModule *imod = TGetTag(tags, TExecBase_ModInit, TNULL);
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
	return kn_mem_allocdata(size);
}

TVOID boot_free(TAPTR mem)
{
	kn_mem_free(mem);
}


/**************************************************************************
**
**	determine TEKlib global system directory
*/

TSTRPTR boot_getsysdir(TTAGITEM *tags)
{
	return absname(TGetTag(tags, TExecBase_SysDir, TEKHOST_SYSDIR));
}


/**************************************************************************
**
**	determine TEKlib global module directory
*/

TSTRPTR boot_getmoddir(TTAGITEM *tags)
{
	return absname(TGetTag(tags, TExecBase_ModDir, TEKHOST_MODDIR));
}


/**************************************************************************
**
**	determine the path to the application, which will
**	later resolve to "PROGDIR:" in teklib semantics.
*/

TSTRPTR boot_getprogdir(TTAGITEM *tags)
{
	TSTRPTR progdir = TNULL;
	TSTRPTR *argv, index;
	TINT argc;

	progdir = (TSTRPTR) TGetTag(tags, TExecBase_ProgDir, TEKHOST_PROGDIR);
	if (progdir)
	{
		return absname(progdir);
	}

	argc = (TINT) TGetTag(tags, TExecBase_ArgC, 0);
	argv = (TSTRPTR *) TGetTag(tags, TExecBase_ArgV, TNULL);

	if (argc >= 1 && argv)
	{
		progdir = absname(argv[0]);
		if (progdir)
		{
			index = strrchr(progdir, '/');
			if (index == progdir) index++;

			*index = 0;
		}
	}
	return progdir;
}


/**************************************************************************
**
**	load a module
*/

TAPTR boot_loadmodule(TSTRPTR progdir, TSTRPTR moddir, TSTRPTR modname, TTAGITEM *tags)
{
	TSTRPTR path;
	ELATE_KN_RET_TOOLOPEN tool;

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

	if ((moddir) && (modname))
	{
		if ((path = boot_alloc(strlen(moddir)+1+strlen(modname)+1)))
		{
			strcpy(path, moddir);
			strcat(path, "/");
			strcat(path, modname);

			tool = kn_tool_open(path+1, TNULL);
			boot_free(path);

			if (tool.entrypoint)
			{
				handle->entry = tool.entrypoint;
				handle->type = TYPE_DLL;
				return handle;
			}
		}
	}
	boot_free(handle);
	return TNULL;
}


/**************************************************************************
**
**	close module
*/

TVOID boot_closemodule(TAPTR halmod)
{
	struct ModHandle *handle = halmod;
	if (handle->type == TYPE_DLL) kn_tool_code_deref(handle->entry);
	boot_free(handle);
}


/**************************************************************************
**
**	get module entry
*/

TAPTR boot_getentry(TAPTR halmod, TSTRPTR entryname)
{
	struct ModHandle *handle = halmod;
	return handle->entry;
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
**	Revision 1.2  2004/01/05 10:55:45  mlukat
**	misc updates (module locations etc)
**	
**	Revision 1.1  2003/12/17 15:34:59  mlukat
**	initial version
**	
**	Revision 1.2  2003/03/08 21:49:49  tmueller
**	The boot libraries now respect the path defines TEKHOST_...DIR
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
**	
*/
