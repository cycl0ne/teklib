
/* 
**	$Id: ps2_mod.c,v 1.3 2005/10/07 12:22:06 fschulze Exp $
**	teklib/mods/ps2/ps2_mod.c - PS2 module setup
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/exec.h>
#include "ps2_mod.h"

static TCALLBACK TMOD_PS2 *mod_open(TMOD_PS2 *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_PS2 *mod, TAPTR task);
static TCALLBACK TVOID mod_destroy(TMOD_PS2 *mod);
static const TAPTR mod_vectors[MOD_NUMVECTORS];
static TVOID mod_exit(TMOD_PS2 *mod);

/*****************************************************************************/
/*
**	module init function
*/

TMODENTRY TUINT 
tek_init_ps2common(TAPTR task, TMOD_PS2 *mod, TUINT16 version, TTAGITEM *tags)
{
	struct TPS2ModPrivate *priv;
	
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * MOD_NUMVECTORS;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_PS2);				/* positive size */

		return 0;
	}

	priv = TAlloc(TNULL, sizeof(struct TPS2ModPrivate));
	if (priv)
	{
		priv->ps2_MMU = TCreateMMU(TNULL, TMMUT_Tracking | TMMUT_TaskSafe, TNULL);
		if (priv->ps2_MMU)
		{
			priv->ps2_Lock = TCreateLock(TNULL);
			if (priv->ps2_Lock)
			{
				mod->ps2_GSInfo = &priv->ps2_GSInfo;
				mod->ps2_Private = priv;

				mod->ps2_Module.tmd_Version = MOD_VERSION;
				mod->ps2_Module.tmd_Revision = MOD_REVISION;
				mod->ps2_Module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
				mod->ps2_Module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;
				mod->ps2_Module.tmd_DestroyFunc = (TDFUNC) mod_destroy;
			
				TInitVectors(mod, (TAPTR *) mod_vectors, MOD_NUMVECTORS);
				return TTRUE;
			}
			TDestroy(priv->ps2_MMU);
		}
		TFree(priv);
	}

	return 0;
}

TMODENTRY TUINT 
tek_init_ps2common_old(TAPTR task, TMOD_PS2 *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)					/* first call */
		{
			if (version <= MOD_VERSION)			/* version check */
			{
				return sizeof(TMOD_PS2);		/* module positive size */
			}
		}
		else									/* second call */
		{
			return sizeof(TAPTR) * MOD_NUMVECTORS;
		}
	}
	else										/* third call */
	{
		struct TPS2ModPrivate *priv;
		priv = TAlloc(TNULL, sizeof(struct TPS2ModPrivate));
		if (priv)
		{
			priv->ps2_MMU = TCreateMMU(TNULL, TMMUT_Tracking | TMMUT_TaskSafe, TNULL);
			if (priv->ps2_MMU)
			{
				priv->ps2_Lock = TCreateLock(TNULL);
				if (priv->ps2_Lock)
				{
					mod->ps2_GSInfo = &priv->ps2_GSInfo;
					mod->ps2_Private = priv;

					mod->ps2_Module.tmd_Version = MOD_VERSION;
					mod->ps2_Module.tmd_Revision = MOD_REVISION;
					mod->ps2_Module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
					mod->ps2_Module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;
					mod->ps2_Module.tmd_DestroyFunc = (TDFUNC) mod_destroy;
				
					TInitVectors(mod, (TAPTR *) mod_vectors, MOD_NUMVECTORS);
					return TTRUE;
				}
				TDestroy(priv->ps2_MMU);
			}
			TFree(priv);
		}
	}

	return 0;
}

/*****************************************************************************/
/*
**	module exit function
*/

static TCALLBACK TVOID mod_destroy(TMOD_PS2 *mod)
{
	struct TPS2ModPrivate *priv = mod->ps2_Private;
	TDestroy(priv->ps2_MMU);
	TDestroy(priv->ps2_Lock);
	TFree(priv);
}

