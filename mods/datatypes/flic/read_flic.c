#include "codec_flic.h"

TINT Read_Flic_Seeklargestframe(TMOD_DTCODEC *dtcodec);
TVOID Read_Flic_Decode_COLOR_256(TMOD_DTCODEC *dtcodec, TIMGARGBCOLOR *palette, TUINT8 *palbuf, TUINT16 numpackets);
TVOID Read_Flic_Decode_DELTA_FLC(TMOD_DTCODEC *dtcodec, TUINT8 *data, TUINT8 *picbuf, TUINT16 numlines);
TVOID Read_Flic_Decode_COLOR_64(TMOD_DTCODEC *dtcodec, TIMGARGBCOLOR *palette, TUINT8 *palbuf, TUINT16 numpackets);
TVOID Read_Flic_Decode_BYTE_RUN(TMOD_DTCODEC *dtcodec, TUINT8 *data, TUINT8 *picbuf);
TVOID Read_Flic_Decode_DELTA_FLI(TMOD_DTCODEC *dtcodec, TUINT8 *data, TUINT8 *picbuf, TUINT16 startline, TUINT16 numlines);

TBOOL read_flic_open(TMOD_DTCODEC *dtcodec)
{
	TUINT8 flc;
	TINT16 header[5];
	TINT speed,maxframesize;

	TBOOL bigendian=TUtilIsBigEndian(TUtilBase);

	dtcodec->length=dtcodec->width=dtcodec->height=dtcodec->depth=dtcodec->speed=0;
	dtcodec->loop=TFALSE;

	/* check for fli or flc */
	TIOSeek(dtcodec->io,dtcodec->fp,4,TNULL,TFPOS_BEGIN);
	if(TIORead(dtcodec->io,dtcodec->fp,&flc,1)!=1)
		return TFALSE;

	if(flc==0x12)
		dtcodec->flc=TTRUE;
	else
		dtcodec->flc=TFALSE;

	/* header einlesen*/
	TIOSeek(dtcodec->io,dtcodec->fp,1,TNULL,TFPOS_CURRENT);
	if(TIORead(dtcodec->io,dtcodec->fp,header,10)!=10)
		return TFALSE;

	/* header auswerten */
	if(bigendian)
	{
		TUtilBSwap16(TUtilBase, &header[0]);
		TUtilBSwap16(TUtilBase, &header[1]);
		TUtilBSwap16(TUtilBase, &header[2]);
		TUtilBSwap16(TUtilBase, &header[3]);
		TUtilBSwap16(TUtilBase, &header[4]);
	}

	dtcodec->numframes=header[0];
	dtcodec->width=header[1];
	dtcodec->bytesperrow=header[1];
	dtcodec->height=header[2];
	dtcodec->depth=header[3];
	if(dtcodec->depth==0)
		dtcodec->depth=8;
	if(header[4]==2)
		dtcodec->loop=TTRUE;
	else
		dtcodec->loop=TFALSE;

	/* ablaufgeschwindigkeit einlesen */
	if(TIORead(dtcodec->io,dtcodec->fp,&speed,4)!=4)
		return TFALSE;

	if(bigendian)
		TUtilBSwap32(TUtilBase, &speed);

	if(dtcodec->flc)
		dtcodec->speed=speed;
	else
		dtcodec->speed=(speed*1000)/70;

	dtcodec->format=IMGFMT_CLUT;

	read_flic_rewind(dtcodec);
	maxframesize=Read_Flic_Seeklargestframe(dtcodec);
	read_flic_rewind(dtcodec);

	if(maxframesize==-1)
		return TFALSE;
	else
	{
		dtcodec->framebuf=TExecAlloc(TExecBase, TNULL,maxframesize);
	}
	return TTRUE;
}

TVOID read_flic_rewind(TMOD_DTCODEC *dtcodec)
{
	TIOSeek(dtcodec->io,dtcodec->fp,0x80,TNULL,TFPOS_BEGIN);
}

