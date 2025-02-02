
/*
**	display_fb_font.c - Framebuffer display driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/hal.h>

#include "display_fb_mod.h"

/*****************************************************************************/

/* internal structures */

struct fnt_node
{
	struct TNode node;
	TSTRPTR fname;
};

struct fnt_attr
{
	/* list of fontnames */
	struct TList fnlist;
	TSTRPTR fname;
	TINT  fpxsize;
	TBOOL fitalic;
	TBOOL fbold;
	TBOOL fscale;
	TINT  fnum;
};

/*****************************************************************************/

static TBOOL hostopenfont(FBDISPLAY *mod, struct FontNode *fn,
	struct fnt_attr *fattr);
static void hostqueryfonts(FBDISPLAY *mod, struct FontQueryHandle *fqh,
	struct fnt_attr *fattr);
static TINT hostprepfont(FBDISPLAY *mod, TAPTR font, TUINT32* text, TINT textlen);

/*****************************************************************************/
/* FontQueryHandle destructor
** free all memory associated with a fontqueryhandle including
** all fontquerynodes, a fontqueryhandle is obtained by calling
** fb_hostqueryfonts()
*/
THOOKENTRY TTAG
fqhdestroy(struct THook *hook, TAPTR obj, TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct FontQueryHandle *fqh = obj;
		FBDISPLAY *mod = fqh->handle.thn_Owner;
		struct TNode *node, *next;

		node = fqh->reslist.tlh_Head;
		for (; (next = node->tln_Succ); node = next)
		{
			struct FontQueryNode *fqn = (struct FontQueryNode *)node;

			/* remove from resultlist */
			TRemove(&fqn->node);

			/* destroy fontname */
			if (fqn->tags[0].tti_Value)
				TExecFree(mod->fbd_ExecBase, (TAPTR)fqn->tags[0].tti_Value);

			/* destroy node */
			TExecFree(mod->fbd_ExecBase, fqn);
		}

		/* destroy queryhandle */
		TExecFree(mod->fbd_ExecBase, fqh);
	}

	return 0;
}

/*****************************************************************************/
/* allocate a fontquerynode and fill in properties
*/
static struct FontQueryNode *
fnt_getfqnode(FBDISPLAY *mod, TSTRPTR filename, TINT pxsize)
{
	struct FontQueryNode *fqnode = TNULL;

	/* allocate fquery node */
	fqnode = TExecAlloc0(mod->fbd_ExecBase, mod->fbd_MemMgr, sizeof(struct FontQueryNode));
	if (fqnode)
	{
		/* fquerynode ready - fill in attributes */
		TSTRPTR myfname = TNULL;
		TINT flen = strlen(filename)-4; /* discard '.ttf' */

		if (flen > 0)
			myfname = TExecAlloc0(mod->fbd_ExecBase, mod->fbd_MemMgr, flen + 1);
		else
			TDBPRINTF(20, ("found invalid font: '%s'\n", filename));

		if (myfname)
		{
			TExecCopyMem(mod->fbd_ExecBase, filename, myfname, flen);
			fqnode->tags[0].tti_Tag = TVisual_FontName;
			fqnode->tags[0].tti_Value = (TTAG) myfname;
		}
		else
		{
			if (flen > 0)
				TDBPRINTF(20, ("out of memory :(\n"));
		}

		if (fqnode->tags[0].tti_Value)
		{
			TINT i = 1;

			fqnode->tags[i].tti_Tag = TVisual_FontPxSize;
			fqnode->tags[i++].tti_Value = (TTAG) pxsize;

			/* always true */
			fqnode->tags[i].tti_Tag = TVisual_FontScaleable;
			fqnode->tags[i++].tti_Value = (TTAG) TTRUE;

			fqnode->tags[i].tti_Tag = TTAG_DONE;
		}
		else
		{
			TExecFree(mod->fbd_ExecBase, fqnode);
			fqnode = TNULL;
		}

	} /* endif fqnode */
	else
		TDBPRINTF(20, ("out of memory :(\n"));

	return fqnode;
}

