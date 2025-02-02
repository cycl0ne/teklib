#include "codec_ilbm.h"
#include <tek/teklib.h>

TBOOL read_ilbm_open(TMOD_DTCODEC *dtcodec)
{
	TINT camg;

	dtcodec->width=dtcodec->height=0;
	dtcodec->depth=dtcodec->realdepth=0;
	dtcodec->ham6=dtcodec->ham8=dtcodec->ehb=dtcodec->lace=dtcodec->hires=dtcodec->superhires=TFALSE;
	dtcodec->pix_w=dtcodec->pix_h=1;

	/*BMHD einlesen*/
	if(!read_ilbm_seekchunk(dtcodec,"BMHD"))
		return TFALSE;

	TIOSeek(dtcodec->io,dtcodec->fp,4,TNULL,TFPOS_CURRENT);

	/* Bildinfos holen */
	TIOFRead(dtcodec->io,dtcodec->fp,(TAPTR)&dtcodec->width,2);
	TIOFRead(dtcodec->io,dtcodec->fp,(TAPTR)&dtcodec->height,2);
	if(!TUtilIsBigEndian(TUtilBase))
	{
	    TUtilBSwap16(TUtilBase, &dtcodec->width);
	    TUtilBSwap16(TUtilBase, &dtcodec->height);
	}

	TIOSeek(dtcodec->io,dtcodec->fp,4,TNULL,TFPOS_CURRENT);
	TIOFRead(dtcodec->io,dtcodec->fp,&dtcodec->realdepth,1);

	TIOSeek(dtcodec->io,dtcodec->fp,1,TNULL,TFPOS_CURRENT);
	TIOFRead(dtcodec->io,dtcodec->fp,&dtcodec->compression,1);

	/* CAMG auswerten */
	if(dtcodec->realdepth<24)
	{
		dtcodec->depth=dtcodec->realdepth;

		if(read_ilbm_seekchunk(dtcodec,"CAMG"))
		{
			TIOSeek(dtcodec->io,dtcodec->fp,4,TNULL,TFPOS_CURRENT);

			TIOFRead(dtcodec->io,dtcodec->fp,(TAPTR)&camg,4);
			if(!TUtilIsBigEndian(TUtilBase))
			{
				TUtilBSwap32(TUtilBase, &camg);
			}

			if((camg & 0x0080)==0x0080) dtcodec->ehb=TTRUE;
			if((camg & 0x0004)==0x0004) dtcodec->lace=TTRUE;
			if((camg & 0x8000)==0x8000) dtcodec->hires=TTRUE;
			if((camg & 0x8020)==0x8020) dtcodec->superhires=TTRUE;

			if((camg & 0x0800)==0x0800)
			{
				dtcodec->depth=24;

				if(dtcodec->realdepth<7)
					dtcodec->ham6=TTRUE;
				else
					dtcodec->ham8=TTRUE;
			}

			if(dtcodec->superhires)
			{
				if(!dtcodec->lace)
				{
					dtcodec->pix_w=1;
					dtcodec->pix_h=4;
				}
				else
				{
					dtcodec->pix_w=1;
					dtcodec->pix_h=2;
				}
			}
			else if(dtcodec->hires)
			{
				if(!dtcodec->lace)
				{
					dtcodec->pix_w=1;
					dtcodec->pix_h=2;
				}
			}
			else
			{
				if(dtcodec->lace)
				{
					dtcodec->pix_w=2;
					dtcodec->pix_h=1;
				}
			}
		}
	}
	else
	{
		dtcodec->depth=24;
	}

	/* im codec format und bytesperrow setzen fuer getattrs */
	if(dtcodec->depth==1)
	{
		dtcodec->bytesperrow=(dtcodec->width+7)/8;
		dtcodec->format=IMGFMT_PLANAR;
	}

	if(dtcodec->depth>1 && dtcodec->depth<=8)
	{
		dtcodec->bytesperrow=dtcodec->width;
		dtcodec->format=IMGFMT_CLUT;
	}

	if(dtcodec->depth==24)
	{
		dtcodec->bytesperrow=dtcodec->width*3;
		dtcodec->format=IMGFMT_R8G8B8;
	}

	return TTRUE;
}

TBOOL read_ilbm_palette(TMOD_DTCODEC *dtcodec, TIMGARGBCOLOR *palette)
{
	TINT numcols,i;

	if(!read_ilbm_seekchunk(dtcodec,"CMAP"))
		return TFALSE;
	else
	{
		TIOFRead(dtcodec->io,dtcodec->fp,(TAPTR)&numcols,4);
		if(!TUtilIsBigEndian(TUtilBase))
		{
		    TUtilBSwap32(TUtilBase, &numcols);
		}

		numcols=numcols/3;
		for(i=0;i<numcols;i++)
		{
			palette[i].a=0;
			palette[i].r=TIOFGetC(dtcodec->io,dtcodec->fp);
			palette[i].g=TIOFGetC(dtcodec->io,dtcodec->fp);
			palette[i].b=TIOFGetC(dtcodec->io,dtcodec->fp);
		}
		if(dtcodec->ehb)
		{
			for(i=0;i<numcols;i++)
			{
				palette[i+numcols].r=palette[i].r/2;
				palette[i+numcols].g=palette[i].g/2;
				palette[i+numcols].b=palette[i].b/2;
			}
		}
	}

	return TTRUE;
}

