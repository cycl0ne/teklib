
/*
**	$Id: hal.c,v 1.8 2005/09/13 02:42:16 tmueller Exp $
**	teklib/mods/hal/amiga/hal.c - Amiga implementation of the HAL layer
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/proto/exec.h>
#include <tek/mod/amiga/hal.h>

#include "hal_mod.h"

#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/timer.h>
#include <dos/dostags.h>

#define SysBase *((struct ExecBase **) 4L)
#define DOSBase specific->has_DOSBase
#define LocaleBase specific->has_LocaleBase

static TVOID TTASKENTRY haldevfunc(TAPTR task);

/*****************************************************************************/

#ifdef TDEBUG
#define TRACKMEM
static int bytecount;
static int allocount;
static int maxbytecount;
#endif

/*****************************************************************************/
/*
**	host init
*/

LOCAL TBOOL
hal_init(TMOD_HAL *hal, TTAGITEM *tags)
{
	struct HALAmiSpecific *specific =
		AllocMem(sizeof(struct HALAmiSpecific), MEMF_CLEAR | MEMF_ANY);
	if (specific)
	{
		specific->has_DOSBase = (struct DosLibrary *)
			OpenLibrary("dos.library", 0);
		specific->has_LocaleBase = OpenLibrary("locale.library", 0);

		if (specific->has_DOSBase && specific->has_LocaleBase)
		{
			InitSemaphore(&specific->has_DevLock);

			specific->has_Locale = OpenLocale(NULL);
			specific->has_TZSec = specific->has_Locale->loc_GMTOffset * 60;
			specific->has_TZDays = specific->has_TZSec;
			specific->has_TZDays /= 86400;

			specific->has_SysDir = (TSTRPTR) TGetTag(tags,
				TExecBase_SysDir, (TTAG) TEKHOST_SYSDIR);
			specific->has_ModDir = (TSTRPTR) TGetTag(tags,
				TExecBase_ModDir, (TTAG) TEKHOST_MODDIR);
			specific->has_ProgDir = (TSTRPTR) TGetTag(tags,
				TExecBase_ProgDir, (TTAG) TEKHOST_PROGDIR);

			specific->has_Tags[0].tti_Tag = TExecBase_SysDir;
			specific->has_Tags[0].tti_Value = (TTAG) specific->has_SysDir;
			specific->has_Tags[1].tti_Tag = TExecBase_ModDir;
			specific->has_Tags[1].tti_Value = (TTAG) specific->has_ModDir;
			specific->has_Tags[2].tti_Tag = TExecBase_ProgDir;
			specific->has_Tags[2].tti_Value = (TTAG) specific->has_ProgDir;
			specific->has_Tags[3].tti_Tag = TTAG_DONE;

			hal->hmb_Specific = specific;

			#ifdef TRACKMEM
			allocount = 0;
			bytecount = 0;
			maxbytecount = 0;
			#endif

			return TTRUE;
		}

		CloseLibrary(specific->has_LocaleBase);
		CloseLibrary((struct Library *) specific->has_DOSBase);
		FreeMem(specific, sizeof(struct HALAmiSpecific));
	}
	return TFALSE;
}

LOCAL TVOID 
hal_exit(TMOD_HAL *hal)
{
	struct HALAmiSpecific *specific = hal->hmb_Specific;
	CloseLocale(specific->has_Locale);
	CloseLibrary(specific->has_LocaleBase);
	CloseLibrary((struct Library *) specific->has_DOSBase);
	FreeMem(specific, sizeof(struct HALAmiSpecific));

	#ifdef TRACKMEM
	if (allocount || bytecount)
	{
		tdbprintf2(10,"*** Global memory leak: %ld allocs, %ld bytes pending\n",
			allocount, bytecount);
	}
	tdbprintf1(5,"*** Peak memory allocated: %ld bytes\n", maxbytecount);
	#endif
}

LOCAL TAPTR 
hal_allocself(TAPTR handle, TUINT size)
{
	return AllocMem(size, MEMF_ANY);
}

LOCAL TVOID 
hal_freeself(TAPTR handle, TAPTR mem, TUINT size)
{
	FreeMem(mem, size);
}

/*****************************************************************************/
/*
**	Memory
*/

