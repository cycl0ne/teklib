
/*
**	$Id: hal.c,v 1.11 2005/09/13 02:42:16 tmueller Exp $
**	teklib/mods/hal/win32/hal.c - Windows implementation of the HAL layer
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "../hal_mod.h"

#include <math.h>
#include <tek/debug.h>
#include <tek/proto/exec.h>
#include <tek/mod/win32/hal.h>

static TVOID TTASKENTRY haldevfunc(TAPTR task);

/*****************************************************************************/
/*
**	Host Init
*/

LOCAL TBOOL
hal_init(TMOD_HAL *hal, TTAGITEM *tags)
{
	struct HALWinSpecific *specific;
	specific = malloc(sizeof(struct HALWinSpecific));
	if (specific)
	{
		memset(specific, 0, sizeof(struct HALWinSpecific));
		specific->hws_TLSIndex = TlsAlloc();
		if (specific->hws_TLSIndex != 0xffffffff)
		{
			TIME_ZONE_INFORMATION tzi;

			InitializeCriticalSection(&specific->hws_DevLock);
	
			specific->hws_SysDir =
				(TSTRPTR) TGetTag(tags, TExecBase_SysDir, TNULL);
			specific->hws_ModDir =
				(TSTRPTR) TGetTag(tags, TExecBase_ModDir, TNULL);
			specific->hws_ProgDir =
				(TSTRPTR) TGetTag(tags, TExecBase_ProgDir, TNULL);
	
			specific->hws_Tags[0].tti_Tag = TExecBase_SysDir;
			specific->hws_Tags[0].tti_Value = (TTAG) specific->hws_SysDir;
			specific->hws_Tags[1].tti_Tag = TExecBase_ModDir;
			specific->hws_Tags[1].tti_Value = (TTAG) specific->hws_ModDir;
			specific->hws_Tags[2].tti_Tag = TExecBase_ProgDir;
			specific->hws_Tags[2].tti_Value = (TTAG) specific->hws_ProgDir;
			specific->hws_Tags[3].tti_Tag = TTAG_DONE;

			/* get timezone bias */			
			GetTimeZoneInformation(&tzi);
			specific->hws_TZSec = tzi.Bias * 60;
			specific->hws_TZDays = (TFLOAT) specific->hws_TZSec;
			specific->hws_TZDays /= 86400;

			/* get performance counter frequency, if available */
			specific->hws_UsePerfCounter =
				QueryPerformanceFrequency((LARGE_INTEGER *)
					&specific->hws_PerfCFreq);
					
			if (specific->hws_UsePerfCounter)
			{
				LARGE_INTEGER filet;
				TDOUBLE t;

				/* get performance counter start */
				QueryPerformanceCounter((LARGE_INTEGER *)	
					&specific->hws_PerfCStart);

				/* "calibrate" perfcounter to 1.1.1970 */
				GetSystemTimeAsFileTime((LPFILETIME) &filet);
				t = (TDOUBLE) filet.QuadPart;
				t /= 10000000;		/* seconds */
				t /= 86400;			/* days */
				t -= 134774;		/* 1.1.1601 -> 1.1.1970 */
				t *= 86400;			/* seconds */
				t *= 1000000;		/* microseconds */
				specific->hws_PerfCStartDate = t;
			}

			hal->hmb_Specific = specific;
			
			return TTRUE;
		}
		free(specific);
	}
	return TFALSE;
}

LOCAL TVOID
hal_exit(TMOD_HAL *hal)
{
	struct HALWinSpecific *specific = hal->hmb_Specific;
	DeleteCriticalSection(&specific->hws_DevLock);
	TlsFree(specific->hws_TLSIndex);
	free(specific);
}

LOCAL TAPTR
hal_allocself(TAPTR boot, TUINT size)
{
	return malloc(size);
}

LOCAL TVOID
hal_freeself(TAPTR boot, TAPTR mem, TUINT size)
{
	free(mem);
}

/*****************************************************************************/
/*
**	Memory
*/

EXPORT TAPTR
hal_alloc(TMOD_HAL *hal, TUINT size)
{
	return malloc(size);
}

EXPORT TVOID
hal_free(TMOD_HAL *hal, TAPTR mem, TUINT size)
{
	free(mem);
}

EXPORT TAPTR
hal_realloc(TMOD_HAL *hal, TAPTR oldmem, TUINT oldsize, TUINT newsize)
{
	return realloc(oldmem, newsize);
}

