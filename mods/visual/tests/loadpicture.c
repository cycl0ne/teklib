
/*
**	$Id: loadpicture.c,v 1.9 2005/09/08 00:05:42 tmueller Exp $
**	apps/tests/loadpicture.c - Picture/Anim datatypes test
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/time.h>
#include <tek/proto/io.h>
#include <tek/proto/visual.h>
#include <tek/proto/datatypehandler.h>
#include <tek/proto/imgproc.h>

#include <stdio.h>

TVOID shutup(TSTRPTR errormessage);
TBOOL loadpicture(THNDL *pichandle, TTAGITEM *pictags, TIMGPICTURE *pic);
TVOID freepicture(TIMGPICTURE *pic);
TVOID showpicture(TIMGPICTURE *pic, TSTRPTR windowname);
TVOID savepicture(TIMGPICTURE *pic, TSTRPTR name, TSTRPTR saveformat,TINT compression);
TVOID showanimation(THNDL *animhandle, TTAGITEM *animtags);

TAPTR TExecBase;
TAPTR TIOBase;
TAPTR TDTHBase;
TAPTR TImgProcBase;
TAPTR TUtilBase;
TAPTR TTimeBase;
struct TTimeReq *treq;

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TINT tekloadfilenamelen;
	TTAGITEM *loadfiletags;
	TAPTR loadfilehandle;
	THNDL *loadfile;
	TSTRPTR tekloadfilename;

	TSTRPTR loadfilename = TNULL;
	TSTRPTR savefilename = TNULL;
	TSTRPTR saveformat = TNULL;
	TINT compression=20;
	TBOOL listreadformats=TFALSE;
	TBOOL listwriteformats=TFALSE;
	
	TTAG args[11];
	TSTRPTR *argv;
	TAPTR arghandle;

	TSTRPTR tmpl = "?=help/S,-s=SOURCE,-d=DEST/K,-f=DESTFORMAT/K,-c=COMPRESSION/N/K,-lr=LISTREADFORMATS/S,-lw=LISTWRITEFORMATS/S";
	
	TExecBase = TGetExecBase(task);
	TUtilBase = TExecOpenModule(TExecBase, "util", 0, TNULL);
	TIOBase = TExecOpenModule(TExecBase, "io", 0, TNULL);
	TDTHBase = TExecOpenModule(TExecBase, "datatypehandler", 0, TNULL);
	TImgProcBase = TExecOpenModule(TExecBase, "imgproc", 0, TNULL);
	TTimeBase = TExecOpenModule(TExecBase, "time", 0, TNULL);

	if (TUtilBase && TIOBase && TImgProcBase && TTimeBase && TDTHBase)
	{
		treq = TTimeAllocTimeRequest(TTimeBase, TNULL);
		if (treq)
		{
			// set default options for commandline args
			args[0]=TFALSE;
			args[1]=(TTAG)loadfilename;
			args[2]=(TTAG)savefilename;
			args[3]=(TTAG)saveformat;
			args[4]=(TTAG)&compression;
			args[5]=(TTAG)listreadformats;
			args[6]=(TTAG)listwriteformats;

			// get commandline
			argv = TUtilGetArgV(TUtilBase);
			arghandle = TUtilParseArgV(TUtilBase, tmpl, argv+1, args);
			if(arghandle)
			{
				if(!args[0])
				{
					loadfilename = (TSTRPTR)args[1];
					savefilename = (TSTRPTR)args[2];
					saveformat = (TSTRPTR)args[3];
					compression  = *(TINT*)args[4];
					listreadformats = args[5];
					listwriteformats = args[6];
					TDestroy(arghandle);
				}
				else
				{
					TDestroy(arghandle);
					printf("%s\n",TUtilStrChr(TUtilBase,tmpl,',')+1);
					shutup(TNULL);
					return;
				}
			}

			/* list datatypes capable to read pictures */
			if(listreadformats)
			{
				TTAGITEM filter[3];
				TLIST *filterlist;
				TNODE *nextnode, *node;
				DTDescData *ddat;

				filter[0].tti_Tag = TDFILTER_CLASS; filter[0].tti_Value = (TTAG)DTCLASS_PIC;
				filter[1].tti_Tag = TTAG_DONE;

				filterlist=TDthListDatatypes(TDTHBase,filter);

				printf("\navailable picture datatypes for read:\n\n");
				node = filterlist->tlh_Head;
				while ((nextnode = node->tln_Succ))
				{
					DTListItem *li=(DTListItem*)node;
					ddat=li->li_ddat;

					printf("\t%s\t-\t%s\n",ddat->datatypeshortname,ddat->datatypefullname);

					node = nextnode;
				}
				printf("\n\n");
			}

			/* list datatypes capable to write pictures */
			if(listwriteformats)
			{
				TTAGITEM filter[3];
				TLIST *filterlist;
				TNODE *nextnode, *node;
				DTDescData *ddat;

				filter[0].tti_Tag = TDFILTER_CLASS;		filter[0].tti_Value = (TTAG)DTCLASS_PIC;
				filter[1].tti_Tag = TDFILTER_CANSAVE;	filter[1].tti_Value = (TTAG)TTRUE;
				filter[2].tti_Tag = TTAG_DONE;

				filterlist=TDthListDatatypes(TDTHBase,filter);

				printf("\navailable picture datatypes for write:\n\n");
				node = filterlist->tlh_Head;
				while ((nextnode = node->tln_Succ))
				{
					DTListItem *li=(DTListItem*)node;
					ddat=li->li_ddat;

					printf("\t%s\t-\t%s\n",ddat->datatypeshortname,ddat->datatypefullname);

					node = nextnode;
				}
				printf("\n\n");
			}

			/* load picture */
			if (loadfilename)
			{
				/* open the file */
				tekloadfilenamelen = TIOMakeName(TIOBase, loadfilename,TNULL,0,TPPF_HOST2TEK,TNULL);
				if(tekloadfilenamelen==-1)
					printf("\n\tCouldn't translate loadfilename!\n");
				else
				{
					tekloadfilename=TExecAlloc0(TExecBase, TNULL,tekloadfilenamelen+1);
					TIOMakeName(TIOBase,loadfilename,tekloadfilename,tekloadfilenamelen+1,TPPF_HOST2TEK,TNULL);
					loadfilehandle = TIOOpenFile(TIOBase, tekloadfilename, TFMODE_READONLY, TNULL);
					if(!loadfilehandle)
					{
						printf("\n\tcouldn't open %s\n\n",tekloadfilename);
					}
					else
					{
						TTAGITEM opentags[2];

						opentags[0].tti_Tag = TDOPENTAG_FILEHANDLE;	opentags[0].tti_Value = (TTAG)loadfilehandle;
						opentags[1].tti_Tag = TTAG_DONE;

						loadfile=TDthOpen(TDTHBase,opentags);
		
						if(!loadfile)
						{
							printf("\n\t%s - file not found or unknown fileformat!\n\n",loadfilename);
						}
						else
						{
							loadfiletags=TDthGetAttrs(TDTHBase,loadfile);
		
							if((TINT)TGetTag(loadfiletags,TDTAG_CLASS,0)==DTCLASS_PIC)
							{
								TIMGPICTURE pic;
								if(loadpicture(loadfile,loadfiletags,&pic))
								{
									if(savefilename && saveformat)
									{
										savepicture(&pic,savefilename,saveformat,compression);
									}
									else
										showpicture(&pic,(TSTRPTR)TGetTag(loadfiletags,TDTAG_FULLNAME,(TTAG)""));

									freepicture(&pic);
								}
							}
							else if((TINT)TGetTag(loadfiletags,TDTAG_CLASS,0)==DTCLASS_ANIM)
							{
								showanimation(loadfile,loadfiletags);
							}
							TDestroy(loadfile);
						}
						TIOCloseFile(TIOBase, loadfilehandle);
					}
					TExecFree(TExecBase, tekloadfilename);
				}
			}
		}
	}
	shutup(TNULL);
}

