#include "imgproc.h"

/***********************************************************************
	Line 8 bit
 ***********************************************************************/
TVOID IMGLine_8(TIMGPICTURE *pic, TINT xa, TINT ya, TINT xe, TINT ye, TUINT col)
{
	if(xa==xe)
	{
		TINT sx,sy,ey;
		TUINT8* dst;
		TINT lineoff;

		if(ya<ye)
		{
			sx=xa;
			sy=ya;
			ey=ye;
		}
		else
		{
			sx=xe;
			sy=ye;
			ey=ya;
		}

		dst=(TUINT8*)pic->data + sy*pic->bytesperrow + sx;
		lineoff=pic->bytesperrow;

		while(sy<=ey)
		{
			*dst=col;
			dst+=lineoff;
			sy++;
		}
	}
	else if(ya==ye)
	{
		TINT sy,sx,ex;
		TUINT8* dst;
		TUINT* dsti;
		TUINT coli;

		if(xa<xe)
		{
			sy=ya;
			sx=xa;
			ex=xe;
		}
		else
		{
			sy=ye;
			sx=xe;
			ex=xa;
		}

		dst=(TUINT8*)pic->data + sy*pic->bytesperrow + sx;
		dsti=(TUINT*)dst;
		coli=(col<<24)|(col<<16)|(col<<8)|col;

		while(sx<(ex & 0x7ffffffc))
		{
			*dsti++=coli;
			sx+=4;
		}
		if(ex & 0x00000003)
		{
			dst=(TUINT8*)dsti;
			while(sx<ex)
			{
				*dst++=col;
				sx++;
			}
		}
	}
	else
	{
		TINT dx,dy,d,sx,sy,ex,ey,incE,incNE,lineoff,rowoff;
		TUINT8 *dst;

		lineoff=pic->bytesperrow;

		if(xe>xa)
			dx=xe-xa;
		else
			dx=xa-xe;

		if(ye>ya)
			dy=ye-ya;
		else
			dy=ya-ye;

		if(dy<=dx)
		{
			if(xe>xa)
			{
				sx=xa;
				ex=xe;
				sy=ya;
				ey=ye;
			}
			else
			{
				sx=xe;
				ex=xa;
				sy=ye;
				ey=ya;
			}

			dy=ey-sy;

			if(dy<0)
			{
				lineoff=-lineoff;
				dy=-dy;
			}

			incE=dy+dy;
			incNE=incE-(dx+dx);
			d=incE-dx;

			dst=(TUINT8*)pic->data + sy*pic->bytesperrow + sx;
			while(sx<=ex)
			{
				*dst=col;

				if(d<=0)
					d+=incE;
				else
				{
					d+=incNE;
					dst+=lineoff;
				}
				dst++;
				sx++;
			}
		}
		else
		{
			if(ye>ya)
			{
				sy=ya;
				ey=ye;
				sx=xa;
				ex=xe;
			}
			else
			{
				sy=ye;
				ey=ya;
				sx=xe;
				ex=xa;
			}

			dx=ex-sx;

			if(dx<0)
			{
				rowoff=-1;
				dx=-dx;
			}
			else
			{
				rowoff=1;
			}

			incE=dx+dx;
			incNE=incE-(dy+dy);
			d=incE-dy;

			dst=(TUINT8*)pic->data + sy*pic->bytesperrow+sx;
			while(sy<=ey)
			{
				*dst=col;

				if(d<=0)
					d+=incE;
				else
				{
					d+=incNE;
					dst+=rowoff;
				}
				dst+=lineoff;
				sy++;
			}
		}
	}
}

/***********************************************************************
	Line 16 bit
 ***********************************************************************/
