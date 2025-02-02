
#include <tek/teklib.h>
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/inline/io.h>

#include "parse.h"
#include "parse_tmkmf.h"
#include "global.h"

const TTAG r_main[];

static TPNODE *p_createnode(TPARSE *p);
static TINT p_getchar(TPARSE *p);

/*****************************************************************************/
/*
**	destroy
*/

static void destroynode(TPARSE *p, TPNODE *pnode)
{
	TINT i;
	struct TNode *nextnode, *node;

	dFreeString(DSTR, pnode->data);

	for (i = 0; i < pnode->numattr; i++)
	{
		dFreeString(DSTR, pnode->attributes[i*2]);
		dFreeString(DSTR, pnode->attributes[i*2+1]);
	}
	TFree(pnode->attributes);

	node = pnode->list.tlh_Head;
	while ((nextnode = node->tln_Succ))
	{
		TRemove(node);
		destroynode(p, (TPNODE *) node);
		node = nextnode;
	}

	TFree(pnode);
}

static TTAG
p_destroy(struct THook *hook, TAPTR obj, TTAG msg)
{
	TPARSE *p = obj;
	TINT i;

	/* clean up from rootnode, recursively */
	destroynode(p, p->nodestack[0]);

	for (i = 0; i < p->numattr; ++i)
	{
		dFreeString(DSTR, p->attributes[i*2]);
		dFreeString(DSTR, p->attributes[i*2+1]);
	}
	TFree(p->attributes);

	dFreeString(DSTR, p->tagname);
	dFreeString(DSTR, p->attname);
	dFreeString(DSTR, p->attvalue);
	dFreeString(DSTR, p->closingtagname);

	TFree(p->nodestack);
	TFree(p->currentnode);
	TFree(p);

	return 0;
}

/*****************************************************************************/
/*
**	new
*/

TPARSE *parse_new(TAPTR readdata, TINT(*readfunc)(TAPTR),
	TINT *errnum, TINT *errline)
{
	if (readfunc)
	{
		TPARSE *p;
		p = TAlloc0(TNULL, sizeof(TPARSE));
		if (p)
		{
			p->handle.thn_Hook.thk_Entry = p_destroy;
			p->readdata = readdata;
			p->readfunc = readfunc;

			p->currentnode = TAlloc(TNULL, sizeof(TPNODE *));
			p->nodestack = TAlloc(TNULL, sizeof(TPNODE *));

			if (p->currentnode && p->nodestack)
			{
				p->currentnode[0] = TNULL;
				p->nodestack[0] = p_createnode(p);
				if (p->nodestack[0])
				{
					p->depth = 1;
					p->getnext = TTRUE;
					p->error = PERR_OKAY;

					p->tagname = -1;
					p->attname = -1;
					p->attvalue = -1;
					p->closingtagname = -1;

					if (parse(p, r_main,
						(TINT(*)(TAPTR)) p_getchar) != P_OKAY)
					{
						TINT i;

						if (errline) *errline = p->line + 1;
						if (errnum) *errnum = p->error;

						/* clean up nodes[i>0] on stack */
						for (i = p->depth - 1; i >= 1; --i)
							destroynode(p, p->nodestack[i]);
					}
					else
						return p;
				}
			}

			TDestroy(&p->handle);
		}
	}
	return TNULL;
}


/*****************************************************************************/
/*
**	nodes
*/

static TPNODE *p_createnode(TPARSE *p)
{
	TPNODE *node;
	node = TAlloc(TNULL, sizeof(TPNODE));
	if (node)
	{
		node->data = -1;
		node->flags = PNODEF_NONE;
		node->attributes = TNULL;
		node->numattr = 0;
		node->numentries = 0;
		node->line = p->line;
		TInitList(&node->list);
	}
	return node;
}

static TPNODE *pushnode(TPARSE *p)
{
	TPNODE *pnode = p_createnode(p);
	if (pnode)
	{
		p->nodestack = TRealloc(p->nodestack,
			sizeof(TPNODE *) * (p->depth + 1));
		if (p->nodestack)
		{
			p->nodestack[p->depth] = pnode;
			p->depth++;
			return pnode;
		}
		TFree(pnode);
	}
	return TNULL;
}

