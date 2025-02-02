
/*
**	$Id: makedir.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/io/tests/makedir.c - Io module unit test
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

#define ARG_TEMPLATE	"-n=NAME/A/M,-h=HELP/S"
enum { ARG_NAME, ARG_HELP, ARG_NUM };

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

		args[ARG_NAME] = TNULL;
		args[ARG_HELP] = TFALSE;

		arghandle = TParseArgV(ARG_TEMPLATE, argv + 1, args);
		if (!arghandle || args[ARG_HELP])
		{
			printf("Usage:       makedir %s\n", ARG_TEMPLATE);
			printf("-n=NAME/A/M  directories to create\n");
			printf("-h=HELP/S    this help\n");
		}
		else
		{
			TSTRPTR *srcp = (TSTRPTR *) args[ARG_NAME];
			TSTRPTR name;
			TBOOL success = TTRUE;

			while (success && (name = *srcp++))
			{
				TAPTR lock = TMakeDir(name, TNULL);
				if (lock)
					TUnlockFile(lock);
				else
					success = TFALSE;
			}

			if (!success && name)
			{
				TCHR buf[200];
				TFault(TGetIOErr(), buf, 200, TNULL);
				printf("creating %s failed. reason: %s\n", name, buf);
				TSetRetVal(20);
			}
		}
		TDestroy(arghandle);
	}

	TCloseModule(TIOBase);
	TCloseModule(TUtilBase);
}
