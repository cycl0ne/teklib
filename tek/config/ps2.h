
/*
**	$Id: ps2.h,v 1.7 2005/09/13 02:44:34 tmueller Exp $
**	tek/config/ps2.h - Playstation 2 platform specific
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	and Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

/*****************************************************************************/
/*
**	Elementary types
*/

typedef void				TVOID;
typedef void *				TAPTR;
typedef char				TINT8;
typedef unsigned char		TUINT8;
typedef signed short		TINT16;
typedef unsigned short		TUINT16;
typedef signed int			TINT32;
typedef unsigned int		TUINT32;
typedef	signed long			TINT64;
typedef unsigned long		TUINT64;
typedef float				TFLOAT;
typedef	double				TDOUBLE;
typedef	unsigned int		TUINTPTR;

typedef __signed__ int TINT128 __attribute__((mode(TI))) __attribute__((aligned(16)));
typedef unsigned int TUINT128 __attribute__((mode(TI))) __attribute__((aligned(16)));

typedef union
{
	TUINT128	ul128;
	TUINT64 	ul64[2];
	TUINT32 	ui32[4];
	TUINT16	 	us16[8];
	TUINT8	 	ub8[16];
	TFLOAT		fs32[4];
} TQWDATA __attribute__	((aligned(16)));

/*****************************************************************************/
/* 
**	Alignment of allocations
*/

struct TMemNodeAlign
{
	TUINT8 tmna_Chunk[16];
};

struct TMMUInfoAlign
{
	TUINT8 tmua_Chunk[16];
};

struct TMemHeadAlign
{
	TUINT8 tmha_Chunk[48];
};

/*****************************************************************************/
/* 
**	HAL Object container
*/

struct THALObject
{
	TAPTR data[4];
};

typedef struct THALObject THALO;

/*****************************************************************************/
/*
**	Date type container
*/

typedef union
{
	TDOUBLE tdtt_Double;
	struct { int hi,lo; } tdtt_HiLo;

} TDATE_T;

/*****************************************************************************/
/* 
**	Debug macros
*/

#ifdef TDEBUG
#include <stdio.h>
#define platform_dbprintf(l,a)				printf( \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__)
#define platform_dbprintf1(l,a,b)			printf( \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b)
#define platform_dbprintf2(l,a,b,c)			printf( \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c)
#define platform_dbprintf3(l,a,b,c,d)		printf( \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d)
#define platform_dbprintf4(l,a,b,c,d,e)		printf( \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d,e)
#define platform_dbprintf5(l,a,b,c,d,e,f)	printf( \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d,e,f)
#define platform_dbprintf6(l,a,b,c,d,e,f,g)	printf( \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d,e,f,g)
#define platform_fatal(l)					{printf( \
	"(8<: %s/%s:%d fatal error\n",__FILE__,__FUNCTION__,__LINE__); \
	*((int *) 0) = 0;}
#endif

/*****************************************************************************/
/* 
**	Default locations
*/

#define TEKHOST_SYSDIR		TNULL

#define TEKHOST_MODDIR		TEKHOST_SYSDIR
#define TEKHOST_PROGDIR		TNULL

/*****************************************************************************/
/* 
**	Module name extension
*/

#define TEKHOST_EXTSTR		TNULL
#define TEKHOST_EXTLEN		3

/*****************************************************************************/
/*      Revision History
**      $Log: ps2.h,v $
**      Revision 1.7  2005/09/13 02:44:34  tmueller
**      updated copyright reference
**
**      Revision 1.6  2005/08/31 09:31:02  tmueller
**      corrected internal layout and size of TDATE structure
**
**      Revision 1.5  2005/04/23 07:40:09  fschulze
**      128-bit integer now use mode TI
**
**      Revision 1.4  2005/01/30 06:21:38  tmueller
**      added fixed-size memory pools
**
**      Revision 1.3  2005/01/29 22:26:52  tmueller
**      added alignment scheme to TMemNode and TMMUInfo
**
**      Revision 1.2  2005/01/03 15:49:39  tmueller
**      added more types to playstation2-specific TQWDATA union
**
**      Revision 1.1  2004/08/10 22:03:20  fschulze
**      added
**
*/
