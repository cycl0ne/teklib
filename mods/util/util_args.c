
/*
**	$Id: util_args.c,v 1.3 2005/09/13 02:42:48 tmueller Exp $
**	teklib/mods/util/util_args.c - Argument parsing
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "util_mod.h"

/*****************************************************************************/
/* 
**	convert argv[]-style string vector to an argument string
*/

static TSTRPTR
util_argv2args(TMOD_UTIL *util, TSTRPTR *argv)
{
	TSTRPTR argstr = TNULL;
	if (argv != TNULL)
	{
		TSTRPTR *arg;
		TSTRPTR d, p;
		TINT len = 1;
		TINT narg = 0;
			
		arg = argv;
		while (*arg)
		{
			if (util_strchr(util, *arg, ' ') || util_strchr(util, *arg, '='))
			{
				len += 2;
			}
			len += util_strlen(util, *arg) + 1;
			arg++;
			narg++;
		}
		
		if (narg == 0) return TNULL;

		d = argstr = TExecAlloc(TExecBase, TNULL, len);
		if (d)
		{
			arg = argv;
			while (*arg)
			{
				if (arg != argv) *d++ = ' ';
				len = util_strlen(util, *arg);
				if ((p = util_strchr(util, *arg, '=')))
				{
					util_strncpy(util, d, *arg, p - *arg);
					d +=  p - *arg;
					*d++ = '=';
					*d++ = '"';
					len = len - (p - *arg) - 1;
					util_strncpy(util, d, p + 1, len);
					d += len;
					*d++ = '"';
				}
				else if ((p = util_strchr(util, *arg, ' ')))
				{
					*d++ = '"';
					util_strncpy(util, d, *arg, len);
					d += len;
					*d++ = '"';
				}
				else
				{
					util_strncpy(util, d, *arg, len);
					d += len;
				}
				arg++;
			}
			*d = 0;
		}
	}
	return argstr;
}

/*****************************************************************************/
/* 
**	convert argument string to an argv[]-style string vector
**	handles foo=bar style arguments and quoted arguments
*/

static TSTRPTR *
util_args2argv(TAPTR util, TAPTR mmu, TSTRPTR progname, TSTRPTR args)
{
	TSTRPTR *argv;
	TSTRPTR s;
	TINT c;
	TINT numarg = 0, len = 2;
	TINT quote = 0, white = 0;
	
	if (util_strlen(util, args)) numarg++;
	s = args;
	while ((c = *s++))
	{
		switch (c)
		{
			case '"':
				quote ^= 1;
				white = 0;
				break;
				
			case '=': case ' ': case 9:
				if (quote == 0)
				{
					if (white) break;
					white = 1;
					
					len++;		/* for \0 termination */
					numarg++;
					break;
				}
	
			default:
				len++;	/* count chars */
				white = 0;
				break;
			
			case 10: case 13:
				break;
		}
	}
	
	if (progname)
	{
		len += util_strlen(util, progname) + 1;
		numarg++;
	}
	
	argv = TExecAlloc(TExecBase, mmu, (numarg + 1) * sizeof(TSTRPTR) + len);
	if (argv)
	{
		TSTRPTR d = (TSTRPTR) (argv + numarg + 1);
		TINT i = 0;
		
		if (progname)
		{
			argv[0] = d;
			util_strcpy(util, d, progname);
			d += util_strlen(util, progname) + 1;
			i++;
		}
				
		argv[i] = TNULL;
		if (util_strlen(util, args))
		{
			quote = 0;
			white = 0;
			argv[i++] = d;
			s = args;
			do
			{
				c = *s++;
				switch (c)
				{
					case '"':
						quote ^= 1;
						white = 0;
						break;
						
					case '=': case ' ': case 9:
						if (quote == 0)
						{
							if (white) break;
							white = 1;
						
							*d++ = 0;
							argv[i++] = d;
							break;
						}

					default:
						*d++ = c;
						white = 0;
						break;
						
					case 0:
						*d++ = 0;
						argv[i] = TNULL;
						white = 0;
						break;

					case 10: case 13:
						break;
				}

			} while (c);
		}
	}	
	return argv;
}

/*****************************************************************************/
/* 
**	Init system-wide arguments from named atoms
*/

