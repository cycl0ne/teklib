
#ifndef _TEK_MOD_EXEC_H
#define _TEK_MOD_EXEC_H

/*
**	$Id: exec.h,v 1.8 2005/09/13 02:45:09 tmueller Exp $
**	teklib/tek/mod/exec.h - Exec module private
**	See copyright notice in teklib/COPYRIGHT
**
**	Do not depend on this include file, as it may break your code's
**	binary compatibility with newer versions of the exec module.
**	Normal applications require only tek/exec.h
*/

#include <tek/exec.h>
#include <tek/mod/time.h>

/*****************************************************************************/
/*
**	MMU allocation header
*/

union TMMUInfo
{
	struct
	{
		struct TMemManager *tmu_MMU;	/* Ptr to memory manager */
		TUINT tmu_UserSize;				/* Size of allocation (excl. mmuinfo) */
	} tmu_Node;
	
	struct TMMUInfoAlign tmu_Align;		/* Enforce per-platform alignment */
};

/*****************************************************************************/
/*
**	Memory free node
*/

union TMemNode
{
	struct
	{
		union TMemNode *tmn_Next;	/* Next free node, or TNULL */
		TUINT tmn_Size;				/* Size of this node, in bytes */
	} tmn_Node;

	struct TMemNodeAlign tmn_Align;	/* Enforce per-platform alignment */
};

/*****************************************************************************/
/*
**	Memheader and pool node
*/

union TMemHead
{
	struct
	{
		struct TNode tmh_Node;			/* Node header */
		union TMemNode *tmh_FreeList;	/* Singly linked list of free nodes */
		TINT8 *tmh_Mem;					/* Memory block */
		TINT8 *tmh_MemEnd;				/* End of memory block, aligned */
		TAPTR tmh_Pad1;					/* Padding */
		TUINT tmh_Free;					/* Number of free bytes */
		TUINT tmh_Align;				/* Alignment in bytes - 1 */
		TUINT tmh_Flags;				/* Flags, see below */
		TUINT tmh_Pad2;					/* Padding */
	} tmh_Node;
	
	struct TMemHeadAlign tmh_Align;		/* enforce alignment */
};

#define TMEMHF_NONE		0			/* Firstmatch allocation strategy */
#define TMEMHF_LOWFRAG	1			/* Bestmatch allocation strategy */
#define TMEMHF_LARGE	2			/* Used in pools: large puddle */
#define TMEMHF_AUTO		4			/* Used in pools: auto adaptive */
#define TMEMHF_FIXED	8			/* Used in pools: fixed size */
#define TMEMHF_FREE		16			/* Used in pools: free allocations */

/*****************************************************************************/
/*
**	Pooled memory header
*/

struct TMemPool
{
	struct TModObject tpl_Handle;	/* Exec object handle */
	struct TList tpl_List;			/* List of puddles */
	TAPTR tpl_MMU;					/* Parent allocator */
	TUINT tpl_PudSize;				/* Size of puddles */
	TUINT tpl_ThresSize;			/* Threshold for large allocations */
	TUINT16 tpl_Align;				/* Alignment in bytes - 1 */
	TUINT16 tpl_Flags;				/* Flags (passed to memheader) */
};

/*****************************************************************************/
/*
**	Locking object
*/

struct TLock
{
	struct TModObject tlk_Handle;	/* Exec object handle */
	struct TList tlk_Waiters;		/* List of lockwait requests */
	struct THALObject tlk_HLock;	/* HAL locking object */
	struct TTask *tlk_Owner;		/* Current owner */
	TUINT16 tlk_NestCount;			/* Recursion counter */
	TUINT16 tlk_WaitCount;			/* Number of waiters */
};

/* 
**	Lockwait request
*/

struct TLockWait
{
	struct TNode tlr_Node;			/* Link to tlk_Waiters */
	struct TTask *tlr_Task;			/* Task waiting for the lock */
};

