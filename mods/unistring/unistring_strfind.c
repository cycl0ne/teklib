
/*
**	$Id: unistring_strfind.c,v 1.3 2005/09/13 02:42:42 tmueller Exp $
**	mods/unistring/unistring_strfind.c - Find with pattern matching
**
**	Implementation derived from the Lua string library,
**	Copyright (C) 2003-2004 Tecgraf, PUC-Rio.
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include "unistring_mod.h"

/*****************************************************************************/

#define MAXCAPTURES			128

typedef struct MatchState
{
	TAHEAD *str;
	TAHEAD *pat;
	TINT spos;					/* startpos in string */
	TINT epos;					/* endpos in string */

	TUString sidx;

	TMOD_US *mod;

	struct
	{
		TINT pos, len;
	} capture[128];

	TINT level;					/* total number of captures (finished or unfinished) */

	TINT mpos;					/* start of match */
	TINT mlen;					/* len of match */

	TINT error;

	TUINT cstate;				/* state of last access to captures */
	TINT capcnt;				/* capture counter when iterating results */

} MatchState;

#define MERR_OKAY						0
#define MERR_NOT_FOUND					-1
#define MERR_MALFORMED_PATTERN			-2
#define MERR_INVALID_ARGUMENTS			-3
#define MERR_TOO_MANY_CAPTURES			-4
#define MERR_INVALID_PATTERN_CAPTURE	-5
#define MERR_OUT_OF_MEMORY				-6

/*****************************************************************************/

#define SPECIALS		"^$*+?.([%-"
#define ESC				'%'

#define CAP_UNFINISHED	(-1)
#define CAP_POSITION	(-2)

static TINT match(MatchState *ms, TINT spos, TINT ppos);

/*****************************************************************************/

static TBOOL
myisdigit(TWCHAR c)
{
	return (TBOOL) (c >= '0' && c <= '9');
}

