
#ifndef _TEK_EXEC_H
#define	_TEK_EXEC_H

/*
**	$Id: exec.h,v 1.13 2005/09/13 02:44:06 tmueller Exp $
**	teklib/tek/exec.h - Public Exec types, structures, macros, constants
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/type.h>

/*****************************************************************************/
/* 
**	Nodes and lists
**
**	Lists are doubly-linked with a header acting as a "Null" node.
**	A typical iterator for that topology may look like this:
**
**	struct TNode *next, *node = list->tlh_Head;
**	for (; (next = node->tln_Succ); node = next)
**	{
**		you can operate on 'node' here, remove it safely,
**		as well as 'break' out from the loop and 'continue' it
**	}
*/

struct TNode
{
	struct TNode *tln_Succ;			/* Ptr to successor in the list */
	struct TNode *tln_Pred;			/* Ptr to predecessor in the list */
};

struct TList
{
	struct TNode *tlh_Head;			/* Ptr to head node of list */
	struct TNode *tlh_Tail;			/* Ptr to tail node of list */
	struct TNode *tlh_TailPred;		/* Ptr to tail predecessor */
};

typedef struct TNode TNODE;
typedef struct TList TLIST;

/*****************************************************************************/
/* 
**	Generic handle with destructor
**
**	Objects of this type can be linked into lists, searched for by name,
**	and cleaned up by calling their destructor. The link library provides
**	the functions TDestroy(), TDestroyList(), and TFindHandle().
*/

typedef TCALLBACK TVOID (*TDFUNC)(TAPTR handle);	/* Destroy function */

struct THandle
{
	struct TNode thn_Node;		/* Node header */
	TAPTR thn_Data;				/* TFindHandle() expects a name here */
	TDFUNC thn_DestroyFunc;		/* Destroy function for this handle */
};

typedef struct THandle THNDL;

/*****************************************************************************/
/* 
**	Module object header
**	
**	Structurally equivalent to a generic handle, extending it by a module
**	base pointer. This makes it easy to retrieve a module base from all
**	kinds of objects provided by a module, like tasks, ports, memory
**	managers, locks, atoms and modules themselves (in case of Exec).
*/

struct TModObject
{
	struct TNode tmo_Node;		/* Node header */
	TSTRPTR tmo_Name;			/* Object name */
	TDFUNC tmo_DestroyFunc;		/* Destroy function for this handle */
	TAPTR tmo_ModBase;			/* Module base pointer */
};

#define TGetModBase(obj)		(((struct TModObject *) (obj))->tmo_ModBase)
#define TGetExecBase(obj)		TGetModBase(obj)	/* An alias */

/*****************************************************************************/
/* 
**	Module header
*/

typedef TCALLBACK struct TModule *(*TMODOPENFUNC) (struct TModule *mod,
	TAPTR task, struct TTagItem *tags);
typedef TCALLBACK TVOID (*TMODCLOSEFUNC) (struct TModule *mod, TAPTR task);

struct TModule
{
	struct TNode tmd_Node;			/* Node header */
	TSTRPTR tmd_Name;				/* Module name */
	TDFUNC tmd_DestroyFunc;			/* Destroy function */
	TAPTR tmd_ExecBase;				/* Exec module base ptr */
	TAPTR tmd_HALMod;				/* Ptr to HAL module handle */
	TAPTR tmd_Reserved;				/* Reserved for future extensions */
	TAPTR tmd_InitTask;				/* Task that opened this instance */
	struct TModule *tmd_ModSuper;	/* (Back-) ptr to module super instance */
	TMODOPENFUNC tmd_OpenFunc;		/* Mod instance open function, optional */
	TMODCLOSEFUNC tmd_CloseFunc;	/* Corresponding close function */
	TUINT tmd_NegSize;				/* Modbase negative size [bytes] */
	TUINT tmd_PosSize;				/* Modbase positive size [bytes] */
	TUINT tmd_Flags;				/* Flags */
	TUINT tmd_RefCount;				/* Number of instances open */
	TUINT16 tmd_Version;			/* Major (checked against) */
	TUINT16 tmd_Revision;			/* Minor (informational) */
};

typedef struct TModule TMODL;

