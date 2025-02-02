
#ifndef _TMKMF_GLOBAL_H
#define _TMKMF_GLOBAL_H

#include "parse_tmkmf.h"

#define ARG_TEMPLATE	\
"-f=FROM,-c=CONTEXT/K,-r=RECURSE/S,-b=BUILDDIR/K,-q=QUIET/S,-m=MAKEDIR/K,-h=HELP/S"
#define ARG_FROM		0
#define ARG_CONTEXT		1
#define ARG_RECURSE		2
#define ARG_BUILDDIR	3
#define ARG_QUIET		4
#define ARG_MAKEDIR		5
#define ARG_HELP		6
#define ARG_NUM			7

/*****************************************************************************/
/*
**	global
*/

struct dStrings
{
	TSTRPTR *dStrings;
	TINT *dStrLens;
	TAPTR mmu;
	TINT dNumStrings;
};

extern TAPTR TExecBase;
extern TAPTR TUtilBase;
extern TAPTR TIOBase;
extern TAPTR TDStrBase;
extern TAPTR MMU;
extern struct dStrings *DSTR;
extern TTAG args[ARG_NUM];

extern TBOOL docontext(TSTRPTR fname, TSTRPTR context);

extern TBOOL dInitStrings(struct dStrings *dstr);
extern void dExitStrings(struct dStrings *dstr);
extern TINT dAllocString(struct dStrings *dstr, TSTRPTR istr);
extern void dFreeString(struct dStrings *dstr, TINT s);
extern TINT dLengthString(struct dStrings *dstr, TINT s);
extern TINT dGetCharString(struct dStrings *dstr, TINT s, TINT pos);
extern TINT dSetCharString(struct dStrings *dstr, TINT s, TINT pos, TINT c);
extern TAPTR dMapString(struct dStrings *dstr, TINT s, TINT offs, TINT len);
extern TINT dCmpNString(struct dStrings *dstr, TINT s1, TINT s2, TINT p1, TINT p2, TINT len);
extern TINT dFindString(struct dStrings *dstr, TINT s, TINT p, TINT pos, TINT len);
extern TINT dDupString(struct dStrings *dstr, TINT s, TINT pos, TINT len);
extern TINT dInsertString(struct dStrings *dstr, TINT d, TINT dpos, TINT s, TINT spos, TINT len);
extern TINT dInsertStrNString(struct dStrings *dstr, TINT s, TINT pos, TSTRPTR ptr, TINT len);

#endif
