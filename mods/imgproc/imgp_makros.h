#define imgp_src_for_8bit		src=(TUINT8*) (imgp->srcdata + (y+imgp->srcy)*imgp->srcbytesperrow) + imgp->srcx;
#define imgp_src_for_16bit		src=(TUINT16*)(imgp->srcdata + (y+imgp->srcy)*imgp->srcbytesperrow) + imgp->srcx;
#define imgp_src_for_24bit		src=(TUINT8*) (imgp->srcdata + (y+imgp->srcy)*imgp->srcbytesperrow) + imgp->srcx*3;
#define imgp_src_for_32bit		src=(TUINT*)  (imgp->srcdata + (y+imgp->srcy)*imgp->srcbytesperrow) + imgp->srcx;
		
#define imgp_dst_for_8bit		dst=(TUINT8*) (imgp->dstdata + (y+imgp->dsty)*imgp->dstbytesperrow) + imgp->dstx;
#define imgp_dst_for_16bit		dst=(TUINT16*)(imgp->dstdata + (y+imgp->dsty)*imgp->dstbytesperrow) + imgp->dstx;
#define imgp_dst_for_24bit		dst=(TUINT8*) (imgp->dstdata + (y+imgp->dsty)*imgp->dstbytesperrow) + imgp->dstx*3;
#define imgp_dst_for_32bit		dst=(TUINT*)  (imgp->dstdata + (y+imgp->dsty)*imgp->dstbytesperrow) + imgp->dstx;

/***********************************************************************
	GetPixel
 ***********************************************************************/
static TINLINE TVOID IMG_GetPixel(TAPTR src, TUINT srcfmt, TUINT dstfmt, TIMGARGBCOLOR *pal, TIMGARGBCOLOR *pixel)
{
	switch(srcfmt)
	{
		case IMGFMT_CLUT:
		{
			if(dstfmt==IMGFMT_CLUT)
			{
				pixel->a = *(TUINT8*)src;
			}
			else
			{
				TUINT8 offset=*(TUINT8*)src;
				if(pal)
				{
					pixel->a = pal[offset].a;
					pixel->r = pal[offset].r;
					pixel->g = pal[offset].g;
					pixel->b = pal[offset].b;
				}
			}
		}
		break;

		case IMGFMT_R5G5B5:
		{
			TUINT16 *s=(TUINT16*)src;
			pixel->a = (*s & 0x8000) >> 8;
			pixel->r = (*s & 0x7c00) >> 7;
			pixel->g = (*s & 0x03e0) >> 2;
			pixel->b = (*s & 0x001f) << 3;
		}
		break;

		case IMGFMT_R5G6B5:
		{
			TUINT16 *s=(TUINT16*)src;
			pixel->a = 0;
			pixel->r = (*s & 0xf800) >> 8;
			pixel->g = (*s & 0x07e0) >> 3;
			pixel->b = (*s & 0x001f) << 3;
		}
		break;

		case IMGFMT_R8G8B8:
		{
			TUINT8 *s=(TUINT8*)src;
			pixel->a = 0;
			pixel->r = s[0];
			pixel->g = s[1];
			pixel->b = s[2];
		}
		break;

		case IMGFMT_A8R8G8B8:
		{
			TUINT32 *s=(TUINT32*)src;
			pixel->a = (*s & 0xff000000) >> 24;
			pixel->r = (*s & 0x00ff0000) >> 16;
			pixel->g = (*s & 0x0000ff00) >> 8;
			pixel->b = (*s & 0x000000ff);
		}
		break;

		case IMGFMT_R8G8B8A8:
		{
			TUINT32* s=(TUINT32*)src;
			pixel->a = (*s & 0x000000ff);
			pixel->r = (*s & 0xff000000) >> 24;
			pixel->g = (*s & 0x00ff0000) >> 16;
			pixel->b = (*s & 0x0000ff00) >> 8;
		}
		break;

		case IMGFMT_B8G8R8:
		{
			TUINT8* s=(TUINT8*)src;
			pixel->a = 0;
			pixel->b = s[0];
			pixel->g = s[1];
			pixel->r = s[2];
		}
		break;

		case IMGFMT_B8G8R8A8:
		{
			TUINT32* s=(TUINT32*)src;
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
static TINLINE TVOID IMG_PutPixel(TAPTR dst, TUINT srcfmt, TUINT dstfmt, TIMGARGBCOLOR *pixel)
{
	switch(dstfmt)
	{
		case IMGFMT_CLUT:
			if(srcfmt==IMGFMT_CLUT)
				*(TUINT8*)dst = pixel->a;
		break;

		case IMGFMT_R5G5B5:
			*(TUINT16*)dst = ((pixel->a & 0x80) << 8) | ((pixel->r & 0xf8) << 7) | ((pixel->g & 0xf8) << 2) | ((pixel->b & 0xf8) >> 3);
		break;

		case IMGFMT_R5G6B5:
			*(TUINT16*)dst = ((pixel->r & 0xf8) << 8) | ((pixel->g & 0xfc) << 3) | ((pixel->b & 0xf8) >> 3);
		break;

		case IMGFMT_R8G8B8:
		{
			TUINT8* d=(TUINT8*)dst;
			d[0]=pixel->r;
			d[1]=pixel->g;
			d[2]=pixel->b;
		}
		break;

		case IMGFMT_A8R8G8B8:
			*(TUINT32*)dst = (pixel->a << 24) | (pixel->r << 16) | (pixel->g << 8) | (pixel->b);
		break;

		case IMGFMT_R8G8B8A8:
			*(TUINT32*)dst = (pixel->a) | (pixel->r << 24) | (pixel->g << 16) | (pixel->b << 8);
		break;

		case IMGFMT_B8G8R8:
		{
			TUINT8* d=(TUINT8*)dst;
			d[0]=pixel->b;
			d[1]=pixel->g;
			d[2]=pixel->r;
		}
		break;

		case IMGFMT_B8G8R8A8:
			*(TUINT32*)dst = (pixel->a) | (pixel->r << 8) | (pixel->g << 16) | (pixel->b << 24);
		break;
	}
}
