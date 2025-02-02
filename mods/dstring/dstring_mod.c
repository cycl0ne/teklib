
/*
**	dynamic string library
*/

#include <tek/exec.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/mod/dstring.h>
#include <tek/debug.h>

#define MOD_VERSION		0
#define MOD_REVISION	4
#define NUMVECTORS		21

static const TAPTR vectors[NUMVECTORS];

typedef struct
{
	struct TModule module;
	TAPTR pool;

} DSMOD;

#define TExecBase	TGetExecBase(ds)

#define TDSTR_ALLOC			32		/* granularity of (re)allocations [bytes]. must be 2^n */
#define _arrayalign(len)	(((len) + TDSTR_ALLOC - 1) & ~(TDSTR_ALLOC - 1))


/*
**	module prototypes
*/

static TCALLBACK DSMOD *mod_open(DSMOD *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(DSMOD *mod, TAPTR task);

static TMODAPI TINT ds_newarray(DSMOD *ds, TDSTR *s, TINT len);
static TMODAPI TVOID ds_delarray(DSMOD *ds, TDSTR *s);
static TMODAPI TINT ds_arraysetlen(DSMOD *ds, TDSTR *s, TINT len);
static TMODAPI TDSTR ds_newstring(DSMOD *ds);
static TMODAPI TDSTR ds_newstringstr(DSMOD *ds, TSTRPTR initial);
static TMODAPI TDSTR _ds_stringdup(DSMOD *ds, TDSTR *olds);
static TMODAPI TINT _ds_stringcpy(DSMOD *ds, TDSTR *d, TDSTR *s);
static TMODAPI TINT _ds_stringcpystr(DSMOD *ds, TDSTR *d, TSTRPTR s);
static TMODAPI TINT _ds_stringncpy(DSMOD *ds, TDSTR *d, TDSTR *s, TINT max);
static TMODAPI TINT _ds_stringncpystr(DSMOD *ds, TDSTR *d, TSTRPTR s, TINT max);
static TMODAPI TINT _ds_stringcat(DSMOD *ds, TDSTR *d, TDSTR *s);
static TMODAPI TINT _ds_stringcatstr(DSMOD *ds, TDSTR *d, TSTRPTR s);
static TMODAPI TINT _ds_stringncat(DSMOD *ds, TDSTR *d, TDSTR *s, TINT max);
static TMODAPI TINT _ds_stringncatstr(DSMOD *ds, TDSTR *d, TSTRPTR s, TINT max);
static TMODAPI TINT _ds_stringcatchr(DSMOD *ds, TDSTR *d, TINT8 c);
static TMODAPI TINT _ds_stringtrunc(DSMOD *ds, TDSTR *s, TINT maxlen);
static TMODAPI TINT _ds_stringexpstr(DSMOD *ds, TDSTR *s, TINT maxlen, TSTRPTR expstr, TINT explen);
static TMODAPI TINT _ds_stringexpchr(DSMOD *ds, TDSTR *s, TINT maxl, TINT8 c);
static TMODAPI TINT _ds_stringexp(DSMOD *ds, TDSTR *s1, TINT maxline, TDSTR *s2);
static TMODAPI TINT _ds_strcmp(DSMOD *ds, TSTRPTR s1, TSTRPTR s2);
static TMODAPI TINT _ds_strfind(DSMOD *ds, TSTRPTR s1, TSTRPTR s2);


/**************************************************************************
**
**	tek_init_<modname>
**	module initializations (not instance-specific)
*/

TMODENTRY TUINT tek_init_dstring(TAPTR task, DSMOD *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)					/* first call */
		{
			if (version <= MOD_VERSION)			/* version check */
			{
				return sizeof(DSMOD);			/* return module positive size */
			}
		}
		else									/* second call */
		{
			return sizeof(TAPTR) * NUMVECTORS;	/* return module negative size */
		}
	}
	else										/* third call */
	{
		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;
		mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;

		TInitVectors(mod, (TAPTR *) vectors, NUMVECTORS);

		return TTRUE;
	}

	return 0;
}


