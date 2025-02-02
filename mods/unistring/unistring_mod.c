
/*
**	$Id: unistring_mod.c,v 1.35 2005/09/13 02:42:42 tmueller Exp $
**	mods/unistring/unistring_mod.c - Dynamic Unicode string module
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include "unistring_mod.h"

#define MOD_NUMVECTORS	62

static const TAPTR mod_vectors[MOD_NUMVECTORS];

static TCALLBACK TVOID mod_destroy(TMOD_US *mod);
static TCALLBACK TMOD_US *mod_open(TMOD_US *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_US *mod, TAPTR task);
static TBOOL mod_createpool(TMOD_US *mod);

EXPORT TUINT8 *convert_big(TAPTR mod, TUINT8 *src, TUINT8 *temp,
	TINT oldels, TINT newels);
EXPORT TUINT8 *convert_little(TAPTR mod, TUINT8 *src, TUINT8 *temp,
	TINT oldels, TINT newels);

/*****************************************************************************/
/*
**	module initialization
*/

TMODENTRY TUINT
tek_init_unistring(TAPTR task, TMOD_US *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)						/* first call */
		{
			if (version <= MOD_VERSION)				/* version check */
			{
				return sizeof(TMOD_US);				/* module positive size */
			}
		}
		else										/* second call */
		{
			return sizeof(TAPTR) * MOD_NUMVECTORS;	/* negative size */
		}
	}
	else											/* third call */
	{
		mod->us_Lock = TExecCreateLock(TExecBase, TNULL);
		if (mod->us_Lock)
		{
			#ifdef TDEBUG
				mod->us_MMU = TExecCreateMMU(TExecBase, TNULL, 
					TMMUT_Tracking | TMMUT_TaskSafe, TNULL);
			#endif

			if (mod_initcaseconversion(mod))
			{
				if (mod_createpool(mod))
				{
					TUINT8 endiancheck[2] = { 0x11, 0x22 };
					mod->us_BigEndian = (*((TUINT16 *) &endiancheck) == 0x1122);
					mod->us_InitNodeSize = DEFNODESIZE;
	
					mod->us_Module.tmd_Version = MOD_VERSION;
					mod->us_Module.tmd_Revision = MOD_REVISION;
					mod->us_Module.tmd_DestroyFunc = (TDFUNC) mod_destroy;
					
					mod->us_Module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
					mod->us_Module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;

					mod->us_Module.tmd_Flags |= TMODF_EXTENDED;
		
					/* setup global function table for the super instance */
					TInitVectors(mod, (TAPTR *) mod_vectors, MOD_NUMVECTORS);
	
					if (mod->us_BigEndian)
					{
						((TAPTR *) mod)[-4-8] = (TAPTR) convert_big;
					}
		
					return TTRUE;
				}
			}
			
			mod_destroy(mod);
		}
	}

	return 0;
}

/*****************************************************************************/

static TBOOL
mod_createpool(TMOD_US *mod)
{
	TTAGITEM pooltags[2];
	pooltags[0].tti_Tag = TPool_MMU;
	pooltags[0].tti_Value = (TTAG) mod->us_MMU;
	pooltags[1].tti_Tag = TTAG_DONE;
	#if 0
		pooltags[1].tti_Tag = TPool_PudSize;
		pooltags[1].tti_Value = (TTAG) 4096;
		pooltags[2].tti_Tag = TPool_ThresSize;
		pooltags[2].tti_Value = (TTAG) 2048;
		pooltags[3].tti_Tag = TPool_AutoAdapt;
		pooltags[3].tti_Value = (TTAG) TFALSE;
		pooltags[4].tti_Tag = TMem_LowFrag;
		pooltags[4].tti_Value = (TTAG) TTRUE;
	#endif
	mod->us_Pool = TExecCreatePool(TExecBase, pooltags);
	return (TBOOL) (mod->us_Pool != TNULL);
}

/*****************************************************************************/

