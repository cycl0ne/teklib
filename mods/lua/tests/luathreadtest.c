
/* 
**	Demonstrate a Lua script running in a thread of its own
**	- use with scripts/dots.lua
*/

#include <stdio.h>
#include <string.h>
#include <tek/teklib.h>
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>
#include <tek/inline/io.h>
#include <tek/proto/visual.h>
#include <tek/proto/lua.h>

#include "luautil.h"

#define ARG_TEMPLATE "-f=FILE,-a=ARGS/M,-h=HELP/S"
enum { ARG_FILE, ARG_ARGS, ARG_HELP, ARG_NUM };

#define CLASSNAME_DOT	"Dot*"

typedef struct
{
	TAPTR file;
	TUINT8 readbuf[1024];

	TBOOL abort;
	TAPTR lock;
	TAPTR visual;
	TVPEN pen[4];
	TLIST objects;

} global;

typedef struct
{
	TNODE node;
	TINT penindex;
	TFLOAT x, y;

} object;

TAPTR TExecBase, TUtilBase, TTimeBase, TIOBase;
TAPTR TimeReq;

/**************************************************************************
** 
**	test.delay(delaysecs)
*/

static LUACFUNC TINT test_delay(lua_State *L)
{
	global *g = lua_touserdata(L, lua_upvalueindex(1));
	TINT numargs;
	TFLOAT delayf;
	TTIME delayt;

	numargs = lua_gettop(L);	/* number of arguments */
	switch (numargs)
	{
		default:
			lua_pushstring(L, "incorrect number of arguments");
			lua_error(L);
		case 0:
			delayf = 1.0;
			break;
		case 1:
			delayf = (TFLOAT) lua_tonumber(L, 1);
			break;
	}

	delayt.ttm_Sec = delayf;
	delayt.ttm_USec = (TINT) ((delayf - delayt.ttm_Sec) * 1000000);

	TDelay((((struct TLuaUserState *) L) - 1)->tlu_TimeRequest, &delayt);

	lua_pushboolean(L, g->abort);
	lua_pushnumber(L, 1.0);
	
	return 2;	/* number of results */
}

/**************************************************************************
** 
**	test.abort()
*/

static LUACFUNC TINT test_abort(lua_State *L)
{
	global *g = lua_touserdata(L, lua_upvalueindex(1));
	g->abort = TTRUE;
	return 0;
}


/*************************************************************************
**
**	test.newdot(pen-nr.)
*/

static LUACFUNC TINT test_newdot(lua_State *L)
{
	global *g = lua_touserdata(L, lua_upvalueindex(1));
	object **pobj, *obj;
	TINT numargs;

	numargs = lua_gettop(L);
	if (numargs != 1) luaL_argerror(L, 1, "wrong number of arguments");
	if (!lua_isnumber(L, 1)) luaL_argerror(L, 1, "argument type incorrect");

	/* reserve full, collectable userdata for a
	pointer to some object, and push it on the stack */
	pobj = lua_newuserdata(L, sizeof(object *));		/* stack: userdata */
	*pobj = TNULL;

	/* upvalueindex(2) refers to the class metatable */
	lua_pushvalue(L, lua_upvalueindex(2));				/* stack: userdata, metatable */

	/* attach metatable to the userdata object */
	lua_setmetatable(L, -2);							/* stack: userdata */

	obj = TAlloc(TNULL, sizeof(object));
	if (obj == TNULL)
	{
		lua_pushstring(L, "could not create object");
		lua_error(L);
	}

	obj->penindex = ((TINT) lua_tonumber(L, 1)) & 3;
	obj->x = 0.5;
	obj->y = 0.5;
	*pobj = obj;

	TLock(g->lock);				/* protect access to the list */
	TAddTail(&g->objects, (TNODE *) obj);
	TUnlock(g->lock);				/* unprotect access to the list */

	return 1;	/* number of return values on the stack: userdata */
}


/*************************************************************************
**
**	dot:destroy()
*/