LOCAL TBOOL
util_initargs(TMOD_UTIL *util)
{
	TAPTR atom_args = TExecLockAtom(TExecBase, "sys.arguments",
		TATOMF_NAME | TATOMF_SHARED);
	TAPTR atom_argv = TExecLockAtom(TExecBase, "sys.argv",
		TATOMF_NAME | TATOMF_SHARED);
	TAPTR atom_prog = TExecLockAtom(TExecBase, "sys.progname",
		TATOMF_NAME | TATOMF_SHARED);

	TSTRPTR args = atom_args ?
		(TSTRPTR) TExecGetAtomData(TExecBase, atom_args) : TNULL;
	TSTRPTR *argv = atom_argv ?
		(TSTRPTR *) TExecGetAtomData(TExecBase, atom_argv) : TNULL;
	TSTRPTR progname = atom_prog ?
		(TSTRPTR) TExecGetAtomData(TExecBase, atom_prog) : TNULL;
	
	TSTRPTR *newargv = TNULL;
	
	if (argv && progname == TNULL)
	{
		progname = argv[0];
	}
	
	if (args)
	{
		TINT len = util_strlen(util, args);
		while (len)
		{
			TINT c = args[len - 1];
			if (c != 10 && c != 32 && c != 13 && c != 9) break;
			len--;
		}
		args = util_strndup(util, TNULL, args, len);
	}
	else if (argv)
	{
		args = util_argv2args(util, argv + 1);
		if (args == TNULL) args = util_strdup(util, TNULL, "");
	}

	if (args && progname)
	{
		newargv = util_args2argv(util, TNULL, progname, args);	
	}
	
	TExecUnlockAtom(TExecBase, atom_prog, TATOMF_KEEP);
	TExecUnlockAtom(TExecBase, atom_argv, TATOMF_KEEP);
	TExecUnlockAtom(TExecBase, atom_args, TATOMF_KEEP);

	if (args && newargv)
	{
		TINT n = 0;
		util->tmu_Arguments = args;
		util->tmu_ArgV = newargv;
		while ((*newargv))
		{
			tdbprintf1(5,"argv: >%s<\n", *newargv);
			n++;
			newargv++;
		}
		util->tmu_ArgC = n;
		return TTRUE;
	}

	TExecFree(TExecBase, args);
	TExecFree(TExecBase, newargv);
	return TFALSE;
}

LOCAL TVOID
util_freeargs(TMOD_UTIL *util)
{
	TExecFree(TExecBase, util->tmu_Arguments);
	TExecFree(TExecBase, util->tmu_ArgV);
}

/*****************************************************************************/
/*
**	success = util_setreturn(util, retvalue)
**	Set system-wide return value
*/

EXPORT TBOOL
util_setreturn(TMOD_UTIL *util, TINT retval)
{
	TAPTR atom;
	TINT *p;

	atom = TExecLockAtom(TExecBase, "sys.returnvalue", TATOMF_NAME);
	if (atom)
	{
		p = (TINT *) TExecGetAtomData(TExecBase, atom);
		if (p)
		{
			*p = retval;
		}
		TExecUnlockAtom(TExecBase, atom, TATOMF_KEEP);
		return TTRUE;
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	argument parser
*/

#define OFL_		0x0000		/* string */
#define OFL_N		0x0001		/* number */
#define OFL_M		0x0002		/* multiple strings */
#define OFL_S		0x0003		/* switch */
#define OFL_F		0x0004		/* rest of line (not implemented) */
#define OFL_MASK	0x000f
#define OFL_K		0x0010		/* keyword */
#define OFL_A		0x0020		/* required */

typedef struct
{	TAPTR arg;
	TUINT flags;
} tentry;

/*
**	find key in a template, return template index, or -1 if not found
*/

static TINT 
findarg(TSTRPTR template, TSTRPTR keyword)
{
	TINT n = 0;
	TINT c, d;
	TSTRPTR p;

loop1:	p = keyword;

loop2:	c = *p++;
		d = *template++;

		if (c == 0)
		{
			if (d == 0 || d == '=' || d == '/' || d == ',') return n;
			d = *template++;
			goto loop3;
		}

		if (c >= 97 && c <= 122) c -= 32;
		if (d >= 97 && d <= 122) d -= 32;

		if (c == d) goto loop2;

loop3:	switch (d)
		{
			case 0:		return -1;
			case ',':	n++;
			case '=':	goto loop1;
		}
		d = *template++;
		goto loop3;
}		

/*
**	parse template string
*/

static TAPTR myrealloc(TAPTR util, TAPTR mmu, TAPTR mem, TINT size)
{
	if (mem == TNULL) return TExecAlloc(TExecBase, mmu, size);
	return TExecRealloc(TExecBase, mem, size);
}

static TINT 
parsetemplate(TAPTR util, TSTRPTR template, TAPTR mmu, tentry **pt)
{
	TINT c, n, size;
	tentry *tt;

		*pt = TNULL;
		size = 0;
		n = 0;

main:	*pt = tt = myrealloc(util, mmu, *pt, size + sizeof(tentry));
		if (tt == TNULL) goto error;

		size += sizeof(tentry);
		tt += n++;

		c = *template;

		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			c == '-' || c == '?')
		{
			tt->flags = OFL_;			/* no flags yet */
			tt->arg = TNULL;			/* no arg yet */
			goto name;
		}
		goto error;

name:	c = *(++template);
		switch (c)
		{
			case ',':
				template++;
				goto main;
			case '=':
				c = *(++template);
				goto alias;
			case '/':
				goto flags;
			case 0:
				goto done;
		}
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			c == '-' || c == '?')
		{
			goto name;
		}						
		goto error;

alias:	switch (c)
		{
			case ',':
				template++;
				goto main;
			case '/':
				goto flags;
			case 0:
				goto done;
		}
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			c == '-' || c == '?')
		{
			c = *(++template);
			goto alias;
		}
		goto error;

