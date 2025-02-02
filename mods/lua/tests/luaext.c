
/* 
**	Demonstrate Lua language extension
*/

#include <stdio.h>
#include <string.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>
#include <tek/proto/lua.h>

#include "luautil.h"

#define ARG_TEMPLATE "-f=FILE,-a=ARGS/M,-h=HELP/S"
enum { ARG_FILE, ARG_ARGS, ARG_HELP, ARG_NUM };

typedef struct
{
	TAPTR file;
	TUINT8 readbuf[1024];

} global;

TAPTR TExecBase, TUtilBase, TIOBase;

/**************************************************************************
** 
**	average,sum = average(num1, num2, ...)
**	a simple test function with n arguments and 2 results
*/

static LUACFUNC TINT average(lua_State *L)
{
	TINT numargs;
	lua_Number sum = 0;
	TINT i;

	numargs = lua_gettop(L);			/* number of arguments */

	for (i = 1; i <= numargs; i++)
	{
		if (!lua_isnumber(L, i))		/* we want numbers */
		{
			lua_pushstring(L, "incorrect argument to function `average'");
			lua_error(L);
		}
		sum += lua_tonumber(L, i);
	}

	lua_pushnumber(L, sum/numargs);		/* first result */
	lua_pushnumber(L, sum);				/* second result */

	return 2;	/* number of results */
}


/**************************************************************************
**
**	"test" class
*/

static LUACFUNC TINT test_print(lua_State *L)
{
	TDOUBLE *test = luaL_checkudata(L, 1, "Test*");
	printf("*** test.print(): %f\n", *test);
	return 0;
}

static LUACFUNC TINT test_destroy(lua_State *L)
{
	TDOUBLE *test = luaL_checkudata(L, 1, "Test*");

	if (test == TNULL) luaL_argerror(L, 1, "bad handle");

	if (*test)
	{
		printf("test %d has been destroyed or collected\n", (TINT) *test);
		*test = TNULL;		/* mark as closed */
	}

	return 0;
}

/*************************************************************************
**
**	a window "class",
**	consisting of some functions, a window object, and methods.
**	the window is subject to garbage collection also.
*/

static LUACFUNC TINT win_open(lua_State *L)
{
	//global *g = lua_touserdata(L, lua_upvalueindex(1));
	TAPTR *pvis;

	/* reserve real, collectable userdata for a pointer to a visual,
	** and push it on the stack */
	pvis = lua_newuserdata(L, sizeof(TAPTR));			/* stack: userdata */
	*pvis = TNULL;

	/* upvalueindex(2) refers to the class metatable */
	lua_pushvalue(L, lua_upvalueindex(2));				/* stack: userdata, metatable */

	/* attach metatable to the userdata object */
	lua_setmetatable(L, -2);							/* stack: userdata */

	/* create underlying instance */
	*pvis = TOpenModule("visual", 0, TNULL);
	if (*pvis == TNULL)
	{
		/* now there are two possibilities for error treatment.
		** which one fits better is a matter of taste and the
		** general availability of the resource */

		/* 1) this will stop the interpreter, and return
		** an error to the caller of luaT_runchunk() */
		lua_pushstring(L, "could not create window");
		lua_error(L);		/* never returns here */

	#if 0
		/* 2) this will return NIL as the result, and
		** the script continues to run */
		lua_pop(L, 1);		/* remove userdata */
		lua_pushnil(L);		/* push NIL instead */
	#endif
	
	}

	return 1;	/* number of return values on the stack: userdata */
}


static LUACFUNC TINT win_close(lua_State *L)
{
	//global *g = lua_touserdata(L, lua_upvalueindex(1));
	TAPTR *pvis = luaL_checkudata(L, 1, "TEKvisual*");

	if (pvis == TNULL) luaL_argerror(L, 1, "bad handle");

	if (*pvis)
	{
		TCloseModule(*pvis);
		*pvis = TNULL;		/* mark as closed */
	}

	return 0;
}


static LUACFUNC TINT win_gc(lua_State *L)
{
	printf("garbage collector invoked for win handle\n");
	return win_close(L);
}


static LUACFUNC TINT win_newtest(lua_State *L)
{
	TAPTR *pvis = luaL_checkudata(L, 1, "TEKvisual*");
	TDOUBLE *test;
	TINT numargs;

	if (pvis == TNULL) luaL_argerror(L, 1, "closed window handle");

	numargs = lua_gettop(L);				/* number of arguments */
	
	/* we have n+1 arguments here. argument 1 is our
	class object, i.e. the window. the regular arguments can be
	found at index 2,3,4,... */

	if (numargs != 2)
	{
		lua_pushstring(L, "incorrect number of arguments");
		lua_error(L);
	}
	
	if (!lua_isnumber(L, 2))
	{
		lua_pushstring(L, "argument type incorrect");
		lua_error(L);
	}

	/* reserve real, collectable userdata for a pointer to some object,
	** and push it on the stack */

	test = lua_newuserdata(L, sizeof(TDOUBLE));			/* stack: userdata */
	*test = lua_tonumber(L, 2);

	/* now we need the class metatable. if we want to be callable from other
	** classes, we need to address the classtable explicitely. (otherwise
	** it would be sufficient to use lua_pushvalue(L, lua_upvalueindex(2));
	** because our own class metatable is also available as an upvalue) */

	luaL_getmetatable(L, "Test*");						/* stack: userdata, metatable */

	/* attach metatable to the userdata object */
	lua_setmetatable(L, -2);							/* stack: userdata */

	return 1;	/* number of return values on the stack: userdata */
}