static TPNODE *poppnode(TPARSE *p)
{
	if (p->depth > 0)
		return p->nodestack[--p->depth];
	return TNULL;
}

static TPNODE *currentnode(TPARSE *p)
{
	if (p->depth > 0)
		return p->nodestack[p->depth - 1];
	return TNULL;
}


/*****************************************************************************/
/*
**	parser callbacks
*/

static TINT p_getchar(TPARSE *p)
{
	if (p->getnext)
	{
		p->getnext = TFALSE;
		p->c = (*p->readfunc)(p->readdata);
	}
	return p->c;
}

static TINT p_nextchar(TPARSE *p, TINT dummy)
{
	if (p->getnext)
		(*p->readfunc)(p->readdata);
	p->getnext = TTRUE;
	return 0;
}

static TINT p_countline(TPARSE *p, TINT c)
{
	p->line++;
	return P_OKAY;
}

static TINT p_endfile(TPARSE *p, TINT c)
{
	if (p->depth > 1)
	{
		p->error = PERR_TAG_STILL_OPEN;
		return P_ERROR;
	}
	return P_OKAY;
}

static TINT p_error(TPARSE *p, TINT c)
{
	p->error = PERR_PARSE_ERROR;
	return P_ERROR;
}

static TINT p_opendata(TPARSE *p, TINT c)
{
	TPNODE *node;

	if (p->depth == 1)
	{
		p->error = PERR_GARBAGE; /* data on document root level */
		return P_ERROR;
	}

	node = pushnode(p);
	if (node)
	{
		node->data = dAllocString(DSTR, TNULL);
		node->flags |= PNODEF_DATA;
		return P_OKAY;
	}
	p->error = PERR_OUT_OF_MEMORY;
	return P_ERROR;
}

static TUINT p_closedata(TPARSE *p, TINT c)
{
	TPNODE *node, *parentnode;
	node = poppnode(p);
	parentnode = currentnode(p);
	if (parentnode)
	{
		TAddTail(&parentnode->list, (struct TNode *) node);
		parentnode->numentries++;
	}
	else
	{
		p->error = PERR_GARBAGE;
		return P_ERROR;
	}
	return P_OKAY;
}

static TUINT p_putdata(TPARSE *p, TINT c)
{
	TPNODE *node = currentnode(p);
	dSetCharString(DSTR, node->data, -1, c);
	return P_OKAY;
}

static TUINT p_putdatawhite(TPARSE *p, TINT c)
{
	return p_putdata(p, 32);
}
static TUINT p_putdatabackslash(TPARSE *p, TINT c)
{
	return p_putdata(p, '\\');
}
static TUINT p_putdatacr(TPARSE *p, TINT c)
{
	return p_putdata(p, 10);
}
static TUINT p_putdatatab(TPARSE *p, TINT c)
{
	return p_putdata(p, 9);
}
static TUINT p_putdatapercent(TPARSE *p, TINT c)
{
	return p_putdata(p, '%');
}

static TUINT p_opentagname(TPARSE *p, TINT c)
{
	p->tagname = dAllocString(DSTR, TNULL);
	return P_OKAY;
}

static TUINT p_closetagname(TPARSE *p, TINT c)
{
	return P_OKAY;
}

static TUINT p_puttagname(TPARSE *p, TINT c)
{
	dSetCharString(DSTR, p->tagname, -1, c);
	return P_OKAY;
}

static TUINT p_openattname(TPARSE *p, TINT c)
{
	p->attname = dAllocString(DSTR, TNULL);
	return P_OKAY;
}

static TUINT p_closeattname(TPARSE *p, TINT c)
{
	p->numattr++;

	if (!p->attributes)
		p->attributes = TAlloc(TNULL, sizeof(TINT) * (p->numattr * 2));
	else
		p->attributes = TRealloc(p->attributes,
			sizeof(TINT) * (p->numattr * 2));

	if (p->attributes)
	{
		if (p->numattr > 1)
		{
			TINT i;
			for (i = 0; i < p->numattr - 1; ++i)
			{
				if (dCmpNString(DSTR, p->attname, p->attributes[i],
					0, 0, -1) == 0)
				{
					p->error = PERR_DUPLICATE_ATTRIBUTE;
					return P_ERROR;
				}
			}
		}

		p->attributes[p->numattr * 2 - 2] = p->attname;
		p->attributes[p->numattr * 2 - 1] = -1;
		p->attname = -1;
		return P_OKAY;
	}

	p->error = PERR_OUT_OF_MEMORY;
	return P_ERROR;
}

