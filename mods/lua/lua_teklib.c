
/*
** $Id: lua_teklib.c,v 1.12 2005/09/08 00:02:23 tmueller Exp $
*/

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/time.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

/*****************************************************************************/

#define TEK_FILEHANDLE	"TFILE*"
#define TEKIO_INPUT		"tek_input"
#define TEKIO_OUTPUT	"tek_output"

/*****************************************************************************/

static TINT
pushresult(lua_State *L, TBOOL res, TSTRPTR filename)
{
	if (res)
	{
		lua_pushboolean(L, 1);
		return 1;
	}
	else
	{
		TUINT8 errbuf[80];
		TINT errnum = TGetIOErr();
		TFault(errnum, errbuf, sizeof(errbuf), TNULL);
		lua_pushnil(L);
		if (filename)
			lua_pushfstring(L, "%s: %s", filename, errbuf);
		else
			lua_pushfstring(L, "%s", errbuf);
		lua_pushnumber(L, errnum);
		return 3;
	}
}

/*****************************************************************************/
/* 
**	addclass(L, libname, classname, functable, methodtable, userdata)
*/

static TVOID 
addclass(lua_State *L, TSTRPTR libname, TSTRPTR classname, luaL_reg *functions,
	luaL_reg *methods, TAPTR userdata)
{
	luaL_newmetatable(L, classname);		/* classtab */
	lua_pushliteral(L, "__index");			/* classtab, "__index" */

	/* insert self: classtab.__index = classtab */
	lua_pushvalue(L, -2);					/* classtab, "__index", classtab */
	lua_rawset(L, -3);						/* classtab */

	/* insert methods. consume 1 userdata. do not create a new tab */
	lua_pushlightuserdata(L, userdata);		/* classtab, userdata */
	luaL_openlib(L, TNULL, methods, 1);		/* classtab */

	/* first upvalue: userdata */
	lua_pushlightuserdata(L, userdata);		/* classtab, userdata */

	/* duplicate table argument to be used as second upvalue for cclosure */
	lua_pushvalue(L, -2);					/* classtab, userdata, classtab */

	/* insert functions */
	luaL_openlib(L, libname, functions, 2);	/* classtab, libtab */

	/* adjust stack */	
	lua_pop(L, 2);	
}

/*****************************************************************************/

static TAPTR *
topfile(lua_State *L, TINT findex)
{
	TAPTR *f = (TAPTR *) luaL_checkudata(L, findex, TEK_FILEHANDLE);
	if (f == TNULL) luaL_argerror(L, findex, "bad file");
	return f;
}

static TAPTR
tofile(lua_State *L, TINT findex)
{
	TAPTR *f = topfile(L, findex);
	if (*f == TNULL) luaL_error(L, "attempt to use a closed file");
	return *f;
}

static TAPTR *
newfile(lua_State *L)
{
	TAPTR *pf = (TAPTR *) lua_newuserdata(L, sizeof(TAPTR));
	*pf = TNULL;	/* file handle is currently `closed' */
	luaL_getmetatable(L, TEK_FILEHANDLE);
	lua_setmetatable(L, -2);
	return pf;
}

/*
** assumes that top of the stack is the `tek' library, and next is
** the `tek' metatable
*/

static TVOID
registerfile(lua_State *L, TAPTR f, TSTRPTR name, TSTRPTR impname)
{
	lua_pushstring(L, name);
	*newfile(L) = f;
	if (impname)
	{
		lua_pushstring(L, impname);
		lua_pushvalue(L, -2);
		lua_settable(L, -6);	/* metatable[impname] = file */
	}
	lua_settable(L, -3);		/* io[name] = file */
}

/*****************************************************************************/

static TINT
aux_close(lua_State *L)
{
	TMOD_LUA *mod = (TMOD_LUA *) LUASUPER(L);
	TAPTR f = tofile(L, 1);
	if (f == mod->tml_StdIn || f == mod->tml_StdOut || f == mod->tml_StdErr)
		return 0;	/* file cannot be closed */
	else
	{
		TINT ok = TCloseFile(f);
		if (ok) *(TAPTR *) lua_touserdata(L, 1) = TNULL;	/* mark as closed */
		return ok;
	}
}

static TAPTR
getiofile(lua_State *L, TSTRPTR name)
{
	lua_pushstring(L, name);
	lua_rawget(L, lua_upvalueindex(1));
	return tofile(L, -1);
}

static TINT
g_iofile(lua_State *L, TSTRPTR name, TUINT mode)
{
	if (!lua_isnoneornil(L, 1))
	{
		TSTRPTR filename = (TSTRPTR) lua_tostring(L, 1);
		lua_pushstring(L, name);
		if (filename)
		{
			TAPTR *pf = newfile(L);
			*pf = TOpenFile(filename, mode, TNULL);
			if (*pf == TNULL)
			{
				TUINT8 errbuf[80];
				TINT errnum = TGetIOErr();
				TFault(errnum, errbuf, sizeof(errbuf), TNULL);
				lua_pushfstring(L, "%s: %s", filename, errbuf);
				luaL_argerror(L, 1, lua_tostring(L, -1));
			}
		}
		else
		{
			tofile(L, 1);		/* check that it's a valid file handle */
			lua_pushvalue(L, 1);
		}
		lua_rawset(L, lua_upvalueindex(1));
	}
	/* return current value */
	lua_pushstring(L, name);
	lua_rawget(L, lua_upvalueindex(1));
	return 1;
}

