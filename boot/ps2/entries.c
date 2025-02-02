
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
#include <tek/proto/time.h>
#include <tek/proto/util.h>
#include <tek/proto/hash.h>
#include <tek/proto/storagemanager.h>
#include <tek/proto/unistring.h>
#include <tek/proto/zlib.h>
#include <tek/proto/io.h>
#include <tek/proto/datatypehandler.h>
#include <tek/proto/imgproc.h>
#include <tek/proto/visual.h>

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
	{"time", tek_init_time, TNULL, 0},
	{"hash", tek_init_hash, TNULL, 0},
	{"storagemanager", tek_init_storagemanager, TNULL, 0},
	{"unistring", tek_init_unistring, TNULL, 0},
	{"zlib", tek_init_zlib, TNULL, 0},
	{"io", tek_init_io, TNULL, 0},
	{"iohnd_ps2io", tek_init_iohnd_ps2io, TNULL, 0},
	{"imgproc", tek_init_imgproc, TNULL, 0},
	{"datatypehandler", tek_init_datatypehandler, TNULL, 0},
	{"datatype_ident_targa", tek_init_datatype_ident_targa, TNULL, 0},
	{"datatype_codec_targa", tek_init_datatype_codec_targa, TNULL, 0},
	{"visual", tek_init_visual, TNULL, 0},
	{"ps2common", tek_init_ps2common, TNULL, 0},
	{"iohnd_stdio", tek_init_iohnd_stdio, TNULL, 0},
	{TNULL}
};

/*****************************************************************************/
/*
**	Revision History
**	$Log: entries.c,v $
**	Revision 1.4  2005/09/18 12:05:23  tmueller
**	added visual and ps2common
**	
**	Revision 1.3  2005/04/01 18:58:46  tmueller
**	Module entries corrected: imgproc, datatypehandler, pcx datatype
**	
**	Revision 1.2  2005/04/01 18:36:22  tmueller
**	A boot-specific object handle is now passed to all boot functions and later
**	handed over to the HAL module, where it helps to abstract from HW resources
**	
**	Revision 1.1  2005/03/13 20:01:39  fschulze
**	added
*/
