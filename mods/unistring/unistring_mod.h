
#ifndef _TEK_MOD_UNISTRING_MOD_H
#define _TEK_MOD_UNISTRING_MOD_H

/*
**	$Id: unistring_mod.h,v 1.35 2005/09/13 02:42:42 tmueller Exp $
**	mods/unistring/unistring_mod.h - Dynamic Unicode string module
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/unistring.h>

#define MOD_VERSION		2
#define MOD_REVISION	0

#ifndef LOCAL
#define LOCAL
#endif
#ifndef EXPORT
#define EXPORT	TMODAPI
#endif

#define DEFNODESIZE		64			/* default size of fragments [elements] */
#define CASECONVRANGE	0x0200		/* range of case-convertible chars */
#define CHARINFORANGE	0x2700		/* range of char info table */

/*****************************************************************************/
/* character classes */

#define C_UNDEFINED		0x00

#define C_LATIN			0x10		/* latin letter */
#define C_CAPITAL		0x11		/* capital letter */
#define C_NOCASE		0x12		/* no case conversion possible */
#define	C_IRREV			0x14		/* case conversion irreversible */
#define C_MULTICHAR		0x18		/* case conversion is multichar */

#define C_SPACE			0x20
#define C_CONTROL		0x40

/*****************************************************************************/

typedef struct						/* Array fragment */
{
	struct TNode tan_Node;			/* Node header */
	TINT tan_AllocLength;			/* Allocated length [elements] */
	TINT tan_UsedLength;			/* Used length [elements] */

} TANODE;

typedef struct						/* Array cursor */
{
	TANODE *tac_Node;				/* Ptr to node containing the cursor */
	TINT tac_RelPos;				/* Relative cursor pos (in node) */
	TINT tac_AbsPos;				/* Absolute cursor pos */

} TACURSOR;

typedef struct						/* Array header */
{
	struct TList tah_List;			/* List of fragments */
	TACURSOR tah_Cursor;			/* Current cursor position */
	TINT tah_Length;				/* Total real length */
	TUINT16 tah_Flags;				/* Flags (see below) */
	TUINT16 tah_ElementSize;		/* Element size [1<<n bytes] */
	TUINT tah_NodeSize;				/* Suggested allocsize [elements] */
	TANODE tah_First;				/* Initial static node */
	TINT8 tah_Buffer[16];			/* Initial static node's buffer */

} TAHEAD;

#define TDSTRF_FREE			0x01	/* this entry is free */
#define TDSTRF_VALID		0x02	/* this entry is in valid state */
#define TDSTRF_8BIT_PRESENT	0x04	/* chars present from 0x80-0xff */
#define TDSTRF_UCS2_PRESENT	0x0c	/* chars present from 0x100-0x7fff */
#define TDSTRF_UCS4_PRESENT	0x1c	/* chars present from 0x8000-0x7fffffff */

typedef struct
{
	struct TModule us_Module;		/* Module header */

	TAPTR us_MMU;					/* Module global memory manager */
	TAPTR us_Lock;					/* Locking (super-instance only) */
	TAPTR us_Pool;					/* Memory pool (per instance) */

	TAHEAD **us_Array;				/* Array of ptrs to array headers */
	TINT us_ArraySize;				/* Allocated size of the array [bytes] */
	TINT us_NumTotal;				/* Total number of indices */
	TINT us_NumFree;				/* Number of unassigned indices */
	TINT us_LastFreed;				/* Hint: Last freed index */

	TINT16 **us_SmallToCaps;		/* Small-to-capital conv table */
	TINT16 **us_CapsToSmall;		/* Capital-to-small conv table */
	TINT8 *us_CharInfo;				/* Character case info table */

	TINT us_InitNodeSize;			/* Initial size of new fragments */
	TBOOL us_BigEndian;

	#ifdef TDEBUG
		TINT us_AllocCount;
	#endif

} TMOD_US;

#define TExecBase TGetExecBase(mod)

LOCAL TBOOL mod_initcaseconversion(TMOD_US *mod);

/*****************************************************************************/
/*
**	internal module functions
*/

EXPORT TANODE *_array_allocnode(TMOD_US *mod, TINT len, TINT elementsize);
EXPORT TVOID _array_freenode(TMOD_US *mod, TAHEAD *str, TANODE *node);
EXPORT TANODE *_array_allocnode_tasksafe(TMOD_US *mod, TINT len, TINT elsize);
EXPORT TVOID _array_freenode_tasksafe(TMOD_US *mod, TAHEAD *str, TANODE *node);

