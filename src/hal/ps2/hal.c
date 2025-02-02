
/*
**	$Id: hal.c,v 1.11 2007/04/21 14:58:44 fschulze Exp $
**	teklib/mods/hal/ps2/hal.c - PS2 implementation of the HAL layer
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	and Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/proto/exec.h>
#include <tek/mod/ps2/hal.h>
#include <tek/mod/ps2/memory.h>
#include <tek/mod/ps2/eekernel.h>

#include "hal_mod.h"

#include <stdlib.h>
#include <string.h>
#include <tamtypes.h>
#include <kernel.h>

#define THREADSTACKSIZE		16384
#define STACKALIGNMASK		0x3fff

TAPTR *GetThreadPrivate(TUINT mask);
static TVOID TTASKENTRY hal_devfunc(struct TTask *task);
static TINT timerfunc(TINT ca);
static TVOID time_init(TVOID);
static TVOID do_gettimeofday(TTIME *tv);

extern void *_gp;

/*****************************************************************************/
/*
**	Host init
*/

LOCAL TBOOL
hal_init(struct THALBase *hal, TTAGITEM *tags)
{
	struct HALPS2Boot *boot = hal->hmb_BootHnd;
	struct HALSpecific *specific;

	specific = (*boot->hpb_Alloc)(boot, sizeof(struct HALSpecific));
	if (specific)
	{
		memset(specific, 0, sizeof(struct HALSpecific));

		specific->hsp_SysDir = (TSTRPTR) TGetTag(tags,
			TExecBase_SysDir, (TTAG) TEKHOST_SYSDIR);
		specific->hsp_ModDir = (TSTRPTR) TGetTag(tags,
			TExecBase_ModDir, (TTAG) TEKHOST_MODDIR);
		specific->hsp_ProgDir = (TSTRPTR) TGetTag(tags,
			TExecBase_ProgDir, (TTAG) TEKHOST_PROGDIR);

		specific->hsp_Tags[0].tti_Tag = TExecBase_SysDir;
		specific->hsp_Tags[0].tti_Value = (TTAG) specific->hsp_SysDir;

		specific->hsp_Tags[1].tti_Tag = TExecBase_ModDir;
		specific->hsp_Tags[1].tti_Value = (TTAG) specific->hsp_ModDir;
		specific->hsp_Tags[2].tti_Tag = TExecBase_ProgDir;

		specific->hsp_Tags[2].tti_Value = (TTAG) specific->hsp_ProgDir;
		specific->hsp_Tags[3].tti_Tag = TTAG_DONE;

		hal->hmb_Specific = specific;
		specific->hsp_TZSec = 0;
		specific->hsp_TZDays = 0;
		time_init();

		/* init devlock */
		TINITLIST(&specific->hsp_DevLock.tlk_Waiters);
		specific->hsp_DevLock.tlk_Owner = TNULL;
		specific->hsp_DevLock.tlk_NestCount = 0;
		specific->hsp_DevLock.tlk_WaitCount = 0;

		return TTRUE;
	}
	return TFALSE;
}

LOCAL TVOID
hal_exit(struct THALBase *hal)
{
	struct HALSpecific *specific = hal->hmb_Specific;
	struct HALPS2Boot *boot = hal->hmb_BootHnd;
	(*boot->hpb_Free)(boot, specific, sizeof(struct HALSpecific));
}

LOCAL TAPTR
hal_allocself(TAPTR handle, TUINT size)
{
	struct HALPS2Boot *boot = handle;
	return (*boot->hpb_Alloc)(boot, size);
}

LOCAL TVOID
hal_freeself(TAPTR handle, TAPTR mem, TUINT size)
{
	struct HALPS2Boot *boot = handle;
	(*boot->hpb_Free)(boot, mem, size);
}

/*****************************************************************************/

static TVOID
hal_disable(struct THALBase *hal)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	DI();
	++hps->hsp_LockCount;
}