TBOOL read_flic_frame(TMOD_DTCODEC *dtcodec, TUINT8 *data, TIMGARGBCOLOR *palette)
{
	TINT framesize,chunksize;
	TUINT16 frametyp,numchunks,chunktyp;

	TBOOL bigendian=TUtilIsBigEndian(TUtilBase);

	/* frameheader */
	/* get framesize */
	if(TIORead(dtcodec->io,dtcodec->fp,&framesize,4)!=4)
		return TFALSE;
	if(bigendian)
		TUtilBSwap32(TUtilBase, &framesize);
	
	/* read in frametype */
	if(TIORead(dtcodec->io,dtcodec->fp,&frametyp,2)!=2)
		return TFALSE;
	if(bigendian)
		TUtilBSwap16(TUtilBase, (TINT16*)&frametyp);


	/* picture area */
	if(frametyp==0xf1fa)
	{
		TINT i,framepos;

		/* read in frame */
		if(TIORead(dtcodec->io,dtcodec->fp,dtcodec->framebuf,framesize-6)!=framesize-6)
			return TFALSE;

		framepos=0;

		/* read in number of chunks */
		TExecCopyMem(TExecBase, dtcodec->framebuf+framepos,&numchunks,2);
		framepos+=2;
		if(bigendian) TUtilBSwap16(TUtilBase, (TINT16*)&numchunks);

		/* seek to start of first chunk */
		framepos+=8;;

		for(i=0;i<numchunks;i++)
		{
			/* get chunksize */
			TExecCopyMem(TExecBase, dtcodec->framebuf+framepos,&chunksize,4);
			framepos+=4;
			if(bigendian) TUtilBSwap32(TUtilBase, &chunksize);

			/* get chunktyp */
			TExecCopyMem(TExecBase, dtcodec->framebuf+framepos,&chunktyp,2);
			framepos+=2;
			if(bigendian) TUtilBSwap16(TUtilBase, (TINT16*)&chunktyp);

			switch(chunktyp)
			{
				case 0x04:      /* COLOR_256 */
				{
					TUINT16 numpackets;
					
					TExecCopyMem(TExecBase, dtcodec->framebuf+framepos,&numpackets,2);
					framepos+=2;
					if(bigendian) TUtilBSwap16(TUtilBase, (TINT16*)&numpackets);
					
					if(chunksize-8>0)
					{
						Read_Flic_Decode_COLOR_256(dtcodec,palette,dtcodec->framebuf+framepos,numpackets);
						framepos+=chunksize-8;
					}
				}
				break;

				case 0x07:      /* DELTA_FLC */
				{
					TUINT16 numlines;

					TExecCopyMem(TExecBase, dtcodec->framebuf+framepos,&numlines,2);
					framepos+=2;
					if(bigendian) TUtilBSwap16(TUtilBase, (TINT16*)&numlines);

					if(chunksize-8>0)
					{
						Read_Flic_Decode_DELTA_FLC(dtcodec,data,dtcodec->framebuf+framepos,numlines);
						framepos+=chunksize-8;
					}
				}
				break;

				case 0x0b:      /* COLOR_64 */
				{
					TUINT16 numpackets;
					
					TExecCopyMem(TExecBase, dtcodec->framebuf+framepos,&numpackets,2);
					framepos+=2;
					if(bigendian) TUtilBSwap16(TUtilBase, (TINT16*)&numpackets);
					
					if(chunksize-8>0)
					{
						Read_Flic_Decode_COLOR_64(dtcodec,palette,dtcodec->framebuf+framepos,numpackets);
						framepos+=chunksize-8;
					}
				}
				break;

				case 0x0c:      /* DELTA_FLI */
				{
					TUINT16 startline,numlines;

					TExecCopyMem(TExecBase, dtcodec->framebuf+framepos,&startline,2);
					framepos+=2;
					if(bigendian) TUtilBSwap16(TUtilBase, (TINT16*)&startline);

					TExecCopyMem(TExecBase, dtcodec->framebuf+framepos,&numlines,2);
					framepos+=2;
					if(bigendian) TUtilBSwap16(TUtilBase, (TINT16*)&numlines);

					if(chunksize-10>0)
					{
						Read_Flic_Decode_DELTA_FLI(dtcodec,data,dtcodec->framebuf+framepos,startline,numlines);
						framepos+=chunksize-10;
					}
				}
				break;

				case 0x0d:      /* BLACK */
					TExecFillMem(TExecBase, data,dtcodec->bytesperrow*dtcodec->height,0);
				break;

				case 0x0f:      /* BYTE_RUN */
				{
					if(chunksize-6>0)
					{
						Read_Flic_Decode_BYTE_RUN(dtcodec,data,dtcodec->framebuf+framepos);
						framepos+=chunksize-6;
					}
				}
				break;

				case 0x10:      /* LITERAL */
					if(chunksize-6>0)
					{
						TExecCopyMem(TExecBase, dtcodec->framebuf+framepos,data,chunksize-6);
						framepos+=chunksize-6;
					}
				break;

				default: /* override unknown chunks */
					if(chunksize-6>0)
					{
						framepos+=chunksize-6;
					}
				break;
			}
		}
	}
	else
	{
		/* ignore other frametypes */
		TIOSeek(dtcodec->io,dtcodec->fp,framesize-6,TNULL,TFPOS_CURRENT);
	}
	return TTRUE;
}

