
/*
**	$Id: luautil.c,v 1.1 2006/08/25 02:39:34 tmueller Exp $
**	teklib/src/lua/tek/luautil.c - Lua/TEKlib helper functions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/proto/io.h>
#include <tek/proto/lua.h>
#include <tek/lib/luautil.h>

/*****************************************************************************/
/*
**	success = luaT_addfunc(L, name, func, userdata)
**		add a simple function to the lua namespace
**
**	luaT_remfunc(L, name)
**		remove a simple function from the lua namespace
*/

TLIBAPI void
luaT_addfunc(lua_State *L, TSTRPTR name, lua_CFunction func, TAPTR userdata)
{
	lua_pushstring(L, name);
	lua_pushlightuserdata(L, userdata);
	lua_pushcclosure(L, func, 1);
	lua_settable(L, LUA_GLOBALSINDEX);
}

TLIBAPI void
luaT_remfunc(lua_State *L, TSTRPTR name)
{
	lua_pushstring(L, name);
	lua_pushnil(L);
	lua_settable(L, LUA_GLOBALSINDEX);
}

/*****************************************************************************/
/*
**	luaT_addclass(L, libname, classname, functable, methodtable, userdata)
**
**	register a class as part of a lua function library. functions
**	registered can be accessed in the table "libname", objects
**	are maintained with a class named "classname", e.g.:
**
**		luaT_addclass(L, "tek", "win", funcs, methods, userdata)
**
**	This may register the functions tek.openwin() and tek.closewin()
**	as part of the "tek" library; methods of the "win" class may
**	be close(), hide() etc. Include a method named "__gc" to include
**	garbage collector support for the respective class.
*/

TLIBAPI void
luaT_addclass(lua_State *L, TSTRPTR libname, TSTRPTR classname,
	luaL_Reg *functions, luaL_Reg *methods, TAPTR userdata)
{
	luaL_newmetatable(L, classname);		/* classtab */
	lua_pushliteral(L, "__index");			/* classtab, "__index" */

	/* insert self: classtab.__index = classtab */
	lua_pushvalue(L, -2);					/* classtab, "__index", classtab */
	lua_rawset(L, -3);						/* classtab */

	/* insert methods. consume 1 userdata. do not create a new tab */
	lua_pushlightuserdata(L, userdata);		/* classtab, userdata */
	luaL_openlib(L, TNULL, methods, 1);		/* classtab */

	/* first upvalue: userdata */
	lua_pushlightuserdata(L, userdata);		/* classtab, userdata */

	/* duplicate table argument to be used as second upvalue for cclosure */
	lua_pushvalue(L, -2);					/* classtab, userdata, classtab */

	/* insert functions */
	luaL_openlib(L, libname, functions, 2);	/* classtab, libtab */

	/* adjust stack */
	lua_pop(L, 2);
}

/*****************************************************************************/
/*
**	luaT_addlib(L, libname, functable, userdata)
*/

TLIBAPI void
luaT_addlib(lua_State *L, TSTRPTR libname, luaL_Reg *functions, TAPTR userdata)
{
	lua_pushlightuserdata(L, userdata);			/* stack: userdata */
	luaL_openlib(L, libname, functions, 1);		/* stack: libtab */
	lua_pop(L, 1);								/* adjust stack */
}

/*****************************************************************************/
/*
**	Place commandline arguments into Lua namespace. They
**	will be available in a global table named "arg".
*/

TLIBAPI void
luaT_getargs(lua_State *L, TSTRPTR progname, TSTRPTR *argv)
{
	TINT i, n = 0;

	lua_newtable(L);

	lua_pushnumber(L, 0);
	lua_pushstring(L, progname);
	lua_rawset(L, -3);

	for (i = 0; argv[i]; ++i)
	{
		lua_pushnumber(L, (lua_Number) (i + 1));
		lua_pushstring(L, argv[i]);
		lua_rawset(L, -3);
		n++;
	}

	/* arg.n = maximum index in table `arg' */
	lua_pushliteral(L, "n");
	lua_pushnumber(L, (lua_Number) n);
	lua_rawset(L, -3);

	lua_setglobal(L, "arg");
}

static LUACFUNC TINT
luaTi_traceback(lua_State *L)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);  /* pass error message */
	lua_pushinteger(L, 2);  /* skip this function and traceback */
	lua_call(L, 2, 1);  /* call debug.traceback */
	return 1;
}

static int
luaTi_tracecall(lua_State *L, int narg)
{
	int status;
	int base = lua_gettop(L) - narg;  /* function index */
	lua_pushcfunction(L, luaTi_traceback);  /* push traceback function */
	lua_insert(L, base);  /* put it under chunk and args */
	status = lua_pcall(L, narg, 1, base);
	lua_remove(L, base);  /* remove traceback function */
	/* force a complete garbage collection in case of errors */
	if (status != 0)
		lua_gc(L, LUA_GCCOLLECT, 0);
	return status;
}

/*
**	this function is executed in a protected environment, i.e.
**	occurances of lua_error() will be caught as exceptions,
**	and cause lua_cpcall() to return with an error value.
*/

static LUACFUNC TINT
luaTi_pmain(lua_State *L)
{
	struct LuaExecData *e = lua_touserdata(L, 1);

	e->led_Error = LUA_ERRMEM;

	/* make commandline arguments available */
	if (e->led_ProgName && e->led_ArgV)
		luaT_getargs(L, e->led_ProgName, e->led_ArgV);

	/* call user init function */
	if (e->led_InitFunc)
		(*e->led_InitFunc)(L, e);

	/* load chunk */
	e->led_Error = lua_load(L, e->led_ReadFunc, e, e->led_ChunkName);
	switch (e->led_Error)
	{
		case 0:
			e->led_Error = luaTi_tracecall(L, 0);
			if (e->led_Error == 0)
			{
				/* success */
				e->led_RetVal = (TINT) lua_tonumber(L, 2);
				break;
			}
			/* fall through: */
		default:
			/* error */
			if (e->led_IOBase && e->led_ErrorFH)
			{
				TSTRPTR msg = lua_tostring(L, -1);
				if (!msg) msg = "(error with no message)";
				TIOFPutS(e->led_IOBase, e->led_ErrorFH, msg);
				TIOFPutS(e->led_IOBase, e->led_ErrorFH, "\n");
			}
			lua_pop(L, 1);
	}

	return e->led_Error;
}

/*****************************************************************************/
/*
**	error = luaT_runchunk(L, execdata)
**	execute protected environment as described by LuaExecData
*/

TLIBAPI TINT
luaT_runchunk(lua_State *L, struct LuaExecData *e)
{
	TINT err = lua_cpcall(L, &luaTi_pmain, e);
	if (err)
		e->led_Error = err;
	return err;
}
