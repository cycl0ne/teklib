
/*
**	$Id: tmkmf.c,v 1.6 2006/09/09 17:41:25 tmueller Exp $
**	teklib/build/tmkmf.c - Standalone tmkmf makefile generator
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <tek/teklib.h>
#include <tek/proto/hal.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/io.h>
#include <tek/proto/iohnd_default.h>

#include "src/teklib/init.c"
#include "src/teklib/teklib.c"
#include "src/teklib/string.c"
#include "src/hal/hal_mod.c"
#include "src/exec/exec_all.c"
#include "src/util/util_all.c"
#include "src/io/io_all.c"

#include "src/tmkmf/tmkmf.c"
#include "src/tmkmf/global.c"
#include "src/tmkmf/strings.c"
#include "src/tmkmf/navigate.c"
#include "src/tmkmf/parse.c"
#include "src/tmkmf/parse_tmkmf.c"

#if defined(TSYS_POSIX)
#include "src/teklib/posix/host.c"
#include "src/hal/posix/hal.c"
#include "src/io/posix/iohnd_default.c"
#elif defined(TSYS_WINNT)
#include "src/teklib/winnt/host.c"
#include "src/hal/winnt/hal.c"
#include "src/io/winnt/iohnd_default.c"
#endif

#if defined(TDEBUG)
#include "src/teklib/debug.c"
#endif

/* Entrypoints of statically linked modules */

static const struct TInitModule initmodules[] =
{
	{"hal", tek_init_hal, TNULL, 0},
	{"exec", tek_init_exec, TNULL, 0},
	{"util", tek_init_util, TNULL, 0},
	{"io", tek_init_io, TNULL, 0},
	{"iohnd_default", tek_init_iohnd_default, TNULL, 0},
	{TNULL}
};

/* Application entrypoint */

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