/**************************************************************************
**
**	instance open
*/

static TCALLBACK DSMOD *mod_open(DSMOD *ds, TAPTR task, TTAGITEM *tags)
{
	ds = TNewInstance(ds, ds->module.tmd_PosSize, ds->module.tmd_NegSize);
	if (!ds) return TNULL;

	ds->pool = TExecCreatePool(TExecBase, TNULL);
	if (ds->pool)
	{
		return ds;
	}

	TFreeInstance(ds);
	return TNULL;
}


/**************************************************************************
**
**	instance close
*/

static TCALLBACK TVOID mod_close(DSMOD *ds, TAPTR task)
{
	TDestroy(ds->pool);
	TFreeInstance(ds);
}


/**************************************************************************
**
**	function table
*/

static const TAPTR vectors[NUMVECTORS] =
{
	(TAPTR) ds_newarray,
	(TAPTR) ds_delarray,
	(TAPTR) ds_arraysetlen,

	(TAPTR) ds_newstring,
	(TAPTR) ds_newstringstr,
	(TAPTR) _ds_stringdup,
	(TAPTR) _ds_stringcpy,
	(TAPTR) _ds_stringcpystr,
	(TAPTR) _ds_stringncpy,
	(TAPTR) _ds_stringncpystr,
	(TAPTR) _ds_stringcat,
	(TAPTR) _ds_stringcatstr,
	(TAPTR) _ds_stringncat,
	(TAPTR) _ds_stringncatstr,
	(TAPTR) _ds_stringcatchr,
	(TAPTR) _ds_stringtrunc,
	(TAPTR) _ds_stringexpstr,
	(TAPTR) _ds_stringexpchr,
	(TAPTR) _ds_stringexp,
	(TAPTR) _ds_strcmp,
	(TAPTR) _ds_strfind,
};


/**************************************************************************
**
**	__stringcpystrn(ds, destdstring, sourcestr, len)
**	copy regular string to dstring (private)
*/

static TINT __stringcpystrn(DSMOD *ds, TDSTR *d, TSTRPTR s, TINT len)
{
	if (_ds_arraylen(d) > 0)
	{
		if (ds_arraysetlen(ds, d, len + 1))
		{
			TSTRPTR p = _ds_arrayptr(d);
			TExecCopyMem(TExecBase, s, p, len);
			p[len] = 0;
			return 1;
		}
	}
	return 0;
}


/**************************************************************************
**
**	__stringcatstrn(ds, destdstring, sourcestr, len)
**	cat regular string to dstring (private)
*/

static TINT __stringcatstrn(DSMOD *ds, TDSTR *d, TSTRPTR s, TINT len)
{
	TINT dlen = _ds_arraylen(d);
	if (dlen > 0)
	{
		if (ds_arraysetlen(ds, d, dlen + len))
		{
			TSTRPTR p = _ds_arrayptr(d) + dlen - 1;
			TExecCopyMem(TExecBase, s, p, len);
			p[len] = 0;
			return 1;
		}
	}
	return 0;
}


/**************************************************************************
**
**	ds_newarray
*/

static TMODAPI TINT ds_newarray(DSMOD *ds, TDSTR *s, TINT len)
{
	if (len <= sizeof(s->s.buf))
	{
		s->uselen = len;
		s->s.buf[0] = 0;		/* ??? */
	}
	else
	{
		s->s.ptr = TExecAllocPool(TExecBase, ds->pool, _arrayalign(len));
		if (s->s.ptr)
		{
			s->uselen = len;
		}
		else
		{
			s->uselen = -1;
			return 0;
		}
	}
	return 1;
}


/**************************************************************************
**
**	ds_delarray
*/

