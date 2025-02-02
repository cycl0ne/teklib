
#ifndef _TEK_MOD_PS2_HAL_H
#define _TEK_MOD_PS2_HAL_H

/*
**	$Id: hal.h,v 1.6 2006/03/11 17:13:03 tmueller Exp $
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

struct HALSpecific
{
	TTAGITEM hsp_Tags[4];				/* Host properties container */
	TSTRPTR hsp_SysDir;					/* Global system directory */
	TSTRPTR hsp_ModDir;					/* Global module directory */
	TSTRPTR hsp_ProgDir;				/* Application local directory */
	TUINT hsp_RefCount;					/* Open reference counter */
	TAPTR hsp_ExecBase;					/* Inserted at device open */
	TAPTR hsp_DevTask;					/* Created at device open */
	struct TList hsp_ReqList;			/* List of pending requests */
	TFLOAT hsp_TZDays;					/* Days west of GMT */
	TINT hsp_TZSec;						/* Seconds west of GMT */
	TINT hsp_LockCount;					/* Global locking counter */
	struct TLock hsp_DevLock;			/* Device lock */
};

struct HALThread
{
	ee_thread_t hth_TCB;				/* Thread control block */
	TINT hth_TID;						/* Thread ID */
	TAPTR hth_Data;						/* Task data ptr */
	TVOID (*hth_Function)(TAPTR);		/* Task function */
	TAPTR hth_HALBase;					/* HAL module base ptr */
	ee_sema_t hth_SigSemaphore;			/* Signal semaphore control block */
	TINT hth_SigSID;					/* Signal semaphore ID */
	TUINT hth_SigState;					/* Signal state */
	TUINT *hth_Stack;
};

struct HALModule
{
	TAPTR hmd_Lib;						/* Host-specific module handle */
	TMODINITFUNC hwm_InitFunc;			/* Initfunction */
	TUINT16 hmd_Version;				/* Module major version */
};


/*****************************************************************************/
/*
**	Revision History
**	$Log: hal.h,v $
**	Revision 1.6  2006/03/11 17:13:03  tmueller
**	renamed HAL structures and fields
**
**	Revision 1.5  2005/11/20 16:08:39  tmueller
**	added stricter funcptr declarations for modentries
**
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
