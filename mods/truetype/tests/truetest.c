
/*
**	mod/truetest.c
**	test for the truetype module
**
**	known issues: currently doesn't work on big endian
*/

#include <stdio.h>
#include <tek/debug.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/truetype.h>
#include <tek/proto/visual.h>

typedef struct _global
{
	TAPTR exec;
	TAPTR util;
	TAPTR ttype;
	TAPTR visual;
	TVPEN pen0,pen1;
	TINT xwin,ywin;
} global;

#define TExecBase g->exec
#define TUtilBase g->util

/* draw font */
TVOID drawfont(global *g, TTVERT *verts)
{
	TTIPOINT *xyarray;
	TINT o,i,xoff,yoff,p;
	TFLOAT s;

	TVisualClear(g->visual,g->pen0);

	s = (g->xwin < g->ywin ? g->xwin : g->ywin);	
	s = s - s/10;
	xoff = (TINT)(g->xwin-s)/2;
	yoff = (TINT)(g->ywin-s)/2;

	for(o=0; o<verts->outlines; o++)
	{
		p = verts->outl[o].numpoint;

		xyarray = (TTIPOINT*)TExecAlloc0(TExecBase, TNULL, sizeof(TTIPOINT)*(verts->outl[o].numpoint+2));

		if(xyarray)
		{
			for(i=0; i<=p; i++)
			{
				xyarray[i].x = (TINT)verts->outl[o].points[i].x*s/verts->width+xoff; 		
				xyarray[i].y = g->ywin - (TINT)verts->outl[o].points[i].y*s/verts->height - yoff;
			}

			
			/* draw polygon */
			if(verts->outl[o].filled)
				TVisualFPoly(g->visual, (TINT16*) xyarray, verts->outl[o].numpoint-1, g->pen1);
			else
				TVisualFPoly(g->visual, (TINT16*) xyarray, verts->outl[o].numpoint-1, g->pen0);

			TExecFree(TExecBase, xyarray);

		}
	}	
}

TVOID drawstring(global *g, TTSTR *tstr)
{
	TTIPOINT *xyarray;
	TINT o,i,xoff,yoff,p,v;
	TFLOAT s;

	TVisualClear(g->visual,g->pen0);

	xyarray = (TTIPOINT*)TExecAlloc0(TExecBase, TNULL, sizeof(TTIPOINT)*(tstr->maxpoints+2));

	if(xyarray)
	{
		TDOUBLE w,h;
		
		s = (g->xwin < g->ywin ? g->xwin : g->ywin);	
		s = s - s/10;
		xoff = (TINT)(g->xwin-s)/2;
		yoff = (TINT)(g->ywin-s)/2;

		w = tstr->width;
		h = tstr->height;
		
		for(v=0;v<tstr->numchar;v++)
		{
			TTVERT *verts;
			
			verts = &tstr->verts[v];
			for(o=0; o<verts->outlines; o++)
			{
				p = verts->outl[o].numpoint;


				for(i=0; i<=p; i++)
				{
					xyarray[i].x = (TINT)verts->outl[o].points[i].x*s/w+xoff; 		
					xyarray[i].y = g->ywin - (TINT)verts->outl[o].points[i].y*s/h - yoff; 		

				}

				/* draw polygon */
				if(verts->outl[o].filled)
					TVisualFPoly(g->visual, (TINT16*) xyarray, verts->outl[o].numpoint-1, g->pen1);
				else
					TVisualFPoly(g->visual, (TINT16*) xyarray, verts->outl[o].numpoint-1, g->pen0);
			}
		}

		TExecFree(TExecBase, xyarray);
	}	
}


