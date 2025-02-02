
/*
**	$Id: audio_ahi.c,v 1.1.1.1 2003/12/11 07:20:02 tmueller Exp $
**	audio/amiga/audio_ahi.c - Amiga AHI audio backend
*/

#include "audio_mod.h"

#include <tek/debug.h>
#include <tek/mod/exec.h>		/* exec-private structures */
#include <tek/proto/hal.h>		/* HAL module interface */
#include <tek/mod/hal.h>		/* HAL private stuff */
#include <tek/mod/amiga/hal.h>	/* Amiga private stuff */

#include <devices/ahi.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <proto/ahi.h>
#include <proto/exec.h>

#define SysBase *((struct ExecBase **) 4L)
#define AHIBase a->ahiBase

/*****************************************************************************/

struct AHIAudioBuffer
{
	struct AHISampleInfo sampleInfo;
	LONG soundNum;
};

struct AHIAudio					/* Host specific */
{
	struct AHIAudioBuffer buffer[2];
	struct Library *ahiBase;
	struct AHIRequest *ahiReq;
	struct AHIAudioCtrl *ahiCtrl;
	struct Hook soundHook;
	ULONG modeID;

	TAPTR halBase;
	struct TTask *tekTask;

	ULONG flags;

	struct MsgPort *msgPort;	/* device I/O port */
	ULONG ahiFmt;				/* AHI sample format */
	LONG numSmp;				/* samples per frame */	
	LONG outFreq;				/* output frequency */
	LONG bitDepth;				/* bits per sample (8/16) */
	LONG numChan;				/* number of channels (1/2) */
	LONG bufLen;				/* bytes per frame */
	
	APTR doub[2];				/* master doublebuffer switch */
};

#define AFL_PLAY			0x0001
#define AFL_EXIT			0x0002
#define AFL_START			0x0004
#define AFL_STOP			0x0008
#define AFL_REFILL			0x0010

/*****************************************************************************/

typedef ULONG (*HOOKENTRY)(APTR data, APTR object, APTR message);
#include "entries.c"

/*****************************************************************************/
/*
**	soundfunc
*/

static ULONG 
SoundFunc(struct Hook *hook, struct AHIAudioCtrl *actrl, 
	struct AHISoundMessage *smsg)
{
	struct AHIAudio *a = actrl->ahiac_UserData;
	struct AHIAudioBuffer *buf;

	buf = a->doub[0];
	a->doub[0] = a->doub[1];
	a->doub[1] = buf;
	
	AHI_SetSound(0, buf->soundNum, 0, 0, actrl, NULL);
	a->flags |= AFL_REFILL;
	
	hal_doevent(a->halBase, &a->tekTask->tsk_SigEvent);

	return 0;
}

/*****************************************************************************/
/*
**	findmode
*/

static const TUINT defrates[] =
{ 44100, 32000, 48000, 22050, 11025, 8000, 0xffffffff };

static const TUINT deffmts[] =
{ TADIOFMT_S16S, TADIOFMT_S8S, TADIOFMT_S16M, 
	TADIOFMT_S8M, TADIOFMT_INVALID };