TINT Read_Flic_Seeklargestframe(TMOD_DTCODEC *dtcodec)
{
	TINT i,maxframesize,framesize;
	TUINT16 frametyp;

	TBOOL bigendian=TUtilIsBigEndian(TUtilBase);
	maxframesize=0;

	for(i=0;i<dtcodec->numframes;i++)
	{
		if(TIORead(dtcodec->io,dtcodec->fp,&framesize,4)!=4)
			return -1;
		if(bigendian) TUtilBSwap32(TUtilBase, (TINT*)&framesize);

		/* read in frametype */
		if(TIORead(dtcodec->io,dtcodec->fp,&frametyp,2)!=2)
			return -1;
		if(bigendian) TUtilBSwap16(TUtilBase, (TINT16*)&frametyp);

		/* picture area */
		if(frametyp==0xf1fa)
		{
			if(maxframesize<framesize-6)
				maxframesize=framesize-6;

			TIOSeek(dtcodec->io,dtcodec->fp,framesize-6,TNULL,TFPOS_CURRENT);
		}
	}
	return maxframesize;
}

TVOID Read_Flic_Decode_COLOR_256(TMOD_DTCODEC *dtcodec, TIMGARGBCOLOR *palette, TUINT8 *palbuf, TUINT16 numpackets)
{
	TINT palpos,i,j,colorstochange;

	palpos=0;
	for(i=0;i<numpackets;i++)
	{
		palpos+=*palbuf++;

		colorstochange=*palbuf++;
		if(!colorstochange)
			colorstochange=256;

		for(j=1;j<=colorstochange;j++)
		{
			palette[palpos].r=*palbuf++;
			palette[palpos].g=*palbuf++;
			palette[palpos].b=*palbuf++;
			palpos++;
		}
	}
}

