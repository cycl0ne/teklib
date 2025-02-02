#include "codec_pcx.h"

/**************************************************************************
	read in pcx header
 **************************************************************************/
TBOOL read_pcx_open(TMOD_DTCODEC *dtcodec)
{
	TUINT8 loaddata[0x80];

	dtcodec->width=dtcodec->height=0;
	dtcodec->startx=dtcodec->starty=dtcodec->endx=dtcodec->endy=dtcodec->bytesperline=dtcodec->palinfo=0;
	dtcodec->version=dtcodec->depth=dtcodec->farbebenen=dtcodec->bitsperpixel=0;
	dtcodec->grey=dtcodec->shortpalette=dtcodec->longpalette=TFALSE;
	dtcodec->compression=0;

	/*Header einlesen*/
	TIOSeek(dtcodec->io,dtcodec->fp,0,TNULL,TFPOS_BEGIN);
	if(TIOFRead(dtcodec->io,dtcodec->fp,loaddata,0x80)!=0x80)
		return TFALSE;

	/*Bildinfos holen*/
	TExecCopyMem(TExecBase, loaddata+0x01,&dtcodec->version,1);
	TExecCopyMem(TExecBase, loaddata+0x02,&dtcodec->compression,1);
	TExecCopyMem(TExecBase, loaddata+0x03,&dtcodec->bitsperpixel,1);
	TExecCopyMem(TExecBase, loaddata+0x41,&dtcodec->farbebenen,1);

	TExecCopyMem(TExecBase, loaddata+0x04,&dtcodec->startx,2);
	TExecCopyMem(TExecBase, loaddata+0x06,&dtcodec->starty,2);
	TExecCopyMem(TExecBase, loaddata+0x08,&dtcodec->endx,2);
	TExecCopyMem(TExecBase, loaddata+0x0a,&dtcodec->endy,2);
	TExecCopyMem(TExecBase, loaddata+0x42,&dtcodec->bytesperline,2);
	TExecCopyMem(TExecBase, loaddata+0x44,&dtcodec->palinfo,2);
	if(TUtilIsBigEndian(TUtilBase))
	{
	    TUtilBSwap16(TUtilBase, &dtcodec->startx);
	    TUtilBSwap16(TUtilBase, &dtcodec->starty);
	    TUtilBSwap16(TUtilBase, &dtcodec->endx);
	    TUtilBSwap16(TUtilBase, &dtcodec->endy);
	    TUtilBSwap16(TUtilBase, &dtcodec->bytesperline);
	    TUtilBSwap16(TUtilBase, &dtcodec->palinfo);
	}

	/*Daten umsetzen*/
	dtcodec->width=(dtcodec->endx-dtcodec->startx)+1;
	dtcodec->height=(dtcodec->endy-dtcodec->starty)+1;
	if(dtcodec->bitsperpixel==1 && dtcodec->farbebenen==1)
	{
		dtcodec->depth=1;
		dtcodec->bytesperrow=(dtcodec->width+7)/8;
		dtcodec->format=IMGFMT_PLANAR;
	}
	else if(dtcodec->bitsperpixel==1 && dtcodec->farbebenen==4)
	{
		dtcodec->depth=4;
		dtcodec->bytesperrow=dtcodec->width;
		dtcodec->format=IMGFMT_CLUT;
	}
	else if(dtcodec->bitsperpixel==8 && dtcodec->farbebenen==1)
	{
		dtcodec->depth=8;
		dtcodec->bytesperrow=dtcodec->width;
		dtcodec->format=IMGFMT_CLUT;
	}
	else if(dtcodec->bitsperpixel==8 && dtcodec->farbebenen==3)
	{
		dtcodec->depth=24;
		dtcodec->bytesperrow=dtcodec->width*3;
		dtcodec->format=IMGFMT_B8G8R8;
	}
	else
		return TFALSE;

	/*Laenge ermitteln*/
	TIOSeek(dtcodec->io,dtcodec->fp,0,TNULL,TFPOS_END);
	dtcodec->length=TIOSeek(dtcodec->io,dtcodec->fp,0,TNULL,TFPOS_CURRENT)-0x80;

	/*Farbtyp ermitteln und Laenge gegebenenfalls korrigieren*/
	if(dtcodec->palinfo==2)
		dtcodec->grey=TTRUE;
	else
	{
		if(dtcodec->version==2 || dtcodec->version==5)
		{
			if(dtcodec->depth==8 && dtcodec->palinfo==1)
			{
				if(TIOSeek(dtcodec->io,dtcodec->fp,-769,TNULL,TFPOS_END))
				{
					if(TIOFRead(dtcodec->io,dtcodec->fp,loaddata,1)!=1)
						return TFALSE;
					if(loaddata[0]==0x0c)
					{
						dtcodec->length-=769;
						dtcodec->longpalette=TTRUE;
					}
					else
						dtcodec->grey=TTRUE;
				}
				else
					dtcodec->grey=TTRUE;
			}
		}
		if(dtcodec->depth<8 && dtcodec->palinfo==1)
			dtcodec->shortpalette=TTRUE;
	}
	return TTRUE;
}