EXPORT TUString _array_alloc(TMOD_US *mod, TUINT flags);
EXPORT TVOID _array_free(TMOD_US *mod, TUString str);
EXPORT TUString _array_alloc_tasksafe(TMOD_US *mod, TUINT flags);
EXPORT TVOID _array_free_tasksafe(TMOD_US *mod, TUString str);

EXPORT TVOID _array_move(TMOD_US *mod, TUString srcidx, TUString dstidx);
EXPORT TVOID _array_move_tasksafe(TMOD_US *mod, TUString srcidx, TUString dstidx);

EXPORT TVOID array_debugprint(TMOD_US *mod, TUString idx);

#define convertelement(mod,src,tmp,ols,nls)	\
	(*(((TMODCALL TUINT8 *(**)(TAPTR,TUINT8 *,TUINT8 *,TINT,TINT))(mod))[-4-8]))(mod,src,tmp,ols,nls)
#define array_alloc(mod,flags)	\
	(*(((TMODCALL TUString(**)(TAPTR,TUINT))(mod))[-5-8]))(mod,flags)
#define array_free(mod,a)		\
	(*(((TMODCALL TVOID(**)(TAPTR,TUString))(mod))[-6-8]))(mod,a)
#define array_allocnode(mod,len,els) \
	(*(((TMODCALL TANODE*(**)(TAPTR,TINT,TINT))(mod))[-1-8]))(mod,len,els)
#define array_freenode(mod,arr,node) \
	(*(((TMODCALL TVOID(**)(TAPTR,TAHEAD*,TANODE*))(mod))[-2-8]))(mod,arr,node)
#define array_move(mod,src,dst) \
	(*(((TMODCALL TVOID(**)(TAPTR,TUString,TUString))(mod))[-19-8]))(mod,src,dst)

/*****************************************************************************/
/* 
**	array API
*/

EXPORT TINT array_ins(TMOD_US *mod, TUString idx, TAPTR data);
EXPORT TINT array_rem(TMOD_US *mod, TUString idx, TAPTR data);
EXPORT TINT array_seek(TMOD_US *mod, TUString idx, TINT steps, TINT mode);
EXPORT TINT array_get(TMOD_US *mod, TUString idx, TAPTR data);
EXPORT TINT array_set(TMOD_US *mod, TUString idx, TAPTR data);
EXPORT TINT array_length(TMOD_US *mod, TUString idx);
EXPORT TAPTR array_map(TMOD_US *mod, TUString idx, TINT of, TINT ln);
EXPORT TINT array_render(TMOD_US *mod, TUString idx, TUINT8 *dest,
	TINT offs, TINT len);
EXPORT TINT array_change(TMOD_US *mod, TUString idx, TUINT flags);
EXPORT TUString array_dup(TMOD_US *mod, TUString idx);
EXPORT TINT array_copy(TMOD_US *mod, TUString srcidx, TUString dstidx);
EXPORT TINT array_trunc(TMOD_US *mod, TUString idx);

LOCAL TINT _array_ins(TMOD_US *mod, TAHEAD *arr, TAPTR data);
LOCAL TINT _array_setsize(TMOD_US *mod, TAHEAD *arr, TUINT newels);
LOCAL TBOOL _array_rem(TMOD_US *mod, TAHEAD *arr, TAPTR data);
LOCAL TINT _array_seek(TAHEAD *arr, TACURSOR *cursor, TINT mode, TINT steps);
LOCAL TAPTR _array_maplinear(TMOD_US *mod, TAHEAD *arr, TINT start, TINT len);
LOCAL TINT _array_render(TMOD_US *mod, TAHEAD *arr, TUINT8 *dest,
	TINT startpos, TINT len, TUINT newels);
LOCAL TINT __array_seek(TAHEAD *arr, TACURSOR *cursor, TINT steps);
LOCAL TVOID _array_set(TAHEAD *arr, TAPTR data);
LOCAL TVOID _array_get(TAHEAD *arr, TAPTR data);
LOCAL TVOID __array_get(TACURSOR *cursor, TINT elementsize, TAPTR data);
LOCAL TUString _array_dup(TMOD_US *, TAHEAD *arr);
LOCAL TUINT getelementsize(TUINT flags);

/*****************************************************************************/
/* 
**	strings API
*/

