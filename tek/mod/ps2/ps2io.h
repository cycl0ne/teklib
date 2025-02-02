
#ifndef _TEK_MOD_PS2IO_H
#define _TEK_MOD_PS2IO_H

/*
**	$Id: ps2io.h,v 1.3 2005/09/18 11:27:22 tmueller Exp $
**	teklib/tek/mod/ps2/ps2io.h - PS2 I/O extensions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/io.h>
#include <tek/mod/ioext.h>

#define TIOCMD_LOADIRX			TIOCMD_EXTENDED

/*
**	structurally equivalent to a TIOPacket structure,
**	additionally defining arguments for the LOADIRX command
*/

struct TPS2IOPacket
{
	struct TIORequest io_Req;	/* I/O request header */
	TAPTR io_FLock;				/* File/Lock handle */
	TAPTR io_Examine;			/* Examination handle */
	TAPTR io_Buffer;			/* I/O buffer handle */
	TUINT io_BufSize;			/* Handler-recommended I/O buffer size */
	TUINT io_BufFlags;			/* Handler-recommended buffer flags */
	union						/* Command arguments */
	{
		struct { TSTRPTR Name; TINT Result; } LoadIRX;
	} io_Op;
};

/*****************************************************************************/
/*
**	Revision History
**	$Log: ps2io.h,v $
**	Revision 1.3  2005/09/18 11:27:22  tmueller
**	added authors
**
**	Revision 1.2  2005/07/30 14:39:17  tmueller
**	added #include io.h
**	
**	Revision 1.1  2005/04/22 14:11:40  fschulze
**	Added definitions and I/O-packet structure for IOCMD_LOADIRX
**	
*/

#endif /* _TEK_MOD_PS2IO_H */
