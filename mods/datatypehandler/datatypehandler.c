
/*
**      $Id: datatypehandler.c,v 1.12 2005/09/13 02:41:44 tmueller Exp $
**		See copyright notice in teklib/COPYRIGHT
*/

#define MOD_VERSION     0
#define MOD_REVISION    5

#include "datatypehandler.h"

/*
**      module prototypes
*/

static TCALLBACK TMOD_DATATYPEHANDLER *mod_open(TMOD_DATATYPEHANDLER *dth, TAPTR selftask, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_DATATYPEHANDLER *dth, TAPTR selftask);

TMODENTRY TUINT tek_init_datatypehandler(TAPTR selftask, TMOD_DATATYPEHANDLER *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)                                  /* first call */
		{
			if (version <= MOD_VERSION)                     /* version check */
			{
				return sizeof(TMOD_DATATYPEHANDLER);    /* return module positive size */
			}
		}
		else                                                                    /* second call */
		{
			return sizeof(TAPTR) * 6;                       /* return module negative size */
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
		((TAPTR *) mod)[-1 ] = (TAPTR) TDthOpen;
		((TAPTR *) mod)[-2 ] = (TAPTR) TDthGetAttrs;
		((TAPTR *) mod)[-3 ] = (TAPTR) TDthDoMethod;
		((TAPTR *) mod)[-4 ] = (TAPTR) TDthListDatatypes;
		((TAPTR *) mod)[-5 ] = (TAPTR) TDthSimpleLoadPicture;

		return TTRUE;
	}
	return 0;
}

/*
**      open instance
*/
static TCALLBACK TMOD_DATATYPEHANDLER *mod_open(TMOD_DATATYPEHANDLER *dth, TAPTR selftask, TTAGITEM *tags)
{
	dth = TNewInstance(dth, dth->module.tmd_PosSize, dth->module.tmd_NegSize);
	if (!dth) return TNULL;

	dth->exec=TGetExecBase(dth);
	dth->io = TExecOpenModule(dth->exec, "io", 0, TNULL);
	dth->util = TExecOpenModule(dth->exec, "util", 0, TNULL);

	dth->identifiers=TFALSE;
	TInitList(&dth->dtmodlist);
	TInitList(&dth->dtfilterlist);

	return dth;
}

/*
**      close hash instance
*/
static TCALLBACK TVOID mod_close(TMOD_DATATYPEHANDLER *dth, TAPTR selftask)
{
	TDestroyList(&dth->dtfilterlist);
	TDestroyList(&dth->dtmodlist);
	DTH_FreeDatatypeIdentifiers(dth);
	TExecCloseModule(dth->exec, dth->util);
	TExecCloseModule(dth->exec, dth->io);
	TFreeInstance(dth);
}

/**************************************************************************
	TDthOpen
 **************************************************************************/
