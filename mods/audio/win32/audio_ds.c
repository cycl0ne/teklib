
/*
**	$Id: audio_ds.c,v 1.5 2004/08/06 18:02:12 tmueller Exp $
**	audio/win32/audio_ds.c - DirectSound audio backend
*/

#define INITGUID				/* order DOES MATTER ... */
#include <windows.h>			/* at least... */
#include <dsound.h>				/* ... in Redmond */

#include "../audio_mod.h"

#include <tek/mod/exec.h>		/* exec-private structures */
#include <tek/proto/hal.h>		/* HAL module interface */
#include <tek/mod/hal.h>		/* HAL private stuff */
#include <tek/mod/win32/hal.h>	/* HAL private stuff */

#define NUMFRAGS	4			/* notification fragments */

struct DSAudio					/* Host specific */
{
	IDirectSound *ds;
	TAPTR hwndatom;
	PCMWAVEFORMAT pcmwf;
	DSBUFFERDESC dsbufdesc;
	LPDIRECTSOUNDBUFFER dsbuf;
	TINT lastpos;

	LPDIRECTSOUNDNOTIFY dsnotify;
	DSBPOSITIONNOTIFY dsnpos[NUMFRAGS];
	
	HANDLE wrapevent;

	TAPTR halbase;					/* HAL module base */
	struct TTask *tektask;			/* Ptr to this task (exec internal structure) */
	HANDLE sigevent;				/* Ptr to this task's Windows event handle */
	struct HALWinThread *thread;	/* Ptr to this task's Windows thread handle */
};

/*****************************************************************************/
/*
**	device task init
*/

LOCAL TTASKENTRY TBOOL aud_init(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_DEV *mod = TExecGetTaskData(exec, task);
	struct DSAudio *audio;
	TAPTR adr1, adr2;
	TUINT size1, size2;
	TINT i;

	audio = TExecAlloc0(mod->exec, mod->mmu, sizeof(struct DSAudio));
	if (!audio) return TFALSE;
	mod->hostspecific = audio;

	/* Get the Windows Event that can wakeup this TEK task */
	audio->halbase = TExecGetHALBase(mod->exec);
	audio->tektask = task;

	//audio->sigevent = hal_getobject(&audio->tektask->tsk_SigEvent, HANDLE);
	audio->thread = THALGetObject(&audio->tektask->tsk_Thread, struct HALWinThread);
	audio->sigevent = audio->thread->hwt_SigEvent;

	//SetThreadPriority(audio->thread->hwt_Thread, THREAD_PRIORITY_TIME_CRITICAL);

	audio->hwndatom = TExecLockAtom(exec, "win32.hwnd", TATOMF_NAME | TATOMF_SHARED);
	if (!audio->hwndatom) goto initerr;

	if (DirectSoundCreate(NULL, &audio->ds, NULL) != DS_OK) goto initerr;

	if (IDirectSound_SetCooperativeLevel(audio->ds,
		(void *) TExecGetAtomData(exec, audio->hwndatom), DSSCL_PRIORITY) != DS_OK) goto initerr2;

	mod->mixfmt = TADIOFMT_S16S;
	mod->mixfreq = 44100;
	mod->mixbps = TADIO_GETNUMCHANNELS(mod->mixfmt) * TADIO_GETBYTESPERCHAN(mod->mixfmt);
	mod->mixbufsize = 8192;		//8192!
	mod->mixbuf = TExecAlloc0(mod->exec, mod->mmu, mod->mixbufsize * 2 * mod->mixbps);
	if (!mod->mixbuf) goto initerr2;

	audio->pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	audio->pcmwf.wf.nChannels = TADIO_GETNUMCHANNELS(mod->mixfmt);
	audio->pcmwf.wBitsPerSample = 16;
	audio->pcmwf.wf.nSamplesPerSec = mod->mixfreq;
	audio->pcmwf.wf.nBlockAlign = mod->mixbps;
	audio->pcmwf.wf.nAvgBytesPerSec = mod->mixfreq * mod->mixbps;

	audio->dsbufdesc.dwSize = sizeof(DSBUFFERDESC);
	audio->dsbufdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY |
		DSBCAPS_GETCURRENTPOSITION2;
	audio->dsbufdesc.dwBufferBytes = mod->mixbufsize * mod->mixbps;
	audio->dsbufdesc.lpwfxFormat = (LPWAVEFORMATEX) &audio->pcmwf;

	if (IDirectSound_CreateSoundBuffer(audio->ds,
		&audio->dsbufdesc, &audio->dsbuf, NULL) != DS_OK) goto initerr2;

	if (IDirectSoundBuffer_QueryInterface(audio->dsbuf, &IID_IDirectSoundNotify,
		&audio->dsnotify) != DS_OK) goto initerr3;

	audio->wrapevent = CreateEvent(NULL, FALSE, FALSE, NULL);
		
	for (i = 0; i < NUMFRAGS; ++i)
	{
		audio->dsnpos[i].dwOffset = mod->mixbufsize * i * mod->mixbps / NUMFRAGS;
		audio->dsnpos[i].hEventNotify = audio->wrapevent;
	}
	
	if (IDirectSoundNotify_SetNotificationPositions(audio->dsnotify,
		NUMFRAGS, audio->dsnpos) != DS_OK) goto initerr3;

	if (IDirectSoundBuffer_Play(audio->dsbuf, 
		0, 0, DSBPLAY_LOOPING) != DS_OK) goto initerr3;

	TInitList(&mod->chanlist);
	mod->port = TExecGetUserPort(mod->exec, mod->task);

	/* Attributes that can be queried by the user */

	mod->attributes[0].tti_Tag = TADIOT_MixFormat;
	mod->attributes[0].tti_Value = (TTAG) mod->mixfmt;
	mod->attributes[1].tti_Tag = TADIOT_MixRate;
	mod->attributes[1].tti_Value = (TTAG) mod->mixfreq;
	mod->attributes[2].tti_Tag = TADIOT_FragmentSize;
	mod->attributes[2].tti_Value = (TTAG) mod->mixbufsize;
	mod->attributes[3].tti_Tag = TTAG_DONE;

	return TTRUE;

initerr3:
	IDirectSoundBuffer_Release(audio->dsbuf);

initerr2:
	TExecFree(mod->exec, mod->mixbuf);
	IDirectSound_Release(audio->ds);

initerr:
	TExecUnlockAtom(mod->exec, audio->hwndatom, TATOMF_KEEP);
	TExecFree(mod->exec, audio);
	return TFALSE;
}

