
/*
**	$Id: util_args.c,v 1.2 2006/09/10 14:40:37 tmueller Exp $
**	teklib/mods/util/util_args.c - Argument parsing
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>,
**	Daniel Trompetter <dtrompetter at oxyron.de>,
**	Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "util_mod.h"
#include <tek/mod/exec.h>

/*****************************************************************************/
/*
**	argument parser
*/

/* string: */
#define ARGFL_		0x0000
/* number: */
#define ARGFL_N		0x0001
/* multiple strings: */
#define ARGFL_M		0x0002
/* switch: */
#define ARGFL_S		0x0003
/* double: */
/*#define ARGFL_D		0x0004*/
/* mask for the above: */
#define ARGFL_MASK	0x000f
/* keyword: */
#define ARGFL_K		0x0010
/* required: */
#define ARGFL_A		0x0020

/* valid characters in a template: */
#define ARG_TEMPLATECHAR(c) ((c >= 'a' && c <= 'z') || \
	(c >= 'A' && c <= 'Z') || c == '-' || c == '_' || c == '?' || \
	(c >= '0' && c <= '9'))

/* maximum number of arguments in template: */
#define ARG_MAXTEMPLATENUM 65535

/* character used for escaping: */
#define ARG_ESCAPECHAR '\\'

#define ARG_MAX(a,b) ((a)>(b)?(a):(b))
#define ARG_TRUE  (1 == 1)
#define ARG_FALSE (1 != 1)

typedef TCHR arg_char_t;
typedef TINT arg_integer_t;
typedef TDOUBLE arg_number_t;
typedef TBOOL arg_bool_t;
typedef TAPTR arg_handle_t;
typedef TTAG arg_t;

#define arg_strton(udata, s, valp) TStrToI(s, valp)
/*#define arg_strtod(udata, s, valp) util_strtod(udata, s, valp)*/

/* argument entry: */
typedef struct
{
	union
	{
		arg_char_t *argstr;
		arg_char_t **multi;
		arg_integer_t integer;
		arg_number_t real;
		arg_bool_t boolean;
	} arg;
	TUINT flags;
} arg_entry_t;

/*****************************************************************************/

static size_t
arg_strlen(const arg_char_t *s)
{
	const arg_char_t *s2 = s;
	while (*s2++);
	return s2 - s + 1;
}

static arg_char_t *
arg_strcpy(arg_char_t *dest, const arg_char_t *src)
{
	arg_char_t *d = dest;
	while ((*d++ = *src++));
	return dest;
}

static unsigned int
arg_lsb(size_t n)
{
	unsigned int free = 0;
	size_t mask = 1;
	mask <<= (sizeof(n) * 8 - 1);
	while (mask && (n & mask) == 0)
	{
		free++;
		mask >>= 1;
	}
	return sizeof(n) * 8 - free;
}

static int
arg_addsize(size_t a, size_t b, size_t *resultp)
{
	if (ARG_MAX(arg_lsb(a), arg_lsb(b)) + 1 > sizeof(*resultp) * 8)
		return ARG_FALSE;
	if (resultp) *resultp = a + b;
	return ARG_TRUE;
}

static int
arg_mulsize(size_t a, size_t b, size_t *resultp)
{
	if (arg_lsb(a) + arg_lsb(b) > sizeof(*resultp) * 8)
		return ARG_FALSE;
	if (resultp) *resultp = a * b;
	return ARG_TRUE;
}

static void *
arg_realloc(arg_handle_t *a, void *mem, size_t size)
{
	TAPTR exec = TGetExecBase(a);
	if (mem == TNULL)
		return TExecAlloc(exec, (struct TMemManager *) a, size);
	return TExecRealloc(exec, mem, size);
}


/*****************************************************************************/
/*
**	convert argument string to an argv[]-style string vector
**	handles foo=bar style arguments and quoted arguments
*/

struct arg_tokendata
{
	void (*emitchar)(struct arg_tokendata *, arg_char_t c);
	void (*emitarg)(struct arg_tokendata *);
	size_t len;
	size_t numarg;
	int index;
	arg_char_t *buf;
	arg_char_t **argv;
};

