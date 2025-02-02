/*
**	$Id: truetype_mod.c,v 1.5 2004/05/29 06:49:59 fpagels Exp $
**
**	truetype font modul
*/

#include <math.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>

#include <tek/mod/truetype.h>
#include <tek/debug.h>
#include <tek/proto/exec.h>
#include <tek/proto/io.h>
#include <tek/proto/util.h>

#define MOD_VERSION		0
#define MOD_REVISION	2

typedef struct _TModTrue
{
	struct TModule module;			/* module header */
	TAPTR io;
	TAPTR util;
} TMOD_TRUETYPE;

#define TExecBase TGetExecBase(ttype)
#define TUtilBase ttype->util

#define step 0.25

#define read32bit(p) (((p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]))
#define read16bit(p) (((p[0]<<8)|p[1]))

/*
**	module prototypes
*/

static TCALLBACK TMOD_TRUETYPE *truetype_modopen(TMOD_TRUETYPE *ttype, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID truetype_modclose(TMOD_TRUETYPE *ttype, TAPTR task);

static TMODAPI TINT truetype_init(TMOD_TRUETYPE *ttype, TTFONT *ttfont, TSTRPTR font);
static TMODAPI TVOID truetype_close(TMOD_TRUETYPE *ttype, TTFONT *ttfont);
static TMODAPI TINT truetype_getchar(TMOD_TRUETYPE *ttype, TTFONT *ttfont, TINT8 c, TTVERT *vert);
static TMODAPI TVOID truetype_freevert(TMOD_TRUETYPE *ttype, TTVERT *vert);
static TMODAPI TINT truetype_getstring(TMOD_TRUETYPE *ttype, TTFONT *ttfont, TSTRPTR s, TTSTR *vert);
static TMODAPI TVOID truetype_freetstr(TMOD_TRUETYPE *ttype, TTSTR *tstr);

TINT truetype_handle_simple_glyf(TMOD_TRUETYPE *ttype, TTFONT *ttfont, TTF_GLYF *glyf, TTVERT *vert, TDOUBLE *orgmatrix);

/*
**	tek_init_<modname>
**	all initializations that are not instance specific.
**
*/

TMODENTRY TUINT tek_init_truetype(TAPTR selftask, TMOD_TRUETYPE *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)					/* first call */
		{
			if (version <= MOD_VERSION)			/* version check */
			{
				return sizeof(TMOD_TRUETYPE);		/* return module positive size */
			}
		}
		else									/* second call */
		{
			return sizeof(TAPTR)*6;			/* return module negative size */
		}
	}
	else										/* third call: init */
	{
		/* put module vectors in front */

		((TAPTR *) mod)[-1] = (TAPTR) truetype_init;
		((TAPTR *) mod)[-2] = (TAPTR) truetype_close;
		((TAPTR *) mod)[-3] = (TAPTR) truetype_getchar;
		((TAPTR *) mod)[-4] = (TAPTR) truetype_freevert;
		((TAPTR *) mod)[-5] = (TAPTR) truetype_getstring;
		((TAPTR *) mod)[-6] = (TAPTR) truetype_freetstr;

		/* init */

		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;
		mod->module.tmd_OpenFunc = (TMODOPENFUNC) truetype_modopen;
		mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) truetype_modclose;
		return TTRUE;
	}

	return 0;
}

static TCALLBACK TMOD_TRUETYPE *truetype_modopen(TMOD_TRUETYPE *ttype, TAPTR task, TTAGITEM *tags)
{
	ttype->io = TExecOpenModule(TExecBase, "io", 0, TNULL);
	if(ttype->io)
	{
		ttype->util = TExecOpenModule(TExecBase, "util", 0, TNULL);
		if(ttype->util)
			return ttype;
		else
			TExecCloseModule(TExecBase, ttype->io);
	}
	return TNULL;
		
}

static TCALLBACK TVOID truetype_modclose(TMOD_TRUETYPE *ttype, TAPTR task)
{
	if(ttype->io)
		TExecCloseModule(TExecBase, ttype->io);

	if(ttype->util)
		TExecCloseModule(TExecBase, ttype->util);
}


/*
**
**	make a bezier curve from 3 points
**
*/
TVOID truetype_makearc(TMOD_TRUETYPE *ttype,
	TDOUBLE x1, TDOUBLE y1,
	TDOUBLE x2, TDOUBLE y2,
	TDOUBLE x3, TDOUBLE y3,
	TTOUTL *out, TDOUBLE st)
{
	TINT i;
	TDOUBLE t;
	
	i = out->numpoint;
	
	t = st;

	out->points[i].x = x1;
	out->points[i].y = y1;
	i++;

	while(t<1.0)
	{
		out->points[i].x = x1+(2*(x2-x1)+(x1-2*x2+x3)*t)*t;
		out->points[i].y = y1+(2*(y2-y1)+(y1-2*y2+y3)*t)*t;
		i++;
		t += st;
	}

	out->points[i].x = x3;
	out->points[i].y = y3;
	i++;
	out->numpoint = i;
}

