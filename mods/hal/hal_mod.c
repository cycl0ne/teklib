
/*
**	$Id: hal_mod.c,v 1.7 2005/09/13 02:42:16 tmueller Exp $
**	teklib/mods/hal/hal_mod.c - HAL module
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "hal_mod.h"
#include <tek/teklib.h>
#include <tek/mod/exec.h>
#include <tek/debug.h>

#define MOD_VERSION		2
#define MOD_REVISION	0
#define MOD_NUMVECTORS	31
static TCALLBACK TVOID hal_freedestroy(TMOD_HAL *mod);
static TCALLBACK TVOID hal_destroy(TMOD_HAL *mod);
static const TAPTR hal_vectors[MOD_NUMVECTORS];

/*****************************************************************************/
/* 
**	Module Init
*/

TMODENTRY TUINT
tek_init_hal(TAPTR selftask, TMOD_HAL *hal, TUINT16 version,
	TTAGITEM *tags)
{
	TMOD_HAL **halbaseptr;
	TAPTR boot;

	if (hal == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * MOD_NUMVECTORS; /* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_HAL); /* positive size */

		return 0;
	}

	boot = (TAPTR) TGetTag(tags, TExecBase_BootHnd, TNULL);

	halbaseptr = (TMOD_HAL **) TGetTag(tags, TExecBase_HAL, TNULL);
	*halbaseptr = TNULL;

	hal = hal_allocself(boot,
		sizeof(TMOD_HAL) + sizeof(TAPTR) * MOD_NUMVECTORS);

	if (!hal) return 0;
	hal = (TMOD_HAL *) (((TAPTR *) hal) + MOD_NUMVECTORS);

	hal_fillmem(hal, hal, sizeof(TMOD_HAL), 0);
	
	hal->hmb_BootHnd = boot;
	
	hal->hmb_Module.tmd_Version = MOD_VERSION;
	hal->hmb_Module.tmd_Revision = MOD_REVISION;
		
	hal->hmb_Module.tmd_Name = "hal";
	hal->hmb_Module.tmd_ModSuper = (struct TModule *) hal;
	hal->hmb_Module.tmd_NegSize = sizeof(TAPTR) * MOD_NUMVECTORS;
	hal->hmb_Module.tmd_PosSize = sizeof(TMOD_HAL);
	hal->hmb_Module.tmd_RefCount = 1;
	hal->hmb_Module.tmd_Flags = TMODF_INITIALIZED;

	hal->hmb_Module.tmd_OpenFunc = (TMODOPENFUNC) hal_open;
	hal->hmb_Module.tmd_CloseFunc = (TMODCLOSEFUNC) hal_close;
	hal->hmb_Module.tmd_DestroyFunc = (TDFUNC) hal_freedestroy;
	hal->hmb_Module.tmd_Flags |= TMODF_EXTENDED;
		
	TInitVectors(hal, hal_vectors, MOD_NUMVECTORS);
	
	if (hal_init(hal, tags))
	{
		*halbaseptr = hal;
		return TTRUE;
	}
	
	hal_freeself(boot, ((TAPTR *) hal) - MOD_NUMVECTORS,
		hal->hmb_Module.tmd_NegSize + hal->hmb_Module.tmd_PosSize);

	return TFALSE;
}

/*****************************************************************************/

static const TAPTR 
hal_vectors[MOD_NUMVECTORS] =
{
	(TAPTR) hal_beginio,
	(TAPTR) hal_abortio,
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */

	(TAPTR) hal_getattr,
	(TAPTR) hal_getsystime,

	(TAPTR) hal_alloc,
	(TAPTR) hal_free,
	(TAPTR) hal_realloc,
	(TAPTR) hal_copymem,
	(TAPTR) hal_fillmem,

	(TAPTR) hal_initlock,
	(TAPTR) hal_destroylock,
	(TAPTR) hal_lock,
	(TAPTR) hal_unlock,

	(TAPTR) hal_initthread,
	(TAPTR) hal_destroythread,
	(TAPTR) hal_findself,

	(TAPTR) hal_wait,
	(TAPTR) hal_signal,
	(TAPTR) hal_setsignal,

	(TAPTR) hal_loadmodule,
	(TAPTR) hal_callmodule,
	(TAPTR) hal_unloadmodule,

	(TAPTR) hal_scanmodules,

	(TAPTR) hal_datetojulian,
	(TAPTR) hal_juliantodate,
};

