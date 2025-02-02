
/*
**	datatype identify module
*/

#include <tek/exec.h>
#include <tek/proto/exec.h>
#include "ident_bmp.h"

#include <stdlib.h>
#include <string.h>

#define TSetTag(tp,t,val) { (tp).tti_Tag = (t); (tp).tti_Value = (TTAG)(val); }
#define TSetTagDone(tp)	{ (tp).tti_Tag = TTAG_DONE; }

typedef struct _TModDatatypeIdent
{
	struct TModule module;					/* module header */

	TTAGITEM save_taglist[14];

} TMOD_DATATYPEIDENT;

#define MOD_VERSION		0
#define MOD_REVISION	1

/*
**	module prototypes
*/
TMODAPI TVOID datatype_ident_getidentdata(TMOD_DATATYPEIDENT *dident, DTIdentifyData *IdentData);

/*
**	tek_init_<modname>
*/

TMODENTRY TUINT tek_init_datatype_ident_bmp(TAPTR selftask, TMOD_DATATYPEIDENT *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)					/* first call */
		{
			if (version <= MOD_VERSION)			/* version check */
			{
				return sizeof(TMOD_DATATYPEIDENT);	/* return module positive size */
			}
		}
		else									/* second call */
		{
			return sizeof(TAPTR) * 1;			/* return module negative size */
		}
	}
	else										/* third call */
	{
		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;

		/* put module vectors in front */
		((TAPTR *) mod)[-1 ] = (TAPTR) datatype_ident_getidentdata;

		return TTRUE;
	}
	return 0;
}

TMODAPI TVOID datatype_ident_getidentdata(TMOD_DATATYPEIDENT *dident, DTIdentifyData *IdentData)
{
	IdentData->datatypecodecname=datatypecodecname;
	IdentData->datatypeclass=datatypeclass;
	IdentData->datatypeidentdata=datatypeidentdata;
	IdentData->datatypesuffix=datatypesuffix;
	IdentData->datatypefullname=datatypefullname;
	IdentData->datatypeshortname=datatypeshortname;
	IdentData->canwrite=canwrite;
}