/*
**
**	check if outline is filled
**
*/
#if 0
TBOOL truetype_isclockwise(TINT16 *x, TINT16 *y, TINT n)
{
     /* first find rightmost lowest vertex of the polygon */
	TINT rmin = 0;
     TINT xmin = x[0];
     TINT ymin = y[0];
 	TINT i;
	
	for (i=1; i<n; i++)
	{
		if (y[i] > ymin)
			continue;
		if (y[i] == ymin)
		{    
			if (x[i]< xmin)
                 continue;
		}
		rmin = i;          /* a new rightmost lowest vertex */
		xmin = x[i];											/* !!!!!!!!!!!!!!!!! check rmin 
		ymin = y[i];												possible out of array error */
	}
 
	if((rmin+1) == n)
		printf("CLOCKWISE RMIN ERROR !!!!!!!!!!!!!!!!!\n");
	
	/* test orientation at this rmin vertex */
	/* ccw <=> the edge leaving is left of the entering edge */
	if (rmin == 0)
		return ( ((x[0] - x[n-1]) * (y[1] - y[n-1]) - (x[1] - x[n-1]) * (y[0] - y[n-1])) < 0 ? 1 : 0 );
     else
		return ( ((x[rmin] - x[rmin-1]) * (y[rmin+1] - y[rmin-1]) - (x[rmin+1] - x[rmin-1]) * (y[rmin] - y[rmin-1])) < 0 ? 1 : 0 );
}
#endif

#if 1
TBOOL truetype_isclockwise(TINT16 *x, TINT16 *y, TINT N)
{
	TINT i,j;
	TINT area = 0;

	for (i=0;i<N;i++)
	{
		j = (i + 1) % N;
		area += x[i] * y[j];
		area -= y[i] * x[j];
	}

	return (TBOOL) (area < 0 ? TTRUE : TFALSE);
}
#endif

#if 0
#define AY_EPSILON 1.0e-06
TBOOL
truetype_isclockwise(TINT16 *x, TINT16 *y, TINT n)
{
 double minx, miny;
 int i, j, m, stride, found, wrap;
 double *cv = NULL, a[2], b[2], c[2];
	int fill;
 
  minx = x[0];
  miny = y[0];

  j = 0;
  m = 0;
  for(i = 0; i < n-1; i++)
    {
      if((y[j] < miny) || ((y[j] == miny) && (x[j] < minx)))
	{
	  minx = x[j];
	  miny = y[j];
	  m = i+1;
	} /* if */

      j += 1;
    } /* for */

  if(m >= n)
    m = 0;

  found = TFALSE;
  wrap = 0;
  j = m;
  while(!found && (wrap<2))
    {
      if((fabs(x[m] - x[j]) > AY_EPSILON) &&
	 (fabs(y[m] - y[j]) > AY_EPSILON))
	{
	  found = TTRUE;
	}
      else
	{
	  if(j<(n-1))
	    {
	      j++;
	    }
	  else
	    {
	      j = 0;
	      wrap++;
	    }
	}
    } /* while */

  if(found)
    {
      c[0] = x[j];
      c[1] = y[j];
    }
  else
    {
  //    (AY_ERROR, fname, "Could not determine 3 different points!\n");
      return 0;
    }

  found = TFALSE;
  wrap = 0;
  j = m;
  while(!found && (wrap<2))
    {
      if((fabs(x[m] != x[j]) > AY_EPSILON) &&
	 (fabs(y[m] != y[j]) > AY_EPSILON))
	{
	  found = TTRUE;
	}
      else
	{
	  if(j > 0)
	    {
	      j--;
	    }
	  else
	    {
	      j = n-1;
	      wrap++;
	    }
	}
    } /* while */

  if(found)
    {
      a[0] = x[j];
      a[1] = y[j];
    }
  else
    {
//      ay_error(AY_ERROR, fname, "Could not determine 3 different points!\n");
      return 0;
    }

  b[0] = x[m];
  b[1] = y[m];


  fill = a[0] * b[1] - a[1] * b[0] +
            a[1] * c[0] - a[0] * c[1] +
            b[0] * c[1] - c[0] * b[1];

  printf("fill %d\n",fill);
  /*
  printf("%g %g|%g %g|%g %g => %g\n",
	 a[0], a[1], b[0], b[1], c[0], c[1], *orient);
  */

 return (fill < 0 ? 0 : 1);
} /* ay_nct_getorientation */

#endif

static double f2dot14(TMOD_TRUETYPE *ttype, short x)
{
	short   y = TUtilNToHS(TUtilBase, x);
	return (y >> 14) + ((y & 0x3fff) / 16384.0);
}

/* limit the recursion level to avoid cycles */
#define MAX_COMPOSITE_LEVEL 20