EXPORT TAPTR
hal_alloc(TMOD_HAL *hal, TUINT size)
{
	TAPTR mem = AllocMem(size, MEMF_ANY);
	#ifdef TRACKMEM	
	if (mem)
	{
		Forbid();
		allocount++;
		bytecount += size;
		if (bytecount > maxbytecount) maxbytecount = bytecount;
		Permit();
	}
	#endif
	return mem;
}

EXPORT TVOID
hal_free(TMOD_HAL *hal, TAPTR mem, TUINT size)
{
	#ifdef TRACKMEM	
	if (mem)
	{
		Forbid();
		allocount--;
		bytecount -= size;
		Permit();
	}
	#endif	
	FreeMem(mem, size);
}

EXPORT TAPTR
hal_realloc(TMOD_HAL *hal, TAPTR oldmem, TUINT oldsize, TUINT newsize)
{
	if (newsize)
	{
		if (oldmem)
		{
			TAPTR newmem = hal_alloc(hal, newsize);
			if (newmem)
			{
				hal_copymem(hal, oldmem, newmem, TMIN(oldsize, newsize));
				hal_free(hal, oldmem, oldsize);
				return newmem;
			}
		}
		else
		{
			return hal_alloc(hal, newsize);
		}
	}
	else
	{
		if (oldmem)
		{
			hal_free(hal, oldmem, oldsize);
		}
	}
	return TNULL;
}

EXPORT TVOID
hal_copymem(TMOD_HAL *hal, TAPTR from, TAPTR to, TUINT numbytes)
{
	CopyMem(from, to, (ULONG) numbytes);
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
	struct SignalSemaphore *sem = 
		THALNewObject(hal, lock, struct SignalSemaphore);
	if (sem)
	{
		memset(sem, 0, sizeof(struct SignalSemaphore));
		InitSemaphore(sem);
		THALSetObject(lock, struct SignalSemaphore, sem);
		return TTRUE;
	}
	tdbprintf(20,"could not create lock\n");
	return TFALSE;
}

EXPORT TVOID
hal_destroylock(TMOD_HAL *hal, THALO *lock)
{
	struct SignalSemaphore *sem = THALGetObject(lock, struct SignalSemaphore);
	THALDestroyObject(hal, sem, struct SignalSemaphore);
}

EXPORT TVOID
hal_lock(TMOD_HAL *hal, THALO *lock)
{
	//struct SignalSemaphore *sem = THALGetObject(lock, struct SignalSemaphore);
	//ObtainSemaphore(sem);
	Forbid();
}

EXPORT TVOID
hal_unlock(TMOD_HAL *hal, THALO *lock)
{
	//struct SignalSemaphore *sem = THALGetObject(lock, struct SignalSemaphore);
	//ReleaseSemaphore(sem);
	Permit();
}

/*****************************************************************************/

static TBOOL
hal_inittimer(TMOD_HAL *hal, struct HALAmiThread *t)
{
	t->hat_TimePort = CreateMsgPort();
	if (t->hat_TimePort)
	{
		t->hat_TimeReq = (struct timerequest *)
			CreateIORequest(t->hat_TimePort, sizeof(struct timerequest));
		if (t->hat_TimeReq)
		{
			if (OpenDevice("timer.device", UNIT_MICROHZ,
				(struct IORequest *) t->hat_TimeReq, 0) == 0)
			{
				return TTRUE;
			}
			DeleteIORequest(t->hat_TimeReq);
		}
		DeleteMsgPort(t->hat_TimePort);
	}
	tdbprintf(20,"creation of task timer failed\n");
	return TFALSE;
}

static TVOID
hal_destroytimer(TMOD_HAL *hal, struct HALAmiThread *t)
{
	CloseDevice((struct IORequest *) t->hat_TimeReq);
	DeleteIORequest(t->hat_TimeReq);
	DeleteMsgPort(t->hat_TimePort);
}

/*****************************************************************************/
/*
**	Threads
*/

