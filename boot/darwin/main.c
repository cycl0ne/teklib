
/* 
**	standalone entrypoint library
**	for regular TEKlib applications.
*/

#include <stdlib.h>
#include <stdio.h>
#include "init.h"

/**************************************************************************
**
**	implement host main()
*/

int main(int argc, char **argv)	
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