TINT truetype_handle_comp_glyf(TMOD_TRUETYPE *ttype, TTFONT *ttfont,
	 TTF_GLYF *glyf, double *orgmatrix, int level, TTVERT *vert)
{

	int		len;
	short	ncontours;
	TUINT16	flagbyte, glyphindex;
	double	arg1, arg2;
	TUINT8	*ptr;
	char		*bptr;
	TINT16	*sptr;
	double	matrix[6], newmatrix[6];
	TINT		go;

	if(ttfont->locformat)
	{
		len = TUtilNToHL(TUtilBase, ttfont->loca[ttfont->nglyf + 1]) -
			TUtilNToHL(TUtilBase, ttfont->loca[ttfont->nglyf]);
	} else
	{
		len = (TUtilNToHS(TUtilBase, ttfont->loca[ttfont->nglyf + 1]) - 
			TUtilNToHS(TUtilBase, ttfont->loca[ttfont->nglyf])) << 1;
	}


	if(ttfont->locformat)
	{						/* long format */
		TUINT *lo;
		
		lo = (TUINT*)ttfont->loca;
		
		len = TUtilNToHL(TUtilBase,  lo[ttfont->nglyf+1])-TUtilNToHL(TUtilBase,  lo[ttfont->nglyf]);
	}
	else
	{
		TUINT16 *lo;
		
		lo = (TUINT16*)ttfont->loca;
		
		len = (TUtilNToHL(TUtilBase,  lo[ttfont->nglyf+1])-TUtilNToHL(TUtilBase,  lo[ttfont->nglyf])) << 1;
	}


	if(len<=0) /* nothing to do */
		return 0;

	ncontours = TUtilNToHS(TUtilBase, glyf->numberOfContours);
	if (ncontours >= 0) { /* simple case */
		//draw_simple_glyf(g, glyph_list, glyphno, orgmatrix);
		truetype_handle_simple_glyf(ttype, ttfont, glyf, vert, orgmatrix);
		return 0;
	}

	/* complex case */
	if(level >= MAX_COMPOSITE_LEVEL) {

	/*	fprintf(stderr,"*** Glyph: stopped (possibly infinite) recursion at depth %d\n",level); */
		return 0;
	}

	ptr = ((TUINT8 *) glyf + sizeof(TTF_GLYF));
	sptr = (TINT16 *) ptr;
	do {
		flagbyte = TUtilNToHS(TUtilBase, *sptr);
		sptr++;
		glyphindex = TUtilNToHS(TUtilBase, *sptr);
		sptr++;

		if (flagbyte & ARG_1_AND_2_ARE_WORDS) {
			arg1 = (TINT16)TUtilNToHS(TUtilBase, *sptr);
			sptr++;
			arg2 = (TINT16)TUtilNToHS(TUtilBase, *sptr);
			sptr++;
		} else {
			bptr = (char *) sptr;
			arg1 = (signed char) bptr[0];
			arg2 = (signed char) bptr[1];
			sptr++;
		}
		matrix[1] = matrix[2] = 0.0;

		if (flagbyte & WE_HAVE_A_SCALE) {
			matrix[0] = matrix[3] = f2dot14(ttype,*sptr);
			sptr++;
		} else if (flagbyte & WE_HAVE_AN_X_AND_Y_SCALE) {
			matrix[0] = f2dot14(ttype,*sptr);
			sptr++;
			matrix[3] = f2dot14(ttype,*sptr);
			sptr++;
		} else if (flagbyte & WE_HAVE_A_TWO_BY_TWO) {
			matrix[0] = f2dot14(ttype,*sptr);
			sptr++;
			matrix[2] = f2dot14(ttype,*sptr);
			sptr++;
			matrix[1] = f2dot14(ttype,*sptr);
			sptr++;
			matrix[3] = f2dot14(ttype,*sptr);
			sptr++;
		} else {
			matrix[0] = matrix[3] = 1.0;
		}

		/*
		 * See *
		 * http://fonts.apple.com/TTRefMan/RM06/Chap6g
		 * lyf.html * matrix[0,1,2,3,4,5]=a,b,c,d,m,n
		 */

		if (fabs(matrix[0]) > fabs(matrix[1]))
			matrix[4] = fabs(matrix[0]);
		else
			matrix[4] = fabs(matrix[1]);
		if (fabs(fabs(matrix[0]) - fabs(matrix[2])) <= 33. / 65536.)
			matrix[4] *= 2.0;

		if (fabs(matrix[2]) > fabs(matrix[3]))
			matrix[5] = fabs(matrix[2]);
		else
			matrix[5] = fabs(matrix[3]);
		if (fabs(fabs(matrix[2]) - fabs(matrix[3])) <= 33. / 65536.)
			matrix[5] *= 2.0;

		/*
		 * fprintf (stderr,"Matrix Opp %hd
		 * %hd\n",arg1,arg2);
		 */
#if 0
		fprintf(stderr, "Matrix: %f %f %f %f %f %f\n",
		 matrix[0], matrix[1], matrix[2], matrix[3],
			matrix[4], matrix[5]);
		fprintf(stderr, "Offset: %f %f (%s)\n",
			arg1, arg2,
			((flagbyte & ARGS_ARE_XY_VALUES) ? "XY" : "index"));
#endif

		if (flagbyte & ARGS_ARE_XY_VALUES) {
			matrix[4] *= arg1;
			matrix[5] *= arg2;
		} else {
	/*		fprintf(stderr, 
				"*** Glyph:  reusing scale from another glyph is unsupported\n");  */
			/*
			 * must extract values from a glyph
			 * but it seems to be too much pain
			 * and it's not clear now that it
			 * would be really used in any
			 * interesting font
			 */
		}

		/* at this point arg1,arg2 contain what logically should be matrix[4,5] */

		/* combine matrices */

		newmatrix[0] = orgmatrix[0]*matrix[0] + orgmatrix[2]*matrix[1];
		newmatrix[1] = orgmatrix[0]*matrix[2] + orgmatrix[2]*matrix[3];

		newmatrix[2] = orgmatrix[1]*matrix[0] + orgmatrix[3]*matrix[1];
		newmatrix[3] = orgmatrix[1]*matrix[2] + orgmatrix[3]*matrix[3];

		newmatrix[4] = orgmatrix[0]*matrix[4] + orgmatrix[2]*matrix[5] + orgmatrix[4];
		newmatrix[5] = orgmatrix[1]*matrix[4] + orgmatrix[3]*matrix[5] + orgmatrix[5];


		ttfont->nglyf = glyphindex;			/* glyf number in Font */

		if(ttfont->locformat)
		{						/* long format */
			TUINT *lo;
		
			lo = (TUINT*)ttfont->loca;
		
			go = TUtilNToHL(TUtilBase,  lo[glyphindex]);
		}
		else
		{
			TUINT16 *lo;
		
			lo = (TUINT16*)ttfont->loca;
		
			go = TUtilNToHS(TUtilBase,  lo[glyphindex]) * 2;
		}

		glyf = (TTF_GLYF*)(ttfont->glyf+go);


		truetype_handle_comp_glyf(ttype, ttfont, glyf, newmatrix, level+1, vert);

	} while (flagbyte & MORE_COMPONENTS);

	return 0;

}


