
#ifndef _PARSE_TMKMF_H
#define _PARSE_TMKMF_H

#include <tek/exec.h>

typedef struct {
	struct TNode node;	
	TINT data;			/* data given: node contains only data and no childs, no name */
	struct TList list;	/* childs */
	TINT *attributes;	/* attributes: array of dynamic strings (name,value pairs each) */
	TUINT flags;
	TINT numattr;		/* number of attributes */
	TUINT numentries;	/* number of childs */
	TINT line;			/* this node's line number */
} TPNODE;

#define PNODEF_NONE		0
#define PNODEF_DATA		1
#define PNODEF_NAME		2

typedef struct {
	struct THandle handle;

	/* parser */
	TAPTR readdata;
	TINT (*readfunc)(TAPTR readdata);
	TINT error;
	TINT line;
	TBOOL getnext;
	TINT c;

	/* string collectors */
	TINT tagname;
	TINT attname;
	TINT attvalue;
	TINT closingtagname;
	TINT *attributes;

	TINT numattr;
	TUINT depth;
	TPNODE **nodestack;

	/* navigation: stack of current node in current frame */
	TPNODE **currentnode;
} TPARSE;


#define PERR_OKAY						0
#define PERR_TAG_STILL_OPEN				1
#define PERR_PARSE_ERROR				2
#define PERR_GARBAGE					3
#define PERR_DATA_ALREADY_OPEN			4
#define PERR_OUT_OF_MEMORY				5
#define PERR_NO_DATA_OPEN				6
#define PERR_NO_TAGNAME_OPEN			7
#define PERR_ATTR_ALREADY_OPEN			8
#define PERR_NO_ATTR_OPEN				9
#define PERR_ATTVAL_ALREADY_OPEN		10
#define PERR_NO_ATTVAL_OPEN				11
#define PERR_UNMATCHED_TAG				12
#define PERR_NO_DATA					13
#define PERR_MULTIPLE_ROOT				14
#define PERR_DUPLICATE_ATTRIBUTE		15

extern TPARSE *parse_new(TAPTR readdata, TINT(*readfunc)(TAPTR), TINT *errnum, TINT *errline);

extern TBOOL dom_root(TPARSE *dom);
extern TBOOL dom_enter(TPARSE *dom);
extern TPNODE *dom_nextnode(TPARSE *dom, TSTRPTR tagname);
extern TSTRPTR dom_getattr(TPARSE *dom, TSTRPTR attrname, TSTRPTR defstr);
extern TINT dom_nextdata(TPARSE *dom);
extern TBOOL dom_gosub(TPARSE *dom);
extern TBOOL dom_return(TPARSE *dom);
extern TBOOL dom_rewind(TPARSE *dom);
extern TBOOL dom_gosubroot(TPARSE *dom);

extern TPNODE *dom_next(TPARSE *p);
extern TSTRPTR dom_getdata(TPARSE *p);
extern TSTRPTR dom_getnode(TPARSE *p);
extern TINT dom_getline(TPARSE *p);

#endif