static TINT 
g_write(lua_State *L, TAPTR f, TINT arg)
{
	TINT nargs = lua_gettop(L) - 1;
	TINT status = 1;

	for (; nargs--; arg++)
	{
		if (lua_type(L, arg) == LUA_TNUMBER)
		{
			TUINT8 buffer[20];
			sprintf(buffer, LUA_NUMBER_FMT, lua_tonumber(L, arg));
			status = status && TFWrite(f, buffer, strlen(buffer));
		}
		else
		{
			size_t l;
			TSTRPTR s = (TSTRPTR) luaL_checklstring(L, arg, &l);
			status = status && (TFWrite(f, s, l) == l);
		}
	}
	return pushresult(L, status, TNULL);
}

/*****************************************************************************/
/*
**	read functions
*/

static lua_Number
power10(TINT e)
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
scannum(lua_State *L, TAPTR file, lua_Number *d)
{
	lua_Number result = 0;
	TINT state = 0;
	TUINT a = 0, b = 0;
	TINT nd = 0, e = 0, e2 = 0, sig = 1, esig = 1;
	TINT c;

	if (!file) goto error;

	while (state >= 0 && (c = TFGetC(file)))
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

	result = a + b / power10(nd);
	result *= power10(esig * e + e2);
	result *= sig;
	*d = result;
	return TTRUE;

  error:
	return TFALSE;
}

static TINT
read_number(lua_State *L, TAPTR f)
{
	lua_Number d;
	if (scannum(L, f, &d))
	{
		lua_pushnumber(L, d);
		return 1;
	}
	else
		return 0;				/* read fails */
}

static TINT
test_eof(lua_State *L, TAPTR f)
{
	TINT iseof = TFEoF(f);
	lua_pushlstring(L, NULL, 0);
	return !iseof;
}

static TINT
read_line(lua_State *L, TAPTR f)
{
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	for (;;)
	{
		size_t l;
		TSTRPTR p = luaL_prepbuffer(&b);
		if (TFGetS(f, p, LUAL_BUFFERSIZE) == TNULL)
		{						/* eof? */
			luaL_pushresult(&b);	/* close buffer */
			return (lua_strlen(L, -1) > 0);	/* check whether read something */
		}
		l = strlen(p);
		if (p[l - 1] != '\n')
			luaL_addsize(&b, l);
		else
		{
			luaL_addsize(&b, l - 1);	/* do not include `eol' */
			luaL_pushresult(&b);	/* close buffer */
			return 1;			/* read at least an `eol' */
		}
	}
}

static TINT
read_chars(lua_State *L, TAPTR f, size_t n)
{
	size_t rlen;				/* how much to read */
	size_t nr;					/* number of chars actually read */
	luaL_Buffer b;

	luaL_buffinit(L, &b);
	rlen = LUAL_BUFFERSIZE;		/* try to read that much each time */
	do
	{
		TSTRPTR p = luaL_prepbuffer(&b);

		if (rlen > n)
			rlen = n;			/* cannot read more than asked */
		nr = TFRead(f, p, rlen);
		luaL_addsize(&b, nr);
		n -= nr;				/* still have to read `n' chars */
	} while (n > 0 && nr == rlen);	/* until end of count or eof */
	luaL_pushresult(&b);		/* close buffer */
	return (n == 0 || lua_strlen(L, -1) > 0);
}

static TINT
g_read(lua_State *L, TAPTR f, TINT first)
{
	TINT nargs = lua_gettop(L) - 1;
	TINT success;
	TINT n;

	if (nargs == 0)
	{							/* no arguments? */
		success = read_line(L, f);
		n = first + 1;			/* to return 1 result */
	}
	else
	{							/* ensure stack space for all results and for auxlib's buffer */
		luaL_checkstack(L, nargs + LUA_MINSTACK, "too many arguments");
		success = 1;
		for (n = first; nargs-- && success; n++)
		{
			if (lua_type(L, n) == LUA_TNUMBER)
			{
				size_t l = (size_t) lua_tonumber(L, n);
				success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
			}
			else
			{
				TSTRPTR p = (TSTRPTR) lua_tostring(L, n);
				luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
				switch (p[1])
				{
					case 'n':	/* number */
						success = read_number(L, f);
						break;
					case 'l':	/* line */
						success = read_line(L, f);
						break;
					case 'a':	/* file */
						read_chars(L, f, ~((size_t) 0));	/* read MAX_SIZE_T chars */
						success = 1;	/* always success */
						break;
					case 'w':	/* word */
						return luaL_error(L,
							"obsolete option `*w' to `read'");
					default:
						return luaL_argerror(L, n, "invalid format");
				}
			}
		}
	}
	if (!success)
	{
		lua_pop(L, 1);			/* remove last result */
		lua_pushnil(L);			/* push nil instead */
	}
	return n - first;
}

static LUACFUNC TINT 
aux_readline(lua_State *L)
{
	TAPTR f = *(TAPTR *) lua_touserdata(L, lua_upvalueindex(2));
	if (f == TNULL)				/* file is already closed? */
		luaL_error(L, "file is already closed");
	if (read_line(L, f))
		return 1;
	else
	{							/* EOF */
		if (lua_toboolean(L, lua_upvalueindex(3)))
		{						/* generator created file? */
			lua_settop(L, 0);
			lua_pushvalue(L, lua_upvalueindex(2));
			aux_close(L);		/* close it */
		}
		return 0;
	}
}