#ifdef __SASC
static __asm __saveds void 
#else
static void
#endif
amithread_entry(void)
{
	struct Task *self; 
	struct MsgPort *initport;
	struct HALAmiThread *t;
	TMOD_HAL *hal;
	struct Message *waiter;

	/* get process message port */
	self = FindTask(NULL);
	initport = &((struct Process *) self)->pr_MsgPort;

	/* wait for init package */
	WaitPort(initport);
	t = (struct HALAmiThread *) GetMsg(initport);
	hal = t->hat_HALBase;

	/* get signal */
	t->hat_SigEvent = AllocSignal(-1);
	if (t->hat_SigEvent >= 0)
	{
		/* get initport */
		t->hat_InitExitPort = CreateMsgPort();
		if (t->hat_InitExitPort)
		{
			/* get timer */
			if (hal_inittimer(hal, t))
			{
				/* insert self context */
				self->tc_UserData = t;
				t->hat_TaskState = 1;

				/* confirm init packet */
				ReplyMsg((struct Message *) t);
		
				/* wait for initial signal to run */
				hal_wait(hal, TTASK_SIG_SINGLE);

				/* call user function */		
				(*t->hat_Function)(t->hat_Data);

				/* closedown */
				hal_destroytimer(hal, t);

				/* wait for sync-on-exit message */	
				WaitPort(initport);
				waiter = (struct Message *) GetMsg(initport);

				/* ensure synchronization */
				Forbid();

				/* reply sync-on-exit message */
				ReplyMsg(waiter);
			}
			DeleteMsgPort(t->hat_InitExitPort);
		}
		FreeSignal(t->hat_SigEvent);
	}
	
	if (!t->hat_TaskState)
	{
		/* confirm init packet with failure */
		Forbid();
		ReplyMsg((struct Message *) t);
	}
}

EXPORT TVOID
hal_destroythread(TMOD_HAL *hal, THALO *thread)
{
	struct HALAmiThread *t = THALGetObject(thread, struct HALAmiThread);

	if (t->hat_Function)
	{
		struct HALAmiThread *self = FindTask(NULL)->tc_UserData;
		self->hat_Message.mn_ReplyPort = self->hat_InitExitPort;
		PutMsg(&((struct Process *) t->hat_AmiTask)->pr_MsgPort,
			(struct Message *) self);
		WaitPort(self->hat_InitExitPort);
		GetMsg(self->hat_InitExitPort);
	}
	else
	{
		DeleteMsgPort(t->hat_InitExitPort);	
		FreeSignal(t->hat_SigEvent);
		hal_destroytimer(hal, t);
	}

	THALDestroyObject(hal, t, struct HALAmiThread);
}

EXPORT TBOOL
hal_initthread(TMOD_HAL *hal, THALO *thread, 
	TTASKENTRY TVOID (*function)(TAPTR task), TAPTR data)
{
	struct HALAmiThread *t = THALNewObject(hal, thread, struct HALAmiThread);
	if (t)
	{
		struct HALAmiSpecific *specific = hal->hmb_Specific;

		memset(t, 0, sizeof(struct HALAmiThread));
		t->hat_Function = function;
		t->hat_Data = data;
		t->hat_HALBase = hal;
		t->hat_SigState = 0;
		InitSemaphore(&t->hat_SigLock);

		THALSetObject(thread, struct HALAmiThread, t);
	
		if (function)
		{
			struct Process *newproc;
			struct TagItem cnptags[3];
			cnptags[0].ti_Tag = NP_Entry;
			cnptags[0].ti_Data = (ULONG) amithread_entry;
	
			#ifdef TSYS_MORPHOS
				cnptags[1].ti_Tag = NP_CodeType;
				cnptags[1].ti_Data = CODETYPE_PPC;
				cnptags[2].ti_Tag = TAG_DONE;
			#else
				cnptags[1].ti_Tag = TAG_DONE;
			#endif
	
			newproc = CreateNewProc(cnptags);
			if (newproc)
			{
				struct HALAmiThread *self = FindTask(NULL)->tc_UserData;
				struct MsgPort *initport = self->hat_InitExitPort;
				t->hat_AmiTask = &newproc->pr_Task;
				t->hat_Message.mn_ReplyPort = initport;
				t->hat_Message.mn_Length = sizeof(struct HALAmiThread);
				PutMsg(&newproc->pr_MsgPort, (struct Message *) t);
				WaitPort(initport);
				GetMsg(initport);
				if (t->hat_TaskState) return TTRUE;
				/* else initialization failed, thread is gone */
			}
		}
		else
		{
			/* init self context */
			if (hal_inittimer(hal, t))
			{
				t->hat_SigEvent = AllocSignal(-1);
				if (t->hat_SigEvent >= 0)
				{
					t->hat_InitExitPort = CreateMsgPort();
					if (t->hat_InitExitPort)
					{
						t->hat_AmiTask = FindTask(NULL);
						t->hat_AmiTask->tc_UserData = t;
						return TTRUE;
					}
					FreeSignal(t->hat_SigEvent);
				}
				hal_destroytimer(hal, t);
			}
		}
		
		THALDestroyObject(hal, t, struct HALAmiThread);
	}
	tdbprintf(20,"could not create thread\n");
	return TFALSE;
}

