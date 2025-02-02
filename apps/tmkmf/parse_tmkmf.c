
#include <tek/teklib.h>
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/inline/io.h>
#include <tek/inline/unistring.h>

#include "parse.h"
#include "parse_tmkmf.h"
#include "global.h"

TAPTR r_main[];

static TPNODE *p_createnode(TPARSE *p);
static TCALLBACK TINT p_getchar(TPARSE *p);

/*****************************************************************************/
/*
**	destroy
*/

static TVOID destroynode(TPARSE *p, TPNODE *pnode)
{
	TINT i;
	TNODE *nextnode, *node;
	
	TFreeString(pnode->data);

	for (i = 0; i < pnode->numattr; i++)
	{
		TFreeString(pnode->attributes[i*2]);
		TFreeString(pnode->attributes[i*2+1]);
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

static TCALLBACK TVOID p_destroy(TPARSE *p)
{
	TINT i;

	/* clean up from rootnode, recursively */	
	destroynode(p, p->nodestack[0]);

	for (i = 0; i < p->numattr; ++i)
	{
		TFreeString(p->attributes[i*2]);
		TFreeString(p->attributes[i*2+1]);
	}
	TFree(p->attributes);

	TFreeString(p->tagname);
	TFreeString(p->attname);
	TFreeString(p->attvalue);
	TFreeString(p->closingtagname);

	TFree(p->nodestack);
	TFree(p->currentnode);
	TFree(p);
}

/*****************************************************************************/
/*
**	new
*/

TPARSE *parse_new(TAPTR readdata, TCALLBACK TINT(*readfunc)(TAPTR), TINT *errnum, TINT *errline)
{
	if (readfunc)
	{
		TPARSE *p;
		p = TAlloc0(TNULL, sizeof(TPARSE));
		if (p)
		{
			p->handle.thn_DestroyFunc = (TDFUNC) p_destroy;
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

					if (parse(p, r_main, (TCALLBACK TINT(*)(TAPTR)) p_getchar) != P_OKAY)
					{
						TINT i;
						
						if (errline) *errline = p->line + 1;
						if (errnum) *errnum = p->error;
						
						/* clean up nodes[i>0] on stack */
						for (i = p->depth - 1; i >= 1; --i)
						{
							destroynode(p, p->nodestack[i]);
						}
					}
					else
					{
						/*printnode(p->nodestack[0], 0);*/
						return p;
					}
				}
			}

			TDestroy(p);
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
		p->nodestack = TRealloc(p->nodestack, sizeof(TPNODE *) * (p->depth + 1));
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
	{
		return p->nodestack[--p->depth];
	}
	return TNULL;
}

static TPNODE *currentnode(TPARSE *p)
{
	if (p->depth > 0)
	{
		return p->nodestack[p->depth - 1];
	}
	return TNULL;
}


/*****************************************************************************/
/*
**	parser callbacks
*/

static TCALLBACK TINT p_getchar(TPARSE *p)
{
	if (p->getnext)
	{
		p->getnext = TFALSE;
		p->c = (*p->readfunc)(p->readdata);
	}
	return p->c;
}

static TCALLBACK TINT p_nextchar(TPARSE *p, TINT dummy)
{
	if (p->getnext)
	{
		(*p->readfunc)(p->readdata);
	}
	p->getnext = TTRUE;
	return 0;
}

static TCALLBACK TINT p_countline(TPARSE *p, TINT c)
{
	p->line++;
	return P_OKAY;
}

static TCALLBACK TINT p_endfile(TPARSE *p, TINT c)
{
	if (p->depth > 1)
	{
		p->error = PERR_TAG_STILL_OPEN;
		return P_ERROR;
	}
	return P_OKAY;
}

static TCALLBACK TINT p_error(TPARSE *p, TINT c)
{
	p->error = PERR_PARSE_ERROR;
	return P_ERROR;
}

static TCALLBACK TINT p_opendata(TPARSE *p, TINT c)
{
	TPNODE *node;
	
	if (p->depth == 1)
	{
		p->error = PERR_GARBAGE;		/* data on document root level */
		return P_ERROR;
	}
	
	node = pushnode(p);
	if (node)
	{
		node->data = TAllocString(TNULL);
		node->flags |= PNODEF_DATA;
		return P_OKAY;
	}
	p->error = PERR_OUT_OF_MEMORY;
	return P_ERROR;
}

static TCALLBACK TUINT p_closedata(TPARSE *p, TINT c)
{
	TPNODE *node, *parentnode;
	node = poppnode(p);
	parentnode = currentnode(p);
	if (parentnode)
	{
		TAddTail(&parentnode->list, (TNODE *) node);
		parentnode->numentries++;
	}
	else
	{
		p->error = PERR_GARBAGE;
		return P_ERROR;
	}
	return P_OKAY;
}

static TCALLBACK TUINT p_putdata(TPARSE *p, TINT c)
{
	TPNODE *node = currentnode(p);
	TSetCharString(node->data, -1, c);
	return P_OKAY;
}

static TCALLBACK TUINT p_putdatawhite(TPARSE *p, TINT c)
{
	return p_putdata(p, 32);
}
static TCALLBACK TUINT p_putdatabackslash(TPARSE *p, TINT c)
{
	return p_putdata(p, '\\');
}
static TCALLBACK TUINT p_putdatacr(TPARSE *p, TINT c)
{
	return p_putdata(p, 10);
}
static TCALLBACK TUINT p_putdatatab(TPARSE *p, TINT c)
{
	return p_putdata(p, 9);
}
static TCALLBACK TUINT p_putdatapercent(TPARSE *p, TINT c)
{
	return p_putdata(p, '%');
}

static TCALLBACK TUINT p_opentagname(TPARSE *p, TINT c)
{
	p->tagname = TAllocString(TNULL);
	return P_OKAY;
}

static TCALLBACK TUINT p_closetagname(TPARSE *p, TINT c)
{
	return P_OKAY;
}

static TCALLBACK TUINT p_puttagname(TPARSE *p, TINT c)
{
	TSetCharString(p->tagname, -1, c);
	return P_OKAY;
}

static TCALLBACK TUINT p_openattname(TPARSE *p, TINT c)
{
	p->attname = TAllocString(TNULL);
	return P_OKAY;
}

static TCALLBACK TUINT p_closeattname(TPARSE *p, TINT c)
{
	p->numattr++;

	if (!p->attributes)
	{
		p->attributes = TAlloc(TNULL, sizeof(TUString) * (p->numattr * 2));
	}
	else
	{
		p->attributes = TRealloc(p->attributes, sizeof(TUString) * (p->numattr * 2));
	}
	
	if (p->attributes)
	{
		if (p->numattr > 1)
		{
			TINT i;
			for (i = 0; i < p->numattr - 1; ++i)
			{
				if (TCmpNString(p->attname, p->attributes[i], 0, 0, -1) == 0)
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

static TCALLBACK TUINT p_putattname(TPARSE *p, TINT c)
{
	TSetCharString(p->attname, -1, c);
	return P_OKAY;
}

static TCALLBACK TUINT p_openattvalue(TPARSE *p, TINT c)
{
	p->attvalue = TAllocString(TNULL);
	return P_OKAY;
}

static TCALLBACK TUINT p_closeattvalue(TPARSE *p, TINT c)
{
	p->attributes[p->numattr * 2 - 1] = p->attvalue;
	p->attvalue = -1;
	return P_OKAY;
}

static TCALLBACK TUINT p_putattvalue(TPARSE *p, TINT c)
{
	TSetCharString(p->attvalue, -1, c);
	return P_OKAY;
}

static TCALLBACK TUINT p_openclosetagname(TPARSE *p, TINT c)
{
	p->closingtagname = TAllocString(TNULL);
	return P_OKAY;
}

static TCALLBACK TUINT p_closeclosetagname(TPARSE *p, TINT c)
{
	TPNODE *node = currentnode(p);
	if (node)
	{
		if (TCmpNString(node->data, p->closingtagname, 0, 0, -1) == 0)
		{
			TFreeString(p->closingtagname);
			p->closingtagname = -1;
			return P_OKAY;
		}
	}
	p->error = PERR_UNMATCHED_TAG;
	return P_ERROR;
}

static TCALLBACK TUINT p_putclosetagname(TPARSE *p, TINT c)
{
	TSetCharString(p->closingtagname, -1, c);
	return P_OKAY;
}

static TCALLBACK TUINT p_tagenter(TPARSE *p, TINT c)
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

static TCALLBACK TUINT p_tagleave(TPARSE *p, TINT c)
{
	TPNODE *node = poppnode(p);
	if (node)
	{
		TPNODE *parentnode;

		parentnode = currentnode(p);
		TAddTail(&parentnode->list, (TNODE *) node);
		parentnode->numentries++;

		return P_OKAY;
	}
	
	p->error = PERR_UNMATCHED_TAG;
	return P_ERROR;
}

static TCALLBACK TINT p_closeonly(TPARSE *p, TINT c)
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

		TAddTail(&parentnode->list, (TNODE *) node);
		parentnode->numentries++;

		poppnode(p);

		return P_OKAY;
	}
	p->error = PERR_OUT_OF_MEMORY;
	return P_ERROR;
}

static TCALLBACK TINT p_checkroot(TPARSE *p, TINT c)
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

static TINT c_white[] = {32,9,0};

static TINT c_namefirst[] = {
'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
'_', ':', 0 };

static TINT c_name[] = {
'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
'.', '-', '_', ':', 0 };

static TINT c_opentag[] = {'[',0};
static TINT c_closetag[] = {']',0};
static TINT c_equal[] = {'=',0};
static TINT c_brack[] = {34,0};
static TINT c_slash[] = {47,0};
static TINT c_eof[] = {TEOF, 0};
static TINT c_cr[] = {13, 0};
static TINT c_lf[] = {10, 0};
static TINT c_excl[] = {'!', 0};
static TINT c_dash[] = {'-', 0};
static TINT c_backslash[] = {'\\', 0};
static TINT c_n[] = {'n', 0};
static TINT c_t[] = {'t', 0};
static TINT c_percent[] = {'%', 0};


/*****************************************************************************/
/*
**	ruleset
*/

TAPTR r_data[];
TAPTR r_datawhite[];
TAPTR r_opentag[];
TAPTR r_newclosetag[];
TAPTR r_tagname_open[];
TAPTR r_tagname_close[];
TAPTR r_moretag[];
TAPTR r_attname[];
TAPTR r_attname2[];
TAPTR r_attvalue[];
TAPTR r_attvalue2[];
TAPTR r_comment[];
TAPTR r_comment2[];
TAPTR r_comment4[];
TAPTR r_comment5[];
TAPTR r_comment6[];
TAPTR r_comment8[];
TAPTR r_closeonly[];
TAPTR r_databackslash[];
TAPTR r_databackslash2[];
TAPTR r_databackslashwhite[];

#define	c_else				TNULL
#define	p_done				TNULL


TAPTR r_main[] =
{
	c_white,		r_main,				p_nextchar, p_done,
	c_opentag,		r_opentag,			p_nextchar, p_done,
	c_lf,			r_main,				p_countline, p_nextchar, p_done,
	c_cr,			r_main,				p_nextchar, p_done,
	c_eof,			TNULL,				p_endfile, p_done,
	c_else,			r_data,				p_opendata, p_done
};

TAPTR r_data[] =
{
	c_white,		r_datawhite,		p_nextchar, p_done,
	c_opentag,		r_opentag,			p_closedata, p_nextchar, p_done,
	c_lf,			r_main,				p_closedata, p_countline, p_nextchar, p_done,
	c_cr,			r_datawhite,		p_nextchar, p_done,
	c_eof,			TNULL,				p_closedata, p_endfile, p_done,
	c_backslash,	r_databackslash,	p_nextchar, p_done,
	c_else,			r_data,				p_putdata, p_nextchar, p_done
};

TAPTR r_datawhite[] =
{
	c_white,		r_datawhite,		p_nextchar, p_done,
	c_opentag,		r_opentag,			p_closedata, p_nextchar, p_done,
	c_lf,			r_main,				p_closedata, p_countline, p_nextchar, p_done,
	c_cr,			r_datawhite,		p_nextchar, p_done,
	c_eof,			TNULL,				p_closedata, p_endfile, p_done,
	c_backslash,	r_databackslashwhite,	p_nextchar, p_done,
	c_else,			r_data,				p_putdatawhite, p_done
};


/*	escaped characters	*/

TAPTR r_databackslashwhite[] =
{
	c_backslash,	r_data,				p_putdatawhite,p_putdatabackslash, p_nextchar, p_done,		/* backslash */
	c_n,			r_data,				p_putdatawhite,p_putdatacr,p_nextchar, p_done,				/* cr */
	c_t,			r_data,				p_putdatawhite,p_putdatatab,p_nextchar, p_done,			/* tab */
	c_percent,		r_data,				p_putdatawhite,p_putdatapercent,p_nextchar, p_done,		/* percent sign */
	c_else,			r_databackslash2,	p_done,
};

TAPTR r_databackslash[] =
{
	c_backslash,	r_data,				p_putdatabackslash, p_nextchar, p_done,		/* backslash */
	c_n,			r_data,				p_putdatacr, p_nextchar, p_done,			/* cr */
	c_t,			r_data,				p_putdatatab,p_nextchar, p_done,			/* tab */
	c_percent,		r_data,				p_putdatapercent,p_nextchar, p_done,		/* percent sign */
	c_else,			r_databackslash2,	p_done,
};

TAPTR r_databackslash2[] =
{
	c_white,		r_databackslash2,	p_nextchar, p_done,
	c_cr,			r_databackslash2,	p_nextchar, p_done,
	c_lf,			r_data,				p_countline, p_nextchar, p_done,
	c_eof,			TNULL,				p_closedata, p_endfile, p_done,
	c_else,			TNULL,				p_error, p_done
};



TAPTR r_opentag[] =
{
	c_namefirst,	r_tagname_open,		p_checkroot, p_opentagname, p_puttagname, p_nextchar, p_done,
	c_slash,		r_newclosetag,		p_checkroot, p_nextchar, p_done,
	c_excl,			r_comment,			p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done
};

TAPTR r_newclosetag[] =
{
	c_namefirst,	r_tagname_close,	p_openclosetagname, p_putclosetagname, p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done
};


TAPTR r_tagname_open[] =
{
	c_name,			r_tagname_open,		p_puttagname, p_nextchar, p_done,
	c_white,		r_moretag,			p_closetagname, p_nextchar, p_done,
	c_slash,		r_closeonly,		p_nextchar, p_done,
	c_closetag,		r_main,				p_closetagname, p_tagenter, p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done
};


TAPTR r_moretag[] =
{
	c_white,		r_moretag,			p_nextchar, p_done,
	c_namefirst,	r_attname,			p_openattname, p_putattname, p_nextchar, p_done,
	c_closetag,		r_main,				p_tagenter, p_nextchar, p_done,
	c_slash,		r_closeonly,		p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done
};

TAPTR r_closeonly[] =
{
	c_closetag,		r_main,				p_closeonly, p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done
};

TAPTR r_attname[] =
{
	c_name,			r_attname,			p_putattname, p_nextchar, p_done,
	c_equal,		r_attvalue,			p_closeattname, p_nextchar, p_done,
	c_white,		r_attname2,			p_closeattname, p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done
};

TAPTR r_attname2[] =
{
	c_white,		r_attname2,			p_nextchar, p_done,
	c_equal,		r_attvalue,			p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done
};

TAPTR r_attvalue[] =
{
	c_brack,		r_attvalue2,		p_openattvalue, p_nextchar, p_done,
	c_white,		r_attvalue,			p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done
};

TAPTR r_attvalue2[] =
{
	c_brack,		r_moretag,			p_closeattvalue, p_nextchar, p_done,
	c_eof,			TNULL,				p_error, p_done,
	c_lf,			TNULL,				p_error, p_done,
	c_else,			r_attvalue2,		p_putattvalue, p_nextchar, p_done,
};


TAPTR r_tagname_close[] =
{
	c_name,			r_tagname_close,	p_putclosetagname, p_nextchar, p_done,
	c_closetag,		r_main,				p_closeclosetagname, p_tagleave, p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done
};

TAPTR r_comment[] =
{
	c_dash,			r_comment2,			p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done	
};
TAPTR r_comment2[] =
{
	c_dash,			r_comment4,			p_nextchar, p_done,
	c_else,			TNULL,				p_error, p_done	
};
TAPTR r_comment4[] =
{
	c_dash,			r_comment4,			p_nextchar, p_done,
	c_closetag,		TNULL,				p_error, p_done,
	c_else,			r_comment5,			p_done	
};
TAPTR r_comment5[] =
{
	c_dash,			r_comment6,			p_nextchar, p_done,
	c_lf,			r_comment5,			p_countline, p_nextchar, p_done,
	c_eof,			TNULL,				p_error, p_done,
	c_else,			r_comment5,			p_nextchar, p_done	
};
TAPTR r_comment6[] =
{
	c_dash,			r_comment8,			p_nextchar, p_done,
	c_else,			r_comment5,			p_done
};
TAPTR r_comment8[] =
{
	c_dash,			r_comment8,			p_nextchar, p_done,
	c_closetag,		r_main,				p_nextchar, p_done,
	c_else,			r_comment5,			p_done	
};