flags:	c = *(++template);
		if (c >= 'a' && c <= 'z') c -= 32;
		switch (c)
		{
			case 'A':
				tt->flags |= OFL_A;
				goto morefl;
			case 'S':
				tt->flags |= OFL_S|OFL_K;
				goto morefl;
			case 'K':
				tt->flags |= OFL_K;
				goto morefl;
			case 'N':
				tt->flags |= OFL_N;
				goto morefl;
			case 'M':
				tt->flags |= OFL_M;
				goto morefl;
		}
		goto error;

morefl:	c = *(++template);
		switch (c)
		{
			case '/':
				goto flags;
			case ',':
				template++;
				goto main;
			case 0:
				goto done;
		}

error:	return -1;

done:	/* add dummy item at end for collecting multiple arguments */
		tt = *pt = TExecRealloc(TExecBase, *pt, size + sizeof(tentry));
		if (!tt) goto error;
		tt[n].flags = OFL_M;
		tt[n].arg = TNULL;
		return n;
}

/*****************************************************************************/
/*
**	parse argument vector - this implementation is "inspired" by
**	the AROS project -- see AROS/rom/dos/readargs.c
*/

static TBOOL
util_parsearray(TAPTR util, TAPTR mmu, TSTRPTR template, TTAG *array,
	TCALLBACK TINT (*readfunc)(TAPTR data, TSTRPTR *buf), TAPTR data)
{
	TINT numt;
	tentry *tt;

	TINT arg, nextarg, idx;
	TSTRPTR item;
	TSTRPTR *multi = TNULL;
	TINT nummult = 0;

	numt = parsetemplate(util, template, mmu, &tt);
	if (numt == 0) return TFALSE;

	for (arg = 0; arg <= numt; arg = nextarg)
	{		
		nextarg = arg + 1;

		if (tt[arg].arg) continue;
		if (tt[arg].flags & OFL_K) continue;
		
		if (!(*readfunc)(data, &item)) break;

		idx = findarg(template, item);
		if (idx >= 0 && !tt[idx].arg)
		{
			/* keyword option */
	
			nextarg = arg;		/* retry current option in next turn */
			arg = idx;

			if ((tt[arg].flags & OFL_MASK) != OFL_S)
			{
				if (!(*readfunc)(data, &item))
				{
					tdbprintf(3,"*** required arg missing\n");
					return TFALSE;
				}
			}
		}
		
		switch (tt[arg].flags & OFL_MASK)
		{
			case OFL_N:
			case OFL_:
				tt[arg].arg = item;
				break;
			case OFL_M:
				multi = myrealloc(util, mmu, multi,
					(nummult + 1) * sizeof(TSTRPTR));
				if (!multi) return TFALSE;
				multi[nummult++] = item;
				nextarg = arg;		/* retry in next turn */
				break;
			case OFL_S:
				tt[arg].arg = (TAPTR) 1;
				break;
		}
	}

	/* unfilled /A options steal from /M */

	{
		TBOOL needextra = TTRUE;		

		for (arg = numt; arg--;)
		{
			/* argument: unfilled, required, not multi? */

			if (!tt[arg].arg && (tt[arg].flags & OFL_A) &&
				(tt[arg].flags & OFL_MASK) != OFL_M)
			{
				if (!nummult) return TFALSE;	/* no arguments left */
				tt[arg].arg = multi[--nummult];	/* steal */
				needextra = TFALSE;				/* no resize needed */
			}
		}
		
		if (needextra)
		{
			/* add extra entry to multi array, for NULL-termination */
			multi = myrealloc(util, mmu, multi,
				(nummult + 1) * sizeof(TSTRPTR));
			if (!multi) return TFALSE;
		}
	}
	
	/* put remaining multi into /M argument */
	
	for (arg = 0; arg < numt; arg++)
	{
		if ((tt[arg].flags & OFL_MASK) == OFL_M)
		{
			if ((tt[arg].flags & OFL_A) && !nummult)
			{
				tdbprintf(3,"*** required argument missing\n");
				return TFALSE;
			}
			multi[nummult] = TNULL;
			tt[arg].arg = multi;
			break;
		}
	}
	
	/* arguments left? */

	if (arg == numt && nummult)
	{
		tdbprintf(3,"*** too many arguments\n");
		return TFALSE;
	}

	/* put args to destination array */

	for (arg = 0; arg < numt; ++arg)
	{
		switch (tt[arg].flags & OFL_MASK)
		{
			case OFL_N:
				if (tt[arg].arg)
				{
					if (util_strtoi(util, tt[arg].arg, 
						(TINT *) &tt[arg].arg) > 0)
					{
						array[arg] = (TTAG) &tt[arg].arg;
						break;
					}
					else
					{
						tdbprintf(3,"*** bad number\n");
						return TFALSE;
					}
					array[arg] = (TTAG) tt[arg].arg;
				}
				break;
				
			case OFL_M:
				if (tt[arg].arg)
				{
					array[arg] = (TTAG) tt[arg].arg;
				}
				break;

			case OFL_:
				if (tt[arg].arg)
				{
					array[arg] = (TTAG) tt[arg].arg;
				}
				break;

			case OFL_S:
				if (tt[arg].arg)
				{
					array[arg] = (TTAG) tt[arg].arg;
				}
				break;
		}
	}
	
	return TTRUE;
}