static TBOOL
myisxdigit(TWCHAR c)
{
	return (TBOOL) ((c >= '0' && c <= '9') ||
		(c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

static TINT
mystrpbrk(TAHEAD *sarr, const char *accept)
{
	TINT i;
	TWCHAR c, d;

	for (i = 0; (c = _str_get(sarr, i)) != -1; ++i)
	{
		const char *p = accept;

		while ((d = *p++))
			if (d == c)
				return i;
	}
	return -1;
}

static TINT
mymemchr(TAHEAD *s, TINT pos, TINT len, TWCHAR c)
{
	while (len--)
	{
		if (_str_get(s, pos) == c)
			return pos;
		pos++;
	}
	return -1;
}

static TINT
mymemcmp(TAHEAD *s1, TINT pos1, TAHEAD *s2, TINT pos2, TINT len)
{
	TWCHAR c1, c2;
	do
	{
		c1 = _str_get(s1, pos1++);
		c2 = _str_get(s2, pos2++);
	}
	while (--len && c1 == c2);
	return c1 - c2;
}

/*****************************************************************************/

static TINT
lmemfind(TAHEAD *sarr, TINT spos, TINT slen, TAHEAD *parr, TINT ppos,
	TINT plen)
{
	if (plen == 0)
		return spos;			/* empty strings are everywhere */
	else if (plen > slen)
		return -1;
	else
	{
		TINT nupos;
		TWCHAR first = _str_get(parr, ppos);
		plen--;					/* 1st char will be checked by `memchr' */
		slen -= plen;			/* `p' cannot be found after that */
		while (slen > 0
			&& (nupos = mymemchr(sarr, spos, slen, first)) >= 0)
		{
			nupos++;			/* 1st char is already checked */
			if (mymemcmp(sarr, nupos, parr, ppos + 1, plen) == 0)
				return nupos - 1;
			slen -= nupos - spos;
			spos = nupos;
		}
		return -1;
	}
}

static TINT
match_class(TMOD_US *mod, TWCHAR c, TWCHAR cl)
{
	TINT res;
	switch (str_tolower(mod, cl))
	{
		case 'a':
			res = str_isalpha(mod, c);
			break;
		case 'c':
			res = str_iscntrl(mod, c);
			break;
		case 'd':
			res = myisdigit(c);
			break;
		case 'l':
			res = str_islower(mod, c);
			break;
		case 'p':
			res = str_ispunct(mod, c);
			break;
		case 's':
			res = str_isspace(mod, c);
			break;
		case 'u':
			res = str_isupper(mod, c);
			break;
		case 'w':
			res = str_isalnum(mod, c);
			break;
		case 'x':
			res = myisxdigit(c);
			break;
		case 'z':
			res = (c == 0);
			break;
		default:
			return (cl == c);
	}
	return (str_islower(mod, cl) ? res : !res);
}

static TINT
matchbracketclass(TMOD_US *mod, TWCHAR c, TAHEAD *p, TINT ppos, TINT epos)
{
	TINT sig = 1;

	if (_str_get(p, ppos + 1) == '^')
	{
		sig = 0;
		ppos++;
	}

	while (++ppos < epos)
	{
		if (_str_get(p, ppos) == ESC)
		{
			ppos++;
			if (match_class(mod, c, _str_get(p, ppos)))
				return sig;
		}
		else if ((_str_get(p, ppos + 1) == '-') && (ppos + 2 < epos))
		{
			ppos += 2;
			if ((_str_get(p, ppos - 2) <= c) && (c <= _str_get(p, ppos)))
				return sig;
		}
		else if (_str_get(p, ppos) == c)
			return sig;
	}

	return !sig;
}

static TINT
luaI_singlematch(TMOD_US *mod, TWCHAR c, TAHEAD *p, TINT ppos, TINT epos)
{
	TWCHAR d = _str_get(p, ppos);
	switch (d)
	{
		case '.':
			return 1;
		case ESC:
			return match_class(mod, c, _str_get(p, ppos + 1));
		case '[':
			return matchbracketclass(mod, c, p, ppos, epos - 1);
		default:
			return (d == c);
	}
}

static TINT
start_capture(MatchState *ms, TINT spos, TINT ppos, TINT what)
{
	TINT res, level = ms->level;
	if (level >= MAXCAPTURES)
	{
		ms->error = MERR_TOO_MANY_CAPTURES;
		return -1;
	}
	ms->capture[level].pos = spos;
	ms->capture[level].len = what;
	ms->level = level + 1;
	res = match(ms, spos, ppos);
	if (res == -1)
		ms->level--;			/* undo capture */
	return res;
}

static TINT
capture_to_close(MatchState *ms)
{
	TINT level = ms->level;
	for (level--; level >= 0; level--)
		if (ms->capture[level].len == CAP_UNFINISHED)
			return level;
	ms->error = MERR_INVALID_PATTERN_CAPTURE;
	return 0;
}

static TINT
end_capture(MatchState *ms, TINT spos, TINT ppos)
{
	TINT res, l = capture_to_close(ms);
	if (ms->error)
		return -1;
	ms->capture[l].len = spos - ms->capture[l].pos;	/* close capture */
	res = match(ms, spos, ppos);
	if (res == -1)
		ms->capture[l].len = CAP_UNFINISHED;	/* undo capture */
	return res;
}

static TINT
matchbalance(MatchState *ms, TINT spos, TINT ppos)
{
	TWCHAR b;

	if (_str_get(ms->pat, ppos) == -1 || _str_get(ms->pat, ppos + 1) == -1)
	{
		ms->error = MERR_MALFORMED_PATTERN;	/*MERR_UNBALANCED_PATTERN;*/
		return -1;
	}

	b = _str_get(ms->pat, ppos);
	if (_str_get(ms->str, spos) != b)
	{
		return -1;
	}
	else
	{
		TWCHAR e = _str_get(ms->pat, ppos + 1);
		TINT cont = 1;

		while (++spos < ms->epos)
		{
			if (_str_get(ms->str, spos) == e)
			{
				if (--cont == 0)
					return spos + 1;
			}
			else if (_str_get(ms->str, spos) == b)
				cont++;
		}
	}
	return -1;		/* string ends out of balance */
}

static TINT
luaI_classend(MatchState *ms, TINT ppos)
{
	switch (_str_get(ms->pat, ppos++))
	{
		case ESC:
		{
			if (_str_get(ms->pat, ppos) == -1)
			{
				ms->error = MERR_MALFORMED_PATTERN;
				return -1;
			}
			return ppos + 1;
		}
		case '[':
		{
			if (_str_get(ms->pat, ppos) == '^')
				ppos++;
			do
			{					/* look for a `]' */
				if (_str_get(ms->pat, ppos) == -1)
				{
					ms->error = MERR_MALFORMED_PATTERN;
					return -1;
				}
				if (_str_get(ms->pat, ppos++) == ESC
					&& _str_get(ms->pat, ppos) != -1)
					ppos++;		/* skip escapes (e.g. `%]') */
			}
			while (_str_get(ms->pat, ppos) != ']');
			return ppos + 1;
		}
		default:
			return ppos;
	}
}

static TINT
check_capture(MatchState *ms, TINT l)
{
	l -= '1';
	if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
	{
		ms->error = MERR_INVALID_PATTERN_CAPTURE;	/*MERR_INVALID_CAPTURE_INDEX;*/
		return 0;
	}
	return l;
}

static TINT
match_capture(MatchState *ms, TINT spos, TINT l)
{
	TINT len;
	l = check_capture(ms, l);
	if (ms->error)
		return -1;
	len = ms->capture[l].len;
	if (ms->epos - spos >= len &&
		mymemcmp(ms->str, ms->capture[l].pos, ms->str, spos, len) == 0)
		return spos + len;
	return -1;
}

static TINT 
max_expand(MatchState *ms, TINT spos, TINT ppos, TINT epos)
{
	TINT i = 0;					/* counts maximum expand for item */

	while (spos + i < ms->epos &&
		luaI_singlematch(ms->mod, _str_get(ms->str, spos + i), ms->pat, ppos, epos))
		i++;
	/* keeps trying to match with the maximum repetitions */
	while (i >= 0)
	{
		TINT res = match(ms, spos + i, epos + 1);

		if (ms->error)
			return -1;
		if (res != -1)
			return res;
		i--;					/* else didn't match; reduce 1 repetition to try again */
	}
	return -1;
}

static TINT
min_expand(MatchState *ms, TINT spos, TINT ppos, TINT epos)
{
	for (;;)
	{
		TINT res = match(ms, spos, epos + 1);

		if (res != -1 || ms->error)
			return res;
		else if (spos < ms->epos
			&& luaI_singlematch(ms->mod, _str_get(ms->str, spos), ms->pat, ppos, epos))
			spos++;				/* try with one more repetition */
		else
			return -1;
	}
}

static TINT
match(MatchState *ms, TINT spos, TINT ppos)
{
  init:						/* using goto's to optimize tail recursion */
	switch (_str_get(ms->pat, ppos))
	{
		case '(':
		{						/* start capture */
			if (_str_get(ms->pat, ppos + 1) == ')')	/* positional capture */
				return start_capture(ms, spos, ppos + 2, CAP_POSITION);
			else
				return start_capture(ms, spos, ppos + 1, CAP_UNFINISHED);
		}
		case ')':
		{						/* end capture */
			return end_capture(ms, spos, ppos + 1);
		}
		case ESC:
		{
			switch (_str_get(ms->pat, ppos + 1))
			{
				case 'b':
				{				/* balanced string? */
					spos = matchbalance(ms, spos, ppos + 2);
					if (spos == -1)
						return -1;
					ppos += 4;
					goto init;	/* else return match(ms, s, p+4); */
				}
				case 'f':
				{				/* frontier? */
					TINT epos;
					TWCHAR previous;

					ppos += 2;
					if (_str_get(ms->pat, ppos) != '[')
					{
						ms->error = MERR_MALFORMED_PATTERN;
						return -1;
					}
					epos = luaI_classend(ms, ppos);	/* points to what is next */
					if (ms->error)
						return -1;
					previous =
						(spos == ms->spos) ? -1 : _str_get(ms->str, spos - 1);
					if (matchbracketclass(ms->mod, previous, ms->pat, ppos, epos - 1)
						|| !matchbracketclass(ms->mod, _str_get(ms->str, spos), ms->pat,
							ppos, epos - 1))
						return -1;
					ppos = epos;
					goto init;	/* else return match(ms, s, ep); */
				}
				default:
				{
					if (myisdigit(_str_get(ms->pat, ppos + 1)))
					{			/* capture results (%0-%9)? */
						spos =
							match_capture(ms, spos, _str_get(ms->pat, ppos + 1));
						if (spos == -1)
							return -1;
						ppos += 2;
						goto init;	/* else return match(ms, s, p+2) */
					}
					goto dflt;	/* case default */
				}
			}
		}
		case -1:
		{						/* end of pattern */
			return spos;		/* match succeeded */
		}
		case '$':
		{
			if (_str_get(ms->pat, ppos + 1) == -1)	/* is the `$' the last char in pattern? */
				return (spos == ms->epos) ? spos : -1;	/* check end of string */
			else
				goto dflt;
		}
		default:
	  dflt:{					/* it is a pattern item */
			TINT m, epos = luaI_classend(ms, ppos);	/* points to what is next */

			if (ms->error)
				return -1;
			m = (spos < ms->epos)
				&& luaI_singlematch(ms->mod, _str_get(ms->str, spos), ms->pat, ppos,
				epos);
			switch (_str_get(ms->pat, epos))
			{
				case '?':
				{				/* optional */
					TINT res;

					if (m)
					{
						res = match(ms, spos + 1, epos + 1);
						if (res != -1 || ms->error)
							return res;
					}
					ppos = epos + 1;
					goto init;	/* else return match(ms, s, ep+1); */
				}
				case '*':
				{				/* 0 or more repetitions */
					return max_expand(ms, spos, ppos, epos);
				}
				case '+':
				{				/* 1 or more repetitions */
					return (m ? max_expand(ms, spos + 1, ppos, epos) : -1);
				}
				case '-':
				{				/* 0 or more repetitions (minimum) */
					return min_expand(ms, spos, ppos, epos);
				}
				default:
				{
					if (!m)
						return -1;
					spos++;
					ppos = epos;
					goto init;	/* else return match(ms, s+1, ep); */
				}
			}
		}
	}
}

/*****************************************************************************/
/* 
**	spos = _str_find(matchstate, str, pat, pos, len, plain?)
*/

static TINT
__str_find(MatchState *ms, TAHEAD *s, TAHEAD *p, TINT spos, TINT len,
	TBOOL plain)
{
	TINT slen = s->tah_Length;
	TINT plen = p->tah_Length;

	ms->error = 0;
	ms->level = 0;

	if (len == 0)				/* empty strings are everywhere */
	{
		ms->mpos = 0;
		ms->mlen = 0;
		return 0;
	}

	if (spos > slen - 1)
		return -1;
	if (len < 0)
		len += slen + 1;
	if (spos + len > slen)
		len -= spos;
	slen = len;

	if (plain || mystrpbrk(p, SPECIALS) == -1)
	{
		/* plain search */
		TINT fpos = lmemfind(s, spos, slen, p, 0, plen);
		if (fpos >= 0)
		{
			ms->mpos = fpos;
			ms->mlen = plen;
			return ms->mpos;
		}
	}
	else
	{
		TINT ppos = 0;
		TINT anchor = 0;

		ms->pat = p;
		ms->str = s;
		ms->spos = spos;
		ms->epos = spos + slen;

		if (_str_get(ms->pat, ppos) == '^')
		{
			ppos++;
			anchor = 1;
		}

		do
		{
			TINT res;

			res = match(ms, spos, ppos);
			if (ms->error)
				return -1;
			if (res != -1)
			{
				TINT i;

				for (i = 0; i < ms->level; ++i)
				{
					if (ms->capture[i].len == CAP_UNFINISHED)
					{
						ms->error = MERR_INVALID_PATTERN_CAPTURE;
						return -1;
					}
				}
				ms->mpos = spos;
				ms->mlen = res - spos;
				return ms->mpos;
			}

		}
		while (spos++ < ms->epos && !anchor);
	}

	return -1;	/* not found */
}

/*****************************************************************************/

static TCALLBACK TBOOL
foreachtag(MatchState *ms, TTAGITEM *item)
{
	TINT *valp = (TINT *) item->tti_Value;
	if (valp)
	{
		switch (item->tti_Tag)
		{
			case TUStrFind_MatchBegin:
				*valp = ms->mpos;
				break;

			case TUStrFind_MatchEnd:
				*valp = ms->mpos + ms->mlen - 1;
				break;

			case TUStrFind_MatchString:
				*valp = str_dup(ms->mod, ms->sidx, ms->mpos, ms->mlen);
				if (*valp == TINVALID_STRING)
				{
					ms->error = MERR_OUT_OF_MEMORY;
					return TFALSE;
				}
				break;

			case TUStrFind_NumCaptures:
				*valp = ms->level;
				break;

			case TUStrFind_CaptureBegin:
				if (ms->cstate & 1)
				{
					ms->cstate = 0;
					if (++ms->capcnt == ms->level)
					{
						ms->error = MERR_INVALID_PATTERN_CAPTURE;
						return TFALSE;
					}
				}
				*valp = ms->capture[ms->capcnt].pos;
				ms->cstate |= 1;
				break;

			case TUStrFind_CaptureEnd:
				if (ms->cstate & 2)
				{
					ms->cstate = 0;
					if (++ms->capcnt == ms->level)
					{
						ms->error = MERR_INVALID_PATTERN_CAPTURE;
						return TFALSE;
					}
				}
				*valp = ms->capture[ms->capcnt].pos + ms->capture[ms->capcnt].len - 1;
				ms->cstate |= 2;
				break;

			case TUStrFind_CaptureString:
				if (ms->cstate & 4)
				{
					ms->cstate = 0;
					if (++ms->capcnt == ms->level)
					{
						ms->error = MERR_INVALID_PATTERN_CAPTURE;
						return TFALSE;
					}
				}

				*valp = str_dup(ms->mod, ms->sidx,
					ms->capture[ms->capcnt].pos, ms->capture[ms->capcnt].len);

				if (*valp == TINVALID_STRING)
				{
					ms->error = MERR_OUT_OF_MEMORY;
					return TFALSE;
				}
				
				ms->cstate |= 4;
				break;
		}
	}
	return TTRUE;
}

static TINT
_str_find(TMOD_US *mod, TUString sidx, TUString pidx, TINT spos, TINT len, TBOOL plain,
	TTAGITEM *tags)
{
	TAHEAD *sarr = _str_valid(mod, sidx);
	TAHEAD *parr = _str_valid(mod, pidx);
	TINT res = MERR_INVALID_ARGUMENTS;
	if (sarr && parr)
	{
		MatchState ms;
		ms.mod = mod;
		res = __str_find(&ms, sarr, parr, spos, len, plain);
		if (res >= 0)
		{
			ms.capcnt = 0;
			ms.cstate = 0;
			ms.sidx = sidx;
			if (TForEachTag(tags, (TTAGFOREACHFUNC) foreachtag, &ms) == TFALSE)
			{
				res = ms.error;
			}
		}
		else
		{
			if (ms.error) res = ms.error;
		}
	}
	return res;
}

/*****************************************************************************/
/* 
**	spos = str_findpat(mod, str, pat, pos, len, tags)
**	spos = str_find(mod, str, pat, pos, len, tags)
**	-1	not found
**	-2	malformed pattern
**	-3	invalid arguments
**	-4	too many captures
**	-5	tried to access invalid capture
**	-6	out of memory (that is, allocating capture string)
*/

EXPORT TINT
str_findpat(TMOD_US *mod, TUString sidx, TUString pidx, TINT spos, TINT len,
	TTAGITEM *tags)
{
	TBOOL plain = TGetTag(tags, TUStrFind_PlainSearch, TFALSE);
	return _str_find(mod, sidx, pidx, spos, len, plain, tags);
}

EXPORT TINT
str_find(TMOD_US *mod, TUString sidx, TUString pidx, TINT spos, TINT len)
{
	return _str_find(mod, sidx, pidx, spos, len, TTRUE, TNULL);
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: unistring_strfind.c,v $
**	Revision 1.3  2005/09/13 02:42:42  tmueller
**	updated copyright reference
**	
**	Revision 1.2  2004/08/01 11:29:19  tmueller
**	compiler glitches fixed
**	
*/
