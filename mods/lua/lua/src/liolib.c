/*
** $Id: liolib.c,v 1.1 2005/05/08 19:15:45 tmueller Exp $
** Standard I/O (and system) library
** See Copyright Notice in lua.h
*/


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define liolib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"



#define IO_INPUT		1
#define IO_OUTPUT		2


static const char *const fnames[] = {"input", "output"};


static int pushresult (lua_State *L, int i, const char *filename) {
  if (i) {
    lua_pushboolean(L, 1);
    return 1;
  }
  else {
    lua_pushnil(L);
    if (filename)
      lua_pushfstring(L, "%s: %s", filename, strerror(errno));
    else
      lua_pushfstring(L, "%s", strerror(errno));
    lua_pushinteger(L, errno);
    return 3;
  }
}


static void fileerror (lua_State *L, int arg, const char *filename) {
  lua_pushfstring(L, "%s: %s", filename, strerror(errno));
  luaL_argerror(L, arg, lua_tostring(L, -1));
}


static FILE **topfile (lua_State *L) {
  FILE **f = (FILE **)luaL_checkudata(L, 1, LUA_FILEHANDLE);
  if (f == NULL) luaL_argerror(L, 1, "bad file");
  return f;
}


static LUACFUNC int io_type (lua_State *L) {
  FILE **f = (FILE **)luaL_checkudata(L, 1, LUA_FILEHANDLE);
  if (f == NULL) lua_pushnil(L);
  else if (*f == NULL)
    lua_pushliteral(L, "closed file");
  else
    lua_pushliteral(L, "file");
  return 1;
}


static FILE *tofile (lua_State *L) {
  FILE **f = topfile(L);
  if (*f == NULL)
    luaL_error(L, "attempt to use a closed file");
  return *f;
}



/*
** When creating file handles, always creates a `closed' file handle
** before opening the actual file; so, if there is a memory error, the
** file is not left opened.
*/
static FILE **newfile (lua_State *L) {
  FILE **pf = (FILE **)lua_newuserdata(L, sizeof(FILE *));
  *pf = NULL;  /* file handle is currently `closed' */
  luaL_getmetatable(L, LUA_FILEHANDLE);
  lua_setmetatable(L, -2);
  return pf;
}


static int aux_close (lua_State *L) {
  FILE *f = tofile(L);
  if (f == stdin || f == stdout || f == stderr)
    return 0;  /* file cannot be closed */
  else {
    int ok = (fclose(f) == 0);
    if (ok)
      *(FILE **)lua_touserdata(L, 1) = NULL;  /* mark file as closed */
    return ok;
  }
}


static LUACFUNC int io_close (lua_State *L) {
  if (lua_isnone(L, 1))
    lua_rawgeti(L, LUA_ENVIRONINDEX, IO_OUTPUT);
  return pushresult(L, aux_close(L), NULL);
}


static LUACFUNC int io_gc (lua_State *L) {
  FILE **f = topfile(L);
  if (*f != NULL)  /* ignore closed files */
    aux_close(L);
  return 0;
}


static LUACFUNC int io_tostring (lua_State *L) {
  FILE *f = *topfile(L);
  if (f == NULL)
    lua_pushstring(L, "file (closed)");
  else
    lua_pushfstring(L, "file (%p)", f);
  return 1;
}


static LUACFUNC int io_open (lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  const char *mode = luaL_optstring(L, 2, "r");
  FILE **pf = newfile(L);
  *pf = fopen(filename, mode);
  return (*pf == NULL) ? pushresult(L, 0, filename) : 1;
}


static LUACFUNC int io_tmpfile (lua_State *L) {
  FILE **pf = newfile(L);
  *pf = tmpfile();
  return (*pf == NULL) ? pushresult(L, 0, NULL) : 1;
}


static FILE *getiofile (lua_State *L, int findex) {
  FILE *f;
  lua_rawgeti(L, LUA_ENVIRONINDEX, findex);
  lua_assert(luaL_checkudata(L, -1, LUA_FILEHANDLE));
  f = *(FILE **)lua_touserdata(L, -1);
  if (f == NULL)
    luaL_error(L, "standard %s file is closed", fnames[findex - 1]);
  return f;
}


static int g_iofile (lua_State *L, int f, const char *mode) {
  if (!lua_isnoneornil(L, 1)) {
    const char *filename = lua_tostring(L, 1);
    if (filename) {
      FILE **pf = newfile(L);
      *pf = fopen(filename, mode);
      if (*pf == NULL)
        fileerror(L, 1, filename);
    }
    else {
      tofile(L);  /* check that it's a valid file handle */
      lua_pushvalue(L, 1);
    }
    lua_assert(luaL_checkudata(L, -1, LUA_FILEHANDLE));
    lua_rawseti(L, LUA_ENVIRONINDEX, f);
  }
  /* return current value */
  lua_rawgeti(L, LUA_ENVIRONINDEX, f);
  return 1;
}