LOCAL TAHEAD *_str_valid(TMOD_US *mod, TUString idx);
LOCAL TWCHAR _str_get(TAHEAD *arr, TINT pos);
LOCAL TINT _str_insert(TMOD_US *mod, TAHEAD *arr, TINT pos, TWCHAR c);

EXPORT TUString str_alloc(TMOD_US *mod, TUINT8 *initstr);
EXPORT TINT str_insert(TMOD_US *mod, TUString idx, TINT pos, TWCHAR c);
EXPORT TWCHAR str_remove(TMOD_US *mod, TUString idx, TINT pos);
EXPORT TAPTR str_map(TMOD_US *mod, TUString idx, TINT offs, TINT len, TUINT md);
EXPORT TINT str_render(TMOD_US *mod, TUString idx, TAPTR ptr, TINT ofs,
	TINT len, TUINT mode);
EXPORT TINT str_set(TMOD_US *mod, TUString idx, TINT pos, TWCHAR c);
EXPORT TWCHAR str_get(TMOD_US *mod, TUString idx, TINT pos);
EXPORT TINT str_insertstrn(TMOD_US *mod, TUString idx, TINT pos, TAPTR str,
	TINT len, TUINT mode);
EXPORT TUString str_encodeutf8(TMOD_US *mod, TUString idx);
EXPORT TINT str_insertutf8str(TMOD_US *mod, TUString idx, TINT pos,
	TUINT8 *utf8str);
EXPORT TINT str_crop(TMOD_US *mod, TUString idx, TINT pos, TINT len);
EXPORT TINT str_ncmp(TMOD_US *mod, TUString s1, TUString s2, TINT pos1,
	TINT pos2, TINT maxlen);
EXPORT TINT str_transform(TMOD_US *mod, TUString idx, TINT startpos,
	TINT len, TUINT mode);
EXPORT TINT str_insertdstr(TMOD_US *mod, TUString idx1, TINT pos1,
	TUString idx2, TINT pos2, TINT maxlen);
EXPORT TUString str_dup(TMOD_US *mod, TUString idx, TINT pos, TINT len);
EXPORT TINT str_parsepattern(TMOD_US *mod, TUString idx, TUINT mode);
EXPORT TINT str_matchpattern(TMOD_US *mod, TUString patidx, TUString stridx);
EXPORT TINT str_addpart(TMOD_US *mod, TUString pathidx, TUString partidx);

EXPORT TBOOL str_isalnum(TMOD_US *mod, TWCHAR c);
EXPORT TBOOL str_isalpha(TMOD_US *mod, TWCHAR c);
EXPORT TBOOL str_iscntrl(TMOD_US *mod, TWCHAR c);
EXPORT TBOOL str_isgraph(TMOD_US *mod, TWCHAR c);
EXPORT TBOOL str_islower(TMOD_US *mod, TWCHAR c);
EXPORT TBOOL str_isprint(TMOD_US *mod, TWCHAR c);
EXPORT TBOOL str_ispunct(TMOD_US *mod, TWCHAR c);
EXPORT TBOOL str_isspace(TMOD_US *mod, TWCHAR c);
EXPORT TBOOL str_isupper(TMOD_US *mod, TWCHAR c);
EXPORT TWCHAR str_tolower(TMOD_US *mod, TWCHAR c);
EXPORT TWCHAR str_toupper(TMOD_US *mod, TWCHAR c);

EXPORT TINT str_findpat(TMOD_US *mod, TUString sidx, TUString pidx, TINT spos,
	TINT len, TTAGITEM *tags);
EXPORT TINT str_find(TMOD_US *mod, TUString sidx, TUString pidx, TINT spos,
	TINT len);

/*****************************************************************************/
/*
**	Revision History
**	$Log: unistring_mod.h,v $
**	Revision 1.35  2005/09/13 02:42:42  tmueller
**	updated copyright reference
**	
**	Revision 1.34  2005/09/08 00:03:54  tmueller
**	API extended
**	
**	Revision 1.33  2004/08/01 10:23:33  tmueller
**	version number bumped
**	
**	Revision 1.32  2004/07/25 00:08:15  tmueller
**	added strfind with Lua-style pattern matching
**	
**	Revision 1.31  2004/07/20 06:55:51  tmueller
**	added private str_duplinear() function
**	
**	Revision 1.30  2004/07/18 20:50:24  tmueller
**	character info added
**	
**	Revision 1.29  2004/07/16 20:15:34  tmueller
**	Cleanup. Mapping limit to 65535 entries removed. Version number bumped to v1.0
*/

#endif