static TVOID
hal_enable(struct THALBase *hal)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	if (--hps->hsp_LockCount == 0)
	{
		EI();
	}
}

/*****************************************************************************/
/*
**	Memory
*/

EXPORT TAPTR
hal_alloc(struct THALBase *hal, TUINT size)
{
	struct HALPS2Boot *boot = hal->hmb_BootHnd;
	TAPTR mem;
	hal_disable(hal);
	mem = (*boot->hpb_Alloc)(boot, size);
	hal_enable(hal);
	return mem;
}

EXPORT TAPTR
hal_realloc(struct THALBase *hal, TAPTR mem, TUINT oldsize, TUINT newsize)
{
	struct HALPS2Boot *boot = hal->hmb_BootHnd;
	TAPTR newmem;
	hal_disable(hal);
	newmem = (*boot->hpb_Realloc)(boot, mem, oldsize, newsize);
	hal_enable(hal);
	return newmem;
}

EXPORT TVOID
hal_free(struct THALBase *hal, TAPTR mem, TUINT size)
{
	struct HALPS2Boot *boot = hal->hmb_BootHnd;
	hal_disable(hal);
	(*boot->hpb_Free)(boot, mem, size);
	hal_enable(hal);
}

EXPORT TVOID
hal_copymem(struct THALBase *hal, TAPTR from, TAPTR to, TUINT numbytes)
{
	memcpy(to, from, numbytes);
}

EXPORT TVOID
hal_fillmem(struct THALBase *hal, TAPTR dest, TUINT numbytes, TUINT8 fillval)
{
	memset(dest, (int) fillval, numbytes);
}

/*****************************************************************************/
/*
**	Locks
*/

EXPORT TBOOL
hal_initlock(struct THALBase *hal, struct THALObject *lock)
{
	return TTRUE;
}

EXPORT TVOID
hal_destroylock(struct THALBase *hal, struct THALObject *lock)
{
}

EXPORT TVOID
hal_lock(struct THALBase *hal, struct THALObject *lock)
{
	hal_disable(hal);
}

EXPORT TVOID
hal_unlock(struct THALBase *hal, struct THALObject *lock)
{
	hal_enable(hal);
}

/*****************************************************************************/
/*
**	Threads
*/

static void
threadentry(struct HALThread *thread)
{
	struct THALBase *hal = thread->hth_HALBase;

	/* wait for initial signal to run */
	hal_wait(hal, TTASK_SIG_SINGLE);

	/* call function */
	(*thread->hth_Function)(thread->hth_Data);

	ExitThread();
}

EXPORT TBOOL
hal_initthread(struct THALBase *hal, struct THALObject *thread,
	TTASKENTRY TVOID (*function)(struct TTask *task), TAPTR data)
{
	struct HALThread *t =
		THALNewObject(hal, thread, struct HALThread);
	if (t)
	{
		t->hth_SigSemaphore.init_count = 0;
		t->hth_SigSemaphore.max_count = 1;
		t->hth_SigSID = CreateSema(&t->hth_SigSemaphore);
		if (t->hth_SigSID != -1)
		{
			t->hth_SigState = 0;
			t->hth_Function = function;
			t->hth_Data = data;
			t->hth_HALBase = hal;
			THALSetObject(thread, struct HALThread, t);
			if (function)
			{
				t->hth_Stack = hal_alloc(hal, THREADSTACKSIZE + STACKALIGNMASK);
				if (t->hth_Stack)
				{
					TUINT aligned = (TUINT) t->hth_Stack;
					TUINT *stackp;
					aligned += STACKALIGNMASK;
					aligned &= ~STACKALIGNMASK;
					stackp = (TUINT *) aligned;

					/* place contextdata in stacktop */
					stackp[THREADSTACKSIZE/4-1] = (TUINT) t;
					t->hth_TCB.func = threadentry;
					t->hth_TCB.stack = stackp;
					t->hth_TCB.stack_size = THREADSTACKSIZE - 16;
					t->hth_TCB.gp_reg = &_gp;
					t->hth_TCB.initial_priority = 1;

					t->hth_TID = CreateThread(&t->hth_TCB);

					if (t->hth_TID != -1)
					{
						StartThread(t->hth_TID, t);
						return TTRUE;
					}
					hal_free(hal, t->hth_Stack, THREADSTACKSIZE + STACKALIGNMASK);
				}
			}
			else
			{
				/* place contextdata in stacktop */
				TAPTR *sptr = GetThreadPrivate(~STACKALIGNMASK);
				*sptr = t;
				t->hth_TID = GetThreadId();
				return TTRUE;
			}
			DeleteSema(t->hth_SigSID);
		}
		THALDestroyObject(hal, t, struct HALThread);
	}
	TDBPRINTF(20,("could not create thread\n"));
	return TFALSE;
}

