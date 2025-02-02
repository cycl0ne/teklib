
/*
**	$Id: libstub.c,v 1.1.1.1 2003/12/11 07:17:59 tmueller Exp $
**	tek/mod/amiga/libstub.c - Amiga/MorphOS module stub
*/

#include <tek/exec.h>
#include <tek/debug.h>

#define MOD_VERSION		1
#define MOD_REVISION 	0

TMODENTRY TUINT tek_init(TAPTR selftask, TMODL *m, TUINT16 version, TTAGITEM *tags);

#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/initializers.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

#define AQuote(string) #string								/* Put quotes around the whole thing */
#define AVersion(major,minor) AQuote(major ## . ## minor)	/* Concatenate the two version-strings */
#define AmVersion(major,minor) AVersion(major,minor)		/* We need to do it this way to elimate the blank spaces */

#define VERSION		MOD_VERSION
#define REVISION	MOD_REVISION
#define LIBVER AmVersion(VERSION,REVISION)
#define LIBNAME MOD_NAME
#define LIBDATE "(" MOD_DATE ")"

#define LIBTYPE "TEKlib module"

struct ModBase
{
	struct Library LibNode;
	struct ExecBase *ExecBase;
	BPTR LibSegment;
};

LONG LibNull(void);

#ifdef __SASC

	#define LIBENT		__asm
	#define REG(x)		register __ ## x
	#define GCCREG(x)

#endif

#ifdef __GNUC__

#ifdef __PPC__

	#define LIBENT
	#define REG(x)
	#define GCCREG(x)

#else

	#define LIBENT 		__saveds
	#define REG(x)
	#define GCCREG(x) 	__asm( #x )

#endif

#endif

struct Library * LIBENT LibInit(
	REG(d0) struct Library *LibBase GCCREG(d0), 
	REG(a0) BPTR Segment GCCREG(a0), 
	REG(a6) struct ExecBase *ExecBase GCCREG(a6));

#ifdef TSYS_MORPHOS

struct Library *LIB_Open(void);
BPTR LIB_Expunge(void);
BPTR LIB_Close(void);
ULONG ModInitFunc(void);

#else

struct Library * LIBENT LibOpen(
	REG(a6) struct Library *LibBase GCCREG(a6));

BPTR LIBENT LibExpunge(
	REG(a6) struct Library *LibBase GCCREG(a6));

BPTR LIBENT LibClose(
	REG(a6) struct Library *LibBase GCCREG(a6));

LIBENT ULONG ModInitFunc(
	REG(a0) APTR self GCCREG(a0), 
	REG(a1) APTR mod GCCREG(a1), 
	REG(d0) UWORD version GCCREG(d0), 
	REG(a2) TAPTR tags GCCREG(a2));

#endif

LONG LibNull(void)
{
	return(NULL);
}

struct ExecBase *SysBase = NULL;
struct ModBase *ModBase = NULL;

ULONG LibVectors[] =
{
#ifdef TSYS_MORPHOS
	FUNCARRAY_32BIT_NATIVE,
	(ULONG) &LIB_Open,
	(ULONG) &LIB_Close,
	(ULONG) &LIB_Expunge,
#else
	(ULONG) &LibOpen,
	(ULONG) &LibClose,
	(ULONG) &LibExpunge,
#endif
	(ULONG) &LibNull,
	(ULONG) &ModInitFunc,
	0xFFFFFFFF
};

struct LibInitStruct
{
	ULONG	LibSize;
	void	*FuncTable;
	void	*DataTable;
	void	(*InitFunc)(void);
};

struct LibInitStruct LibInitStruct = {
	sizeof(struct ModBase),
	LibVectors,
	NULL,
	(void (*)(void)) &LibInit
};

static const char LibVersion[] = "$VER: " LIBNAME " " LIBVER " " LIBDATE LIBTYPE;

static const struct Resident RomTag = {
	RTC_MATCHWORD,
	(APTR) &RomTag,
	(APTR) (&RomTag + 1),
#ifdef TSYS_MORPHOS
	RTF_PPC | RTF_AUTOINIT,
#else
	RTF_AUTOINIT,
#endif
	VERSION,
	NT_LIBRARY,
	0,
	LIBNAME,
	(STRPTR) &LibVersion[6],
	&LibInitStruct
};

