/*
	displayhandler by dante@oxyron.de
*/

#define MOD_VERSION     0
#define MOD_REVISION    9

#include <tek/exec.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/io.h>
#include <tek/proto/util.h>
#include <tek/proto/display.h>
#include <tek/proto/imgproc.h>
#include <tek/debug.h>

#include <tek/mod/displayhandler.h>

#include <stdlib.h>

typedef struct _TModDisplayHandler
{
	TMODL module;                                           /* module header */
	TAPTR exec;
	TAPTR util;
	TAPTR imgp;

	TINT numdismods;
	TDISPROPS *disprops;
	TLIST displaymodules_list;
	TINT8 modname[64];

	TINT locktype,drawtype;

	/* infos about locked display/bitmap */
	THNDL *beginhandle,*lockhandle;
	THNDL *dishost;
	TDISMODULE *dismod;
	TAPTR theDisplay;
	TDISBITMAP *bitmap;
	TDISDESCRIPTOR desc;

	TIMGPICTURE lockimg;
	TBOOL emuldraw,lock;

} TMOD_DISPLAYHANDLER;

/* module prototypes */
static TCALLBACK TMOD_DISPLAYHANDLER *mod_open(TMOD_DISPLAYHANDLER *dis, TAPTR selftask, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_DISPLAYHANDLER *dis, TAPTR selftask);

TMODAPI THNDL*          TDisFindDisplay(TMOD_DISPLAYHANDLER *dis,TTAGITEM *tags);
TMODAPI TVOID           TDisGetDisplayProperties(TMOD_DISPLAYHANDLER *dis,THNDL *dishost, TDISPROPS *props);
TMODAPI TINT            TDisGetModeList(TMOD_DISPLAYHANDLER *dis,THNDL *dishost, TDISMODE **modelist);
TMODAPI TDISMODE*       TDisGetBestMode(TMOD_DISPLAYHANDLER *dis,THNDL *dishost, TINT w, TINT h, TINT d);
TMODAPI TBOOL           TDisCreateView(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TTAGITEM *tags);
TMODAPI THNDL*          TDisSimpleCreateView(TMOD_DISPLAYHANDLER *dis, TSTRPTR title, TINT w, TINT h, TINT d, TBOOL opengl, TBOOL fullscreen, TBOOL dblbuf, TBOOL resize);
TMODAPI TVOID           TDisGetDisplayCaps(TMOD_DISPLAYHANDLER *dis,THNDL *dishost, TDISCAPS *caps);

TMODAPI TVOID           TDisWaitMsg(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TTAGITEM *tags);
TMODAPI TBOOL           TDisGetMsg(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TDISMSG *dismsg);
TMODAPI TVOID           TDisSetAttrs(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TTAGITEM *tags);
TMODAPI TVOID           TDisFlush(TMOD_DISPLAYHANDLER *dis,THNDL *dishost);
TMODAPI THNDL*          TDisAllocPen(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TUINT color);
TMODAPI TVOID           TDisSetDPen(TMOD_DISPLAYHANDLER *dis, THNDL *pen);

TMODAPI TVOID           TDisSetPalette(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TIMGARGBCOLOR *pal, TINT sp, TINT sd, TINT numentries);
TMODAPI THNDL*          TDisAllocBitmap(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TINT w, TINT h, TINT flags);

TMODAPI TVOID           TDisDescribe(TMOD_DISPLAYHANDLER *dis,THNDL *handle, TDISDESCRIPTOR *desc);
TMODAPI TBOOL           TDisLock(TMOD_DISPLAYHANDLER *dis,THNDL *handle,TIMGPICTURE *img);
TMODAPI TVOID           TDisUnlock(TMOD_DISPLAYHANDLER *dis);
TMODAPI TBOOL           TDisBegin(TMOD_DISPLAYHANDLER *dis,THNDL *handle);
TMODAPI TVOID           TDisEnd(TMOD_DISPLAYHANDLER *dis);

TMODAPI TBOOL           TDisBlit(TMOD_DISPLAYHANDLER *dis,THNDL *bmhndl,TTAGITEM *tags);

TMODAPI TVOID           TDisTextout(TMOD_DISPLAYHANDLER *dis, TINT8 *text, TINT row, TINT column);
TMODAPI TVOID           TDisPutImage(TMOD_DISPLAYHANDLER *dis,TIMGPICTURE *img, TTAGITEM *tags);

TMODAPI TVOID           TDisFill(TMOD_DISPLAYHANDLER *dis);
TMODAPI TVOID           TDisPlot(TMOD_DISPLAYHANDLER *dis,TINT x,TINT y);
TMODAPI TVOID           TDisLine(TMOD_DISPLAYHANDLER *dis,TINT sx, TINT sy, TINT ex, TINT ey);
TMODAPI TVOID           TDisBox(TMOD_DISPLAYHANDLER *dis,TINT sx, TINT sy, TINT w, TINT h);
TMODAPI TVOID           TDisBoxf(TMOD_DISPLAYHANDLER *dis,TINT sx, TINT sy, TINT w, TINT h);
TMODAPI TVOID           TDisPoly(TMOD_DISPLAYHANDLER *dis,TINT numpoints, TINT *points);
TMODAPI TVOID           TDisPolyf(TMOD_DISPLAYHANDLER *dis,TINT numpoints, TINT *points);
TMODAPI TVOID           TDisEllipse(TMOD_DISPLAYHANDLER *dis,TINT x,TINT y,TINT rx,TINT ry);
TMODAPI TVOID           TDisEllipsef(TMOD_DISPLAYHANDLER *dis,TINT x,TINT y,TINT rx,TINT ry);

TMODAPI TVOID           TDisMovePixels(TMOD_DISPLAYHANDLER *dis,TINT sx,TINT sy,TINT dx,TINT dy, TINT w, TINT h);

/* private prototypes */
TBOOL                   TDIS_ReadDisplayHostsProps(TMOD_DISPLAYHANDLER *dis);
TBOOL                   TDIS_ChooseDisplayModule(TMOD_DISPLAYHANDLER *dis, TTAGITEM *tags);
TCALLBACK TINT  TDIS_ModulesSortFunc(TAPTR udata, TAPTR e1, TAPTR e2);
TCALLBACK TVOID TDIS_DestroyDisplayModule(THNDL *handle);
TCALLBACK TVOID TDIS_DestroyPen(THNDL *penhndl);
TCALLBACK TVOID TDIS_DestroyBitmap(THNDL *bmhndl);
TCALLBACK TINT  TDIS_ModeSortFunc(TAPTR udata, TTAG e1, TTAG e2);

/**************************************************************************
	tek_init_<modname>
 **************************************************************************/
