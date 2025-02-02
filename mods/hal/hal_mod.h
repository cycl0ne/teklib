
#ifndef _TEK_HAL_HAL_MOD_H
#define	_TEK_HAL_HAL_MOD_H

/*
**	$Id: hal_mod.h,v 1.5 2005/09/13 02:42:16 tmueller Exp $
**	teklib/mods/hal/hal_mod.h - HAL module internal definitions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/teklib.h>
#include <tek/mod/hal.h>
#include <tek/mod/time.h>

#ifndef LOCAL
#define LOCAL
#endif

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

/*****************************************************************************/
/* 
**	HAL module structure
*/

typedef struct
{
	struct TModule hmb_Module;	/* Module header */
	TAPTR hmb_Specific;			/* Ptr to platform-specific data */
	TAPTR hmb_BootHnd;			/* Ptr to boot handle */

} TMOD_HAL;

/*****************************************************************************/
/* 
**	internal prototypes
*/

LOCAL TBOOL hal_init(TMOD_HAL *hal, TTAGITEM *tags);
LOCAL TVOID hal_exit(TMOD_HAL *hal);

LOCAL TAPTR hal_allocself(TAPTR boot, TUINT size);
LOCAL TVOID hal_freeself(TAPTR boot, TAPTR mem, TUINT size);

LOCAL TCALLBACK struct TTimeRequest *hal_open(TMOD_HAL *hal, TAPTR task, TTAGITEM *tags);
LOCAL TCALLBACK TVOID hal_close(TMOD_HAL *hal, TAPTR task);

LOCAL TVOID hal_subtime(TTIME *a, TTIME *b);
LOCAL TVOID hal_addtime(TTIME *a, TTIME *b);
LOCAL TINT hal_cmptime(TTIME *a, TTIME *b);

/*****************************************************************************/
/* 
**	API prototypes
*/

EXPORT TVOID hal_beginio(TMOD_HAL *hal, struct TTimeRequest *req);
EXPORT TINT hal_abortio(TMOD_HAL *hal, struct TTimeRequest *req);

EXPORT TAPTR hal_alloc(TMOD_HAL *hal, TUINT size);
EXPORT TVOID hal_free(TMOD_HAL *hal, TAPTR mem, TUINT size);
EXPORT TAPTR hal_realloc(TMOD_HAL *hal, TAPTR mem, TUINT oldsize,
	TUINT newsize);
EXPORT TVOID hal_copymem(TMOD_HAL *hal, TAPTR from, TAPTR to, TUINT numbytes);
EXPORT TVOID hal_fillmem(TMOD_HAL *hal, TAPTR dest, TUINT numbytes,
	TUINT8 fillval);
EXPORT TBOOL hal_initlock(TMOD_HAL *hal, THALO *lock);
EXPORT TVOID hal_destroylock(TMOD_HAL *hal, THALO *lock);
EXPORT TVOID hal_lock(TMOD_HAL *hal, THALO *lock);
EXPORT TVOID hal_unlock(TMOD_HAL *hal, THALO *lock);
EXPORT TBOOL hal_initthread(TMOD_HAL *hal, THALO *thread,
	TTASKENTRY TVOID (*function)(TAPTR task), TAPTR data);
EXPORT TVOID hal_destroythread(TMOD_HAL *hal, THALO *thread);
EXPORT TAPTR hal_findself(TMOD_HAL *hal);
EXPORT TAPTR hal_loadmodule(TMOD_HAL *hal, TSTRPTR name, TUINT16 version,
	TUINT *psize, TUINT *nsize);
EXPORT TBOOL hal_callmodule(TMOD_HAL *hal, TAPTR halmod, TAPTR task,
	TAPTR mod);
EXPORT TVOID hal_unloadmodule(TMOD_HAL *hal, TAPTR halmod);
EXPORT TBOOL hal_scanmodules(TMOD_HAL *hal, TSTRPTR path,
	TCALLBACK TBOOL (*callb)(TAPTR, TSTRPTR, TINT), TAPTR userdata);
EXPORT TTAG hal_getattr(TMOD_HAL *hal, TUINT tag, TTAG defval);
EXPORT TUINT hal_wait(TMOD_HAL *hal, TUINT signals);
EXPORT TVOID hal_signal(TMOD_HAL *hal, THALO *thread, TUINT signals);
EXPORT TUINT hal_setsignal(TMOD_HAL *hal, TUINT newsig, TUINT sigmask);
EXPORT TVOID hal_getsystime(TMOD_HAL *hal, TTIME *time);

EXPORT TDOUBLE hal_datetojulian(TMOD_HAL *hal, TDATE *date);
EXPORT TVOID hal_juliantodate(TMOD_HAL *hal, TDOUBLE jd, TDATE *date);

/*****************************************************************************/
/*
**	Revision History
**	$Log: hal_mod.h,v $
**	Revision 1.5  2005/09/13 02:42:16  tmueller
**	updated copyright reference
**	
**	Revision 1.4  2005/09/07 23:57:52  tmueller
**	added date to julian type conversion
**	
**	Revision 1.3  2005/04/01 18:39:28  tmueller
**	added boot handled to HAL module base
**	
**	Revision 1.2  2004/02/07 05:03:57  tmueller
**	Time support functions added
**	
**	Revision 1.1.1.1  2003/12/11 07:18:49  tmueller
**	Krypton import
**	
**	Revision 1.3  2003/10/28 08:50:51  tmueller
**	Cleanup of HAL-internal structures, style and formatting updated
**	
**	Revision 1.2  2003/09/17 16:32:12  tmueller
**	New TTAGITEM structure requires fewer casts
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
