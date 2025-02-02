
/*
**	$Id: win32.h,v 1.8 2005/09/13 02:44:34 tmueller Exp $
**	tek/config/win32.h - Windows 32bit platform specific
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	TODO: how determine a 64bit version of Windows?
**	- An appropriate type is required for TUINTPTR
*/

#include <windows.h>

typedef void				TVOID;
typedef void *				TAPTR;
typedef char				TINT8;
typedef unsigned char		TUINT8;
typedef signed short		TINT16;
typedef unsigned short		TUINT16;
typedef signed int			TINT32;
typedef unsigned int		TUINT32;
typedef float				TFLOAT;
typedef	double				TDOUBLE;

typedef TUINT32				TUINTPTR;	/* integer that can store a pointer */

/*****************************************************************************/
/* 
**	Alignment of allocations
*/

struct TMemNodeAlign
{
	TUINT8 tmna_Chunk[8];
};

struct TMMUInfoAlign
{
	TUINT8 tmua_Chunk[8];
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
	struct { DWORD hi,lo; } tdtt_HiLo;
	LARGE_INTEGER tdtt_LInt;

} TDATE_T;

/*****************************************************************************/

#ifdef TDEBUG
	#include <stdio.h>
	#define platform_dbprintf(l,a)			printf("(%02d %s:%d) " a,l,__FILE__,__LINE__)
	#define platform_dbprintf1(l,a,b)		printf("(%02d %s:%d) " a,l,__FILE__,__LINE__,b)
	#define platform_dbprintf2(l,a,b,c)		printf("(%02d %s:%d) " a,l,__FILE__,__LINE__,b,c)
	#define platform_dbprintf3(l,a,b,c,d)	printf("(%02d %s:%d) " a,l,__FILE__,__LINE__,b,c,d)
	#define platform_dbprintf4(l,a,b,c,d,e)	printf("(%02d %s:%d) " a,l,__FILE__,__LINE__,b,c,d,e)
	#define platform_dbprintf5(l,a,b,c,d,e,f)	printf("(%02d %s:%d) " a,l,__FILE__,__LINE__,b,c,d,e,f)
	#define platform_dbprintf6(l,a,b,c,d,e,f,g)	printf("(%02d %s:%d) " a,l,__FILE__,__LINE__,b,c,d,e,f,g)
	#define platform_fatal(l)				printf("(8<: %s:%d fatal error\n",__FILE__,__LINE__); *((int *) 0) = 0;
#endif


#ifndef TMODENTRY
#define TMODENTRY __declspec(dllexport)
#endif

/*****************************************************************************/
/* 
**	Inline
*/

#define TINLINE __inline

/*****************************************************************************/
/*
**	Revision History
**	$Log: win32.h,v $
**	Revision 1.8  2005/09/13 02:44:34  tmueller
**	updated copyright reference
**	
**	Revision 1.7  2005/08/31 09:31:02  tmueller
**	corrected internal layout and size of TDATE structure
**	
**	Revision 1.6  2005/01/30 06:21:38  tmueller
**	added fixed-size memory pools
**	
**	Revision 1.5  2005/01/29 22:26:52  tmueller
**	added alignment scheme to TMemNode and TMMUInfo
**	
**	Revision 1.4  2004/10/23 18:14:00  dtrompetter
**	converted to unix text format
**	
**	Revision 1.3  2004/04/18 14:23:35  tmueller
**	Added native integer type capable of keeping a pointer, TUINTPTR
**	
**	Revision 1.2  2004/02/07 04:56:01  tmueller
**	Added TINLINE definition
**	
**	Revision 1.1.1.1  2003/12/11 07:18:09  tmueller
**	Krypton import
**	
**	Revision 1.2  2003/03/08 21:48:38  tmueller
**	Added defines for sysdir, moddir, progdir default paths
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
**	
*/
