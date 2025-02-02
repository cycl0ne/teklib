
/*
**	$Id: lua_mod.c,v 1.9 2005/08/16 17:57:05 tmueller Exp $
**	teklib/mods/lua/lua_mod.c - LUA module API implementation
**
**	module memory layout:

	  ____________          
	 |            |
	 |  function  |
	 |    table   |       
	_|____________|_ <---.   _ _____________ _ module instance
	 |            |      |    |             |
	 |           -+------^----+-   module   |   LUA_USERSTATE
	 |  TMOD_LUA  |  modsuper |    header   |
	 | (modsuper) |  backptr  |_____________|_ L 
	 |            |           |             |
	 |            |           |             |
     |____________|           |  LUA state  |
	                          |             |
	                          |_____________|

**	this layout solves these problems:
**	* changes in the LUA state structure do not affect binary
**	  compatibility
**  * the LUA state itself is the module base, API functions
**	  need no additional modulebase argument
**
**	caveats:
**	* only one function table for all instances; double indirect calls
**	* TCloseModule() must be applied to ((struct TLuaModule *) L) - 1
*/

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "lua_mod.h"

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/time.h>
#include <tek/proto/io.h>

#define MOD_VERSION		0
#define MOD_REVISION	9
#define MOD_NUMVECTORS	95

static TCALLBACK TAPTR mod_open(TMOD_LUA *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(LUA_USERSTATE *inst, TAPTR task);
static TCALLBACK TVOID mod_destroy(TMOD_LUA *mod);

static const TAPTR vectors[MOD_NUMVECTORS];

/*****************************************************************************/
/* 
**	Module init
*/

TMODENTRY TUINT 
tek_init_lua5(TAPTR selftask, TMOD_LUA *mod, TUINT16 version, TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * MOD_NUMVECTORS;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_LUA);				/* positive size */

		return 0;
	}

	mod->tml_ExecBase = TGetExecBase(mod);
	mod->tml_Lock = TExecCreateLock(mod->tml_ExecBase, TNULL);
	if (mod->tml_Lock)
	{
		#ifdef TDEBUG
		mod->tml_MMU = TExecCreateMMU(mod->tml_ExecBase, TNULL, 
			TMMUT_Tracking | TMMUT_TaskSafe, TNULL);
		#endif

		mod->tml_Module.tmd_Version = MOD_VERSION;
		mod->tml_Module.tmd_Revision = MOD_REVISION;

		mod->tml_Module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->tml_Module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;
		mod->tml_Module.tmd_DestroyFunc = (TDFUNC) mod_destroy;
		
		TInitVectors(mod, (TAPTR *) vectors, MOD_NUMVECTORS);
		return TTRUE;
	}

	return 0;
}

static TCALLBACK TVOID
mod_destroy(TMOD_LUA *mod)
{
	TDestroy(mod->tml_Lock);
	TDestroy(mod->tml_MMU);
}

/*****************************************************************************/
/* 
**	Wrappers for functions that LUA implements as macros
*/

static TMODAPI size_t
_lua_strlen(lua_State *L, TINT idx)
{
	return lua_strlen(L, idx);
}

static TMODAPI TVOID
_lua_newtable(lua_State *L, TINT narr, TINT nrec)
{
	lua_createtable(L, 0, 0);
}

/*****************************************************************************/

