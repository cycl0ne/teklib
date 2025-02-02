
/*
**	$Id: astro_mod.c,v 1.5 2006/11/11 14:19:10 tmueller Exp $
**	teklib/src/time/astro_mod.c - Astronomical calculations module
**
**	Written by Frank Pagels <copper at coplabs.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/mod/astro.h>
#include <math.h>

/*****************************************************************************/

#define MOD_VERSION		0
#define MOD_REVISION	8
#define MOD_NUMVECTORS	20

struct TAstroBase
{
	struct TModule mod;
	TAPTR exec;
	TAPTR util;
	TAPTR lock;
	TUINT refcount;
};

#ifndef LOCAL
#define LOCAL
#endif

#ifndef EXPORT
#define EXPORT	TMODAPI
#endif

#define RADS 0.0174532925199433
#define DEGS 57.2957795130823

/* Astronomical Unit, median distance Sun - Earth: */
#define AE 149598000

/*****************************************************************************/
/*
**	Prototypes
*/

static const TMFPTR mod_vectors[MOD_NUMVECTORS];
static THOOKENTRY TTAG mod_dispatch(struct THook *hook, TAPTR obj, TTAG msg);
static struct TAstroBase *mod_open(struct TAstroBase *astro, TTAGITEM *tags);
static void mod_close(struct TAstroBase *astro);

EXPORT void dt_geteaster(struct TAstroBase *astro, TINT y,TINT *d, TINT *m );
EXPORT TINT dt_getastrofacts(struct TAstroBase *astro, TTAGITEM *in, TTAGITEM *out);
EXPORT void dt_getsunpos(struct TAstroBase *astro, TDOUBLE T, TDOUBLE *Ra,
	TDOUBLE *Dec, TDOUBLE *di);
EXPORT void dt_objectriseset(struct TAstroBase *astro, TDOUBLE T, TDOUBLE Ra,
	TDOUBLE Dec, TFLOAT l, TFLOAT b, TINT ob, TFLOAT *rt, TFLOAT *st);
EXPORT void dt_converttohms(struct TAstroBase *astro, TFLOAT t, TINT *h, TINT *m,
	TINT *s);
EXPORT void dt_getmoonpos(struct TAstroBase *astro, TDOUBLE T, TDOUBLE *Ra,
	TDOUBLE *Dec, TDOUBLE *di);
EXPORT void dt_getflexevent(struct TAstroBase *astro, TINT y, TTAGITEM *events);
EXPORT void dt_getphases(struct TAstroBase *astro,struct TDateBox *db,
	struct TDTMoonPhase *phase);
EXPORT void dt_nexteclipse(struct TAstroBase *astro,struct TDateBox *db,
	struct TDTEclipse *eclipse, TINT mode);
EXPORT TBOOL dt_makedate(struct TAstroBase *astro, TDATE *date, TINT d, TINT m,
	TINT y, TTIME *tm);
EXPORT TDOUBLE dt_datetojulian(struct TAstroBase *tmod, TDATE *td);
EXPORT void dt_juliantodmy(struct TAstroBase *mod, TDOUBLE jd, TINT *pd, TINT *pm,
	TINT *py);

/*****************************************************************************/
/*
**	A few selected locations
*/

static const struct TDTLocation places[] = {
	{-12.13f , 54.09f , 1},		/* Rostock */
	{-8.654f , 49.87f , 1},		/* Darmstadt */
	{-9.944f , 51.53f , 1},		/* Goettingen */
	{-13.297f, 52.52f , 1},		/* Berlin */
	{-9.99f  , 53.55f , 1},		/* Hamburg */
	{-6.084f , 50.776f, 1}, 	/* Aachen */
	{-13.735f, 51.054f, 1},		/* Dresden */
	{-9.4366f, 54.785f, 1},		/* Flensburg */
	{-6.97f  , 50.88f , 1},		/* Koeln */
	{-4.9f   , 52.38f , 1},		/* Amsterdam */
	{-32.86f , 39.93f , 2},		/* Ankara */
	{-69.21f , 34.50f , 4.5f},	/* Kabul */
	{0.0833f , 51.516f, 0}, 	/* London */
	{3.683f  , 40.4f  , 1},		/* Madrid */
	{-37.58f , 55.75f  , 3}, 	/* Moscow */
	{-74.00f , 40.75f , -5},	/* New York */
	{0.0f    , 89.98f , 0}, 	/* North Pole */
	{-151.2f ,-33.88f ,10}, 	/* Sydney */
	{-44.42f , 33.35f , 3},		/* Bagdad */
	{118.23f , 34.05f ,-8},		/* Los Angeles */
	{-23.72f , 37.97f , 2},		/* Athen */
	{-174.75f,-36.85f , 12},	/* Auckland */
	{-4.4166f, 51.216f, 1},		/* Antwerpen */
	{-10.883f, 48.366f, 1},		/* Augsburg */
	{ 97.75f , 30.266f,-6},		/* Austin Texas */
	{-49.85f , 40.38f , 4},		/* Baku */
	{-100.52f, 13.75f , 7}, 	/* Bangkok */
	{-2.1833f, 41.38f , 1},		/* Barcelona Spain */
	{-7.6f   , 47.55f , 1},		/* Basel */
	{-35.5f  , 33.88f , 2},		/* Beirut */
	{ 5.933f , 54.6f  , 0}, 	/* Belfast */
	{-72.83f , 18.966f, 5},		/* Bombay */
	{ 0.5667f, 44.833f, 1},		/* Bordeaux */
	{ 47.916f,-15.78f ,-3},		/* Brasilia */
	{-19.08f , 47.5f  , 1},		/* Budapest */
	{-11.38f , 53.63f , 1}		/* Schwerin */
};

/*****************************************************************************/
/*
**	Module init
*/

TMODENTRY TUINT tek_init_astro(struct TTask *task, struct TAstroBase *astro,
	TUINT16 version, TTAGITEM *tags)
{
	if (astro == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * MOD_NUMVECTORS;
		if (version <= MOD_VERSION)
			return sizeof(struct TAstroBase);
		return 0;
	}

	astro->exec = TGetExecBase(astro);
	astro->lock = TExecCreateLock(astro->exec, TNULL);
	if (astro->lock)
	{
		astro->mod.tmd_Version = MOD_VERSION;
		astro->mod.tmd_Revision = MOD_REVISION;
		astro->mod.tmd_Handle.thn_Hook.thk_Entry = mod_dispatch;
		astro->mod.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;
		TInitVectors(&astro->mod, mod_vectors, MOD_NUMVECTORS);
		return TTRUE;
	}

	return 0;
}

