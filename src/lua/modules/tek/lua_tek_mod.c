
/*
**	$Id: lua_tek_mod.c,v 1.5 2006/11/11 14:19:10 tmueller Exp $
**	teklib/src/lua/modules/tek/lua_tek_mod.c - TEKlib extension module
**
**	luaopen_tek_vis (TEKlib module/interface startup)
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "lua_tek_mod.h"

#define LUAVIS_VERSION		0
#define LUAVIS_REVISION		2

static THOOKENTRY TTAG luatek_dispatch(struct THook *hook, TAPTR obj,
	TTAG msg);
EXPORT struct LuaTEKBase *luatek_open(struct LuaTEKBase *mod, TTAGITEM *tags);
EXPORT TVOID luatek_close(struct LuaTEKBase *mod);
static TAPTR luatek_queryinterface(struct LuaTEKBase *mod, TSTRPTR name,
	TUINT16 version, TTAGITEM *tags);

/*****************************************************************************/
/*
**	TEKlib module setup
*/

TMODENTRY TUINT
tek_init_lua5_mod_tek(struct TTask *task, struct LuaTEKBase *mod, TUINT16 version,
	TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return 0; /* negative size */
		if (version <= LUAVIS_VERSION)
			return sizeof(struct LuaTEKBase); /* positive size */
		return 0;
	}

	mod->lock = TCreateLock(TNULL);
	if (mod->lock)
	{
		mod->mod.tmd_Version = LUAVIS_VERSION;
		mod->mod.tmd_Revision = LUAVIS_REVISION;

		mod->mod.tmd_Handle.thn_Hook.thk_Entry = luatek_dispatch;
		mod->mod.tmd_Flags |= TMODF_OPENCLOSE | TMODF_QUERYIFACE;

		mod->luaextiface.IFace.tif_Name = "luaopen_tek_vis";
		mod->luaextiface.IFace.tif_Module = (struct TModule *) mod;
		mod->luaextiface.IFace.tif_Version = 1;
		mod->luaextiface.Open = &luaopen_tek_vis;

		return TTRUE;
	}

	return TFALSE;
}

static THOOKENTRY TTAG
luatek_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct LuaTEKBase *mod = (struct LuaTEKBase *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			TDestroy(mod->lock);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) luatek_open(mod, obj);
		case TMSG_CLOSEMODULE:
			luatek_close(obj);
			break;
		case TMSG_QUERYIFACE:
		{
			struct TIFaceQuery *ifq = obj;
			return (TTAG) luatek_queryinterface(mod, ifq->tfq_Name,
				ifq->tfq_Version, ifq->tfq_Tags);
		}
		case TMSG_DROPIFACE:
			break;
	}
	return 0;
}

EXPORT struct LuaTEKBase *
luatek_open(struct LuaTEKBase *mod, TTAGITEM *tags)
{
	struct LuaTEKBase *res = TNULL;
	TBOOL success = TTRUE;
	TLock(mod->lock);
	if (++mod->refcount == 1)
	{
		success = TFALSE;
		mod->timebase = TOpenModule("time", 0, TNULL);
		if (mod->timebase)
		{
			mod->timereq = TAllocTimeRequest(TNULL);
			mod->utilbase = TOpenModule("util", 0, TNULL);
			if (mod->timereq && mod->timereq)
				success = TTRUE;
			else
			{
				TCloseModule(mod->utilbase);
				TFreeTimeRequest(mod->timereq);
				TCloseModule(mod->timebase);
			}
		}
	}
	if (success)
		res = mod;
	TUnlock(mod->lock);
	return res;
}

EXPORT TVOID
luatek_close(struct LuaTEKBase *mod)
{
	TLock(mod->lock);
	if (--mod->refcount == 0)
	{
		TCloseModule(mod->utilbase);
		TFreeTimeRequest(mod->timereq);
		TCloseModule(mod->timebase);
	}
	TUnlock(mod->lock);
}

static TAPTR
luatek_queryinterface(struct LuaTEKBase *mod, TSTRPTR name, TUINT16 version, TTAGITEM *tags)
{
	if (TStrEqual(name, "luaopen_tek_vis") && version >= 1)
		return &mod->luaextiface;
	return TNULL;
}

/*****************************************************************************/
/*
**	determine modbase from classname
*/

static TAPTR
getmodbase(lua_State *L, TSTRPTR name)
{
	TAPTR mod = TNULL;
	TSTRPTR p = name;
	while (*p && *p != '.') p++;
	lua_pushstring(L, "LOADLIB: lua5_mod_");
	lua_pushlstring(L, name, p - name);
	lua_concat(L, 2);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if (!lua_isnil(L, -1))
		mod = *(void **) lua_touserdata(L, -1);
	lua_pop(L, 1);
	return mod;
}

/*
**	open
*/

EXPORT TINT
luaopen_tek_vis(lua_State *L)
{
	TSTRPTR name = (TSTRPTR) luaL_checkstring(L, 1);
	TAPTR mod = getmodbase(L, name);
	return luavis_init(L, mod, name);
}