EXPORT TVOID
hal_copymem(TMOD_HAL *hal, TAPTR from, TAPTR to, TUINT numbytes)
{
	CopyMemory(to, from, numbytes);
}

EXPORT TVOID
hal_fillmem(TMOD_HAL *hal, TAPTR dest, TUINT numbytes, TUINT8 fillval)
{
	FillMemory(dest, numbytes, fillval);
}

/*****************************************************************************/
/*
**	Locking
*/

EXPORT TBOOL
hal_initlock(TMOD_HAL *hal, THALO *lock)
{
	CRITICAL_SECTION *cs = THALNewObject(hal, lock, CRITICAL_SECTION);
	if (cs)
	{
		InitializeCriticalSection(cs);
		THALSetObject(lock, CRITICAL_SECTION, cs);
		return TTRUE;
	}
	tdbprintf(20,"could not create lock\n");
	return TFALSE;
}

EXPORT TVOID
hal_destroylock(TMOD_HAL *hal, THALO *lock)
{
	CRITICAL_SECTION *cs = THALGetObject(lock, CRITICAL_SECTION);
	DeleteCriticalSection(cs);	
	THALDestroyObject(hal, cs, CRITICAL_SECTION);
}

EXPORT TVOID
hal_lock(TMOD_HAL *hal, THALO *lock)
{
	CRITICAL_SECTION *cs;
	cs = THALGetObject(lock, CRITICAL_SECTION);
	EnterCriticalSection(cs);
}

EXPORT TVOID
hal_unlock(TMOD_HAL *hal, THALO *lock)
{
	CRITICAL_SECTION *cs = THALGetObject(lock, CRITICAL_SECTION);
	LeaveCriticalSection(cs);
}

/*****************************************************************************/
/*
**	Threads
*/

static unsigned _stdcall 
hal_win32thread_entry(void *t)
{
	struct HALWinThread *wth = (struct HALWinThread *) t;
	TMOD_HAL *hal = wth->hwt_HALBase;
	struct HALWinSpecific *hws = hal->hmb_Specific;

	TlsSetValue(hws->hws_TLSIndex, t);

	hal_wait(hal, TTASK_SIG_SINGLE);

	(*wth->hwt_Function)(wth->hwt_Data);

	_endthreadex(0);
	return 0;
}

EXPORT TBOOL 
hal_initthread(TMOD_HAL *hal, THALO *thread, 
	TTASKENTRY TVOID (*function)(TAPTR task), TAPTR data)
{
	struct HALWinThread *wth = THALNewObject(hal, thread, struct HALWinThread);
	if (wth)
	{
		wth->hwt_SigEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (wth->hwt_SigEvent)
		{
			InitializeCriticalSection(&wth->hwt_SigLock);
			wth->hwt_SigState = 0;
			wth->hwt_HALBase = hal;
			wth->hwt_Data = data;
			wth->hwt_Function = function;
			THALSetObject(thread, struct HALWinThread, wth);
			if (function)
			{
				wth->hwt_Thread = (TAPTR) _beginthreadex(NULL, 0,
					hal_win32thread_entry, wth, 0, (TUINT*) &wth->hwt_ThreadID);
				if (wth->hwt_Thread) return TTRUE;
			}
			else
			{
				struct HALWinSpecific *hws = hal->hmb_Specific;
				TlsSetValue(hws->hws_TLSIndex, wth);
				return TTRUE;
			}
			DeleteCriticalSection(&wth->hwt_SigLock);
			CloseHandle(wth->hwt_SigEvent);
		}
		THALDestroyObject(hal, wth, struct HALWinThread);
	}
	tdbprintf(20,"could not create thread\n");
	return TFALSE;
}

EXPORT TVOID
hal_destroythread(TMOD_HAL *hal, THALO *thread)
{
	struct HALWinThread *wth = THALGetObject(thread, struct HALWinThread);
	if (wth->hwt_Function)
	{
		WaitForSingleObject(wth->hwt_Thread, INFINITE);
		CloseHandle(wth->hwt_Thread);
		CloseHandle(wth->hwt_SigEvent);
		DeleteCriticalSection(&wth->hwt_SigLock);
	}
	THALDestroyObject(hal, wth, struct HALWinThread);
}

EXPORT TAPTR
hal_findself(TMOD_HAL *hal)
{
	struct HALWinSpecific *hws = hal->hmb_Specific;
	struct HALWinThread *wth = TlsGetValue(hws->hws_TLSIndex);
	return wth->hwt_Data;
}