/***********************************************************************
	shutup
 ***********************************************************************/
TVOID shutup(TSTRPTR errormessage)
{
	if(treq && TTimeBase) TTimeFreeTimeRequest(TTimeBase, treq);	
	if(TTimeBase) TExecCloseModule(TExecBase, TTimeBase);
	if(TDTHBase) TExecCloseModule(TExecBase, TDTHBase);
	if(TImgProcBase) TExecCloseModule(TExecBase, TImgProcBase);
	if(TIOBase) TExecCloseModule(TExecBase, TIOBase);
	if(TUtilBase) TExecCloseModule(TExecBase, TUtilBase);

	if(errormessage) printf("\n\tError: %s\n\n",errormessage);
}

/***********************************************************************
	Load Picture
 ***********************************************************************/
TBOOL loadpicture(THNDL *pichandle, TTAGITEM *pictags, TIMGPICTURE *pic)
{
	TINT width,height,depth,bytesperrow,format;
	TTAGITEM picloadtags[3];

	TUINT8 *picdata=TNULL;
	TIMGARGBCOLOR *picpalette=TNULL;


	/* load the picture */
	width=(TINT)TGetTag(pictags,TDTAG_PICWIDTH,0);
	height=(TINT)TGetTag(pictags,TDTAG_PICHEIGHT,0);
	depth=(TINT)TGetTag(pictags,TDTAG_PICDEPTH,0);
	bytesperrow=(TINT)TGetTag(pictags,TDTAG_PICBYTESPERROW,0);
	format=(TINT)TGetTag(pictags,TDTAG_PICFORMAT,0);

	/* datatypes don't allocate memory for loading, thats's your job */
	picdata=TExecAlloc0(TExecBase, TNULL,bytesperrow*height);

	picloadtags[0].tti_Tag = TDOTAG_GETDATA; picloadtags[0].tti_Value = (TTAG) picdata;

	if(depth<=8)
	{
		picpalette=(TIMGARGBCOLOR*)TExecAlloc0(TExecBase, TNULL,sizeof(TIMGARGBCOLOR)*256);
		picloadtags[1].tti_Tag = TDOTAG_GETPALETTE; picloadtags[1].tti_Value = (TTAG) picpalette;
		picloadtags[2].tti_Tag = TTAG_DONE;
	}
	else
		picloadtags[1].tti_Tag = TTAG_DONE;

	/* call datatypehandlers domethod for loading the data */
	if(TDthDoMethod(TDTHBase,pichandle,picloadtags))
	{
		pic->data=picdata;
		pic->palette=picpalette;
		pic->format=format;
		pic->width=width;
		pic->height=height;
		pic->depth=depth;
		pic->bytesperrow=bytesperrow;

		return TTRUE;
	}
	return TFALSE;
}

