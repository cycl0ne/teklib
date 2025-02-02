/*
** $Id: lapi.h,v 1.1 2005/05/08 19:15:45 tmueller Exp $
** Auxiliary functions from Lua API
** See Copyright Notice in lua.h
*/

#ifndef lapi_h
#define lapi_h


#include "lobject.h"


void luaA_pushobject (lua_State *L, const TValue *o);

#endif
