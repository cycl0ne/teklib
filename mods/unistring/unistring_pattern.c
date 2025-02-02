
/*
**	$Id: unistring_pattern.c,v 1.10 2005/09/13 02:42:42 tmueller Exp $
**	mods/unistring/unistring_pattern.c - Pattern matching
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include "unistring_mod.h"

/*****************************************************************************/
/* 
**	The range U+E000 to U+F8FF is reserved for private use.
**	In TEKlib, we use U+F8F0 - U+F8FF for pattern-parsed control characters
*/

#define P_ANY				0xF8F0	/* Matches everything ("#?" and "*") */
#define P_SINGLE			0xF8F1	/* Any character ("?") */
#define P_ORSTART			0xF8F2	/* Opening parenthesis for OR'ing ("(") */
#define P_ORNEXT			0xF8F3	/* Field delimiter for OR'ing ("|") */
#define P_OREND				0xF8F4	/* Closing parenthesis for OR'ing (")") */
#define P_NOT				0xF8F5	/* Inversion ("~") */
#define P_NOTEND			0xF8F6	/* Inversion end */
#define P_NOTCLASS			0xF8F7	/* Inversion class ("^") */
#define P_CLASS				0xF8F8	/* Class ("[" and "]") */
#define P_REPBEG			0xF8F9	/* Beginning of repetition ("[") */
#define P_REPEND			0xF8FA	/* End of repetition ("]") */
#define P_STOP				0xF8FB

#define RES_OKAY			0
#define RES_ISWILD			1
#define RES_ERROR			-1
#define RES_TEMPLATE_ERROR	-2

#define RES_MISMATCH		0		/* false */
#define RES_MATCH			1		/* true */

/*****************************************************************************/
/*
**	From the commentary in the AROS source:
**
**	A simple method for pattern matching with multiple wildcards:
**	I use markers that consist of both a pointer into the string
**	and one into the pattern. The marker simply follows the string
**	and everytime it hits a wildcard it's split into two new markers
**	(one to assume that the wildcard match has ended and one to assume
**	that it continues). If a marker doesn't fit any longer it's
**	removed and if all of them are gone the pattern mismatches.
**	OTOH if any of the markers reaches the end of both the string
**	and the pattern simultaneously the pattern matches the string.
*/

struct marker
{
	TINT ppos;					/* Offset into pattern */
	TINT spos;					/* Offset into string */
	TINT type;					/* 0: Split 1: MP_NOT */
};

#define NUMMARK		128

struct markerarray
{
	struct markerarray *next;
	struct markerarray *prev;
	struct marker marker[NUMMARK];
};

#define PUSH(t,p,s)									\
{													\
    if(macnt==NUMMARK)								\
    {												\
		if(macur->next==TNULL)						\
		{											\
			macur->next = TExecAlloc(TExecBase, mod->us_MMU, sizeof(struct markerarray));	\
			if(macur->next==TNULL) { result = RES_ERROR; goto end; }	\
			macur->next->prev=macur;				\
		}											\
		macur=macur->next;							\
		macnt=0;									\
    }												\
    macur->marker[macnt].type=(t);					\
    macur->marker[macnt].ppos=(p);					\
    macur->marker[macnt].spos=(s);					\
    macnt++;										\
}

#define POP(t,p,s)									\
{													\
	macnt--;										\
	if(macnt<0)										\
	{												\
		macnt=NUMMARK-1;							\
		macur=macur->prev;							\
		if(macur==TNULL) { result = RES_OKAY; goto end; }	\
	}												\
	(t)=macur->marker[macnt].type;					\
	(p)=macur->marker[macnt].ppos;					\
	(s)=macur->marker[macnt].spos;					\
}