EXPORT TAPTR
hal_findself(TMOD_HAL *hal)
{
	struct HALAmiThread *ath = FindTask(NULL)->tc_UserData;
	return ath->hat_Data;
}

/*****************************************************************************/
/*
**	Signals
*/

EXPORT TVOID
hal_signal(TMOD_HAL *hal, THALO *thread, TUINT signals)
{
	struct HALAmiThread *t = THALGetObject(thread, struct HALAmiThread);
	//ObtainSemaphore(&t->hat_SigLock);
	Forbid();
	if (signals & ~t->hat_SigState)
	{
		t->hat_SigState |= signals;
		Signal(t->hat_AmiTask, 1L << t->hat_SigEvent);
	}
	Permit();
	//ReleaseSemaphore(&t->hat_SigLock);
}

EXPORT TUINT
hal_setsignal(TMOD_HAL *hal, TUINT newsig, TUINT sigmask)
{
	struct HALAmiThread *t = FindTask(NULL)->tc_UserData;
	TUINT oldsig;
	
	//ObtainSemaphore(&t->hat_SigLock);
	Forbid();
	oldsig = t->hat_SigState;
	t->hat_SigState &= ~sigmask;
	t->hat_SigState |= newsig;
	if ((newsig & sigmask) & ~oldsig)
	{
		Signal(t->hat_AmiTask, 1L << t->hat_SigEvent);
	}
	Permit();
	//ReleaseSemaphore(&t->hat_SigLock);
	
	return oldsig;
}

EXPORT TUINT
hal_wait(TMOD_HAL *hal, TUINT sigmask)
{
	struct HALAmiThread *t = FindTask(NULL)->tc_UserData;
	TUINT sig;

	for (;;)
	{
		//ObtainSemaphore(&t->hat_SigLock);
		Forbid();
		sig = t->hat_SigState & sigmask;
		t->hat_SigState &= ~sigmask;
		Permit();
		//ReleaseSemaphore(&t->hat_SigLock);
		if (sig) break;
		Wait(1L << t->hat_SigEvent);
	}

	return sig;
}

static TUINT
hal_timedwaitevent(TMOD_HAL *hal, struct HALAmiThread *t, TTIME *wt,
	TUINT sigmask)
{
	TUINT timesig;
	TTIME waitt, curt;
	TUINT sig;

	hal_getsystime(hal, &curt);

	waitt = *wt;
	hal_subtime(&waitt, &curt);
	if (waitt.ttm_Sec < 0) return 0;

	t->hat_TimeReq->tr_node.io_Command = TR_ADDREQUEST;
	t->hat_TimeReq->tr_time.tv_secs = waitt.ttm_Sec;
	t->hat_TimeReq->tr_time.tv_micro = waitt.ttm_USec;
	SendIO((struct IORequest *) t->hat_TimeReq);

	timesig = 1L << t->hat_TimePort->mp_SigBit;

	for (;;)
	{
		//ObtainSemaphore(&t->hat_SigLock);
		Forbid();
		sig = t->hat_SigState & sigmask;
		t->hat_SigState &= ~sigmask;
		Permit();
		//ReleaseSemaphore(&t->hat_SigLock);
		if (sig) break;
		if (Wait((1L << t->hat_SigEvent) | timesig) & timesig) break;
	}

	AbortIO((struct IORequest *) t->hat_TimeReq);
	WaitIO((struct IORequest *) t->hat_TimeReq);
	SetSignal(0, timesig);
	
	return sig;
}

/*****************************************************************************/
/*
**	Time and date
*/

