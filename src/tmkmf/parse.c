
#include "parse.h"

TINT parse(TAPTR userdata, const TTAG *rp, TINT (*getcharfunc)(TAPTR userdata))
{
	TINT c, c2;
	TINT *charset;
	const TTAG *nextrule, *call;
	TINT (*nextcall)(TAPTR userdata, TINT c);
	TINT error = P_OKAY;

mainloop:	if (!rp) goto exit;

			charset = (TINT *) *rp++;
			nextrule = (const TTAG *) *rp++;

			c = (*getcharfunc)(userdata);
		
			if (charset == TNULL) goto docall;

getchar:	c2 = *charset++;
			if (c2 == 0) goto skipcall;
			if (c2 != c) goto getchar;
		
docall:		call = rp;

calloop:	nextcall = (TINT(*)(TAPTR,TINT)) *call++;

			if (nextcall == TNULL)
			{
				rp = nextrule;
				goto mainloop;
			}
			
			error = (*nextcall)(userdata, c);
			
			switch (error)
			{
				case P_OKAY:
					goto calloop;
				default:
					error = P_ERROR;
				case P_ERROR:
					goto exit;
				case P_SKIP:
					error = P_OKAY;
			}

skipcall:	while (*rp++);
			goto mainloop;

exit:		return error;

}