TINT truetype_handle_simple_glyf(TMOD_TRUETYPE *ttype, TTFONT *ttfont, TTF_GLYF *glyf, TTVERT *vert, TDOUBLE *matrix)
{
	TUINT16 *contour_end_pt;
	TINT16	ncontours, n_inst, last_point,oldcontour;
	TUINT8	*ptr;
#define GLYFSZ	2000
	TINT16    xabs[GLYFSZ], yabs[GLYFSZ], xrel[GLYFSZ], yrel[GLYFSZ];
	TUINT8    flags[GLYFSZ] = {0};
	TINT16	j,k,k1,co,lp,lsb,startx,yoffset;
	TDOUBLE	scale_factor;
	TDOUBLE	tx, ty;
	TINT gly;
		
	ncontours = TUtilNToHS(TUtilBase,  glyf->numberOfContours);
	contour_end_pt = (TUINT16 *) ((TUINT8 *) glyf + sizeof(TTF_GLYF));

	oldcontour = vert->outlines;
	vert->outlines += ncontours;
	
	if(vert->outl) vert->outl = (TTOUTL *)TExecRealloc(TExecBase, vert->outl, sizeof(TTOUTL)*vert->outlines);
	else vert->outl = (TTOUTL *)TExecAlloc0(TExecBase, TNULL, sizeof(TTOUTL)*vert->outlines);

	last_point = TUtilNToHS(TUtilBase, contour_end_pt[ncontours - 1]);
	n_inst = TUtilNToHS(TUtilBase,  contour_end_pt[ncontours]);

	ptr = ((TUINT8 *) contour_end_pt) + (ncontours << 1) + n_inst + 2;

	j = k = 0;
	while (k <= last_point)
	{
		flags[k] = ptr[j];

		if (ptr[j] & REPEAT)
		{
			for (k1 = 0; k1 < ptr[j + 1]; k1++)
			{
				k++;
				flags[k] = ptr[j];
			}
			j++;
		}
		j++;
		k++;
	}

	for (k = 0; k <= last_point; k++)
	{
		if (flags[k] & XSHORT)
		{
			if (flags[k] & XSAME)
			{
				xrel[k] = ptr[j];
			} else
			{
				xrel[k] = -ptr[j];
			}
			j++;
		} else
		if (flags[k] & XSAME)
		{
			xrel[k] = 0.0;
		} else {
			xrel[k] = (short)( ptr[j] * 256 + ptr[j + 1] );
			j += 2;
		}
		if (k == 0)
		{
			xabs[k] = xrel[k];
		} else
		{
			xabs[k] = xrel[k] + xabs[k - 1];
		}

	}

	for (k = 0; k <= last_point; k++)
	{
		if (flags[k] & YSHORT)
		{
			if (flags[k] & YSAME)
			{
				yrel[k] = ptr[j];
			} else
			{
				yrel[k] = -ptr[j];
			}
			j++;
		} else if (flags[k] & YSAME)
		{
			yrel[k] = 0;
		} else
		{
			yrel[k] = ptr[j] * 256 + ptr[j + 1];
			j += 2;
		}
		if (k == 0)
		{
			yabs[k] = yrel[k];
		} else
		{
			yabs[k] = yrel[k] + yabs[k - 1];
		}
	}


/*	scale_factor = 2000.0 / (TDOUBLE) ttfont->unitem; */
	scale_factor = 1.0;

	if (matrix)
	{
		TINT i;
		
		for (i = 0; i <= last_point; i++)
		{
			tx = xabs[i];
			ty = yabs[i];
			xabs[i] = scale_factor*(matrix[0] * tx + matrix[2] * ty + matrix[4]);
			yabs[i] = scale_factor*(matrix[1] * tx + matrix[3] * ty + matrix[5]);
		}
	} else
	{
		TINT i;
		
		for (i = 0; i <= last_point; i++)
		{
			xabs[i] = scale_factor*(xabs[i]);
			yabs[i] = scale_factor*(yabs[i]);
		}
	}

	
	k = 0;
	lp = 0;
	gly = ttfont->nglyf < ttfont->numberOfHMetrics ? ttfont->nglyf : 0;
	lsb = TUtilNToHS(TUtilBase, ttfont->hmtx[gly].lsb);
	startx = ttfont->soffset;
	yoffset = ttfont->yoffset;
		
	for(co = oldcontour; co < (ncontours+oldcontour); co++)
	{
		TINT i,maxpoint;
		TDOUBLE x1 = 0.0, y1 = 0.0, x2,y2,x3,y3;
		
		last_point = TUtilNToHS(TUtilBase, contour_end_pt[co-oldcontour]);

		maxpoint = 2000;
		
		vert->outl[co].points = (TTPOINT *)TExecAlloc0(TExecBase, TNULL, sizeof(TTPOINT)*(maxpoint));
		vert->outl[co].numpoint = 0;
		lp = last_point+1;
		
		i = 0;

		vert->outl[co].filled = truetype_isclockwise(&xabs[k],&yabs[k],(last_point-k));
		
		while(k <=last_point)
		{
			if(flags[k]&ONOROFF)
			{
				
				x1 = (xabs[k]-lsb+startx)*scale_factor;
				y1 = (yabs[k]-ttfont->descent+yoffset)*scale_factor;
					
				if(k==last_point)
				{
					i = vert->outl[co].numpoint;
	
					vert->outl[co].points[i].x = x1;
					vert->outl[co].points[i].y = y1;
					vert->outl[co].numpoint++;
					
					vert->outl[co].points[i+1].x = vert->outl[co].points[0].x;
					vert->outl[co].points[i+1].y = vert->outl[co].points[0].y;
					vert->outl[co].numpoint++;
					
				}
		
				if((flags[k+1]&ONOROFF)&&((k+1)<=last_point))
				{
					i = vert->outl[co].numpoint;

					vert->outl[co].points[i].x = x1;
					vert->outl[co].points[i].y = y1;
					vert->outl[co].numpoint++;
				}

				k++;
			}
			else
			{
				if(k==last_point)
				{
					x2 = (xabs[k]-lsb+startx)*scale_factor;
					y2 = (yabs[k]-ttfont->descent+yoffset)*scale_factor;
					
					x3 = vert->outl[co].points[0].x;
					y3 = vert->outl[co].points[0].y;
		
					truetype_makearc(ttype, x1, y1, x2, y2, x3, y3, &vert->outl[co], step);
		
					k++;
				}
				else
				{
					if(flags[k+1]&ONOROFF)
					{
						x2 = (xabs[k]-lsb+startx)*scale_factor;
						y2 = (yabs[k]-ttfont->descent+yoffset)*scale_factor;
					
						x3 = (xabs[k+1]-lsb+startx)*scale_factor;
						y3 = (yabs[k+1]-ttfont->descent+yoffset)*scale_factor;
					
						truetype_makearc(ttype, x1, y1, x2, y2, x3, y3, &vert->outl[co], step);
						k += 2;
	
						x1 = x3;
						y1 = y3;
		
						if(k-1==last_point)
						{
							i = vert->outl[co].numpoint;
	
							vert->outl[co].points[i].x = vert->outl[co].points[0].x;
							vert->outl[co].points[i].y = vert->outl[co].points[0].y;
							vert->outl[co].numpoint++;
						}
					}
					else
					{
						x2 = (xabs[k]-lsb+startx)*scale_factor;
						y2 = (yabs[k]-ttfont->descent+yoffset)*scale_factor;
					
						x3 = (xabs[k+1]-lsb+startx)*scale_factor;
						y3 = (yabs[k+1]-ttfont->descent+yoffset)*scale_factor;
				
						x3 = (x2+x3)/2;		/* implicit on curve point */
						y3 = (y2+y3)/2;
	
						truetype_makearc(ttype, x1, y1, x2, y2, x3, y3, &vert->outl[co], step);
						
						k += 1;
						x1 = x3;
						y1 = y3;
					} /* end else*/
				
				} /* end else */
				
			} /* end else */

		} /* end while */

		vert->outl[co].points = TExecRealloc(TExecBase, vert->outl[co].points,(sizeof(TTPOINT)*vert->outl[co].numpoint+1));		

	} /* end for */

	return 0;
}

