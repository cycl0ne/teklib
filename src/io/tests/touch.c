
/*
**	$Id: touch.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/io/tests/touch.c - Io module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/mod/time.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>
#include <tek/debug.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TIOBase;

#define ARG_TEMPLATE	"-f=FROM/A/M"
enum { ARG_FROM, ARG_HELP, ARG_NUM };

/*****************************************************************************/
/*
**	main program
*/

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TExecOpenModule(TExecBase, "util", 0, TNULL);
	if (TUtilBase)
	{
		TINT retval = 20;
		TIOBase = TExecOpenModule(TExecBase, "io", 0, TNULL);
		if (TIOBase)
		{
			TSTRPTR *argv = TUtilGetArgV(TUtilBase);
			TAPTR arghandle;
			TTAG args[ARG_NUM];

			args[ARG_FROM] = TNULL;
			args[ARG_HELP] = TFALSE;

			arghandle = TUtilParseArgV(TUtilBase, ARG_TEMPLATE, argv + 1, args);
			if (!arghandle || args[ARG_HELP])
			{
				printf("Usage:      touch %s\n", ARG_TEMPLATE);
				printf("-f=FROM/A/M files\n");
				printf("-h=HELP/S   this help\n");
			}
			else
			{
				TDATE dt;
				TSTRPTR *srcp = (TSTRPTR *) args[ARG_FROM];
				TSTRPTR fname;
				TBOOL success = TTRUE;

				TGetUniversalDate(&dt);
				retval = 0;

				while (success && (fname = *srcp++))
				{
					success = TSetFileDate(fname, &dt, TNULL);
					if (!success)
					{
						printf("*** failed to touch %s\n", fname);
						retval = 10;
					}
				}
			}
			TDestroy(arghandle);
		}
		TUtilSetRetVal(TUtilBase, retval);
		TExecCloseModule(TExecBase, TIOBase);
		TExecCloseModule(TExecBase, TUtilBase);
	}
}