static TMODAPI TVOID ds_delarray(DSMOD *ds, TDSTR *s)
{
	if (!_ds_arraylocal(s))
	{
		TExecFreePool(TExecBase, ds->pool, s->s.ptr, _arrayalign(s->uselen));
		s->s.ptr = 0;
	}
	s->uselen = -1;
}


/**************************************************************************
**
**	ds_arraysetlen
*/

static TMODAPI TINT ds_arraysetlen(DSMOD *ds, TDSTR *s, TINT len)
{
	if (_ds_arrayvalid(s))
	{
		if (len <= sizeof(s->s.buf))	/* new size fits to local buffer */
		{
			if (!_ds_arraylocal(s))		/* previously allocated */
			{
				TAPTR ptr = s->s.ptr;
				TExecCopyMem(TExecBase, ptr, s->s.buf, len);
				TExecFreePool(TExecBase, ds->pool, ptr, _arrayalign(s->uselen));
			}
		}
		else							/* new size does not fit to local buffer */
		{
			TAPTR newptr;
			TINT newlen = _arrayalign(len);

			if (_ds_arraylocal(s))		/* previously local */
			{
				newptr = TExecAllocPool(TExecBase, ds->pool, newlen);
				if (newptr)
				{
					TExecCopyMem(TExecBase, s->s.buf, newptr, s->uselen);
				}
				else
				{
					len = -1;
				}
			}
			else					/* previously allocated */
			{
				newptr = s->s.ptr;
				if (_arrayalign(s->uselen) != newlen)			
				{
					newptr = TExecReallocPool(TExecBase, ds->pool, newptr, _arrayalign(s->uselen), newlen);
					if (!newptr)
					{
						TExecFreePool(TExecBase, ds->pool, s->s.ptr, _arrayalign(s->uselen));
						len = -1;
					}
				}
			}
			s->s.ptr = newptr;			
		}
		s->uselen = len;
		return (len >= 0);
	}
	return 0;
}


/**************************************************************************
**
**	dstr = ds_newstring(ds)
**	create a new string, length zero
*/

static TMODAPI TDSTR ds_newstring(DSMOD *ds)
{
	TDSTR s;
	if (ds_newarray(ds, &s, 1))
	{
		_ds_arrayptr(&s)[0] = 0;
	}
	return s;
}


/**************************************************************************
**
**	dstr = ds_newstringstr(ds, str)
**	create a new string from a regular string
*/

static TMODAPI TDSTR ds_newstringstr(DSMOD *ds, TSTRPTR initial)
{
	TDSTR s;
	if (initial)
	{
		TSTRPTR p = initial;
		TINT len;

		while (*p) p++;
		len = (p - initial) + 1;

		if (ds_newarray(ds, &s, len))
		{
			TExecCopyMem(TExecBase, initial, _ds_arrayptr(&s), len);
		}
	}
	else if (ds_newarray(ds, &s, 1))
	{
		_ds_arrayptr(&s)[0] = 0;
	}
	return s;
}


/**************************************************************************
**
**	dstr = _ds_stringdup(ds, dstring)
**	create a duplicate from a dstring
*/

static TMODAPI TDSTR _ds_stringdup(DSMOD *ds, TDSTR *olds)
{
	TDSTR news;
	TINT len = _ds_arraylen(olds);
	if (len > 0)
	{
		/* TODO! */
		news = ds_newstringstr(ds, _ds_arrayptr(olds));
	}
	else
	{
		news.uselen = -1;
	}
	return news;
}


/**************************************************************************
**
**	_ds_stringcpy(ds, destdstring, sourcedstring)
**	copy dstring to dstring
*/

static TMODAPI TINT _ds_stringcpy(DSMOD *ds, TDSTR *d, TDSTR *s)
{
	TINT slen = _ds_arraylen(s);
	if (slen > 0)
	{
		return __stringcpystrn(ds, d, _ds_arrayptr(s), slen - 1);
	}
	return 0;
}