static TUINT p_putattname(TPARSE *p, TINT c)
{
	dSetCharString(DSTR, p->attname, -1, c);
	return P_OKAY;
}

static TUINT p_openattvalue(TPARSE *p, TINT c)
{
	p->attvalue = dAllocString(DSTR, TNULL);
	return P_OKAY;
}

static TUINT p_closeattvalue(TPARSE *p, TINT c)
{
	p->attributes[p->numattr * 2 - 1] = p->attvalue;
	p->attvalue = -1;
	return P_OKAY;
}

static TUINT p_putattvalue(TPARSE *p, TINT c)
{
	dSetCharString(DSTR, p->attvalue, -1, c);
	return P_OKAY;
}

static TUINT p_openclosetagname(TPARSE *p, TINT c)
{
	p->closingtagname = dAllocString(DSTR, TNULL);
	return P_OKAY;
}

static TUINT p_closeclosetagname(TPARSE *p, TINT c)
{
	TPNODE *node = currentnode(p);
	if (node)
	{
		if (dCmpNString(DSTR, node->data, p->closingtagname, 0, 0, -1) == 0)
		{
			dFreeString(DSTR, p->closingtagname);
			p->closingtagname = -1;
			return P_OKAY;
		}
	}
	p->error = PERR_UNMATCHED_TAG;
	return P_ERROR;
}

static TUINT p_putclosetagname(TPARSE *p, TINT c)
{
	dSetCharString(DSTR, p->closingtagname, -1, c);
	return P_OKAY;
}

static TUINT p_tagenter(TPARSE *p, TINT c)
{
	TPNODE *node = pushnode(p);
	if (node)
	{
		node->data = p->tagname;
		node->flags |= PNODEF_NAME;
		p->tagname = -1;

		node->attributes = p->attributes;
		p->attributes = TNULL;

		node->numattr = p->numattr;
		p->numattr = 0;
		return P_OKAY;
	}
	p->error = PERR_OUT_OF_MEMORY;
	return P_ERROR;
}

static TUINT p_tagleave(TPARSE *p, TINT c)
{
	TPNODE *node = poppnode(p);
	if (node)
	{
		TPNODE *parentnode;

		parentnode = currentnode(p);
		TAddTail(&parentnode->list, (struct TNode *) node);
		parentnode->numentries++;

		return P_OKAY;
	}

	p->error = PERR_UNMATCHED_TAG;
	return P_ERROR;
}

static TINT p_closeonly(TPARSE *p, TINT c)
{
	TPNODE *parentnode = currentnode(p);
	TPNODE *node = pushnode(p);
	if (node)
	{
		node->flags |= PNODEF_NAME;
		node->data = p->tagname;
		p->tagname = -1;

		node->attributes = p->attributes;
		p->attributes = TNULL;

		node->numattr = p->numattr;
		p->numattr = 0;

		TAddTail(&parentnode->list, (struct TNode *) node);
		parentnode->numentries++;

		poppnode(p);

		return P_OKAY;
	}
	p->error = PERR_OUT_OF_MEMORY;
	return P_ERROR;
}

static TINT p_checkroot(TPARSE *p, TINT c)
{
	/* multi-root allowed */

	#if 0
		if (p->depth == 1)
		{
			if (currentnode(p)->numentries > 0)
			{
				p->error = PERR_MULTIPLE_ROOT;
				return P_ERROR;
			}
		}
	#endif

	return P_OKAY;
}


/*****************************************************************************/
/*
**	charsets
*/

static const TINT c_white[] = {32,9,0};

static const TINT c_namefirst[] = {
'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s',
't','u','v','w','x','y','z','A','B','C','D','E','F','G','H','I','J','K','L',
'M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z', '_', ':', 0 };