static TBOOL findmode(TMOD_DEV *mod, struct AHIAudio *a)
{
	TUINT *formats, format;
	TUINT *rates, rate, minrate = 22050, maxrate = 22050;

	rates = TGetTag(mod->inittags, TADIOT_PrefRates, (TTAG) defrates);
	if (rates)
	{
		while ((rate = *rates++) != 0xffffffff)
		{
			if (rate > maxrate) maxrate = rate;
			if (rate < minrate) minrate = rate;
		}
	}				

	formats = TGetTag(mod->inittags, TADIOT_PrefFormats, (TTAG) deffmts);
	if (formats)
	{
		while ((format = *formats++) != TADIOFMT_INVALID)
		{
			switch (format)
			{
				case TADIOFMT_U8M:
				case TADIOFMT_S8M:
					a->ahiFmt = AHIST_M8S;
					a->bitDepth = 8;
					a->numChan = 1;
					break;
				case TADIOFMT_U8S:
				case TADIOFMT_S8S:
					a->ahiFmt = AHIST_S8S;
					a->bitDepth = 8;
					a->numChan = 2;
					break;
				case TADIOFMT_S16M:
					a->ahiFmt = AHIST_M16S;
					a->bitDepth = 16;
					a->numChan = 1;
					break;
				case TADIOFMT_S16S:
					a->ahiFmt = AHIST_S16S;
					a->bitDepth = 16;
					a->numChan = 2;
					break;
				default:
					return TFALSE;
			}

			if (a->numChan == 1)
			{
				a->modeID = AHI_BestAudioID(
					AHIDB_Realtime, TRUE,
					AHIDB_MinMixFreq, minrate,
					AHIDB_MaxMixFreq, maxrate,
					AHIDB_Bits, a->bitDepth,
					TAG_DONE);
			}
			else if (a->numChan == 2)
			{
				a->modeID = AHI_BestAudioID(
					AHIDB_Panning, TRUE,			//?
					AHIDB_Stereo, TRUE,
					AHIDB_Realtime, TRUE,
					AHIDB_MaxChannels, 2,
					AHIDB_MinMixFreq, minrate,
					AHIDB_MaxMixFreq, maxrate,
					AHIDB_Bits, a->bitDepth,
					TAG_DONE);
			}

			if (a->modeID != AHI_INVALID_ID)
			{
				mod->mixfmt = format;
				return TTRUE;
			}
		}
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	allocaudio
*/

static TBOOL allocaudio(TMOD_DEV *mod, struct AHIAudio *a)
{
	TUINT *rates, rate;
	
	if (a->modeID != AHI_INVALID_ID)
	{
		rates = TGetTag(mod->inittags, TADIOT_PrefRates, (TTAG) defrates);
		if (rates)
		{
			while ((rate = *rates++) != 0xffffffff)
			{
				ULONG index;
	
				AHI_GetAudioAttrs(a->modeID, NULL, AHIDB_IndexArg, rate, 
					AHIDB_Index, (ULONG) &index, TAG_DONE);
				AHI_GetAudioAttrs(a->modeID, NULL, AHIDB_FrequencyArg, index, 
					AHIDB_Frequency, (ULONG) &a->outFreq, TAG_DONE);
	
				a->ahiCtrl = AHI_AllocAudio(AHIA_AudioID, a->modeID,
					AHIA_MixFreq, a->outFreq, AHIA_Channels, 1, AHIA_Sounds, 2,
					AHIA_SoundFunc, (ULONG) &a->soundHook, 
					AHIA_UserData, (ULONG) a, TAG_DONE);
					
				if (a->ahiCtrl) return TTRUE;
			}
		}
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	device task init
*/

LOCAL TTASKENTRY TBOOL aud_init(TAPTR task)
{
	TAPTR exec = TExecBase(task);
	TMOD_DEV *mod = TGetTaskData(exec, task);
	struct AHIAudio *a;

	a = TAllocMMU(mod->exec, mod->mmu, sizeof(struct AHIAudio));
	if (!a) return TFALSE;
	mod->hostspecific = a;

	/* Get the event that can wakeup this TEK task */
	a->halBase = TGetHALBase(mod->exec);
	a->tekTask = task;

	a->msgPort = CreateMsgPort();
	if (a->msgPort)
	{
		a->ahiReq = CreateIORequest(a->msgPort, sizeof(struct AHIRequest));
		if (a->ahiReq)
		{
			a->ahiReq->ahir_Version = 4;
			if (OpenDevice(AHINAME, AHI_NO_UNIT,
				(struct IORequest *) a->ahiReq, NULL) == 0)
			{
				a->ahiBase = (struct Library *) a->ahiReq->ahir_Std.io_Device;

				a->modeID = AHI_INVALID_ID;
				findmode(mod, a);

				if (allocaudio(mod, a))
				{
					AHI_GetAudioAttrs(AHI_INVALID_ID, a->ahiCtrl, 
						AHIDB_MaxPlaySamples, (ULONG) &a->numSmp, TAG_DONE);

					a->bufLen = a->numSmp * (a->bitDepth >> 3) * a->numChan;

					InitHook(&a->soundHook, (HOOKENTRY) SoundFunc, NULL);
					
					a->doub[0] = &a->buffer[0];
					a->doub[1] = &a->buffer[1];
					
					a->buffer[0].soundNum = 0;
					a->buffer[1].soundNum = 1;
					
					a->buffer[0].sampleInfo.ahisi_Type = a->ahiFmt;
					a->buffer[1].sampleInfo.ahisi_Type = a->ahiFmt;
					a->buffer[0].sampleInfo.ahisi_Length = a->numSmp;
					a->buffer[1].sampleInfo.ahisi_Length = a->numSmp;

					a->buffer[0].sampleInfo.ahisi_Address =
						AllocMem(a->bufLen, MEMF_PUBLIC | MEMF_CLEAR);
					if (a->buffer[0].sampleInfo.ahisi_Address)
					{
						a->buffer[1].sampleInfo.ahisi_Address = 
							AllocMem(a->bufLen, MEMF_PUBLIC | MEMF_CLEAR);
						if (a->buffer[1].sampleInfo.ahisi_Address)
						{
							if (AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, 
								&a->buffer[0].sampleInfo, a->ahiCtrl) == 0)
							{
								if (AHI_LoadSound(1, AHIST_DYNAMICSAMPLE,
									&a->buffer[1].sampleInfo, a->ahiCtrl) == 0)
								{
									mod->mixfreq = a->outFreq;
									mod->mixbps = TADIO_GETNUMCHANNELS(mod->mixfmt) * TADIO_GETBYTESPERCHAN(mod->mixfmt);
									mod->fragsize = a->numSmp;
									mod->mixbufsize = mod->fragsize * 2;
									mod->mixbuf = TAllocMMU0(mod->exec, mod->mmu, mod->mixbufsize * 2 * mod->mixbps);
									if (mod->mixbuf)
									{
										/* Attributes that can be queried by the user */
										mod->attributes[0].tti_Tag = TADIOT_MixFormat;
										mod->attributes[0].tti_Value = (TTAG) mod->mixfmt;
										mod->attributes[1].tti_Tag = TADIOT_MixRate;
										mod->attributes[1].tti_Value = (TTAG) mod->mixfreq;
										mod->attributes[2].tti_Tag = TADIOT_FragmentSize;
										mod->attributes[2].tti_Value = (TTAG) mod->fragsize;
										mod->attributes[3].tti_Tag = TTAG_DONE;
	
										TInitList(&mod->chanlist);
										mod->port = TGetUserPort(mod->exec, mod->task);

										AHI_ControlAudio(a->ahiCtrl, AHIC_Play, TRUE, TAG_DONE);
											AHI_Play(a->ahiCtrl,
											AHIP_BeginChannel, 0,
											AHIP_Freq, a->outFreq,
											AHIP_Vol, 0x10000L,
										//	AHIP_Pan, 0x8000L,
											AHIP_Sound, 1,
											AHIP_Offset, 0, AHIP_Length, 0, AHIP_EndChannel, NULL, TAG_DONE);

										tdbprintf(20,"audio init ok\n");

										return TTRUE;
									}
									AHI_UnloadSound(1, a->ahiCtrl);
								}
								AHI_UnloadSound(0, a->ahiCtrl);
							}
							FreeMem(a->buffer[1].sampleInfo.ahisi_Address, a->bufLen);
						}
						FreeMem(a->buffer[0].sampleInfo.ahisi_Address, a->bufLen);
					}
					AHI_FreeAudio(a->ahiCtrl);
				}
				CloseDevice((struct IORequest *) a->ahiReq);
			}
			DeleteIORequest((struct IORequest *) a->ahiReq);
		}
		DeleteMsgPort(a->msgPort);
	}
	TFree(mod->exec, mod->hostspecific);
	tdbprintf(20,"audio init failed\n");
	return TFALSE;
}

/*****************************************************************************/
/*
**	signals = aud_wait(mod, waitsig)
*/

LOCAL TUINT aud_wait(TMOD_DEV *mod, TUINT waitsig)
{
	struct AHIAudio *a = mod->hostspecific;
	hal_waitevent(a->halBase, &a->tekTask->tsk_SigEvent);
	return TSetSignal(mod->exec, 0, waitsig);
}

/*****************************************************************************/
/*
**	aud_exit(mod)
*/

LOCAL TVOID aud_exit(TMOD_DEV *mod)
{
	struct AHIAudio *a = mod->hostspecific;
	if (a)
	{
		AHI_ControlAudio(a->ahiCtrl, AHIC_Play, FALSE, TAG_DONE);
		AHI_UnloadSound(1, a->ahiCtrl);
		AHI_UnloadSound(0, a->ahiCtrl);
		FreeMem(a->buffer[1].sampleInfo.ahisi_Address, a->bufLen);
		FreeMem(a->buffer[0].sampleInfo.ahisi_Address, a->bufLen);
		AHI_FreeAudio(a->ahiCtrl);
		CloseDevice((struct IORequest *) a->ahiReq);
		DeleteIORequest((struct IORequest *) a->ahiReq);
		DeleteMsgPort(a->msgPort);
		TFree(mod->exec, a);
	}
	TFree(mod->exec, mod->mixbuf);
}

/*****************************************************************************/
/*
**	samples_written = aud_write()
*/

LOCAL TINT aud_write(TMOD_DEV *mod)
{
	return 0;
}

#undef AHIBase
#undef SysBase

/*****************************************************************************/
/*
**	Revision History
**	$Log: audio_ahi.c,v $
**	Revision 1.1.1.1  2003/12/11 07:20:02  tmueller
**	Krypton import
**	
*/

#if 0

		do
		{
			if (a->flags & AFL_REFILL)
			{
				struct AHIAudioBuffer *buf = a->doub[1];
				LONG bpc = a->bitDepth >> 3;

				(*a->fillFunc)(a->fillData, 
					buf->sampleInfo.ahisi_Address, (BYTE *) buf->sampleInfo.ahisi_Address + bpc, bpc * a->numChan,
					a->numSmp, a->fScale, a->bitDepth, a->numChan);

				a->flags &= ~AFL_REFILL;
			}
			else if (a->flags & AFL_START)
			{
				if (!(a->flags & AFL_PLAY))
				{
					struct AHIAudioBuffer *buf = a->doub[1];
					AHI_ControlAudio(a->ahiCtrl, AHIC_Play, TRUE, TAG_DONE);
					AHI_Play(a->ahiCtrl,
						AHIP_BeginChannel, 0,
						AHIP_Freq, a->outFreq,
						AHIP_Vol, 0x10000L,
					//	AHIP_Pan, 0x8000L,
						AHIP_Sound, buf->soundNum,
						AHIP_Offset, 0, AHIP_Length, 0, AHIP_EndChannel, NULL, TAG_DONE);
					a->flags |= AFL_PLAY | AFL_REFILL;
				}
				a->flags &= ~AFL_START;
			}
			else if (a->flags & AFL_STOP)
			{
				if (a->flags & AFL_PLAY)
				{
					AHI_ControlAudio(a->ahiCtrl, AHIC_Play, FALSE, TAG_DONE);
				}
				a->flags &= ~(AFL_STOP | AFL_PLAY);
			}
			else if (a->flags & AFL_EXIT)
			{
				if (a->flags & AFL_PLAY) a->flags |= AFL_STOP;
			}
			else
			{
				Wait(1L << a->playSig);
			}

		} while (a->flags != AFL_EXIT);


#endif

