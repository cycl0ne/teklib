
/*
**	$Id: main.c,v 1.3 2005/09/18 11:33:57 tmueller Exp $
**	boot/ps2/main.c - PS2 application entrypoint
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	and Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdlib.h>
#include <sifrpc.h>
#include "init.h"

#define gs_bgcolor		0x120000e0	/* Set CRTC background color */
#define GS_SET_BGCOLOR(r,g,b) 			\
	*(volatile TUINT64 *)(gs_bgcolor)= 	\
	((TUINT64)(r)	<< 0)	| 			\
	((TUINT64)(g)	<< 8)	| 			\
	((TUINT64)(b)	<< 16)

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

	printf("heap      : %08x\n", (TUINT) heap);
	printf("heap size : %08x\n", (TUINT) heapsize);

	apptask = TEKCreate(tags);
	if (apptask)
	{
		retval = EXIT_SUCCESS;
		TEKMain(apptask);
		TDestroy(apptask);
	}
	
	GS_SET_BGCOLOR(0,0,155);
		
	return retval;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: main.c,v $
**	Revision 1.3  2005/09/18 11:33:57  tmueller
**	create memory manager, alignsp moved to crt0.S
**	
**	Revision 1.2  2005/04/01 18:33:21  tmueller
**	All memory is now obtained from a single allocator in the boot backend
**	
**	Revision 1.1  2005/03/13 20:01:39  fschulze
**	added
*/