static const TINT c_name[] = {
'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s',
't','u','v','w','x','y','z','A','B','C','D','E','F','G','H','I','J','K','L',
'M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z', '0', '1', '2', '3',
'4', '5', '6', '7', '8', '9', '.', '-', '_', ':', 0 };

static const TINT c_opentag[] = {'[',0};
static const TINT c_closetag[] = {']',0};
static const TINT c_equal[] = {'=',0};
static const TINT c_brack[] = {34,0};
static const TINT c_slash[] = {47,0};
static const TINT c_eof[] = {TEOF, 0};
static const TINT c_cr[] = {13, 0};
static const TINT c_lf[] = {10, 0};
static const TINT c_excl[] = {'!', 0};
static const TINT c_dash[] = {'-', 0};
static const TINT c_backslash[] = {'\\', 0};
static const TINT c_n[] = {'n', 0};
static const TINT c_t[] = {'t', 0};
static const TINT c_percent[] = {'%', 0};


/*****************************************************************************/
/*
**	ruleset
*/

const TTAG r_data[];
const TTAG r_datawhite[];
const TTAG r_opentag[];
const TTAG r_newclosetag[];
const TTAG r_tagname_open[];
const TTAG r_tagname_close[];
const TTAG r_moretag[];
const TTAG r_attname[];
const TTAG r_attname2[];
const TTAG r_attvalue[];
const TTAG r_attvalue2[];
const TTAG r_comment[];
const TTAG r_comment2[];
const TTAG r_comment4[];
const TTAG r_comment5[];
const TTAG r_comment6[];
const TTAG r_comment8[];
const TTAG r_closeonly[];
const TTAG r_databackslash[];
const TTAG r_databackslash2[];
const TTAG r_databackslashwhite[];

#define	c_else			TNULL
#define	p_done			TNULL

const TTAG r_main[] =
{
	(TTAG) c_white,		(TTAG) r_main,		(TTAG) p_nextchar, p_done,
	(TTAG) c_opentag,	(TTAG) r_opentag,	(TTAG) p_nextchar, p_done,
	(TTAG) c_lf,		(TTAG) r_main,		(TTAG) p_countline, (TTAG) p_nextchar, p_done,
	(TTAG) c_cr,		(TTAG) r_main,		(TTAG) p_nextchar, p_done,
	(TTAG) c_eof,		(TTAG) TNULL,		(TTAG) p_endfile, p_done,
	(TTAG) c_else,		(TTAG) r_data,		(TTAG) p_opendata, p_done
};

const TTAG r_data[] =
{
	(TTAG) c_white,		(TTAG) r_datawhite,		(TTAG) p_nextchar, p_done,
	(TTAG) c_opentag,	(TTAG) r_opentag,		(TTAG) p_closedata, (TTAG) p_nextchar, p_done,
	(TTAG) c_lf,		(TTAG) r_main,			(TTAG) p_closedata, (TTAG) p_countline, (TTAG) p_nextchar, p_done,
	(TTAG) c_cr,		(TTAG) r_datawhite,		(TTAG) p_nextchar, p_done,
	(TTAG) c_eof,		(TTAG) TNULL,			(TTAG) p_closedata, (TTAG) p_endfile, p_done,
	(TTAG) c_backslash,	(TTAG) r_databackslash,	(TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) r_data,			(TTAG) p_putdata, (TTAG) p_nextchar, p_done
};

const TTAG r_datawhite[] =
{
	(TTAG) c_white,		(TTAG) r_datawhite,			(TTAG) p_nextchar, p_done,
	(TTAG) c_opentag,	(TTAG) r_opentag,			(TTAG) p_closedata, (TTAG) p_nextchar, p_done,
	(TTAG) c_lf,		(TTAG) r_main,				(TTAG) p_closedata, (TTAG) p_countline, (TTAG) p_nextchar, p_done,
	(TTAG) c_cr,		(TTAG) r_datawhite,			(TTAG) p_nextchar, p_done,
	(TTAG) c_eof,		(TTAG) TNULL,				(TTAG) p_closedata, (TTAG) p_endfile, p_done,
	(TTAG) c_backslash,	(TTAG) r_databackslashwhite, (TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) r_data,				(TTAG) p_putdatawhite, p_done
};