/***********************************************************************
	Free Picture
 ***********************************************************************/
TVOID freepicture(TIMGPICTURE *pic)
{
	if(pic->palette) TExecFree(TExecBase, pic->palette);
	if(pic->data) TExecFree(TExecBase, pic->data);
}

/***********************************************************************
	Show Picture
 ***********************************************************************/
TVOID showpicture(TIMGPICTURE *pic, TSTRPTR windowname)
{
	TAPTR v;
	TTAGITEM vistags[4];
	TTAGITEM sizetags[3];
	TTAGITEM imgperttags[16];
	TINT nw, nh, ow, oh;
	TBOOL refresh;
	TAPTR buffer;

	TIMGPICTURE showpic;

	/* prepare visual */

	vistags[0].tti_Tag = TVisual_PixWidth; vistags[0].tti_Value = (TTAG) pic->width;
	vistags[1].tti_Tag = TVisual_PixHeight; vistags[1].tti_Value = (TTAG) pic->height;
	vistags[2].tti_Tag = TVisual_Title; vistags[2].tti_Value = (TTAG) windowname;
	vistags[3].tti_Tag = TTAG_DONE;

	/* prepare scaling */
	sizetags[0].tti_Tag = TVisual_PixWidth; sizetags[0].tti_Value = (TTAG) &nw;
	sizetags[1].tti_Tag = TVisual_PixHeight; sizetags[1].tti_Value = (TTAG) &nh;
	sizetags[2].tti_Tag = TTAG_DONE;

	imgperttags[0].tti_Tag = IMGTAG_SCALEMETHOD; imgperttags[0].tti_Value = (TTAG) IMGSMT_SMOOTH;
	imgperttags[1].tti_Tag = TTAG_DONE;

	/* create visual and show picture */
	v = TExecOpenModule(TExecBase, "visual", 0, vistags);
	if (v)
	{
		TIMSG *imsg;
		TBOOL abort = TFALSE;
		TAPTR iport;

		TVPEN pentab;

		pentab = TVisualAllocPen(v, 0);
		TVisualClear(v, pentab);

		TVisualSetInput(v, TITYPE_NONE, TITYPE_CLOSE | TITYPE_COOKEDKEY | TITYPE_NEWSIZE | TITYPE_REFRESH | TITYPE_FOCUS);

		iport = TVisualGetPort(v);

		ow=pic->width;
		oh=pic->height;
		nw=pic->width;
		nh=pic->height;

		refresh=TTRUE;

		buffer=TExecAlloc(TExecBase, TNULL,pic->width*pic->height*4);

		/* first convert to A8R8G8B8 */
		showpic.data=buffer;
		showpic.palette=TNULL;
		showpic.format=IMGFMT_A8R8G8B8;
		showpic.width=nw;
		showpic.height=nh;
		showpic.depth=32;
		showpic.bytesperrow=nw*4;

		TExecFillMem(TExecBase,buffer,nw*4*nh,0);
		TImgDoMethod(TImgProcBase,pic,&showpic,IMGMT_CONVERT,TNULL);

		do
		{
			if(refresh)
			{
				if(nw!=ow || nh!=oh)
				{
					buffer=TExecRealloc(TExecBase, buffer,nw*nh*4);
					ow=nw;
					oh=nh;

					showpic.data=buffer;
					showpic.palette=TNULL;
					showpic.format=IMGFMT_A8R8G8B8;
					showpic.width=nw;
					showpic.height=nh;
					showpic.depth=32;
					showpic.bytesperrow=nw*4;

					TExecFillMem(TExecBase,buffer,nw*4*nh,0);
					
					TImgDoMethod(TImgProcBase,pic,&showpic,IMGMT_SCALE,imgperttags);
				}

				TVisualDrawRGB(v,0,0,(TUINT*)buffer,nw,nh,nw);
				TVisualFlush(v);
				refresh=TFALSE;
			}

			TExecWait(TExecBase, TExecGetPortSignal(TExecBase, iport));
			while ((imsg = (TIMSG *) TExecGetMsg(TExecBase, iport)))
			{
				switch (imsg->timsg_Type)
				{
					case TITYPE_NEWSIZE: case TITYPE_REFRESH: case TITYPE_FOCUS:
						TVisualGetAttrs(v, sizetags);
						refresh=TTRUE;
					break;

					case TITYPE_CLOSE:
						abort = TTRUE;
					break;

					case TITYPE_COOKEDKEY:
						if (imsg->timsg_Code == TKEYC_ESC)
						{
							abort = TTRUE;
						}
					break;
				}
				TExecAckMsg(TExecBase, imsg);
			}
		} while (!abort);

		TExecFree(TExecBase, buffer);
		TVisualFreePen(v, pentab);
		TExecCloseModule(TExecBase, v);
	}
}

