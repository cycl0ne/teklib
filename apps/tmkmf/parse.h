
#ifndef _PARSE_H
#define _PARSE_H	1

#include <tek/type.h>

TINT parse(TAPTR userdata, TAPTR *rule, TCALLBACK TINT (*getcharfunc)(TAPTR userdata));

#define P_OKAY	0
#define P_ERROR	1
#define P_SKIP	2


#endif

