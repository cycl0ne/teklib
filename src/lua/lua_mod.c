
/*
**	$Id: lua_mod.c,v 1.6 2006/11/11 14:19:10 tmueller Exp $
**	teklib/src/lua/lua_mod.c - Lua module API implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	module memory layout:
**
**	  ____________
**	 |            |
**	 |  function  |
**	 |    table   |
**	_|____________|_ <---.   _ _____________ _ module instance
**	 |            |      |    |             |
**	 |           -+------^----+-   module   |   LUA_USERSTATE
**	 |  TLUABASE  |  modsuper |    header   |
**	 | (modsuper) |  backptr  |_____________|_ L
**	 |            |           |             |
**	 |            |           |             |
**	 |____________|           |  Lua state  |
**	                          |             |
**	                          |_____________|
**
**	this layout solves these problems:
**
**	* changes in the Lua state structure do not affect binary
**	  compatibility
**  * the Lua state itself is the module base, API functions
**	  need no additional modulebase argument
**
**	caveats:
+*
**	* only one function table for all instances; double indirect calls;
**	* TCloseModule() must be applied to ((struct TLuaModule *) L) - 1
*/

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/inline/exec.h>
#include <tek/proto/io.h>
#include <tek/mod/time.h>
#include <tek/mod/luabase.h>

#define LUAMOD_VERSION		1
#define LUAMOD_REVISION		0
#define LUAMOD_NUMVECTORS	114

static THOOKENTRY TTAG luamod_dispatch(struct THook *hook, TAPTR obj, TTAG msg);
static TAPTR luamod_open(TLUABASE *mod, TTAGITEM *tags);
static void luamod_close(LUA_USERSTATE *inst);

static const TMFPTR luamod_vectors[LUAMOD_NUMVECTORS];

LUALIB_API TINT luaopen_tek(lua_State *L);

/*****************************************************************************/
/*
**	Module init
*/

TMODENTRY TUINT
tek_init_lua5(struct TTask *task, TLUABASE *mod, TUINT16 version, TTAGITEM *tags)
{
	struct TExecBase *TExecBase;
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * LUAMOD_NUMVECTORS; /* negative size */

		if (version <= LUAMOD_VERSION)
			return sizeof(TLUABASE); /* positive size */

		return 0;
	}

	TExecBase = TGetExecBase(mod);
	mod->tml_Lock = TCreateLock(TNULL);
	if (mod->tml_Lock)
	{
		#ifdef TDEBUG
		mod->tml_MM = TCreateMemManager(TNULL,
			TMMT_Tracking | TMMT_TaskSafe, TNULL);
		#endif

		mod->tml_Module.tmd_Version = LUAMOD_VERSION;
		mod->tml_Module.tmd_Revision = LUAMOD_REVISION;
		mod->tml_Module.tmd_Handle.thn_Hook.thk_Entry = luamod_dispatch;
		mod->tml_Module.tmd_Flags = TMODF_OPENCLOSE | TMODF_VECTORTABLE;
		TInitVectors(&mod->tml_Module, luamod_vectors, LUAMOD_NUMVECTORS);
		return TTRUE;
	}

	return 0;
}

static THOOKENTRY TTAG
luamod_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	TLUABASE *mod = (TLUABASE *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			TDestroy((struct THandle *) mod->tml_Lock);
			TDestroy((struct THandle *) mod->tml_MM);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) luamod_open(mod, obj);
		case TMSG_CLOSEMODULE:
			luamod_close(obj);
	}
	return 0;
}

/*****************************************************************************/
/*
**	wrappers for functions that are now implemented differently
*/

static TMODAPI void
_lua_setn_obsolete(lua_State *L, TINT t, TINT n)
{
	/* no op */
}

static TMODAPI void
_lua_newtable(lua_State *L, TINT narr, TINT nrec)
{
	lua_createtable(L, 0, 0);
}

static TMODAPI const char *
_lua_tostring(lua_State *L, int idx)
{
	return lua_tolstring(L, idx, NULL);
}

static TMODAPI void
_luaL_getmetatable(lua_State *L, int idx, const char *tname)
{
	lua_getfield(L, LUA_REGISTRYINDEX, tname);
}

/*
**	ugly - these functions do not have a state argument
**	and must be wrapped back and forth.
*/

static TMODAPI TSTRPTR
_luaL_prepbuffer(lua_State *L, luaL_Buffer *B)
{
	return luaL_prepbuffer(B);
}

static TMODAPI void
_luaL_addlstring(lua_State *L, luaL_Buffer *B, TSTRPTR s, TSIZE l)
{
	luaL_addlstring(B, s, l);
}