TMODAPI THNDL* TDthOpen(TMOD_DATATYPEHANDLER *dth, TTAGITEM *tags)
{
	datatypemodule *d;
	THNDL *handle;
	TINT i,filenamelen;
	TSTRPTR filename;
	TSTRPTR dtcodecname;
	TAPTR fp;

	fp=(TAPTR)TGetTag(tags,TDOPENTAG_FILEHANDLE,TNULL);
	dtcodecname=(TSTRPTR)TGetTag(tags,TDOPENTAG_DTNAME,TNULL);

	if(fp)
	{
		/* if we have a datatypecodec name, skip the identify procedure */
		if(!dtcodecname)
		{
			filenamelen=TIONameOf(dth->io,fp,TNULL,0);
			if(filenamelen==-1)
				return TNULL;
			else
			{
				filename=TExecAlloc0(dth->exec, TNULL,filenamelen+1);
				TIONameOf(dth->io,fp,filename,filenamelen+1);
			}

			if(!dth->identifiers)
			{
				if(!DTH_ReadDatatypeIdentifiers(dth))
					return TNULL;
			}

			i=0;
			do
			{
				if(!(DTH_IdentifyFile(dth,fp,filename,i)))
					i++;
				else
					dtcodecname=dth->IdentData[i].datatypecodecname;
			}while(i<dth->numidentifiers && !dtcodecname);

			TExecFree(dth->exec,filename);

			TIOSeek(dth->io,fp,0,TNULL,TFPOS_BEGIN);
		}

		if(dtcodecname)
		{
			d=TExecAlloc0(dth->exec, TNULL,sizeof(datatypemodule));
			if(d)
			{
				TTAGITEM dtctags[3];

				dtctags[0].tti_Tag = TDCTAG_FILEHANDLE;
				dtctags[0].tti_Value = (TTAG)fp;
				dtctags[1].tti_Tag = TDCTAG_WRITE;
				dtctags[1].tti_Value = (TTAG)TFALSE;
				/* pass user tags through to codec: */
				dtctags[2].tti_Tag = TTAG_MORE;
				dtctags[2].tti_Value = (TTAG) tags;

				d->modul=TExecOpenModule(dth->exec, dtcodecname, 0, dtctags);
				if(d->modul)
				{
					d->dth=dth;
					handle=TExecAlloc0(dth->exec, TNULL,sizeof(THNDL));
					if(handle)
					{
						handle->thn_Data=d;
						handle->thn_DestroyFunc=(TDFUNC)DTH_DestroyCodecHandle;
						TAddTail(&dth->dtmodlist, (TNODE *) handle);
						return handle;
					}
					TExecCloseModule(dth->exec, d->modul);
				}
			}
			TExecFree(dth->exec, d);
		}
	}
	else if(dtcodecname)
	{
		d=TExecAlloc0(dth->exec, TNULL,sizeof(datatypemodule));
		if(d)
		{
			TTAGITEM dtctags[2];

			dtctags[0].tti_Tag = TDCTAG_WRITE;              dtctags[0].tti_Value = (TTAG)TTRUE;
			dtctags[1].tti_Tag = TTAG_DONE;

			d->modul=TExecOpenModule(dth->exec, dtcodecname, 0, dtctags);
			if(d->modul)
			{
				d->dth=dth;
				handle=TExecAlloc0(dth->exec, TNULL,sizeof(THNDL));
				if(handle)
				{
					handle->thn_Data=d;
					handle->thn_DestroyFunc=(TDFUNC)DTH_DestroyCodecHandle;
					TAddTail(&dth->dtmodlist, (TNODE *) handle);
					return handle;
				}
				TExecCloseModule(dth->exec, d->modul);
			}
		}
		TExecFree(dth->exec, d);
	}
	return TNULL;
}

/**************************************************************************
	TDthGetAttrs
 **************************************************************************/
TMODAPI TTAGITEM* TDthGetAttrs(TMOD_DATATYPEHANDLER *dth, THNDL *handle)
{
	datatypemodule *d=(datatypemodule *)handle->thn_Data;
	return dtcodec_getattrs(d->modul);
}

/**************************************************************************
	TDthDoMethod
 **************************************************************************/
TMODAPI TINT TDthDoMethod(TMOD_DATATYPEHANDLER *dth, THNDL *handle, TTAGITEM *taglist)
{
	datatypemodule *d=(datatypemodule *)handle->thn_Data;
	return dtcodec_domethod(d->modul,taglist);
}

/**************************************************************************
	TDthListDatatypes
 **************************************************************************/
