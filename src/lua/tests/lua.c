
/*
**	$Id: lua.c,v 1.1 2006/08/25 02:39:34 tmueller Exp $
**	teklib/src/lua/tests/lua.c - TEKlib-based standalone Lua interpreter
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>
#include <tek/proto/lua.h>
#include <tek/lib/luautil.h>

TAPTR TExecBase, TUtilBase, TIOBase;

typedef struct
{
	TAPTR file;
	TBOOL extraline;
	TUINT8 readbuf[1024];
} luaRun;

static LUACFUNC TSTRPTR
readfunc(lua_State *L, struct LuaExecData *e, size_t *size)
{
	luaRun *lr = e->led_UserData;
	if (lr->extraline)
	{
		lr->extraline = 0;
		*size = 1;
		return "\n";
	}
	if (TFEoF(lr->file))
		return TNULL;
	*size = TFRead(lr->file, lr->readbuf, sizeof(lr->readbuf));
	if (*size > 0)
		return (TSTRPTR) lr->readbuf;
	return TNULL;
}

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TIOBase = TOpenModule("io", 0, TNULL);

	if (TUtilBase && TIOBase)
	{
		TSTRPTR *argv = TGetArgV();
		TSTRPTR fname = argv[1];
		TINT ret = 20;

		if (fname)
		{
			TBOOL isstdin = (TStrCaseCmp("stdio:in", fname) == 0);
			lua_State *L = (lua_State *) TOpenModule("lua5", 0, TNULL);
			if (L)
			{
				luaRun lrun;
				lrun.extraline = 0;
				lrun.file = TOpenFile(fname, TFMODE_READONLY, TNULL);
				if (lrun.file)
				{
					struct LuaExecData chunkexec;
					TINT err;

					TINT c = TFGetC(lrun.file);
					if (c == '#') /* Unix exec. file? */
					{
						lrun.extraline = 1;
						/* skip first line */
						while ((c = TFGetC(lrun.file)) != TEOF && c != '\n');
						if (c == '\n')
							c = TFGetC(lrun.file);
					}
					if (c == LUA_SIGNATURE[0] && !isstdin)
					{
						TSeek(lrun.file, 0, TNULL, TFPOS_BEGIN);
						/* skip eventual `#!...' */
						while ((c = TFGetC(lrun.file)) != TEOF
							&& c != LUA_SIGNATURE[0]);
						lrun.extraline = 0;
					}
					TFUngetC(lrun.file, c);

					TFillMem(&chunkexec, sizeof(chunkexec), 0);

					/* name of the chunk executed */
					chunkexec.led_ChunkName = fname;
					/* program name, aka argv[0] */
					chunkexec.led_ProgName = fname;
					/* remaining argument vector */
					chunkexec.led_ArgV = &argv[2];
					/* read function */
					chunkexec.led_ReadFunc = (lua_Reader) readfunc;
					/* user init function */
					chunkexec.led_InitFunc = TNULL;
					/* userdata */
					chunkexec.led_UserData = &lrun;

					/* set iobase and filehandle if you want errors printed: */
					chunkexec.led_IOBase = TIOBase;
					chunkexec.led_ErrorFH = TErrorFH();

					/* run chunk protected, with userinit */
					err = luaT_runchunk(L, &chunkexec);

					TCloseFile(lrun.file);

					if (err)
						printf("*** Lua initialization or runtime error.\n");
					else if (chunkexec.led_Error)
					{
						printf("*** Lua script error.\n");
						ret = 10;
					}
					else
						ret = chunkexec.led_RetVal;
				}
				else
					printf("*** could not open script: %s\n", fname);
			}
			else
				printf("*** initialization failed.\n");

			if (L)
				TCloseModule((struct TModule *) LUABASE(L));
		}
		else
		{
			printf("Usage: lua FILE [ARGS]\n");
			printf("Run a Lua script with `tek' library extensions.\n");
			printf("Use `stdio:in' to run a script from standard input.\n");
		}

		TDBPRINTF(TDB_INFO,("return value: %d\n", ret));
		TSetRetVal(ret);
	}

	TCloseModule(TIOBase);
	TCloseModule(TUtilBase);
}
