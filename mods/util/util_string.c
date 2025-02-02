
/*
**	$Id: util_string.c,v 1.6 2005/09/13 02:42:48 tmueller Exp $
**	teklib/mods/util/util_string.c - Utility string functions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "util_mod.h"

/*****************************************************************************/
/*
**	len = util_strlen(utilbase, string)
**	Return length of a string
*/

EXPORT TINT
util_strlen(TMOD_UTIL *util, TSTRPTR s)
{
	if (s)
	{
		TSTRPTR p = s;
		while (*p) p++;
		return (TINT) (p - s);
	}
	return 0;
}

/*****************************************************************************/
/*
**	p = util_strcpy(util, dest, source)
**	Copy string
*/

EXPORT TSTRPTR
util_strcpy(TMOD_UTIL *util, TSTRPTR d, TSTRPTR s)
{
	if (s && d)
	{
		TSTRPTR p = d;
		while ((*d++ = *s++));
		return p;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	p = util_strncpy(util, dest, source, maxlen)
**	Copy string, length-limited
*/

EXPORT TSTRPTR
util_strncpy(TMOD_UTIL *util, TSTRPTR d, TSTRPTR s, TINT maxl)
{
	if (s && d)
	{
		TSTRPTR p = d;
		TINT i;
		TINT8 c = 1;
		for (i = 0; i < maxl; ++i)
		{
			if (c) c = *s++;
			*d++ = c;
		}
		return p;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	p = util_strcat(util, dest, add)
**	Concatenate strings
*/

EXPORT TSTRPTR 
util_strcat(TMOD_UTIL *util, TSTRPTR d, TSTRPTR s)
{
	if (s && d)
	{
		TSTRPTR p = d;
		while (*d) d++;
		while ((*d++ = *s++));
		return p;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	p = util_strncat(util, dest, add, maxl)
**	Concatenate strings, length-limited
*/

EXPORT TSTRPTR
util_strncat(TMOD_UTIL *util, TSTRPTR d, TSTRPTR s, TINT maxl)
{
	if (s && d)
	{
		TSTRPTR p = d;
		while (*d) d++;
		while (maxl-- && (*d++ = *s++));
		*d = 0;
		return p;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	result = util_strcmp(utilbase, string1, string2)
**	Compare strings, with slightly extended semantics: Either or both
**	strings may be TNULL pointers. a TNULL string is 'less than' a
**	non-TNULL string.
*/

EXPORT TINT
util_strcmp(TMOD_UTIL *util, TSTRPTR s1, TSTRPTR s2)
{
	if (s1)
	{
		if (s2)
		{
			TINT8 t1 = *s1, t2 = *s2;
			TINT8 c1, c2;

			do
			{
				if ((c1 = t1)) t1 = *s1++;
				if ((c2 = t2)) t2 = *s2++;

				if (!c1 || !c2) break;

			} while (c1 == c2);

			return ((TINT) c1 - (TINT) c2);
		}

		return 1;
	}

	if (s2) return -1;

	return 0;
}

/*****************************************************************************/
/*
**	result = util_strncmp(utilbase, string1, string2, count)
**	Compare strings, length-limited, with slightly extended semantics:
**	either or both strings may be TNULL pointers. a TNULL string is
**	'less than' a non-TNULL string.
*/

EXPORT TINT 
util_strncmp(TMOD_UTIL *util, TSTRPTR s1, TSTRPTR s2, TINT count)
{
	if (s1)
	{
		if (s2)
		{
			TINT8 t1 = *s1, t2 = *s2;
			TINT8 c1, c2;

			do
			{
				if ((c1 = t1)) t1 = *s1++;
				if ((c2 = t2)) t2 = *s2++;

				if (!c1 || !c2) break;

			} while (count-- && c1 == c2);

			return ((TINT) c1 - (TINT) c2);
		}

		return 1;
	}

	if (s2) return -1;

	return 0;
}

/*****************************************************************************/
/*
**	result = util_strcasecmp(utilbase, string1, string2)
**	Compare strings without regard to case, with slightly extended
**	semantics: Either or both strings may be TNULL pointers. a TNULL
**	string is 'less than' a non-TNULL string.
*/

EXPORT TINT 
util_strcasecmp(TMOD_UTIL *util, TSTRPTR s1, TSTRPTR s2)
{
	if (s1)
	{
		if (s2)
		{
			TINT8 t1 = *s1, t2 = *s2;
			TINT8 c1, c2;

			if (t1 >= 'a' && t1 <= 'z') t1 -= 'a' - 'A';
			if (t2 >= 'a' && t2 <= 'z') t2 -= 'a' - 'A';

			do
			{
				if ((c1 = t1))
				{
					t1 = *s1++;
					if (t1 >= 'a' && t1 <= 'z') t1 -= 'a' - 'A';
				}

				if ((c2 = t2))
				{
					t2 = *s2++;
					if (t2 >= 'a' && t2 <= 'z') t2 -= 'a' - 'A';
				}

				if (!c1 || !c2) break;

			} while (c1 == c2);

			return ((TINT) c1 - (TINT) c2);
		}

		return 1;
	}

	if (s2) return -1;

	return 0;
}

/*****************************************************************************/
/*
**	result = util_strncasecmp(utilbase, string1, string2, count)
**	Compare characters of strings without regard to case, length-limited,
**	with slightly extended semantics: Either or both strings may be TNULL
**	pointers. a TNULL string is 'less than' a non-TNULL string.
*/

EXPORT TINT 
util_strncasecmp(TMOD_UTIL *util, TSTRPTR s1, TSTRPTR s2, TINT count)
{
	if (s1)
	{
		if (s2)
		{
			TINT8 t1 = *s1, t2 = *s2;
			TINT8 c1, c2;

			if (t1 >= 'a' && t1 <= 'z') t1 -= 'a' - 'A';
			if (t2 >= 'a' && t2 <= 'z') t2 -= 'a' - 'A';

			do
			{
				if ((c1 = t1))
				{
					t1 = *s1++;
					if (t1 >= 'a' && t1 <= 'z') t1 -= 'a' - 'A';
				}

				if ((c2 = t2))
				{
					t2 = *s2++;
					if (t2 >= 'a' && t2 <= 'z') t2 -= 'a' - 'A';
				}

				if (!c1 || !c2) break;

			} while (count-- && c1 == c2);

			return ((TINT) c1 - (TINT) c2);
		}

		return 1;
	}

	if (s2) return -1;

	return 0;
}

/*****************************************************************************/
/*
**	strptr = util_strstr(util, s1, s2)
**	Find substring s2 in string s1
*/

EXPORT TSTRPTR
util_strstr(TMOD_UTIL *util, TSTRPTR s1, TSTRPTR s2)
{
	TINT c, d, x;

	if (s1 == TNULL) return TNULL;
	if (s2 == TNULL) return s1;
	
	x = 0;
	d = *s2;
	while ((c = *s1++))
	{
		if (c != d)
		{
			d = *s2;
			x = 0;
		}
	
		if (c == d)
		{
			d = s2[++x];
			if (d == 0)
			{
				return s1 - x;
			}
		}
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	strptr = util_strchr(utilbase, string, char)
**	Find a character in a string
*/

EXPORT TSTRPTR
util_strchr(TMOD_UTIL *util, TSTRPTR s, TINT c)
{
	TINT d;
	while ((d = *s))
	{
		if (d == c) return s;
		s++;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	strptr = util_strrchr(utilbase, string, char)
**	Find a character in a string, reverse
*/

EXPORT TSTRPTR
util_strrchr(TMOD_UTIL *util, TSTRPTR s, TINT c)
{
	TSTRPTR s2;
	TUINT l = util_strlen(util, s);
	TBOOL suc=TFALSE;

	s2=s+l-1;
	while(!suc && l)
	{
		if(*s2 == c)
			suc=TTRUE;
		else
		{
			s2--;
			l--;
		}
	}

	if(suc)
		return s2;
	else
		return TNULL;
}

/*****************************************************************************/
/*
**	s2 = util_strdup(utilbase, mmu, string)
**	Duplicate string. Must be freed using TFree()
*/

EXPORT TSTRPTR 
util_strdup(TMOD_UTIL *util, TAPTR mmu, TSTRPTR s)
{
	TUINT l = util_strlen(util, s);
	TSTRPTR s2 = TExecAlloc(TExecBase, mmu, l + 1);
	if (s2)
	{
		if (l)
		{
			TExecCopyMem(TExecBase, s, s2, l);
		}
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
util_strndup(TMOD_UTIL *util, TAPTR mmu, TSTRPTR s, TINT len)
{
	TSTRPTR s2;
	TINT l = util_strlen(util, s);
	l = TMIN(l, len);
	s2 = TExecAlloc(TExecBase, mmu, l + 1);
	if (s2)
	{
		if (l)
		{
			TExecCopyMem(TExecBase, s, s2, l);
		}
		s2[l] = 0;
	}
	return s2;
}

/*****************************************************************************/
/*
**	numchars = util_strtoi(utilbase, s, &valp)
**	Get integer from ASCII (signed)
*/

EXPORT TINT
util_strtoi(TAPTR util, TSTRPTR s, TINT *valp)
{
	TSTRPTR p = s;
	TINT n = 0;
	TINT c;
	TBOOL neg = 0;

	if (!valp) return -1;
	if (!s) goto error;

	for (;;)
	{
		c = *s++;
		switch (c)
		{
			case '-':
				neg ^= 1;
			case 9:
			case 10:
			case 13:
			case 32:
			case '+':
				continue;
		}
		
		c -= '0';
		if (c >= 0 && c <= 9) break;

error:	*valp = 0;
		return -1;
	}
	
	for (;;)
	{
		if (n < 214748364 || (n == 214748364 && c < (neg ? 9 : 8)))
		{
			n = n * 10 + c;
			c = *s++;
			c -= '0';
			if (c >= 0 && c <= 9) continue;
			break;		/* no more digits */
		}
		/* overflow */
		goto error;
	}

	if (neg) n = -n;
	*valp = (TINT) n;
	return s - p;
}

/*****************************************************************************/
/*
**	numchars = util_strtod(util, string, valp)
*/

static TDOUBLE 
power10(TINT e)
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
util_strtod(TMOD_UTIL *util, TSTRPTR nptr, TDOUBLE *valp)
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
						{
							e2++;
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

	result = a + b / power10(nd);
 	result *= power10(esig * e + e2);
	result *= sig;

	*valp = result;
	return nptr - sptr;

error:
	*valp = 0;
	return -1;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: util_string.c,v $
**	Revision 1.6  2005/09/13 02:42:48  tmueller
**	updated copyright reference
**	
**	Revision 1.5  2005/09/08 03:30:00  tmueller
**	strchr/strrchr char argument changed to TINT
**	
**	Revision 1.4  2005/09/08 00:04:27  tmueller
**	API extended; strchr and strcasecmp optimized
**	
**	Revision 1.3  2004/07/16 10:17:24  tmueller
**	Great, so TUtilStrStr() wasn't fully functional all the time. Fixed
**	
**	Revision 1.2  2004/07/05 21:51:54  tmueller
**	cosmetic
**	
**	Revision 1.1  2004/07/04 21:42:12  tmueller
**	The utility module grew too large -- now splitted over several files
**	
*/