/*****************************************************************************/
/* 
**	Task signals
**
**	Currently there are 28 free user signals, but the number of signals with
**	predefined meaning (like TTASK_SIG_ABORT or those reserved for the
**	inbuilt message ports) may grow in the future. TEKlib guarantees that the
**	upper 24 signal bits (0x80000000 through 0x00000100) will remain available
**	to the user. More free user signals (if available) can be obtained safely
**	with TExecAllocSignal(). Allocation is recommended anyway.
*/

#define TTASK_SIG_ABORT		0x00000001	/* Reserved meaning: ABORT */
#define TTASK_SIG_BREAK		0x00000002	/* Reserved meaning: BREAK */
#define TTASK_SIG_SINGLE	0x00000004	/* Reserved for task's syncport */
#define TTASK_SIG_USER		0x00000008	/* Reserved for task's userport */

/* 
**	Task entry functions
*/

typedef TTASKENTRY TVOID (*TTASKFUNC)(TAPTR task);	/* Main function */
typedef TTASKENTRY TBOOL (*TINITFUNC)(TAPTR task);	/* Init function */

/* 
**	Task tags
*/

#define TEXECTAGS_			(TTAG_USER + 0x300)

#define TTask_UserData		(TEXECTAGS_ + 0)	/* Ptr to user/init data */
#define TTask_Name			(TEXECTAGS_ + 1)	/* Task name */
#define TTask_MMU			(TEXECTAGS_ + 2)	/* Parent memory manager */
#define TTask_HeapMMU		(TEXECTAGS_ + 3)	/* Task's heap memmanager */
#define TTask_MsgMMU		(TEXECTAGS_ + 4)	/* Message memory manager */

#define TTask_CurrentDir	(TEXECTAGS_ + 5)	/* Lock to a currentdir */
#define TTask_InputFH		(TEXECTAGS_ + 6)	/* Input file handle */
#define TTask_OutputFH		(TEXECTAGS_ + 7)	/* Output file handle */
#define TTask_ErrorFH		(TEXECTAGS_ + 8)	/* Error file handle */

/*****************************************************************************/
/*
**	Application, Execbase and HAL entry tags
*/

#define TExecBase_Version	(TEXECTAGS_ + 16)	/* Exec module version */
#define TExecBase_ArgC		(TEXECTAGS_ + 17)	/* Argument count */
#define TExecBase_ArgV		(TEXECTAGS_ + 18)	/* Argument array */
#define TExecBase_ModInit	(TEXECTAGS_ + 20)	/* Startup modules */
#define TExecBase_RetValP	(TEXECTAGS_ + 21)	/* Ptr to return value */
#define TExecBase_SysDir	(TEXECTAGS_ + 22)	/* System global path */
#define TExecBase_ModDir	(TEXECTAGS_ + 23)	/* Global path to modules */
#define TExecBase_ProgDir	(TEXECTAGS_ + 24)	/* Application progdir */
#define TExecBase_HAL		(TEXECTAGS_ + 25)	/* Pass HAL base to Exec */
#define THalBase_Exec		(TEXECTAGS_ + 26)	/* Pass Exec base to HAL */
#define TExecBase_Arguments	(TEXECTAGS_ + 27)	/* Single string of args */
#define TExecBase_ProgName	(TEXECTAGS_ + 28)	/* Progname, like argv[0] */
#define TExecBase_MemBase	(TEXECTAGS_ + 29)	/* System memory base */
#define TExecBase_MemSize	(TEXECTAGS_ + 30)	/* System memory size */
#define TExecBase_BootHnd	(TEXECTAGS_ + 31)	/* Boot handle passed to HAL */

/*****************************************************************************/
/* 
**	System names
*/

#define TTASKNAME_EXEC		"exec"			/* Name of task running Exec */
#define TTASKNAME_ENTRY		"entry"			/* Name of initial entry task */
#define TTASKNAME_RAMLIB	"ramlib"		/* Name of module loader task */
#define TTASKNAME_HALDEV	"hal.device"	/* Name of HAL device task */
#define TMODNAME_EXEC		"exec"			/* Name of the Exec Module */
#define TMODNAME_HAL		"hal"			/* Name of the HAL Module */
#define TMODNAME_TIMER		"timer.device"	/* Alias for the HAL device */

