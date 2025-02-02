
/*
**	ztest.c
*/

#include <stdio.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/zlib.h>

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TAPTR TExecBase = TGetExecBase(task);
	TAPTR TUtilBase = TExecOpenModule(TExecBase, "util", 0, TNULL);
	TAPTR zlib = TExecOpenModule(TExecBase, "zlib", 0, TNULL);

	if (TUtilBase && zlib)
	{
		TSTRPTR in = "hello world tek rular yo africa hello world tek rular yo africa";
		TINT8 out[100];
		TUINT slen = TUtilStrLen(TUtilBase, in) + 1;
		TAPTR cdata;
		
		printf("compressing %d bytes\n", slen);
		cdata = zlib_getcompressed(zlib, in, slen, TNULL);
		if (cdata)
		{
			TINT clen = TExecGetSize(TExecBase, cdata);
			TINT dlen;
			printf("compressed size is %d bytes\n", clen);
			
			dlen = zlib_uncompress(zlib, cdata, out, clen, sizeof(out), TNULL);
			if (dlen)
			{
				printf("uncompressed size was %d bytes\n", dlen);
				printf("%s\n", out);
			}
			
			TExecFree(TExecBase, cdata);
		}
	}

	TExecCloseModule(TExecBase, zlib);
	TExecCloseModule(TExecBase, TUtilBase);

	printf("bye\n");
}