EXPORT TVOID
hal_destroythread(struct THALBase *hal, struct THALObject *thread)
{
	struct HALThread *t = THALGetObject(thread, struct HALThread);
	if (t->hth_Function)
	{
		/* !!!FIXME!!! */
		if (DeleteThread(t->hth_TID) == -1)
		{
			TDBPRINTF(20,("could not destroy thread\n"));
		}

		hal_free(hal, t->hth_Stack, THREADSTACKSIZE + STACKALIGNMASK);
	}
	DeleteSema(t->hth_SigSID);
	THALDestroyObject(hal, t, struct HALThread);
}

EXPORT TAPTR
hal_findself(struct THALBase *hal)
{
	struct HALThread *t = *GetThreadPrivate(~STACKALIGNMASK);
	return t->hth_Data;
}

/*****************************************************************************/
/*
**	Signals
*/

EXPORT TVOID
hal_signal(struct THALBase *hal, struct THALObject *thread, TUINT signals)
{
	struct HALThread *t = THALGetObject(thread, struct HALThread);
	hal_disable(hal);
	if (signals & ~t->hth_SigState)
	{
		t->hth_SigState |= signals;
		hal_enable(hal);
		WakeupThread(t->hth_TID);
	}
	else
	{
		hal_enable(hal);
	}
}

EXPORT TUINT
hal_setsignal(struct THALBase *hal, TUINT newsig, TUINT sigmask)
{
	TUINT oldsig;
	struct HALThread *t = *GetThreadPrivate(~STACKALIGNMASK);

	hal_disable(hal);
	oldsig = t->hth_SigState;
	t->hth_SigState &= ~sigmask;
	t->hth_SigState |= newsig;
	if ((newsig & sigmask) & ~oldsig)
	{
		hal_enable(hal);
		WakeupThread(t->hth_TID);
	}
	else
	{
		hal_enable(hal);
	}
	return oldsig;
}

EXPORT TUINT
hal_wait(struct THALBase *hal, TUINT sigmask)
{
	TUINT sig;
	TAPTR *sptr = GetThreadPrivate(~STACKALIGNMASK);
	struct HALThread *t = *sptr;

	for (;;)
	{
		hal_disable(hal);
		sig = t->hth_SigState & sigmask;
		t->hth_SigState &= ~sigmask;
		hal_enable(hal);

		if (sig) break;

		SleepThread();
	}

	return sig;
}


/*****************************************************************************/
/*
**	Time and date
*/

static TUINT64	TCount = 0;
static volatile TINT *T0Count = (volatile TINT *)T0_COUNT;
static volatile TINT *T0Mode  = (volatile TINT *)T0_MODE;
static volatile TINT *T0Comp  = (volatile TINT *)T0_COMP;

static TUINT64 sig_counter = 0;
static TINT sig_thread_ID = -1;