TBOOL read_ilbm_data(TMOD_DTCODEC *dtcodec, TUINT8 *data)
{
	TBOOL suc;
	TINT numbytes,srcbpr;
	TUINT8 *tmpbuf, *tmpbuf2=TNULL;

	if(!read_ilbm_seekchunk(dtcodec,"BODY"))
		return TFALSE;
	else
	{
		TIOFRead(dtcodec->io,dtcodec->fp,(TAPTR)&numbytes,4);
		if(!TUtilIsBigEndian(TUtilBase))
		{
		    TUtilBSwap32(TUtilBase, &numbytes);
		}

		srcbpr=((dtcodec->width+7)/8);

		if(dtcodec->depth!=1)
		{
			tmpbuf=TExecAlloc0(TExecBase, TNULL,srcbpr*dtcodec->height*dtcodec->realdepth);
			if(!tmpbuf) return TFALSE;

			if(dtcodec->ham6 || dtcodec->ham8)
			{
				tmpbuf2=TExecAlloc0(TExecBase, TNULL,dtcodec->width*dtcodec->height);
				if(!tmpbuf2)
				{
					TExecFree(TExecBase, tmpbuf);
					return TFALSE;
				}
			}
			else
			{
				tmpbuf2=data;
			}
		}
		else
		{
			tmpbuf=data;
		}

		if(dtcodec->compression)
			suc=read_ilbm_data_compressed(dtcodec,tmpbuf,numbytes);
		else
			suc=read_ilbm_data_uncompressed(dtcodec,tmpbuf,numbytes);

		if(!suc) return TFALSE;

		if(dtcodec->depth!=1)
		{
			TIMGPICTURE loadedpic, convertedpic;

			loadedpic.data=tmpbuf;
			loadedpic.palette=TNULL;
			loadedpic.format=IMGFMT_PLANAR;
			loadedpic.width=dtcodec->width;
			loadedpic.height=dtcodec->height;
			loadedpic.depth=dtcodec->realdepth;
			loadedpic.bytesperrow=srcbpr;

			convertedpic.data=tmpbuf2;
			convertedpic.palette=TNULL;
			if(dtcodec->realdepth<=8)
				convertedpic.format=IMGFMT_CLUT;
			else if(dtcodec->realdepth==24)
				convertedpic.format=IMGFMT_R8G8B8;
			convertedpic.width=dtcodec->width;
			convertedpic.height=dtcodec->height;
			convertedpic.depth=dtcodec->realdepth;
			convertedpic.bytesperrow=dtcodec->bytesperrow;

			TImgDoMethod(dtcodec->imgp,&loadedpic,&convertedpic,IMGMT_CONVERT,TNULL);

			if(dtcodec->ham6 || dtcodec->ham8)
			{
				TIMGARGBCOLOR *picpalette;

				picpalette=(TIMGARGBCOLOR*)TExecAlloc0(TExecBase, TNULL,sizeof(TIMGARGBCOLOR)*256);
				if(!picpalette)
				{
					TExecFree(TExecBase, tmpbuf2);
					TExecFree(TExecBase, tmpbuf);
					return TFALSE;
				}
				if(!read_ilbm_palette(dtcodec,picpalette))
				{
					TExecFree(TExecBase, picpalette);
					TExecFree(TExecBase, tmpbuf2);
					TExecFree(TExecBase, tmpbuf);
					return TFALSE;
				}

				if(dtcodec->ham6)
					read_ilbm_ham62rgb(dtcodec,tmpbuf2,data,picpalette);
				else
					read_ilbm_ham82rgb(dtcodec,tmpbuf2,data,picpalette);

				TExecFree(TExecBase, picpalette);
				TExecFree(TExecBase, tmpbuf2);
			}
		}
		TExecFree(TExecBase, tmpbuf);
		return TTRUE;
	}
	return TFALSE;
}

TBOOL read_ilbm_data_uncompressed(TMOD_DTCODEC *dtcodec, TUINT8 *data, TINT numbytes)
{
	TINT srcbpr, readbpr;
	TINT y,d;
	TUINT8 *pptr;

	srcbpr=(dtcodec->width+7)/8;
	readbpr=(srcbpr+1) & 0xfffffffe;

	for(y=0;y<dtcodec->height;y++)
	{
		for(d=0;d<dtcodec->realdepth;d++)
		{
			pptr=data + srcbpr*dtcodec->height*d + y*srcbpr;
			TIOFRead(dtcodec->io,dtcodec->fp,pptr,readbpr);
			pptr+=readbpr;
		}
	}

	return TTRUE;
}