static TVOID
aux_lines(lua_State *L, TINT idx, TINT close)
{
	lua_pushliteral(L, TEK_FILEHANDLE);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, idx);
	lua_pushboolean(L, close);	/* close/not close file when finished */
	lua_pushcclosure(L, aux_readline, 3);
}

/*****************************************************************************/

static LUACFUNC TINT
f_lines(lua_State * L)
{
	tofile(L, 1);				/* check that it's a valid file handle */
	aux_lines(L, 1, 0);
	return 1;
}

static LUACFUNC TINT
f_read(lua_State * L)
{
	return g_read(L, tofile(L, 1), 2);
}

static LUACFUNC TINT
f_flush(lua_State *L)
{
	return pushresult(L, TFlush(tofile(L, 1)) == 0, TNULL);
}

static LUACFUNC TINT
f_write(lua_State *L)
{
	return g_write(L, tofile(L, 1), 2);
}

static LUACFUNC TINT
f_seek(lua_State *L)
{
	static const TINT mode[] = { TFPOS_BEGIN, TFPOS_CURRENT, TFPOS_END };
	static const char *modenames[] = { "set", "cur", "end", NULL };
	TAPTR f = tofile(L, 1);
	TINT op = luaL_findstring(luaL_optstring(L, 2, "cur"), modenames);
	long offset = luaL_optlong(L, 3, 0);
	TUINT offs;

	luaL_argcheck(L, op != -1, 2, "invalid mode");
	offs = TSeek(f, offset, TNULL, mode[op]);
	if (offs == 0xffffffff)
		return pushresult(L, 0, NULL);	/* error */
	else
	{
		lua_pushnumber(L, offs);
		return 1;
	}
}

static LUACFUNC TINT
f_examine(lua_State *L)
{
	TAPTR file = tofile(L, 1);
	TTAGITEM extags[3];
	TUINT type = 0, size = 0;
	TDATE date;
	
	extags[0].tti_Tag = TFATTR_Type;
	extags[0].tti_Value = (TTAG) &type;
	extags[1].tti_Tag = TFATTR_Size;
	extags[1].tti_Value = (TTAG) &size;
	extags[2].tti_Tag = TFATTR_Date;
	extags[2].tti_Value = (TTAG) &date;
	extags[3].tti_Tag = TTAG_DONE;
	
	TExamine(file, extags);

	lua_pushnumber(L, type);
	lua_pushnumber(L, size);
	lua_pushnumber(L, TDateToJulian(&date));

	return 3;
}

/*****************************************************************************/

static LUACFUNC TINT 
tek_clock(lua_State *L)
{
	TTIME elapsed;
	lua_Number t;
	TQueryTime(LUABASE(L)->tlu_TimeRequest, &elapsed);
	t = (lua_Number) elapsed.ttm_USec / 1000000 + elapsed.ttm_Sec;
	lua_pushnumber(L, t);
	return 1;
}

static int 
getfield(lua_State *L, const char *key, int d)
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

static void setfield (lua_State *L, const char *key, int value) {
  lua_pushinteger(L, value);
  lua_setfield(L, -2, key);
}

static LUACFUNC TINT
tek_time(lua_State *L)
{
	TDATE date;
	TDOUBLE t = -1;
	TINT error = 0;

	if (lua_isnoneornil(L, 1)) /* called without args? */
	{
		TDATE date;
		error = TGetDate(LUABASE(L)->tlu_TimeRequest, &date, TNULL);
		if (error == 0) t = TDateToJulian(&date);
	}
	else
	{
		struct TDateBox db;

		luaL_checktype(L, 1, LUA_TTABLE);
		lua_settop(L, 1);  /* make sure table is at the top */

		db.tdb_Fields = TDB_USEC | TDB_SEC | TDB_MINUTE | TDB_HOUR;

		db.tdb_USec = getfield(L, "usec", 0);
		db.tdb_Sec = getfield(L, "sec", 0);
		db.tdb_Minute = getfield(L, "min", 0);
		db.tdb_Hour = getfield(L, "hour", 12);

		db.tdb_Day = getfield(L, "day", 0);
		if (db.tdb_Day > 0) db.tdb_Fields |= TDB_DAY;

		db.tdb_Month = getfield(L, "month", 0);
		if (db.tdb_Month > 0) db.tdb_Fields |= TDB_MONTH;

		db.tdb_Year = getfield(L, "year", -4714);
		if (db.tdb_Year >= -4713) db.tdb_Fields |= TDB_YEAR;
		
		if (TPackDate(&db, &date))
		{
			error = 0;
			t = TDateToJulian(&date);
		}
	}
	
	if (error)
	{
		lua_pushnil(L);
	}
	else
	{
		lua_pushnumber(L, t);
	}
	
	return 1;
}

