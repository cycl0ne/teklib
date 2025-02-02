
/*
**	$Id: entries.c,v 1.4 2005/09/18 12:05:23 tmueller Exp $
** 	teklib/boot/ps2/entries.c - Host entrypoints (for building stand-alone tests)
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/teklib.h>
#include <tek/proto/hal.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/io.h>
/*#include <tek/proto/datatypehandler.h>*/
/*#include <tek/proto/imgproc.h>*/

extern TMODENTRY TUINT tek_init_iohnd_ps2io(TAPTR, struct TModule *, TUINT16, TTAGITEM *);
extern TMODENTRY TUINT tek_init_datatype_ident_targa(TAPTR, struct TModule *, TUINT16, TTAGITEM *);
extern TMODENTRY TUINT tek_init_datatype_codec_targa(TAPTR, struct TModule *, TUINT16, TTAGITEM *);
extern TMODENTRY TUINT tek_init_ps2common(TAPTR, struct TModule *, TUINT16, TTAGITEM *);
extern TMODENTRY TUINT tek_init_iohnd_stdio(TAPTR, struct TModule *, TUINT16, TTAGITEM *);

const struct TInitModule TEKModules[] =
{
	{"hal", tek_init_hal, TNULL, 0},
	{"exec", tek_init_exec, TNULL, 0},
	{"util", tek_init_util, TNULL, 0},
	{"io", tek_init_io, TNULL, 0},
	{"iohnd_ps2io", tek_init_iohnd_ps2io, TNULL, 0},
	/*{"imgproc", tek_init_imgproc, TNULL, 0},*/
	/*{"datatypehandler", tek_init_datatypehandler, TNULL, 0},*/
	/*{"datatype_ident_targa", tek_init_datatype_ident_targa, TNULL, 0},*/
	/*{"datatype_codec_targa", tek_init_datatype_codec_targa, TNULL, 0},*/
	{"ps2common", tek_init_ps2common, TNULL, 0},
	{"iohnd_stdio", tek_init_iohnd_stdio, TNULL, 0},
	{TNULL}
};
