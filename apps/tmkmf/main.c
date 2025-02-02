
/* 
**	Entrypoint for a standalone version of tmkmf
**	(TEKlib linked statically to the application)
*/

#include <stdio.h>
#include <stdlib.h>
#include <tek/teklib.h>
#include <tek/proto/hal.h>
#include <tek/proto/exec.h>
#include <tek/proto/time.h>
#include <tek/proto/hash.h>
#include <tek/proto/util.h>
#include <tek/proto/unistring.h>
#include <tek/proto/io.h>

#include "tmkmf.c"
#include "global.c"
#include "navigate.c"
#include "parse.c"
#include "parse_tmkmf.c"

/* We don't have a proto.h for this backend module: */
extern TMODENTRY TUINT 
tek_init_iohnd_default(TAPTR, struct TModule *, TUINT16, TTAGITEM *);

/*****************************************************************************/
/* 
**	Pass entrypoints of statically linked modules:
*/

static const struct TInitModule initmodules[] =
{
	{"hal", tek_init_hal, TNULL, 0},
	{"exec", tek_init_exec, TNULL, 0},
	{"time", tek_init_time, TNULL, 0},
	{"hash", tek_init_hash, TNULL, 0},
	{"util", tek_init_util, TNULL, 0},
	{"unistring", tek_init_unistring, TNULL, 0},
	{"io", tek_init_io, TNULL, 0},
	{"iohnd_default", tek_init_iohnd_default, TNULL, 0},
	{TNULL}
};

/* 
**	Host entrypoint
*/

int main(int argc, char **argv)	
{
	TAPTR apptask;
	TTAGITEM tags[5];
	TUINT retval = EXIT_FAILURE;

	tags[0].tti_Tag = TExecBase_ArgC;
	tags[0].tti_Value = (TTAG) argc; 
	tags[1].tti_Tag = TExecBase_ArgV;
	tags[1].tti_Value = (TTAG) argv; 
	tags[2].tti_Tag = TExecBase_RetValP;
	tags[2].tti_Value = (TTAG) &retval; 
	/* pass static module entrypoints */
	tags[3].tti_Tag = TExecBase_ModInit;
	tags[3].tti_Value = (TTAG) initmodules; 
	tags[4].tti_Tag = TTAG_DONE; 

	apptask = TEKCreate(tags);
	if (apptask)
	{
		retval = EXIT_SUCCESS;
		TEKMain(apptask);
		TDestroy(apptask);
	}

	return retval;
}