/***********************************************************************
	Save Picture
 ***********************************************************************/
TVOID savepicture(TIMGPICTURE *pic, TSTRPTR name, TSTRPTR saveformat, TINT compression)
{
	TINT teksavefilenamelen;
	TAPTR savefilehandle;
	THNDL *savefile;
	TTAGITEM savefiletags[4];

	TTAGITEM filter[4];
	TLIST *filterlist;
	TNODE *node;
	DTDescData *ddat;

	filter[0].tti_Tag = TDFILTER_CLASS;		filter[0].tti_Value = (TTAG)DTCLASS_PIC;
	filter[1].tti_Tag = TDFILTER_CANSAVE;	filter[1].tti_Value = (TTAG)TTRUE;
	filter[2].tti_Tag = TDFILTER_SHORTNAME;	filter[2].tti_Value = (TTAG)saveformat;
	filter[3].tti_Tag = TTAG_DONE;

	filterlist=TDthListDatatypes(TDTHBase,filter);

	node = filterlist->tlh_Head;
	if(!node->tln_Succ)
	{
		printf("\n\tNo datatype for format %s found!\n",saveformat);
	}
	else
	{
		DTListItem *li=(DTListItem*)node;
		ddat=li->li_ddat;

		teksavefilenamelen = TIOMakeName(TIOBase, name,TNULL,0,TPPF_HOST2TEK,TNULL);
		if(teksavefilenamelen==-1)
		{
			TINT8 errbuf[128];
			TIOFault(TIOBase,TIOGetIOErr(TIOBase),errbuf,128,TNULL);
			printf("\n\tCouldn't translate savefilename!\n");
			printf("\tio_error: %s!\n",errbuf);
		}
		else
		{
			TTAGITEM opentags[2];

			opentags[0].tti_Tag = TDOPENTAG_DTNAME;	opentags[0].tti_Value = (TTAG)ddat->datatypecodecname;
			opentags[1].tti_Tag = TTAG_DONE;

			savefile=TDthOpen(TDTHBase,opentags);
			if(savefile)
			{
				TTAGITEM *saveattrs=TDthGetAttrs(TDTHBase,savefile);
				TSTRPTR teksavefilename;
				
				TINT maxwidth=(TUINT)TGetTag(saveattrs,TDTAG_SAVEPICMAXWIDTH,pic->width+1);
				TINT maxheight=(TUINT)TGetTag(saveattrs,TDTAG_SAVEPICMAXHEIGHT,pic->height+1);
				
				if(maxwidth<pic->width || maxheight<pic->height)
				{
					printf("\n\tpicture exceeds maximum dimensions for %s format!\n",saveformat);
					TDestroy(savefile);
					return;
				}

				teksavefilename=TExecAlloc0(TExecBase, TNULL,teksavefilenamelen+1);
				TIOMakeName(TIOBase,name,teksavefilename,teksavefilenamelen+1,TPPF_HOST2TEK,TNULL);
				savefilehandle = TIOOpenFile(TIOBase, teksavefilename, TFMODE_NEWFILE, TNULL);
				if(!savefilehandle)
				{
					printf("\n\tcouldn't open %s for write!\n\n",teksavefilename);
					TDestroy(savefile);
					TExecFree(TExecBase, teksavefilename);
					return;
				}

				savefiletags[0].tti_Tag = TDOTAG_WRITEFILE;		savefiletags[0].tti_Value = (TTAG)savefilehandle;
				savefiletags[1].tti_Tag = TDOTAG_SETDATA;		savefiletags[1].tti_Value = (TTAG)pic;
				savefiletags[2].tti_Tag = TDOTAG_COMPRESSION;	savefiletags[2].tti_Value = (TTAG)compression;
				savefiletags[3].tti_Tag = TTAG_DONE;

				TDthDoMethod(TDTHBase,savefile,savefiletags);

				TIOCloseFile(TIOBase, savefilehandle);
				TDestroy(savefile);
				TExecFree(TExecBase, teksavefilename);
			}
		}
	}
}