/**************************************************************************
	read pcx palette
 **************************************************************************/
TBOOL read_pcx_palette(TMOD_DTCODEC *dtcodec, TIMGARGBCOLOR *palette)
{
	TINT i;

	if(dtcodec->longpalette)
	{
		TIOSeek(dtcodec->io,dtcodec->fp,-768,TNULL,TFPOS_END);
		for(i=0;i<256;i++)
		{
			palette[i].a=0;
			TIOFRead(dtcodec->io,dtcodec->fp,&palette[i].r,1);
			TIOFRead(dtcodec->io,dtcodec->fp,&palette[i].g,1);
			TIOFRead(dtcodec->io,dtcodec->fp,&palette[i].b,1);
		}
	}
	else if(dtcodec->shortpalette)
	{
		TIOSeek(dtcodec->io,dtcodec->fp,0x10,TNULL,TFPOS_BEGIN);
		if(dtcodec->depth==4)
		{
			for(i=0;i<16;i++)
			{
				palette[i].a=0;
				TIOFRead(dtcodec->io,dtcodec->fp,&palette[i].r,1);
				TIOFRead(dtcodec->io,dtcodec->fp,&palette[i].g,1);
				TIOFRead(dtcodec->io,dtcodec->fp,&palette[i].b,1);
			}
		}
		else if(dtcodec->depth==1)
		{
			for(i=0;i<2;i++)
			{
				palette[i].a=0;
				TIOFRead(dtcodec->io,dtcodec->fp,&palette[i].r,1);
				TIOFRead(dtcodec->io,dtcodec->fp,&palette[i].g,1);
				TIOFRead(dtcodec->io,dtcodec->fp,&palette[i].b,1);
			}
		}
	}
	else if(dtcodec->grey)
	{
		if(dtcodec->depth==1)
		{
			palette[0].a=0;
			palette[0].r=0;
			palette[0].g=0;
			palette[0].b=0;
			palette[1].a=0;
			palette[1].r=255;
			palette[1].g=255;
			palette[1].b=255;
		}
		else if(dtcodec->depth==4)
		{
			for(i=0;i<16;i++)
			{
				palette[i].a=0;
				palette[i].r=i<<4;
				palette[i].g=i<<4;
				palette[i].g=i<<4;
			}
		}
		else if(dtcodec->depth==8)
		{
			for(i=0;i<256;i++)
			{
				palette[i].a=0;
				palette[i].r=i;
				palette[i].g=i;
				palette[i].g=i;
			}
		}
	}
	return TTRUE;
}

/**************************************************************************
	read pcx data
 **************************************************************************/
TBOOL read_pcx_data(TMOD_DTCODEC *dtcodec, TUINT8 *data)
{
	TINT lsize;
	TUINT8 *tmpbuf;

	if(dtcodec->depth==4)
	{
		lsize = dtcodec->bytesperline * dtcodec->farbebenen * dtcodec->width * 2;
		tmpbuf=(TUINT8*)TExecAlloc0(TExecBase, TNULL,lsize);
		if(dtcodec->compression)
		{
			if(read_pcx_data_4bit(dtcodec,tmpbuf))
			{
				read_pcx_decode4bit(dtcodec,tmpbuf,data);
				TExecFree(TExecBase, tmpbuf);
				return TTRUE;
			}
			else
			{
				TExecFree(TExecBase, tmpbuf);
				return TFALSE;
			}
		}
		else
		{
			if(read_pcx_data_4bit_unpacked(dtcodec,tmpbuf))
			{
				read_pcx_decode4bit(dtcodec,tmpbuf,data);
				TExecFree(TExecBase, tmpbuf);
				return TTRUE;
			}
			else
			{
				TExecFree(TExecBase, tmpbuf);
				return TFALSE;
			}
		}
	}
	else
		if(dtcodec->compression)
			return read_pcx_data_normal(dtcodec,data);
		else
			return read_pcx_data_normal_unpacked(dtcodec,data);
}

/**************************************************************************
	read pcx data normal
 **************************************************************************/
TBOOL read_pcx_data_normal(TMOD_DTCODEC *dtcodec, TUINT8 *data)
{
	TINT i,j,k,m,lb;
	TINT X,N;

	if(dtcodec->depth==1)
		lb=(dtcodec->width+7)/8;
	else
		lb=dtcodec->width;

	TIOSeek(dtcodec->io,dtcodec->fp,0x80,TNULL,TFPOS_BEGIN);
	i=0;
	for(i=0;i<dtcodec->height;i++)
	{
		for(j=dtcodec->farbebenen-1;j>=0;j--)
		{
			k=0;
			while(k<lb)
			{
				if(TEOF == (X=(TIOFGetC(dtcodec->io,dtcodec->fp))))
					return TTRUE;

				if((X & 0xc0)==0xc0)
				{
					N=X & 0x3f;
					if(TEOF == (X=(TIOFGetC(dtcodec->io,dtcodec->fp))))
						return TTRUE;
				}
				else
					N=1;

				for(m=0;m<N;m++)
					data[(i*lb+k+m)*dtcodec->farbebenen+j]=X;

				k+=N;
			}
			if(k<dtcodec->bytesperline)
				TIOSeek(dtcodec->io,dtcodec->fp,1,TNULL,TFPOS_CURRENT);
		}
	}
	return TTRUE;
}

