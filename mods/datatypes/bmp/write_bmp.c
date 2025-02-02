#include "codec_bmp.h"

TBOOL make_palette(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic);
TINT make_bmp_1(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic);
TINT make_bmp_4(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic);
TINT make_bmp_8(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic);
TINT make_bmp_24(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic);
TINT make_bmp_4rle(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic, TUINT8 *tmpdata, TBOOL write);
TINT make_bmp_8rle(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic, TUINT8 *tmpdata, TBOOL write);

TBOOL write_bmp(TMOD_DTCODEC *dtcodec)
{
	TIMGPICTURE *spic=TNULL;
	TIMGPICTURE dpic;
	TINT i;
	TUINT8 *tmpdata=TNULL;

	TUINT16 ident=0x4d42;
	TUINT   datasize;
	TUINT   pad=0;
	TUINT   distance;

	TUINT   bmihsize=40;
	TUINT   picwidth,picheight;
	TUINT16 levels=1;
	TUINT16 bpp;
	TUINT   compression;
	TUINT   picsize,writesize;
	TUINT   xppm=0,yppm=0;
	TUINT   usedcolors;
	TUINT   importantcolors=0;

	TExecFillMem(TExecBase,&dpic,sizeof(TIMGPICTURE),0);

	// convert picture to needed format
	if(dtcodec->srcpic->depth<=8 && dtcodec->srcpic->format!=IMGFMT_CLUT)
	{
		spic=TImgAllocBitmap(TIMGPBase,dtcodec->srcpic->width,dtcodec->srcpic->height,IMGFMT_CLUT);
		TImgDoMethod(TIMGPBase,dtcodec->srcpic,spic,IMGMT_CONVERT,TNULL);
	}
	else if(dtcodec->srcpic->depth>8 && dtcodec->srcpic->format!=IMGFMT_B8G8R8)
	{
		spic=TImgAllocBitmap(TIMGPBase,dtcodec->srcpic->width,dtcodec->srcpic->height,IMGFMT_B8G8R8);
		TImgDoMethod(TIMGPBase,dtcodec->srcpic,spic,IMGMT_CONVERT,TNULL);
	}
	else
		spic=dtcodec->srcpic;

	// prepare palette data
	if(make_palette(dtcodec,spic,&dpic))
	{
		// prepare and convert source data to saveable bmp-data
		if(spic->depth==1)
		{
			picsize=make_bmp_1(dtcodec,spic,&dpic);
			compression=0;

			datasize=picsize+54+8;
			distance=54+8;
			picwidth=dpic.width;
			picheight=dpic.height;
			bpp=1;
			usedcolors=2;
		}
		else if(spic->depth<=4)
		{
			picsize=make_bmp_4(dtcodec,spic,&dpic);
			if(dtcodec->compression==0)
				compression=0;
			else
			{
				tmpdata=(TUINT8*)dpic.data;
				dpic.data=TNULL;
				picsize=make_bmp_4rle(dtcodec,spic,&dpic,tmpdata,TFALSE);
				if(picsize)
				{
					dpic.data=TExecAlloc0(TExecBase, TNULL,picsize);
					if(make_bmp_4rle(dtcodec,spic,&dpic,tmpdata,TTRUE))
						compression=2;
				}
			}

			datasize=picsize+54+64;
			distance=54+64;
			picwidth=dpic.width;
			picheight=dpic.height;
			bpp=4;
			usedcolors=16;
		}
		else if(spic->depth<=8)
		{
			picsize=make_bmp_8(dtcodec,spic,&dpic);
			if(dtcodec->compression==0)
				compression=0;
			else
			{
				tmpdata=(TUINT8*)dpic.data;
				dpic.data=TNULL;
				picsize=make_bmp_8rle(dtcodec,spic,&dpic,tmpdata,TFALSE);
				if(picsize)
				{
					dpic.data=TExecAlloc0(TExecBase, TNULL,picsize);
					if(make_bmp_8rle(dtcodec,spic,&dpic,tmpdata,TTRUE))
						compression=1;
				}
			}

			datasize=picsize+54+1024;
			distance=54+1024;
			picwidth=dpic.width;
			picheight=dpic.height;
			bpp=8;
			usedcolors=256;
		}
		else
		{
			picsize=make_bmp_24(dtcodec,spic,&dpic);
			compression=0;

			datasize=picsize+54;
			distance=54;
			picwidth=dpic.width;
			picheight=dpic.height;
			bpp=24;
			usedcolors=0;
		}

		writesize=picsize;

		// convert header to little endian, if running on a big endian machine
		if(TUtilIsBigEndian(TUtilBase))
		{
			TUtilBSwap16(TUtilBase, &ident);
			TUtilBSwap32(TUtilBase, &datasize);
			TUtilBSwap32(TUtilBase, &distance);
			TUtilBSwap32(TUtilBase, &bmihsize);
			TUtilBSwap32(TUtilBase, &picwidth);
			TUtilBSwap32(TUtilBase, &picheight);
			TUtilBSwap16(TUtilBase, &levels);
			TUtilBSwap16(TUtilBase, &bpp);
			TUtilBSwap32(TUtilBase, &compression);
			TUtilBSwap32(TUtilBase, &picsize);
			TUtilBSwap32(TUtilBase, &xppm);
			TUtilBSwap32(TUtilBase, &yppm);
			TUtilBSwap32(TUtilBase, &usedcolors);
		}

		// write header
		TIOWrite(TIOBase,dtcodec->fp,&ident,2);
		TIOWrite(TIOBase,dtcodec->fp,&datasize,4);
		TIOWrite(TIOBase,dtcodec->fp,&pad,4);
		TIOWrite(TIOBase,dtcodec->fp,&distance,4);

		TIOWrite(TIOBase,dtcodec->fp,&bmihsize,4);
		TIOWrite(TIOBase,dtcodec->fp,&picwidth,4);
		TIOWrite(TIOBase,dtcodec->fp,&picheight,4);
		TIOWrite(TIOBase,dtcodec->fp,&levels,2);
		TIOWrite(TIOBase,dtcodec->fp,&bpp,2);
		TIOWrite(TIOBase,dtcodec->fp,&compression,4);
		TIOWrite(TIOBase,dtcodec->fp,&picsize,4);
		TIOWrite(TIOBase,dtcodec->fp,&xppm,4);
		TIOWrite(TIOBase,dtcodec->fp,&yppm,4);
		TIOWrite(TIOBase,dtcodec->fp,&usedcolors,4);
		TIOWrite(TIOBase,dtcodec->fp,&importantcolors,4);

		// write palette, if needed
		if(dpic.depth<=8)
		{
			TUINT8 pad8=0;
			for(i=0;i<pow(2,dpic.depth);i++)
			{
				TIOWrite(TIOBase,dtcodec->fp,&dpic.palette[i].b,1);
				TIOWrite(TIOBase,dtcodec->fp,&dpic.palette[i].g,1);
				TIOWrite(TIOBase,dtcodec->fp,&dpic.palette[i].r,1);
				TIOWrite(TIOBase,dtcodec->fp,&pad8,1);
			}
		}

		// write picturedata
		TIOWrite(TIOBase,dtcodec->fp,dpic.data,writesize);

		// free memory
		if(dpic.data) TExecFree(TExecBase,dpic.data);
		if(dpic.palette) TExecFree(TExecBase,dpic.palette);
		if(tmpdata) TExecFree(TExecBase,tmpdata);

		// convert picture to needed format
		if(dtcodec->srcpic->depth<=8 && dtcodec->srcpic->format!=IMGFMT_CLUT)
			TImgFreeBitmap(TIMGPBase,spic);
		else if(dtcodec->srcpic->depth>8 && dtcodec->srcpic->format!=IMGFMT_B8G8R8)
			TImgFreeBitmap(TIMGPBase,spic);

		return TTRUE;
	}
	return TFALSE;
}