/*	escaped characters	*/

const TTAG r_databackslashwhite[] =
{
	(TTAG) c_backslash,	(TTAG) r_data,				(TTAG) p_putdatawhite, (TTAG) p_putdatabackslash, (TTAG) p_nextchar, p_done,	/* backslash */
	(TTAG) c_n,			(TTAG) r_data,				(TTAG) p_putdatawhite, (TTAG) p_putdatacr, (TTAG) p_nextchar, p_done,			/* cr */
	(TTAG) c_t,			(TTAG) r_data,				(TTAG) p_putdatawhite, (TTAG) p_putdatatab, (TTAG) p_nextchar, p_done,			/* tab */
	(TTAG) c_percent,	(TTAG) r_data,				(TTAG) p_putdatawhite, (TTAG) p_putdatapercent, (TTAG) p_nextchar, p_done,		/* percent sign */
	(TTAG) c_else,		(TTAG) r_databackslash2,	(TTAG) p_done,
};

const TTAG r_databackslash[] =
{
	(TTAG) c_backslash,	(TTAG) r_data,				(TTAG) p_putdatabackslash, (TTAG) p_nextchar, p_done,	/* backslash */
	(TTAG) c_n,			(TTAG) r_data,				(TTAG) p_putdatacr, (TTAG) p_nextchar, p_done,			/* cr */
	(TTAG) c_t,			(TTAG) r_data,				(TTAG) p_putdatatab, (TTAG) p_nextchar, p_done,			/* tab */
	(TTAG) c_percent,	(TTAG) r_data,				(TTAG) p_putdatapercent, (TTAG) p_nextchar, p_done,		/* percent sign */
	(TTAG) c_else,		(TTAG) r_databackslash2,	(TTAG) p_done,
};

const TTAG r_databackslash2[] =
{
	(TTAG) c_white,		(TTAG) r_databackslash2,	(TTAG) p_nextchar, p_done,
	(TTAG) c_cr,		(TTAG) r_databackslash2,	(TTAG) p_nextchar, p_done,
	(TTAG) c_lf,		(TTAG) r_data,				(TTAG) p_countline, (TTAG) p_nextchar, p_done,
	(TTAG) c_eof,		(TTAG) TNULL,				(TTAG) p_closedata, (TTAG) p_endfile, p_done,
	(TTAG) c_else,		(TTAG) TNULL,				(TTAG) p_error, p_done
};

