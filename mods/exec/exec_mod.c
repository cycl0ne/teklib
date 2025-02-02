
/*
**	$Id: exec_mod.c,v 1.13 2005/09/13 02:41:58 tmueller Exp $
**	teklib/mods/exec/exec_mod.c - TEKlib Exec module
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "exec_mod.h"
#include <tek/debug.h>
#include <tek/teklib.h>

static TBOOL exec_init(TEXECBASE *exec, TTAGITEM *tags);
static TCALLBACK TVOID exec_destroy(TEXECBASE *exec);
static const TAPTR exec_vectors[MOD_NUMVECTORS];

/*****************************************************************************/
/*
**	Module Init
**	Note: For initialization, a pointer to the HAL module
**	must be submitted via the TExecBase_HAL tag.
*/

TMODENTRY TUINT 
tek_init_exec(TAPTR selftask, TEXECBASE *mod, TUINT16 version, TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * MOD_NUMVECTORS;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TEXECBASE);				/* positive size */

		return 0;
	}

	if (exec_init(mod, tags))
	{
		mod->texb_Module.tmd_DestroyFunc = (TDFUNC) exec_destroy;
		mod->texb_Module.tmd_Version = MOD_VERSION;
		mod->texb_Module.tmd_Revision = MOD_REVISION;
		mod->texb_Module.tmd_Flags |= TMODF_EXTENDED;

		TInitVectors(mod, exec_vectors, MOD_NUMVECTORS);
		
		/* overwrite TExecCopyMem vector with THALCopyMem vector,
		thus getting rid of the overhead for one call frame */
		((TAPTR *) mod)[-16] = ((TAPTR *) mod->texb_HALBase)[-14];

		return TTRUE;
	}

	return 0;
}

/*****************************************************************************/
/*
**	Function vector table
*/

static const TAPTR exec_vectors[MOD_NUMVECTORS] =
{
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,

 	(TAPTR) exec_DoExec,
	(TAPTR) exec_CreateSysTask,
	(TAPTR) exec_GetHALBase,
	(TAPTR) TNULL,		/* obsolete as of v1: exec_findhandle() */

	(TAPTR) exec_OpenModule,
	(TAPTR) exec_CloseModule,
	(TAPTR) TNULL,		/* obsolete as of v1: exec_getmodules() */

	(TAPTR) exec_CopyMem,
	(TAPTR) exec_FillMem,
	(TAPTR) exec_FillMem32,

	(TAPTR) exec_CreateMMU,
	(TAPTR) exec_AllocMMU,
	(TAPTR) exec_AllocMMU0,
	(TAPTR) TNULL,		/* obsolete as of v1: exec_alloctask() */
	(TAPTR) TNULL,		/* obsolete as of v1: exec_alloctask0() */
	(TAPTR) exec_Free,
	(TAPTR) exec_Realloc,
	(TAPTR) exec_GetMMU,
	(TAPTR) exec_GetSize,

	(TAPTR) exec_CreateLock,
	(TAPTR) exec_Lock,
	(TAPTR) exec_Unlock,

	(TAPTR) exec_AllocSignal,
	(TAPTR) exec_FreeSignal,
	(TAPTR) exec_Signal,
	(TAPTR) exec_SetSignal,
	(TAPTR) exec_Wait,
	(TAPTR) TNULL,		/* obsolete as of v1: exec_timedwait() */

	(TAPTR) exec_CreatePort,
	(TAPTR) exec_PutMsg,
	(TAPTR) exec_GetMsg,
	(TAPTR) exec_AckMsg,
	(TAPTR) exec_ReplyMsg,
	(TAPTR) exec_DropMsg,
	(TAPTR) exec_SendMsg,
	(TAPTR) exec_WaitPort,
	(TAPTR) exec_GetPortSignal,
	(TAPTR) exec_GetUserPort,
	(TAPTR) exec_GetSyncPort,

	(TAPTR) exec_CreateTask,
	(TAPTR) exec_FindTask,
	(TAPTR) exec_GetTaskData,
	(TAPTR) exec_SetTaskData,
	(TAPTR) exec_GetTaskMMU,
	(TAPTR) exec_AllocMsg,
	(TAPTR) exec_AllocMsg0,

	(TAPTR) TNULL,		/* obsolete as of v1: exec_getdate() */
	(TAPTR) TNULL,		/* obsolete as of v1: exec_querytime() */
	(TAPTR) TNULL,		/* obsolete as of v1: exec_resettime() */
	(TAPTR) TNULL,		/* obsolete as of v1: exec_delay() */

	(TAPTR) exec_LockAtom,
	(TAPTR) exec_UnlockAtom,
	(TAPTR) exec_GetAtomData,
	(TAPTR) exec_SetAtomData,

	(TAPTR) exec_CreatePool,
	(TAPTR) exec_AllocPool,
	(TAPTR) exec_FreePool,
	(TAPTR) exec_ReallocPool,
	
	(TAPTR) exec_PutIO,
	(TAPTR) exec_WaitIO,
	(TAPTR) exec_DoIO,
	(TAPTR) exec_CheckIO,
	(TAPTR) exec_AbortIO,
	
	(TAPTR) exec_LockPort,
	(TAPTR) exec_UnlockPort,
	(TAPTR) exec_InsertMsg,
	(TAPTR) exec_RemoveMsg,
	(TAPTR) exec_GetMsgStatus,
	(TAPTR) exec_SetMsgReplyPort,
};

