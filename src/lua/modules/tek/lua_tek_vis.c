
/*
**	$Id: lua_tek_vis.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/lua/modules/tek/lua_tek_vis.c - TEKlib extension module
**
**	Common module startup (luavis_init)
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "lua_tek_mod.h"

#define LUACLASSNAME_TEK_VISUAL_PEN "tpen*"
#define LUACLASSNAME_TEK_VISUAL "tvis*"

/*****************************************************************************/
/*
**	Class implementation
*/

static TAPTR *
luavis_getinstptr(lua_State *L, TINT narg, TSTRPTR classname)
{
	TAPTR *pinst = luaL_checkudata(L, narg, classname);
	if (*pinst) return pinst;
	luaL_argerror(L, narg, "Closed handle");
	return TNULL;
}

static LUACFUNC TINT
luavis_open(lua_State *L)
{
	TTAGITEM tags[8];

 	struct LuaTEKBase *mod = lua_touserdata(L, lua_upvalueindex(1));
	TAPTR *pvis = lua_newuserdata(L, sizeof(TAPTR));	/* s: udata */

	*pvis = TNULL;
	/* upvalueindex(2) refers to the class metatable */
	lua_pushvalue(L, lua_upvalueindex(2));				/* s: udata, mt */
	/* attach metatable to the userdata object */
	lua_setmetatable(L, -2);							/* s: udata */
	/* create class instance */

	tags[0].tti_Tag = TVisual_Title;
	tags[0].tti_Value = (TTAG) luaL_optstring(L, 1, "TEKlib visual");
	tags[1].tti_Tag = TVisual_PixWidth;
	tags[1].tti_Value = (TTAG) luaL_optnumber(L, 2, -1);
	tags[2].tti_Tag = TVisual_PixHeight;
	tags[2].tti_Value = (TTAG) luaL_optnumber(L, 3, -1);
	tags[3].tti_Tag = TVisual_MinWidth;
	tags[3].tti_Value = (TTAG) luaL_optnumber(L, 4, -1);
	tags[4].tti_Tag = TVisual_MinHeight;
	tags[4].tti_Value = (TTAG) luaL_optnumber(L, 5, -1);
	tags[5].tti_Tag = TVisual_MaxWidth;
	tags[5].tti_Value = (TTAG) luaL_optnumber(L, 6, -1);
	tags[6].tti_Tag = TVisual_MaxHeight;
	tags[6].tti_Value = (TTAG) luaL_optnumber(L, 7, -1);
	tags[7].tti_Tag = TTAG_DONE;

	*pvis = TOpenModule("visual", 0, tags);
	if (*pvis == TNULL)
	{
		lua_pushstring(L, "Could not create visual");
		lua_error(L);
	}
	return 1;
}

static LUACFUNC TINT
luavis_close(lua_State *L)
{
 	struct LuaTEKBase *mod = lua_touserdata(L, lua_upvalueindex(1));
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TCloseModule(*pv);
	*pv = TNULL;	/* mark as closed */
	return 0;
}

static LUACFUNC TINT
luavis_delay(lua_State *L)
{
 	struct LuaTEKBase *mod = lua_touserdata(L, lua_upvalueindex(1));
 	TDOUBLE sec = luaL_checknumber(L, 1);
 	TTIME dt;

 	dt.ttm_Sec = sec;
 	dt.ttm_USec = sec * 1000000;

 	TDelay(TimeRequest, &dt);
	return 0;
}

static LUACFUNC TINT
luavis_gc(lua_State *L)
{
	return luavis_close(L);
}

static const struct luavis_itypes {TUINT mask; TSTRPTR name; } itypes[] =
{
	{ TITYPE_CLOSE, "close" },
	{ TITYPE_FOCUS, "focus" },
	{ TITYPE_NEWSIZE, "newsize" },
	{ TITYPE_REFRESH, "refresh" },
	{ TITYPE_MOUSEOVER, "mouseover" },
	{ TITYPE_COOKEDKEY, "cookedkey" },
	{ TITYPE_MOUSEMOVE, "mousemove" },
	{ TITYPE_MOUSEBUTTON, "mousebutton" },
	{ TITYPE_INTERVAL, "interval" },
	{ TITYPE_ALL, "all" },
	{ 0, TNULL }
};