/*****************************************************************************/
/* check if a font with similar properties is already contained
** in our resultlist
*/
static TBOOL
fnt_checkfqnode(struct TList *rlist, struct FontQueryNode *fqnode)
{
	TUINT8 flags;
	TBOOL match = TFALSE;
	struct TNode *node, *next;
	TSTRPTR newfname = (TSTRPTR)fqnode->tags[0].tti_Value;
	TINT newpxsize = (TINT)fqnode->tags[1].tti_Value;

	/* not yet
	TBOOL newslant = (TBOOL)fqnode->tags[2].tti_Value;
	TBOOL newweight = (TBOOL)fqnode->tags[3].tti_Value;
	*/

	TSIZE flen = strlen(newfname);

	for (node = rlist->tlh_Head; (next = node->tln_Succ); node = next)
	{
		struct FontQueryNode *fqn = (struct FontQueryNode *)node;
		flags = 0;

		if (strlen((TSTRPTR) fqn->tags[0].tti_Value) == flen)
		{
			if (strncmp((TSTRPTR)fqn->tags[0].tti_Value, newfname, flen) == 0)
				flags = FNT_MATCH_NAME;
		}

		if ((TINT)fqn->tags[1].tti_Value == newpxsize)
			flags |= FNT_MATCH_SIZE;

		/* not yet
		if ((TBOOL)fqn->tags[2].tti_Value == newslant)
			flags |= FNT_MATCH_SLANT;

		if ((TBOOL)fqn->tags[3].tti_Value == newweight)
			flags |= FNT_MATCH_WEIGHT;
		*/

		if (flags == FNT_MATCH_ALL)
		{
			/* fqnode is not unique */
			match = TTRUE;
			break;
		}
	}

	return match;
}

/*****************************************************************************/
/* dump properties of a fontquerynode
*/
static void
fnt_dumpnode(struct FontQueryNode *fqn)
{
	TDBPRINTF(10, ("-----------------------------------------------\n"));
	TDBPRINTF(10, ("dumping fontquerynode @ %p\n", fqn));
	TDBPRINTF(10, (" * FontName: %s\n", (TSTRPTR)fqn->tags[0].tti_Value));
	TDBPRINTF(10, (" * PxSize:   %d\n", (TINT)fqn->tags[1].tti_Value));
	//TDBPRINTF(10, (" * Italic:   %s\n", (TBOOL)fqn->tags[2].tti_Value ? "on" : "off"));
	//TDBPRINTF(10, (" * Bold:     %s\n", (TBOOL)fqn->tags[3].tti_Value ? "on" : "off"));
	TDBPRINTF(10, ("-----------------------------------------------\n"));
}

/*****************************************************************************/
/* dump all fontquerynodes of a (result-)list
*/
static void
fnt_dumplist(struct TList *rlist)
{
	struct TNode *node, *next;
	node = rlist->tlh_Head;
	for (; (next = node->tln_Succ); node = next)
	{
		struct FontQueryNode *fqn = (struct FontQueryNode *)node;
		fnt_dumpnode(fqn);
	}
}

/*****************************************************************************/
/* parses a single fontname or a comma separated list of fontnames
** and returns a list of fontnames, spaces are NOT filtered, so
** "helvetica, fixed" will result in "helvetica" and " fixed"
*/
static void
fnt_getfnnodes(FBDISPLAY *mod, struct TList *fnlist, TSTRPTR fname)
{
	TINT i, p = 0;
	TBOOL lastrun = TFALSE;
	TINT fnlen = strlen(fname);

	for (i = 0; i < fnlen; i++)
	{
		if (i == fnlen-1) lastrun = TTRUE;

		if (fname[i] == ',' || lastrun)
		{
			TINT len = (i > p) ? (lastrun ? (i-p+1) : (i-p)) : fnlen+1;
			TSTRPTR ts = TExecAlloc0(mod->fbd_ExecBase, mod->fbd_MemMgr, len+1);

			if (ts)
			{
				struct fnt_node *fnn;

				TExecCopyMem(mod->fbd_ExecBase, fname+p, ts, len);

				fnn = TExecAlloc0(mod->fbd_ExecBase, mod->fbd_MemMgr, sizeof(struct fnt_node));
				if (fnn)
				{
					/* add fnnode to fnlist */
					fnn->fname = ts;
					TAddTail(fnlist, &fnn->node);
				}
				else
				{
					TDBPRINTF(20, ("out of memory :(\n"));
					break;
				}
			}
			else
			{
				TDBPRINTF(20, ("out of memory :(\n"));
				break;
			}

			p = i+1;
		}
	}
}

