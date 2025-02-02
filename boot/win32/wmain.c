
/* 
**	$Id: wmain.c,v 1.2 2004/01/03 17:40:29 dtrompetter Exp $
**	boot/win32/wmain.c - Win32 window application entrypoint 
*/

#include "boot/init.h"
#include <windows.h>

/*****************************************************************************/
/*
**	implement host main()
*/

int WINAPI 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
	int nCmdShow)
{
	TAPTR apptask;
	TTAGITEM tags[2];
	TUINT retval = EXIT_FAILURE;

	tags[0].tti_Tag = TExecBase_RetValP;
	tags[0].tti_Value = (TTAG) &retval; 
	tags[1].tti_Tag = TTAG_DONE; 

	apptask = TEKCreate(tags);
	if (apptask)
	{
		retval = EXIT_SUCCESS;
		TEKMain(apptask);

		TExecLockAtom(TGetExecBase(apptask), "win32.hwnd", 
			TATOMF_NAME | TATOMF_DESTROY);
	
		TDestroy(apptask);
	}

	return retval;
}