TMODAPI TLIST* TDthListDatatypes(TMOD_DATATYPEHANDLER *dth, TTAGITEM *filtertags)
{
	DTListItem *li;
	TINT dtclass;
	TSTRPTR dtshortname;
	TBOOL dtsave;
	TINT i;
	TBOOL suc=TFALSE;

	if(!dth->identifiers)
	{
		if(!DTH_ReadDatatypeIdentifiers(dth))
			return TNULL;
	}

	TDestroyList(&dth->dtfilterlist);
	TInitList(&dth->dtfilterlist);

	dtclass=(TINT)TGetTag(filtertags,TDFILTER_CLASS,(TTAG)-1);
	dtshortname=(TSTRPTR)TGetTag(filtertags,TDFILTER_SHORTNAME,TNULL);
	dtsave=(TBOOL)TGetTag(filtertags,TDFILTER_CANSAVE,(TTAG)TFALSE);

	for(i=0;i<dth->numidentifiers;i++)
	{
		if(dtclass!=-1)
		{
			if(dth->IdentData[i].datatypeclass == dtclass)
				suc=TTRUE;
			else
				suc=TFALSE;
		}

		if(suc && dtshortname!=TNULL)
		{
			if(TUtilStrCaseCmp(dth->util,dth->IdentData[i].datatypeshortname,dtshortname)==0)
				suc=TTRUE;
			else
				suc=TFALSE;
		}

		if(suc && dtsave)
		{
			if(dth->IdentData[i].canwrite)
				suc=TTRUE;
			else
				suc=TFALSE;
		}

		if(suc)
		{
			TINT size;

			li=TExecAlloc0(dth->exec, TNULL,sizeof(DTListItem));
			li->li_ddat=TExecAlloc0(dth->exec, TNULL,sizeof(DTDescData));

			size=TUtilStrLen(TUtilBase, dth->IdentData[i].datatypecodecname)+1;
			li->li_ddat->datatypecodecname=(TINT8*)TExecAlloc(dth->exec, TNULL,size);
			TExecCopyMem(dth->exec, dth->IdentData[i].datatypecodecname,li->li_ddat->datatypecodecname,size);

			size=TUtilStrLen(TUtilBase, dth->IdentData[i].datatypefullname)+1;
			li->li_ddat->datatypefullname=(TINT8*)TExecAlloc(dth->exec, TNULL,size);
			TExecCopyMem(dth->exec, dth->IdentData[i].datatypefullname,li->li_ddat->datatypefullname,size);

			size=TUtilStrLen(TUtilBase, dth->IdentData[i].datatypeshortname)+1;
			li->li_ddat->datatypeshortname=(TINT8*)TExecAlloc(dth->exec, TNULL,size);
			TExecCopyMem(dth->exec, dth->IdentData[i].datatypeshortname,li->li_ddat->datatypeshortname,size);

			size=TUtilStrLen(TUtilBase, dth->IdentData[i].datatypesuffix)+1;
			li->li_ddat->datatypesuffix=(TINT8*)TExecAlloc(dth->exec, TNULL,size);
			TExecCopyMem(dth->exec, dth->IdentData[i].datatypesuffix,li->li_ddat->datatypesuffix,size);

			li->li_ddat->datatypeclass=dth->IdentData[i].datatypeclass;
			li->li_ddat->canwrite=dth->IdentData[i].canwrite;
			li->li_ddat->dtnum=i;

			li->li_DestroyFunc=(TDFUNC)DTH_DestroyListItem;
			li->li_ExecBase=dth->exec;
			TAddTail(&dth->dtfilterlist, (TNODE *)li);
		}
	}
	return &dth->dtfilterlist;
}



/**************************************************************************
 **************************************************************************

  PRIVATE Stuff
  
 **************************************************************************
 **************************************************************************/

/**************************************************************************
	read in datatype identifiers
 **************************************************************************/
TBOOL DTH_ReadDatatypeIdentifiers(TMOD_DATATYPEHANDLER *dth)
{
	TAPTR dtident_base;
	TLIST modlist;
	TNODE *nextnode, *node;
	struct TModuleEntry *entry;
	DTIdentifyData idat;
	TINT i,size;

	TInitList(&modlist);
	if (TUtilGetModules(TUtilBase, "datatype_ident", &modlist, TNULL))
	{
		node = modlist.tlh_Head;
		while ((nextnode = node->tln_Succ))
		{
			dth->numidentifiers++;
			node = nextnode;
		}

		dth->IdentData=(DTIdentifyData*)TExecAlloc(dth->exec, TNULL,sizeof(DTIdentifyData)*dth->numidentifiers);

		i=0;
		node = modlist.tlh_Head;
		while ((nextnode = node->tln_Succ))
		{
			entry = (struct TModuleEntry *) node;

			dtident_base = TExecOpenModule(dth->exec, entry->tme_Handle.tmo_Name, 0, TNULL);

			if(dtident_base)
			{
				datatype_ident_getidentdata(dtident_base,&idat);

				size=TUtilStrLen(TUtilBase, idat.datatypecodecname)+1;
				dth->IdentData[i].datatypecodecname=(TINT8*)TExecAlloc(dth->exec, TNULL,size);
				TExecCopyMem(dth->exec, idat.datatypecodecname,dth->IdentData[i].datatypecodecname,size);

				size=0;
				while(idat.datatypeidentdata[size]!=DTIC_DONE)
				{
					size++;
				}
				size++;
				dth->IdentData[i].datatypeidentdata=(TINT8*)TExecAlloc(dth->exec, TNULL,size);
				TExecCopyMem(dth->exec, idat.datatypeidentdata,dth->IdentData[i].datatypeidentdata,size);

				size=TUtilStrLen(TUtilBase, idat.datatypesuffix)+1;
				dth->IdentData[i].datatypesuffix=(TINT8*)TExecAlloc(dth->exec, TNULL,size);
				TExecCopyMem(dth->exec, idat.datatypesuffix,dth->IdentData[i].datatypesuffix,size);

				dth->IdentData[i].datatypeclass=idat.datatypeclass;

				size=TUtilStrLen(TUtilBase, idat.datatypefullname)+1;
				dth->IdentData[i].datatypefullname=(TINT8*)TExecAlloc(dth->exec, TNULL,size);
				TExecCopyMem(dth->exec, idat.datatypefullname,dth->IdentData[i].datatypefullname,size);

				size=TUtilStrLen(TUtilBase, idat.datatypeshortname)+1;
				dth->IdentData[i].datatypeshortname=(TINT8*)TExecAlloc(dth->exec, TNULL,size);
				TExecCopyMem(dth->exec, idat.datatypeshortname,dth->IdentData[i].datatypeshortname,size);

				dth->IdentData[i].canwrite=idat.canwrite;

				TExecCloseModule(dth->exec, dtident_base);

				i++;
			}
			else
			{
				tdbprintf1(20,"open failed!!! %s\n", entry->tme_Handle.tmo_Name);
				/* TODO: MUST break out gently here, otherwise CRASH */
			}

			TRemove(node);
			TDestroy(node);
			node = nextnode;
		}
		dth->identifiers=TTRUE;
		return TTRUE;
	}

	dth->identifiers=TFALSE;
	return TFALSE;
}

