
/*
**	$Header: /cvs/teklib/teklib/mods/hal/intent/hal.c,v 1.8 2004/02/07 05:06:04 tmueller Exp $
*/

#include <tek/debug.h>
#include <tek/mod/exec.h>
#include <tek/proto/exec.h>
#include <tek/mod/intent/hal.h>

#include "hal_mod.h"

#include <elate/tool.h>
#include <stdlib.h>
#include <sys/time.h>
#include <lib/time.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

/**************************************************************************
**
**	host
*/

LOCAL TBOOL hal_init(TMOD_HAL *hal, TTAGITEM *tags)
{
	struct HALintentSpecific *specific = kn_mem_allocdata(sizeof(struct HALintentSpecific));
	if (specific)
	{
		memset(specific, 0, sizeof(struct HALintentSpecific));

		if (!iselateerrno(specific->his_DevLock = kn_mtx_create(MTX_PRIO|MTX_BPIP|MTX_SIGMASK,0)))
		{
			time_t now;

			specific->his_SysDir  = (TSTRPTR) TGetTag(tags, TExecBase_SysDir, (TTAG) TEKHOST_SYSDIR);
			specific->his_ModDir  = (TSTRPTR) TGetTag(tags, TExecBase_ModDir, (TTAG) TEKHOST_MODDIR);
			specific->his_ProgDir = (TSTRPTR) TGetTag(tags, TExecBase_ProgDir, (TTAG) TEKHOST_PROGDIR);

			specific->his_Tags[0].tti_Tag = TExecBase_SysDir;
			specific->his_Tags[0].tti_Value = specific->his_SysDir;
			specific->his_Tags[1].tti_Tag = TExecBase_ModDir;
			specific->his_Tags[1].tti_Value = specific->his_ModDir;
			specific->his_Tags[2].tti_Tag = TExecBase_ProgDir;
			specific->his_Tags[2].tti_Value = specific->his_ProgDir;
			specific->his_Tags[3].tti_Tag = TTAG_DONE;

			tzset();

			time(&now);
			specific->his_TZSec = mktime(gmtime(&now)) - mktime(localtime(&now));
			specific->his_TZDay = specific->his_TZSec;
			specific->his_TZDay /= 86400;

			hal->hmb_Specific = specific;
			
			return TTRUE;
		}
		kn_mem_free(specific);
	}
	return TFALSE;
}

LOCAL TVOID hal_exit(TMOD_HAL *hal)
{
	struct HALintentSpecific *specific = hal->hmb_Specific;
	kn_mtx_delete(specific->his_DevLock);
	kn_mem_free(specific);
}

LOCAL TAPTR hal_allocself(TUINT size)
{
	return kn_mem_allocdata(size);
}

LOCAL TVOID hal_freeself(TAPTR mem, TUINT size)
{
	kn_mem_free(mem);
}


/**************************************************************************
**
**	memory
*/

EXPORT TAPTR hal_alloc(TMOD_HAL *hal, TUINT size)
{
	return kn_mem_allocdata(size);
}

EXPORT TVOID hal_free(TMOD_HAL *hal, TAPTR mem, TUINT size)
{
	kn_mem_free(mem);
}

EXPORT TAPTR hal_realloc(TMOD_HAL *hal, TAPTR mem, TUINT oldsize, TUINT newsize)
{
	ELATE_KN_RET_MEMALLOC newmem = kn_mem_realloc(mem, newsize);
	return (TAPTR) newmem.ptr;
}

EXPORT TVOID hal_fillmem(TMOD_HAL *hal, TAPTR dest, TUINT numbytes, TUINT8 fillval)
{
	memset(dest, (int) fillval, numbytes);
}

EXPORT TVOID hal_copymem(TMOD_HAL *hal, TAPTR from, TAPTR to, TUINT numbytes)
{
	memcpy(to, from, numbytes);
}


/**************************************************************************
**
**	locks
*/

EXPORT TBOOL hal_initlock(TMOD_HAL *hal, THALO *lock)
{
	ELATE_MUTEX *mtx;

	assert(sizeof(THALO) < sizeof(ELATE_MUTEX));

	if (!iselateerrno(mtx = kn_mtx_create(MTX_FIFO|MTX_NONE|MTX_RECURSIVE, 0)))
	{
		THALSetObject(lock, ELATE_MUTEX, mtx);
		return TTRUE;
	}
	return TFALSE;
}

