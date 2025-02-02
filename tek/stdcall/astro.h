#ifndef _TEK_STDCALL_ASTRO_H
#define _TEK_STDCALL_ASTRO_H

/*
**	$Id: astro.h $
**	teklib/tek/stdcall/astro.h - astro module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TAstroGetEaster(astro,year,day,month) \
	(*(((TMODCALL void(**)(TAPTR,TINT,TINT *,TINT *))(astro))[-9]))(astro,year,day,month)

#define TAstroGetFacts(astro,tags1,tags2) \
	(*(((TMODCALL TINT(**)(TAPTR,TTAGITEM *,TTAGITEM *))(astro))[-10]))(astro,tags1,tags2)

#define TAstroGetSunPos(astro,T,ra,dec,di) \
	(*(((TMODCALL void(**)(TAPTR,TDOUBLE,TDOUBLE *,TDOUBLE *,TDOUBLE *))(astro))[-11]))(astro,T,ra,dec,di)

#define TAstroGetObjectRiseSet(astro,t,ra,dec,l,b,p,rt,st) \
	(*(((TMODCALL void(**)(TAPTR,TDOUBLE,TDOUBLE,TDOUBLE,TFLOAT,TFLOAT,TINT,TFLOAT *,TFLOAT *))(astro))[-12]))(astro,t,ra,dec,l,b,p,rt,st)

#define TAstroConvertToHMS(astro,t,h,m,s) \
	(*(((TMODCALL void(**)(TAPTR,TFLOAT,TINT *,TINT *,TINT *))(astro))[-13]))(astro,t,h,m,s)

#define TAstroGetMoonPos(astro,T,ra,dec,di) \
	(*(((TMODCALL void(**)(TAPTR,TDOUBLE,TDOUBLE *,TDOUBLE *,TDOUBLE *))(astro))[-14]))(astro,T,ra,dec,di)

#define TAstroGetFlexEvents(astro,y,tags) \
	(*(((TMODCALL void(**)(TAPTR,TINT,TTAGITEM *))(astro))[-15]))(astro,y,tags)

#define TAstroGetMoonPhases(astro,db,ph) \
	(*(((TMODCALL void(**)(TAPTR,struct TDateBox *,struct TDTMoonPhase *))(astro))[-16]))(astro,db,ph)

#define TAstroNextEclipse(astro,db,ec,mode) \
	(*(((TMODCALL void(**)(TAPTR,struct TDateBox *,struct TDTEclipse *,TINT))(astro))[-17]))(astro,db,ec,mode)

#define TAstroMakeDate(astro,date,d,m,y,tm) \
	(*(((TMODCALL void(**)(TAPTR,TDATE *,TINT,TINT,TINT,TTIME *))(astro))[-18]))(astro,date,d,m,y,tm)

#define TAstroDateToJulian(astro,td) \
	(*(((TMODCALL TDOUBLE(**)(TAPTR,TDATE *))(astro))[-19]))(astro,td)

#define TAstroJulianToDMY(astro,jd,pd,pm,py) \
	(*(((TMODCALL void(**)(TAPTR,TDOUBLE,TINT *,TINT *,TINT *))(astro))[-20]))(astro,jd,pd,pm,py)

#endif /* _TEK_STDCALL_ASTRO_H */
