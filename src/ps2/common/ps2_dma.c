
/*
**	$Id: ps2_dma.c,v 1.6 2007/05/20 00:49:43 fschulze Exp $
**	teklib/mods/ps2/common/ps2_dma.c - DMA utils and DMA manager
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	and Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
*/

#include <tek/inline/ps2.h>
#include <tek/mod/ps2/gs.h>
#include <tek/mod/ps2/dma.h>
#include <tek/mod/ps2/memory.h>
#include <tek/mod/ps2/eekernel.h>

#include <tek/proto/hal.h>
#include <kernel.h>
#include "ps2_mod.h"

enum { MODE_UNDEF, MODE_MEMORY, MODE_SPR };

TINT AddDmacHandler2(TINT chan, TINT (*handler)(TINT ch, TAPTR udata, TAPTR addr), TINT next, TAPTR arg)
	{ SYSCALL(  18); }

LOCAL TVOID
dma_reset(TVOID)
{
	TINT i, temp;
	TINT base[] = { D0_CHCR, D1_CHCR, D2_CHCR, D3_CHCR, D4_CHCR, D8_CHCR, D9_CHCR };

	/* clear channel register */
	for (i = 0; i < 7; i++)
	{
		*(volatile TUINT *) base[i] = 0;
		*(volatile TUINT *)(base[i] + 0x10) = 0;
		*(volatile TUINT *)(base[i] + 0x30) = 0;
		*(volatile TUINT *)(base[i] + 0x40) = 0;
		*(volatile TUINT *)(base[i] + 0x50) = 0;
		*(volatile TUINT *)(base[i] + 0x80) = 0;
	}

	/* clear status bits */
	*(volatile TUINT *)D_STAT = 0xff1f;

	/* disable interrupts */
	temp = *(volatile TUINT *)D_STAT & 0xff1b0000;
	*(volatile TUINT *)D_STAT = temp;

	*(volatile TUINT *)D_CTRL = 0;
	*(volatile TUINT *)D_PCR  = 0;
	*(volatile TUINT *)D_SQWC = 0;
	*(volatile TUINT *)D_RBOR = 0;
	*(volatile TUINT *)D_RBSR = 0;

	/* enable all DMAs */
	*(volatile TUINT *)D_CTRL |= 0x1;
}


static TVOID
dma_initchain(struct DMAManager *dma)
{
	dma->dma_AllocLastTagptr = TNULL;
	dma->dma_AllocCallptr = dma->dma_AllocCallstack;
}

