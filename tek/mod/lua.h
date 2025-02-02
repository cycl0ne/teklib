#ifndef _TEK_MOD_LUA_H
#define _TEK_MOD_LUA_H

/*
**	$Id: lua.h,v 1.1.1.1 2006/08/20 22:15:25 tmueller Exp $
**	teklib/tek/mod/lua.h - Lua module definitions
**	See copyright notice in teklib/mods/lua/lua/COPYRIGHT
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <stdarg.h>
#include <stddef.h>


/* additional qualifier for C function arguments */
#define LUACFUNC TCALLBACK


/* mark for precompiled code (`<esc>Lua') */
#define	LUA_SIGNATURE	"\033Lua"


/* option for multiple returns in `lua_pcall' and `lua_call' */
#define LUA_MULTRET	(-1)


/*
** pseudo-indices
*/
#define LUA_REGISTRYINDEX	(-10000)
#define LUA_ENVIRONINDEX	(-10001)
#define LUA_GLOBALSINDEX	(-10002)
#define lua_upvalueindex(i)	(LUA_GLOBALSINDEX-(i))


/* thread status; 0 is OK */
#define LUA_YIELD	1
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5


typedef struct lua_State lua_State;

typedef LUACFUNC TINT (*lua_CFunction) (lua_State *L);


/*
** functions that read/write blocks when loading/dumping Lua chunks
*/
typedef LUACFUNC const char * (*lua_Reader) (lua_State *L, void *ud, size_t *sz);

typedef LUACFUNC int (*lua_Writer) (lua_State *L, const void* p, size_t sz, void* ud);


/*
** prototype for memory-allocation functions
*/
typedef LUACFUNC void * (*lua_Alloc) (TAPTR ud, TAPTR ptr, TSIZE osize, TSIZE nsize);


/*
** basic types
*/
#define LUA_TNONE	(-1)

#define LUA_TNIL	0
#define LUA_TBOOLEAN	1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER	3
#define LUA_TSTRING	4
#define LUA_TTABLE	5
#define LUA_TFUNCTION	6
#define LUA_TUSERDATA	7
#define LUA_TTHREAD	8


/* minimum Lua stack available to a C function */
#define LUA_MINSTACK	20


/* type of numbers in Lua */
#if !defined(TSYS_PS2)
typedef TDOUBLE lua_Number;
#else
typedef TFLOAT lua_Number;
#endif


/* type for integer functions */
typedef TINTPTR lua_Integer;



/* garbage collector definitions */
#define LUA_GCSTOP			0
#define LUA_GCRESTART		1
#define LUA_GCCOLLECT		2
#define LUA_GCCOUNT			3
#define LUA_GCCOUNTB		4
#define LUA_GCSTEP			5
#define LUA_GCSETPAUSE		6
#define LUA_GCSETSTEPMUL	7


/*****************************************************************************/
/*
**	auxlib
*/

#define LUAL_BUFFERSIZE		2048

typedef struct luaL_Reg {
  const char *name;
  lua_CFunction func;
} luaL_Reg;

#define luaL_reg	luaL_Reg


typedef struct luaL_Buffer {
  char *p;			/* current position in buffer */
  int lvl;  /* number of strings in the stack (level) */
  lua_State *L;
  char buffer[LUAL_BUFFERSIZE];
} luaL_Buffer;


/*****************************************************************************/
/*
**	Lua instance base
*/

typedef union TLuaUserState
{
	struct
	{
		struct TModule tlu_Module;
		/* instance-specific: */
		TAPTR tlu_UserData;
	} State;

	/* enforce sane alignment: */
	TUINT8 Align[128];

} LUA_USERSTATE;

/* get from interpreter to instance userstate: */
#define LUABASE(L) (&(((LUA_USERSTATE *)(L))-1)->State)

/* get from interpreter to super instance: */
#define LUASUPER(L) (LUABASE(L)->tlu_Module.tmd_ModSuper)

#endif /* _TEK_MOD_LUA_H */