TVOID IMGLine_16(TIMGPICTURE *pic, TINT xa, TINT ya, TINT xe, TINT ye, TUINT col)
{
	if(xa==xe)
	{
		TINT sx,sy,ey;
		TUINT16* dst;
		TINT lineoff;

		if(ya<ye)
		{
			sx=xa;
			sy=ya;
			ey=ye;
		}
		else
		{
			sx=xe;
			sy=ye;
			ey=ya;
		}

		dst=(TUINT16*)((TUINT8*)pic->data + sy*pic->bytesperrow) + sx;
		lineoff=pic->bytesperrow>>1;

		while(sy<=ey)
		{
			*dst=col;
			dst+=lineoff;
			sy++;
		}
	}
	else if(ya==ye)
	{
		TINT sy,sx,ex;
		TUINT16* dst;
		TUINT* dsti;
		TUINT coli;

		if(xa<xe)
		{
			sy=ya;
			sx=xa;
			ex=xe;
		}
		else
		{
			sy=ye;
			sx=xe;
			ex=xa;
		}

		dst=(TUINT16*)((TUINT8*)pic->data + sy*pic->bytesperrow) + sx;
		dsti=(TUINT*)dst;
		coli=(col<<16)|col;

		while(sx<(ex & 0x7ffffffe))
		{
			*dsti++=coli;
			sx+=2;
		}
		if(ex & 0x00000001)
		{
			dst=(TUINT16*)dsti;
			*dst=col;
		}
	}
	else
	{
		TINT dx,dy,d,sx,sy,ex,ey,incE,incNE,lineoff,rowoff;
		TUINT16 *dst;

		lineoff=pic->bytesperrow>>1;

		if(xe>xa)
			dx=xe-xa;
		else
			dx=xa-xe;

		if(ye>ya)
			dy=ye-ya;
		else
			dy=ya-ye;

		if(dy<=dx)
		{
			if(xe>xa)
			{
				sx=xa;
				ex=xe;
				sy=ya;
				ey=ye;
			}
			else
			{
				sx=xe;
				ex=xa;
				sy=ye;
				ey=ya;
			}

			dy=ey-sy;

			if(dy<0)
			{
				lineoff=-lineoff;
				dy=-dy;
			}

			incE=dy+dy;
			incNE=incE-(dx+dx);
			d=incE-dx;

			dst=(TUINT16*)((TUINT8*)pic->data + sy*pic->bytesperrow) + sx;
			while(sx<=ex)
			{
				*dst=col;

				if(d<=0)
					d+=incE;
				else
				{
					d+=incNE;
					dst+=lineoff;
				}
				dst++;
				sx++;
			}
		}
		else
		{
			if(ye>ya)
			{
				sy=ya;
				ey=ye;
				sx=xa;
				ex=xe;
			}
			else
			{
				sy=ye;
				ey=ya;
				sx=xe;
				ex=xa;
			}

			dx=ex-sx;

			if(dx<0)
			{
				rowoff=-1;
				dx=-dx;
			}
			else
			{
				rowoff=1;
			}

			incE=dx+dx;
			incNE=incE-(dy+dy);
			d=incE-dy;

			dst=(TUINT16*)((TUINT8*)pic->data + sy*pic->bytesperrow)+sx;
			while(sy<=ey)
			{
				*dst=col;

				if(d<=0)
					d+=incE;
				else
				{
					d+=incNE;
					dst+=rowoff;
				}
				dst+=lineoff;
				sy++;
			}
		}
	}
}

/***********************************************************************
	Line 24 bit
 ***********************************************************************/
TVOID IMGLine_24(TIMGPICTURE *pic, TINT xa, TINT ya, TINT xe, TINT ye, TUINT8 r, TUINT8 g, TUINT8 b)
{
	if(xa==xe)
	{
		TINT sx,sy,ey;
		TUINT8* dst;
		TINT lineoff;

		if(ya<ye)
		{
			sx=xa;
			sy=ya;
			ey=ye;
		}
		else
		{
			sx=xe;
			sy=ye;
			ey=ya;
		}

		dst=(TUINT8*)pic->data + sy*pic->bytesperrow + sx*3;
		lineoff=pic->bytesperrow;

		while(sy<=ey)
		{
			dst[0]=r;
			dst[1]=g;
			dst[2]=b;
			dst+=lineoff;
			sy++;
		}
	}
	else if(ya==ye)
	{
		TINT sy,sx,ex;
		TUINT8* dst;

		if(xa<xe)
		{
			sy=ya;
			sx=xa;
			ex=xe;
		}
		else
		{
			sy=ye;
			sx=xe;
			ex=xa;
		}

		dst=(TUINT8*)pic->data + sy*pic->bytesperrow + sx*3;

		while(sx<ex)
		{
			dst[0]=r;
			dst[1]=g;
			dst[2]=b;
			dst+=3;
			sx++;
		}
	}
	else
	{
		TINT dx,dy,d,sx,sy,ex,ey,incE,incNE,lineoff,rowoff;
		TUINT8 *dst;

		lineoff=pic->bytesperrow;

		if(xe>xa)
			dx=xe-xa;
		else
			dx=xa-xe;

		if(ye>ya)
			dy=ye-ya;
		else
			dy=ya-ye;

		if(dy<=dx)
		{
			if(xe>xa)
			{
				sx=xa;
				ex=xe;
				sy=ya;
				ey=ye;
			}
			else
			{
				sx=xe;
				ex=xa;
				sy=ye;
				ey=ya;
			}

			dy=ey-sy;

			if(dy<0)
			{
				lineoff=-lineoff;
				dy=-dy;
			}

			incE=dy+dy;
			incNE=incE-(dx+dx);
			d=incE-dx;

			dst=(TUINT8*)pic->data + sy*pic->bytesperrow + sx*3;
			while(sx<=ex)
			{
				dst[0]=r;
				dst[1]=g;
				dst[2]=b;

				if(d<=0)
					d+=incE;
				else
				{
					d+=incNE;
					dst+=lineoff;
				}
				dst+=3;
				sx++;
			}
		}
		else
		{
			if(ye>ya)
			{
				sy=ya;
				ey=ye;
				sx=xa;
				ex=xe;
			}
			else
			{
				sy=ye;
				ey=ya;
				sx=xe;
				ex=xa;
			}

			dx=ex-sx;

			if(dx<0)
			{
				rowoff=-3;
				dx=-dx;
			}
			else
			{
				rowoff=3;
			}

			incE=dx+dx;
			incNE=incE-(dy+dy);
			d=incE-dy;

			dst=(TUINT8*)pic->data + sy*pic->bytesperrow+sx*3;
			while(sy<=ey)
			{
				dst[0]=r;
				dst[1]=g;
				dst[2]=b;

				if(d<=0)
					d+=incE;
				else
				{
					d+=incNE;
					dst+=rowoff;
				}
				dst+=lineoff;
				sy++;
			}
		}
	}
}