static LUACFUNC int io_input (lua_State *L) {
  return g_iofile(L, IO_INPUT, "r");
}


static LUACFUNC int io_output (lua_State *L) {
  return g_iofile(L, IO_OUTPUT, "w");
}


static LUACFUNC int io_readline (lua_State *L);


static void aux_lines (lua_State *L, int idx, int close) {
  lua_pushvalue(L, idx);
  lua_pushboolean(L, close);  /* close/not close file when finished */
  lua_pushcclosure(L, io_readline, 2);
}


static LUACFUNC int f_lines (lua_State *L) {
  tofile(L);  /* check that it's a valid file handle */
  aux_lines(L, 1, 0);
  return 1;
}


static LUACFUNC int io_lines (lua_State *L) {
  if (lua_isnoneornil(L, 1)) {  /* no arguments? */
    /* will iterate over default input */
    lua_rawgeti(L, LUA_ENVIRONINDEX, IO_INPUT);
    return f_lines(L);
  }
  else {
    const char *filename = luaL_checkstring(L, 1);
    FILE **pf = newfile(L);
    *pf = fopen(filename, "r");
    if (*pf == NULL)
      fileerror(L, 1, filename);
    aux_lines(L, lua_gettop(L), 1);
    return 1;
  }
}


/*
** {======================================================
** READ
** =======================================================
*/


static int read_number (lua_State *L, FILE *f) {
  lua_Number d;
  if (fscanf(f, LUA_NUMBER_SCAN, &d) == 1) {
    lua_pushnumber(L, d);
    return 1;
  }
  else return 0;  /* read fails */
}


static int test_eof (lua_State *L, FILE *f) {
  int c = getc(f);
  ungetc(c, f);
  lua_pushlstring(L, NULL, 0);
  return (c != EOF);
}


static int read_line (lua_State *L, FILE *f) {
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  for (;;) {
    size_t l;
    char *p = luaL_prepbuffer(&b);
    if (fgets(p, LUAL_BUFFERSIZE, f) == NULL) {  /* eof? */
      luaL_pushresult(&b);  /* close buffer */
      return (lua_strlen(L, -1) > 0);  /* check whether read something */
    }
    l = strlen(p);
    if (p[l-1] != '\n')
      luaL_addsize(&b, l);
    else {
      luaL_addsize(&b, l - 1);  /* do not include `eol' */
      luaL_pushresult(&b);  /* close buffer */
      return 1;  /* read at least an `eol' */
    }
  }
}


static int read_chars (lua_State *L, FILE *f, size_t n) {
  size_t rlen;  /* how much to read */
  size_t nr;  /* number of chars actually read */
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  rlen = LUAL_BUFFERSIZE;  /* try to read that much each time */
  do {
    char *p = luaL_prepbuffer(&b);
    if (rlen > n) rlen = n;  /* cannot read more than asked */
    nr = fread(p, sizeof(char), rlen, f);
    luaL_addsize(&b, nr);
    n -= nr;  /* still have to read `n' chars */
  } while (n > 0 && nr == rlen);  /* until end of count or eof */
  luaL_pushresult(&b);  /* close buffer */
  return (n == 0 || lua_strlen(L, -1) > 0);
}


