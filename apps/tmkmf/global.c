
#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/proto/hash.h>
#include <tek/inline/io.h>
#include <tek/inline/unistring.h>

#include "global.h"

/*****************************************************************************/
/*
**	management of body nodes
*/

typedef struct {
	THNDL handle;
	TLIST list;
} bodynode;

static TCALLBACK TVOID destroybody(bodynode *bn) {
	TNODE *nn, *n = bn->list.tlh_Head;
	while ((nn = n->tln_Succ)) {
		TFree(((THNDL *)n)->thn_Data);
		TFree(n);						
		n = nn;
	}
	TFree(bn->handle.thn_Data);
	TFree(bn);
}

static bodynode *newbody(TSTRPTR name) {
	bodynode *bn = TAlloc(MMU, sizeof(bodynode));
	if (bn) {
		bn->handle.thn_Data = TStrDup(MMU, name);
		if (bn->handle.thn_Data) {
			bn->handle.thn_DestroyFunc = (TDFUNC) destroybody;
			TInitList(&bn->list);
			return bn;
		}
		TFree(bn);
	}
	return TNULL;
}

static TBOOL addbodyline(bodynode *bn, TSTRPTR string) {
	THNDL *h = TAlloc(MMU, sizeof(THNDL));
	if (h) {
		h->thn_Data = TStrDup(MMU, string);
		if (h->thn_Data) {
			TAddTail(&bn->list, (TNODE *) h);
			return TTRUE;
		}
		TFree(h);
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	context management
*/

struct readdata {
	TAPTR io;
	TAPTR file;
};

static TCALLBACK TINT readfunc(TAPTR data) {
	return TFGetC(((struct readdata *) data)->file);
}

typedef struct {
	THNDL handle;
	TLIST contextlist;	/* stack of contexts, organized as a list */
	TLIST bodylist;		/* all bodys */
	TUINT flags;
	TSTRPTR pathname;
	TSTRPTR filename;
	TPARSE *dom;
	TAPTR hash;
	bodynode *currentbody;
	TINT line;
	TSTRPTR errtext;
	TAPTR outfile;
} context;

#define CFL_NONE			0x0000
#define CFL_ISROOT			0x0001
#define CFL_PARSE_ERROR		0x0002

static context *rootcontext(context *this) {
	context *root = TNULL;
	if (this) {
		TNODE *pn;
		while ((pn = ((TNODE *) this)->tln_Pred)) {
			root = this;
			this = (context *) pn;
		}
	}
	return root;
}

static context *topcontext(context *this) {
	context *top = TNULL;
	if (this) {
		TNODE *nn;
		while ((nn = ((TNODE *) this)->tln_Succ)) {
			top = this;
			this = (context *) nn;
		}
	}
	return top;
}

static context *popcontext(context *this) {
	context *top = topcontext(this);
	if (top) TRemove((TNODE *) top);
	return top;
}

static TVOID TCALLBACK destroycontext(context *this) {
	if (this->flags & CFL_ISROOT) {
		context *t;
		TCloseModule(this->hash);
		TDestroyList(&this->bodylist);
		for (;;) {
			t = popcontext(this);
			if (t == this || !t) break;
			TDestroy(t);
		}
	}
	TCloseFile(this->outfile);
	TDestroy(this->dom);
	TFree(this->pathname);
	TFree(this->filename);
	TFree(this);
}

static context *pushcontext(context *parent, TSTRPTR fname) {
	context *root = rootcontext(parent);
	context *top = topcontext(parent);
	context *this;

	this = TAlloc0(MMU, sizeof(context));
	if (this) {
		TSTRPTR fullname = TNULL;
		TBOOL success = TFALSE;

		this->handle.thn_DestroyFunc = (TDFUNC) destroycontext;
				
		if (top) {
			if (TStrChr(fname, ':')) {
				/* absolute path */
				fullname = TStrDup(MMU, fname);
				if (fullname) success = TTRUE;
			}
			else {
				/* fname is relative to the topmost element on the stack. */
				TINT len = TStrLen(top->pathname);
				len += TStrLen(fname) + 2;
				fullname = TAlloc(MMU, len);
				if (fullname) {
					TStrCpy(fullname, top->pathname);
					TStrCat(fullname, "/");
					TStrCat(fullname, fname);
					fname = fullname;
					success = TTRUE;
				}
			}
			if (success) this->hash = root->hash;
		} else {
			this->hash = TOpenModule("hash", 0, TNULL);
			if (this->hash)  {
				this->flags |= CFL_ISROOT;
				TInitList(&this->contextlist);
				TInitList(&this->bodylist);
				root = this;
				success = TTRUE;
			}
		}

		if (success) {
			TAPTR lock, parentlock;

			success = TFALSE;
			lock = TLockFile(fname, TFLOCK_READ, TNULL);
			parentlock = TParentDir(lock);
			
			if (lock && parentlock) {
				TINT len;
				
				len = TNameOf(parentlock, TNULL, 0);
				this->pathname = TAlloc(TNULL, len + 1);
				TNameOf(parentlock, this->pathname, len + 1);

				len = TNameOf(lock, TNULL, 0);
				this->filename = TAlloc(TNULL, len + 1);
				TNameOf(lock, this->filename, len + 1);
				
				if (this->pathname && this->filename) {
					struct readdata readdata;
					readdata.file = TOpenFromLock(lock);
					if (readdata.file) {
						TINT errnum, errline;
						this->dom = parse_new(&readdata, 
							readfunc, &errnum, &errline);
						if (!this->dom) {
							this->line = errline;
							this->errtext = "syntax error";
							this->flags |= CFL_PARSE_ERROR;
						}

						TCloseFile(readdata.file);
						/* the lock is being absorbed by the file handle */
						lock = TNULL;		
						success = TTRUE;
					}
				}
			}

			TUnlockFile(parentlock);
			TUnlockFile(lock);
		}
		TFree(fullname);

		if (success) {
			if (!TFindHandle(&root->contextlist, this->filename)) {
				this->handle.thn_Data = this->filename;
				TAddTail(&root->contextlist, (TNODE *) this);
				return this;
			}
			tdbprintf(5,"cyclic dependency detected\n");
		}

		TDestroy(this);
	}
	
	return TNULL;
}

/*****************************************************************************/
/*
**	render
*/

static TBOOL newoutfile(context *c, TSTRPTR fname, TSTRPTR buildcontext) {
	if (fname) {
		TSTRPTR fullname;
		TINT l;
		
		l = TStrLen(fname);
		l += TStrLen(c->pathname);
		l += TStrLen(buildcontext);
		l += 2;

		fullname = TAlloc(MMU, l);
		if (fullname) {
			TStrCpy(fullname, c->pathname);
			TStrCat(fullname, "/");
			TStrCat(fullname, fname);
			TStrCat(fullname, buildcontext);

			c->outfile = TOpenFile(fullname, TFMODE_NEWFILE, TNULL);
			
			TFree(fullname);
			
			if (c->outfile)
				return TTRUE;
		}
		return TFALSE;
	}
	return TTRUE;
}

static TVOID closeoutfile(context *c) {
	TCloseFile(c->outfile);
	c->outfile = TNULL;
}

static TVOID renderbody(context *c, bodynode *bn) {
	TNODE *nn, *n = bn->list.tlh_Head;
	while ((nn = n->tln_Succ)) {
		TSTRPTR line = ((THNDL *)n)->thn_Data;
		if (c->outfile) {
			TINT l = TStrLen(line);
			TFWrite(c->outfile, line, l);
			TFPutC(c->outfile, 10);			
		} else {
			printf("%s\n", (TSTRPTR) line);
		}
		n = nn;
	}
}

/*****************************************************************************/
/*
**	in string s1, replace string s2 with string s3; return a new string
*/

static TUString stringreplacestr(TUString s1, TUString s2, TUString s3) {
	TUString news;
	TINT pos = TFindString(s1, s2, 0, -1);
	if (pos == -1) {
		news = TDupString(s1, 0, -1);
	} else {
		news = TDupString(s1, 0, pos);
		//TInsertStrNString(news, -1, s3, TStrLen(s3), TASIZE_8BIT);
		TInsertString(news, -1, s3, 0, -1);
		TInsertString(news, -1, s1, pos + TLengthString(s2), -1);
	}
	return news;
}

/*****************************************************************************/
/*
**	success = substitute(context, line, dynstrings, buildcontext)
**	substitute %0, %1, %2, ... arguments in a line
*/

static TUString substitute(context *this, TSTRPTR p, TSTRPTR buildcontext) {
	TUString outstring = TAllocString(TNULL);
	if (dom_gosub(this->dom)) {
		TINT c, i, num = -1;
	
		for(;;) {
			c = *p;
	
			if (c >= '0' && c <= '9') {
				if (num >= 0) {
					num = num * 10 + (c - '0');
				} else {
					TSetCharString(outstring, -1, c);
				}
			} else {
				if (num == 0) {
					if (c == 'n') {
						TSetCharString(outstring, -1, '%');
					} else {
						TInsertStrNString(outstring, -1, buildcontext, 
							TStrLen(buildcontext), TASIZE_8BIT);
					}
				}
				else if (num > 0) {
					TUString arg;
					dom_rewind(this->dom);
					i = 0;
					while ((arg = dom_nextdata(this->dom))) {
						i++;
						if (i == num) {
							i = 0;
							TInsertString(outstring, -1, arg, 0, -1);
							break;
						}
					}
				}
	
				if (c == '%') {
					num = 0;
				} else {
					num = -1;
					if (c) {
						TSetCharString(outstring, -1, c);
					} else {
						break;
					}
				}
			}
			p++;
		}
		dom_return(this->dom);
	} else {
		/* no arguments: add entire line */
		TInsertStrNString(outstring, -1, p, TStrLen(p), TASIZE_8BIT);
	}
	
	return outstring;
}

/*****************************************************************************/
/*
**	substituten(context, line)
**	perform %n argument substitutions to line
*/

static TUString substituten(context *this, TUString line) {
	//TSTRPTR arg;
	TUString outstr = line;
	TUString pat = TAllocString("%n");
	
	if (TFindString(line, pat, 0, -1) >= 0) {
		if (dom_gosub(this->dom)) {
			TUString arg;
			outstr = TAllocString(TNULL);
			while ((arg = dom_nextdata(this->dom))) {
				TUString newl = stringreplacestr(line, pat, arg);
			    TInsertString(outstr, -1, newl, 0, -1);
				TSetCharString(outstr, -1, 32);
				TFreeString(newl);
			}
			TFreeString(line);
			dom_return(this->dom);
		}
	}

	TFreeString(pat);
	return outstr;
}

/*****************************************************************************/
/*
**	success = embed(g, context, refbody, buildcontext)
**	process [embed] tag
*/

static TBOOL embed(context *this, bodynode *refbody, TSTRPTR buildcontext) {
	TSTRPTR errtext = "out of memory";
	TBOOL success = TTRUE;
	TNODE *nn, *n = refbody->list.tlh_Head;
	TUString outstr;

	while (success && (nn = n->tln_Succ)) {
		/* substitute %0, %1, ... */
		outstr = substitute(this, ((THNDL *) n)->thn_Data, buildcontext);
		if (TLengthString(outstr) >= 0) {
			outstr = substituten(this, outstr);
			if (TLengthString(outstr) >= 0) {
				TSTRPTR s;
				TSetCharString(outstr, -1, 0);
				s = TMapString(outstr, 0, -1, TASIZE_8BIT);
				if (s) success = addbodyline(this->currentbody, s);
				else success = TFALSE;
			} 
			else success = TFALSE;
			TFreeString(outstr);
		}
		else success = TFALSE;
		n = nn;
	}

	if (!success) {
		this->errtext = errtext;
	}

	return success;
}

/*****************************************************************************/
/*
**	success = process(context, buildcontext, state)
*/

#define STATE_NONE		0x0000
#define STATE_SWITCH	0x0001
#define STATE_BODY		0x0100
#define STATE_RENDER	0x0200

static TBOOL process(context *this, TSTRPTR buildcontext, TUINT state) {
	TBOOL success = TTRUE;
	TBOOL breakout = TFALSE;
	TSTRPTR data;
	TSTRPTR attr; 
	TSTRPTR newbodyname;
	TINT attlen;
	TBOOL descent;
	TUINT newstate;
	TINT resetcontextlen;

	while (success && !breakout && dom_next(this->dom)) {
		this->line = dom_getline(this->dom);
		data = dom_getdata(this->dom);
		if (data) {
			switch (state & 0xff00) {
				case STATE_BODY:
					if (!addbodyline(this->currentbody, data)) {
						this->errtext = "out of memory";
						goto error;
					}
					break;

				case STATE_RENDER: {
					bodynode *bn;
					if (THashGet(this->hash, (TTAG) data, &bn)) {
						renderbody(this, bn);
					} else {
						this->errtext = "body not found for rendering";
						goto error;
					}
					break;
				}

				default:
					this->errtext =
						"data found outside [body] or [render] tag";
					goto error;
			}
			continue;
		}

		/* process a tag */
		
		data = dom_getnode(this->dom);

		descent = TTRUE;
		newstate = state & 0xff00;
		newbodyname = TNULL;
		resetcontextlen = 0;
		
		switch (state & 0xff00) {
			case STATE_BODY: 
				if (TStrCmp(data, "body") == 0) {
					this->errtext = "[body] in [body] tag detected";
					goto error;
				}
				if (TStrCmp(data, "include") == 0) {
					this->errtext = "[include] in [body] tag detected";
					goto error;
				}
				if (TStrCmp(data, "embed") == 0) {
					descent = TFALSE;
					attr = dom_getattr(this->dom, "body", TNULL);
					if (attr) {
						bodynode *refbody;
						if (THashGet(this->hash, (TTAG) attr, &refbody)) {
							if (!embed(this, refbody, buildcontext))
								goto error;
						}
					} else {
						this->errtext = "[embed] requires a body attribute";
						goto error;
					}
				}
		}

		switch (state & 0x00ff) {
			default:
				this->errtext = "invalid state entered";
				goto error;

			case STATE_NONE:
				if (TStrCmp(data, "body") == 0) {
					newbodyname = dom_getattr(this->dom, "name", TNULL);
					if (newbodyname) {
						bodynode *oldbody;
						if (THashGet(this->hash,
							(TTAG) newbodyname, &oldbody)) {
							/* overwrite body */
							THashRemove(this->hash, (TTAG) newbodyname);
							TRemove((TNODE *) oldbody);
							TDestroy(oldbody);
						}

						this->currentbody = newbody(newbodyname);
						if (this->currentbody) {
							newstate |= STATE_BODY;
						} else {
							this->errtext = "out of memory";
							goto error;
						}
					} else {
						this->errtext = "[body] requires a name attribute";
						goto error;
					}
				}
				else if (TStrCmp(data, "switch") == 0) {
					newstate |= STATE_SWITCH;
				}
				else if (TStrCmp(data, "if") == 0) {
					attr = dom_getattr(this->dom, "config", TNULL);
					if (attr) {
						attlen = TStrLen(attr);
						if (!TStrNCmp(attr, buildcontext, attlen)) {
							buildcontext += attlen;
							resetcontextlen = attlen;
						} else {
							descent = TFALSE;
						}
					} else {
						this->errtext = "[if] requires a config attribute";
						goto error;
					}
				}
				else if (TStrCmp(data, "render") == 0) {
					attr = dom_getattr(this->dom, "to", TNULL);
					if (newoutfile(this, attr, buildcontext)) {
						newstate |= STATE_RENDER;
					} else {
						this->errtext = "could not open output file";
						goto error;
					}
				} else if (TStrCmp(data, "include") == 0) {
					attr = dom_getattr(this->dom, "name", TNULL);
					if (attr) {
						context *subc = pushcontext(this, attr);
						if (subc) {
							TBOOL success;

							if (subc->flags & CFL_PARSE_ERROR) {
								success = TFALSE;
							} else {
								success = 
									process(subc, buildcontext, STATE_NONE);
							}
							
							if (!success) {
								return TFALSE;
							}

							popcontext(subc);
							TDestroy(subc);
						} else {
							this->errtext = "include file not found, out of memory, or cyclic dependency detected";
							goto error;
						}

						descent = TFALSE;
					} else {
						this->errtext = "[include] requires a name attribute";
						goto error;
					}
				}
				break;
					
			case STATE_SWITCH:
				if (TStrCmp(data, "case") == 0) {
					attr = dom_getattr(this->dom, "config", TNULL);
					if (attr) {
						attlen = TStrLen(attr);
						if (!TStrNCmp(attr, buildcontext, attlen)) {
							buildcontext += attlen;
							/* descent this node, then break out */
							breakout = TTRUE;	
						} else {
							descent = TFALSE;
						}
					} else {
						this->errtext = "[case] requires a config attribute";
						goto error;
					}
				} else if (TStrCmp(data, "case_no_descend") == 0) {
					attr = dom_getattr(this->dom, "config", TNULL);
					if (attr) {
						attlen = TStrLen(attr);
						if (!TStrNCmp(attr, buildcontext, attlen)) {
							breakout = TTRUE;
						} else {
							descent = TFALSE;
						}
					} else {
						this->errtext = "[case_no_descend] requires a config attribute";
						goto error;
					}
				}	
				else if (TStrCmp(data, "default") == 0) {
					/* descent this node, then break out */
					breakout = TTRUE;
				} else {
					this->errtext = "a switch case requires a [case] or [default] tag";
					goto error;
				}
				break;
		}
		
		if (descent) {
			if (dom_gosub(this->dom)) {
				success = process(this, buildcontext, newstate);
				dom_return(this->dom);
			}
			
			if (resetcontextlen) {
				buildcontext -= resetcontextlen;
			}
			
			if (newstate & STATE_RENDER) {
				closeoutfile(this);
			}
		
			if (newbodyname && success) {
				THashPut(this->hash, (TTAG) this->currentbody->handle.thn_Data,
					(TTAG) this->currentbody);
				TAddTail(&(rootcontext(this)->bodylist),
					(TNODE *) this->currentbody);
				this->currentbody = TNULL;
			}
		}
	}

	return success;

error:

	if (this->currentbody) {
		TDestroy(this->currentbody);
		this->currentbody = TNULL;
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	printerror(anycontext)
*/

static TVOID printerror(context *this) {
	context *errnode = TNULL;
	context *root = rootcontext(this);
	TNODE *nn, *n = root->contextlist.tlh_Head;
	while ((nn = n->tln_Succ)) {
		printf("*** in %s, line %d:\n",
			((context *) n)->filename, ((context *) n)->line);
		errnode = (context *) n;
		n = nn;
	}
	if (errnode) {
		printf("*** %s\n", errnode->errtext);
	}
}

/*****************************************************************************/
/*
**	main
*/

TBOOL docontext(TSTRPTR fname, TSTRPTR buildcontext) {
	TBOOL success = TFALSE;
	context *c = pushcontext(TNULL, fname);
	if (c) {
		if (!(c->flags & CFL_PARSE_ERROR)) {
			success = TTRUE;
		}

		if (success) {	
			success = process(c, buildcontext, STATE_NONE);
		}
		
		if (!success) {
			printerror(c);
		}
		
		TDestroy(c);
	}
	return success;
}