/*****************************************************************************/
/*
**	samples_written = aud_write()
*/

LOCAL TINT aud_write(TMOD_DEV *mod)
{
	struct DSAudio *audio = mod->hostspecific;
	TAPTR adr1, adr2;
	TUINT size1, size2;
	TINT numsmp, wrlen, overlen;
	TINT playcursor;
	TINT freelen;

	numsmp = mod->writecursor - mod->playcursor;
	if (numsmp <= 0) numsmp += mod->mixbufsize;
	if (numsmp == 0) return 0;

	IDirectSoundBuffer_GetCurrentPosition(audio->dsbuf, &playcursor, NULL);

	freelen = playcursor - audio->lastpos;
	if (freelen < 0) freelen += mod->mixbufsize * mod->mixbps;	
	if (freelen == 0) return 0;
	
	wrlen = numsmp * mod->mixbps;
	if (wrlen > freelen) wrlen = freelen;

	overlen = (mod->playcursor - mod->mixbufsize) * mod->mixbps + wrlen;
	if (overlen > 0)
	{
		/* combine to a single write */
		TExecCopyMem(mod->exec, mod->mixbuf, mod->mixbuf + mod->mixbufsize * mod->mixbps, overlen);
	}

	if (IDirectSoundBuffer_Lock(audio->dsbuf, audio->lastpos, wrlen, &adr1, &size1, &adr2, &size2, 0) == DS_OK)
	{
		TExecCopyMem(mod->exec,
			(TINT8 *) mod->mixbuf + mod->playcursor * mod->mixbps, adr1, size1);
			
		if (adr2)
		{
			TExecCopyMem(mod->exec, 
				(TINT8 *) mod->mixbuf + mod->playcursor * mod->mixbps + size1, adr2, size2);
		}

		IDirectSoundBuffer_Unlock(audio->dsbuf, adr1, size1, adr2, size2);

		audio->lastpos = (audio->lastpos + wrlen) % (mod->mixbufsize * mod->mixbps);
		wrlen /= mod->mixbps;
		mod->playcursor = (mod->playcursor + wrlen) % mod->mixbufsize;
		
		return wrlen;
	}

	return 0;
}

/*****************************************************************************/
/*
**	signals = aud_wait(mod, waitsig)
*/

LOCAL TUINT aud_wait(TMOD_DEV *mod, TUINT sigmask)
{
	struct DSAudio *audio = mod->hostspecific;
	TAPTR hal = TExecGetHALBase(mod->exec);
	struct TTask *task = THALFindSelf(hal);
	struct HALWinThread *wth = THALGetObject(&task->tsk_Thread, struct HALWinThread);
	TUINT sig;
	TINT idx;
	HANDLE events[2];
	events[0] = audio->wrapevent;
	events[1] = audio->sigevent;

	for (;;)
	{
		EnterCriticalSection(&wth->hwt_SigLock);
		sig = wth->hwt_SigState & sigmask;
		wth->hwt_SigState &= ~sigmask;
		LeaveCriticalSection(&wth->hwt_SigLock);
		if (sig) break;
		idx = WaitForMultipleObjects(2, events, FALSE, INFINITE) - WAIT_OBJECT_0;
		if (idx == 0) break;
	}
	
	return sig;
}

/*****************************************************************************/
/*
**	aud_exit(mod)
*/

LOCAL TVOID aud_exit(TMOD_DEV *mod)
{
	struct DSAudio *audio = mod->hostspecific;
	IDirectSoundBuffer_Release(audio->dsbuf);
	TExecFree(mod->exec, mod->mixbuf);
	IDirectSound_Release(audio->ds);
	CloseHandle(audio->wrapevent);
	TExecUnlockAtom(mod->exec, audio->hwndatom, TATOMF_KEEP);
	TExecFree(mod->exec, audio);
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: audio_ds.c,v $
**	Revision 1.5  2004/08/06 18:02:12  tmueller
**	some buffer synchronization problems fixed
**	
**	Revision 1.4  2004/04/27 15:40:13  dtrompetter
**	changed includepath
**	
**	Revision 1.3  2004/03/20 00:29:13  tmueller
**	using buffersize 4096 now instead of 8192
**	
**	Revision 1.2  2003/12/13 14:12:38  tmueller
**	Working again
**	
**	Revision 1.1.1.1  2003/12/11 07:20:03  tmueller
**	Krypton import
**	
**	Revision 1.3  2003/10/20 14:27:28  tmueller
**	Field names in tek/type.h changed: TTime, TDate, TTagItem
**	
**	Revision 1.2  2003/10/17 23:22:35  tmueller
**	applied exec structure changes
**
**	Revision 1.1  2003/10/12 19:35:18  tmueller
**	win32 backend added
*/