static TUINT
luavis_getinputmask(lua_State *L, struct LuaTEKBase *mod)
{
	TUINT i, sig = 0, narg = lua_gettop(L) - 1;
	for (i = 0; i < narg; ++i)
	{
 		const struct luavis_itypes *fp = itypes;
		TUINT newsig = 0;
		while (fp->mask)
		{
 			if (TStrCaseCmp(fp->name, (TSTRPTR) luaL_checkstring(L, 2 + i)) == 0)
			{
				newsig = fp->mask;
				break;
			}
			fp++;
		}
		if (newsig == 0)
			luaL_argerror(L, i + 2, "Unknown keyword");
		sig |= newsig;
	}
	return sig;
}

static LUACFUNC TINT
luavis_setinput(lua_State *L)
{
	struct LuaTEKBase *mod = lua_touserdata(L, lua_upvalueindex(1));
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TVisualSetInput(*pv, 0, luavis_getinputmask(L, mod));
	return 0;
}

static LUACFUNC TINT
luavis_clearinput(lua_State *L)
{
	struct LuaTEKBase *mod = lua_touserdata(L, lua_upvalueindex(1));
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TVisualSetInput(*pv, luavis_getinputmask(L, mod), 0);
	return 0;
}

/*
**	wait for one or a set of visuals. returns true for each visual
**	that has messages pending
*/

static LUACFUNC TINT
luavis_wait(lua_State *L)
{
	struct LuaTEKBase *mod = lua_touserdata(L, lua_upvalueindex(1));
	TINT narg = lua_gettop(L);
	TUINT sigmask = 0, sigs;
	TINT i;

	for (i = 0; i < narg; ++i)
	{
		TAPTR *pv = luaL_checkudata(L, i + 1, LUACLASSNAME_TEK_VISUAL);
		if (*pv == TNULL)
			luaL_argerror(L, i + 1, "Closed handle");
		sigmask |= TGetPortSignal(TVisualGetPort(*pv));
	}

	sigs = TWait(sigmask);

	for (i = 0; i < narg; ++i)
	{
		TAPTR *pv = luaL_checkudata(L, i + 1, LUACLASSNAME_TEK_VISUAL);
		lua_pushboolean(L, (sigs & TGetPortSignal(TVisualGetPort(*pv))));
	}

	return narg;
}

static LUACFUNC TINT
luavis_getmsg(lua_State *L)
{
	struct LuaTEKBase *mod = lua_touserdata(L, lua_upvalueindex(1));
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TIMSG *imsg = (TIMSG *) TGetMsg(TVisualGetPort(*pv));
	if (imsg)
	{
		const struct luavis_itypes *fp = itypes;
		while (fp->mask)
		{
			if (fp->mask == imsg->timsg_Type)
				break;
			fp++;
		}
		if (fp->name == TNULL)
		{
			lua_pushstring(L, "Unknown message at port");
			lua_error(L);
		}

		lua_newtable(L);
		lua_pushnumber(L, (lua_Number) imsg->timsg_TimeStamp.ttm_Sec * 1000 + imsg->timsg_TimeStamp.ttm_USec / 1000);
		lua_rawseti(L, -2, 1);
		lua_pushnumber(L, imsg->timsg_Type);
		lua_rawseti(L, -2, 2);
		if (imsg->timsg_Type != TITYPE_INTERVAL)
		{
			lua_pushnumber(L, imsg->timsg_Code);
			lua_rawseti(L, -2, 3);
			lua_pushnumber(L, imsg->timsg_MouseX);
			lua_rawseti(L, -2, 4);
			lua_pushnumber(L, imsg->timsg_MouseY);
			lua_rawseti(L, -2, 5);
			lua_pushnumber(L, imsg->timsg_Qualifier);
			lua_rawseti(L, -2, 6);
			if (imsg->timsg_Type == TITYPE_REFRESH)
			{
				lua_pushnumber(L, imsg->timsg_X);
				lua_rawseti(L, -2, 7);
				lua_pushnumber(L, imsg->timsg_Y);
				lua_rawseti(L, -2, 8);
				lua_pushnumber(L, imsg->timsg_X + imsg->timsg_Width - 1);
				lua_rawseti(L, -2, 9);
				lua_pushnumber(L, imsg->timsg_Y + imsg->timsg_Height - 1);
				lua_rawseti(L, -2, 10);
			}
		}

		TAckMsg(imsg);
		return 1;
	}
	return 0;
}