const TTAG r_opentag[] =
{
	(TTAG) c_namefirst,	(TTAG) r_tagname_open,		(TTAG) p_checkroot, (TTAG) p_opentagname, (TTAG) p_puttagname, (TTAG) p_nextchar, p_done,
	(TTAG) c_slash,		(TTAG) r_newclosetag,		(TTAG) p_checkroot, (TTAG) p_nextchar, p_done,
	(TTAG) c_excl,		(TTAG) r_comment,			(TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,				(TTAG) p_error, p_done
};

const TTAG r_newclosetag[] =
{
	(TTAG) c_namefirst,	(TTAG) r_tagname_close,	(TTAG) p_openclosetagname, (TTAG) p_putclosetagname, (TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,			(TTAG) p_error, p_done
};

const TTAG r_tagname_open[] =
{
	(TTAG) c_name,		(TTAG) r_tagname_open,	(TTAG) p_puttagname, (TTAG) p_nextchar, p_done,
	(TTAG) c_white,		(TTAG) r_moretag,		(TTAG) p_closetagname, (TTAG) p_nextchar, p_done,
	(TTAG) c_slash,		(TTAG) r_closeonly,		(TTAG) p_nextchar, p_done,
	(TTAG) c_closetag,	(TTAG) r_main,			(TTAG) p_closetagname, (TTAG) p_tagenter, (TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,			(TTAG) p_error, p_done
};

const TTAG r_moretag[] =
{
	(TTAG) c_white,		(TTAG) r_moretag,	(TTAG) p_nextchar, p_done,
	(TTAG) c_namefirst,	(TTAG) r_attname,	(TTAG) p_openattname, (TTAG) p_putattname, (TTAG) p_nextchar, p_done,
	(TTAG) c_closetag,	(TTAG) r_main,		(TTAG) p_tagenter, (TTAG) p_nextchar, p_done,
	(TTAG) c_slash,		(TTAG) r_closeonly,	(TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,		(TTAG) p_error, p_done
};

const TTAG r_closeonly[] =
{
	(TTAG) c_closetag,	(TTAG) r_main,		(TTAG) p_closeonly, (TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,		(TTAG) p_error, p_done
};

const TTAG r_attname[] =
{
	(TTAG) c_name,		(TTAG) r_attname,	(TTAG) p_putattname, (TTAG) p_nextchar, p_done,
	(TTAG) c_equal,		(TTAG) r_attvalue,	(TTAG) p_closeattname, (TTAG) p_nextchar, p_done,
	(TTAG) c_white,		(TTAG) r_attname2,	(TTAG) p_closeattname, (TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,		(TTAG) p_error, p_done
};

const TTAG r_attname2[] =
{
	(TTAG) c_white,		(TTAG) r_attname2,	(TTAG) p_nextchar, p_done,
	(TTAG) c_equal,		(TTAG) r_attvalue,	(TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,		(TTAG) p_error, p_done
};

const TTAG r_attvalue[] =
{
	(TTAG) c_brack,		(TTAG) r_attvalue2,	(TTAG) p_openattvalue, (TTAG) p_nextchar, p_done,
	(TTAG) c_white,		(TTAG) r_attvalue,	(TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,		(TTAG) p_error, p_done
};

const TTAG r_attvalue2[] =
{
	(TTAG) c_brack,	(TTAG) r_moretag,	(TTAG) p_closeattvalue, (TTAG) p_nextchar, p_done,
	(TTAG) c_eof,	(TTAG) TNULL,		(TTAG) p_error, p_done,
	(TTAG) c_lf,	(TTAG) TNULL,		(TTAG) p_error, p_done,
	(TTAG) c_else,	(TTAG) r_attvalue2,	(TTAG) p_putattvalue, (TTAG) p_nextchar, p_done,
};

const TTAG r_tagname_close[] =
{
	(TTAG) c_name,		(TTAG) r_tagname_close,	(TTAG) p_putclosetagname, (TTAG) p_nextchar, p_done,
	(TTAG) c_closetag,	(TTAG) r_main,			(TTAG) p_closeclosetagname, (TTAG) p_tagleave, (TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,			(TTAG) p_error, p_done
};

const TTAG r_comment[] =
{
	(TTAG) c_dash,		(TTAG) r_comment2,	(TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,		(TTAG) p_error, p_done
};

const TTAG r_comment2[] =
{
	(TTAG) c_dash,		(TTAG) r_comment4,	(TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) TNULL,		(TTAG) p_error, p_done
};

const TTAG r_comment4[] =
{
	(TTAG) c_dash,		(TTAG) r_comment4,	(TTAG) p_nextchar, p_done,
	(TTAG) c_closetag,	(TTAG) TNULL,		(TTAG) p_error, p_done,
	(TTAG) c_else,		(TTAG) r_comment5,	(TTAG) p_done
};

const TTAG r_comment5[] =
{
	(TTAG) c_dash,		(TTAG) r_comment6,	(TTAG) p_nextchar, p_done,
	(TTAG) c_lf,		(TTAG) r_comment5,	(TTAG) p_countline, (TTAG) p_nextchar, p_done,
	(TTAG) c_eof,		(TTAG) TNULL,		(TTAG) p_error, p_done,
	(TTAG) c_else,		(TTAG) r_comment5,	(TTAG) p_nextchar, p_done
};

const TTAG r_comment6[] =
{
	(TTAG) c_dash,		(TTAG) r_comment8,	(TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) r_comment5,	(TTAG) p_done
};

const TTAG r_comment8[] =
{
	(TTAG) c_dash,		(TTAG) r_comment8,	(TTAG) p_nextchar, p_done,
	(TTAG) c_closetag,	(TTAG) r_main,		(TTAG) p_nextchar, p_done,
	(TTAG) c_else,		(TTAG) r_comment5,	(TTAG) p_done
};

