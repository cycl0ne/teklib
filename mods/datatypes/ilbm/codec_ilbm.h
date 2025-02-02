#include <tek/exec.h>
#include <tek/proto/exec.h>
#include <tek/proto/io.h>
#include <tek/proto/util.h>
#include <tek/proto/imgproc.h>
#include <tek/debug.h>

#include <tek/mod/imgproc.h>
#include <tek/mod/datatypehandler.h>
#include <tek/mod/datatype_codec.h>

typedef struct _TModDatatypeCodec
{
	struct TModule module;                                     /* module header */
	TAPTR io;
	TAPTR util;
	TAPTR imgp;

	TAPTR fp;
	TBOOL read,write;
	TINT format,bytesperrow;
	TTAGITEM *attribut_taglist;

	/* ilbm typical stuff */
	TINT16 width, height;
	TINT8 depth, realdepth, compression;
	TINT pix_w, pix_h;

	TBOOL ham6,ham8,ehb,lace,hires,superhires;

} TMOD_DTCODEC;

#define TExecBase TGetExecBase(dtcodec)
#define TUtilBase dtcodec->util

TBOOL read_ilbm_open(TMOD_DTCODEC *dtcodec);
TBOOL read_ilbm_palette(TMOD_DTCODEC *dtcodec, TIMGARGBCOLOR *palette);
TBOOL read_ilbm_data(TMOD_DTCODEC *dtcodec, TUINT8 *data);
TBOOL read_ilbm_data_uncompressed(TMOD_DTCODEC *dtcodec, TUINT8 *data, TINT numbytes);
TBOOL read_ilbm_data_compressed(TMOD_DTCODEC *dtcodec, TUINT8 *data, TINT numbytes);
TBOOL read_ilbm_seekchunk(TMOD_DTCODEC *dtcodec, TSTRPTR chunkname);
TVOID read_ilbm_ham62rgb(TMOD_DTCODEC *dtcodec, TUINT8 *srcdata, TUINT8 *dstdata,  TIMGARGBCOLOR *picpalette);
TVOID read_ilbm_ham82rgb(TMOD_DTCODEC *dtcodec, TUINT8 *srcdata, TUINT8 *dstdata,  TIMGARGBCOLOR *picpalette);
