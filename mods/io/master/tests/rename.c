
/*
**	$Id: rename.c,v 1.3 2005/09/08 00:01:53 tmueller Exp $
**	apps/tests/rename.c - TEKlib filesystem namespace implementation
**	of a mv/rename command
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/io.h>
#include <tek/debug.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR io;

#define ARG_TEMPLATE	"-f=FROM/A/M,-t=TO/A,-h=HELP/S"
enum { ARG_FROM, ARG_TO, ARG_HELP, ARG_NUM };

/*****************************************************************************/

static TVOID printerr(void)
{
	char errtext[80];
	TIOFault(io, TIOGetIOErr(io), errtext, 80, TNULL);
	printf("%s\n", errtext);
}

/*****************************************************************************/

static TSTRPTR addpath(TSTRPTR part1, TSTRPTR part2)
{
	TSTRPTR fullpath = TNULL;
	TINT flen;

	if (!part1) part1 = "";
	if (!part2) part2 = "";

	flen = TIOAddPart(io, part1, part2, TNULL, 0);
	if (flen >= 0)
	{
		fullpath = TExecAlloc(TExecBase, TNULL, flen + 1);
		if (fullpath)
		{
			TIOAddPart(io, part1, part2, fullpath, flen + 1);
		}
	}

	return fullpath;
}

/*****************************************************************************/
/* 
**	main program
*/

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TExecOpenModule(TExecBase, "util", 0, TNULL);
	io = TExecOpenModule(TExecBase, "io", 0, TNULL);
	
	if (TUtilBase && io)
	{
		TSTRPTR *argv = TUtilGetArgV(TUtilBase);
		TAPTR arghandle;
		TTAG args[ARG_NUM];
		
		args[ARG_FROM] = TNULL;
		args[ARG_TO] = TNULL;
		args[ARG_HELP] = TFALSE;

		arghandle = TUtilParseArgV(TUtilBase, ARG_TEMPLATE, argv + 1, args);
		if (!arghandle || args[ARG_HELP])
		{
			printf("Usage:      rename %s\n", ARG_TEMPLATE);
			printf("-f=FROM/A/M one or more source files or directories\n");
			printf("-t=TO/A     destination filename or directory\n");
			printf("-h=HELP/S   this help\n");
		}
		else
		{
			TSTRPTR *srcp = (TSTRPTR *) args[ARG_FROM];
			TSTRPTR srcname;
			TAPTR destlock = TIOLockFile(io, (TAPTR) args[ARG_TO], TFLOCK_READ, TNULL);
			TBOOL success = TTRUE;

			while (success && (srcname = *srcp++))
			{
				TSTRPTR fullname = destlock? 
					addpath((TSTRPTR) args[ARG_TO], srcname) : TNULL;
				success = TIORename(io, srcname, fullname? 
					fullname : (TSTRPTR) args[ARG_TO]);
				TExecFree(TExecBase, fullname);
			}
			
			if (!success && srcname)
			{
				printf("renaming %s to %s failed\n",
					srcname, (TSTRPTR) args[ARG_TO]);
				printerr();
				TUtilSetRetVal(TUtilBase, 20);
			}
			
			TIOUnlockFile(io, destlock);
		}
		TDestroy(arghandle);
	}
	
	TExecCloseModule(TExecBase, io);
	TExecCloseModule(TExecBase, TUtilBase);
}

