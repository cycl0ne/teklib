#ifndef _TEK_ANSICALL_HASH_H
#define _TEK_ANSICALL_HASH_H

/*
**	$Id: hash.h,v 1.5 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/hash.h - hash module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define THashGet(hash,key,valp) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TTAG,TAPTR))(hash))[-9]))(hash,key,valp)

#define THashPut(hash,key,value) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TTAG,TTAG))(hash))[-10]))(hash,key,value)

#define THashRemove(hash,key) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TTAG))(hash))[-11]))(hash,key)

#define THashValid(hash,reset) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TBOOL))(hash))[-12]))(hash,reset)

#define THashFreeze(hash,freeze) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TBOOL))(hash))[-13]))(hash,freeze)

#endif /* _TEK_ANSICALL_HASH_H */