TBOOL make_palette(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic)
{
	TINT i;

	if(srcpic->depth>8)
	{
		dstpic->palette=TNULL;
		return TTRUE;
	}
	else
	{
		dstpic->palette=TExecAlloc0(TExecBase, TNULL,256*sizeof(TIMGARGBCOLOR));
		if(dstpic->palette)             
		{
			for(i=0;i<pow(2,srcpic->depth);i++)
			{
				TExecCopyMem(TExecBase,&srcpic->palette[i],&dstpic->palette[i],sizeof(TIMGARGBCOLOR));
			}
			return TTRUE;
		}
	}
	return TFALSE;
}

TINT make_bmp_1(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic)
{
	TINT i,j,k;
	TUINT8 val;

	dstpic->width=srcpic->width;
	dstpic->height=srcpic->height;
	dstpic->depth=1;
	dstpic->bytesperrow=(((srcpic->width+7)>>3)+3) & 0xfffffffc;
	dstpic->data=TExecAlloc0(TExecBase, TNULL,dstpic->bytesperrow*dstpic->height);
	if(dstpic->data)
	{
		for(i=0;i<srcpic->height;i++)
		{
			TUINT8 *src=(TUINT8*)srcpic->data + (srcpic->height-1-i)*srcpic->bytesperrow;
			TUINT8 *dst=(TUINT8*)dstpic->data + i*dstpic->bytesperrow;

			j=0;
			while(j<srcpic->width)
			{
				k=0;
				val=0;
				while(j<srcpic->width && k<8)
				{
					val     |= (*src++)<<(7-k);
					j++;
					k++;
				}
				*dst++ = val;
			}

		}
		return dstpic->bytesperrow*dstpic->height;
	}
	return 0;
}