static LUACFUNC TINT
tek_date(lua_State *L)
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
			TINT tzsec;
			error = TGetDate(LUABASE(L)->tlu_TimeRequest, &date, &tzsec);
		}
		else
		{
			/* local time. TODO: incorporate Daylight Saving */
			error = TGetDate(LUABASE(L)->tlu_TimeRequest, &date, TNULL);
		}
	}
	else
	{
		TJulianToDate(n, &date);
	}

	if (error)
	{
		lua_pushnil(L);
	}
	else
	{
		struct TDateBox db;
		TUnpackDate(&date, &db, TDB_ALL);
		if (TStrCmp(s, "*t") == 0) /* table */
		{
			lua_createtable(L, 0, 8);  /* 8 = number of fields */
			setfield(L, "sec", db.tdb_Sec);
			setfield(L, "min", db.tdb_Minute);
			setfield(L, "hour", db.tdb_Hour);
			setfield(L, "day", db.tdb_Day);
			setfield(L, "month", db.tdb_Month);
			setfield(L, "year", db.tdb_Year);
			setfield(L, "wday", db.tdb_WDay);
			setfield(L, "yday", db.tdb_YDay);
			/*setboolfield(L, "isdst", stm->tm_isdst);*/
		}
		else /* format to string */
		{
			if (TStrCmp(s, "%c") == 0)
			{
				static const char *dn[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
				static const char *mn[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
				char b[256];
				sprintf(b, "%s %s % 2d %02d:%02d:%02d %d",
					dn[db.tdb_WDay - 1], mn[db.tdb_Month - 1], db.tdb_Day,
					db.tdb_Hour, db.tdb_Minute, db.tdb_Sec, db.tdb_Year);
				lua_pushstring(L, b); /* e.g. Wed Jul  6 02:09:49 2005 */
			}
			else
			{
				luaL_error(L, "Cannot handle date format");
			}
		}	
	}

	return 1;
}


/*****************************************************************************/
/* 
**	TEK I/O
*/

static LUACFUNC TINT
tek_type(lua_State * L)
{
	TAPTR *f = (TAPTR *) luaL_checkudata(L, 1, TEK_FILEHANDLE);
	if (f == NULL)
		lua_pushnil(L);
	else if (*f == TNULL)
		lua_pushliteral(L, "closed file");
	else
		lua_pushliteral(L, "file");
	return 1;
}

static LUACFUNC TINT
tek_close(lua_State *L)
{
	if (lua_isnone(L, 1) && lua_type(L, lua_upvalueindex(1)) == LUA_TTABLE)
	{
		lua_pushstring(L, TEKIO_OUTPUT);
		lua_rawget(L, lua_upvalueindex(1));
	}
	return pushresult(L, aux_close(L), TNULL);
}

static LUACFUNC TINT
tek_gc(lua_State *L)
{
	TAPTR *f = topfile(L, 1);
	if (*f != TNULL) aux_close(L);
	return 0;
}

static LUACFUNC TINT
tek_tostring(lua_State *L)
{
	TUINT8 buff[128];
	TAPTR *f = topfile(L, 1);
	if (*f == TNULL)
		strcpy(buff, "closed");
	else
		sprintf(buff, "%p", lua_touserdata(L, 1));
	lua_pushfstring(L, "file (%s)", buff);
	return 1;
}

static LUACFUNC TINT
tek_open(lua_State *L)
{
	TSTRPTR filename = (TSTRPTR) luaL_checkstring(L, 1);
	TSTRPTR mode = (TSTRPTR) luaL_optstring(L, 2, "r");
	TAPTR *pf = (TAPTR *) newfile(L);
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
			if (*mode == 'b') mode++;
			if (*mode == '+')
			{
				tekmode = TFMODE_READWRITE;
				break;
			}
		
		default:
			tekmode = TFMODE_READONLY;
			break;
	}

	*pf = TOpenFile(filename, tekmode, TNULL);
	if (*pf)
	{
		if (append) TSeek(*pf, 0, TNULL, TFPOS_END);
		return 1;
	}
	
	return pushresult(L, 0, filename);
}

static LUACFUNC TINT
tek_input(lua_State *L)
{
	return g_iofile(L, TEKIO_INPUT, TFMODE_READONLY);
}

static LUACFUNC TINT
tek_output(lua_State *L)
{
	return g_iofile(L, TEKIO_OUTPUT, TFMODE_NEWFILE);
}

static LUACFUNC TINT
tek_lines(lua_State *L)
{
	if (lua_isnoneornil(L, 1))	/* no arguments? */
	{
		lua_pushstring(L, TEKIO_INPUT);
		lua_rawget(L, lua_upvalueindex(1));	/* will iterate over default input */
		return f_lines(L);
	}
	else
	{
		TSTRPTR filename = (TSTRPTR) luaL_checkstring(L, 1);
		TUINT8 errbuf[80];
		TINT errnum;
		TAPTR *pf = newfile(L);
		*pf = TOpenFile(filename, TFMODE_READONLY, TNULL);
		errnum = TGetIOErr();
		TFault(errnum, errbuf, sizeof(errbuf), TNULL);
		luaL_argcheck(L, *pf, 1, errbuf);
		aux_lines(L, lua_gettop(L), 1);
		return 1;
	}
}

static LUACFUNC TINT
tek_read(lua_State *L)
{
	return g_read(L, getiofile(L, TEKIO_INPUT), 1);
}

static LUACFUNC TINT
tek_write(lua_State *L)
{
	return g_write(L, getiofile(L, TEKIO_OUTPUT), 1);
}

static LUACFUNC TINT
tek_flush(lua_State *L)
{
	return pushresult(L, TFlush(getiofile(L, TEKIO_OUTPUT)) == 0, TNULL);
}