/***********************************************************************
	Line 32 bit
 ***********************************************************************/
TVOID IMGLine_32(TIMGPICTURE *pic, TINT xa, TINT ya, TINT xe, TINT ye, TUINT col)
{
	if(xa==xe)
	{
		TINT sx,sy,ey;
		TUINT* dst;
		TINT lineoff;

		if(ya<ye)
		{
			sx=xa;
			sy=ya;
			ey=ye;
		}
		else
		{
			sx=xe;
			sy=ye;
			ey=ya;
		}

		dst=(TUINT*)((TUINT8*)pic->data + sy*pic->bytesperrow) + sx;
		lineoff=pic->bytesperrow>>2;

		while(sy<=ey)
		{
			*dst=col;
			dst+=lineoff;
			sy++;
		}
	}
	else if(ya==ye)
	{
		TINT sy,sx,ex;
		TUINT* dst;

		if(xa<xe)
		{
			sy=ya;
			sx=xa;
			ex=xe;
		}
		else
		{
			sy=ye;
			sx=xe;
			ex=xa;
		}

		dst=(TUINT*)((TUINT8*)pic->data + sy*pic->bytesperrow) + sx;

		while(sx<ex)
		{
			*dst++=col;
			sx++;
		}
	}
	else
	{
		TINT dx,dy,d,sx,sy,ex,ey,incE,incNE,lineoff,rowoff;
		TUINT *dst;

		lineoff=pic->bytesperrow>>2;

		if(xe>xa)
			dx=xe-xa;
		else
			dx=xa-xe;

		if(ye>ya)
			dy=ye-ya;
		else
			dy=ya-ye;

		if(dy<=dx)
		{
			if(xe>xa)
			{
				sx=xa;
				ex=xe;
				sy=ya;
				ey=ye;
			}
			else
			{
				sx=xe;
				ex=xa;
				sy=ye;
				ey=ya;
			}

			dy=ey-sy;

			if(dy<0)
			{
				lineoff=-lineoff;
				dy=-dy;
			}

			incE=dy+dy;
			incNE=incE-(dx+dx);
			d=incE-dx;

			dst=(TUINT*)((TUINT8*)pic->data + sy*pic->bytesperrow) + sx;
			while(sx<=ex)
			{
				*dst=col;

				if(d<=0)
					d+=incE;
				else
				{
					d+=incNE;
					dst+=lineoff;
				}
				dst++;
				sx++;
			}
		}
		else
		{
			if(ye>ya)
			{
				sy=ya;
				ey=ye;
				sx=xa;
				ex=xe;
			}
			else
			{
				sy=ye;
				ey=ya;
				sx=xe;
				ex=xa;
			}

			dx=ex-sx;

			if(dx<0)
			{
				rowoff=-1;
				dx=-dx;
			}
			else
			{
				rowoff=1;
			}

			incE=dx+dx;
			incNE=incE-(dy+dy);
			d=incE-dy;

			dst=(TUINT*)((TUINT8*)pic->data + sy*pic->bytesperrow)+sx;
			while(sy<=ey)
			{
				*dst=col;

				if(d<=0)
					d+=incE;
				else
				{
					d+=incNE;
					dst+=rowoff;
				}
				dst+=lineoff;
				sy++;
			}
		}
	}
}
