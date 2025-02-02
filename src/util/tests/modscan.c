
/*
**	$Id: modscan.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/util/tests/modscan.c - Util module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

void TEKMain(struct TTask *task)
{
	TAPTR TExecBase = TGetExecBase(task);
	TAPTR TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		TSTRPTR *argv = TGetArgV();
		TSTRPTR moddir = argv[1];
		TTAGITEM tags[2];
		struct THandle *handle;

		tags[0].tti_Tag = TExec_ModulePrefix;
		tags[0].tti_Value = (TTAG) (moddir ? moddir : "");
		tags[1].tti_Tag = TTAG_DONE;

		handle = TScanModules(tags);
		if (handle)
		{
			TTAGITEM *attrs;
			while ((attrs = TGetNextEntry(handle)))
				puts((TSTRPTR) TGetTag(attrs, TExec_ModuleName, TNULL));
			TDestroy(handle);
		}
		else
			printf("*** couldn't scan modules\n");

		TCloseModule(TUtilBase);
	}
	else
		printf("*** couldn't open utility module\n");
}