EXPORT TVOID
hal_getsystime(TMOD_HAL *hal, TTIME *time)
{
	struct HALAmiThread *t = FindTask(NULL)->tc_UserData;
	#define TimerBase t->hat_TimeReq->tr_node.io_Device
	GetSysTime((struct timeval *) time);
	#undef TimerBase
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
	struct HALAmiSpecific *specific = hal->hmb_Specific;
	TDOUBLE syst;
	TTIME curt;

	hal_getsystime(hal, &curt);
	
	syst = curt.ttm_USec;
	syst /= 1000000;
	syst += curt.ttm_Sec;
	
	if (tzsecp)
	{
		*tzsecp = specific->has_TZSec;
		syst += specific->has_TZSec;
	}

	if (datep)
	{
		syst /= 86400;			/* secs -> days */
		syst += 2443509.5;		/* 1.1.1978 in Julian days */
		datep->tdt_Day.tdtt_Double = syst;
	}

	return 0;
}

/*****************************************************************************/
/*
**	Modules
*/

static TSTRPTR 
getmodpathname(TSTRPTR path, TSTRPTR extra, TSTRPTR modname)
{
	TSTRPTR modpath;
	TINT l = strlen(path) + strlen(modname) + TEKHOST_EXTLEN + 1;

	if (extra) l += strlen(extra);

	modpath = AllocVec(l, MEMF_ANY);
	if (modpath)
	{
		strcpy(modpath, path);
		if (extra) strcat(modpath, extra);
		strcat(modpath, modname);
		strcat(modpath, TEKHOST_EXTSTR);
	}
	return modpath;
}

static TSTRPTR 
getmodpath(TSTRPTR path, TSTRPTR name)
{
	int l = strlen(path);
	char *p = AllocVec(l + strlen(name) + 1, MEMF_ANY);
	if (p)
	{
		strcpy(p, path);
		strcpy(p + l, name);
	}
	return p;
}

static TBOOL 
scanpathtolist(TMOD_HAL *hal, TCALLBACK TBOOL (*callb)(TAPTR, TSTRPTR, TINT),
	TAPTR userdata, TSTRPTR path, TSTRPTR name)
{
	struct HALAmiSpecific *specific = hal->hmb_Specific;

	TBOOL success = TTRUE;
	BPTR lock = Lock(path, ACCESS_READ);
	if (lock)
	{
		struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, TNULL);
		if (fib)
		{
			if (Examine(lock, fib) && (fib->fib_DirEntryType > 0))
			{
				TINT l, pl = strlen(name);
				while (success && ExNext(lock, fib))
				{
					if (fib->fib_DirEntryType >= 0) continue;
					l = strlen(fib->fib_FileName);
					if (l < pl + TEKHOST_EXTLEN) continue;
					if (strcmp(fib->fib_FileName + l - TEKHOST_EXTLEN, 
						TEKHOST_EXTSTR)) continue;
					if (strncmp(fib->fib_FileName, name, pl)) continue;
					success = (*callb)(userdata, fib->fib_FileName, l - 
						TEKHOST_EXTLEN);
				}
			}
			FreeDosObject(DOS_FIB, fib);
		}
		UnLock(lock);
	}

	return success;
}

EXPORT TAPTR 
hal_loadmodule(TMOD_HAL *hal, TSTRPTR name, TUINT16 version, TUINT *psize,
	TUINT *nsize)
{
	struct HALAmiMod *halmod = hal_alloc(hal, sizeof(struct HALAmiMod));
	if (halmod)
	{
		TSTRPTR modpath;
		struct HALAmiSpecific *specific = hal->hmb_Specific;

		halmod->ham_Lib = TNULL;
		modpath = getmodpathname(specific->has_ProgDir, "mod/", name);
		if (modpath)
		{
			tdbprintf1(5,"trying %s\n", modpath);
			halmod->ham_Lib = OpenLibrary(modpath, 0);
			if (!halmod->ham_Lib)
			{
				FreeVec(modpath);
				modpath = getmodpathname(specific->has_ModDir, TNULL, name);
				if (modpath)
				{
					tdbprintf1(5,"trying %s\n", modpath);
					halmod->ham_Lib = OpenLibrary(modpath, 0);
				}
			}
			FreeVec(modpath);
		}

		if (halmod->ham_Lib)
		{
			#define ModBase halmod->ham_Lib
	
			*psize = tek_mod_enter(TNULL, TNULL, version, TNULL);
			if (*psize)
			{
				*nsize = tek_mod_enter(TNULL, TNULL, 0xffff, TNULL);
				halmod->ham_Version = version;
				return (TAPTR) halmod;

			} else tdbprintf1(10,"module %s returned no size or error\n",name);
	
			#undef ModBase
	
			CloseLibrary(halmod->ham_Lib);

		} else tdbprintf1(10,"could not open module %s\n", name);
		
		hal_free(hal, halmod, sizeof(struct HALAmiMod));
	}
	return TNULL;
}

