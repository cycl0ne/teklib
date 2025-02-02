
/*
**	$Id: unistring_path.c,v 1.4 2005/09/13 02:42:42 tmueller Exp $
**	mods/unistring/unistring_path.c - Path naming functions
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
**	result = str_validpath(mod, idx)
**		0  - valid path
**		1  - valid path with a colon
**  	-1 - illegal
*/

static TINT
_str_validpath(TMOD_US *mod, TAHEAD *arr)
{
	TINT slc = 0;
	TINT clc = 0;
	TWCHAR c;
	TINT i;
	
	for (i = 0; i < arr->tah_Length; ++i)
	{
		c = _str_get(arr, i);
		if (c == '/')
		{
			if (clc == 1 && i == 1) return -1;	/* :/ */
			slc++;
		} 
		else if (c == ':')
		{
			if (clc > 0 || slc > 0) return -1;	/* :...: or /: */
			clc++;
		}
	}

	return clc;
}

EXPORT TINT
str_validpath(TMOD_US *mod, TUString idx)
{
	TAHEAD *arr = _str_valid(mod, idx);
	if (arr) return _str_validpath(mod, arr);
	return -1;
}

/*****************************************************************************/
/* 
**	result = str_addpart(mod, path, part)
**	add part to path. result:
**		1	success
**		0	not valid path/part. path unmodified.
**		-1	out of memory, illegal arguments; path unmodified.
*/

EXPORT TINT
str_addpart(TMOD_US *mod, TUString pathidx, TUString partidx)
{
	TAHEAD *path = _str_valid(mod, pathidx);
	TAHEAD *part = _str_valid(mod, partidx);
	if (path && part)
	{
		TINT clc1 = _str_validpath(mod, path);
		TINT clc2 = _str_validpath(mod, part);
		TUString newidx = TINVALID_STRING;

		if (clc1 == 0 && clc2 == 1)
		{
			/* ":" in second part replaces path */
			newidx = _array_dup(mod, part);
		}
		else if (clc1 >= 0 && clc2 == 0)
		{
			newidx = array_alloc(mod, 0);
			if (newidx >= 0)
			{
				TAHEAD *newarr = mod->us_Array[newidx];
				TAHEAD *curarr = part;
				TINT curpos = part->tah_Length;
				TWCHAR c;
				TINT slc = 0, eatc = 0, len = 0;
				
				for (;;)
				{
					if (curpos == 0)
					{
						/* start of first part reached */
						if (curarr == path) break;

						/* continue at end of first part */
						curpos = path->tah_Length;
						if (curpos == 0) break;

						curarr = path;
						c = _str_get(curarr, curpos - 1);

						/* need a slash between */
						if (c != '/' && c != ':') slc++;
					}
					
					c = _str_get(curarr, --curpos);
		
					if (c == '/')
					{
						slc++;
					}
					else if (c == ':')
					{
						if (eatc > 1) slc += eatc - 1;
						
						while (slc)
						{
							len++;
							_str_insert(mod, newarr, 0, '/');
							slc--;
						}
				
						len++;
						_str_insert(mod, newarr, 0, c);
						eatc = 0; slc = 0;
					}
					else
					{
						if (slc > 0)
						{
							if (eatc > 0) eatc--;
							if (slc > 1) eatc += slc - 1;
							if (!eatc)
							{
								if (len)
								{
									len++;
									_str_insert(mod, newarr, 0, '/');
								}
							}
							slc = 0;
						}
						
						if (!eatc)
						{
							len++;
							_str_insert(mod, newarr, 0, c);
						}
					}
				}
	
				if (eatc > 1) slc += eatc - 1;
				while (slc)
				{
					len++;
					_str_insert(mod, newarr, 0, '/');
					slc--;
				}
			}
		}
		else
		{
			/* invalid path(s) */
			return 0;	
		}
		
		if (_str_valid(mod, newidx))
		{
			/* success */
			array_move(mod, newidx, pathidx);
			/* newidx is now relinquished */
			return 1;
		}

		/* out of memory */
		array_free(mod, newidx);
	}

	/* illegal arguments */
	return -1;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: unistring_path.c,v $
**	Revision 1.4  2005/09/13 02:42:42  tmueller
**	updated copyright reference
**	
**	Revision 1.3  2004/08/01 11:31:52  tmueller
**	removed lots of history garbage
**	
*/