#ifdef TSYS_MORPHOS
ULONG __abox__ = 1;
#endif

struct Library * LIBENT LibInit(
		REG(d0) struct Library *LibBase GCCREG(d0),
		REG(a0) BPTR Segment GCCREG(a0),
		REG(a6) struct ExecBase *ExecBase GCCREG(a6))
{
	SysBase = ExecBase;
	ModBase = (struct ModBase *) LibBase;
	ModBase->LibNode.lib_Revision = REVISION;
	ModBase->LibSegment = Segment;
	ModBase->ExecBase = ExecBase;
	return(LibBase);
}

#ifdef TSYS_MORPHOS
struct Library *LIB_Open(void)
{
	struct Library *LibBase = (struct Library *)REG_A6;
#else
struct Library * LIBENT LibOpen(
	REG(a6) struct Library *LibBase GCCREG(a6))
{
#endif

	ModBase->LibNode.lib_Flags &= ~LIBF_DELEXP;
	ModBase->LibNode.lib_OpenCnt++;
	return(LibBase);
}

#ifdef TSYS_MORPHOS
BPTR LibExpunge(struct Library *LibBase);

BPTR LIB_Expunge(void)
{
	struct Library *LibBase = (struct Library *)REG_A6;

	return LibExpunge(LibBase);
}

BPTR LibExpunge(struct Library *LibBase)
{
#else
BPTR LIBENT LibExpunge(
	REG(a6) struct Library *LibBase GCCREG(a6))
{
#endif

	if (ModBase->LibNode.lib_OpenCnt == 0 && ModBase->LibSegment != NULL)
	{
		BPTR TempSegment = ModBase->LibSegment;
		Remove((struct Node *) ModBase);
		FreeMem((APTR)((ULONG)(ModBase) - (ULONG)(ModBase->LibNode.lib_NegSize)), 
			(ULONG)(ModBase->LibNode.lib_NegSize + ModBase->LibNode.lib_PosSize));
		ModBase = NULL;
		return(TempSegment);
	}
	else
	{
		ModBase->LibNode.lib_Flags |= LIBF_DELEXP;
		return(NULL);
	}
}

#ifdef TSYS_MORPHOS
BPTR LIB_Close(void)
{
	struct Library *LibBase = (struct Library *)REG_A6;
#else
BPTR LIBENT LibClose(
	REG(a6) struct Library *LibBase GCCREG(a6))
{
#endif

	if (ModBase->LibNode.lib_OpenCnt > 0)
	{
		--ModBase->LibNode.lib_OpenCnt;
	}

	if (ModBase->LibNode.lib_OpenCnt == 0 && (ModBase->LibNode.lib_Flags & LIBF_DELEXP))
	{
		return(LibExpunge(LibBase));
	}
	else
	{
		return(NULL);
	}
}

#ifdef TSYS_MORPHOS
ULONG ModInitFunc(void)
{
	APTR self = (APTR)REG_A0;
	APTR mod = (APTR)REG_A1;
	UWORD version = (UWORD)REG_D0;
	APTR tags = (APTR)REG_A2;
#else
LIBENT ULONG ModInitFunc(
	REG(a0) APTR self GCCREG(a0), 
	REG(a1) APTR mod GCCREG(a1), 
	REG(d0) UWORD version GCCREG(d0), 
	REG(a2) APTR tags GCCREG(a2))
{
#endif

	return tek_init((TAPTR) self, (TMODL *) mod, (TUINT16) version, (TTAGITEM *) tags);
}

/*
**	Revision History
**	$Log: libstub.c,v $
**	Revision 1.1.1.1  2003/12/11 07:17:59  tmueller
**	Krypton import
**	
**	Revision 1.5  2003/09/19 01:45:10  tmueller
**	romtag header was broken. fixed
**	
**	Revision 1.4  2003/09/17 16:51:38  tmueller
**	(TTAG) casts removed
**	
**	Revision 1.3  2003/06/05 23:21:08  tmueller
**	reverted
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
**	
*/