/***********************************************************************
	Show Animation
 ***********************************************************************/
TVOID showanimation(THNDL *animhandle, TTAGITEM *animtags)
{
	TTIME t0, t1;
	TFLOAT elapsed,framespeed;
	TINT numframes;
	TINT width,height,depth,bytesperrow,format,ow,oh,nw,nh;
	TAPTR v;
	TTAGITEM picloadtags[4];
	TTAGITEM vistags[4];
	TTAGITEM sizetags[3];
	TTAGITEM imgperttags[16];
	TBOOL refresh=TFALSE;
	TIMGPICTURE loadedpic, theFrame, scaledFrame;
	TINT8 windowname[256];

	TUINT8 *picdata=TNULL;
	TUINT8 *condata=TNULL;
	TUINT8 *scaledata=TNULL;
	TIMGARGBCOLOR *picpalette=TNULL;

	/* prepare data for visual and frame loading */
	width=(TINT)TGetTag(animtags,TDTAG_PICWIDTH,0);
	height=(TINT)TGetTag(animtags,TDTAG_PICHEIGHT,0);
	depth=(TINT)TGetTag(animtags,TDTAG_PICDEPTH,0);
	bytesperrow=(TINT)TGetTag(animtags,TDTAG_PICBYTESPERROW,0);
	format=(TINT)TGetTag(animtags,TDTAG_PICFORMAT,0);

	framespeed=(TFLOAT)((TINT)TGetTag(animtags,TDTAG_ANIMFRAMETIME,0))/1000.0f;
	numframes=(TINT)TGetTag(animtags,TDTAG_ANIMNUMFRAMES,0);

	/* datatypes don't allocate memory for loading, thats's your job */
	picdata=TExecAlloc0(TExecBase, TNULL,bytesperrow*height);
	condata=TExecAlloc0(TExecBase, TNULL,4*width*height);
	scaledata=TExecAlloc0(TExecBase, TNULL,4*width*height);

	/* prepare TAG-List for loading via datatypehandler */
	picloadtags[0].tti_Tag = TDOTAG_ANIMSTEP; picloadtags[0].tti_Value = (TTAG) DTANIM_FORWARD;
	picloadtags[1].tti_Tag = TDOTAG_GETDATA; picloadtags[1].tti_Value = (TTAG) picdata;
	picloadtags[2].tti_Tag = TTAG_DONE;

	if(depth<=8)
	{
		picpalette=(TIMGARGBCOLOR*)TExecAlloc0(TExecBase, TNULL,sizeof(TIMGARGBCOLOR)*256);
		picloadtags[2].tti_Tag = TDOTAG_GETPALETTE; picloadtags[2].tti_Value = (TTAG) picpalette;
		picloadtags[3].tti_Tag = TTAG_DONE;
	}

	/* prepare structs for converting frames */
	loadedpic.data=picdata;
	loadedpic.palette=picpalette;
	loadedpic.format=format;
	loadedpic.width=width;
	loadedpic.height=height;
	loadedpic.depth=depth;
	loadedpic.bytesperrow=bytesperrow;

	theFrame.data=condata;
	theFrame.palette=TNULL;
	theFrame.format=IMGFMT_A8R8G8B8;
	theFrame.width=width;
	theFrame.height=height;
	theFrame.depth=32;
	theFrame.bytesperrow=width*4;

	imgperttags[0].tti_Tag = IMGTAG_SCALEMETHOD; imgperttags[0].tti_Value = (TTAG) IMGSMT_SMOOTH;
	imgperttags[1].tti_Tag = TTAG_DONE;

	/* prepare visual */
	sprintf(windowname,"%s",(TINT8*)TGetTag(animtags,TDTAG_FULLNAME,0));

	vistags[0].tti_Tag = TVisual_PixWidth; vistags[0].tti_Value = (TTAG) width;
	vistags[1].tti_Tag = TVisual_PixHeight; vistags[1].tti_Value = (TTAG) height;
	vistags[2].tti_Tag = TVisual_Title; vistags[2].tti_Value = (TTAG) windowname;
	vistags[3].tti_Tag = TTAG_DONE;

	sizetags[0].tti_Tag = TVisual_PixWidth; sizetags[0].tti_Value = (TTAG) &nw;
	sizetags[1].tti_Tag = TVisual_PixHeight; sizetags[1].tti_Value = (TTAG) &nh;
	sizetags[2].tti_Tag = TTAG_DONE;

	ow=nw=width;
	oh=nh=height;

	/* create visual and start animation */
	v = TExecOpenModule(TExecBase, "visual", 0, vistags);
	if (v)
	{
		TIMSG *imsg;
		TBOOL abort = TFALSE;
		TAPTR iport;
		TVPEN pentab;

		pentab = TVisualAllocPen(v, 0);
		TVisualClear(v, pentab);
		TVisualSetInput(v, TITYPE_NONE, TITYPE_CLOSE | TITYPE_COOKEDKEY |
			TITYPE_NEWSIZE | TITYPE_REFRESH | TITYPE_FOCUS);
		iport = TVisualGetPort(v);

		TTimeQueryTime(TTimeBase, treq, &t0);

		/* THE loop */
		do
		{
			TTimeQueryTime(TTimeBase, treq, &t0);

			/* call datatypehandlers domethod for loading the data */
			if(!TDthDoMethod(TDTHBase,animhandle,picloadtags))
			{
				abort=TTRUE;
			}
			if(!abort)
			{
				/* convert the frame, if needed */
				if(format!=IMGFMT_A8R8G8B8)
				{
					TImgDoMethod(TImgProcBase,&loadedpic,&theFrame,IMGMT_CONVERT,TNULL);
				}

				if(ow!=nw || oh!=nh)
				{
					scaledata=TExecRealloc(TExecBase, scaledata,nw*nh*4);

					scaledFrame.data=scaledata;
					scaledFrame.palette=picpalette;
					scaledFrame.format=IMGFMT_A8R8G8B8;
					scaledFrame.width=nw;
					scaledFrame.height=nh;
					scaledFrame.depth=32;
					scaledFrame.bytesperrow=nw*4;

					ow=nw;
					oh=nh;
				}

				/* redraw frame */
				if(ow!=loadedpic.width || oh!=loadedpic.height)
				{
					TImgDoMethod(TImgProcBase,&theFrame,&scaledFrame,IMGMT_SCALE,imgperttags);
					TVisualDrawRGB(v,0,0,(TUINT*)scaledFrame.data,scaledFrame.width,scaledFrame.height,scaledFrame.width);
				}
				else if(loadedpic.format!=IMGFMT_A8R8G8B8)
					TVisualDrawRGB(v,0,0,(TUINT*)theFrame.data,theFrame.width,theFrame.height,theFrame.width);
				else
					TVisualDrawRGB(v,0,0,(TUINT*)loadedpic.data,loadedpic.width,loadedpic.height,loadedpic.width);

				TVisualFlush(v);

				/* message handling */
				while ((imsg = (TIMSG *) TExecGetMsg(TExecBase, iport)))
				{
					switch (imsg->timsg_Type)
					{
						case TITYPE_NEWSIZE: case TITYPE_REFRESH: case TITYPE_FOCUS:
							TVisualGetAttrs(v, sizetags);
							refresh=TTRUE;
						break;

						case TITYPE_CLOSE:
							abort = TTRUE;
						break;

						case TITYPE_COOKEDKEY:
							if (imsg->timsg_Code == TKEYC_ESC)
							{
								abort = TTRUE;
							}
						break;
					}
					TExecAckMsg(TExecBase, imsg);
				}
			}

			/* sync animation */
			TTimeQueryTime(TTimeBase, treq, &t1);
			TTimeSubTime(TTimeBase, &t1, &t0);
			elapsed = (TFLOAT) t1.ttm_USec / 1000000 + t1.ttm_Sec;
			if(elapsed<framespeed)
			{
				TFLOAT delta = framespeed - elapsed;
				t0.ttm_Sec = (TINT) delta;
				t0.ttm_USec = (TINT) ((delta - t0.ttm_Sec) * 1000000);
				TTimeDelay(TTimeBase, treq, &t0);
			}
		} while (!abort);

		/* clean up */
		TExecFree(TExecBase, picpalette);
		TExecFree(TExecBase, picdata);
		TExecFree(TExecBase, condata);
		TExecFree(TExecBase, scaledata);

		TVisualFreePen(v, pentab);
		TExecCloseModule(TExecBase, v);
	}
}
