#ifndef _TEK_ANSICALL_ZLIB_H
#define _TEK_ANSICALL_ZLIB_H

/*
**	$Id: zlib.h,v 1.2 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/zlib.h - zlib module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define zlib_crc32(zlib,a,b,c) \
	(*(((TMODCALL TUINT(**)(TAPTR,uLong,const Bytef*,uInt))(zlib))[-1]))(zlib,a,b,c)

#define zlib_deflateInit(zlib,a,b) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp,int))(zlib))[-2]))(zlib,a,b)

#define zlib_deflate(zlib,a,b) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp,int))(zlib))[-3]))(zlib,a,b)

#define zlib_deflateEnd(zlib,a) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp))(zlib))[-4]))(zlib,a)

#define zlib_inflateInit(zlib,a) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp))(zlib))[-5]))(zlib,a)

#define zlib_inflate(zlib,a,b) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp,int))(zlib))[-6]))(zlib,a,b)

#define zlib_inflateEnd(zlib,a) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp))(zlib))[-7]))(zlib,a)

#define zlib_deflateInit2(zlib,a,b,c,d,e,f) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp,int,int,int,int,int))(zlib))[-8]))(zlib,a,b,c,d,e,f)

#define zlib_setDictionary(zlib,a,b,c) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp,const Bytef*,uInt))(zlib))[-9]))(zlib,a,b,c)

#define zlib_deflateCopy(zlib,a,b) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp,z_streamp))(zlib))[-10]))(zlib,a,b)

#define zlib_deflateReset(zlib,a) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp))(zlib))[-11]))(zlib,a)

#define zlib_deflateParams(zlib,a,b,c) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp,int,int))(zlib))[-12]))(zlib,a,b,c)

#define zlib_inflateInit2(zlib,a,b) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp,int))(zlib))[-13]))(zlib,a,b)

#define zlib_inflateSetDictionary(zlib,a,b,c) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp,const Bytef*,uInt))(zlib))[-14]))(zlib,a,b,c)

#define zlib_inflateSync(zlib,a) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp))(zlib))[-15]))(zlib,a)

#define zlib_inflateReset(zlib,a) \
	(*(((TMODCALL int(**)(TAPTR,z_streamp))(zlib))[-16]))(zlib,a)

#define zlib_compress(zlib,a,b,c,d,e) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TAPTR,TINT,TINT,TTAGITEM*))(zlib))[-17]))(zlib,a,b,c,d,e)

#define zlib_uncompress(zlib,a,b,c,d,e) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TAPTR,TINT,TINT,TTAGITEM*))(zlib))[-18]))(zlib,a,b,c,d,e)

#define zlib_getcompressed(zlib,a,b,c) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TAPTR,TINT,TTAGITEM*))(zlib))[-19]))(zlib,a,b,c)

#endif /* _TEK_ANSICALL_ZLIB_H */
