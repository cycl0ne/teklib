#include <tek/exec.h>
#include <tek/proto/exec.h>
#include <tek/proto/io.h>
#include <tek/proto/util.h>
#include <tek/debug.h>

#include <tek/mod/imgproc.h>
#include <tek/mod/datatypehandler.h>
#include <tek/mod/datatype_codec.h>

typedef struct _TModDatatypeCodec
{
	struct TModule module;                                   /* module header */
	TAPTR io;
	TAPTR util;

	TAPTR fp;
	TBOOL read,write;
	TINT format,bytesperrow;
	TTAGITEM *attribut_taglist;

	/* flic typical stuff */
	TBOOL	flc;

	TINT	length;
	TINT	numframes;
	TINT    width, height, depth;
	TBOOL	loop;
	TINT	speed;

	TINT	currentframe;
	TUINT8 *framebuf;

} TMOD_DTCODEC;

#define TExecBase TGetExecBase(dtcodec)
#define TUtilBase dtcodec->util

TBOOL read_flic_open(TMOD_DTCODEC *dtcodec);
TVOID read_flic_rewind(TMOD_DTCODEC *dtcodec);
TBOOL read_flic_frame(TMOD_DTCODEC *dtcodec, TUINT8 *data, TIMGARGBCOLOR *palette);

