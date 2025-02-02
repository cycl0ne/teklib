
#ifndef _TEK_MOD_AMIGA_HAL_H
#define _TEK_MOD_AMIGA_HAL_H

/*
**	$Id: hal.h,v 1.2 2003/12/20 13:58:26 tmueller Exp $
**	tek/mod/amiga/hal.h - Amiga/MorphOS HAL module internal
*/

#include <tek/exec.h>
#include <exec/exec.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <libraries/locale.h>

struct HALAmiSpecific
{
	TTAGITEM has_Tags[4];
	TSTRPTR has_SysDir;
	TSTRPTR has_ModDir;
	TSTRPTR has_ProgDir;
	struct DosLibrary *has_DOSBase;
	struct Library *has_LocaleBase;
	struct Locale *has_Locale;
	TFLOAT has_TZDays;					/* Days west of GMT */
	TINT has_TZSec;						/* Seconds west of GMT */
	struct SignalSemaphore has_DevLock;	/* Locking for device */
	TUINT has_RefCount;					/* Open reference counter */
	TAPTR has_ExecBase;					/* Inserted at device open */
	TAPTR has_DevTask;					/* Created at device open */
	TLIST has_ReqList;					/* List of pending requests */
};

struct HALAmiThread
{
	struct Message hat_Message;
	struct SignalSemaphore hat_SigLock;
	struct timeval hat_TimeVal;
	struct MsgPort *hat_TimePort;
	struct timerequest *hat_TimeReq;
	struct Task *hat_AmiTask;
	struct MsgPort *hat_InitExitPort;
	TAPTR hat_HALBase;
	TAPTR hat_Data;
	TTASKFUNC hat_Function;
	TUINT hat_TaskState;
	TUINT hat_SigState;
	BYTE hat_SigEvent;
};

struct HALAmiMod
{
	struct Library *ham_Lib;
	TUINT16 ham_Version;
};

/*****************************************************************************/
/*
**	Revision History
**	$Log: hal.h,v $
**	Revision 1.2  2003/12/20 13:58:26  tmueller
**	Fixed waiting for an absolute date
**	
**	Revision 1.1.1.1  2003/12/11 07:17:59  tmueller
**	Krypton import
**	
**	Revision 1.2  2003/10/27 22:50:11  tmueller
**	Major source cleanup (style, formatting), renamed HAL structures
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
