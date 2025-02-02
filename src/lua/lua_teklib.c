
/*
**	$Id: lua_teklib.c,v 1.3 2006/09/09 20:18:01 tmueller Exp $
**	teklib/src/lua/lua_teklib.c - 'tek' library implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>
#include <tek/mod/time.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include <tek/mod/luabase.h>

/*****************************************************************************/
/*
**	Globals (standalone build only)
*/

#if defined(TEKLUA)

TAPTR TBaseTask;
TAPTR TExecBase;
TAPTR TIOBase;
TAPTR TUtilBase;
TAPTR lua_StdIn, lua_StdOut, lua_StdErr;

#endif

/*****************************************************************************/

#define TIO_INPUT	1
#define TIO_OUTPUT	2

#define	LUA_TEKLIBNAME		"tek"
#define LUA_TEKFILEHANDLE	"TFILE*"
#define LUA_TEKLOCK			"TLOCK*"

/*****************************************************************************/

static const char *const luatek_fnames[] = {"input", "output"};

static TSTRPTR
luatek_geterrs(lua_State *L, TSTRPTR buf, TINT len)
{
	TINT errnum = TIOGetIOErr(lua_IOBase);
	if (errnum)
	{
		TIOFault(lua_IOBase, errnum, buf, len, TNULL);
		return buf;
	}
	return TNULL;
}

static TINT
luatek_pushresult(lua_State *L, TBOOL res, const char *filename)
{
	if (res)
	{
		lua_pushboolean(L, 1);
		return 1;
	}
	else
	{
		TCHR errbuf[80];
		luatek_geterrs(L, errbuf, sizeof(errbuf));
		lua_pushnil(L);
		if (filename)
			lua_pushfstring(L, "%s: %s", filename, errbuf);
		else
			lua_pushfstring(L, "%s", errbuf);
		lua_pushnumber(L, TIOGetIOErr(lua_IOBase));
		return 3;
	}
}

static void
luatek_fileerror (lua_State *L, int arg, const char *filename)
{
	TCHR errbuf[80];
	luatek_geterrs(L, errbuf, sizeof(errbuf));
	lua_pushfstring(L, "%s: %s", filename, errbuf);
	luaL_argerror(L, arg, lua_tostring(L, -1));
}

#define topobject(L, class)	((TAPTR *) luaL_checkudata(L, 1, class))

static LUACFUNC TINT
luatek_type (lua_State *L)
{
	void *ud;
	luaL_checkany(L, 1);
	ud = lua_touserdata(L, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, LUA_TEKFILEHANDLE);
	if (ud == TNULL || !lua_getmetatable(L, 1) || !lua_rawequal(L, -2, -1))
		lua_pushnil(L);  /* not a file */
	else if (*((TAPTR *)ud) == TNULL)
		lua_pushliteral(L, "closed file");
	else
		lua_pushliteral(L, "file");
	return 1;
}

static TAPTR
luatek_toobject(lua_State *L, const char *class)
{
	TAPTR *f = topobject(L, class);
	if (*f == TNULL)
		luaL_error(L, "attempt to use a closed object");
	return *f;
}

/*
** When creating file handles, always creates a `closed' file handle
** before opening the actual file; so, if there is a memory error, the
** file is not left opened.
*/

static TAPTR *
luatek_newobject(lua_State *L, const char *class)
{
	TAPTR *pf = (TAPTR *) lua_newuserdata(L, sizeof(TAPTR));
	*pf = TNULL;	/* handle is currently `closed' */
	luaL_getmetatable(L, class);
	lua_setmetatable(L, -2);
	return pf;
}

static LUACFUNC TINT
luatek_fclose (lua_State *L)
{
	TAPTR *p = topobject(L, LUA_TEKFILEHANDLE);
	int ok = TIOCloseFile(lua_IOBase, *p);
	if (ok)
		*p = TNULL;
	return luatek_pushresult(L, ok, NULL);
}

static TINT
luatek_aux_close (lua_State *L)
{
	lua_getfenv(L, 1);
	lua_getfield(L, -1, "__close");
	return (lua_tocfunction(L, -1))(L);
}

static LUACFUNC TINT
luatek_close (lua_State *L)
{
	if (lua_isnone(L, 1))
		lua_rawgeti(L, LUA_ENVIRONINDEX, TIO_OUTPUT);
	luatek_toobject(L, LUA_TEKFILEHANDLE);  /* make sure argument is a file */
	return luatek_aux_close(L);
}

static LUACFUNC TINT
luatek_gc (lua_State *L)
{
	TAPTR f = *topobject(L, LUA_TEKFILEHANDLE);
	/* ignore closed files and standard files */
	if (f != TNULL && f != lua_StdIn && f != lua_StdOut && f != lua_StdErr)
		luatek_aux_close(L);
	return 0;
}

static LUACFUNC TINT
luatek_tostring (lua_State *L)
{
	TAPTR f = *topobject(L, LUA_TEKFILEHANDLE);
	if (f == TNULL)
		lua_pushstring(L, "file (closed)");
	else
		lua_pushfstring(L, "file (%p)", f);
	return 1;
}

static LUACFUNC TINT
luatek_open (lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);
	const char *mode = luaL_optstring(L, 2, "r");
	TAPTR *pf = luatek_newobject(L, LUA_TEKFILEHANDLE);
	TBOOL append = TFALSE;
	TUINT tekmode;
	switch (*mode++)
	{
		case 'w':
			tekmode = TFMODE_NEWFILE;
			break;
		case 'a':
			append = TTRUE;
			tekmode = TFMODE_READWRITE;
			break;
		case 'r':
			if (*mode == 'b')
				mode++;
			if (*mode == '+')
			{
				tekmode = TFMODE_READWRITE;
				break;
			}
		default:
			tekmode = TFMODE_READONLY;
			break;
	}
	*pf = TIOOpenFile(lua_IOBase, (TSTRPTR) filename, tekmode, TNULL);
	if (*pf)
	{
		if (append)
		{
			TIOSeek(lua_IOBase, *pf, 0, TNULL, TFPOS_END);
			/* TODO: handle seek error */
		}
		return 1;
	}
	return luatek_pushresult(L, 0, filename);
}