static TUINT
hal_timedwaitevent(TAPTR hal, struct HALThread *t, TTIME *wt, TUINT sigmask)
{
	TUINT sig;
	TTIME waitt, curt;
	TINT64 cycles = 0;

	/* calc relative time */
	waitt = *wt;
	hal_getsystime(hal, &curt);
	hal_subtime(&waitt, &curt);

	if (waitt.ttm_Sec >= 0)
	{
		if (waitt.ttm_Sec) cycles = (TUINT64) waitt.ttm_Sec * 1474560 / 256 / 100;
		if (waitt.ttm_USec)	cycles += (TUINT64) waitt.ttm_USec * 1474560 / (256*1000000) / 100;
	}

	sig_counter = cycles;
	for (;;)
	{
		hal_disable(hal);
		sig = t->hth_SigState & sigmask;
		t->hth_SigState &= ~sigmask;
		hal_enable(hal);
		if (sig || sig_counter <= 0) break;
		sig_thread_ID = t->hth_TID;
		SleepThread();
		sig_thread_ID = -1;
	}

	return sig;
}

static TINT
timerfunc(TINT ca)
{
	TCount++;
	*T0Mode |= 1<<7;

	if (sig_counter > 0)
	{
		sig_counter--;
		if (sig_counter == 0 && sig_thread_ID > -1)
		{
			iWakeupThread(sig_thread_ID);
		}
	}

	ExitHandler();
	return 0;
}

static TVOID
time_init(TVOID)
{
	AddIntcHandler(9, timerfunc, 0);

	*T0Count = 0;
	/* number of clock cycles for 256*100 interrupts per second @147.456MHz */
	*T0Comp = 147456000/(256*100);
	*T0Mode = 2 | (1<<6) | (1<<7) | (1<<8) | (1<<10);

	EnableIntc(9);
}

static TVOID
do_gettimeofday(TTIME *tv)
{
	TUINT64 t = TCount;
	TUINT offset = *T0Count;

	tv->ttm_Sec = t / 100;
	tv->ttm_USec = (t - tv->ttm_Sec * 100) * 10000 + (TFLOAT)offset * 1.7361111f;

	if (tv->ttm_USec >= 1000000)
	{
		tv->ttm_USec -= 1000000;
		tv->ttm_Sec++;
	}
}

EXPORT TVOID
hal_getsystime(struct THALBase *hal, TTIME *time)
{
	do_gettimeofday(time);
}

/*****************************************************************************/
/*
**	err = hal_getsysdate(hal, datep, tzsecp)
**
**	Insert datetime into *datep. If tzsecp is NULL, *datep will
**	be set to local time. Otherwise, *datep will be set to UT,
**	and *tzsecp will be set to seconds west of GMT.
**
**	err = 0 - ok
**	err = 1 - no date resource available
**	err = 2 - no timezone info available
*/


static TINT
hal_getsysdate(struct THALBase *hal, TDATE *datep, TINT *tzsecp)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	TTIME tv;
	TDOUBLE syst;

	do_gettimeofday(&tv);

	syst = tv.ttm_USec;
	syst /= 1000000;

	syst += tv.ttm_Sec;

	if (tzsecp)
	{
		*tzsecp = hps->hsp_TZSec;
	}
	else
	{
		syst -= hps->hsp_TZSec;
	}

	if (datep)
	{
		syst /= 86400;			/* secs -> days */
		syst += 2440587.5;		/* 1.1.1970 in Julian days */

		datep->tdt_Double = syst;
	}

	return 0;
}


/*****************************************************************************/
/*
**	Module open and entry
*/

EXPORT TAPTR
hal_loadmodule(struct THALBase *hal, TSTRPTR name, TUINT16 version, TUINT *psize,
	TUINT *nsize)
{
	TDBPRINTF(20,("*** hal_loadmodule (%s) called\n", name));
	return TNULL;
}

EXPORT TBOOL
hal_callmodule(struct THALBase *hal, TAPTR mod, struct TTask *task, TAPTR data)
{
	TDBFATAL();
	return 0;
}