static TMODAPI void
_luaL_addstring(lua_State *L, luaL_Buffer *B, TSTRPTR s)
{
	luaL_addstring(B, s);
}

static TMODAPI void
_luaL_addvalue(lua_State *L, luaL_Buffer *B)
{
	luaL_addvalue(B);
}

static TMODAPI void
_luaL_pushresult(lua_State *L, luaL_Buffer *B)
{
	luaL_pushresult(B);
}

/*****************************************************************************/

static const TMFPTR
luamod_vectors[LUAMOD_NUMVECTORS] =
{
	(TMFPTR) TNULL,				/* reserved */
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) lua_newthread,
	(TMFPTR) lua_atpanic,

	(TMFPTR) lua_call,
	(TMFPTR) lua_pcall,
	(TMFPTR) lua_cpcall,
	(TMFPTR) lua_load,
	(TMFPTR) lua_dump,

	(TMFPTR) lua_gettop,
	(TMFPTR) lua_settop,
	(TMFPTR) lua_pushvalue,
	(TMFPTR) lua_remove,
	(TMFPTR) lua_insert,
	(TMFPTR) lua_replace,
	(TMFPTR) lua_checkstack,
	(TMFPTR) lua_xmove,

	(TMFPTR) lua_isnumber,
	(TMFPTR) lua_isstring,
	(TMFPTR) lua_iscfunction,
	(TMFPTR) lua_isuserdata,
	(TMFPTR) lua_type,
	(TMFPTR) lua_typename,

	(TMFPTR) lua_equal,
	(TMFPTR) lua_rawequal,
	(TMFPTR) lua_lessthan,

	(TMFPTR) lua_tonumber,
	(TMFPTR) lua_toboolean,
	(TMFPTR) _lua_tostring,
	(TMFPTR) lua_objlen,
	(TMFPTR) lua_tocfunction,
	(TMFPTR) lua_touserdata,
	(TMFPTR) lua_tothread,
	(TMFPTR) lua_topointer,

	(TMFPTR) lua_pushnil,
	(TMFPTR) lua_pushnumber,
	(TMFPTR) lua_pushlstring,
	(TMFPTR) lua_pushstring,
	(TMFPTR) lua_pushinteger,
	(TMFPTR) lua_pushcclosure,
	(TMFPTR) lua_pushboolean,
	(TMFPTR) lua_pushlightuserdata,

	(TMFPTR) lua_gettable,
	(TMFPTR) lua_rawget,
	(TMFPTR) lua_rawgeti,
	(TMFPTR) _lua_newtable,
	(TMFPTR) lua_newuserdata,
	(TMFPTR) lua_getmetatable,
	(TMFPTR) lua_getfenv,

	(TMFPTR) lua_settable,
	(TMFPTR) lua_rawset,
	(TMFPTR) lua_rawseti,
	(TMFPTR) lua_setmetatable,
	(TMFPTR) lua_setfenv,

	(TMFPTR) lua_yield,
	(TMFPTR) lua_resume,

	(TMFPTR) TNULL,		/* lua_getgcthreshold */
	(TMFPTR) TNULL,		/* lua_getgccount */
	(TMFPTR) TNULL,		/* lua_setgcthreshold */
	(TMFPTR) TNULL,		/* lua_version */

	(TMFPTR) lua_error,
	(TMFPTR) lua_next,
	(TMFPTR) lua_concat,

	/* auxlib functions */

	(TMFPTR) luaI_openlib,
	(TMFPTR) luaL_getmetafield,
	(TMFPTR) luaL_callmeta,
	(TMFPTR) luaL_typerror,
	(TMFPTR) luaL_argerror,
	(TMFPTR) luaL_checklstring,
	(TMFPTR) luaL_optlstring,
	(TMFPTR) luaL_checknumber,
	(TMFPTR) luaL_optnumber,
	(TMFPTR) luaL_checkstack,
	(TMFPTR) luaL_checktype,
	(TMFPTR) luaL_checkany,
	(TMFPTR) luaL_newmetatable,
	(TMFPTR) _luaL_getmetatable,
	(TMFPTR) luaL_checkudata,
	(TMFPTR) luaL_where,
	(TMFPTR) luaL_ref,
	(TMFPTR) luaL_unref,
	(TMFPTR) lua_objlen,
	(TMFPTR) _lua_setn_obsolete,
	(TMFPTR) luaL_loadfile,
	(TMFPTR) luaL_loadbuffer,

	(TMFPTR) luaL_gsub,
	(TMFPTR) lua_getfield,
	(TMFPTR) lua_setfield,

	(TMFPTR) lua_tointeger,
	(TMFPTR) lua_tolstring,
	(TMFPTR) lua_pushthread,
	(TMFPTR) lua_createtable,
	(TMFPTR) lua_status,
	(TMFPTR) lua_gc,
	(TMFPTR) lua_getallocf,
	(TMFPTR) lua_setallocf,
	(TMFPTR) luaL_register,
	(TMFPTR) luaL_checkinteger,
	(TMFPTR) luaL_optinteger,
	(TMFPTR) luaL_checkoption,
	(TMFPTR) luaL_loadstring,
	(TMFPTR) luaL_findtable,

	(TMFPTR) luaL_buffinit,
	(TMFPTR) _luaL_prepbuffer,
	(TMFPTR) _luaL_addlstring,
	(TMFPTR) _luaL_addstring,
	(TMFPTR) _luaL_addvalue,
	(TMFPTR) _luaL_pushresult,
};