/*****************************************************************************/
/* examine filename according to the specified flags and set the
** corresponding bits in the flagfield the function returns
** at the moment only FNT_MATCH_NAME is suppported
*/
static TUINT
fnt_matchfont(FBDISPLAY *mod, TSTRPTR filename, TSTRPTR fname,
	struct fnt_attr *fattr, TUINT flag)
{
	TUINT match = 0;

	if (flag & FNT_MATCH_NAME)
	{
		TINT i;
		TINT len = strlen(fname);

		if (strncmp(fname, FNT_WILDCARD, len) == 0)
		{
			/* match all, but filter out invalid filenames like '.' or '..' */
			if (strlen(filename) > 4)
				match = FNT_MATCH_NAME;
		}
		else
		{
			TSTRPTR tempname = TNULL;

			/* convert fontnames to lower case */
			for (i = 0; i < len; i++)
				fname[i] = tolower(fname[i]);

			tempname = TExecAlloc0(mod->fbd_ExecBase, mod->fbd_MemMgr, len+1);
			if (!tempname)
			{
				TDBPRINTF(20, ("out of memory :(\n"));
				return -1;
			}

			for (i = 0; i < len; i++)
				tempname[i] = tolower(filename[i]);

			/* compare converted fontnames */
			if (strncmp(fname, tempname, len) == 0)
				match = FNT_MATCH_NAME;

			TExecFree(mod->fbd_ExecBase, tempname);
		}
	}

	/* not yet
	if (flag & FNT_MATCH_SLANT)		;
	if (flag & FNT_MATCH_WEIGHT)	;
	*/

	return match;
}

/*****************************************************************************/
/* CALL:
**	fb_hostopenfont(visualbase, tags)
**
** USE:
**  to match and open exactly one font, according to its properties
**
** INPUT:
**	tag name				| description
**	------------------------+---------------------------
**	 TVisual_FontName		| font name
**   TVisual_FontPxSize		| font size in pixel
**
**	tag name				| default¹	| wildcard
**	------------------------+-----------+---------------
**	 TVisual_FontName		| "decker"	| "*"
**   TVisual_FontPxSize		| 14		|  /
**
** ¹ the defaults are used when the tag is missing
**
** RETURN:
** - a pointer to a font ready to be used or TNULL
**
** EXAMPLES:
** - to open the default font of your platform leave all tags empty
** - to open the default font in say 16px, set TVisual_FontPxSize to
**   16 and leave all other tags empty
**
** NOTES:
** - this function won't activate the font, use setfont() to make the
**   font the current active font
** - the function will open the first matching font
*/

