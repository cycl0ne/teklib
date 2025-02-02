
/*
**	display_fb_draw.c - Primitive drawing
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <string.h>
#include "display_fb_mod.h"

#define	MAX_VERT	6
#define	NUM_VERT	3

#define	OC_TOP		0x1
#define	OC_BOTTOM	0x2
#define	OC_RIGHT	0x4
#define	OC_LEFT		0x8

struct Coord
{
	TINT x, y;
};

struct Slope
{
	TINT inta;		/* integer part */
	TINT z;			/* numerator */
	TINT n;			/* denumerator */
	TINT cz;		/* current numerator */
	struct Coord S;	/* start coordinate */
};

/*****************************************************************************/

LOCAL void
WritePixel(FBWINDOW *v, TINT x, TINT y, struct FBPen *pen)
{
	((TUINT32 *)v->fbv_BufPtr)[y * v->fbv_PixelPerLine + x] = pen->rgb;

#if 0
	if (x >= v->fbv_ClipRect[0] && x <= v->fbv_ClipRect[2] && y >= v->fbv_ClipRect[1] && y <= v->fbv_ClipRect[3])
		((TUINT32 *)v->bufptr)[(y*(v->fbv_Width + v->fbv_Modulo) + x)] = pen->rgb;
	else
	{
		printf("clip: %d, %d, %d, %d\n", v->fbv_ClipRect[0], v->fbv_ClipRect[1], v->fbv_ClipRect[2], v->fbv_ClipRect[3]);
		printf("HILFE! %d, %d out of range\n\n", x, y);
		*(TUINT *)(0x0) = 0;
	}
#endif
}

LOCAL TUINT32
GetPixel(FBWINDOW *v, TINT x, TINT y)
{
	return ((TUINT32 *)v->fbv_BufPtr)[y * v->fbv_PixelPerLine + x];
}

/*****************************************************************************/

TINLINE static void
CopyLine(FBWINDOW *v, TUINT8 *buf, TINT totw, TINT xs, TINT ys,
	TINT xd, TINT yd, TINT numb)
{
	memcpy(&((TUINT32 *)v->fbv_BufPtr)[(yd * v->fbv_PixelPerLine + xd)],
		   &((TUINT32 *)buf)[ys*totw+xs], numb);
}

/*****************************************************************************/

TINLINE static void
CopyLineOver(FBWINDOW *v, TINT xs, TINT ys, TINT xd, TINT yd, TINT numb)
{
	memmove(&((TUINT32 *)v->fbv_BufPtr)[yd * v->fbv_PixelPerLine + xd],
		    &((TUINT32 *)v->fbv_BufPtr)[ys * v->fbv_PixelPerLine + xs], numb);
}

/*****************************************************************************/

TINLINE static TBOOL
clippoint(TINT x, TINT y, TINT xmin, TINT ymin, TINT xmax, TINT ymax)
{
	TBOOL accept = TFALSE;
	if (x >= xmin && x <= xmax &&
		y >= ymin && y <= ymax)
		accept = TTRUE;

	return accept;
}

/*****************************************************************************/
/* calculate intersection with clipping edge */

static void
intersect(struct Coord *res, struct Coord v1, struct Coord v2,
	struct Coord ce[2], TUINT which)
{
	TINT a, b;
	TINT x0 = v1.x, y0 = v1.y;
	TINT x1 = v2.x, y1 = v2.y;
	TINT xmin = ce[0].x, ymin = ce[0].y;
	TINT xmax = ce[1].x, ymax = ce[1].y;

	if (which & OC_TOP)
	{
		b = y1 - y0;
		a = (x1 - x0) * (ymax - y0);

		if ((a < 0) != (b < 0))
			res->x = x0 + (a - b/2) / b;
		else
			res->x = x0 + (a + b/2) / b;

		res->y = ymax;
	}
	else if (which & OC_BOTTOM)
	{
		b = y1 - y0;
		a = (x1 - x0) * (ymin - y0);

		if ((a < 0) != (b < 0))
			res->x = x0 + (a - b/2) / b;
		else
			res->x = x0 + (a + b/2) / b;

		res->y = ymin;
	}
	else if (which & OC_RIGHT)
	{
		b = x1 - x0;
		a = (y1 - y0) * (xmax - x0);

		if ((a < 0) != (b < 0))
			res->y = y0 + (a - b/2) / b;
		else
			res->y = y0 + (a + b/2) / b;

		res->x = xmax;
	}
	else
	{
		b = x1 - x0;
		a = (y1 - y0) * (xmin - x0);

		if ((a < 0) != (b < 0))
			res->y = y0 + (a - b/2) / b;
		else
			res->y = y0 + (a + b/2) / b;

		res->x = xmin;
	}
}

