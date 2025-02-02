
/*
**	$Id: hal.c,v 1.9 2005/09/13 02:42:16 tmueller Exp $
**	teklib/mods/hal/posix/hal.c - POSIX implementation of the HAL layer
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/proto/exec.h>
#include <tek/mod/posix/hal.h>

#include "hal_mod.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

static TVOID TTASKENTRY haldevfunc(TAPTR task);

/*****************************************************************************/

#ifdef TDEBUG
#define TRACKMEM
static int bytecount;
static int allocount;
static int maxbytecount;
static pthread_mutex_t alloclock;
#endif

/*****************************************************************************/
/*
**	Host init
*/

LOCAL TBOOL
hal_init(TMOD_HAL *hal, TTAGITEM *tags)
{
	struct HALPosixSpecific *specific =
		malloc(sizeof(struct HALPosixSpecific));
	if (specific)
	{
		memset(specific, 0, sizeof(struct HALPosixSpecific));

		pthread_mutex_init(&specific->hps_DevLock, TNULL);
	
		if (pthread_key_create(&specific->hps_TSDKey, NULL) == 0)
		{
			time_t curt;
		
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
			
			tzset();	/* set global timezone variable */

			time(&curt);
			specific->hps_TZSec = mktime(gmtime(&curt)) -
				mktime(localtime(&curt));
			specific->hps_TZDays = specific->hps_TZSec;
			specific->hps_TZDays /= 86400;
			
			#ifdef TRACKMEM
			pthread_mutex_init(&alloclock, TNULL);
			allocount = 0;
			bytecount = 0;
			maxbytecount = 0;
			#endif
			
			return TTRUE;
		}
		free(specific);
	}
	return TFALSE;
}

LOCAL TVOID
hal_exit(TMOD_HAL *hal)
{
	struct HALPosixSpecific *specific = hal->hmb_Specific;
	pthread_key_delete(specific->hps_TSDKey);
	pthread_mutex_destroy(&specific->hps_DevLock);
	free(specific);

	#ifdef TRACKMEM
	pthread_mutex_destroy(&alloclock);
	if (allocount || bytecount)
	{
		tdbprintf2(10,"*** Global memory leak: %d allocs, %d bytes pending\n",
			allocount, bytecount);
	}
	tdbprintf1(5,"*** Peak memory allocated: %d bytes\n", maxbytecount);
	#endif
}

LOCAL TAPTR
hal_allocself(TAPTR handle, TUINT size)
{
	return malloc(size);
}

LOCAL TVOID
hal_freeself(TAPTR handle, TAPTR mem, TUINT size)
{
	return free(mem);
}

/*****************************************************************************/
/*
**	Memory
*/

EXPORT TAPTR
hal_alloc(TMOD_HAL *hal, TUINT size)
{
	TAPTR mem = malloc(size);
	#ifdef TRACKMEM	
	if (mem)
	{
		pthread_mutex_lock(&alloclock);
		allocount++;
		bytecount += size;
		if (bytecount > maxbytecount) maxbytecount = bytecount;
		pthread_mutex_unlock(&alloclock);
	}
	#endif	
	return mem;
}

EXPORT TAPTR
hal_realloc(TMOD_HAL *hal, TAPTR mem, TUINT oldsize, TUINT newsize)
{
	TAPTR newmem;

	#ifdef TRACKMEM
	pthread_mutex_lock(&alloclock);
	if (mem)
	{
		allocount--;
		bytecount -= oldsize;
	}
	#endif

	newmem = realloc(mem, newsize);

	#ifdef TRACKMEM
	if (newmem)
	{
		allocount++;
		bytecount += newsize;
	}
	pthread_mutex_unlock(&alloclock);
	#endif
	
	return newmem;
}

EXPORT TVOID
hal_free(TMOD_HAL *hal, TAPTR mem, TUINT size)
{
	#ifdef TRACKMEM	
	if (mem)
	{
		pthread_mutex_lock(&alloclock);
		allocount--;
		bytecount -= size;
		pthread_mutex_unlock(&alloclock);
	}
	#endif	
	free(mem);
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
	pthread_mutex_t *mut = THALNewObject(hal, lock, pthread_mutex_t);
	if (mut)
	{
		pthread_mutex_init(mut, TNULL);
		THALSetObject(lock, pthread_mutex_t, mut);
		return TTRUE;
	}
	tdbprintf(20,"could not create lock\n");
	return TFALSE;
}