static LUACFUNC TINT test_destroydot(lua_State *L)
{
	global *g = lua_touserdata(L, lua_upvalueindex(1));
	object *obj, **pobj = luaL_checkudata(L, 1, CLASSNAME_DOT);
	TINT numargs = lua_gettop(L);
	if (numargs != 1)
	{
		lua_pushstring(L, "wrong number of arguments");
		lua_error(L);
	}

	if (pobj == TNULL) luaL_argerror(L, 1, "bad handle");
	obj = *pobj;
	if (obj)
	{
		TLock(g->lock);				/* protect access to the list */
		TRemove((TNODE *) obj);
		TUnlock(g->lock);				/* unprotect access to the list */
		TFree(obj);
		*pobj = TNULL;							/* mark as closed */
	}
	return 0;
}


/*************************************************************************
**
**	dot:setpos()
*/

static LUACFUNC TINT test_setdotpos(lua_State *L)
{
	global *g = lua_touserdata(L, lua_upvalueindex(1));
	object *obj, **pobj = luaL_checkudata(L, 1, CLASSNAME_DOT);
	TINT numargs = lua_gettop(L);
	if (numargs != 3)
	{
		lua_pushstring(L, "wrong number of arguments");
		lua_error(L);
	}
	if (!lua_isnumber(L, 2)) luaL_argerror(L, 2, "argument type incorrect");
	if (!lua_isnumber(L, 3)) luaL_argerror(L, 3, "argument type incorrect");

	if (pobj == TNULL) luaL_argerror(L, 1, "bad handle");
	obj = *pobj;
	if (obj)
	{
		TLock(g->lock);				/* protect access to the list */
		obj->x = lua_tonumber(L, 2);
		obj->y = lua_tonumber(L, 3);
		TUnlock(g->lock);				/* unprotect access to the list */
	}
	return 0;
}


/*************************************************************************
**
**	tables of library and class functions and methods
*/

static luaL_reg test_functions[] =
{
	{"delay", test_delay},
	{"abort", test_abort},
	{TNULL, TNULL}
};

static luaL_reg dot_functions[] =
{
	{"newdot", test_newdot},
	{TNULL, TNULL}
};

static luaL_reg dot_methods[] =
{
	{"setpos", test_setdotpos},
	{"destroy", test_destroydot},
	{TNULL, TNULL}
};


/*************************************************************************
**
**	Callbacks for reading Lua chunks and extending the language
*/

static LUACFUNC TVOID initextensions(lua_State *L, struct LuaExecData *e)
{
	/* Add a set of simple functions (no class/methods) to the library "test" */
	luaT_addlib(L, "test", test_functions, e->led_UserData);

	/* Add a "dot" class (functions and methods) to the library "test" */
	luaT_addclass(L, "test", CLASSNAME_DOT, dot_functions, dot_methods, e->led_UserData);
}

static LUACFUNC TSTRPTR readfunc(lua_State *L, struct LuaExecData *e, size_t *size)
{
	global *g = e->led_UserData;
	*size = TRead(g->file, g->readbuf, sizeof(g->readbuf));
	if (*size > 0) return (TSTRPTR) g->readbuf;
	return TNULL;
}


/*************************************************************************
**
**	run script in the background
*/

struct ScriptMessage
{
	TAPTR childtask;
	TAPTR replyport;
	lua_State *interpreter;
	struct LuaExecData execdata;
};

static TTASKENTRY TVOID scriptexecfunc(TAPTR task)
{
	struct ScriptMessage *msg;
	TAPTR exec = TGetExecBase(task);	
	TAPTR port = TExecGetUserPort(exec, task);
	TExecWaitPort(exec, port);
	msg = TExecGetMsg(exec, port);
	luaT_runchunk(msg->interpreter, &msg->execdata);
	TExecReplyMsg(exec, msg);
}

