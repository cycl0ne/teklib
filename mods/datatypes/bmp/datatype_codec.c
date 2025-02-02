
/*
**      datatype codec module
**
*/

#include "codec_bmp.h"
#include <tek/teklib.h>

#define MOD_VERSION		0
#define MOD_REVISION	1

/*
**      module prototypes
*/
static TCALLBACK TMOD_DTCODEC *mod_open(TMOD_DTCODEC *mod, TAPTR selftask, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_DTCODEC *dtcodec, TAPTR selftask);

TMODAPI TTAGITEM* dtcodec_getattrs(TMOD_DTCODEC *dtcodec);
TMODAPI TBOOL dtcodec_domethod(TMOD_DTCODEC *dtcodec, TTAGITEM *taglist);

#define TSetTag(tp,t,val) { (tp).tti_Tag = (t); (tp).tti_Value = (TTAG)(val); }
#define TSetTagDone(tp)	{ (tp).tti_Tag = TTAG_DONE; }

/*
**      tek_init_<modname>
**      all initializations that are not instance specific.
*/
TMODENTRY TUINT tek_init_datatype_codec_bmp(TAPTR selftask, TMOD_DTCODEC *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)                                  /* first call */
		{
			if (version <= MOD_VERSION)                     /* version check */
			{
				return sizeof(TMOD_DTCODEC);    /* return module positive size */
			}
		}
		else                                                                    /* second call */
		{
			return sizeof(TAPTR) * 2;                       /* return module negative size */
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
		((TAPTR *) mod)[-1 ] = (TAPTR) dtcodec_getattrs;
		((TAPTR *) mod)[-2 ] = (TAPTR) dtcodec_domethod;

		return TTRUE;
	}

	return 0;
}

/*
**      open instance
*/
static TCALLBACK TMOD_DTCODEC *mod_open(TMOD_DTCODEC *dtcodec, TAPTR selftask, TTAGITEM *tags)
{
	TAPTR fp=(TAPTR)TGetTag(tags,TDCTAG_FILEHANDLE,TNULL);
	TBOOL write=(TBOOL)TGetTag(tags,TDCTAG_WRITE,TFALSE);

	if(fp || write)
	{
		dtcodec = TNewInstance(dtcodec, dtcodec->module.tmd_PosSize, dtcodec->module.tmd_NegSize);
		if (dtcodec)
		{
			dtcodec->io = TExecOpenModule(TExecBase, "io", 0, TNULL);
			dtcodec->util = TExecOpenModule(TExecBase, "util", 0, TNULL);
			dtcodec->imgp = TExecOpenModule(TExecBase, "imgproc", 0, TNULL);

			if(fp)
			{
				dtcodec->fp=fp;
				dtcodec->read=read_bmp_open(dtcodec);
				if(dtcodec->read)
					return dtcodec;
			}
			else
			{
				dtcodec->write=TTRUE;
				return dtcodec;
			}
		}
	}
	return TNULL;
}

/*
**      close hash instance
*/
static TCALLBACK TVOID mod_close(TMOD_DTCODEC *dtcodec, TAPTR selftask)
{
	if(dtcodec->attribut_taglist)
	{
		TExecFree(TExecBase, dtcodec->attribut_taglist);
		dtcodec->attribut_taglist=TNULL;
	}
	TExecCloseModule(TExecBase, dtcodec->imgp);
	TExecCloseModule(TExecBase, dtcodec->util);
	TExecCloseModule(TExecBase, dtcodec->io);
	TFreeInstance(dtcodec);
}

/**************************************************************************
 **************************************************************************

	here starts the codec specific stuff

 **************************************************************************
 **************************************************************************/

/**************************************************************************
	dtcodec_getattrs
 **************************************************************************/