/**************************************************************************
	clear datatype identifiers
 **************************************************************************/
TVOID DTH_FreeDatatypeIdentifiers(TMOD_DATATYPEHANDLER *dth)
{
	TINT i;

	for(i=0;i<dth->numidentifiers;i++)
	{
		TExecFree(dth->exec, dth->IdentData[i].datatypecodecname);
		TExecFree(dth->exec, dth->IdentData[i].datatypeidentdata);
		TExecFree(dth->exec, dth->IdentData[i].datatypesuffix);
		TExecFree(dth->exec, dth->IdentData[i].datatypefullname);
		TExecFree(dth->exec, dth->IdentData[i].datatypeshortname);
	}
	TExecFree(dth->exec, dth->IdentData);
}

/**************************************************************************
	identify file
 **************************************************************************/
TBOOL DTH_IdentifyFile(TMOD_DATATYPEHANDLER *dth,TAPTR fp, TINT8 *name, TINT dtnum)
{
	TBOOL usesuffix;
	TBOOL suc,cmpsuc;
	TINT i,j,pos;
	TINT8 idval,cmpval,numcmpvals,numcmpvals2,numcmpseqs;
	TINT8 *scanvals,*identsuffix;
	TINT8 *src_suffix;

	TINT8 *idat=dth->IdentData[dtnum].datatypeidentdata;

	TIOSeek(dth->io,fp,0,TNULL,TFPOS_BEGIN);

	/* compare file with identifydata */
	usesuffix=TFALSE;
	suc=TTRUE;
	i=0;
	do
	{
		switch(idat[i])
		{
			case DTIC_USESUFFIX:
				i++;
				usesuffix=TTRUE;
			break;

			case DTIC_TOP:
				i++;
				TIOSeek(dth->io,fp,0,TNULL,TFPOS_BEGIN);
			break;

			case DTIC_END:
				i++;
				TIOSeek(dth->io,fp,0,TNULL,TFPOS_END);
			break;

			case DTIC_MOVE:
				i++;
				TIOSeek(dth->io,fp,idat[i++],TNULL,TFPOS_CURRENT);
			break;

			case DTIC_BYTE:
				i++;
				idval=idat[i++];
				TIOFRead(dth->io,fp,&cmpval,1);
				if(!TIOFEoF(dth->io,fp))
				{
					if(cmpval!=idval) suc=TFALSE;
				}
			break;

			case DTIC_ORBYTE:
				i++;
				numcmpvals=idat[i++];
				TIOFRead(dth->io,fp,&cmpval,1);
				if(!TIOFEoF(dth->io,fp))
				{
					j=i;
					i+=numcmpvals;
					cmpsuc=TFALSE;
					do
					{
						if(cmpval==idat[j])
							cmpsuc=TTRUE;
						else
							j++;
					}while(j<i && !cmpsuc);

					if(!cmpsuc) suc=TFALSE;
				}
			break;

			case DTIC_BYTES:
				i++;
				numcmpvals=idat[i++];
				do
				{
					TIOFRead(dth->io,fp,&cmpval,1);
					if(cmpval!=idat[i++])
						suc=TFALSE;
					else
						numcmpvals--;
				}while(suc && numcmpvals && !TIOFEoF(dth->io,fp));
			break;

			case DTIC_ORBYTES:
				i++;
				numcmpseqs=idat[i++];
				numcmpvals=idat[i++];
				pos=TIOSeek(dth->io,fp,0,TNULL,TFPOS_CURRENT);
				suc=TFALSE;
				do
				{
					j=i;
					numcmpvals2=numcmpvals;
					cmpsuc=TTRUE;
					do
					{
						TIOFRead(dth->io,fp,&cmpval,1);
						if(cmpval!=idat[j++])
							cmpsuc=TFALSE;
						else
							numcmpvals2--;
					}while(numcmpvals2 && cmpsuc && !TIOFEoF(dth->io,fp));

					if(!TIOFEoF(dth->io,fp))
					{
						if(!cmpsuc)
						{
							TIOSeek(dth->io,fp,pos,TNULL,TFPOS_BEGIN);
							numcmpseqs--;
						}
						else
						{
							i+=numcmpvals;
							suc=TTRUE;
						}
					}
				}while(numcmpseqs && !suc && !TIOFEoF(dth->io,fp));
			break;

			case DTIC_SCANBYTES:
				i++;
				numcmpvals=idat[i++];
				scanvals=(TINT8*)TExecAlloc(dth->exec, TNULL,numcmpvals);
				do
				{
					TIOFRead(dth->io,fp,scanvals,numcmpvals);
					suc=TTRUE;
					j=0;
					while(j<numcmpvals && suc==TTRUE)
					{
						if(idat[i+j]!=scanvals[j])
							suc=TFALSE;
						else
							j++;
					}
					if(!suc)
						TIOSeek(dth->io,fp,1-numcmpvals,TNULL,TFPOS_CURRENT);
				}while(!suc && !TIOFEoF(dth->io,fp));
				TExecFree(dth->exec, scanvals);
				i+=numcmpvals;
			break;
		}

		if (TIOFEoF(dth->io, fp)) suc = TFALSE;

	}while(suc && idat[i]!=DTIC_DONE);

	/* check suffix */
	if(suc && usesuffix)
	{
		suc=TFALSE;
		identsuffix=dth->IdentData[dtnum].datatypesuffix;
		if(TUtilStrLen(TUtilBase, identsuffix))
		{
			src_suffix=TUtilStrRChr(TUtilBase, name,'.');
			if(src_suffix && TUtilStrLen(TUtilBase, src_suffix)<TUtilStrLen(TUtilBase, name))
			{
				src_suffix++;
				do
				{
					if(TUtilStrNCaseCmp(TUtilBase, src_suffix,identsuffix,TUtilStrLen(TUtilBase, src_suffix)) == 0)
						suc=TTRUE;
					else
					{
						identsuffix=TUtilStrChr(TUtilBase, identsuffix,',');
						if(identsuffix)
							identsuffix++;
					}
				}while(!suc && identsuffix);
			}
		}
	}
	return suc;
}

