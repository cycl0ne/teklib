
/*
**  $Id: ps2_debug.c,v 1.3 2006/02/24 15:45:44 fschulze Exp $
**  teklib/mods/ps2/common/ps2_debug.c - Dump registers and memory
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/mod/ps2/memory.h>
#include "ps2_mod.h"

EXPORT TVOID 
util_dumpreg(TMOD_PS2 *mod, TUINT regdesc)
{
	TUINT regval = 0;

	if (regdesc != 0)
		regval = *(volatile TUINT *)regdesc;

	switch (regdesc)
	{
		case 0:
			printf("--------------------------------------------------------\n");
			printf(" MISC\n");
			printf("--------------------------------------------------------\n");
			printf(" * VIF1_CODE: ................................ 0x%x\n",
						*(volatile TUINT *)VIF1_CODE);
			printf(" * VIF1_ITOPS: ............................... 0x%x\n",
						*(volatile TUINT *)VIF1_ITOPS);
			printf(" * VIF1_BASE: ................................ 0x%x\n",
						*(volatile TUINT *)VIF1_BASE);
			printf(" * VIF1_OFST: ................................ 0x%x\n",
						*(volatile TUINT *)VIF1_OFST);
			printf(" * VIF1_TOPS: ................................ 0x%x\n", 
						*(volatile TUINT *)VIF1_TOPS);
			printf(" * VIF1_ITOP: ................................ 0x%x\n", 
						*(volatile TUINT *)VIF1_ITOP);
			printf(" * VIF1_TOP: ................................. 0x%x\n",
						*(volatile TUINT *)VIF1_TOP);
			printf(" * GIFtag0: .................................. 0x%x\n", 
						*(volatile TUINT *)GIF_TAG0);
			printf(" * GIFtag1: .................................. 0x%x\n", 
						*(volatile TUINT *)GIF_TAG1);
			printf(" * GIFtag2: .................................. 0x%x\n", 
						*(volatile TUINT *)GIF_TAG2);
			printf(" * GIFtag3: .................................. 0x%x\n", 
						*(volatile TUINT *)GIF_TAG3);
			printf(" * GIFcnt: ................................... 0x%x\n", 
						*(volatile TUINT *)GIF_CNT);
			break;
			
		case D_STAT:
			printf("--------------------------------------------------------\n");
			printf(" DMA Interrupt Status\n");
			printf("--------------------------------------------------------\n");
			printf(" * Transfer processing ended for channel: .... %s%s%s%s%s%s%s%s%s%s\n",
						regval & 0x001 ? "0 " : "",
						regval & 0x002 ? "1 " : "",
						regval & 0x004 ? "2 " : "",
						regval & 0x008 ? "3 " : "",
						regval & 0x010 ? "4 " : "",
						regval & 0x020 ? "5 " : "",
						regval & 0x040 ? "6 " : "",
						regval & 0x080 ? "7 " : "",
						regval & 0x100 ? "8 " : "",
						regval & 0x200 ? "9"  : "");
			printf(" * DMA Stall condition is met: ............... %s\n",
						regval & 0x2000 ? "yes" : "no");
			printf(" * MFIFO is empty: ........................... %s\n", 
						regval & 0x4000 ? "yes" : "no");
			printf(" * Buserror occured: ......................... %s\n", 
						regval & 0x8000 ? "yes" : "no");
			printf(" * Channel interrupt mask enabled for channel: %s%s%s%s%s%s%s%s%s%s\n",
						regval & 0x0010000 ? "0 " : "",
						regval & 0x0020000 ? "1 " : "",
						regval & 0x0040000 ? "2 " : "",
						regval & 0x0080000 ? "3 " : "",
						regval & 0x0100000 ? "4 " : "",
						regval & 0x0200000 ? "5 " : "",
						regval & 0x0400000 ? "6 " : "",
						regval & 0x0800000 ? "7 " : "",
						regval & 0x1000000 ? "8 " : "",
						regval & 0x2000000 ? "9 " : "");
			printf(" * DMA Stall Interrupt Mask is enabled: ...... %s\n",
						regval & 0x20000000 ? "yes" : "no");
			printf(" * MFIFO empty interrupt mask enabled: ....... %s\n",
						regval & 0x40000000 ? "yes" : "no");
			break;
			
		case I_STAT:
			printf("--------------------------------------------------------\n");
			printf(" Interrupt Status\n");
			printf("--------------------------------------------------------\n");
			printf(" * GS    interrupt request exists: ........... %s\n",
						regval & 0x0001 ? "yes" : "no");
			printf(" * SBUS  interrupt request exists: ........... %s\n",
						regval & 0x0002 ? "yes" : "no");
			printf(" * VBON  interrupt request exists: ........... %s\n",
						regval & 0x0004 ? "yes" : "no");
			printf(" * VBOF  interrupt request exists: ........... %s\n",
						regval & 0x0008 ? "yes" : "no");
			printf(" * VIF0  interrupt request exists: ........... %s\n",
						regval & 0x0010 ? "yes" : "no");
			printf(" * VIF1  interrupt request exists: ........... %s\n",
						regval & 0x0020 ? "yes" : "no");
			printf(" * VU0   interrupt request exists: ........... %s\n",
						regval & 0x0040 ? "yes" : "no");
			printf(" * VU1   interrupt request exists: ........... %s\n",
						regval & 0x0080 ? "yes" : "no");
			printf(" * IPU   interrupt request exists: ........... %s\n",
						regval & 0x0100 ? "yes" : "no");
			printf(" * TIM0  interrupt request exists: ........... %s\n",
						regval & 0x0200 ? "yes" : "no");
			printf(" * TIM1  interrupt request exists: ........... %s\n",
						regval & 0x0400 ? "yes" : "no");
			printf(" * TIM2  interrupt request exists: ........... %s\n",
						regval & 0x0800 ? "yes" : "no");
			printf(" * TIM3  interrupt request exists: ........... %s\n",
						regval & 0x1000 ? "yes" : "no");
			printf(" * SFIFO interrupt request exists: ........... %s\n",
						regval & 0x2000 ? "yes" : "no");
			printf(" * VU0WD interrupt request exists: ........... %s\n",
						regval & 0x4000 ? "yes" : "no");
			break;
		
		case GS_CSR:
			printf("--------------------------------------------------------\n");
			printf(" GS System Status\n");
			printf("--------------------------------------------------------\n");
			printf(" * SIGNAL event has been generated: .......... %s\n",
						regval & 0x001 ? "yes" : "no");
			printf(" * FINISH event has been generated: .......... %s\n",
						regval & 0x002 ? "yes" : "no");
			printf(" * HSYNC interrupt has been generated: ....... %s\n",
						regval & 0x004 ? "yes" : "no");
			printf(" * VSYNC interrupt has been generated: ....... %s\n",
						regval & 0x008 ? "yes" : "no");
			printf(" * Rectangular area write interrupt has been\n"); 
			printf("   generated: ................................ %s\n",
						regval & 0x010 ? "yes" : "no");
			printf(" * Drawing Suspend and FIFO clear is flushed:  %s\n",
						regval & 0x100 ? "yes" : "no");
			printf(" * GS System Reset: .......................... %s\n",
						regval & 0x200 ? "yes" : "no");
			printf(" * Output value of NFIELD output: ............ %d\n",
						regval & 0x1000);
			printf(" * Field displayed currently: ................ %s\n",
						regval & 0x2000 ? "ODD" : "EVEN");
			printf(" * Host interface FIFO status: ............... %s%s%s\n",
						regval & 0xc000 ? "" : "Neither empty nor almost full",
						regval & 0x4000 ? "Empty" : "",
						regval & 0x8000 ? "Almost full" : "");
			printf(" * Revision Number of the GS: ................ 0x%x\n",
						(regval & 0x00ff0000)>>16);
			printf(" * ID of the GS: ............................. 0x%x\n",
						(regval & 0xff000000)>>24);
			break;
			
		case GIF_STAT:
			printf("--------------------------------------------------------\n");
			printf(" GIF Status\n");
			printf("--------------------------------------------------------\n");
			printf(" * PATH3 mask status disabled: ............... %s\n",
						regval & 0x001 ? "yes" : "no");
			printf(" * PATH3 VIF mask status disabled: ........... %s\n",
						regval & 0x002 ? "yes" : "no");
			printf(" * PATH3 IMT transfer mode: .................. %s\n",
						regval & 0x004 ? "Intermittent" : "Continuous");
			printf(" * Temporary transfer stop by PSE flag of\n");
			printf("   GIF_MODE register: ........................ %s\n",
						regval & 0x008 ? "yes" : "no");
			printf(" * Transfer via PATH3 interrupted: ........... %s\n",
						regval & 0x020 ? "yes" : "no");
			printf(" * Request to wait for processing in PATH3: .. %s\n",
						regval & 0x040 ? "yes" : "no");
			printf(" * Request to wait for processing in PATH2: .. %s\n",
						regval & 0x080 ? "yes" : "no");
			printf(" * Request to wait for processing in PATH1: .. %s\n",
						regval & 0x100 ? "yes" : "no");
			printf(" * Output path outputting data: .............. %s\n",
						regval & 0x200 ? "yes" : "no, idle state");
			printf(" * Data path transferring data: .............. %s%s%s%s\n",
						regval & 0xc00 ? "" : "idle state",
						((regval & 0x400) && !(regval & 0x800)) ? "via PATH1" : "",
						(!(regval & 0x400) && (regval & 0x800)) ? "via PATH2" : "",
						((regval & 0x400) && (regval & 0x800))  ? "via PATH3" : "");
			printf(" * Transfer direction: ....................... %s\n",
						regval & 0x1000 ? "GS -> EE" : "EE -> GS");
			printf(" * Effective data count in GIF-FIFO (qwords):  %d\n",
						(regval & 0x1f000000)>>24);
			break;

		case VIF0_STAT:
			printf("--------------------------------------------------------\n");
			printf(" VIF0 Status\n");
			printf("--------------------------------------------------------\n");
			printf(" * VIF0 status: .............................. %s%s%s%s\n",
						regval & 0x3 ? "" : "Idle",
						((regval & 0x1) && !(regval & 0x2)) ? "Waits for data following VIFcode" : "",
						(!(regval & 0x1) && (regval & 0x2)) ? "Decoding the VIFcode" : "",
						((regval & 0x1)  && (regval & 0x2)) ? "Decompressing data following VIFcode" : "");
			printf(" * VIF0 E-bit wait: .......................... %s\n",
						regval & 0x4 ? "Wait" : "Not-wait");
			printf(" * VIF0 Mark detect: ......................... %s\n",
						regval & 0x40 ? "Detect" : "Not-detect");
			printf(" * Stop by STOP: ............................. %s\n",
						regval & 0x100 ? "Stall" : "Not-stall");
			printf(" * Stop by ForceBreak: ....................... %s\n",
						regval & 0x200 ? "Stall" : "Not-stall");
			printf(" * Stop by the i-bit: ........................ %s\n",
						regval & 0x400 ? "Stall" : "Not-stall");
			printf(" * Interrupt bit detected flag: .............. %s\n",
						regval & 0x800 ? "Detect" : "Not-detect");
			printf(" * DMAtag mismatch error detected: ........... %s\n",
						regval & 0x1000 ? "yes" : "no");
			printf(" * VIFcode error detected: ................... %s\n",
						regval & 0x2000 ? "yes" : "no");
			printf(" * Effective data count in VIF0-FIFO (qwords): %d\n",
						(regval & 0xf000000)>>24);
			break;
		case VIF1_STAT:
			printf("--------------------------------------------------------\n");
			printf(" VIF1 Status\n");
			printf("--------------------------------------------------------\n");
			printf(" * VIF1 status: .............................. %s%s%s%s\n",
						regval & 0x3 ? "" : "Idle",
						((regval & 0x1) && !(regval & 0x2)) ? "Waits for data following VIFcode" : "",
						(!(regval & 0x1) && (regval & 0x2)) ? "Decoding the VIFcode" : "",
						((regval & 0x1)  && (regval & 0x2)) ? "Decompressing data following VIFcode" : "");
			printf(" * VIF1 E-bit wait: .......................... %s\n",
						regval & 0x4 ? "Wait" : "Not-wait");
			printf(" * Status waiting for end of GIF transfer: ... %s\n",
						regval & 0x8 ? "Wait" : "Not-wait");
			printf(" * VIF1 Mark detect: ......................... %s\n",
						regval & 0x40 ? "Detect" : "Not-detect");
			printf(" * Double buffer flag: ....................... %s\n",
						regval & 0x80 ? "TOPS=BASE+OFFSET" : "TOPS=BASE");
			printf(" * Stop by STOP: ............................. %s\n",
						regval & 0x100 ? "Stall" : "Not-stall");
			printf(" * Stop by ForceBreak: ....................... %s\n",
						regval & 0x200 ? "Stall" : "Not-stall");
			printf(" * VIF1 interrupt stall: ..................... %s\n",
						regval & 0x400 ? "Stall" : "Not-stall");
			printf(" * Interrupt by the i-bit: ................... %s\n",
						regval & 0x800 ? "Detect" : "Not-detect");
			printf(" * DMAtag mismatch error detected: ........... %s\n",
						regval & 0x1000 ? "yes" : "no");
			printf(" * VIFcode error detected: ................... %s\n",
						regval & 0x2000 ? "yes" : "no");
			printf(" * VIF1-FIFO transfer direction: ............. %s\n",
						regval & 0x800000 ? "VIF1 -> Main memory/SPRAM" : "Main memory/SPRAM -> VIF1");
			printf(" * Effective data count in VIF1-FIFO (qwords): %d\n",
						(regval & 0x1f000000)>>24);
			break;

		default:
			printf("Unknown register descriptor!\n");
			break;
	}
	
	printf("\n");
}

EXPORT TVOID 
util_hexdump(TMOD_PS2 *mod, TSTRPTR s, TQWDATA *data, TINT qwc)
{
	TINT i;
	
	for(i = 0; i < qwc; i++)
		printf("%s[%03d]: 0x%08x 0x%08x 0x%08x 0x%08x\n", s, i, 
					data[i].ui32[0], data[i].ui32[1], 
					data[i].ui32[2], data[i].ui32[3]);
}


/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2_debug.c,v $
**	Revision 1.3  2006/02/24 15:45:44  fschulze
**	u_ functions renamed to util_
**
**	Revision 1.2  2005/11/20 18:00:55  tmueller
**	stringptr type corrected
**	
**	Revision 1.1  2005/09/18 12:40:09  fschulze
**	added
**	
**	
*/
