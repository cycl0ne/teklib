
/*************************************************************************
**
**	TEKlib filesystem namespace implementation
**	of a makedir command
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

#define ARG_TEMPLATE	"-n=NAME/A/M,-h=HELP/S"
enum { ARG_NAME, ARG_HELP, ARG_NUM };

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
		
		args[ARG_NAME] = TNULL;
		args[ARG_HELP] = TFALSE;

		arghandle = TUtilParseArgV(TUtilBase, ARG_TEMPLATE, argv + 1, args);
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
				TAPTR lock = TIOMakeDir(io, name, TNULL);
				if (lock)
				{
					TIOUnlockFile(io, lock);
				}
				else
				{
					success = TFALSE;
				}
			}

			if (!success && name)
			{
				char buf[200];
				TIOFault(io, TIOGetIOErr(io), buf, 200, TNULL);
				printf("creating %s failed. reason: %s\n", name, buf);
				TUtilSetRetVal(TUtilBase, 20);
			}
		}
		TDestroy(arghandle);
	}
	
	TExecCloseModule(TExecBase, io);
	TExecCloseModule(TExecBase, TUtilBase);
}

