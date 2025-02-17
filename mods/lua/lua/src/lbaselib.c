/*
** $Id: lbaselib.c,v 1.1 2005/05/08 19:15:45 tmueller Exp $
** Basic library
** See Copyright Notice in lua.h
*/



#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define lbaselib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"




/*
** If your system does not support `stdout', you can just remove this function.
** If you need, you can define your own `print' function, following this
** model but changing `fputs' to put the strings at a proper place
** (a console window or a log file, for instance).
*/
static LUACFUNC int luaB_print (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      return luaL_error(L, "`tostring' must return a string to `print'");
    if (i>1) TIOFPutS(TIOBase, ((TMOD_LUA *) LUASUPER(L))->tml_StdOut, "\t");
    TIOFPutS(TIOBase, ((TMOD_LUA *) LUASUPER(L))->tml_StdOut, (TSTRPTR) s);
    lua_pop(L, 1);  /* pop result */
  }
  TIOFPutS(TIOBase, ((TMOD_LUA *) LUASUPER(L))->tml_StdOut, "\n");
  return 0;
}


static LUACFUNC int luaB_tonumber (lua_State *L) {
  int base = luaL_optint(L, 2, 10);
  if (base == 10) {  /* standard conversion */
    luaL_checkany(L, 1);
    if (lua_isnumber(L, 1)) {
      lua_pushnumber(L, lua_tonumber(L, 1));
      return 1;
    }
  }
  else {
    const char *s1 = luaL_checkstring(L, 1);
    char *s2;
    unsigned long n;
    luaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
    n = strtoul(s1, &s2, base);
    if (s1 != s2) {  /* at least one valid digit? */
      while (isspace((unsigned char)(*s2))) s2++;  /* skip trailing spaces */
      if (*s2 == '\0') {  /* no invalid trailing characters? */
        lua_pushnumber(L, (lua_Number)n);
        return 1;
      }
    }
  }
  lua_pushnil(L);  /* else not a number */
  return 1;
}


static LUACFUNC int luaB_error (lua_State *L) {
  int level = luaL_optint(L, 2, 1);
  lua_settop(L, 1);
  if (lua_isstring(L, 1) && level > 0) {  /* add extra information? */
    luaL_where(L, level);
    lua_pushvalue(L, 1);
    lua_concat(L, 2);
  }
  return lua_error(L);
}


static LUACFUNC int luaB_getmetatable (lua_State *L) {
  luaL_checkany(L, 1);
  if (!lua_getmetatable(L, 1)) {
    lua_pushnil(L);
    return 1;  /* no metatable */
  }
  luaL_getmetafield(L, 1, "__metatable");
  return 1;  /* returns either __metatable field (if present) or metatable */
}


static LUACFUNC int luaB_setmetatable (lua_State *L) {
  int t = lua_type(L, 2);
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_argcheck(L, t == LUA_TNIL || t == LUA_TTABLE, 2,
                    "nil or table expected");
  if (luaL_getmetafield(L, 1, "__metatable"))
    luaL_error(L, "cannot change a protected metatable");
  lua_settop(L, 2);
  lua_setmetatable(L, 1);
  return 1;
}


static void getfunc (lua_State *L) {
  if (lua_isfunction(L, 1)) lua_pushvalue(L, 1);
  else {
    lua_Debug ar;
    int level = luaL_optint(L, 1, 1);
    luaL_argcheck(L, level >= 0, 1, "level must be non-negative");
    if (lua_getstack(L, level, &ar) == 0)
      luaL_argerror(L, 1, "invalid level");
    lua_getinfo(L, "f", &ar);
    if (lua_isnil(L, -1))
      luaL_error(L, "no function environment for tail call at level %d",
                    level);
  }
}


static LUACFUNC int luaB_getfenv (lua_State *L) {
  getfunc(L);
  if (lua_iscfunction(L, -1))  /* is a C function? */
    lua_pushvalue(L, LUA_GLOBALSINDEX);  /* return the global env. */
  else
    lua_getfenv(L, -1);
  return 1;
}


