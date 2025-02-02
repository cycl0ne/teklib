#include <tek/exec.h>
#include <tek/proto/exec.h>
#include <tek/proto/io.h>
#include <tek/proto/util.h>
#include <tek/debug.h>
#include <tek/proto/imgproc.h>
#include <tek/mod/datatypehandler.h>
#include <tek/mod/datatype_codec.h>

#include <math.h>

typedef struct _TModDatatypeCodec
{
	struct TModule module;                                   /* module header */
	TAPTR io;
	TAPTR util;
	TAPTR imgp;

	TAPTR fp;
	TBOOL read,write;
	TINT format,bytesperrow;
	TTAGITEM *attribut_taglist;

	/* bmp read typical stuff */
	TBOOL   rle4, rle8;
	TINT    start_data, length;
	TINT    width, height, usedcolors;
	TINT16  depth;

	/* bmp write typical stuff */
	TIMGPICTURE *srcpic;
	TINT compression;

} TMOD_DTCODEC;

#define TExecBase TGetExecBase(dtcodec)
#define TUtilBase dtcodec->util
#define TIOBase dtcodec->io
#define TIMGPBase dtcodec->imgp

TBOOL read_bmp_open(TMOD_DTCODEC *dtcodec);
TBOOL read_bmp_palette(TMOD_DTCODEC *dtcodec, TIMGARGBCOLOR *palette);
TBOOL read_bmp_data(TMOD_DTCODEC *dtcodec, TUINT8 *data);

TBOOL write_bmp(TMOD_DTCODEC *dtcodec);