/*****************************************************************************/

static TCALLBACK TVOID
hal_destroy(TMOD_HAL *hal)
{
	hal_exit(hal);
}

static TCALLBACK TVOID
hal_freedestroy(TMOD_HAL *mod)
{
	hal_destroy(mod);
	hal_freeself(mod->hmb_BootHnd, ((TAPTR *) mod) - MOD_NUMVECTORS,
		mod->hmb_Module.tmd_NegSize + mod->hmb_Module.tmd_PosSize);
}

/*****************************************************************************/
/* 
**	Get a HAL attribute
*/

EXPORT TTAG
hal_getattr(TMOD_HAL *hal, TUINT tag, TTAG defval)
{
	return TGetTag((TTAGITEM *) hal->hmb_Specific, tag, defval);
}

/*****************************************************************************/
/*
**	TSubTime(a, b) - Subtract time: a - b -> a
**	TAddTime(a, b) - Add time: a + b -> a
**	TCmpTime(a, b) - a > b: 1, a < b: -1, a = b: 0
*/

LOCAL TVOID
hal_subtime(TTIME *a, TTIME *b)
{
	if (a->ttm_USec < b->ttm_USec)
	{
		a->ttm_Sec = a->ttm_Sec - b->ttm_Sec - 1;
		a->ttm_USec = 1000000 - (b->ttm_USec - a->ttm_USec);
	}
	else
	{
		a->ttm_Sec = a->ttm_Sec - b->ttm_Sec;
		a->ttm_USec = a->ttm_USec - b->ttm_USec;
	}
}

LOCAL TVOID
hal_addtime(TTIME *a, TTIME *b)
{
	a->ttm_Sec += b->ttm_Sec;
	a->ttm_USec += b->ttm_USec;
	if (a->ttm_USec >= 1000000)
	{
		a->ttm_USec -= 1000000;
		a->ttm_Sec++;
	}
}

LOCAL TINT
hal_cmptime(TTIME *a, TTIME *b)
{
	if (a->ttm_Sec < b->ttm_Sec) return -1;
	if (a->ttm_Sec > b->ttm_Sec) return 1;
	if (a->ttm_USec == b->ttm_USec) return 0;
	if (a->ttm_USec > b->ttm_USec) return 1;
	return -1;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: hal_mod.c,v $
**	Revision 1.7  2005/09/13 02:42:16  tmueller
**	updated copyright reference
**	
**	Revision 1.6  2005/09/07 23:57:04  tmueller
**	module interface extended
**	
**	Revision 1.5  2005/04/01 18:44:03  tmueller
**	Supplied boot argument to hal_allocself and hal_freeself
**	
**	Revision 1.4  2004/04/18 14:11:01  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.3  2004/02/07 05:03:08  tmueller
**	Time support functions added
**	
**	Revision 1.2  2004/01/18 03:52:48  tmueller
**	Version number bumped to 1.0; why the Dell not
**	
**	Revision 1.1.1.1  2003/12/11 07:18:49  tmueller
**	Krypton import
**	
**	Revision 1.4  2003/10/28 08:50:51  tmueller
**	Cleanup of HAL-internal structures, style and formatting updated
**	
**	Revision 1.3  2003/10/19 03:38:18  tmueller
**	Changed exec structure field names: Node, List, Handle, Module, ModuleEntry
**	
**	Revision 1.2  2003/10/18 21:17:37  tmueller
**	Adapted to the changed mod->handle field names
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/