/**************************************************************************
**
**	_ds_stringcpystr(ds, destdstring, sourcestr)
**	copy regular string to dstring
*/

static TMODAPI TINT _ds_stringcpystr(DSMOD *ds, TDSTR *d, TSTRPTR s)
{
	if (s)
	{
		TSTRPTR p = s;
		while (*p) p++;
		return __stringcpystrn(ds, d, s, p - s);
	}
	return 0;
}


/**************************************************************************
**
**	_ds_stringncpy(ds, destdstring, sourcedstring, maxlen)
**	copy dstring to dstring, length-limited
*/

static TMODAPI TINT _ds_stringncpy(DSMOD *ds, TDSTR *d, TDSTR *s, TINT max)
{
	TINT slen = _ds_arraylen(s) - 1;
	if (slen >= 0)
	{
		if (max > slen) max = slen;
		return __stringcpystrn(ds, d, _ds_arrayptr(s), max);
	}
	return 0;
}


/**************************************************************************
**
**	_ds_stringcpystr(ds, destdstring, sourcestr, maxlen)
**	copy regular string to dstring, length-limited
*/

static TMODAPI TINT _ds_stringncpystr(DSMOD *ds, TDSTR *d, TSTRPTR s, TINT max)
{
	if (s)
	{
		TINT slen;
		TSTRPTR p = s;
		while (*p) p++;
		slen = p - s;
		if (max > slen) max = slen;
		return __stringcpystrn(ds, d, s, max);
	}
	return 0;
}


/**************************************************************************
**
**	_ds_stringcat(ds, destdstring, sourcedstring)
**	append dstring to dstring
*/

static TMODAPI TINT _ds_stringcat(DSMOD *ds, TDSTR *d, TDSTR *s)
{
	TINT slen = _ds_arraylen(s);
	if (slen > 0)
	{
		return __stringcatstrn(ds, d, _ds_arrayptr(s), slen - 1);
	}
	return 0;
}


/**************************************************************************
**
**	_ds_stringcatstr(ds, destdstring, sourcestr)
**	append regular string to dstring
*/

static TMODAPI TINT _ds_stringcatstr(DSMOD *ds, TDSTR *d, TSTRPTR s)
{
	if (s)
	{
		TSTRPTR p = s;
		while (*p) p++;
		return __stringcatstrn(ds, d, s, p - s);
	}
	return 0;
}


/**************************************************************************
**
**	_ds_stringncat(ds, destdstring, sourcedstring, maxlen)
**	append dstring to dstring, length-limited
*/

static TMODAPI TINT _ds_stringncat(DSMOD *ds, TDSTR *d, TDSTR *s, TINT max)
{
	TINT slen = _ds_arraylen(s) - 1;
	if (slen >= 0)
	{
		if (max > slen) max = slen;
		return __stringcatstrn(ds, d, _ds_arrayptr(s), max);
	}
	return 0;
}


/**************************************************************************
**
**	_ds_stringncatstr(ds, destdstring, sourcestr, maxlen)
**	append regular string to dstring, length-limited
*/

static TMODAPI TINT _ds_stringncatstr(DSMOD *ds, TDSTR *d, TSTRPTR s, TINT max)
{
	if (s)
	{
		TINT slen;
		TSTRPTR p = s;
		while (*p) p++;
		slen = p - s;
		if (max > slen) max = slen;
		return __stringcatstrn(ds, d, s, max);
	}
	return 0;
}


/**************************************************************************
**
**	_ds_stringcatchr(ds, dstring, char)
**	append char to dstring
*/

static TMODAPI TINT _ds_stringcatchr(DSMOD *ds, TDSTR *d, TINT8 c)
{
	TINT dlen = _ds_arraylen(d);
	if (dlen > 0)
	{
		if (ds_arraysetlen(ds, d, dlen + 1))
		{
			TSTRPTR p = _ds_arrayptr(d) + dlen - 1;
			p[0] = c;
			p[1] = 0;
			return 1;
		}
	}
	return 0;
}


