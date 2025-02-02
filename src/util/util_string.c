
/*
**	$Id: util_string.c,v 1.1.1.1 2006/08/20 22:15:06 tmueller Exp $
**	teklib/src/util/util_string.c - Utility string functions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "util_mod.h"

/*****************************************************************************/
/*
**	s2 = util_strdup(utilbase, mmu, string)
**	Duplicate string. Must be freed using TFree()
*/

EXPORT TSTRPTR
util_strdup(struct TUtilBase *util, TAPTR mmu, TSTRPTR s)
{
	TUINT l = TStrLen(s);
	TSTRPTR s2 = TExecAlloc(util->tmu_ExecBase, mmu, l + 1);
	if (s2)
	{
		if (l)
			TExecCopyMem(util->tmu_ExecBase, (TAPTR) s, s2, l);
		s2[l] = 0;
	}
	return s2;
}

/*****************************************************************************/
/*
**	s2 = util_strndup(utilbase, mmu, string, maxlen)
**	Duplicate string, length-limited. Must be freed using TFree().
*/

EXPORT TSTRPTR
util_strndup(struct TUtilBase *util, TAPTR mmu, TSTRPTR s, TSIZE len)
{
	TSTRPTR s2;
	TSIZE l = TStrLen(s);
	l = TMIN(l, len);
	s2 = TExecAlloc(util->tmu_ExecBase, mmu, l + 1);
	if (s2)
	{
		if (l)
			TExecCopyMem(util->tmu_ExecBase, (TAPTR) s, s2, l);
		s2[l] = 0;
	}
	return s2;
}

#if 0
/*****************************************************************************/
/*
**	numchars = util_strtod(util, string, valp)
*/

static TDOUBLE
util_power10(TINT e)
{
	TDOUBLE res = 1;
	if (e > 0)
	{
		while (e--) res *= 10;
	}
	else if (e < 0)
	{
		while (e++) res /= 10;
	}
	return res;
}

EXPORT TINT
util_strtod(struct TUtilBase *util, TSTRPTR nptr, TDOUBLE *valp)
{
	TINT state = 0;
	TUINT a = 0, b = 0;
	TINT nd = 0, e = 0, e2 = 0, sig = 1, esig = 1;
	TINT c;
	TSTRPTR sptr = nptr;
	TDOUBLE result;

	if (!valp) return -1;
	if (!nptr) goto error;

	while (state >= 0 && (c = *nptr))
	{
		switch (state)
		{
			case 0:		/* waiting for start; reading +, -, . or 0-9 */
				switch (c)
				{
					case '+':
					case 32: case 10: case 13: case 9:
						break;
					case '-':
						sig = -sig;
						break;
					case '.':
						state = 2;
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						a = c - '0';
						state = 1;
						break;
					default:
						goto error;
				}
				break;

			case 1:		/* expecting 0-9 or . or e or end */
				switch (c)
				{
					case '.':
						state = 2;
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						if (a < 429400000)
						{
							a *= 10;
							a += c - '0';
						}
						else
							e2++;
						break;
					case 'E':
					case 'e':
						state = 3;
						break;
					case 32: case 10: case 13: case 9:
						state = -1;
						continue;
					default:
						goto error;
				}
				break;

			case 2:		/* waiting for 0-9 or e or end */
				switch (c)
				{
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						if (b < 429400000)
						{
							b *= 10;
							b += c - '0';
							nd++;
						}
						break;
					case 'E':
					case 'e':
						state = 3;
						break;
					case 32: case 10: case 13: case 9:
						state = -1;
						continue;
					default:
						goto error;
				}
				break;

			case 3:		/* reading exponent; +, -, or number */
				switch (c)
				{
					default:
						goto error;
					case '+':
						break;
					case '-':
						esig = -1;
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						e = c - '0';
						break;
				}
				state = 4;
				break;

			case 4:		/* more digits of the exponent, or end */
				switch (c)
				{
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						e *= 10;
						e += c - '0';
						break;
					case 32: case 10: case 13: case 9:
						state = -1;
						continue;
					default:
						goto error;
				}
				break;
		}

		nptr++;
	}

	result = a + b / util_power10(nd);
 	result *= util_power10(esig * e + e2);
	result *= sig;

	*valp = result;
	return nptr - sptr;

error:
	*valp = 0;
	return -1;
}
#endif