EXPORT TVOID hal_destroylock(TMOD_HAL *hal, THALO *lock)
{
	ELATE_MUTEX *mtx = THALGetObject(lock, ELATE_MUTEX);
	kn_mtx_delete(mtx);

/*	THALSetObject(thread, ELATE_MUTEX, TNULL);	*/
/*	THALDestroyObject(hal, t, ELATE_MUTEX);		*/
}

EXPORT TVOID hal_lock(TMOD_HAL *hal, THALO *lock)
{
	ELATE_MUTEX *mtx = THALGetObject(lock, ELATE_MUTEX);
	kn_mtx_siglock(mtx);
}

EXPORT TVOID hal_unlock(TMOD_HAL *hal, THALO *lock)
{
	ELATE_MUTEX *mtx = THALGetObject(lock, ELATE_MUTEX);
	kn_mtx_unlock(mtx);
}


/**************************************************************************
**
**	threads
*/

extern struct HALintentThread *hal_getuserdata(TVOID) __attribute__
(( "qcall lib/tek/hal/getuserdata" ));

extern TVOID hal_setuserdata(TAPTR data) __attribute__
(( "qcall lib/tek/hal/setuserdata" ));

static TVOID hal_threadwait(struct HALintentThread *thread)
{
	hal_wait(TNULL, TTASK_SIG_SINGLE);
	(*thread->hit_Code)(thread->hit_Data);
}

EXPORT TBOOL hal_initthread(TMOD_HAL *hal, THALO *thread, TTASKENTRY TVOID (*function)(TAPTR task), TAPTR data)
{
	ELATE_PCB pcb;
	ELATE_SPAWN *spawn;

	struct HALintentThread *t;

	assert(sizeof(THALO) < sizeof(struct HALintentThread));

	if ((t = THALNewObject(hal, thread, struct HALintentThread)))
	{
		t->hit_Init = (TVOID *)hal_threadwait;
		t->hit_Data = data;
		t->hit_Code = function;

		t->hit_SigState = 0;

		if (!iselateerrno(t->hit_Sync = kn_mtx_create(MTX_PRIO|MTX_BPIP|MTX_SIGMASK,0)))
		{
			if (!iselateerrno(t->hit_Cond = kn_cond_create(CVAR_SHARED|CVAR_DONTFAILONTIMEOUT|CVAR_DONTFAILONSIGNAL|CVAR_PRIORITY)))
			{
				if (!function)
				{
					hal_setuserdata(t);
					THALSetObject(thread, struct HALintentThread, t);
					return TTRUE;
				}

				if (kn_proc_getparams(kn_proc_pid_get(), &pcb) == 0)
				{
					spawn = kn_proc_spawn_make("lib/tek/hal/thread", TNULL, TNULL, TNULL, &t, 4, 1, 1);
					if (spawn)
					{
						t->hit_PID = kn_proc_exec_local(spawn, &pcb);
						kn_mem_free(spawn);

						if (!iselateerrno(t->hit_PID))
						{
							THALSetObject(thread, struct HALintentThread, t);
							return TTRUE;
						}
					}
				}
				kn_cond_delete(t->hit_Cond);
			}
			kn_mtx_delete(t->hit_Sync);
		}
		THALDestroyObject(hal, t, struct HALintentThread);
	}
	return TFALSE;
}	

EXPORT TVOID hal_destroythread(TMOD_HAL *hal, THALO *thread)
{
	struct HALintentThread *t = THALGetObject(thread, struct HALintentThread);
	ELATE_MUTEX *sync = t->hit_Sync;
	ELATE_COND_VAR *cond = t->hit_Cond;

	if (t->hit_Code) kn_proc_wait(t->hit_PID, TNULL, 0);

	kn_mtx_delete(sync);		/* thread structure is invalid here	*/
	kn_cond_delete(cond);		/* for child processes				*/

/*	THALSetObject(thread, struct HALintentThread, TNULL);	*/
/*	THALDestroyObject(hal, t, struct HALintentThread);		*/
}

