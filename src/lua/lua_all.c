
/*
**	Stub to build module from single source
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#ifndef EXPORT
#define EXPORT static TMODAPI
#endif

#ifndef LOCAL
#define LOCAL static
#endif

#define luaall_c
#define LUA_CORE

#ifndef TLIBAPI
#define TLIBAPI static
#endif

#include "../teklib/teklib.c"
#include "../teklib/string.c"

#include "lua_mod.c"
#include "lua_teklib.c"
#include "luasrc/lapi.c"
#include "luasrc/lcode.c"
#include "luasrc/ldebug.c"
#include "luasrc/ldo.c"
#include "luasrc/ldump.c"
#include "luasrc/lfunc.c"
#include "luasrc/lgc.c"
#include "luasrc/llex.c"
#include "luasrc/lmem.c"
#include "luasrc/lobject.c"
#include "luasrc/lopcodes.c"
#include "luasrc/lparser.c"
#include "luasrc/lstate.c"
#include "luasrc/lstring.c"
#include "luasrc/ltable.c"
#include "luasrc/ltm.c"
#include "luasrc/lundump.c"
#include "luasrc/lvm.c"
#include "luasrc/lzio.c"

#include "luasrc/lmathlib.c"
#include "luasrc/lstrlib.c"
#include "luasrc/lbaselib.c"
#include "luasrc/ltablib.c"
#include "luasrc/lauxlib.c"
#include "luasrc/loadlib.c"
#include "luasrc/ldblib.c"

#if defined(TEKLIB_LUA_STDLIBS) && !defined(TSYS_PS2) && !defined(TSYS_AMIGA) && !defined(TSYS_MORPHOS)
#include "luasrc/loslib.c"
#include "luasrc/liolib.c"
#endif
