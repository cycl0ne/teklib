
#ifndef _TEK_CONFIG_INTENT_H
#define _TEK_CONFIG_INTENT_H

/*
**	$Header: /cvs/teklib/teklib/tek/config/intent.h,v 1.6 2005/01/30 06:21:38 tmueller Exp $
*/

/*****************************************************************************/
/* 
**	Elementary types
*/

#include <elate/elate.h>

typedef void				TVOID;
typedef void *				TAPTR;
typedef char				TINT8;
typedef unsigned char		TUINT8;
typedef signed short		TINT16;
typedef unsigned short		TUINT16;
typedef signed int			TINT32;
typedef unsigned int		TUINT32;
typedef signed long			TINT64;
typedef unsigned long		TUINT64;
typedef float				TFLOAT;
typedef	double				TDOUBLE;

typedef unsigned int		TUINTPTR;	/* integer that can store a pointer */


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
	TUINT8 tmha_Chunk[40];
};

/*****************************************************************************/
/* 
**	HAL Object container
*/

struct THALObject
{
	TAPTR data[1];
};

typedef struct THALObject THALO;


/*****************************************************************************/
/*
**	Date type container
*/

typedef union
{
	TDOUBLE	tdtt_Double;
	TINT64	tdtt_Int64;

} TDATE_T;


/*****************************************************************************/
/* 
**	debug macros
*/

#ifdef TDEBUG
	#define platform_dbprintf(l,a)			tracef("(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__)
	#define platform_dbprintf1(l,a,b)		tracef("(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b)
	#define platform_dbprintf2(l,a,b,c)		tracef("(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c)
	#define platform_dbprintf3(l,a,b,c,d)	tracef("(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d)
	#define platform_dbprintf4(l,a,b,c,d,e)	tracef("(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d,e)
	#define platform_dbprintf5(l,a,b,c,d,e,f)	tracef("(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d,e,f)
	#define platform_dbprintf6(l,a,b,c,d,e,f,g)	tracef("(%02d %s/%s:%d) " a,l,__FILE__,__FUNCTION__,__LINE__,b,c,d,e,f,g)
	#define platform_fatal(l)				tracef("(8<: %s/%s:%d fatal error\n",__FILE__,__FUNCTION__,__LINE__); *((int *) 0) = 0;
#endif


/*****************************************************************************/
/* 
**	Default locations
*/

#define TEKHOST_SYSDIR		"/lib/tek/sys"
#define TEKHOST_MODDIR		"/lib/tek/mod"
#define TEKHOST_PROGDIR		TNULL


/*****************************************************************************/
/* 
**	Module name extension
*/

#define TEKHOST_EXTLEN		3


/*****************************************************************************/
/* 
**	Inline
*/

#define TINLINE __inline


/*
**	Revision History
**	$Log: intent.h,v $
**	Revision 1.6  2005/01/30 06:21:38  tmueller
**	added fixed-size memory pools
**	
**	Revision 1.5  2005/01/29 22:26:52  tmueller
**	added alignment scheme to TMemNode and TMMUInfo
**	
**	Revision 1.4  2004/04/18 14:23:35  tmueller
**	Added native integer type capable of keeping a pointer, TUINTPTR
**	
**	Revision 1.3  2004/02/07 04:56:01  tmueller
**	Added TINLINE definition
**	
**	Revision 1.2  2004/01/05 10:55:45  mlukat
**	misc updates (module locations etc)
**	
**	Revision 1.1  2003/12/17 15:24:17  mlukat
**	initial version
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

#endif