TVOID truetype_read_head(TMOD_TRUETYPE *ttype, TTFONT *ttfont)
{
	
	TUINT8 *buffer,*tmp;
	
	buffer = TExecAlloc0(TExecBase, TNULL, ttfont->size);
	
	io_fread(ttype->io, ttfont->fontptr, buffer, ttfont->size);
	
	tmp = buffer+16;
	ttfont->flags = read16bit(tmp);
	tmp = buffer+18;
	ttfont->unitem = read16bit(tmp);
	tmp = buffer+50;
	ttfont->locformat = read16bit(tmp);
	
	TExecFree(TExecBase, buffer);

}


TBOOL truetype_searchtable(TMOD_TRUETYPE *ttype, TTFONT *ttfont, TSTRPTR tablename)
{
	TINT count = 0;
	TUINT8 dummy[5];
	TBOOL success;
	
	io_seek(ttype->io, ttfont->fontptr, 12, TNULL, TFPOS_BEGIN);
	
	dummy[4] = 0;
	success = TFALSE;
	
	while(count < ttfont->numtable && !success)
	{
		io_fread(ttype->io, ttfont->fontptr, dummy, 4);

		if(!(TUtilStrCmp(TUtilBase, tablename,dummy)))
				success=TTRUE;
		else
			io_seek(ttype->io, ttfont->fontptr, 12, TNULL, TFPOS_CURRENT);
			
		count++;	
	}	

	if(success)
	{
		TUINT8 *dummy;
		TINT offset;
		
		io_fread(ttype->io, ttfont->fontptr, ttfont->buffer, 12);
		
		dummy = ttfont->buffer+4;
		offset = read32bit(dummy);
		dummy += 4;

		ttfont->size = read32bit(dummy);

		io_seek(ttype->io, ttfont->fontptr, offset, TNULL, TFPOS_BEGIN);

	}	

	return success;
}