static THOOKENTRY TTAG
mod_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct TAstroBase *mod = (struct TAstroBase *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			TDestroy(mod->lock);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) mod_open(mod, obj);
		case TMSG_CLOSEMODULE:
			mod_close(obj);
	}
	return 0;
}

/*****************************************************************************/
/*
**	Instance open/close. We need the time module here.
*/

static struct TAstroBase *
mod_open(struct TAstroBase *astro, TTAGITEM *tags)
{
	struct TAstroBase *result = astro;

	TExecLock(astro->exec, astro->lock);
	if (astro->util)
		astro->refcount++;
	else
	{
		astro->util = TExecOpenModule(astro->exec, "util", 0, TNULL);
		if (astro->util)
			astro->refcount = 1;
		else
			result = TNULL;
	}
	TExecUnlock(astro->exec, astro->lock);

	return result;
}

static void
mod_close(struct TAstroBase *astro)
{
	TExecLock(astro->exec, astro->lock);
	if (astro->util && --astro->refcount == 0)
		TExecCloseModule(astro->exec, astro->util);
	TExecUnlock(astro->exec, astro->lock);
}

/*****************************************************************************/

static const TMFPTR
mod_vectors[MOD_NUMVECTORS] =
{
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) dt_geteaster,
	(TMFPTR) dt_getastrofacts,
	(TMFPTR) dt_getsunpos,
	(TMFPTR) dt_objectriseset,
	(TMFPTR) dt_converttohms,
	(TMFPTR) dt_getmoonpos,
	(TMFPTR) dt_getflexevent,
	(TMFPTR) dt_getphases,
	(TMFPTR) dt_nexteclipse,
	(TMFPTR) dt_makedate,
	(TMFPTR) dt_datetojulian,
	(TMFPTR) dt_juliantodmy,
};

/*****************************************************************************/
/*
**	calculate the easter date
*/

EXPORT void
dt_geteaster(struct TAstroBase *astro, TINT y, TINT *dd, TINT *mm)
{
	TINT a,b,c,d,e,f,g,h,i,k,l,m,n,p;

	if (y < 1583)	/* julian easter */
	{
		a = y%4;
		b = y%7;
		c = y%19;
		d = (19*c+15)%30;
		e = (2*a+4*b-d+34)%7;
		f = (TINT)((d+e+114)/31);
		g = (d+e+114)%31;

		*dd = g+1;
		*mm = f;
	}
	else	/* gregorian easter */
	{
		a = y%19;
		b = (TINT)(y/100);
		c = y%100;
		d = (TINT)b/4;
		e = b%4;
		f = (TINT)((b+8)/25);
		g = (TINT)((b-f+1)/3);
		h = (19*a+b-d-g+15)%30;
		i = (TINT)(c/4);
		k = c%4;
		l = (32+2*e+2*i-h-k)%7;
		m = (TINT)((a+11*h+22*l)/451);
		n = (TINT)((h+l-7*m+114)/31);
		p = (h+l-7*m+114)%31;

		*dd = p+1;
		*mm = n;
	}
}

void dt_subaddeaster(struct TAstroBase *astro, TDATE *easter, TINT diff, struct TDateBox *db, TINT mode)
{
	TDATE tmp;

	tmp = *easter;
	if(mode)
		TAddDate(&tmp, diff, TNULL);
	else
		TSubDate(&tmp, diff, TNULL);

	TUtilUnpackDate(astro->util, &tmp, db, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);
}

EXPORT TBOOL dt_makedate(struct TAstroBase *astro, TDATE *date, TINT d, TINT m,
	TINT y, TTIME *tm)
{
	struct TDateBox db;
	TBOOL success;
	db.tdb_Year = y;
	db.tdb_Month = m;
	db.tdb_Day = d;
	db.tdb_Fields = TDB_YEAR | TDB_MONTH | TDB_DAY;
	if (tm)
	{
		db.tdb_Sec = tm->tdt_Int64 / 1000000;
		db.tdb_USec = tm->tdt_Int64 % 1000000;
		db.tdb_Fields |= TDB_SEC | TDB_USEC;
	}
	success = TUtilPackDate(astro->util, &db, date);
	return success;
}

EXPORT TDOUBLE
dt_datetojulian(struct TAstroBase *tmod, TDATE *td)
{
	TDOUBLE j = td->tdt_Int64 / 86400000000ULL + 2305813.5;
	return j;
}

static void
dt_juliantodate(struct TAstroBase *tmod, TDOUBLE jd, TDATE *td)
{
	td->tdt_Int64 = (jd - 2305813.5) * 86400000000ULL;
}

static TDOUBLE
dt_mytojulian(struct TAstroBase *mod, TINT m, TINT y)
{
	TDOUBLE j = (TDOUBLE) TUtilMYToDay(mod->util, m, y) + 2305813.5;
	return j;
}

EXPORT void
dt_juliantodmy(struct TAstroBase *mod, TDOUBLE jd, TINT *pd, TINT *pm, TINT *py)
{
	TDATE dt;
	dt.tdt_Int64 = (jd - 2305813.5) * 86400000000ULL;
	TUtilDateToDMY(mod->util, &dt, (TUINT *) pd, (TUINT *) pm, (TUINT *) py,
		TNULL);
}