static int g_read (lua_State *L, FILE *f, int first) {
  int nargs = lua_gettop(L) - 1;
  int success;
  int n;
  clearerr(f);
  if (nargs == 0) {  /* no arguments? */
    success = read_line(L, f);
    n = first+1;  /* to return 1 result */
  }
  else {  /* ensure stack space for all results and for auxlib's buffer */
    luaL_checkstack(L, nargs+LUA_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (lua_type(L, n) == LUA_TNUMBER) {
        size_t l = (size_t)lua_tointeger(L, n);
        success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
      }
      else {
        const char *p = lua_tostring(L, n);
        luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
        switch (p[1]) {
          case 'n':  /* number */
            success = read_number(L, f);
            break;
          case 'l':  /* line */
            success = read_line(L, f);
            break;
          case 'a':  /* file */
            read_chars(L, f, ~((size_t)0));  /* read MAX_SIZE_T chars */
            success = 1; /* always success */
            break;
          case 'w':  /* word */
            return luaL_error(L, "obsolete option `*w' to `read'");
          default:
            return luaL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (ferror(f))
    return pushresult(L, 0, NULL);
  if (!success) {
    lua_pop(L, 1);  /* remove last result */
    lua_pushnil(L);  /* push nil instead */
  }
  return n - first;
}


static LUACFUNC int io_read (lua_State *L) {
  return g_read(L, getiofile(L, IO_INPUT), 1);
}


static LUACFUNC int f_read (lua_State *L) {
  return g_read(L, tofile(L), 2);
}


static LUACFUNC int io_readline (lua_State *L) {
  FILE *f = *(FILE **)lua_touserdata(L, lua_upvalueindex(1));
  int sucess;
  if (f == NULL)  /* file is already closed? */
    luaL_error(L, "file is already closed");
  sucess = read_line(L, f);
  if (ferror(f))
    luaL_error(L, "%s", strerror(errno));
  if (sucess) return 1;
  else {  /* EOF */
    if (lua_toboolean(L, lua_upvalueindex(2))) {  /* generator created file? */
      lua_settop(L, 0);
      lua_pushvalue(L, lua_upvalueindex(1));
      aux_close(L);  /* close it */
    }
    return 0;
  }
}

/* }====================================================== */


static int g_write (lua_State *L, FILE *f, int arg) {
  int nargs = lua_gettop(L) - 1;
  int status = 1;
  for (; nargs--; arg++) {
    if (lua_type(L, arg) == LUA_TNUMBER) {
      /* optimization: could be done exactly as for strings */
      status = status &&
          fprintf(f, LUA_NUMBER_FMT, lua_tonumber(L, arg)) > 0;
    }
    else {
      size_t l;
      const char *s = luaL_checklstring(L, arg, &l);
      status = status && (fwrite(s, sizeof(char), l, f) == l);
    }
  }
  return pushresult(L, status, NULL);
}


static LUACFUNC int io_write (lua_State *L) {
  return g_write(L, getiofile(L, IO_OUTPUT), 1);
}


static LUACFUNC int f_write (lua_State *L) {
  return g_write(L, tofile(L), 2);
}


static LUACFUNC int f_seek (lua_State *L) {
  static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
  static const char *const modenames[] = {"set", "cur", "end", NULL};
  FILE *f = tofile(L);
  int op = luaL_findstring(luaL_optstring(L, 2, "cur"), modenames);
  lua_Integer offset = luaL_optinteger(L, 3, 0);
  luaL_argcheck(L, op != -1, 2, "invalid mode");
  op = fseek(f, offset, mode[op]);
  if (op)
    return pushresult(L, 0, NULL);  /* error */
  else {
    lua_pushinteger(L, ftell(f));
    return 1;
  }
}


static LUACFUNC int f_setvbuf (lua_State *L) {
  static const int mode[] = {_IONBF, _IOFBF, _IOLBF};
  static const char *const modenames[] = {"no", "full", "line", NULL};
  FILE *f = tofile(L);
  int op = luaL_findstring(luaL_checkstring(L, 2), modenames);
  luaL_argcheck(L, op != -1, 2, "invalid mode");
  return pushresult(L, setvbuf(f, NULL, mode[op], 0) == 0, NULL);
}



static LUACFUNC int io_flush (lua_State *L) {
  return pushresult(L, fflush(getiofile(L, IO_OUTPUT)) == 0, NULL);
}


static LUACFUNC int f_flush (lua_State *L) {
  return pushresult(L, fflush(tofile(L)) == 0, NULL);
}


static const luaL_reg iolib[] = {
  {"input", io_input},
  {"output", io_output},
  {"lines", io_lines},
  {"close", io_close},
  {"flush", io_flush},
  {"open", io_open},
  {"read", io_read},
  {"tmpfile", io_tmpfile},
  {"type", io_type},
  {"write", io_write},
  {NULL, NULL}
};


static const luaL_reg flib[] = {
  {"flush", f_flush},
  {"read", f_read},
  {"lines", f_lines},
  {"seek", f_seek},
  {"setvbuf", f_setvbuf},
  {"write", f_write},
  {"close", io_close},
  {"__gc", io_gc},
  {"__tostring", io_tostring},
  {NULL, NULL}
};


static void createmeta (lua_State *L) {
  luaL_newmetatable(L, LUA_FILEHANDLE);  /* create metatable for file handles */
  lua_pushvalue(L, -1);  /* push metatable */
  lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  luaL_openlib(L, NULL, flib, 0);  /* file methods */
}


static void createupval (lua_State *L) {
  lua_newtable(L);
  /* create (and set) default files */
  *newfile(L) = stdin;
  lua_rawseti(L, -2, IO_INPUT);
  *newfile(L) = stdout;
  lua_rawseti(L, -2, IO_OUTPUT);
}



LUALIB_API int luaopen_io (lua_State *L) {
  createmeta(L);
  createupval(L);
  lua_pushvalue(L, -1);
  lua_replace(L, LUA_ENVIRONINDEX);
  luaL_openlib(L, LUA_IOLIBNAME, iolib, 0);
  /* put predefined file handles into `io' table */
  lua_rawgeti(L, -2, IO_INPUT);  /* get current input from upval */
  lua_setfield(L, -2, "stdin");  /* io.stdin = upval[IO_INPUT] */
  lua_rawgeti(L, -2, IO_OUTPUT);  /* get current output from upval */
  lua_setfield(L, -2, "stdout");  /* io.stdout = upval[IO_OUTPUT] */
  *newfile(L) = stderr;
  lua_setfield(L, -2, "stderr");  /* io.stderr = newfile(stderr) */
  return 1;
}

