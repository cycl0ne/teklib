
/*
**	display_fb_region.c - Region utilities
**	Written by Timm S. Mueller <tmueller@schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "display_fb_mod.h"

/*****************************************************************************/

#define MERGE_RECTS	5

#define OVERLAP(d0, d1, d2, d3, s0, s1, s2, s3) \
((s2) >= (d0) && (s0) <= (d2) && (s3) >= (d1) && (s1) <= (d3))

#define OVERLAPRECT(d, s) \
OVERLAP((d)[0], (d)[1], (d)[2], (d)[3], (s)[0], (s)[1], (s)[2], (s)[3])

/*****************************************************************************/

static void relinklist(struct TList *dlist, struct TList *slist)
{
	if (!TISLISTEMPTY(slist))
	{
		struct TNode *first = slist->tlh_Head;
		struct TNode *last = slist->tlh_TailPred;
		first->tln_Pred = (struct TNode *) dlist;
		last->tln_Succ = (struct TNode *) &dlist->tlh_Tail;
		dlist->tlh_Head = first;
		dlist->tlh_TailPred = last;
	}
}

static struct RectNode *allocrectnode(TAPTR exec,
	TINT x0, TINT y0, TINT x1, TINT y1)
{
	struct RectNode *rn = TExecAlloc(exec, TNULL, sizeof(struct RectNode));
	if (rn)
	{
		rn->rn_Rect[0] = x0;
		rn->rn_Rect[1] = y0;
		rn->rn_Rect[2] = x1;
		rn->rn_Rect[3] = y1;
	}
	return rn;
}

static void freelist(TAPTR exec, struct TList *list)
{
	struct TNode *next, *node = list->tlh_Head;
	for (; (next = node->tln_Succ); node = next)
	{
		TREMOVE(node);
		TExecFree(exec, node);
	}
}

/*****************************************************************************/

static TBOOL insertrect(TAPTR exec, struct TList *list,
	TINT s0, TINT s1, TINT s2, TINT s3)
{
	struct TNode *temp, *next, *node = list->tlh_Head;
	struct RectNode *rn;
	int i;

	#if defined(MERGE_RECTS)
	for (i = 0; i < MERGE_RECTS && (next = node->tln_Succ); node = next, ++i)
	{
		rn = (struct RectNode *) node;
		if (rn->rn_Rect[1] == s1 && rn->rn_Rect[3] == s3)
		{
			if (rn->rn_Rect[2] + 1 == s0)
			{
				rn->rn_Rect[2] = s2;
				return TTRUE;
			}
			else if (rn->rn_Rect[0] == s2 + 1)
			{
				rn->rn_Rect[0] = s0;
				return TTRUE;
			}
		}
		else if (rn->rn_Rect[0] == s0 && rn->rn_Rect[2] == s2)
		{
			if (rn->rn_Rect[3] + 1 == s1)
			{
				rn->rn_Rect[3] = s3;
				return TTRUE;
			}
			else if (rn->rn_Rect[1] == s3 + 1)
			{
				rn->rn_Rect[1] = s1;
				return TTRUE;
			}
		}
	}
	#endif

	rn = allocrectnode(exec, s0, s1, s2, s3);
	if (rn)
	{
		TADDHEAD(list, &rn->rn_Node, temp);
		return TTRUE;
	}

	return TFALSE;
}

static TBOOL cutrect(TAPTR exec, struct TList *list, const TINT d[4],
	const TINT s[4])
{
	TINT d0 = d[0];
	TINT d1 = d[1];
	TINT d2 = d[2];
	TINT d3 = d[3];

	if (!OVERLAPRECT(d, s))
		return insertrect(exec, list, d[0], d[1], d[2], d[3]);

	for (;;)
	{
		if (d0 < s[0])
		{
			if (!insertrect(exec, list, d0, d1, s[0] - 1, d3))
				break;
			d0 = s[0];
		}

		if (d1 < s[1])
		{
			if (!insertrect(exec, list, d0, d1, d2, s[1] - 1))
				break;
			d1 = s[1];
		}

		if (d2 > s[2])
		{
			if (!insertrect(exec, list, s[2] + 1, d1, d2, d3))
				break;
			d2 = s[2];
		}

		if (d3 > s[3])
		{
			if (!insertrect(exec, list, d0, s[3] + 1, d2, d3))
				break;
		}

		return TTRUE;

	}
	return TFALSE;
}

/*****************************************************************************/

LOCAL struct Region *fb_region_new(FBDISPLAY *mod, TINT s[])
{
	struct Region *region = TExecAlloc(mod->fbd_ExecBase, TNULL,
		sizeof(struct Region));
	if (region)
	{
		TINITLIST(&region->rg_List);
		if (!insertrect(mod->fbd_ExecBase, &region->rg_List,
			s[0], s[1], s[2], s[3]))
		{
			TExecFree(mod->fbd_ExecBase, region);
			region = TNULL;
		}
	}
	return region;
}