/*****************************************************************************/
/*
**	dt_getflexevent(astro, year, events)
**	calculate some flexible holiday events
*/
EXPORT void
dt_getflexevent(struct TAstroBase *astro, TINT y, TTAGITEM *events)
{
	struct TDateBox *db;
	TDATE	easter;
	TDATE 	tmp;
	TINT	d,m;

	/* first get easterdate */
	dt_geteaster(astro,  y, &d, &m);
	dt_makedate(astro, &easter, d,m,y, TNULL);

	/* get Easter */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_Easter, TNULL)))
		TUtilUnpackDate(astro->util, &easter, db, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);

	/* get Ash Wednesday (Aschermittwoch) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_AshWnd, TNULL)))
		dt_subaddeaster(astro,&easter,46,db,0);

	/* get Good Friday (Karfreitag) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_GoodFrd, TNULL)))
		dt_subaddeaster(astro,&easter,2,db,0);

	/* get Ascension (Christi Himmelfahrt) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_Ascension, TNULL)))
		dt_subaddeaster(astro,&easter,39,db,1);

	/* get Pentecost (Pfingst Sonntag) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_Pentecost, TNULL)))
		dt_subaddeaster(astro,&easter,49,db,1);

	/* get Corp Christi (Fronleichnam) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_CorpChrist, TNULL)))
		dt_subaddeaster(astro,&easter,60,db,1);

	/* get TEVT_HerzJesu (Herz-Jesu-Tag) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_HerzJesu, TNULL)))
		dt_subaddeaster(astro,&easter,68,db,1);

	/* get 1. Advent */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_FirstAdvt, TNULL)))
	{
		TINT d;
		dt_makedate(astro, &tmp, 26,11,y, TNULL);
		d = 7-TUtilGetWeekDay(astro->util, 26,  11, y);
		TAddDate(&tmp, d, TNULL);
		TUtilUnpackDate(astro->util, &tmp, db, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);
	}

	/* get Penance (Bu� und Bettag) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_Penance, TNULL)))
	{
		TINT d;
		dt_makedate(astro, &tmp, 26,11,y, TNULL);
		d = 4+TUtilGetWeekDay(astro->util, 26,  11, y);
		TSubDate(&tmp, d, TNULL);
		TUtilUnpackDate(astro->util, &tmp, db, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);
	}


	/* get Totensonntag */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_DeadSun, TNULL)))
	{
		TINT d;
		dt_makedate(astro, &tmp, 26,11,y, TNULL);
		d = TUtilGetWeekDay(astro->util, 26,  11, y);
		TSubDate(&tmp, d, TNULL);
		TUtilUnpackDate(astro->util, &tmp, db, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);
	}


	/* Mother Day */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_MotherDay, TNULL)))
	{
		TINT d;
		dt_makedate(astro, &tmp, 1,5,y, TNULL);
		d = TUtilGetWeekDay(astro->util, 1,  5, y);
		if(d==0)
			d = 7;
		else
			d = 14-d;

		TAddDate(&tmp, d, TNULL);
		TUtilUnpackDate(astro->util, &tmp, db, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);
	}

	/* get Passion (Passions Sonntag) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_Passion, TNULL)))
		dt_subaddeaster(astro,&easter,14,db,0);

	/* get Palm (Palm Sonntag) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_Palm, TNULL)))
		dt_subaddeaster(astro,&easter,7,db,0);

	/* get Thanks Giving (Ernte Dank) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_Thanks, TNULL)))
	{
		TINT d;
		dt_makedate(astro, &tmp, 1,10,y, TNULL);
		d = TUtilGetWeekDay(astro->util, 1,  10, y);
		if(d!=0)
			d = 7-d;

		TAddDate(&tmp, d, TNULL);
		TUtilUnpackDate(astro->util, &tmp, db, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);
	}

	/* Laetare Sunday */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_Laetare, TNULL)))
		dt_subaddeaster(astro,&easter,21,db,0);

	/* Trinity Sunday  (Dreifaltigkeitssonntag) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_Trinity, TNULL)))
		dt_subaddeaster(astro,&easter,56,db,1);

	/* begin DST (Sommerzeit) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_BeginDST, TNULL)))
	{
		TINT d;
		if(db->tdb_Year<1996)
		{
			dt_makedate(astro, &tmp, 1,5,y, TNULL);
			d = TUtilGetWeekDay(astro->util, 1,  5, y);
			if(d!=0)
				d = 7-d;

			TAddDate(&tmp, d, TNULL);
		}else
		{
			dt_makedate(astro, &tmp, 31,3,y, TNULL);
			d = TUtilGetWeekDay(astro->util, 31,  3, y);
			TSubDate(&tmp, d, TNULL);
		}
		TUtilUnpackDate(astro->util, &tmp, db, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);
	}

	/* end DST (ende Sommerzeit) */
	if((db = (struct TDateBox *) TGetTag(events, TEVT_EndDST, TNULL)))
	{
		TINT d;
		if(y < 1996)
		{
			dt_makedate(astro, &tmp, 30,9,y, TNULL);
			d = TUtilGetWeekDay(astro->util, 30,  9, y);
			TSubDate(&tmp, d, TNULL);
		}else
		{
			dt_makedate(astro, &tmp, 31,10,y, TNULL);
			d = TUtilGetWeekDay(astro->util, 31, 10, y);
			TSubDate(&tmp, d, TNULL);
		}
		TUtilUnpackDate(astro->util, &tmp, db, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);
	}
}


/*****************************************************************************/

static TDOUBLE
myatan2(TDOUBLE y, TDOUBLE x)
{
	TDOUBLE  ta;

	if(x > 0) ta = atan(y/x);
	else
	{
		if (x < 0) ta = atan(y/x)+TPI;
		else
		{
			if (y > 0) ta = TPI/2;
			else
			{
				 if (y < 0) ta = -TPI/2;
				 else
	       			{
	       			ta = 0;
	       			}
			}
		}
	}
	return(ta);
}

static TDOUBLE
fixangle(TDOUBLE a)
{
	while(a>360)
		a -= 360;

	while(a<0)
		a += 360;

	return a;
}

/*****************************************************************************/
/*
**	calculate some astromical facts for a specific place
*/

