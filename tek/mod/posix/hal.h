
#ifndef _TEK_MOD_POSIX_HAL_H
#define _TEK_MOD_POSIX_HAL_H

/*
**	$Id: hal.h,v 1.4 2005/09/13 02:45:09 tmueller Exp $
**	teklib/tek/mod/hal/posix/hal.h - HAL internal structures on POSIX
**
**	Written by Timm S. Mueller <tmueller@neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	These structures should be accessed only (and only as a
**	last resort) in platform-specific driver code
*/

#include <tek/exec.h>
#include <pthread.h>
#include <sys/time.h>

/*****************************************************************************/

struct HALPosixSpecific
{
	TTAGITEM hps_Tags[4];				/* Host properties container */
	TSTRPTR hps_SysDir;					/* Global system directory */
	TSTRPTR hps_ModDir;					/* Global module directory */
	TSTRPTR hps_ProgDir;				/* Application local directory */
	pthread_key_t hps_TSDKey;			/* Thread specific data key */
	pthread_mutex_t hps_DevLock;		/* Locking for module base */
	TUINT hps_RefCount;					/* Open reference counter */
	TAPTR hps_ExecBase;					/* Inserted at device open */
	TAPTR hps_DevTask;					/* Created at device open */
	TLIST hps_ReqList;					/* List of pending requests */
	TFLOAT hps_TZDays;					/* Days west of GMT */
	TINT hps_TZSec;						/* Seconds west of GMT */
};

struct HALPosixThread
{
	pthread_t hpt_PThread;				/* Thread handle */
	void *hpt_Data;						/* Task data ptr */
	void (*hpt_Function)(void *);		/* Task function */
	TAPTR hpt_HALBase;					/* HAL module base ptr */
	pthread_mutex_t hpt_SigMutex;		/* Signal mutex */
	pthread_cond_t hpt_SigCond;			/* Signal conditional */
	TUINT hpt_SigState;					/* Signal state */
};

struct HALUnixMod
{
	void *hum_Lib;						/* Host-specific module handle */
	TUINT (*hum_InitFunc)(TAPTR, TAPTR, TUINT16, TAPTR);
										/* Initfunction ptr */
	TUINT16 hum_Version;				/* Module major version */
};

/*****************************************************************************/
/*
**	Revision History
**	$Log: hal.h,v $
**	Revision 1.4  2005/09/13 02:45:09  tmueller
**	updated copyright reference
**	
**	Revision 1.3  2003/12/20 14:00:18  tmueller
**	hps_TZSecDays renamed to hps_TZDays
**	
**	Revision 1.2  2003/12/19 14:16:18  tmueller
**	Added hps_TZSecDays field
**	
**	Revision 1.1.1.1  2003/12/11 07:18:00  tmueller
**	Krypton import
**	
**	Revision 1.2  2003/10/28 08:52:46  tmueller
**	Reworked HAL-internal structures
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
