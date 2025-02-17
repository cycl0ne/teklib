/*
** $Id: lauxlib.c,v 1.2 2005/07/03 20:34:30 sskjaeret Exp $
** Auxiliary functions for building Lua libraries
** See Copyright Notice in lua.h
*/


#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* This file uses only the official API of Lua.
** Any function declared here could be written as an application function.
*/

#define lauxlib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"


/* number of prereserved references (for internal use) */
#define RESERVED_REFS	2

/* reserved references */
#define FREELIST_REF	1	/* free list of references */
#define ARRAYSIZE_REF	2	/* array sizes */


/* convert a stack index to positive */
#define abs_index(L, i)		((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : \
					lua_gettop(L) + (i) + 1)


/*
** {======================================================
** Error-report functions
** =======================================================
*/


LUALIB_API int luaL_argerror (lua_State *L, int narg, const char *extramsg) {
  lua_Debug ar;
  lua_getstack(L, 0, &ar);
  lua_getinfo(L, "n", &ar);
  if (strcmp(ar.namewhat, "method") == 0) {
    narg--;  /* do not count `self' */
    if (narg == 0)  /* error is in the self argument itself? */
      return luaL_error(L, "calling `%s' on bad self (%s)", ar.name, extramsg);
  }
  if (ar.name == NULL)
    ar.name = "?";
  return luaL_error(L, "bad argument #%d to `%s' (%s)",
                        narg, ar.name, extramsg);
}


LUALIB_API int luaL_typerror (lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}


static void tag_error (lua_State *L, int narg, int tag) {
  luaL_typerror(L, narg, lua_typename(L, tag)); 
}


LUALIB_API void luaL_where (lua_State *L, int level) {
  lua_Debug ar;
  if (lua_getstack(L, level, &ar)) {  /* check function at level */
    lua_getinfo(L, "Sl", &ar);  /* get info about it */
    if (ar.currentline > 0) {  /* is there info? */
      lua_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
      return;
    }
  }
  lua_pushliteral(L, "");  /* else, no information available... */
}


LUALIB_API int luaL_error (lua_State *L, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  luaL_where(L, 1);
  lua_pushvfstring(L, fmt, argp);
  va_end(argp);
  lua_concat(L, 2);
  return lua_error(L);
}

/* }====================================================== */


LUALIB_API int luaL_findstring (const char *name, const char *const list[]) {
  int i;
  for (i=0; list[i]; i++)
    if (strcmp(list[i], name) == 0)
      return i;
  return -1;  /* name not found */
}


LUALIB_API int luaL_newmetatable (lua_State *L, const char *tname) {
  lua_getfield(L, LUA_REGISTRYINDEX, tname);  /* get registry.name */
  if (!lua_isnil(L, -1))  /* name already in use? */
    return 0;  /* leave previous value on top, but return 0 */
  lua_pop(L, 1);
  lua_newtable(L);  /* create metatable */
  lua_pushvalue(L, -1);
  lua_setfield(L, LUA_REGISTRYINDEX, tname);  /* registry.name = metatable */
  lua_pushvalue(L, -1);
  lua_pushstring(L, tname);
  lua_rawset(L, LUA_REGISTRYINDEX);  /* registry[metatable] = name */
  return 1;
}


LUALIB_API void  luaL_getmetatable (lua_State *L, const char *tname) {
  lua_getfield(L, LUA_REGISTRYINDEX, tname);
}


LUALIB_API void *luaL_checkudata (lua_State *L, int ud, const char *tname) {
  const char *tn;
  if (!lua_getmetatable(L, ud)) return NULL;  /* no metatable? */
  lua_rawget(L, LUA_REGISTRYINDEX);  /* get registry[metatable] */
  tn = lua_tostring(L, -1);
  if (tn && (strcmp(tn, tname) == 0)) {
    lua_pop(L, 1);
    return lua_touserdata(L, ud);
  }
  else {
    lua_pop(L, 1);
    return NULL;
  }
}


LUALIB_API void luaL_checkstack (lua_State *L, int space, const char *mes) {
  if (!lua_checkstack(L, space))
    luaL_error(L, "stack overflow (%s)", mes);
}


LUALIB_API void luaL_checktype (lua_State *L, int narg, int t) {
  if (lua_type(L, narg) != t)
    tag_error(L, narg, t);
}


LUALIB_API void luaL_checkany (lua_State *L, int narg) {
  if (lua_type(L, narg) == LUA_TNONE)
    luaL_argerror(L, narg, "value expected");
}


