
#ifndef _TEK_MOD_LUA_TEK_MOD_H
#define _TEK_MOD_LUA_TEK_MOD_H

/*
**	$Id: lua_tek_mod.h,v 1.3 2006/09/10 14:44:57 tmueller Exp $
**	teklib/src/lua/modules/tek/lua_tek_mod.h - TEKlib extension module
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>
#include <tek/proto/visual.h>
#include <tek/iface/luaext.h>

/*****************************************************************************/

#ifndef EXPORT
#define EXPORT	LUACFUNC
#endif

#ifndef LOCAL
#define LOCAL
#endif

#if defined(TEKLUA)

/*****************************************************************************/
/*
**	Standalone Lua module
*/

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

struct LuaTEKBase;

extern TAPTR TExecBase;
extern TAPTR TTimeBase;
extern TAPTR TUtilBase;
extern TAPTR TimeRequest;

#else

/*****************************************************************************/
/*
**	TEKlib module
*/

#include <tek/mod/lua.h>
#include <tek/proto/lua.h>

struct LuaTEKBase
{ 
	struct TModule mod;
	TAPTR utilbase;
	TAPTR timebase;
	TAPTR lock;
	TUINT refcount;
	TAPTR timereq;
	struct TLuaExtIFace luaextiface;
};

#define TExecBase TGetExecBase(mod)
#define TUtilBase mod->utilbase
#define TTimeBase mod->timebase
#define TimeRequest mod->timereq

#endif

/*****************************************************************************/

EXPORT TINT luaopen_tek_vis(lua_State *L);
LOCAL TINT luavis_init(lua_State *L, TAPTR mod, TSTRPTR name);

#endif