static LUACFUNC int luaB_setfenv (lua_State *L) {
  luaL_checktype(L, 2, LUA_TTABLE);
  getfunc(L);
  lua_pushvalue(L, 2);
  if (lua_isnumber(L, 1) && lua_tonumber(L, 1) == 0) {
    lua_replace(L, LUA_GLOBALSINDEX);
    return 0;
  }
  else if (lua_iscfunction(L, -2) || lua_setfenv(L, -2) == 0)
    luaL_error(L, "`setfenv' cannot change environment of given object");
  return 1;
}


static LUACFUNC int luaB_rawequal (lua_State *L) {
  luaL_checkany(L, 1);
  luaL_checkany(L, 2);
  lua_pushboolean(L, lua_rawequal(L, 1, 2));
  return 1;
}


static LUACFUNC int luaB_rawget (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  lua_rawget(L, 1);
  return 1;
}

static LUACFUNC int luaB_rawset (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  luaL_checkany(L, 3);
  lua_rawset(L, 1);
  return 1;
}


static LUACFUNC int luaB_gcinfo (lua_State *L) {
  lua_pushinteger(L, lua_getgccount(L));
  return 1;
}


static LUACFUNC int luaB_collectgarbage (lua_State *L) {
  static const char *const opts[] = {"stop", "restart", "collect",
    "count", "step", "setpace", "setstepmul", NULL};
  static const int optsnum[] = {LUA_GCSTOP, LUA_GCRESTART, LUA_GCCOLLECT,
    LUA_GCCOUNT, LUA_GCSTEP, LUA_GCSETPACE, LUA_GCSETSTEPMUL};
  int o = luaL_findstring(luaL_optstring(L, 1, "collect"), opts);
  int ex = luaL_optinteger(L, 2, 0);
  luaL_argcheck(L, o >= 0, 1, "invalid option");
  lua_pushinteger(L, lua_gc(L, optsnum[o], ex));
  return 1;
}


static LUACFUNC int luaB_type (lua_State *L) {
  luaL_checkany(L, 1);
  lua_pushstring(L, luaL_typename(L, 1));
  return 1;
}


static LUACFUNC int luaB_next (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (lua_next(L, 1))
    return 2;
  else {
    lua_pushnil(L);
    return 1;
  }
}


static LUACFUNC int luaB_pairs (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_pushvalue(L, lua_upvalueindex(1));  /* return generator, */
  lua_pushvalue(L, 1);  /* state, */
  lua_pushnil(L);  /* and initial value */
  return 3;
}


static LUACFUNC int ipairsaux (lua_State *L) {
  int i = luaL_checkint(L, 2);
  luaL_checktype(L, 1, LUA_TTABLE);
  i++;  /* next value */
  lua_pushinteger(L, i);
  lua_rawgeti(L, 1, i);
  return (lua_isnil(L, -1)) ? 0 : 2;
}


static LUACFUNC int luaB_ipairs (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_pushvalue(L, lua_upvalueindex(1));  /* return generator, */
  lua_pushvalue(L, 1);  /* state, */
  lua_pushinteger(L, LUA_FIRSTINDEX - 1);  /* and initial value */
  return 3;
}


static int load_aux (lua_State *L, int status) {
  if (status == 0)  /* OK? */
    return 1;
  else {
    lua_pushnil(L);
    lua_insert(L, -2);  /* put before error message */
    return 2;  /* return nil plus error message */
  }
}


static LUACFUNC int luaB_loadstring (lua_State *L) {
  size_t l;
  const char *s = luaL_checklstring(L, 1, &l);
  const char *chunkname = luaL_optstring(L, 2, s);
  return load_aux(L, luaL_loadbuffer(L, s, l, chunkname));
}


static LUACFUNC int luaB_loadfile (lua_State *L) {
  const char *fname = luaL_optstring(L, 1, NULL);
  const char *path = luaL_optstring(L, 2, NULL);
  int status;
  if (path == NULL)
    status = luaL_loadfile(L, fname);
  else {
    fname = luaL_searchpath(L, fname, path);
    status = (fname) ? luaL_loadfile(L, fname) : 1;
  }
  return load_aux(L, status);
}


