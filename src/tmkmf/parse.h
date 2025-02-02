
#ifndef _TMKMF_PARSE_H
#define _TMKMF_PARSE_H

#include <tek/type.h>

TINT parse(TAPTR userdata, const TTAG *rule, TINT (*getcharfunc)(TAPTR userdata));

#define P_OKAY	0
#define P_ERROR	1
#define P_SKIP	2

#endif