static TCALLBACK TVOID
mod_destroy(TMOD_US *mod)
{
	TExecFree(TExecBase, mod->us_CharInfo);
	TExecFree(TExecBase, mod->us_SmallToCaps);
	TExecFree(TExecBase, mod->us_CapsToSmall);
	TDestroy(mod->us_Pool);
	TDestroy(mod->us_MMU);
	TDestroy(mod->us_Lock);
	
	#ifdef TDEBUG
		if (mod->us_AllocCount)
		{
			tdbprintf1(20,"alloccount: %d\n", mod->us_AllocCount);
		}
	#endif
}

/*****************************************************************************/

static TCALLBACK TMOD_US *
mod_open(TMOD_US *mod, TAPTR task, TTAGITEM *tags)
{
	if (TGetTag(tags, TUString_Local, TFALSE) == TFALSE)
	{
		/* Drop through; operate on the global super instance */
		tdbprintf(5,"operating on global instance\n");
		return mod;
	}

	/* Get a new, a local instance */
	mod = TNewInstance(mod, 
		mod->us_Module.tmd_PosSize, mod->us_Module.tmd_NegSize);
	if (mod)
	{
		TINT fs = (TINT) TGetTag(tags, TUString_FragSize, (TTAG) DEFNODESIZE);
		mod->us_InitNodeSize = TMAX(8, fs);
	
		/* Create an instance-specific memory pool */
		if (mod_createpool(mod))
		{
			/* overwrite function vector table with local functions */
			((TAPTR *) mod)[-1-8] = (TAPTR) _array_allocnode;
			((TAPTR *) mod)[-2-8] = (TAPTR) _array_freenode;
			((TAPTR *) mod)[-5-8] = (TAPTR) _array_alloc;
			((TAPTR *) mod)[-6-8] = (TAPTR) _array_free;
			((TAPTR *) mod)[-19-8] = (TAPTR) _array_move;
			((TAPTR *) mod)[-21-8] = (TAPTR) _array_free;
			((TAPTR *) mod)[-40-8] = (TAPTR) _array_move;

			tdbprintf(5,"operating on local instance\n");
			return mod;
		}
		TFreeInstance(mod);
	}
	
	return mod;
}

/*****************************************************************************/

static TCALLBACK TVOID
mod_close(TMOD_US *mod, TAPTR task)
{
	if ((TMOD_US *) mod->us_Module.tmd_ModSuper != mod)
	{
		/* Destroy instance-specific memory pool */
		TDestroy(mod->us_Pool);
		TFreeInstance(mod);
	}	
}

/*****************************************************************************/
/*
**	Function vector table (default)
*/

static const TAPTR 
mod_vectors[MOD_NUMVECTORS] =
{
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,

	(TAPTR) _array_allocnode_tasksafe,
	(TAPTR) _array_freenode_tasksafe,
	(TAPTR) array_debugprint,
	(TAPTR) convert_little,

	(TAPTR) _array_alloc_tasksafe,
	(TAPTR) _array_free_tasksafe,
	(TAPTR) array_ins,
	(TAPTR) array_rem,
	(TAPTR) array_seek,
	(TAPTR) array_get,
	(TAPTR) array_set,
	(TAPTR) array_length,
	(TAPTR) array_map,
	(TAPTR) array_render,
	(TAPTR) array_change,
	(TAPTR) array_dup,
	(TAPTR) array_copy,
	(TAPTR) array_trunc,
	(TAPTR) _array_move_tasksafe,
	
	(TAPTR) str_alloc,
	(TAPTR) _array_free_tasksafe,	/* same as array func */
	(TAPTR) str_insert,
	(TAPTR) str_remove,
	(TAPTR) array_length,			/* same as array func */
	(TAPTR) str_map,
	(TAPTR) str_render,
	(TAPTR) str_set,
	(TAPTR) str_get,
	(TAPTR) str_dup,
	(TAPTR) array_copy,				/* same as array func */
	(TAPTR) str_insertdstr,
	(TAPTR) str_insertstrn,
	(TAPTR) str_encodeutf8,
	(TAPTR) str_insertutf8str,
	(TAPTR) str_ncmp,
	(TAPTR) str_crop,
	(TAPTR) str_transform,
	(TAPTR) str_parsepattern,
	(TAPTR) str_matchpattern,
	(TAPTR) _array_move_tasksafe,	/* same as array func */
	(TAPTR) str_addpart,

	(TAPTR) str_isalnum,
	(TAPTR) str_isalpha,
	(TAPTR) str_iscntrl,
	(TAPTR) str_isgraph,
	(TAPTR) str_islower,
	(TAPTR) str_isprint,
	(TAPTR) str_ispunct,
	(TAPTR) str_isspace,
	(TAPTR) str_isupper,
	(TAPTR) str_tolower,
	(TAPTR) str_toupper,

	(TAPTR) str_findpat,
	(TAPTR) str_find,
};

