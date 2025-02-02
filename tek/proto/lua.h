
#ifndef _TEK_PROTO_LUA_H
#define _TEK_PROTO_LUA_H

/*
**	$Id: lua.h,v 1.5 2005/09/13 02:45:20 tmueller Exp $
**	teklib/tek/proto/lua.h - Lua module prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/lua.h>
#include <tek/ansicall/lua.h>

extern TMODENTRY TUINT
tek_init_lua5(TAPTR, struct TModule *, TUINT16, TTAGITEM *);

/* -- utility macros -- */
#define lua_pop(L,n)				lua_settop((L), -(n)-1)
#define lua_pushliteral(L, s)		lua_pushlstring((L), "" s, (sizeof(s)/sizeof(char))-1)
#define lua_boxpointer(L,u)			(*(TAPTR *)(lua_newuserdata(L, sizeof(TAPTR))) = (u))
#define lua_unboxpointer(L,i)		(*(TAPTR *)(lua_touserdata(L, i)))
#define lua_pop(L,n)				lua_settop((L), -(n)-1)
#define lua_register(L,n,f)			(lua_pushstring(L, n), lua_pushcfunction(L, f), lua_settable(L, LUA_GLOBALSINDEX))
#define lua_pushcfunction(L,f)		lua_pushcclosure(L, f, 0)
#define lua_isfunction(L,n)			(lua_type(L,n) == LUA_TFUNCTION)
#define lua_istable(L,n)			(lua_type(L,n) == LUA_TTABLE)
#define lua_islightuserdata(L,n)	(lua_type(L,n) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L,n)				(lua_type(L,n) == LUA_TNIL)
#define lua_isboolean(L,n)			(lua_type(L,n) == LUA_TBOOLEAN)
#define lua_isnone(L,n)				(lua_type(L,n) == LUA_TNONE)
#define lua_isnoneornil(L, n)		(lua_type(L,n) <= 0)

/* -- compatibility macros -- */
#define lua_getglobal(L,s)			(lua_pushstring(L, s), lua_gettable(L, LUA_GLOBALSINDEX))
#define lua_setglobal(L,s)			(lua_pushstring(L, s), lua_insert(L, -2), lua_settable(L, LUA_GLOBALSINDEX))

/* -- auxlib macros -- */
#define luaL_checkstring(L,n)		(luaL_checklstring(L, (n), TNULL))
#define luaL_optstring(L,n,d)		(luaL_optlstring(L, (n), (d), TNULL))
#define luaL_checkint(L,n)			((TINT)luaL_checknumber(L, n))
#define luaL_optint(L,n,d)			((TINT)luaL_optnumber(L, n,(lua_Number)(d)))

#endif /* _TEK_PROTO_LUA_H */
