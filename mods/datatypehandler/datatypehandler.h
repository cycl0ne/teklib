#include <tek/exec.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/io.h>
#include <tek/proto/util.h>
#include <tek/proto/imgproc.h>
#include <tek/debug.h>

#include <tek/mod/datatypehandler.h>
#include <tek/mod/datatype_codec.h>
#include <tek/proto/datatype_codec.h>
#include <tek/proto/datatype_ident.h>

typedef struct _TModDatatypehandler
{
	struct TModule module;                                      /* module header */
	TAPTR exec;
	TAPTR io;
	TAPTR util;

	TBOOL identifiers;
	TINT numidentifiers;
	DTIdentifyData *IdentData;

	TLIST dtmodlist;                // list of all open datatype codecs
	TLIST dtfilterlist;             // filtered list of datatypes

} TMOD_DATATYPEHANDLER;

#define TExecBase TGetExecBase(dth)
#define TUtilBase dth->util

TMODAPI THNDL* TDthOpen(TMOD_DATATYPEHANDLER *dth, TTAGITEM *tags);
TMODAPI TTAGITEM* TDthGetAttrs(TMOD_DATATYPEHANDLER *dth, THNDL *handle);
TMODAPI TINT TDthDoMethod(TMOD_DATATYPEHANDLER *dth, THNDL *handle, TTAGITEM *taglist);
TMODAPI TLIST* TDthListDatatypes(TMOD_DATATYPEHANDLER *dth, TTAGITEM *filtertags);

TMODAPI TBOOL TDthSimpleLoadPicture(TMOD_DATATYPEHANDLER *dth, TSTRPTR filename, TIMGPICTURE *pic);

TBOOL DTH_ReadDatatypeIdentifiers(TMOD_DATATYPEHANDLER *dth);
TVOID DTH_FreeDatatypeIdentifiers(TMOD_DATATYPEHANDLER *dth);
TBOOL DTH_IdentifyFile(TMOD_DATATYPEHANDLER *dth,TAPTR fp, TINT8 *name, TINT dtnum);
TCALLBACK TVOID DTH_DestroyCodecHandle(THNDL *handle);
TCALLBACK TVOID DTH_DestroyListItem(THNDL *handle);