/*
** Reader for generic `load' function: `lua_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader (lua_State *L, void *ud, size_t *size) {
  (void)ud;  /* to avoid warnings */
  luaL_checkstack(L, 2, "too many nested functions");
  lua_pushvalue(L, 1);  /* get function */
  lua_call(L, 0, 1);  /* call it */
  if (lua_isnil(L, -1)) {
    *size = 0;
    return NULL;
  }
  else if (lua_isstring(L, -1)) {
    const char *res;
    lua_replace(L, 3);  /* save string in a reserved stack slot */
    res = lua_tostring(L, 3);
    *size = lua_strlen(L, 3);
    return res;
  }
  else luaL_error(L, "reader function must return a string");
  return NULL;  /* to avoid warnings */
}


static LUACFUNC int luaB_load (lua_State *L) {
  int status;
  const char *cname = luaL_optstring(L, 2, "=(load)");
  luaL_checktype(L, 1, LUA_TFUNCTION);
  lua_settop(L, 3);  /* function, eventual name, plus one reserved slot */
  status = lua_load(L, generic_reader, NULL, cname);
  return load_aux(L, status);
}


static LUACFUNC int luaB_dofile (lua_State *L) {
  const char *fname = luaL_optstring(L, 1, NULL);
  int n = lua_gettop(L);
  if (luaL_loadfile(L, fname) != 0) lua_error(L);
  lua_call(L, 0, LUA_MULTRET);
  return lua_gettop(L) - n;
}


static LUACFUNC int luaB_assert (lua_State *L) {
  luaL_checkany(L, 1);
  if (!lua_toboolean(L, 1))
    return luaL_error(L, "%s", luaL_optstring(L, 2, "assertion failed!"));
  return lua_gettop(L);
}


static LUACFUNC int luaB_unpack (lua_State *L) {
  int i = luaL_optint(L, 2, LUA_FIRSTINDEX);
  int e = luaL_optint(L, 3, -1);
  int n;
  luaL_checktype(L, 1, LUA_TTABLE);
  if (e == -1)
    e = luaL_getn(L, 1) + LUA_FIRSTINDEX - 1;
  n = e - i + 1;  /* number of elements */
  if (n <= 0) return 0;  /* empty range */
  luaL_checkstack(L, n, "table too big to unpack");
  for (; i<=e; i++)  /* push arg[i...e] */
    lua_rawgeti(L, 1, i);
  return n;
}


static LUACFUNC int luaB_select (lua_State *L) {
  int n = lua_gettop(L);
  if (lua_type(L, 1) == LUA_TSTRING && *lua_tostring(L, 1) == '#') {
    lua_pushinteger(L, n-1);
    return 1;
  }
  else {
    int i = luaL_checkint(L, 1);
    if (i <= 0) i = 1;
    else if (i >= n) i = n;
    return n - i;
  }
}


static LUACFUNC int luaB_pcall (lua_State *L) {
  int status;
  luaL_checkany(L, 1);
  status = lua_pcall(L, lua_gettop(L) - 1, LUA_MULTRET, 0);
  lua_pushboolean(L, (status == 0));
  lua_insert(L, 1);
  return lua_gettop(L);  /* return status + all results */
}


static LUACFUNC int luaB_xpcall (lua_State *L) {
  int status;
  luaL_checkany(L, 2);
  lua_settop(L, 2);
  lua_insert(L, 1);  /* put error function under function to be called */
  status = lua_pcall(L, 0, LUA_MULTRET, 1);
  lua_pushboolean(L, (status == 0));
  lua_replace(L, 1);
  return lua_gettop(L);  /* return status + all results */
}


static LUACFUNC int luaB_tostring (lua_State *L) {
  luaL_checkany(L, 1);
  if (luaL_callmeta(L, 1, "__tostring"))  /* is there a metafield? */
    return 1;  /* use its value */
  switch (lua_type(L, 1)) {
    case LUA_TNUMBER:
      lua_pushstring(L, lua_tostring(L, 1));
      break;
    case LUA_TSTRING:
      lua_pushvalue(L, 1);
      break;
    case LUA_TBOOLEAN:
      lua_pushstring(L, (lua_toboolean(L, 1) ? "true" : "false"));
      break;
    case LUA_TNIL:
      lua_pushliteral(L, "nil");
      break;
    default:
      lua_pushfstring(L, "%s: %p", luaL_typename(L, 1), lua_topointer(L, 1));
      break;
  }
  return 1;
}