/**************************************************************************
	read pcx data normal unpacked
 **************************************************************************/
TBOOL read_pcx_data_normal_unpacked(TMOD_DTCODEC *dtcodec, TUINT8* data)
{
	TINT i,j,k,lb;
	TINT X;

	if(dtcodec->depth==1)
		lb=(dtcodec->width+7)/8;
	else
		lb=dtcodec->width;

	TIOSeek(dtcodec->io,dtcodec->fp,0x80,TNULL,TFPOS_BEGIN);
	i=0;
	for(i=0;i<dtcodec->height;i++)
	{
		for(j=dtcodec->farbebenen-1;j>=0;j--)
		{
			k=0;
			while(k<lb)
			{
				if(TEOF == (X=(TIOFGetC(dtcodec->io,dtcodec->fp))))
					return TTRUE;

				data[(i*lb+k)*dtcodec->farbebenen+j]=X;

				k++;
			}
			if(k<dtcodec->bytesperline)
				TIOSeek(dtcodec->io,dtcodec->fp,1,TNULL,TFPOS_CURRENT);
		}
	}
	return TTRUE;
}

/**************************************************************************
	read pcx encget
 **************************************************************************/
TINT read_pcx_encget(TMOD_DTCODEC *dtcodec, TUINT8 *pbyt, TUINT8 *pcnt)
{
	TINT i;

	*pcnt = 1;
	if(TEOF == (i=(TIOFGetC(dtcodec->io,dtcodec->fp))))
		return (TEOF);
	if (0xC0 == (0xC0 & i))
	{
		*pcnt = 0x3F & i;
		if(TEOF == (i=(TIOFGetC(dtcodec->io,dtcodec->fp))))
			return (TEOF);
	}
	*pbyt = i;
	return (0);
}

/**************************************************************************
	read pcx data 4bit
 **************************************************************************/
TBOOL read_pcx_data_4bit(TMOD_DTCODEC *dtcodec, TUINT8*tmpbuf)
{
	TINT i,l,lsize;
	TUINT8 chr,cnt;
	TUINT8 *wptr;

	lsize = dtcodec->bytesperline * dtcodec->farbebenen * dtcodec->width * 2;
	wptr=tmpbuf;

	TIOSeek(dtcodec->io,dtcodec->fp,0x80,TNULL,TFPOS_BEGIN);

	for(l = 0; l < lsize; )
	{
		if(TEOF == read_pcx_encget(dtcodec,&chr, &cnt))
			return TTRUE;
		for (i = 0; i < cnt; i++)
			*wptr++ = chr;

		l += cnt;
	}
	return TTRUE;
}

/**************************************************************************
	read pcx data 4bit unpacked
 **************************************************************************/
TBOOL read_pcx_data_4bit_unpacked(TMOD_DTCODEC *dtcodec, TUINT8 *tmpbuf)
{
	TINT l, lsize;
	TINT chr;
	TUINT8 *wptr;

	lsize = dtcodec->bytesperline * dtcodec->farbebenen * dtcodec->width * 2;
	wptr=tmpbuf;

	TIOSeek(dtcodec->io,dtcodec->fp,0x80,TNULL,TFPOS_BEGIN);

	for(l = 0; l < lsize; l++ )
	{
		if(TEOF == (chr = TIOFGetC(dtcodec->io,dtcodec->fp)))
			return TTRUE;

		*wptr++ = chr;
	}
	return TTRUE;
}

/**************************************************************************
	read pcx data decode4bit
 **************************************************************************/
TVOID read_pcx_decode4bit(TMOD_DTCODEC *dtcodec, TUINT8* tmpbuf, TUINT8 *data)
{
	TINT i,j,rowoffset,lcount,lineoffset;
	TUINT8 v1,v2,v3,v4,val;

	TUINT8 *wptr=data;
	for(i=0;i<dtcodec->height;i++)
	{
		lineoffset=i*dtcodec->bytesperline*4;
		rowoffset=0;
		for(j=0;j<dtcodec->width;j+=8)
		{
			v1=tmpbuf[lineoffset+rowoffset];
			v2=tmpbuf[lineoffset+dtcodec->bytesperline+rowoffset];
			v3=tmpbuf[lineoffset+dtcodec->bytesperline+dtcodec->bytesperline+rowoffset];
			v4=tmpbuf[lineoffset+dtcodec->bytesperline+dtcodec->bytesperline+dtcodec->bytesperline+rowoffset];
			rowoffset++;
			if(j<dtcodec->width-8)
				lcount=7;
			else
				lcount=dtcodec->width-j-1;

			do
			{
				val=((((v4 << (7-lcount)) & 0x80)>>4) |
					 (((v3 << (7-lcount)) & 0x80)>>5) |
					 (((v2 << (7-lcount)) & 0x80)>>6) |
					 (((v1 << (7-lcount)) & 0x80)>>7));

				*wptr++ = val;
			}while(lcount--);
		}
	}
}