static void
arg_tokenize(const arg_char_t *s, struct arg_tokendata *data)
{
	int escaped = 0, state = -1, literal;
	arg_char_t c, lc = 0;

	do
	{
		literal = 0;
		c = *s++;
		if (c != 0 && escaped)
		{
			escaped = 0;
			literal = 1;
			lc = c;
			c = 1; /* fallthru to default case */
		}
		else if (c == ARG_ESCAPECHAR)
		{
			escaped = 1;
			continue;
		}

		switch (c)
		{
			case 0:
				if (state != -1)
					(*data->emitarg)(data);
				break;
			case '\n': case '\r': case ' ': case '\t':
				if (state == 2)
					(*data->emitchar)(data, ' ');
				else
				{
					if (state == 1)
						(*data->emitarg)(data);
					state = -1;
				}
				break;
			case '"':
				if (state == 2)
				{
					(*data->emitarg)(data);
					state = -1;
				}
				else
					state = 2;
				break;
			case '=':
				if (state == 2)
					(*data->emitchar)(data, '=');
				else
				{
 					if (state == 1)
						(*data->emitarg)(data);
					state = -1;
				}
				break;

			default:
				if (state != 2)
					state = 1;
				if (literal)
					(*data->emitchar)(data, lc);
				else
					(*data->emitchar)(data, c);
		}
	} while (c);

}

static void
arg_countarg(struct arg_tokendata *data)
{
	data->numarg++;
}

static void
arg_countchar(struct arg_tokendata *data, arg_char_t c)
{
	data->len++;
}

static void
arg_emitarg(struct arg_tokendata *data)
{
	*(data->buf++) = 0;
	data->argv[data->index++] = data->buf;
}

static void
arg_emitchar(struct arg_tokendata *data, arg_char_t c)
{
	*(data->buf++) = c;
}

static arg_char_t **
args_args2argv(void *a, const arg_char_t *progname, const arg_char_t *args)
{
	struct arg_tokendata data;
	size_t size, size2;

	data.numarg = 0;
	data.len = 0;
	data.emitarg = arg_countarg;
	data.emitchar = arg_countchar;

	/* first pass, calculate length and numargs: */
	arg_tokenize(args, &data);

	if (progname)
	{
		data.len += arg_strlen(progname);
		data.numarg++;
	}

	/* include \0 termination: */
	data.len += data.numarg;

	if (!arg_mulsize(data.numarg + 1, sizeof(arg_char_t *), &size))
		return NULL;
	if (!arg_mulsize(data.len, sizeof(arg_char_t), &size2))
		return NULL;
	if (!arg_addsize(size, size2, &size))
		return NULL;
	data.argv = arg_realloc(a, NULL, size);
	if (!data.argv)
		return NULL;
	data.buf = (arg_char_t *) (data.argv + data.numarg + 1);

	data.emitarg = arg_emitarg;
	data.emitchar = arg_emitchar;
 	data.index = 0;

	if (progname)
	{
		data.argv[data.index++] = data.buf;
		arg_strcpy(data.buf, progname);
		data.buf += arg_strlen(progname) + 1;
	}

	data.argv[data.index++] = data.buf;

	/* second pass, emit arguments: */
	arg_tokenize(args, &data);
	data.argv[data.numarg] = NULL;

	return data.argv;
}

/*****************************************************************************/
/*
**	find key in a template, return template index, or -1 if not found
*/

static int
arg_findarg(const arg_char_t *template, const arg_char_t *keyword)
{
	int n = 0;
	arg_char_t c, d;
	const arg_char_t *p;

loop1:
	p = keyword;

loop2:
	c = *p++;
	d = *template++;

	if (c == 0)
	{
		if (d == 0 || d == '=' || d == '/' || d == ',')
			return n;
		d = *template++;
		goto loop3;
	}

	if (c >= 'a' && c <= 'z')
		c -= 'a' - 'A';
	if (d >= 'a' && d <= 'z')
		d -= 'a' - 'A';

	if (c == d)
		goto loop2;

loop3:
	switch (d)
	{
		case 0:
			return -1;
		case ',':
			if (++n >= ARG_MAXTEMPLATENUM)
				return -1;
		case '=':
			goto loop1;
	}
	d = *template++;
	goto loop3;
}

/*****************************************************************************/
/*
**	parse template string
*/