EXPORT TAPTR hal_findself(TMOD_HAL *hal)
{
	struct HALintentThread *t = hal_getuserdata();
	return (t)?t->hit_Data:t;
}


/**************************************************************************
**
**	modules
*/

EXPORT TAPTR hal_loadmodule(TMOD_HAL *hal, TSTRPTR name, TUINT16 version, TUINT *psize, TUINT *nsize)
{
	TSTRPTR path;
	ELATE_KN_RET_TOOLOPEN tool;

	TVOID *moddir = ((struct HALintentSpecific *) hal->hmb_Specific)->his_ModDir;
	TVOID *prgdir = ((struct HALintentSpecific *) hal->hmb_Specific)->his_ProgDir;

	struct HALintentMod *handle = hal_alloc(TNULL, sizeof(struct HALintentMod));
	if (!handle) return TNULL;

	if (name)
	{
		if (prgdir)
		{
			if ((path = hal_alloc(TNULL, strlen(prgdir)+5+strlen(name)+1)))
			{
				strcpy(path, prgdir);
				strcat(path, "/mod/");
				strcat(path, name);

				tool = kn_tool_open(path+1, TNULL);
				hal_free(TNULL, path, 0);

				if (tool.entrypoint)
				{
					handle->him_InitFunc = tool.entrypoint;
					*psize = (*handle->him_InitFunc)(TNULL, TNULL, version, TNULL);
					if (*psize)
					{
						*nsize = (*handle->him_InitFunc)(TNULL, TNULL, 0xFFFF, TNULL);
						handle->him_Version = version;
						return handle;
					}
					kn_tool_deref(tool.header);
				}
			}
		}

		if (moddir)
		{
			if ((path = hal_alloc(TNULL, strlen(moddir)+1+strlen(name)+1)))
			{
				strcpy(path, moddir);
				strcat(path, "/");
				strcat(path, name);

				tool = kn_tool_open(path+1, TNULL);
				hal_free(TNULL, path, 0);

				if (tool.entrypoint)
				{
					handle->him_InitFunc = tool.entrypoint;
					*psize = (*handle->him_InitFunc)(TNULL, TNULL, version, TNULL);
					if (*psize)
					{
						*nsize = (*handle->him_InitFunc)(TNULL, TNULL, 0xFFFF, TNULL);
						handle->him_Version = version;
						return handle;
					}
					kn_tool_deref(tool.header);
				}
			}
		}
	}
	hal_free(TNULL, handle, 0);
	return TNULL;
}

EXPORT TBOOL hal_callmodule(TMOD_HAL *hal, TAPTR halmod, TAPTR task, TAPTR data)
{
	struct HALintentMod *him = halmod;
	return (TBOOL) (*him->him_InitFunc)(task, data, him->him_Version, TNULL);
}

EXPORT TVOID hal_unloadmodule(TMOD_HAL *hal, TAPTR halmod)
{
	struct HALintentMod *him = halmod;
	kn_tool_code_deref(him->him_InitFunc);
 	hal_free(TNULL, halmod, 0);
}

static TBOOL hal_scanpath(TCALLBACK TBOOL (*callback)(TAPTR, TSTRPTR, TINT), TAPTR userdata, TSTRPTR path, TSTRPTR prefix)
{
	TBOOL response = TTRUE;
	DIR *dfd = opendir(path);
	if (dfd)
	{
		TINT elength;
		TINT plength = strlen(prefix);
		struct dirent *de;
		while (response && (de = readdir(dfd)))
		{
			elength = strlen(de->d_name);
			if (elength < plength + TEKHOST_EXTLEN) continue;
			if (strncmp(de->d_name, prefix, plength)) continue;

			response = (*callback)(userdata, de->d_name, elength - TEKHOST_EXTLEN);
		}
		closedir(dfd);
	}
	return response;
}