static TAPTR
luatek_getiofile (lua_State *L, int findex)
{
	TAPTR f;
	lua_rawgeti(L, LUA_ENVIRONINDEX, findex);
	f = *(TAPTR *)lua_touserdata(L, -1);
	if (f == NULL)
		luaL_error(L, "standard %s file is closed", luatek_fnames[findex - 1]);
	return f;
}

static TINT
luatek_g_iofile (lua_State *L, int f, TUINT mode)
{
	if (!lua_isnoneornil(L, 1))
	{
		const char *filename = lua_tostring(L, 1);
		if (filename)
		{
			TAPTR *pf = luatek_newobject(L, LUA_TEKFILEHANDLE);
			*pf = TIOOpenFile(lua_IOBase, (TSTRPTR) filename, mode, TNULL);
			if (*pf == NULL)
				luatek_fileerror(L, 1, filename);
		}
		else
		{
			/* check that it's a valid file handle */
			luatek_toobject(L, LUA_TEKFILEHANDLE);
			lua_pushvalue(L, 1);
		}
		lua_rawseti(L, LUA_ENVIRONINDEX, f);
	}
	/* return current value */
	lua_rawgeti(L, LUA_ENVIRONINDEX, f);
	return 1;
}

static LUACFUNC TINT
luatek_input (lua_State *L)
{
	return luatek_g_iofile(L, TIO_INPUT, TFMODE_READONLY);
}

static LUACFUNC TINT
luatek_output (lua_State *L)
{
	return luatek_g_iofile(L, TIO_OUTPUT, TFMODE_NEWFILE);
}

static LUACFUNC int luatek_readline (lua_State *L);

static void luatek_aux_lines (lua_State *L, int idx, int toclose) {
  lua_pushvalue(L, idx);
  lua_pushboolean(L, toclose);  /* close/not close file when finished */
  lua_pushcclosure(L, luatek_readline, 2);
}

static LUACFUNC TINT
luatek_f_lines (lua_State *L) {
  luatek_toobject(L, LUA_TEKFILEHANDLE); /* check that it's a valid file handle */
  luatek_aux_lines(L, 1, 0);
  return 1;
}

static LUACFUNC TINT
luatek_lines (lua_State *L) {
  if (lua_isnoneornil(L, 1)) {  /* no arguments? */
    /* will iterate over default input */
    lua_rawgeti(L, LUA_ENVIRONINDEX, TIO_INPUT);
    return luatek_f_lines(L);
  }
  else {
    const char *filename = luaL_checkstring(L, 1);
	TAPTR *pf = luatek_newobject(L, LUA_TEKFILEHANDLE);
	*pf = TIOOpenFile(lua_IOBase, (TSTRPTR) filename, TFMODE_READONLY, TNULL);
    if (*pf == NULL)
      luatek_fileerror(L, 1, filename);
    luatek_aux_lines(L, lua_gettop(L), 1);
    return 1;
  }
}

/*
** {======================================================
** READ
** =======================================================
*/

static lua_Number
luatek_power10(TINT e)
{
	lua_Number res = 1;
	if (e > 0)
	{
		while (e--)
			res *= 10;
	}
	else if (e < 0)
	{
		while (e++)
			res /= 10;
	}
	return res;
}

static TBOOL
luatek_scannum(lua_State *L, TAPTR file, lua_Number *d)
{
	lua_Number result = 0;
	TINT state = 0;
	TUINT a = 0, b = 0;
	TINT nd = 0, e = 0, e2 = 0, sig = 1, esig = 1;
	TINT c;

	if (!file) goto error;

	while (state >= 0 && (c = TIOFGetC(lua_IOBase, file)))
	{
		switch (state)
		{
			case 0:			/* waiting for start; reading +, -, . or 0-9 */
				switch (c)
				{
					case '+': case 32: case 10: case 13: case 9:
						break;
					case '-':
						sig = -sig;
						break;
					case '.':
						state = 2;
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						a = c - '0';
						state = 1;
						break;
					default:
						goto error;
				}
				break;

			case 1:			/* expecting 0-9 or . or e or end */
				switch (c)
				{
					case '.':
						state = 2;
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						if (a < 429400000)
						{
							a *= 10;
							a += c - '0';
						}
						else
						{
							e2++;
						}
						break;
					case 'E': case 'e':
						state = 3;
						break;
					case 32: case 10: case 13: case 9:
						state = -1;
						continue;
					default:
						goto error;
				}
				break;

			case 2:			/* waiting for 0-9 or e or end */
				switch (c)
				{
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						if (b < 429400000)
						{
							b *= 10;
							b += c - '0';
							nd++;
						}
						break;
					case 'E': case 'e':
						state = 3;
						break;
					case 32: case 10: case 13: case 9:
						state = -1;
						continue;
					default:
						goto error;
				}
				break;

			case 3:			/* reading exponent; +, -, or number */
				switch (c)
				{
					default:
						goto error;
					case '+':
						break;
					case '-':
						esig = -1;
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						e = c - '0';
						break;
				}
				state = 4;
				break;

			case 4:			/* more digits of the exponent, or end */
				switch (c)
				{
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						e *= 10;
						e += c - '0';
						break;
					case 32: case 10: case 13: case 9:
						state = -1;
						continue;
					default:
						goto error;
				}
				break;
		}
	}

	result = a + b / luatek_power10(nd);
	result *= luatek_power10(esig * e + e2);
	result *= sig;
	*d = result;
	return TTRUE;

  error:
	return TFALSE;
}

