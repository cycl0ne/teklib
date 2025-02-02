
#ifndef _TEK_MOD_WIN32_HAL_H
#define _TEK_MOD_WIN32_HAL_H

/*
**	$Id: hal.h,v 1.4 2003/12/22 23:59:52 tmueller Exp $
**	tek/mod/win32/hal.h - Win32 HAL module internal
*/

#include <tek/exec.h>
#include <tek/mod/hal.h>
#include <stdlib.h>
#include <windows.h>
#include <winreg.h>
#include <process.h>

/*****************************************************************************/

struct HALWinSpecific
{
	TTAGITEM hws_Tags[4];
	TSTRPTR hws_SysDir;
	TSTRPTR hws_ModDir;
	TSTRPTR hws_ProgDir;
	DWORD hws_TLSIndex;
	CRITICAL_SECTION hws_DevLock;		/* Locking for module base */
	TUINT hws_RefCount;					/* Open reference counter */
	TAPTR hws_ExecBase;					/* Inserted at device open */
	TAPTR hws_DevTask;					/* Created at device open */
	TLIST hws_ReqList;					/* List of pending requests */
	TFLOAT hws_TZDays;					/* Days west of GMT */
	TINT hws_TZSec;						/* Seconds west of GMT */
	TBOOL hws_UsePerfCounter;			/* Performance counter available */
	LONGLONG hws_PerfCStart;			/* Performance counter start */
	LONGLONG hws_PerfCFreq;				/* Performance counter frequency */
	TDOUBLE hws_PerfCStartDate;			/* Perfcounter calibration date */
};

struct HALWinTimer
{
	LARGE_INTEGER htm_BeginTime;
	LARGE_INTEGER htm_Freq;
};

struct HALWinThread
{
	TAPTR hwt_HALBase;
	HANDLE hwt_Thread;	
	long hwt_ThreadID;
	void *hwt_Data;
	void (*hwt_Function)(void *);

	HANDLE hwt_SigEvent;				/* Windows Event object */
	CRITICAL_SECTION hwt_SigLock;		/* Protection for sigstate */
	TUINT hwt_SigState;					/* State of signals */
};

struct HALWinModule
{
	HINSTANCE hwm_Lib;
	TUINT (*hwm_InitFunc)(TAPTR, TAPTR, TUINT16, TAPTR);
	TUINT16 hwm_Version;

};

/*****************************************************************************/
/*
**	Revision History
**	$Log: hal.h,v $
**	Revision 1.4  2003/12/22 23:59:52  tmueller
**	Added calibration field for measurements with performance counter
**	
**	Revision 1.3  2003/12/22 23:01:12  tmueller
**	Fixed waiting for an absolute date
**	
**	Revision 1.2  2003/12/12 22:51:09  tmueller
**	Now using performance counters for relative time measurement
**	
**	Revision 1.1.1.1  2003/12/11 07:18:00  tmueller
**	Krypton import
**	
**	Revision 1.3  2003/10/28 08:52:46  tmueller
**	Reworked HAL-internal structures
**	
**	Revision 1.2  2003/10/12 16:38:26  tschwinger
**	
**	Support for mingw32 (windows gcc) compiler added.
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