EXPORT TINT
dt_getastrofacts(struct TAstroBase *astro, TTAGITEM *input, TTAGITEM *output)
{
	TINT d,m,y,*pi;
	TFLOAT *rise,*set,*dist, *mlight, longitude, latitude;
	TFLOAT up,down,tzone,*mposangel;
	TDOUBLE jd, T, Ra, Dec,di,sa,sd,psi;
	struct TDateBox *db;
	struct TDTMoonPhase *phase;

	if(!(db = (struct TDateBox *) TGetTag(input, TDT_DateBox, TNULL)))
		return TDT_ERROR;

	d = db->tdb_Day;
	m = db->tdb_Month;
	y = db->tdb_Year;

	if (!(pi = (TINT *) TGetTag(input, TDT_Place, TNULL )))
	{
		struct TDTLocation *pl;
		if (!(pl = (struct TDTLocation *) TGetTag(input, TDT_Pos, TNULL)))
			return TDT_ERROR;

		longitude = pl->tlc_Longitude;
		latitude = pl->tlc_Latitude;
		tzone = pl->tlc_TZone;
	}
	else
	{
		longitude = places[*pi].tlc_Longitude;
		latitude = places[*pi].tlc_Latitude;
		tzone = places[*pi].tlc_TZone;
	}

	jd = dt_mytojulian(astro, m, y) + d;

	T=(jd-2451545)/36525;	 		/* Zeit in Julianischen Jahrhunderten */

	rise = (TFLOAT *) TGetTag(output, TDT_Sunrise, TNULL);
	set = (TFLOAT *) TGetTag(output, TDT_Sunset, TNULL);
	dist = (TFLOAT *) TGetTag(output, TDT_Sundist, TNULL);

	/* calculate sunstuff */
	dt_getsunpos(astro,T,&Ra,&Dec,&di);
	sa = Ra*RADS;	/* for moonlight */
	sd = Dec*RADS;
	dt_objectriseset(astro,T,Ra,Dec,longitude,latitude,TDT_SUN,&up,&down);
	if(rise)
		*rise = up + tzone;
	if(set)
		*set = down + tzone;
	if(dist)
		*dist = di * AE;

	rise = (TFLOAT *) TGetTag(output, TDT_Moonrise, TNULL);
	set =  (TFLOAT *) TGetTag(output, TDT_Moonset, TNULL);
	dist = (TFLOAT *) TGetTag(output, TDT_Moondist, TNULL);
	mlight = (TFLOAT *) TGetTag(output, TDT_Moonlight, TNULL);
	phase = (struct TDTMoonPhase *) TGetTag(output, TDT_Moonphase, TNULL);
	mposangel = (TFLOAT *) TGetTag(output, TDT_Moonpangle, TNULL);

	/* calculate moonstuff */
	dt_getmoonpos(astro,T,&Ra,&Dec,&di);

	dt_objectriseset(astro,T,Ra,Dec,longitude,latitude,TDT_MOON,&up,&down);
	if(rise)
		*rise = up + tzone;
	if(set)
		*set = down + tzone;
	if(dist)
		*dist = di;

	Ra *= RADS;
	Dec *= RADS;

	if(mlight)	/* illuminated part of the moon */
	{
		/* Meeus side. 344 */
		psi = sin(sd)*sin(Dec)+cos(sd)*cos(Dec)*cos(sa-Ra);
		*mlight = (1-psi)/2;
	}

	if(mposangel) /* position angel of illuminated part */
	{
		TDOUBLE w,w1;
		w = cos(sd)*sin(sa-Ra);
		w1 = (sin(sd)*cos(Dec)-cos(sd)*sin(Dec)*cos(sa-Ra));
		*mposangel = (TFLOAT)fixangle(atan2(w,w1)*DEGS);
	}
	/* get moon phases */
	if(phase)
		dt_getphases(astro,db,phase);

	return TDT_OK;

}

/*****************************************************************************/
/*
**	calculate sun position
*/

EXPORT void
dt_getsunpos(struct TAstroBase *astro, TDOUBLE T,TDOUBLE *Ra, TDOUBLE *Dec,
	TDOUBLE *di)
{
	TDOUBLE L0,MA,MAr,e,C,slon,v,U,e0;


	L0=280.46645+36000.76983*T+0.0003032*(T*T);  /*mittlere L�ge der Sonne */

	MA=357.52910+T*(35999.05030-T*(0.0001559-T*0.00000048));
	MAr = MA*TPI/180;

	e=0.016708617-T*(0.000042037-T*0.0000001236);

	C=(1.914600-T*(0.004817-0.000014*T))*sin(MAr)+(0.019993-0.000101*T)*sin(2*MAr)+0.000290*sin(3*MAr);

	slon=L0+C;		/*wahre L�ge */

	v=MA+C;		/* wahre Anomalie */

	*di=(1.000001018*(1-(e*e)))/(1+e*(cos(v*RADS))); /*radius vektor , erdabstand zur Sonne */

	U=T/100;

	e0=23.4392911-U*(1.30025833-U*(1.55+U*(1999.25-U*(51.38-U*(249.67-U*(39.05+U*(7.12+U*(27.87+U*(5.79+2.45*U)))))))));

	*Ra=myatan2 (cos(e0*RADS)*sin(slon*RADS), cos(slon*RADS))*180/TPI;
	*Dec=asin(sin(e0*RADS)*sin(slon*RADS))*180/TPI;

}

/*****************************************************************************/
/*
**	calculate rise and set of object
*/

EXPORT void
dt_objectriseset(struct TAstroBase *astro, TDOUBLE T, TDOUBLE Ra, TDOUBLE Dec,
	TFLOAT l, TFLOAT b, TINT ob, TFLOAT *rt, TFLOAT *st)
{
	TDOUBLE HA,h0,theta0,MT;
	TFLOAT rtime,stime;

	switch(ob)
	{
		case TDT_SUN: h0 = -0.8333;
				break;
		case TDT_MOON: h0 = 0.125;
				break;
		case TDT_PLANET:	h0 = -0.5667;
				break;
		default: h0 = -0.8333;
	}


	HA=acos((sin(h0*RADS)-sin(b*RADS)*sin(Dec*RADS))/(cos(b*RADS)*cos(Dec*RADS)))*180/TPI;

	while (HA > 180)
		HA -= 180;

	while (HA < 0)
		HA += 180;

	theta0=100.46061837+T*(36000.770053608+T*(0.000387933-T/38710000));

	theta0=fixangle(theta0);

	MT=(Ra+l-theta0)/360;

	rtime=MT-(HA/360);
	stime=MT+(HA/360);

	while (rtime > 1)  {  rtime=rtime-1; }
	while (rtime < 0)  {  rtime=rtime+1; }
	while (stime > 1)  {  stime=stime-1; }
	while (stime < 0)  {  stime=stime+1; }


#if 0
	if(ob==TDT_MOON)
	{
		TDOUBLE Ra1,Ra2,Dec1,Dec2,th1,th2,a,d,H,h,dm;
		TFLOAT n;

		dt_getmoonpos(astro,T-1/36525,&Ra1,&Dec1);
		dt_getmoonpos(astro,T+1/36525,&Ra2,&Dec2);

		th1 = theta0 + 360.985647*rtime;

		n = rtime + 68/86400;

		a = Ra + n/2*(-Ra1+Ra2+n*(Ra2-2*Ra+Ra1));
		d = Dec + n/2*(-Dec1+Dec2+n*(Dec2-2*Dec+Dec1));
		H = th1 - l - a;
		h = asin(sin(b*RADS)*sin(d*RADS)+cos(b*RADS)*cos(d*RADS)*cos(H*RADS));
		dm = (h-h0)/(360*cos(d*RADS)*cos(b*RADS)*sin(H*RADS));
		rtime += dm;
	}
#endif
	rtime=rtime*24;
	stime=stime*24;

	*rt = rtime;
	*st = stime;

}

/*****************************************************************************/
/*
**	convert to h,m,s
*/

EXPORT void
dt_converttohms(struct TAstroBase *astro, TFLOAT t, TINT *h, TINT *m, TINT *s)
{
	*h = (int)t;
	*m = (int)((t - *h)*60);
	*s = (int)((((t - *h) * 60) - *m) * 60);
}

