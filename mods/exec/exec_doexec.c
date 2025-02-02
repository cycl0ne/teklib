
/*
**	$Id: exec_doexec.c,v 1.11 2005/09/13 02:41:58 tmueller Exp $
**	teklib/mods/exec/exec_doexec.c - Exec task contexts
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/io.h>
#include "exec_mod.h"

static TVOID exec_requestmod(TEXECBASE *exec, struct TTask *taskmsg);
static TVOID exec_closemod(TEXECBASE *exec, struct TTask *taskmsg);
static TVOID exec_modreply(TEXECBASE *exec, struct TTask *taskmsg);
static TVOID exec_main(TEXECBASE *exec, struct TTask *task,
	struct TTagItem *tags);
static TVOID exec_ramlib(TEXECBASE *exec, struct TTask *task,
	struct TTagItem *tags);
static TVOID exec_loadmod(TEXECBASE *exec, struct TTask *task,
	struct TTask *taskmsg);
static TVOID exec_unloadmod(TEXECBASE *exec, struct TTask *task,
	struct TTask *taskmsg);
static TVOID exec_newtask(TEXECBASE *exec, struct TTask *taskmsg);
static struct TTask *exec_createtask(TEXECBASE *exec, TTASKFUNC func,
	TINITFUNC initfunc, struct TTagItem *tags);
static TTASKENTRY TVOID exec_taskentryfunc(struct TTask *task);
static TVOID exec_childinit(TEXECBASE *exec);
static TVOID exec_childexit(TEXECBASE *exec);
static TVOID exec_lockatom(TEXECBASE *exec, struct TTask *msg);
static TVOID exec_unlockatom(TEXECBASE *exec, struct TTask *msg);

/*****************************************************************************/
/*
**	success = TDoExec(exec, tags)
**	entrypoint to exec internal services and initializations
*/