TINT make_bmp_4(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic)
{
	TINT i,j;

	dstpic->width=srcpic->width;
	dstpic->height=srcpic->height;
	dstpic->depth=4;
	dstpic->bytesperrow=(((srcpic->width+1)>>1)+3) & 0xfffffffc;
	dstpic->data=TExecAlloc0(TExecBase, TNULL,dstpic->bytesperrow*dstpic->height);
	if(dstpic->data)
	{
		for(i=0;i<srcpic->height;i++)
		{
			TUINT8 *src=(TUINT8*)srcpic->data + (srcpic->height-1-i)*srcpic->bytesperrow;
			TUINT8 *dst=(TUINT8*)dstpic->data + i*dstpic->bytesperrow;
			for(j=0;j<srcpic->width/2;j++)
			{
				*dst++ = (src[0] << 4) | src[1];
				src+=2;
			}
			if(srcpic->width & 0x00000001)
				*dst++ = src[0] << 4;
		}
		return dstpic->bytesperrow*dstpic->height;
	}
	return 0;
}

TINT make_bmp_8(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic)
{
	TINT i,j;

	dstpic->width=srcpic->width;
	dstpic->height=srcpic->height;
	dstpic->depth=8;
	dstpic->bytesperrow=(srcpic->width+3) & 0xfffffffc;
	dstpic->data=TExecAlloc0(TExecBase, TNULL,dstpic->bytesperrow*dstpic->height);
	if(dstpic->data)
	{
		for(i=0;i<srcpic->height;i++)
		{
			TUINT8 *src=(TUINT8*)srcpic->data + (srcpic->height-1-i)*srcpic->bytesperrow;
			TUINT8 *dst=(TUINT8*)dstpic->data + i*dstpic->bytesperrow;
			for(j=0;j<srcpic->width;j++)
			{
				*dst++ = *src++;
			}
		}
		return dstpic->bytesperrow*dstpic->height;
	}
	return 0;
}