static const TAPTR vectors[MOD_NUMVECTORS] =
{
	(TAPTR) TNULL,				/* reserved */
	(TAPTR) lua_newthread,
	(TAPTR) lua_atpanic,

	(TAPTR)	lua_call,
	(TAPTR)	lua_pcall,
	(TAPTR)	lua_cpcall,
	(TAPTR)	lua_load,
	(TAPTR) lua_dump,

	(TAPTR) lua_gettop,
	(TAPTR) lua_settop,
	(TAPTR) lua_pushvalue,
	(TAPTR) lua_remove,
	(TAPTR) lua_insert,
	(TAPTR) lua_replace,
	(TAPTR) lua_checkstack,
	(TAPTR) lua_xmove,

	(TAPTR) lua_isnumber,
	(TAPTR) lua_isstring,
	(TAPTR) lua_iscfunction,
	(TAPTR) lua_isuserdata,
	(TAPTR) lua_type,
	(TAPTR) lua_typename,

	(TAPTR) lua_equal,
	(TAPTR) lua_rawequal,
	(TAPTR) lua_lessthan,

	(TAPTR) lua_tonumber,
	(TAPTR) lua_toboolean,
	(TAPTR) lua_tostring,
	(TAPTR) _lua_strlen,
	(TAPTR) lua_tocfunction,
	(TAPTR) lua_touserdata,
	(TAPTR) lua_tothread,
	(TAPTR) lua_topointer,

	(TAPTR) lua_pushnil,
	(TAPTR) lua_pushnumber,
	(TAPTR) lua_pushlstring,
	(TAPTR) lua_pushstring,
	(TAPTR) lua_pushinteger,
	(TAPTR) lua_pushcclosure,
	(TAPTR) lua_pushboolean,
	(TAPTR) lua_pushlightuserdata,
	
	(TAPTR) lua_gettable,
	(TAPTR) lua_rawget,
	(TAPTR) lua_rawgeti,
	(TAPTR) _lua_newtable,
	(TAPTR) lua_newuserdata,
	(TAPTR) lua_getmetatable,
	(TAPTR) lua_getfenv,
	
	(TAPTR) lua_settable,
	(TAPTR) lua_rawset,
	(TAPTR) lua_rawseti,
	(TAPTR) lua_setmetatable,
	(TAPTR) lua_setfenv,

	(TAPTR) lua_yield,
	(TAPTR) lua_resume,

	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,

	(TAPTR) TNULL,
	(TAPTR) lua_error,
	(TAPTR) lua_next,
	(TAPTR) lua_concat,				/* 62 */

	/* reserved */
	
	(TAPTR) TNULL,					/* 63 */
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,					/* 69 */

	/* auxlib functions */

	(TAPTR) luaL_openlib,			/* 70 */
	(TAPTR) luaL_getmetafield,
	(TAPTR) luaL_callmeta,
	(TAPTR) luaL_typerror,
	(TAPTR) luaL_argerror,
	(TAPTR) luaL_checklstring,
	(TAPTR) luaL_optlstring,
	(TAPTR) luaL_checknumber,
	(TAPTR) luaL_optnumber,
	(TAPTR) luaL_checkstack,
	(TAPTR) luaL_checktype,
	(TAPTR) luaL_checkany,
	(TAPTR) luaL_newmetatable,
	(TAPTR) luaL_getmetatable,
	(TAPTR) luaL_checkudata,
	(TAPTR) luaL_where,
	(TAPTR) luaL_ref,
	(TAPTR) luaL_unref,
	(TAPTR) luaL_getn,
	(TAPTR) luaL_setn,
	(TAPTR) luaL_loadfile,
	(TAPTR) luaL_loadbuffer,		/* 91 */

	(TAPTR) TNULL,
	(TAPTR) luaL_gsub,
	(TAPTR) luaL_getfield,
	(TAPTR) luaL_setfield
};

/*****************************************************************************/
/* 
**	Module instance open
*/

