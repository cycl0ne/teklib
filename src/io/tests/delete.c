
/*
**	$Id: delete.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/io/tests/delete.c - Io module unit test
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

#define STATUS_OK		0
#define STATUS_ERROR	10
#define STATUS_FAIL		20

static TBOOL deltree(TSTRPTR path, TSTRPTR newpath, TTAG *args);

#define ARG_TEMPLATE "-f=FILE/A/M,-a=ALL/S,-q=QUIET/S,IAGREE/S,-h=HELP/S"
enum { ARG_FILE, ARG_ALL, ARG_QUIET, ARG_AGREE, ARG_HELP, ARG_NUM };

void TEKMain(struct TTask *task)
{
	TUINT status = STATUS_FAIL;

	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TIOBase = TOpenModule("io", 0, TNULL);

	if (TUtilBase && TIOBase)
	{
		TSTRPTR *argv = TGetArgV();
		TAPTR arghandle;
		TTAG args[ARG_NUM];

		status = STATUS_ERROR;

		args[ARG_FILE] = TNULL;
		args[ARG_ALL] = TFALSE;
		args[ARG_QUIET] = TFALSE;
		args[ARG_AGREE] = TFALSE;
		args[ARG_HELP] = TFALSE;

		arghandle = TParseArgV(ARG_TEMPLATE, argv + 1, args);
		if (!arghandle || args[ARG_HELP] || !args[ARG_AGREE])
		{
			printf("Usage:      delete %s\n", ARG_TEMPLATE);
			printf("-f=FILE/A/M files or directories to delete\n");
			printf("-a=ALL/S    delete recursively\n");
			printf("-q=QUIET/S  be quiet\n");
			printf("IAGREE/S    The I/O module is in beta stage. You must specify this\n");
			printf("            option in order to actually delete something. NOTE THAT\n");
			printf("            THIS TEST USES THE TEKLIB FILESYSTEM NAMING CONVENTIONS!\n");
			printf("-h=HELP/S   this help\n");
		}
		else
		{
			TSTRPTR *entries = (TSTRPTR *) args[ARG_FILE];
			TSTRPTR entry;

			status = STATUS_OK;

			while (status == STATUS_OK && (entry = *entries++))
			{
				if (!deltree(entry, TNULL, args))
				{
					printf("could not delete %s\n", entry);
					status = STATUS_ERROR;
				}
			}
		}

		TDestroy(arghandle);
	}

	if (status != STATUS_OK)
		TSetRetVal(status);

	TCloseModule(TIOBase);
	TCloseModule(TUtilBase);
}


static TSTRPTR addpath(TSTRPTR part1, TSTRPTR part2)
{
	TSTRPTR fullpath = TNULL;
	TINT flen;

	if (!part1) part1 = "";
	if (!part2) part2 = "";

	flen = TAddPart(part1, part2, TNULL, 0);
	if (flen >= 0)
	{
		fullpath = TAlloc(TNULL, flen + 1);
		if (fullpath)
			TAddPart(part1, part2, fullpath, flen + 1);
	}

	return fullpath;
}


static TBOOL deltree(TSTRPTR path, TSTRPTR newpath, TTAG *args)
{
	TSTRPTR fullpath;
	TAPTR lock;
	TBOOL success = TTRUE;

	fullpath = addpath(path, newpath);
	if (!fullpath)
	{
		printf("could not use path\n");
		return TFALSE;
	}

	if (args[ARG_ALL])
	{
		lock = TLockFile(fullpath, TFLOCK_READ, TNULL);
		if (lock)
		{
			TUINT type = TFTYPE_Unknown;
			TSTRPTR name;
			TTAGITEM extags[3];

			extags[0].tti_Tag = TFATTR_Type;
			extags[0].tti_Value = (TTAG) &type;
			extags[1].tti_Tag = TFATTR_Name;
			extags[1].tti_Value = (TTAG) &name;
			extags[2].tti_Tag = TTAG_DONE;

			TExamine(lock, extags);
			if (type == TFTYPE_Directory)
			{
				while (success && TExNext(lock, extags) == 2)
				{
					if (type == TFTYPE_Directory)
						success = deltree(fullpath, name, args);
					else
					{
						TSTRPTR delname = addpath(fullpath, name);

						success = TFALSE;
						if (delname)
						{
							if (!args[ARG_QUIET]) puts(delname);
							success = TDeleteFile(delname);
							TFree(delname);
						}
					}
				}
			}

			TUnlockFile(lock);
		}
	}

	if (success)
	{
		if (!args[ARG_QUIET]) puts(fullpath);
		success = TDeleteFile(fullpath);
	}

	TFree(fullpath);
	return success;
}
