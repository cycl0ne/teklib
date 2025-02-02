
/*
**	$Id: ps2_mod.c,v 1.8 2007/05/19 14:04:32 fschulze Exp $
**	teklib/mods/ps2/ps2_mod.c - PS2 module setup
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/exec.h>
#include "ps2_mod.h"

static TMOD_PS2 *mod_open(TMOD_PS2 *TPS2Base, TTAGITEM *tags);
static TVOID mod_close(TMOD_PS2 *TPS2Base);
static const TMFPTR mod_vectors[MOD_NUMVECTORS];
static TVOID mod_exit(TMOD_PS2 *TPS2Base);

/*****************************************************************************/

static void mod_destroy(TMOD_PS2 *TPS2Base)
{
	struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;
	TDestroy(priv->ps2_MM);
	TDestroy(priv->ps2_Lock);
	TFree(priv);
}

static THOOKENTRY TTAG
vis_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	TMOD_PS2 *mod = (TMOD_PS2 *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			mod_destroy(mod);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) mod_open(mod, obj);
		case TMSG_CLOSEMODULE:
			mod_close(obj);
	}
	return 0;
}

TMODENTRY TUINT
tek_init_ps2common(struct TTask *task, TMOD_PS2 *TPS2Base, TUINT16 version, TTAGITEM *tags)
{
	struct TPS2ModPrivate *priv;

	if (TPS2Base == TNULL)
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
		priv->ps2_MM = TCreateMemManager(TNULL,
			TMMT_Tracking | TMMT_TaskSafe, TNULL);
		if (priv->ps2_MM)
		{
			priv->ps2_Lock = TCreateLock(TNULL);
			if (priv->ps2_Lock)
			{
				TPS2Base->ps2_GSInfo = &priv->ps2_GSInfo;
				TPS2Base->ps2_Private = priv;

				TPS2Base->ps2_Module.tmd_Version = MOD_VERSION;
				TPS2Base->ps2_Module.tmd_Revision = MOD_REVISION;

				TPS2Base->ps2_Module.tmd_Handle.thn_Hook.thk_Entry = vis_dispatch;
				TPS2Base->ps2_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;

				TInitVectors(TPS2Base, mod_vectors, MOD_NUMVECTORS);
				return TTRUE;
			}
			TDestroy(priv->ps2_MM);
		}
		TFree(priv);
	}

	return 0;
}

/*****************************************************************************/
/*
**	instance open
*/

static TMOD_PS2 *
mod_open(TMOD_PS2 *TPS2Base, TTAGITEM *tags)
{
	TMOD_PS2 *result = TPS2Base;
	struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;

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
			dma_initManager(TPS2Base, DMC_GIF, 1000000);
			priv->ps2_GSInfo.gsi_regs[0] = 0xdeadbeef;
			TPS2Base->ps2_DMADebug = TFALSE;
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

static TVOID
mod_close(TMOD_PS2 *TPS2Base)
{
	struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;
	TLock(priv->ps2_Lock);
	if (TUtilBase)
	{
		if (--priv->ps2_RefCount == 0)
		{
			mod_exit(TPS2Base);
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

static const TMFPTR
mod_vectors[MOD_NUMVECTORS] =
{
	(TMFPTR) TNULL,		/* reserved */
	(TMFPTR) TNULL,		/* reserved */
	(TMFPTR) TNULL,		/* reserved */
	(TMFPTR) TNULL,		/* reserved */
	(TMFPTR) TNULL,		/* reserved */
	(TMFPTR) TNULL,		/* reserved */
	(TMFPTR) TNULL,		/* reserved */
	(TMFPTR) TNULL,		/* reserved */

	(TMFPTR) util_dumpreg,
	(TMFPTR) util_hexdump,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) dma_initManager,
	(TMFPTR) dma_alloc,
	(TMFPTR) dma_commit,
	(TMFPTR) dma_allocspr,
	(TMFPTR) dma_allocCall,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) gs_setReg,
	(TMFPTR) gs_vsync,
	(TMFPTR) gs_enableZBuf,
	(TMFPTR) gs_clearScreen,
	(TMFPTR) gs_clearScreenAlpha,
	(TMFPTR) gs_flipBuffer,
	(TMFPTR) gs_setClearColor,
	(TMFPTR) gs_setDepthFunc,
	(TMFPTR) gs_setDepthClear,
	(TMFPTR) gs_initImage,
	(TMFPTR) gs_loadImage,
	(TMFPTR) gs_freeImage,
	(TMFPTR) gs_initScreen,
	(TMFPTR) gs_initDisplay,
	(TMFPTR) gs_initFramebuffer,
	(TMFPTR) gs_enableContext,
	(TMFPTR) gs_initZBuf,
	(TMFPTR) gs_initTexEnv,
	(TMFPTR) gs_initTexReg,
	(TMFPTR) gs_init,
	(TMFPTR) gs_allocMem,
	(TMFPTR) gs_freeMem,
	(TMFPTR) gs_drawTRect,
	(TMFPTR) gs_drawFRect,
	(TMFPTR) gs_drawRect,
	(TMFPTR) gs_drawLine,
	(TMFPTR) gs_drawPoint,
	(TMFPTR) gs_drawTRectUV,
	(TMFPTR) gs_drawTPointUV,
	(TMFPTR) gs_syncPath,
	(TMFPTR) gs_getImage,
	(TMFPTR) gs_setActiveFb,
	(TMFPTR) gs_getReg,
	(TMFPTR) gs_set_csr,
	(TMFPTR) gs_set_pmode,
	(TMFPTR) gs_set_smode2,
	(TMFPTR) gs_set_dispfb1,
	(TMFPTR) gs_set_display1,
	(TMFPTR) gs_set_dispfb2,
	(TMFPTR) gs_set_display2,
	(TMFPTR) gs_set_bgcolor,
	(TMFPTR) gs_setRegCb,
	(TMFPTR) gs_setVisibleFb
};

/*****************************************************************************/
/*
**	mod_exit
**	cleanup
*/

static TVOID
mod_exit(TMOD_PS2 *TPS2Base)
{
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_mod.c,v $
**	Revision 1.8  2007/05/19 14:04:32  fschulze
**	g_initContext renamed to g_initFramebuffer
**
**	Revision 1.7  2006/03/26 14:28:12  fschulze
**	init DMA debug flag
**
**	Revision 1.6  2006/03/10 17:01:34  fschulze
**	added gs_setRegCb() and gs_setVisibleFb()
**
**	Revision 1.5  2006/02/24 15:44:40  fschulze
**	renamed modbase to TPS2Base; added initialization of
**	priv->ps2_GSInfo.gsi_regs; adapted to renamed ps2 common
**	functions; added new gs functions to modvector
**
**	Revision 1.4  2005/11/20 16:18:38  tmueller
**	type definition for module functions added
**
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