/*
**
** find glyf index 
**
**/
TINT truetype_findglyf(TMOD_TRUETYPE *ttype,TTF_CMAP4 *cm, TINT c)
{
	TINT i,seg_c2,cmap_n_segs,n;
	TINT8 *ptr;
	TUINT16 *cmap_seg_end, *cmap_seg_start;
	TINT16 *cmap_idDelta, *cmap_idRangeOffset,ro,delta;
	
	
	if(TUtilNToHS(TUtilBase, cm->format) == 4)
	{
		TBOOL found;

		seg_c2 = TUtilNToHS(TUtilBase, cm->segCountX2);
		cmap_n_segs = seg_c2 >> 1;
		ptr = (TINT8 *) cm + 14;
		cmap_seg_end = (TUINT16 *) ptr;
		cmap_seg_start = (TUINT16 *) (ptr + seg_c2 + 2);
		cmap_idDelta = (TINT16 *) (ptr + (seg_c2 * 2) + 2);
		cmap_idRangeOffset = (TINT16 *) (ptr + (seg_c2 * 3) + 2);
		/* search range for code */
		
		i=-1;
		found = TFALSE;
		do
		{
			i++;
			if(TUtilNToHS(TUtilBase, cmap_seg_end[i]) >= c)
				found = TTRUE;
				
		}while((i<cmap_n_segs)&&(!found));
	
		if(TUtilNToHS(TUtilBase, cmap_seg_start[i])>c)		
			return 0;			/* glyf not found */
			
		ro = TUtilNToHS(TUtilBase, cmap_idRangeOffset[i]);
		delta = TUtilNToHS(TUtilBase, cmap_idDelta[i]);
		
		if(ro==0)
			n = c + delta;
		else
		{
			n = TUtilNToHS(TUtilBase, *((ro>>1) + 
					(c - TUtilNToHS(TUtilBase, cmap_seg_start[i])) + &cmap_idRangeOffset[i]));		
		}

		return n;
	}

	return 0;
}

/*
**
** get wanted glyf
**
**/
TVOID truetype_getglyf(TMOD_TRUETYPE *ttype, TTFONT *ttfont, TUINT c, TTF_GLYF **glyf)
{
	TINT		i,k,gl, go, format,platform;
	TBOOL	b;
	TTF_CMAP_ENTRY *cme = TNULL;
	TTF_CMAP4	*cm4 = TNULL;


	/* number of tables in font */
	k = TUtilNToHS(TUtilBase,  ttfont->cmap->numberOfEncodingTables);

	i = 0;
	b = TFALSE;

	while(i<k && !b)
	{
		cme = &(ttfont->cmap->encodingTable[i]);
		format = TUtilNToHS(TUtilBase,  cme->encodingID);
		platform = TUtilNToHS(TUtilBase,  cme->platformID);
		if(format == 1 && platform == 3)			/* CMAP4 */
		  b = TTRUE;

		i++; 
	}


	k = TUtilNToHL(TUtilBase,  cme->offset);		/* table offset */

	cm4 = (TTF_CMAP4 *)((TUINT8 *)ttfont->cmap + k);

	gl = truetype_findglyf(ttype, cm4, (TINT)c); /* find glyf */
	
	ttfont->nglyf = gl;			/* glyf number in Font */
	
	if(ttfont->locformat)
	{						/* long format */
		TUINT *lo;
		
		lo = (TUINT*)ttfont->loca;

		go = TUtilNToHL(TUtilBase,  lo[gl]);
	}
	else
	{
		TUINT16 *lo;
		
		lo = (TUINT16*)ttfont->loca;
		
		go = TUtilNToHS(TUtilBase,  lo[gl]) * 2;
	}

	/* printf("Glyf offset: %x\n",go); */

	*glyf = (TTF_GLYF*)(ttfont->glyf+go);

}


