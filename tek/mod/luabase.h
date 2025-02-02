#ifndef _TEK_MOD_LUABASE_H
#define _TEK_MOD_LUABASE_H

#include <tek/mod/lua.h>
#include <tek/mod/io.h>

typedef struct TLuaBase
{
	struct TModule tml_Module;
	struct TUtilBase *tml_UtilBase;
	struct TIOBase *tml_IOBase;
	TFILE *tml_StdIn;
	TFILE *tml_StdOut;
	TFILE *tml_StdErr;
	struct TMemManager *tml_MM;
	struct TLock *tml_Lock;
	TINT tml_RefCount;

} TLUABASE;

#define lua_ExecBase TGetExecBase((struct TLuaBase *) LUASUPER(L))
#define lua_UtilBase ((struct TLuaBase *)LUASUPER(L))->tml_UtilBase
#define lua_IOBase ((struct TLuaBase *)LUASUPER(L))->tml_IOBase
#define lua_StdIn ((struct TLuaBase *)LUASUPER(L))->tml_StdIn
#define lua_StdOut ((struct TLuaBase *)LUASUPER(L))->tml_StdOut
#define lua_StdErr ((struct TLuaBase *)LUASUPER(L))->tml_StdErr

#endif