LUALIB_API const char *luaL_checklstring (lua_State *L, int narg, size_t *len) {
  const char *s = lua_tostring(L, narg);
  if (!s) tag_error(L, narg, LUA_TSTRING);
  if (len) *len = lua_strlen(L, narg);
  return s;
}


LUALIB_API const char *luaL_optlstring (lua_State *L, int narg,
                                        const char *def, size_t *len) {
  if (lua_isnoneornil(L, narg)) {
    if (len)
      *len = (def ? strlen(def) : 0);
    return def;
  }
  else return luaL_checklstring(L, narg, len);
}


LUALIB_API lua_Number luaL_checknumber (lua_State *L, int narg) {
  lua_Number d = lua_tonumber(L, narg);
  if (d == 0 && !lua_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, LUA_TNUMBER);
  return d;
}


LUALIB_API lua_Number luaL_optnumber (lua_State *L, int narg, lua_Number def) {
  if (lua_isnoneornil(L, narg)) return def;
  else return luaL_checknumber(L, narg);
}


LUALIB_API lua_Integer luaL_checkinteger (lua_State *L, int narg) {
  lua_Integer d = lua_tointeger(L, narg);
  if (d == 0 && !lua_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, LUA_TNUMBER);
  return d;
}


LUALIB_API lua_Integer luaL_optinteger (lua_State *L, int narg,
                                        lua_Integer def) {
  if (lua_isnoneornil(L, narg)) return def;
  else return luaL_checkinteger(L, narg);
}


LUALIB_API int luaL_getmetafield (lua_State *L, int obj, const char *event) {
  if (!lua_getmetatable(L, obj))  /* no metatable? */
    return 0;
  lua_getfield(L, -1, event);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 2);  /* remove metatable and metafield */
    return 0;
  }
  else {
    lua_remove(L, -2);  /* remove only metatable */
    return 1;
  }
}


LUALIB_API int luaL_callmeta (lua_State *L, int obj, const char *event) {
  obj = abs_index(L, obj);
  if (!luaL_getmetafield(L, obj, event))  /* no metafield? */
    return 0;
  lua_pushvalue(L, obj);
  lua_call(L, 1, 1);
  return 1;
}


LUALIB_API void luaL_openlib (lua_State *L, const char *libname,
                              const luaL_reg *l, int nup) {
  if (libname) {
    /* check whether lib already exists */
    luaL_getfield(L, LUA_GLOBALSINDEX, libname);
    if (lua_isnil(L, -1)) {  /* not found? */
      lua_pop(L, 1);  /* remove previous result */
      lua_newtable(L);  /* create it */
      if (lua_getmetatable(L, LUA_GLOBALSINDEX))
        lua_setmetatable(L, -2);  /* share metatable with global table */
      /* register it with given name */
      lua_pushvalue(L, -1);
      luaL_setfield(L, LUA_GLOBALSINDEX, libname);
    }
    else if (!lua_istable(L, -1))
      luaL_error(L, "name conflict for library `%s'", libname);
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, libname);  /* _LOADED[modname] = new table */
    lua_pop(L, 1);  /* remove _LOADED table */
    lua_insert(L, -(nup+1));  /* move library table to below upvalues */
  }
  for (; l->name; l++) {
    int i;
    for (i=0; i<nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -nup);
    lua_pushcclosure(L, l->func, nup);
    lua_setfield(L, -(nup+2), l->name);
  }
  lua_pop(L, nup);  /* remove upvalues */
}



/*
** {======================================================
** getn-setn: size for arrays
** =======================================================
*/

static int checkint (lua_State *L, int topop) {
  int n = (lua_type(L, -1) == LUA_TNUMBER) ? lua_tointeger(L, -1) : -1;
  lua_pop(L, topop);
  return n;
}


static void getsizes (lua_State *L) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, ARRAYSIZE_REF);
  if (lua_isnil(L, -1)) {  /* no `size' table? */
    lua_pop(L, 1);  /* remove nil */
    lua_newtable(L);  /* create it */
    lua_pushvalue(L, -1);  /* `size' will be its own metatable */
    lua_setmetatable(L, -2);
    lua_pushliteral(L, "kv");
    lua_setfield(L, -2, "__mode");  /* metatable(N).__mode = "kv" */
    lua_pushvalue(L, -1);
    lua_rawseti(L, LUA_REGISTRYINDEX, ARRAYSIZE_REF);  /* store in register */
  }
}