/**************************************************************************
**
**	_ds_stringtrunc(ds, dstring, maxlen)
**	truncate dstring to maxlen
*/

static TMODAPI TINT _ds_stringtrunc(DSMOD *ds, TDSTR *s, TINT maxlen)
{
	TINT len = _ds_arraylen(s);
	if (len > 0 && len > maxlen + 1)
	{
		if (ds_arraysetlen(ds, s, maxlen + 1))
		{
			_ds_arrayptr(s)[maxlen] = 0;
			return 1;			
		}
		return 0;
	}
	return 1;
}


/**************************************************************************
**
**	_ds_stringexpstr(ds, dstring, maxlen, expstr, explen)
**	expand dstring to maxlen, fill with expstr/explen
*/

static TMODAPI TINT _ds_stringexpstr(DSMOD *ds, TDSTR *s, TINT maxlen, TSTRPTR expstr, TINT explen)
{
	TINT len = _ds_arraylen(s);
	if (len > 0 && len < maxlen + 1)
	{
		if (ds_arraysetlen(ds, s, maxlen + 1))
		{
			TSTRPTR p = _ds_arrayptr(s) + len - 1;
			if (explen == 1)
			{
				TINT i;
				TINT8 c = *expstr;
				for (i = 0; i < maxlen - len + 1; ++i)
				{
					*p++ = c;
				}
			}
			else
			{
				TINT left = maxlen - len + 1;
				do
				{	
					if (left < explen)
					{
						TExecCopyMem(TExecBase, expstr, p, left);
						p += left;
						break;
					}

					TExecCopyMem(TExecBase, expstr, p, explen);					
					p += explen;
					left -= explen;

				} while (left);
			}
			*p = 0;
			return 1;		
		}
		return 0;
	}
	return 1;
}


/**************************************************************************
**
**	_ds_stringexpchr(ds, dstring, maxlen, char)
**	expand dstring to maxlen, fill with character
*/

static TMODAPI TINT _ds_stringexpchr(DSMOD *ds, TDSTR *s, TINT maxl, TINT8 c)
{
	return _ds_stringexpstr(ds, s, maxl, &c, 1);
}


/**************************************************************************
**
**	_ds_stringexp(ds, dstring, maxlen, expdstring)
**	expand dstring to maxlen, fill with expdstring
*/

static TMODAPI TINT _ds_stringexp(DSMOD *ds, TDSTR *s1, TINT maxline, TDSTR *s2)
{
	TINT len2 = _ds_arraylen(s2);
	if (_ds_arraylen(s1) > 0 && len2 > 0)
	{
		return _ds_stringexpstr(ds, s1, maxline, _ds_arrayptr(s2), len2 - 1);
	}
	return 0;
}


/**************************************************************************
**
**	_ds_strcmp(ds, str1, str2)
**	compare regular strings, with extended semantics: either or
**	both strings may be TNULL. TNULL is "less than" a string.
*/

static TMODAPI TINT _ds_strcmp(DSMOD *ds, TSTRPTR s1, TSTRPTR s2)
{
	if (s1)
	{
		if (s2)
		{
			TINT8 t1 = *s1, t2 = *s2, c1, c2;

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


/**************************************************************************
**
**	pos = _ds_strfind(ds, str1, str2)
**	find str2 in str1, return starting pos, or -1 if not found or
**	string(s) invalid
*/

static TMODAPI TINT _ds_strfind(DSMOD *ds, TSTRPTR s1, TSTRPTR s2)
{
	TINT res = -1;
	if (s1 && s2)
	{
		TINT c, d = *s2, x = 0, p = 0;
		while ((c = s1[p++]))
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
					res = p - x;
					break;
				}
			}
		}
	}
	return res;
}
