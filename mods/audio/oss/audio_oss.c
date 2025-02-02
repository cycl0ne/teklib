
/*
**	$Id: audio_oss.c,v 1.3 2004/06/04 00:15:01 tmueller Exp $
**	audio/oss/audio_oss.c - OSS audio backend
*/

#include "audio_mod.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

struct OSSAudio					/* Host specific */
{
	int ossfd;					/* File descriptor */
	int ossfmt;					/* Native audio format */
	int ossrate;				/* Native rate */
	int osschans;				/* Number of channels */
	audio_buf_info ossbufinfo;	/* Fragment info block */
};

static const TUINT defrates[] =
{ 44100, 32000, 48000, 22050, 11025, 8000, 0xffffffff };

static const TUINT deffmts[] =
{ TADIOFMT_S16S, TADIOFMT_S8S, TADIOFMT_S16M, 
	TADIOFMT_S8M, TADIOFMT_INVALID };

/**************************************************************************
**
**	device task init
*/

LOCAL TTASKENTRY TBOOL aud_init(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_DEV *mod = TExecGetTaskData(exec, task);
	TUINT *formats, *rates, format, rate;
	int formatmask;
	struct OSSAudio *oss;
	
	oss = TExecAlloc(exec, mod->mmu, sizeof(struct OSSAudio));
	if (!oss) return TFALSE;
	mod->hostspecific = oss;

	/* open OSS descriptor */

	oss->ossfd = open("/dev/dsp", O_WRONLY);
	if (oss->ossfd == -1) goto err_devinit2;

	/*
	**	fragment size should be adjusted here (if at all)
	**	frag = 0x7fff000b;
	**	if (ioctl(mod->ossfd, SNDCTL_DSP_SETFRAGMENT, &frag) == -1) goto err_devinit;
	*/

	/* find format, preferrably a native one */

	if (ioctl(oss->ossfd, SNDCTL_DSP_GETFMTS, &formatmask) == -1) goto err_devinit;

	oss->osschans = 1;
	oss->ossfmt = AFMT_QUERY;
	if (ioctl(oss->ossfd, SNDCTL_DSP_SETFMT, &oss->ossfmt) == -1) goto err_devinit;

	oss->osschans = 1;
	if (ioctl(oss->ossfd, SNDCTL_DSP_CHANNELS, &oss->osschans) == -1) goto err_devinit;

	formats = (TUINT *) TGetTag(mod->inittags, TADIOT_PrefFormats, (TTAG) deffmts);
	if (formats)
	{
		while ((format = *formats++) != TADIOFMT_INVALID)
		{
			int ossfmt, chans;
			switch (format)
			{
				case TADIOFMT_U8M:
					ossfmt = AFMT_U8;
					chans = 1;
					break;
				case TADIOFMT_U8S:
					ossfmt = AFMT_U8;
					chans = 2;
					break;
				case TADIOFMT_S8M:
					ossfmt = AFMT_S8;
					chans = 1;
					break;
				case TADIOFMT_S8S:
					ossfmt = AFMT_S8;
					chans = 2;
					break;
				case TADIOFMT_S16M:
					ossfmt = AFMT_S16_NE;
					chans = 1;
					break;
				case TADIOFMT_S16S:
					ossfmt = AFMT_S16_NE;
					chans = 2;
					break;
				default:
					goto err_devinit;
			}

			/* we try to find native formats */
			if (!(formatmask & ossfmt)) continue;

			oss->ossfmt = ossfmt;
			if (ioctl(oss->ossfd, SNDCTL_DSP_SETFMT, &oss->ossfmt) == -1) goto err_devinit;
			if (oss->ossfmt != ossfmt) continue;

			oss->osschans = chans;
			if (ioctl(oss->ossfd, SNDCTL_DSP_CHANNELS, &oss->osschans) == -1) goto err_devinit;
			if (oss->osschans != chans) continue;

			break;
		}
	}

	switch (oss->ossfmt)
	{
		case AFMT_U8:
			mod->mixfmt = TADIOFMT_U8M + (oss->osschans - 1);
			break;
		case AFMT_S8:
			mod->mixfmt = TADIOFMT_S8M + (oss->osschans - 1);
			break;
		case AFMT_S16_NE:
			mod->mixfmt = TADIOFMT_S16M + (oss->osschans - 1);
			break;
		default:
			goto err_devinit;
	}

	/* find a frequency */

	oss->ossrate = 22050;	/* rate attempted as a last resort */
	rates = (TUINT *) TGetTag(mod->inittags, TADIOT_PrefRates, (TTAG) defrates);
	if (rates)
	{
		while ((rate = *rates++) != 0xffffffff)
		{
			oss->ossrate = rate;
			if (ioctl(oss->ossfd, SNDCTL_DSP_SPEED, &oss->ossrate) == -1) goto err_devinit;
			if (rate == oss->ossrate) break;
		}
	}
	if (ioctl(oss->ossfd, SNDCTL_DSP_SPEED, &oss->ossrate) == -1) goto err_devinit;

	/* Collect properties and init device */

	if (ioctl(oss->ossfd, SNDCTL_DSP_GETOSPACE, &oss->ossbufinfo) == -1) goto err_devinit;

	mod->mixfreq = oss->ossrate;
	mod->mixbps = TADIO_GETNUMCHANNELS(mod->mixfmt) * TADIO_GETBYTESPERCHAN(mod->mixfmt);
	mod->fragsize = oss->ossbufinfo.fragsize / mod->mixbps;
	mod->mixbufsize = mod->fragsize * 2;
	mod->mixbuf = TExecAlloc0(exec, mod->mmu, mod->mixbufsize * 2 * mod->mixbps);
	if (!mod->mixbuf) goto err_devinit;

	TInitList(&mod->chanlist);

	mod->port = TExecGetUserPort(exec, mod->task);

	tdbprintf1(2,"fragsize returned from device: %d\n", mod->fragsize);

	/* Attributes that can be queried by the user */

	mod->attributes[0].tti_Tag = TADIOT_MixFormat;
	mod->attributes[0].tti_Value = (TTAG) mod->mixfmt;
	mod->attributes[1].tti_Tag = TADIOT_MixRate;
	mod->attributes[1].tti_Value = (TTAG) mod->mixfreq;
	mod->attributes[2].tti_Tag = TADIOT_FragmentSize;
	mod->attributes[2].tti_Value = (TTAG) (mod->fragsize * 2);	/* ... a bit more on the safe side :-/ */
	mod->attributes[3].tti_Tag = TTAG_DONE;

	return TTRUE;

err_devinit:
	close(oss->ossfd);

err_devinit2:
	TExecFree(TExecBase, mod->hostspecific);

	return TFALSE;
}