LOCAL TAPTR
fb_hostopenfont(FBDISPLAY *mod, TTAGITEM *tags)
{
	struct fnt_attr fattr;
	struct FontNode *fn;
	TAPTR font = TNULL;

	/* fetch user specified attributes */
	fattr.fname = (TSTRPTR) TGetTag(tags, TVisual_FontName, (TTAG) FNT_DEFNAME);
	fattr.fpxsize = (TINT) TGetTag(tags, TVisual_FontPxSize, (TTAG) FNT_DEFPXSIZE);

	/* not yet
	fattr.fitalic = (TBOOL) TGetTag(tags, TVisual_FontItalic, (TTAG) TFALSE);
	fattr.fbold = (TBOOL) TGetTag(tags, TVisual_FontBold, (TTAG) TFALSE);
	fattr.fscale = (TBOOL) TGetTag(tags, TVisual_FontScaleable, (TTAG) TFALSE);
	*/

	if (fattr.fname)
	{
		fn = TExecAlloc0(mod->fbd_ExecBase, mod->fbd_MemMgr, sizeof(struct FontNode));
		if (fn)
		{
			fn->handle.thn_Owner = mod;

			if (hostopenfont(mod, fn, &fattr))
			{
				/* load succeeded, save font attributes */
				fn->pxsize = fattr.fpxsize;
				fn->height = fn->face->size->metrics.height >> 6;
				fn->ascent = fn->face->size->metrics.ascender >> 6;
				fn->descent = fn->face->size->metrics.descender >> 6;

				/* not yet
				if (fattr.fitalic)
					fn->attr = FNT_ITALIC;
				if (fattr.fbold)
					fn->attr |= FNT_BOLD;
				*/

				/* append to the list of open fonts */
				TDBPRINTF(TDB_WARN, ("O '%s' %dpx\n", fattr.fname, fattr.fpxsize));
				TAddTail(&mod->fbd_FontManager.openfonts, &fn->handle.thn_Node);
				font = (TAPTR)fn;
			}
			else
			{
				/* load failed, free fontnode */
				TDBPRINTF(TDB_ERROR,("X unable to load '%s'\n", fattr.fname));
				TExecFree(mod->fbd_ExecBase, fn);
			}
		}
		else
			TDBPRINTF(TDB_ERROR,("out of memory\n"));
	}
	else
		TDBPRINTF(TDB_ERROR,("X invalid fontname '%s' specified\n", fattr.fname));

	return font;
}

static TBOOL
hostopenfont(FBDISPLAY *mod, struct FontNode *fn, struct fnt_attr *fattr)
{
	TBOOL succ = TFALSE;
	TSTRPTR fontfile = TExecAlloc0(mod->fbd_ExecBase, mod->fbd_MemMgr,
		strlen(fattr->fname) + strlen(FNT_DEFDIR) + 5);

	if (fontfile)
	{
		sprintf(fontfile, "%s%s.ttf", FNT_DEFDIR, fattr->fname);
		TDBPRINTF(TDB_INFO,("? %s\n", fontfile));

		if (FT_New_Face(mod->fbd_FTLibrary, fontfile, 0, &fn->face) == 0
			&& FT_IS_SCALABLE(fn->face))
		{
			if (FT_Set_Char_Size(fn->face, 0, fattr->fpxsize*64, 72, 72) == 0)
				succ = TTRUE;
		}
		TExecFree(mod->fbd_ExecBase, fontfile);
	}
	else
		TDBPRINTF(TDB_ERROR,("out of memory\n"));

	return succ;
}

/*****************************************************************************/
/* CALL:
**  fb_hostqueryfonts(visualbase, tags)
**
** USE:
**  to match one or more fonts, according to their properties
**
** INPUT:
**	tag name				| description
**	------------------------+---------------------------
**	 TVisual_FontName		| font name
**   TVisual_FontPxSize		| font size in pixel
**	 TVisual_FontNumResults	| how many fonts to return
**
**	tag name				| default¹
**	------------------------+----------------------------
**	 TVisual_FontName		| FNTQUERY_UNDEFINED
**   TVisual_FontPxSize		| FNTQUERY_UNDEFINED
**	 TVisual_FontNumResults	| INT_MAX
**
** ¹ the defaults are used when the tag is missing
**
** RETURN:
** - a pointer to a FontQueryHandle, which is basically a list of
**   taglists, referring to the fonts matched
** - use fb_hostgetnextfont() to traverse the list
** - use TDestroy to free all memory associated with a FontQueryHandle
**
** EXAMPLES:
** - to match all available fonts, use an empty taglist
** - to match more than one specific font, use a coma separated list
**   for TVisual_FontName, e.g. "helvetica,utopia,fixed", note that
**   spaces are not filtered
**
** NOTES:
** - this function won't open any fonts, to do so use fb_hostopenfont()
*/