LOCAL void fb_region_destroy(FBDISPLAY *mod, struct Region *region)
{
	freelist(mod->fbd_ExecBase, &region->rg_List);
	TExecFree(mod->fbd_ExecBase, region);
}

LOCAL TBOOL fb_region_overlap(FBDISPLAY *mod, struct Region *region,
	TINT s[])
{
	struct TNode *next, *node;
	node = region->rg_List.tlh_Head;
	for (; (next = node->tln_Succ); node = next)
	{
		struct RectNode *rn = (struct RectNode *) node;
		if (OVERLAPRECT(rn->rn_Rect, s))
			return TTRUE;
	}
	return TFALSE;
}

LOCAL TBOOL fb_region_subrect(FBDISPLAY *mod, struct Region *region,
	TINT s[])
{
	struct TList r1;
	struct TNode *next, *node;
	TBOOL success = TTRUE;
	TAPTR exec = mod->fbd_ExecBase;

	TINITLIST(&r1);
	node = region->rg_List.tlh_Head;
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct TNode *next2, *node2;
		struct RectNode *rn = (struct RectNode *) node;
		struct TList temp;

		TINITLIST(&temp);
		success = cutrect(exec, &temp, rn->rn_Rect, s);

		node2 = temp.tlh_Head;
		for (; success && (next2 = node2->tln_Succ); node2 = next2)
		{
			struct RectNode *rn2 = (struct RectNode *) node2;
			success = insertrect(exec, &r1, rn2->rn_Rect[0],
				rn2->rn_Rect[1], rn2->rn_Rect[2], rn2->rn_Rect[3]);
		}

		freelist(exec, &temp);
	}

	if (success)
	{
		freelist(exec, &region->rg_List);
		relinklist(&region->rg_List, &r1);
	}
	else
		freelist(exec, &r1);

	return success;
}

LOCAL TBOOL fb_region_subregion(FBDISPLAY *mod, struct Region *dregion,
	struct Region *sregion)
{
	TBOOL success = TTRUE;
	struct TNode *next, *node;
	node = sregion->rg_List.tlh_Head;
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct RectNode *rn = (struct RectNode *) node;
		success = fb_region_subrect(mod, dregion, rn->rn_Rect);
	}
	/* note: if unsucessful, dregion is of no use anymore */
	return success;
}

static TBOOL int_andrect(FBDISPLAY *mod, struct TList *temp,
	struct Region *region, TINT s[])
{
	struct TNode *next, *node = region->rg_List.tlh_Head;
	TBOOL success = TTRUE;
	TINT s0 = s[0];
	TINT s1 = s[1];
	TINT s2 = s[2];
	TINT s3 = s[3];
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct RectNode *dr = (struct RectNode *) node;
		TINT x0 = dr->rn_Rect[0];
		TINT y0 = dr->rn_Rect[1];
		TINT x1 = dr->rn_Rect[2];
		TINT y1 = dr->rn_Rect[3];
		if (OVERLAP(x0, y0, x1, y1, s0, s1, s2, s3))
		{
			success = insertrect(mod->fbd_ExecBase, temp,
				TMAX(x0, s0), TMAX(y0, s1), TMIN(x1, s2), TMIN(y1, s3));
		}
	}
	if (!success)
		freelist(mod->fbd_ExecBase, temp);
	return success;
}

LOCAL TBOOL fb_region_andrect(FBDISPLAY *mod, struct Region *region,
	TINT s[])
{
	struct TList temp;
	TINITLIST(&temp);
	if (int_andrect(mod, &temp, region, s))
	{
		freelist(mod->fbd_ExecBase, &region->rg_List);
		relinklist(&region->rg_List, &temp);
		return TTRUE;
	}
	return TFALSE;
}

LOCAL TBOOL fb_region_andregion(FBDISPLAY *mod, struct Region *dregion,
	struct Region *sregion)
{
	struct TNode *next, *node = sregion->rg_List.tlh_Head;
	TBOOL success = TTRUE;
	struct TList temp;
	TINITLIST(&temp);
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct RectNode *sr = (struct RectNode *) node;
		success = int_andrect(mod, &temp, dregion, sr->rn_Rect);
	}
	if (success)
	{
		freelist(mod->fbd_ExecBase, &dregion->rg_List);
		relinklist(&dregion->rg_List, &temp);
	}
	/* note: if unsucessful, dregion is of no use anymore */
	return success;
}

LOCAL TBOOL fb_region_isempty(FBDISPLAY *mod, struct Region *region)
{
	return TISLISTEMPTY(&region->rg_List);
}