LUALIB_API void luaL_setn (lua_State *L, int t, int n) {
  t = abs_index(L, t);
  getsizes(L);
  lua_pushvalue(L, t);
  lua_pushinteger(L, n);
  lua_rawset(L, -3);  /* sizes[t] = n */
  lua_pop(L, 1);  /* remove `sizes' */
}


/* find an `n' such that t[n] ~= nil and t[n+1] == nil */
static int countn (lua_State *L, int t) {
  int i = LUA_FIRSTINDEX - 1;
  int j = 2;
  /* find `i' such that i <= n < i*2 (= j) */
  for (;;) {
    lua_rawgeti(L, t, j);
    if (lua_isnil(L, -1)) break;
    lua_pop(L, 1);
    i = j;
    j = i*2;
  }
  lua_pop(L, 1);
  /* i <= n < j; do a binary search */
  while (i < j-1) {
    int m = (i+j)/2;
    lua_rawgeti(L, t, m);
    if (lua_isnil(L, -1)) j = m;
    else i = m;
    lua_pop(L, 1);
  }
  return i - LUA_FIRSTINDEX + 1;
}


LUALIB_API int luaL_getn (lua_State *L, int t) {
  int n;
  t = abs_index(L, t);
  getsizes(L);  /* try sizes[t] */
  lua_pushvalue(L, t);
  lua_rawget(L, -2);
  if ((n = checkint(L, 2)) >= 0) return n;
  lua_getfield(L, t, "n");  /* else try t.n */
  if ((n = checkint(L, 1)) >= 0) return n;
  return countn(L, t);
}

/* }====================================================== */


static const char *pushnexttemplate (lua_State *L, const char *path) {
  const char *l;
  if (*path == '\0') return NULL;  /* no more templates */
  if (*path == LUA_PATHSEP) path++;  /* skip separator */
  l = strchr(path, LUA_PATHSEP);  /* find next separator */
  if (l == NULL) l = path+strlen(path);
  lua_pushlstring(L, path, l - path);  /* template */
  return l;
}


LUALIB_API const char *luaL_gsub (lua_State *L, const char *s, const char *p,
                                                               const char *r) {
  const char *wild;
  int l = strlen(p);
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  while ((wild = strstr(s, p)) != NULL) {
    luaL_addlstring(&b, s, wild - s);  /* push prefix */
    luaL_addstring(&b, r);  /* push replacement in place of pattern */
    s = wild + l;  /* continue after `p' */
  }
  luaL_addstring(&b, s);  /* push last suffix (`n' already includes this) */
  luaL_pushresult(&b);
  return lua_tostring(L, -1);
}


static int readable (lua_State *L, const char *fname) {
  int err;
  TAPTR f = TIOOpenFile(TIOBase, (TSTRPTR) fname, TFMODE_READONLY, TNULL);
  if (f == TNULL) return 0;
  TIOFGetC(TIOBase, f);
  err = TIOGetIOErr(TIOBase);
  TIOCloseFile(TIOBase, f);
  return (err == 0);
}


LUALIB_API const char *luaL_searchpath (lua_State *L, const char *name,
                                                      const char *path) {
  const char *p = path;
  for (;;) {
    const char *fname;
    if ((p = pushnexttemplate(L, p)) == NULL) {
      lua_pushfstring(L, "no readable `%s' in path `%s'", name, path);
      return NULL;
    }
    fname = luaL_gsub(L, lua_tostring(L, -1), LUA_PATH_MARK, name);
    lua_remove(L, -2);  /* remove path template */
    if (readable(L, fname))  /* does file exist and is readable? */
      return fname;  /* return that file name */
    lua_pop(L, 1);  /* remove file name */ 
  }
}


LUALIB_API const char *luaL_getfield (lua_State *L, int idx,
                                                    const char *fname) {
  const char *e;
  lua_pushvalue(L, idx);
  while ((e = strchr(fname, '.')) != NULL) {
    lua_pushlstring(L, fname, e - fname);
    lua_rawget(L, -2);
    lua_remove(L, -2);  /* remove previous table */
    fname = e + 1;
    if (!lua_istable(L, -1)) return fname;
  }
  lua_pushstring(L, fname);
  lua_rawget(L, -2);  /* get last field */
  lua_remove(L, -2);  /* remove previous table */
  return NULL;
}


