
/*
**	$Id: unistring_char.c,v 1.3 2005/09/13 02:42:42 tmueller Exp $
**	mods/unistring/unistring_char.c - Character utilities
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include "unistring_mod.h"

/*****************************************************************************/

EXPORT TBOOL
str_isalnum(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CHARINFORANGE)
	{
		if ((c >= '0' && c <= '9') ||
			(mod->us_CharInfo[c] & C_LATIN) == C_LATIN)
				return TTRUE;
	}
	return TFALSE;
}

EXPORT TBOOL
str_isalpha(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CHARINFORANGE)
	{
		if ((mod->us_CharInfo[c] & C_LATIN) == C_LATIN)
			return TTRUE;
	}
	return TFALSE;
}

EXPORT TBOOL
str_iscntrl(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CHARINFORANGE)
	{
		if (mod->us_CharInfo[c] == C_CONTROL)
			return TTRUE;
	}
	return TFALSE;
}

EXPORT TBOOL
str_isgraph(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CHARINFORANGE)
	{
		TINT i = mod->us_CharInfo[c];
		if (i != C_CONTROL && i != C_SPACE)
			return TTRUE;
	}
	return TFALSE;
}

EXPORT TBOOL
str_islower(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CHARINFORANGE)
	{
		TINT i = mod->us_CharInfo[c];
		if ((i & C_CAPITAL) == C_LATIN)
			return TTRUE;
	}
	return TFALSE;
}

EXPORT TBOOL
str_isprint(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CHARINFORANGE)
	{
		TINT i = mod->us_CharInfo[c];
		if (i != C_CONTROL)
			return TTRUE;
	}
	return TFALSE;
}

EXPORT TBOOL
str_ispunct(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CHARINFORANGE)
	{
		TINT i = mod->us_CharInfo[c];
		if (i != C_CONTROL && i != C_SPACE &&
			!(i & C_LATIN) && !(c >= '0' && c <= '9'))
				return TTRUE;
	}
	return TFALSE;
}

EXPORT TBOOL
str_isspace(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CHARINFORANGE)
	{
		TINT i = mod->us_CharInfo[c];
		if (i == C_SPACE)
			return TTRUE;
	}
	return TFALSE;
}

EXPORT TBOOL
str_isupper(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CHARINFORANGE)
	{
		TINT i = mod->us_CharInfo[c];
		if ((i & C_CAPITAL) == C_CAPITAL)
			return TTRUE;
	}
	return TFALSE;
}

EXPORT TWCHAR
str_tolower(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CASECONVRANGE)
	{
		TINT16 *conv = mod->us_CapsToSmall[c];
		if (conv && (conv[0] == 0 || conv[0] == 2))
		{
			c = conv[1];
		}
	}
	return c;
}

EXPORT TWCHAR
str_toupper(TMOD_US *mod, TWCHAR c)
{
	if (c >= 0 && c < CASECONVRANGE)
	{
		TINT16 *conv = mod->us_SmallToCaps[c];
		if (conv && (conv[0] == 0 || conv[0] == 1))
		{
			c = conv[2];
		}
	}
	return c;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: unistring_char.c,v $
**	Revision 1.3  2005/09/13 02:42:42  tmueller
**	updated copyright reference
**	
**	Revision 1.2  2004/08/01 11:31:52  tmueller
**	removed lots of history garbage
**	
**	Revision 1.1  2004/07/18 20:48:53  tmueller
**	character utils added
*/
