
#ifndef _TEK_IFACE_LUAEXT_H
#define _TEK_IFACE_LUAEXT_H

/*
**	$Id: luaext.h,v 1.2 2006/09/10 14:50:02 tmueller Exp $
**	teklib/tek/iface/luaext.h - Lua extension module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/mod/lua.h>

typedef struct TLuaExtIFace
{
	struct TInterface IFace;
	LUACFUNC TINT (*Open)(lua_State *L);

} TLUAEXTIFACE;

#endif /* _TEK_IFACE_LUAEXT_H */