EXPORT TVOID
hal_destroylock(TMOD_HAL *hal, THALO *lock)
{
	pthread_mutex_t *mut = THALGetObject(lock, pthread_mutex_t);
	if (pthread_mutex_destroy(mut)) tdbprintf(20, "mutex_destroy\n");
	THALDestroyObject(hal, mut, pthread_mutex_t);
}

EXPORT TVOID
hal_lock(TMOD_HAL *hal, THALO *lock)
{
	pthread_mutex_t *mut = THALGetObject(lock, pthread_mutex_t);
	if (pthread_mutex_lock(mut)) tdbprintf(20, "mutex_lock\n");
	/*while (pthread_mutex_trylock(mut));*/
}

EXPORT TVOID
hal_unlock(TMOD_HAL *hal, THALO *lock)
{
	pthread_mutex_t *mut = THALGetObject(lock, pthread_mutex_t);
	if (pthread_mutex_unlock(mut)) tdbprintf(20, "mutex_unlock\n");
}

/*****************************************************************************/
/*
**	Threads
*/

static void *
posixthread_entry(struct HALPosixThread *thread)
{
	TMOD_HAL *hal = thread->hpt_HALBase;
	struct HALPosixSpecific *hps = hal->hmb_Specific;

	if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL))
		tdbprintf(20, "pthread_setcancelstate\n");

	if (pthread_setspecific(hps->hps_TSDKey, (void *) thread))
		tdbprintf(20,"failed to set TSD key\n");

	/* wait for initial signal to run */
	hal_wait(hal, TTASK_SIG_SINGLE);

	/* call function */
	(*thread->hpt_Function)(thread->hpt_Data);

	return NULL;
}

EXPORT TBOOL 
hal_initthread(TMOD_HAL *hal, THALO *thread,
	TTASKENTRY TVOID (*function)(TAPTR task), TAPTR data)
{
	struct HALPosixThread *t =
		THALNewObject(hal, thread, struct HALPosixThread);
	if (t)
	{
		if (pthread_cond_init(&t->hpt_SigCond, NULL) == 0)
		{
			pthread_mutex_init(&t->hpt_SigMutex, NULL);
			t->hpt_SigState = 0;
			t->hpt_Function = function;
			t->hpt_Data = data;
			t->hpt_HALBase = hal;
			THALSetObject(thread, struct HALPosixThread, t);
			if (function)
			{
				if (pthread_create(&t->hpt_PThread, NULL,
					(void *(*)(void *)) posixthread_entry, t) == 0)
				{
					return TTRUE;
				}
			}
			else
			{
				struct HALPosixSpecific *hps = hal->hmb_Specific;
				if (pthread_setspecific(hps->hps_TSDKey, (void *) t) == 0)
				{
					return TTRUE;
				}
			}
			pthread_cond_destroy(&t->hpt_SigCond);
		}
		THALDestroyObject(hal, t, struct HALPosixThread);
	}
	tdbprintf(20,"could not create thread\n");
	return TFALSE;
}

EXPORT TVOID
hal_destroythread(TMOD_HAL *hal, THALO *thread)
{
	struct HALPosixThread *t = THALGetObject(thread, struct HALPosixThread);
	if (t->hpt_Function)
	{
		if (pthread_join(t->hpt_PThread, NULL)) tdbprintf(20,"pthread_join\n");
	}
	if (pthread_mutex_destroy(&t->hpt_SigMutex))
		tdbprintf(20, "mutex_destroy\n");
	if (pthread_cond_destroy(&t->hpt_SigCond))
		tdbprintf(20, "cond_destroy\n");
	THALDestroyObject(hal, t, struct HALPosixThread);
}

EXPORT TAPTR
hal_findself(TMOD_HAL *hal)
{
	struct HALPosixSpecific *hps = hal->hmb_Specific;
	struct HALPosixThread *t = pthread_getspecific(hps->hps_TSDKey);
	return t->hpt_Data;
}

/*****************************************************************************/
/*
**	Signals
*/

EXPORT TVOID
hal_signal(TMOD_HAL *hal, THALO *thread, TUINT signals)
{
	struct HALPosixThread *t = THALGetObject(thread, struct HALPosixThread);
	if (pthread_mutex_lock(&t->hpt_SigMutex)) tdbprintf(20, "mutex_lock\n");
	if (signals & ~t->hpt_SigState)
	{
		t->hpt_SigState |= signals;
		if (pthread_cond_signal(&t->hpt_SigCond))
			tdbprintf(20, "cond_signal\n");
	}
	if (pthread_mutex_unlock(&t->hpt_SigMutex))
		tdbprintf(20, "mutex_unlock\n");
}