EXPORT TVOID
hal_unloadmodule(struct THALBase *hal, TAPTR halmod)
{
	TDBFATAL();
}

EXPORT TBOOL
hal_scanmodules(struct THALBase *hal, TSTRPTR path, struct THook *hook)
{
	return 1;
}

/*****************************************************************************/
/*
**	HAL device - instance open/close
*/

LOCAL TCALLBACK struct TTimeRequest *
hal_open(struct THALBase *hal, struct TTask *task, TTAGITEM *tags)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	TAPTR exec = (TAPTR) TGetTag(tags, THalBase_Exec, TNULL);
	struct TTimeRequest *req;
	hps->hsp_ExecBase = exec;

	req = TExecAllocMsg0(exec, sizeof(struct TTimeRequest));
	if (req)
	{
		TExecLock(exec, &hps->hsp_DevLock);

		if (!hps->hsp_DevTask)
		{
			TTAGITEM tasktags[2];
			tasktags[0].tti_Tag = TTask_Name;			/* set task name */
			tasktags[0].tti_Value = (TTAG) TTASKNAME_HALDEV;
			tasktags[1].tti_Tag = TTAG_DONE;
			TInitList(&hps->hsp_ReqList);
			hps->hsp_DevTask = TExecCreateSysTask(exec, hal_devfunc, tasktags);
		}

		if (hps->hsp_DevTask)
		{
			hps->hsp_RefCount++;
			req->ttr_Req.io_Device = (struct TModule *) hal;
		}
		else
		{
			TExecFree(exec, req);
			req = TNULL;
		}

		TExecUnlock(exec, &hps->hsp_DevLock);
	}

	return req;
}

/*****************************************************************************/

LOCAL TCALLBACK TVOID
hal_close(struct THALBase *hal, struct TTask *task)
{
	struct HALSpecific *hps = hal->hmb_Specific;

	TExecLock(hps->hsp_ExecBase, &hps->hsp_DevLock);

	if (hps->hsp_DevTask)
	{
		if (--hps->hsp_RefCount == 0)
		{
			TDBPRINTF(2,("hal.device refcount dropped to 0\n"));
			TExecSignal(hps->hsp_ExecBase, hps->hsp_DevTask, TTASK_SIG_ABORT);
			TDBPRINTF(2,("destroy hal.device task...\n"));
			TDestroy(hps->hsp_DevTask);
			hps->hsp_DevTask = TNULL;
		}
	}

	TExecUnlock(hps->hsp_ExecBase, &hps->hsp_DevLock);
}

/*****************************************************************************/

static TVOID TTASKENTRY hal_devfunc(struct TTask *task)
{
	TAPTR exec = TGetExecBase(task);
	struct THALBase *hal = TExecGetHALBase(exec);
	struct HALSpecific *hps = hal->hmb_Specific;
	struct HALThread *thread = *GetThreadPrivate(~STACKALIGNMASK);
	TAPTR port = TExecGetUserPort(exec, task);
	struct TTimeRequest *msg;
	TUINT sig = 0;
	TTIME waittime, curtime;
	struct TNode *nnode, *node;

 	waittime.ttm_Sec = 2000000000;
 	waittime.ttm_USec = 0;

	for (;;)
	{
		sig = hal_timedwaitevent(hal, thread, &waittime,
			TTASK_SIG_ABORT | TTASK_SIG_USER);

		if (sig & TTASK_SIG_ABORT)
			break;

		TExecLock(exec, &hps->hsp_DevLock);
		hal_getsystime(hal, &curtime);

		while ((msg = TExecGetMsg(exec, port)))
		{
			hal_addtime(&msg->ttr_Data.ttr_Time, &curtime);
			TAddTail(&hps->hsp_ReqList, (struct TNode *) msg);
		}

		waittime.ttm_Sec = 2000000000;
		waittime.ttm_USec = 0;
		node = hps->hsp_ReqList.tlh_Head;
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
			if (hal_cmptime(tm, &waittime) < 0)
				waittime = *tm;
		}

		TExecUnlock(exec, &hps->hsp_DevLock);
	}

	TDBPRINTF(2,("goodbye from HAL device\n"));
}