/*****************************************************************************/
/*
**	util_parseargv(util, template, argv, array)
**		parse an array of strings, aka an argv[] vector
*/

static TCALLBACK TINT 
rdargvfunc(TSTRPTR **argvp, TSTRPTR *buf)
{
	*buf = *(*argvp)++;
	if (*buf) return 1;
	return 0;
}

EXPORT TAPTR
util_parseargv(TAPTR util, TSTRPTR template, TSTRPTR *argv, TTAG *array)
{
	if (template && argv && array)
	{
		TAPTR mmu = TExecCreateMMU(TExecBase, TNULL, TMMUT_Pooled, TNULL);
		if (mmu)
		{
			if (util_parsearray(util, mmu, template, array, 
				(TCALLBACK TINT (*)(TAPTR, TSTRPTR *)) rdargvfunc, &argv))
			{
				return mmu;
			}
			TDestroy(mmu);
		}
	}
	return TNULL;
}

EXPORT TAPTR
util_parseargs(TAPTR util, TSTRPTR template, TSTRPTR args, TTAG *array)
{
	if (template && args && array)
	{
		TAPTR mmu = TExecCreateMMU(TExecBase, TNULL, TMMUT_Pooled, TNULL);
		if (mmu)
		{
			TSTRPTR *argv = util_args2argv(util, mmu, TNULL, args);
			if (argv)
			{
				if (util_parsearray(util, mmu, template, array, 
					(TCALLBACK TINT (*)(TAPTR, TSTRPTR *)) rdargvfunc, &argv))
				{
					return mmu;
				}
			}
			TDestroy(mmu);
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	argc = util_getargc(utilbase)
**	Get system-wide argument count
*/

EXPORT TINT
util_getargc(TMOD_UTIL *util)
{
	return util->tmu_ArgC;
}

/*****************************************************************************/
/*
**	argv = util_getargv(utilbase)
**	Get system-wide argument vector
*/

EXPORT TSTRPTR *
util_getargv(TMOD_UTIL *util)
{
	return util->tmu_ArgV;
}

/*****************************************************************************/
/*
**	args = util_getarguments(utilbase)
**	Get system-wide argument string
*/

EXPORT TSTRPTR
util_getargs(TMOD_UTIL *util)
{
	return util->tmu_Arguments;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: util_args.c,v $
**	Revision 1.3  2005/09/13 02:42:48  tmueller
**	updated copyright reference
**	
**	Revision 1.2  2004/07/05 21:33:42  tmueller
**	added TUtilGetArgs() and TUtilParseArgs()
**	
**	Revision 1.1  2004/07/04 21:42:12  tmueller
**	The utility module grew too large -- now splitted over several files
**	
*/
