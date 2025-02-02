
#ifndef _TEK_MOD_DATATYPEIDENT_H
#define	_TEK_MOD_DATATYPEIDENT_H

#include <tek/exec.h>
#include <tek/mod/datatypehandler.h>

/*
**	definitions for the datatypes identification module.
*/

TINT8 datatypecodecname[]="datatype_codec_flic";

TINT8 datatypefullname[]="FLI/FLC Animation";

TINT8 datatypeshortname[]="flic";

TINT8 datatypesuffix[]="fli,flc";

TINT8 datatypeclass=DTCLASS_ANIM;

TBOOL canwrite=TFALSE;

TUINT8 datatypeidentdata[] = 
{
	DTIC_USESUFFIX,
	DTIC_MOVE,4,
	DTIC_ORBYTE,2,0x11,0x12,
	DTIC_BYTE,0xAF,
	DTIC_DONE
};

#endif

