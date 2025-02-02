
#ifndef _TEK_MOD_DATATYPEIDENT_H
#define	_TEK_MOD_DATATYPEIDENT_H

#include <tek/exec.h>
#include <tek/mod/datatypehandler.h>

/*
**	definitions for the datatypes identification module.
*/

TINT8 datatypecodecname[]="datatype_codec_pcx";

TINT8 datatypefullname[]="PCX Picture";

TINT8 datatypeshortname[]="pcx";

TINT8 datatypesuffix[]="pcx";

TINT8 datatypeclass=DTCLASS_PIC;

TBOOL canwrite=TFALSE;

TUINT8 datatypeidentdata[] = 
{
	DTIC_USESUFFIX,
	DTIC_MOVE,1,
	DTIC_ORBYTE,4,0x00,0x02,0x03,0x05,
	DTIC_DONE
};

#endif

