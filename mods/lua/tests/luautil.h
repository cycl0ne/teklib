
#ifndef _LUA_UTIL_H
#define _LUA_UTIL_H

/* 
**	$Id: luautil.h,v 1.2 2005/05/12 13:48:57 tmueller Exp $
**	Lua/TEKlib helper functions. This toolkit should make it
**	dead easy to extend the language from within applications.
*/

#include <tek/mod/lua.h>

/* 
**	Data structure describing Lua chunk to be executed.
*/

struct LuaExecData
{
	TUINT led_Size;					/* sizeof(struct LuaExecData) */
	TUINT led_Flags;				/* control flags, currently always zero */
	TINT led_Error;					/* Major error code (LUA_ERR..., see lua.h) */
	TSTRPTR led_ChunkName;			/* Name of the chunk being executed */
	TSTRPTR led_ProgName;			/* Program name (in the sense of argv[0]) */
	TSTRPTR *led_ArgV;				/* Argument vector argv[1]... */
	lua_Chunkreader led_ReadFunc;	/* User callback for reading a chunk */
	TVOID LUACFUNC (*led_InitFunc)(lua_State *L, struct LuaExecData *e); /* User init callback */
	TAPTR led_UserData;				/* Userdata passed to reader and init callbacks */
	TAPTR led_ErrorFH;				/* TEKlib filehandle for error printing (may be TNULL) */
	TAPTR led_IOBase;				/* I/O module base ptr (if you want error output) */
	TINT led_RetVal;
};

#ifdef __cplusplus
extern "C" {
#endif

extern TVOID luaT_addfunc(lua_State *L, TSTRPTR name, lua_CFunction func, TAPTR userdata);
extern TVOID luaT_remfunc(lua_State *L, TSTRPTR name);
extern TVOID luaT_addclass(lua_State *L, TSTRPTR libname, TSTRPTR classname, luaL_reg *functions, luaL_reg *methods, TAPTR userdata);
extern TVOID luaT_getargs(lua_State *L, TSTRPTR progname, TSTRPTR *argv);
extern TINT luaT_runchunk(lua_State *L, struct LuaExecData *e);
extern TVOID luaT_addlib(lua_State *L, TSTRPTR libname, luaL_reg *functions, TAPTR userdata);

#ifdef __cplusplus
}
#endif

/*
**	Revision History
**	$Log: luautil.h,v $
**	Revision 1.2  2005/05/12 13:48:57  tmueller
**	added return value processing
**	
**	Revision 1.1  2005/05/08 19:13:21  tmueller
**	added
**	
**	Revision 1.1.1.1  2003/12/11 07:12:38  tmueller
**	Krypton import
**	
**	Revision 1.4  2003/08/14 19:16:07  tmueller
**	Housekeeping and more cleanup. Added some standard CVS headers.
**	
*/

#endif
