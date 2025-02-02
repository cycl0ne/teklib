#ifndef _TEK_ANSICALL_ASTRO_H
#define _TEK_ANSICALL_ASTRO_H

/*
**	$Id: astro.h,v 1.3 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/astro.h - astro module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TAstroGetEaster(astro,year,day,month) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TINT *,TINT *))(astro))[-9]))(astro,year,day,month)

#define TAstroGetFacts(astro,tags1,tags2) \
	(*(((TMODCALL TINT(**)(TAPTR,TTAGITEM *,TTAGITEM *))(astro))[-10]))(astro,tags1,tags2)

#define TAstroGetSunPos(astro,T,ra,dec,di) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDOUBLE,TDOUBLE *,TDOUBLE *,TDOUBLE *))(astro))[-11]))(astro,T,ra,dec,di)

#define TAstroGetObjectRiseSet(astro,t,ra,dec,l,b,p,rt,st) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDOUBLE,TDOUBLE,TDOUBLE,TFLOAT,TFLOAT,TINT,TFLOAT *,TFLOAT *))(astro))[-12]))(astro,t,ra,dec,l,b,p,rt,st)

#define TAstroConvertToHMS(astro,t,h,m,s) \
	(*(((TMODCALL TVOID(**)(TAPTR,TFLOAT,TINT *,TINT *,TINT *))(astro))[-13]))(astro,t,h,m,s)

#define TAstroGetMoonPos(astro,T,ra,dec,di) \
	(*(((TMODCALL TVOID(**)(TAPTR,TDOUBLE,TDOUBLE *,TDOUBLE *,TDOUBLE *))(astro))[-14]))(astro,T,ra,dec,di)

#define TAstroGetFlexEvents(astro,y,tags) \
	(*(((TMODCALL TVOID(**)(TAPTR,TINT,TTAGITEM *))(astro))[-15]))(astro,y,tags)

#define TAstroGetMoonPhases(astro,db,ph) \
	(*(((TMODCALL TVOID(**)(TAPTR,struct TDateBox *,struct TDTMoonPhase *))(astro))[-16]))(astro,db,ph)

#define TAstroNextEclipse(astro,db,ec,mode) \
	(*(((TMODCALL TVOID(**)(TAPTR,struct TDateBox *,struct TDTEclipse *,TINT))(astro))[-17]))(astro,db,ec,mode)

#endif /* _TEK_ANSICALL_ASTRO_H */