static LUACFUNC TINT
tek_delete(lua_State *L)
{
	TSTRPTR filename = (TSTRPTR) luaL_checkstring(L, 1);
	return pushresult(L, TDeleteFile((TSTRPTR) filename), filename);
}

static LUACFUNC TINT
tek_rename(lua_State *L)
{
	TSTRPTR fromname = (TSTRPTR) luaL_checkstring(L, 1);
	TSTRPTR toname = (TSTRPTR) luaL_checkstring(L, 2);
	return pushresult(L, TRename((TSTRPTR) fromname,
		(TSTRPTR) toname), fromname);
}

/*****************************************************************************/

static LUACFUNC TINT 
tek_addpart(lua_State *L)
{
	TINT numargs = lua_gettop(L);
	TSTRPTR part1, part2;
	TINT len;
	
	if (numargs != 2)
	{
		lua_pushstring(L, "incorrect number of arguments");
		lua_error(L);
	}

	part1 = (TSTRPTR) lua_tostring(L, 1);	
	part2 = (TSTRPTR) lua_tostring(L, 2);
	
	len = TAddPart(part1, part2, TNULL, 0) + 1;
	if (len > 0)
	{
		TSTRPTR buf = TAlloc(TNULL, len);
		if (buf)
		{
			TAddPart(part1, part2, buf, len);
			lua_pushstring(L, buf);
			TFree(buf);
		}
		else
		{
			lua_pushstring(L, "out of memory");
			lua_error(L);
		}
	}
	else
	{
		lua_pushnil(L);
	}
	
	return 1;
}	

static LUACFUNC TINT 
tek_mount(lua_State *L)
{
	TINT numargs = lua_gettop(L);
	TSTRPTR mountname;
	TSTRPTR handlername;
	TSTRPTR initstring = TNULL;
	TTAGITEM tags[3];

	switch (numargs)
	{
		default:
			lua_pushstring(L, "incorrect number of arguments");
			lua_error(L);

		case 3:
			initstring = (TSTRPTR) lua_tostring(L, 3);

		case 2:
			break;
	}
	
	mountname = (TSTRPTR) lua_tostring(L, 1);
	handlername = (TSTRPTR) lua_tostring(L, 2);

	tags[0].tti_Tag = TIOMount_Handler;
	tags[0].tti_Value = (TTAG) handlername;
	tags[1].tti_Tag = TIOMount_InitString;
	tags[1].tti_Value = (TTAG) initstring;
	tags[2].tti_Tag = TTAG_DONE;

	lua_pushboolean(L, TMount(mountname, TIOMNT_ADD, tags));

	return 1;
}	

static LUACFUNC TINT 
tek_makename(lua_State *L)
{
	TINT numargs = lua_gettop(L);
	TSTRPTR hostname;
	TINT len;

	if (numargs != 1)
	{
		lua_pushstring(L, "incorrect number of arguments");
		lua_error(L);
	}
	
	hostname = (TSTRPTR) lua_tostring(L, 1);
	
	len = TMakeName(hostname, TNULL, 0, TPPF_HOST2TEK, TNULL) + 1;
	if (len > 0)
	{
		TSTRPTR destname = TAlloc(TNULL, len);
		if (destname)
		{
			TMakeName(hostname, destname, len, TPPF_HOST2TEK, TNULL);
			lua_pushstring(L, destname);
			TFree(destname);
			return 1;
		}
		else
		{
			lua_pushstring(L, "out of memory");
			lua_error(L);
		}
	}

	return 0;
}	

