
/*
**	$Id: hal.c,v 1.7 2005/09/18 11:27:22 tmueller Exp $
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

#include "hal_mod.h"

#include <stdlib.h>
#include <string.h>
#include <tamtypes.h>
#include <kernel.h>

#define THREADSTACKSIZE		16384
#define STACKALIGNMASK		0x3fff

TAPTR *GetThreadPrivate(TUINT mask);
static TVOID TTASKENTRY haldevfunc(TAPTR task);
static TINT timerfunc(TINT ca);
static TVOID time_init(TVOID);
static TVOID do_gettimeofday(TTIME *tv);

extern void *_gp;

/*****************************************************************************/
/*
**	Host init
*/

LOCAL TBOOL
hal_init(TMOD_HAL *hal, TTAGITEM *tags)
{
	struct HALPS2Boot *boot = hal->hmb_BootHnd;
	struct HALPS2Specific *specific;
	
	specific = (*boot->hpb_Alloc)(boot, sizeof(struct HALPS2Specific));
	if (specific)
	{
		memset(specific, 0, sizeof(struct HALPS2Specific));
		
		specific->hps_SysDir = (TSTRPTR) TGetTag(tags,
			TExecBase_SysDir, (TTAG) TEKHOST_SYSDIR);
		specific->hps_ModDir = (TSTRPTR) TGetTag(tags,
			TExecBase_ModDir, (TTAG) TEKHOST_MODDIR);
		specific->hps_ProgDir = (TSTRPTR) TGetTag(tags,
			TExecBase_ProgDir, (TTAG) TEKHOST_PROGDIR);

		specific->hps_Tags[0].tti_Tag = TExecBase_SysDir;
		specific->hps_Tags[0].tti_Value = (TTAG) specific->hps_SysDir;

		specific->hps_Tags[1].tti_Tag = TExecBase_ModDir;
		specific->hps_Tags[1].tti_Value = (TTAG) specific->hps_ModDir;
		specific->hps_Tags[2].tti_Tag = TExecBase_ProgDir;
	
		specific->hps_Tags[2].tti_Value = (TTAG) specific->hps_ProgDir;
		specific->hps_Tags[3].tti_Tag = TTAG_DONE;
	
		hal->hmb_Specific = specific;
		specific->hps_TZSec = 0;
		specific->hps_TZDays = 0;
		time_init();

		/* init devlock */
		TINITLIST(&specific->hps_DevLock.tlk_Waiters);
		specific->hps_DevLock.tlk_Owner = TNULL;
		specific->hps_DevLock.tlk_NestCount = 0;
		specific->hps_DevLock.tlk_WaitCount = 0;

		return TTRUE;
	}
	return TFALSE;
}

