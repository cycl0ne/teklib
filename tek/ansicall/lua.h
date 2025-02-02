#ifndef _TEK_ANSICALL_LUA_H
#define _TEK_ANSICALL_LUA_H

/*
**	$Id: lua.h,v 1.3.2.1 2005/12/04 22:31:10 tmueller Exp $
**	teklib/tek/ansicall/lua.h - lua5 module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

/* -- state manipulation -- */

#define lua_newthread(L) \
	(*(((TMODCALL lua_State *(**)(lua_State *))(LUASUPER(L)))[-2]))(L)

#define lua_atpanic(L,panicf) \
	(*(((TMODCALL lua_CFunction(**)(lua_State *,lua_CFunction))(LUASUPER(L)))[-3]))(L,panicf)

/* -- load and call -- */

#define lua_call(L,narg,nres) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT,TINT))(LUASUPER(L)))[-4]))(L,narg,nres)

#define lua_pcall(L,narg,nres,errf) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT,TINT,TINT))(LUASUPER(L)))[-5]))(L,narg,nres,errf)

#define lua_cpcall(L,func,udata) \
	(*(((TMODCALL TINT(**)(lua_State *,lua_CFunction,TAPTR))(LUASUPER(L)))[-6]))(L,func,udata)

#define lua_load(L,readf,data,cname) \
	(*(((TMODCALL TINT(**)(lua_State *,lua_Chunkreader,TAPTR,TSTRPTR))(LUASUPER(L)))[-7]))(L,readf,data,cname)

#define lua_dump(L,writef,data) \
	(*(((TMODCALL TINT(**)(lua_State *,lua_Chunkwriter,TAPTR))(LUASUPER(L)))[-8]))(L,writef,data)

/* -- basic stack manipulation -- */

#define lua_gettop(L) \
	(*(((TMODCALL TINT(**)(lua_State *))(LUASUPER(L)))[-9]))(L)

#define lua_settop(L,idx) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-10]))(L,idx)

#define lua_pushvalue(L,idx) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-11]))(L,idx)

#define lua_remove(L,idx) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-12]))(L,idx)

#define lua_insert(L,idx) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-13]))(L,idx)

#define lua_replace(L,idx) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-14]))(L,idx)

#define lua_checkstack(L,size) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-15]))(L,size)

#define lua_xmove(L,dest,n) \
	(*(((TMODCALL TVOID(**)(lua_State *,TAPTR,TINT))(LUASUPER(L)))[-16]))(L,dest,n)

/* -- access functions (stack -> C) -- */

#define lua_isnumber(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-17]))(L,idx)

#define lua_isstring(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-18]))(L,idx)

#define lua_iscfunction(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-19]))(L,idx)

#define lua_isuserdata(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-20]))(L,idx)

#define lua_type(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-21]))(L,idx)

#define lua_typename(L,tp) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-22]))(L,tp)

#define lua_equal(L,i1,i2) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT,TINT))(LUASUPER(L)))[-23]))(L,i1,i2)

#define lua_rawequal(L,i1,i2) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT,TINT))(LUASUPER(L)))[-24]))(L,i1,i2)

#define lua_lessthan(L,i1,i2) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT,TINT))(LUASUPER(L)))[-25]))(L,i1,i2)

#define lua_tonumber(L,idx) \
	(*(((TMODCALL lua_Number(**)(lua_State *,TINT))(LUASUPER(L)))[-26]))(L,idx)

#define lua_toboolean(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-27]))(L,idx)

#define lua_tostring(L,idx) \
	(*(((TMODCALL TSTRPTR(**)(lua_State *,TINT))(LUASUPER(L)))[-28]))(L,idx)

#define lua_strlen(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-29]))(L,idx)

#define lua_tocfunction(L,idx) \
	(*(((TMODCALL lua_CFunction(**)(lua_State *,TINT))(LUASUPER(L)))[-30]))(L,idx)

#define lua_touserdata(L,idx) \
	(*(((TMODCALL TAPTR(**)(lua_State *,TINT))(LUASUPER(L)))[-31]))(L,idx)

