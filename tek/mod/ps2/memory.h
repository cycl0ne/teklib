#ifndef _TEK_MOD_PS2_MEMORY_H
#define _TEK_MOD_PS2_MEMORY_H

/*
**	$Id: memory.h,v 1.4 2005/09/18 11:27:22 tmueller Exp $
**	teklib/tek/mod/ps2/memory.h - memory mapped registers
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/type.h>

#define ALIGN16			__attribute__((aligned(16)))
#define PACKED			__attribute__((packed))

/*****************************************************************************/
/* 
**	Segments
*/

#define	PHYSICAL(p)		((TAPTR) (((TUINT)(p)) & 0x1fffff))
#define CACHED(p)		((TAPTR) (((TUINT)(p)) & ~0x20000000))
#define UNCACHED(p)		((TAPTR) (((TUINT)(p)) | 0x20000000))
#define VU0MEM(qw)		((TAPTR) (0x11004000 + (qw) * 16))
#define VU0MICROMEM(qw)	((TAPTR) (0x11000000 + (qw) * 16))
#define VU1MEM(qw)		((TAPTR) (0x1100c000 + (qw) * 16))
#define VU1MICROMEM(qw)	((TAPTR) (0x11008000 + (qw) * 16))

/*****************************************************************************/
/* 
**	GS
*/

#define GS_PMODE		0x12000000	/* various PCRTC modes 	*/
#define GS_SMODE1		0x12000010	/* related to Sync */
#define GS_SMODE2		0x12000020	/* related to Sync */
#define GS_SRFSH		0x12000020	/* DRAM refresh */
#define GS_SYNCH1		0x12000030	/* related to Sync */
#define GS_SYNCH2		0x12000040	/* related to Sync */
#define GS_SYNCV		0x12000040	/* related to Sync/start */
#define GS_DISPFB1		0x12000070	/* related to display buffer of Rectangular Area 1 */
#define GS_DISPLAY1		0x12000080	/* Rectangular Area 1 display position etc. */
#define GS_DISPFB2		0x12000090	/* related to display buffer of Rectangular Area 2 */
#define GS_DISPLAY2		0x120000a0	/* Rectangular Area 2 display position etc. */
#define GS_EXTBUF		0x120000b0	/* Rectangular Area write buffer */
#define GS_EXTDATA		0x120000c0	/* Rectangular Area write data */
#define GS_EXTWRITE		0x120000d0	/* Rectangular Area write start */
#define GS_BGCOLOR		0x120000e0	/* background color */
#define GS_CSR			0x12001000	/* various GS status */
#define GS_IMR			0x12001010	/* Interrupt mask */
#define GS_BUSDIR		0x12001040	/* Host I/F switching */
#define GS_SIGLBLID		0x12001080	/* SIGNALID/LABELID */

/*****************************************************************************/
/* 
**	TIMER
*/

#define T0_COUNT		0x10000000
#define	T0_MODE			0x10000010
#define T0_COMP			0x10000020

/*****************************************************************************/
/* 
**	VIF
*/

#define VIF0_STAT		0x10003800
#define	VIF1_STAT		0x10003c00
#define VIF1_FBRST		0x10003c10
#define VIF1_ERROR		0x10003c20
#define VIF1_MARK		0x10003c30
#define VIF1_CYCLE		0x10003c40
#define VIF1_MODE		0x10003c50
#define VIF1_NUM		0x10003c60
#define VIF1_MASK		0x10003c70
#define VIF1_CODE 		0x10003c80
#define VIF1_ITOPS		0x10003c90
#define VIF1_BASE		0x10003ca0
#define VIF1_OFST		0x10003cb0
#define VIF1_TOPS		0x10003cc0
#define VIF1_ITOP		0x10003cd0
#define VIF1_TOP		0x10003ce0
#define VIF1_R0			0x10003d00
#define VIF1_R1			0x10003d10
#define VIF1_R2			0x10003d20
#define VIF1_R3			0x10003d30
#define VIF1_C0			0x10003d40
#define VIF1_C1			0x10003d50
#define VIF1_C2			0x10003d60
#define VIF1_C3			0x10003d70

/*****************************************************************************/
/* 
**	IRQ
*/

#define I_STAT			0x1000f000
#define I_MASK			0x1000f010

/*****************************************************************************/
/* 
**	GIF
*/

#define GIF_STAT		0x10003020
#define GIF_TAG0		0x10003040
#define GIF_TAG1		0x10003050
#define GIF_TAG2		0x10003060
#define GIF_TAG3		0x10003070
#define GIF_CNT			0x10003080


/*****************************************************************************/
/* 
**	DMA
*/