TINT make_bmp_24(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic)
{
	TINT i,j;

	dstpic->width=srcpic->width;
	dstpic->height=srcpic->height;
	dstpic->depth=24;
	dstpic->bytesperrow=(srcpic->width*3+3) & 0xfffffffc;
	dstpic->data=TExecAlloc0(TExecBase, TNULL,dstpic->bytesperrow*dstpic->height);
	if(dstpic->data)
	{
		for(i=0;i<srcpic->height;i++)
		{
			TUINT8 *src=(TUINT8*)srcpic->data + (srcpic->height-1-i)*srcpic->bytesperrow;
			TUINT8 *dst=(TUINT8*)dstpic->data + i*dstpic->bytesperrow;
			for(j=0;j<srcpic->width;j++)
			{
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
			}
		}
		return dstpic->bytesperrow*dstpic->height;
	}
	return 0;
}

TINT make_bmp_4rle(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic, TUINT8 *tmpdata, TBOOL write)
{
	TINT width=((srcpic->width+1)/2);

	TINT picsize=0;
	TINT8 val1,val2;
	TINT y1=0;
	TINT y2=0;
	TUINT8 *s1,*s2;
	TUINT8 *d=(TUINT8*)dstpic->data;

	do
	{
		TINT x1=0;
		TINT x2=0;
		
		s1=tmpdata+y1*dstpic->bytesperrow;
		while(x1<width)
		{
			// testen, ob Ende der Zeile 0 ist
			TBOOL suc=TTRUE;
			x2=x1;
			while(x2<width && suc)
			{
				if(s1[x2]!=0)
					suc=TFALSE;
				else
					x2++;
			}

			// wir sind bis zum Ende der Zeile gekommen - pruefen, ob der gesamte Rest 0 ist
			if(suc)
			{
				x1=width;

				y2=y1+1;
				while(y2<srcpic->height && suc)
				{
					s2=tmpdata+y2*dstpic->bytesperrow;
					x2=0;
					while(x2<width && suc)
					{
						if(s2[x2]!=0)
							suc=TFALSE;
						else
							x2++;
					}
					y2++;
				}

				if(suc)
					y1=srcpic->height;
			}
			// normal rle packen
			else
			{
				val1=s1[x1];
				x2=x1+1;
				if(x2>=width)
				{
					if(write)
					{
						d[picsize]=1;
						d[picsize+1]=val1;
					}
					picsize+=2;
				}
				else
				{
					val2=s1[x2];
					if(val1==val2)
					{
						TINT count=1;
						do
						{
							x2++;
							count++;
							if(x2<width)
							{
								val2=s1[x2];
							}
						} while(x2<width && val1==val2 && count<127);
						if(write)
						{
							d[picsize]=count*2;
							d[picsize+1]=val1;
						}
						picsize+=2;
					}
					else
					{
						TINT count=0;

						do 
						{
							x2++;
							count++;
							val1=val2;
							if(x2<width)
							{
								val2=s1[x2];
							}

							if(x2+1<width)
							{
								if(s1[x2+1]!=val2)
								{
									x2++;
									count++;
									val1=val2;
									val2=s1[x2];
								}
							}
						} while(x2<width && val1!=val2 && count<127);

						if(val1==val2)
							x2--;

						if(count*2>2)
						{
							if(write)
							{
								d[picsize]=0;
								d[picsize+1]=count*2;
							}
							picsize+=2;
							
							if(write)
								TExecCopyMem(TExecBase,s1+x1,d+picsize,count);

							picsize+=count;

							if(count & 0x00000001)
							{
								if(write)
									d[picsize]=0;

								picsize++;
							}
						}
						else
						{
							while(count)
							{
								if(write)
								{
									d[picsize]=2;
									d[picsize+1]=s1[x1];
								}
								picsize+=2;
								x1++;
								count--;
							}
						}
					}
				}
				x1=x2;
			}
		}
		y1++;

		// Zeilenende markieren mit $00 / $00
		if(y1<srcpic->height)
		{
			if(write)
			{
				d[picsize]=0;
				d[picsize+1]=0;
			}
			picsize+=2;
		}
	}while(y1<srcpic->height);

	// zum Schluss $00 / $01 schreiben für Ende der Grafik
	if(write)
	{
		d[picsize]=0;
		d[picsize+1]=1;
	}
	picsize+=2;

	return picsize;
}

