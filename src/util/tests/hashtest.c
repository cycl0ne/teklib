
/*
**	teklib/src/util/hashtest.c
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

/*****************************************************************************/

void TEKMain(struct TTask *task)
{
	TAPTR TExecBase = TGetExecBase(task);
	TAPTR TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		TAPTR hash = TCreateHash(TNULL);
		TSTRPTR s;

		TPutHash(hash, (TTAG) "hello", (TTAG) "world");
		TPutHash(hash, (TTAG) "yo", (TTAG) "africa");
		TPutHash(hash, (TTAG) "tek", (TTAG) "rular");

		if (TGetHash(hash, (TTAG) "yo", (TAPTR) &s))
			printf("yo: %s\n", s);

		if (TGetHash(hash, (TTAG) "tek", (TAPTR) &s))
			printf("tek: %s\n", s);

		if (TGetHash(hash, (TTAG) "hello", (TAPTR) &s))
			printf("hello: %s\n", s);

		TDestroy(hash);

		TCloseModule(TUtilBase);
	}
	else
		printf("*** Hash module open failed\n");
}
