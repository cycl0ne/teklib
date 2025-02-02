#include "datatypehandler.h"

/**************************************************************************
	TDthSimpleLoadPicture
	---------------------

	this function collects anything needed to load a picture in a single
	function call.

	this isn't really necessary for the datatypehandler, but personally i 
	think it will help a lot to have simple functions for the most needed 
	datatype classes.

    so in future eventually i will add comparable functions for other often
	used datatype classes, i.e. for text or sound
 **************************************************************************/
TMODAPI TBOOL TDthSimpleLoadPicture(TMOD_DATATYPEHANDLER *dth, TSTRPTR filename, TIMGPICTURE *pic)
{
	TTAGITEM *pictags;
	TAPTR filehandle;
	THNDL *pichandle;
	TTAGITEM picloadtags[3];

	TBOOL suc=TFALSE;

	TExecFillMem(dth->exec,pic,sizeof(TIMGPICTURE),0);

	filehandle = TIOOpenFile(dth->io, filename, TFMODE_OLDFILE, TNULL);
	if(filehandle)
	{
		TTAGITEM opentags[3];

		opentags[0].tti_Tag = TDOPENTAG_FILEHANDLE;	opentags[0].tti_Value = (TTAG)filehandle;
		opentags[1].tti_Tag = TTAG_DONE;

		pichandle=TDthOpen(dth,opentags);

		if(pichandle)
		{
			pictags=TDthGetAttrs(dth,pichandle);

			if((TINT)TGetTag(pictags,TDTAG_CLASS,0)==DTCLASS_PIC)
			{
				// load the picture
				pic->width=(TINT)TGetTag(pictags,TDTAG_PICWIDTH,0);
				pic->height=(TINT)TGetTag(pictags,TDTAG_PICHEIGHT,0);
				pic->depth=(TINT)TGetTag(pictags,TDTAG_PICDEPTH,0);
				pic->bytesperrow=(TINT)TGetTag(pictags,TDTAG_PICBYTESPERROW,0);
				pic->format=(TINT)TGetTag(pictags,TDTAG_PICFORMAT,0);

				pic->data=TExecAlloc0(dth->exec,TNULL,pic->bytesperrow*pic->height);
				if(!pic->data)
				{
					TDestroy(pichandle);
					TIOCloseFile(dth->io, filehandle);
					return TFALSE;
				}
				picloadtags[0].tti_Tag = TDOTAG_GETDATA; picloadtags[0].tti_Value = (TTAG) pic->data;

				if(pic->depth<=8)
				{
					pic->palette=(TIMGARGBCOLOR*)TExecAlloc0(dth->exec,TNULL,sizeof(TIMGARGBCOLOR)*256);
					if(!pic->palette)
					{
						TExecFree(dth->exec,pic->data);
						pic->data=TNULL;
						TDestroy(pichandle);
						TIOCloseFile(dth->io, filehandle);
						return TFALSE;
					}
					picloadtags[1].tti_Tag = TDOTAG_GETPALETTE; picloadtags[1].tti_Value = (TTAG) pic->palette;
					picloadtags[2].tti_Tag = TTAG_DONE;
				}
				else
					picloadtags[1].tti_Tag = TTAG_DONE;

				// call datatypehandlers domethod for loading the data
				if(TDthDoMethod(dth,pichandle,picloadtags))
					suc=TTRUE;
				else
				{
					TExecFree(dth->exec,pic->data);
					pic->data=TNULL;

					if(pic->palette)
					{
						TExecFree(dth->exec,pic->palette);
						pic->palette=TNULL;
					}
				}
			}
			TDestroy(pichandle);
		}
		TIOCloseFile(dth->io, filehandle);
	}
	return suc;
}