/*****************************************************************************/

static TDOUBLE
mooncalclenghtterm(TDOUBLE Em,TDOUBLE Em2, TDOUBLE Mm, TDOUBLE Dm,TDOUBLE Ms,
	TDOUBLE F)
{
	TDOUBLE sl;

	sl=6288774*sin(Mm)
	   +1274027*sin(2*Dm-Mm)
	   +658314*sin(2*Dm)
	   +213618*sin(2*Mm)
	   -185116*Em*sin(Ms)
	   -114332*sin(2*F)
	   +58793*sin(2*Dm-2*Mm)
	   +57066*Em*sin(2*Dm-Ms-Mm)
	   +53322*sin(2*Dm+Mm)
	   +45758*Em*sin(2*Dm-Ms)
	   -40923*Em*sin(Ms-Mm)
	   -34720*sin(Dm)
	   -30383*Em*sin(Ms+Mm)
	   +15327*sin(2*Dm-2*F)
	   -12528*sin(Mm+2*F)
	   +10980*sin(Mm-2*F)
	   +10675*sin(4*Dm-Mm)
	   +10034*sin(3*Mm)
	   +8548*sin(4*Dm-2*Mm)
	   -7888*Em*sin(2*Dm+Ms-Mm)
	   -6766*Em*sin(2*Dm+Ms)
	   -5163*sin(Dm-Mm)
	   +4987*Em*sin(Dm+Ms)
	   +4036*Em*sin(2*Dm-Ms+Mm)
	   +3994*sin(2*Dm+2*Mm)
	   +3861*sin(4*Dm)
	   +3665*sin(2*Dm-3*Mm)
	   -2689*Em*sin(Ms-2*Mm)
	   -2602*sin(2*Dm-Mm+2*F)
	   +2390*Em*sin(2*Dm-Ms-2*Mm)
	   -2348*sin(Dm+Mm)
	   +2236*Em2*sin(2*Dm-2*Ms)
	   -2120*Em*sin(Ms+2*Mm)
	   -2069*Em2*sin(2*Ms)
	   +2048*Em2*sin(2*Dm-2*Ms-Mm)
	   -1773*sin(2*Dm+Mm-2*F)
	   -1595*sin(2*Dm+2*F)
	   +1215*Em*sin(4*Dm-Ms-Mm)
	   -1110*sin(2*Mm+2*F)
	   -892*sin(3*Dm-Mm)
	   -810*Em*sin(2*Dm+Ms+Mm)
	   +759*Em*sin(4*Dm-Ms-2*Mm)
	   -713*Em2*sin(2*Ms-Mm)
	   -700*Em2*sin(2*Dm+2*Ms-Mm)
	   +691*Em*sin(2*Dm+Ms-2*Mm)
	   +596*Em*sin(2*Dm-Ms-2*F)
	   +549*sin(4*Dm+Mm)
	   +537*sin(4*Mm)
	   +520*Em*sin(4*Dm-Ms)
	   -487*sin(Dm-2*Mm)
	   -399*Em*sin(2*Dm+Ms-2*F)
	   -381*sin(2*Mm-2*F)
	   +351*Em*sin(Dm+Ms+Mm)
	   -340*sin(3*Dm-2*Mm)
	   +330*sin(4*Dm-3*Mm)
	   +327*Em*sin(2*Dm-Ms+2*Mm)
	   -323*Em2*sin(2*Ms+Mm)
	   +299*Em*sin(Dm+Ms-Mm)
	   +294*sin(2*Dm+3*Mm);

	return (sl);

}

/*****************************************************************************/

static TDOUBLE
mooncalcwidthterm(TDOUBLE Em,TDOUBLE Em2, TDOUBLE Mm, TDOUBLE Dm, TDOUBLE Ms,
	TDOUBLE F)
{
	TDOUBLE sb;

	sb=5128122*sin(F)
	   +280602*sin(Mm+F)
	   +277693*sin(Mm-F)
	   +173237*sin(2*Dm-F)
	   +55413*sin(2*Dm-Mm+F)
	   +46271*sin(2*Dm-Mm-F)
	   +32573*sin(2*Dm+F)
	   +17198*sin(2*Mm+F)
	   +9266*sin(2*Dm+Mm-F)
	   +8822*sin(2*Mm-F)
	   +8216*Em*sin(2*Dm-Ms-F)
	   +4324*sin(2*Dm-2*Mm-F)
	   +4200*sin(2*Dm+Mm+F)
	   -3359*Em*sin(2*Dm+Ms-F)
	   +2463*Em*sin(2*Dm-Ms-Mm-F)
	   +2211*Em*sin(2*Dm-Ms+F)
	   +2065*Em*sin(2*Dm-Ms-Mm-F)
	   -1870*Em*sin(Ms-Mm-F)
	   +1828*sin(4*Dm-Mm-F)
	   -1794*Em*sin(Ms+F)
	   -1749*sin(3*F)
	   -1565*Em*sin(Ms-Mm+F)
	   -1491*sin(Dm+F)
	   -1475*Em*sin(Ms+Mm+F)
	   -1410*Em*sin(Ms+Mm-F)
	   -1344*Em*sin(Ms-F)
	   -1335*sin(Dm-F)
	   +1107*sin(3*Mm+F)
	   +1021*sin(4*Dm-F)
	   +833*sin(4*Dm-Mm+F)
	   +777*sin(Mm-3*F)
	   +671*sin(4*Dm-2*Mm+F)
	   +607*sin(2*Dm-3*F)
	   +596*sin(2*Dm+2*Mm-F)
	   +491*Em*sin(2*Dm-Ms+Mm-F)
	   -451*sin(2*Dm-2*Mm+F)
	   +439*sin(3*Mm-F)
	   +422*sin(2*Dm+2*Mm+F)
	   +421*sin(2*Dm-3*Mm-F)
	   -366*Em*sin(2*Dm+Ms-Mm+F)
	   -351*Em*sin(2*Dm+Ms+F)
	   +331*sin(4*Dm+F)
	   +315*Em*sin(2*Dm-Ms+Mm+F)
	   +302*Em2*sin(2*Dm-2*Ms-F)
	   -283*sin(Mm+3*F)
	   -229*Em*sin(2*Dm+Ms+Mm-F)
	   +223*Em*sin(Dm+Ms-F)
	   +223*Em*sin(Dm+Ms+F)
	   -220*Em*sin(Ms-2*Mm-F)
	   -220*Em*sin(2*Dm+Ms-Mm-F)
	   -185*sin(Dm+Mm+F)
	   +181*Em*sin(2*Dm-Ms-2*Mm-F)
	   -177*Em*sin(Ms+2*Mm+F)
	   +176*sin(4*Dm-2*Mm-F)
	   +166*Em*sin(4*Dm-Ms-Mm-F)
	   -164*sin(Dm+Mm-F)
	   +132*sin(4*Dm+Mm-F)
	   -119*sin(Dm-Mm-F)
	   +115*Em*sin(4*Dm-Ms-F)
	   +107*Em2*sin(2*Dm-2*Ms+F);

	return sb;

}