#define lua_tothread(L,idx) \
	(*(((TMODCALL lua_State *(**)(lua_State *,TINT))(LUASUPER(L)))[-32]))(L,idx)

#define lua_topointer(L,idx) \
	(*(((TMODCALL TAPTR(**)(lua_State *,TINT))(LUASUPER(L)))[-33]))(L,idx)

/* -- push functions (C -> stack) -- */

#define lua_pushnil(L) \
	(*(((TMODCALL TVOID(**)(lua_State *))(LUASUPER(L)))[-34]))(L)

#define lua_pushnumber(L,n) \
	(*(((TMODCALL TVOID(**)(lua_State *,lua_Number))(LUASUPER(L)))[-35]))(L,n)

#define lua_pushlstring(L,s,l) \
	(*(((TMODCALL TVOID(**)(lua_State *,TSTRPTR,TINT))(LUASUPER(L)))[-36]))(L,s,l)

#define lua_pushstring(L,s) \
	(*(((TMODCALL TVOID(**)(lua_State *,TSTRPTR))(LUASUPER(L)))[-37]))(L,s)

#define lua_pushinteger(L,n) \
	(*(((TMODCALL TVOID(**)(lua_State *,lua_Integer))(LUASUPER(L)))[-38]))(L,n)

#define lua_pushcclosure(L,func,n) \
	(*(((TMODCALL TVOID(**)(lua_State *,lua_CFunction,TINT))(LUASUPER(L)))[-39]))(L,func,n)

#define lua_pushboolean(L,b) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-40]))(L,b)

#define lua_pushlightuserdata(L,udata) \
	(*(((TMODCALL TVOID(**)(lua_State *,TAPTR))(LUASUPER(L)))[-41]))(L,udata)

/* -- get functions (Lua -> stack) -- */

#define lua_gettable(L,idx) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-42]))(L,idx)

#define lua_rawget(L,idx) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-43]))(L,idx)

#define lua_rawgeti(L,idx,n) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT,TINT))(LUASUPER(L)))[-44]))(L,idx,n)

#define lua_newtable(L) \
	(*(((TMODCALL TVOID(**)(lua_State *))(LUASUPER(L)))[-45]))(L)

#define lua_newuserdata(L,size) \
	(*(((TMODCALL TAPTR(**)(lua_State *,TINT))(LUASUPER(L)))[-46]))(L,size)

#define lua_getmetatable(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-47]))(L,idx)

#define lua_getfenv(L,idx) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-48]))(L,idx)

/* -- set functions (stack -> Lua) -- */

#define lua_settable(L,idx) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-49]))(L,idx)

#define lua_rawset(L,idx) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-50]))(L,idx)

#define lua_rawseti(L,idx,n) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT,TINT))(LUASUPER(L)))[-51]))(L,idx,n)

#define lua_setmetatable(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-52]))(L,idx)

#define lua_setfenv(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-53]))(L,idx)

/* -- coroutine functions */

#define lua_yield(L,nresults) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-54]))(L,nresults)

#define lua_resume(L,narg) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-55]))(L,narg)

/* -- garbage-collection functions -- */

#define lua_getgcthreshold(L) \
	(*(((TMODCALL TINT(**)(lua_State *))(LUASUPER(L)))[-56]))(L)

#define lua_getgccount(L) \
	(*(((TMODCALL TINT(**)(lua_State *))(LUASUPER(L)))[-57]))(L)

#define lua_setgcthreshold(L,newth) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-58]))(L,newth)

/* -- misc functions */

#define lua_version(L) \
	(*(((TMODCALL TSTRPTR(**)(lua_State *))(LUASUPER(L)))[-59]))(L)

#define lua_error(L) \
	(*(((TMODCALL TINT(**)(lua_State *))(LUASUPER(L)))[-60]))(L)

#define lua_next(L,idx) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-61]))(L,idx)

#define lua_concat(L,n) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-62]))(L,n)

/* -- auxlib functions -- */

#define luaL_openlib(L,name,a,nup) \
	(*(((TMODCALL TVOID(**)(lua_State *,TSTRPTR,luaL_reg *,TINT))(LUASUPER(L)))[-70]))(L,name,a,nup)