TVOID Read_Flic_Decode_DELTA_FLC(TMOD_DTCODEC *dtcodec, TUINT8 *data, TUINT8 *picbuf, TUINT16 numlines)
{
	TINT x,y,e,n,z,linecount,count;
	TUINT16 c,opcode;
	TUINT8 *tmpdat;
	TUINT16 *picbuf16, *tmpdat16;

	TBOOL bigendian=TUtilIsBigEndian(TUtilBase);

	linecount=0;
	y=0;
	tmpdat=data;
	while(linecount<numlines)
	{
		picbuf16=(TUINT16*)picbuf;
		opcode=*picbuf16;
		if(bigendian)
			TUtilBSwap16(TUtilBase, (TINT16*)&opcode);
		picbuf+=2;
		if( (opcode & 0xc000)==0x8000 )
		{
			/* Store the opcode's low byte in the last pixel of the line */
			tmpdat[dtcodec->width-1]=opcode & 0x00ff;
		}
		else if( (opcode & 0xc000)==0xc000 )
		{
			/* The absolute value of the opcode is the line skip count */
			y+=(opcode^0xffff)+1;
		}
		else if( (opcode & 0x4000)==0 )
		{
			/* Packet count for the line, it can be zero */
			count=opcode & 0x3fff;
			tmpdat=data+y*dtcodec->width;
			tmpdat16=(TUINT16*)tmpdat;

			if(count)
			{
				x=0;
				for(e=1;e<=count;e++)
				{
					x+=*picbuf++;
					n=*picbuf++;
					if(n>0x7f)
					{
						n=0x100-n;
						picbuf16=(TUINT16*)picbuf;
						c=*picbuf16;
						picbuf+=2;
						for(z=0;z<n;z++)
						{
							tmpdat16[x/2+z] = c;
						}

					}
					else
					{
						picbuf16=(TUINT16*)picbuf;
						for(z=0;z<n;z++)
						{
							tmpdat16[x/2+z] = *picbuf16++;
						}
						picbuf=(TUINT8*)picbuf16;
					}
					x+=n*2;
				}
			}
			y++;
			linecount++;
		}
	}
}

TVOID Read_Flic_Decode_COLOR_64(TMOD_DTCODEC *dtcodec, TIMGARGBCOLOR *palette, TUINT8 *palbuf, TUINT16 numpackets)
{
	TINT palpos,i,j,colorstochange;

	palpos=0;
	for(i=0;i<numpackets;i++)
	{
		palpos+=*palbuf++;

		colorstochange=*palbuf++;
		if(!colorstochange)
			colorstochange=256;

		for(j=1;j<=colorstochange;j++)
		{
			palette[palpos].r=(*palbuf++)<<2;
			palette[palpos].g=(*palbuf++)<<2;
			palette[palpos].b=(*palbuf++)<<2;
			palpos++;
		}
	}
}

TVOID Read_Flic_Decode_BYTE_RUN(TMOD_DTCODEC *dtcodec, TUINT8 *data, TUINT8 *picbuf)
{
	TINT x,y,n,z;
	TUINT8 c;
	TUINT8 *tmpdat;

	for(y=0;y<dtcodec->height;y++)
	{
		tmpdat=data+y*dtcodec->width;
		picbuf++; /* override packet count */
		x=0;
		while(x<dtcodec->width)
		{
			n=*picbuf++;
			if(n>0x7f)
			{
				n=0x100-n;
				for(z=0;z<n;z++)
				{
					tmpdat[x+z] = *picbuf++;
				}

			}
			else
			{
				c=*picbuf++;
				for(z=0;z<n;z++)
				{
					tmpdat[x+z] = c;
				}
			}
			x+=n;
		}
	}
}

TVOID Read_Flic_Decode_DELTA_FLI(TMOD_DTCODEC *dtcodec, TUINT8 *data, TUINT8 *picbuf, TUINT16 startline, TUINT16 numlines)
{
	TINT x,y,count,e,n,z;
	TUINT8 c;
	TUINT8 *tmpdat;

	for(y=startline;y<startline+numlines;y++)
	{
		tmpdat=data+y*dtcodec->width;
		count=*picbuf++;
		if(count)
		{
			x=0;
			for(e=1;e<=count;e++)
			{
				c=*picbuf++;
				x+=c;
				n=*picbuf++;
				if(n>0x7f)
				{
					n=0x100-n;
					c=*picbuf++;
					for(z=0;z<n;z++)
					{
						tmpdat[x+z] = c;
					}

				}
				else
				{
					for(z=0;z<n;z++)
					{
						tmpdat[x+z] = *picbuf++;
					}
				}
				x+=n;
			}
		}
	}
}