LOCAL TAPTR
fb_hostqueryfonts(FBDISPLAY *mod, TTAGITEM *tags)
{
	TSTRPTR fname = TNULL;
	struct fnt_attr fattr;
	struct TNode *node, *next;
	struct FontQueryHandle *fqh = TNULL;

	/* init fontname list */
	TInitList(&fattr.fnlist);

	/* fetch and parse fname */
	fname = (TSTRPTR) TGetTag(tags, TVisual_FontName, (TTAG) FNT_WILDCARD);
	if (fname)	fnt_getfnnodes(mod, &fattr.fnlist, fname);

	/* fetch user specified attributes */
	fattr.fpxsize = (TINT) TGetTag(tags, TVisual_FontPxSize, (TTAG) FNT_DEFPXSIZE);
	fattr.fnum = (TINT) TGetTag(tags, TVisual_FontNumResults, (TTAG) INT_MAX);
	/* not yet
	fattr.fitalic = (TBOOL) TGetTag(tags, TVisual_FontItalic, (TTAG) FNTQUERY_UNDEFINED);
	fattr.fbold = (TBOOL) TGetTag(tags, TVisual_FontBold, (TTAG) FNTQUERY_UNDEFINED);
	*/

	/* init result list */
	fqh = TExecAlloc0(mod->fbd_ExecBase, mod->fbd_MemMgr, sizeof(struct FontQueryHandle));
	if (fqh)
	{
		fqh->handle.thn_Owner = mod;
		/* connect destructor */
		TInitHook(&fqh->handle.thn_Hook, fqhdestroy, fqh);
		TInitList(&fqh->reslist);
		/* init list iterator */
		fqh->nptr = &fqh->reslist.tlh_Head;

		hostqueryfonts(mod, fqh, &fattr);
		TDB(10,(fnt_dumplist(&fqh->reslist)));
		TDBPRINTF(10, ("***********************************************\n"));
	}
	else
		TDBPRINTF(20, ("out of memory :(\n"));

	/* free memory of fnt_nodes */
	for (node = fattr.fnlist.tlh_Head; (next = node->tln_Succ); node = next)
	{
		struct fnt_node *fnn = (struct fnt_node *)node;
		TExecFree(mod->fbd_ExecBase, fnn->fname);
		TRemove(&fnn->node);
		TExecFree(mod->fbd_ExecBase, fnn);
	}

	return fqh;
}

static void
hostqueryfonts(FBDISPLAY *mod, struct FontQueryHandle *fqh, struct fnt_attr *fattr)
{
	TINT i, nfont, fcount = 0;
	struct TNode *node, *next;
	struct dirent **dirlist;
	TUINT matchflg = 0;

	/* scan default font directory */
	nfont = scandir(FNT_DEFDIR, &dirlist, 0, alphasort);
	if (nfont < 0)
	{
		perror("scandir");
		return;
	}

	if (nfont > 0)
	{
		/* found fonts in default font directory */
		for (node = fattr->fnlist.tlh_Head; (next = node->tln_Succ); node = next)
		{
			struct fnt_node *fnn = (struct fnt_node *)node;

			/* build matchflag, font pxsize attribute is ignored,
			   because it's not relevant when matching ttf fonts */

			matchflg = FNT_MATCH_NAME;

			/* not yet
			if (fattr->fitalic != FNTQUERY_UNDEFINED)
				matchflg |= FNT_MATCH_SLANT;
			if (fattr->fbold != FNTQUERY_UNDEFINED)
				matchflg |= FNT_MATCH_WEIGHT;
			*/

			for (i = 0; i < nfont; i++)
			{
				if (fnt_matchfont(mod, dirlist[i]->d_name, fnn->fname,
					fattr, matchflg) == matchflg)
				{
					struct FontQueryNode *fqnode;

					/* create fqnode and fill in attributes */
					fqnode = fnt_getfqnode(mod, dirlist[i]->d_name, fattr->fpxsize);
					if (!fqnode)	break;

					/* compare fqnode with nodes in result list */
					if (fnt_checkfqnode(&fqh->reslist, fqnode) == 0)
					{
						if (fcount < fattr->fnum)
						{
							/* fqnode is unique, add to result list */
							TAddTail(&fqh->reslist, &fqnode->node);
							fcount++;
						}
						else
						{
							/* max count of desired results reached */
							TExecFree(mod->fbd_ExecBase, (TSTRPTR)fqnode->tags[0].tti_Value);
							TExecFree(mod->fbd_ExecBase, fqnode);
							break;
						}
					}
					else
					{
						/* fqnode is not unique, destroy it */
						TDBPRINTF(10,("X node is not unique\n"));
						TExecFree(mod->fbd_ExecBase, (TSTRPTR)fqnode->tags[0].tti_Value);
						TExecFree(mod->fbd_ExecBase, fqnode);
					}
				}
			}

			if (fcount == fattr->fnum)
				break;

		} /* end of fnlist iteration */

	} /* endif fonts found */
	else
		TDBPRINTF(10, ("X no fonts found in '%s'\n", FNT_DEFDIR));

	while (nfont--)
		free(dirlist[nfont]);
	free(dirlist);
}

