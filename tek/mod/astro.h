#ifndef _TEK_MOD_ASTRO_H
#define _TEK_MOD_ASTRO_H

/*
**	$Id: astro.h,v 1.1.1.1 2006/08/20 22:15:26 tmueller Exp $
**	teklib/tek/mod/astro.h - Astro module definitions
**
**	Written by Frank Pagels <copper at coplabs.org>
**	Additional work by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/time.h>
#include <tek/mod/util.h>

/*****************************************************************************/
/*
**	Forward declarations
*/

/* Astro module base structure: */
struct TAstroBase;

/*****************************************************************************/
/*
**	Weekday definitions
*/

#define TDT_SUNDAY		0
#define TDT_MONDAY		1
#define TDT_TUESDAY		2
#define TDT_WEDNESDAY	3
#define TDT_THURSDAY	4
#define TDT_FRIDAY		5
#define TDT_SATURDAY	6

/*****************************************************************************/
/*
**	Tag items
*/

#define TDATETAGS_		(TTAG_USER + 0x1000)
#define TDT_DateBox		(TDATETAGS_ + 1)
#define TDT_Pos			(TDATETAGS_ + 2)
#define TDT_Place		(TDATETAGS_ + 3)
#define TDT_Sunrise		(TDATETAGS_ + 4)
#define TDT_Sunset		(TDATETAGS_ + 5)
#define TDT_Moonrise	(TDATETAGS_ + 6)
#define TDT_Moonset		(TDATETAGS_ + 7)
#define TDT_Moondist	(TDATETAGS_ + 8)
#define TDT_Sundist		(TDATETAGS_ + 9)
#define TDT_Moonlight	(TDATETAGS_ + 10)
#define TDT_Moonphase	(TDATETAGS_ + 11)
#define TDT_Moonpangle	(TDATETAGS_ + 12)

/* Event Tags */
#define TEVT_Easter		(TTAG_USER + 0x2000)	/* Ostern */
#define TEVT_AshWnd		(TTAG_USER + 0x2001)	/* Aschermittwoch */
#define TEVT_GoodFrd	(TTAG_USER + 0x2002)	/* Karfreitag */
#define TEVT_Ascension	(TTAG_USER + 0x2003)	/* Christi Himmelfahrt */
#define TEVT_Pentecost	(TTAG_USER + 0x2004)	/* Pfingsten */
#define TEVT_CorpChrist	(TTAG_USER + 0x2005)	/* Fronleichnam */
#define TEVT_HerzJesu	(TTAG_USER + 0x2006)	/* Herz-Jesu-Freitag */
#define TEVT_Penance	(TTAG_USER + 0x2007)	/* Buﬂ- und Bettag */
#define TEVT_FirstAdvt	(TTAG_USER + 0x2008)	/* Erster Advent */
#define TEVT_MotherDay	(TTAG_USER + 0x2009)	/* Muttertag */
#define TEVT_Passion	(TTAG_USER + 0x2010)	/* Passionssonntag */
#define TEVT_Palm		(TTAG_USER + 0x2011)	/* Palmsonntag */
#define TEVT_Thanks		(TTAG_USER + 0x2012)	/* Erntedank */
#define TEVT_Laetare	(TTAG_USER + 0x2013)	/* Mid-Lent, Mitte Fastenzeit */
#define TEVT_Trinity	(TTAG_USER + 0x2014)	/* Trinity Sunday / Dreifaltigkeitssonntag */
#define TEVT_DeadSun	(TTAG_USER + 0x2015)	/* Sunday in commemoration of the dead / Totensonntag */
#define TEVT_BeginDST	(TTAG_USER + 0x2016)	/* begin daylight savings time */
#define TEVT_EndDST		(TTAG_USER + 0x2017)	/* end of daylight savings time */


/*****************************************************************************/
/*
**	Constants
*/

#define TDT_SUN			0
#define TDT_MOON		1
#define TDT_PLANET		3

/* moon phases */
#define TDT_MNEW		0
#define TDT_MFIRST		1
#define TDT_MFULL		2
#define TDT_MLAST		3

#define TDT_OK			0
#define TDT_ERROR		1

/*****************************************************************************/
/*
**	Location
*/

struct TDTLocation
{
	TFLOAT tlc_Longitude;
	TFLOAT tlc_Latitude;
	TFLOAT tlc_TZone;
};

/*****************************************************************************/
/*
**	Moon Phases
*/

struct TDTMoonPhase
{
	struct TDateBox new;	/* newmoon */
	struct TDateBox first;	/* first quarter */
	struct TDateBox full;	/* full moon */
	struct TDateBox last;	/* last quarter */
};

struct TDTEclipse
{
	TINT type;	/* Eclipse Type*/
	TINT visible;	/* where visible */
	struct TDateBox event;
};

/* type of eclipse */
#define TDT_ETOTAL			0
#define TDT_ECIRCULAR		1
#define TDT_ECIRCULARTOTAL	2
#define TDT_ENONCENTRAL		3
#define TDT_EPARTIAL		4
#define TDT_ENONE			5

/* eclipse visibility */
#define TDT_NORTH			0
#define TDT_SOUTH			1
#define TDT_EQUATOR			2

/*****************************************************************************/
/*
**	Selected locations
*/

#define TLC_ROSTOCK		0
#define TLC_DARMSTADT	1
#define TLC_GOETTINGEN	2
#define TLC_BERLIN		3
#define TLC_HAMBURG		4
#define TLC_AACHEN		5
#define TLC_DRESDEN		6
#define TLC_FLENSBURG	7
#define TLC_KOELN		8
#define TLC_AMSTERDAM	9
#define TLC_ANKARA		10
#define TLC_KABUL		11
#define TLC_LONDON		12
#define TLC_MADRID		13
#define TLC_MOSCOW		14
#define TLC_NEWYORK		15
#define TLC_NORTHPOLE	16
#define TLC_SYDNEY		17
#define TLC_BAGDAD		18
#define TLC_LOSANGELES	19
#define TLC_ATHEN		20
#define TLC_AUCKLAND	21
#define TLC_ANTWERPEN	22
#define TLC_AUGSBURG	23
#define TLC_AUSTIN		24
#define	TLC_BAKU		25
#define TLC_BANGKOK		26
#define TLC_BARCELONA	27
#define TLC_BEIRUT		28
#define TLC_BELFAST		29
#define TLC_BOMBAY		30
#define TLC_BORDEAUX	31
#define TLC_BRASILIA	32
#define TLC_BUDAPEST	33
#define TLC_SCHWERIN	34

#endif
