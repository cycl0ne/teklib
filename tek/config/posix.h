
/*
**	$Id: posix.h,v 1.5 2005/09/20 18:17:35 fschulze Exp $
**	teklib/tek/config/posix32 - POSIX types
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdint.h>

/*****************************************************************************/
/* 
**	Elementary types
*/

typedef void				TVOID;
typedef void *				TAPTR;
typedef signed char			TINT8;
typedef unsigned char		TUINT8;
typedef signed short		TINT16;
typedef unsigned short		TUINT16;
typedef signed int			TINT32;
typedef unsigned int		TUINT32;
typedef float				TFLOAT;
typedef	double				TDOUBLE;

typedef uintptr_t			TUINTPTR;	/* integer that can store a pointer */

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
	struct { int hi,lo; } tdtt_HiLo;

} TDATE_T;

/*****************************************************************************/
/* 
**	Debug macros
*/

#ifdef TDEBUG
#include <stdio.h>
#define platform_dbprintf(l,a)				fprintf(stderr, \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__)
#define platform_dbprintf1(l,a,b)			fprintf(stderr, \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b)
#define platform_dbprintf2(l,a,b,c)			fprintf(stderr, \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c)
#define platform_dbprintf3(l,a,b,c,d)		fprintf(stderr, \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d)
#define platform_dbprintf4(l,a,b,c,d,e)		fprintf(stderr, \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d,e)
#define platform_dbprintf5(l,a,b,c,d,e,f)	fprintf(stderr, \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d,e,f)
#define platform_dbprintf6(l,a,b,c,d,e,f,g)	fprintf(stderr, \
	"(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d,e,f,g)
#define platform_fatal(l)					{fprintf(stderr, \
	"(8<: %s/%s:%d fatal error\n",__FILE__,__FUNCTION__,__LINE__); \
	*((int *) 0) = 0;}
#endif

/*****************************************************************************/
/* 
**	Default locations
*/

#ifndef TEKHOST_SYSDIR
#ifdef __linux__
#define TEKHOST_SYSDIR		"/opt/tek/"
#else
#define TEKHOST_SYSDIR		"/usr/local/tek/"
#endif
#endif

#define TEKHOST_MODDIR		TEKHOST_SYSDIR "mod/"
#define TEKHOST_PROGDIR		TNULL

/*****************************************************************************/
/* 
**	Module name extension
*/

#ifdef TSYS_DARWIN
#define TEKHOST_EXTSTR		".dylib"
#define TEKHOST_EXTLEN 		6
#else
#define TEKHOST_EXTSTR		".so"
#define TEKHOST_EXTLEN		3
#endif

/*****************************************************************************/
/* 
**	Inline
*/

#define TINLINE __inline

/*****************************************************************************/
/*
**	Revision History
**	$Log: posix.h,v $
**	Revision 1.5  2005/09/20 18:17:35  fschulze
**	fbsd now uses stdint include
**	
**	Revision 1.4  2005/09/13 02:44:34  tmueller
**	updated copyright reference
**	
**	Revision 1.3  2005/08/31 09:31:02  tmueller
**	corrected internal layout and size of TDATE structure
**	
**	Revision 1.2  2005/01/30 06:21:38  tmueller
**	added fixed-size memory pools
**	
**	Revision 1.1  2005/01/29 22:20:43  tmueller
**	removed posix32.h config - new name is posix.h
**	
**	Revision 1.6  2004/06/04 08:25:20  tmueller
**	Great, so TINT8 still wasn't explicitely signed. Fixed.
**	
**	Revision 1.5  2004/05/07 07:38:25  fschulze
**	added fbsd include for TUINTPTR
**	
**	Revision 1.4  2004/04/18 14:23:35  tmueller
**	Added native integer type capable of keeping a pointer, TUINTPTR
**	
**	Revision 1.3  2004/04/17 02:45:02  tmueller
**	TDATE structure was larger than necessary on 64bit arch; fixed
**	
**	Revision 1.2  2004/02/07 04:56:01  tmueller
**	Added TINLINE definition
**	
**	Revision 1.1.1.1  2003/12/11 07:18:08  tmueller
**	Krypton import
**	
**	Revision 1.4  2003/10/28 08:55:38  tmueller
**	Added definitions for variable module name extensions
**	
**	Revision 1.3  2003/05/15 20:45:05  tmueller
**	Added TEKSYSDIR option. Hard-hard-default is still /opt/tek/, but it can
**	be overriden in the build procedure.
**	
**	Revision 1.2  2003/03/08 21:48:38  tmueller
**	Added defines for sysdir, moddir, progdir default paths
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.2  2003/02/13 18:27:06  cybin
**	darwin port now compiles with -DTSYS_DARWIN and uses correct
**	host and platform values.
**	
**	added files for io module. should be the same like posix32.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/
