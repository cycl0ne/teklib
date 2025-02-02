
/* 
**	$Id: main.c,v 1.5 2005/09/13 02:41:00 tmueller Exp $
**	boot/amiga/main.c - Standalone Amiga/MorphOS application entrypoint 
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "init.h"
#include <stdlib.h>
#include <string.h>

#include <workbench/startup.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>

extern struct WBStartup *_WBenchMsg;

/*****************************************************************************/

TTASKENTRY int
main(int argc, char **argv)	
{
	TAPTR apptask;
	TTAGITEM tags[4];
	TINT retval = EXIT_FAILURE;
	TSTRPTR freeptr = TNULL;
	TINT t = 0;
	
	struct Process *self = (struct Process *) FindTask(NULL);
	struct CommandLineInterface *cli = BADDR(self->pr_CLI);

	if (cli)
	{
		TSTRPTR temp = BADDR(cli->cli_CommandName);
		if (temp)
		{
			TINT len = *temp++;
			TSTRPTR progname = AllocVec(len + 1, MEMF_ANY);		//boot_alloc(len + 1);
			if (progname == TNULL) return EXIT_FAILURE;
			freeptr = progname;

			progname[len] = 0;
			while (len--) progname[len] = temp[len];

			tags[t].tti_Tag = TExecBase_ProgName;
			tags[t++].tti_Value = (TTAG) progname;
			tags[t].tti_Tag = TExecBase_Arguments;
			tags[t++].tti_Value = (TTAG) self->pr_Arguments;
		}
	}
	else if (_WBenchMsg)
	{
		struct Library *IconBase = OpenLibrary("icon.library", 0);
		if (IconBase)
		{
			struct WBArg *wba = _WBenchMsg->sm_ArgList;
			TSTRPTR *tt = TNULL;
			BPTR oldcd = CurrentDir(wba->wa_Lock);
			struct DiskObject *dob = GetDiskObject(wba->wa_Name);
			TINT len = 2;
			char buf[512];
			TSTRPTR args;
			TINT i;

			if (dob)
			{
				tt = dob->do_ToolTypes;
				if (tt)
				{
					TSTRPTR *t = tt;
					while (*t) len += strlen(*t++) + 1;
				}
			}
			
			for (i = 1; i < _WBenchMsg->sm_NumArgs; ++i)
			{
				if (NameFromLock(wba[i].wa_Lock, buf, sizeof(buf) - 1))
				{
					len += strlen(buf) + strlen(wba[i].wa_Name) + 2;
				}
			}

			args = AllocVec(len, MEMF_ANY);		//boot_alloc(len);
			if (args)
			{
				TSTRPTR p = args;

				for (i = 1; i < _WBenchMsg->sm_NumArgs; ++i)
				{
					if (NameFromLock(wba[i].wa_Lock, buf, sizeof(buf) - 1))
					{
						len = strlen(buf);
						strncpy(p, buf, len);
						p += len;
						if (buf[len - 1] != ':' && buf[len - 1] != '/')
						{
							*p++ = '/';
						}
						strcpy(p, wba[i].wa_Name);
						p += strlen(wba[i].wa_Name);
						*p++ = ' ';
					}
				}

				if (tt)
				{
					while (*tt)
					{
						strcpy(p, *tt);
						p += strlen(*tt);
						*p++ = ' ';
						tt++;
					}
				}
				
				*p = 0;
				if (strlen(args)) p[-1] = 0;
			}

			if (dob) FreeDiskObject(dob);
			CurrentDir(oldcd);
			CloseLibrary(IconBase);
			
			if (args)
			{
				tags[t].tti_Tag = TExecBase_ProgName;
				tags[t++].tti_Value = (TTAG) wba->wa_Name;
				tags[t].tti_Tag = TExecBase_Arguments;
				tags[t++].tti_Value = (TTAG) args;
				freeptr = args;
			}
			else return EXIT_FAILURE;
		}
	}
	else return EXIT_FAILURE;

	tags[t].tti_Tag = TExecBase_RetValP;
	tags[t++].tti_Value = (TTAG) &retval; 
	tags[t].tti_Tag = TTAG_DONE;

	apptask = TEKCreate(tags);
	if (apptask)
	{
		retval = EXIT_SUCCESS;
		TEKMain(apptask);
		TDestroy(apptask);
	}
	
	if (freeptr) FreeVec(freeptr);		//boot_free(freeptr);

	return retval;
}
