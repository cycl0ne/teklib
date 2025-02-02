
#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>

#include "global.h"

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TIOBase;
TAPTR MMU;
struct dStrings *DSTR;
TTAG args[ARG_NUM];

/*****************************************************************************/
/*
**	scan recursively
*/

static TBOOL
scan(TSTRPTR path, TSTRPTR newpath, TSTRPTR fname, TSTRPTR context) {
	TTAGITEM extags[3];
	TSTRPTR name;
	TUINT type;
	TAPTR lock;
	TSTRPTR fullpath = TNULL;
	TINT l1, l2;
	TBOOL success = TTRUE;

	l1 = TStrLen(path);
	l2 = TStrLen(newpath);
	fullpath = TAlloc(MMU, l1 + l2 + 2);
	if (fullpath) {
		TStrCpy(fullpath, path);
		if (l1) TStrCat(fullpath, "/");
		TStrCat(fullpath, newpath);
	}

	if (!fullpath) return TFALSE;

	extags[0].tti_Tag = TFATTR_Type;
	extags[0].tti_Value = (TTAG) &type;
	extags[1].tti_Tag = TFATTR_Name;
	extags[1].tti_Value = (TTAG) &name;
	extags[2].tti_Tag = TTAG_DONE;

	lock = TLockFile(fullpath, TFLOCK_READ, TNULL);
	if (lock) {
		if (TExamine(lock, TNULL) == 0) {
			while (success && TExNext(lock, extags) == 2) {
				if (type == TFTYPE_Directory) {
					success = scan(fullpath, name, fname, context);
				} else if (type == TFTYPE_File) {
					if (TStrCmp(fname, name) == 0) {
						TSTRPTR fullname;
						TINT l = TStrLen(fullpath);
						l += TStrLen(name);
						fullname = TAlloc(MMU, l + 2);
						if (fullname) {
							TStrCpy(fullname, fullpath);
							TStrCat(fullname, "/");
							TStrCat(fullname, name);
							if (!args[ARG_QUIET])
								printf("processing %s\n", fullname);
							success = docontext(fullname, context);
							TFree(fullname);
						}
					}
				}
			}
		}
		TUnlockFile(lock);
	}

	TFree(fullpath);
	return success;
}

/*****************************************************************************/

static TSTRPTR
gettekname(TSTRPTR pathname)
{
	TINT tlen = TMakeName(pathname, TNULL, 0, TPPF_HOST2TEK, TNULL);
	if (tlen > 0)
	{
		TSTRPTR tekname = TAlloc(TNULL, tlen + 1);
		if (tekname)
		{
			TMakeName(pathname, tekname, tlen + 1, TPPF_HOST2TEK, TNULL);
			return tekname;
		}
	}
	return TNULL;
}

/*****************************************************************************/

static TBOOL
do_tmkmf(void)
{
	TBOOL success = TFALSE;
	TAPTR buildlock =
		TLockFile((TSTRPTR) args[ARG_BUILDDIR], TFLOCK_READ, TNULL);
	if (buildlock)
	{
		if (TAssignLock("BUILD", buildlock))
		{
			if (args[ARG_RECURSE])
			{
				if (!args[ARG_QUIET])
					printf("processing build context %s recursively\n",
						(TSTRPTR) args[ARG_CONTEXT]);
				success = scan("", "", (TSTRPTR) args[ARG_FROM],
					(TSTRPTR) args[ARG_CONTEXT]);
			}
			else
			{
				TCHR hostfname[2048];
				if (TMakeName((TSTRPTR) args[ARG_FROM], hostfname,
					sizeof(hostfname), TPPF_HOST2TEK, TNULL) >= 0)
				{
					if (!args[ARG_QUIET])
						printf("processing build context %s\n",
							(TSTRPTR) args[ARG_CONTEXT]);
					success = docontext(hostfname,
						(TSTRPTR) args[ARG_CONTEXT]);
				}
			}
		}
		else
		{
			printf("*** could not assign BUILD: directory\n");
			TUnlockFile(buildlock);
		}
	}
	else
	{
		printf("*** could not lock BUILD: directory\n");
	}
	return success;
}

/*****************************************************************************/

static TBOOL
do_mkdir(TSTRPTR path)
{
	TAPTR lock;
	TSTRPTR p = path;

	while ((p = TStrChr(p, '/')))
	{
		if (p - path > 0)
		{
			TSTRPTR sub = TStrNDup(TNULL, path, p - path);
			if (sub == TNULL) return TFALSE;
			lock = TMakeDir(sub, TNULL);
			TFree(sub);
			if (lock)
			{
				TUnlockFile(lock);
			}
			p++;
		}
	}

	lock = TMakeDir(path, TNULL);
	if (lock)
	{
		TUnlockFile(lock);
		return TTRUE;
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	main
*/

#define DEFAULT_CONTEXT "posix_linux"

void TEKMain(struct TTask *task)
{
	struct dStrings dstr;

	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TIOBase = TOpenModule("io", 0, TNULL);
	MMU = TCreateMemManager(TNULL, TMMT_Tracking, TNULL);
	DSTR = &dstr;

	if (TUtilBase && TIOBase && MMU && dInitStrings(DSTR))
	{
		TSTRPTR arguments = TGetArgs();

		TAPTR arghandle;
		TBOOL success = TFALSE;

		args[ARG_FROM] = (TTAG) "tmkmakefile";
		args[ARG_CONTEXT] = (TTAG) DEFAULT_CONTEXT;
		args[ARG_RECURSE] = TFALSE;
		args[ARG_BUILDDIR] = (TTAG) "PROGDIR://build";
		args[ARG_QUIET] = TFALSE;
		args[ARG_MAKEDIR] = TNULL;
		args[ARG_HELP] = TFALSE;

		arghandle = TParseArgs(ARG_TEMPLATE, arguments, args);

		if (args[ARG_HELP] || !arghandle)
		{
			printf("Usage: tmkmf %s\n", ARG_TEMPLATE);
			printf("-f=FROM       : tmkmakefile to process [default: tmkmakefile]\n");
			printf("-c=CONTEXT/K  : build context [default: %s]\n", DEFAULT_CONTEXT);
			printf("-r=RECURSE/S  : recurse from current directory\n");
			printf("-b=BUILDDIR/K : path to BUILD: directory [default: PROGDIR://build]\n");
			printf("-q=QUIET/S    : silent execution\n");
			printf("-m=MAKEDIR/K  : create a directory, with full path\n");
			printf("-h=HELP/S     : get this help\n");
			printf("Render TEKlib tmkmakefile(s) to context-specific makefile(s).\n");
			printf("If RECURSE is given then FROM specifies the filename to look up\n");
			printf("in a recursice directory scan, starting at the current directory.\n");
			printf("Otherwise FROM specifies the path and filename of a distinct\n");
			printf("tmkmakefile.\n");
		}
		else if (arghandle)
		{
			if (args[ARG_MAKEDIR])
			{
				TSTRPTR tname = gettekname((TSTRPTR) args[ARG_MAKEDIR]);
				if (tname)
				{
					success = do_mkdir(tname);
					TFree(tname);
				}
			}
			else
				success = do_tmkmf();
		}

		TDestroy(arghandle);

		dExitStrings(DSTR);
		TDestroy(MMU);
		TCloseModule(TIOBase);
		TCloseModule(TUtilBase);

		if (!success) printf("*** not done.\n");
	}
}