#define luaL_getmetafield(L,obj,e) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT,TSTRPTR))(LUASUPER(L)))[-71]))(L,obj,e)

#define luaL_callmeta(L,obj,e) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT,TSTRPTR))(LUASUPER(L)))[-72]))(L,obj,e)

#define luaL_typerror(L,narg,tname) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT,TSTRPTR))(LUASUPER(L)))[-73]))(L,narg,tname)

#define luaL_argerror(L,narg,extramsg) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT,TSTRPTR))(LUASUPER(L)))[-74]))(L,narg,extramsg)

#define luaL_checklstring(L,narg,l) \
	(*(((TMODCALL TSTRPTR(**)(lua_State *,TINT,TSIZE *))(LUASUPER(L)))[-75]))(L,narg,l)

#define luaL_optlstring(L,narg,def,l) \
	(*(((TMODCALL TSTRPTR(**)(lua_State *,TINT,TSTRPTR,TSIZE *))(LUASUPER(L)))[-76]))(L,narg,def,l)

#define luaL_checknumber(L,narg) \
	(*(((TMODCALL lua_Number(**)(lua_State *,TINT))(LUASUPER(L)))[-77]))(L,narg)

#define luaL_optnumber(L,narg,def) \
	(*(((TMODCALL lua_Number(**)(lua_State *,TINT,lua_Number))(LUASUPER(L)))[-78]))(L,narg,def)

#define luaL_checkstack(L,sz,msg) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT,TSTRPTR))(LUASUPER(L)))[-79]))(L,sz,msg)

#define luaL_checktype(L,narg,t) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT,TINT))(LUASUPER(L)))[-80]))(L,narg,t)

#define luaL_checkany(L,narg) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-81]))(L,narg)

#define luaL_newmetatable(L,name) \
	(*(((TMODCALL TINT(**)(lua_State *,TSTRPTR))(LUASUPER(L)))[-82]))(L,name)

#define luaL_getmetatable(L,name) \
	(*(((TMODCALL TVOID(**)(lua_State *,TSTRPTR))(LUASUPER(L)))[-83]))(L,name)

#define luaL_checkudata(L,ud,name) \
	(*(((TMODCALL TAPTR(**)(lua_State *,TINT,TSTRPTR))(LUASUPER(L)))[-84]))(L,ud,name)

#define luaL_where(L,lvl) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT))(LUASUPER(L)))[-85]))(L,lvl)

#define luaL_ref(L,t) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-86]))(L,t)

#define luaL_unref(L,t,ref) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT,TINT))(LUASUPER(L)))[-87]))(L,t,ref)

#define luaL_getn(L,t) \
	(*(((TMODCALL TINT(**)(lua_State *,TINT))(LUASUPER(L)))[-88]))(L,t)

#define luaL_setn(L,t,n) \
	(*(((TMODCALL TVOID(**)(lua_State *,TINT,TINT))(LUASUPER(L)))[-89]))(L,t,n)

#define luaL_loadfile(L,fname) \
	(*(((TMODCALL TINT(**)(lua_State *,TSTRPTR))(LUASUPER(L)))[-90]))(L,fname)

#define luaL_loadbuffer(L,buf,size,name) \
	(*(((TMODCALL TINT(**)(lua_State *,TSTRPTR,TSIZE,TSTRPTR))(LUASUPER(L)))[-91]))(L,buf,size,name)

#define luaL_gsub(L,s,p,r) \
	(*(((TMODCALL TSTRPTR(**)(lua_State *,TSTRPTR,TSTRPTR,TSTRPTR))(LUASUPER(L)))[-93]))(L,s,p,r)

#define luaL_getfield(L,idx,fname) \
	(*(((TMODCALL TSTRPTR(**)(lua_State *,TINT,TSTRPTR))(LUASUPER(L)))[-94]))(L,idx,fname)

#define luaL_setfield(L,idx,fname) \
	(*(((TMODCALL TSTRPTR(**)(lua_State *,TINT,TSTRPTR))(LUASUPER(L)))[-95]))(L,idx,fname)

#endif /* _TEK_ANSICALL_LUA_H */