TMODENTRY TUINT tek_init_displayhandler(TAPTR selftask, TMOD_DISPLAYHANDLER *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)                                  /* first call */
		{
			if (version <= MOD_VERSION)                     /* version check */
			{
				return sizeof(TMOD_DISPLAYHANDLER);    /* return module positive size */
			}
		}
		else                                                                    /* second call */
		{
			return sizeof(TAPTR) * 71;                       /* return module negative size */
		}
	}
	else                                                                            /* third call */
	{
		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;

		/* this module has instances. place instance
		** open/close functions into the module structure. */
		mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;

		/* put module vectors in front */
		((TAPTR *) mod)[-1 ] = (TAPTR) TDisFindDisplay;
		((TAPTR *) mod)[-2 ] = (TAPTR) TDisGetDisplayProperties;
		((TAPTR *) mod)[-3 ] = (TAPTR) TDisGetModeList;
		((TAPTR *) mod)[-4 ] = (TAPTR) TDisGetBestMode;
		((TAPTR *) mod)[-5 ] = (TAPTR) TDisCreateView;
		((TAPTR *) mod)[-6 ] = (TAPTR) TDisSimpleCreateView;
		((TAPTR *) mod)[-7 ] = (TAPTR) TDisGetDisplayCaps;

		((TAPTR *) mod)[-20] = (TAPTR) TDisWaitMsg;
		((TAPTR *) mod)[-21] = (TAPTR) TDisGetMsg;
		((TAPTR *) mod)[-22] = (TAPTR) TDisSetAttrs;
		((TAPTR *) mod)[-24] = (TAPTR) TDisFlush;
		((TAPTR *) mod)[-25] = (TAPTR) TDisAllocPen;
		((TAPTR *) mod)[-26] = (TAPTR) TDisSetDPen;
		((TAPTR *) mod)[-30] = (TAPTR) TDisSetPalette;
		((TAPTR *) mod)[-31] = (TAPTR) TDisAllocBitmap;

		((TAPTR *) mod)[-40] = (TAPTR) TDisDescribe;
		((TAPTR *) mod)[-41] = (TAPTR) TDisLock;
		((TAPTR *) mod)[-42] = (TAPTR) TDisUnlock;
		((TAPTR *) mod)[-43] = (TAPTR) TDisBegin;
		((TAPTR *) mod)[-44] = (TAPTR) TDisEnd;

		((TAPTR *) mod)[-50] = (TAPTR) TDisBlit;

		((TAPTR *) mod)[-51] = (TAPTR) TDisTextout;

		((TAPTR *) mod)[-59] = (TAPTR) TDisPutImage;
		((TAPTR *) mod)[-61] = (TAPTR) TDisFill;
		((TAPTR *) mod)[-62] = (TAPTR) TDisPlot;
		((TAPTR *) mod)[-63] = (TAPTR) TDisLine;
		((TAPTR *) mod)[-64] = (TAPTR) TDisBox;
		((TAPTR *) mod)[-65] = (TAPTR) TDisBoxf;
		((TAPTR *) mod)[-66] = (TAPTR) TDisPoly;
		((TAPTR *) mod)[-67] = (TAPTR) TDisPolyf;
		((TAPTR *) mod)[-68] = (TAPTR) TDisEllipse;
		((TAPTR *) mod)[-69] = (TAPTR) TDisEllipsef;

		((TAPTR *) mod)[-70] = (TAPTR) TDisMovePixels;

		return TTRUE;
	}
	return 0;
}

/**************************************************************************
	open instance
 **************************************************************************/
static TCALLBACK TMOD_DISPLAYHANDLER *mod_open(TMOD_DISPLAYHANDLER *dis, TAPTR selftask, TTAGITEM *tags)
{
	dis = TNewInstance(dis, dis->module.tmd_PosSize, dis->module.tmd_NegSize);
	if (!dis) return TNULL;

	dis->exec = TGetExecBase(dis);

	dis->util = TExecOpenModule(dis->exec, "util", 0, TNULL);
	if(!dis->util)
	{
		TFreeInstance(dis);
		return TNULL;
	}

	dis->imgp = TExecOpenModule(dis->exec, "imgproc", 0, TNULL);
	if(!dis->imgp)
	{
		TExecCloseModule(dis->exec, dis->util);
		TFreeInstance(dis);
		return TNULL;
	}

	dis->emuldraw=TFALSE;
	dis->lock=TFALSE;

	return dis;
}

/**************************************************************************
	close instance
 **************************************************************************/
static TCALLBACK TVOID mod_close(TMOD_DISPLAYHANDLER *dis, TAPTR selftask)
{
	if(dis->imgp)
		TExecCloseModule(dis->exec, dis->imgp);

	if(dis->util)
		TExecCloseModule(dis->exec, dis->util);

	TFreeInstance(dis);
}

/**************************************************************************
	TDisFindDisplay
 **************************************************************************/
TMODAPI THNDL* TDisFindDisplay(TMOD_DISPLAYHANDLER *dis, TTAGITEM *tags)
{
	TDISMODULE *d;
	THNDL *handle;

	if(!(TDIS_ChooseDisplayModule(dis,tags)))
		return TNULL;
	else
	{
		d=TExecAlloc0(dis->exec,TNULL,sizeof(TDISMODULE));
		if(d)
		{
			d->modul=TNULL;
			d->modul=TExecOpenModule(dis->exec, dis->modname, 0, TNULL);
			if(d->modul)
			{
				handle=TExecAlloc0(dis->exec,TNULL,sizeof(THNDL));
				if(handle)
				{
					d->type=TDIS_DISPLAY;
					d->displayhandler=dis;
					dismod_getproperties(d->modul,&d->props);

					d->numdismodes=dismod_getmodelist(d->modul,&d->modelist);
					if(d->numdismodes!=0 && d->modelist)
						TUtilQSort(dis->util,d->modelist, d->numdismodes, sizeof(TDISMODE), (TCMPFUNC)TDIS_ModeSortFunc,TNULL);

					d->palette=TNULL;
					TInitList(&d->pens_list);
					TInitList(&d->bitmaps_list);

					handle->thn_Data=d;
					handle->thn_DestroyFunc=(TDFUNC)TDIS_DestroyDisplayModule;
					TAddTail(&dis->displaymodules_list, (TNODE *) handle);
					return handle;
				}
				TExecCloseModule(dis->exec,d->modul);
			}
			TExecFree(dis->exec,d);
		}
	}
	return TNULL;
}

/**************************************************************************
	TDisGetDisplayProperties
 **************************************************************************/
TMODAPI TVOID TDisGetDisplayProperties(TMOD_DISPLAYHANDLER *dis,THNDL *dishost, TDISPROPS *props)
{
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	dismod_getproperties(theDisplay,props);
}

/**************************************************************************
	TDisGetDisplayCaps
 **************************************************************************/
TMODAPI TVOID TDisGetDisplayCaps(TMOD_DISPLAYHANDLER *dis,THNDL *dishost, TDISCAPS *caps)
{
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	dismod_getcaps(theDisplay,caps);
}

/**************************************************************************
	TDisGetDisModeList
 **************************************************************************/
TMODAPI TINT TDisGetModeList(TMOD_DISPLAYHANDLER *dis,THNDL *dishost, TDISMODE **modelist)
{
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;

	*modelist=dismod->modelist;
	return(dismod->numdismodes);
}

/**************************************************************************
	TDisGetBestMode
 **************************************************************************/