/*****************************************************************************/
/* 
**	Initial startup module
**
**	An array of this structure can be passed to a TEKlib framework using
**	the TExecBase_ModInit tag. This allows to statically link modules to
**	an application. The array is terminated with an entry whose tinm_Name
**	is TNULL.
*/

typedef TMODENTRY TUINT (*TMODINITFUNC)
	(TAPTR task, struct TModule *mod, TUINT16 version, struct TTagItem *tags);

struct TInitModule
{
	TSTRPTR tinm_Name;			/* Module name */
	TMODINITFUNC tinm_InitFunc;	/* Mod init function */
	TAPTR tinm_Reserved;		/* For future extensions, must be TNULL */
	TUINT tinm_Flags;			/* Reserved, must be 0 */
};

/*****************************************************************************/
/* 
**	Atom mode flags
*/

#define TATOMF_KEEP		0x00	/* Do not create a new or destroy an atom */
#define TATOMF_CREATE	0x01	/* Create a new atom, if it does not exist */
#define TATOMF_DESTROY	0x02	/* Destroy an existing atom */
#define TATOMF_SHARED	0x04	/* Lock shared */
#define TATOMF_NAME		0x08	/* Name is passed instead of an atom */
#define TATOMF_TRY		0x10	/* Only attempt locking (or creation) */

/*****************************************************************************/
/* 
**	Memory manager types
*/

#define TMMUT_Void		0x00000000	/* Void MMU incapable of allocating */
#define TMMUT_MMU		0x00000001	/* Put MMU on top of a parent MMU */
#define TMMUT_Static	0x00000002	/* Put MMU on top of a static memblock */
#define TMMUT_Pooled	0x00000004	/* Put MMU on top of a pooled allocator */
#define TMMUT_Tracking	0x00000008	/* Leak-tracking on top of a parent MMU */
#define TMMUT_TaskSafe	0x00000100	/* Thread-safety on top of a parent MMU */
#define TMMUT_Message	0x00000200	/* Msg allocator on parent msg MMU */
#define TMMUT_Debug		0x00000400	/* Bounds checking on top of parent MMU */

/* 
**	Tags for memory allocators
*/

#define	TMem_LowFrag	(TEXECTAGS_ + 64)	/* Low-fragment strategy */
#define TMem_StaticSize	(TEXECTAGS_ + 65)	/* Static mmu: size of memblock */
#define	TPool_MMU		(TEXECTAGS_ + 66)	/* Parent memory manager */
#define	TPool_PudSize	(TEXECTAGS_ + 67)	/* Size of puddles */
#define	TPool_ThresSize	(TEXECTAGS_ + 68)	/* Thressize for large puddles */
#define	TPool_AutoAdapt	(TEXECTAGS_ + 69)	/* Auto-adaptive pud/thressize */
#define TPool_Static	(TEXECTAGS_ + 70)
#define TPool_StaticSize (TEXECTAGS_ + 71)

/*****************************************************************************/
/* 
**	Message status, as returned by TExecSendMsg().
**	Flags are used by Exec internally, applications use the status defines.
*/

#define TMSGF_SENT			0x0001	/* Flag: Message has been sent */
#define TMSGF_RETURNED		0x0002	/* Flag: Message was returned */
#define TMSGF_MODIFIED		0x0004	/* Flag: Message body may be modified */
#define TMSGF_QUEUED		0x0008	/* Flag: Message is queued in a port */

#define TMSG_STATUS_FAILED	0x0000	/* Status: Message was dropped */
#define TMSG_STATUS_SENT	0x0001	/* Status: Message was sent */
#define TMSG_STATUS_ACKD	0x0003	/* Status: Message TExecAckMsg()'ed */
#define TMSG_STATUS_REPLIED	0x0007	/* Status: Message TExecReplyMsg()'ed */

/*****************************************************************************/
/* 
**	Standard device I/O return codes
*/

#define TIOERR_SUCCESS				0
#define TIOERR_UNKNOWN_COMMAND		(-1)
#define TIOERR_ABORTED				(-2)
#define TIOERR_DEVICE_OPEN_FAILED	(-3)

/*****************************************************************************/
/* 
**	Module flags
*/

#define TMODF_NONE			0x0000
#define TMODF_INITIALIZED	0x0001	/* Module is ready - internal use only */
#define TMODF_EXTENDED		0x0002	/* reserve lower eight function vectors */