static struct ScriptMessage *script_init(global *g, lua_State *L, TSTRPTR scriptname, TSTRPTR *args)
{
	struct ScriptMessage *scriptmsg;
	scriptmsg = TAllocMsg0(sizeof(struct ScriptMessage));
	if (scriptmsg)
	{
		scriptmsg->replyport = TCreatePort(TNULL);
		if (scriptmsg->replyport)
		{
			scriptmsg->childtask = TCreateTask(scriptexecfunc, TNULL, TNULL);
			if (scriptmsg->childtask)
			{
				scriptmsg->interpreter = L;
				scriptmsg->execdata.led_ChunkName = scriptname;			/* name of the chunk executed */
				scriptmsg->execdata.led_ProgName = scriptname;			/* program name, aka argv[0] */
				scriptmsg->execdata.led_ArgV = args;					/* argument vector */
				
				return scriptmsg;
			}
			TDestroy(scriptmsg->replyport);
		}
		TFree(scriptmsg);
	}
	return TNULL;
}

static TVOID script_run(global *g, struct ScriptMessage *scriptmsg)
{
	TAPTR childport = TGetUserPort(scriptmsg->childtask);
	TPutMsg(childport, scriptmsg->replyport, scriptmsg);
}

static TINT script_finish(global *g, struct ScriptMessage *scriptmsg)
{
	TINT error;
	TWaitPort(scriptmsg->replyport);
	TGetMsg(scriptmsg->replyport);
	error = scriptmsg->execdata.led_Error;
	TDestroy(scriptmsg->childtask);
	TDestroy(scriptmsg->replyport);
	TFree(scriptmsg);
	return error;
}


/*************************************************************************
**
**	do some asynchronous interaction with the script
*/

static TBOOL render_init(global *g)
{
	g->lock = TCreateLock(TNULL);
	if (g->lock)
	{
		g->visual = TOpenModule("visual", 0, TNULL);
		if (g->visual)
		{
			g->pen[0] = TVisualAllocPen(g->visual, 0x000000);
			g->pen[1] = TVisualAllocPen(g->visual, 0xff0000);
			g->pen[2] = TVisualAllocPen(g->visual, 0x00ff00);
			g->pen[3] = TVisualAllocPen(g->visual, 0x0000ff);
			TInitList(&g->objects);
			g->abort = TFALSE;
			return TTRUE;
		}
		TDestroy(g->lock);
	}
	return TFALSE;
}

static TVOID render_exit(global *g)
{
	object *obj;

	while ((obj = (object *) TRemHead(&g->objects)))
	{
		TFree(obj);
	}

	TCloseModule(g->visual);
	TDestroy(g->lock);
}

static TVOID render_main(global *g)
{
	TAPTR iport = TVisualGetPort(g->visual);
	TUINT waitsig = TGetPortSignal(iport);
	TIMSG *imsg;
	TTIME delayt;
	TNODE *nextnode, *node;
	TTAGITEM attrs[3];
	TINT width, height, dotw = 1, doth = 1;
	TBOOL checksize = TTRUE;

	attrs[0].tti_Tag = TVisual_PixWidth;
	attrs[0].tti_Value = (TTAG) &width;
	attrs[1].tti_Tag = TVisual_PixHeight;
	attrs[1].tti_Value = (TTAG) &height;
	attrs[2].tti_Tag = TTAG_DONE;

	TVisualSetInput(g->visual, 0, TITYPE_CLOSE | TITYPE_COOKEDKEY | TITYPE_NEWSIZE);
	delayt.ttm_Sec = 0;
	delayt.ttm_USec = 1000000/50;

	while (!g->abort)
	{
		while ((imsg = (TIMSG *) TGetMsg(iport)))
		{
			if (imsg->timsg_Type == TITYPE_NEWSIZE) checksize = TTRUE;
			if (imsg->timsg_Type == TITYPE_CLOSE) g->abort = TTRUE;
			if (imsg->timsg_Type == TITYPE_COOKEDKEY &&
				imsg->timsg_Code == TKEYC_ESC) g->abort = TTRUE;
			TAckMsg(imsg);
		}

		if (checksize)
		{
			TVisualGetAttrs(g->visual, attrs);
			checksize = TFALSE;
			dotw = TMAX(1, width / 100);
			doth = TMAX(1, height / 100);
		}

		TVisualClear(g->visual, g->pen[0]);

		TLock(g->lock);	/* protect access to the list */

		node = g->objects.tlh_Head;
		while ((nextnode = node->tln_Succ))
		{
			object *obj = (object *) node;
			TVisualFRect(g->visual, (TINT) (obj->x * width), (TINT) (obj->y * height), dotw, doth, g->pen[obj->penindex]);
			node = nextnode;
		}

		TUnlock(g->lock);	/* unprotect access to the list */

		TVisualFlush(g->visual);

		TWaitTime(TimeReq, &delayt, waitsig);
	}
}