/**************************************************************************
**
**	samples_written = aud_write()
*/

LOCAL TINT aud_write(TMOD_DEV *mod)
{
	struct OSSAudio *oss = mod->hostspecific;
	TINT numsmp, overlen, wrlen;

	numsmp = mod->writecursor - mod->playcursor;
	if (numsmp <= 0) numsmp += mod->mixbufsize;
	if (numsmp == 0) return 0;

	overlen = mod->playcursor + numsmp - mod->mixbufsize;
	if (overlen > 0)
	{
		/* combine to a single write */
		TExecCopyMem(TExecBase, mod->mixbuf, 
			mod->mixbuf + mod->mixbufsize * mod->mixbps, overlen * mod->mixbps);
	}

	wrlen = numsmp * mod->mixbps;

	/*
	**	ioctl(mod->ossfd, SNDCTL_DSP_GETOSPACE, &mod->ossbufinfo);
	**	if (mod->ossbufinfo.bytes < wrlen) return TFALSE;
	*/

	wrlen = write(oss->ossfd, (TINT8 *) mod->mixbuf + mod->playcursor * mod->mixbps, wrlen);
	wrlen /= mod->mixbps;
	/*if (wrlen != numsmp) tdbprintf2(10,"unaligned write to device: %d -> %d\n", numsmp, wrlen);*/
	mod->playcursor = (mod->playcursor + wrlen) % mod->mixbufsize;

	return wrlen;
}

/**************************************************************************
**
**	signals = aud_wait(mod, waitsig)
**	Nothing to do here for OSS, since we write on a blocking
**	descriptor. Only fetch and return the TEKlib signals.
*/

LOCAL TUINT aud_wait(TMOD_DEV *mod, TUINT waitsig)
{
	return TExecSetSignal(TExecBase, 0, waitsig);
}

/**************************************************************************
**
**	aud_exit(mod)
*/

LOCAL TVOID aud_exit(TMOD_DEV *mod)
{
	struct OSSAudio *oss = mod->hostspecific;
	if (oss)
	{
		close(oss->ossfd);
		TExecFree(TExecBase, oss);
	}
	TExecFree(TExecBase, mod->mixbuf);
}

/*
**	Revision History
**	$Log: audio_oss.c,v $
**	Revision 1.3  2004/06/04 00:15:01  tmueller
**	Fixed some minor quirks that produced warnings
**	
**	Revision 1.2  2003/12/12 11:52:21  fschulze
**	removed include <netinet/in.h>
**	
**	Revision 1.1.1.1  2003/12/11 07:19:57  tmueller
**	Krypton import
**	
**	Revision 1.3  2003/10/22 03:21:48  tmueller
**	TagItem name field changes applied
**	
**	Revision 1.2  2003/10/12 19:36:42  tmueller
**	Audio device separated into host-specific and host-independent routines
**	
**	Revision 1.1  2003/10/09 22:12:46  tmueller
**	OSS backend added
**	
*/