/**************************************************************************
	destroyfunc for codec handle
 **************************************************************************/
TCALLBACK TVOID DTH_DestroyCodecHandle(THNDL *handle)
{
	if(handle)
	{
		datatypemodule *d=(datatypemodule *)handle->thn_Data;
		TMOD_DATATYPEHANDLER *dth=d->dth;;

		TExecCloseModule(dth->exec, d->modul);
		TRemove((TNODE*)handle);

		TExecFree(dth->exec,d);
		TExecFree(dth->exec,handle);
	}
}

/**************************************************************************
	destroyfunc for filter handle
 **************************************************************************/
TCALLBACK TVOID DTH_DestroyListItem(THNDL *handle)
{
	if(handle)
	{
		DTListItem *li=(DTListItem *)handle;
		
		TExecFree(li->li_ExecBase,li->li_ddat->datatypecodecname);
		TExecFree(li->li_ExecBase,li->li_ddat->datatypefullname);
		TExecFree(li->li_ExecBase,li->li_ddat->datatypeshortname);
		TExecFree(li->li_ExecBase,li->li_ddat->datatypesuffix);

		TExecFree(li->li_ExecBase,li->li_ddat);

		TRemove((TNODE*)handle);
		TExecFree(li->li_ExecBase,handle);
	}
}