/*************************************************************************
**
**	main
*/

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TINT res = 20;
	global gdata, *g = &gdata;
	lua_State *L;

	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TTimeBase = TOpenModule("time", 0, TNULL);
	TIOBase = TOpenModule("io", 0, TNULL);
	L = TOpenModule("lua5", 0, TNULL);
	if (TUtilBase && TTimeBase && TIOBase && L)
	{
		TimeReq = TAllocTimeRequest(TNULL);
		if (TimeReq)
		{
			TSTRPTR *argv = TGetArgV();
			TTAG args[ARG_NUM];
			TAPTR arghandle;
			
			args[ARG_FILE] = (TTAG) "mods/lua/scripts/dots.lua";
			args[ARG_ARGS] = TNULL;
			args[ARG_HELP] = TFALSE;
			
			res = 10;
			
			arghandle = TParseArgV(ARG_TEMPLATE, argv + 1, args);
			if (args[ARG_HELP] || !arghandle)
			{
				printf("Usage: %s %s\n", argv[0], ARG_TEMPLATE);
				printf("(Test for interaction with Lua interpreter in a thread)\n");
				printf("-f=FROM      : Lua script to run [mods/lua/scripts/dots.lua]\n");
				printf("-a=ARGS/M    : arguments passed to the script\n");
				printf("-h=HELP/S    : get this help\n");
			}
			else
			{
				if (render_init(g))
				{
					g->file = TOpenFile((TSTRPTR) args[ARG_FILE], TFMODE_READONLY, TNULL);
					if (g->file)
					{
						TINT err = LUA_ERRRUN;
						struct ScriptMessage *msg;
		
						msg = script_init(g, L, (TSTRPTR) args[ARG_FILE], (TSTRPTR *) args[ARG_ARGS]);
						if (msg)
						{
							msg->execdata.led_ReadFunc = (lua_Chunkreader) readfunc;	/* read function */
							msg->execdata.led_InitFunc = initextensions;				/* user init */
							msg->execdata.led_UserData = g;								/* userdata to script */
			
							/* set these if you want errors to be printed: */
							msg->execdata.led_IOBase = TIOBase;							/* I/O module base */
							msg->execdata.led_ErrorFH = TErrorFH();						/* error filehandle */
							
							/* launch script */
							script_run(g, msg);

							/* script runs in the background here */
							
							render_main(g);

							err = script_finish(g, msg);
						}
						else
						{
							printf("*** could not initialize script\n");
						}
						
						TCloseFile(g->file);
						
						switch (err)
						{
							default:
								printf("*** Lua initialization or runtime error.\n");
								res = 10;
								break;
						
							case LUA_ERRSYNTAX:
								printf("*** Syntax error.\n");
								res = 5;
								break;
								
							case 0:
								res = 0;
								break;
						}
					}
					else
					{
						printf("*** could not open file: %s\n", (TSTRPTR) args[ARG_FILE]);
					}
		
					render_exit(g);
				}
				else
				{
					printf("*** could not initialize window\n");
				}
			}
			TDestroy(arghandle);
			TFreeTimeRequest(TimeReq);
		}
	}
	else
	{
		printf("*** could not open modules.\n");
	}

	TSetRetVal(res);
	if (L) TCloseModule(LUABASE(L));
	TCloseModule(TIOBase);
	TCloseModule(TTimeBase);
	TCloseModule(TUtilBase);

	tdbprintf1(5, "bye... return value: %d\n", res);
}
