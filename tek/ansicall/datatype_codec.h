#ifndef _TEK_ANSICALL_DATATYPE_CODEC_H
#define _TEK_ANSICALL_DATATYPE_CODEC_H

/*
**	$Id: datatype_codec.h,v 1.2 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/datatype_codec.h - datatype_codec module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define dtcodec_getattrs(datatype_codec) \
	(*(((TMODCALL TTAGITEM *(**)(TAPTR))(datatype_codec))[-1]))(datatype_codec)

#define dtcodec_domethod(datatype_codec,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TTAGITEM *))(datatype_codec))[-2]))(datatype_codec,tags)

#endif /* _TEK_ANSICALL_DATATYPE_CODEC_H */