/*****************************************************************************/
/*
**	Module instance open
*/

static const luaL_Reg lualibs[] =
{
	{"", luaopen_base},
	{LUA_LOADLIBNAME, luaopen_package},
	{LUA_TABLIBNAME, luaopen_table},
#if defined(TEKLIB_LUA_STDLIBS) && !defined(TSYS_PS2) && !defined(TSYS_AMIGA) && !defined(TSYS_MORPHOS)
	{LUA_IOLIBNAME, luaopen_io},
	{LUA_OSLIBNAME, luaopen_os},
#endif
	{LUA_STRLIBNAME, luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
	{LUA_DBLIBNAME, luaopen_debug},
	{"tek", luaopen_tek},
	{TNULL, TNULL}
};

static LUACFUNC int
luainitfunc(lua_State *L)
{
	const luaL_reg *lib = lualibs;
	for (; lib->func; lib++)
	{
		TDBPRINTF(TDB_INFO,("open internal lua library: %s\n", lib->name));
		lib->func(L);		/* open */
		lua_settop(L, 0);	/* discard any results */
	}
	return 0;
}

static void
closeall(TLUABASE *mod)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TCloseModule((struct TModule *) mod->tml_IOBase);
	TCloseModule((struct TModule *) mod->tml_UtilBase);
}

static LUACFUNC TAPTR
tek_alloc(TAPTR ud, TAPTR ptr, size_t osize, size_t nsize)
{
	TLUABASE *mod = ud;
	struct TExecBase *TExecBase = TGetExecBase(mod);
	if (nsize == 0)
	{
		TFree(ptr);
		return TNULL;
	}
	if (ptr)
		return TRealloc(ptr, nsize);
	return TAlloc(mod->tml_MM, nsize);
}

static TAPTR
luamod_open(TLUABASE *mod, TTAGITEM *tags)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	LUA_USERSTATE *inst = TNULL;

	TLock(mod->tml_Lock);

	if (mod->tml_RefCount == 0)
	{
		mod->tml_UtilBase = (struct TUtilBase *) TOpenModule("util", 0, TNULL);
		mod->tml_IOBase = (struct TIOBase *) TOpenModule("io", 0, TNULL);
		if (mod->tml_IOBase)
		{
			mod->tml_StdIn = TIOInputFH(mod->tml_IOBase);
			mod->tml_StdOut = TIOOutputFH(mod->tml_IOBase);
			mod->tml_StdErr = TIOErrorFH(mod->tml_IOBase);
		}
	}

	if (mod->tml_UtilBase && mod->tml_IOBase)
	{
		LUA_USERSTATE *L = (LUA_USERSTATE *)
			lua_newstate((lua_Alloc) tek_alloc, mod);
		if (L)
		{
			inst = L - 1;

			TCopyMem(mod, inst, sizeof(struct TModule));
			inst->State.tlu_Module.tmd_PosSize =
				TGetSize(inst - 1);
			inst->State.tlu_Module.tmd_NegSize = 0;
			inst->State.tlu_Module.tmd_InitTask =
				TFindTask(TNULL);

			/* open Lua libraries */
			lua_cpcall((lua_State *) L, luainitfunc, TNULL);
			inst->State.tlu_UserData = TNULL;
			inst++;
		}
	}

	if (inst)
		mod->tml_RefCount++;
	else
		closeall(mod);

	TUnlock(mod->tml_Lock);

	return inst;
}

static void
luamod_close(LUA_USERSTATE *inst)
{
	TLUABASE *mod = (TLUABASE *) inst->State.tlu_Module.tmd_ModSuper;
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TAPTR L = inst + 1;

	TLock(mod->tml_Lock);

	lua_close(L);

	if (--mod->tml_RefCount == 0)
		closeall(mod);

	TUnlock(mod->tml_Lock);
}