static TVOID
dma_init(TMOD_PS2 *TPS2Base, struct DMAManager *dma, TUINT size, TUINT chn)
{
	//struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;
	TTAGITEM tags[4];

	dma->dma_Ready = 1;
	dma->dma_Channel = chn;

	tags[0].tti_Tag = TPool_MemManager;
	tags[0].tti_Value = (TTAG) TNULL;	//priv->ps2_MM;
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
dma_alloc(TMOD_PS2 *TPS2Base, TUINT chn, TUINT qwc, DMACB cb, TAPTR udata)
{
	struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	TINT size = (qwc + 1) * 16;
	TQWDATA *mem;

	if (dma->dma_mode == MODE_SPR)
	{
		GS_SET_BGCOLOR(255,255,0);
		// *(int *) 0 = 0;
		d_commit(chn);
		d_commit(chn);
	}
	dma->dma_mode = MODE_MEMORY;

	THALLock(THALBase, TNULL);

	mem = TAllocPool(dma->dma_MemPool, size);

	if (mem == TNULL)
	{
		TINT x = 1000000;
		THALUnlock(THALBase, TNULL);
		TDBPRINTF(99, ("*** DMA memory pool exhausted\n"));
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
dma_allocspr(TMOD_PS2 *TPS2Base, TUINT chn, TUINT qwc, DMACB cb, TAPTR udata)
{
	struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	TINT size = (qwc + 1) * 16;
	TQWDATA *mem;

	if (dma->dma_mode == MODE_MEMORY)
	{
		GS_SET_BGCOLOR(255,255,0);
		// *(int *) 0 = 0;
		d_commit(chn);
		d_commit(chn);
	}
	dma->dma_mode = MODE_SPR;

	THALLock(THALBase, TNULL);

	mem = TAllocPool(dma->dma_MemPoolSPR, size);

	if (mem == TNULL)
	{
		TINT x = 1000000;
		THALUnlock(THALBase, TNULL);
		TDBPRINTF(99, ("*** DMA memory pool exhausted\n"));
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
dma_allocCall(TMOD_PS2 *TPS2Base, TUINT chn, TQWDATA* dmadata, DMACB cb, TAPTR udata)
{
	struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	TINT size = 2 * 16; /* call + next */
	TQWDATA *mem;

	THALLock(THALBase, TNULL);

	mem = TExecAllocPool(TExecBase, dma->dma_MemPool, size);

	if (mem == TNULL)
	{
		TINT x = 1000000;
		THALUnlock(THALBase, TNULL);
		TDBPRINTF(99, ("*** DMA memory pool exhausted\n"));
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

static TVOID
dma_dumpChain(TMOD_PS2 *TPS2Base, TQWDATA *dmachain)
{
	TQWDATA *next;
	TINT qwc = 0;

	next = dmachain;

	for (;;)
	{
		qwc = next[0].ui32[0] & 0xffff;
		printf("0x%p:\n", next);

		u_hexdump("", next, qwc+1);

		if ((next[0].ui32[0] & 0x70000000) != 0x70000000)
		{
			next = (TQWDATA *)next[0].ui32[1];
		}
		else
		{
			break;
		}
	}
}

EXPORT TVOID
dma_commit(TMOD_PS2 *TPS2Base, TUINT chn)
{
	struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	TUINT offs = dma->dma_Channel * 0x1000;
	TUINT adr;
	TUINT timeout = 0x1000000;

	while (!dma->dma_Ready && --timeout)	;

	if (timeout == 0)
	{
		printf("DMA timeout - I'll kill myself now :(\n");
		*(TUINT *) 0x0 = 0;
	}

	if (dma->dma_AllocLastTagptr == TNULL) return;

	dma->dma_AllocLastTagptr[0].ui32[0] |= 0x70000000;
	dma->dma_AllocLastTagptr[0].ui32[1] = 0;
	dma->dma_AllocLastTagptr[0].ui32[2] = 0;
	dma->dma_AllocLastTagptr[0].ui32[3] = 0;

	if (TPS2Base->ps2_DMADebug)
		dma_dumpChain(TPS2Base, dma->dma_AllocChain);

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
dma_irqfunc(TINT chn, TAPTR udata, TAPTR addr)
{
	TMOD_PS2 *TPS2Base = udata;
	struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	TUINT offs = chn * 0x1000;
	TUINT chcr = *(volatile TUINT *) (D0_CHCR+offs);

	/* someone wants to use the channel without us */
	if (dma->dma_Ready)	return 0;

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

	ExitHandler();
	return 0;
}

EXPORT TVOID
dma_initManager(TMOD_PS2 *TPS2Base, TUINT chn, TUINT size)
{
	struct TPS2ModPrivate *priv = TPS2Base->ps2_Private;
	struct DMAManager *dma = &priv->ps2_DMAManager[chn];
	switch (chn)
	{
		case 2: /* channel: GIF */

			DMA_WAIT((TUINT *)D2_CHCR);
			*(TUINT volatile *) D2_CHCR &= 0x0000ffff;
			*(TUINT volatile *) D_STAT = 4;
			dma_init(TPS2Base, dma, size, 2);

			AddDmacHandler2(2, dma_irqfunc, 0, TPS2Base);

			EnableDmac(2);
	}
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_dma.c,v $
**	Revision 1.6  2007/05/20 00:49:43  fschulze
**	dma_reset: major overhaul
**
**	Revision 1.5  2007/04/21 14:57:41  fschulze
**	added ExitHandler; cosmetic
**
**	Revision 1.4  2006/08/18 11:32:48  fschulze
**	+ added dma timeout for debugging
**	+ added a small workaround to share a managed dma channel with
**	  user processes
**
**	Revision 1.3  2006/03/26 14:29:53  fschulze
**	added function to dump DMA chains
**
**	Revision 1.2  2006/02/24 15:46:17  fschulze
**	renamed modbase to TPS2Base; d_ functions renamed to dma_;
**	now uses ps2 inline functions; adapted to new debug macros
**
**	Revision 1.1  2005/09/18 12:40:09  fschulze
**	added
**
**
*/
