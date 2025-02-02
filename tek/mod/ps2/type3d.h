
#ifndef _TEK_MOD_PS2_TYPE3D_H
#define _TEK_MOD_PS2_TYPE3D_H

/*
**	$Id: type3d.h,v 1.1 2005/09/18 12:33:39 tmueller Exp $
**	teklib/tek/mod/ps2/type3d.h - PS2 vector and matrix datatypes
**
**	Written by Franciska Schulze <fschulze at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/ps2/memory.h>

typedef	TINT				TVECTOR3I[3]	ALIGN16;
typedef TFLOAT				TVECTOR3F[3] 	ALIGN16;
typedef	TINT				TMATRIX3I[3][3] ALIGN16;
typedef	TFLOAT				TMATRIX3F[3][3] ALIGN16;

typedef	TINT				TVECTOR4I[4] 	ALIGN16;
typedef TFLOAT				TVECTOR4F[4] 	ALIGN16;
typedef	TINT				TMATRIX4I[4][4] ALIGN16;
typedef	TFLOAT				TMATRIX4F[4][4] ALIGN16;

typedef struct _tvector
{
	TINT8 type;      		/* int or float									*/
	TINT8 mcount;    		/* count of elements							*/
	TAPTR vector;    		/* TVECTOR3F/TVECTOR3I/TVECTOR4F/TVECTOR4I...	*/
} TVECTOR;

typedef struct _tmatrix
{
	TINT8 type;      		/* int or float									*/
	TINT8 mcount;    		/* count of elements							*/
	TAPTR matrix;    		/* TMATRIX3F/TMATRIX3I/TMATRIX4F/TMATRIX4I...	*/    
} TMATRIX;

/*****************************************************************************/
/*
**	Revision History
**	$Log: type3d.h,v $
**	Revision 1.1  2005/09/18 12:33:39  tmueller
**	added
**	
**	Revision 1.1.1.1  2005/04/17 16:09:46  tmueller
**	initial
**	
*/

#endif /* _TEK_MOD_PS2_TYPE3D_H */
