
/* 
**	Lua runner
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>
#include <tek/proto/lua.h>

#include "luautil.h"

TAPTR TExecBase, TUtilBase, TIOBase;

typedef struct
{
	TAPTR file;
	TUINT8 readbuf[1024];
} luaRun;

static LUACFUNC TSTRPTR
readfunc(lua_State *L, struct LuaExecData *e, size_t *size)
{
	luaRun *lr = e->led_UserData;
	*size = TRead(lr->file, lr->readbuf, sizeof(lr->readbuf));
	if (*size > 0) return (TSTRPTR) lr->readbuf;
	return TNULL;
}

TTASKENTRY TVOID 
TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TIOBase = TOpenModule("io", 0, TNULL);
	if (TUtilBase && TIOBase)
	{
		TINT ret = 20;
		lua_State *L = TOpenModule("lua5", 0, TNULL);
		if (L)
		{
			TSTRPTR *argv = TGetArgV();
			TSTRPTR fname = argv[1];
			if (fname == TNULL)
			{
				printf("TEKlib Lua launcher\n");
				printf("Usage: lua script [args]\n");
			}
			else
			{
				luaRun lrun;
				lrun.file = TOpenFile(fname, TFMODE_READONLY, TNULL);
				if (lrun.file)
				{
					struct LuaExecData chunkexec;
					TINT err;
					
					TFillMem(&chunkexec, sizeof(chunkexec), 0);
					chunkexec.led_ChunkName = fname;						/* name of the chunk executed */
					chunkexec.led_ProgName = fname;							/* program name, aka argv[0] */
					chunkexec.led_ArgV = &argv[2];							/* argument vector */
					chunkexec.led_ReadFunc = (lua_Chunkreader) readfunc;	/* read function */
					chunkexec.led_InitFunc = TNULL;							/* user init function */
					chunkexec.led_UserData = &lrun;							/* userdata */
	
					/* set these if you want errors printed */
					chunkexec.led_IOBase = TIOBase;							/* I/O module base */
					chunkexec.led_ErrorFH = TErrorFH();						/* filehandle to print errors to */
					
					err = luaT_runchunk(L, &chunkexec);
					TCloseFile(lrun.file);
	
					if (err)
					{
						printf("*** Lua initialization or runtime error.\n");
					}
					else if (chunkexec.led_Error)
					{
						printf("*** Script error.\n");
						ret = 10;
					}
					else
					{
						ret = chunkexec.led_RetVal;
					}
				}
				else
				{
					printf("*** could not open script: %s\n", fname);
				}
			}
		}
		else
		{
			printf("*** initialization failed.\n");
		}

		if (L) TCloseModule(LUABASE(L));
		
		tdbprintf1(5, "return value: %d\n", ret);
		TSetRetVal(ret);
	}
	
	TCloseModule(TIOBase);
	TCloseModule(TUtilBase);
}

