
#ifndef _TEK_MOD_HASH_H
#define _TEK_MOD_HASH_H

/*
**	$Id: hash.h,v 1.4 2005/09/13 02:45:09 tmueller Exp $
**	teklib/tek/mod/hash.h - Hash module definitions
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/mod/util.h>

/*****************************************************************************/
/* 
**	tags for TOpenModule()
*/

#define THash_Type			(TTAG_USER + 0x1000)
#define THash_HashFunc		(TTAG_USER + 0x1001)
#define THash_CmpFunc		(TTAG_USER + 0x1002)
#define THash_UserData		(TTAG_USER + 0x1003)

/*****************************************************************************/
/* 
**	hash types
*/

#define THASHTYPE_STRING	0		/* hash of strings (default) */
#define THASHTYPE_PTR		1		/* hash of pointers */
#define THASHTYPE_INT		2		/* hash of integers */
#define THASHTYPE_CUSTOM	3		/* you supply HashFunc and CmpFunc */

/*****************************************************************************/
/* 
**	hash function
**	note: TCMPFUNC is defined in tek/mod/util.h
*/

typedef TCALLBACK TUINT (*THASHFUNC)(TAPTR userdata, TTAG key);

/*****************************************************************************/
/* 
**	hash node
*/

struct THashNode
{
	struct THashNode *thn_Next;		/* Link to next node */
	TTAG thn_Key;					/* Key */
	TTAG thn_Value;					/* Value */
	TUINT thn_HashVal;				/* Hash value */
};

typedef struct THashNode THASHNODE;

/*****************************************************************************/
/*
**	Revision History
**	$Log: hash.h,v $
**	Revision 1.4  2005/09/13 02:45:09  tmueller
**	updated copyright reference
**	
**	Revision 1.3  2005/06/29 09:11:25  tmueller
**	hash keys are now of type TTAG
**	
*/

#endif