/*****************************************************************************/

static TDOUBLE
mooncalcdistanceterm(TDOUBLE Em,TDOUBLE Em2, TDOUBLE Mm, TDOUBLE Dm,
	TDOUBLE Ms, TDOUBLE F)
{
	TDOUBLE sr;

	sr = -20905355*cos(Mm)
	     -3699111*cos(2*Dm-Mm)
	     -2955968*cos(2*Dm)
	     -569925*cos(2*Mm)
	     +48888*Em*cos(Ms)
	     -3149*cos(2*F)
	     +246158*cos(2*Dm-2*Mm)
	     -152138*Em*cos(2*Dm-Ms-Mm)
	     -170733*cos(Dm+Mm)
	     -204586*Em*cos(2*Dm-Ms)
	     -129620*Em*cos(Ms-Mm)
	     +108743*cos(Dm)
	     +104755*Em*cos(Ms+Mm)
	     +10321*cos(2*Dm-2*F)
	     +79661*cos(Mm-2*F)
	     -34782*cos(4*Dm-Mm)
	     -23210*cos(3*Mm)
	     -21636*cos(4*Dm-2*Mm)
	     +24208*Em*cos(2*Dm+Ms-Mm)
	     +30824*Em*cos(2*Dm+Ms)
	     -8379*cos(Dm-Mm)
	     -16675*Em*cos(Dm+Ms)
	     -12831*Em*cos(2*Dm-Ms+Mm)
	     -10445*cos(2*Dm+2*Mm)
	     -11650*cos(4*Dm)
	     +14404*cos(2*Dm-3*Mm)
	     -7003*Em*cos(Ms-2*Mm)
	     +10056*Em*cos(2*Dm-Ms-2*Mm)
	     +6322*cos(Dm+Mm)
	     -9884*Em2*cos(2*Dm-2*Ms)
	     +5751*Em*cos(Ms+2*Mm)
	     -4950*Em2*cos(2*Dm-2*Ms-Mm)
	     +4130*cos(2*Dm+Mm-2*F)
	     -3958*Em*cos(4*Dm-Ms-Mm)
	     +3258*cos(3*Dm-Mm)
	     +2616*Em*cos(2*Dm+Ms+Mm)
	     -1897*Em*cos(4*Dm-Ms-2*Mm)
	     -2117*Em2*cos(2*Ms-Mm)
	     +2354*Em2*cos(2*Dm+2*Ms-Mm)
	     -1423*cos(4*Dm+Mm)
	     -1117*cos(4*Mm)
	     -1571*Em*cos(4*Dm-Ms)
	     -1739*cos(Dm-2*Mm)
	     -4421*cos(2*Mm-2*F)
	     +1165*Em2*cos(2*Ms+Mm)
	     +8752*cos(2*Dm-Mm-2*F);

	return sr;
}

/*****************************************************************************/
/*
**	calculate moon position
*/

EXPORT void
dt_getmoonpos(struct TAstroBase *astro, TDOUBLE T,TDOUBLE *Ra, TDOUBLE *Dec,
	TDOUBLE *di)
{
	TDOUBLE lambda,beta,omega,psi;
	TDOUBLE a,Lm,Dm,Ms,Mm,F,A1,A2,A3,Em,Em2,sl,sb,sr,U,e,e0,L0;
/*	TDOUBLE l,m,n; */

	a=218.3164591+T*(481267.88134236-T*(0.0013268+T/(538841-T/6519400)));
	Lm=fixangle(a);
	Lm=Lm*TPI/180;

	a=297.8502042+T*(445267.1115168-T*(0.0016300+T/(545868-T/113065000)));
	Dm=fixangle(a);
	Dm=Dm*TPI/180;

	a=357.5291092+T*(35999.0502909-T*(0.0001536+T/24490000));
	Ms=fixangle(a);
	Ms=Ms*TPI/180;

	a=134.9634114+T*(477198.8676313+T*(0.0089970+T*(1/69699-T/14712000)));
	Mm=fixangle(a);
	Mm=Mm*TPI/180;

	a=93.2720993+T*(483202.0175273-T*(0.0034029-T/(3526000+T/863310000)));
	F=fixangle(a);
	F=F*TPI/180;

	A1=119.75+131.849*T;

	A2=53.09+479264.290*T;

	A3=313.45+481266.484*T;

 	Em=1-T*(0.002516-T*0.0000074);

	Em2=Em*Em;

	sl = mooncalclenghtterm(Em,Em2,Mm,Dm,Ms,F);
	sl=sl+3958*sin(A1*TPI/180)+1962*sin(Lm-F)+318*sin(A2*TPI/180);


	/* distance from earth */
	sr = mooncalcdistanceterm(Em,Em2,Mm,Dm,Ms,F);
	sr = 385000.56 + sr/1000;
	*di = sr;

	sb = mooncalcwidthterm(Em,Em2,Mm,Dm,Ms,F);

	sb=sb-2235*sin(Lm)
	     +382*sin(A3*TPI/180)
	     +175*sin((A1*TPI/180)-F)
	     +175*sin((A1*TPI/180)+F)
	     +127*sin(Lm-Mm)
	     -115*sin(Lm+Mm);


	lambda=Lm*180/TPI+sl/1000000;
	beta=sb/1000000;

	U=T/100;
	e0=23.4392911-U*(1.30025833-U*(1.55+U*(1999.25-U*(51.38-U*(249.67-U*(39.05+U*(7.12+U*(27.87+U*(5.79+2.45*U)))))))));

	omega=125.04452-T*(1934.136261+T*(0.0020708+T/450000));
	L0=280.46645+36000.76983*T+0.0003032*(T*T);  /*mittlere L�ge der Sonne */

	omega=omega*TPI/180;
	L0=L0*TPI/180;
	psi= -17.20*sin(omega)-1.32*sin(2*L0)-0.23*sin(2*Lm)+0.21*sin(2*omega);
	psi=psi/3600;
	e=9.20*cos(omega)+0.57*cos(2*L0)+0.10*cos(2*Lm)-0.09*cos(2*omega);
	e=e/3600;
	e=e+e0;

	lambda=lambda+psi;

	*Ra=myatan2((sin(lambda*RADS)*cos(e*RADS))-(tan(beta*RADS)*sin(e*RADS)), cos(lambda*RADS))*180/TPI;
	*Dec=asin(sin(beta*RADS)*cos(e*RADS)+cos(beta*RADS)*sin(e*RADS)*sin(lambda*RADS))*180/TPI;

#if 0
	l = cos(beta*RADS)*cos(lambda*RADS);
	m = 0.9175*cos(beta*RADS)*sin(lambda*RADS)-0.3987*sin(beta*RADS);
	n = 0.3978*cos(beta*RADS)*sin(lambda*RADS)+0.9175*sin(beta*RADS);

	*Ra = myatan2(m,l)*180/TPI;
	*Dec= asin(n)*180/TPI;
#endif

}