static LUACFUNC int luaB_newproxy (lua_State *L) {
  lua_settop(L, 1);
  lua_newuserdata(L, 0);  /* create proxy */
  if (lua_toboolean(L, 1) == 0)
    return 1;  /* no metatable */
  else if (lua_isboolean(L, 1)) {
    lua_newtable(L);  /* create a new metatable `m' ... */
    lua_pushvalue(L, -1);  /* ... and mark `m' as a valid metatable */
    lua_pushboolean(L, 1);
    lua_rawset(L, lua_upvalueindex(1));  /* weaktable[m] = true */
  }
  else {
    int validproxy = 0;  /* to check if weaktable[metatable(u)] == true */
    if (lua_getmetatable(L, 1)) {
      lua_rawget(L, lua_upvalueindex(1));
      validproxy = lua_toboolean(L, -1);
      lua_pop(L, 1);  /* remove value */
    }
    luaL_argcheck(L, validproxy, 1, "boolean or proxy expected");
    lua_getmetatable(L, 1);  /* metatable is valid; get it */
  }
  lua_setmetatable(L, 2);
  return 1;
}


static const luaL_reg base_funcs[] = {
  {"error", luaB_error},
  {"getmetatable", luaB_getmetatable},
  {"setmetatable", luaB_setmetatable},
  {"getfenv", luaB_getfenv},
  {"setfenv", luaB_setfenv},
  {"next", luaB_next},
  {"print", luaB_print},
  {"tonumber", luaB_tonumber},
  {"tostring", luaB_tostring},
  {"type", luaB_type},
  {"assert", luaB_assert},
  {"unpack", luaB_unpack},
  {"select", luaB_select},
  {"rawequal", luaB_rawequal},
  {"rawget", luaB_rawget},
  {"rawset", luaB_rawset},
  {"pcall", luaB_pcall},
  {"xpcall", luaB_xpcall},
  {"collectgarbage", luaB_collectgarbage},
  {"gcinfo", luaB_gcinfo},
  {"loadfile", luaB_loadfile},
  {"dofile", luaB_dofile},
  {"loadstring", luaB_loadstring},
  {"load", luaB_load},
  {NULL, NULL}
};


/*
** {======================================================
** Coroutine library
** =======================================================
*/

static int auxresume (lua_State *L, lua_State *co, int narg) {
  int status;
  if (!lua_checkstack(co, narg))
    luaL_error(L, "too many arguments to resume");
  if (lua_status(co) == 0 && lua_gettop(co) == 0) {
    lua_pushliteral(L, "cannot resume dead coroutine");
    return -1;  /* error flag */
  }
  lua_xmove(L, co, narg);
  status = lua_resume(co, narg);
  if (status == 0 || status == LUA_YIELD) {
    int nres = lua_gettop(co);
    if (!lua_checkstack(L, nres))
      luaL_error(L, "too many results to resume");
    lua_xmove(co, L, nres);  /* move yielded values */
    return nres;
  }
  else {
    lua_xmove(co, L, 1);  /* move error message */
    return -1;  /* error flag */
  }
}


static LUACFUNC int luaB_coresume (lua_State *L) {
  lua_State *co = lua_tothread(L, 1);
  int r;
  luaL_argcheck(L, co, 1, "coroutine expected");
  r = auxresume(L, co, lua_gettop(L) - 1);
  if (r < 0) {
    lua_pushboolean(L, 0);
    lua_insert(L, -2);
    return 2;  /* return false + error message */
  }
  else {
    lua_pushboolean(L, 1);
    lua_insert(L, -(r + 1));
    return r + 1;  /* return true + `resume' returns */
  }
}