static int
arg_parsetemplate(arg_handle_t *a, const arg_char_t *template,
	arg_entry_t **pt)
{
	size_t size = 0;
	arg_entry_t *tt;
	arg_char_t c;
	int n = 0;

	*pt = NULL;

main:
	if (!arg_addsize(size, sizeof(arg_entry_t), &size))
		goto error;
	*pt = tt = arg_realloc(a, *pt, size);
	if (tt == NULL)
		goto error;
	tt += n++;

	c = *template;

	if (ARG_TEMPLATECHAR(c))
	{
		tt->flags = ARGFL_; /* no flags yet */
		tt->arg.argstr = NULL; /* no arg yet */
		goto name;
	}
	goto error;

name:
	c = *(++template);
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
	if (ARG_TEMPLATECHAR(c))
		goto name;
	goto error;

alias:
	switch (c)
	{
		case ',':
			template++;
			goto main;
		case '/':
			goto flags;
		case 0:
			goto done;
	}
	if (ARG_TEMPLATECHAR(c))
	{
		c = *(++template);
		goto alias;
	}
	goto error;

flags:
	c = *(++template);
	if (c >= 'a' && c <= 'z')
		c -= 'a' - 'A';
	switch (c)
	{
		case 'A':
			tt->flags |= ARGFL_A;
			goto morefl;
		case 'S':
			tt->flags |= ARGFL_S|ARGFL_K;
			goto morefl;
		case 'K':
			tt->flags |= ARGFL_K;
			goto morefl;
		case 'N':
			tt->flags |= ARGFL_N;
			goto morefl;
		#if 0
		case 'D':
			tt->flags |= ARGFL_D;
			goto morefl;
		#endif
		case 'M':
			tt->flags |= ARGFL_M;
			goto morefl;
	}
	goto error;

morefl:
	c = *(++template);
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

error:
	return -1;

done:
	/* add extra item for collecting multiple arguments: */
	if (!arg_addsize(size, sizeof(arg_entry_t), &size))
		goto error;
	tt = *pt = arg_realloc(a, *pt, size);
	if (!tt)
		goto error;
	tt[n].flags = ARGFL_M;
	tt[n].arg.argstr = NULL;
	return n;
}

/*****************************************************************************/
/*
**	parse argument vector - this implementation is "inspired" by
**	the AROS project -- see AROS/rom/dos/readargs.c
*/

