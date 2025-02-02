#include "imgproc.h"

/***********************************************************************
	GetPixel
 ***********************************************************************/
TINLINE TVOID IMG_GetPixel(TMOD_IMGPROC *imgp, TUINT8 *src, TINT x, TIMGARGBCOLOR *pixel)
{
	switch(imgp->srcfmt)
	{
		case IMGFMT_CLUT:
		{
			TUINT8* s=(TUINT8*)(src + x);

			if(imgp->dstfmt==IMGFMT_CLUT)
			{
				pixel->a = *s;
			}
			else
			{
				if(imgp->srcpalette)
				{
					pixel->a = (imgp->srcpalette[*s].a & 0x8000) >> 8;
					pixel->r = (imgp->srcpalette[*s].r & 0x7c00) >> 7;
					pixel->g = (imgp->srcpalette[*s].g & 0x03e0) >> 2;
					pixel->b = (imgp->srcpalette[*s].b & 0x001f) << 3;
				}
			}
		}
		break;

		case IMGFMT_R5G5B5:
		{
			TUINT16* s=(TUINT16*)(src + x+x);
			pixel->a = (*s & 0x8000) >> 8;
			pixel->r = (*s & 0x7c00) >> 7;
			pixel->g = (*s & 0x03e0) >> 2;
			pixel->b = (*s & 0x001f) << 3;
		}
		break;

		case IMGFMT_R5G6B5:
		{
			TUINT16* s=(TUINT16*)(src + x+x);
			pixel->a = 0;
			pixel->r = (*s & 0xf800) >> 8;
			pixel->g = (*s & 0x07e0) >> 3;
			pixel->b = (*s & 0x001f) << 3;
		}
		break;

		case IMGFMT_R8G8B8:
		{
			TUINT8* s=(TUINT8*)(src + x+x+x);
			pixel->a = 0;
			pixel->r = s[0];
			pixel->g = s[1];
			pixel->b = s[2];
		}
		break;

		case IMGFMT_A8R8G8B8:
		{
			TUINT32* s=(TUINT32*)(src + (x<<2));
			pixel->a = (*s & 0xff000000) >> 24;
			pixel->r = (*s & 0x00ff0000) >> 16;
			pixel->g = (*s & 0x0000ff00) >> 8;
			pixel->b = (*s & 0x000000ff);
		}
		break;

		case IMGFMT_R8G8B8A8:
		{
			TUINT32* s=(TUINT32*)(src + (x<<2));
			pixel->a = (*s & 0x000000ff);
			pixel->r = (*s & 0xff000000) >> 24;
			pixel->g = (*s & 0x00ff0000) >> 16;
			pixel->b = (*s & 0x0000ff00) >> 8;
		}
		break;

		case IMGFMT_B8G8R8:
		{
			TUINT8* s=(TUINT8*)(src + x+x+x);
			pixel->a = 0;
			pixel->b = s[0];
			pixel->g = s[1];
			pixel->r = s[2];
		}
		break;

		case IMGFMT_B8G8R8A8:
		{
			TUINT32* s=(TUINT32*)(src + (x<<2));
			pixel->a = (*s & 0x000000ff);
			pixel->b = (*s & 0xff000000) >> 24;
			pixel->g = (*s & 0x00ff0000) >> 16;
			pixel->r = (*s & 0x0000ff00) >> 8;
		}
		break;
	}
}

/***********************************************************************
	PutPixel
 ***********************************************************************/
TVOID IMG_PutPixel(TMOD_IMGPROC *imgp, TUINT8 *dst, TINT x, TIMGARGBCOLOR *pixel)
{
	switch(imgp->dstfmt)
	{
		case IMGFMT_CLUT:
		{
			TUINT8* d=(TUINT8*)(dst + x);

			if(imgp->srcfmt==IMGFMT_CLUT)
			{
				*d = pixel->a;
			}
		}
		break;

		case IMGFMT_R5G5B5:
		{
			TUINT16* d=(TUINT16*)(dst + x+x);
			*d = ((pixel->a & 0x80) << 8) | ((pixel->r & 0xf8) << 7) | ((pixel->g & 0xf8) << 2) | ((pixel->b & 0xf8) >> 3);
		}
		break;

		case IMGFMT_R5G6B5:
		{
			TUINT16* d=(TUINT16*)(dst + x+x);
			*d = ((pixel->r & 0xf8) << 8) | ((pixel->g & 0xfc) << 3) | ((pixel->b & 0xf8) >> 3);
		}
		break;

		case IMGFMT_R8G8B8:
		{
			TUINT8* d=(TUINT8*)(dst + x+x+x);
			d[0]=pixel->r;
			d[1]=pixel->g;
			d[2]=pixel->b;
		}
		break;

		case IMGFMT_A8R8G8B8:
		{
			TUINT32* d=(TUINT32*)(dst + (x<<2));
			*d = (pixel->a << 24) | (pixel->r << 16) | (pixel->g << 8) | (pixel->b);
		}
		break;

		case IMGFMT_R8G8B8A8:
		{
			TUINT32* d=(TUINT32*)(dst + (x<<2));
			*d = (pixel->a) | (pixel->r << 24) | (pixel->g << 16) | (pixel->b << 8);
		}
		break;

		case IMGFMT_B8G8R8:
		{
			TUINT8* d=(TUINT8*)(dst + x+x+x);
			d[0]=pixel->b;
			d[1]=pixel->g;
			d[2]=pixel->r;
		}
		break;

		case IMGFMT_B8G8R8A8:
		{
			TUINT32* d=(TUINT32*)(dst + (x<<2));
			*d = (pixel->a) | (pixel->r << 8) | (pixel->g << 16) | (pixel->b << 24);
		}
		break;
	}
}