/*****************************************************************************/

static TBOOL
inside(struct Coord v, struct Coord ce[2], TUINT which)
{
	switch (which)
	{
		case OC_BOTTOM:
			if (v.y >= ce[0].y) return TTRUE;
			break;
		case OC_TOP:
			if (v.y <= ce[1].y) return TTRUE;
			break;
		case OC_LEFT:
			if (v.x >= ce[0].x) return TTRUE;
			break;
		case OC_RIGHT:
			if (v.x <= ce[1].x) return TTRUE;
			break;
		default:
			break;
	}
	return TFALSE;
}

static void
output(struct Coord outva[MAX_VERT], struct Coord v, TINT *outlen)
{
	if (*outlen < MAX_VERT)
	{
		/* filter out double entries */
		if (*outlen == 0 || (outva[(*outlen)-1].x != v.x
			|| outva[(*outlen)-1].y != v.y))
		{
			outva[*outlen].x = v.x;
			outva[*outlen].y = v.y;
			(*outlen)++;
		}
	}
}

/* sutherland hodgman polygon clipping */
static void
shpolyclip(struct Coord outva[MAX_VERT], TINT *outlen, TINT inlen,
	struct Coord inva[NUM_VERT], struct Coord clipedge[2], TUINT which)
{
	struct Coord s, p, i;
	TINT j;

	*outlen = 0;
	s.x = inva[inlen-1].x;
	s.y = inva[inlen-1].y;

	for (j = 0; j < inlen; j++)
	{
		p.x = inva[j].x;
		p.y = inva[j].y;

		if (inside(p, clipedge, which))
		{
			/* cases 1 and 4 */
			if (inside(s, clipedge, which))
			{
				/* case 1 */
				output(outva, p, outlen);
			}
			else
			{
				/* case 4 */
				intersect(&i, s, p, clipedge, which);
				output(outva, i, outlen);
				output(outva, p, outlen);
			}
		}
		else
		{
			/* cases 2 and 3 */
			if (inside(s, clipedge, which))
			{
				/* case 2 */
				intersect(&i, s, p, clipedge, which);
				output(outva, i, outlen);
			} /* else do nothing (case 3) */
		}

		/* advance to next pair of vertices */
		s.x = p.x;
		s.y = p.y;
	}
}

static TBOOL
clippoly(struct Coord res[MAX_VERT], TINT *outlen,
	TINT x0, TINT y0, TINT x1, TINT y1, TINT x2, TINT y2,
	TINT xmin, TINT ymin, TINT xmax, TINT ymax)
{
	struct Coord outva[MAX_VERT];
	struct Coord inva[NUM_VERT];
	struct Coord clipedge[2];
	TINT inlen = NUM_VERT;

	inva[0].x = x0; inva[0].y = y0;
	inva[1].x = x1; inva[1].y = y1;
	inva[2].x = x2; inva[2].y = y2;

	clipedge[0].x = xmin; clipedge[0].y = ymin;
	clipedge[1].x = xmax; clipedge[1].y = ymax;

	/* process right clip boundary */
	shpolyclip(outva, outlen, inlen, inva, clipedge, OC_RIGHT);
	if (!*outlen) return TFALSE;

	/* process bottom clip boundary */
	inlen = *outlen;
	shpolyclip(res, outlen, inlen, outva, clipedge, OC_BOTTOM);
	if (!*outlen) return TFALSE;

	/* process left clip boundary */
	inlen = *outlen;
	shpolyclip(outva, outlen, inlen, res, clipedge, OC_LEFT);
	if (!*outlen) return TFALSE;

	/* process top clip boundary */
	inlen = *outlen;
	shpolyclip(res, outlen, inlen, outva, clipedge, OC_TOP);
	if (!*outlen) return TFALSE;

	return TTRUE;
}

/*****************************************************************************/

static TUINT
getoutcode(TINT x, TINT y, TINT xmin, TINT ymin, TINT xmax, TINT ymax)
{
	TUINT outcode = 0;

	if (y > ymax)
		outcode |= OC_TOP;
	else if (y < ymin)
		outcode |= OC_BOTTOM;
	if (x > xmax)
		outcode |= OC_RIGHT;
	else if (x < xmin)
		outcode |= OC_LEFT;

	return outcode;
}

