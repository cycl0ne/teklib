
/*
**	$Id: cat.c,v 1.4 2005/09/18 13:06:34 fschulze Exp $ 
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>
#include <tek/inline/unistring.h>

TAPTR TExecBase;
TAPTR TIOBase;
TAPTR TUStrBase;
TAPTR TUtilBase;

#define ARG_TEMPLATE	"-f=FILE,-h=HELP/S"
enum { ARG_FILE, ARG_HELP, ARG_NUM };

static TVOID
dumpstring(TUString s)
{
	TSTRPTR p;
	TINT len = TLengthString(s);
	TSetCharString(s, len - 1, 0);
	p = TMapString(s, 0, len, TASIZE_8BIT);
	if (p) printf("%s\n", p);
	TCropString(s, 0, 0);
}

TVOID TTASKENTRY
TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TIOBase = TOpenModule("io", 0, TNULL);
	TUStrBase = TOpenModule("unistring", 0, TNULL);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (TIOBase && TUStrBase && TUtilBase)
	{
		TSTRPTR *argv = TGetArgV();
		TTAG args[ARG_NUM] = { (TTAG) "ps2io:host/README", TFALSE };
		TAPTR arghandle = TParseArgV(ARG_TEMPLATE, argv + 1, args);
		if (!arghandle || args[ARG_HELP])
		{
			printf("Dump file to stdout. Usage:\n%s %s\n", argv[0], ARG_TEMPLATE);
		}
		else
		{
			TAPTR f = TOpenFile((TSTRPTR) args[ARG_FILE], TFMODE_READONLY, TNULL);
			if (f)
			{
				TWCHAR c;
				TUString s = TAllocString(TNULL);
				while ((c = TFGetC(f)) != TEOF)
				{
					TSetCharString(s, -1, c);
					if (c == '\n') dumpstring(s);
				}
				dumpstring(s);
				TFreeString(s);
				TCloseFile(f);
			}
			else
			{
				printf("*** Could not open file %s.\n", (TSTRPTR) args[ARG_FILE]);
			}
		}
		TDestroy(arghandle);
	}
	else
	{
		printf("*** Could not open required modules.\n");
	}
	TCloseModule(TUtilBase);
	TCloseModule(TUStrBase);
	TCloseModule(TIOBase);
}