/*****************************************************************************/
/* 
**	Utility macros
*/

/* Get first node of a list */
#define	TFirstNode(list)	((list)->tlh_Head->tln_Succ ? \
							(list)->tlh_Head : TNULL)

/* Get last node of a list */
#define TLastNode(list)		((list)->tlh_TailPred->tln_Pred ? \
							(list)->tlh_TailPred : TNULL)

/* Test if a list is empty */
#define TListEmpty(list)	((list)->tlh_TailPred == (struct TNode *) (list))

/*****************************************************************************/
/*
**	Macro versions of some list functions
*/

#define TINITLIST(l) (										\
		(l)->tlh_Head = (struct TNode *) &(l)->tlh_Tail,	\
		(l)->tlh_Tail = TNULL,								\
		(l)->tlh_TailPred = (struct TNode *) (l))			\

#define TREMHEAD(l,t) (										\
		((t) = (l)->tlh_Head)->tln_Succ ?					\
		(l)->tlh_Head = (t)->tln_Succ,						\
		(t)->tln_Succ->tln_Pred = 							\
			(struct TNode *) &(l)->tlh_Head,				\
		(t) : TNULL)

#define TREMTAIL(l,t) (										\
		((t) = (l)->tlh_TailPred)->tln_Pred ?				\
		(l)->tlh_TailPred = (t)->tln_Pred,					\
		(t)->tln_Pred->tln_Succ = 							\
			(struct TNode *) &(l)->tlh_Tail,				\
		(t) : TNULL)

#define TADDHEAD(l,n,t) (									\
		(t) = (l)->tlh_Head,								\
		(l)->tlh_Head = (n),								\
		(n)->tln_Succ = (t),								\
		(n)->tln_Pred = (struct TNode *) &(l)->tlh_Head,	\
		(t)->tln_Pred = (n))

#define TADDTAIL(l,n,t) (									\
		(t) = (l)->tlh_TailPred,							\
		(l)->tlh_TailPred = (n),							\
		(n)->tln_Succ = (struct TNode *) &(l)->tlh_Tail,	\
		(n)->tln_Pred = (t),								\
		(t)->tln_Succ = (n))

#define TREMOVE(n) (										\
		(n)->tln_Pred->tln_Succ = (n)->tln_Succ,			\
		(n)->tln_Succ->tln_Pred = (n)->tln_Pred)

#define TDESTROY(h) (										\
		(h) ? (((struct THandle *)(h))->thn_DestroyFunc ?	\
		(*((struct THandle *)(h))->thn_DestroyFunc)(h), 	\
		(h) : TNULL) : TNULL)

/*****************************************************************************/
/*
**	Revision History
**	$Log: exec.h,v $
**	Revision 1.13  2005/09/13 02:44:06  tmueller
**	updated copyright reference
**	
**	Revision 1.12  2005/09/08 00:06:39  tmueller
**	made modflags public; introduced TMODF_EXTENDED
**	
**	Revision 1.11  2005/05/27 19:45:04  tmueller
**	added author to header
**	
**	Revision 1.10  2005/04/01 18:51:03  tmueller
**	Added ExecBase init tags: MemBase, MemSize, BootHnd
**	
**	Revision 1.9  2005/01/30 06:21:38  tmueller
**	added fixed-size memory pools
**	
**	Revision 1.8  2005/01/21 11:41:13  tmueller
**	added private alignment tag TMem_Align
**	
**	Revision 1.7  2004/07/04 17:18:51  tmueller
**	added tags for passing arguments and progname
**	
**	Revision 1.6  2004/04/17 13:34:51  tmueller
**	Improved commentary; Increased number of user-reserved signals to 24
**	
**	Revision 1.5  2004/03/29 02:50:20  tmueller
**	Added module name definitions
**	
**	Revision 1.4  2004/01/31 11:32:59  tmueller
**	Fixed some typos in comments
**	
**	Revision 1.3  2003/12/24 16:41:31  tmueller
**	The evil and dangerous TTimeToF() macro has been removed.
**	
**	Revision 1.2  2003/12/12 03:45:03  tmueller
**	Cosmetic
**	
**	Revision 1.1.1.1  2003/12/11 07:17:44  tmueller
**	Krypton import
*/

#endif