EXPORT TBOOL 
hal_callmodule(TMOD_HAL *hal, TAPTR knmod, TAPTR task, TAPTR mod)
{
	struct HALAmiMod *ham = knmod;
	#define ModBase ham->ham_Lib
	return (TBOOL) tek_mod_enter(task, mod, ham->ham_Version, TNULL);
	#undef ModBase
}

EXPORT TVOID
hal_unloadmodule(TMOD_HAL *hal, TAPTR knmod)
{
	struct HALAmiMod *ham = knmod;
	CloseLibrary(ham->ham_Lib);
	hal_free(hal, ham, sizeof(struct HALAmiMod));
	tdbprintf(5,"module unloaded\n");
}

EXPORT TBOOL
hal_scanmodules(TMOD_HAL *hal, TSTRPTR path, 
	TCALLBACK TBOOL (*callb)(TAPTR, TSTRPTR, TINT), TAPTR userdata)
{
	struct HALAmiSpecific *specific = hal->hmb_Specific;
	TSTRPTR p;
	TBOOL success = TFALSE;

	p = getmodpath(specific->has_ProgDir, "mod/");
	if (p)
	{
		success = scanpathtolist(hal, callb, userdata, p, path);
		FreeVec(p);
	}
	
	if (success)
	{
		p = getmodpath(specific->has_ModDir, "");
		if (p)
		{
			success = scanpathtolist(hal, callb, userdata, p, path);
			FreeVec(p);
		}
	}
	return success;
}

/*****************************************************************************/
/*
**	HAL device - instance open/close
*/

LOCAL TCALLBACK struct TTimeRequest *
hal_open(TMOD_HAL *hal, TAPTR task, TTAGITEM *tags)
{
	struct HALAmiSpecific *specific = hal->hmb_Specific;
	TAPTR exec = (TAPTR) TGetTag(tags, THalBase_Exec, TNULL);
	struct TTimeRequest *req;
	specific->has_ExecBase = exec;
	
	req = TExecAllocMsg0(exec, sizeof(struct TTimeRequest));
	if (req)
	{
		ObtainSemaphore(&specific->has_DevLock);
		
		if (!specific->has_DevTask)
		{
			TTAGITEM tasktags[2];
			tasktags[0].tti_Tag = TTask_Name;			/* set task name */
			tasktags[0].tti_Value = (TTAG) TTASKNAME_HALDEV;
			tasktags[1].tti_Tag = TTAG_DONE;
			TInitList(&specific->has_ReqList);
			specific->has_DevTask = TExecCreateSysTask(exec, haldevfunc,
				tasktags);
		}

		if (specific->has_DevTask)
		{
			specific->has_RefCount++;
			req->ttr_Req.io_Device = (struct TModule *) hal;
		}
		else
		{	
			TExecFree(exec, req);
			req = TNULL;
		}

		ReleaseSemaphore(&specific->has_DevLock);
	}

	return req;
}

/*****************************************************************************/

LOCAL TCALLBACK TVOID 
hal_close(TMOD_HAL *hal, TAPTR task)
{
	struct HALAmiSpecific *specific = hal->hmb_Specific;
	ObtainSemaphore(&specific->has_DevLock);
	if (specific->has_DevTask)
	{
		if (--specific->has_RefCount == 0)
		{
			tdbprintf(2,"hal.device refcount dropped to 0\n");
			TExecSignal(specific->has_ExecBase, specific->has_DevTask, 
				TTASK_SIG_ABORT);
			tdbprintf(2,"destroy hal.device task...\n");
			TDestroy(specific->has_DevTask);
			specific->has_DevTask = TNULL;
		}
	}
	ReleaseSemaphore(&specific->has_DevLock);
}

/*****************************************************************************/