LUALIB_API const char *luaL_setfield (lua_State *L, int idx,
                                                    const char *fname) {
  const char *e;
  lua_pushvalue(L, idx);
  while ((e = strchr(fname, '.')) != NULL) {
    lua_pushlstring(L, fname, e - fname);
    lua_rawget(L, -2);
    if (lua_isnil(L, -1)) {  /* no such field? */
      lua_pop(L, 1);  /* remove this nil */
      lua_newtable(L);  /* create a new table for field */
      lua_pushlstring(L, fname, e - fname);
      lua_pushvalue(L, -2);
      lua_settable(L, -4);  /* set new table into field */
    }
    lua_remove(L, -2);  /* remove previous table */
    fname = e + 1;
    if (!lua_istable(L, -1)) {
      lua_pop(L, 2);  /* remove table and value */
      return fname;
    }
  }
  lua_pushstring(L, fname);
  lua_pushvalue(L, -3);  /* move value to the top */
  lua_rawset(L, -3);  /* set last field */
  lua_pop(L, 2);  /* remove value and table */
  return NULL;
}



/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/


#define bufflen(B)	((B)->p - (B)->buffer)
#define bufffree(B)	((size_t)(LUAL_BUFFERSIZE - bufflen(B)))

#define LIMIT	(LUA_MINSTACK/2)


static int emptybuffer (luaL_Buffer *B) {
  size_t l = bufflen(B);
  if (l == 0) return 0;  /* put nothing on stack */
  else {
    lua_pushlstring(B->L, B->buffer, l);
    B->p = B->buffer;
    B->lvl++;
    return 1;
  }
}


static void adjuststack (luaL_Buffer *B) {
  if (B->lvl > 1) {
    lua_State *L = B->L;
    int toget = 1;  /* number of levels to concat */
    size_t toplen = lua_strlen(L, -1);
    do {
      size_t l = lua_strlen(L, -(toget+1));
      if (B->lvl - toget + 1 >= LIMIT || toplen > l) {
        toplen += l;
        toget++;
      }
      else break;
    } while (toget < B->lvl);
    lua_concat(L, toget);
    B->lvl = B->lvl - toget + 1;
  }
}


LUALIB_API char *luaL_prepbuffer (luaL_Buffer *B) {
  if (emptybuffer(B))
    adjuststack(B);
  return B->buffer;
}


LUALIB_API void luaL_addlstring (luaL_Buffer *B, const char *s, size_t l) {
  while (l--)
    luaL_putchar(B, *s++);
}


LUALIB_API void luaL_addstring (luaL_Buffer *B, const char *s) {
  luaL_addlstring(B, s, strlen(s));
}


LUALIB_API void luaL_pushresult (luaL_Buffer *B) {
  emptybuffer(B);
  lua_concat(B->L, B->lvl);
  B->lvl = 1;
}


LUALIB_API void luaL_addvalue (luaL_Buffer *B) {
  lua_State *L = B->L;
  size_t vl = lua_strlen(L, -1);
  if (vl <= bufffree(B)) {  /* fit into buffer? */
    memcpy(B->p, lua_tostring(L, -1), vl);  /* put it there */
    B->p += vl;
    lua_pop(L, 1);  /* remove from stack */
  }
  else {
    if (emptybuffer(B))
      lua_insert(L, -2);  /* put buffer before new value */
    B->lvl++;  /* add new value into B stack */
    adjuststack(B);
  }
}


LUALIB_API void luaL_buffinit (lua_State *L, luaL_Buffer *B) {
  B->L = L;
  B->p = B->buffer;
  B->lvl = 0;
}

/* }====================================================== */


LUALIB_API int luaL_ref (lua_State *L, int t) {
  int ref;
  t = abs_index(L, t);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);  /* remove from stack */
    return LUA_REFNIL;  /* `nil' has a unique fixed reference */
  }
  lua_rawgeti(L, t, FREELIST_REF);  /* get first free element */
  ref = (int)lua_tointeger(L, -1);  /* ref = t[FREELIST_REF] */
  lua_pop(L, 1);  /* remove it from stack */
  if (ref != 0) {  /* any free element? */
    lua_rawgeti(L, t, ref);  /* remove it from list */
    lua_rawseti(L, t, FREELIST_REF);  /* (t[FREELIST_REF] = t[ref]) */
  }
  else {  /* no free elements */
    ref = luaL_getn(L, t);
    if (ref < RESERVED_REFS)
      ref = RESERVED_REFS;  /* skip reserved references */
    ref++;  /* create new reference */
    luaL_setn(L, t, ref);
  }
  lua_rawseti(L, t, ref);
  return ref;
}


