
#ifndef _TEK_MOD_POSIX_HAL_H
#define _TEK_MOD_POSIX_HAL_H

/*
**	$Id: hal.h,v 1.1.1.1 2003/12/11 07:18:00 tmueller Exp $
**	tek/mod/darwin/hal.h - POSIX HAL module internal
*/

#include <tek/exec.h>
#include <pthread.h>
#include <sys/time.h>

struct posixspecific
{
	TTAGITEM tags[4];
	TSTRPTR sysdir;
	TSTRPTR moddir;
	TSTRPTR progdir;
	pthread_key_t tsdkey;
};

struct posixthread
{
	pthread_t pthread;	
	void *data;
	void (*function)(void *);
	TAPTR hal;
};

struct posixevent
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int status;
};

struct posixtimer
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	struct timeval timeval;
};

struct unixmod
{
	void *lib;
	TUINT (*initfunc)(TAPTR, TAPTR, TUINT16, TAPTR);
	TUINT16 version;
};

/*
**	Revision History
**	$Log: hal.h,v $
**	Revision 1.1.1.1  2003/12/11 07:18:00  tmueller
**	Krypton import
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1  2003/01/29 09:57:33  cybin
**	added tmkmf based make procedure for darwin platform.
**	
**	fixed one error in mod/hal/posix/hal.c where
**	
**	pthread_mutexattr_settype was using PTHREAD_MUTEX_RECURSIVE_NP
**	
**	and
**	
**	pthread_mutexattr_setkind_np PTHREAD_MUTEX_RECURSIVE
**	
**	which leads into non-portable use in every case and it might
**	not work on some plattforms.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
**	
*/

#endif
