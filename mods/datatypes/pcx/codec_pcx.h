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
	struct TModule module;                             /* module header */
	TAPTR io;
	TAPTR util;

	TAPTR fp;
	TBOOL read,write;
	TINT format,bytesperrow;
	TTAGITEM *attribut_taglist;

	/* pcx typical stuff */
	TINT    width, height, length;
	TINT16  startx, starty,endx, endy,bytesperline,palinfo;
	TUINT8  version, depth, farbebenen, bitsperpixel, compression;
	TBOOL   grey, shortpalette, longpalette;

} TMOD_DTCODEC;

#define TExecBase TGetExecBase(dtcodec)
#define TUtilBase dtcodec->util

TBOOL read_pcx_open(TMOD_DTCODEC *dtcodec);
TBOOL read_pcx_data_normal(TMOD_DTCODEC *dtcodec, TUINT8 *data);
TBOOL read_pcx_data_normal_unpacked(TMOD_DTCODEC *dtcodec, TUINT8* data);
TINT read_pcx_encget(TMOD_DTCODEC *dtcodec, TUINT8 *pbyt, TUINT8 *pcnt);
TBOOL read_pcx_data_4bit(TMOD_DTCODEC *dtcodec, TUINT8 *tmpbuf);
TBOOL read_pcx_data_4bit_unpacked(TMOD_DTCODEC *dtcodec, TUINT8 *tmpbuf);
TVOID read_pcx_decode4bit(TMOD_DTCODEC *dtcodec, TUINT8 *tmpbuf, TUINT8 *data);
TBOOL read_pcx_palette(TMOD_DTCODEC *dtcodec, TIMGARGBCOLOR *palette);
TBOOL read_pcx_data(TMOD_DTCODEC *dtcodec, TUINT8 *data);