static TMODAPI TINT 
truetype_getstring(TMOD_TRUETYPE *ttype, TTFONT *ttfont, TSTRPTR s, TTSTR *tstr)
{
	TINT		i,sl,lines,o;
	TTF_GLYF *glyf = TNULL;
	TUINT8 c;
	TDOUBLE scale,tw;
	TINT16 	out;
	TINT16 w,l,xmin,xmax;
	TINT16 pp1;
	TSTRPTR tmpstr;
	TDOUBLE  matrix[6];

	sl = TUtilStrLen(TUtilBase, s);
	tstr->numchar=sl;
		
	tmpstr = s;
	lines = 0;
	/* check for newlines */
	for(i=0; i<sl; i++)
	{
		if(*tmpstr=='\n')
			lines++;
		
		tmpstr++;
	}
	
	tstr->verts = (TTVERT*)TExecAlloc0(TExecBase, TNULL, sizeof(TTVERT)*sl);
	ttfont->soffset = 0;
	ttfont->yoffset = (ttfont->ascent - ttfont->descent)*lines;
	tstr->maxpoints = 0;
	
/*	scale = 1000.0 / (TDOUBLE) ttfont->unitem; */
	scale = 1.0;
	
	for(i=0; i<sl; i++)
	{
		c = *s;
		
		if(c == '\n')
		{
			ttfont->yoffset -= (ttfont->ascent - ttfont->descent);
			ttfont->soffset = 0;
			s += 1;
			i += 1;
			c = *s;
		}

		
		s++;
	
		truetype_getglyf(ttype, ttfont, (TUINT)c, &glyf);
	
		out = TUtilNToHS(TUtilBase,  glyf->numberOfContours);
		
		matrix[0] = matrix[3] = 1.0;
		matrix[1] = matrix[2] = matrix[4] = matrix[5] = 0.0;

		if(out<0)
		{
			truetype_handle_comp_glyf(ttype, ttfont, glyf, matrix, 0 /* level */, &tstr->verts[i]);
		}
		else
			truetype_handle_simple_glyf(ttype, ttfont, glyf, &tstr->verts[i], matrix);
		
		for(o=0; o<tstr->verts[i].outlines; o++)
		{
			if(tstr->verts[i].outl[o].numpoint > tstr->maxpoints)
				tstr->maxpoints = tstr->verts[i].outl[o].numpoint;
		}

		/* new start for next glyf */
	/*	pp1 = TUtilNToHS(TUtilBase, glyf->xMin) - TUtilNToHS(TUtilBase, ttfont->hmtx[ttfont->nglyf].lsb); */
		
		if(ttfont->numberOfHMetrics <= 3)
			ttfont->soffset += (TUtilNToHS(TUtilBase, ttfont->hmtx[0].advanceWidth));
		else
			ttfont->soffset += (TUtilNToHS(TUtilBase, ttfont->hmtx[ttfont->nglyf].advanceWidth));
	
		
		xmin = TUtilNToHS(TUtilBase, glyf->xMin);
		xmax = TUtilNToHS(TUtilBase, glyf->xMax);
		
/*		printf("xmin: %d  xmax: %d\n",xmin,xmax); */
		
		if(ttfont->numberOfHMetrics <= 3)
		{
			w = TUtilNToHS(TUtilBase, ttfont->hmtx[0].advanceWidth);
			l = TUtilNToHS(TUtilBase, ttfont->hmtx[0].lsb);
		}else
		{
			w = TUtilNToHS(TUtilBase, ttfont->hmtx[ttfont->nglyf].advanceWidth);
			l = TUtilNToHS(TUtilBase, ttfont->hmtx[ttfont->nglyf].lsb);
		}
	/*	printf("rsb %d\n",w-(l+xmax-xmin)); */
	
//	ttfont->soffset += (w-(l+xmax-xmin)); 		/* courier ???? */

/*	printf("width %d  lsb %d\n",w, l); */
	
	tw = ttfont->soffset*scale;
	
	if(tstr->width<tw)
		tstr->width=tw;

	}

	tstr->height = ((ttfont->ascent - ttfont->descent)*(lines+1))*scale;

//	tstr->width= 220;
	return 0;
}

static TMODAPI TINT 
truetype_getchar(TMOD_TRUETYPE *ttype, TTFONT *ttfont, TINT8 c, TTVERT *vert)
{
	TINT16		out;
	TTF_GLYF *glyf = TNULL;
	TDOUBLE  matrix[6];

	ttfont->soffset = 0;
	ttfont->yoffset = 0;

	truetype_getglyf(ttype, ttfont, c, &glyf);
	
	out = TUtilNToHS(TUtilBase,  glyf->numberOfContours);
	vert->outlines = 0;
	
	matrix[0] = matrix[3] = 1.0;
	matrix[1] = matrix[2] = matrix[4] = matrix[5] = 0.0;

	if(out<0)
	{
		truetype_handle_comp_glyf(ttype, ttfont, glyf, matrix, 0 /* level */, vert);
	}
	else
		truetype_handle_simple_glyf(ttype, ttfont, glyf, vert, matrix);

	if(ttfont->numberOfHMetrics <= 3)
		vert->width = (TUtilNToHS(TUtilBase, ttfont->hmtx[0].advanceWidth));
	else
		vert->width = (TUtilNToHS(TUtilBase, ttfont->hmtx[ttfont->nglyf].advanceWidth));
	
	vert->height = (ttfont->ascent - ttfont->descent);
	
	return 0;
}

