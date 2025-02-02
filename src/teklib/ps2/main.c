
/*
**	$Id: main.c,v 1.5 2006/02/24 15:43:50 fschulze Exp $
**	boot/ps2/main.c - PS2 application entrypoint
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	and Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdlib.h>
#include <sifrpc.h>
#include <tek/mod/ps2/gs.h>

#include <tek/lib/init.h>


int __errno;	/* FIXME: symbol needed by newlib -> should point to errno */

extern void *EndOfHeap(void);
extern const struct TInitModule TEKModules[];

TINT _tekmain(TINT argc, TSTRPTR *argv, TUINT8 *heap)
{
	TAPTR apptask;
	TINT retval = EXIT_FAILURE;
	TTAGITEM tags[7];

	TUINT8 *heapend = EndOfHeap();
	TUINT heapsize = heapend - heap;

	tags[0].tti_Tag = TExecBase_ArgC;
	tags[0].tti_Value = (TTAG) argc;
	tags[1].tti_Tag = TExecBase_ArgV;
	tags[1].tti_Value = (TTAG) argv;
	tags[2].tti_Tag = TExecBase_RetValP;
	tags[2].tti_Value = (TTAG) &retval;
	tags[3].tti_Tag = TExecBase_ModInit;
	tags[3].tti_Value = (TTAG) TEKModules;
	tags[4].tti_Tag = TExecBase_MemBase;
	tags[4].tti_Value = (TTAG) heap;
	tags[5].tti_Tag = TExecBase_MemSize;
	tags[5].tti_Value = (TTAG) heapsize;
	tags[6].tti_Tag = TTAG_DONE;

	SifInitRpc(0);

	TDBPRINTF(2, ("heap      : %08x\n", (TUINT) heap));
	TDBPRINTF(2, ("heap size : %08x\n", (TUINT) heapsize));

	apptask = TEKCreate(tags);
	if (apptask)
	{
		retval = EXIT_SUCCESS;
		TEKMain(apptask);
		TDestroy(apptask);
	}

	*(volatile TUINT64 *)(GS_BGCOLOR)= GS_SETREG_BGCOLOR(0,0,155);

	return retval;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: main.c,v $
**	Revision 1.5  2006/02/24 15:43:50  fschulze
**	adapted to new debug macros; GS_BGCOLOR register is now set
**	directly
**
**	Revision 1.4  2005/11/11 21:15:22  fschulze
**	minor cleanup
**
**	Revision 1.3  2005/09/18 11:33:57  tmueller
**	create memory manager, alignsp moved to crt0.S
**
**	Revision 1.2  2005/04/01 18:33:21  tmueller
**	All memory is now obtained from a single allocator in the boot backend
**
**	Revision 1.1  2005/03/13 20:01:39  fschulze
**	added
*/