static TVOID TTASKENTRY 
haldevfunc(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_HAL *hal = TExecGetHALBase(exec);
	struct HALAmiSpecific *specific = hal->hmb_Specific;
	struct HALAmiThread *thread = FindTask(NULL)->tc_UserData;
	TAPTR port = TExecGetUserPort(exec, task);
	struct TTimeRequest *msg;
	TUINT sig;
	TTIME waittime, curtime;
	
	tdbprintf(5,"hal.device thread created\n");
	
	waittime.ttm_Sec = 2000000000;
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
			
			//ObtainSemaphore(&specific->has_DevLock);
			Forbid();
			node = specific->has_ReqList.tlh_Head;
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
			Permit();
			//ReleaseSemaphore(&specific->has_DevLock);
			continue;
		}
		
		if (sig & TTASK_SIG_USER)
		{
			/* got message */
			//ObtainSemaphore(&specific->has_DevLock);
			Forbid();
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
				TAddTail(&specific->has_ReqList, (struct TNode *) msg);
			}
			Permit();
			//ReleaseSemaphore(&specific->has_DevLock);
		}
	}

	tdbprintf(5,"goodbye from HAL device\n");
}

EXPORT TVOID 
hal_beginio(TMOD_HAL *hal, struct TTimeRequest *req)
{
	struct HALAmiSpecific *specific = hal->hmb_Specific;
	TAPTR exec = specific->has_ExecBase;
	TTIME nowtime;
	TDOUBLE x = 0;

	switch (req->ttr_Req.io_Command)
	{
		/* execute asynchronously */
		case TTREQ_ADDLOCALDATE:
			x = specific->has_TZDays;				/* days west of GMT */
		case TTREQ_ADDUNIDATE:
			x += req->ttr_Data.ttr_Date.ttr_Date.tdt_Day.tdtt_Double;
			x -= specific->has_TZDays;				/* Amiga time is local */
			x -= 2443509.5;							/* days since 1.1.1978 */
			x *= 86400;								/* secs since 1.1.1978 */
			req->ttr_Data.ttr_Time.ttm_Sec = x;
			x -= req->ttr_Data.ttr_Time.ttm_Sec;	/* fraction of sec */
			x *= 1000000;
			req->ttr_Data.ttr_Time.ttm_USec = x;	/* microseconds */
			hal_getsystime(hal, &nowtime);
			hal_subtime(&req->ttr_Data.ttr_Time, &nowtime);
			/* relative time */
		case TTREQ_ADDTIME:
			TExecPutMsg(exec, TExecGetUserPort(exec, specific->has_DevTask), 
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
hal_abortio(TMOD_HAL *hal, struct TTimeRequest *req)
{
	struct HALAmiSpecific *specific = hal->hmb_Specific;
	TAPTR exec = specific->has_ExecBase;
	TINT error = -1;

	//ObtainSemaphore(&specific->has_DevLock);
	Forbid();

	if (TExecGetMsgStatus(exec, req) == (TMSG_STATUS_SENT|TMSGF_QUEUED))
	{
		TExecRemoveMsg(exec, req->ttr_Req.io_ReplyPort, req);
	}
	else
	{
		req = TNULL;
	}

	Permit();
	//ReleaseSemaphore(&specific->has_DevLock);

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

#undef SysBase
#undef DOSBase
#undef LocaleBase

/*****************************************************************************/
/*
**	Revision History
**	$Log: hal.c,v $
**	Revision 1.8  2005/09/13 02:42:16  tmueller
**	updated copyright reference
**	
**	Revision 1.7  2005/09/07 23:58:30  tmueller
**	added date to julian type conversion
**	
**	Revision 1.6  2005/04/01 18:40:26  tmueller
**	additional boot argument to hal_allocself
**	
**	Revision 1.5  2004/07/05 21:51:42  tmueller
**	minor compiler glitches fixed
**	
**	Revision 1.4  2004/04/18 14:11:01  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.3  2004/02/07 05:06:04  tmueller
**	Internal time support functions used; TIOF_QUICK no longer cleared
**	
**	Revision 1.2  2003/12/20 13:58:26  tmueller
**	Fixed waiting for an absolute date
**	
**	Revision 1.1.1.1  2003/12/11 07:18:52  tmueller
**	Krypton import
*/
