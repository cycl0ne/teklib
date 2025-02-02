
/*
**	$Id: exec_api.c,v 1.10 2005/09/13 02:41:58 tmueller Exp $
**	teklib/mods/exec/exec_api.c - Exec module API implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "exec_mod.h"
#include <tek/debug.h>
#include <tek/teklib.h>

/*****************************************************************************/
/*
**	task = exec_createsystask(exec, func, tags)
**	Create TEKlib-internal task
*/

static TCALLBACK TVOID 
exec_systaskdestroy(struct TTask *task)
{
	TEXECBASE *exec = TGetExecBase(task);
	TAPTR hal = exec->texb_HALBase;

	THALDestroyThread(hal, &task->tsk_Thread);
	THALLock(hal, &exec->texb_Lock);
	TREMOVE((struct TNode *) task);
	THALUnlock(hal, &exec->texb_Lock);
	TDESTROY(&task->tsk_SyncPort);
	TDESTROY(&task->tsk_UserPort);
	TDESTROY(&task->tsk_HeapMMU);
	THALDestroyLock(hal, &task->tsk_TaskLock);
	exec_Free(exec, task);
}

static TVOID
exec_initsystask(TEXECBASE *exec, struct TTask *task)
{
	TAPTR hal = exec->texb_HALBase;
	struct TNode *tempn;
	
	/* special initializations of reserved system tasks */
	if (exec_strequal(task->tsk_Handle.tmo_Name, (TSTRPTR) TTASKNAME_EXEC))
	{
		/* insert to execbase */
		exec->texb_ExecTask = task;
		/* the exectask userport will be used for task messages */
		exec->texb_ExecPort = &task->tsk_UserPort;
		/* the exectask syncport will be 'abused' for async mod replies */
		exec->texb_ModReply = &task->tsk_SyncPort;
	}
	else if (exec_strequal(task->tsk_Handle.tmo_Name, 
		(TSTRPTR) TTASKNAME_RAMLIB))
	{
		/* insert to execbase */
		exec->texb_IOTask = task;
	}

	/* link to task list */
	THALLock(hal, &exec->texb_Lock);
	TADDTAIL(&exec->texb_TaskList, (struct TNode *) task, tempn);
	THALUnlock(hal, &exec->texb_Lock);

	tdbprintf1(2,"added task name: %s\n", task->tsk_Handle.tmo_Name);
}