/* 
**	tables of methods and functions for the "win" class
*/

static luaL_reg win_functions[] =
{
	{"openwin", win_open},		/* this gives tek.openwin() */
	{"closewin", win_close},	/* this gives tek.closewin() */
	{TNULL, TNULL}
};

static luaL_reg win_methods[] =
{
	{"close", win_close},		/* this gives win:close() */
	{"__gc", win_gc},			/* this enables garbage collection */
	{"newtest", win_newtest},
	{TNULL, TNULL}
};


/* 
**	tables of methods and functions for the "test" class
*/

static luaL_reg test_functions[] =
{
	{TNULL, TNULL}
};

static luaL_reg test_methods[] =
{
	{"print", test_print},
	{"destroy", test_destroy},
	{"__gc", test_destroy},
	{TNULL, TNULL}
};


/* 
**	table of "simple" library functions
*/

static luaL_reg simple_functions[] =
{
	{"average", average},		/* this gives us lib.average() */
	{TNULL, TNULL}
};


/* 
**	Callback to read Lua chunk;
**	in this case from a TEKlib filehandle
*/

static LUACFUNC TSTRPTR readfunc(lua_State *L, struct LuaExecData *e, size_t *size)
{
	global *g = e->led_UserData;
	*size = TRead(g->file, g->readbuf, sizeof(g->readbuf));
	if (*size > 0) return (TSTRPTR) g->readbuf;
	return TNULL;
}


/* 
**	In this callback we extend the language with
**	our own functions.
*/

static LUACFUNC TVOID initextensions(lua_State *L, struct LuaExecData *e)
{
	/* Add a simple function */
	luaT_addfunc(L, "average", average, e->led_UserData);

	/* Add a set of "simple" functions (no class/methods) to the library "tek" */
	luaT_addlib(L, "tek", simple_functions, e->led_UserData);

	/* Add a class "TEKvisual*" (functions and methods) to the library "tek" */
	luaT_addclass(L, "tek", "TEKvisual*", win_functions, win_methods, e->led_UserData);

	/* Add a class "Test*" to the library "tek" */
	luaT_addclass(L, "tek", "Test*", test_functions, test_methods, e->led_UserData);

	/* Add a global variable */
	lua_pushstring(L, "COLOR");
	lua_pushnumber(L, 1);
	lua_settable(L, LUA_GLOBALSINDEX);
}


/* 
**	main
*/

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TINT res = 20;
	global gdata, *g = &gdata;
	lua_State *L;

	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TIOBase = TOpenModule("io", 0, TNULL);
	L = TOpenModule("lua5", 0, TNULL);
	if (TUtilBase && TIOBase && L)
	{
		TSTRPTR *argv = TGetArgV();
		TTAG args[ARG_NUM];
		TAPTR arghandle;
		
		args[ARG_FILE] = (TTAG) "mods/lua/scripts/luaext.lua";
		args[ARG_ARGS] = TNULL;
		args[ARG_HELP] = TFALSE;
		
		res = 10;
		
		arghandle = TParseArgV(ARG_TEMPLATE, argv + 1, args);
		if (args[ARG_HELP] || !arghandle)
		{
			printf("Usage: luaext %s\n", ARG_TEMPLATE);
			printf("Execute Lua script (test for Lua extension)\n\n");
			printf("-f=FROM      : Lua script to run [mods/lua/scripts/luaext.lua]\n");
			printf("-a=ARGS/M    : arguments passed to the script\n");
			printf("-h=HELP/S    : get this help\n");
		}
		else
		{
			g->file = TOpenFile((TSTRPTR) args[ARG_FILE], TFMODE_READONLY, TNULL);
			if (g->file)
			{
				struct LuaExecData chunkexec;
				TINT err;
				
				/* 
				**	initialize LuaExecData block
				*/

				TFillMem(&chunkexec, sizeof(chunkexec), 0);
				chunkexec.led_ChunkName = (TSTRPTR) args[ARG_FILE];		/* name of the chunk executed */
				chunkexec.led_ProgName = (TSTRPTR) args[ARG_FILE];		/* program name, aka argv[0] */
				chunkexec.led_ArgV = (TSTRPTR *) args[ARG_ARGS];		/* argument vector */
				chunkexec.led_ReadFunc = (lua_Chunkreader) readfunc;	/* read function */
				chunkexec.led_InitFunc = initextensions;				/* user init function */
				chunkexec.led_UserData = g;								/* userdata */

				/* set these if you want errors printed */
				chunkexec.led_IOBase = TIOBase;							/* I/O module base */
				chunkexec.led_ErrorFH = TErrorFH();						/* filehandle to print errors to */
				
				err = luaT_runchunk(L, &chunkexec);
				TCloseFile(g->file);

				if (err)
				{
					printf("*** Lua initialization or runtime error.\n");
					res = 10;
				}
				else if (chunkexec.led_Error)
				{
					printf("*** Script error.\n");
					res = 5;
				}
				else
				{
					res = 0;
				}
			}
			else
			{
				printf("*** could not open file: %s\n", (TSTRPTR) args[ARG_FILE]);
			}
		}

		TDestroy(arghandle);
	}
	else
	{
		printf("*** could not open modules.\n");
	}

	TSetRetVal(res);
	if (L) TCloseModule(LUABASE(L));
	TCloseModule(TIOBase);
	TCloseModule(TUtilBase);
}