/*****************************************************************************/

static TBOOL
cliprect(struct Coord res[2], TINT x0, TINT y0, TINT x1, TINT y1,
	TINT xmin, TINT ymin, TINT xmax, TINT ymax)
{
	TBOOL accept = TFALSE;
	TBOOL done = TFALSE;
	TUINT outc0, outc1, outc;

	outc0 = getoutcode(x0, y0, xmin, ymin, xmax, ymax);
	outc1 = getoutcode(x1, y1, xmin, ymin, xmax, ymax);

	do
	{
		if (!(outc0 | outc1))
		{
			/* trivial accept and exit */
			accept = TTRUE;
			done = TTRUE;
		}
		else if (outc0 & outc1)
		{
			/* trivial reject and exit */
			done = TTRUE;
		}
		else
		{
			/* move vertices to clipping edge */
			TINT x = 0, y = 0;

			outc = outc0 ? outc0 : outc1;
			if (outc & OC_TOP)
				y = ymax;
			else if (outc & OC_BOTTOM)
				y = ymin;
			else if (outc & OC_RIGHT)
				x = xmax;
			else
				x = xmin;

			/* get ready for next pass */
			if (outc == outc0)
			{
				if (outc & (OC_TOP | OC_BOTTOM))
					y0 = y;
				else
					x0 = x;

				outc0 = getoutcode(x0, y0, xmin, ymin, xmax, ymax);
			}
			else
			{
				if (outc & (OC_TOP | OC_BOTTOM))
					y1 = y;
				else
					x1 = x;

				outc1 = getoutcode(x1, y1, xmin, ymin, xmax, ymax);
			}

		} /* endif subdivide */

	} while (done == TFALSE);

	if (accept)
	{
		res[0].x = x0; res[0].y = y0;
		res[1].x = x1; res[1].y = y1;
	}

	return accept;
}

/*****************************************************************************/

static TBOOL
clipline(struct Coord res[2], TINT x0, TINT y0, TINT x1, TINT y1,
	TINT xmin, TINT ymin, TINT xmax, TINT ymax)
{
	TBOOL accept = TFALSE;
	TBOOL done = TFALSE;
	TUINT outc0, outc1, outc;

	outc0 = getoutcode(x0, y0, xmin, ymin, xmax, ymax);
	outc1 = getoutcode(x1, y1, xmin, ymin, xmax, ymax);

	do
	{
		if (!(outc0 | outc1))
		{
			/* trivial accept and exit */
			accept = TTRUE;
			done = TTRUE;
		}
		else if (outc0 & outc1)
		{
			/* trivial reject and exit */
			done = TTRUE;
		}
		else
		{
			/* calculate intersection with clipping edge */
			struct Coord r;
			struct Coord v1 = {x0, y0};
			struct Coord v2 = {x1, y1};
			struct Coord ce[2] = {{xmin, ymin}, {xmax, ymax}};

			outc = outc0 ? outc0 : outc1;
			intersect(&r, v1, v2, ce, outc);

			/* get ready for next pass */
			if (outc == outc0)
			{
				x0 = r.x; y0 = r.y;
				outc0 = getoutcode(x0, y0, xmin, ymin, xmax, ymax);
			}
			else
			{
				x1 = r.x; y1 = r.y;
				outc1 = getoutcode(x1, y1, xmin, ymin, xmax, ymax);
			}

		} /* endif subdivide */

	} while (done == TFALSE);

	if (accept)
	{
		res[0].x = x0; res[0].y = y0;
		res[1].x = x1; res[1].y = y1;
	}

	return accept;
}

/*****************************************************************************/

LOCAL void fbp_drawpoint(FBWINDOW *v, TINT x, TINT y, struct FBPen *pen)
{
	if (clippoint(x, y, v->fbv_ClipRect[0], v->fbv_ClipRect[1], v->fbv_ClipRect[2], v->fbv_ClipRect[3]))
		WritePixel(v, x, y, pen);
}

/*****************************************************************************/