/*****************************************************************************/
/* CALL:
**  fb_hostsetfont(visual, fontpointer)
**
** USE:
**  makes the font referred to by fontpointer the current active font
**  for the visual
**
** INPUT:
**  a pointer to a font returned by fb_hostopenfont()
**
** NOTES:
** - if a font is active it can't be closed
*/

LOCAL void
fb_hostsetfont(FBDISPLAY *mod, FBWINDOW *v, TAPTR font)
{
	if (font)
	{
		v->curfont = font;
	}
	else
		TDBPRINTF(20, ("invalid font specified\n"));
}

/*****************************************************************************/
/* CALL:
**  fb_hostgetnextfont(visualbase, fontqueryhandle)
**
** USE:
**  iterates a list of taglists, returning the next taglist
**  pointer or TNULL
**
** INPUT:
**  a fontqueryhandle obtained by calling fb_hostqueryfonts()
**
** RETURN:
**  a pointer to a taglist, representing a font or TNULL
**
** NOTES:
**  - the taglist returned by this function can be directly fed to
**	  fb_hostopenfont()
**  - if the end of the list is reached, TNULL is returned and the
**    iterator is reset to the head of the list
*/

LOCAL TTAGITEM *
fb_hostgetnextfont(FBDISPLAY *mod, TAPTR fqhandle)
{
	struct FontQueryHandle *fqh = fqhandle;
	struct TNode *next = *fqh->nptr;

	if (next->tln_Succ == TNULL)
	{
		fqh->nptr = &fqh->reslist.tlh_Head;
		return TNULL;
	}

	fqh->nptr = (struct TNode **)next;
	return ((struct FontQueryNode *)next)->tags;
}

/*****************************************************************************/
/* CALL:
**  fb_hostclosefont(visualbase, fontpointer)
**
** USE:
**  attempts to free all memory associated with the font referred to
**  by fontpointer
**
** INPUT:
**  a pointer to a font returned by fb_hostopenfont()
**
** NOTES:
**  - the default font is only freed, if there are no more references
**	  to it left
**  - the attempt to free any other font which is currently in use,
**	  will be ignored
*/

LOCAL void
fb_hostclosefont(FBDISPLAY *mod, TAPTR font)
{
	struct FontNode *fn = (struct FontNode *) font;

	if (font == mod->fbd_FontManager.deffont)
	{
		if (mod->fbd_FontManager.defref)
		{
			/* prevent freeing of default font if it's */
			/* still referenced */
			mod->fbd_FontManager.defref--;
			return;
		}
	}

	/* free fbfont */
	if (fn->face)
	{
		FT_Done_Face(fn->face);
		fn->face = TNULL;
	}

	/* remove font from openfonts list */
	TRemove(&fn->handle.thn_Node);

	/* free fontnode itself */
	TExecFree(mod->fbd_ExecBase, fn);
}

/*****************************************************************************/
/* CALL:
**  fb_hosttextsize(visualbase, fontpointer, textstring)
**
** USE:
**  to obtain the width of a given string when the font referred to
**  by fontpointer is used to render the text
**
** INPUT:
**  - a pointer to a font returned by fb_hostopenfont()
**  - the textstring to measure
**
** RETURN:
**  - the width of the textstring in pixel
*/