/*****************************************************************************/
/* 
**	Message port
**	access point for inter-tasks messages, coupled with a task signal
*/

struct TMsgPort
{
	struct TModObject tmp_Handle;	/* Exec object handle */
	struct TList tmp_MsgList;		/* List of queued messages */
	struct THALObject tmp_Lock;		/* HAL locking object */
	struct TTask *tmp_SigTask;		/* Task to be signalled on arrival */
	TUINT tmp_Signal;				/* Signal to appear in sigtask */
};

/*****************************************************************************/
/*
**	Memory manager, aka 'MMU'
*/

struct TMemManager
{
	struct TModObject tmm_Handle;	/* Exec object handle */
	TCALLBACK TAPTR (*tmm_Realloc)(struct TMemManager *mmu, TINT8 *mem, 
		TUINT osize, TUINT nsize);	/* Realloc function */
	TCALLBACK TAPTR (*tmm_Alloc)(struct TMemManager *mmu,
		TUINT size);				/* Alloc function */
	TCALLBACK TVOID (*tmm_Free)(struct TMemManager *mmu, 
		TINT8 *mem, TUINT size);	/* Free function */
	TAPTR tmm_Allocator;			/* MMU's underlying allocator */
	struct TList tmm_TrackList;		/* List header for tracking managers */
	struct TLock tmm_Lock;			/* Locking for thread-safe managers */
	TDFUNC tmm_DestroyFunc;			/* MMU destructor */
	TUINT tmm_Type;					/* MMU type and capability flags */
};

/*****************************************************************************/
/* 
**	Task request
**	The task structure is actually a message. A command code and request
**	structure determines what kind of action is to be taken when a task
**	is being messaged to Exec.
*/

union TTaskRequest
{
	struct
	{
		struct TNode trt_Node;		/* Link to sysbase task lists */
		struct TTask *trt_Task;		/* Task to be created/closed */
		struct TTask *trt_Parent;	/* Backptr to initiating task */
		TTASKFUNC trt_Func;			/* Task function */
		TINITFUNC trt_InitFunc;		/* Task init function */
		TTAGITEM *trt_Tags;			/* User-supplied tags for creation */
	} trq_Task;

	struct
	{
		struct TModule trm_InitMod;	/* Module 'stub' during initialization */
		struct TModule *trm_Module;	/* Loaded/initialized module */
		struct TList trm_Waiters;	/* List of waiters for this module */
		struct TMsgPort *trm_RPort;	/* Backup of original replyport */
		struct TTask *trm_ReqTask;	/* Backup of requesting task */
	} trq_Mod;

	struct
	{
		struct TNode tra_Node;		/* Link to atom's list of waiters */
		struct TTask *tra_Task;		/* Requesting/waiting task */
		TAPTR tra_Atom;				/* Ptr to name or atom */
		TUINT tra_Mode;				/* Access mode */
	} trq_Atom;
};

/* 
**	Request codes for task->tsk_ReqCode
*/

#define TTREQ_OPENMOD		0		/* Get a new module */
#define TTREQ_CLOSEMOD		1		/* Close a module */
#define TTREQ_LOADMOD		2		/* Load a module from ramlib */
#define TTREQ_UNLOADMOD		3		/* Let ramlib unload module */
#define TTREQ_CREATETASK	4		/* Create a new task */
#define TTREQ_DESTROYTASK	5		/* Destroy task */
#define TTREQ_LOCKATOM		6		/* Lock a named atom */
#define TTREQ_UNLOCKATOM	7		/* Unlock a named atom */

/*****************************************************************************/
/* 
**	Task. TEKlib tasks are 'heavyweight threads', as they come with
**	predefined message ports and signals, a memory manager, and they
**	can inherit a currentdir and I/O descriptors for stdin/out/err
*/

struct TTask						/* Task */
{
	struct TModObject tsk_Handle;	/* Exec object handle */
	TAPTR tsk_UserData;				/* User/init data */