/*****************************************************************************/
/*
**	instance open
*/

static TCALLBACK TMOD_PS2 *
mod_open(TMOD_PS2 *mod, TAPTR task, TTAGITEM *tags)
{
	TMOD_PS2 *result = mod;
	struct TPS2ModPrivate *priv = mod->ps2_Private;

	TLock(priv->ps2_Lock);
	if (TUtilBase)
	{
		priv->ps2_RefCount++;
	}
	else
	{
		THALBase = TGetHALBase();
		TUtilBase = TOpenModule("util", 0, TNULL);
		if (TUtilBase)
		{
			priv->ps2_RefCount = 1;
			d_initManager(mod, DMC_GIF, 1000000);
		}
		else
		{
			result = TNULL;
        }
	}
	TUnlock(priv->ps2_Lock);

	return result;
}

/*****************************************************************************/
/*
**	instance close
*/

static TCALLBACK TVOID 
mod_close(TMOD_PS2 *mod, TAPTR task)
{
	struct TPS2ModPrivate *priv = mod->ps2_Private;
	TLock(priv->ps2_Lock);
	if (TUtilBase)
	{
		if (--priv->ps2_RefCount == 0)
		{
			mod_exit(mod);
			TCloseModule(TUtilBase);
			TUtilBase = TNULL;
		}
	}
	TUnlock(priv->ps2_Lock);
}

/*****************************************************************************/
/*
**	function vector table
*/

static const TAPTR
mod_vectors[MOD_NUMVECTORS] =
{
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,		/* reserved */	
	(TAPTR) TNULL,		/* reserved */
					
	(TAPTR) u_dumpreg,
	(TAPTR) u_hexdump,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	
	(TAPTR) d_initManager,
	(TAPTR) d_alloc,
	(TAPTR) d_commit,
	(TAPTR) d_allocspr,
	(TAPTR) d_allocCall,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	
	(TAPTR) g_setReg,
	(TAPTR) g_vsync,
	(TAPTR) g_enableZBuf,
	(TAPTR) g_clearScreen,
	(TAPTR) g_clearScreenAlpha,
	(TAPTR) g_flipBuffer,
	(TAPTR) g_setClearColor,
	(TAPTR) g_setDepthFunc,
	(TAPTR) g_setDepthClear,
	(TAPTR) g_initImage,
	(TAPTR) g_loadImage,
	(TAPTR) g_freeImage,
	(TAPTR) g_initScreen,
	(TAPTR) g_initDisplay,
	(TAPTR) g_initContext,
	(TAPTR) g_enableContext,
	(TAPTR) g_initZBuf,
	(TAPTR) g_initTexEnv,
	(TAPTR) g_initTexReg,
	(TAPTR) g_init,
	(TAPTR) g_allocMem,
	(TAPTR) g_freeMem,
	(TAPTR) g_drawTRect,
	(TAPTR) g_drawFRect,
	(TAPTR) g_drawRect,
	(TAPTR) g_drawLine,
	(TAPTR) g_drawPoint,
	(TAPTR) g_drawTRectUV,
	(TAPTR) g_drawTPointUV,
	(TAPTR) g_syncPath,
	(TAPTR) g_getImage,
	(TAPTR) g_setActiveFb
};

/*****************************************************************************/
/*
**	mod_exit
**	cleanup
*/

static TVOID 
mod_exit(TMOD_PS2 *mod)
{
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_mod.c,v $
**	Revision 1.3  2005/10/07 12:22:06  fschulze
**	renamed some primitive drawing functions
**	
**	Revision 1.2  2005/10/05 22:03:55  fschulze
**	added new gs functions: local mem downloads, syncing of paths and
**	activation of framebuffers; renamed setactivebuf to setvisiblebuf;
**	fixed interface definition of GSVSync
**	
**	Revision 1.1  2005/09/18 12:38:11  fschulze
**	added
**	
**	
*/