LOCAL TINT
fb_hosttextsize(FBDISPLAY *mod, TAPTR font, TSTRPTR text, TINT len)
{
	TINT width = 0;
	TUINT32 *unicode = fb_utf8tounicode(mod, text, len, &len);
	width = hostprepfont(mod, font, unicode, len);
	return width;
}

/*****************************************************************************/
/* prepare glyphs for rendering via freetype
*/

static TINT
hostprepfont(FBDISPLAY *mod, TAPTR font, TUINT32 *text, TINT textlen)
{
	TINT n, error, pen_x, pen_y;
	FT_UInt glyph_index;
	FT_UInt previous;
	FT_Bool use_kerning;
	FT_GlyphSlot slot = ((struct FontNode *)font)->face->glyph;
	FT_Face face = ((struct FontNode *)font)->face;
	struct FontManager *fm = &mod->fbd_FontManager;

	pen_x = 0;   /* start at (0,0) */
	pen_y = 0;

	previous = 0;
	fm->num_glyphs = 0;
	use_kerning = FT_HAS_KERNING(face);

	for (n = 0; n < textlen; n++)
	{
		/* convert character code to glyph index */
		glyph_index = FT_Get_Char_Index(face, text[n]);

		/* retrieve kerning distance and move pen position */
		if (use_kerning && previous && glyph_index)
		{
			FT_Vector delta;

			FT_Get_Kerning(face, previous, glyph_index,
						FT_KERNING_DEFAULT, &delta);

			pen_x += delta.x >> 6;
		}

		/* store current pen position */
		fm->pos[fm->num_glyphs].x = pen_x;
		fm->pos[fm->num_glyphs].y = pen_y;

		/* load glyph image into the slot without rendering */
		error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
		if (error)
			continue;  /* ignore errors, jump to next glyph */

		/* extract glyph image and store it in our table */
		error = FT_Get_Glyph(face->glyph, &fm->glyphs[fm->num_glyphs]);
		if (error)
			continue;  /* ignore errors, jump to next glyph */

		/* increment pen position */
		pen_x += slot->advance.x >> 6;

		/* record current glyph index */
		previous = glyph_index;

		/* increment number of glyphs */
		fm->num_glyphs++;
	}

	return pen_x;
}

/*****************************************************************************/
/* CALL:
**  fb_hostdrawtext(visualbase, text, textlen, text pos x, text pos y,
**		textpen)
**
** USE:
**  draw text using the current active font
**
** INPUT:
**  - an utf8 string and its length
**  - x and y position of text
**  - a pen to color the text
**
** NOTES:
**  - the text is clipped against v->fbv_ClipRect[4]
*/

