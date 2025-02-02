
#ifndef _TEK_MOD_PS2_HAL_H
#define _TEK_MOD_PS2_HAL_H

/*
**	$Id: hal.h,v 1.4 2005/09/18 11:27:22 tmueller Exp $
**	teklib/tek/mod/hal/ps2/hal.h - HAL internal structures on PS2
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	and Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tamtypes.h>
#include <kernel.h>

#include <tek/mod/exec.h>				/* exec internal stuff */

/*****************************************************************************/

struct HALMemNode
{
	struct HALMemNode *hmn_Next;		/* Next free node, or TNULL */
	TUINT hmn_Size;						/* Size of this node, in bytes */
};

struct HALMemHead
{
	struct HALMemNode *hmh_FreeList;	/* Singly linked list of free nodes */
	TINT8 *hmh_Mem;						/* Memory block */
	TINT8 *hmh_MemEnd;					/* End of memory block, aligned */
	TAPTR hmh_Pad1;						/* Padding */
	TUINT hmh_Free;						/* Number of free bytes */
	TUINT hmh_Align;					/* Alignment in bytes - 1 */
	TUINT hmh_Flags;					/* Flags, see below */
	TUINT hmh_Pad2;						/* Padding */
};

struct HALPS2Boot
{
	struct HALMemHead *hpb_MemHead;
	TAPTR (*hpb_Alloc)(TAPTR, TUINT);
	TVOID (*hpb_Free)(TAPTR, TAPTR, TUINT);
	TAPTR (*hpb_Realloc)(TAPTR, TAPTR, TUINT, TUINT);
};

struct HALPS2Specific
{
	TTAGITEM hps_Tags[4];				/* Host properties container */
	TSTRPTR hps_SysDir;					/* Global system directory */
	TSTRPTR hps_ModDir;					/* Global module directory */
	TSTRPTR hps_ProgDir;				/* Application local directory */
	TUINT hps_RefCount;					/* Open reference counter */
	TAPTR hps_ExecBase;					/* Inserted at device open */
	TAPTR hps_DevTask;					/* Created at device open */
	TLIST hps_ReqList;					/* List of pending requests */
	TFLOAT hps_TZDays;					/* Days west of GMT */
	TINT hps_TZSec;						/* Seconds west of GMT */
	TINT hps_LockCount;					/* Global locking counter */
	struct TLock hps_DevLock;			/* Device lock */
};

struct HALPS2Thread
{
	ee_thread_t hpt_TCB;				/* Thread control block */
	TINT hpt_TID;						/* Thread ID */
	TAPTR hpt_Data;						/* Task data ptr */
	TVOID (*hpt_Function)(TAPTR);		/* Task function */
	TAPTR hpt_HALBase;					/* HAL module base ptr */
	ee_sema_t hpt_SigSemaphore;			/* Signal semaphore control block */
	TINT hpt_SigSID;					/* Signal semaphore ID */
	TUINT hpt_SigState;					/* Signal state */
	TUINT *hpt_Stack;
};

struct HALPS2Mod
{
	TAPTR hpm_Lib;						/* Host-specific module handle */
	TUINT (*hpm_InitFunc)(TAPTR, TAPTR, TUINT16, TAPTR);
										/* Initfunction ptr */
	TUINT16 hpm_Version;				/* Module major version */
};


/*****************************************************************************/
/*
**	Revision History
**	$Log: hal.h,v $
**	Revision 1.4  2005/09/18 11:27:22  tmueller
**	added authors
**	
**	Revision 1.3  2005/04/01 18:52:51  tmueller
**	Added HALPS2Boot structure containing the platform's global allocator
**	
**	Revision 1.2  2005/03/19 19:25:09  fschulze
**	added decent device locks and fixed timer device
**	
**	Revision 1.1  2005/03/13 20:01:39  fschulze
**	added
*/

#endif /* _TEK_MOD_PS2_HAL_H */
