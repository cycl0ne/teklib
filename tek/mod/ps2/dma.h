
#ifndef _TEK_MOD_PS2_DMA_H
#define _TEK_MOD_PS2_DMA_H

/*
**	$Id: dma.h,v 1.1 2005/09/18 12:33:39 tmueller Exp $
**	teklib/tek/mod/ps2/dma.h - DMA specific macros and defines
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/ps2/memory.h>

/* channel IDs */
#define DMC_VIF0		0
#define DMC_VIF1		1
#define DMC_GIF			2
#define DMC_FROMIPU		3
#define DMC_TOIPU		4
#define DMC_SIF0		5
#define DMC_SIF1		6
#define DMC_SIF2		7
#define DMC_FROMSPR		8
#define DMC_TOSPR		9

/* dma tag ID */

#define DMATAG_REFE	0x00
#define DMATAG_CNT	0x01
#define DMATAG_NEXT	0x02
#define DMATAG_REF	0x03
#define DMATAG_REFS	0x04
#define DMATAG_CALL	0x05
#define DMATAG_RET	0x06
#define DMATAG_END	0x07


#define DMA_SET_CHCR(channel,dir,mod,asp,tte,tie,str,tag) \
	(*(volatile TUINT *)(channel) = \
	((TUINT)(dir)	<< 0)		| \
	((TUINT)(mod)	<< 2)		| \
	((TUINT)(asp)	<< 4)		| \
	((TUINT)(tte)	<< 6)		| \
	((TUINT)(tie)	<< 7)		| \
	((TUINT)(str)	<< 8)		| \
	((TUINT)(tag)	<< 16))

#define DMA_WAIT(channel) 				\
	while((*(volatile TUINT *)(channel)) & (1<<8))

#define DMA_SET_MADR(channel,addr,spr)	\
	(*(volatile TUINT *)(channel) = 	\
	((TUINT)(addr)<< 0)		| 			\
	((TUINT)(spr)	<< 31))
	
#define DMA_SET_TADR(channel,addr,spr) 	\
	(*(volatile TUINT *)(channel) = 	\
	((TUINT)(addr)<< 0)		| 			\
	((TUINT)(spr)	<< 31))

#define DMA_SET_SADR(channel,addr) 		\
	(*(volatile TUINT *)(channel) = ((TUINT)(addr) & 0x3fff))
	
#define DMA_SET_QWC(channel,size) 		\
	(*(volatile TUINT *)(channel) = (TUINT)(size))

/*****************************************************************************/
/*
**	Revision History
**	$Log: dma.h,v $
**	Revision 1.1  2005/09/18 12:33:39  tmueller
**	added
**	
**
*/

#endif /* _TEK_MOD_PS2_DMA_H */