EXPORT TINT
str_matchpattern(TMOD_US *mod, TUString patidx, TUString stridx)
{
	TINT result = RES_ERROR;
	TAHEAD *patarr = _str_valid(mod, patidx);
	TAHEAD *strarr = _str_valid(mod, stridx);
	if (patarr && strarr)
	{
		TINT slen = strarr->tah_Length;
		TINT ppos = 0, spos = 0;

		struct markerarray ma;
		struct markerarray *macur = &ma;

		TINT macnt = 0;
		TWCHAR c, t;

		ma.next = ma.prev = TNULL;
		result = RES_OKAY;

		for (;;)
		{
			switch (_str_get(patarr, ppos))
			{
				case P_REPBEG:		/* _#(_a), _#a_ or _#[_a] */
				{
					TINT level = 1;

					PUSH(0, ++ppos, spos);

					for (;;)
					{
						c = _str_get(patarr, ppos++);
						if (c == P_REPBEG)
						{
							level++;
						}
						else if (c == P_REPEND)
						{
							if (!--level) break;
						}
					}

					break;
				}
	
				case P_REPEND:		/* #(a_)_ */
				{
					TINT level = 1;

					for (;;)
					{
						c = _str_get(patarr, --ppos);
						if (c == P_REPEND)
						{
							level++;
						}
						else if (c == P_REPBEG)
						{
							if (!--level) break;
						}
					}

					break;
				}
	
				case P_NOT:		/* _~(_a) */
				{
					TINT tpos = ++ppos;
					TINT level = 1;

					for (;;)
					{
						c = _str_get(patarr, tpos++);
						if (c == P_NOT)
						{
							level++;
						}
						else if (c == P_NOTEND)
						{
							if (!--level) break;
						}
					}

					PUSH(1, tpos, spos);

					break;
				}
	
				case P_NOTEND:		/* ~(a_)_ */
				{
					struct markerarray *cur2 = macur;
					TINT cnt2 = macnt;

					do
					{
						cnt2--;
						if (cnt2 < 0)
						{
							cnt2 = NUMMARK - 1;
							cur2 = cur2->prev;
						}
					} while (!cur2->marker[cnt2].type);

					++spos;
					if (spos == slen + 1)
					{
						macnt = cnt2;
						macur = cur2;
					}
					else if (spos > cur2->marker[cnt2].spos)
					{
						cur2->marker[cnt2].spos = spos;
					}

					POP(t, ppos, spos);

					if (t && (spos != slen))
					{
						PUSH(1, ppos, spos + 1);
					}

					break;
				}
	
				case P_ORSTART:	/* ( */
				{
					TINT tpos = ++ppos;
					TINT level = 1;

					for (;;)
					{
						c = _str_get(patarr, tpos++);
						if (c == P_ORSTART)
						{
							level++;
						}
						else if (c == P_ORNEXT)
						{
							if (level == 1)
							{
								PUSH(0, tpos, spos);
							}
						}
						else if (c == P_OREND)
						{
							if (!--level)
							{
								break;
							}
						}
					}

					break;
				}
	
				case P_ORNEXT:		/* | */
				{
					TINT level = 1;
					ppos++;

					for (;;)
					{
						c = _str_get(patarr, ppos++);
						if (c == P_ORSTART)
						{
							level++;
						}
						else if (c == P_OREND)
						{
							if (!--level) break;
						}
					}

					break;
				}
	
				case P_OREND:		/* ) */
					ppos++;
					break;
	
				case P_SINGLE:		/* ? */
					ppos++;
					if (spos != slen)
					{
						spos++;
					}
					else
					{
						POP(t, ppos, spos);
						if (t && (spos != slen))
						{
							PUSH(1, ppos, spos + 1);
						}
					}
					break;
	
				case P_CLASS:		/* [ */
				{
					TWCHAR a, b;
					ppos++;

					for (;;)
					{
						a = b = _str_get(patarr, ppos++);
						if (a == P_CLASS)
						{
							POP(t, ppos, spos);
							if (t && (spos != slen))
							{
								PUSH(1, ppos, spos + 1);
							}
							break;
						}
	
						if (_str_get(patarr, ppos) == '-')
						{
							b = _str_get(patarr, ++ppos);
							if (b == P_CLASS) b = 255;
						}
	
						c = _str_get(strarr, spos);
						if (c >= a && c <= b)
						{
							spos++;
							while (_str_get(patarr, ppos++) != P_CLASS);
							break;
						}
					}

					break;
				}
	
				case P_NOTCLASS:	/* [~ */
				{
					TWCHAR a, b;

					if (spos == slen)
				    {
						POP(t, ppos, spos);
						if (t && (spos != slen))
						{
							PUSH(1, ppos, spos + 1);
						}
						break;
					}

					ppos++;
					for (;;)
					{
						a = b = _str_get(patarr, ppos++);
						if (a == P_CLASS)
						{
							spos++;
							break;
						}
	
						if (_str_get(patarr, ppos) == '-')
						{
							b = _str_get(patarr, ++ppos);
							if (b == P_CLASS) b = 255;
						}
						
						c = _str_get(strarr, spos);
						if (c >= a && c <= b)
						{
							POP(t, ppos, spos);
							if (t && (spos != slen))
							{
								PUSH(1, ppos, spos + 1);
							}
							break;
						}
					}

					break;
				}

				case P_ANY:		/* #? */
					/* This often used pattern has extra treatment */
					if (spos != slen)
					{
						PUSH(0, ppos, spos + 1);
					}
					ppos++;
					break;
	
				case -1:
					if (spos == slen)
					{
						result = RES_MATCH;
						goto end;
					}
					else
					{
						POP(t, ppos, spos);
						if (t && (spos != slen))
						{
							PUSH(1, ppos, spos + 1);
						}
					}
					break;
	
				default:
					c = _str_get(strarr, spos);

					if (_str_get(patarr, ppos++) == c)
					{
						spos++;
					}
					else
					{
						POP(t, ppos, spos);
						if (t && (spos != slen))
						{
							PUSH(1, ppos, spos + 1);
						}
					}
					break;
			}
		}

	end:

		macur = ma.next;
		while (macur != TNULL)
		{
			struct markerarray *next = macur->next;
			TExecFree(TExecBase, macur);
			macur = next;
		}
	}

	return result;
}

