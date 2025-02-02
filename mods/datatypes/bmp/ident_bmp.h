
#ifndef _TEK_MOD_DATATYPEIDENT_H
#define	_TEK_MOD_DATATYPEIDENT_H

#include <tek/exec.h>
#include <tek/mod/datatypehandler.h>

#define TExecBase TGetExecBase(dident)

/*
**	definitions for the datatypes identification module.
*/

TINT8 datatypecodecname[]="datatype_codec_bmp";

TINT8 datatypefullname[]="BMP Picture";

TINT8 datatypeshortname[]="bmp";

TINT8 datatypesuffix[]="bmp";

TINT8 datatypeclass=DTCLASS_PIC;

TBOOL canwrite=TTRUE;

TUINT8 datatypeidentdata[] = 
{
	DTIC_USESUFFIX,
	DTIC_BYTES,2,0x42,0x4d,
	DTIC_DONE
};

#endif