static LUACFUNC TINT 
tek_readargs(lua_State *L)
{
	TINT numargs = lua_gettop(L);
	TSTRPTR tmpl;
	TSTRPTR *argv;
	TTAG *args;
	TAPTR argh;
	TINT8 *argt;
	TINT i, j, c, numret, numtmp;
	TSTRPTR p;
	
	if (numargs == 0)
	{
		lua_pushstring(L, "not enough arguments");
		lua_error(L);
	}
	
	tmpl = (TSTRPTR) lua_tostring(L, 1);
	
	p = tmpl;
	numtmp = 1;
	while ((c = *p++))
		if (c == ',') numtmp++;
	
	argv = TAlloc(TNULL, numargs * sizeof(*argv));
	args = TAlloc0(TNULL, numtmp * sizeof(*args));
	argt = TAlloc0(TNULL, numtmp * sizeof(*argt));
	
	if (argv == TNULL || args == TNULL || argt == TNULL)
	{
		TFree(argt);
		TFree(args);
		TFree(argv);
		lua_pushstring(L, "out of memory");
		lua_error(L);
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

	argh = TParseArgV(tmpl, argv, args);
	if (argh)
	{
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
		numret = numtmp;
	}
	else
	{
		lua_pushnil(L);
		numret = 1;
	}
	
	TFree(argt);
	TFree(args);
	TFree(argv);
	
	return numret;
}

/*****************************************************************************/
/*
**	hack to access IEEE numbers in memory;
**	depends on native float type to be IEEE-conformant
*/

static LUACFUNC TINT 
tek_ieeetofloat(lua_State *L)
{
	TINT numargs = lua_gettop(L);
	
	union {
		TUINT s;
		TFLOAT f;
	} n;

	if (numargs != 1)
	{
		lua_pushstring(L, "incorrect number of arguments");
		lua_error(L);
	}

	if (lua_strlen(L, 1) != 4)
	{
		lua_pushstring(L, "requires string of 4 bytes length");
		lua_error(L);
	}
	
	n.s = THToNL(*(TUINT *) lua_tostring(L, 1));
	
	lua_pushnumber(L, (lua_Number) n.f);
	
	return 1;
}

/*****************************************************************************/

static const luaL_reg teklib[] = {
	{"input", tek_input},
	{"output", tek_output},
	{"lines", tek_lines},
	{"close", tek_close},
	{"flush", tek_flush},
	{"open", tek_open},
	{"read", tek_read},
	{"type", tek_type},
	{"write", tek_write},
	{"delete", tek_delete},
	{"rename", tek_rename},

	{"clock", tek_clock},
	{"time", tek_time},
	{"date", tek_date},
	
	{"addpart", tek_addpart},
	{"mount", tek_mount},
	{"makename", tek_makename},
	{"readargs", tek_readargs},
	
	{"ieeetofloat", tek_ieeetofloat},		/* 4 bytes to number */
	
	{NULL, NULL}
};

static const luaL_reg flib[] = {
	{"flush", f_flush},
	{"read", f_read},
	{"lines", f_lines},
	{"seek", f_seek},
	{"write", f_write},
	{"close", tek_close},
	{"examine", f_examine},
	{"__gc", tek_gc},
	{"__tostring", tek_tostring},
	{NULL, NULL}
};

/*****************************************************************************/
/* 
**	TEK Lock Class
*/

#define CLASSNAME_LOCK	"FileLock*"

static TAPTR * 
getObjectPtr(lua_State *L, TSTRPTR classname, TINT numargs)
{
	TAPTR *p;

	if (lua_gettop(L) != numargs)
	{
		lua_pushstring(L, "incorrect number of arguments");
		lua_error(L);
	}

	p = luaL_checkudata(L, 1, classname);
	if (p == TNULL)
	{
		luaL_argerror(L, 1, "invalid handle");
	}
	
	return p;
}

static TAPTR
getObject(lua_State *L, TSTRPTR classname, TINT numargs)
{
	TAPTR *p = getObjectPtr(L, classname, numargs);
	if (*p == TNULL) luaL_argerror(L, 1, "closed handle");
	return *p;
}

static LUACFUNC TINT
lock_lock(lua_State *L)
{
	/*Global *g = lua_touserdata(L, lua_upvalueindex(1));*/
	TAPTR *pobj;
	TINT numargs;
	TSTRPTR fname;
	TUINT mode = TFLOCK_READ;

	numargs = lua_gettop(L);
	if (numargs == 2)
	{
		mode = lua_toboolean(L, 2) ? TFLOCK_READ : TFLOCK_WRITE;
	}
	else if (numargs != 1)
	{
		lua_pushstring(L, "incorrect number of arguments");
		lua_error(L);
	}
	
	fname = (TSTRPTR) lua_tostring(L, 1);

	/* reserve real, collectable userdata for a pointer,
		and push it on the stack */

	pobj = lua_newuserdata(L, sizeof(TAPTR));			/* stack: userdata */

	/* upvalueindex(2) refers to the class metatable */
	lua_pushvalue(L, lua_upvalueindex(2));				/* stack: userdata, metatable */

	/* attach metatable to the userdata object */
	lua_setmetatable(L, -2);							/* stack: userdata */

	/* create underlying object */
	*pobj = TLockFile(fname, mode, TNULL);
	if (*pobj == TNULL)
	{
		lua_pop(L, 1);		/* remove userdata */
		lua_pushnil(L);		/* replace with NIL */
	}

	return 1;	/* number of return values on the stack: userdata */
}

static LUACFUNC TINT
lock_cdlock(lua_State *L)
{
	/*Global *g = lua_touserdata(L, lua_upvalueindex(1));*/
	TAPTR *pobj;
	TINT numargs;
	TAPTR *poldlock = TNULL;

	numargs = lua_gettop(L);
	if (numargs == 1)
	{
		poldlock = getObjectPtr(L, CLASSNAME_LOCK, 1);
		if (*poldlock == TNULL) luaL_argerror(L, 1, "closed handle");
	}
	else if (numargs != 0)
	{
		lua_pushstring(L, "incorrect number of arguments");
		lua_error(L);
	}
	
	/* reserve real, collectable userdata for a pointer,
		and push it on the stack */

	pobj = lua_newuserdata(L, sizeof(TAPTR));			/* stack: userdata */

	/* upvalueindex(2) refers to the class metatable */
	lua_pushvalue(L, lua_upvalueindex(2));				/* stack: userdata, metatable */

	/* attach metatable to the userdata object */
	lua_setmetatable(L, -2);							/* stack: userdata */

	/* create underlying object */
	
	if (poldlock)
	{
		*pobj = TChangeDir(*poldlock);
		if (*pobj)
		{
			*poldlock = TNULL;		/* mark as closed */
		}
	}
	else
	{
		*pobj = TLockFile("", TFLOCK_READ, TNULL);
	}
	
	if (*pobj == TNULL)
	{
		lua_pop(L, 1);		/* remove userdata */
		lua_pushnil(L);		/* replace with NIL */
	}

	return 1;	/* number of return values on the stack: userdata */
}

static LUACFUNC TINT
lock_makedir(lua_State *L)
{
	/*Global *g = lua_touserdata(L, lua_upvalueindex(1));*/
	TAPTR *pobj;
	TINT numargs;
	TSTRPTR fname;

	numargs = lua_gettop(L);
	if (numargs != 1)
	{
		lua_pushstring(L, "incorrect number of arguments");
		lua_error(L);
	}
	
	fname = (TSTRPTR) lua_tostring(L, 1);

	/* reserve real, collectable userdata for a pointer,
		and push it on the stack */

	pobj = lua_newuserdata(L, sizeof(TAPTR));			/* stack: userdata */

	/* upvalueindex(2) refers to the class metatable */
	lua_pushvalue(L, lua_upvalueindex(2));				/* stack: userdata, metatable */

	/* attach metatable to the userdata object */
	lua_setmetatable(L, -2);							/* stack: userdata */

	/* create underlying object */
	*pobj = TMakeDir(fname, TNULL);
	if (*pobj == TNULL)
	{
		lua_pop(L, 1);		/* remove userdata */
		lua_pushnil(L);		/* replace with NIL */
	}

	return 1;	/* number of return values on the stack: userdata */
}

static LUACFUNC TINT
lock_unlock(lua_State *L)
{
	TAPTR *plock = getObjectPtr(L, CLASSNAME_LOCK, 1);
	if (*plock)
	{
		TUnlockFile(*plock);
		*plock = TNULL;	/* mark as closed */
	}
	return 0;
}

static LUACFUNC TINT
lock_assign(lua_State *L)
{
	TBOOL success;
	TAPTR *plock = getObjectPtr(L, CLASSNAME_LOCK, 2);
	if (*plock == TNULL) luaL_argerror(L, 1, "closed handle");
	success = TAssignLock((TSTRPTR) lua_tostring(L, 2), *plock);
	if (success) *plock = TNULL;	/* lock is now relinquished */
	lua_pushboolean(L, success);
	return 1;
}

static LUACFUNC TINT
lock_examine(lua_State *L)
{
	TAPTR lock = getObject(L, CLASSNAME_LOCK, 1);
	TTAGITEM extags[3];
	TUINT type = 0, size = 0;
	TDATE date;
	
	extags[0].tti_Tag = TFATTR_Type;
	extags[0].tti_Value = (TTAG) &type;
	extags[1].tti_Tag = TFATTR_Size;
	extags[1].tti_Value = (TTAG) &size;
	extags[2].tti_Tag = TFATTR_Date;
	extags[2].tti_Value = (TTAG) &date;
	extags[3].tti_Tag = TTAG_DONE;
	
	TExamine(lock, extags);

	lua_pushnumber(L, type);
	lua_pushnumber(L, size);
	lua_pushnumber(L, TDateToJulian(&date));

	return 3;
}

static LUACFUNC TINT
lock_exnext(lua_State *L)
{
	TAPTR lock = getObject(L, CLASSNAME_LOCK, 1);
	TTAGITEM extags[4];
	TSTRPTR name = "";
	TUINT type = 0, size = 0;
	TDATE date;
	
	extags[0].tti_Tag = TFATTR_Name;
	extags[0].tti_Value = (TTAG) &name;
	extags[1].tti_Tag = TFATTR_Type;
	extags[1].tti_Value = (TTAG) &type;
	extags[2].tti_Tag = TFATTR_Size;
	extags[2].tti_Value = (TTAG) &size;
	extags[3].tti_Tag = TFATTR_Date;
	extags[3].tti_Value = (TTAG) &date;
	extags[4].tti_Tag = TTAG_DONE;
	
	if (TExNext(lock, extags) != -1)
	{
		lua_pushstring(L, name);
		lua_pushnumber(L, type);
		lua_pushnumber(L, size);
		lua_pushnumber(L, TDateToJulian(&date));
		return 4;
	}
	else
	{
		return 0;
	}
}

static LUACFUNC TINT
lock_nameof(lua_State *L)
{
	TUINT8 buf[1024];
	TAPTR lock = getObject(L, CLASSNAME_LOCK, 1);
	if (TNameOf(lock, buf, 1024) >= 0)
	{
		lua_pushstring(L, buf);
	}
	else
	{
		lua_pushnil(L);
	}
	return 1;
}

static luaL_reg lock_functions[] =
{
	{"lock", lock_lock},
	{"unlock", lock_unlock},
	{"cdlock", lock_cdlock},
	{"makedir", lock_makedir},
	{TNULL, TNULL}
};

static luaL_reg lock_methods[] =
{
	{"unlock", lock_unlock},
	{"assign", lock_assign},
	{"examine", lock_examine},
	{"exnext", lock_exnext},
	{"nameof", lock_nameof},
	{"__gc", lock_unlock},
	{TNULL, TNULL}
};

/*****************************************************************************/

static TVOID
createmeta(lua_State *L)
{
	luaL_newmetatable(L, TEK_FILEHANDLE);	/* create new metatable for file handles */
	/* file methods */
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);		/* push metatable */
	lua_rawset(L, -3);			/* metatable.__index = metatable */
	luaL_openlib(L, TNULL, flib, 0);
}

