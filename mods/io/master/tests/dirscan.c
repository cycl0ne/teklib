
/*
**	$Id: dirscan.c,v 1.4 2005/09/07 01:10:04 tmueller Exp $
**	apps/tests/dirscan.c - simple dir/ls command in TEKlib's namespace
*/

#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TIOBase;

#define STATUS_OK		0
#define STATUS_ERROR	10
#define STATUS_FAIL		20

static TBOOL scan(TSTRPTR path, TSTRPTR newpath, TINT indent, TTAG *args);

#define ARG_TEMPLATE \
"-f=FROM/M,-a=ALL/S,-dd=DIRS/S,-df=FILES/S,-t=TEK/S,-h=HELP/S"

enum { ARG_DIR, ARG_ALL, ARG_DIRS, ARG_FILES, ARG_TEK, ARG_HELP, ARG_NUM };

/*****************************************************************************/

TTASKENTRY TVOID 
TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TIOBase = TOpenModule("io", 0, TNULL);

	if (TUtilBase && TIOBase)
	{
		TUINT status = STATUS_FAIL;
		TSTRPTR *argv = TGetArgV();
		TAPTR arghandle;
		TTAG args[ARG_NUM];

		status = STATUS_ERROR;		
		
		args[ARG_DIR] = TNULL;
		args[ARG_ALL] = TFALSE;
		args[ARG_DIRS] = TFALSE;
		args[ARG_FILES] = TFALSE;
		args[ARG_TEK] = TFALSE;
		args[ARG_HELP] = TFALSE;

		arghandle = TParseArgV(ARG_TEMPLATE, argv + 1, args);
		if (!arghandle || args[ARG_HELP])
		{
			printf("Usage:      dirscan %s\n", ARG_TEMPLATE);
			printf("-f=FROM/M   one or more directories to list\n");
			printf("-a=ALL/S    scan recursively\n");
			printf("-dd=DIRS/S  show only directories\n");
			printf("-df=FILES/S show only files\n");
			printf("-t=TEK/S    use TEKlib path naming conventions\n");
			printf("-h=HELP/S   this help\n");
		}
		else
		{
			TSTRPTR *path = (TSTRPTR *) args[ARG_DIR];
			TSTRPTR pathname;
			TINT num = 0;

			if (!args[ARG_FILES] && !args[ARG_DIRS])
			{
				args[ARG_FILES] = (TTAG) TTRUE;
				args[ARG_DIRS] = (TTAG) TTRUE;
			}

			status = STATUS_OK;
			while ((pathname = *path++))
			{
				TSTRPTR tekname = TNULL;
				TBOOL success = TTRUE;

				num++;

				if (!args[ARG_TEK])
				{
					TINT tlen;
					success = TFALSE;					

					tlen = TMakeName(pathname, TNULL, 0, TPPF_HOST2TEK, TNULL);
					if (tlen > 0)
					{
						tekname = TAlloc(TNULL, tlen + 1);
						if (tekname)
						{
							TMakeName(pathname, tekname, tlen + 1, TPPF_HOST2TEK, TNULL);
							pathname = tekname;
							success = TTRUE;
						}
					}
				}

				printf("%s (dir)\n", pathname);

				if (success)
				{
					if (!scan(pathname, TNULL, 0, args))
					{
						status = STATUS_ERROR;
						break;
					}
				}
				else
				{
					printf("cannot resolve name\n");
				}

				TFree(tekname);
			}

			if (num == 0)
			{
				if (!scan(TNULL, TNULL, 0, args))
				{
					status = STATUS_ERROR;
				}
			}
		}
		
		TDestroy(arghandle);

		if (status != STATUS_OK)
		{
			TSetRetVal(status);
		}
	}
	
	TCloseModule(TIOBase);
	TCloseModule(TUtilBase);
}

static TVOID printindent(TINT indent)
{
	while (indent--) printf("  ");
}

static TVOID printerr(void)
{
	static char errtext[80];
	TFault(TGetIOErr(), errtext, 80, TNULL);
	printf("%s\n", errtext);
}

static TSTRPTR addpath(TSTRPTR part1, TSTRPTR part2)
{
	TSTRPTR fullpath = TNULL;
	TINT flen;

	if (!part1) part1 = "";
	if (!part2) part2 = "";

	flen = TAddPart(part1, part2, TNULL, 0);
	if (flen >= 0)
	{
		fullpath = TAlloc(TNULL, flen + 1);
		if (fullpath)
		{
			TAddPart(part1, part2, fullpath, flen + 1);
		}
	}

	return fullpath;
}

static TBOOL scan(TSTRPTR path, TSTRPTR newpath, TINT indent, TTAG *args)
{
	TSTRPTR fullpath;
	TAPTR lock;
	TBOOL success = TFALSE;

	fullpath = addpath(path, newpath);
	if (!fullpath)
	{
		printerr();
		return TFALSE;
	}

	lock = TLockFile(fullpath, TFLOCK_READ, TNULL);
	if (lock)
	{
		TUINT type;
		TSTRPTR name;
		TUINT size;
		TUINT size_hi = 0;
		TTAGITEM extags[5];

		extags[0].tti_Tag = TFATTR_Type;
		extags[0].tti_Value = (TTAG) &type;
		extags[1].tti_Tag = TFATTR_Name;
		extags[1].tti_Value = (TTAG) &name;
		extags[2].tti_Tag = TFATTR_Size;
		extags[2].tti_Value = (TTAG) &size;
		extags[3].tti_Tag = TFATTR_SizeHigh;
		extags[3].tti_Value = (TTAG) &size_hi;
		extags[4].tti_Tag = TTAG_DONE;

		success = (TExamine(lock, extags) == 4);
		if (success)
		{
			if (type & TFTYPE_Directory)
			{
				if (newpath && args[ARG_DIRS])
				{
					printindent(indent);
					printf("%s (dir)\n", newpath);
				}

				while (success && TExNext(lock, extags) == 4)
				{
					if (type == TFTYPE_Directory)
					{
						if (args[ARG_ALL])
						{
							success = scan(fullpath, name, indent + 1, args);
						}
						else if (args[ARG_DIRS])
						{
							printindent(indent + 1);
							printf("%s (dir)\n", name);
						}
					}
					else if (args[ARG_FILES])
					{
						printindent(indent + 1);
						puts(name);
					}
				}
			}
			else
			{
				printerr();
			}
		}

		TUnlockFile(lock);
	}
	else
	{
		printerr();
	}

	TFree(fullpath);
	return success;
}