static TMODAPI TINT 
truetype_init(TMOD_TRUETYPE *ttype, TTFONT *ttfont, TSTRPTR font)
{
	TINT tfontnamelen;
	TSTRPTR tfontname;
	
	tfontnamelen = io_makename(ttype->io, font, TNULL, 0, TPPF_HOST2TEK, TNULL);
	
	if(tfontnamelen != -1)
	{
		tfontname = TExecAlloc0(TExecBase, TNULL, tfontnamelen+1);
		
		if(tfontname)
		{
			io_makename(ttype->io, font, tfontname, tfontnamelen+1, TPPF_HOST2TEK, TNULL);
			ttfont->fontptr = io_open(ttype->io, tfontname, TFMODE_READONLY, TNULL);
			TExecFree(TExecBase, tfontname);
		}
	}

	if(ttfont->fontptr)
	{
		TUINT8 *dummy;
		TINT idummy;
		TTF_DIRECTORY *tdir;
		TTF_HHEA *hhea;
				
		/* check if we have a truetype font */
			
		ttfont->buffer = TExecAlloc0(TExecBase, TNULL, 16);
		dummy = ttfont->buffer;
			
		io_fread(ttype->io, ttfont->fontptr, ttfont->buffer, 16);

		tdir = (TTF_DIRECTORY *) ttfont->buffer;
		idummy = TUtilNToHL(TUtilBase, tdir->sfntVersion);
		
		if((idummy == 0x00010000)||(idummy == 0x74727565))
		{
			ttfont->numtable = 0;
			ttfont->numtable = TUtilNToHS(TUtilBase, tdir->numTables);

			/* search head table */
				
			if(!(truetype_searchtable(ttype, ttfont, "head")))
				return TTE_BADFONT;
			
			truetype_read_head(ttype, ttfont);				

			/* search cmap table */

			if(!(truetype_searchtable(ttype, ttfont, "cmap")))
				return TTE_BADFONT;

			ttfont->cmap = (TTF_CMAP *)TExecAlloc0(TExecBase, TNULL, ttfont->size);
			io_fread(ttype->io, ttfont->fontptr, (TUINT8 *) ttfont->cmap, ttfont->size);

			/* search loca table */

			if(!(truetype_searchtable(ttype, ttfont, "loca")))
				return TTE_BADFONT;

			ttfont->loca = TExecAlloc0(TExecBase, TNULL, ttfont->size);
			io_fread(ttype->io, ttfont->fontptr, ttfont->loca, ttfont->size);

			/* search glyf table */

			if(!(truetype_searchtable(ttype, ttfont, "glyf")))
				return TTE_BADFONT;

			ttfont->glyf = TExecAlloc0(TExecBase, TNULL, ttfont->size);
			io_fread(ttype->io, ttfont->fontptr, ttfont->glyf, ttfont->size);
				

			/* search hhea table */

			if(!(truetype_searchtable(ttype, ttfont, "hhea")))
				return TTE_BADFONT;

			hhea = TExecAlloc0(TExecBase, TNULL, ttfont->size);
			if(hhea)
			{
				io_fread(ttype->io, ttfont->fontptr, (TUINT8 *) hhea, ttfont->size);
			
				ttfont->ascent  = TUtilNToHS(TUtilBase, hhea->ascender);
				ttfont->descent  = TUtilNToHS(TUtilBase, hhea->descender);
				ttfont->numberOfHMetrics = TUtilNToHS(TUtilBase, hhea->numberOfHMetrics);
				
				TExecFree(TExecBase, hhea);
			}
	

			/* search hmtx table */

			if(!(truetype_searchtable(ttype, ttfont, "hmtx")))
				return TTE_BADFONT;

			ttfont->hmtx = TExecAlloc0(TExecBase, TNULL, ttfont->size);
			io_fread(ttype->io, ttfont->fontptr, (TUINT8 *) ttfont->hmtx, ttfont->size);

			return TTE_OK;

		}
		else		/* not a font */
		{
			TExecFree(TExecBase, ttfont->buffer);
			return TTE_NOFOUND;
		}


	}
	else
		return TTE_NOFOUND;

}

static TMODAPI TVOID
truetype_close(TMOD_TRUETYPE *ttype, TTFONT *ttfont)
{
	if(ttfont)
	{
		io_close(ttype->io,ttfont->fontptr);
		TExecFree(TExecBase, ttfont->buffer);
		TExecFree(TExecBase, ttfont->cmap);
		TExecFree(TExecBase, ttfont->loca);
		TExecFree(TExecBase, ttfont->glyf);
		TExecFree(TExecBase, ttfont->hmtx);
	}

}

static TMODAPI TVOID 
truetype_freevert(TMOD_TRUETYPE *ttype, TTVERT *vert)
{
	if(vert)
	{
		TINT i;
		
		if(vert->outl)
		{
			for(i=0; i<vert->outlines; i++)
			{
				if(vert->outl[i].points)
					TExecFree(TExecBase, vert->outl[i].points);
			}	
		
			TExecFree(TExecBase, vert->outl);
			vert->outl = TNULL;
		}
		
	}

}

static TMODAPI TVOID
truetype_freetstr(TMOD_TRUETYPE *ttype, TTSTR *tstr)
{
	if(tstr)
	{
		TINT i,v;
		TTVERT *vert;
		
		for(v=0;v <tstr->numchar; v++)
		{

			vert = &tstr->verts[v];
			
			if(vert->outl)
			{
				for(i=0; i<vert->outlines; i++)
				{
					if(vert->outl[i].points)
					{
						if(vert->outl[i].points)
							TExecFree(TExecBase, vert->outl[i].points);
					}
				}	

				TExecFree(TExecBase, vert->outl);
			}

		}
		
		TExecFree(TExecBase, tstr->verts);
		tstr->verts = TNULL;
	}

}

/*
**	Revision History
**	$Log: truetype_mod.c,v $
**	Revision 1.5  2004/05/29 06:49:59  fpagels
**	fix a possible crash with monotype fonts
**	
**	Revision 1.4  2004/05/22 16:21:23  fpagels
**	fix componend glyfs, getchar() was broken
**	
**	Revision 1.3  2004/05/01 08:26:34  fpagels
**	courier works now better, distance beetwen chars looks better now, i think :)
**	
**	Revision 1.2  2003/12/12 10:59:12  fschulze
**	added path/filename conversion
**	
**	Revision 1.1.1.1  2003/12/11 07:20:18  tmueller
**	Krypton import
**	
**	Revision 1.8  2003/10/19 03:38:18  tmueller
**	Changed exec structure field names: Node, List, Handle, Module, ModuleEntry
**	
**	Revision 1.7  2003/10/12 16:38:26  tschwinger
**	
**	Support for mingw32 (windows gcc) compiler added.
**	
**	Revision 1.6  2003/09/17 16:35:46  tmueller
**	few warnings fixed
**	
**	Revision 1.5  2003/03/26 10:17:08  fpagels
**	add experimental componenten glyf support. i need more testfonts with component glyfs
**	
**	Revision 1.4  2003/03/22 05:27:50  tmueller
**	Adapted to new io_seek()
**	
**	Revision 1.3  2003/03/14 01:36:08  sskjaeret
**	Commented out two remaining printf()s to make the module link .. is it supposed to work yet?
**	
**	Revision 1.2  2003/03/11 17:31:12  fpagels
**	uses now new io-module
**	
**	Revision 1.2  2003/01/26 17:09:05  copper
**	versions test
**	
**
*/
