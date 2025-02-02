#ifndef _TEK_MOD_TRUETYPE_H_
#define _TEK_MOD_TRUETYPE_H_ 1

/*
**	tek/mod/truetype.h
*/

#include <tek/type.h>


enum {
		TTE_OK,
		TTE_NOFSYS,
		TTE_NOMEM,
		TTE_NOFOUND,
		TTE_BADFONT
};


#define ONOROFF	0x01
#define XSHORT	0x02
#define YSHORT	0x04
#define REPEAT	0x08
#define XSAME	0x10
#define YSAME	0x20

#define ARG_1_AND_2_ARE_WORDS		0x0001
#define ARGS_ARE_XY_VALUES 			0x0002
#define XY_BOUND_TO_GRID			0x0004
#define WE_HAVE_A_SCALE		 		0x0008
#define MORE_COMPONENTS				0x0020
#define WE_HAVE_AN_X_AND_Y_SCALE	0x0040
#define WE_HAVE_A_TWO_BY_TWO		0x0080
#define WE_HAVE_INSTRUCTIONS		0x0100
#define USE_MY_METRICS				0x0200


typedef struct _ttpoint
{
	TFLOAT	x;
	TFLOAT	y;
} TTPOINT;

typedef struct _ttipoint
{
	TINT16	x;
	TINT16	y;
} TTIPOINT;

typedef struct ttf_glyf
{
	TINT16	numberOfContours;
	TINT16	xMin, yMin, xMax, yMax;
} TTF_GLYF ;

typedef struct _ttglyf
{
	TBOOL	filled;
	TINT		numpoint;
	TTPOINT	*points;
} TTOUTL;

typedef struct _ttvert
{
	TINT	outlines;
	TTOUTL	*outl;
	TINT	width,height;
} TTVERT;

typedef struct _ttstr
{
	TINT		numchar;
	TINT		maxpoints;
	TINT		width,height;
	TDOUBLE 	scale;
	TTVERT	*verts;
} TTSTR;

typedef struct longhormetric
{
	TUINT16	advanceWidth;
	TINT16	lsb;
} LONGHORMETRIC;

typedef struct ttf_hhea
{
	TUINT8	version[4];
	TINT16	ascender, descender, lineGap;
	TUINT16	advnaceWidthMax;
	TINT16	minLSB, minRSB, xMaxExtent;
	TINT16	caretSlopeRise, caretSlopeRun;
	TINT16	reserved[5];
	TINT16	metricDataFormat;
	TUINT16	numberOfHMetrics;
} TTF_HHEA;


typedef struct ttf_cmap0
{
	TUINT16	format;
	TUINT16	length;
	TUINT16	language;
	TUINT8	glyphIndexArray[256];
} TTF_CMAP0;

typedef struct ttf_cmap4 {
	TUINT16	format;
	TUINT16	length;
	TUINT16	version;
	TUINT16	segCountX2;
	TUINT16	searchRange;
	TUINT16	entrySelector;
	TUINT16	rangeShift;
} TTF_CMAP4;


typedef struct ttf_cmap_entry {
	TUINT16	platformID;
	TUINT16	encodingID;
	TUINT	offset;
} TTF_CMAP_ENTRY;

typedef struct ttf_cmap
{
	TUINT16	version;
	TUINT16	numberOfEncodingTables;
	TTF_CMAP_ENTRY	encodingTable[1];
} TTF_CMAP ;

typedef struct ttf_dir_entry
{
	TUINT8	tag[4];
	TUINT	checksum;
	TUINT	offset;
	TUINT	length;
} TTF_DIR_ENTRY;

typedef struct ttf_directory
{
	TUINT	sfntVersion;
	TUINT16	numTables;
	TUINT16	searchRange;
	TUINT16	entrySelector;
	TUINT16	rangeShift;
	TTF_DIR_ENTRY	list;
} TTF_DIRECTORY;


typedef struct _ttfont
{
	TAPTR	fontptr;
	TTF_CMAP	*cmap;
	TUINT8	*loca;
	TUINT8	*glyf;
	LONGHORMETRIC *hmtx;
	TUINT8	*buffer;
	TUINT8	*dbuffer;
	TINT		numtable;
	TINT		size;
	TINT		flags;
	TINT		unitem;
	TINT		locformat;
	TINT16	ascent,descent;
	TINT		nglyf;
	TINT		soffset;		/* start offset for outline in string */
	TINT		yoffset;
	TUINT16		numberOfHMetrics;
	TDOUBLE	scale;

} TTFONT;

#endif