TMODAPI TDISMODE* TDisGetBestMode(TMOD_DISPLAYHANDLER *dis,THNDL *dishost, TINT w, TINT h, TINT d)
{
	TINT i,startarea,endarea;
	TBOOL suc;
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;


	/* seek for right colordepth */
	suc=TFALSE;
	i=0;
	while(!suc && i<dismod->numdismodes)
	{
		if(dismod->modelist[i].depth == d)
			suc=TTRUE;
		else
			i++;
	}

	/* nothing found, trying some alternatives */
	if(!suc)
	{
		if(d>8)
		{
			switch(d)
			{
				case 15:
					d=16;
				break;

				case 16:
					d=15;
				break;

				case 24:
					d=32;
				break;

				case 32:
					d=24;
				break;
			}

			/* and seek again */
			i=0;
			while(!suc && i<dismod->numdismodes)
			{
				if(dismod->modelist[i].depth >= d)
					suc=TTRUE;
				else
					i++;
			}

			/* still nothing found?! now we take everything from 15 bits upward */
			if(!suc && d>15)
			{
				d=15;

				i=0;
				while(!suc && i<dismod->numdismodes)
				{
					if(dismod->modelist[i].depth >= d)
						suc=TTRUE;
					else
						i++;
				}
			}
		}

		/* nothing found!!! */
		if(!suc)
			return TNULL;
	}

	startarea=i;

	/* seek for end of area with same depth */
	suc=TFALSE;
	i=startarea+1;
	while(!suc && i<dismod->numdismodes)
	{
		if(dismod->modelist[startarea].depth != dismod->modelist[i].depth)
			suc=TTRUE;
		else
			i++;
	}

	endarea=i;

	/* now seek for width and height */
	suc=TFALSE;
	i=startarea;
	while(!suc && i<endarea)
	{
		if(dismod->modelist[i].width==w && dismod->modelist[i].height==h)
			suc=TTRUE;
		else
			i++;
	}

	if(suc)
		return &dismod->modelist[i];

	/* ok, nothing found with the same size, lets seek for a slightly bigger mode */
	suc=TFALSE;
	i=startarea;
	while(!suc && i<endarea)
	{
		if(dismod->modelist[i].width>=w && dismod->modelist[i].height>=h)
			suc=TTRUE;
		else
			i++;
	}

	if(suc)
		return &dismod->modelist[i];

	/* the wished mode must be out of range - return the biggest mode in this depth */
	return &dismod->modelist[endarea-1];
}

/**************************************************************************
	TDisCreateView
 **************************************************************************/
TMODAPI TBOOL TDisCreateView(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TTAGITEM *tags)
{
	TINT w,h,d,x,y,flags;
	TSTRPTR title;

	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;
	TDISPROPS *disprops=&dismod->props;

	title=(TSTRPTR)TGetTag(tags,TDISC_TITLE,(TTAG)"teklib display");

	w=(TINT)TGetTag(tags,TDISC_WIDTH,(TTAG)(disprops->defaultwidth));
	if(w>disprops->maxwidth) w=disprops->maxwidth;
	h=(TINT)TGetTag(tags,TDISC_HEIGHT,(TTAG)(disprops->defaultheight));
	if(h>disprops->maxheight) h=disprops->maxheight;
	d=(TINT)TGetTag(tags,TDISC_DEPTH,(TTAG)(disprops->defaultdepth));
	if(d<disprops->mindepth || d>disprops->maxdepth) return TFALSE;
	x=(TINT)TGetTag(tags,TDISC_XPOS,(TTAG)0);
	if(x<0 || x+w>disprops->maxwidth) x=disprops->maxwidth-w;
	y=(TINT)TGetTag(tags,TDISC_YPOS,(TTAG)0);
	if(y<0 || y+h>disprops->maxheight) y=disprops->maxheight-h;
	flags=(TINT)TGetTag(tags,TDISC_FLAGS,(TTAG)0);

	if(dismod_create(theDisplay,title,x,y,w,h,d,flags))
	{
		dismod_describe_dis(theDisplay,&dismod->desc);
		dismod_getcaps(theDisplay,&dismod->caps);
		return TTRUE;
	}

	return TFALSE;
}

/**************************************************************************
	TDisSimpleCreateView
 **************************************************************************/
TMODAPI THNDL* TDisSimpleCreateView(TMOD_DISPLAYHANDLER *dis, TSTRPTR title, TINT w, TINT h, TINT d, TBOOL opengl, TBOOL fullscreen, TBOOL dblbuf, TBOOL resize)
{
	TTAGITEM finddistags[8];
	TTAGITEM distags[8];
	THNDL *theDisplay=TNULL;

	/* set up the taglist for finddisplay */
	finddistags[0].tti_Tag = TDISMUSTHAVE_CLASS;
	if(opengl)
		finddistags[0].tti_Value = (TTAG) TDISCLASS_OPENGL;
	else
		finddistags[0].tti_Value = (TTAG) TDISCLASS_STANDARD;

	finddistags[1].tti_Tag = TDISMUSTHAVE_MODE;
	if(fullscreen)
		finddistags[1].tti_Value = (TTAG) TDISMODE_FULLSCREEN;
	else
		finddistags[1].tti_Value = (TTAG) TDISMODE_WINDOW;

	finddistags[2].tti_Tag = TDIS_MINWIDTH;         finddistags[2].tti_Value = (TTAG) w;
	finddistags[3].tti_Tag = TDIS_MINHEIGHT;        finddistags[3].tti_Value = (TTAG) h;

	if(d==8)
	{
		finddistags[4].tti_Tag = TDISMUSTHAVE_COLORS;   finddistags[4].tti_Value = (TTAG) TDISCOLORS_CLUT;
		finddistags[5].tti_Tag = TTAG_DONE;
	}
	else
		finddistags[4].tti_Tag = TTAG_DONE;


	/* now lets see, if we find something */
	theDisplay=TDisFindDisplay(dis,finddistags);

	/* nothing found, lets try, if its just a problem with the screenmode */
	if(!theDisplay)
	{
		if(fullscreen)
		{
			finddistags[1].tti_Value = (TTAG) TDISMODE_WINDOW;
			theDisplay=TDisFindDisplay(dis,finddistags);
		}
		else if(!fullscreen)
		{
			finddistags[1].tti_Value = (TTAG) TDISMODE_FULLSCREEN;
			theDisplay=TDisFindDisplay(dis,finddistags);
		}
	}

	/* if we have a displaymodule, create the display */
	if(theDisplay)
	{
		TINT flags=0;
		TINT nw=w;
		TINT nh=h;
		TINT nd=d;

		if(fullscreen)
		{
			TDISMODE *dmode=TDisGetBestMode(dis,theDisplay,w,h,d);
			if(dmode)
			{
				nw=dmode->width;
				nh=dmode->height;
				nd=dmode->depth;
			}
		}

		if(dblbuf)
			flags|=TDISCF_DOUBLEBUFFER;

		if(resize)
			flags|=TDISCF_RESIZEABLE;

		/* set up the taglist for create */
		distags[0].tti_Tag = TDISC_WIDTH;       distags[0].tti_Value = (TTAG)nw;
		distags[1].tti_Tag = TDISC_HEIGHT;      distags[1].tti_Value = (TTAG)nh;
		distags[2].tti_Tag = TDISC_DEPTH;       distags[2].tti_Value = (TTAG)nd;
		distags[3].tti_Tag = TDISC_TITLE;       distags[3].tti_Value = (TTAG)title;
		distags[4].tti_Tag = TDISC_FLAGS;       distags[4].tti_Value = (TTAG)flags;
		distags[5].tti_Tag = TTAG_DONE;

		/* and gogogo! */
		if(TDisCreateView(dis,theDisplay,distags))
			return theDisplay;
		else
		{
			/* if we couldn't open a display, cycle the depth and try again */
			if(d>=15)
			{
				distags[2].tti_Value = (TTAG)16;
				if(TDisCreateView(dis,theDisplay,distags))
					return theDisplay;
				else
				{
					distags[2].tti_Value = (TTAG)32;
					if(TDisCreateView(dis,theDisplay,distags))
						return theDisplay;
					else
					{
						distags[2].tti_Value = (TTAG)24;
						if(TDisCreateView(dis,theDisplay,distags))
							return theDisplay;
					}
				}
			}
		}
	}
	return TNULL;
}

/**************************************************************************
	TDisWaitMsg
 **************************************************************************/