EXPORT TVOID
hal_beginio(struct THALBase *hal, struct TTimeRequest *req)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	TAPTR exec = hps->hsp_ExecBase;
	TTIME nowtime;
	TDOUBLE x = 0;

	switch (req->ttr_Req.io_Command)
	{
		/* execute asynchronously */
		case TTREQ_ADDLOCALDATE:
			x = hps->hsp_TZDays;					/* days west of GMT */
		case TTREQ_ADDUNIDATE:
			x += req->ttr_Data.ttr_Date.ttr_Date.tdt_Double;
			x -= 2440587.5;							/* days since 1.1.1970 */
			x *= 86400;								/* secs since 1.1.1970 */
			req->ttr_Data.ttr_Time.ttm_Sec = x;
			x -= req->ttr_Data.ttr_Time.ttm_Sec;	/* fraction of sec */
			x *= 1000000;
			req->ttr_Data.ttr_Time.ttm_USec = x;	/* microseconds */
			hal_getsystime(hal, &nowtime);
			hal_subtime(&req->ttr_Data.ttr_Time, &nowtime);
			/* relative time */
		case TTREQ_ADDTIME:
			TExecPutMsg(exec, TExecGetUserPort(exec, hps->hsp_DevTask),
				req->ttr_Req.io_ReplyPort, req);
			return;

		/* execute synchronously */
		default:
		case TTREQ_GETUNIDATE:
			hal_getsysdate(hal, &req->ttr_Data.ttr_Date.ttr_Date,
				&req->ttr_Data.ttr_Date.ttr_TimeZone);
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

EXPORT TINT
hal_abortio(struct THALBase *hal, struct TTimeRequest *req)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	TAPTR exec = hps->hsp_ExecBase;
	TUINT status;

	TExecLock(exec, &hps->hsp_DevLock);
	status = TExecGetMsgStatus(exec, req);
	if (!(status & TMSGF_RETURNED))
	{
		if (status & TMSGF_QUEUED)
		{
			/* remove from ioport */
			TAPTR ioport = TExecGetUserPort(exec, hps->hsp_DevTask);
			TExecRemoveMsg(exec, ioport, req);
		}
		else
		{
			/* remove from reqlist */
			TRemove((struct TNode *) req);
			TExecSignal(exec, hps->hsp_DevTask, TTASK_SIG_USER);
		}
	}
	else
	{
		/* already replied */
		req = TNULL;
	}
	TExecUnlock(exec, &hps->hsp_DevLock);

	if (req)
	{
		req->ttr_Req.io_Error = TIOERR_ABORTED;
		TExecReplyMsg(exec, req);
	}

	return 0;
}

#if 0
static TVOID TTASKENTRY
haldevfunc(struct TTask *task)
{
	TAPTR exec = TGetExecBase(task);
	struct THALBase *hal = TExecGetHALBase(exec);
	struct HALSpecific *hps = hal->hmb_Specific;
	struct HALThread *thread = *GetThreadPrivate(~STACKALIGNMASK);
	TAPTR port = TExecGetUserPort(exec, task);
	struct TTimeRequest *msg;
	TUINT sig;
	TTIME waittime, curtime;

	waittime.ttm_Sec  = 2000000000;
	waittime.ttm_USec = 0;

	for (;;)
	{
		sig = hal_timedwaitevent(hal, thread, &waittime,
			TTASK_SIG_ABORT | TTASK_SIG_USER);

		if (sig & TTASK_SIG_ABORT) break;

		hal_getsystime(hal, &curtime);

		if (sig == 0)
		{
			struct TNode *nnode, *node;
			waittime.ttm_Sec = 2000000000;

			TExecLock(exec, &hps->hsp_DevLock);
			node = hps->hsp_ReqList.tlh_Head;
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
			TExecUnlock(exec, &hps->hsp_DevLock);
			continue;
		}

		if (sig & TTASK_SIG_USER)
		{
			/* got message */
			TExecLock(exec, &hps->hsp_DevLock);
			while ((msg = TExecGetMsg(exec, port)))
			{
				TTIME *tm = &msg->ttr_Data.ttr_Time;
				hal_addtime(tm, &curtime);

				if (hal_cmptime(tm, &waittime) < 0)
				{
					/* next event */
					waittime = *tm;
				}

				/* insert to queue */
				TAddTail(&hps->hsp_ReqList, (struct TNode *) msg);
			}
			TExecUnlock(exec, &hps->hsp_DevLock);
		}
	}

	TDBPRINTF(2,("goodbye from HAL device\n"));
}


