
#ifndef _TEK_MOD_LUA_MOD_H
#define _TEK_MOD_LUA_MOD_H	1

/* 
**	$Id: lua_mod.h,v 1.4 2005/09/08 00:02:23 tmueller Exp $
**	Lua / TEKlib module interface (module internal)
*/

#include <stdlib.h>

#include <tek/debug.h>
#include <tek/mod/lua.h>

#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/time.h>
#include <tek/proto/io.h>

#ifndef LUA_API
#define LUA_API		TMODAPI
#endif

#ifndef LUALIB_API
#define LUALIB_API	TMODAPI
#endif

#undef USE_DLL
#undef USE_DYLD
#undef USE_DLOPEN

#define LUA_DIRSEP	"/"

#define LUA_ROOT	"LUA:"
#define LUA_LDIR	LUA_ROOT "lib"
#define LUA_CDIR	LUA_ROOT "mod"
#define LUA_PATH_DEFAULT	"?;?.lua;" LUA_LDIR "/?.lua;" LUA_LDIR "/?/init.lua"
#define LUA_CPATH_DEFAULT	"?.mod;" LUA_CDIR "/?.mod"


typedef struct
{
	TMODL tml_Module;
	TAPTR tml_ExecBase;
	TAPTR tml_UtilBase;
	TAPTR tml_TimeBase;
	TAPTR tml_IOBase;
	TAPTR tml_StdIn;
	TAPTR tml_StdOut;
	TAPTR tml_StdErr;
	TAPTR tml_MMU;
	TAPTR tml_Lock;
	TINT tml_RefCount;

} TMOD_LUA;

#define TExecBase ((TMOD_LUA *) LUASUPER(L))->tml_ExecBase
#define TUtilBase ((TMOD_LUA *) LUASUPER(L))->tml_UtilBase
#define TIOBase ((TMOD_LUA *) LUASUPER(L))->tml_IOBase
#define TTimeBase ((TMOD_LUA *) LUASUPER(L))->tml_TimeBase

#define LUA_TEKLIBNAME	"tek"
LUALIB_API int (luaopen_tek) (lua_State *L);

/*
**	Revision History
**	$Log: lua_mod.h,v $
**	Revision 1.4  2005/09/08 00:02:23  tmueller
**	removed TIO_NO_API_COMPAT
**	
**	Revision 1.3  2005/05/11 00:30:57  tmueller
**	added '?' to package path lookup patterns
**	
**	Revision 1.2  2005/05/10 09:04:34  tmueller
**	Improved default TEKlib paths for package loading
**	
**	Revision 1.1  2005/05/08 19:11:45  tmueller
**	added
**	
*/

#endif