EXPORT struct TTask *
exec_CreateSysTask(TEXECBASE *exec, TTASKFUNC func, struct TTagItem *tags)
{
	TAPTR hal = exec->texb_HALBase;
	struct TTask *newtask;
	TUINT tnlen = 0;
	TSTRPTR tname;

	tname = (TSTRPTR) TGetTag(tags, TTask_Name, TNULL);
	if (tname)
	{
		TSTRPTR t = tname;
		while (*t++);
		tnlen = t - tname;
	}
		
	/* tasks are messages */
	newtask = exec_AllocMMU(exec, &exec->texb_MsgMMU,
		sizeof(struct TTask) + tnlen);
	if (newtask)
	{
		exec_FillMem(exec, newtask, sizeof(struct TTask), 0);
		if (THALInitLock(hal, &newtask->tsk_TaskLock))
		{
			if (exec_initmmu(exec, &newtask->tsk_HeapMMU, TNULL,
				TMMUT_Tracking | TMMUT_TaskSafe, TNULL))
			{
				if (exec_initport(exec, &newtask->tsk_UserPort, newtask,
					TTASK_SIG_USER))
				{
					if (exec_initport(exec, &newtask->tsk_SyncPort, newtask,
						TTASK_SIG_SINGLE))
					{
						if (tname)
						{
							TSTRPTR t = (TSTRPTR) (newtask + 1);
							newtask->tsk_Handle.tmo_Name = t;
							while ((*t++ = *tname++));
						}
	
						newtask->tsk_Handle.tmo_DestroyFunc =
							(TDFUNC) exec_systaskdestroy;
						newtask->tsk_Handle.tmo_ModBase = exec;
						newtask->tsk_UserData =
							(TAPTR) TGetTag(tags, TTask_UserData, TNULL);
						newtask->tsk_SigFree = ~((TUINT) TTASK_SIG_RESERVED);
						newtask->tsk_SigUsed = TTASK_SIG_RESERVED;
						newtask->tsk_Status = TTASK_RUNNING;
	
						/* create thread */
						if (THALInitThread(hal, &newtask->tsk_Thread, 
							(TTASKENTRY TVOID (*)(TAPTR data)) func, newtask))
						{
							exec_initsystask(exec, newtask);
							if (func)
							{
								THALSignal(hal, &newtask->tsk_Thread,
									TTASK_SIG_SINGLE);
							}
							return newtask;
						}
	
						TDESTROY(&newtask->tsk_SyncPort);
					}
					TDESTROY(&newtask->tsk_UserPort);
				}
				TDESTROY(&newtask->tsk_HeapMMU);
			}
			THALDestroyLock(hal, &newtask->tsk_TaskLock);
		}
		exec_Free(exec, newtask);
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	sigs = exec_AllocSignal(execbase, signals)
**	Alloc signal(s) from a task
*/

EXPORT TUINT
exec_AllocSignal(TEXECBASE *exec, TUINT signals)
{
	struct TTask *task = THALFindSelf(exec->texb_HALBase);
	return exec_allocsignal(exec, task, signals);
}

/*****************************************************************************/
/*
**	exec_FreeSignal(execbase, signals)
**	Return signal(s) to a task
*/

EXPORT TVOID
exec_FreeSignal(TEXECBASE *exec, TUINT signals)
{
	struct TTask *task = THALFindSelf(exec->texb_HALBase);
	exec_freesignal(exec, task, signals);
}

/*****************************************************************************/
/*
**	oldsignals = exec_SetSignal(exec, newsignals, sigmask)
**	Set/get task's signal state
*/

EXPORT TUINT
exec_SetSignal(TEXECBASE *exec, TUINT newsignals, TUINT sigmask)
{
	return THALSetSignal(exec->texb_HALBase, newsignals, sigmask);
}

/*****************************************************************************/
/*
**	exec_Signal(exec, task, signals)
**	Send signals to a task. An event is thrown when not all of the
**	affecting signals are already present in the task's signal mask.
*/

EXPORT TVOID
exec_Signal(TEXECBASE *exec, struct TTask *task, TUINT signals)
{
	if (task) THALSignal(exec->texb_HALBase, &task->tsk_Thread, signals);
}

/*****************************************************************************/
/*
**	signals = exec_Wait(exec, sigmask)
**	Suspend task to wait for a set of signals
*/

EXPORT TUINT
exec_Wait(TEXECBASE *exec, TUINT sig)
{
	if (sig) sig = THALWait(exec->texb_HALBase, sig);
	return sig;
}

/*****************************************************************************/
/*
**	mmu = exec_GetTaskMMU(exec, task)
**	Get task's heap memory manager. Allocations made from this
**	memory manager are automatically freed on exit of this task
*/

EXPORT TAPTR
exec_GetTaskMMU(TEXECBASE *exec, struct TTask *task)
{
	if (task == TNULL) task = THALFindSelf(exec->texb_HALBase);
	return &task->tsk_HeapMMU;
}

/*****************************************************************************/
/*
**	port = exec_GetUserPort(exec, task)
**	Get task's user port
*/

EXPORT TAPTR
exec_GetUserPort(TEXECBASE *exec, struct TTask *task)
{
	if (task == TNULL) task = THALFindSelf(exec->texb_HALBase);
	return &task->tsk_UserPort;
}

/*****************************************************************************/
/*
**	port = exec_GetSyncPort(exec, task)
**	Get task's sync port - the one being restricted to synchronized
**	message passing. Functions like TDoIO() and TSendMsg() are using it.
*/

EXPORT TAPTR
exec_GetSyncPort(TEXECBASE *exec, struct TTask *task)
{
	if (task == TNULL) task = THALFindSelf(exec->texb_HALBase);
	return &task->tsk_SyncPort;
}

/*****************************************************************************/
/*
**	userdata = exec_GetTaskData(exec, task)
**	Get a task's userdata pointer
*/

EXPORT TAPTR
exec_GetTaskData(TEXECBASE *exec, struct TTask *task)
{
	if (task == TNULL) task = THALFindSelf(exec->texb_HALBase);
	return task->tsk_UserData;
}

/*****************************************************************************/
/*
**	olddata = exec_SetTaskData(exec, task, data)
**	Set task's userdata pointer
*/

EXPORT TAPTR
exec_SetTaskData(TEXECBASE *exec, struct TTask *task, TAPTR data)
{
	TAPTR hal = exec->texb_HALBase;
	TAPTR olddata;
	if (task == TNULL) task = THALFindSelf(hal);
	THALLock(hal, &task->tsk_TaskLock);
	olddata = task->tsk_UserData;
	task->tsk_UserData = data;
	THALUnlock(hal, &task->tsk_TaskLock);
	return olddata;
}

/*****************************************************************************/
/*
**	halbase = exec_GetHALBase(exec)
**	Get HAL module base pointer. As the HAL module interface is
**	currently a moving target and requires profound understanding of
**	the inner working, this should be considered a private function.
*/

EXPORT TAPTR
exec_GetHALBase(TEXECBASE *exec)
{
	return exec->texb_HALBase;
}

/*****************************************************************************/
/*
**	mem = exec_AllocMsg(exec, size)
**	Allocate message memory
*/

EXPORT TAPTR
exec_AllocMsg(TEXECBASE *exec, TUINT size)
{
	return exec_AllocMMU(exec, &exec->texb_MsgMMU, size);
}

/*****************************************************************************/
/*
**	mem = exec_AllocMsg0(exec, task, size)
**	Allocate message memory, blank
*/

EXPORT TAPTR
exec_AllocMsg0(TEXECBASE *exec, TUINT size)
{
	TAPTR mem = exec_AllocMMU(exec, &exec->texb_MsgMMU, size);
	if (mem) exec_FillMem(exec, mem, size, 0);
	return mem;
}

/*****************************************************************************/
/*
**	sig = exec_GetPortSignal(exec, port)
**	Get a message port's underlying signal
*/

EXPORT TUINT
exec_GetPortSignal(TEXECBASE *exec, TAPTR port)
{
	if (port) return ((struct TMsgPort *) port)->tmp_Signal;
	return 0;
}

/*****************************************************************************/
/*
**	data = exec_GetAtomData(exec, atom)
**	Get an atom's data pointer
*/

EXPORT TTAG
exec_GetAtomData(TEXECBASE *exec, struct TAtom *atom)
{
	if (atom) return atom->tatm_Data;
	return TNULL;
}

/*****************************************************************************/
/*
**	olddata = exec_SetAtomData(exec, atom, data)
**	Set an atom's data pointer
*/

EXPORT TTAG
exec_SetAtomData(TEXECBASE *exec, struct TAtom *atom, TTAG data)
{
	TTAG olddata = atom->tatm_Data;
	atom->tatm_Data = data;
	return olddata;
}

/*****************************************************************************/
/*
**	exec_PutIO(exec, ioreq)
**	Put an I/O request to an I/O device or handler,
**	preferring asynchronous operation.
*/

EXPORT TVOID
exec_PutIO(TEXECBASE *exec, struct TIORequest *ioreq)
{
	ioreq->io_Flags &= ~TIOF_QUICK;
	TGetMsgPtr(ioreq)->tmsg_Flags = 0;
	TBeginIO(ioreq->io_Device, ioreq);
}

/*****************************************************************************/
/*
**	err = exec_WaitIO(exec, ioreq)
**	Wait for an I/O request to complete. If the request was
**	processed synchronously, drop through immediately.
*/

EXPORT TINT
exec_WaitIO(TEXECBASE *exec, struct TIORequest *ioreq)
{
	TAPTR hal = exec->texb_HALBase;
	struct TMessage *msg = TGetMsgPtr(ioreq);
	TUINT status = msg->tmsg_Flags;

	if (status & TMSGF_SENT)	/* i.e. not processed quickly */
	{
		struct TMsgPort *replyport = ioreq->io_ReplyPort;

		while (status != (TMSG_STATUS_REPLIED | TMSGF_QUEUED))
		{
			THALWait(hal, replyport->tmp_Signal);
			status = msg->tmsg_Flags;
		}

		THALLock(hal, &replyport->tmp_Lock);
		TREMOVE((struct TNode *) msg);
		THALUnlock(hal, &replyport->tmp_Lock);

		msg->tmsg_Flags = 0;
	}

	return (TINT) ioreq->io_Error;
}

/*****************************************************************************/
/*
**	err = exec_DoIO(exec, ioreq)
**	Send an I/O request and wait for completion. Sets the TIOF_QUICK flag
**	for synchronous operation, if supported by the device.
*/

EXPORT TINT
exec_DoIO(TEXECBASE *exec, struct TIORequest *ioreq)
{
	TAPTR replyport = ioreq->io_ReplyPort;

	if (replyport == TNULL)
	{
		/* temporarily use task's syncport */
		struct TTask *task = THALFindSelf(exec->texb_HALBase);
		ioreq->io_ReplyPort = &task->tsk_SyncPort;
	}

	ioreq->io_Flags |= TIOF_QUICK;
	TGetMsgPtr(ioreq)->tmsg_Flags = 0;

	TBeginIO(ioreq->io_Device, ioreq);
	exec_WaitIO(exec, ioreq);

	if (replyport == TNULL)
	{
		/* restore task's syncport */
		ioreq->io_ReplyPort = TNULL;
		/* clear potentially pending sync signal */
		THALSetSignal(exec->texb_HALBase, 0, TTASK_SIG_SINGLE);
	}

	return (TINT) ioreq->io_Error;
}

/*****************************************************************************/
/*
**	complete = exec_CheckIO(exec, ioreq)
**	Test if an I/O request is ready
*/

EXPORT TBOOL
exec_CheckIO(TEXECBASE *exec, struct TIORequest *ioreq)
{
	struct TMessage *msg = TGetMsgPtr(ioreq);
	TUINT status = msg->tmsg_Flags;

	return (TBOOL) (!(status & TMSGF_SENT) ||
		(status == (TMSG_STATUS_REPLIED | TMSGF_QUEUED)));
}

/*****************************************************************************/
/*
**	error = exec_AbortIO(exec, ioreq)
**	Attempt to abort an I/O request
*/

EXPORT TINT
exec_AbortIO(TEXECBASE *exec, struct TIORequest *ioreq)
{
	return TAbortIO(ioreq->io_Device, ioreq);
}

/*****************************************************************************/
/*
**	lock = exec_CreateLock(exec, tags)
**	Create locking object.
*/

static TCALLBACK TVOID 
destroyfreelock(struct TLock *lock)
{
	TEXECBASE *exec = TGetExecBase(lock);
	THALDestroyLock(exec->texb_HALBase, &lock->tlk_HLock);
	exec_Free(exec, lock);
}

EXPORT TAPTR
exec_CreateLock(TEXECBASE *exec, TTAGITEM *tags)
{
	struct TLock *lock;
	lock = exec_AllocMMU(exec, TNULL, sizeof(struct TLock));
	if (lock)
	{
		if (exec_initlock(exec, lock))
		{
			lock->tlk_Handle.tmo_DestroyFunc = (TDFUNC) destroyfreelock;
			return (TAPTR) lock;
		}
		exec_Free(exec, lock);
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	exec_Lock(exec, lock)
*/

EXPORT TVOID
exec_Lock(TEXECBASE *exec, struct TLock *lock)
{
	TAPTR hal = exec->texb_HALBase;
	struct TTask *waiter = THALFindSelf(hal); 
	struct TLockWait request;
	request.tlr_Task = waiter;

	THALLock(hal, &lock->tlk_HLock);

	if (lock->tlk_Owner == TNULL)
	{
		lock->tlk_Owner = waiter;
		lock->tlk_NestCount++;
		THALUnlock(hal, &lock->tlk_HLock);
	}
	else if (lock->tlk_Owner == waiter)
	{
		lock->tlk_NestCount++;
		THALUnlock(hal, &lock->tlk_HLock);
	}
	else
	{
		struct TNode *tempn;
		TADDTAIL(&lock->tlk_Waiters, &request.tlr_Node, tempn);
		lock->tlk_WaitCount++;
		THALUnlock(hal, &lock->tlk_HLock);

		THALWait(hal, TTASK_SIG_SINGLE);
	}
}

/*****************************************************************************/
/*
**	exec_Unlock(exec, lock)
*/

EXPORT TVOID
exec_Unlock(TEXECBASE *exec, struct TLock *lock)
{
	TAPTR hal = exec->texb_HALBase;

	THALLock(hal, &lock->tlk_HLock);

	lock->tlk_NestCount--;

	if (lock->tlk_NestCount == 0)
	{
		if (lock->tlk_WaitCount > 0)
		{
			struct TNode *tempn;
			struct TLockWait *request =
				(struct TLockWait *) TREMHEAD(&lock->tlk_Waiters, tempn);
			lock->tlk_WaitCount--;
			lock->tlk_Owner = request->tlr_Task;
			lock->tlk_NestCount = 1;
			THALSignal(hal, &request->tlr_Task->tsk_Thread, TTASK_SIG_SINGLE);
		}
		else
		{
			lock->tlk_Owner = TNULL;
		}
	}
	
	THALUnlock(hal, &lock->tlk_HLock);
}

/*****************************************************************************/
/*
**	port = exec_CreatePort(execbase, tags)
**	Create a message port
*/

static TCALLBACK TVOID
destroyuserport(struct TMsgPort *port)
{
	TEXECBASE *exec = TGetExecBase(port);
	exec_freesignal(exec, port->tmp_SigTask, port->tmp_Signal);
	THALDestroyLock(exec->texb_HALBase, &port->tmp_Lock);
	exec_Free(exec, port);
}

EXPORT struct TMsgPort *
exec_CreatePort(TEXECBASE *exec, struct TTagItem *tags)
{
	struct TTask *task = THALFindSelf(exec->texb_HALBase);
	struct TMsgPort *port = 
		exec_AllocMMU(exec, &task->tsk_HeapMMU, sizeof(struct TMsgPort));
	if (port)
	{
		if (exec_initport(exec, port, task, 0))
		{
			/* overwrite destructor */
			port->tmp_Handle.tmo_DestroyFunc = (TDFUNC) destroyuserport;
			return port;
		}
		exec_Free(exec, port);
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	msg = exec_WaitPort(exec, port)
**	Wait for a message port to be non-empty
*/

EXPORT TAPTR
exec_WaitPort(TEXECBASE *exec, struct TMsgPort *port)
{
	struct TNode *node = TNULL;
	if (port)
	{
		TAPTR hal = exec->texb_HALBase;
		
		#ifdef TDEBUG
		if (THALFindSelf(hal) != port->tmp_SigTask)
		{
			exec_panic(exec, (TSTRPTR) "Caller must be owner of the port\n");
		}
		#endif
		
		for (;;)
		{
			THALLock(hal, &port->tmp_Lock);
			node = port->tmp_MsgList.tlh_Head;
			if (node->tln_Succ == TNULL) node = TNULL;
			THALUnlock(hal, &port->tmp_Lock);
			if (node) break;
			THALWait(hal, port->tmp_Signal);
		}
	} else tdbprintf(10,"port=TNULL\n");
	return (TAPTR) node;
}

/*****************************************************************************/
/*
**	msg = exec_GetMsg(exec, port)
**	Get next pending message from message port
*/

EXPORT TAPTR
exec_GetMsg(TEXECBASE *exec, struct TMsgPort *port)
{
	if (port) return exec_getmsg(exec, port);
	tdbprintf(10,"port=TNULL\n");
	return TNULL;
}

/*****************************************************************************/
/*
**	exec_PutMsg(exec, port, replyport, msg)
**	Put msg to a message port, with a reply expected at replyport
*/

EXPORT TVOID
exec_PutMsg(TEXECBASE *exec, struct TMsgPort *port, struct TMsgPort *replyport,
	TAPTR mem)
{
	if (port && mem)
	{
		struct TNode *tempn;
		TAPTR hal = exec->texb_HALBase;
		struct TMessage *msg = TGetMsgPtr(mem);

		msg->tmsg_RPort = replyport;
		msg->tmsg_Sender = TNULL;	/* sender is local address space */

		THALLock(hal, &port->tmp_Lock);
		TADDTAIL(&port->tmp_MsgList, (struct TNode *) msg, tempn);
		msg->tmsg_Flags = TMSGF_SENT | TMSGF_QUEUED;
		THALUnlock(hal, &port->tmp_Lock);

		THALSignal(hal, &port->tmp_SigTask->tsk_Thread, port->tmp_Signal);
	}
	else
	{
		tdbprintf(10,"port/msg=TNULL\n");
	}
}

/*****************************************************************************/
/*
**	exec_AckMsg(execbase, msg)
**	Acknowledge a message to its sender. Unlike TReplyMsg(),
**	this function indicates that the message body is unmodified.
*/

EXPORT TVOID
exec_AckMsg(TEXECBASE *exec, TAPTR mem)
{
	if (mem)
	{
		exec_returnmsg(exec, mem, TMSG_STATUS_ACKD | TMSGF_QUEUED);
	}
	else
	{
		tdbprintf(10,"msg=TNULL\n");
	}
}

/*****************************************************************************/
/*
**	exec_ReplyMsg(exec, msg)
**	Reply a message to its sender. This includes the semantic that the
**	message body has or may have been modified.
*/

EXPORT TVOID
exec_ReplyMsg(TEXECBASE *exec, TAPTR mem)
{
	if (mem)
	{
		exec_returnmsg(exec, mem, TMSG_STATUS_REPLIED | TMSGF_QUEUED);
	}
	else
	{
		tdbprintf(10,"msg=TNULL\n");
	}
}

/*****************************************************************************/
/*
**	exec_DropMsg(exec, msg)
**	Drop a message, i.e. make it fail in the sender's context. This is
**	an important semantic for proxied messageports. In local address space,
**	it's particularly useful for synchronized messages sent with TSendMsg().
*/

EXPORT TVOID
exec_DropMsg(TEXECBASE *exec, TAPTR mem)
{
	if (mem)
	{
		exec_returnmsg(exec, mem, TMSG_STATUS_FAILED | TMSGF_QUEUED);
	}
	else
	{
		tdbprintf(10,"msg=TNULL\n");
	}
}

/*****************************************************************************/
/*
**	success = exec_SendMsg(exec, port, msg)
**	Send message, two-way, blocking
*/

EXPORT TUINT
exec_SendMsg(TEXECBASE *exec, struct TMsgPort *port, TAPTR msg)
{
	if (port && msg)
	{
		struct TTask *task = THALFindSelf(exec->texb_HALBase);
		return exec_sendmsg(exec, task, port, msg);
	}
	tdbprintf(10,"port/msg=TNULL\n");
	return TMSG_STATUS_FAILED;
}

/*****************************************************************************/
/*
**	task = exec_FindTask(exec, name)
**	Find a task by name, or the caller's own task, if name is TNULL
*/

EXPORT struct TTask *
exec_FindTask(TEXECBASE *exec, TSTRPTR name)
{
	TAPTR hal = exec->texb_HALBase;
	struct TTask *task;
	if (name == TNULL)
	{
		task = THALFindSelf(hal);
	}
	else
	{
		THALLock(hal, &exec->texb_Lock);
		task = (struct TTask *) TFindHandle(&exec->texb_TaskList, name);
		THALUnlock(hal, &exec->texb_Lock);
	}
	return task;
}

/*****************************************************************************/
/*
**	mod = exec_OpenModule(exec, name, version, tags)
**	Open a module and/or an module instance
*/

EXPORT TAPTR
exec_OpenModule(TEXECBASE *exec, TSTRPTR modname, TUINT16 version, 
	struct TTagItem *tags)
{
	if (modname)
	{
		TTAGITEM extratags[2];
		struct TTask *task = THALFindSelf(exec->texb_HALBase);
		union TTaskRequest *taskreq;

		/* called in module init? this is not allowed */
		if (task == exec->texb_IOTask) return TNULL;

		/* Shortcut: provide access to a 'timer.device', which is actually
		** the hal module. A ptr to the Execbase must be supplied as well */

		if (exec_strequal((TSTRPTR) TMODNAME_TIMER, modname))
		{
			modname = (TSTRPTR) TMODNAME_HAL;
			extratags[0].tti_Tag = THalBase_Exec;
			extratags[0].tti_Value = (TTAG) exec;
			extratags[1].tti_Tag = TTAG_MORE;
			extratags[1].tti_Value = (TTAG) tags;
			tags = extratags;
		}
		
		/* insert module request */
		task->tsk_ReqCode = TTREQ_OPENMOD;
		taskreq = &task->tsk_Request;
		taskreq->trq_Mod.trm_InitMod.tmd_Name = modname;
		taskreq->trq_Mod.trm_InitMod.tmd_Version = version;
		taskreq->trq_Mod.trm_Module = TNULL;
	
		/* note that the task itself is the message, not taskreq */
		if (exec_sendmsg(exec, task, exec->texb_ExecPort, task))
		{
			struct TModule *mod = taskreq->trq_Mod.trm_Module;
			struct TModule *openmod;
			
			if (mod->tmd_OpenFunc == TNULL)
			{
				return mod;		/* no instance, succeed */
			}
			
			openmod = (*mod->tmd_OpenFunc)(mod, task, tags);
			if (openmod)
			{
				return openmod;	/* instance open succeeded */
			}
	
			/* failed - send back module.
			** note that taskreq->mod needs to be re-initialized, because
			** it could have been used and overwritten in the meantime */

			task->tsk_ReqCode = TTREQ_CLOSEMOD;	
			taskreq->trq_Mod.trm_Module = mod;
			exec_sendmsg(exec, task, exec->texb_ExecPort, task);
		}
	}
	else
	{
		tdbprintf(10,"modname=TNULL\n");
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	exec_CloseModule(exec, module)
**	Close module
*/

EXPORT TVOID
exec_CloseModule(TEXECBASE *exec, struct TModule *modinst)
{
	if (modinst)
	{
		struct TTask *task = THALFindSelf(exec->texb_HALBase);
		union TTaskRequest *taskreq = &task->tsk_Request;
		struct TModule *modsuper = modinst->tmd_ModSuper;
	
		if (modinst->tmd_CloseFunc)
		{
			/* this is a module instance. close first. */
			(*modinst->tmd_CloseFunc)(modinst, task);
		}
	
		task->tsk_ReqCode = TTREQ_CLOSEMOD;
		taskreq->trq_Mod.trm_Module = modsuper;
		exec_sendmsg(exec, task, exec->texb_ExecPort, task);
	}
}	

/*****************************************************************************/
/*
**	task = exec_CreateTask(exec, parenttask, function, initfunction, tags)
**	Create task
*/

EXPORT struct TTask *
exec_CreateTask(TEXECBASE *exec, TTASKFUNC func, TINITFUNC initfunc,
	struct TTagItem *tags)
{
	struct TTask *newtask = TNULL;

	if (func || initfunc)
	{
		struct TTagItem tasktags[3];
		union TTaskRequest *taskreq;
		struct TTask *task = THALFindSelf(exec->texb_HALBase);
		
		tasktags[0].tti_Tag = TTAG_GOSUB;
		tasktags[0].tti_Value = (TTAG) tags;	/* prefer user tags */
		tasktags[1].tti_Tag = TTask_CurrentDir;	/* override, if not present */
		/* by default, inherit currentdir from current */
		tasktags[1].tti_Value = (TTAG) task->tsk_CurrentDir;
		tasktags[2].tti_Tag = TTAG_DONE;

		task->tsk_ReqCode = TTREQ_CREATETASK;
		taskreq = &task->tsk_Request;
		taskreq->trq_Task.trt_Func = func;
		taskreq->trq_Task.trt_InitFunc = initfunc;
		taskreq->trq_Task.trt_Tags = tasktags;

		if (exec_sendmsg(exec, task, exec->texb_ExecPort, task))
		{
			newtask = taskreq->trq_Task.trt_Task;
		}
	}
	else
	{
		tdbprintf(10,"func/initfunc missing\n");
	}
	
	return newtask;
}

/*****************************************************************************/
/*
**	atom = exec_LockAtom(exec, data, mode)
**	Lock named atom
*/

EXPORT struct TAtom *
exec_LockAtom(TEXECBASE *exec, TAPTR atom, TUINT mode)
{
	if (atom)
	{
		struct TTask *task = THALFindSelf(exec->texb_HALBase);

		task->tsk_ReqCode = TTREQ_LOCKATOM;		
		task->tsk_Request.trq_Atom.tra_Atom = atom;
		task->tsk_Request.trq_Atom.tra_Task = task;
		task->tsk_Request.trq_Atom.tra_Mode = mode;
	
		if (exec_sendmsg(exec, task, exec->texb_ExecPort, task))
		{
			atom = task->tsk_Request.trq_Atom.tra_Atom;
		}
		
		if (atom && (mode & TATOMF_DESTROY))
		{
			exec_UnlockAtom(exec, atom, TATOMF_DESTROY);
		}
	}
	return atom;
}

/*****************************************************************************/
/*
**	exec_UnlockAtom(exec, atom, mode)
**	Unlock an atom
*/

EXPORT TVOID
exec_UnlockAtom(TEXECBASE *exec, struct TAtom *atom, TUINT mode)
{
	if (atom)
	{
		struct TTask *task = THALFindSelf(exec->texb_HALBase);

		task->tsk_ReqCode = TTREQ_UNLOCKATOM;
		task->tsk_Request.trq_Atom.tra_Atom = atom;
		task->tsk_Request.trq_Atom.tra_Task = task;
		task->tsk_Request.trq_Atom.tra_Mode = mode | TATOMF_UNLOCK;
		
		exec_sendmsg(exec, task, exec->texb_ExecPort, task);
	}
}

/*****************************************************************************/
/*
**	list = exec_LockPort(exec, port)
**	Lock a message port and return a ptr to its queue. Currently this is
**	a private function. The caller is allowed to hold the lock only for a
**	very short time.
*/

EXPORT struct TList *
exec_LockPort(TEXECBASE *exec, struct TMsgPort *port)
{
	THALLock(exec->texb_HALBase, &port->tmp_Lock);
	return &port->tmp_MsgList;
}

/*****************************************************************************/
/*
**	exec_UnlockPort(exec, port)
**	Unlock message port
*/

EXPORT TVOID
exec_UnlockPort(TEXECBASE *exec, struct TMsgPort *port)
{
	THALUnlock(exec->texb_HALBase, &port->tmp_Lock);	
}

/*****************************************************************************/
/*
**	exec_InsertMsg(exec, port, msg, predmsg, status)
**	Insert msg after predmsg in a msgport's queue and
**	set its status
*/

EXPORT TVOID
exec_InsertMsg(TEXECBASE *exec, struct TMsgPort *port, TAPTR mem,
	TAPTR predmem, TUINT status)
{
	struct TMessage *msg = TGetMsgPtr(mem);
	struct TMessage *predmsg = predmem ? TGetMsgPtr(predmem) : TNULL;

	THALLock(exec->texb_HALBase, &port->tmp_Lock);
	if (predmsg)
	{
		TInsert(&port->tmp_MsgList, &msg->tmsg_Node, &predmsg->tmsg_Node);
	}
	else
	{
		TAddTail(&port->tmp_MsgList, &msg->tmsg_Node);
	}

	THALUnlock(exec->texb_HALBase, &port->tmp_Lock);

	msg->tmsg_Flags = status | TMSGF_QUEUED;
}

/*****************************************************************************/
/*
**	exec_RemoveMsg(exec, port, msg)
**	Remove msg from message port
*/

EXPORT TVOID
exec_RemoveMsg(TEXECBASE *exec, struct TMsgPort *port, TAPTR mem)
{
	struct TMessage *msg = TGetMsgPtr(mem);
	THALLock(exec->texb_HALBase, &port->tmp_Lock);
	TREMOVE(&msg->tmsg_Node);
	THALUnlock(exec->texb_HALBase, &port->tmp_Lock);
}

/*****************************************************************************/
/*
**	status = exec_GetMsgStatus(exec, msg)
**	Get message status
*/

EXPORT TUINT
exec_GetMsgStatus(TEXECBASE *exec, TAPTR mem)
{
	return TGetMsgPtr(mem)->tmsg_Flags;
}

/*****************************************************************************/
/*
**	oldrport = exec_SetMsgReplyPort(exec, msg, rport)
**	Set a message's replyport
*/

EXPORT struct TMsgPort *
exec_SetMsgReplyPort(TEXECBASE *exec, TAPTR mem, struct TMsgPort *rport)
{
	struct TMessage *msg = TGetMsgPtr(mem);
	struct TMsgPort *oport;
	
	oport = msg->tmsg_RPort;
	msg->tmsg_RPort = rport;
	
	return oport;
}


#if 0

/*****************************************************************************/
/*
**	success = exec_AddModule(exec, module, tags)
**	Add module to internal module list
*/

EXPORT TBOOL
exec_AddModule(TEXECBASE *exec, struct TModule *module, TTAGITEM *tags)
{
	if (module)
	{
		TAPTR hal = exec->texb_HALBase;
		module->tmd_Flags |= TMODF_INITIALIZED;
		THALLock(hal, &exec->texb_Lock);
		TAddTail(&exec->texb_ModList, (struct TNode *) module);
		THALUnlock(hal, &exec->texb_Lock);
		return TTRUE;
	}
	return TFALSE;
}

#endif

/*****************************************************************************/
/*
**	Revision History
**	$Log: exec_api.c,v $
**	Revision 1.10  2005/09/13 02:41:58  tmueller
**	updated copyright reference
**	
**	Revision 1.9  2004/06/11 17:47:09  tmueller
**	TExecLockAtom() with TATOMF_DESTROY | TATOMF_TRY is now valid.
**	
**	Revision 1.8  2004/04/18 14:08:52  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
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
**	Revision 1.4  2004/01/13 02:16:44  tmueller
**	TExecWaitIO() resets the message status to zero after unlinking a request
**	from messageport. Previously only the QUEUED flag was removed. TExecCheckIO
**	does no longer complain when a message was not sent at all.
**	
**	Revision 1.3  2003/12/22 22:58:51  tmueller
**	A few more object==TNULL cases are now treated more gently
**	
**	Revision 1.2  2003/12/12 03:42:09  tmueller
**	Sanity check added in TExecWaitPort
**	
**	Revision 1.1.1.1  2003/12/11 07:19:00  tmueller
**	Krypton import
**	
*/