	TUINT tsk_ReqCode;				/* Task request code */
	union TTaskRequest tsk_Request;	/* Task request data */

	struct TMsgPort tsk_UserPort;	/* Task's primary async user port */
	struct TMsgPort tsk_SyncPort;	/* Task's internal sync replyport */

	struct TMemManager tsk_HeapMMU;	/* Heap memory manager */

	struct THALObject tsk_TaskLock;	/* HAL locking object */
	struct THALObject tsk_Thread;	/* HAL thread object */

	TUINT tsk_SigFree;				/* Free signals */
	TUINT tsk_SigUsed;				/* Used signals */
	TUINT tsk_Status;				/* Task status */

	TAPTR tsk_CurrentDir;			/* Lock to current directory */
	TINT tsk_IOErr;					/* Last I/O error */
	
	TAPTR tsk_IOBase;				/* I/O module base ptr */

	TAPTR tsk_FHOut;				/* Output stream */
	TAPTR tsk_FHIn;					/* Input stream */
	TAPTR tsk_FHErr;				/* Error stream */
	
	TUINT tsk_Flags;				/* Task flags, see below */
};

/* 
**	Task flags
*/

#define TTASKF_CLOSEINPUT	1		/* Close input FH on task exit */
#define TTASKF_CLOSEOUTPUT	2		/* Close output FH on task exit */
#define TTASKF_CLOSEERROR	4		/* Close error FH on task exit */

/* 
**	Task status
*/

#define TTASK_INITIALIZING	0		/* Task is initializing */
#define TTASK_RUNNING		1		/* Task is running */	
#define TTASK_FINISHED		2		/* Task has concluded */
#define TTASK_FAILED		3		/* Task failed to initialize */

/* 
**	Set of signals reserved for the system
*/

#define TTASK_SIG_RESERVED 	0x0000000f

/*****************************************************************************/
/* 
**	Execbase structure
*/

typedef struct
{
	struct TModule texb_Module;			/* Module header */
	struct TModule *texb_HALBase;		/* HAL module base */
	struct TMemManager texb_BaseMMU;	/* General purpose memory manager */
	struct TMemManager texb_MsgMMU;		/* Message memory manager */
	struct THALObject texb_Lock;		/* Locking for Execbase structures */
	struct TList texb_ModList;			/* List of modules */
	struct TList texb_TaskList;			/* List of tasks */
	struct TList texb_TaskInitList;		/* List of initializing tasks */
	struct TList texb_TaskExitList;		/* List of closing tasks */
	struct TList texb_AtomList;			/* List of named atoms */
	struct TInitModule *texb_IMods;		/* Array of internal modules */
	struct TTask *texb_ExecTask;		/* Ptr to Execbase task */
	struct TTask *texb_IOTask;			/* Ptr to task for module loading */
	struct TMsgPort *texb_ExecPort;		/* Ptr to &exectask->userport */
	struct TMsgPort *texb_ModReply;		/* Ptr to &exectask->syncport */
	TINT texb_NumTasks;					/* Number of user tasks running */
	TINT texb_NumInitTasks;				/* Number of initializing tasks */

} TEXECBASE;

/* 
**	Execbase signals
*/

#define TTASK_SIG_CHILDINIT	0x80000000	/* Task change from init to running */
#define TTASK_SIG_CHILDEXIT	0x40000000	/* Task exited */

/*****************************************************************************/
/* 
**	Message
**	Note that in TEKlib the message header is encapsulated in the
**	message allocation, and invisible to the application.
*/

struct TMessage
{
	struct TNode tmsg_Node;			/* Node header */
	struct TMsgPort *tmsg_RPort;	/* Port to which msg is returned */
	TAPTR tmsg_Sender;				/* Sender object (reserved) */
	TAPTR tmsg_Proxy;				/* Proxy object (reserved) */
	TUINT tmsg_Flags;				/* Delivery status */
};

/* 
**	Support macros
*/