TDOUBLE dt_calcphase(TDOUBLE k, TDOUBLE ts, TINT mode)
{
	TDOUBLE JDE,t,m,ms,f,korr,e,o;
	k += 0.25*mode;

	t = k/1236.85;

	JDE = 2451550.09765 + 29.530588853*k
	+t*t*(0.0001337-t*(0.000000150-0.00000000073*t));

	m=(2.5534+29.10535669*k-t*t*(0.0000218+0.00000011*t))*RADS;
	ms=(201.5643+385.81693528*k+t*t*(0.1017438+t*(0.00001239-t*0.000000058)))*RADS;
	f= (160.7108+390.67050274*k-t*t*(0.0016341+t*(0.00000227-t*0.000000011)))*RADS;
	o=(124.7746-1.56375580*k+t*t*(0.0020691+t*0.00000215))*RADS;
	e=(1-ts*(0.002516+ts*0.0000074));

	korr = 0.0;
	switch(mode)
	{
		case TDT_MNEW:
		      korr= -0.40720*sin(ms)
             +0.17241*e*sin(m)
             +0.01608*sin(2*ms)
             +0.01039*sin(2*f)
             +0.00739*e*sin(ms-m)
             -0.00514*e*sin(ms+m)
             +0.00208*e*e*sin(2*m)
             -0.00111*sin(ms-2*f)
             -0.00057*sin(ms+2*f)
             +0.00056*e*sin(2*ms+m)
             -0.00042*sin(3*ms)
             +0.00042*e*sin(m+2*f)
             +0.00038*e*sin(m-2*f)
             -0.00024*e*sin(2*ms-m)
             -0.00017*sin(o)
             -0.00007*sin(ms+2*m)
             +0.00004*sin(2*ms-2*f)
             +0.00004*sin(3*m)
             +0.00003*sin(ms+m-2*f)
             +0.00003*sin(2*ms+2*f)
             -0.00003*sin(ms+m+2*f)
             +0.00003*sin(ms-m+2*f)
             -0.00002*sin(ms-m-2*f)
             -0.00002*sin(3*ms+m)
             +0.00002*sin(4*ms);
		break;
		case TDT_MFIRST:
		case TDT_MLAST:
		     korr= -0.62801*sin(ms)
             +0.17172*e*sin(m)
             -0.01183*e*sin(ms+m)
             +0.00862*sin(2*ms)
             +0.00804*sin(2*f)
             +0.00454*e*sin(ms-m)
             +0.00204*e*e*sin(2*m)
             -0.00180*sin(ms-2*f)
             -0.00070*sin(ms+2*f)
             -0.00040*sin(3*ms)
             -0.00034*e*sin(2*ms-m)
             +0.00032*e*sin(m+2*f)
             +0.00032*e*sin(m-2*f)
             -0.00028*e*e*sin(ms+2*m)
             +0.00027*e*sin(2*ms+m)
             -0.00017*sin(o)
             -0.00005*sin(ms-m-2*f)
             +0.00004*sin(2*ms+2*f)
             -0.00004*sin(ms+m+2*f)
             +0.00004*sin(ms-2*m)
             +0.00003*sin(ms+m-2*f)
             +0.00003*sin(3*m)
             +0.00002*sin(2*ms-2*f)
             +0.00002*sin(ms-m+2*f)
             -0.00002*sin(3*ms+m);
		break;
		case TDT_MFULL:
		      korr= -0.40614*sin(ms)
             +0.17302*e*sin(m)
             +0.01614*sin(2*ms)
             +0.01043*sin(2*f)
             +0.00734*e*sin(ms-m)
             -0.00515*e*sin(ms+m)
             +0.00209*e*e*sin(2*m)
             -0.00111*sin(ms-2*f)
             -0.00057*sin(ms+2*f)
             +0.00056*e*sin(2*ms+m)
             -0.00042*sin(3*ms)
             +0.00042*e*sin(m+2*f)
             +0.00038*e*sin(m-2*f)
             -0.00024*e*sin(2*ms-m)
             -0.00017*sin(o)
             -0.00007*sin(ms+2*m)
             +0.00004*sin(2*ms-2*f)
             +0.00004*sin(3*m)
             +0.00003*sin(ms+m-2*f)
             +0.00003*sin(2*ms+2*f)
             -0.00003*sin(ms+m+2*f)
             +0.00003*sin(ms-m+2*f)
             -0.00002*sin(ms-m-2*f)
             -0.00002*sin(3*ms+m)
             +0.00002*sin(4*ms);
	}
	JDE += korr;
	return JDE;
}

EXPORT void
dt_getphases(struct TAstroBase *astro, struct TDateBox *db,struct TDTMoonPhase *ph)
{
	TDOUBLE JDE,j,m,k,tmp,T;
	TDOUBLE behind,jd;
	TINT D,M,J;
	TDATE mdate;

	m = (TDOUBLE)db->tdb_Month;
	j = (TDOUBLE)db->tdb_Year;
	j = j + (m-1.0)/12;

	tmp = (j-2000.0)*12.3685;
	behind = modf(tmp,&k);

	if(behind > 0.5)	/* next newmoon */
		k += 1.0;

	T = k/1236.85;
	jd = dt_mytojulian(astro, (TINT)m,(TINT)j);
	jd=(jd-2451545)/36525;	 		/* Zeit in Julianischen Jahrhunderten */
	/* newmoon */
	JDE = dt_calcphase(k,jd,0);
	dt_juliantodmy(astro,JDE,&D,&M,&J);
	dt_makedate(astro,&mdate,D,M,J,TNULL);
	TUtilUnpackDate(astro->util, &mdate, &ph->new, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);

	/* first quater */
	JDE = dt_calcphase(k,jd,1);
	dt_juliantodmy(astro,JDE,&D,&M,&J);
	dt_makedate(astro,&mdate,D,M,J,TNULL);
	TUtilUnpackDate(astro->util, &mdate, &ph->first, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);

	/* fullmoon */
	JDE = dt_calcphase(k,jd,2);
	dt_juliantodmy(astro,JDE,&D,&M,&J);
	dt_makedate(astro,&mdate,D,M,J,TNULL);
	TUtilUnpackDate(astro->util, &mdate, &ph->full, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);

	/* last quater */
	JDE = dt_calcphase(k,jd,3);
	dt_juliantodmy(astro,JDE,&D,&M,&J);
	dt_makedate(astro,&mdate,D,M,J,TNULL);
	TUtilUnpackDate(astro->util, &mdate, &ph->last, TDB_WDAY|TDB_DAY|TDB_MONTH|TDB_YEAR);
}

