
#ifndef _TEK_MOD_DTCODEC_H
#define	_TEK_MOD_DTCODEC_H

/*
**	definitions for the datatype module
*/

#include <tek/exec.h>

/*	Tags for mod_open 
	----------------- */

#define TDCTAG_FILEHANDLE		(TTAG_USER+1)		/* file handle of open file to READ, must be TNULL if WRITE is TTRUE */
#define TDCTAG_WRITE			(TTAG_USER+2)		/* TBOOL indicating if datatype opened for WRITE */

#endif

