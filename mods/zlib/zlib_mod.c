
/*
**	zlib module
**	$Id: zlib_mod.c,v 1.4 2005/09/11 00:57:19 tmueller Exp $
*/

#include <tek/exec.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/debug.h>
#include <tek/mod/zlib/zlib.h>

#include "zlib_mod.h"

#define MOD_VERSION		1
#define MOD_REVISION	23
#define MOD_NUMVECTORS	19

/*****************************************************************************/
/*
**	module prototypes
*/

static const TAPTR vectors[MOD_NUMVECTORS];

static TCALLBACK TVOID mod_destroy(ZMOD *mod);

static TMODAPI TUINT zlib_crc32(ZMOD *zlib, uLong crc, const Bytef *buf, uInt len);
static TMODAPI int zlib_deflateInit (ZMOD *zlib, z_streamp s, int level);
static TMODAPI int zlib_deflate (ZMOD *zlib, z_streamp s, int flush);
static TMODAPI int zlib_deflateEnd (ZMOD *zlib, z_streamp s);
static TMODAPI int zlib_inflateInit (ZMOD *zlib, z_streamp s);
static TMODAPI int zlib_inflate (ZMOD *zlib, z_streamp s, int flush);
static TMODAPI int zlib_inflateEnd (ZMOD *zlib, z_streamp s);
static TMODAPI int zlib_deflateInit2 (ZMOD *zlib, z_streamp s, int level, int method, int windowBits, int memLevel, int strategy);
static TMODAPI int zlib_deflateSetDictionary (ZMOD *zlib, z_streamp s, const Bytef *dictionary, uInt dictLength);
static TMODAPI int zlib_deflateCopy (ZMOD *zlib, z_streamp dest, z_streamp source);
static TMODAPI int zlib_deflateReset (ZMOD *zlib, z_streamp s);
static TMODAPI int zlib_deflateParams (ZMOD *zlib, z_streamp s, int level, int strategy);
static TMODAPI int zlib_inflateInit2 (ZMOD *zlib, z_streamp s, int windowBits);
static TMODAPI int zlib_inflateSetDictionary (ZMOD *zlib, z_streamp s, const Bytef *dictionary, uInt dictLength);
static TMODAPI int zlib_inflateSync (ZMOD *zlib, z_streamp s);
static TMODAPI int zlib_inflateReset (ZMOD *zlib, z_streamp s);

static TMODAPI TINT zlib_compress(ZMOD *zlib, TAPTR src, TAPTR dst, TINT slen, TINT dlen, TTAGITEM *tags);
static TMODAPI TAPTR zlib_getcompressed(ZMOD *zlib, TAPTR src, TINT slen, TTAGITEM *tags);
static TMODAPI TINT zlib_uncompress(ZMOD *zlib, TAPTR src, TAPTR dst, TINT slen, TINT dlen, TTAGITEM *tags);

/*****************************************************************************/
/*
**	tek_init_<modname>
**	module initializations (not instance-specific)
*/

TMODENTRY TUINT 
tek_init_zlib(TAPTR task, ZMOD *mod, TUINT16 version, TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * MOD_NUMVECTORS;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(ZMOD);					/* positive size */

		return 0;
	}

	mod->mmu = TNULL;

	mod->module.tmd_DestroyFunc = (TDFUNC) mod_destroy;
	mod->module.tmd_Version = MOD_VERSION;
	mod->module.tmd_Revision = MOD_REVISION;

	TInitVectors(mod, (TAPTR *) vectors, MOD_NUMVECTORS);

	return TTRUE;
}

static TCALLBACK TVOID
mod_destroy(ZMOD *mod)
{
	TDestroy(mod->mmu);
}

/*****************************************************************************/
/*
**	function table
*/

static const TAPTR vectors[MOD_NUMVECTORS] =
{
	(TAPTR) zlib_crc32,
	(TAPTR) zlib_deflateInit,
	(TAPTR) zlib_deflate,
	(TAPTR) zlib_deflateEnd,
	(TAPTR) zlib_inflateInit,
	(TAPTR) zlib_inflate,
	(TAPTR) zlib_inflateEnd,
	(TAPTR) zlib_deflateInit2,
	(TAPTR) zlib_deflateSetDictionary,
	(TAPTR) zlib_deflateCopy,
	(TAPTR) zlib_deflateReset,
	(TAPTR) zlib_deflateParams,
	(TAPTR) zlib_inflateInit2,
	(TAPTR) zlib_inflateSetDictionary,
	(TAPTR) zlib_inflateSync,
	(TAPTR) zlib_inflateReset,

	(TAPTR) zlib_compress,
	(TAPTR) zlib_uncompress,
	(TAPTR) zlib_getcompressed,
};