LOCAL TVOID
fb_hostdrawtext(FBDISPLAY *mod, FBWINDOW *v, TSTRPTR text, TINT len, TUINT posx,
	TUINT posy, TVPEN fgpen)
{
	if (text)
	{
		TINT n, error, asc;
		struct FontManager *fm = &mod->fbd_FontManager;
		FT_Face face = ((struct FontNode *)v->curfont)->face;
		struct FBPen *textpen = (struct FBPen *) fgpen;
		TUINT32 *unicode = fb_utf8tounicode(mod, text, len, &len);

		hostprepfont(mod, v->curfont, unicode, len);
		asc = face->size->metrics.ascender >> 6;

		for (n = 0; n < fm->num_glyphs; n++)
		{
			FT_Vector pen;
			FT_Glyph image = fm->glyphs[n];

			pen.x = fm->pos[n].x;
			pen.y = fm->pos[n].y;

			error = FT_Glyph_To_Bitmap(&image, FT_RENDER_MODE_NORMAL, &pen, 0);

			if (!error)
			{
				FT_BitmapGlyph bit = (FT_BitmapGlyph)image;

				int x, y;
				int cx = 0, cy = 0;
				int cw = bit->bitmap.width;
				int ch = bit->bitmap.rows;

				pen.x += (posx + bit->left);
				pen.y += ((posy+asc) - bit->top);

				/* clipping tests */
				if (v->fbv_ClipRect[0] > pen.x)
					cx = v->fbv_ClipRect[0] - pen.x;

				if (v->fbv_ClipRect[1] > pen.y)
					cy = v->fbv_ClipRect[0] - pen.y;

				if (v->fbv_ClipRect[2] < pen.x + bit->bitmap.width)
					cw -= (pen.x + bit->bitmap.width) - (v->fbv_ClipRect[2]);

				if (v->fbv_ClipRect[3] < pen.y + bit->bitmap.rows)
					ch -= (pen.y + bit->bitmap.rows) - (v->fbv_ClipRect[3]);

				for (y = cy; y < ch; y++)
				{
					for (x = cx; x < cw; x++)
					{
						struct FBPen fbpen;
						TUINT8 a = bit->bitmap.buffer[y*bit->bitmap.width+x];
						TUINT32 pix = GetPixel(v, (x+pen.x), (y+pen.y));
						TUINT8 dr = pix >> 16;
						TUINT8 dg = pix >> 8;
						TUINT8 db = pix & 0xff;

						/*	Alpha blending with a = 0..255
							dr += (((sr - dr) * a) >> 8);
							dg += (((sg - dg) * a) >> 8);
							db += (((sb - db) * a) >> 8);
						*/
						dr += ((((textpen->rgb >> 16) & 0xff) - dr) * a) >> 8;
						dg += ((((textpen->rgb >>  8) & 0xff) - dg) * a) >> 8;
						db += (((textpen->rgb & 0xff) - db) * a) >> 8;

						fbpen.rgb = (dr << 16) | (dg << 8) | db;
						WritePixel(v, (x+pen.x), (y+pen.y), &fbpen);
					}
				}

				FT_Done_Glyph(image);
			}
		}
	} /* endif text */
}

/*****************************************************************************/
/* CALL:
**	fb_getfattrs(visualbase, fontpointer, taglist);
**
** USE:
**  fills the taglist with the requested properties of the
**  font referred to by fontpointer
**
** INPUT:
**  - a pointer to a font returned by fb_hostopenfont()
**	- the following tags can be used
**
**  tag name				| description
**	------------------------+---------------------------
**  TVisual_FontPxSize		| font size in pixel
** 	TVisual_FontAscent		| the font ascent in pixel
**	TVisual_FontDescent		| the font descent in pixel
**  TVisual_FontHeight		| height in pixel
**	TVisual_FontUlPosition	| position of an underline
**	TVisual_FontUlThickness	| thickness of the underline
**
** RETURN:
**  - the number of processed properties
**
** NOTES:
**  - TVisual_FontUlPosition defaults to fontdescent / 2
**	  and TVisual_FontUlThickness defaults to 1
*/

LOCAL THOOKENTRY TTAG
fb_hostgetfattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct attrdata *data = hook->thk_Data;
	TTAGITEM *item = obj;
	struct FontNode *fn = (struct FontNode *) data->font;

	switch (item->tti_Tag)
	{
		default:
			return TTRUE;

		case TVisual_FontPxSize:
			*((TINT *) item->tti_Value) = fn->pxsize;
			break;

		/* not yet
		case TVisual_FontItalic:
			*((TINT *) item->tti_Value) = (fn->attr & FNT_ITALIC) ?
				TTRUE : TFALSE;
			break;

		case TVisual_FontBold:
			*((TINT *) item->tti_Value) = (fn->attr & FNT_BOLD) ?
				TTRUE : TFALSE;
			break;
		*/
		case TVisual_FontAscent:
			*((TINT *) item->tti_Value) = fn->ascent;
			break;

		case TVisual_FontDescent:
			*((TINT *) item->tti_Value) = fn->descent;
			break;

		case TVisual_FontHeight:
			*((TINT *) item->tti_Value) = fn->height;
			break;

		case TVisual_FontUlPosition:
		{
			*((TINT *) item->tti_Value) = fn->descent / 2;
			break;
		}
		case TVisual_FontUlThickness:
		{
			*((TINT *) item->tti_Value) = 1;
			break;
		}

		/* ... */
	}
	data->num++;
	return TTRUE;
}

/*****************************************************************************/