LUALIB_API TINT
luaopen_tek(lua_State *L)
{
	TMOD_LUA *mod = (TMOD_LUA *) LUASUPER(L);
 	createmeta(L);
 	lua_pushvalue(L, -1);
	luaL_openlib(L, LUA_TEKLIBNAME, teklib, 1);
	registerfile(L, mod->tml_StdIn, "stdin", TEKIO_INPUT);
	registerfile(L, mod->tml_StdOut, "stdout", TEKIO_OUTPUT);
	registerfile(L, mod->tml_StdErr, "stderr", NULL);

	addclass(L, "tek", CLASSNAME_LOCK, lock_functions, lock_methods, TNULL);

	return 1;
}





#if 0

/*****************************************************************************/
/*****************************************************************************/

#if 0
static LUACFUNC TINT
io_execute(lua_State *L)
{
	lua_pushnumber(L, system(luaL_checkstring(L, 1)));
	return 1;
}
#endif


static LUACFUNC TINT
tek_getenv(lua_State * L)
{
	lua_pushstring(L, getenv(luaL_checkstring(L, 1)));	/* if NULL push nil */
	return 1;
}

/*****************************************************************************/

static void setfield(lua_State * L, const char *key, int value)
{
	lua_pushstring(L, key);
	lua_pushnumber(L, value);
	lua_rawset(L, -3);
}

