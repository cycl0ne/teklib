#ifndef _TEK_PROTO_LUA_H
#define _TEK_PROTO_LUA_H

/*
**	$Id: lua.h,v 1.1 2006/08/25 02:24:43 tmueller Exp $
**	teklib/tek/proto/lua.h - Lua module prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/lua.h>
#include <tek/stdcall/lua.h>

extern TMODENTRY TUINT
tek_init_lua5(struct TTask *, struct TModule *, TUINT16, TTAGITEM *);

/* -- utility macros -- */
#define lua_pop(L,n)				lua_settop(L, -(n)-1)
#define lua_register(L,n,f) 		(lua_pushcfunction(L, (f)), lua_setglobal(L, (n)))
#define lua_pushcfunction(L,f)		lua_pushcclosure(L, f, 0)
#define lua_strlen(L,i)				lua_objlen(L, (i))
#define lua_isfunction(L,n)			(lua_type(L,n) == LUA_TFUNCTION)
#define lua_istable(L,n)			(lua_type(L,n) == LUA_TTABLE)
#define lua_islightuserdata(L,n)	(lua_type(L,n) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L,n)				(lua_type(L,n) == LUA_TNIL)
#define lua_isboolean(L,n)			(lua_type(L,n) == LUA_TBOOLEAN)
#define lua_isthread(L,n)			(lua_type(L,n) == LUA_TTHREAD)
#define lua_isnone(L,n)				(lua_type(L,n) == LUA_TNONE)
#define lua_isnoneornil(L, n)		(lua_type(L,n) <= 0)
#define lua_pushliteral(L, s)		lua_pushlstring(L, "" s, (sizeof(s)/sizeof(char))-1)
#define lua_setglobal(L,s)			lua_setfield(L, LUA_GLOBALSINDEX, (s))
#define lua_getglobal(L,s)			lua_getfield(L, LUA_GLOBALSINDEX, (s))
#define lua_getregistry(L)			lua_pushvalue(L, LUA_REGISTRYINDEX)

/* -- auxlib macros -- */
#define luaL_getn(L,i)				((int)lua_objlen(L, i))
#define luaL_setn(L,i,j)			((void)0)  /* no op! */
#define luaL_argcheck(L,cond,numarg,extramsg)	((void)((cond) || luaL_argerror(L, (numarg), (extramsg))))
#define luaL_checkstring(L,n)		(luaL_checklstring(L, (n), TNULL))
#define luaL_optstring(L,n,d)		(luaL_optlstring(L, (n), (d), TNULL))
#define luaL_checkint(L,n)			((TINT)luaL_checkinteger(L, (n)))
#define luaL_optint(L,n,d)			((TINT)luaL_optinteger(L, (n), (d)))
#define luaL_typename(L,i)			lua_typename(L, lua_type(L,(i)))
#define luaL_dofile(L, fn)			(luaL_loadfile(L, fn) || lua_pcall(L, 0, 0, 0))
#define luaL_dostring(L, s)			(luaL_loadstring(L, s) || lua_pcall(L, 0, 0, 0))
#define luaL_opt(L,f,n,d)			(lua_isnoneornil(L,(n)) ? (d) : f(L,(n)))

/* -- auxlib generic buffer manipulation -- */
#define luaL_prepbuffer(B)			_luaL_prepbuffer((B)->L, B)
#define luaL_addlstring(B,s,l)		_luaL_addlstring((B)->L, B, s, l)
#define luaL_addstring(B,s)			_luaL_addstring((B)->L, B, s)
#define luaL_addvalue(B)			_luaL_addvalue((B)->L, B)
#define luaL_pushresult(B)			_luaL_pushresult((B)->L, B)


#define luaL_addchar(B,c) \
  ((void)((B)->p < ((B)->buffer+LUAL_BUFFERSIZE) || luaL_prepbuffer(B)), \
   (*(B)->p++ = (char)(c)))


#endif /* _TEK_PROTO_LUA_H */