LUALIB_API void luaL_unref (lua_State *L, int t, int ref) {
  if (ref >= 0) {
    t = abs_index(L, t);
    lua_rawgeti(L, t, FREELIST_REF);
    lua_rawseti(L, t, ref);  /* t[ref] = t[FREELIST_REF] */
    lua_pushinteger(L, ref);
    lua_rawseti(L, t, FREELIST_REF);  /* t[FREELIST_REF] = ref */
  }
}



/*
** {======================================================
** Load functions
** =======================================================
*/

typedef struct LoadF {
  int extraline;
  TAPTR f;
  char buff[LUAL_BUFFERSIZE];
} LoadF;


static LUACFUNC const char *getF (lua_State *L, void *ud, size_t *size) {
  LoadF *lf = (LoadF *)ud;
  (void)L;
  if (lf->extraline) {
    lf->extraline = 0;
    *size = 1;
    return "\n";
  }
  if (TIOFEoF(TIOBase, lf->f)) return NULL;
  *size = TIOFRead(TIOBase, lf->f, lf->buff, LUAL_BUFFERSIZE);
  return (*size > 0) ? lf->buff : NULL;
}


static int errfile (lua_State *L, const char *what, int fnameindex) {
  const char *serr = strerror(errno);
  const char *filename = lua_tostring(L, fnameindex) + 1;
  lua_pushfstring(L, "cannot %s %s: %s", what, filename, serr);
  lua_remove(L, fnameindex);
  return LUA_ERRFILE;
}


LUALIB_API int luaL_loadfile (lua_State *L, const char *filename) {
  LoadF lf;
  TBOOL isstdin = TFALSE;
  int status, readstatus;
  int c;
  int fnameindex = lua_gettop(L) + 1;  /* index of filename on the stack */
  lf.extraline = 0;
  if (filename == NULL) {
    lua_pushliteral(L, "=stdin");
    lf.f = TIOInputFH(TIOBase);
    isstdin = TTRUE;
  }
  else {
    lua_pushfstring(L, "@%s", filename);
    lf.f = TIOOpenFile(TIOBase, (TSTRPTR) filename, TFMODE_READONLY, TNULL);
    if (lf.f == NULL) return errfile(L, "open", fnameindex);
  }
  c = TIOFGetC(TIOBase, lf.f);
  if (c == '#') {  /* Unix exec. file? */
    lf.extraline = 1;
    while ((c = TIOFGetC(TIOBase, lf.f)) != TEOF && c != '\n') ;  /* skip first line */
    if (c == '\n') c = TIOFGetC(TIOBase, lf.f);
  }
  if (c == LUA_SIGNATURE[0] && !isstdin) {  /* binary file? */
    /* skip eventual `#!...' */
    while ((c = TIOFGetC(TIOBase, lf.f)) != TEOF && c != LUA_SIGNATURE[0]) ;
    lf.extraline = 0;
  }
  TIOFUngetC(TIOBase, lf.f, c);
  status = lua_load(L, getF, &lf, lua_tostring(L, -1));
  readstatus = TIOGetIOErr(TIOBase);
  if (!isstdin) TIOCloseFile(TIOBase, lf.f);  /* close file (even in case of errors) */
  if (readstatus) {
    lua_settop(L, fnameindex);  /* ignore results from `lua_load' */
    return errfile(L, "read", fnameindex);
  }
  lua_remove(L, fnameindex);
  return status;
}


typedef struct LoadS {
  const char *s;
  size_t size;
} LoadS;


static LUACFUNC const char *getS (lua_State *L, void *ud, size_t *size) {
  LoadS *ls = (LoadS *)ud;
  (void)L;
  if (ls->size == 0) return NULL;
  *size = ls->size;
  ls->size = 0;
  return ls->s;
}


LUALIB_API int luaL_loadbuffer (lua_State *L, const char *buff, size_t size,
                                const char *name) {
  LoadS ls;
  ls.s = buff;
  ls.size = size;
  return lua_load(L, getS, &ls, name);
}


/* }====================================================== */


#if 0
static LUACFUNC void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud;
  (void)osize;
  if (nsize == 0) {
    free(ptr);
    return NULL;
  }
  else 
    return realloc(ptr, nsize);
}


static LUACFUNC int panic (lua_State *L) {
  (void)L;  /* to avoid warnings */
  tdbprintf(99, "PANIC: unprotected error during Lua-API call\n");
  return 0;
}


LUALIB_API lua_State *luaL_newstate (void) {
  lua_State *L = lua_newstate(l_alloc, NULL);
  lua_atpanic(L, &panic);
  return L;
}
#endif