TBOOL
dt_eclipse(struct TAstroBase *astro, TDOUBLE k, TDOUBLE ts, struct TDTEclipse *eclipse, TINT mode)
{
	TDOUBLE jde,t,m,ms,f,f1,a1,e,o,p,q,w,gamma,u;
	TDATE date;

	if(mode==TDT_MOON)
		k += 0.5;

	t = k/1236.85;

	jde = 2451550.09765 + 29.530588853*k
	+t*t*(0.0001337-t*(0.000000150-0.00000000073*t));

	t = k/1236.85;

	m=fixangle((2.5534+29.10535669*k-t*t*(0.0000218+0.00000011*t)))*RADS;
	ms=fixangle((201.5643+385.81693528*k+t*t*(0.1017438+t*(0.00001239-t*0.000000058))))*RADS;
	f= fixangle((160.7108+390.67050274*k-t*t*(0.0016341+t*(0.00000227-t*0.000000011))))*RADS;
	o=fixangle((124.7746-1.56375580*k+t*t*(0.0020691+t*0.00000215)))*RADS;

	e=(1-ts*(0.002516+ts*0.0000074));

	if(fabs(sin(f))>0.36) return TFALSE;	/* no eclipse */

	f1=f-0.02665*sin(o)*RADS;
	a1=(299.77+0.107408*k-0.009173*t*t)*RADS;

	if(mode==TDT_SUN)
	{
      jde=jde - 0.4075*sin(ms)
               + 0.1721* e * sin(m);
	}else
	{
      jde=jde - 0.4065     * sin(ms)
               + 0.1727 * e * sin(m);
	}
	jde=jde   + 0.0161     * sin(2*ms)
               - 0.0097     * sin(2*f1)
               + 0.0073 * e * sin(ms-m)
               - 0.0050 * e * sin(ms+m)
               - 0.0023     * sin(ms-2*f1)
               + 0.0021 * e * sin(2*m)
               + 0.0012     * sin(ms+2*f1)
               + 0.0006 * e * sin(2*ms+m)
               - 0.0004     * sin(3*ms)
               - 0.0003 * e * sin(m+2*f1)
               + 0.0003     * sin(a1)
               - 0.0002 * e * sin(m-2*f1)
               - 0.0002 * e * sin(2*ms-m)
               - 0.0002     * sin(o);

	p=        + 0.2070 * e * sin(m)
               + 0.0024 * e * sin(2*m)
               - 0.0392     * sin(ms)
               + 0.0116     * sin(2*ms)
               - 0.0073 * e * sin(ms+m)
               + 0.0067 * e * sin(ms-m)
               + 0.0118     * sin(2*f1);
    q=        + 5.2207
               - 0.0048 * e * cos(m)
               + 0.0020 * e * cos(2*m)
               - 0.3299     * cos(ms)
               - 0.0060 * e * cos(ms+m)
               + 0.0041 * e * cos(ms-m);

	w=fabs(cos(f1));
    gamma=(p*cos(f1)+q*sin(f1))*(1-0.0048*w);
    u= + 0.0059
        + 0.0046 * e * cos(m)
        - 0.0182     * cos(ms)
        + 0.0004     * cos(2*ms)
        - 0.0005     * cos(m+ms);

	if(gamma < 0.0) eclipse->visible = TDT_SOUTH;
	else eclipse->visible = TDT_NORTH;
	if(fabs(gamma)<0.2)	eclipse->visible = TDT_EQUATOR;

	/* analysis of the eclipse */
	if(mode==TDT_SUN)
	{
		if( fabs(gamma)<0.9972)
		{
			if(u<0) eclipse->type = TDT_ETOTAL;
	        else if(u>0.0047) eclipse->type = TDT_ECIRCULAR;
				else if(u<0.00464*sqrt(1-gamma*gamma)) eclipse->type = TDT_ECIRCULARTOTAL;
					else eclipse->type = TDT_ECIRCULAR;
		} else
		{
			if(fabs(gamma)>(1.5433+u)) return TFALSE;	/* no eclipse */
				else if(fabs(gamma)<(0.9972+fabs(u))) eclipse->type =TDT_ENONCENTRAL;
						else eclipse->type = TDT_EPARTIAL;
		}

	}else
	{
		if((1.0128 - u - fabs(gamma)) / 0.5450 > 0) eclipse->type = TDT_ETOTAL;
			else if( (1.5573 + u - fabs(gamma)) / 0.5450 > 0) eclipse->type = TDT_EPARTIAL;
					else return TFALSE;
	}

	dt_juliantodate(astro, jde, &date);
	TUtilUnpackDate(astro->util, &date, &eclipse->event, TDB_ALL);
	return TTRUE;

}

EXPORT void
dt_nexteclipse(struct TAstroBase *astro,struct TDateBox *db, struct TDTEclipse *eclipse, TINT mode)
{
	TDOUBLE j,m,k,tmp,d;
	TDOUBLE behind,jd;

	d = (TDOUBLE)TUtilDMYToYDay(astro->util,db->tdb_Day,db->tdb_Month,db->tdb_Year);

	m = (TDOUBLE)db->tdb_Month;
	j = (TDOUBLE)db->tdb_Year;
	/*j = j + (m-1.0)/12;*/
	j += d/365.0;

	tmp = (j-2000.0)*12.3685;
	behind = modf(tmp,&k);

	k++;
	jd = dt_mytojulian(astro, (TINT)m,(TINT)j);
	jd=(jd-2451545)/36525;	 		/* Zeit in Julianischen Jahrhunderten */

	/* search a eclipse */
	while(!dt_eclipse(astro, k, jd, eclipse, mode)) k++;

}
