
#ifndef _TEK_MOD_INTENT_HAL_H
#define _TEK_MOD_INTENT_HAL_H

/*
**	$Header: /cvs/teklib/teklib/tek/mod/intent/hal.h,v 1.4 2003/12/19 15:35:43 mlukat Exp $
*/

#include <tek/exec.h>
#include <elate/kn.h>

struct HALintentSpecific
{
	TTAGITEM his_Tags[4];				/* Host properties container */
	TSTRPTR his_SysDir;					/* Global system directory */
	TSTRPTR his_ModDir;					/* Global module directory */
	TSTRPTR his_ProgDir;				/* Application local directory */

	ELATE_MUTEX *his_DevLock;			/* Locking for module base */
	TUINT his_RefCount;					/* Open reference counter */
	TAPTR his_ExecBase;					/* Inserted at device open */
	TAPTR his_DevTask;					/* Created at device open */
	TLIST his_ReqList;					/* List of pending requests */
	TFLOAT his_TZDay;					/* Days west of GMT */
	TINT his_TZSec;						/* Seconds west of GMT */
};

struct HALintentThread
{
	TVOID (*hit_Init)(void *);			/* Task initialisation */
	TVOID (*hit_Code)(void *);			/* Task code */
	TVOID *hit_Data;					/* Task data */

	ELATE_MUTEX *hit_Sync;				/* Task mutex */
	ELATE_COND_VAR *hit_Cond;			/* Task condition variable */

	TUINT hit_SigState;					/* signal state */
	TUINT hit_PID;						/* Task ID */
};

struct HALintentMod
{
	TUINT16 him_Version;
	TUINT (*him_InitFunc)(TAPTR, TAPTR, TUINT16, TAPTR);
};


/*
**	Revision History
**	$Log: hal.h,v $
**	Revision 1.4  2003/12/19 15:35:43  mlukat
**	back in sync with posix version (applied abs date fix)
**	
**	Revision 1.3  2003/12/19 11:14:57  mlukat
**	added hal_beginio support
**	
**	Revision 1.2  2003/12/18 14:47:31  mlukat
**	added support for hal_open, hal_close, hal_abortio
**	
**	Revision 1.1  2003/12/17 15:32:49  mlukat
**	initial version
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
**	
*/

#endif
