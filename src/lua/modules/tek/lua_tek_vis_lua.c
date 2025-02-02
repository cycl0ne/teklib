
/*
**	$Id: lua_tek_vis_lua.c,v 1.1 2006/08/30 20:21:04 tmueller Exp $
**	teklib/src/lua/modules/vis/vis_mod.c - TEKlib extension module
**
**	standalone luaopen_tek_vis (TEKlib encapsulated)
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>
#include <tek/proto/hal.h>
#include <tek/proto/io.h>
#include <tek/proto/iohnd_default.h>
#include <tek/proto/visual.h>

#include "lua_tek_mod.h"

/* global module bases */
TAPTR TBaseTask = TNULL;
TAPTR TExecBase = TNULL;
TAPTR TTimeBase = TNULL;
TAPTR TUtilBase = TNULL;
TAPTR TimeRequest = TNULL;

/* table of module entrypoints */
static const struct TInitModule luavis_initmodules[] =
{
	{"hal", tek_init_hal, TNULL, 0},
	{"exec", tek_init_exec, TNULL, 0},
	{"time", tek_init_time, TNULL, 0},
	{"util", tek_init_util, TNULL, 0},
	{"io", tek_init_io, TNULL, 0},
	{"iohnd_default", tek_init_iohnd_default, TNULL, 0},
	{"visual", tek_init_visual, TNULL, 0},
	{TNULL}
};

static TVOID luavis_exit(lua_State *L)
{
	if (TTimeBase)
	{
		TFreeTimeRequest(TimeRequest);
		TimeRequest = TNULL;
		TCloseModule(TTimeBase);
		TTimeBase = TNULL;
	}
	TCloseModule(TUtilBase);
	TUtilBase = TNULL;
	TDestroy(TBaseTask);
	TBaseTask = TNULL;
}

EXPORT TINT
luaopen_tek_vis(lua_State *L)
{
	TTAGITEM tags[2];
	tags[0].tti_Tag = TExecBase_ModInit;
	tags[0].tti_Value = (TTAG) luavis_initmodules; 
	tags[1].tti_Tag = TTAG_DONE; 
 	
 	TBaseTask = TEKCreate(tags);
 	if (TBaseTask)
 	{
		TExecBase = TGetExecBase(TBaseTask);
		TUtilBase = TOpenModule("util", 0, TNULL);
		if (TUtilBase)
		{
			TTimeBase = TOpenModule("time", 0, TNULL);
			if (TTimeBase)
			{
				TimeRequest = TAllocTimeRequest(TNULL);
				if (TimeRequest)
					return luavis_init(L, TNULL, "tek.vis");
			}
		}
		luavis_exit(L);
		luaL_error(L, "Failed to initialze TEKlib modules");
	}
	
	luaL_error(L, "Failed to initialize TEKlib base context");
	return 0;
}
#warning unreviewed