static LUACFUNC TINT
luavis_allocpen(lua_State *L)
{
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TVPEN *ppen;

	TINT r = luaL_checknumber(L, 2);
	TINT g = luaL_checknumber(L, 3);
	TINT b = luaL_checknumber(L, 4);

	r = TCLAMP(0, r, 255);
	g = TCLAMP(0, g, 255);
	b = TCLAMP(0, b, 255);

	/* reserve userdata for a collectable object */
	ppen = lua_newuserdata(L, sizeof(TVPEN));

	*ppen = TVisualAllocPen(*pv, (r << 16) | (g << 8) | b);

	/* attach class metatable to userdata object */
	luaL_newmetatable(L, LUACLASSNAME_TEK_VISUAL_PEN);
	lua_setmetatable(L, -2);

	return 1;
}

static LUACFUNC TINT
luavis_freepen(lua_State *L)
{
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TVPEN *ppen = (TVPEN *) luavis_getinstptr(L, 2, LUACLASSNAME_TEK_VISUAL_PEN);
	TVisualFreePen(*pv, *ppen);
	return 0;
}

static LUACFUNC TINT
luavis_frect(lua_State *L)
{
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TINT x0 = luaL_checknumber(L, 2);
	TINT y0 = luaL_checknumber(L, 3);
	TINT x1 = luaL_checknumber(L, 4);
	TINT y1 = luaL_checknumber(L, 5);
	TVPEN *ppen = luaL_checkudata(L, 6, LUACLASSNAME_TEK_VISUAL_PEN);
	TVisualFRect(*pv, x0, y0, x1 - x0 + 1, y1 - y0 + 1, *ppen);
	return 0;
}

static LUACFUNC TINT
luavis_rect(lua_State *L)
{
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TINT x0 = luaL_checknumber(L, 2);
	TINT y0 = luaL_checknumber(L, 3);
	TINT x1 = luaL_checknumber(L, 4);
	TINT y1 = luaL_checknumber(L, 5);
	TVPEN *ppen = luaL_checkudata(L, 6, LUACLASSNAME_TEK_VISUAL_PEN);
	TVisualRect(*pv, x0, y0, x1 - x0 + 1, y1 - y0 + 1, *ppen);
	return 0;
}

static LUACFUNC TINT
luavis_line(lua_State *L)
{
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TDOUBLE x0 = luaL_checknumber(L, 2);
	TDOUBLE y0 = luaL_checknumber(L, 3);
	TDOUBLE x1 = luaL_checknumber(L, 4);
	TDOUBLE y1 = luaL_checknumber(L, 5);
	TVPEN *ppen = luaL_checkudata(L, 6, LUACLASSNAME_TEK_VISUAL_PEN);
	TVisualLine(*pv, x0, y0, x1, y1, *ppen);
	return 0;
}