TMODAPI TBOOL hal_scanmodules(TMOD_HAL *hal, TSTRPTR prefix, TCALLBACK TBOOL (*callback)(TAPTR, TSTRPTR, TINT), TAPTR userdata)
{
	TBOOL response = TFALSE;
	TVOID *moddir = ((struct HALintentSpecific *) hal->hmb_Specific)->his_ModDir;
	TVOID *prgdir = ((struct HALintentSpecific *) hal->hmb_Specific)->his_ProgDir;

	if (moddir)
	{
		response = hal_scanpath(callback, userdata, moddir, prefix);
	}

	if (prgdir)
	{
		TSTRPTR path = hal_alloc(TNULL, strlen(prgdir)+4+1);
		if (path)
		{
			strcpy(path, prgdir);
			strcat(path, "/mod");
			response = hal_scanpath(callback, userdata, path, prefix);

			hal_free(TNULL, path, 0);
		}
	}
	return response;
}


/*****************************************************************************/
/*
**	time
*/

EXPORT TVOID hal_getsystime(TMOD_HAL *hal, TTIME *time)
{
	struct timeval tv;

	gettimeofday(&tv, TNULL);
	time->ttm_Sec  = (TUINT) tv.tv_sec;
	time->ttm_USec = (TUINT) tv.tv_usec;
}

static TINT hal_getsysdate(TMOD_HAL *hal, TDATE *datep, TINT *tzsecp)
{
	struct HALintentSpecific *his = hal->hmb_Specific;
	struct timeval tv;
	TDOUBLE syst;

	gettimeofday(&tv, NULL);

	syst = tv.tv_usec;
	syst /= 1000000;
	syst += tv.tv_sec;

	if (tzsecp)
	{
		*tzsecp = his->his_TZSec;
	}
	else
	{
		syst -= his->his_TZSec;
	}

	if (datep)
	{
		syst /= 86400;			/* secs -> days */
		syst += 2440587.5;		/* 1.1.1970 in Julian days */
		datep->tdt_Day.tdtt_Double = syst;
	}
	return 0;
}


/*****************************************************************************/
/*
**	signals
*/

EXPORT TVOID hal_signal(TMOD_HAL *hal, THALO *thread, TUINT signals)
{
	struct HALintentThread *t = THALGetObject(thread, struct HALintentThread);

	kn_mtx_siglock(t->hit_Sync);
	if (signals & ~t->hit_SigState)
	{
		t->hit_SigState |= signals;
		kn_cond_signal(t->hit_Cond);
	}
	kn_mtx_unlock(t->hit_Sync);
}

EXPORT TUINT hal_wait(TMOD_HAL *hal, TUINT sigmask)
{
	TUINT sig;
	struct HALintentThread *t = hal_getuserdata();

	kn_mtx_siglock(t->hit_Sync);
	for (;;)
	{
		sig = t->hit_SigState & sigmask;
		t->hit_SigState &= ~sigmask;

		if (sig) break;
		kn_cond_wait(t->hit_Cond, t->hit_Sync);
	}
	kn_mtx_unlock(t->hit_Sync);
	return sig;
}

EXPORT TUINT hal_setsignal(TMOD_HAL *hal, TUINT newsig, TUINT sigmask)
{
	TUINT oldsig;
	struct HALintentThread *t = hal_getuserdata();

	kn_mtx_siglock(t->hit_Sync);

	oldsig = t->hit_SigState;
	t->hit_SigState &= ~sigmask;
	t->hit_SigState |= newsig;
	if ((newsig & sigmask) & ~oldsig) kn_cond_signal(t->hit_Cond);

	kn_mtx_unlock(t->hit_Sync);
	return oldsig;
}

static TUINT hal_timedwait(TAPTR hal, struct HALintentThread *t, TTIME *wt, TUINT sigmask)
{
	TUINT sig;
	TUINT64 now = microtime(TNULL)*1000;
	TUINT64 timeout = ((TUINT64)wt->ttm_Sec*1000000+(TUINT64)wt->ttm_USec)*1000;

	if (now >= timeout) return 0;

	timeout -= now;

	kn_mtx_siglock(t->hit_Sync);
	for (;;)
	{
		sig = t->hit_SigState & sigmask;
		t->hit_SigState &= ~sigmask;

		if (sig) break;
		if (kn_cond_timedwait(t->hit_Cond, t->hit_Sync, timeout) == ETIMEDOUT) break;
	}
	kn_mtx_unlock(t->hit_Sync);
	return sig;
}