EXPORT TUINT
hal_setsignal(TMOD_HAL *hal, TUINT newsig, TUINT sigmask)
{
	TUINT oldsig;
	struct HALPosixSpecific *hps = hal->hmb_Specific;
	struct HALPosixThread *t = pthread_getspecific(hps->hps_TSDKey);
	if (pthread_mutex_lock(&t->hpt_SigMutex)) tdbprintf(20, "mutex_lock\n");

	oldsig = t->hpt_SigState;
	t->hpt_SigState &= ~sigmask;
	t->hpt_SigState |= newsig;
	if ((newsig & sigmask) & ~oldsig)
	{
		if (pthread_cond_signal(&t->hpt_SigCond))
			tdbprintf(20, "cond_signal\n");
	}

	if (pthread_mutex_unlock(&t->hpt_SigMutex))
		tdbprintf(20, "mutex_unlock\n");
	return oldsig;
}

EXPORT TUINT
hal_wait(TMOD_HAL *hal, TUINT sigmask)
{
	TUINT sig;
	struct HALPosixSpecific *hps = hal->hmb_Specific;
	struct HALPosixThread *t = pthread_getspecific(hps->hps_TSDKey);

	if (pthread_mutex_lock(&t->hpt_SigMutex)) tdbprintf(20, "mutex_lock\n");
	for (;;)
	{
		sig = t->hpt_SigState & sigmask;
		t->hpt_SigState &= ~sigmask;
		if (sig) break;
		pthread_cond_wait(&t->hpt_SigCond, &t->hpt_SigMutex);
	}
	if (pthread_mutex_unlock(&t->hpt_SigMutex))
		tdbprintf(20, "mutex_unlock\n");

	return sig;
}

static TUINT
hal_timedwaitevent(TAPTR hal, struct HALPosixThread *t, TTIME *wt,
	TUINT sigmask)
{
	TUINT sig;

	struct timespec tv;
	tv.tv_sec = wt->ttm_Sec;
	tv.tv_nsec = wt->ttm_USec * 1000;

	pthread_mutex_lock(&t->hpt_SigMutex);

	for (;;)
	{
		sig = t->hpt_SigState & sigmask;
		t->hpt_SigState &= ~sigmask;
		if (sig) break;
		if (pthread_cond_timedwait(&t->hpt_SigCond,
			&t->hpt_SigMutex, &tv) == ETIMEDOUT) break;
	}

	pthread_mutex_unlock(&t->hpt_SigMutex);
	return sig;
}

/*****************************************************************************/
/*
**	Time and date
*/

EXPORT TVOID
hal_getsystime(TMOD_HAL *hal, TTIME *time)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time->ttm_Sec = tv.tv_sec;
	time->ttm_USec = tv.tv_usec;
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
	struct HALPosixSpecific *hps = hal->hmb_Specific;
	struct timeval tv;
	TDOUBLE syst;

	gettimeofday(&tv, NULL);

	syst = tv.tv_usec;
	syst /= 1000000;
	syst += tv.tv_sec;

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

#ifdef TSYS_DARWIN

#include <mach-o/dyld.h>

