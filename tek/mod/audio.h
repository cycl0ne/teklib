
#ifndef _TEK_MOD_AUDIO_H
#define _TEK_MOD_AUDIO_H

/*
**	$Id: audio.h,v 1.2 2005/08/11 18:38:58 tmueller Exp $
**	tek/mod/audio.h
*/

#include <tek/exec.h>
#include <tek/mod/io.h>
#include <tek/mod/ioext.h>

/* 
**	Audio formats
*/

#define TADIOFMT_INVALID	0xffffffff	/* not a valid audio format */
#define TADIOFMT_U8M		0x00000000	/* unsigned 8bit, mono */
#define TADIOFMT_U8S		0x00000001	/* unsigned 8bit, stereo */
#define TADIOFMT_S8M		0x00000400	/* signed 8bit, mono */
#define TADIOFMT_S8S		0x00000401	/* signed 8bit, stereo */
#define TADIOFMT_S16M		0x00000410	/* signed 16bit, native endian, mono */
#define TADIOFMT_S16S		0x00000411	/* signed 16bit, native endian, stereo */
#if 0
#define TADIOFMT_SB16M		0x00000510	/* signed 16bit, big endian, mono */
#define TADIOFMT_SB16S		0x00000511	/* signed 16bit, big endian, stereo */
#define TADIOFMT_SL16M		0x00000610	/* signed 16bit, little endian, mono */
#define TADIOFMT_SL16S		0x00000611	/* signed 16bit, little endian, stereo */
#endif

#define TADIOM_NUMCHAN		0x0000000f	/* Mask for number of channels -1 */
#define TADIOM_BYTESPC		0x000000f0	/* Mask for bytes per channel -1 */
#define TADIOF_BIG			0x00000100	/* Flag indicating big endian */
#define TADIOF_LITTLE		0x00000200	/* Flag indicating little endian */
#define TADIOF_SIGN			0x00000400	/* Flag indicating signedness */

#define TADIO_GETNUMCHANNELS(F)		(((F) & TADIOM_NUMCHAN) + 1)
#define TADIO_GETBYTESPERCHAN(F)	((((F) & TADIOM_BYTESPC) >> 4) + 1)

#define	TADIOT_PrefFormats	(TTAG_USER + 0x7001)
#define	TADIOT_PrefRates	(TTAG_USER + 0x7002)
#define TADIOT_MixFormat	(TTAG_USER + 0x7003)
#define TADIOT_MixRate		(TTAG_USER + 0x7004)
#define TADIOT_FragmentSize	(TTAG_USER + 0x7005)


/* 
**	Audio request
*/

struct TAudIORequest
{
	struct TStdIORequest audio_Std;		/* Standard I/O request header */
   	struct TAudIORequest *audio_Link;	/* Link to previous request */
	struct TTagItem *audio_Tags;		/* Additional attributes */
	TUINT audio_Format;					/* Format, default inserted by device */
	TUINT audio_Rate;					/* Frequency, default inserted by device */
	TUINT audio_Volume;					/* Volume, 0x00010000 is 100 percent */
	
	/* Private fields follow. An audio request *MUST* be allocated with
	** TAllocMsg(). Do not use sizeof(audioreq), use TGetSize(exec, audioreq)
	** to determine the actual size of the message */
};


/*
**	Revision History
**	$Log: audio.h,v $
**	Revision 1.2  2005/08/11 18:38:58  tmueller
**	io.h include added
**	
**	Revision 1.1.1.1  2003/12/11 07:17:51  tmueller
**	Krypton import
**	
**	Revision 1.4  2003/10/06 00:58:56  tmueller
**	Support macros now read "TADIO_GETNUMCHANNELS" and "TADIO_GETBYTESPERCHAN"
**	
**	Revision 1.3  2003/10/04 21:07:43  tmueller
**	Updated comments for struct TAudIORequest
**	
**	Revision 1.2  2003/09/29 12:17:33  tmueller
**	LogInfo header was missing. added
**	
*/

#endif