/*****************************************************************************/
/*
**	device
*/

static TVOID TTASKENTRY hal_devfunc(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_HAL *hal = TExecGetHALBase(exec);
	struct HALintentSpecific *his = hal->hmb_Specific;
	struct HALintentThread *thread = hal_getuserdata();
	TAPTR port = TExecGetUserPort(exec, task);
	struct TTimeRequest *msg;
	TUINT sig;
	TTIME waittime, curtime;
	
	waittime.ttm_Sec = 2000000000;
	waittime.ttm_USec = 0;

	for (;;)
	{
		sig = hal_timedwait(hal, thread, &waittime, TTASK_SIG_ABORT|TTASK_SIG_USER);

		if (sig & TTASK_SIG_ABORT) break;
	
		hal_getsystime(hal, &curtime);

		if (sig == 0)
		{
			struct TNode *nnode, *node;
			waittime.ttm_Sec = 2000000000;
			
			kn_mtx_siglock(his->his_DevLock);
			node = his->his_ReqList.tlh_Head;
			for (; (nnode = node->tln_Succ); node = nnode)
			{
				struct TTimeRequest *tr = (struct TTimeRequest *) node;
				TTIME *tm = &tr->ttr_Data.ttr_Time;
				if (hal_cmptime(&curtime, tm) >= 0)
				{
					TRemove(node);
					TExecReplyMsg(exec, node);
					continue;
				}
				if (hal_cmptime(tm, &waittime) < 0) waittime = *tm;
			}
			kn_mtx_unlock(his->his_DevLock);
			continue;
		}
		
		if (sig & TTASK_SIG_USER)
		{
			kn_mtx_siglock(his->his_DevLock);
			while ((msg = TExecGetMsg(exec, port)))
			{
				TTIME *tm = &msg->ttr_Data.ttr_Time;

				hal_addtime(tm, &curtime);

				if (hal_cmptime(tm, &waittime) < 0)
				{
					waittime = *tm;
				}
				TAddTail(&his->his_ReqList, (struct TNode *) msg);
			}
			kn_mtx_unlock(his->his_DevLock);
		}
	}
}

LOCAL TCALLBACK struct TTimeRequest *hal_open(TMOD_HAL *hal, TAPTR task, TTAGITEM *tags)
{
	struct HALintentSpecific *his = hal->hmb_Specific;
	TAPTR exec = TGetTag(tags, THalBase_Exec, TNULL);
	struct TTimeRequest *req;
	his->his_ExecBase = exec;

	req = TExecAllocMsg0(exec, sizeof(struct TTimeRequest));
	if (req)
	{
		kn_mtx_siglock(his->his_DevLock);
		
		if (!his->his_DevTask)
		{
			TTAGITEM tasktags[2];
			tasktags[0].tti_Tag = TTask_Name;
			tasktags[0].tti_Value = TTASKNAME_HALDEV;
			tasktags[1].tti_Tag = TTAG_DONE;
			TInitList(&his->his_ReqList);
			his->his_DevTask = TExecCreateSysTask(exec, hal_devfunc, tasktags);
		}

		if (his->his_DevTask)
		{
			his->his_RefCount++;
			req->ttr_Req.io_Device = (struct TModule *) hal;
		}
		else
		{	
			TExecFree(exec, req);
			req = TNULL;
		}
		kn_mtx_unlock(his->his_DevLock);
	}
	return req;
}

LOCAL TCALLBACK TVOID hal_close(TMOD_HAL *hal, TAPTR task)
{
	struct HALintentSpecific *his = hal->hmb_Specific;

	kn_mtx_siglock(his->his_DevLock);
	if (his->his_DevTask)
	{
		if (--his->his_RefCount == 0)
		{
			TExecSignal(his->his_ExecBase, his->his_DevTask, TTASK_SIG_ABORT);
			TDestroy(his->his_DevTask);
			his->his_DevTask = TNULL;
		}
	}
	kn_mtx_unlock(his->his_DevLock);
}

