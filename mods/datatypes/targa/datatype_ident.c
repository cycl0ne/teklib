
/*
**	datatype identify module
*/

#include <tek/exec.h>
#include <tek/proto/exec.h>
#include <tek/mod/datatypehandler.h>

typedef struct
{
	TMODL module;

} DTIDENT;

#define MOD_VERSION		0
#define MOD_REVISION	1

/*
**	Module prototypes
*/

static TMODAPI TVOID getidentdata(DTIDENT *dident, DTIdentifyData *IdentData);

/*
**	Module init
*/

TMODENTRY TUINT 
tek_init_datatype_ident_targa(TAPTR task, DTIDENT *mod, TUINT16 version, TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * 1;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(DTIDENT);		/* positive size */

		return 0;
	}

	mod->module.tmd_Version = MOD_VERSION;
	mod->module.tmd_Revision = MOD_REVISION;

	/* put module vectors in front */
	((TAPTR *) mod)[-1] = (TAPTR) getidentdata;

	return TTRUE;
}


/* 
**	Ident data
*/

static const TUINT8 datatypeidentdata[] =
{
	DTIC_USESUFFIX,
	DTIC_DONE
};

static TMODAPI TVOID getidentdata(DTIDENT *dident, DTIdentifyData *IdentData)
{
	IdentData->datatypecodecname = "datatype_codec_targa";
	IdentData->datatypeclass = DTCLASS_PIC;
	IdentData->datatypeidentdata = (TUINT8 *) datatypeidentdata;
	IdentData->datatypesuffix = "tga";
	IdentData->datatypefullname = "Targa picture";
	IdentData->datatypeshortname = "targa";
	IdentData->canwrite = TFALSE;
}
