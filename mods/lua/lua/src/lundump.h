/*
** $Id: lundump.h,v 1.1 2005/05/08 19:15:46 tmueller Exp $
** load pre-compiled Lua chunks
** See Copyright Notice in lua.h
*/

#ifndef lundump_h
#define lundump_h

#include "lobject.h"
#include "lzio.h"

/* load one chunk; from lundump.c */
Proto* luaU_undump (lua_State* L, ZIO* Z, Mbuffer* buff, const char *name);

/* find byte order; from lundump.c */
int luaU_endianness (void);

/* dump one chunk; from ldump.c */
int luaU_dump (lua_State* L, const Proto* f, lua_Chunkwriter w, void* data, int strip);

/* print one chunk; from print.c */
void luaU_print (const Proto* f, int full);

/* for header of binary files -- this is Lua 5.1 */
#define	VERSION		0x51

/* for testing native format of lua_Numbers */
#define	TEST_NUMBER	((lua_Number)31415926.0)

#endif
