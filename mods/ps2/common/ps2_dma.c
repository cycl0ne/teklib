
/*
**	$Id: ps2_dma.c,v 1.1 2005/09/18 12:40:09 fschulze Exp $
**	teklib/mods/ps2/common/ps2_dma.c - DMA utils and DMA manager
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	and Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	DMA reset function based on code by Gustavo Scotti <gustavo at scotti.com>
*/

#include <tek/mod/ps2/gs.h>
#include <tek/mod/ps2/dma.h>
#include <tek/mod/ps2/memory.h>
#include <tek/proto/hal.h>
#include <kernel.h>		/* FIXME */
#include "ps2_mod.h"

enum { MODE_UNDEF, MODE_MEMORY, MODE_SPR };

static TVOID 
int_addDmacHandler2(TINT chan, TINT (*handler)(TINT ch, TAPTR udata, TAPTR addr), TINT next, TAPTR arg)
{
	__asm__ __volatile__("
		.set push
		.set noreorder
		li	$3,18
		syscall
		.set pop
	");
}

LOCAL TVOID 
d_reset(TVOID)
{
  TUINT dma_addr;
  TUINT temp;
 
  asm volatile ("        .set push               \n"
                "        .set noreorder          \n"
                "        lui   %0, 0x1001        \n"
                "        sw    $0, -0x5f80(%0)   \n"
                "        sw    $0, -0x5000(%0)   \n"
                "        sw    $0, -0x5fd0(%0)   \n"
                "        sw    $0, -0x5ff0(%0)   \n"
                "        sw    $0, -0x5fb0(%0)   \n"
                "        sw    $0, -0x5fc0(%0)   \n"
                "        lui   %1, 0             \n"
                "        ori   %1, %1, 0xff1f    \n"
                "        sw    %1, -0x1ff0(%0)   \n"
                "        lw    %1, -0x1ff0(%0)   \n"
                "        andi  %1, %1, 0xff1f    \n"
                "        sw    %1, -0x1ff0(%0)   \n"
                "        sw    $0, -0x2000(%0)   \n"
                "        sw    $0, -0x1fe0(%0)   \n"
                "        sw    $0, -0x1fd0(%0)   \n"
                "        sw    $0, -0x1fb0(%0)   \n"
                "        sw    $0, -0x1fc0(%0)   \n"
                "        lw    %1, -0x2000(%0)   \n"
                "        ori   %1, %1, 1         \n"
                "        sw    %1, -0x2000(%0)   \n"
                "        .set pop                \n"
                : "=&r" (dma_addr), "=&r" (temp) );

	*(volatile TUINT *)D_CTRL |= 0x2;
}

static TVOID 
dma_initchain(struct DMAManager *dma)
{
	dma->dma_AllocLastTagptr = TNULL;
	dma->dma_AllocCallptr = dma->dma_AllocCallstack;
}

static TVOID 
d_init(TMOD_PS2 *mod, struct DMAManager *dma, TUINT size, TUINT chn)
{
	//struct TPS2ModPrivate *priv = mod->ps2_Private;
	TTAGITEM tags[4];

	dma->dma_Ready = 1;
	dma->dma_Channel = chn;
	
	tags[0].tti_Tag = TPool_MMU;
	tags[0].tti_Value = (TTAG) TNULL;	//priv->ps2_MMU;
	tags[1].tti_Tag = TPool_StaticSize;
	tags[1].tti_Value = size;
	tags[2].tti_Tag = TMem_LowFrag;
	tags[2].tti_Value = TTRUE;
	tags[3].tti_Tag = TTAG_DONE;

	dma->dma_MemPool = TCreatePool(tags);

	tags[0].tti_Tag = TPool_Static;
	tags[0].tti_Value = (TTAG) 0x70000000;
	tags[1].tti_Tag = TPool_StaticSize;
	tags[1].tti_Value = 0x4000;
	tags[2].tti_Tag = TMem_LowFrag;
	tags[2].tti_Value = TTRUE;
	tags[3].tti_Tag = TTAG_DONE;
		
	dma->dma_MemPoolSPR = TCreatePool(tags);
	
	
	dma->dma_mode = MODE_UNDEF;
	
	dma_initchain(dma);
}

EXPORT TQWDATA *
d_alloc(TMOD_PS2 *mod, TUINT chn, TUINT qwc, DMACB cb, TAPTR udata)
{
	struct TPS2ModPrivate *priv = mod->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	TINT size = (qwc + 1) * 16;
	TQWDATA *mem;

	if (dma->dma_mode == MODE_SPR)
	{
		GS_SET_BGCOLOR(255,255,0);
		// *(int *) 0 = 0;
		d_commit(mod, chn);
		d_commit(mod, chn);
	}
	dma->dma_mode = MODE_MEMORY;
			
	THALLock(THALBase, TNULL);

	mem = TAllocPool(dma->dma_MemPool, size);
	
	if (mem == TNULL)
	{
		TINT x = 1000000;
		THALUnlock(THALBase, TNULL);
		tdbprintf(99,"*** DMA memory pool exhausted\n");
		while (x--) GS_SET_BGCOLOR((x>>16)&255,(x>>8)&255,x&255);
		*(TINT *) 0 = 0;
	}	
	
	mem[0].ui32[0] = cb ? 0xa0000000+qwc : 0x20000000+qwc;
	
	if (dma->dma_AllocLastTagptr)
	{
		dma->dma_AllocLastTagptr[0].ui32[1] = (TUINT) mem;
		dma->dma_AllocLastTagptr[0].ui32[2] = 0;
		dma->dma_AllocLastTagptr[0].ui32[3] = 0;
	}
	else
	{
		dma->dma_AllocChain = mem;
	}

	dma->dma_AllocLastTagptr = mem;

	if (cb)
	{
		dma->dma_AllocCallptr->dcb_Func = cb;
		dma->dma_AllocCallptr->dcb_UserData = udata;
		dma->dma_AllocCallptr++;
	}
	
	THALUnlock(THALBase, TNULL);
	
	return mem + 1;
}

EXPORT TQWDATA *
d_allocspr(TMOD_PS2 *mod, TUINT chn, TUINT qwc, DMACB cb, TAPTR udata)
{
	struct TPS2ModPrivate *priv = mod->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	TINT size = (qwc + 1) * 16;
	TQWDATA *mem;
	
	if (dma->dma_mode == MODE_MEMORY)
	{
		GS_SET_BGCOLOR(255,255,0);
		// *(int *) 0 = 0;
		d_commit(mod, chn);
		d_commit(mod, chn);
	}
	dma->dma_mode = MODE_SPR;

	THALLock(THALBase, TNULL);

	mem = TAllocPool(dma->dma_MemPoolSPR, size);
	
	if (mem == TNULL)
	{
		TINT x = 1000000;
		THALUnlock(THALBase, TNULL);
		tdbprintf(99,"*** DMA memory pool exhausted\n");
		while (x--) GS_SET_BGCOLOR((x>>16)&255,(x>>8)&255,x&255);
		*(TINT *) 0 = 0;
	}	
	
	mem[0].ui32[0] = cb ? 0xa0000000+qwc : 0x20000000+qwc;
	
	if (dma->dma_AllocLastTagptr)
	{
		dma->dma_AllocLastTagptr[0].ui32[1] = (TUINT) mem;
		dma->dma_AllocLastTagptr[0].ui32[2] = 0;
		dma->dma_AllocLastTagptr[0].ui32[3] = 0;
	}
	else
	{
		dma->dma_AllocChain = mem;
	}

	dma->dma_AllocLastTagptr = mem;

	if (cb)
	{
		dma->dma_AllocCallptr->dcb_Func = cb;
		dma->dma_AllocCallptr->dcb_UserData = udata;
		dma->dma_AllocCallptr++;
	}
	
	THALUnlock(THALBase, TNULL);
	
	return mem + 1;
}

EXPORT TQWDATA *
d_allocCall(TMOD_PS2 *mod, TUINT chn, TQWDATA* dmadata, DMACB cb, TAPTR udata)
{
	struct TPS2ModPrivate *priv = mod->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	TINT size = 2 * 16; /* call + next */
	TQWDATA *mem;
	
	THALLock(THALBase, TNULL);

	mem = TExecAllocPool(TExecBase, dma->dma_MemPool, size);
	
	if (mem == TNULL)
	{
		TINT x = 1000000;
		THALUnlock(THALBase, TNULL);
		tdbprintf(99,"*** DMA memory pool exhausted\n");
		while (x--) GS_SET_BGCOLOR((x>>16)&255,(x>>8)&255,x&255);
		*(TINT *) 0 = 0;
	}	
	
	mem[0].ui32[0] = 0x50000000; 					/* call */
	mem[0].ui32[1] = (TUINT) dmadata;
	mem[1].ui32[0] = cb ? 0xa0000000 : 0x20000000; 	/* next */
	
	if (dma->dma_AllocLastTagptr)
	{
		dma->dma_AllocLastTagptr[0].ui32[1] = (TUINT) mem;
		dma->dma_AllocLastTagptr[0].ui32[2] = 0;
		dma->dma_AllocLastTagptr[0].ui32[3] = 0;
	}
	else
	{
		dma->dma_AllocChain = mem;
	}

	dma->dma_AllocLastTagptr = mem+1;

	if (cb)
	{
		dma->dma_AllocCallptr->dcb_Func = cb;
		dma->dma_AllocCallptr->dcb_UserData = udata;
		dma->dma_AllocCallptr++;
	}
	
	THALUnlock(THALBase, TNULL);
	
	return dmadata;
}

EXPORT TVOID
d_commit(TMOD_PS2 *mod, TUINT chn)
{
	struct TPS2ModPrivate *priv = mod->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	TUINT offs = dma->dma_Channel * 0x1000;
	TUINT adr;

	while (!dma->dma_Ready);

	if (dma->dma_AllocLastTagptr == TNULL) return;

	dma->dma_AllocLastTagptr[0].ui32[0] |= 0x70000000;
	dma->dma_AllocLastTagptr[0].ui32[1] = 0;
	dma->dma_AllocLastTagptr[0].ui32[2] = 0;
	dma->dma_AllocLastTagptr[0].ui32[3] = 0;
	
	TCopyMem(dma->dma_AllocCallstack, dma->dma_BusyCallstack, 
		(dma->dma_AllocCallptr - dma->dma_AllocCallstack) * sizeof(struct DMACallBack));

	dma->dma_BusyCallptr = dma->dma_BusyCallstack;

	dma->dma_BusyChain = dma->dma_AllocChain;

	FlushCache(0);		/* FIXME */

	dma->dma_Ready = 0;
	dma_initchain(dma);
	
	adr = (TUINT) PHYSICAL(dma->dma_BusyChain);
	if (dma->dma_mode == MODE_SPR)
		adr |= 0x80000000;

	DMA_SET_QWC((TUINT *)(D0_QWC+offs), 0);
	DMA_SET_MADR((TUINT *)(D0_MADR+offs), adr, 0);
	DMA_SET_TADR((TUINT *)(D0_TADR+offs), adr, 0);
	DMA_SET_CHCR((TUINT *)(D0_CHCR+offs), 1, 1, 0, 0, 1, 1, 0);
}

static TINT 
d_irqfunc(TINT chn, TAPTR udata, TAPTR addr)
{
	TMOD_PS2 *mod = udata;
	struct TPS2ModPrivate *priv = mod->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	TUINT offs = chn * 0x1000;
	TUINT chcr = *(volatile TUINT *) (D0_CHCR+offs);

	if (chcr & 0x80000000)
	{
		/* caused by I-bit */
		(*dma->dma_BusyCallptr->dcb_Func)(dma->dma_BusyCallptr->dcb_UserData);
		dma->dma_BusyCallptr++;
	}

	if ((chcr & 0x70000000) == 0x70000000)
	{
		TAPTR pool = dma->dma_mode == MODE_SPR ? 
			dma->dma_MemPoolSPR : dma->dma_MemPool;
		TQWDATA *p = dma->dma_BusyChain;
		
		while (p)
		{
			TQWDATA *next = (TQWDATA *) p[0].ui32[1];
			TINT size = ((p[0].ui32[0] & 0xffff) + 1) * 16;
			TFreePool(pool, p, size);
			p = next;
		}

		dma->dma_Ready = 1;
		dma->dma_mode = MODE_UNDEF;
	}

	return 0;
}

EXPORT TVOID 
d_initManager(TMOD_PS2 *mod, TUINT chn, TUINT size)
{
	struct TPS2ModPrivate *priv = mod->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	switch (chn)
	{
		case 2: /* channel: GIF */
			DMA_WAIT((TUINT *)D2_CHCR);
			*(TUINT volatile *) D2_CHCR &= 0x0000ffff;
			*(TUINT volatile *) D_STAT = 4;
			d_init(mod, dma, size, 2);		
			
			int_addDmacHandler2(2, d_irqfunc, 0, mod);

			EnableDmac(2);		/* FIXME */
	}
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_dma.c,v $
**	Revision 1.1  2005/09/18 12:40:09  fschulze
**	added
**	
**	
*/