/*****************************************************************************/
/*
**	custom memory allocation
*/

voidpf zcalloc(voidpf opaque, unsigned items, unsigned size)
{
	ZMOD *zlib = (ZMOD *) opaque;
	return TExecAlloc(TExecBase, zlib->mmu, (TUINT) (items * size));
}

void zcfree(voidpf opaque, voidpf ptr)
{
	ZMOD *zlib = (ZMOD *) opaque;
	TExecFree(TExecBase, (TAPTR) ptr);
}

static void initalloc(ZMOD *zlib, z_streamp s)
{
	if (s->zalloc == Z_NULL)
	{
		s->zalloc = (alloc_func) zcalloc;
		s->opaque = (voidpf) zlib;
	}

	if (s->zfree == Z_NULL)
	{
		s->zfree = (free_func) zcfree;
	}
}

/*****************************************************************************/
/*
**	regular zlib API
*/

static TMODAPI TUINT
zlib_crc32(ZMOD *zlib, uLong crc, const Bytef *buf, uInt len)
{
	return crc32(crc, buf, len);
}

static TMODAPI int
zlib_deflateInit (ZMOD *zlib, z_streamp s, int level)
{
	initalloc(zlib, s);
	return deflateInit(s, level);
}

static TMODAPI int
zlib_deflate (ZMOD *zlib, z_streamp s, int flush)
{
	return deflate(s, flush);
}

static TMODAPI int
zlib_deflateEnd (ZMOD *zlib, z_streamp s)
{
	return deflateEnd(s);
}

static TMODAPI int
zlib_inflateInit (ZMOD *zlib, z_streamp s)
{
	initalloc(zlib, s);
	return inflateInit(s);
}

static TMODAPI int
zlib_inflate (ZMOD *zlib, z_streamp s, int flush)
{
	return inflate(s, flush);
}

static TMODAPI int
zlib_inflateEnd (ZMOD *zlib, z_streamp s)
{
	return inflateEnd(s);
}

static TMODAPI int
zlib_deflateInit2 (ZMOD *zlib, z_streamp s, int level, int method,
	int windowBits, int memLevel, int strategy)
{
	initalloc(zlib, s);
	return deflateInit2(s, level, method, windowBits, memLevel, strategy);
}

static TMODAPI int
zlib_deflateSetDictionary (ZMOD *zlib, z_streamp s, const Bytef *dictionary,
	uInt dictLength)
{
	return deflateSetDictionary(s, dictionary, dictLength);
}

static TMODAPI int
zlib_deflateCopy (ZMOD *zlib, z_streamp dest, z_streamp source)
{
	return deflateCopy(dest, source);
}

static TMODAPI int zlib_deflateReset (ZMOD *zlib, z_streamp s)
{
	return deflateReset(s);
} 

static TMODAPI int
zlib_deflateParams (ZMOD *zlib, z_streamp s, int level, int strategy)
{
	return deflateParams(s, level, strategy);
}

static TMODAPI int
zlib_inflateInit2 (ZMOD *zlib, z_streamp s, int windowBits)
{
	initalloc(zlib, s);
	return inflateInit2(s, windowBits);
}

static TMODAPI int 
zlib_inflateSetDictionary (ZMOD *zlib, z_streamp s, const Bytef *dictionary,
	uInt dictLength)
{
	return inflateSetDictionary(s, dictionary, dictLength);
}

static TMODAPI int
zlib_inflateSync (ZMOD *zlib, z_streamp s)
{
	return inflateSync(s);
}

static TMODAPI int
zlib_inflateReset (ZMOD *zlib, z_streamp s)
{
	return inflateReset(s);
}

