
#ifndef _TEK_MODS_AUDIO_MOD_H
#define _TEK_MODS_AUDIO_MOD_H

/*
**	$Id: audio_mod.h,v 1.3 2005/09/13 02:41:44 tmueller Exp $
**	teklib/mods/audio/audio_mod.h - Audio device internal
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/debug.h>
#include <tek/teklib.h>

#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/hal.h>
#include <tek/proto/io.h>

#include <tek/mod/audio.h>

#define MOD_VERSION		0
#define MOD_REVISION	1

typedef struct AudioDev
{
	struct TModule module;		/* Module header */

	TAPTR exec;					/* Execbase ptr */
	TAPTR mmu;					/* Memory manager */
	TAPTR lock;					/* Locking for module base structure */
	TAPTR task;					/* Device task */
	TAPTR port;					/* Device task's message port */
	TUINT refcount;				/* Count of module opens */

	TTAGITEM attributes[4];		/* Attributes that the user can query */
	TTAGITEM tasktags[2];		/* Task init tags */
	TTAGITEM *inittags;			/* Device init tags passed to TOpenModule */

	/* Dynamic channel management and mixer chracteristics */

	TAPTR mixerlock;			/* Protection for the channel list */
	TLIST chanlist;				/* Dynamic list of channels */
	TINT writecursor;			/* mixing goes to this position [samples] */
	TINT playcursor;			/* position written to the device [samples] */
	TINT fragsize;				/* as suggested by the device [samples] */

	TUINT8 *mixbuf;				/* Ptr to the mixing buffer */
	TUINT mixbufsize;			/* Size of mixing buffer [samples] */
	TUINT mixfmt;				/* Format; in TEKlib notation */
	TUINT mixfreq;				/* Mixing frequency */
	TUINT mixbps;				/* Mixing format bytes per sample */

	TAPTR hostspecific;			/* Ptr to host-specific data */

} TMOD_DEV;

#define TExecBase	TGetExecBase(mod)

/*
**	Audio request (internal version)
*/

typedef struct TAudioReqInternal
{
	struct TStdIORequest audio_Std;			/* Standard I/O request header */
   	struct TAudioReqInternal *audio_Link;	/* Previous chain member */
	struct TTagItem *audio_Tags;			/* Attributes */
	TUINT audio_Format;						/* Default inserted by device */
	TUINT audio_Rate;						/* Default inserted by device */
	TUINT audio_Volume;						/* 0x00010000 is 100% */

	/* Private. These fields are invisible for the user: */

	struct TAudioReqInternal *audio_Next;
	TUINT audio_Flags;
	TFLOAT audio_Pos;
	TFLOAT audio_Delta;
	TUINT audio_Length;
	TINT audio_NumChan;
	TINT audio_BytesPC;
	TINT audio_BytesPS;

} TIOMSG;

#define TAUDIOF_ACTIVE	0x0001
#define TAUDIOF_QUEUED	0x0002
#define TAUDIOF_ABORTED	0x0004

#ifndef LOCAL
#define LOCAL
#endif

LOCAL TTASKENTRY TBOOL aud_init(TAPTR task);
LOCAL TINT aud_write(TMOD_DEV *mod);
LOCAL TUINT aud_wait(TMOD_DEV *mod, TUINT waitsig);
LOCAL TVOID aud_exit(TMOD_DEV *mod);

/*****************************************************************************/
/*
**	Revision History
**	$Log: audio_mod.h,v $
**	Revision 1.3  2005/09/13 02:41:44  tmueller
**	updated copyright reference
**	
**	Revision 1.2  2003/12/13 14:12:56  tmueller
**	Minor sourcecode cleanup
**	
**	Revision 1.1.1.1  2003/12/11 07:19:56  tmueller
**	Krypton import
**	
**	Revision 1.1  2003/10/09 22:14:21  tmueller
**	Platform-independent and platform-specific code is now nicely seperated
*/

#endif