EXPORT TVOID hal_beginio(TMOD_HAL *hal, struct TTimeRequest *req)
{
	struct HALintentSpecific *his = hal->hmb_Specific;
	TAPTR exec = his->his_ExecBase;
	TTIME now;
	TDOUBLE x = 0;

	switch (req->ttr_Req.io_Command)
	{
		/* execute asynchronously */
		case TTREQ_ADDLOCALDATE:
			x = his->his_TZDay;						/* days west of GMT */
		case TTREQ_ADDUNIDATE:
			x += req->ttr_Data.ttr_Date.ttr_Date.tdt_Day.tdtt_Double;
			x -= 2440587.5;							/* days since 1.1.1970 */
			x *= 86400;								/* secs since 1.1.1970 */
			req->ttr_Data.ttr_Time.ttm_Sec = x;
			x -= req->ttr_Data.ttr_Time.ttm_Sec;	/* fraction of sec */
			x *= 1000000;
			req->ttr_Data.ttr_Time.ttm_USec = x;	/* microseconds */
			hal_getsystime(hal, &now);
			hal_subtime(&req->ttr_Data.ttr_Time, &now);
			/* relative time */
		case TTREQ_ADDTIME:
			TExecPutMsg(exec, TExecGetUserPort(exec, his->his_DevTask), req->ttr_Req.io_ReplyPort, req);
			return;

		/* execute synchronously */
		default:
		case TTREQ_GETUNIDATE:
			hal_getsysdate(hal, &req->ttr_Data.ttr_Date.ttr_Date, &req->ttr_Data.ttr_Date.ttr_TimeZone);
			break;

		case TTREQ_GETLOCALDATE:
			hal_getsysdate(hal, &req->ttr_Data.ttr_Date.ttr_Date, TNULL);
			break;

		case TTREQ_GETTIME:
			hal_getsystime(hal, &req->ttr_Data.ttr_Time);
			break;
	}

	if (!(req->ttr_Req.io_Flags & TIOF_QUICK))
	{
		/* async operation indicated; reply to self */
		TExecReplyMsg(exec, req);
	}
}

EXPORT TINT hal_abortio(TMOD_HAL *hal, struct TTimeRequest *req)
{
	struct HALintentSpecific *his = hal->hmb_Specific;
	TAPTR exec = his->his_ExecBase;
	TINT error = -1;

	kn_mtx_siglock(his->his_DevLock);

	if (TExecGetMsgStatus(exec, req) == (TMSG_STATUS_SENT|TMSGF_QUEUED))
	{
		TExecRemoveMsg(exec, req->ttr_Req.io_ReplyPort, req);
	}
	else
	{
		req = TNULL;
	}

	kn_mtx_unlock(his->his_DevLock);

	if (req)
	{
		TExecInsertMsg(exec, req->ttr_Req.io_ReplyPort,	req, TNULL, TMSG_STATUS_REPLIED|TMSGF_QUEUED);
		req->ttr_Req.io_Error = TIOERR_ABORTED;
		error = 0;
	}
	return error;
}


/*
**	Revision History
**	$Log: hal.c,v $
**	Revision 1.8  2004/02/07 05:06:04  tmueller
**	Internal time support functions used; TIOF_QUICK no longer cleared
**	
**	Revision 1.7  2004/01/05 10:55:45  mlukat
**	misc updates (module locations etc)
**	
**	Revision 1.6  2003/12/19 15:35:18  mlukat
**	back in sync with posix version (applied abs date fix)
**	
**	Revision 1.5  2003/12/19 14:01:21  mlukat
**	- sorted out rel/abs timestamp issues
**	- the timestamps passed in the request are always made absolute
**	
**	Revision 1.4  2003/12/19 11:15:27  mlukat
**	added hal_beginio support
**	
**	Revision 1.3  2003/12/18 14:49:18  mlukat
**	added support for hal_open, hal_close, hal_abortio
**	
**	Revision 1.2  2003/12/18 10:04:28  mlukat
**	move helper tools out of the way, i.e. don't share directory with 'real'
**	binaries
**	
**	Revision 1.1  2003/12/17 15:34:00  mlukat
**	initial version
**	
**	Revision 1.2  2003/03/08 22:05:32  tmueller
**	The HAL modules now also respect the TEKHOST_...DIR defines
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
**	
*/
