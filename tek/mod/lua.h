
#ifndef _TEK_MOD_LUA_H
#define _TEK_MOD_LUA_H

/*
**	tek/mod/lua.h
*/

#include <tek/exec.h>
#include <stdarg.h>
#include <stddef.h>

typedef struct lua_State lua_State;

/*****************************************************************************/

/* option for multiple returns in `lua_pcall' and `lua_call' */
#define LUA_MULTRET	(-1)

/*
** pseudo-indices
*/
#define LUA_REGISTRYINDEX	(-10000)
#define LUA_ENVIRONINDEX	(-10001)
#define LUA_GLOBALSINDEX	(-10002)
#define lua_upvalueindex(i)	(LUA_GLOBALSINDEX-(i))

/* return codes for `lua_pcall', `lua_resume', and `lua_status' */
#define LUA_YIELD	1
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5

/* additional Lua C function qualifiers */
#ifndef LUACFUNC
#define LUACFUNC TCALLBACK
#endif

/* Lua C function type */
typedef LUACFUNC TINT (*lua_CFunction) (lua_State *L);


/*
** functions that read/write blocks when loading/dumping Lua chunks
*/
typedef LUACFUNC const char * (*lua_Chunkreader) (lua_State *L, void *ud, TSIZE *sz);

typedef LUACFUNC TINT (*lua_Chunkwriter) (lua_State *L, const void * p,
                                TSIZE sz, TAPTR ud);


/*
** prototype for memory-allocation functions
*/
typedef LUACFUNC TAPTR (*lua_Alloc) (TAPTR ud, TAPTR ptr, TSIZE osize, TSIZE nsize);


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


/* first index for arrays */
#define LUA_FIRSTINDEX		1


/* minimum Lua stack available to a C function */
#define LUA_MINSTACK	20


/* type of numbers in Lua */

#ifdef TSYS_PS2
typedef TFLOAT lua_Number;
#define LUA_NUMBER_SCAN		"%f"
#define LUA_NUMBER_FMT		"%.5g"
#else
typedef TDOUBLE lua_Number;
#define LUA_NUMBER_SCAN		"%lf"
#define LUA_NUMBER_FMT		"%.14g"
#endif

/* type for integer functions */
typedef TUINTPTR lua_Integer;


/*****************************************************************************/
/* 
**	auxlib
*/

typedef struct luaL_reg {
  TSTRPTR name;
  lua_CFunction func;
} luaL_reg;


/*****************************************************************************/
/* 
**	Lua instance base
*/

struct TLuaUserState
{
	struct TModule tlu_Module;		/* Module header */
	TAPTR tlu_UserData;				/* Userdata */
	TAPTR tlu_TimeRequest;			/* instance-specific */
	TAPTR tlu_Reserved[2];			/* Private */
};

#define LUA_USERSTATE struct TLuaUserState

/* get pointer to the Lua super instance */
#define LUABASE(L) (((LUA_USERSTATE *)(L))-1)
#define LUASUPER(L)	(LUABASE(L)->tlu_Module.tmd_ModSuper)

#endif /* _TEK_MOD_LUA_H */