TTASKENTRY TVOID TEKMain(TAPTR task)
{
	global gdata, *g = &gdata;
	TTFONT font;
	TINT	tt_status = TTE_OK;
	TSTRPTR *argv;
	TINT	argc;
	TTVERT verts;
	TTSTR  tstr;
	TAPTR v;
	TTAGITEM vistags[4];
	TINT	isstring = 1;

	g->exec = TGetExecBase(task);
	g->util = TExecOpenModule(TExecBase, "util", 0, TNULL);
	g->ttype = TExecOpenModule(TExecBase, "truetype", 0, TNULL);

	argv = TUtilGetArgV(TUtilBase);
	argc = TUtilGetArgC(TUtilBase);

	if(argc>1)
	{		
		g->xwin = 640;
		g->ywin = 480;
	
		vistags[0].tti_Tag = TVisual_PixWidth; vistags[0].tti_Value = (TTAG) g->xwin;
		vistags[1].tti_Tag = TVisual_PixHeight; vistags[1].tti_Value = (TTAG) g->ywin;
		vistags[2].tti_Tag = TVisual_Title; vistags[2].tti_Value = (TTAG) "Truetype Test";
		vistags[3].tti_Tag = TTAG_DONE;

		TExecFillMem(TExecBase, &verts, sizeof(TTVERT), 0);
		TExecFillMem(TExecBase, &tstr, sizeof(TTSTR), 0);
		TExecFillMem(TExecBase, &font, sizeof(TTFONT), 0);
	
		v = TExecOpenModule(TExecBase, "visual", 0, vistags);
		if(v)
		{
			TAPTR iport;
			TIMSG *imsg;
			TBOOL abort = TFALSE;
		
			g->visual = v;

			g->pen0 = TVisualAllocPen(v, 0x000000);
			g->pen1 = TVisualAllocPen(v, 0xffffff);

			TVisualClear(v, g->pen0);

			TVisualSetInput(v, TITYPE_NONE, TITYPE_CLOSE | TITYPE_COOKEDKEY |
				TITYPE_NEWSIZE | TITYPE_REFRESH);
	
			iport = TVisualGetPort(v);

			if(g->ttype)
			{
				TSTRPTR str;

				str = "!\"§$%&/()\n123456789\nabcdefghijkl\nABCDEFGHIJ";

				tt_status = truetype_init(g->ttype,&font,argv[1]);

				if(tt_status == TTE_OK)
				{
					TINT tt_error = 0;

					printf("Font found\n");

					if(argc == 2)
						tt_error = truetype_getstring(g->ttype, &font, str, &tstr);
					else
						tt_error = truetype_getstring(g->ttype, &font, argv[2], &tstr);
				}else
				{
					printf("Font not found!\n");
				}

			}
			else
				printf("truetype module not found!\n");

			if(tt_status == TTE_OK)
			{
				drawstring(g,&tstr);
		
				do
				{
					TExecWait(TExecBase, TExecGetPortSignal(TExecBase, iport));

					while ((imsg = (TIMSG *) TExecGetMsg(TExecBase, iport)))
					{
						switch (imsg->timsg_Type)
						{
							case TITYPE_NEWSIZE:
							{
								TTAGITEM tags[3];
								TFLOAT ow,oh;

								ow = (TFLOAT)g->xwin;
								oh = (TFLOAT)g->ywin;

								tags[0].tti_Tag = TVisual_PixWidth;
								tags[0].tti_Value = &g->xwin;
								tags[1].tti_Tag = TVisual_PixHeight;
								tags[1].tti_Value = &g->ywin;
								tags[2].tti_Tag = TTAG_DONE;

								TVisualGetAttrs(v, tags);
								if(isstring)
									drawstring(g,&tstr);
								else
									drawfont(g, &verts);
							}
							break;

							case TITYPE_CLOSE:
									abort = TTRUE;
									break;

							case TITYPE_COOKEDKEY:
								switch (imsg->timsg_Code)
								{
									case TKEYC_ESC:
										abort = TTRUE;
										break;
									case '\n':
										break;
										default:
										{
											TINT error;

											if(isstring)
											{
												isstring = 0;
												truetype_freetstr(g->ttype, &tstr);
											}
											if(verts.outl)
												truetype_freevert(g->ttype, &verts);

											error = truetype_getchar(g->ttype, &font,
												imsg->timsg_Code, &verts);
											drawfont(g, &verts);
										}
										break;
								}
								break;

						}
						TExecAckMsg(TExecBase, imsg);
					}

				} while (!abort);
			}

			TExecCloseModule(TExecBase, v);

			if(verts.outl)
				truetype_freevert(g->ttype, &verts);
			
			if(tstr.verts)
				truetype_freetstr(g->ttype, &tstr);
		
			truetype_close(g->ttype,&font);

		}
	}
	else
		printf("Use: truetest fontname [string]\n");
	

	if(g->util)
		TExecCloseModule(TExecBase, g->util);
	if(g->ttype)
		TExecCloseModule(TExecBase, g->ttype);

}
