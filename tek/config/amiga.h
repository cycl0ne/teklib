
/*
**	$Id: amiga.h,v 1.7 2005/09/13 02:44:34 tmueller Exp $
**	teklib/tek/config/amiga.h - Amiga types
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
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
	TAPTR data[2];
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

	void kprintf(char *, ...);
	#define platform_dbprintf(l,a)				\
		kprintf("(%02ld %s:%ld) "a,l,__FILE__,__LINE__)
	#define platform_dbprintf1(l,a,b)			\
		kprintf("(%02ld %s:%ld) "a,l,__FILE__,__LINE__,b)
	#define platform_dbprintf2(l,a,b,c)			\
		kprintf("(%02ld %s:%ld) "a,l,__FILE__,__LINE__,b,c)
	#define platform_dbprintf3(l,a,b,c,d)		\
		kprintf("(%02ld %s:%ld) "a,l,__FILE__,__LINE__,b,c,d)
	#define platform_dbprintf4(l,a,b,c,d,e)		\
		kprintf("(%02ld %s:%ld) "a,l,__FILE__,__LINE__,b,c,d,e)
	#define platform_dbprintf5(l,a,b,c,d,e,f)	\
		kprintf("(%02ld %s:%ld) "a,l,__FILE__,__LINE__,b,c,d,e,f)
	#define platform_dbprintf6(l,a,b,c,d,e,f,g)	\
		kprintf("(%02ld %s:%ld) "a,l,__FILE__,__LINE__,b,c,d,e,f,g)

#ifdef TSYS_MORPHOS
	#define platform_fatal(l)					\
		kprintf("(8<: %s/%s:%ld fatal error\n",	\
		__FILE__,__FUNCTION__,__LINE__);  		\
		__asm volatile ("twi 31,0,0");
#else
	#define platform_fatal(l)							\
		{TUINT16 illegal = 0x4afc;						\
		TVOID (*f)(TVOID) = (TVOID(*)(TVOID))&illegal;	\
		(*f)(); }
#endif

#endif

/*****************************************************************************/
/* 
**	Default locations
*/

#ifndef TEKHOST_SYSDIR
#define TEKHOST_SYSDIR		"TEKLIB:"
#endif
#define TEKHOST_MODDIR		TEKHOST_SYSDIR "mod/"
#define TEKHOST_PROGDIR		"PROGDIR:"

/*****************************************************************************/
/* 
**	Module name extensions
*/

#ifdef TSYS_MORPHOS
	#define TEKHOST_EXTSTR	".elfmod"
	#define TEKHOST_EXTLEN 	7
#else
	#define TEKHOST_EXTSTR	".mod"
	#define TEKHOST_EXTLEN	4
#endif

/*****************************************************************************/
/* 
**	Calling conventions
*/

#ifdef __SASC

	#pragma libcall ModBase tek_mod_enter 01e a09804
	TUINT32 tek_mod_enter(TAPTR, TAPTR, TUINT16, TAPTR);		/* proto */

	/*
	**	TEKlib's general calling convention for module functions and
	**	callbacks is stack-based on the Amiga platform. The following
	**	defininitions allow you to compile your applications with register
	**	arguments, too. Applications compiled with SAS/C can use modules
	**	compiled with gcc, and vice versa. See also the annotations in
	**	config.h
	*/

	#ifndef TMODAPI	
	#define TMODAPI __stdargs
	#endif

	#ifndef TLIBAPI	
	#define TLIBAPI __stdargs
	#endif

	#ifndef TMODCALL
	#define TMODCALL __stdargs
	#endif

	#ifndef TMODENTRY
	#define TMODENTRY __stdargs
	#endif
	
	#ifndef TCALLBACK
	#define TCALLBACK __stdargs
	#endif

	#ifndef TTASKENTRY	
	#define TTASKENTRY __stdargs
	#endif
	
#endif

#ifdef __GNUC__

#ifdef __PPC__

	#define tek_mod_enter(self, mod, version, tags) \
		LP4(0x1e, ULONG, tek_mod_enter, APTR, self, a0, APTR, mod, a1, UWORD, version, d0, APTR, tags, a2, \
		, ModBase, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#else

	#define tek_mod_enter(self, mod, version, tags) \
		LP4(0x1e, ULONG, tek_mod_enter, APTR, self, a0, APTR, mod, a1, UWORD, version, d0, APTR, tags, a2, \
		, ModBase)

#endif

#endif

/*****************************************************************************/
/* 
**	Inline
*/

#define TINLINE __inline

/*****************************************************************************/
/*
**	Revision History
**	$Log: amiga.h,v $
**	Revision 1.7  2005/09/13 02:44:34  tmueller
**	updated copyright reference
**	
**	Revision 1.6  2005/08/31 09:31:02  tmueller
**	corrected internal layout and size of TDATE structure
**	
**	Revision 1.5  2005/01/30 06:21:38  tmueller
**	added fixed-size memory pools
**	
**	Revision 1.4  2005/01/29 22:26:52  tmueller
**	added alignment scheme to TMemNode and TMMUInfo
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
**	Revision 1.6  2003/10/19 03:38:18  tmueller
**	Changed exec structure field names: Node, List, Handle, Module, ModuleEntry
**	
**	Revision 1.5  2003/09/19 01:50:18  tmueller
**	correction: amiga system path is now TEKLIB:
**	
**	Revision 1.4  2003/09/17 16:51:38  tmueller
**	(TTAG) casts removed
**	
**	Revision 1.3  2003/06/28 02:52:52  tmueller
**	Morphos build adapted to Morphos SDK
**	
**	Revision 1.2  2003/03/08 21:48:38  tmueller
**	Added defines for sysdir, moddir, progdir default paths
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/