TBOOL read_ilbm_data_compressed(TMOD_DTCODEC *dtcodec, TUINT8 *data, TINT numbytes)
{
	TINT srcbpr, readbpr, roundbyte;
	TINT y,d,i;
	TUINT8 x,n,c,o;
	TUINT8 *pptr;

	srcbpr=(dtcodec->width+7)/8;
	readbpr=(srcbpr+1) & 0xfffffffe;
	roundbyte=readbpr-srcbpr;

	for(y=0;y<dtcodec->height;y++)
	{
		for(d=0;d<dtcodec->realdepth;d++)
		{
			pptr=data + srcbpr*dtcodec->height*d + y*srcbpr;
			i=readbpr;
			while(i>0)
			{
				x=TIOFGetC(dtcodec->io,dtcodec->fp);
				if(x>0x80)
				{
					c=TIOFGetC(dtcodec->io,dtcodec->fp);
					o=0x101-x;
					i-=o;
					if(!i && roundbyte)
						o--;

					for(n=0;n<o;n++)
					{
						*pptr++=c;
					}
				}
				else if(x<0x80)
				{
					i-=(x+1);
					if(!i && roundbyte)
						x--;

					TIOFRead(dtcodec->io,dtcodec->fp,pptr,x+1);
					pptr+=x+1;

					if(!i && roundbyte)
						TIOSeek(dtcodec->io,dtcodec->fp,1,TNULL,TFPOS_CURRENT);
				}
				else
				{
					i--;
				}
			}
		}
	}

	return TTRUE;
}

TBOOL read_ilbm_seekchunk(TMOD_DTCODEC *dtcodec, TSTRPTR chunkname)
{
	TINT repeats;
	TBOOL success,body;
	TINT8 dummy[5];

	dummy[4]=0;
	repeats=0;
	success=TFALSE;
	while(repeats<2 && !success)
	{
		body=TFALSE;

		if(body || TIOFEoF(dtcodec->io,dtcodec->fp))
		{
			TIOSeek(dtcodec->io,dtcodec->fp,12,TNULL,TFPOS_BEGIN);
		}

		TIOFRead(dtcodec->io,dtcodec->fp,dummy,4);
		do
		{
			if(!TUtilStrCmp(TUtilBase, chunkname,dummy))
				success=TTRUE;
			else if(!TUtilStrCmp(TUtilBase, "BODY",dummy))
				body=TTRUE;
			else
			{
				dummy[0]=dummy[1];
				dummy[1]=dummy[2];
				dummy[2]=dummy[3];
				dummy[3]=TIOFGetC(dtcodec->io,dtcodec->fp);
			}
		}while(!success && !body && !TIOFEoF(dtcodec->io,dtcodec->fp));

		repeats++;
	}

	return success;
}

TVOID read_ilbm_ham62rgb(TMOD_DTCODEC *dtcodec, TUINT8 *srcdata, TUINT8 *dstdata,  TIMGARGBCOLOR *picpalette)
{
	TINT x,y;
	TUINT8 colv,r,g,b;

	for(y=0;y<dtcodec->height;y++)
	{
		r=g=b=0;
		for(x=0;x<dtcodec->width;x++)
		{
			colv=srcdata[y*dtcodec->width+x];

			switch(colv>>4)
			{
				case 0:
					r=picpalette[colv].r;
					g=picpalette[colv].g;
					b=picpalette[colv].b;
				break;

				case 1:
					b=(colv-16)<<4;
				break;

				case 2:
					r=(colv-32)<<4;
				break;

				case 3:
					g=(colv-48)<<4;
				break;
			}
			dstdata[y*dtcodec->width*3+x*3]=r;
			dstdata[y*dtcodec->width*3+x*3+1]=g;
			dstdata[y*dtcodec->width*3+x*3+2]=b;
		}
	}
}

TVOID read_ilbm_ham82rgb(TMOD_DTCODEC *dtcodec, TUINT8 *srcdata, TUINT8 *dstdata, TIMGARGBCOLOR *picpalette)
{
	TINT x,y;
	TUINT8 colv,r,g,b;

	for(y=0;y<dtcodec->height;y++)
	{
		r=g=b=0;
		for(x=0;x<dtcodec->width;x++)
		{
			colv=srcdata[y*dtcodec->width+x];

			switch(colv>>6)
			{
				case 0:
					r=picpalette[colv].r;
					g=picpalette[colv].g;
					b=picpalette[colv].b;
				break;

				case 1:
					b=(colv-64)<<2;
				break;

				case 2:
					r=(colv-128)<<2;
				break;

				case 3:
					g=(colv-192)<<2;
				break;
			}
			dstdata[y*dtcodec->width*3+x*3]=r;
			dstdata[y*dtcodec->width*3+x*3+1]=g;
			dstdata[y*dtcodec->width*3+x*3+2]=b;
		}
	}
}








