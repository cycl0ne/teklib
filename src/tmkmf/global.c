
#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>

#include "global.h"

/*****************************************************************************/
/*
**	management of body nodes
*/

typedef struct {
	struct THandle handle;
	struct TList list;
} bodynode;

static THOOKENTRY TTAG
destroybody(struct THook *hook, TAPTR obj, TTAG msg) {
	bodynode *bn = obj;
	struct TNode *nn, *n = bn->list.tlh_Head;
	while ((nn = n->tln_Succ)) {
		TFree(((struct THandle *)n)->thn_Name);
		TFree(n);
		n = nn;
	}
	TFree(bn->handle.thn_Name);
	TFree(bn);
	return 0;
}

static bodynode *newbody(TSTRPTR name) {
	bodynode *bn = TAlloc(MMU, sizeof(bodynode));
	if (bn) {
		bn->handle.thn_Name = TStrDup(MMU, name);
		if (bn->handle.thn_Name) {
			bn->handle.thn_Hook.thk_Entry = destroybody;
			TInitList(&bn->list);
			return bn;
		}
		TFree(bn);
	}
	return TNULL;
}

static TBOOL addbodyline(bodynode *bn, TSTRPTR string) {
	struct THandle *h = TAlloc(MMU, sizeof(struct THandle));
	if (h) {
		h->thn_Name = TStrDup(MMU, string);
		if (h->thn_Name) {
			TAddTail(&bn->list, (struct TNode *) h);
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

static TINT readfunc(TAPTR data) {
	return TFGetC(((struct readdata *) data)->file);
}

typedef struct {
	struct THandle handle;
	/* stack of contexts, organized as a list */
	struct TList contextlist;
	/* all bodys */
	struct TList bodylist;
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
		struct TNode *pn;
		while ((pn = ((struct TNode *) this)->tln_Pred)) {
			root = this;
			this = (context *) pn;
		}
	}
	return root;
}

static context *topcontext(context *this) {
	context *top = TNULL;
	if (this) {
		struct TNode *nn;
		while ((nn = ((struct TNode *) this)->tln_Succ)) {
			top = this;
			this = (context *) nn;
		}
	}
	return top;
}

static context *popcontext(context *this) {
	context *top = topcontext(this);
	if (top) TRemove((struct TNode *) top);
	return top;
}

static TTAG THOOKENTRY
destroycontext(struct THook *hook, TAPTR obj, TTAG msg) {
	context *ctx = obj;
	if (ctx->flags & CFL_ISROOT) {
		context *t;
		TDestroy(ctx->hash);
		TDestroyList(&ctx->bodylist);
		for (;;) {
			t = popcontext(ctx);
			if (t == ctx || !t) break;
			TDestroy(&t->handle);
		}
	}
	TCloseFile(ctx->outfile);
	TDestroy(&ctx->dom->handle);
	TFree(ctx->pathname);
	TFree(ctx->filename);
	TFree(ctx);
	return 0;
}

static context *pushcontext(context *parent, TSTRPTR fname) {
	context *root = rootcontext(parent);
	context *top = topcontext(parent);
	context *this;

	this = TAlloc0(MMU, sizeof(context));
	if (this) {
		TSTRPTR fullname = TNULL;
		TBOOL success = TFALSE;

		this->handle.thn_Hook.thk_Entry = destroycontext;

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
			this->hash = TCreateHash(TNULL);
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
				this->handle.thn_Name = this->filename;
				TAddTail(&root->contextlist, (struct TNode *) this);
				return this;
			}
			TDBPRINTF(5, ("cyclic dependency detected\n"));
		}

		TDestroy(&this->handle);
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

static void closeoutfile(context *c) {
	TCloseFile(c->outfile);
	c->outfile = TNULL;
}

static void renderbody(context *c, bodynode *bn) {
	struct TNode *nn, *n = bn->list.tlh_Head;
	while ((nn = n->tln_Succ)) {
		TSTRPTR line = ((struct THandle *)n)->thn_Name;
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

static TINT stringreplacestr(TINT s1, TINT s2, TINT s3) {
	TINT news;
	TINT pos = dFindString(DSTR, s1, s2, 0, -1);
	if (pos == -1) {
		news = dDupString(DSTR, s1, 0, -1);
	} else {
		news = dDupString(DSTR, s1, 0, pos);
		dInsertString(DSTR, news, -1, s3, 0, -1);
		dInsertString(DSTR, news, -1, s1, pos + dLengthString(DSTR, s2), -1);
	}
	return news;
}

/*****************************************************************************/
/*
**	success = substitute(context, line, dynstrings, buildcontext)
**	substitute %0, %1, %2, ... arguments in a line
*/

static TINT substitute(context *this, TSTRPTR p, TSTRPTR buildcontext) {
	TINT outstring = dAllocString(DSTR, TNULL);
	if (dom_gosub(this->dom)) {
		TINT c, i, num = -1;

		for(;;) {
			c = *p;

			if (c >= '0' && c <= '9') {
				if (num >= 0) {
					num = num * 10 + (c - '0');
				} else {
					dSetCharString(DSTR, outstring, -1, c);
				}
			} else {
				if (num == 0) {
					if (c == 'n') {
						dSetCharString(DSTR, outstring, -1, '%');
					} else {
						dInsertStrNString(DSTR, outstring, -1, buildcontext,
							TStrLen(buildcontext));
					}
				}
				else if (num > 0) {
					TINT arg;
					dom_rewind(this->dom);
					i = 0;
					while ((arg = dom_nextdata(this->dom))) {
						i++;
						if (i == num) {
							i = 0;
							dInsertString(DSTR, outstring, -1, arg, 0, -1);
							break;
						}
					}
				}

				if (c == '%') {
					num = 0;
				} else {
					num = -1;
					if (c) {
						dSetCharString(DSTR, outstring, -1, c);
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
		dInsertStrNString(DSTR, outstring, -1, p, TStrLen(p));
	}

	return outstring;
}

/*****************************************************************************/
/*
**	substituten(context, line)
**	perform %n argument substitutions to line
*/

static TINT substituten(context *this, TINT line) {
	/*TSTRPTR arg;*/
	TINT outstr = line;
	TINT pat = dAllocString(DSTR, "%n");

	if (dFindString(DSTR, line, pat, 0, -1) >= 0) {
		if (dom_gosub(this->dom)) {
			TINT arg;
			outstr = dAllocString(DSTR, TNULL);
			while ((arg = dom_nextdata(this->dom))) {
				TINT newl;
				newl = stringreplacestr(line, pat, arg);
			    dInsertString(DSTR, outstr, -1, newl, 0, -1);
				dSetCharString(DSTR, outstr, -1, 32);
				dFreeString(DSTR, newl);
			}
			dFreeString(DSTR, line);
			dom_return(this->dom);
		}
	}

	dFreeString(DSTR, pat);
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
	struct TNode *nn, *n = refbody->list.tlh_Head;
	TINT outstr;

	while (success && (nn = n->tln_Succ)) {
		/* substitute %0, %1, ... */
		outstr = substitute(this, ((struct THandle *) n)->thn_Name, buildcontext);
		if (dLengthString(DSTR, outstr) >= 0) {
			outstr = substituten(this, outstr);
			if (dLengthString(DSTR, outstr) >= 0) {
				TSTRPTR s;
				dSetCharString(DSTR, outstr, -1, 0);
				s = dMapString(DSTR, outstr, 0, -1);
				if (s) success = addbodyline(this->currentbody, s);
				else success = TFALSE;
			}
			else success = TFALSE;
			dFreeString(DSTR, outstr);
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
					if (TGetHash(this->hash, (TTAG) data, (TAPTR) &bn)) {
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
						if (TGetHash(this->hash, (TTAG) attr, (TAPTR) &refbody)) {
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
						if (TGetHash(this->hash,
							(TTAG) newbodyname, (TAPTR) &oldbody)) {
							/* overwrite body */
							TRemHash(this->hash, (TTAG) newbodyname);
							TRemove((struct TNode *) oldbody);
							TDestroy(&oldbody->handle);
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
							TDestroy(&subc->handle);
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
				TPutHash(this->hash, (TTAG) this->currentbody->handle.thn_Name,
					(TTAG) this->currentbody);
				TAddTail(&(rootcontext(this)->bodylist),
					(struct TNode *) this->currentbody);
				this->currentbody = TNULL;
			}
		}
	}

	return success;

error:

	if (this->currentbody) {
		TDestroy(&this->currentbody->handle);
		this->currentbody = TNULL;
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	printerror(anycontext)
*/

static void printerror(context *this) {
	context *errnode = TNULL;
	context *root = rootcontext(this);
	struct TNode *nn, *n = root->contextlist.tlh_Head;
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

		TDestroy(&c->handle);
	}
	return success;
}
