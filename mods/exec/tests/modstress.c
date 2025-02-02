
/*
**	torture test:
**	opening/instiantiating/using modules
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/proto/hash.h>

TTASKENTRY TVOID 
subfunc(TAPTR task)
{
	TAPTR TExecBase = TGetExecBase(task);
	TINT i;

	for (i = 0; i < 100; ++i)
	{
		TAPTR hash = TOpenModule("hash", 0, TNULL);
		if (hash)
		{
			THashPut(hash, (TTAG) "hello", (TTAG) "world");
			THashPut(hash, (TTAG) "yo", (TTAG) "africa");
			THashPut(hash, (TTAG) "tek", (TTAG) "rular");
			
			THashGet(hash, (TTAG) "yo", TNULL);
			THashGet(hash, (TTAG) "tek", TNULL);
			THashGet(hash, (TTAG) "hallo", TNULL);
		
			TCloseModule(hash);
		}
	}
}

#define NUMTASKS 100

TTASKENTRY TVOID 
TEKMain(TAPTR task)
{
	TAPTR TExecBase = TGetExecBase(task);
	TAPTR tasks[NUMTASKS];
	TINT i;

	for (i = 0; i < NUMTASKS; ++i)
	{
		tasks[i] = TCreateTask(subfunc, TNULL, TNULL);
	}
	
	for (i = 0; i < NUMTASKS; ++i)
	{
		TDestroy(tasks[i]);
	}
	
	printf("all done\n");
}