LOCAL TVOID
hal_exit(TMOD_HAL *hal)
{
	struct HALPS2Specific *specific = hal->hmb_Specific;
	struct HALPS2Boot *boot = hal->hmb_BootHnd;
	(*boot->hpb_Free)(boot, specific, sizeof(struct HALPS2Specific));
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
hal_disable(TMOD_HAL *hal)
{
	struct HALPS2Specific *hps = hal->hmb_Specific;
	DI();
	++hps->hps_LockCount;
}

static TVOID
hal_enable(TMOD_HAL *hal)
{
	struct HALPS2Specific *hps = hal->hmb_Specific;
	if (--hps->hps_LockCount == 0)
	{
		EI();
	}
}

/*****************************************************************************/
/*
**	Memory
*/

EXPORT TAPTR
hal_alloc(TMOD_HAL *hal, TUINT size)
{
	struct HALPS2Boot *boot = hal->hmb_BootHnd;
	TAPTR mem;
	hal_disable(hal);
	mem = (*boot->hpb_Alloc)(boot, size);
	hal_enable(hal);
	return mem;
}

EXPORT TAPTR
hal_realloc(TMOD_HAL *hal, TAPTR mem, TUINT oldsize, TUINT newsize)
{
	struct HALPS2Boot *boot = hal->hmb_BootHnd;
	TAPTR newmem;
	hal_disable(hal);
	newmem = (*boot->hpb_Realloc)(boot, mem, oldsize, newsize);
	hal_enable(hal);
	return newmem;
}

EXPORT TVOID
hal_free(TMOD_HAL *hal, TAPTR mem, TUINT size)
{
	struct HALPS2Boot *boot = hal->hmb_BootHnd;
	hal_disable(hal);
	(*boot->hpb_Free)(boot, mem, size);
	hal_enable(hal);
}

EXPORT TVOID
hal_copymem(TMOD_HAL *hal, TAPTR from, TAPTR to, TUINT numbytes)
{
	memcpy(to, from, numbytes);
}

EXPORT TVOID
hal_fillmem(TMOD_HAL *hal, TAPTR dest, TUINT numbytes, TUINT8 fillval)
{
	memset(dest, (int) fillval, numbytes);
}

/*****************************************************************************/
/*
**	Locks
*/

EXPORT TBOOL
hal_initlock(TMOD_HAL *hal, THALO *lock)
{
	return TTRUE;
}

EXPORT TVOID
hal_destroylock(TMOD_HAL *hal, THALO *lock)
{
}

EXPORT TVOID
hal_lock(TMOD_HAL *hal, THALO *lock)
{
	hal_disable(hal);
}

EXPORT TVOID
hal_unlock(TMOD_HAL *hal, THALO *lock)
{
	hal_enable(hal);
}

/*****************************************************************************/
/*
**	Threads
*/

static void
threadentry(struct HALPS2Thread *thread)
{
	TMOD_HAL *hal = thread->hpt_HALBase;

	/* wait for initial signal to run */
	hal_wait(hal, TTASK_SIG_SINGLE);

	/* call function */
	(*thread->hpt_Function)(thread->hpt_Data);

	ExitThread();
}

EXPORT TBOOL 
hal_initthread(TMOD_HAL *hal, THALO *thread,
	TTASKENTRY TVOID (*function)(TAPTR task), TAPTR data)
{
	struct HALPS2Thread *t =
		THALNewObject(hal, thread, struct HALPS2Thread);
	if (t)
	{
		t->hpt_SigSemaphore.init_count = 0;
		t->hpt_SigSemaphore.max_count = 1;
		t->hpt_SigSID = CreateSema(&t->hpt_SigSemaphore);
		if (t->hpt_SigSID != -1)
		{
			t->hpt_SigState = 0;
			t->hpt_Function = function;
			t->hpt_Data = data;
			t->hpt_HALBase = hal;
			THALSetObject(thread, struct HALPS2Thread, t);
			if (function)
			{
				t->hpt_Stack = hal_alloc(hal, THREADSTACKSIZE + STACKALIGNMASK);
				if (t->hpt_Stack)
				{
					TUINT aligned = (TUINT) t->hpt_Stack;
					TUINT *stackp;
					aligned += STACKALIGNMASK;
					aligned &= ~STACKALIGNMASK;
					stackp = (TUINT *) aligned;
				
					/* place contextdata in stacktop */
					stackp[THREADSTACKSIZE/4-1] = (TUINT) t;
					t->hpt_TCB.func = threadentry;
					t->hpt_TCB.stack = stackp;
					t->hpt_TCB.stack_size = THREADSTACKSIZE - 16;
					t->hpt_TCB.gp_reg = &_gp;
					t->hpt_TCB.initial_priority = 1;
					
					t->hpt_TID = CreateThread(&t->hpt_TCB);
					
					if (t->hpt_TID != -1)
					{
						StartThread(t->hpt_TID, t);
						return TTRUE;
					}
					hal_free(hal, t->hpt_Stack, THREADSTACKSIZE + STACKALIGNMASK);
				}
			}
			else
			{
				/* place contextdata in stacktop */
				TAPTR *sptr = GetThreadPrivate(~STACKALIGNMASK);
				*sptr = t;
				t->hpt_TID = GetThreadId();
				return TTRUE;
			}
			DeleteSema(t->hpt_SigSID);
		}
		THALDestroyObject(hal, t, struct HALPS2Thread);
	}
	tdbprintf(20,"could not create thread\n");
	return TFALSE;
}

EXPORT TVOID
hal_destroythread(TMOD_HAL *hal, THALO *thread)
{
	struct HALPS2Thread *t = THALGetObject(thread, struct HALPS2Thread);
	if (t->hpt_Function)
	{
		/* !!!FIXME!!! */
		if (DeleteThread(t->hpt_TID) == -1)
		{
			tdbprintf(20,"could not destroy thread\n");
		}
		
		hal_free(hal, t->hpt_Stack, THREADSTACKSIZE + STACKALIGNMASK);
	}
	DeleteSema(t->hpt_SigSID);
	THALDestroyObject(hal, t, struct HALPS2Thread);
}

EXPORT TAPTR
hal_findself(TMOD_HAL *hal)
{
	struct HALPS2Thread *t = *GetThreadPrivate(~STACKALIGNMASK);
	return t->hpt_Data;
}

/*****************************************************************************/
/*
**	Signals
*/

EXPORT TVOID
hal_signal(TMOD_HAL *hal, THALO *thread, TUINT signals)
{
	struct HALPS2Thread *t = THALGetObject(thread, struct HALPS2Thread);
	hal_disable(hal);
	if (signals & ~t->hpt_SigState)
	{
		t->hpt_SigState |= signals;
		hal_enable(hal);
		WakeupThread(t->hpt_TID);
	}
	else
	{
		hal_enable(hal);
	}
}

EXPORT TUINT
hal_setsignal(TMOD_HAL *hal, TUINT newsig, TUINT sigmask)
{
	TUINT oldsig;
	struct HALPS2Thread *t = *GetThreadPrivate(~STACKALIGNMASK);

	hal_disable(hal);
	oldsig = t->hpt_SigState;
	t->hpt_SigState &= ~sigmask;
	t->hpt_SigState |= newsig;
	if ((newsig & sigmask) & ~oldsig)
	{
		hal_enable(hal);
		WakeupThread(t->hpt_TID);
	}
	else
	{
		hal_enable(hal);
	}
	return oldsig;
}

EXPORT TUINT
hal_wait(TMOD_HAL *hal, TUINT sigmask)
{
	TUINT sig;
	TAPTR *sptr = GetThreadPrivate(~STACKALIGNMASK);
	struct HALPS2Thread *t = *sptr;

	for (;;)
	{
		hal_disable(hal);
		sig = t->hpt_SigState & sigmask;
		t->hpt_SigState &= ~sigmask;
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
hal_timedwaitevent(TAPTR hal, struct HALPS2Thread *t, TTIME *wt, TUINT sigmask)
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
		sig = t->hpt_SigState & sigmask;
		t->hpt_SigState &= ~sigmask;
		hal_enable(hal);
		if (sig || sig_counter <= 0) break;
		sig_thread_ID = t->hpt_TID;
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
hal_getsystime(TMOD_HAL *hal, TTIME *time)
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
hal_getsysdate(TMOD_HAL *hal, TDATE *datep, TINT *tzsecp)
{	
	struct HALPS2Specific *hps = hal->hmb_Specific;
	TTIME tv;
	TDOUBLE syst;
	
	do_gettimeofday(&tv);

	syst = tv.ttm_USec;
	syst /= 1000000;
	
	syst += tv.ttm_Sec;
	
	if (tzsecp)
	{
		*tzsecp = hps->hps_TZSec;
	}
	else
	{
		syst -= hps->hps_TZSec;
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
**	Module open and entry
*/

EXPORT TAPTR
hal_loadmodule(TMOD_HAL *hal, TSTRPTR name, TUINT16 version, TUINT *psize,
	TUINT *nsize)
{
	tdbprintf1(20, "*** hal_loadmodule (%s) called\n", name);
	return TNULL;
}

EXPORT TBOOL
hal_callmodule(TMOD_HAL *hal, TAPTR mod, TAPTR task, TAPTR data)
{
	tdbfatal(99);
	return 0;
}

EXPORT TVOID
hal_unloadmodule(TMOD_HAL *hal, TAPTR halmod)
{
	tdbfatal(99);
}

EXPORT TBOOL
hal_scanmodules(TMOD_HAL *hal, TSTRPTR path,
	TCALLBACK TBOOL (*callb)(TAPTR, TSTRPTR, TINT), TAPTR userdata)
{
	return 1;
}

/*****************************************************************************/
/*
**	HAL device - instance open/close
*/

LOCAL TCALLBACK struct TTimeRequest *
hal_open(TMOD_HAL *hal, TAPTR task, TTAGITEM *tags)
{
	struct HALPS2Specific *hps = hal->hmb_Specific;
	TAPTR exec = (TAPTR) TGetTag(tags, THalBase_Exec, TNULL);
	struct TTimeRequest *req;
	hps->hps_ExecBase = exec;
	
	req = TExecAllocMsg0(exec, sizeof(struct TTimeRequest));
	if (req)
	{
		TExecLock(exec, &hps->hps_DevLock);
		
		if (!hps->hps_DevTask)
		{
			TTAGITEM tasktags[2];
			tasktags[0].tti_Tag = TTask_Name;			/* set task name */
			tasktags[0].tti_Value = (TTAG) TTASKNAME_HALDEV;
			tasktags[1].tti_Tag = TTAG_DONE;
			TInitList(&hps->hps_ReqList);
			hps->hps_DevTask = TExecCreateSysTask(exec, haldevfunc, tasktags);
		}

		if (hps->hps_DevTask)
		{
			hps->hps_RefCount++;
			req->ttr_Req.io_Device = (struct TModule *) hal;
		}
		else
		{	
			TExecFree(exec, req);
			req = TNULL;
		}

		TExecUnlock(exec, &hps->hps_DevLock);
	}

	return req;
}

/*****************************************************************************/

LOCAL TCALLBACK TVOID 
hal_close(TMOD_HAL *hal, TAPTR task)
{
	struct HALPS2Specific *hps = hal->hmb_Specific;
	
	TExecLock(hps->hps_ExecBase, &hps->hps_DevLock);
	
	if (hps->hps_DevTask)
	{
		if (--hps->hps_RefCount == 0)
		{
			tdbprintf(2,"hal.device refcount dropped to 0\n");
			TExecSignal(hps->hps_ExecBase, hps->hps_DevTask, TTASK_SIG_ABORT);
			tdbprintf(2,"destroy hal.device task...\n");
			TDestroy(hps->hps_DevTask);
			hps->hps_DevTask = TNULL;
		}
	}
	
	TExecUnlock(hps->hps_ExecBase, &hps->hps_DevLock);
}

/*****************************************************************************/

static TVOID TTASKENTRY 
haldevfunc(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_HAL *hal = TExecGetHALBase(exec);
	struct HALPS2Specific *hps = hal->hmb_Specific;
	struct HALPS2Thread *thread = *GetThreadPrivate(~STACKALIGNMASK);
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
			
			TExecLock(exec, &hps->hps_DevLock);
			node = hps->hps_ReqList.tlh_Head;
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
			TExecUnlock(exec, &hps->hps_DevLock);
			continue;
		}
		
		if (sig & TTASK_SIG_USER)
		{
			/* got message */
			TExecLock(exec, &hps->hps_DevLock);
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
				TAddTail(&hps->hps_ReqList, (struct TNode *) msg);
			}
			TExecUnlock(exec, &hps->hps_DevLock);
		}
	}
	
	tdbprintf(2,"goodbye from HAL device\n");
}


EXPORT TVOID
hal_beginio(TMOD_HAL *hal, struct TTimeRequest *req)
{
	struct HALPS2Specific *hps = hal->hmb_Specific;
	TAPTR exec = hps->hps_ExecBase;
	TTIME nowtime;
	TDOUBLE x = 0;
	
	switch (req->ttr_Req.io_Command)
	{
		/* execute asynchronously */
		case TTREQ_ADDLOCALDATE:
			x = hps->hps_TZDays;					/* days west of GMT */
		case TTREQ_ADDUNIDATE:
			x += req->ttr_Data.ttr_Date.ttr_Date.tdt_Day.tdtt_Double;
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
			TExecPutMsg(exec, TExecGetUserPort(exec, hps->hps_DevTask), 
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
hal_abortio(TMOD_HAL *hal, struct TTimeRequest *req)
{
	struct HALPS2Specific *hps = hal->hmb_Specific;
	TAPTR exec = hps->hps_ExecBase;
	TINT error = -1;

	TExecLock(exec, &hps->hps_DevLock);
	
	if (TExecGetMsgStatus(exec, req) == (TMSG_STATUS_SENT|TMSGF_QUEUED))
	{
		TExecRemoveMsg(exec, req->ttr_Req.io_ReplyPort, req);
	}
	else
	{
		req = TNULL;
	}

	TExecUnlock(exec, &hps->hps_DevLock);
	
	if (req)
	{
		TExecInsertMsg(exec, req->ttr_Req.io_ReplyPort,
			req, TNULL, TMSG_STATUS_REPLIED | TMSGF_QUEUED);
		req->ttr_Req.io_Error = TIOERR_ABORTED;
		error = 0;
	}

	return error;
}

/*****************************************************************************/

EXPORT TDOUBLE
hal_datetojulian(TMOD_HAL *hal, TDATE *date)
{
	return date->tdt_Day.tdtt_Double;
}

EXPORT TVOID
hal_juliantodate(TMOD_HAL *hal, TDOUBLE jd, TDATE *date)
{
	date->tdt_Day.tdtt_Double = jd;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: hal.c,v $
**	Revision 1.7  2005/09/18 11:27:22  tmueller
**	added authors
**	
**	Revision 1.6  2005/09/07 23:58:30  tmueller
**	added date to julian type conversion
**	
**	Revision 1.5  2005/04/24 17:33:12  fschulze
**	now uses symbolic names for timer registers
**	
**	Revision 1.4  2005/04/01 18:43:32  tmueller
**	Enable/Disable is now nesting; added boot handle to allocself; global
**	memory allocations are now performed using callbacks in the boot object
**	
**	Revision 1.3  2005/03/21 11:12:49  fschulze
**	cosmetic
**	
**	Revision 1.2  2005/03/19 19:25:09  fschulze
**	added decent device locks and fixed timer device
**	
**	Revision 1.1  2005/03/13 20:01:39  fschulze
**	added
**	
**
*/