/* VIF0 */
#define D0_CHCR		0x10008000	/* ch0 channel control */
#define	D0_MADR		0x10008010	/* ch0 memory address */
#define D0_QWC		0x10008020	/* ch0 quadword count */
#define	D0_TADR		0x10008030	/* ch0 tag address */
#define	D0_ASR0		0x10008040	/* ch0 address stack 0 */
#define D0_ASR1		0x10008050	/* ch0 address stack 1 */

/* VIF1 */
#define D1_CHCR		0x10009000	/* ch1 channel control */
#define	D1_MADR		0x10009010	/* ch1 memory address */
#define D1_QWC		0x10009020	/* ch1 quadword count */
#define	D1_TADR		0x10009030	/* ch1 tag address */
#define	D1_ASR0		0x10009040	/* ch1 address stack 0 */
#define D1_ASR1		0x10009050	/* ch1 address stack 1 */

/* GIF */
#define D2_CHCR		0x1000a000	/* ch2 channel control */
#define	D2_MADR		0x1000a010	/* ch2 memory address */
#define D2_QWC		0x1000a020	/* ch2 quadword count */
#define	D2_TADR		0x1000a030	/* ch2 tag address */
#define	D2_ASR0		0x1000a040	/* ch2 address stack 0 */
#define D2_ASR1		0x1000a050	/* ch2 address stack 1 */

/* fromIPU */
#define D3_CHCR		0x1000b000	/* ch3 channel control */
#define	D3_MADR		0x1000b010	/* ch3 memory address */
#define D3_QWC		0x1000b020	/* ch3 quadword count */

/* toIPU */
#define D4_CHCR		0x1000b400	/* ch4 channel control */
#define	D4_MADR		0x1000b410	/* ch4 memory address */
#define D4_QWC		0x1000b420	/* ch4 quadword count */
#define	D4_TADR		0x1000b430	/* ch4 tag address */

/* SIF0 */
#define D5_CHCR		0x1000c000	/* ch5 channel control */
#define	D5_MADR		0x1000c010	/* ch5 memory address */
#define D5_QWC		0x1000c020	/* ch5 quadword count */

/* SIF1 */
#define D6_CHCR		0x1000c400	/* ch6 channel control */
#define	D6_MADR		0x1000c410	/* ch6 memory address */
#define D6_QWC		0x1000c420	/* ch6 quadword count */
#define	D6_TADR		0x1000c430	/* ch6 tag address */

/* SIF2 */
#define D7_CHCR		0x1000c800	/* ch7 channel control */
#define	D7_MADR		0x1000c810	/* ch7 memory address */
#define D7_QWC		0x1000c820	/* ch7 quadword count */

/* fromSPR */
#define D8_CHCR		0x1000d000	/* ch8 channel control */
#define	D8_MADR		0x1000d010	/* ch8 memory address */
#define D8_QWC		0x1000d020	/* ch8 quadword count */
#define	D8_SADR		0x1000d080	/* ch8 SPR address */

/* toSPR */
#define D9_CHCR		0x1000d400	/* ch9 channel control */
#define	D9_MADR		0x1000d410	/* ch9 memory address */
#define D9_QWC		0x1000d420	/* ch9 quadword count */
#define D9_TADR		0x1000d430	/* ch9 tag address */
#define	D9_SADR		0x1000d480	/* ch9 SPR address */

#define	D_CTRL		0x1000e000	/* DMAC control */
#define	D_STAT		0x1000e010	/* DMAC status */
#define	D_PCR		0x1000e020	/* DMAC priority ctrl */
#define	D_SQWC		0x1000e030	/* DMAC skip quadword */
#define	D_RBSR		0x1000e040	/* DMAC ring buf size */
#define	D_RBOR		0x1000e050	/* DMAC ring buf offset	*/
#define	D_STADR		0x1000e060	/* DMA stall address */

#define D_ENABLER	0x1000f520	/* acquisition of DMA suspend status (read)	*/
#define	D_ENABLEW	0x1000f590	/* DMA suspend control (write) */

/*****************************************************************************/
/*
**	Revision History
**	$Log: memory.h,v $
**	Revision 1.4  2005/09/18 11:27:22  tmueller
**	added authors
**	
**
**	Revision 1.3  2005/05/30 00:44:23  fschulze
**	added some registers, improved comments
**	
**	Revision 1.2  2005/05/08 14:21:13  fschulze
**	added vu macros
**	
**	Revision 1.1  2005/04/24 17:31:38  fschulze
**	added
**	
*/

#endif	/* _TEK_MOD_PS2_MEMORY_H */