static TINT
luatek_read_number(lua_State *L, TAPTR f)
{
	lua_Number d;
	if (luatek_scannum(L, f, &d))
	{
		lua_pushnumber(L, d);
		return 1;
	}
	else
		return 0;	/* read fails */
}

static TINT
luatek_test_eof (lua_State *L, TAPTR f)
{
  	TINT iseof = TIOFEoF(lua_IOBase, f);
	lua_pushlstring(L, NULL, 0);
	return !iseof;
}

static int
luatek_read_line (lua_State *L, TAPTR f) {
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  for (;;) {
    size_t l;
    char *p = luaL_prepbuffer(&b);
	if (TIOFGetS(lua_IOBase, f, p, LUAL_BUFFERSIZE) == TNULL)
	{
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

static int
luatek_read_chars (lua_State *L, TAPTR f, size_t n) {
  size_t rlen;  /* how much to read */
  size_t nr;  /* number of chars actually read */
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  rlen = LUAL_BUFFERSIZE;  /* try to read that much each time */
  do {
    char *p = luaL_prepbuffer(&b);
    if (rlen > n) rlen = n;  /* cannot read more than asked */
      nr = TIOFRead(lua_IOBase, f, p, rlen);
    luaL_addsize(&b, nr);
    n -= nr;  /* still have to read `n' chars */
  } while (n > 0 && nr == rlen);  /* until end of count or eof */
  luaL_pushresult(&b);  /* close buffer */
  return (n == 0 || lua_strlen(L, -1) > 0);
}

static int
luatek_g_read (lua_State *L, TAPTR f, int first) {
  int nargs = lua_gettop(L) - 1;
  int success;
  int n;
  TIOSetIOErr(lua_IOBase, 0);
  if (nargs == 0) {  /* no arguments? */
    success = luatek_read_line(L, f);
    n = first+1;  /* to return 1 result */
  }
  else {  /* ensure stack space for all results and for auxlib's buffer */
    luaL_checkstack(L, nargs+LUA_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (lua_type(L, n) == LUA_TNUMBER) {
        size_t l = (size_t)lua_tointeger(L, n);
        success = (l == 0) ? luatek_test_eof(L, f) : luatek_read_chars(L, f, l);
      }
      else {
        const char *p = lua_tostring(L, n);
        luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
        switch (p[1]) {
          case 'n':  /* number */
            success = luatek_read_number(L, f);
            break;
          case 'l':  /* line */
            success = luatek_read_line(L, f);
            break;
          case 'a':  /* file */
            luatek_read_chars(L, f, ~((size_t)0));  /* read MAX_SIZE_T chars */
            success = 1; /* always success */
            break;
          default:
            return luaL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (((struct TIORequest *) f)->io_Error)
    return luatek_pushresult(L, 0, NULL);
  if (!success) {
    lua_pop(L, 1);  /* remove last result */
    lua_pushnil(L);  /* push nil instead */
  }
  return n - first;
}

static LUACFUNC TINT
luatek_read (lua_State *L) {
  return luatek_g_read(L, luatek_getiofile(L, TIO_INPUT), 1);
}

static LUACFUNC TINT
luatek_f_read (lua_State *L) {
  return luatek_g_read(L, luatek_toobject(L, LUA_TEKFILEHANDLE), 2);
}

static LUACFUNC TINT
luatek_readline (lua_State *L) {
  TAPTR f = *(TAPTR *)lua_touserdata(L, lua_upvalueindex(1));
  int sucess;
  if (f == NULL)  /* file is already closed? */
    luaL_error(L, "file is already closed");
  sucess = luatek_read_line(L, f);
  if (((struct TIORequest *) f)->io_Error)
  {
	TCHR errbuf[80];
	luatek_geterrs(L, errbuf, sizeof(errbuf));
    return luaL_error(L, "%s", errbuf);
  }
  if (sucess) return 1;
  else {  /* EOF */
    if (lua_toboolean(L, lua_upvalueindex(2))) {  /* generator created file? */
      lua_settop(L, 0);
      lua_pushvalue(L, lua_upvalueindex(1));
      luatek_aux_close(L);  /* close it */
    }
    return 0;
  }
}

/* }====================================================== */

static TINT
luatek_g_write (lua_State *L, TAPTR f, int arg)
{
	int nargs = lua_gettop(L) - 1;
	int status = 1;
	for (; nargs--; arg++)
	{
		if (lua_type(L, arg) == LUA_TNUMBER)
		{
			TCHR buffer[64];
			sprintf(buffer, LUA_NUMBER_FMT, lua_tonumber(L, arg));
			status = status && TIOFWrite(lua_IOBase, f, buffer, strlen(buffer));
		}
		else
		{
			size_t l;
			TSTRPTR s = (TSTRPTR) luaL_checklstring(L, arg, &l);
			status = status &&
				((size_t) TIOFWrite(lua_IOBase, f, s, l) == l);
		}
	}
	return luatek_pushresult(L, status, NULL);
}

static LUACFUNC TINT
luatek_write (lua_State *L)
{
	return luatek_g_write(L, luatek_getiofile(L, TIO_OUTPUT), 1);
}

static LUACFUNC TINT
luatek_f_write (lua_State *L)
{
	return luatek_g_write(L, luatek_toobject(L, LUA_TEKFILEHANDLE), 2);
}

static LUACFUNC TINT
luatek_f_seek (lua_State *L) {
  static const TINT mode[] = { TFPOS_BEGIN, TFPOS_CURRENT, TFPOS_END };
  static const char *const modenames[] = {"set", "cur", "end", NULL};
  TAPTR f = luatek_toobject(L, LUA_TEKFILEHANDLE);
  int op = luaL_checkoption(L, 2, "cur", modenames);
  long offset = luaL_optlong(L, 3, 0);
	TUINT res;
	/* TODO: 64bit offs */
	res = TIOSeek(lua_IOBase, f, offset, TNULL, mode[op]);
	if (res == 0xffffffff)
		return luatek_pushresult(L, 0, NULL);	/* error */
	else
	{
		lua_pushnumber(L, res);	/* lua_pushinteger() */
		return 1;
	}
}

static LUACFUNC TINT
luatek_flush (lua_State *L) {
  return luatek_pushresult(L, TIOFlush(lua_IOBase, luatek_getiofile(L, TIO_OUTPUT)), NULL);
}

static LUACFUNC TINT
luatek_f_flush (lua_State *L) {
  return luatek_pushresult(L, TIOFlush(lua_IOBase, luatek_toobject(L, LUA_TEKFILEHANDLE)), NULL);
}

static LUACFUNC TINT
luatek_f_examine(lua_State *L)
{
	TAPTR file = luatek_toobject(L, LUA_TEKFILEHANDLE);
	TTAGITEM extags[5];
	TUINT type = 0, size = 0, sizehi = 0;
	TDATE date;
	lua_Number lsize = 0;

	extags[0].tti_Tag = TFATTR_Type;
	extags[0].tti_Value = (TTAG) &type;
	extags[1].tti_Tag = TFATTR_Size;
	extags[1].tti_Value = (TTAG) &size;
	extags[2].tti_Tag = TFATTR_SizeHigh;
	extags[2].tti_Value = (TTAG) &sizehi;
	extags[3].tti_Tag = TFATTR_Date;
	extags[3].tti_Value = (TTAG) &date;
	extags[4].tti_Tag = TTAG_DONE;

	TIOExamine(lua_IOBase, file, extags);

	#if defined(TSYS_HAVE_INT64)
		lsize = (lua_Number) (TINT64)
			((((TUINT64) sizehi) << 32) | (TUINT64) size);
	#else
		lsize = size;
	#endif /* defined(TSYS_HAVE_INT64) */

	lua_pushnumber(L, type);
	lua_pushnumber(L, lsize);
	/* convert to Julian date: */
	lua_pushnumber(L,
		(TDOUBLE) date.tdt_Int64 / 86400000000ULL + 2305813.5);

	return 3;
}

/*****************************************************************************/

static LUACFUNC TINT
luatek_addpart(lua_State *L)
{
	TSTRPTR part1 = (TSTRPTR) luaL_checkstring(L, 1);
	TSTRPTR part2 = (TSTRPTR) luaL_checkstring(L, 2);
	TINT len = TIOAddPart(lua_IOBase, part1, part2, TNULL, 0);
	if (len >= 0)
	{
		TSTRPTR buf = TExecAlloc(lua_ExecBase, TNULL, len + 1);
		if (buf)
		{
			TIOAddPart(lua_IOBase, part1, part2, buf, len + 1);
			lua_pushlstring(L, buf, len);
			TExecFree(lua_ExecBase, buf);
		}
		else
			luaL_error(L, "out of memory");
	}
	else
		lua_pushnil(L);

	return 1;
}

static LUACFUNC TINT
luatek_delete(lua_State *L)
{
	TSTRPTR filename = (TSTRPTR) luaL_checkstring(L, 1);
	return luatek_pushresult(L, TIODeleteFile(lua_IOBase, (TSTRPTR) filename), filename);
}

static LUACFUNC TINT
luatek_rename(lua_State *L)
{
	TSTRPTR fromname = (TSTRPTR) luaL_checkstring(L, 1);
	TSTRPTR toname = (TSTRPTR) luaL_checkstring(L, 2);
	return luatek_pushresult(L, TIORename(lua_IOBase, (TSTRPTR) fromname,
		(TSTRPTR) toname), fromname);
}

static LUACFUNC TINT
luatek_mount(lua_State *L)
{
	TSTRPTR mountname = (TSTRPTR) luaL_checkstring(L, 1);
	TSTRPTR handlername = (TSTRPTR) luaL_checkstring(L, 2);
	TSTRPTR initstring = (TSTRPTR) luaL_optstring(L, 3, TNULL);
	TTAGITEM tags[3];
	tags[0].tti_Tag = TIOMount_Handler;
	tags[0].tti_Value = (TTAG) handlername;
	tags[1].tti_Tag = TIOMount_InitString;
	tags[1].tti_Value = (TTAG) initstring;
	tags[2].tti_Tag = TTAG_DONE;
	lua_pushboolean(L, TIOMount(lua_IOBase, mountname, TIOMNT_ADD, tags));
	return 1;
}

static LUACFUNC TINT
luatek_makename(lua_State *L)
{
	TSTRPTR hostname = (TSTRPTR) luaL_checkstring(L, 1);
	TINT len = TIOMakeName(lua_IOBase, hostname, TNULL, 0, TPPF_HOST2TEK, TNULL);
	if (len >= 0)
	{
		TSTRPTR destname = TExecAlloc(lua_ExecBase, TNULL, len + 1);
		if (destname)
		{
			TIOMakeName(lua_IOBase, hostname, destname, len + 1, TPPF_HOST2TEK, TNULL);
			lua_pushlstring(L, destname, len);
			TExecFree(lua_ExecBase, destname);
		}
		else
			luaL_error(L, "out of memory");
	}
	else
		lua_pushnil(L);

	return 1;
}

static LUACFUNC TINT
luatek_readargs(lua_State *L)
{
	TINT numargs = lua_gettop(L);
	TSTRPTR tmpl;
	TSTRPTR *argv;
	TTAG *args;
	TAPTR argh;
	TSTRPTR argt;
	TINT i, j, c, numret, numtmp;
	TSTRPTR p;

	if (numargs == 0)
		luaL_error(L, "not enough arguments");

	/* check arguments */
	tmpl = (TSTRPTR) luaL_checkstring(L, 1);
	for (i = 0; i < numargs - 1; ++i)
		lua_tostring(L, 2 + i);

	/* count options in template */
	p = tmpl;
	numtmp = 1;
	while ((c = *p++))
		if (c == ',') numtmp++;

	argv = TExecAlloc(lua_ExecBase, TNULL, numargs * sizeof(*argv));
	args = TExecAlloc0(lua_ExecBase, TNULL, numtmp * sizeof(*args));
	argt = TExecAlloc0(lua_ExecBase, TNULL, numtmp * sizeof(*argt));

	if (argv == TNULL || args == TNULL || argt == TNULL)
	{
		TExecFree(lua_ExecBase, argt);
		TExecFree(lua_ExecBase, args);
		TExecFree(lua_ExecBase, argv);
		luaL_error(L, "out of memory");
	}

	p = tmpl;
	i = 0;
	j = 0;
	while ((c = *p++))
	{
		switch (c)
		{
			case ',':
				++i;
				j = 0;
				break;

			case '/':
				j = 1;
				break;

			default:
				if (j)
				{
					switch (c)
					{
						case 'm': case 'M':
							argt[i] = 'm';
							break;
						case 'n': case 'N':
							argt[i] = 'n';
							break;
						case 's': case 'S':
							argt[i] = 's';
							break;
					}
					j = 0;
				}
		}
	}

	for (i = 0; i < numargs - 1; ++i)
		argv[i] = (TSTRPTR) lua_tostring(L, 2 + i);
	argv[numargs - 1] = TNULL;

	argh = TUtilParseArgV(lua_UtilBase, tmpl, argv, args);
	if (argh)
	{
		lua_pushboolean(L, 1);
		for (i = 0; i < numtmp; ++i)
		{
			switch (argt[i])
			{
				case 'm':
					if (args[i])
					{
						TSTRPTR *s = (TSTRPTR *) args[i];
						lua_newtable(L);
						j = 1;
						while (*s)
						{
							lua_pushstring(L, *s);
							lua_rawseti(L, -2, j);
							s++;
							j++;
						}
					}
					else
						lua_pushnil(L);
					break;

				case 'n':
					if (args[i])
						lua_pushnumber(L, (lua_Number) *(TINT *) args[i]);
					else
						lua_pushnil(L);
					break;

				case 's':
					if (args[i])
						lua_pushboolean(L, TTRUE);
					else
						lua_pushnil(L);
					break;

				default:
					if (args[i])
						lua_pushstring(L, (TSTRPTR) args[i]);
					else
						lua_pushnil(L);
					break;
			}
		}
		TDestroy(argh);
		numret = numtmp + 1;
	}
	else
	{
		lua_pushnil(L);
		numret = 1;
	}

	TExecFree(lua_ExecBase, argt);
	TExecFree(lua_ExecBase, args);
	TExecFree(lua_ExecBase, argv);

	return numret;
}

static LUACFUNC TINT
luatek_clock(lua_State *L)
{
	TTIME elapsed;
	lua_Number t;
	TExecGetSystemTime(lua_ExecBase, &elapsed);
	t = (lua_Number) elapsed.tdt_Int64 / 1000000;
	lua_pushnumber(L, t);
	return 1;
}

static int
luatek_getfield(lua_State *L, const char *key, int d)
{
	int res;
	lua_getfield(L, -1, key);
	if (lua_isnumber(L, -1))
		res = (int)lua_tointeger(L, -1);
	else {
		if (d < 0)
			return luaL_error(L, "field `%s' missing in date table", key);
		res = d;
	}
	lua_pop(L, 1);
	return res;
}

static void luatek_setfield (lua_State *L, const char *key, int value) {
  lua_pushinteger(L, value);
  lua_setfield(L, -2, key);
}

static LUACFUNC TINT
luatek_time(lua_State *L)
{
	TDATE date;
	TDOUBLE t = -1;
	TINT error = 0;

	if (lua_isnoneornil(L, 1)) /* called without args? */
	{
		error = TExecGetLocalDate(lua_ExecBase, &date);
		if (error == 0)
			t = (TDOUBLE) date.tdt_Int64 / 86400000000ULL + 2305813.5;
	}
	else
	{
		struct TDateBox db;

		luaL_checktype(L, 1, LUA_TTABLE);
		lua_settop(L, 1);  /* make sure table is at the top */

		db.tdb_Fields = TDB_USEC | TDB_SEC | TDB_MINUTE | TDB_HOUR;

		db.tdb_USec = luatek_getfield(L, "usec", 0);
		db.tdb_Sec = luatek_getfield(L, "sec", 0);
		db.tdb_Minute = luatek_getfield(L, "min", 0);
		db.tdb_Hour = luatek_getfield(L, "hour", 12);

		db.tdb_Day = luatek_getfield(L, "day", 0);
		if (db.tdb_Day > 0) db.tdb_Fields |= TDB_DAY;

		db.tdb_Month = luatek_getfield(L, "month", 0);
		if (db.tdb_Month > 0) db.tdb_Fields |= TDB_MONTH;

		db.tdb_Year = luatek_getfield(L, "year", 0);
		if (db.tdb_Year > 0) db.tdb_Fields |= TDB_YEAR;

		if (TUtilPackDate(lua_UtilBase, &db, &date))
		{
			error = 0;
			/* days since 1.1.1601: */
			t = (TDOUBLE) date.tdt_Int64 / 86400000000ULL;
		}
	}

	if (error)
		lua_pushnil(L);
	else
		lua_pushnumber(L, t);

	return 1;
}

static LUACFUNC TINT
luatek_date(lua_State *L)
{
	TSTRPTR s = (TSTRPTR) luaL_optstring(L, 1, "%c");
	lua_Number n = luaL_optnumber(L, 2, -1);
	TINT error = 0;
	TDATE date;
	TBOOL utc = TFALSE;

	if (*s == '!')
	{
		utc = TTRUE;
		s++;
	}

	if (n == -1) /* no time argument */
	{
		if (utc)
		{
			/* UTC */
			error = TExecGetUniversalDate(lua_ExecBase, &date);
		}
		else
		{
			/* local time. TODO: incorporate Daylight Saving */
			error = TExecGetLocalDate(lua_ExecBase, &date);
		}
	}
	else
		date.tdt_Int64 = (n - 2305813.5) * 86400000000ULL;

	if (error)
		lua_pushnil(L);
	else
	{
		struct TDateBox db;
		/*TTimeUnpackDate(lua_TimeBase, &date, &db, TDB_ALL);*/
		TUtilUnpackDate(lua_UtilBase, &date, &db, TDB_ALL);
		if (TStrCmp(s, "*t") == 0) /* table */
		{
			lua_createtable(L, 0, 8);  /* 8 = number of fields */
			luatek_setfield(L, "sec", db.tdb_Sec);
			luatek_setfield(L, "min", db.tdb_Minute);
			luatek_setfield(L, "hour", db.tdb_Hour);
			luatek_setfield(L, "day", db.tdb_Day);
			luatek_setfield(L, "month", db.tdb_Month);
			luatek_setfield(L, "year", db.tdb_Year);
			luatek_setfield(L, "wday", db.tdb_WDay);
			luatek_setfield(L, "yday", db.tdb_YDay);
			/*setboolfield(L, "isdst", stm->tm_isdst);*/
		}
		else /* format to string */
		{
			if (TStrCmp(s, "%c") == 0)
			{
				static const char *dn[7] =
				{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
				static const char *mn[12] =
				{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
					"Sep", "Oct", "Nov", "Dec" };
				char b[256];

				sprintf(b, "%s %s % 2d %02d:%02d:%02d %d",
					dn[db.tdb_WDay], mn[db.tdb_Month - 1], db.tdb_Day,
					db.tdb_Hour, db.tdb_Minute, db.tdb_Sec, db.tdb_Year);
				lua_pushstring(L, b); /* e.g. Wed Jul  6 02:09:49 2005 */
			}
			else
				luaL_error(L, "Cannot handle date format");
		}
	}

	return 1;
}

/*****************************************************************************/

static LUACFUNC TINT
luatek_lock(lua_State *L)
{
	TSTRPTR fname = (TSTRPTR) lua_tostring(L, 1);
	const char *mode = luaL_optstring(L, 2, "r");
	TAPTR *pobj = luatek_newobject(L, LUA_TEKLOCK);
	*pobj = TIOLockFile(lua_IOBase, fname,
		!strcmp(mode, "w") ? TFLOCK_WRITE : TFLOCK_READ, TNULL);
	return (*pobj == NULL) ? luatek_pushresult(L, 0, fname) : 1;
}

static LUACFUNC TINT
luatek_lock_gc(lua_State *L)
{
	TAPTR *p = topobject(L, LUA_TEKLOCK);
	TIOUnlockFile(lua_IOBase, *p);
	*p = TNULL;
	return luatek_pushresult(L, TTRUE, TNULL);
}

static LUACFUNC TINT
luatek_lock_unlock(lua_State *L)
{
	TAPTR *p = topobject(L, LUA_TEKLOCK);
	luatek_toobject(L, LUA_TEKLOCK);
	TIOUnlockFile(lua_IOBase, *p);
	*p = TNULL;
	return luatek_pushresult(L, TTRUE, TNULL);
}

static LUACFUNC TINT
luatek_lock_tostring (lua_State *L)
{
	TAPTR f = *topobject(L, LUA_TEKLOCK);
	if (f == TNULL)
		lua_pushstring(L, "filesystem lock (closed)");
	else
		lua_pushfstring(L, "filesystem lock (%p)", f);
	return 1;
}

static LUACFUNC TINT
luatek_lock_assign(lua_State *L)
{
	TAPTR *p = topobject(L, LUA_TEKLOCK);
	TBOOL success = TIOAssignLock(lua_IOBase, (TSTRPTR) lua_tostring(L, 2), *p);
	if (success) *p = TNULL;	/* lock is now relinquished */
	lua_pushboolean(L, success);
	return 1;
}

static LUACFUNC TINT
luatek_lock_nameof(lua_State *L)
{
	TAPTR lock = *topobject(L, LUA_TEKLOCK);
	TINT len = TIONameOf(lua_IOBase, lock, TNULL, 0);
	if (len >= 0)
	{
		TSTRPTR buf = TExecAlloc(lua_ExecBase, TNULL, len + 1);
		if (buf)
		{
			TIONameOf(lua_IOBase, lock, buf, len + 1);
			lua_pushlstring(L, buf, len);
			TExecFree(lua_ExecBase, buf);
			return 1;
		}
		/* TODO: error */
	}
	lua_pushnil(L);
	return 1;
}

static LUACFUNC TINT
luatek_lock_cdlock(lua_State *L)
{
	TAPTR *nl = TNULL, *ol;
	switch (lua_gettop(L))
	{
		default:
			luaL_error(L, "incorrect number of arguments");
		case 1:
			if (lua_type(L, 1) == LUA_TSTRING)
			{
				TSTRPTR fname = (TSTRPTR) lua_tostring(L, 1);
				nl = luatek_newobject(L, LUA_TEKLOCK);
				*nl = TIOLockFile(lua_IOBase, fname, TFLOCK_READ, TNULL);
				lua_remove(L, -2);
			}
			ol = topobject(L, LUA_TEKLOCK);
			nl = luatek_newobject(L, LUA_TEKLOCK);
			*nl = TIOChangeDir(lua_IOBase, *ol);
			if (*nl) *ol = TNULL; /* relinquish old lock */
			break;
		case 0:
			nl = luatek_newobject(L, LUA_TEKLOCK);
			*nl = TIOLockFile(lua_IOBase, "", TFLOCK_READ, TNULL);
	}
	return (*nl == TNULL) ? luatek_pushresult(L, 0, TNULL) : 1;
}

static LUACFUNC TINT
luatek_lock_makedir(lua_State *L)
{
	TSTRPTR fname = (TSTRPTR) luaL_checkstring(L, 1);
	TAPTR *nl = luatek_newobject(L, LUA_TEKLOCK);
	*nl = TIOMakeDir(lua_IOBase, fname, TNULL);
	return (*nl == TNULL) ? luatek_pushresult(L, 0, TNULL) : 1;
}

static LUACFUNC TINT
luatek_lock_examine(lua_State *L)
{
	TAPTR lock = luatek_toobject(L, LUA_TEKLOCK);
	TTAGITEM extags[5];
	TUINT type = 0, size = 0, sizehi = 0;
	TDATE date;
	lua_Number lsize;

	extags[0].tti_Tag = TFATTR_Type;
	extags[0].tti_Value = (TTAG) &type;
	extags[1].tti_Tag = TFATTR_Size;
	extags[1].tti_Value = (TTAG) &size;
	extags[2].tti_Tag = TFATTR_SizeHigh;
	extags[2].tti_Value = (TTAG) &sizehi;
	extags[3].tti_Tag = TFATTR_Date;
	extags[3].tti_Value = (TTAG) &date;
	extags[4].tti_Tag = TTAG_DONE;

	TIOExamine(lua_IOBase, lock, extags);

	#if defined(TSYS_HAVE_INT64)
		lsize = (lua_Number) (TINT64)
			((((TUINT64) sizehi) << 32) | (TUINT64) size);
	#else
		lsize = size;
	#endif /* defined(TSYS_HAVE_INT64) */

	lua_pushnumber(L, type);
	lua_pushnumber(L, lsize);
	lua_pushnumber(L, (TDOUBLE) date.tdt_Int64 / 86400000000ULL + 2305813.5);

	return 3;
}

static LUACFUNC TINT
luatek_lock_exnext(lua_State *L)
{
	TAPTR lock = luatek_toobject(L, LUA_TEKLOCK);
	TTAGITEM extags[6];
	TSTRPTR name = "";
	TUINT type = 0, size = 0, sizehi = 0;
	TDATE date;

	extags[0].tti_Tag = TFATTR_Name;
	extags[0].tti_Value = (TTAG) &name;
	extags[1].tti_Tag = TFATTR_Type;
	extags[1].tti_Value = (TTAG) &type;
	extags[2].tti_Tag = TFATTR_Size;
	extags[2].tti_Value = (TTAG) &size;
	extags[3].tti_Tag = TFATTR_SizeHigh;
	extags[3].tti_Value = (TTAG) &sizehi;
	extags[4].tti_Tag = TFATTR_Date;
	extags[4].tti_Value = (TTAG) &date;
	extags[5].tti_Tag = TTAG_DONE;

	if (TIOExNext(lua_IOBase, lock, extags) != -1)
	{
		lua_Number lsize;

	#if defined(TSYS_HAVE_INT64)
		lsize = (lua_Number) (TINT64)
			((((TUINT64) sizehi) << 32) | (TUINT64) size);
	#else
		lsize = size;
	#endif /* defined(TSYS_HAVE_INT64) */

		lua_pushstring(L, name);
		lua_pushnumber(L, type);
		lua_pushnumber(L, lsize);
		lua_pushnumber(L, (TDOUBLE) date.tdt_Int64 / 86400000000ULL + 2305813.5);
		return 4;
	}
	else
	{
		return 0;
	}
}

/*****************************************************************************/
/*
**	tek.execute(command, arguments)
*/

static LUACFUNC TINT
luatek_execute (lua_State *L)
{
	TSTRPTR cmd = (TSTRPTR) luaL_optstring(L, 1, TNULL);
	TSTRPTR args = (TSTRPTR) luaL_optstring(L, 2, TNULL);

	/* get lock on current dir */
	struct TIOPacket *cdlock = (struct TIOPacket *)
		TIOLockFile(lua_IOBase, "", TFLOCK_READ, TNULL);
	if (cdlock)
	{
		TSTRPTR namep;
		TTAGITEM locktags[2];
		struct TIOPacket *cmdlock;
		locktags[0].tti_Tag = TIOLock_NamePart;
		locktags[0].tti_Value = (TTAG) &namep;
		locktags[1].tti_Tag = TNULL;

		/* get lock on command */
		cmdlock = (struct TIOPacket *)
			TIOLockFile(lua_IOBase, cmd, TFLOCK_READ, locktags);
		if (cmdlock)
		{
			/* absolute path */
			cdlock->io_Op.Execute.Command = namep;
			cdlock->io_Op.Execute.Flags = 0;
		}
		else
		{
			/* relative path */
			cdlock->io_Op.Execute.Command = cmd;
			cdlock->io_Op.Execute.Flags = 1;
		}

		/* send to handler */
		cdlock->io_Op.Execute.Args = args;
		cdlock->io_Req.io_Command = TIOCMD_EXECUTE;
		TExecDoIO(lua_ExecBase, (struct TIORequest *) cdlock);
		lua_pushinteger(L, cdlock->io_Op.Execute.Result);

		TIOUnlockFile(lua_IOBase, (TFILE *) cmdlock);
		TIOUnlockFile(lua_IOBase, (TFILE *) cdlock);
	}
	else
		lua_pushnil(L);

	return 1;
}

/*****************************************************************************/

static const luaL_Reg luatek_iolib[] =
{
	{"close", luatek_close},
	{"flush", luatek_flush},
	{"input", luatek_input},
	{"lines", luatek_lines},
	{"open", luatek_open},
	{"output", luatek_output},
	{"read", luatek_read},
	{"type", luatek_type},
	{"write", luatek_write},

	{"lock", luatek_lock},
	{"unlock", luatek_lock_unlock},
	{"cdlock", luatek_lock_cdlock},
	{"makedir", luatek_lock_makedir},

	{"delete", luatek_delete},
	{"rename", luatek_rename},
	{"mount", luatek_mount},
	{"addpart", luatek_addpart},
	{"makename", luatek_makename},

	{"clock", luatek_clock},
	{"time", luatek_time},
	{"date", luatek_date},
	{"readargs", luatek_readargs},
	{"execute", luatek_execute},

	/* {"popen", tek_popen}, */
	/* {"tmpfile", tek_tmpfile}, */
	/* {"ieeetofloat", tek_ieeetofloat}, */

	{NULL, NULL}
};

static const luaL_Reg luatek_flib[] =
{
 	{"close", luatek_close},
 	{"flush", luatek_f_flush},
 	{"lines", luatek_f_lines},
 	{"read", luatek_f_read},
 	{"seek", luatek_f_seek},
 	/* {"setvbuf", f_setvbuf}, */
 	{"examine", luatek_f_examine},
 	{"write", luatek_f_write},
 	{"__gc", luatek_gc},
 	{"__tostring", luatek_tostring},
	{NULL, NULL}
};

static const luaL_Reg luatek_locklib[] =
{
 	{"unlock", luatek_lock_unlock},
 	{"assign", luatek_lock_assign},
 	{"examine", luatek_lock_examine},
 	{"exnext", luatek_lock_exnext},
 	{"nameof", luatek_lock_nameof},
	{"__gc", luatek_lock_gc},
 	{"__tostring", luatek_lock_tostring},
	{NULL, NULL}
};

/*****************************************************************************/

static void
luatek_registervars(lua_State *L)
{
	TCHR progname[LUAL_BUFFERSIZE];
	if (TIOMakeName(lua_IOBase, TUtilGetArgV(lua_UtilBase)[0], progname, sizeof(progname),
		TPPF_HOST2TEK, TNULL) > 0)
	{
		/* register tek.progname */
		lua_getfield(L, LUA_GLOBALSINDEX, LUA_TEKLIBNAME);
		lua_pushstring(L, progname);
		lua_setfield(L, 2, "progname");
		lua_pop(L, 1);
	}

	/* register tek.args (all arguments in one string) */
	lua_getfield(L, LUA_GLOBALSINDEX, LUA_TEKLIBNAME);
	lua_pushstring(L, TUtilGetArgs(lua_UtilBase));
	lua_setfield(L, 2, "args");
	lua_pop(L, 1);

}

static void
luatek_createstdfile (lua_State *L, TAPTR f, int k, const char *fname)
{
	*luatek_newobject(L, LUA_TEKFILEHANDLE) = f;
	if (k > 0)
	{
		lua_pushvalue(L, -1);
		lua_rawseti(L, LUA_ENVIRONINDEX, k);
	}
	lua_setfield(L, -2, fname);
}

LUALIB_API TINT
luaopen_tek(lua_State *L)
{

#if defined(TEKLUA)

	/* Global initializations in standalone build */

 	TBaseTask = TEKCreate(TNULL);
 	if (!TBaseTask)
		luaL_error(L, "Failed to initialize TEKlib");

	TExecBase = TGetExecBase(TBaseTask);
	TIOBase = TOpenModule("io", 0, TNULL);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (TIOBase == TNULL || TUtilBase == TNULL)
		luaL_error(L, "Failed to initialze TEKlib modules");

	lua_StdIn = TInputFH();
	lua_StdOut = TOutputFH();
	lua_StdErr = TErrorFH();

#endif /* defined(TEKLUA) */

	/* create/get metatable for file handles */
	luaL_newmetatable(L, LUA_TEKFILEHANDLE);
	/* push duplicate */
	lua_pushvalue(L, -1);
	/* metatable.__index = metatable */
	lua_setfield(L, -2, "__index");
	/* register flib file methods in mt */
	luaL_register(L, NULL, luatek_flib);
	lua_pop(L, 1);


	/* create/get metatable for locks */
	luaL_newmetatable(L, LUA_TEKLOCK);
	/* push duplicate */
	lua_pushvalue(L, -1);
	/* metatable.__index = metatable */
	lua_setfield(L, -2, "__index");
	/* register flib file methods in mt */
	luaL_register(L, NULL, luatek_locklib);
	lua_pop(L, 1);


	/* create new (private) environment */
	lua_newtable(L);
	lua_replace(L, LUA_ENVIRONINDEX);

	/* open library */
	luaL_register(L, LUA_TEKLIBNAME, luatek_iolib);

	/* create (and set) default files */
	luatek_createstdfile(L, lua_StdIn, TIO_INPUT, "stdin");
	luatek_createstdfile(L, lua_StdOut, TIO_OUTPUT, "stdout");
	luatek_createstdfile(L, lua_StdErr, 0, "stderr");

	/* set default close function */
	lua_pushcfunction(L, luatek_fclose);
	lua_setfield(L, LUA_ENVIRONINDEX, "__close");

	luatek_registervars(L);

	return 1;
}