/*****************************************************************************/
/* 
**	convert(src, temp, ols, nls) - convert/pad a single element, according
**	to native endianness. on big endian, elements are padded with zeros in
**	front of the element, on little endian with zeros behind
**
**		src  - ptr to source element
**		temp - 16 bytes, blank
**		ols  - old element size (0,1,2,3,4)
**		nls  - new element size (0,1,2,3,4)
*/

EXPORT TUINT8 *
convert_big(TAPTR mod, TUINT8 *src, TUINT8 *temp, TINT oldels, TINT newels)
{
	switch (oldels)
	{
		case 0:	switch (newels)
				{
					case 0:	return src;
					case 1:	temp[1] = src[0];
							break;
					case 2:	temp[3] = src[0];
							break;
					case 3:	temp[7] = src[0];
							break;
					case 4:	temp[15] = src[0];
							break;
				}
				break;

		case 1:	switch (newels)
				{
					case 0:	return src + 1;
					case 1: return src;
					case 2:	temp[2] = src[0];
							temp[3] = src[1];
							break;
					case 3:	temp[6] = src[0];
							temp[7] = src[1];
							break;
					case 4:	temp[14] = src[0];
							temp[15] = src[1];
							break;
				}
				break;

		case 2:	switch (newels)
				{
					case 0:	return src + 3;
					case 1:	return src + 2;
					case 2: return src;
					case 3:	temp[4] = src[0];
							temp[5] = src[1];
							temp[6] = src[2];
							temp[7] = src[3];
							break;
					case 4:	temp[12] = src[0];
							temp[13] = src[1];
							temp[14] = src[2];
							temp[15] = src[3];
							break;
				}
				break;

		case 3:	switch (newels)
				{
					case 0:	return src + 7;
					case 1:	return src + 6;
					case 2:	return src + 4;
					case 3:	return src;
					case 4:	temp[8] = src[0];
							temp[9] = src[1];
							temp[10] = src[2];
							temp[11] = src[3];
							temp[12] = src[4];
							temp[13] = src[5];
							temp[14] = src[6];
							temp[15] = src[7];
							break;
				}
				break;

		case 4:	switch (newels)
				{
					case 0:	return src + 15;
					case 1:	return src + 14;
					case 2:	return src + 12;
					case 3:	return src + 8;
					case 4:	return src;
				}
				break;
	}
	return temp;
}
		
/*****************************************************************************/

EXPORT TUINT8 *
convert_little(TAPTR mod, TUINT8 *src, TUINT8 *temp, TINT oldels, TINT newels)
{
	if (newels < oldels)
	{
		return src;
	}
	else
	{
		switch (oldels)
		{
			case 3: temp[7] = src[7];
					temp[6] = src[6];
					temp[5] = src[5];
					temp[4] = src[4];
			case 2: temp[3] = src[3];
					temp[2] = src[2];
			case 1: temp[1] = src[1];
			case 0:	temp[0] = src[0];
		}
		return temp;
	}
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: unistring_mod.c,v $
**	Revision 1.35  2005/09/13 02:42:42  tmueller
**	updated copyright reference
**	
**	Revision 1.34  2005/09/08 00:03:54  tmueller
**	API extended
**	
**	Revision 1.33  2004/08/01 11:31:52  tmueller
**	removed lots of history garbage
**	
**	Revision 1.32  2004/07/25 00:08:15  tmueller
**	added strfind with Lua-style pattern matching
**	
**	Revision 1.31  2004/07/20 06:55:51  tmueller
**	added private str_duplinear() function
**	
**	Revision 1.30  2004/07/18 20:50:24  tmueller
**	character info added
*/
