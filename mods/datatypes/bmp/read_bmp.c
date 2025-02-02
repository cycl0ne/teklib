#include "codec_bmp.h"

TBOOL read_bmp_data4rle(TMOD_DTCODEC *dtcodec, TUINT8 *data);
TBOOL read_bmp_data8rle(TMOD_DTCODEC *dtcodec, TUINT8 *data);
TBOOL read_bmp_data1(TMOD_DTCODEC *dtcodec, TUINT8 *data);
TBOOL read_bmp_data4(TMOD_DTCODEC *dtcodec, TUINT8 *data);
TBOOL read_bmp_data8(TMOD_DTCODEC *dtcodec, TUINT8 *data);
TBOOL read_bmp_data24(TMOD_DTCODEC *dtcodec, TUINT8 *data);

TBOOL read_bmp_open(TMOD_DTCODEC *dtcodec)
{
	TUINT8 loaddata[0x36];
	TINT length,compression;

	dtcodec->start_data=dtcodec->length=0;
	dtcodec->width=dtcodec->height=dtcodec->usedcolors=0;
	dtcodec->depth=0;
	dtcodec->rle4=dtcodec->rle8=TFALSE;

	/*Header einlesen*/
	length=TIORead(dtcodec->io,dtcodec->fp,loaddata,0x36);
	if(length!=0x36)
		return TFALSE;

	/*im BMP-File angegebene Laenge kopieren*/
	TExecCopyMem(TExecBase, loaddata+0x02,&dtcodec->length,4);

	/*Start der Bilddaten holen*/
	TExecCopyMem(TExecBase, loaddata+0x0a,&dtcodec->start_data,4);

	if(TUtilIsBigEndian(TUtilBase))
	{
		TUtilBSwap32(TUtilBase, &dtcodec->length);
		TUtilBSwap32(TUtilBase, &dtcodec->start_data);
	}

	/*tatsaechliche Laenge der Bilddaten*/
	dtcodec->length-=dtcodec->start_data;

	/*Bildinfos holen*/
	TExecCopyMem(TExecBase, loaddata+0x12,&dtcodec->width,4);
	TExecCopyMem(TExecBase, loaddata+0x16,&dtcodec->height,4);
	TExecCopyMem(TExecBase, loaddata+0x1c,&dtcodec->depth,2);
	TExecCopyMem(TExecBase, loaddata+0x1e,&compression,4);
	TExecCopyMem(TExecBase, loaddata+0x2e,&dtcodec->usedcolors,4);

	if(TUtilIsBigEndian(TUtilBase))
	{
		TUtilBSwap32(TUtilBase, &dtcodec->width);
		TUtilBSwap32(TUtilBase, &dtcodec->height);
		TUtilBSwap16(TUtilBase, &dtcodec->depth);
		TUtilBSwap32(TUtilBase, &compression);
		TUtilBSwap32(TUtilBase, &dtcodec->usedcolors);
	}

	/* im codec format und bytesperrow setzen fuer getattrs */
	switch(dtcodec->depth)
	{
		case 1:
			dtcodec->bytesperrow=dtcodec->width;
			dtcodec->format=IMGFMT_CLUT;
			if(dtcodec->usedcolors==0)
				dtcodec->usedcolors=2;
		break;

		case 4:
			dtcodec->bytesperrow=dtcodec->width;
			dtcodec->format=IMGFMT_CLUT;
			if(dtcodec->usedcolors==0)
				dtcodec->usedcolors=16;
		break;

		case 8:
			dtcodec->bytesperrow=dtcodec->width;
			dtcodec->format=IMGFMT_CLUT;
			if(dtcodec->usedcolors==0)
				dtcodec->usedcolors=256;
		break;

		case 24:
			dtcodec->bytesperrow=dtcodec->width*3;
			dtcodec->format=IMGFMT_B8G8R8;
		break;

		default:
			return TFALSE;
	}

	if(compression==1)
		dtcodec->rle8=TTRUE;
	if(compression==2)
		dtcodec->rle4=TTRUE;

	return TTRUE;
}

TBOOL read_bmp_palette(TMOD_DTCODEC *dtcodec, TIMGARGBCOLOR *palette)
{
	TINT ry;
	TBOOL suc;

	if(dtcodec->depth<24)
	{
		TIOSeek(dtcodec->io,dtcodec->fp,0x36,TNULL,TFPOS_BEGIN);
		for(ry=0;ry<dtcodec->usedcolors;ry++)
		{
			suc=TFALSE;
			if(TIORead(dtcodec->io,dtcodec->fp,&palette[ry].b,1)==1)
			{
				if(TIORead(dtcodec->io,dtcodec->fp,&palette[ry].g,1)==1)
				{
					if(TIORead(dtcodec->io,dtcodec->fp,&palette[ry].r,1)==1)
					{
						if(TIORead(dtcodec->io,dtcodec->fp,&palette[ry].a,1)==1)
						{
							suc=TTRUE;
						}
					}
				}
			}
			if(!suc) return TFALSE;
		}
	}
	return TTRUE;
}

