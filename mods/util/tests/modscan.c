
/*
**	modscan.c
**	test for scanning modules.
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TAPTR TExecBase = TGetExecBase(task);
	TAPTR TUtilBase = TExecOpenModule(TExecBase, "util", 0, TNULL);
	if (TUtilBase)
	{
		TLIST modlist;
		struct TModuleEntry *entry;
		TSTRPTR *argv = TUtilGetArgV(TUtilBase);
		TSTRPTR moddir = argv[1];

		if (moddir)
		{
			printf("modules available with prefix \"%s\"\n", moddir);
		}
		else
		{
			printf("modules available:\n");
		}
		
		TInitList(&modlist);

		if (TUtilGetModules(TUtilBase, moddir, &modlist, TNULL))
		{
			TNODE *nextnode, *node = modlist.tlh_Head;

			printf("------------------------------\n");
	
			while ((nextnode = node->tln_Succ))
			{
				entry = (struct TModuleEntry *) node;
				printf("%s\n",entry->tme_Handle.tmo_Name);
	
				node = nextnode;
			}
	
			printf("------------------------------\n");
			
			TDestroyList(&modlist);
		}
		else printf("*** couldn't scan modules\n");

		TExecCloseModule(TExecBase, TUtilBase);
	}
	else printf("*** couldn't open utility module\n");
}