LOCAL void fbp_drawfrect(FBWINDOW *v, TINT rect[4], struct FBPen *pen)
{
	TINT x, y;
	struct Coord res[2];
	TINT xmin = rect[0];
	TINT ymin = rect[1];
	TINT xmax = rect[0] + rect[2] - 1;
	TINT ymax = rect[1] + rect[3] - 1;

	if (!cliprect(res, xmin, ymin, xmax, ymax,
		v->fbv_ClipRect[0], v->fbv_ClipRect[1], v->fbv_ClipRect[2], v->fbv_ClipRect[3]))
		return;

	xmin = res[0].x; ymin = res[0].y; xmax = res[1].x; ymax = res[1].y;

	for (y = ymin; y <= ymax; y++)
		for (x = xmin; x <= xmax; x++)
			WritePixel(v, x, y, pen);
}

/*****************************************************************************/

LOCAL void fbp_drawrect(FBWINDOW *v, TINT rect[4], struct FBPen *pen)
{
	TINT x, y;
	struct Coord res[2];
	TINT xmin = rect[0];
	TINT ymin = rect[1];
	TINT xmax = rect[0] + rect[2] - 1;
	TINT ymax = rect[1] + rect[3] - 1;

	if (!cliprect(res, xmin, ymin, xmax, ymax,
		v->fbv_ClipRect[0], v->fbv_ClipRect[1], v->fbv_ClipRect[2], v->fbv_ClipRect[3]))
		return;

	if (ymin == res[0].y)
	{
		for (x = res[0].x; x <= res[1].x; x++)
			WritePixel(v, x, ymin, pen);
	}

	if (ymax == res[1].y)
	{
		for (x = res[0].x; x <= res[1].x; x++)
			WritePixel(v, x, ymax, pen);
	}

	if (xmin == res[0].x)
	{
		for (y = res[0].y; y <= res[1].y; y++)
			WritePixel(v, xmin, y, pen);
	}

	if (xmax == res[1].x)
	{
		for (y = res[0].y; y <= res[1].y; y++)
			WritePixel(v, xmax, y, pen);
	}
}

/*****************************************************************************/

LOCAL void fbp_drawline(FBWINDOW *v, TINT rect[4], struct FBPen *pen)
{
	struct Coord res[2];
  	TINT x0, y0, x1, y1;
	TINT dx, dy, d, x, y;
	TINT incE, incNE, incSE, incN, incS;
	TBOOL accept;

	if (rect[2] < rect[0])
	{
		/* swap points */
		accept = clipline(res, rect[2], rect[3], rect[0], rect[1],
			v->fbv_ClipRect[0], v->fbv_ClipRect[1], v->fbv_ClipRect[2], v->fbv_ClipRect[3]);
	}
	else
	{
		accept = clipline(res, rect[0], rect[1], rect[2], rect[3],
			v->fbv_ClipRect[0], v->fbv_ClipRect[1], v->fbv_ClipRect[2], v->fbv_ClipRect[3]);
	}

	if (!accept) return;

	x0 = res[0].x; y0 = res[0].y; x1 = res[1].x; y1 = res[1].y;

	dx = x1 - x0;
	dy = y1 - y0;

	x = x0;
	y = y0;

	WritePixel(v, x, y, pen);

	if ((y0 <= y1) && (dy <= dx))
	{
    	/* m <= 1 */
		d = 2 * dy - dx;
		incE = 2 * dy;
		incNE = 2 * (dy - dx);

		while (x < x1)
		{
			if (d <= 0)
			{
				d = d + incE;
				x++;
			}
			else
			{
				d = d + incNE;
				x++;
				y++;
			}

			WritePixel(v, x, y, pen);
		}
	}

	if ((y0 <= y1) && (dy > dx))
    {
    	/* m > 1 */
		d = 2 * dx - dy;
		incN = 2 * dx;
		incNE = 2 * (dx - dy);

		while (y < y1)
		{
			if (d <= 0)
			{
				d = d + incN;
				y++;
			}
			else
			{
				d = d + incNE;
				x++;
				y++;
			}

			WritePixel(v, x, y, pen);
		}
	}

	if ((y0 > y1) && (-dy <= dx))
	{
		dy = -dy;
		d = 2 * dy - dx;
		incE = 2 * dy;
		incSE = 2 * (dy - dx);

		while (x < x1)
		{
			if (d <= 0)
			{
				d = d + incE;
				x++;
			}
			else
			{
				d = d + incSE;
				x++;
				y--;
			}

			WritePixel(v, x, y, pen);
		}
	}

	if ((y0 > y1) && (-dy > dx))
	{
		dy = -dy;
		d = 2 * dx - dy;
		incS = 2 * dx;
		incSE = 2 * (dx - dy);

		while (y > y1)
		{
			if (d <= 0)
			{
				d = d + incS;
				y--;
			}
			else
			{
				d = d + incSE;
				x++;
				y--;
			}

			WritePixel(v, x, y, pen);
		}
	}
}