/*****************************************************************************/
/*
**	ExecBase closedown
*/

static TCALLBACK TVOID
exec_destroy(TEXECBASE *exec)
{
	THALDestroyLock(exec->texb_HALBase, &exec->texb_Lock);
	TDESTROY(&exec->texb_BaseMMU);
	TDESTROY(&exec->texb_MsgMMU);
}

/*****************************************************************************/
/*
**	ExecBase init
*/

static TBOOL
exec_init(TEXECBASE *exec, TTAGITEM *tags)
{
	TAPTR *halp, hal;

	halp = (TAPTR *) TGetTag(tags, TExecBase_HAL, TNULL);
	if (!halp) return TFALSE;
	hal = *halp;
	
	THALFillMem(hal, exec, sizeof(TEXECBASE), 0);
	exec->texb_HALBase = hal;

	if (THALInitLock(hal, &exec->texb_Lock))
	{
		if (exec_initmmu(exec, &exec->texb_MsgMMU, TNULL, TMMUT_Message, 
			TNULL))
		{
			if (exec_initmmu(exec, &exec->texb_BaseMMU, TNULL, TMMUT_TaskSafe, 
				TNULL))
			{
				struct TNode *tempn;

				exec->texb_Module.tmd_Name = (TSTRPTR) TMODNAME_EXEC;
				exec->texb_Module.tmd_ExecBase = (struct TModule *) exec;
				exec->texb_Module.tmd_ModSuper = (struct TModule *) exec;
				exec->texb_Module.tmd_InitTask = TNULL;	/* inserted later */
				exec->texb_Module.tmd_HALMod = TNULL;	/* inserted later */
				exec->texb_Module.tmd_NegSize = MOD_NUMVECTORS * sizeof(TAPTR);
				exec->texb_Module.tmd_PosSize = sizeof(TEXECBASE);
				exec->texb_Module.tmd_RefCount = 1;
				exec->texb_Module.tmd_Flags = TMODF_INITIALIZED;
				exec->texb_IMods = (struct TInitModule *)
					TGetTag(tags, TExecBase_ModInit, TNULL);

				TINITLIST(&exec->texb_AtomList);
				TINITLIST(&exec->texb_TaskList);
				TINITLIST(&exec->texb_TaskInitList);
				TINITLIST(&exec->texb_TaskExitList);
				TINITLIST(&exec->texb_ModList);
				TADDHEAD(&exec->texb_ModList, (TNODE *) exec, tempn);
				TADDHEAD(&exec->texb_ModList, (TNODE *) hal, tempn);

				return TTRUE;
			}
			TDESTROY(&exec->texb_MsgMMU);
		}
		THALDestroyLock(hal, &exec->texb_Lock);
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	iseq = exec_strequal(s1, s2)
*/

LOCAL TBOOL
exec_strequal(TSTRPTR s1, TSTRPTR s2)
{
	if (s1 && s2)
	{
		TINT8 a;
		while ((a = *s1++) == *s2++)
		{
			if (a == 0) return TTRUE;
		}
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	sigs = exec_allocsignal(execbase, task, signals)
**	Alloc signal(s) from a task
*/

LOCAL TUINT 
exec_allocsignal(TEXECBASE *exec, struct TTask *task, TUINT signals)
{
	TUINT newsignal = 0;
	TAPTR hal = exec->texb_HALBase;

	THALLock(hal, &task->tsk_TaskLock);

	if (signals)
	{
		if ((signals & task->tsk_SigFree) == signals)
		{
			newsignal = signals;
		}
	}
	else
	{
		TINT x;
		TUINT trysignal = 0x00000001;
		
		for (x = 0; x < 32; ++x)
		{
			if (!(trysignal & TTASK_SIG_RESERVED))
			{
				if (trysignal & task->tsk_SigFree)
				{
					newsignal = trysignal;
					break;
				}
			}
			trysignal <<= 1;
		}
	}

	task->tsk_SigFree &= ~newsignal;
	task->tsk_SigUsed |= newsignal;

	THALUnlock(hal, &task->tsk_TaskLock);
	
	return newsignal;
}

/*****************************************************************************/
/*
**	exec_freesignal(execbase, task, signals)
**	Return signal(s) to a task
*/

LOCAL TVOID 
exec_freesignal(TEXECBASE *exec, struct TTask *task, TUINT signals)
{
	TAPTR hal = exec->texb_HALBase;

	THALLock(hal, &task->tsk_TaskLock);
	
	#ifdef TDEBUG
	if ((task->tsk_SigUsed & signals) != signals)
	{
		tdbprintf(10,"signals freed did not match signals allocated\n");
	}
	#endif
	
	task->tsk_SigFree |= signals;
	task->tsk_SigUsed &= ~signals;

	THALUnlock(hal, &task->tsk_TaskLock);
}

/*****************************************************************************/
/*
**	success = exec_initport(exec, task, prefsignal)
**	Init a message port for the given task, with a preferred signal,
**	or allocate a new signal if prefsignal is 0
*/

static TCALLBACK TVOID 
exec_destroyport_internal(struct TMsgPort *port)
{
	TEXECBASE *exec = TGetExecBase(port);

	#ifdef TDEBUG
	if (!TListEmpty(&port->tmp_MsgList))
	{
		tdbprintf(20,"Message queue was not empty\n");
	}
	#endif
	
	exec_freesignal(exec, port->tmp_SigTask, port->tmp_Signal);
	THALDestroyLock(exec->texb_HALBase, &port->tmp_Lock);
}

LOCAL TBOOL
exec_initport(TEXECBASE *exec, struct TMsgPort *port, struct TTask *task,
	TUINT signal)
{
	if (!signal)
	{
		signal = exec_allocsignal(exec, task, 0);
		if (!signal) return TFALSE;
	}

	if (THALInitLock(exec->texb_HALBase, &port->tmp_Lock))
	{
		port->tmp_Handle.tmo_DestroyFunc = 
			(TDFUNC) exec_destroyport_internal;
		port->tmp_Handle.tmo_ModBase = exec;
		port->tmp_Handle.tmo_Name = TNULL;

		TINITLIST(&port->tmp_MsgList);
		port->tmp_Signal = signal;
		port->tmp_SigTask = task;

		return TTRUE;
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	Initialize a locking object
*/

static TCALLBACK TVOID 
destroylock(struct TLock *lock)
{
	TEXECBASE *exec = TGetExecBase(lock);
	THALDestroyLock(exec->texb_HALBase, &lock->tlk_HLock);
}

LOCAL TBOOL
exec_initlock(TEXECBASE *exec, struct TLock *lock)
{
	if (THALInitLock(exec->texb_HALBase, &lock->tlk_HLock))
	{
		lock->tlk_Handle.tmo_Name = TNULL;
		lock->tlk_Handle.tmo_DestroyFunc = (TDFUNC) destroylock;
		lock->tlk_Handle.tmo_ModBase = exec;
		TINITLIST(&lock->tlk_Waiters);
		lock->tlk_Owner = TNULL;
		lock->tlk_NestCount = 0;
		lock->tlk_WaitCount = 0;
		return TTRUE;
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	msg = exec_getmsg(exec, port)
**	Get next pending message from messageport
*/

LOCAL TAPTR
exec_getmsg(TEXECBASE *exec, struct TMsgPort *port)
{
	struct TMessage *msg;
	struct TNode *tempn;
	TAPTR hal = exec->texb_HALBase;

	THALLock(hal, &port->tmp_Lock);
	msg = (struct TMessage *) TREMHEAD(&port->tmp_MsgList, tempn);
	THALUnlock(hal, &port->tmp_Lock);

	if (msg)
	{
		TUINT status = msg->tmsg_Flags;
	
		#ifdef TDEBUG
		if (!(status & TMSGF_QUEUED))
		{
			tdbprintf(10, "got msg with TMSGF_QUEUED not set\n");
		}
		#endif

		#if 1

			msg->tmsg_Flags &= ~TMSGF_QUEUED;

		#else

			if (status == (TMSG_STATUS_REPLIED | TMSGF_QUEUED))
			{
				/* end of lifecycle */
				msg->tmsg_Flags = 0;
			}
			else
			{
				/* now pending at receiver */
				msg->tmsg_Flags &= ~TMSGF_QUEUED;
			}

		#endif
		
		return (TAPTR)((TINT8 *) (msg + 1) + sizeof(union TMMUInfo));
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	success = exec_sendmsg(exec, task, port, msg)
**	Send message, two-way, blocking
*/

LOCAL TUINT 
exec_sendmsg(TEXECBASE *exec, struct TTask *task, struct TMsgPort *port, 
	TAPTR mem)
{
	TAPTR hal = exec->texb_HALBase;
	struct TMessage *msg = TGetMsgPtr(mem);
	struct TNode *tempn;
	TAPTR reply;

	/* replyport is task's syncport */
	msg->tmsg_RPort = &task->tsk_SyncPort;

	/* sender is local address space */
	msg->tmsg_Sender = TNULL;

	THALLock(hal, &port->tmp_Lock);
	TADDTAIL(&port->tmp_MsgList, (struct TNode *) msg, tempn);
	msg->tmsg_Flags = TMSG_STATUS_SENT | TMSGF_QUEUED;
	THALUnlock(hal, &port->tmp_Lock);

	THALSignal(hal, &port->tmp_SigTask->tsk_Thread, port->tmp_Signal);
	
	for (;;)
	{
		THALWait(exec->texb_HALBase, task->tsk_SyncPort.tmp_Signal);
		reply = exec_getmsg(exec, msg->tmsg_RPort);
		if (reply)
		{
			TUINT status = msg->tmsg_Flags;

			if (reply != mem)
				exec_panic(exec, "TExecSendMessage(): Message reply mismatch");

			msg->tmsg_Flags = 0;
			return status;
		}
		tdbprintf(20, "signal on syncport, no message!\n");
	}
}

/*****************************************************************************/
/*
**	exec_returnmsg(exec, mem, status)
**	Return a message to its sender, or free it, transparently
*/

LOCAL TVOID
exec_returnmsg(TEXECBASE *exec, TAPTR mem, TUINT status)
{
	struct TMessage *msg = TGetMsgPtr(mem);
	struct TMsgPort *replyport = msg->tmsg_RPort;
	if (replyport)
	{
		TAPTR hal = exec->texb_HALBase;
		struct TNode *tempn;

		THALLock(hal, &replyport->tmp_Lock);
		TADDTAIL(&replyport->tmp_MsgList, (struct TNode *) msg, tempn);
		msg->tmsg_Flags = status;
		THALUnlock(hal, &replyport->tmp_Lock);

		THALSignal(hal, &replyport->tmp_SigTask->tsk_Thread,
			replyport->tmp_Signal);
	}
	else
	{
		exec_Free(exec, mem);	/* free one-way msg transparently */
		tdbprintf(2, "message returned to memory manager\n");
	}
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: exec_mod.c,v $
**	Revision 1.13  2005/09/13 02:41:58  tmueller
**	updated copyright reference
**	
**	Revision 1.12  2005/09/07 23:56:29  tmueller
**	updated I/O calls, extended module interface
**	
**	Revision 1.11  2005/01/29 22:25:36  tmueller
**	added alignment scheme to TMemNode and TMMUInfo
**	
**	Revision 1.10  2004/07/05 21:50:02  tmueller
**	sendmsg reply mismatch now invokes exec_panic
**	
**	Revision 1.9  2004/04/18 14:08:52  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.8  2004/04/02 23:56:50  tmueller
**	setting Msg->status within the lock eliminates race cond. in exec_sendmsg()
**	
**	Revision 1.7  2004/03/29 02:49:12  tmueller
**	Minor fixes that allow the exec module to compile with strict warning flags
**	
**	Revision 1.6  2004/01/15 22:59:57  tmueller
**	The last change corrected problems regarding I/O requests, but it
**	introduced a new race condition between getmsg/putmsg. Fixed.
**	
**	Revision 1.5  2004/01/15 02:57:07  tmueller
**	Fixed some of the more obvious inconsistencies in the lifecycle of the
**	message->status field, but it's unsure if this caught all problems. An
**	inconsistency is still pending at TExecGetMsg().
**	
**	Revision 1.4  2004/01/09 18:42:56  tmueller
**	Fixed race condition in message processing which affected TExecWaitIO
**	
**	Revision 1.3  2003/12/22 23:00:40  tmueller
**	Added private function TExecSetMsgReplyPort()
**	
**	Revision 1.2  2003/12/12 03:44:40  tmueller
**	Sanity check added to messageport destructor
**	
**	Revision 1.1.1.1  2003/12/11 07:19:02  tmueller
**	Krypton import
**	
*/