TMODAPI TTAGITEM* dtcodec_getattrs(TMOD_DTCODEC *dtcodec)
{
	if(!dtcodec->attribut_taglist)
		dtcodec->attribut_taglist=(TTAGITEM*)TExecAlloc(TExecBase, TNULL,sizeof(TTAGITEM)*13);

	if(dtcodec->read)
	{

		/* set basic attributes for a readfile */
		TSetTag(dtcodec->attribut_taglist[0], TDTAG_CLASS, DTCLASS_PIC);
		TSetTag(dtcodec->attribut_taglist[1], TDTAG_FULLNAME, "BMP Picture");
		TSetTag(dtcodec->attribut_taglist[2], TDTAG_NUMSUBCLASSES, 0);

		/* fill with class-typical stuff */
		TSetTag(dtcodec->attribut_taglist[3], TDTAG_PICWIDTH, dtcodec->width);
		TSetTag(dtcodec->attribut_taglist[4], TDTAG_PICHEIGHT, dtcodec->height);
		TSetTag(dtcodec->attribut_taglist[5], TDTAG_PICDEPTH, (TINT)dtcodec->depth);
		TSetTag(dtcodec->attribut_taglist[6], TDTAG_PICBYTESPERROW, dtcodec->bytesperrow);
		TSetTag(dtcodec->attribut_taglist[7], TDTAG_PICPIXELWIDTH, 1);
		TSetTag(dtcodec->attribut_taglist[8], TDTAG_PICPIXELHEIGHT, 1);
		TSetTag(dtcodec->attribut_taglist[9], TDTAG_PICFORMAT, dtcodec->format);

		TSetTagDone(dtcodec->attribut_taglist[10]);
	}
	else
	{
		TSetTag(dtcodec->attribut_taglist[ 0], TDTAG_CLASS, DTCLASS_PIC);
		TSetTag(dtcodec->attribut_taglist[ 1], TDTAG_FULLNAME, "BMP Picture");
		TSetTag(dtcodec->attribut_taglist[ 2], TDTAG_NUMSUBCLASSES, 0);
		TSetTag(dtcodec->attribut_taglist[ 3], TDTAG_SHORTNAME, "bmp");
		TSetTag(dtcodec->attribut_taglist[ 4], TDTAG_COMPRESSION, 1);
		TSetTag(dtcodec->attribut_taglist[ 7], TDTAG_SAVEPICMAXWIDTH, 32768);
		TSetTag(dtcodec->attribut_taglist[ 8], TDTAG_SAVEPICMAXHEIGHT, 32768);
		TSetTag(dtcodec->attribut_taglist[ 9], TDTAG_SAVEPICMINDEPTH, 1);
		TSetTag(dtcodec->attribut_taglist[10], TDTAG_SAVEPICMAXDEPTH, 32);

		TSetTagDone(dtcodec->attribut_taglist[11]);
	}
	return dtcodec->attribut_taglist;
}

/**************************************************************************
	dtcodec_domethod
 **************************************************************************/
TMODAPI TBOOL dtcodec_domethod(TMOD_DTCODEC *dtcodec, TTAGITEM *taglist)
{
	TBOOL suc=TFALSE;

	if(dtcodec->read)
	{
		TUINT8 *data=(TUINT8*)TGetTag(taglist,TDOTAG_GETDATA,0);
		TIMGARGBCOLOR *palette=(TIMGARGBCOLOR*)TGetTag(taglist,TDOTAG_GETPALETTE,0);

		if(data)
			suc=read_bmp_data(dtcodec,data);

		if(dtcodec->format==IMGFMT_CLUT || dtcodec->format==IMGFMT_PLANAR)
		{
			if(palette && suc)
				suc=read_bmp_palette(dtcodec,palette);
		}
	}
	else if(dtcodec->write)
	{
		dtcodec->fp=(TAPTR)TGetTag(taglist,TDOTAG_WRITEFILE,TNULL);
		if(dtcodec->fp)
		{
			dtcodec->srcpic=(TIMGPICTURE*)TGetTag(taglist,TDOTAG_SETDATA,TNULL);
			if(dtcodec->srcpic)
			{
				dtcodec->compression=(TINT)TGetTag(taglist,TDOTAG_COMPRESSION,0);
				write_bmp(dtcodec);
			}
		}
	}
	return suc;
}
