
/*
**	$Id: rename.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/io/tests/rename.c - Io module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>
#include <tek/debug.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TIOBase;

#define ARG_TEMPLATE	"-f=FROM/A/M,-t=TO/A,-h=HELP/S"
enum { ARG_FROM, ARG_TO, ARG_HELP, ARG_NUM };

/*****************************************************************************/

static void printerr(void)
{
	TCHR errtext[80];
	TFault(TGetIOErr(), errtext, 80, TNULL);
	printf("%s\n", errtext);
}

/*****************************************************************************/

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
			TAddPart(part1, part2, fullpath, flen + 1);
	}

	return fullpath;
}

/*****************************************************************************/
/*
**	main program
*/

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TIOBase = TOpenModule("io", 0, TNULL);

	if (TUtilBase && TIOBase)
	{
		TSTRPTR *argv = TGetArgV();
		TAPTR arghandle;
		TTAG args[ARG_NUM];

		args[ARG_FROM] = TNULL;
		args[ARG_TO] = TNULL;
		args[ARG_HELP] = TFALSE;

		arghandle = TParseArgV(ARG_TEMPLATE, argv + 1, args);
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
			TAPTR destlock = TLockFile((TAPTR) args[ARG_TO], TFLOCK_READ, TNULL);
			TBOOL success = TTRUE;

			while (success && (srcname = *srcp++))
			{
				TSTRPTR fullname = destlock?
					addpath((TSTRPTR) args[ARG_TO], srcname) : TNULL;
				success = TRename(srcname, fullname?
					fullname : (TSTRPTR) args[ARG_TO]);
				TFree(fullname);
			}

			if (!success && srcname)
			{
				printf("renaming %s to %s failed\n",
					srcname, (TSTRPTR) args[ARG_TO]);
				printerr();
				TSetRetVal(20);
			}

			TUnlockFile(destlock);
		}
		TDestroy(arghandle);
	}

	TCloseModule(TIOBase);
	TCloseModule(TUtilBase);
}