static void setboolfield(lua_State * L, const char *key, int value)
{
	lua_pushstring(L, key);
	lua_pushboolean(L, value);
	lua_rawset(L, -3);
}

static int getboolfield(lua_State * L, const char *key)
{
	int res;

	lua_pushstring(L, key);
	lua_gettable(L, -2);
	res = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return res;
}

static int getfield(lua_State * L, const char *key, int d)
{
	int res;

	lua_pushstring(L, key);
	lua_gettable(L, -2);
	if (lua_isnumber(L, -1))
		res = (int) (lua_tonumber(L, -1));
	else
	{
		if (d == -2)
			return luaL_error(L, "field `%s' missing in date table", key);
		res = d;
	}
	lua_pop(L, 1);
	return res;
}

static LUACFUNC int tek_date(lua_State * L)
{
	const char *s = luaL_optstring(L, 1, "%c");
	time_t t = (time_t) (luaL_optnumber(L, 2, -1));
	struct tm *stm;

	if (t == (time_t) (-1))		/* no time given? */
		t = time(NULL);			/* use current time */
	if (*s == '!')
	{							/* UTC? */
		stm = gmtime(&t);
		s++;					/* skip `!' */
	}
	else
		stm = localtime(&t);
	if (stm == NULL)			/* invalid date? */
		lua_pushnil(L);
	else if (strcmp(s, "*t") == 0)
	{
		lua_newtable(L);
		setfield(L, "sec", stm->tm_sec);
		setfield(L, "min", stm->tm_min);
		setfield(L, "hour", stm->tm_hour);
		setfield(L, "day", stm->tm_mday);
		setfield(L, "month", stm->tm_mon + 1);
		setfield(L, "year", stm->tm_year + 1900);
		setfield(L, "wday", stm->tm_wday + 1);
		setfield(L, "yday", stm->tm_yday + 1);
		setboolfield(L, "isdst", stm->tm_isdst);
	}
	else
	{
		char b[256];

		if (strftime(b, sizeof(b), s, stm))
			lua_pushstring(L, b);
		else
			return luaL_error(L, "`date' format too long");
	}
	return 1;
}

static LUACFUNC int tek_time(lua_State * L)
{
	if (lua_isnoneornil(L, 1))	/* called without args? */
		lua_pushnumber(L, time(NULL));	/* return current time */
	else
	{
		time_t t;
		struct tm ts;

		luaL_checktype(L, 1, LUA_TTABLE);
		lua_settop(L, 1);		/* make sure table is at the top */
		ts.tm_sec = getfield(L, "sec", 0);
		ts.tm_min = getfield(L, "min", 0);
		ts.tm_hour = getfield(L, "hour", 12);
		ts.tm_mday = getfield(L, "day", -2);
		ts.tm_mon = getfield(L, "month", -2) - 1;
		ts.tm_year = getfield(L, "year", -2) - 1900;
		ts.tm_isdst = getboolfield(L, "isdst");
		t = mktime(&ts);
		if (t == (time_t) (-1))
			lua_pushnil(L);
		else
			lua_pushnumber(L, t);
	}
	return 1;
}

static LUACFUNC int tek_difftime(lua_State * L)
{
	lua_pushnumber(L, difftime((time_t) (luaL_checknumber(L, 1)),
			(time_t) (luaL_optnumber(L, 2, 0))));
	return 1;
}

/*****************************************************************************/

static LUACFUNC int tek_setloc(lua_State * L)
{
	static const int cat[] = { LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY,
		LC_NUMERIC, LC_TIME
	};
	static const char *const catnames[] =
		{ "all", "collate", "ctype", "monetary",
		"numeric", "time", NULL
	};
	const char *l = lua_tostring(L, 1);
	int op = luaL_findstring(luaL_optstring(L, 2, "all"), catnames);

	luaL_argcheck(L, l || lua_isnoneornil(L, 1), 1, "string expected");
	luaL_argcheck(L, op != -1, 2, "invalid option");
	lua_pushstring(L, setlocale(cat[op], l));
	return 1;
}

static const luaL_reg syslib[] = {
	
//	{"date", tek_date},
//	{"difftime", tek_difftime},
//	{"getenv", tek_getenv},
//	{"setlocale", tek_setloc},
//	{"time", tek_time},
	{TNULL, TNULL}
};

#endif