TMODAPI TVOID TDisWaitMsg(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TTAGITEM *tags)
{
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	dismod_waitmsg(theDisplay);
}

/**************************************************************************
	TDisGetMsg
 **************************************************************************/
TMODAPI TBOOL TDisGetMsg(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TDISMSG *dismsg)
{
	TDISMSG msg;
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	if(dismod_getmsg(theDisplay,&msg))
	{
		switch (msg.code)
		{
			case TDISMSG_RESIZE: case TDISMSG_MOVE:
				TExecCopyMem(dis->exec,msg.data,dismsg->data,sizeof(TDISRECT));
			break;

			case TDISMSG_KEYDOWN: case TDISMSG_KEYUP:
				TExecCopyMem(dis->exec,msg.data,dismsg->data,sizeof(TDISKEY));
			break;

			case TDISMSG_MOUSEMOVE:
				TExecCopyMem(dis->exec,msg.data,dismsg->data,sizeof(TDISMOUSEPOS));
			break;

			case TDISMSG_MBUTTONDOWN: case TDISMSG_MBUTTONUP:
				TExecCopyMem(dis->exec,msg.data,dismsg->data,sizeof(TDISMBUTTON));
			break;
		}
		dismsg->code=msg.code;
		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	TDisSetAttrs
 **************************************************************************/
TMODAPI TVOID TDisSetAttrs(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TTAGITEM *tags)
{
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	dismod_setattrs(theDisplay,tags);
}

/**************************************************************************
	TDisFlush
 **************************************************************************/
TMODAPI TVOID TDisFlush(TMOD_DISPLAYHANDLER *dis,THNDL *dishost)
{
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	dismod_flush(theDisplay);
}

/**************************************************************************
	TDisAllocPen
 **************************************************************************/
TMODAPI THNDL* TDisAllocPen(TMOD_DISPLAYHANDLER *dis,THNDL *dishost, TUINT color)
{
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	TDISPEN *pen;
	THNDL *handle;

	pen=TExecAlloc0(dis->exec,TNULL,sizeof(TDISPEN));
	if(pen)
	{
		pen->display=dishost;
		pen->color.a=(color & 0xff000000)>>24;
		pen->color.r=(color & 0x00ff0000)>>16;
		pen->color.g=(color & 0x0000ff00)>>8;
		pen->color.b=color & 0x000000ff;

		if(dismod_allocpen(theDisplay,pen))
		{
			handle=TExecAlloc0(dis->exec,TNULL,sizeof(THNDL));
			if(handle)
			{
				handle->thn_Data=pen;
				handle->thn_DestroyFunc=(TDFUNC)TDIS_DestroyPen;
				TAddTail(&dismod->pens_list, (TNODE *) handle);
				return handle;
			}
		}
		TExecFree(dis->exec,pen);
	}
	return TNULL;
}

/**************************************************************************
	TDisSetDPen
 **************************************************************************/
TMODAPI TVOID TDisSetDPen(TMOD_DISPLAYHANDLER *dis, THNDL *penhndl)
{
	TDISPEN *pen=(TDISPEN*)penhndl->thn_Data;
	THNDL *dishost=pen->display;
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	TExecCopyMem(dis->exec,pen,&dismod->drawpen,sizeof(TDISPEN));
	dismod_setdpen(theDisplay,pen);
}

/**************************************************************************
	TDisSetPalette
 **************************************************************************/
TMODAPI TVOID TDisSetPalette(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TIMGARGBCOLOR *pal, TINT sp, TINT sd, TINT numentries)
{
	TINT i;

	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	if(dismod_setpalette(theDisplay,pal,sp,sd,numentries))
	{
		if(!dismod->palette)
			dismod->palette=TExecAlloc0(dis->exec,TNULL,256*sizeof(TIMGARGBCOLOR));

		for(i=0;i<numentries;i++)
		{
			dismod->palette[i+sd].a=0;
			dismod->palette[i+sd].r=pal[i+sp].r;
			dismod->palette[i+sd].g=pal[i+sp].g;
			dismod->palette[i+sd].b=pal[i+sp].b;
		}
	}
}

/**************************************************************************
	TDisAllocBitmap
 **************************************************************************/
TMODAPI THNDL* TDisAllocBitmap(TMOD_DISPLAYHANDLER *dis,THNDL *dishost,TINT w, TINT h, TINT flags)
{
	TDISBITMAP *bitmap;
	THNDL *handle;

	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	if( (flags & TDISCF_SCALEBLIT) && !dismod->caps.blitscale)
		return TNULL;

	if( (flags & TDISCF_CKEYBLIT) && !dismod->caps.blitckey)
		return TNULL;

	if( (flags & TDISCF_CALPHABLIT) && !dismod->caps.blitalpha)
		return TNULL;

	bitmap=TExecAlloc0(dis->exec,TNULL,sizeof(TDISBITMAP));
	if(bitmap)
	{
		bitmap->type=TDIS_BITMAP;
		bitmap->display=dishost;
		bitmap->flags=flags;

		if(dismod_allocbitmap(theDisplay,bitmap,w,h,flags))
		{
			handle=TExecAlloc0(dis->exec,TNULL,sizeof(THNDL));
			if(handle)
			{
				handle->thn_Data=bitmap;
				handle->thn_DestroyFunc=(TDFUNC)TDIS_DestroyBitmap;
				TAddTail(&dismod->bitmaps_list, (TNODE *) handle);
				return handle;
			}
		}
		TExecFree(dis->exec,bitmap);
	}
	return TNULL;
}

/**************************************************************************
	TDisDescribe
 **************************************************************************/
TMODAPI TVOID TDisDescribe(TMOD_DISPLAYHANDLER *dis,THNDL *handle, TDISDESCRIPTOR *desc)
{
	TDISMODULE *dismod=(TDISMODULE*)handle->thn_Data;
	if(dismod->type==TDIS_DISPLAY)
	{
		TAPTR theDisplay=dismod->modul;
		dismod_describe_dis(theDisplay,desc);
	}
	else if(dismod->type==TDIS_BITMAP)
	{
		TDISBITMAP *bitmap=(TDISBITMAP*)handle->thn_Data;
		THNDL *dishost=bitmap->display;
		TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
		TAPTR theDisplay=dismod->modul;
		dismod_describe_bm(theDisplay,bitmap,desc);
	}
}

/**************************************************************************
	TDisLock
 **************************************************************************/
TMODAPI TBOOL TDisLock(TMOD_DISPLAYHANDLER *dis,THNDL *handle,TIMGPICTURE *img)
{
	THNDL *dishost;
	TDISMODULE *dismod;
	TAPTR theDisplay;

	if(!dis->lockhandle)
	{
		TDISBITMAP *bitmap=TNULL;
		dismod=(TDISMODULE*)handle->thn_Data;

		dis->locktype=dismod->type;

		if(dis->locktype==TDIS_DISPLAY)
		{
			theDisplay=dismod->modul;
			if(dismod_lock_dis(theDisplay,img))
			{
				dis->lockhandle=handle;
				return TTRUE;
			}
		}
		else if(dis->locktype==TDIS_BITMAP)
		{
			bitmap=(TDISBITMAP*)handle->thn_Data;
			dishost=bitmap->display;
			dismod=(TDISMODULE*)dishost->thn_Data;
			theDisplay=dismod->modul;
			if(dismod_lock_bm(theDisplay,bitmap,img))
			{
				dis->lockhandle=handle;
				return TTRUE;
			}
		}
	}
	return TFALSE;
}

/**************************************************************************
	TDisUnlock
 **************************************************************************/
TMODAPI TVOID TDisUnlock(TMOD_DISPLAYHANDLER *dis)
{
	THNDL *dishost;
	TDISMODULE *dismod;
	TAPTR theDisplay;

	TDISBITMAP *bitmap=TNULL;
	dismod=(TDISMODULE*)dis->lockhandle->thn_Data;

	if(dis->locktype==TDIS_DISPLAY)
	{
		theDisplay=dismod->modul;
		dismod_unlock_dis(theDisplay);
		dis->lockhandle=TNULL;
	}
	else if(dis->locktype==TDIS_BITMAP)
	{
		bitmap=(TDISBITMAP*)dis->lockhandle->thn_Data;
		dishost=bitmap->display;
		dismod=(TDISMODULE*)dishost->thn_Data;
		theDisplay=dismod->modul;
		dismod_unlock_bm(theDisplay,bitmap);
		dis->lockhandle=TNULL;
	}
}

/**************************************************************************
	TDisBegin
 **************************************************************************/
TMODAPI TBOOL TDisBegin(TMOD_DISPLAYHANDLER *dis,THNDL *handle)
{
	TBOOL suc=TFALSE;
	
	dis->dismod=(TDISMODULE*)handle->thn_Data;
	
	dis->drawtype=dis->dismod->type;

	if(dis->drawtype==TDIS_DISPLAY)
	{
		dis->theDisplay=dis->dismod->modul;
		dismod_describe_dis(dis->theDisplay,&dis->desc);
		suc=dismod_begin_dis(dis->theDisplay);
	}
	else if(dis->drawtype==TDIS_BITMAP)
	{
		dis->bitmap=(TDISBITMAP*)handle->thn_Data;
		dis->dishost=dis->bitmap->display;
		dis->dismod=(TDISMODULE*)dis->dishost->thn_Data;

		dis->theDisplay=dis->dismod->modul;
		dismod_describe_bm(dis->theDisplay,dis->bitmap,&dis->desc);
		if(dis->dismod->caps.candrawbitmap)
			suc=dismod_begin_bm(dis->theDisplay,dis->bitmap);
		else
		{
			dis->emuldraw=TTRUE;
			if(dismod_lock_bm(dis->theDisplay,dis->bitmap,&dis->lockimg))
			{
				dis->lock=TTRUE;
			}
			suc=dis->lock;
		}
	}
	if(suc)
	{
		dis->beginhandle=handle;
		return TTRUE;
	}
	else
		return TFALSE;
}

/**************************************************************************
	TDisEnd
 **************************************************************************/
TMODAPI TVOID TDisEnd(TMOD_DISPLAYHANDLER *dis)
{
	if(dis->drawtype==TDIS_DISPLAY)
	{
		if(!dis->emuldraw)
			dismod_end_dis(dis->theDisplay);
		else
		{
			dismod_unlock_dis(dis->theDisplay);
			dis->emuldraw=TFALSE;
			dis->lock=TFALSE;
		}
	}
	else if(dis->drawtype==TDIS_BITMAP)
	{
		if(!dis->emuldraw)
			dismod_end_bm(dis->theDisplay,dis->bitmap);
		else
		{
			dismod_unlock_bm(dis->theDisplay,dis->bitmap);
			dis->emuldraw=TFALSE;
			dis->lock=TFALSE;
		}
	}
	dis->beginhandle=TNULL;
}

/**************************************************************************
	TDisBlit
 **************************************************************************/
TMODAPI TBOOL TDisBlit(TMOD_DISPLAYHANDLER *dis,THNDL *bmhndl,TTAGITEM *tags)
{
	TDISDESCRIPTOR desc;
	TDBLITOPS bops;
	TIMGARGBCOLOR *ckval;

	TDISBITMAP *bm=(TDISBITMAP*)bmhndl->thn_Data;
	THNDL *dishost=bm->display;
	TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
	TAPTR theDisplay=dismod->modul;

	dismod_describe_dis(theDisplay,&desc);

	bops.src.x=(TINT)TGetTag(tags,TDISB_SRCX,(TTAG)0);
	bops.src.y=(TINT)TGetTag(tags,TDISB_SRCY,(TTAG)0);
	bops.src.width=(TINT)TGetTag(tags,TDISB_SRCWIDTH,(TTAG)(bm->image.width-bops.src.x));
	bops.src.height=(TINT)TGetTag(tags,TDISB_SRCHEIGHT,(TTAG)(bm->image.height-bops.src.y));
	bops.dst.x=(TINT)TGetTag(tags,TDISB_DSTX,(TTAG)0);
	bops.dst.y=(TINT)TGetTag(tags,TDISB_DSTY,(TTAG)0);
	bops.dst.width=(TINT)TGetTag(tags,TDISB_DSTWIDTH,(TTAG)(bops.src.width));
	bops.dst.height=(TINT)TGetTag(tags,TDISB_DSTHEIGHT,(TTAG)(bops.src.height));

	ckval=(TIMGARGBCOLOR*)TGetTag(tags,TDISB_CKEY,(TTAG)TNULL);
	bops.ckey=TFALSE;
	if(ckval)
	{
		if(!(bm->flags & TDISCF_CKEYBLIT))
			return TFALSE;
		else
		{
			bops.ckey_val.a=ckval->a;
			bops.ckey_val.r=ckval->r;
			bops.ckey_val.g=ckval->g;
			bops.ckey_val.b=ckval->b;

			bops.ckey=TTRUE;
		}
	}

	bops.calpha_val=(TINT)TGetTag(tags,TDISB_CALPHA,(TTAG)-1);
	bops.calpha=TFALSE;
	if(bops.calpha_val!=-1)
	{
		if(!(bm->flags & TDISCF_CALPHABLIT))
			return TFALSE;
		else
			bops.calpha=TTRUE;
	}

	if(bops.dst.x>=0 && bops.dst.y>=0 && bops.dst.x+bops.dst.width<=desc.width && bops.dst.y+bops.dst.height<=desc.height)
	{
		if(bops.src.width!=bops.dst.width || bops.src.height!=bops.dst.height)
		{
			if(!(bm->flags & TDISCF_SCALEBLIT))
				return TFALSE;
		}
		dismod_blit(theDisplay,bm,&bops);
	}
	return TTRUE;
}

/**************************************************************************
	dis textout
 **************************************************************************/
TMODAPI TVOID TDisTextout(TMOD_DISPLAYHANDLER *dis, TINT8 *text, TINT row, TINT column)
{
	if(dis->drawtype==TDIS_DISPLAY)
		dismod_textout_dis(dis->theDisplay,text,row,column);
	else if(dis->drawtype==TDIS_BITMAP)
		dismod_textout_bm(dis->theDisplay,text,row,column);
}

/**************************************************************************
	TDisPutImage
 **************************************************************************/
TMODAPI TVOID TDisPutImage(TMOD_DISPLAYHANDLER *dis,TIMGPICTURE *img, TTAGITEM *tags)
{
	TBOOL isbitmap=TFALSE;
	TDISDESCRIPTOR desc;
	TDISRECT src,dst;
	THNDL *bhandle=TNULL;

	src.x=(TINT)TGetTag(tags,TDISB_SRCX,0);
	src.y=(TINT)TGetTag(tags,TDISB_SRCY,0);
	src.width=(TINT)TGetTag(tags,TDISB_SRCWIDTH,(TTAG)(img->width-src.x));
	src.height=(TINT)TGetTag(tags,TDISB_SRCHEIGHT,(TTAG)(img->height-src.y));

	dst.x=(TINT)TGetTag(tags,TDISB_DSTX,0);
	dst.y=(TINT)TGetTag(tags,TDISB_DSTY,0);
	dst.width=(TINT)TGetTag(tags,TDISB_DSTWIDTH,(src.width));
	dst.height=(TINT)TGetTag(tags,TDISB_DSTHEIGHT,(src.height));

	if(dis->drawtype==TDIS_DISPLAY)
	{
		isbitmap=TFALSE;
		dismod_describe_dis(dis->theDisplay,&desc);
	}
	else if(dis->drawtype==TDIS_BITMAP)
	{
		isbitmap=TTRUE;
		dismod_describe_bm(dis->theDisplay,dis->bitmap,&desc);
	}

	if(dst.x>=0 && dst.y>=0 && dst.x+dst.width<=desc.width && dst.y+dst.height<=desc.height)
	{
		if(dst.width!=src.width || dst.height!=src.height)
		{
			if(!isbitmap && dis->dismod->caps.canconvertscaledisplay)
				dismod_putscaleimage_dis(dis->theDisplay,img,&src,&dst);
			else if(isbitmap && dis->dismod->caps.canconvertscalebitmap)
				dismod_putscaleimage_bm(dis->theDisplay,dis->bitmap,img,&src,&dst);
			else
			{
				TIMGPICTURE bufimg;
				TTAGITEM scaletags[10];
				
				scaletags[0].tti_Tag = IMGTAG_SRCX;             scaletags[0].tti_Value = (TTAG)(src.x);
				scaletags[1].tti_Tag = IMGTAG_SRCY;             scaletags[1].tti_Value = (TTAG)(src.y);
				scaletags[2].tti_Tag = IMGTAG_DSTX;             scaletags[2].tti_Value = (TTAG)(dst.x);
				scaletags[3].tti_Tag = IMGTAG_DSTY;             scaletags[3].tti_Value = (TTAG)(dst.y);
				scaletags[4].tti_Tag = IMGTAG_WIDTH;            scaletags[4].tti_Value = (TTAG)(src.width);
				scaletags[5].tti_Tag = IMGTAG_HEIGHT;           scaletags[5].tti_Value = (TTAG)(src.height);
				scaletags[6].tti_Tag = IMGTAG_SCALEWIDTH;       scaletags[6].tti_Value = (TTAG)(dst.width);
				scaletags[7].tti_Tag = IMGTAG_SCALEHEIGHT;      scaletags[7].tti_Value = (TTAG)(dst.height);
				scaletags[8].tti_Tag = IMGTAG_SCALEMETHOD;      scaletags[8].tti_Value = (TTAG)IMGSMT_HARD;
				scaletags[9].tti_Tag = TTAG_DONE;

				if(dis->beginhandle)
				{
					bhandle=dis->beginhandle;
					TDisEnd(dis);
				}
				
				if(!isbitmap)
				{
					if(dismod_lock_dis(dis->theDisplay,&bufimg))
					{
						if(bufimg.data)
							TImgDoMethod(dis->imgp,img,&bufimg,IMGMT_SCALE,scaletags);
					}
				}
				else
				{
					if(dismod_lock_bm(dis->theDisplay,dis->bitmap,&bufimg))
					{
						if(bufimg.data)
							TImgDoMethod(dis->imgp,img,&bufimg,IMGMT_SCALE,scaletags);
					}
				}

				if(!isbitmap)
					dismod_unlock_dis(dis->theDisplay);
				else
					dismod_unlock_bm(dis->theDisplay,dis->bitmap);
			}
		}
		else
		{
			if(!isbitmap && dis->dismod->caps.canconvertdisplay)
				dismod_putimage_dis(dis->theDisplay,img,&src,&dst);
			else if(isbitmap && dis->dismod->caps.canconvertbitmap)
				dismod_putimage_bm(dis->theDisplay,dis->bitmap,img,&src,&dst);
			else
			{
				TIMGPICTURE bufimg;
				TTAGITEM convtags[7];

				convtags[0].tti_Tag = IMGTAG_SRCX;      convtags[0].tti_Value = (TTAG)(src.x);
				convtags[1].tti_Tag = IMGTAG_SRCY;      convtags[1].tti_Value = (TTAG)(src.y);
				convtags[2].tti_Tag = IMGTAG_DSTX;      convtags[2].tti_Value = (TTAG)(dst.x);
				convtags[3].tti_Tag = IMGTAG_DSTY;      convtags[3].tti_Value = (TTAG)(dst.y);
				convtags[4].tti_Tag = IMGTAG_WIDTH;     convtags[4].tti_Value = (TTAG)(src.width);
				convtags[5].tti_Tag = IMGTAG_HEIGHT;    convtags[5].tti_Value = (TTAG)(src.height);
				convtags[6].tti_Tag = TTAG_DONE;

				if(dis->beginhandle)
				{
					bhandle=dis->beginhandle;
					TDisEnd(dis);
				}
				
				if(!isbitmap)
				{
					if(dismod_lock_dis(dis->theDisplay,&bufimg))
						TImgDoMethod(dis->imgp,img,&bufimg,IMGMT_CONVERT,convtags);
				}
				else
				{
					if(dismod_lock_bm(dis->theDisplay,dis->bitmap,&bufimg))
						TImgDoMethod(dis->imgp,img,&bufimg,IMGMT_CONVERT,convtags);
				}

				if(!isbitmap)
					dismod_unlock_dis(dis->theDisplay);
				else
					dismod_unlock_bm(dis->theDisplay,dis->bitmap);
			}
		}
	}
	if(bhandle)
		TDisBegin(dis,bhandle);
}

/**************************************************************************
	TDisFill
 **************************************************************************/
TMODAPI TVOID TDisFill(TMOD_DISPLAYHANDLER *dis)
{
	if(!dis->emuldraw)
	{
		if(dis->drawtype==TDIS_DISPLAY)
			dismod_fill_dis(dis->theDisplay);
		else if(dis->drawtype==TDIS_BITMAP)
			dismod_fill_bm(dis->theDisplay);
	}
	else
	{
		if(dis->lock)
			TImgFill(dis->imgp,&dis->lockimg,&dis->dismod->drawpen.color);
	}
}

/**************************************************************************
	TDisPlot
 **************************************************************************/
TMODAPI TVOID TDisPlot(TMOD_DISPLAYHANDLER *dis,TINT x, TINT y)
{
	if(!dis->emuldraw)
	{
		if(dis->drawtype==TDIS_DISPLAY)
			dismod_plot_dis(dis->theDisplay,x,y);
		else if(dis->drawtype==TDIS_BITMAP)
			dismod_plot_bm(dis->theDisplay,x,y);
	}
	else
	{
		if(dis->lock)
			TImgPlot(dis->imgp,&dis->lockimg,x,y,&dis->dismod->drawpen.color);
	}
}

/**************************************************************************
	TDisLine
 **************************************************************************/
TMODAPI TVOID TDisLine(TMOD_DISPLAYHANDLER *dis, TINT sx, TINT sy, TINT ex, TINT ey)
{
	if(!dis->emuldraw)
	{
		if(dis->drawtype==TDIS_DISPLAY)
			dismod_line_dis(dis->theDisplay,sx,sy,ex,ey);
		else if(dis->drawtype==TDIS_BITMAP)
			dismod_line_bm(dis->theDisplay,sx,sy,ex,ey);
	}
	else
	{
		if(dis->lock)
			TImgLine(dis->imgp,&dis->lockimg,sx,sy,ex,ey,&dis->dismod->drawpen.color);
	}
}

/**************************************************************************
	TDisBox
 **************************************************************************/
TMODAPI TVOID TDisBox(TMOD_DISPLAYHANDLER *dis,TINT sx, TINT sy, TINT w, TINT h)
{
	if(!dis->emuldraw)
	{
		if(dis->drawtype==TDIS_DISPLAY)
			dismod_box_dis(dis->theDisplay,sx,sy,w,h);
		else if(dis->drawtype==TDIS_BITMAP)
			dismod_box_bm(dis->theDisplay,sx,sy,w,h);
	}
	else
	{
		if(dis->lock)
			TImgBox(dis->imgp,&dis->lockimg,sx,sy,w,h,&dis->dismod->drawpen.color);
	}
}

/**************************************************************************
	TDisBoxf
 **************************************************************************/
TMODAPI TVOID TDisBoxf(TMOD_DISPLAYHANDLER *dis,TINT sx, TINT sy, TINT w, TINT h)
{
	if(!dis->emuldraw)
	{
		if(dis->drawtype==TDIS_DISPLAY)
			dismod_boxf_dis(dis->theDisplay,sx,sy,w,h);
		else if(dis->drawtype==TDIS_BITMAP)
			dismod_boxf_bm(dis->theDisplay,sx,sy,w,h);
	}
	else
	{
		if(dis->lock)
			TImgBoxf(dis->imgp,&dis->lockimg,sx,sy,w,h,&dis->dismod->drawpen.color);
	}
}

/**************************************************************************
	TDisPoly
 **************************************************************************/
TMODAPI TVOID TDisPoly(TMOD_DISPLAYHANDLER *dis,TINT numpoints, TINT *points)
{
	if(!dis->emuldraw)
	{
		if(dis->drawtype==TDIS_DISPLAY)
			dismod_poly_dis(dis->theDisplay,numpoints,points);
		else if(dis->drawtype==TDIS_BITMAP)
			dismod_poly_bm(dis->theDisplay,numpoints,points);
	}
	else
	{
		if(dis->lock)
			TImgPoly(dis->imgp,&dis->lockimg,numpoints,points,&dis->dismod->drawpen.color);
	}
}

/**************************************************************************
	TDisPolyf
 **************************************************************************/
TMODAPI TVOID TDisPolyf(TMOD_DISPLAYHANDLER *dis,TINT numpoints, TINT *points)
{
	if(!dis->emuldraw)
	{
		if(dis->drawtype==TDIS_DISPLAY)
			dismod_polyf_dis(dis->theDisplay,numpoints,points);
		else if(dis->drawtype==TDIS_BITMAP)
			dismod_polyf_bm(dis->theDisplay,numpoints,points);
	}
	else
	{
		if(dis->lock)
			TImgPolyf(dis->imgp,&dis->lockimg,numpoints,points,&dis->dismod->drawpen.color);
	}
}

/**************************************************************************
	TDisEllipse
 **************************************************************************/
TMODAPI TVOID TDisEllipse(TMOD_DISPLAYHANDLER *dis,TINT x,TINT y,TINT rx,TINT ry)
{
	if(!dis->emuldraw)
	{
		if(dis->drawtype==TDIS_DISPLAY)
			dismod_ellipse_dis(dis->theDisplay,x,y,rx,ry);
		else if(dis->drawtype==TDIS_BITMAP)
			dismod_ellipse_bm(dis->theDisplay,x,y,rx,ry);
	}
	else
	{
		if(dis->lock)
			TImgEllipse(dis->imgp,&dis->lockimg,x,y,rx,ry,&dis->dismod->drawpen.color);
	}
}

/**************************************************************************
	TDisEllipsef
 **************************************************************************/
TMODAPI TVOID TDisEllipsef(TMOD_DISPLAYHANDLER *dis,TINT x,TINT y,TINT rx,TINT ry)
{
	if(!dis->emuldraw)
	{
		if(dis->drawtype==TDIS_DISPLAY)
			dismod_ellipsef_dis(dis->theDisplay,x,y,rx,ry);
		else if(dis->drawtype==TDIS_BITMAP)
			dismod_ellipsef_bm(dis->theDisplay,x,y,rx,ry);
	}
	else
	{
		if(dis->lock)
			TImgEllipsef(dis->imgp,&dis->lockimg,x,y,rx,ry,&dis->dismod->drawpen.color);
	}
}

/**************************************************************************
	TDisEllipsef
 **************************************************************************/
TMODAPI TVOID TDisMovePixels(TMOD_DISPLAYHANDLER *dis,TINT sx,TINT sy,TINT dx,TINT dy, TINT w, TINT h)
{
	if(!dis->emuldraw)
	{
		if(dis->drawtype==TDIS_DISPLAY)
			dismod_movepixels_dis(dis->theDisplay,sx,sy,dx,dy,w,h);
		else if(dis->drawtype==TDIS_BITMAP)
			dismod_movepixels_bm(dis->theDisplay,sx,sy,dx,dy,w,h);
	}
	else
	{
		// not implemented yet
	}
}

/**************************************************************************
 **************************************************************************

  private routines

 **************************************************************************
 **************************************************************************/

/**************************************************************************
	read in display props
 **************************************************************************/
TBOOL TDIS_ReadDisplayHostsProps(TMOD_DISPLAYHANDLER *dis)
{
	TAPTR dmod_base;
	TLIST modlist;
	TNODE *nextnode, *node;
	struct TModuleEntry *entry;
	TINT i,size;

	dis->disprops=(TDISPROPS*)TExecAlloc(dis->exec,TNULL,sizeof(TDISPROPS));

	TInitList(&modlist);
	if (TUtilGetModules(dis->util, dis->modname, &modlist, TNULL))
	{
		i=0;
		node = modlist.tlh_Head;
		while ((nextnode = node->tln_Succ))
		{
			entry = (struct TModuleEntry *) node;
			dmod_base = TExecOpenModule(dis->exec, entry->tme_Handle.tmo_Name, 0, TNULL);
			if(dmod_base)
			{
				dismod_getproperties(dmod_base,&dis->disprops[i]);
				if(dis->disprops[i].version>=DISPLAYHANDLER_VERSION)
				{
					size=TUtilStrLen(dis->util, entry->tme_Handle.tmo_Name)+1;
					dis->disprops[i].name=(TINT8*)TExecAlloc(dis->exec,TNULL,size);
					TExecCopyMem(dis->exec,entry->tme_Handle.tmo_Name,dis->disprops[i].name,size);

					i++;
					dis->disprops=(TDISPROPS*)TExecRealloc(dis->exec,dis->disprops,sizeof(TDISPROPS)*(i+1));
				}
				TExecCloseModule(dis->exec, dmod_base);
			}
			TRemove(node);
			TDestroy(node);
			node = nextnode;
		}
		dis->numdismods=i;
		if(dis->numdismods>1)
			TUtilQSort(dis->util,dis->disprops, dis->numdismods, sizeof(TDISPROPS), (TCMPFUNC)TDIS_ModulesSortFunc,TNULL);

		return TTRUE;
	}
	return TFALSE;
}

/**************************************************************************
	choose display module
 **************************************************************************/
TBOOL TDIS_ChooseDisplayModule(TMOD_DISPLAYHANDLER *dis, TTAGITEM *tags)
{
	TINT *legal_modules;
	TINT musthave_class, mustnothave_class;
	TINT musthave_mode, mustnothave_mode;
	TINT musthave_colors, mustnothave_colors;
	TINT minwidth,minheight;
	TINT i,j;
	
	musthave_class=(TINT)TGetTag(tags,TDISMUSTHAVE_CLASS,TDISCLASS_STANDARD);
	mustnothave_class=(TINT)TGetTag(tags,TDISMUSTNOTHAVE_CLASS,0);
	musthave_mode=(TINT)TGetTag(tags,TDISMUSTHAVE_MODE,TDISMODE_WINDOW);
	mustnothave_mode=(TINT)TGetTag(tags,TDISMUSTNOTHAVE_MODE,0);
	musthave_colors=(TINT)TGetTag(tags,TDISMUSTHAVE_COLORS,TDISCOLORS_TRUECOLOR);
	mustnothave_colors=(TINT)TGetTag(tags,TDISMUSTNOTHAVE_COLORS,0);

	minwidth=(TINT)TGetTag(tags,TDIS_MINWIDTH,1);
	minheight=(TINT)TGetTag(tags,TDIS_MINHEIGHT,1);

	/* build name prefix for module */
	TExecFillMem(dis->exec,dis->modname,64,0);
	TUtilStrCpy(dis->util,dis->modname,"display");

	switch(musthave_mode)
	{
		case TDISMODE_WINDOW:
			TUtilStrCat(dis->util,dis->modname,"_window");
		break;

		case TDISMODE_FULLSCREEN:
			TUtilStrCat(dis->util,dis->modname,"_full");
		break;
	}

	switch(musthave_class)
	{
		case TDISCLASS_STANDARD:
			TUtilStrCat(dis->util,dis->modname,"_std");
		break;

		case TDISCLASS_OPENGL:
			TUtilStrCat(dis->util,dis->modname,"_gl");
		break;
	}

	/* read list of display modules fitting the above build name prefix */
	TInitList(&dis->displaymodules_list);
	if(!(TDIS_ReadDisplayHostsProps(dis)))
	{
		TDestroyList(&dis->displaymodules_list);
		return -1;
	}
	
	/* now choose a module from this list */
	legal_modules=(TINT*)TExecAlloc(dis->exec,TNULL,dis->numdismods*4);
	for(i=0;i<dis->numdismods;i++)
		legal_modules[i]=1;

	for(i=0;i<dis->numdismods;i++)
	{
		if(dis->disprops[i].dispclass!=musthave_class)
			legal_modules[i]=0;

		if(dis->disprops[i].dispclass==mustnothave_class)
			legal_modules[i]=0;

		if(dis->disprops[i].dispmode!=musthave_mode)
			legal_modules[i]=0;

		if(dis->disprops[i].dispmode==mustnothave_mode)
			legal_modules[i]=0;

		if(dis->disprops[i].maxdepth<=8 && musthave_colors==TDISCOLORS_TRUECOLOR)
			legal_modules[i]=0;

		if(dis->disprops[i].mindepth>8 && musthave_colors==TDISCOLORS_CLUT)
			legal_modules[i]=0;

		if(dis->disprops[i].maxwidth<minwidth)
			legal_modules[i]=0;

		if(dis->disprops[i].maxheight<minheight)
			legal_modules[i]=0;
	}


       /* and look, if we found something */
	i=0;
	j=-1;
	do
	{
		if(legal_modules[i])
			j=i;
		else
			i++;
	}while(j==-1 && i<dis->numdismods);

	TExecFree(dis->exec,legal_modules);

	if(j>=0)
	{
	    TExecFillMem(dis->exec,dis->modname,64,0);
	    TUtilStrCpy(dis->util,dis->modname,dis->disprops[j].name);
	}

	/* clean up the modules list */
	if(dis->disprops)
	{
		TINT i;
		for(i=0;i<dis->numdismods;i++)
		{
			if(dis->disprops[i].name)
				TExecFree(dis->exec,dis->disprops[i].name);
		}
		TExecFree(dis->exec,dis->disprops);
	}
	TDestroyList(&dis->displaymodules_list);

	if(j>=0)
		return TTRUE;
	else
		return TFALSE;
}

/**************************************************************************
	Callback for qsort of modules
 **************************************************************************/
TCALLBACK TINT TDIS_ModulesSortFunc(TAPTR udata, TAPTR e1, TAPTR e2)
{
	TDISPROPS *dp1 = (TDISPROPS*)e1;
	TDISPROPS *dp2 = (TDISPROPS*)e2;

	if(dp1->priority<dp2->priority)
		return -1;
	if(dp1->priority>dp2->priority)
		return 1;

	return 0;
}

/**************************************************************************
	destroyfunc for displaymodule
 **************************************************************************/
TCALLBACK TVOID TDIS_DestroyDisplayModule(THNDL *handle)
{
	if(handle)
	{
		TDISMODULE *d=(TDISMODULE *)handle->thn_Data;
		if(d)
		{
			TMOD_DISPLAYHANDLER *dis=d->displayhandler;

			if(d->palette)
				TExecFree(dis->exec,d->palette);

			TDestroyList(&d->bitmaps_list);
			TDestroyList(&d->pens_list);

			TExecCloseModule(dis->exec,d->modul);
			TRemove((TNODE*)handle);

			TExecFree(dis->exec,d);
			TExecFree(dis->exec,handle);
		}
	}
}

/**************************************************************************
	destroyfunc for pen
 **************************************************************************/
TCALLBACK TVOID TDIS_DestroyPen(THNDL *penhndl)
{
	if(penhndl)
	{
		TDISPEN *pen=(TDISPEN*)penhndl->thn_Data;
		if(pen)
		{
			THNDL *dishost=pen->display;
			TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
			TMOD_DISPLAYHANDLER *dis=dismod->displayhandler;
			TAPTR theDisplay=dismod->modul;

			dismod_freepen(theDisplay,pen);

			TRemove((TNODE*)penhndl);

			TExecFree(dis->exec,pen);
			TExecFree(dis->exec,penhndl);
		}
	}
}

/**************************************************************************
	destroyfunc for bitmap
 **************************************************************************/
TCALLBACK TVOID TDIS_DestroyBitmap(THNDL *bmhndl)
{
	if(bmhndl)
	{
		TDISBITMAP *bitmap=(TDISBITMAP*)bmhndl->thn_Data;
		if(bitmap)
		{
			THNDL *dishost=bitmap->display;
			TDISMODULE *dismod=(TDISMODULE*)dishost->thn_Data;
			TMOD_DISPLAYHANDLER *dis=dismod->displayhandler;
			TAPTR theDisplay=dismod->modul;

			dismod_freebitmap(theDisplay,bitmap);

			TRemove((TNODE*)bmhndl);

			TExecFree(dis->exec,bitmap);
			TExecFree(dis->exec,bmhndl);
		}
	}
}

/**************************************************************************
	callback for qsort of displaymodes
 **************************************************************************/
TCALLBACK TINT TDIS_ModeSortFunc(TAPTR udata, TTAG e1, TTAG e2)
{
	TDISMODE* m1 = (TDISMODE*)e1;
	TDISMODE* m2 = (TDISMODE*)e2;

	if(m1->depth<m2->depth)
		return -1;
	if(m1->depth>m2->depth)
		return 1;

	if(m1->width<m2->width)
		return -1;

	if(m1->width>m2->width)
		return 1;

	if(m1->height<m2->height)
		return -1;

	if(m1->height>m2->height)
		return 1;

	return 0;
}