/*****************************************************************************/
/*
**	Utility functions
*/
/*****************************************************************************/
/*
**	SYNOPSIS
**		clen = zlib_compress(zlib, srcbuf, dstbuf, slen, dlen, tags)
**		TINT                 TAPTR TAPTR   TAPTR   TINT  TINT TTAGITEM*
**
**	FUNCTION
**		compress a block of memory. the destination buffer should be
**		0.1% larger than the source buffer, plus an extra of 12 bytes.
**
**	TAGS
**		none defined yet
**
**	RESULTS
**		clen  -1 on error,
**		      0 if nothing to do,
**		      else length of compressed data [bytes]
*/

static TMODAPI TINT 
zlib_compress(ZMOD *zlib, TAPTR src, TAPTR dst, TINT slen, TINT dlen,
	TTAGITEM *tags)
{
	z_stream s;
	int err;
	
	s.next_in = (Bytef*) src;
	s.avail_in = (uInt) slen;
	s.next_out = (Bytef*) dst;
	s.avail_out = (uInt) dlen;

	s.zalloc = (alloc_func) zcalloc;
	s.zfree = (free_func) zcfree;
	s.opaque = (voidpf) zlib;

    err = deflateInit(&s, Z_DEFAULT_COMPRESSION);
    if (err == Z_OK)
    {
		err = deflate(&s, Z_FINISH);
		if (err == Z_STREAM_END)
		{
			err = deflateEnd(&s);
			if (err == Z_OK)
			{
				return (TINT) s.total_out;
			}
		}
		else
		{
			deflateEnd(&s);
			if (err == Z_OK) return 0;
		}
	}
	
	return -1;
}

/*****************************************************************************/
/*
**	SYNOPSIS
**		cdata = zlib_getcompressed(zlib, srcbuf, slen, tags)
**		TAPTR                      TAPTR TAPTR   TINT  TTAGITEM*
**
**	FUNCTION
**		compress a block of memory. returns a compressed chunk of memory,
**		or TNULL for failure. the size of the compressed data can be
**		determined using TGetSize(). the memory must be freed using
**		TFree().
**
**	TAGS
**		none defined yet
**
**	RESULTS
**		cdata - compressed data
**
*/

static TMODAPI TAPTR
zlib_getcompressed(ZMOD *zlib, TAPTR src, TINT slen, TTAGITEM *tags)
{
	if (src && slen)
	{
		TUINT dlen = slen + slen / 1000 + 12;
		TAPTR dst = TExecAlloc(TExecBase, zlib->mmu, dlen);
		if (dst)
		{
			TINT clen = zlib_compress(zlib, src, dst, slen, dlen, TNULL);
			if (clen >= 0)
			{
				TAPTR cdata = TExecRealloc(TExecBase, dst, clen);
				if (cdata) return cdata;
				TExecFree(TExecBase, dst);
			}
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	SYNOPSIS
**		ulen = zlib_uncompress(zlib, srcbuf, dstbuf, slen, dlen, tags)
**		TINT                   TAPTR TAPTR   TAPTR   TINT  TINT TTAGITEM*
**
**	FUNCTION
**		uncompress a block of memory. the destination buffer should be
**		large enough to contain the uncompressed data.
**
**	TAGS
**		none defined yet
**
**	RESULTS
**		ulen   -1 on error,
**		       0 if nothing to do,
**		       else length of uncompressed data [bytes]
*/

static TMODAPI TINT
zlib_uncompress(ZMOD *zlib, TAPTR src, TAPTR dst, TINT slen, TINT dlen,
	TTAGITEM *tags)
{
	z_stream s;
	int err;
	
	s.next_in = (Bytef*) src;
	s.avail_in = (uInt) slen;
	s.next_out = (Bytef*) dst;
	s.avail_out = (uInt) dlen;

	s.zalloc = (alloc_func) zcalloc;
	s.zfree = (free_func) zcfree;
	s.opaque = (voidpf) zlib;

	err = inflateInit(&s);
	if (err == Z_OK)
	{
		err = inflate(&s, Z_FINISH);
		if (err == Z_STREAM_END)
		{
			err = inflateEnd(&s);
			if (err == Z_OK)
			{
				return (TINT) s.total_out;
			}
		}
		else
		{
			inflateEnd(&s);
			if (err == Z_OK) return 0;
		}
	}
	
	return -1;
}

