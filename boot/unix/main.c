
/* 
**	$Id: main.c,v 1.2 2004/04/18 13:57:53 tmueller Exp $
**	boot/unix/main.c - Standalone Unix application entrypoint 
*/

#include <stdlib.h>
#include "init.h"

/*****************************************************************************/
/*
**	implement host main()
*/

int 
main(int argc, char **argv)	
{
	TAPTR apptask;
	TTAGITEM tags[4];
	TUINT retval = EXIT_FAILURE;

	tags[0].tti_Tag = TExecBase_ArgC;
	tags[0].tti_Value = (TTAG) argc;
	tags[1].tti_Tag = TExecBase_ArgV;
	tags[1].tti_Value = (TTAG) argv; 
	tags[2].tti_Tag = TExecBase_RetValP;
	tags[2].tti_Value = (TTAG) &retval; 
	tags[3].tti_Tag = TTAG_DONE; 

	apptask = TEKCreate(tags);
	if (apptask)
	{
		retval = EXIT_SUCCESS;
		TEKMain(apptask);
		TDestroy(apptask);
	}

	return retval;
}