static const luaL_reg lualibs[] =
{
	{"", luaopen_base},
	{LUA_TABLIBNAME, luaopen_table},
	{LUA_STRLIBNAME, luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
#if !defined(TSYS_PS2) && !defined(TSYS_AMIGA) && !defined(TSYS_MORPHOS)
	{LUA_OSLIBNAME, luaopen_os},
	{LUA_IOLIBNAME, luaopen_io},
#endif
	{LUA_DBLIBNAME, luaopen_debug},
	{"tek", luaopen_tek},
	{"", luaopen_loadlib},
	{TNULL, TNULL}
};

static int LUACFUNC
luainitfunc(lua_State *L)
{
	const luaL_reg *lib = lualibs;
	for (; lib->func; lib++)
	{
		tdbprintf1(5, "open internal lua library: %s\n", lib->name);
		lib->func(L);		/* open */
		lua_settop(L, 0);	/* discard any results */
	}
	return 0;
}

static TVOID 
closeall(TMOD_LUA *mod)
{
	TExecCloseModule(mod->tml_ExecBase, mod->tml_IOBase);
	TExecCloseModule(mod->tml_ExecBase, mod->tml_TimeBase);
	TExecCloseModule(mod->tml_ExecBase, mod->tml_UtilBase);
}

static LUACFUNC TAPTR
tek_alloc(TAPTR ud, TAPTR ptr, size_t osize, size_t nsize)
{
	TMOD_LUA *mod = ud;

	if (nsize == 0)
	{
		tdbprintf(2,"tek_free\n");
		TExecFree(mod->tml_ExecBase, ptr);
		return TNULL;
	}

	if (ptr)
	{
		tdbprintf(2,"tek_realloc\n");
		return TExecRealloc(mod->tml_ExecBase, ptr, nsize);
	}

	tdbprintf(2,"tek_alloc\n");
	return TExecAlloc(mod->tml_ExecBase, mod->tml_MMU, nsize);
}

static TCALLBACK TAPTR 
mod_open(TMOD_LUA *mod, TAPTR task, TTAGITEM *tags)
{
	LUA_USERSTATE *inst = TNULL;

	TExecLock(mod->tml_ExecBase, mod->tml_Lock);
	
	if (mod->tml_RefCount == 0)
	{
		mod->tml_UtilBase = 
			TExecOpenModule(mod->tml_ExecBase, "util", 0, TNULL);
		mod->tml_TimeBase =
			TExecOpenModule(mod->tml_ExecBase, "time", 0, TNULL);
		mod->tml_IOBase =
			TExecOpenModule(mod->tml_ExecBase, "io", 0, TNULL);
		if (mod->tml_IOBase)
		{
			mod->tml_StdIn = TIOInputFH(mod->tml_IOBase);
			mod->tml_StdOut = TIOOutputFH(mod->tml_IOBase);
			mod->tml_StdErr = TIOErrorFH(mod->tml_IOBase);
		}
	}
	
	if (mod->tml_UtilBase && mod->tml_IOBase && mod->tml_TimeBase)
	{
		LUA_USERSTATE *L = (LUA_USERSTATE *) lua_newstate(tek_alloc, mod);
		if (L)
		{
			TBOOL success = TFALSE;
			inst = L - 1;

			TExecCopyMem(mod->tml_ExecBase, mod, inst, sizeof(struct TModule));
			inst->tlu_Module.tmd_PosSize = 
				TExecGetSize(mod->tml_ExecBase, inst - 1);
			inst->tlu_Module.tmd_NegSize = 0;
			inst->tlu_Module.tmd_InitTask = 
				TExecFindTask(mod->tml_ExecBase, TNULL);
		
			inst->tlu_TimeRequest =
				TTimeAllocTimeRequest(mod->tml_TimeBase, TNULL);
			if (inst->tlu_TimeRequest)
			{
				/* open LUA libraries */
				lua_cpcall((lua_State *) L, luainitfunc, TNULL);
				success = TTRUE;
				inst++;
			}
			
			if (!success)
			{
				TTimeFreeTimeRequest(mod->tml_TimeBase, inst->tlu_TimeRequest);
				TFreeInstance(inst);
				inst = TNULL;
			}
		}
	}
	
	if (inst)
	{
		mod->tml_RefCount++;
	}
	else
	{
		closeall(mod);
	}

	TExecUnlock(mod->tml_ExecBase, mod->tml_Lock);

	return inst;
}

static TCALLBACK TVOID 
mod_close(LUA_USERSTATE *inst, TAPTR task)
{
	TMOD_LUA *mod = (TMOD_LUA *) inst->tlu_Module.tmd_ModSuper;
	TAPTR L = inst + 1;
	
	TExecLock(mod->tml_ExecBase, mod->tml_Lock);

	TTimeFreeTimeRequest(mod->tml_TimeBase, inst->tlu_TimeRequest);
	lua_close(L);

	if (--mod->tml_RefCount == 0)
	{
		closeall(mod);
	}

	TExecUnlock(mod->tml_ExecBase, mod->tml_Lock);
}

/*
**	Revision History
**	$Log: lua_mod.c,v $
**	Revision 1.9  2005/08/16 17:57:05  tmueller
**	tracking MMU now only used in TDEBUG case
**	
**	Revision 1.8  2005/07/03 20:34:30  sskjaeret
**	amiga/morphos fixes
**	
**	Revision 1.7  2005/06/28 23:14:36  tmueller
**	debug verbosity lowered in alloc functions
**	
**	Revision 1.6  2005/05/24 21:49:24  tmueller
**	the new 'lua' module has been renamed to 'lua5'
**	
**	Revision 1.5  2005/05/13 14:46:58  tmueller
**	dblib put pack in place
**	
**	Revision 1.4  2005/05/11 21:31:44  tmueller
**	debuglib disabled again
**	
**	Revision 1.3  2005/05/10 20:46:51  tmueller
**	removed debuglib dependency from ps2 build
**	
**	Revision 1.2  2005/05/09 22:47:31  tmueller
**	fixed teklib opening procedure, added loadlib
**	
**	Revision 1.1  2005/05/08 19:11:45  tmueller
**	added
**	
*/