/*****************************************************************************/
/*
**	Signals
*/

EXPORT TVOID
hal_signal(TMOD_HAL *hal, THALO *thread, TUINT signals)
{
	struct HALWinThread *wth = THALGetObject(thread, struct HALWinThread);
	
	EnterCriticalSection(&wth->hwt_SigLock);
	if (signals & ~wth->hwt_SigState)
	{
		wth->hwt_SigState |= signals;
		SetEvent(wth->hwt_SigEvent);
	}
	LeaveCriticalSection(&wth->hwt_SigLock);
}

EXPORT TUINT
hal_setsignal(TMOD_HAL *hal, TUINT newsig, TUINT sigmask)
{
	struct HALWinSpecific *hws = hal->hmb_Specific;
	struct HALWinThread *wth = TlsGetValue(hws->hws_TLSIndex);
	TUINT oldsig;

	EnterCriticalSection(&wth->hwt_SigLock);
	oldsig = wth->hwt_SigState;
	wth->hwt_SigState &= ~sigmask;
	wth->hwt_SigState |= newsig;
	if ((newsig & sigmask) & ~oldsig)
	{
		SetEvent(wth->hwt_SigEvent);
	}
	LeaveCriticalSection(&wth->hwt_SigLock);

	return oldsig;
}

EXPORT TUINT
hal_wait(TMOD_HAL *hal, TUINT sigmask)
{
	struct HALWinSpecific *hws = hal->hmb_Specific;
	struct HALWinThread *wth = TlsGetValue(hws->hws_TLSIndex);
	TUINT sig;

	for (;;)
	{
		EnterCriticalSection(&wth->hwt_SigLock);
		sig = wth->hwt_SigState & sigmask;
		wth->hwt_SigState &= ~sigmask;
		LeaveCriticalSection(&wth->hwt_SigLock);
		if (sig) break;
		WaitForSingleObject(wth->hwt_SigEvent, INFINITE);
	}
	
	return sig;
}

static TUINT
hal_timedwaitevent(TMOD_HAL *hal, struct HALWinThread *t,
	TTIME *tektime, TUINT sigmask)
{
	struct HALWinSpecific *hws = hal->hmb_Specific;
	struct HALWinThread *wth = TlsGetValue(hws->hws_TLSIndex);

	TTIME waitt, curt;
	TUINT millis;
	TUINT sig;

	for (;;)
	{
		EnterCriticalSection(&wth->hwt_SigLock);
		sig = wth->hwt_SigState & sigmask;
		wth->hwt_SigState &= ~sigmask;
		LeaveCriticalSection(&wth->hwt_SigLock);
		if (sig) break;

		waitt = *tektime;
		hal_getsystime(hal, &curt);
		hal_subtime(&waitt, &curt);
		if (waitt.ttm_Sec < 0) break;

		if (waitt.ttm_Sec > 1000000)
		{
			millis = 1000000000;
		}
		else
		{
			millis = waitt.ttm_Sec * 1000;
			millis += waitt.ttm_USec / 1000;
		}

		WaitForSingleObject(wth->hwt_SigEvent, millis);
	}

	return sig;
}

/*****************************************************************************/
/*
**	Time and date
*/