TINT make_bmp_8rle(TMOD_DTCODEC *dtcodec, TIMGPICTURE *srcpic, TIMGPICTURE *dstpic, TUINT8 *tmpdata, TBOOL write)
{
	TINT picsize=0;
	TINT8 val1,val2;
	TINT y1=0;
	TINT y2=0;
	TUINT8 *s1,*s2;
	TUINT8 *d=(TUINT8*)dstpic->data;

	TINT width=(srcpic->width+3) & 0xfffffffc;

	do
	{
		TINT x1=0;
		TINT x2=0;
		
		s1=tmpdata+y1*dstpic->bytesperrow;
		while(x1<width)
		{
			// testen, ob Ende der Zeile 0 ist
			TBOOL suc=TTRUE;
			x2=x1;
			while(x2<width && suc)
			{
				if(s1[x2]!=0)
					suc=TFALSE;
				else
					x2++;
			}

			// wir sind bis zum Ende der Zeile gekommen - prüfen, ob der gesamte Rest 0 ist
			if(suc)
			{
				x1=width;

				y2=y1+1;
				while(y2<srcpic->height && suc)
				{
					s2=tmpdata+y2*dstpic->bytesperrow;
					x2=0;
					while(x2<width && suc)
					{
						if(s2[x2]!=0)
							suc=TFALSE;
						else
							x2++;
					}
					y2++;
				}

				if(suc)
					y1=srcpic->height;
			}
			// normal rle packen
			else
			{
				val1=s1[x1];
				x2=x1+1;
				if(x2==width)
				{
					if(write)
					{
						d[picsize]=1;
						d[picsize+1]=val1;
					}
					picsize+=2;
				}
				else
				{
					val2=s1[x2];
					if(val1==val2)
					{
						TINT count=1;
						do
						{
							x2++;
							count++;
							if(x2<width)
							{
								val2=s1[x2];
							}
						} while(x2<width && val1==val2 && count<255);
						if(write)
						{
							d[picsize]=count;
							d[picsize+1]=val1;
						}
						picsize+=2;
					}
					else
					{
						TINT count=0;

						do 
						{
							x2++;
							count++;
							val1=val2;
							if(x2<width)
							{
								val2=s1[x2];
							}

							if(x2+1<width)
							{
								if(s1[x2+1]!=val2)
								{
									x2++;
									count++;
									val1=val2;
									val2=s1[x2];
								}
							}
						} while(x2<width && val1!=val2 && count<255);

						if(val1==val2)
							x2--;

						if(count>2)
						{
							if(write)
							{
								d[picsize]=0;
								d[picsize+1]=count;
							}
							picsize+=2;

							if(write)
								TExecCopyMem(TExecBase,s1+x1,d+picsize,count);

							picsize+=count;
							if(count & 0x00000001)
							{
								if(write)
									d[picsize]=0;

								picsize++;
							}
						}
						else
						{
							while(count)
							{
								if(write)
								{
									d[picsize]=1;
									d[picsize+1]=s1[x1];
								}
								picsize+=2;
								x1++;
								count--;
							}
						}
					}
				}
				x1=x2;
			}
		}
		y1++;

		// Zeilenende markieren mit $00 / $00
		if(y1<srcpic->height)
		{
			if(write)
			{
				d[picsize]=0;
				d[picsize+1]=0;
			}
			picsize+=2;
		}
	}while(y1<srcpic->height);

	// zum Schluss $00 / $01 schreiben für Ende der Grafik
	if(write)
	{
		d[picsize]=0;
		d[picsize+1]=1;
	}
	picsize+=2;

	return picsize;
}
