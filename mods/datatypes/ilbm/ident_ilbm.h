
#ifndef _TEK_MOD_DATATYPEIDENT_H
#define	_TEK_MOD_DATATYPEIDENT_H

#include <tek/exec.h>
#include <tek/mod/datatypehandler.h>

/*
**	definitions for the datatypes identification module.
*/

TINT8 datatypecodecname[]="datatype_codec_ilbm";

TINT8 datatypefullname[]="ILBM Picture";

TINT8 datatypeshortname[]="ilbm";

TINT8 datatypesuffix[]="iff";

TINT8 datatypeclass=DTCLASS_PIC;

TBOOL canwrite=TFALSE;

TUINT8 datatypeidentdata[] =
{
	DTIC_BYTES,4,0x46,0x4f,0x52,0x4d,
	DTIC_MOVE,4,
	DTIC_BYTES,4,0x49,0x4c,0x42,0x4d,
	DTIC_DONE
};

#endif