EXPORT TBOOL
exec_DoExec(TEXECBASE *exec, struct TTagItem *tags)
{
	struct TTask *task = exec_FindTask(exec, TNULL);

	if (task == exec_FindTask(exec, (TSTRPTR) TTASKNAME_EXEC))
	{
		/* perform execbase context */
		exec_main(exec, task, tags);
		return TTRUE;
	}
	else if (task == exec_FindTask(exec, (TSTRPTR) TTASKNAME_RAMLIB))
	{
		/* perform ramlib context */
		exec_ramlib(exec, task, tags);
		return TTRUE;
	}
	else if (task == exec_FindTask(exec, (TSTRPTR) TTASKNAME_ENTRY))
	{
		/*
		**	Place for initializations in entry context
		*/

		return TTRUE;
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	Panic
*/

LOCAL TVOID
exec_panic(TEXECBASE *exec, TSTRPTR suggestion)
{
	tdbprintf1(20,"*** Fatal error: %s\n", suggestion);
	tdbfatal(99);
}

/*****************************************************************************/
/*
**	Ramlib task - Module loading
*/

static TVOID
exec_ramlib(TEXECBASE *exec, struct TTask *task, struct TTagItem *tags)
{
	struct TMsgPort *port = &task->tsk_UserPort;
	TUINT waitsig = TTASK_SIG_ABORT | port->tmp_Signal;

	TUINT sig;
	struct TTask *taskmsg;

	for (;;)
	{
		sig = THALWait(exec->texb_HALBase, waitsig);
		if (sig & TTASK_SIG_ABORT) break;
		
		while ((taskmsg = exec_getmsg(exec, port)))
		{
			switch (taskmsg->tsk_ReqCode)
			{
				case TTREQ_LOADMOD:
					exec_loadmod(exec, task, taskmsg);
					break;
					
				case TTREQ_UNLOADMOD:
					exec_unloadmod(exec, task, taskmsg);
					break;
			}
		}
	}
	
	{
		TINT n = 0;
		while ((taskmsg = exec_getmsg(exec, port))) n++;
		if (n > 0)
		{
			tdbprintf1(20,"Module requests pending: %d\n", n);
			exec_panic(exec, (TSTRPTR) "Applications must close all modules");
		}
	}
}

/*****************************************************************************/

static TVOID
exec_loadmod(TEXECBASE *exec, struct TTask *task, struct TTask *taskmsg)
{
	TAPTR halmod = TNULL;
	TAPTR hal = exec->texb_HALBase;
	union TTaskRequest *req = &taskmsg->tsk_Request;
	struct TInitModule *imod = exec->texb_IMods;
	TSTRPTR modname = req->trq_Mod.trm_InitMod.tmd_Name;
	TUINT nsize = 0, psize = 0;

	if (imod)
	{
		TSTRPTR tempn;
		for (; (tempn = imod->tinm_Name); imod++)
		{
			if (exec_strequal(tempn, modname))
			{
				psize = (*imod->tinm_InitFunc)(TNULL, TNULL, 
					req->trq_Mod.trm_InitMod.tmd_Version, TNULL);
	
				if (psize)
				{
					nsize = (*imod->tinm_InitFunc)(TNULL, TNULL, 0xffff,
						TNULL);
				}
	
				break;
			}
		}
	}
	
	if (psize == 0)
	{
		/* try to load module via HAL interface */
		halmod = THALLoadModule(hal, modname, 
			req->trq_Mod.trm_InitMod.tmd_Version, &psize, &nsize);
	}
	
	if (psize)
	{
		TINT nlen;
		TSTRPTR s = modname;
		TINT8 *mmem;

		while (*s++);
		nlen = s - modname;

		mmem = exec_AllocMMU(exec, TNULL, psize + nsize + nlen);
		if (mmem)
		{
			struct TModule *mod = (struct TModule *) (mmem + nsize);
			TSTRPTR d = (TSTRPTR) mmem + nsize + psize;
			
			exec_FillMem(exec, mmem, psize + nsize, 0);

			s = modname;			
			mod->tmd_Name = d;				/* insert name */
			while ((*d++ = *s++));			/* copy name */

			mod->tmd_ExecBase = exec;
			mod->tmd_ModSuper = mod;
			mod->tmd_InitTask = task;
			mod->tmd_HALMod = halmod;
			mod->tmd_PosSize = psize;
			mod->tmd_NegSize = nsize;
			mod->tmd_RefCount = 1;

			if (halmod)
			{
				if (THALCallModule(hal, halmod, task, mod))
				{
					req->trq_Mod.trm_Module = mod;
					exec_ReplyMsg(exec, taskmsg);
					return;
				}
			}
			else
			{
				if ((*imod->tinm_InitFunc)(task, mod,
					req->trq_Mod.trm_InitMod.tmd_Version, TNULL))
				{
					req->trq_Mod.trm_Module = mod;
					exec_ReplyMsg(exec, taskmsg);
					return;
				}
			}								
			
			exec_Free(exec, mmem);
		}

		if (halmod) THALUnloadModule(hal, halmod);
	}

	/* failed */
	exec_DropMsg(exec, taskmsg);
}

/*****************************************************************************/

static TVOID
exec_unloadmod(TEXECBASE *exec, struct TTask *task, struct TTask *taskmsg)
{
	TAPTR hal = exec->texb_HALBase;
	union TTaskRequest *req = &taskmsg->tsk_Request;
	struct TModule *mod = req->trq_Mod.trm_Module;

	/* invoke user destructor */
	TDESTROY(mod);

	/* unload */
	if (mod->tmd_HALMod) THALUnloadModule(hal, mod->tmd_HALMod);

	/* free */
	exec_Free(exec, (TINT8 *) mod - mod->tmd_NegSize);

	/* restore original replyport */	
	TGetMsgPtr(taskmsg)->tmsg_RPort = req->trq_Mod.trm_RPort;

	/* return to caller */
	exec_returnmsg(exec, taskmsg, TMSG_STATUS_ACKD | TMSGF_QUEUED);
}

/*****************************************************************************/
/*
**	Exec task
*/

static TVOID
exec_checkmodules(TEXECBASE *exec)
{
	TINT n = 0;
	struct TNode *nnode, *node = exec->texb_ModList.tlh_Head;
	for (; (nnode = node->tln_Succ); node = nnode)
	{
		struct TModule *mod = (struct TModule *) node;
		if (mod == (TAPTR) exec) continue;
		if (mod == exec->texb_HALBase) continue;
		tdbprintf1(20,"Module not closed: %s\n", mod->tmd_Name);
		n++;
	}

	if (n > 0)
	{
		exec_panic(exec, (TSTRPTR) "Applications must close all modules");
	}
}

static TVOID
exec_main(TEXECBASE *exec, struct TTask *exectask, struct TTagItem *tags)
{
	struct TMsgPort *execport = exec->texb_ExecPort;
	struct TMsgPort *modreply = exec->texb_ModReply;
	TUINT waitsig = TTASK_SIG_ABORT | execport->tmp_Signal | 
		modreply->tmp_Signal | TTASK_SIG_CHILDINIT | TTASK_SIG_CHILDEXIT;

	TUINT sig;
	struct TTask *taskmsg;
	
	for (;;)
	{
		sig = THALWait(exec->texb_HALBase, waitsig);
		if (sig & TTASK_SIG_ABORT) break;
		
		if (sig & modreply->tmp_Signal)
		{
			while ((taskmsg = exec_getmsg(exec, modreply)))
			{
				exec_modreply(exec, taskmsg);
			}
		}
		
		if (sig & execport->tmp_Signal)
		{
			while ((taskmsg = exec_getmsg(exec, execport)))
			{
				switch (taskmsg->tsk_ReqCode)
				{
					case TTREQ_OPENMOD:
						exec_requestmod(exec, taskmsg);
						break;
						
					case TTREQ_CLOSEMOD:
						exec_closemod(exec, taskmsg);
						break;
	
					case TTREQ_CREATETASK:
						exec_newtask(exec, taskmsg);
						break;
	
					case TTREQ_DESTROYTASK:
					{
						struct TNode *tempn;
						/* insert backptr to self */
						taskmsg->tsk_Request.trq_Task.trt_Parent = taskmsg;
						/* add request to taskexitlist */
						TADDTAIL(&exec->texb_TaskExitList,
							(struct TNode *) &taskmsg->tsk_Request, tempn);
						/* force list processing */
						sig |= TTASK_SIG_CHILDEXIT;
						break;
					}

					case TTREQ_LOCKATOM:
						exec_lockatom(exec, taskmsg);
						break;

					case TTREQ_UNLOCKATOM:
						exec_unlockatom(exec, taskmsg);
						break;
				}
			}
		}
		
		if (sig & TTASK_SIG_CHILDINIT)
		{
			exec_childinit(exec);
		}

		if (sig & TTASK_SIG_CHILDEXIT)
		{
			exec_childexit(exec);
		}
	}

	if (exec->texb_NumTasks || exec->texb_NumInitTasks)
	{
		tdbprintf2(20,"Number of tasks running: %d - initializing: %d\n", 
			exec->texb_NumTasks, exec->texb_NumInitTasks);
		exec_panic(exec, (TSTRPTR) "Applications must close all tasks");
	}
	
	exec_checkmodules(exec);
}

/*****************************************************************************/

static TVOID
exec_requestmod(TEXECBASE *exec, struct TTask *taskmsg)
{
	struct TModule *mod;
	union TTaskRequest *req;
	struct TNode *tempn;
	
	req = &taskmsg->tsk_Request;
	mod = (struct TModule *) TFindHandle(&exec->texb_ModList, 
		req->trq_Mod.trm_InitMod.tmd_Name);

	if (mod == TNULL)
	{
		/* prepare load request */
		taskmsg->tsk_ReqCode = TTREQ_LOADMOD;

		/* backup msg original replyport */
		req->trq_Mod.trm_RPort = TGetMsgReplyPort(taskmsg);

		/* init list of waiters */
		TINITLIST(&req->trq_Mod.trm_Waiters);

		/* mark module as uninitialized */
		req->trq_Mod.trm_InitMod.tmd_Flags = 0;

		/* insert request as fake/uninitialized module to modlist */
		TADDTAIL(&exec->texb_ModList, (struct TNode *) req, tempn);
		
		/* forward this request to I/O task */
		exec_PutMsg(exec, &exec->texb_IOTask->tsk_UserPort,
			exec->texb_ModReply, taskmsg);

		return;
	}

	if (mod->tmd_Version < req->trq_Mod.trm_InitMod.tmd_Version)
	{
		/* mod version insufficient */
		exec_returnmsg(exec, taskmsg, TMSG_STATUS_FAILED | TMSGF_QUEUED);
		return;
	}

	if (mod->tmd_Flags & TMODF_INITIALIZED)
	{
		/* request succeeded, reply */
		mod->tmd_RefCount++;
		req->trq_Mod.trm_Module = mod;
		exec_returnmsg(exec, taskmsg, TMSG_STATUS_REPLIED | TMSGF_QUEUED);
		return;
	}
	
	/* this mod is an initializing request.
	add new request to list of its waiters */

	req->trq_Mod.trm_ReqTask = taskmsg;

	TADDTAIL(&((union TTaskRequest *) mod)->trq_Mod.trm_Waiters, 
		(struct TNode *) req, tempn);
}

/*****************************************************************************/

static TVOID
exec_modreply(TEXECBASE *exec, struct TTask *taskmsg)
{
	struct TModule *mod;
	union TTaskRequest *req;
	struct TNode *node, *tnode;
	
	req = &taskmsg->tsk_Request;
	node = req->trq_Mod.trm_Waiters.tlh_Head;
	mod = req->trq_Mod.trm_Module;

	/* unlink fake/initializing module request from modlist */
	TREMOVE((struct TNode *) req);
	
	/* restore original replyport */
	TGetMsgPtr(taskmsg)->tmsg_RPort = req->trq_Mod.trm_RPort;
	
	if (TGetMsgStatus(taskmsg) == TMSG_STATUS_FAILED)
	{
		/* fail-reply to all waiters */
		while ((tnode = node->tln_Succ))
		{
			TREMOVE(node);
	
			/* reply to opener */
			exec_returnmsg(exec, 
				((union TTaskRequest *) node)->trq_Mod.trm_ReqTask, 
				TMSG_STATUS_FAILED | TMSGF_QUEUED);

			node = tnode;
		}

		/* forward failure to opener */
		exec_returnmsg(exec, taskmsg, TMSG_STATUS_FAILED | TMSGF_QUEUED);

		return;
	}

	/* mark as ready */
	mod->tmd_Flags |= TMODF_INITIALIZED;
	
	/* link real module to modlist */
	TADDTAIL(&exec->texb_ModList, (struct TNode *) mod, tnode);

	/* send replies to all waiters */
	while ((tnode = node->tln_Succ))
	{
		TREMOVE(node);
		
		/* insert module */
		((union TTaskRequest *) node)->trq_Mod.trm_Module = mod;

		/* reply to opener */
		exec_returnmsg(exec,
			((union TTaskRequest *) node)->trq_Mod.trm_ReqTask, 
			TMSG_STATUS_REPLIED | TMSGF_QUEUED);

		mod->tmd_RefCount++;
		node = tnode;
	}
	
	/* forward success to opener */
	exec_returnmsg(exec, taskmsg, TMSG_STATUS_REPLIED | TMSGF_QUEUED);
}

/*****************************************************************************/

static TVOID
exec_closemod(TEXECBASE *exec, struct TTask *taskmsg)
{
	struct TModule *mod;
	union TTaskRequest *req;
	
	req = &taskmsg->tsk_Request;
	mod = req->trq_Mod.trm_Module;

	if (--mod->tmd_RefCount == 0)
	{
		/* remove from modlist */
		TREMOVE((struct TNode *) mod);

		/* prepare unload request */
		taskmsg->tsk_ReqCode = TTREQ_UNLOADMOD;
		
		/* backup msg original replyport */
		req->trq_Mod.trm_RPort = TGetMsgReplyPort(taskmsg);

		/* forward this request to I/O task */
		exec_PutMsg(exec, &exec->texb_IOTask->tsk_UserPort, TNULL, taskmsg);

		return;
	}

	/* confirm */	
	exec_returnmsg(exec, taskmsg, TMSG_STATUS_ACKD | TMSGF_QUEUED);
}

/*****************************************************************************/

static TVOID
exec_newtask(TEXECBASE *exec, struct TTask *taskmsg)
{
	union TTaskRequest *req;
	struct TTask *newtask;
	struct TNode *tempn;
	
	req = &taskmsg->tsk_Request;

	req->trq_Task.trt_Task = newtask =
		exec_createtask(exec, req->trq_Task.trt_Func,
		req->trq_Task.trt_InitFunc, req->trq_Task.trt_Tags);

	if (newtask)
	{
		/* insert ptr to self, i.e. the requesting parent */
		req->trq_Task.trt_Parent = taskmsg;
		/* add request (not taskmsg) to list of initializing tasks */
		TADDTAIL(&exec->texb_TaskInitList, (struct TNode *) req, tempn);
		exec->texb_NumInitTasks++;
	}
	else
	{
		/* failed */
		exec_returnmsg(exec, taskmsg, TMSG_STATUS_FAILED | TMSGF_QUEUED);
	}
}

/*****************************************************************************/
/*
**	create task
*/

static TCALLBACK TVOID 
exec_usertaskdestroy(struct TTask *task)
{
	TEXECBASE *exec = TGetExecBase(task);
	struct TTask *self = THALFindSelf(exec->texb_HALBase);
	union TTaskRequest *req = &self->tsk_Request;
	self->tsk_ReqCode = TTREQ_DESTROYTASK;
	req->trq_Task.trt_Task = task;
	exec_sendmsg(exec, self, exec->texb_ExecPort, self);
}

static struct TTask *
exec_createtask(TEXECBASE *exec, TTASKFUNC func, TINITFUNC initfunc,
	struct TTagItem *tags)
{
	TAPTR hal = exec->texb_HALBase;
	TSTRPTR tname;
	TUINT tnlen = 0;
	struct TTask *newtask;
	
	tname = (TSTRPTR) TGetTag(tags, TTask_Name, TNULL);
	if (tname)
	{
		TSTRPTR t = tname;
		while (*t++);
		tnlen = t - tname;
	}

	/* note that tasks are message allocations */	
	newtask = exec_AllocMMU(exec, &exec->texb_MsgMMU, 
		sizeof(struct TTask) + tnlen);
	if (newtask == TNULL) return TNULL;

	exec_FillMem(exec, newtask, sizeof(struct TTask), 0);
	if (THALInitLock(hal, &newtask->tsk_TaskLock))
	{
		if (exec_initmmu(exec, &newtask->tsk_HeapMMU, TNULL, 
			TMMUT_TaskSafe | TMMUT_Tracking, TNULL))
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
						(TDFUNC) exec_usertaskdestroy;
					newtask->tsk_Handle.tmo_ModBase = exec;
					newtask->tsk_UserData = 
						(TAPTR) TGetTag(tags, TTask_UserData, TNULL);
					newtask->tsk_SigFree = ~((TUINT) TTASK_SIG_RESERVED);
					newtask->tsk_SigUsed = TTASK_SIG_RESERVED;
					newtask->tsk_Status = TTASK_INITIALIZING;
	
					newtask->tsk_Request.trq_Task.trt_Func = func;
					newtask->tsk_Request.trq_Task.trt_InitFunc = initfunc;
					newtask->tsk_Request.trq_Task.trt_Tags = tags;
						
					if (THALInitThread(hal, &newtask->tsk_Thread, 
						(TTASKENTRY TVOID (*)(TAPTR data)) exec_taskentryfunc,
							newtask))
					{
						/* kick it to life */
						THALSignal(hal, &newtask->tsk_Thread,
							TTASK_SIG_SINGLE);
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

	return TNULL;
}

/*****************************************************************************/
/*
**	Task entry
*/

static TVOID
closetaskfh(struct TTask *task, TAPTR fh, TUINT flag)
{
	if (task->tsk_Flags & flag)
	{
		TREMOVE((struct TNode *) fh);
		TIOCloseFile(task->tsk_IOBase, fh);		
	}
}

static TTASKENTRY TVOID
exec_taskentryfunc(struct TTask *task)
{
	TEXECBASE *exec = TGetExecBase(task);
	union TTaskRequest *req = &task->tsk_Request;

	TTASKFUNC func = req->trq_Task.trt_Func;
	TINITFUNC initfunc = req->trq_Task.trt_InitFunc;
	TTAGITEM *tags = req->trq_Task.trt_Tags;
	
	TAPTR currentdir = (TAPTR) TGetTag(tags, TTask_CurrentDir, TNULL);
	TUINT status = 0;

	if (currentdir)
	{
		task->tsk_IOBase = exec_OpenModule(exec, (TSTRPTR) "io", 0, TNULL);
		if (task->tsk_IOBase)
		{
			TAPTR newcd = TIODupLock(task->tsk_IOBase, currentdir);
			if (newcd)
			{
				/* new lock on currentdir duplicated from parent */
				TIOChangeDir(task->tsk_IOBase, newcd);
			}
			else goto closedown;
		} else goto closedown;
	}
	
	task->tsk_FHIn = (TAPTR) TGetTag(tags, TTask_InputFH, TNULL);
	task->tsk_FHOut = (TAPTR) TGetTag(tags, TTask_OutputFH, TNULL);
	task->tsk_FHErr = (TAPTR) TGetTag(tags, TTask_ErrorFH, TNULL);
	
	if (initfunc)
	{
		status = (*initfunc)(task);
	}
	else
	{
		status = 1;
	}
	
	if (status)
	{
		/* change from initializing to running state */

		task->tsk_Status = TTASK_RUNNING;
		THALSignal(exec->texb_HALBase, &exec->texb_ExecTask->tsk_Thread,
			TTASK_SIG_CHILDINIT);

		if (func)
		{
			(*func)(task);
		}
	}

	if (task->tsk_Flags & (TTASKF_CLOSEOUTPUT | TTASKF_CLOSEINPUT |
		TTASKF_CLOSEERROR))
	{
		if (task->tsk_IOBase == TNULL)
		{
			/* Someone handed filehandles over to this task, but
			** we don't even have an open I/O module. this shouldn't
			fail, really: */
			task->tsk_IOBase = exec_OpenModule(exec, (TSTRPTR) "io", 0, TNULL);
		}
		closetaskfh(task, task->tsk_FHOut, TTASKF_CLOSEOUTPUT);
		closetaskfh(task, task->tsk_FHIn, TTASKF_CLOSEINPUT);
		closetaskfh(task, task->tsk_FHErr, TTASKF_CLOSEERROR);
	}

closedown:	

	if (task->tsk_IOBase)
	{
		/* if we are responsible for a currentdir lock, close it */
		TAPTR cd = TIOChangeDir(task->tsk_IOBase, TNULL);
		if (cd) TIOUnlockFile(task->tsk_IOBase, cd);
		exec_CloseModule(exec, task->tsk_IOBase);
	}

	if (status)
	{
		/* succeeded */
		task->tsk_Status = TTASK_FINISHED;
		THALSignal(exec->texb_HALBase, &exec->texb_ExecTask->tsk_Thread,
				TTASK_SIG_CHILDEXIT);
	}
	else
	{
		/* system initialization failed */
		task->tsk_Status = TTASK_FAILED;
		THALSignal(exec->texb_HALBase, &exec->texb_ExecTask->tsk_Thread,
			TTASK_SIG_CHILDINIT);
	}
}

/*****************************************************************************/

static TVOID
exec_destroytask(TEXECBASE *exec, struct TTask *task)
{
	TAPTR hal = exec->texb_HALBase;
	THALDestroyThread(hal, &task->tsk_Thread);
	TDESTROY(&task->tsk_SyncPort);
	TDESTROY(&task->tsk_UserPort);
	TDESTROY(&task->tsk_HeapMMU);
	THALDestroyLock(hal, &task->tsk_TaskLock);
	exec_Free(exec, task);
}

/*****************************************************************************/
/* 
**	handle tasks that change from initializing to running state
*/

static TVOID
exec_childinit(TEXECBASE *exec)
{
	TAPTR hal = exec->texb_HALBase;
	struct TNode *nnode, *node;

	node = exec->texb_TaskInitList.tlh_Head;
	while ((nnode = node->tln_Succ))
	{
		union TTaskRequest *req = (union TTaskRequest *) node;
		struct TTask *task = req->trq_Task.trt_Task;
		struct TTask *taskmsg = req->trq_Task.trt_Parent;

		if (task->tsk_Status == TTASK_FAILED)
		{
			/* remove request from taskinitlist */
			TREMOVE((struct TNode *) req);
			exec->texb_NumInitTasks--;
			
			/* destroy task corpse */
			exec_destroytask(exec, task);

			/* fail-reply taskmsg to sender */
			exec_returnmsg(exec, taskmsg, TMSG_STATUS_FAILED | TMSGF_QUEUED);
		}
		else if (task->tsk_Status != TTASK_INITIALIZING)
		{		
			/* remove taskmsg from taskinitlist */
			TREMOVE((struct TNode *) req);
			exec->texb_NumInitTasks--;

			/* link task to list of running tasks */
			THALLock(hal, &exec->texb_Lock);
			/* note: using node as tempnode argument here */
			TADDTAIL(&exec->texb_TaskList, (struct TNode *) task, node);
			THALUnlock(hal, &exec->texb_Lock);
			exec->texb_NumTasks++;

			/* reply taskmsg */
			exec_returnmsg(exec, taskmsg, TMSG_STATUS_REPLIED | TMSGF_QUEUED);
		}

		node = nnode;
	}
}

/*****************************************************************************/
/* 
**	handle exiting tasks
*/

static TVOID
exec_childexit(TEXECBASE *exec)
{
	TAPTR hal = exec->texb_HALBase;
	struct TNode *nnode, *node;
	
	node = exec->texb_TaskExitList.tlh_Head;
	while ((nnode = node->tln_Succ))
	{
		union TTaskRequest *req = (union TTaskRequest *) node;
		struct TTask *task = req->trq_Task.trt_Task;
		struct TTask *taskmsg = req->trq_Task.trt_Parent;

		if (task->tsk_Status == TTASK_FINISHED)
		{
			/* unlink task from list of running tasks */
			THALLock(hal, &exec->texb_Lock);
			TREMOVE((struct TNode *) task);
			THALUnlock(hal, &exec->texb_Lock);
			
			/* destroy task */
			exec_destroytask(exec, task);
			
			/* unlink taskmsg from list of exiting tasks */
			TREMOVE((struct TNode *) req);
			exec->texb_NumTasks--;

			/* reply to caller */
			exec_returnmsg(exec, taskmsg, TMSG_STATUS_REPLIED | TMSGF_QUEUED);
		}

		node = nnode;
	}
}

/*****************************************************************************/
/* 
**	Atoms
*/

static TVOID 
replyatom(TEXECBASE *exec, struct TTask *msg, struct TAtom *atom)
{
	msg->tsk_Request.trq_Atom.tra_Atom = atom;
	exec_returnmsg(exec, msg, TMSG_STATUS_REPLIED | TMSGF_QUEUED);
}

static TAPTR
lookupatom(TEXECBASE *exec, TSTRPTR name)
{
	return TFindHandle(&exec->texb_AtomList, name);
}

static struct TAtom *
newatom(TEXECBASE *exec, TSTRPTR name)
{
	struct TAtom *atom;
	struct TNode *tempn;
	TSTRPTR s, d;
	
	s = d = name;
	while (*s++);
	
	atom = exec_AllocMMU(exec, TNULL, sizeof(struct TAtom) + (s - d));
	if (atom)
	{
		atom->tatm_Handle.tmo_ModBase = exec;
		atom->tatm_Handle.tmo_DestroyFunc = TNULL;
		s = d;
		d = (TSTRPTR) (atom + 1);
		atom->tatm_Handle.tmo_Name = d;
		while ((*d++ = *s++));
		TINITLIST(&atom->tatm_Waiters);
		atom->tatm_State = TATOMF_LOCKED;
		atom->tatm_Nest = 1;
		TADDHEAD(&exec->texb_AtomList, (struct TNode *) atom, tempn);
		tdbprintf1(2,"atom %s created - nest: 1\n", name);
	}
	
	return atom;
} 

/*****************************************************************************/

static TVOID
exec_lockatom(TEXECBASE *exec, struct TTask *msg)
{
	TUINT mode = msg->tsk_Request.trq_Atom.tra_Mode;
	struct TAtom *atom = msg->tsk_Request.trq_Atom.tra_Atom;
	struct TTask *task = msg->tsk_Request.trq_Atom.tra_Task;
	struct TNode *tempn;

	switch (mode & (TATOMF_CREATE|TATOMF_SHARED|TATOMF_NAME|TATOMF_TRY))
	{
		case TATOMF_CREATE | TATOMF_SHARED | TATOMF_NAME:
		case TATOMF_CREATE | TATOMF_NAME:

			atom = lookupatom(exec, (TSTRPTR) atom);
			if (atom) goto obtain;
			goto create;
	
		case TATOMF_CREATE | TATOMF_SHARED | TATOMF_TRY | TATOMF_NAME:
		case TATOMF_CREATE | TATOMF_TRY | TATOMF_NAME:

			atom = lookupatom(exec, (TSTRPTR) atom);
			if (atom)
			{
				atom = TNULL;	/* already exists - deny */
				goto reply;
			}

		create:	
			atom = newatom(exec, msg->tsk_Request.trq_Atom.tra_Atom);

		reply:
			replyatom(exec, msg, atom);
			return;
	
		case TATOMF_NAME | TATOMF_SHARED | TATOMF_TRY:
		case TATOMF_NAME | TATOMF_SHARED:
		case TATOMF_NAME | TATOMF_TRY:
		case TATOMF_NAME:

			atom = lookupatom(exec, (TSTRPTR) atom);
	
		case TATOMF_SHARED | TATOMF_TRY:
		case TATOMF_SHARED:
		case TATOMF_TRY:
		case 0:

			if (atom) goto obtain;

		fail:
		default:

			atom = TNULL;
			goto reply;

		obtain:

			if (atom->tatm_State & TATOMF_LOCKED)
			{
				if (atom->tatm_State & TATOMF_SHARED)
				{
					if (mode & TATOMF_SHARED)
					{
		nest:			atom->tatm_Nest++;
						tdbprintf1(2,"nest: %d\n", atom->tatm_Nest);
						goto reply;
					}
				}
				else
				{
					if (atom->tatm_Owner == task) goto nest;
				}
					
				if (mode & TATOMF_TRY) goto fail;
			}
			else
			{
				if (atom->tatm_Nest)
					tdbprintf1(20,"atom->nestcount %d!\n", atom->tatm_Nest);
					
				atom->tatm_State = TATOMF_LOCKED;
				if (mode & TATOMF_SHARED)
				{
					atom->tatm_State |= TATOMF_SHARED;
					atom->tatm_Owner = TNULL;
				}
				else
				{
					atom->tatm_Owner = task;
				}
				atom->tatm_Nest = 1;
				tdbprintf1(2,"atom taken. nest: %d\n", atom->tatm_Nest);
				goto reply;
			}
					
			/* put this request into atom's list of waiters */

			msg->tsk_Request.trq_Atom.tra_Mode = mode & TATOMF_SHARED;
			TADDTAIL(&atom->tatm_Waiters, &msg->tsk_Request.trq_Atom.tra_Node,
				tempn);
			tdbprintf(2,"must wait\n");
	}
}

/*****************************************************************************/

static TVOID
exec_unlockatom(TEXECBASE *exec, struct TTask *msg)
{
	TUINT mode = msg->tsk_Request.trq_Atom.tra_Mode;
	struct TAtom *atom = msg->tsk_Request.trq_Atom.tra_Atom;

	atom->tatm_Nest--;
	tdbprintf1(2,"unlock. nest: %d\n", atom->tatm_Nest);

	if (atom->tatm_Nest == 0)
	{
		union TTaskRequest *waiter;
	
		atom->tatm_State = 0;
		atom->tatm_Owner = TNULL;
		
		if (mode & TATOMF_DESTROY)
		{
			struct TNode *nextnode, *node = atom->tatm_Waiters.tlh_Head;
			while ((nextnode = node->tln_Succ))
			{
				waiter = (union TTaskRequest *) node;
				TREMOVE(node);
				replyatom(exec, waiter->trq_Atom.tra_Task, TNULL);
				node = nextnode;
			}
			
			tdbprintf1(2,"atom free: %s\n", atom->tatm_Handle.tmo_Name);
			TREMOVE((struct TNode *) atom);
			exec_Free(exec, atom);
		}
		else
		{
			struct TNode *tempn;
			waiter = (union TTaskRequest *)
				TREMHEAD(&atom->tatm_Waiters, tempn);
			if (waiter)
			{
				TUINT waitmode = waiter->trq_Atom.tra_Mode;

				atom->tatm_Nest++;
				atom->tatm_State = TATOMF_LOCKED;
				tdbprintf1(2,"restarted waiter. nest: %d\n", atom->tatm_Nest);
				replyatom(exec, waiter->trq_Atom.tra_Task, atom);
				
				/* just restarted a shared waiter?
					then restart ALL shared waiters */

				if (waitmode & TATOMF_SHARED)
				{
					struct TNode *nextnode, *node = 
						atom->tatm_Waiters.tlh_Head;
					atom->tatm_State |= TATOMF_SHARED;
					while ((nextnode = node->tln_Succ))
					{
						waiter = (union TTaskRequest *) node;
						if (waiter->trq_Atom.tra_Mode & TATOMF_SHARED)
						{
							TREMOVE(node);
							atom->tatm_Nest++;
							tdbprintf1(2,"restarted waiter. nest: %d\n",
								atom->tatm_Nest);
							replyatom(exec, waiter->trq_Atom.tra_Task, atom);
						}
						node = nextnode;
					}
				}
				else
				{
					atom->tatm_Owner = waiter->trq_Atom.tra_Task;
				}
			}
		}
	}

	exec_returnmsg(exec, msg, TMSG_STATUS_ACKD | TMSGF_QUEUED);
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: exec_doexec.c,v $
**	Revision 1.11  2005/09/13 02:41:58  tmueller
**	updated copyright reference
**	
**	Revision 1.10  2005/09/07 23:55:48  tmueller
**	updated old-style I/O module calls to new syntax
**	
**	Revision 1.9  2005/05/28 02:16:45  tmueller
**	improved panic message
**	
**	Revision 1.8  2004/07/05 21:31:33  tmueller
**	dead assignment removed and cosmetic
**	
**	Revision 1.7  2004/06/11 18:00:50  tmueller
**	Minor cleanup; added a seperate function exec_checkmodules()
**	
**	Revision 1.6  2004/04/18 14:08:52  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.5  2004/03/29 02:49:12  tmueller
**	Minor fixes that allow the exec module to compile with strict warning flags
**	
**	Revision 1.4  2004/02/22 04:20:47  tmueller
**	Minor cleanup and commentary changes
**	
**	Revision 1.3  2003/12/19 14:13:20  tmueller
**	Added sanity check for pending modules to the exectask closedown
**	
**	Revision 1.2  2003/12/12 03:43:49  tmueller
**	exec_panic() made LOCAL
**	
**	Revision 1.1.1.1  2003/12/11 07:19:05  tmueller
**	Krypton import
**	
*/