/*****************************************************************************/
/*
**	error = str_parsepattern(mod, idx, flags)
**	error:	 1	- ok, is wild
**			 0	- ok, not wild
**			-1  - illegal args, out of memory
**			-2	- corrupt pattern
*/

EXPORT TINT
str_parsepattern(TMOD_US *mod, TUString srcidx, TUINT mode)
{
	TINT result = RES_ERROR;
	TAHEAD *srcarr = _str_valid(mod, srcidx);
	TUString destidx = array_alloc(mod, TASIZE_32BIT);
	if (srcarr && destidx)
	{
		TAHEAD *destarr = mod->us_Array[destidx];
		TINT srclen = srcarr->tah_Length;
		TINT destlen = srclen * 2 + 1;
		TWCHAR *pat;
		TINT i;
		
		for (i = 0; i < destlen; ++i)
		{
			_array_ins(mod, destarr, TNULL);
		}

		pat = _array_maplinear(mod, destarr, 0, destlen);
		if (pat)
		{
			TWCHAR c, d, a;
			TWCHAR *stack, *end, *patstart;

			result = RES_OKAY;

			patstart = pat;
			stack = end = pat + destlen;

			i = 0;
			while (i < srclen)
			{
				c = _str_get(srcarr, i++);
				d = _str_get(srcarr, i);
				switch (c)
				{
					case '#':
						result = RES_ISWILD;
						switch (d)
						{
							case '?':
								i++;
								*pat++ = P_ANY;
								break;

							case ')':
							case -1:
								result = RES_TEMPLATE_ERROR;
								goto err;

							default:
								*pat++ = P_REPBEG;
								*--stack = P_REPEND;
								continue;
						}
						break;

					case '~':
						switch (d)
						{
							case -1:
								*pat++ = c;
								break;

							case ')':
								result = RES_TEMPLATE_ERROR;
								goto err;

							default:
								result = RES_ISWILD;
								*pat++ = P_NOT;
								*--stack = P_NOTEND;
								continue;
						}
						break;

					case '?':
						result = RES_ISWILD;
						*pat++ = P_SINGLE;
						continue;

					case '(':
						*pat++ = P_ORSTART;
						*--stack = P_OREND;
						continue;

					case '|':
						result = RES_ISWILD;

						if (stack == end)
						{
							result = RES_TEMPLATE_ERROR;
							goto err;
						}

						while (!(*stack == P_OREND || stack == end))
						{
							*pat++ = *stack++;
						}

						*pat++ = P_ORNEXT;
						continue;

					case ')':
						while (!(stack == end || *stack == P_OREND))
						{
							*pat++ = *stack++;
						}

						if (stack == end)
						{
							result = RES_TEMPLATE_ERROR;
							goto err;
						}
						else
						{
							*pat++ = *stack++;
						}

						break;

					case '[':
						result = RES_ISWILD;

						if (d == '~')
						{
							i++;
							*pat++ = P_NOTCLASS;
						}
						else
						{
							*pat++ = P_CLASS;
						}

						a = _str_get(srcarr, i++);
						if (a == -1)
						{
							result = RES_TEMPLATE_ERROR;
							goto err;
						}

						do
						{
							if (a == '\'')
							{
								a = _str_get(srcarr, i++);
							}

							*pat++ = a;
							a = _str_get(srcarr, i++);
							if (a == -1)
							{
								result = RES_TEMPLATE_ERROR;
								goto err;
							}
						}
						while (a != ']');

						*pat++ = P_CLASS;
						break;

					case '*':
						result = RES_ISWILD;
						*pat++ = P_ANY;
						break;

					case '%':
						continue;

					case '\'':
						switch (d)
						{
							case '#':
							case '*':
							case '?':
							case '(':
							case '|':
							case ')':
							case '~':
							case '[':
							case ']':
							case '%':
							case '\'':
								i++;
							default:
								break;
						}

						/* Fall through */
					default:
						*pat++ = c;
						break;
				}

				while (stack != end && *stack != P_OREND)
				{
					*pat++ = *stack++;
				}
			}

			if (stack != end)
			{
				result = RES_TEMPLATE_ERROR;
				goto err;
			}

			str_crop(mod, destidx, 0, pat - patstart);
			array_move(mod, destidx, srcidx);
			/* destidx is now relinquished */
		}
	}

err:
	if (result < 0) array_free(mod, destidx);
	return result;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: unistring_pattern.c,v $
**	Revision 1.10  2005/09/13 02:42:42  tmueller
**	updated copyright reference
**	
**	Revision 1.9  2004/08/01 11:31:52  tmueller
**	removed lots of history garbage
**	
**	Revision 1.8  2004/04/04 12:20:29  tmueller
**	Datatype TDIndex renamed to TUString. Docs and Prototypes adapted.
*/
