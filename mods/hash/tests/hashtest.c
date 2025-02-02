
/*
**	$Id: hashtest.c,v 1.3 2005/06/29 09:11:25 tmueller Exp $
**	apps/tests/hashtest.c - Hash module test
*/

#include <stdio.h>
#include <tek/inline/exec.h>
#include <tek/proto/hash.h>

/*****************************************************************************/

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TAPTR TExecBase = TGetExecBase(task);
	TAPTR hash = TOpenModule("hash", 0, TNULL);
	if (hash)
	{
		TSTRPTR s;
	
		THashPut(hash, (TTAG) "hello", (TTAG) "world");
		THashPut(hash, (TTAG) "yo", (TTAG) "africa");
		THashPut(hash, (TTAG) "tek", (TTAG) "rular");
	
		if (THashGet(hash, (TTAG) "yo", &s))
			printf("yo: %s\n", s);
		
		if (THashGet(hash, (TTAG) "tek", &s))
			printf("tek: %s\n", s);
		
		if (THashGet(hash, (TTAG) "hello", &s))
			printf("hello: %s\n", s);
	
		TCloseModule(hash);
	}
	else
	{
		printf("*** Hash module open failed\n");
	}
}