static int
arg_parsearray(arg_handle_t *a, const arg_char_t *template, arg_t *array,
	int (*readfunc)(void *data, arg_char_t **buf), void *data)
{
	arg_entry_t *tt;
	arg_char_t *item;
	arg_char_t **multi = NULL;
	int arg, nextarg, numt, nualt = 0;
	int idx, needextra;
	size_t size = 0;

	numt = arg_parsetemplate(a, template, &tt);
	if (numt <= 0)
		return ARG_FALSE;

	for (arg = 0; arg <= numt; arg = nextarg)
	{
		nextarg = arg + 1;

		if (tt[arg].arg.argstr)
			continue;
		if (tt[arg].flags & ARGFL_K)
			continue;

		if (!(*readfunc)(data, &item))
			break;

		idx = arg_findarg(template, item);
		if (idx >= 0 && !tt[idx].arg.argstr)
		{
			/* keyword option */
			nextarg = arg; /* retry current option in next turn */
			arg = idx;
			if (((tt[arg].flags & ARGFL_MASK) != ARGFL_S) &&
				!(*readfunc)(data, &item))
				return ARG_FALSE;
		}

		switch (tt[arg].flags & ARGFL_MASK)
		{
			#if 0
 			case ARGFL_D:
 			#endif
			case ARGFL_N:
			case ARGFL_:
				tt[arg].arg.argstr = item;
				break;
			case ARGFL_M:
				if (!arg_mulsize(nualt + 1, sizeof(arg_char_t *), &size))
					return ARG_FALSE;
				multi = arg_realloc(a, multi, size);
				if (!multi)
					return ARG_FALSE;
				multi[nualt++] = item;
				nextarg = arg; /* retry in next turn */
				break;
			case ARGFL_S:
				tt[arg].arg.boolean = ARG_TRUE;
				break;
		}
	}

	/* unfilled /A options steal from /M */

	needextra = ARG_TRUE;

	for (arg = numt; arg--;)
	{
		/* argument: unfilled, required, not multi? */
		if (!tt[arg].arg.argstr && (tt[arg].flags & ARGFL_A) &&
			(tt[arg].flags & ARGFL_MASK) != ARGFL_M)
		{
			if (!nualt)
				return ARG_FALSE; /* no arguments left */
			tt[arg].arg.argstr = multi[--nualt]; /* steal */
			needextra = ARG_FALSE; /* no resize needed */
		}
	}

	if (needextra)
	{
		/* add extra entry to multi array, for NULL-termination */
		if (!arg_mulsize(nualt + 1, sizeof(arg_char_t *), &size))
			return ARG_FALSE;
		multi = arg_realloc(a, multi, size);
		if (!multi)
			return ARG_FALSE;
	}

	/* put remaining multi into /M argument */

	for (arg = 0; arg < numt; arg++)
	{
		if ((tt[arg].flags & ARGFL_MASK) == ARGFL_M)
		{
			if ((tt[arg].flags & ARGFL_A) && !nualt)
				return ARG_FALSE;
			multi[nualt] = NULL;
			tt[arg].arg.multi = multi;
			break;
		}
	}

	/* arguments left? */

	if (arg == numt && nualt)
		return ARG_FALSE;

	/* put args to destination array */

	for (arg = 0; arg < numt; ++arg)
	{
		if (tt[arg].arg.argstr)
		{
			switch (tt[arg].flags & ARGFL_MASK)
			{
				#if 0
				case ARGFL_D:
					if (arg_strtod((void *) a, tt[arg].arg.argstr,
						&tt[arg].arg.real) <= 0)
						return ARG_FALSE;
					array[arg] = (arg_t) &tt[arg].arg.real;
					break;
				#endif

				case ARGFL_N:
					if (arg_strton(a, tt[arg].arg.argstr,
						&tt[arg].arg.integer) < 0)
						return ARG_FALSE;
					array[arg] = (arg_t) &tt[arg].arg.integer;
					break;

				case ARGFL_M:
					array[arg] = (arg_t) tt[arg].arg.multi;
					break;

				case ARGFL_:
					array[arg] = (arg_t) tt[arg].arg.argstr;
					break;

				case ARGFL_S:
					array[arg] = (arg_t) tt[arg].arg.boolean;
					break;
			}
		}
	}

	return ARG_TRUE;
}

/*****************************************************************************/
/*
**	util_parseargv(util, template, argv, array)
**		parse an array of strings, aka an argv[] vector
*/

static TINT
rdargvfunc(TSTRPTR **argvp, TSTRPTR *buf)
{
	*buf = *(*argvp)++;
	if (*buf)
		return 1;
	return 0;
}

