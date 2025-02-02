
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/inline/io.h>
#include <tek/inline/util.h>
#include <tek/inline/unistring.h>

#include "global.h"
#include "parse_tmkmf.h"

/*****************************************************************************/

static TSTRPTR ToCString(TUString s) {
	TINT len = TLengthString(s);
	if (TGetCharString(s, len - 1) != 0)
		TSetCharString(s, len++, 0);
	return TMapString(s, 0, len, TASIZE_8BIT);
}

static TBOOL pushstack(TPARSE *p) {
	TPNODE **nn;

	nn = TRealloc(p->nodestack, sizeof(TPNODE *) * (p->depth + 1));
	if (!nn) return TFALSE;
	p->nodestack = nn;

	nn = TRealloc(p->currentnode, sizeof(TPNODE *) * (p->depth + 1));
	if (!nn) return TFALSE;
	p->currentnode = nn;

	return TTRUE;
}

/*****************************************************************************/
/*
**	goto root node
*/

TBOOL dom_root(TPARSE *p) {
	p->depth = 1;
	p->currentnode[0] = TNULL;
	return TTRUE;
}

/*****************************************************************************/
/*
**	go to first node on current level
*/

TBOOL dom_rewind(TPARSE *p) {
	p->currentnode[p->depth - 1] = TNULL;
	return TTRUE;
}

/*****************************************************************************/
/*
**	seek to next node.
*/

TPNODE *dom_next(TPARSE *p) {
	TPNODE *n = p->currentnode[p->depth - 1];

	if (n) {
		n = (TPNODE *) n->node.tln_Succ;
	} else {
		n = (TPNODE *) p->nodestack[p->depth - 1]->list.tlh_Head;
	}

	p->currentnode[p->depth - 1] = n;

	if (n) {
		if (n->node.tln_Succ) return n;
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	seek to next named node.
*/

TPNODE *dom_nextnode(TPARSE *p, TSTRPTR tagname) {
	TPNODE *n = p->currentnode[p->depth - 1];
	TPNODE *nn;

	if (n) {
		n = (TPNODE *) n->node.tln_Succ;
	} else {
		n = (TPNODE *) p->nodestack[p->depth - 1]->list.tlh_Head;
	}

	p->currentnode[p->depth - 1] = n;
	
	if (n) {
		while ((nn = (TPNODE *) n->node.tln_Succ)) {
			if (n->flags & PNODEF_NAME) {
				if (tagname) {
					TUString tstr = TAllocString(tagname);
					TINT res = TCmpNString(n->data, tstr, 0, 0, -1);
					TFreeString(tstr);
					if (res == 0) {
						p->currentnode[p->depth - 1] = n;
						return n;
					}
				} else {
					p->currentnode[p->depth - 1] = n;
					return n;
				}
			}
			n = nn;
		}		
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	enter node
*/

TBOOL dom_gosub(TPARSE *p) {
	TPNODE *n = p->currentnode[p->depth - 1];
	
	if (!n) return TFALSE;
	if (TListEmpty(&n->list)) return TFALSE;
	if (!pushstack(p)) return TFALSE;
	
	p->nodestack[p->depth] = n;
	p->currentnode[p->depth] = TNULL;
	p->depth++;

	return TTRUE;
}

/*****************************************************************************/
/*
**	return to previous stackframe
*/

TBOOL dom_return(TPARSE *p) {
	if (p->depth <= 1) return TFALSE;	
	p->depth--;
	return TTRUE;
}

/*****************************************************************************/
/*
**	seek to next data
*/

TUString dom_nextdata(TPARSE *p) {
	TPNODE *n = p->currentnode[p->depth - 1];
	TPNODE *nn;

	if (n) {
		n = (TPNODE *) n->node.tln_Succ;
	} else {
		n = (TPNODE *) p->nodestack[p->depth - 1]->list.tlh_Head;
	}

	p->currentnode[p->depth - 1] = n;
	
	if (n) {
		while ((nn = (TPNODE *) n->node.tln_Succ)) {
			if (n->flags & PNODEF_DATA) {
				p->currentnode[p->depth - 1] = n;
				return n->data;
			}
			n = nn;
		}		
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	gosub rootnode
*/

TBOOL dom_gosubroot(TPARSE *p) {
	TPNODE *n = p->nodestack[0];
	
	if (TListEmpty(&n->list)) return TFALSE;
	if (!pushstack(p)) return TFALSE;

	p->nodestack[p->depth] = n;
	p->currentnode[p->depth] = TNULL;
	p->depth++;

	return TTRUE;
}

/*****************************************************************************/
/*
**	get attribute
*/

TSTRPTR dom_getattr(TPARSE *p, TSTRPTR attname, TSTRPTR defstr) {
	TUString astr;
	TINT i;
	TPNODE *n = p->currentnode[p->depth - 1];
	if (!(n->flags & PNODEF_NAME)) return TNULL;
	i = n->numattr;
	astr = TAllocString(attname);
	while (--i >= 0) {
		if (TCmpNString(n->attributes[i * 2], astr, 0, 0, -1) == 0) {
			TFreeString(astr);
			return ToCString(n->attributes[i * 2 + 1]);
		}
	}
	TFreeString(astr);
	return defstr;
}

/*****************************************************************************/
/*
**	get data from node
*/

TSTRPTR dom_getdata(TPARSE *p) {
	TPNODE *n = p->currentnode[p->depth - 1];
	if (n->flags & PNODEF_DATA) {
		return ToCString(n->data);
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	get name from node
*/

TSTRPTR dom_getnode(TPARSE *p) {
	TPNODE *n = p->currentnode[p->depth - 1];
	if (n->flags & PNODEF_NAME) {
		return ToCString(n->data);
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	get node's linenumer
*/

TINT dom_getline(TPARSE *p) {
	return p->currentnode[p->depth - 1]->line + 1;
}