TBOOL read_bmp_data(TMOD_DTCODEC *dtcodec, TUINT8 *data)
{
	TIOSeek(dtcodec->io,dtcodec->fp,dtcodec->start_data,TNULL,TFPOS_BEGIN);

	if(dtcodec->rle4)
	{
		return read_bmp_data4rle(dtcodec,data);
	}
	else if(dtcodec->rle8)
	{
		return read_bmp_data8rle(dtcodec,data);
	}
	else if(dtcodec->depth==1)
	{
		return read_bmp_data1(dtcodec,data);
	}
	else if(dtcodec->depth==4)
	{
		return read_bmp_data4(dtcodec,data);
	}
	else if(dtcodec->depth==8)
	{
		return read_bmp_data8(dtcodec,data);
	}
	else if(dtcodec->depth==24)
	{
		return read_bmp_data24(dtcodec,data);
	}
	return TFALSE;
}

TBOOL read_bmp_data4rle(TMOD_DTCODEC *dtcodec, TUINT8 *data)
{
	TINT rx,ry;
	TINT count,readcount,writecount;
	TINT X1,X2,X3;
	TINT rb=((((dtcodec->width+1)>>1)+3) & 0xfffffffc)*2;

	TUINT8 *linebuf=TExecAlloc0(TExecBase,TNULL,rb);

	ry=dtcodec->height-1;
	rx=0;
	count=0;
	while(count<dtcodec->length && ry>=0)
	{
		if((X1=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
			return TFALSE;

		count++;

		if(!X1)
		{
			if((X2=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;

			count++;

			if(X2<3)
			{
				if(X2==0)
				{
					while(rx<dtcodec->width)
					{
						linebuf[rx]=0;
						rx++;
					}
					rx=0;
					ry--;
				}
				else if(X2==1)
				{
					while(ry>=0)
					{
						while(rx<dtcodec->width)
						{
							linebuf[rx]=0;
							rx++;
						}
						rx=0;
						ry--;
					}
				}
				else if(X2==2)
				{
					if((X3=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
						return TFALSE;

					count++;

					rx+=X3;

					if((X3=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
						return TFALSE;

					count++;

					ry-=X3;
				}
				TExecCopyMem(TExecBase,linebuf,data+(ry+1)*dtcodec->bytesperrow,dtcodec->bytesperrow);
				TExecFillMem(TExecBase,linebuf,rb,0);
			}
			else
			{
				readcount=(X2+1)/2;
				writecount=X2;
				while(readcount && rx<rb)
				{
					if((X3=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
						return TFALSE;

					count++;

					linebuf[rx]=X3>>4;
					writecount--;
					rx++;

					if(writecount && rx<rb)
					{
						linebuf[rx]=X3 & 0x0f;
						writecount--;
						rx++;
					}

					readcount--;
				}
				if(((X2+1)/2) & 0x00000001)
				{
					if((X3=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
						return TFALSE;

					count++;
				}
			}
		}
		else
		{
			if((X2=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;

			count++;

			while(X1 && rx<rb)
			{
				linebuf[rx]=X2>>4;
				X1--;
				rx++;

				if(X1 && rx<rb)
				{
					linebuf[rx]=X2 & 0x0f;
					X1--;
					rx++;
				}
			}
		}
	}
	TExecFree(TExecBase,linebuf);
	return TTRUE;
}

TBOOL read_bmp_data8rle(TMOD_DTCODEC *dtcodec, TUINT8 *data)
{
	TINT rx,ry;
	TINT count,readcount,writecount;
	TINT X1,X2,X3;
	TINT rb=(dtcodec->width+3) & 0xfffffffc;

	TUINT8 *linebuf=TExecAlloc0(TExecBase,TNULL,rb);

	ry=dtcodec->height-1;
	rx=0;
	count=0;
	while(count<dtcodec->length && ry>=0)
	{
		if((X1=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
			return TFALSE;

		count++;

		if(!X1)
		{
			if((X2=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;

			count++;

			if(X2<3)
			{
				if(X2==0)
				{
					while(rx<dtcodec->width)
					{
						linebuf[rx]=0;
						rx++;
					}
					rx=0;
					ry--;
				}
				else if(X2==1)
				{
					while(ry>=0)
					{
						while(rx<dtcodec->width)
						{
							linebuf[rx]=0;
							rx++;
						}
						rx=0;
						ry--;
					}
				}
				else if(X2==2)
				{
					if((X3=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
						return TFALSE;

					count++;

					rx+=X3;

					if((X3=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
						return TFALSE;

					count++;

					ry-=X3;
				}
				TExecCopyMem(TExecBase,linebuf,data+(ry+1)*dtcodec->bytesperrow,dtcodec->bytesperrow);
				TExecFillMem(TExecBase,linebuf,rb,0);
			}
			else
			{
				readcount=X2;
				writecount=X2;
				while(readcount && rx<rb)
				{
					if((X3=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
						return TFALSE;

					count++;

					linebuf[rx]=X3;
					writecount--;
					rx++;

					readcount--;
				}
				if(X2 & 0x00000001)
				{
					if((X3=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
						return TFALSE;

					count++;
				}
			}
		}
		else
		{
			if((X2=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;

			count++;

			while(X1 && rx<rb)
			{
				linebuf[rx]=X2;
				X1--;
				rx++;
			}
		}
	}
	TExecFree(TExecBase,linebuf);
	return TTRUE;
}

TBOOL read_bmp_data1(TMOD_DTCODEC *dtcodec, TUINT8 *data)
{
	TINT x,y,val;
	TINT lb=dtcodec->width>>3;
	TINT skip=((((dtcodec->width+7)>>3)+3) & 0xfffffffc)-lb;
	TINT rest=dtcodec->width-(lb<<3);
	if(rest)
		skip--;

	for(y=dtcodec->height-1;y>=0;y--)
	{
		TUINT8 *dst=data+y*dtcodec->bytesperrow;
		for(x=0;x<lb;x++)
		{
			if((val=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;
			
			dst[0]=val >>7;
			dst[1]=(val & 0x40) >>6;
			dst[2]=(val & 0x20) >>5;
			dst[3]=(val & 0x10) >>4;
			dst[4]=(val & 0x08) >>3;
			dst[5]=(val & 0x04) >>2;
			dst[6]=(val & 0x02) >>1;
			dst[7]=val & 0x01;

			dst+=8;
		}
		if(rest)
		{
			TINT andval=0x80;
			TINT sval=7;

			if((val=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;
			
			for(x=0;x<rest;x++)
			{
				*dst++ = (val & andval)>>sval;
				andval>>=1;
				sval--;
			}
		}
		if(skip)
			TIOSeek(TIOBase,dtcodec->fp,skip,TNULL,TFPOS_CURRENT);
	}

	return TTRUE;
}

TBOOL read_bmp_data4(TMOD_DTCODEC *dtcodec, TUINT8 *data)
{
	TINT x,y,val;
	TINT lb=dtcodec->width>>1;
	TINT skip=((((dtcodec->width+1)>>1)+3) & 0xfffffffc)-lb;
	TINT rest=dtcodec->width-(lb<<1);
	if(rest)
		skip--;

	for(y=dtcodec->height-1;y>=0;y--)
	{
		TUINT8 *dst=data+y*dtcodec->bytesperrow;
		for(x=0;x<lb;x++)
		{
			if((val=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;

			dst[0]=val>>4;
			dst[1]=val & 0x0f;
			dst+=2;
		}
		if(rest)
		{
			if((val=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;

			dst[0] = val >> 4;
		}
		if(skip)
			TIOSeek(TIOBase,dtcodec->fp,skip,TNULL,TFPOS_CURRENT);
	}

	return TTRUE;
}

TBOOL read_bmp_data8(TMOD_DTCODEC *dtcodec, TUINT8 *data)
{
	TINT x,y,val;
	TINT lb=dtcodec->width;
	TINT skip=((dtcodec->width+3) & 0xfffffffc)-lb;

	for(y=dtcodec->height-1;y>=0;y--)
	{
		TUINT8 *dst=data+y*dtcodec->bytesperrow;
		for(x=0;x<lb;x++)
		{
			if((val=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;

			*dst++=val;
		}
		if(skip)
			TIOSeek(TIOBase,dtcodec->fp,skip,TNULL,TFPOS_CURRENT);
	}
	return TTRUE;
}

TBOOL read_bmp_data24(TMOD_DTCODEC *dtcodec, TUINT8 *data)
{
	TINT x,y,val1,val2,val3;
	TINT lb=dtcodec->width*3;
	TINT skip=((dtcodec->width*3+3) & 0xfffffffc)-lb;

	for(y=dtcodec->height-1;y>=0;y--)
	{
		TUINT8 *dst=data+y*dtcodec->bytesperrow;
		for(x=0;x<lb;x+=3)
		{
			if((val1=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;
			if((val2=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;
			if((val3=TIOFGetC(TIOBase,dtcodec->fp))==TEOF)
				return TFALSE;

			dst[0]=val1;
			dst[1]=val2;
			dst[2]=val3;
			dst+=3;
		}
		if(skip)
			TIOSeek(TIOBase,dtcodec->fp,skip,TNULL,TFPOS_CURRENT);
	}
	return TTRUE;
}