static LUACFUNC int luaB_auxwrap (lua_State *L) {
  lua_State *co = lua_tothread(L, lua_upvalueindex(1));
  int r = auxresume(L, co, lua_gettop(L));
  if (r < 0) {
    if (lua_isstring(L, -1)) {  /* error object is a string? */
      luaL_where(L, 1);  /* add extra info */
      lua_insert(L, -2);
      lua_concat(L, 2);
    }
    lua_error(L);  /* propagate error */
  }
  return r;
}


static LUACFUNC int luaB_cocreate (lua_State *L) {
  lua_State *NL = lua_newthread(L);
  luaL_argcheck(L, lua_isfunction(L, 1) && !lua_iscfunction(L, 1), 1,
    "Lua function expected");
  lua_pushvalue(L, 1);  /* move function to top */
  lua_xmove(L, NL, 1);  /* move function from L to NL */
  return 1;
}


static LUACFUNC int luaB_cowrap (lua_State *L) {
  luaB_cocreate(L);
  lua_pushcclosure(L, luaB_auxwrap, 1);
  return 1;
}


static LUACFUNC int luaB_yield (lua_State *L) {
  return lua_yield(L, lua_gettop(L));
}


static LUACFUNC int luaB_costatus (lua_State *L) {
  lua_State *co = lua_tothread(L, 1);
  luaL_argcheck(L, co, 1, "coroutine expected");
  if (L == co) lua_pushliteral(L, "running");
  else {
    switch (lua_status(co)) {
      case LUA_YIELD:
        lua_pushliteral(L, "suspended");
        break;
      case 0: {
        lua_Debug ar;
        if (lua_getstack(co, 0, &ar) > 0)  /* does it have frames? */
          lua_pushliteral(L, "normal");  /* it is running */
        else if (lua_gettop(co) == 0)
            lua_pushliteral(L, "dead");
        else
          lua_pushliteral(L, "suspended");  /* initial state */
        break;  
      }
      default:  /* some error occured */
        lua_pushliteral(L, "dead");
        break;
    }
  }
  return 1;
}


static LUACFUNC int luaB_corunning (lua_State *L) {
  if (lua_pushthread(L))
    return 0;  /* main thread is not a coroutine */
  else
    return 1;
}


static const luaL_reg co_funcs[] = {
  {"create", luaB_cocreate},
  {"wrap", luaB_cowrap},
  {"resume", luaB_coresume},
  {"yield", luaB_yield},
  {"status", luaB_costatus},
  {"running", luaB_corunning},
  {NULL, NULL}
};

/* }====================================================== */


static void auxopen (lua_State *L, const char *name,
                     lua_CFunction f, lua_CFunction u) {
  lua_pushcfunction(L, u);
  lua_pushcclosure(L, f, 1);
  lua_setfield(L, -2, name);
}


static void base_open (lua_State *L) {
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  luaL_openlib(L, NULL, base_funcs, 0);  /* open lib into global table */
  lua_pushliteral(L, LUA_VERSION);
  lua_setglobal(L, "_VERSION");  /* set global _VERSION */
  /* `ipairs' and `pairs' need auxliliary functions as upvalues */
  auxopen(L, "ipairs", luaB_ipairs, ipairsaux);
  auxopen(L, "pairs", luaB_pairs, luaB_next);
  /* `newproxy' needs a weaktable as upvalue */
  lua_newtable(L);  /* new table `w' */
  lua_pushvalue(L, -1);  /* `w' will be its own metatable */
  lua_setmetatable(L, -2);
  lua_pushliteral(L, "kv");
  lua_setfield(L, -2, "__mode");  /* metatable(w).__mode = "kv" */
  lua_pushcclosure(L, luaB_newproxy, 1);
  lua_setglobal(L, "newproxy");  /* set global `newproxy' */
  /* create register._LOADED to track loaded modules */
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "_LOADED");
  /* set global _G */
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  lua_setglobal(L, "_G");
}


LUALIB_API int luaopen_base (lua_State *L) {
  base_open(L);
  luaL_openlib(L, LUA_COLIBNAME, co_funcs, 0);
  return 2;
}