EXPORT TVOID
hal_beginio(struct THALBase *hal, struct TTimeRequest *req)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	TAPTR exec = hps->hsp_ExecBase;
	TTIME nowtime;
	TDOUBLE x = 0;

	switch (req->ttr_Req.io_Command)
	{
		/* execute asynchronously */
		case TTREQ_ADDLOCALDATE:
			x = hps->hsp_TZDays;					/* days west of GMT */
		case TTREQ_ADDUNIDATE:
			x += req->ttr_Data.ttr_Date.ttr_Date.tdt_Double;
			x -= 2440587.5;							/* days since 1.1.1970 */
			x *= 86400;								/* secs since 1.1.1970 */
			req->ttr_Data.ttr_Time.ttm_Sec = x;
			x -= req->ttr_Data.ttr_Time.ttm_Sec;	/* fraction of sec */
			x *= 1000000;
			req->ttr_Data.ttr_Time.ttm_USec = x;	/* microseconds */
			hal_getsystime(hal, &nowtime);
			hal_subtime(&req->ttr_Data.ttr_Time, &nowtime);
			/* relative time */
		case TTREQ_ADDTIME:
			TExecPutMsg(exec, TExecGetUserPort(exec, hps->hsp_DevTask),
				req->ttr_Req.io_ReplyPort, req);
			return;

		/* execute synchronously */
		default:
		case TTREQ_GETUNIDATE:
			hal_getsysdate(hal, &req->ttr_Data.ttr_Date.ttr_Date,
				&req->ttr_Data.ttr_Date.ttr_TimeZone);
			break;

		case TTREQ_GETLOCALDATE:
			hal_getsysdate(hal, &req->ttr_Data.ttr_Date.ttr_Date, TNULL);
			break;

		case TTREQ_GETTIME:
			hal_getsystime(hal, &req->ttr_Data.ttr_Time);
			break;
	}

	if (req->ttr_Req.io_Flags & TIOF_QUICK)
	{
		/* commit synchronous execution */
		req->ttr_Req.io_Flags &= ~TIOF_QUICK;
	}
	else
	{
		/* reply to self: fake asynchronous execution */
		TExecReplyMsg(exec, req);
	}
}

EXPORT TINT
hal_abortio(struct THALBase *hal, struct TTimeRequest *req)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	TAPTR exec = hps->hsp_ExecBase;
	TINT error = -1;

	TExecLock(exec, &hps->hsp_DevLock);

	if (TExecGetMsgStatus(exec, req) == (TMSG_STATUS_SENT|TMSGF_QUEUED))
	{
		TExecRemoveMsg(exec, req->ttr_Req.io_ReplyPort, req);
	}
	else
	{
		req = TNULL;
	}

	TExecUnlock(exec, &hps->hsp_DevLock);

	if (req)
	{
		TExecInsertMsg(exec, req->ttr_Req.io_ReplyPort,
			req, TNULL, TMSG_STATUS_REPLIED | TMSGF_QUEUED);
		req->ttr_Req.io_Error = TIOERR_ABORTED;
		error = 0;
	}

	return error;
}
#endif
