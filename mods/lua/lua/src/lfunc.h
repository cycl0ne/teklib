/*
** $Id: lfunc.h,v 1.1 2005/05/08 19:15:45 tmueller Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/

#ifndef lfunc_h
#define lfunc_h


#include "lobject.h"


#define sizeCclosure(n)	(cast(int, sizeof(CClosure)) + \
                         cast(int, sizeof(TValue)*((n)-1)))

#define sizeLclosure(n)	(cast(int, sizeof(LClosure)) + \
                         cast(int, sizeof(TValue *)*((n)-1)))


Proto *luaF_newproto (lua_State *L);
Closure *luaF_newCclosure (lua_State *L, int nelems, Table *e);
Closure *luaF_newLclosure (lua_State *L, int nelems, Table *e);
UpVal *luaF_newupval (lua_State *L);
UpVal *luaF_findupval (lua_State *L, StkId level);
void luaF_close (lua_State *L, StkId level);
void luaF_freeproto (lua_State *L, Proto *f);
void luaF_freeclosure (lua_State *L, Closure *c);
void luaF_freeupval (lua_State *L, UpVal *uv);

const char *luaF_getlocalname (const Proto *func, int local_number, int pc);


#endif
