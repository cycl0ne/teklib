#ifndef _TEK_CONFIG_PS2_H
#define _TEK_CONFIG_PS2_H

/*
**	teklib/tek/config/ps2.h - Playstation 2 configuration
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	and Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

/*****************************************************************************/
/*
**	Elementary types
*/

typedef char				TCHR;
typedef void				TVOID;
typedef void *				TAPTR;
typedef signed char			TINT8;
typedef unsigned char		TUINT8;
typedef signed short		TINT16;
typedef unsigned short		TUINT16;
typedef signed int			TINT32;
typedef unsigned int		TUINT32;
typedef	signed long			TINT64;
typedef unsigned long		TUINT64;
typedef float				TFLOAT;
typedef	double				TDOUBLE;
typedef signed int			TINTPTR;
typedef	unsigned int		TUINTPTR;

typedef __signed__ int TINT128 __attribute__((mode(TI))) __attribute__((aligned(16)));
typedef unsigned int TUINT128 __attribute__((mode(TI))) __attribute__((aligned(16)));

#define TSYS_HAVE_INT64
#define TSYS_HAVE_INT128

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

struct TMemNodeAlign { TUINT8 tmna_Chunk[16]; };
struct TMemManagerInfoAlign { TUINT8 tmua_Chunk[16]; };
struct TMemHeadAlign { TUINT8 tmha_Chunk[48]; };

/*****************************************************************************/
/*
**	HAL Object container
*/

struct THALObject { TUINTPTR tho_Chunk[4]; };

/*****************************************************************************/
/*
**	Date type container
*/

typedef union { TINT64 tdt_Int64; } TDATE_T;

/*****************************************************************************/
/*
**	Debug support
*/

#include <stdio.h>
#define TDEBUG_PLATFORM_PUTS(s) printf("%s",s)
#define TDEBUG_PLATFORM_FATAL() (*(int *) 0 = 0)

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
/*
**	Inline
*/

#define TINLINE __inline

#endif