static LUACFUNC TINT
luavis_getattrs(lua_State *L)
{
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TINT pw, ph, tw, th, fw, fh;
	TTAGITEM tags[7];

	tags[0].tti_Tag = TVisual_PixWidth;
	tags[0].tti_Value = (TTAG) &pw;
	tags[1].tti_Tag = TVisual_PixHeight;
	tags[1].tti_Value = (TTAG) &ph;
/*	tags[2].tti_Tag = TVisual_TextWidth;
	tags[2].tti_Value = (TTAG) &tw;
	tags[3].tti_Tag = TVisual_TextHeight;
	tags[3].tti_Value = (TTAG) &th;
	tags[4].tti_Tag = TVisual_FontWidth;
	tags[4].tti_Value = (TTAG) &fw;
	tags[5].tti_Tag = TVisual_FontHeight;
	tags[5].tti_Value = (TTAG) &fh;*/
	tags[2].tti_Tag = TTAG_DONE;

	TVisualGetAttrs(*pv, tags);
	lua_pushnumber(L, pw);
	lua_pushnumber(L, ph);
/*	lua_pushnumber(L, tw);
	lua_pushnumber(L, th);
	lua_pushnumber(L, fw);
	lua_pushnumber(L, fh);*/
	return 2;
}

static LUACFUNC TINT
luavis_text(lua_State *L)
{
	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TSIZE tlen;
	TDOUBLE x0 = luaL_checknumber(L, 2);
	TDOUBLE y0 = luaL_checknumber(L, 3);
	TSTRPTR text = (TSTRPTR) luaL_checklstring(L, 4, &tlen);
	TVPEN *fpen = luaL_checkudata(L, 5, LUACLASSNAME_TEK_VISUAL_PEN);
	if (!lua_isuserdata(L, 6))
		TVisualText(*pv, x0, y0, text, tlen, *fpen, TVPEN_UNDEFINED);
	else
	{
		TVPEN *bpen = luaL_checkudata(L, 6, LUACLASSNAME_TEK_VISUAL_PEN);
		TVisualText(*pv, x0, y0, text, tlen, *fpen, *bpen);
	}
	return 0;
}

static LUACFUNC TINT
luavis_setattrs(lua_State *L)
{
 	TAPTR *pv = luavis_getinstptr(L, 1, LUACLASSNAME_TEK_VISUAL);
	TTAGITEM tags[3], *tp = tags;

	lua_getfield(L, 2, "MinWidth");
	if (lua_isnumber(L, 3))
	{
		tp->tti_Tag = (TTAG) TVisual_MinWidth;
		tp->tti_Value = (TTAG) lua_tonumber(L, 3);
		tp++;
		lua_pop(L, 1);
	}

	lua_getfield(L, 2, "MinHeight");
	if (lua_isnumber(L, 3))
	{
		tp->tti_Tag = (TTAG) TVisual_MinHeight;
		tp->tti_Value = (TTAG) lua_tonumber(L, 3);
		tp++;
		lua_pop(L, 1);
	}

	tp->tti_Tag = TTAG_DONE;
	lua_pushnumber(L, TVisualSetAttrs(*pv, tags));

	return 1;
}

/*****************************************************************************/
/*
**	Module setup
*/

static const luaL_Reg luavis_methods[] =
{
	{"setinput", luavis_setinput},
	{"clearinput", luavis_clearinput},
	{"close", luavis_close},
	{"getmsg", luavis_getmsg},
	{"allocpen", luavis_allocpen},
	{"freepen", luavis_freepen},
	{"frect", luavis_frect},
	{"rect", luavis_rect},
	{"line", luavis_line},
	{"text", luavis_text},
	{"getattrs", luavis_getattrs},
	{"setattrs", luavis_setattrs},
	{"__gc", luavis_gc},
	{TNULL, TNULL}
};

static const luaL_Reg luavis_lib[] =
{
	{"open", luavis_open},
	{"wait", luavis_wait},
	{"delay", luavis_delay},
	{NULL, NULL}
};

static TVOID
luavis_addclass(lua_State *L, TSTRPTR libname, TSTRPTR classname,
	luaL_Reg *functions, luaL_Reg *methods, TAPTR userdata)
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

LOCAL TINT
luavis_init(lua_State *L, TAPTR mod, TSTRPTR name)
{
	luavis_addclass(L, name, LUACLASSNAME_TEK_VISUAL, (luaL_Reg *) luavis_lib,
		(luaL_Reg *) luavis_methods, mod);
	return 0;
}