/*****************************************************************************/

static void
initslope(struct Slope *m, struct Coord *c1, struct Coord *c2)
{
	TINT dx = c2->x - c1->x;
	TINT dy = c2->y - c1->y;

	memset(m, 0x0, sizeof(struct Slope));
	m->S.x = c1->x;	m->S.y = c1->y;

	if (dx && dy)
	{
		if (abs(dx) >= dy)
		{
			/* calculate integer part */
			m->inta = dx/dy;
			m->z = dx - (m->inta * dy);
			if (m->z) m->n = dy;
		}
		else
		{
			/* dx < dy */
			m->z = dx;
			m->n = dy;
		}
	}
}

static void
hline(FBWINDOW *v, struct Slope *ms, struct Slope *me, TINT y,
	struct FBPen *pen)
{
	TINT xs, xe, rs = 0, re = 0;
	TINT x;

	/* check, if we need to round up coordinates */
	if (ms->n && abs(ms->cz)*2 >= ms->n)
		rs = 1;
	if (me->n && abs(me->cz)*2 >= me->n)
		re = 1;

	/* sort that xs < xe */
	if (ms->S.x+rs > me->S.x+re)
	{
		xs = me->S.x + re;
		xe = ms->S.x + rs;
	}
	else
	{
		xs = ms->S.x + rs;
		xe = me->S.x + re;
	}

	//printf("%d: %d -> %d\n", y, xs, xe);

	for (x = xs; x <= xe; x++)
		WritePixel(v, x, y, pen);
}

static void
rendertriangle(FBWINDOW *v, struct Coord v1, struct Coord v2,
	struct Coord v3, struct FBPen *pen)
{
	struct Coord A, B, C;
	struct Slope mAB, mAC, mBC;
	TINT y;

	/* sort that A.y <= B.y <= C.y */
	A.x = v1.x;	A.y = v1.y;
	B.x = v2.x;	B.y = v2.y;
	C.x = v3.x;	C.y = v3.y;

	if (A.y > v2.y)
	{
		B.x = A.x;	B.y = A.y;
		A.x = v2.x;	A.y = v2.y;
	}

	if (A.y > v3.y)
	{
		C.x = B.x;	C.y = B.y;
		B.x = A.x;	B.y = A.y;
		A.x = v3.x;	A.y = v3.y;
	}
	else
	{
		if (B.y > v3.y)
		{
			C.x = B.x;	C.y = B.y;
			B.x = v3.x;	B.y = v3.y;
		}
	}

	initslope(&mAB, &A, &B);
	initslope(&mAC, &A, &C);
	initslope(&mBC, &B, &C);

	for (y = A.y; y < B.y; y++, mAC.S.x += mAC.inta, mAB.S.x += mAB.inta)
	{
		hline(v, &mAC, &mAB, y, pen);

		if (mAC.n)
		{
			mAC.cz += mAC.z;
			if (mAC.z < 0)
			{
				if (mAC.cz < 0)
				{
					mAC.S.x -= 1;
					mAC.cz += mAC.n;
				}
			}
			else
			{
				if (mAC.cz >= mAC.n)
				{
					mAC.S.x += 1;
					mAC.cz -= mAC.n;
				}
			}
		}

		if (mAB.n)
		{
			mAB.cz += mAB.z;

			if (mAB.z < 0)
			{
				if (mAB.cz < 0)
				{
					mAB.S.x -= 1;
					mAB.cz += mAB.n;
				}
			}
			else
			{
				if (mAB.cz >= mAB.n)
				{
					mAB.S.x += 1;
					mAB.cz -= mAB.n;
				}
			}
		}
	}

