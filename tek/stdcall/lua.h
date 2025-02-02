#ifndef _TEK_STDCALL_LUA_H
#define _TEK_STDCALL_LUA_H

/*
**	$Id: lua.h $
**	teklib/tek/stdcall/lua.h - lua5 module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

/* -- state manipulation -- */

#define lua_newthread(lua5) \
	(*(((TMODCALL lua_State *(**)(TAPTR))(LUASUPER(L)))[-9]))(lua5)

#define lua_atpanic(lua5,panicf) \
	(*(((TMODCALL lua_CFunction(**)(TAPTR,lua_CFunction))(LUASUPER(L)))[-10]))(lua5,panicf)

/* -- load and call -- */

#define lua_call(lua5,narg,nres) \
	(*(((TMODCALL void(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-11]))(lua5,narg,nres)

#define lua_pcall(lua5,narg,nres,errf) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TINT,TINT))(LUASUPER(L)))[-12]))(lua5,narg,nres,errf)

#define lua_cpcall(lua5,func,udata) \
	(*(((TMODCALL TINT(**)(TAPTR,lua_CFunction,TAPTR))(LUASUPER(L)))[-13]))(lua5,func,udata)

#define lua_load(lua5,readf,data,cname) \
	(*(((TMODCALL TINT(**)(TAPTR,lua_Reader,TAPTR,TSTRPTR))(LUASUPER(L)))[-14]))(lua5,readf,data,cname)

#define lua_dump(lua5,writef,data) \
	(*(((TMODCALL TINT(**)(TAPTR,lua_Writer,TAPTR))(LUASUPER(L)))[-15]))(lua5,writef,data)

/* -- basic stack manipulation -- */

#define lua_gettop(lua5) \
	(*(((TMODCALL TINT(**)(TAPTR))(LUASUPER(L)))[-16]))(lua5)

#define lua_settop(lua5,idx) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-17]))(lua5,idx)

#define lua_pushvalue(lua5,idx) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-18]))(lua5,idx)

#define lua_remove(lua5,idx) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-19]))(lua5,idx)

#define lua_insert(lua5,idx) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-20]))(lua5,idx)

#define lua_replace(lua5,idx) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-21]))(lua5,idx)

#define lua_checkstack(lua5,size) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-22]))(lua5,size)

#define lua_xmove(lua5,dest,n) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR,TINT))(LUASUPER(L)))[-23]))(lua5,dest,n)

/* -- access functions (stack -> C) -- */

#define lua_isnumber(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-24]))(lua5,idx)

#define lua_isstring(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-25]))(lua5,idx)

#define lua_iscfunction(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-26]))(lua5,idx)

#define lua_isuserdata(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-27]))(lua5,idx)

#define lua_type(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-28]))(lua5,idx)

#define lua_typename(lua5,tp) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-29]))(lua5,tp)

#define lua_equal(lua5,i1,i2) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-30]))(lua5,i1,i2)

#define lua_rawequal(lua5,i1,i2) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-31]))(lua5,i1,i2)

#define lua_lessthan(lua5,i1,i2) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-32]))(lua5,i1,i2)

#define lua_tonumber(lua5,idx) \
	(*(((TMODCALL lua_Number(**)(TAPTR,TINT))(LUASUPER(L)))[-33]))(lua5,idx)

#define lua_toboolean(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-34]))(lua5,idx)

#define lua_tostring(lua5,idx) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TINT))(LUASUPER(L)))[-35]))(lua5,idx)

#define lua_objlen(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-36]))(lua5,idx)

#define lua_tocfunction(lua5,idx) \
	(*(((TMODCALL lua_CFunction(**)(TAPTR,TINT))(LUASUPER(L)))[-37]))(lua5,idx)

#define lua_touserdata(lua5,idx) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TINT))(LUASUPER(L)))[-38]))(lua5,idx)

#define lua_tothread(lua5,idx) \
	(*(((TMODCALL lua_State *(**)(TAPTR,TINT))(LUASUPER(L)))[-39]))(lua5,idx)

#define lua_topointer(lua5,idx) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TINT))(LUASUPER(L)))[-40]))(lua5,idx)

/* -- push functions (C -> stack) -- */

#define lua_pushnil(lua5) \
	(*(((TMODCALL void(**)(TAPTR))(LUASUPER(L)))[-41]))(lua5)

#define lua_pushnumber(lua5,n) \
	(*(((TMODCALL void(**)(TAPTR,lua_Number))(LUASUPER(L)))[-42]))(lua5,n)

#define lua_pushlstring(lua5,s,l) \
	(*(((TMODCALL void(**)(TAPTR,TSTRPTR,TINT))(LUASUPER(L)))[-43]))(lua5,s,l)

#define lua_pushstring(lua5,s) \
	(*(((TMODCALL void(**)(TAPTR,TSTRPTR))(LUASUPER(L)))[-44]))(lua5,s)

#define lua_pushinteger(lua5,n) \
	(*(((TMODCALL void(**)(TAPTR,lua_Integer))(LUASUPER(L)))[-45]))(lua5,n)

#define lua_pushcclosure(lua5,func,n) \
	(*(((TMODCALL void(**)(TAPTR,lua_CFunction,TINT))(LUASUPER(L)))[-46]))(lua5,func,n)

#define lua_pushboolean(lua5,b) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-47]))(lua5,b)

#define lua_pushlightuserdata(lua5,udata) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(LUASUPER(L)))[-48]))(lua5,udata)

/* -- get functions (Lua -> stack) -- */

#define lua_gettable(lua5,idx) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-49]))(lua5,idx)

#define lua_rawget(lua5,idx) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-50]))(lua5,idx)

#define lua_rawgeti(lua5,idx,n) \
	(*(((TMODCALL void(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-51]))(lua5,idx,n)

#define lua_newtable(lua5) \
	(*(((TMODCALL void(**)(TAPTR))(LUASUPER(L)))[-52]))(lua5)

#define lua_newuserdata(lua5,size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TINT))(LUASUPER(L)))[-53]))(lua5,size)

#define lua_getmetatable(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-54]))(lua5,idx)

#define lua_getfenv(lua5,idx) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-55]))(lua5,idx)

/* -- set functions (stack -> Lua) -- */

#define lua_settable(lua5,idx) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-56]))(lua5,idx)

#define lua_rawset(lua5,idx) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-57]))(lua5,idx)

#define lua_rawseti(lua5,idx,n) \
	(*(((TMODCALL void(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-58]))(lua5,idx,n)

#define lua_setmetatable(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-59]))(lua5,idx)

#define lua_setfenv(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-60]))(lua5,idx)

/* -- coroutine functions */

#define lua_yield(lua5,nresults) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-61]))(lua5,nresults)

#define lua_resume(lua5,narg) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-62]))(lua5,narg)

/* -- garbage-collection functions -- */

#define lua_getgcthreshold(lua5) \
	(*(((TMODCALL TINT(**)(TAPTR))(LUASUPER(L)))[-63]))(lua5)

#define lua_getgccount(lua5) \
	(*(((TMODCALL TINT(**)(TAPTR))(LUASUPER(L)))[-64]))(lua5)

#define lua_setgcthreshold(lua5,newth) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-65]))(lua5,newth)

/* -- misc functions */

#define lua_version(lua5) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR))(LUASUPER(L)))[-66]))(lua5)

#define lua_error(lua5) \
	(*(((TMODCALL TINT(**)(TAPTR))(LUASUPER(L)))[-67]))(lua5)

#define lua_next(lua5,idx) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-68]))(lua5,idx)

#define lua_concat(lua5,n) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-69]))(lua5,n)

/* -- auxlib functions -- */

#define luaL_openlib(lua5,name,a,nup) \
	(*(((TMODCALL void(**)(TAPTR,TSTRPTR,luaL_Reg *,TINT))(LUASUPER(L)))[-70]))(lua5,name,a,nup)

#define luaL_getmetafield(lua5,obj,e) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TSTRPTR))(LUASUPER(L)))[-71]))(lua5,obj,e)

#define luaL_callmeta(lua5,obj,e) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TSTRPTR))(LUASUPER(L)))[-72]))(lua5,obj,e)

#define luaL_typerror(lua5,narg,tname) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TSTRPTR))(LUASUPER(L)))[-73]))(lua5,narg,tname)

#define luaL_argerror(lua5,narg,extramsg) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TSTRPTR))(LUASUPER(L)))[-74]))(lua5,narg,extramsg)

#define luaL_checklstring(lua5,narg,l) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TINT,TSIZE *))(LUASUPER(L)))[-75]))(lua5,narg,l)

#define luaL_optlstring(lua5,narg,def,l) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TINT,TSTRPTR,TSIZE *))(LUASUPER(L)))[-76]))(lua5,narg,def,l)

#define luaL_checknumber(lua5,narg) \
	(*(((TMODCALL lua_Number(**)(TAPTR,TINT))(LUASUPER(L)))[-77]))(lua5,narg)

#define luaL_optnumber(lua5,narg,def) \
	(*(((TMODCALL lua_Number(**)(TAPTR,TINT,lua_Number))(LUASUPER(L)))[-78]))(lua5,narg,def)

#define luaL_checkstack(lua5,sz,msg) \
	(*(((TMODCALL void(**)(TAPTR,TINT,TSTRPTR))(LUASUPER(L)))[-79]))(lua5,sz,msg)

#define luaL_checktype(lua5,narg,t) \
	(*(((TMODCALL void(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-80]))(lua5,narg,t)

#define luaL_checkany(lua5,narg) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-81]))(lua5,narg)

#define luaL_newmetatable(lua5,name) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR))(LUASUPER(L)))[-82]))(lua5,name)

#define luaL_getmetatable(lua5,name) \
	(*(((TMODCALL void(**)(TAPTR,TSTRPTR))(LUASUPER(L)))[-83]))(lua5,name)

#define luaL_checkudata(lua5,ud,name) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TINT,TSTRPTR))(LUASUPER(L)))[-84]))(lua5,ud,name)

#define luaL_where(lua5,lvl) \
	(*(((TMODCALL void(**)(TAPTR,TINT))(LUASUPER(L)))[-85]))(lua5,lvl)

#define luaL_ref(lua5,t) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-86]))(lua5,t)

#define luaL_unref(lua5,t,ref) \
	(*(((TMODCALL void(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-87]))(lua5,t,ref)

#define luaL_objlen(lua5,t) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(LUASUPER(L)))[-88]))(lua5,t)

#define luaL_setn_obsolete(lua5,t,n) \
	(*(((TMODCALL void(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-89]))(lua5,t,n)

#define luaL_loadfile(lua5,fname) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR))(LUASUPER(L)))[-90]))(lua5,fname)

#define luaL_loadbuffer(lua5,buf,size,name) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSIZE,TSTRPTR))(LUASUPER(L)))[-91]))(lua5,buf,size,name)

#define luaL_gsub(lua5,s,p,r) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TSTRPTR,TSTRPTR,TSTRPTR))(LUASUPER(L)))[-92]))(lua5,s,p,r)

#define lua_getfield(lua5,idx,fname) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TINT,TSTRPTR))(LUASUPER(L)))[-93]))(lua5,idx,fname)

#define lua_setfield(lua5,idx,fname) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TINT,TSTRPTR))(LUASUPER(L)))[-94]))(lua5,idx,fname)

/* -- new in v1.0 (marks Lua5.1) -- */

#define lua_tointeger(lua5,idx) \
	(*(((TMODCALL lua_Integer(**)(TAPTR,TINT))(LUASUPER(L)))[-95]))(lua5,idx)

#define lua_tolstring(lua5,idx,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TINT,TSIZE *))(LUASUPER(L)))[-96]))(lua5,idx,len)

#define lua_pushthread(lua5) \
	(*(((TMODCALL TINT(**)(TAPTR))(LUASUPER(L)))[-97]))(lua5)

#define lua_createtable(lua5,narr,nrec) \
	(*(((TMODCALL void(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-98]))(lua5,narr,nrec)

#define lua_status(lua5) \
	(*(((TMODCALL TINT(**)(TAPTR))(LUASUPER(L)))[-99]))(lua5)

#define lua_gc(lua5,what,data) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TINT))(LUASUPER(L)))[-100]))(lua5,what,data)

#define lua_getallocf(lua5,ud) \
	(*(((TMODCALL lua_Alloc(**)(TAPTR,TAPTR *))(LUASUPER(L)))[-101]))(lua5,ud)

#define lua_setallocf(lua5,f,ud) \
	(*(((TMODCALL void(**)(TAPTR,lua_Alloc,TAPTR))(LUASUPER(L)))[-102]))(lua5,f,ud)

#define luaL_register(lua5,libname,l) \
	(*(((TMODCALL void(**)(TAPTR,TSTRPTR,luaL_Reg *))(LUASUPER(L)))[-103]))(lua5,libname,l)

#define luaL_checkinteger(lua5,numArg) \
	(*(((TMODCALL lua_Integer(**)(TAPTR,TINT))(LUASUPER(L)))[-104]))(lua5,numArg)

#define luaL_optinteger(lua5,nArg,def) \
	(*(((TMODCALL lua_Integer(**)(TAPTR,TINT,lua_Integer))(LUASUPER(L)))[-105]))(lua5,nArg,def)

#define luaL_checkoption(lua5,narg,def,lst) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TSTRPTR,TSTRPTR *))(LUASUPER(L)))[-106]))(lua5,narg,def,lst)

#define luaL_loadstring(lua5,s) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR))(LUASUPER(L)))[-107]))(lua5,s)

#define luaL_findtable(lua5,idx,fname,szhint) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TINT,TSTRPTR,TINT))(LUASUPER(L)))[-108]))(lua5,idx,fname,szhint)

#define luaL_buffinit(lua5,B) \
	(*(((TMODCALL void(**)(TAPTR,luaL_Buffer *))(LUASUPER(L)))[-109]))(lua5,B)

#define _luaL_prepbuffer(lua5,B) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,luaL_Buffer *))(LUASUPER(L)))[-110]))(lua5,B)

#define _luaL_addlstring(lua5,B,s,l) \
	(*(((TMODCALL void(**)(TAPTR,luaL_Buffer *,TSTRPTR,TSIZE))(LUASUPER(L)))[-111]))(lua5,B,s,l)

#define _luaL_addstring(lua5,B,s) \
	(*(((TMODCALL void(**)(TAPTR,luaL_Buffer *,TSTRPTR))(LUASUPER(L)))[-112]))(lua5,B,s)

#define _luaL_addvalue(lua5,B) \
	(*(((TMODCALL void(**)(TAPTR,luaL_Buffer *))(LUASUPER(L)))[-113]))(lua5,B)

#define _luaL_pushresult(lua5,B) \
	(*(((TMODCALL void(**)(TAPTR,luaL_Buffer *))(LUASUPER(L)))[-114]))(lua5,B)

#endif /* _TEK_STDCALL_LUA_H */