static TAPTR
getmodule(TSTRPTR modpath)
{
	return (TAPTR) NSAddImage(modpath, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
}

static TAPTR
getsymaddr(struct mach_header *mod, TSTRPTR name)
{
	if (NSIsSymbolNameDefinedInImage(mod, name))
	{
		NSSymbol *sym = NSLookupSymbolInImage(mod, name,
			NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
			NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
		return NSAddressOfSymbol(sym);
	}
	return TNULL;
}

static TVOID
closemodule()
{
	/* is there no equivalent to dlclose()? */
}

#else

#include <dlfcn.h>

static TAPTR
getmodule(TSTRPTR modpath)
{
	return dlopen(modpath, RTLD_NOW);
}

static TAPTR
getsymaddr(TAPTR mod, TSTRPTR name)
{
	return dlsym(mod, name);
}

static TVOID
closemodule(TAPTR mod)
{
	dlclose(mod);
}

#endif

/*****************************************************************************/
/* 
**	Module loading and calling
*/

static TSTRPTR
getmodpathname(TSTRPTR path, TSTRPTR extra, TSTRPTR modname)
{
	TSTRPTR modpath;
	TINT l;
	if (!path || !modname) return TNULL;
	l = strlen(path) + strlen(modname) + TEKHOST_EXTLEN + 1;
	if (extra) l += strlen(extra);
	modpath = malloc(l);
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
getmodsymbol(TSTRPTR modname)
{
	TINT l = strlen(modname) + 11;
	TSTRPTR sym = malloc(l);
	if (sym)
	{
		#ifdef TSYS_DARWIN
			strcpy(sym, "_tek_init_");
		#else
			strcpy(sym, "tek_init_");
		#endif
		strcat(sym, modname);
	}
	return sym;
}

EXPORT TAPTR
hal_loadmodule(TMOD_HAL *hal, TSTRPTR name, TUINT16 version, TUINT *psize,
	TUINT *nsize)
{
	struct HALUnixMod *halmod = hal_alloc(hal, sizeof(struct HALUnixMod));
	if (halmod)
	{
		struct HALPosixSpecific *hps = hal->hmb_Specific;
		TSTRPTR modpath;

		halmod->hum_Lib = TNULL;

		modpath = getmodpathname(hps->hps_ProgDir, "mod/", name);
		if (modpath)
		{
			tdbprintf1(2,"trying %s\n", modpath);
			halmod->hum_Lib = getmodule(modpath);
			if (!halmod->hum_Lib)
			{
				free(modpath);
				modpath = getmodpathname(hps->hps_ModDir, TNULL, name);
				if (modpath)
				{
					tdbprintf1(2,"trying %s\n", modpath);
					halmod->hum_Lib = getmodule(modpath);
				}
			}
			if (modpath) free(modpath);
		}
		
		if (halmod->hum_Lib)
		{
			TSTRPTR modsym = getmodsymbol(name);
			if (modsym)
			{
				tdbprintf1(3,"resolving %s\n", modsym);
				halmod->hum_InitFunc = getsymaddr(halmod->hum_Lib, modsym);
				free(modsym);

				if (halmod->hum_InitFunc)
				{
					*psize = (*halmod->hum_InitFunc)
						(TNULL, TNULL, version, TNULL);

					if (*psize)
					{
						*nsize = (*halmod->hum_InitFunc)
							(TNULL, TNULL, 0xffff, TNULL);
						halmod->hum_Version = version;
						return (TAPTR) halmod;
	
					} else tdbprintf1(5,"module %s returned 0\n", name);
	
				} else
					tdbprintf1(10,"could not resolve %s entrypoint\n", name);
			}

			closemodule(halmod->hum_Lib);

		} else tdbprintf1(10,"could not open module %s\n", name);
		
		hal_free(hal, halmod, sizeof(struct HALUnixMod));
	}
	return TNULL;
}

EXPORT TBOOL
hal_callmodule(TMOD_HAL *hal, TAPTR mod, TAPTR task, TAPTR data)
{
	struct HALUnixMod *hum = mod;
	return (TBOOL) (*hum->hum_InitFunc)(task, data, hum->hum_Version, TNULL);
}

EXPORT TVOID
hal_unloadmodule(TMOD_HAL *hal, TAPTR halmod)
{
	struct HALUnixMod *hum = halmod;
	closemodule(hum->hum_Lib);
 	hal_free(hal, halmod, sizeof(struct HALUnixMod));
	tdbprintf(2,"module unloaded\n");
}

/*****************************************************************************/
/* 
**	Module scanning
*/

static TSTRPTR 
getmodpath(TSTRPTR path, TSTRPTR name)
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

static TBOOL
scanpathtolist(TCALLBACK TBOOL (*callb)(TAPTR, TSTRPTR, TINT),
	TAPTR userdata, TSTRPTR path, TSTRPTR name)
{
	TBOOL success = TTRUE;
	DIR *dfd = opendir(path);
	if (dfd)
	{
		TINT l;
		struct dirent *de;
		TINT pl = strlen(name);
		while (success && (de = readdir(dfd)))
		{
			l = strlen(de->d_name);
			if (l < pl + TEKHOST_EXTLEN) continue;
			if (strcmp(de->d_name + l - TEKHOST_EXTLEN, TEKHOST_EXTSTR))
				continue;
			if (strncmp(de->d_name, name, pl)) continue;
			success = (*callb)(userdata, de->d_name, l - TEKHOST_EXTLEN);
		}
		closedir(dfd);
	}
	return success;
}

EXPORT TBOOL
hal_scanmodules(TMOD_HAL *hal, TSTRPTR path,
	TCALLBACK TBOOL (*callb)(TAPTR, TSTRPTR, TINT), TAPTR userdata)
{
	TSTRPTR p;
	struct HALPosixSpecific *hps = hal->hmb_Specific;
	TBOOL success = TFALSE;

	p = getmodpath(hps->hps_ProgDir, "mod/");
	if (p)
	{
		success = scanpathtolist(callb, userdata, p, path);
		free(p);
	}
	
	if (success)
	{
		p = getmodpath(hps->hps_ModDir, "");
		if (p)
		{
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
	struct HALPosixSpecific *hps = hal->hmb_Specific;
	TAPTR exec = (TAPTR) TGetTag(tags, THalBase_Exec, TNULL);
	struct TTimeRequest *req;
	hps->hps_ExecBase = exec;
	
	req = TExecAllocMsg0(exec, sizeof(struct TTimeRequest));
	if (req)
	{
		pthread_mutex_lock(&hps->hps_DevLock);
		
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

		pthread_mutex_unlock(&hps->hps_DevLock);
	}

	return req;
}

/*****************************************************************************/

LOCAL TCALLBACK TVOID 
hal_close(TMOD_HAL *hal, TAPTR task)
{
	struct HALPosixSpecific *hps = hal->hmb_Specific;
	pthread_mutex_lock(&hps->hps_DevLock);
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
	pthread_mutex_unlock(&hps->hps_DevLock);
}

/*****************************************************************************/

static TVOID TTASKENTRY 
haldevfunc(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_HAL *hal = TExecGetHALBase(exec);
	struct HALPosixSpecific *hps = hal->hmb_Specific;
	struct HALPosixThread *thread = pthread_getspecific(hps->hps_TSDKey);
	TAPTR port = TExecGetUserPort(exec, task);
	struct TTimeRequest *msg;
	TUINT sig;
	TTIME waittime, curtime;
	
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
			
			pthread_mutex_lock(&hps->hps_DevLock);
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
			pthread_mutex_unlock(&hps->hps_DevLock);
			continue;
		}
		
		if (sig & TTASK_SIG_USER)
		{
			/* got message */
			pthread_mutex_lock(&hps->hps_DevLock);
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
			pthread_mutex_unlock(&hps->hps_DevLock);
		}
	}
	tdbprintf(2,"goodbye from HAL device\n");
}

EXPORT TVOID 
hal_beginio(TMOD_HAL *hal, struct TTimeRequest *req)
{
	struct HALPosixSpecific *hps = hal->hmb_Specific;
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

	if (!(req->ttr_Req.io_Flags & TIOF_QUICK))
	{
		/* async operation indicated; reply to self */
		TExecReplyMsg(exec, req);
	}
}

EXPORT TINT 
hal_abortio(TMOD_HAL *hal, struct TTimeRequest *req)
{
	struct HALPosixSpecific *hps = hal->hmb_Specific;
	TAPTR exec = hps->hps_ExecBase;
	TINT error = -1;

	pthread_mutex_lock(&hps->hps_DevLock);

	if (TExecGetMsgStatus(exec, req) == (TMSG_STATUS_SENT|TMSGF_QUEUED))
	{
		TExecRemoveMsg(exec, req->ttr_Req.io_ReplyPort, req);
	}
	else
	{
		req = TNULL;
	}

	pthread_mutex_unlock(&hps->hps_DevLock);

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
**	Revision 1.9  2005/09/13 02:42:16  tmueller
**	updated copyright reference
**	
**	Revision 1.8  2005/09/07 23:58:30  tmueller
**	added date to julian type conversion
**	
**	Revision 1.7  2005/04/01 18:40:26  tmueller
**	additional boot argument to hal_allocself
**	
**	Revision 1.6  2004/04/18 14:11:01  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.5  2004/04/17 02:44:18  tmueller
**	Handling of time structures has been cleaned up; works on 64bit arch now
**	
**	Revision 1.4  2004/02/07 05:06:04  tmueller
**	Internal time support functions used; TIOF_QUICK no longer cleared
**	
**	Revision 1.3  2003/12/20 14:00:18  tmueller
**	hps_TZSecDays renamed to hps_TZDays
**	
**	Revision 1.2  2003/12/19 14:14:09  tmueller
**	Waiting for an absolute date was broken. Fixed.
**	
**	Revision 1.1.1.1  2003/12/11 07:18:55  tmueller
**	Krypton import
*/