EXPORT TAPTR
util_parseargv(TAPTR util, TSTRPTR template, TSTRPTR *argv, TTAG *array)
{
	if (template && argv && array)
	{
		TAPTR exec = TGetExecBase(util);
		TAPTR mmu = TExecCreateMemManager(exec, TNULL, TMMT_Pooled, TNULL);
		if (mmu)
		{
			if (arg_parsearray(mmu, template, array,
				(TINT (*)(TAPTR, TSTRPTR *)) rdargvfunc, &argv))
				return mmu;
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
		TAPTR exec = TGetExecBase(util);
		TAPTR mmu = TExecCreateMemManager(exec, TNULL, TMMT_Pooled, TNULL);
		if (mmu)
		{
			TSTRPTR *argv = args_args2argv(mmu, TNULL, args);
			if (argv)
			{
				if (arg_parsearray(mmu, template, array,
					(TINT (*)(TAPTR, TSTRPTR *)) rdargvfunc, &argv))
					return mmu;
			}
			TDestroy(mmu);
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	convert argv[]-style string vector to an argument string
*/

static TSTRPTR
util_argv2args(struct TUtilBase *util, TSTRPTR *argv)
{
	TAPTR exec = TGetExecBase(util);
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
			if (TStrChr(*arg, ' ') || TStrChr(*arg, '='))
				len += 2;
			len += TStrLen(*arg) + 1;
			arg++;
			narg++;
		}

		if (narg == 0)
			return TNULL;

		d = argstr = TExecAlloc(exec, TNULL, len);
		if (d)
		{
			arg = argv;
			while (*arg)
			{
				if (arg != argv) *d++ = ' ';
				len = TStrLen(*arg);
				if ((p = TStrChr(*arg, '=')))
				{
					TStrNCpy(d, *arg, p - *arg);
					d +=  p - *arg;
					*d++ = '=';
					*d++ = '"';
					len = len - (p - *arg) - 1;
					TStrNCpy(d, p + 1, len);
					d += len;
					*d++ = '"';
				}
				else if ((p = TStrChr(*arg, ' ')))
				{
					*d++ = '"';
					TStrNCpy(d, *arg, len);
					d += len;
					*d++ = '"';
				}
				else
				{
					TStrNCpy(d, *arg, len);
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
**	Init system-wide arguments from named atoms
*/

LOCAL TBOOL
util_initargs(struct TUtilBase *util)
{
	TAPTR exec = TGetExecBase(util);

	TAPTR atom_args = TExecLockAtom(exec, "sys.arguments",
		TATOMF_NAME | TATOMF_SHARED);
	TAPTR atom_argv = TExecLockAtom(exec, "sys.argv",
		TATOMF_NAME | TATOMF_SHARED);
	TAPTR atom_prog = TExecLockAtom(exec, "sys.progname",
		TATOMF_NAME | TATOMF_SHARED);

	TSTRPTR args = atom_args ?
		(TSTRPTR) TExecGetAtomData(exec, atom_args) : TNULL;
	TSTRPTR *argv = atom_argv ?
		(TSTRPTR *) TExecGetAtomData(exec, atom_argv) : TNULL;
	TSTRPTR progname = atom_prog ?
		(TSTRPTR) TExecGetAtomData(exec, atom_prog) : TNULL;

	TSTRPTR *newargv = TNULL;

	if (argv && progname == TNULL)
		progname = argv[0];

	if (args)
	{
		TINT len = TStrLen(args);
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

	newargv = args_args2argv(TExecGetTaskMemManager(exec, TNULL), progname,
		args ? args : "");

	TExecUnlockAtom(exec, atom_prog, TATOMF_KEEP);
	TExecUnlockAtom(exec, atom_argv, TATOMF_KEEP);
	TExecUnlockAtom(exec, atom_args, TATOMF_KEEP);

	if (newargv)
	{
		TINT n = 0;
		util->tmu_Arguments = args;
		util->tmu_ArgV = newargv;
		while ((*newargv))
		{
			TDBPRINTF(TDB_INFO,("argv: >%s<\n", *newargv));
			n++;
			newargv++;
		}
		util->tmu_ArgC = n;
		return TTRUE;
	}

	TExecFree(exec, args);
	TExecFree(exec, newargv);
	return TFALSE;
}

LOCAL void
util_freeargs(struct TUtilBase *util)
{
	TAPTR exec = TGetExecBase(util);
	TExecFree(exec, util->tmu_Arguments);
	TExecFree(exec, util->tmu_ArgV);
}

/*****************************************************************************/
/*
**	success = util_setreturn(util, retvalue)
**	Set system-wide return value
*/

EXPORT TBOOL
util_setreturn(struct TUtilBase *util, TINT retval)
{
	TAPTR exec = TGetExecBase(util);
	TAPTR atom;
	TINT *p;

	atom = TExecLockAtom(exec, "sys.returnvalue", TATOMF_NAME);
	if (atom)
	{
		p = (TINT *) TExecGetAtomData(exec, atom);
		if (p)
			*p = retval;
		TExecUnlockAtom(exec, atom, TATOMF_KEEP);
		return TTRUE;
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	argc = util_getargc(utilbase)
**	Get system-wide argument count
*/

EXPORT TINT
util_getargc(struct TUtilBase *util)
{
	return util->tmu_ArgC;
}

/*****************************************************************************/
/*
**	argv = util_getargv(utilbase)
**	Get system-wide argument vector
*/

EXPORT TSTRPTR *
util_getargv(struct TUtilBase *util)
{
	return util->tmu_ArgV;
}

/*****************************************************************************/
/*
**	args = util_getarguments(utilbase)
**	Get system-wide argument string
*/

EXPORT TSTRPTR
util_getargs(struct TUtilBase *util)
{
	return util->tmu_Arguments;
}