	for (; y <= C.y; y++, mAC.S.x += mAC.inta, mBC.S.x += mBC.inta)
	{
		hline(v, &mAC, &mBC, y, pen);

		if (mAC.n)
		{
			mAC.cz += mAC.z;

			if (mAC.z < 0)
			{
				if (mAC.cz < 0)
				{
					mAC.S.x -= 1;
					mAC.cz += mAC.n;
				}
			}
			else
			{
				if (mAC.cz >= mAC.n)
				{
					mAC.S.x += 1;
					mAC.cz -= mAC.n;
				}
			}
		}

		if (mBC.n)
		{
			mBC.cz += mBC.z;

			if (mBC.z < 0)
			{
				if (mBC.cz < 0)
				{
					mBC.S.x -= 1;
					mBC.cz += mBC.n;
				}
			}
			else
			{
				if (mBC.cz >= mBC.n)
				{
					mBC.S.x += 1;
					mBC.cz -= mBC.n;
				}
			}
		}
	}
}

/*****************************************************************************/

LOCAL void
fbp_drawtriangle(FBWINDOW *v, TINT x0, TINT y0, TINT x1, TINT y1,
	TINT x2, TINT y2, struct FBPen *pen)
{
	struct Coord res[MAX_VERT];
	TINT outlen;

	if (!clippoly(res, &outlen, x0, y0, x1, y1, x2, y2,
		v->fbv_ClipRect[0], v->fbv_ClipRect[1], v->fbv_ClipRect[2], v->fbv_ClipRect[3]))
		return;

	if (outlen == 1)
	{
		WritePixel(v, res[0].x, res[0].y, pen);
	}
	else if (outlen == 2)
	{
		TINT rect[4] = {res[0].x, res[0].y, res[1].x, res[1].y};
		fbp_drawline(v, rect, pen);
	}
	else
	{
		TINT i;

		rendertriangle(v, res[0], res[1], res[2], pen);
		for (i = 2; i < outlen; i++)
		{
			if ((i+1) < outlen)
				rendertriangle(v, res[0], res[i], res[i+1], pen);
		}
	}
}

/*****************************************************************************/

LOCAL void fbp_drawbuffer(FBWINDOW *v, TUINT8 *buf, TINT rect[4], TINT totw)
{
	struct Coord res[2];
	TINT xmin = rect[0];
	TINT ymin = rect[1];
	TINT xmax = rect[0] + rect[2] - 1;
	TINT ymax = rect[1] + rect[3] - 1;
	TINT yd, xs, ys, numb;

	if (!cliprect(res, xmin, ymin, xmax, ymax,
		v->fbv_ClipRect[0], v->fbv_ClipRect[1], v->fbv_ClipRect[2], v->fbv_ClipRect[3]))
		return;

	xmin = res[0].x; ymin = res[0].y; xmax = res[1].x; ymax = res[1].y;
	xs = xmin-rect[0]; ys = ymin-rect[1];
	numb = (xmax - xmin + 1) * 4;

	for (yd = ymin; yd <= ymax; yd++, ys++)
		CopyLine(v, buf, totw, xs, ys, xmin, yd, numb);
}

/*****************************************************************************/

LOCAL void fbp_copyarea(FBWINDOW *v, TINT rect[4], TINT xd, TINT yd)
{
	TINT dx = xd - rect[0];
	TINT dy = yd - rect[1];
	TINT xmin = rect[0] + dx;
	TINT ymin = rect[1] + dy;
	TINT xmax = rect[0] + rect[2] - 1 + dx;
	TINT ymax = rect[1] + rect[3] - 1 + dy;
	struct Coord res[2];
	TINT y, numb;

	/* clip dest rect */
	if (!cliprect(res, xmin, ymin, xmax, ymax,
		v->fbv_ClipRect[0], v->fbv_ClipRect[1], v->fbv_ClipRect[2], v->fbv_ClipRect[3]))
		return;

	xmin = res[0].x - dx; ymin = res[0].y - dy;
	xmax = res[1].x - dx; ymax = res[1].y - dy;

	/* clip src rect */
	if (!cliprect(res, xmin, ymin, xmax, ymax,
		0,
		0,
		v->fbv_WinRect[2] - v->fbv_WinRect[0] + 1,
		v->fbv_WinRect[3] - v->fbv_WinRect[1] + 1))
		return;

	xmin = res[0].x; ymin = res[0].y; xmax = res[1].x; ymax = res[1].y;
	numb = (xmax - xmin + 1) * 4;

	if ((ymin + dx) < ymin)
	{
		for (y = ymin; y <= ymax; y++)
			CopyLineOver(v, xmin, y, xmin+dx, y+dy, numb);
	}
	else
	{
		for (y = ymax; y >= ymin; y--)
			CopyLineOver(v, xmin, y, xmin+dx, y+dy, numb);
	}
}

/*****************************************************************************/