EXPORT TVOID
hal_getsystime(TMOD_HAL *hal, TTIME *time)
{
	struct HALWinSpecific *hws = hal->hmb_Specific;
	TDOUBLE t;

	if (hws->hws_UsePerfCounter)
	{
		LONGLONG perft;
		QueryPerformanceCounter((LARGE_INTEGER *) &perft);
		perft -= hws->hws_PerfCStart;
		perft *= 1000000;
		perft /= hws->hws_PerfCFreq;
		perft += (LONGLONG) hws->hws_PerfCStartDate;	/* make absolute */
		time->ttm_Sec = perft / 1000000;
		time->ttm_USec = perft % 1000000;
	}
	else
	{
		LARGE_INTEGER filet;
		
		GetSystemTimeAsFileTime((LPFILETIME) &filet);
		t = (TDOUBLE) filet.QuadPart;
	
		/* 1.1.1970 is as useless as 1.1.1601, but it fits better into a TTIME
		** structure. TTIME is used for relative time measurement only - it
		** doesn't have to be consistent across all systems */
	
		t /= 10000000;		/* seconds */
		t /= 86400;			/* days */
		t -= 134774;		/* 1.1.1601 -> 1.1.1970 */
		t *= 86400;			/* seconds */
		time->ttm_Sec = (TUINT) t;
		time->ttm_USec = (TUINT) ((t - time->ttm_Sec) * 1000000);
	}
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
	struct HALWinSpecific *hws = hal->hmb_Specific;
	LARGE_INTEGER filet;
	TDOUBLE syst;
	
	GetSystemTimeAsFileTime((LPFILETIME) &filet);

	syst = (TDOUBLE) filet.QuadPart;
	syst /= 10000000;		/* seconds */

	if (tzsecp)
	{
		*tzsecp = hws->hws_TZSec;
	}
	else
	{
		syst -= hws->hws_TZSec;
	}

	if (datep)
	{
		syst /= 60 * 60 * 24;	/* days */
		syst += 2305813.5;		/* 1.1.1601 in Julian days */
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
	TINT l = strlen(path) + strlen(modname) + 5;	/* + .dll + \0 */
	if (extra) l += strlen(extra);
	modpath = malloc(l);
	if (modpath)
	{
		strcpy(modpath, path);
		if (extra) strcat(modpath, extra);
		strcat(modpath, modname);
		strcat(modpath, ".dll");
	}
	return modpath;
}

static TSTRPTR getmodpath(TSTRPTR path, TSTRPTR name)
{
	TINT l = strlen(path);
	TSTRPTR p = malloc(l + strlen(name) + 1);
	if (p)
	{
		strcpy(p, path);
		strcpy(p + l, name);
	}
	return p;
}

static TSTRPTR getmodsymbol(TSTRPTR modname)
{
	TINT l = strlen(modname) + 11;
	TSTRPTR sym = malloc(l);
	if (sym)
	{
		strcpy(sym, "_tek_init_");
		strcpy(sym + 10, modname);
	}
	return sym;
}

static TBOOL
scanpathtolist(TCALLBACK TBOOL (*callb)(TAPTR, TSTRPTR, TINT),
	TAPTR userdata, TSTRPTR path, TSTRPTR name)
{
	WIN32_FIND_DATA fd;
	TBOOL success = TTRUE;
	HANDLE hfd = FindFirstFile(path, &fd);
	if (hfd != INVALID_HANDLE_VALUE)
	{
		TINT l, pl = strlen(name);
		do
		{
			l = strlen(fd.cFileName);
			if (l >= pl + 4)
			{
				if (!strncmp(fd.cFileName, name, pl))
				{
					success = (*callb)(userdata, fd.cFileName, l - 4);
				}
			}
		} while (success && FindNextFile(hfd, &fd));
		FindClose(hfd);
	}
	return success;
}

EXPORT TAPTR
hal_loadmodule(TMOD_HAL *hal, TSTRPTR name, TUINT16 version,
	TUINT *psize, TUINT *nsize)
{
	struct HALWinModule *mod = hal_alloc(hal, sizeof(struct HALWinModule));
	if (mod)
	{
		TSTRPTR modpath;
		struct HALWinSpecific *hws = hal->hmb_Specific;

		mod->hwm_Lib = TNULL;
		mod->hwm_InitFunc = TNULL;

		modpath = getmodpathname(hws->hws_ProgDir, "mod\\", name);
		if (modpath)
		{
			tdbprintf1(2,"trying %s\n", modpath);
			mod->hwm_Lib = LoadLibraryEx(modpath, 0,
				LOAD_WITH_ALTERED_SEARCH_PATH);
			if (!mod->hwm_Lib)
			{
				free(modpath);
				modpath = getmodpathname(hws->hws_ModDir, TNULL, name);
				if (modpath)
				{
					tdbprintf1(2,"trying %s\n", modpath);
					mod->hwm_Lib = LoadLibraryEx(modpath, 0,
						LOAD_WITH_ALTERED_SEARCH_PATH);
				}
			}
			if (modpath) free(modpath);
		}

		if (mod->hwm_Lib)
		{
			TSTRPTR modsym;
			modsym = getmodsymbol(name);
			if (modsym)
			{
				tdbprintf1(2,"resolving %s\n", modsym);
				mod->hwm_InitFunc =
					(TAPTR) GetProcAddress(mod->hwm_Lib, modsym + 1);
				if (!mod->hwm_InitFunc)
				{
					mod->hwm_InitFunc =
						(TAPTR) GetProcAddress(mod->hwm_Lib, modsym);
				}
				free(modsym);
			}

			if (mod->hwm_InitFunc)
			{
				*psize = (*mod->hwm_InitFunc)(TNULL, TNULL, version, TNULL);
				if (*psize)
				{
					*nsize = (*mod->hwm_InitFunc)(TNULL, TNULL, 0xffff, TNULL);
					mod->hwm_Version = version;
					return (TAPTR) mod;
				}
			} else tdbprintf1(5,"could not resolve %s entrypoint\n", name);

			FreeLibrary(mod->hwm_Lib);
		}
		free(mod);
	}
	return TNULL;
}

EXPORT TBOOL
hal_callmodule(TMOD_HAL *hal, TAPTR halmod, TAPTR task, TAPTR mod)
{
	struct HALWinModule *hwm = halmod;
	return (TBOOL) (*hwm->hwm_InitFunc)(task, mod, hwm->hwm_Version, TNULL);
}

EXPORT TVOID
hal_unloadmodule(TMOD_HAL *hal, TAPTR halmod)
{
	struct HALWinModule *hwm = halmod;
	FreeLibrary(hwm->hwm_Lib);
	tdbprintf(2,"module unloaded\n");
	free(halmod);
}

EXPORT TBOOL
hal_scanmodules(TMOD_HAL *hal, TSTRPTR path,
	TCALLBACK TBOOL (*callb)(TAPTR, TSTRPTR, TINT), TAPTR userdata)
{
	TSTRPTR p;
	TBOOL success = TFALSE;
	struct HALWinSpecific *hws = hal->hmb_Specific;

	p = getmodpath(hws->hws_ProgDir, "mod\\*.dll");
	if (p)
	{
		tdbprintf1(2,"scanning %s\n", p);
		success = scanpathtolist(callb, userdata, p, path);
		free(p);
	}

	if (success)
	{
		p = getmodpath(hws->hws_ModDir, "*.dll");
		if (p)
		{
			tdbprintf1(2,"scanning %s\n", p);
			success = scanpathtolist(callb, userdata, p, path);
			free(p);
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
	struct HALWinSpecific *hws = hal->hmb_Specific;
	TAPTR exec = (TAPTR) TGetTag(tags, THalBase_Exec, TNULL);
	struct TTimeRequest *req;
	hws->hws_ExecBase = exec;
	
	req = TExecAllocMsg0(exec, sizeof(struct TTimeRequest));
	if (req)
	{
		EnterCriticalSection(&hws->hws_DevLock);

		if (!hws->hws_DevTask)
		{
			TTAGITEM tasktags[2];
			tasktags[0].tti_Tag = TTask_Name;			/* set task name */
			tasktags[0].tti_Value = (TTAG) TTASKNAME_HALDEV;
			tasktags[1].tti_Tag = TTAG_DONE;
			TInitList(&hws->hws_ReqList);
			hws->hws_DevTask = TExecCreateSysTask(exec, haldevfunc, tasktags);
		}

		if (hws->hws_DevTask)
		{
			hws->hws_RefCount++;
			req->ttr_Req.io_Device = (struct TModule *) hal;
		}
		else
		{	
			TExecFree(exec, req);
			req = TNULL;
		}
		
		LeaveCriticalSection(&hws->hws_DevLock);
	}

	return req;
}

/*****************************************************************************/

LOCAL TCALLBACK TVOID 
hal_close(TMOD_HAL *hal, TAPTR task)
{
	struct HALWinSpecific *hws = hal->hmb_Specific;

	EnterCriticalSection(&hws->hws_DevLock);

	if (hws->hws_DevTask)
	{
		if (--hws->hws_RefCount == 0)
		{
			tdbprintf(2,"hal.device refcount dropped to 0\n");
			TExecSignal(hws->hws_ExecBase, hws->hws_DevTask, TTASK_SIG_ABORT);
			tdbprintf(2,"destroy hal.device task...\n");
			TDestroy(hws->hws_DevTask);
			hws->hws_DevTask = TNULL;
		}
	}

	LeaveCriticalSection(&hws->hws_DevLock);
}

/*****************************************************************************/

static TVOID TTASKENTRY 
haldevfunc(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_HAL *hal = TExecGetHALBase(exec);
	struct HALWinSpecific *hws = hal->hmb_Specific;
	struct HALWinThread *thread = TlsGetValue(hws->hws_TLSIndex);
	TAPTR port = TExecGetUserPort(exec, task);
	struct TTimeRequest *msg;
	TTIME waittime, curtime;
	TUINT sig;
	
	waittime.ttm_Sec = 2000000000;
	waittime.ttm_USec = 0;

	for (;;)
	{
		sig = hal_timedwaitevent(hal, thread, &waittime, TTASK_SIG_ABORT | TTASK_SIG_USER);

		if (sig & TTASK_SIG_ABORT) break;
	
		hal_getsystime(hal, &curtime);

		if (sig == 0)
		{
			struct TNode *nnode, *node;
			waittime.ttm_Sec = 2000000000;

			EnterCriticalSection(&hws->hws_DevLock);

			node = hws->hws_ReqList.tlh_Head;
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

			LeaveCriticalSection(&hws->hws_DevLock);
			continue;
		}
		
		if (sig & TTASK_SIG_USER)
		{
			/* got message */

			EnterCriticalSection(&hws->hws_DevLock);

			while ((msg = TExecGetMsg(exec, port)))
			{
				TTIME *tm = &msg->ttr_Data.ttr_Time;
				hal_addtime(tm, &curtime);					/* rel. -> abs. time */
				if (hal_cmptime(tm, &waittime) < 0)
				{
					/* next event */
					waittime = *tm;
				}
				/* insert to queue */
				TAddTail(&hws->hws_ReqList, (struct TNode *) msg);
			}

			LeaveCriticalSection(&hws->hws_DevLock);
		}
	}

	tdbprintf(2,"goodbye from HAL device\n");
}

EXPORT TVOID 
hal_beginio(TMOD_HAL *hal, struct TTimeRequest *req)
{
	struct HALWinSpecific *hws = hal->hmb_Specific;
	TAPTR exec = hws->hws_ExecBase;
	TTIME nowtime;
	TDOUBLE x = 0;

	switch (req->ttr_Req.io_Command)
	{
		/* execute asynchronously */
		case TTREQ_ADDLOCALDATE:
			x = hws->hws_TZDays;					/* days west of GMT */
		case TTREQ_ADDUNIDATE:
			x += req->ttr_Data.ttr_Date.ttr_Date.tdt_Day.tdtt_Double;
			x -= 2440587.5;							/* days since 1.1.1970 */
			x *= 86400;								/* secs since 1.1.1970 */
			req->ttr_Data.ttr_Time.ttm_Sec = (TINT) x;
			x -= req->ttr_Data.ttr_Time.ttm_Sec;	/* fraction of sec */
			x *= 1000000;
			req->ttr_Data.ttr_Time.ttm_USec = (TINT) x;	/* microseconds */
			hal_getsystime(hal, &nowtime);
			hal_subtime(&req->ttr_Data.ttr_Time, &nowtime);
			/* relative time */
		case TTREQ_ADDTIME:
			TExecPutMsg(exec, TExecGetUserPort(exec, hws->hws_DevTask), 
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
	struct HALWinSpecific *hws = hal->hmb_Specific;
	TAPTR exec = hws->hws_ExecBase;
	TINT error = -1;

	EnterCriticalSection(&hws->hws_DevLock);
	if (TExecGetMsgStatus(exec, req) == (TMSG_STATUS_SENT|TMSGF_QUEUED))
	{
		TExecRemoveMsg(exec, req->ttr_Req.io_ReplyPort, req);
	}
	else
	{
		req = TNULL;
	}
	LeaveCriticalSection(&hws->hws_DevLock);

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
**	Revision 1.11  2005/09/13 02:42:16  tmueller
**	updated copyright reference
**	
**	Revision 1.10  2005/09/07 23:58:30  tmueller
**	added date to julian type conversion
**	
**	Revision 1.9  2005/04/01 18:40:26  tmueller
**	additional boot argument to hal_allocself
**	
**	Revision 1.8  2004/04/18 14:11:01  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.7  2004/02/07 05:06:04  tmueller
**	Internal time support functions used; TIOF_QUICK no longer cleared
**	
**	Revision 1.6  2003/12/22 23:55:18  tmueller
**	Fixed waiting for an absolute date. Maybe this time...
**	
**	Revision 1.5  2003/12/22 23:01:12  tmueller
**	Fixed waiting for an absolute date
**	
**	Revision 1.4  2003/12/12 23:10:09  tmueller
**	Time query now using only integer artithmetics
**	
**	Revision 1.2  2003/12/12 14:22:34  dtrompetter
**	changed win32 include path from hal_mod.h to ../hal_mod.h
**	
**	Revision 1.1.1.1  2003/12/11 07:18:57  tmueller
**	Krypton import
*/