#define TGetMsgPtr(mem)				((struct TMessage *) ((TINT8 *) (mem) - \
									sizeof(union TMMUInfo)) - 1)
#define TGetMsgStatus(mem)			TGetMsgPtr(mem)->tmsg_Flags
#define TGetMsgReplyPort(mem)		TGetMsgPtr(mem)->tmsg_RPort

/*****************************************************************************/
/* 
**	Atoms
*/

struct TAtom
{
	struct TModObject tatm_Handle;	/* Exec object handle */
	struct TList tatm_Waiters;		/* List of waiting tasks */
	TTAG tatm_Data;					/* User data field */
	TAPTR tatm_Owner;				/* Exclusive owner task */
	TUINT tatm_State;				/* State */
	TUINT tatm_Nest;				/* Nest count */
};

#define TATOMF_LOCKED	0x0020		/* Internal use only */
#define TATOMF_UNLOCK	0x0040		/* Internal use only */

/*****************************************************************************/
/*
**	Revision History
**	$Log: exec.h,v $
**	Revision 1.8  2005/09/13 02:45:09  tmueller
**	updated copyright reference
**	
**	Revision 1.7  2005/09/08 00:07:50  tmueller
**	modflags are now in tek/exec.h
**	
**	Revision 1.6  2005/01/30 06:21:38  tmueller
**	added fixed-size memory pools
**	
**	Revision 1.5  2005/01/29 22:27:03  tmueller
**	added alignment scheme to TMemNode and TMMUInfo
**	
**	Revision 1.4  2004/04/18 14:21:28  tmueller
**	tatm_Data atom userdata field changed from TAPTR to TTAG
**	
**	Revision 1.3  2004/04/17 12:11:18  tmueller
**	Fixed padding of struct TMemHead for 64bit; 32bit-specific comments removed
**	
**	Revision 1.2  2003/12/19 14:15:53  tmueller
**	Misalignment in message structure fixed by padding with tmsg_Proxy
**	
**	Revision 1.1.1.1  2003/12/11 07:17:49  tmueller
**	Krypton import
**	
**	Revision 1.8  2003/10/18 21:10:48  tmueller
**	TExecObject renamed to TModObject
**	
**	Revision 1.7  2003/10/16 23:11:38  tmueller
**	Major cleanup, formatting, comments added
**	
**	Revision 1.6  2003/10/16 17:34:47  tmueller
**	Renamed many Exec-internal structures, comments added, cleanup
**	
**	Revision 1.5  2003/10/13 20:59:32  tmueller
**	Renamed exec-internal structures TMMUInfo, TMemHead and TMemPool. Internal
**	exec_free() function calls have been replaced with the external exec_Free()
**	
**	Revision 1.4  2003/10/12 19:19:47  tmueller
**	struct TTask equivalent to TTASK type added
**	
**	Revision 1.3  2003/10/04 02:02:26  tmueller
**	Cleanup and beautyfication
**	
**	Revision 1.2  2003/05/11 14:09:56  tmueller
**	Message status flags changed names and values. The message->status field
**	has been renamed to message->flags
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.7  2003/02/13 18:27:06  cybin
**	darwin port now compiles with -DTSYS_DARWIN and uses correct
**	host and platform values.
**	
**	added files for io module. should be the same like posix32.
**	
**	Revision 1.6  2003/02/09 13:33:11  tmueller
**	new definitions for I/O added
**	
**	Revision 1.5  2003/02/02 23:11:25  tmueller
**	Tasks now by default inherit the current directory from their parent. The
**	parentdir lock to inherit from can be overriden using the TTask_CurrentDir
**	attribute passed to TCreateTask()
**	
**	Revision 1.4  2003/02/02 03:45:42  tmueller
**	atoms are now handled in execbase context
**	
**	Revision 1.3  2003/01/20 21:22:05  tmueller
**	currentdir and ioerr added to task structure
**	
**	Revision 1.2  2002/12/07 09:23:28  bifat
**	msgmmu ptr removed from task structure
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
