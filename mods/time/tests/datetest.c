
/*
**	$Id: datetest.c,v 1.15 2005/09/13 02:42:26 tmueller Exp $
**	teklib/apps/tests/datetest.c - Date module tests
**
**	Written by Frank Pagels <copper at coplabs.org>
**	Additional work by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/debug.h>
#include <tek/proto/exec.h>
#include <tek/proto/time.h>
#include <tek/proto/astro.h>

struct TModule *TExecBase;
struct TModule *TTimeBase;
struct TTimeRequest *treq;
struct TTimeRequest *treq2;

/*****************************************************************************/

static const TSTRPTR DayNames[7] = 
{ "Sunday", "Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

static const TSTRPTR EclipseTypes[6] = 
{ "Total", "Circular","Circulartotal","Noncentral","Partial","None"};

static const TSTRPTR VisibleTypes[3] = 
{ "North", "South","Equatorial"};

typedef struct
{
	TINT days;
	char name[21];

} adt_month;

static const adt_month months[] = 
{ 
	{31, "    January"}, {28, "    February"}, {31, "     March"},
	{30, "     April"}, {31, "      May"}, {30, "      June"},
	{31, "      July"}, {31, "     August"}, {30, "   September"}, 
	{31, "    October"}, {30, "    November"},  {31, "    December"}
};

/*****************************************************************************/

TVOID test0(TVOID)
{
	TDATE td;
	struct TDateBox db;

	TTimeMakeDate(TTimeBase, &td, 24,10,1648, TNULL);
	TTimeUnpackDate(TTimeBase, &td, &db, TDB_DAY|TDB_MONTH|TDB_YEAR);
	TTimePackDate(TTimeBase, &db, &td);
	TTimeUnpackDate(TTimeBase, &td, &db, TDB_DAY|TDB_MONTH|TDB_YEAR);

	if (db.tdb_Day == 24 &&
		db.tdb_Month == 10 &&
		db.tdb_Year == 1648)
	{
		printf("Pack/unpack test passed.\n");
	}
	else
	{
		printf("Pack/unpack test NOT PASSED!\n");
	}
}

/*****************************************************************************/

TVOID test1(TVOID)
{
	TDATE d1, d2;
	TINT nd;
	TDOUBLE jd;

	TTimeMakeDate(TTimeBase, &d1, 1,1,1978, TNULL);
	jd = TTimeDateToJulian(TTimeBase, &d1);
	printf("1.1.1978 in Julian Days: %f days\n", jd);

	TTimeMakeDate(TTimeBase, &d1, 24,10,1648, TNULL);
	TTimeMakeDate(TTimeBase, &d2, 1,1,1978, TNULL);
	nd = TTimeDiffDate(TTimeBase, &d2, &d1, TNULL);

	printf("Diff between 24.10.1648 and 1.1.1978: %d days\n", nd);

	TTimeMakeDate(TTimeBase, &d1, 1,1,1601, TNULL);
	TTimeMakeDate(TTimeBase, &d2, 1,1,1970, TNULL);
	nd = TTimeDiffDate(TTimeBase, &d2, &d1, TNULL);

	printf("Diff between 1.1.1601 and 1.1.1970: %d days\n", nd);

	jd = TTimeDateToJulian(TTimeBase, &d1);
	printf("1.1.1601 in Julian days: %f\n", jd);
}

/*****************************************************************************/

TVOID test2(TVOID)
{
	TDATE td;
	struct TDateBox db;
	
	TTimeMakeDate(TTimeBase, &td, 11,7,1991, TNULL);
	TTimeAddDate(TTimeBase, &td, 10000, TNULL);
	TTimeUnpackDate(TTimeBase, &td, &db, TDB_DAY|TDB_MONTH|TDB_YEAR);

	printf("11.7.1991 + 10000 Days = %d.%d.%d\n",
		db.tdb_Day, db.tdb_Month, db.tdb_Year);
}

/*****************************************************************************/

TVOID test3(TVOID)
{
	TDATE td;
	struct TDateBox db;

	/* method 1 */	
	TTimeMakeDate(TTimeBase, &td, 12,6,1971, TNULL);
	TTimeUnpackDate(TTimeBase, &td, &db, TDB_WDAY);
	printf("The 12.6.1971 is a %s\n", DayNames[db.tdb_WDay]);

	/* method 2 */	
	printf("The 12.1.1973 is a %s\n", 
		DayNames[TTimeGetWeekDay(TTimeBase, 12,1,1973)]);
}

/*****************************************************************************/

TVOID test4(TVOID)
{
	TDATE td;
	struct TDateBox db;

	/* method 1 */	
	TTimeMakeDate(TTimeBase, &td, 6,3,2002, TNULL);
	TTimeUnpackDate(TTimeBase, &td, &db, TDB_YDAY);
	printf("The 6.3. is day number %d in 2002\n", db.tdb_YDay);

	/* method 2 */	
	printf("The 16.11. is day number %d in 2003\n",
		TTimeDMYToYDay(TTimeBase, 16,11,2003));
}

/*****************************************************************************/

TVOID test5(TVOID)
{
	TDATE td;
	struct TDateBox db;
	TINT m, d;

	/* method 1 */	
	TTimeMakeDate(TTimeBase, &td, 1,1,1988, TNULL);
	TTimeAddDate(TTimeBase, &td, 114, TNULL);
	TTimeUnpackDate(TTimeBase, &td, &db, TDB_DAY|TDB_MONTH);
	printf("Day number 115 in 1988 is the %d.%d.\n", db.tdb_Day, db.tdb_Month);

	/* method 2 */
	TTimeYDayToDM(TTimeBase, 115, 1900, &d, &m);
	printf("Day number 115 in 1900 is the %d.%d.\n", d, m);
}

/*****************************************************************************/

TVOID test6(TVOID)
{
	TDATE td;
	struct TDateBox db;

	/* method 1 */
	TTimeMakeDate(TTimeBase, &td, 6,3,2002, TNULL);
	TTimeUnpackDate(TTimeBase, &td, &db, TDB_WEEK);
	printf("The 6.3.2002 is in week number %d\n", db.tdb_Week);

	/* method 2 */
	printf("The 31.12.2007 is in week number %d\n", 
		TTimeGetWeekNumber(TTimeBase, 31,12,2007));
}

/*****************************************************************************/

TVOID test10(TVOID)
{
	TDATE wait;
	TTIME dt;
	TTIME t0, t1;
	
	dt.ttm_Sec = 2;
	dt.ttm_USec = 345678;

	TTimeGetDate(TTimeBase, treq, &wait, TNULL);
	TTimeAddDate(TTimeBase, &wait, 0, &dt);
	printf("waiting for a date 2.345678 seconds in the future...\n");

	TTimeQueryTime(TTimeBase, treq2, &t0);
	TTimeWaitDate(TTimeBase, treq, &wait, 0);
	TTimeQueryTime(TTimeBase, treq2, &t1);
	
	TTimeSubTime(TTimeBase, &t1, &t0);
	printf("Done. Time measured: %ds, %dus\n", t1.ttm_Sec, t1.ttm_USec);
}

/*****************************************************************************/

TINT printmonthascii(TINT m, TINT y)
{
	TINT day,i,j,d,we;
	
	if(m>12 || m < 1)
		return TDT_ERROR;

	day = TTimeGetWeekDay(TTimeBase, 1, m, y);	/* get Weekday */
	day--;
	if(day<0)
		day = 6;
		
	we = TTimeGetWeekNumber(TTimeBase, 1, m, y);	/* get Weeknumber */
	m--;

	if(months[m].days == 28)
	{
		d = 28;
		if(TTimeIsLeapYear(TTimeBase, y))
			d = 29;
	}
	else
		d = months[m].days;
		
	printf("%s %d\n",months[m].name,y);
	printf("Mo Tu We Th Fr Sa So | We\n");
	printf("=====================|===\n");
	
	i = 0;
	j = 1;
	while(i<day)
	{
		printf("   ");
		i++;
	}
	
	while(j<=d)
	{
		while((i<7)&&(j<=d))
		{
			if(j<10)
				printf(" %d ",j);
			else
				printf("%d ",j);
		
			i++;
			j++;
		}
		
		while(i<7)
		{
			printf("   ");
			i++;
		}

		if(we<10)
			printf("|  %d\n",we);
		else
			printf("| %d\n",we);
		we++;

		i = 0;
	}

	return TDT_OK;
}

/*****************************************************************************/

TVOID test7(TVOID)
{
	TDATE d1, d2;
	TINT y, m, d, nd;
	TINT i = 0;

	TINT from = -1000;
	TINT to = 2000;
	
	printf("Testing Yearday <-> Date conversions between %d and %d\n", from, to);
	
	for (y = from; y <= to; ++y)
	{
		for (m = 1; m <= 12; ++m)
		{
			for (d = 1; d <= 31; ++d)
			{
				if (TTimeIsValidDate(TTimeBase, d,m,y))
				{
					TINT yd = TTimeDMYToYDay(TTimeBase, d,m,y);
					TINT d2, m2;
					
					TTimeYDayToDM(TTimeBase, yd, y, &d2, &m2);
					
					if (d != d2 || m != m2)
					{
						printf("Conversion failed: %d.%d.%d <-> %d.%d.%d\n", d,m,y, d2,m2,y);
						printf("TEST NOT PASSED\n");
						return;
					}
					
					i++;
				}
			}
		}
	}

	printf("Number of days between 1.1.%d and 31.12.%d: %d\n", from, to, i);

	TTimeMakeDate(TTimeBase, &d1, 1,1,from, TNULL);
	TTimeMakeDate(TTimeBase, &d2, 31,12,to, TNULL);
	nd = TTimeDiffDate(TTimeBase, &d2, &d1, TNULL);

	if (nd + 1 != i)
	{
		printf("Daycount differs - Test NOT PASSED\n");
	}
	else
	{
		printf("Test passed.\n");
	}
}

/*****************************************************************************/

TVOID test8(TAPTR astro)
{
	TINT d, m;

	TAstroGetEaster(astro,-666,&d,&m);
	printf("Easter in the year -666 is the %d.%d.\n",d,m);
	TAstroGetEaster(astro,33333,&d,&m);
	printf("Easter in the year 33333 is the %d.%d.\n",d,m);
}

/*****************************************************************************/

TVOID test9(TAPTR astro, TDATE *when)
{
	TTAGITEM itags[4];
	TTAGITEM otags[7];
	struct TDateBox db;
	TFLOAT sr, ss, di, light,mpangle;
	TINT id, m, y, pla;
	struct TDTMoonPhase moonphase;
	
	/* set day and place of interest */
	TTimeUnpackDate(TTimeBase, when, &db, TDB_DAY|TDB_MONTH|TDB_YEAR);

	pla = TLC_ROSTOCK;
	
	itags[0].tti_Tag = TDT_DateBox;
	itags[0].tti_Value = (TTAG) &db;
	itags[1].tti_Tag = TDT_Place;
	itags[1].tti_Value = (TTAG) &pla;
	itags[2].tti_Tag = TTAG_DONE;

	/*
	**	use a dt_place item to use a custom position definition 
	**	TSetTag(itags[1], TDT_Pos, &pl);
	*/

	otags[0].tti_Tag = TDT_Moonrise;
	otags[0].tti_Value = (TTAG) &sr;
	otags[1].tti_Tag = TDT_Moonset;
	otags[1].tti_Value = (TTAG) &ss;
	otags[2].tti_Tag = TDT_Moondist;
	otags[2].tti_Value = (TTAG) &di;
	otags[3].tti_Tag = TDT_Moonlight;
	otags[3].tti_Value = (TTAG) &light;
	otags[4].tti_Tag = TDT_Moonphase;
	otags[4].tti_Value = (TTAG) &moonphase;
	otags[5].tti_Tag = TDT_Moonpangle;
	otags[5].tti_Value = (TTAG) &mpangle;
	otags[6].tti_Tag = TTAG_DONE;

	TAstroGetFacts(astro, itags, otags);

	TAstroConvertToHMS(astro,sr,&id,&m,&y);
	printf("Sun and Moon rise and set for Rostock (%d.%d.%d):\n", 
		db.tdb_Day, db.tdb_Month, db.tdb_Year);

	printf("Moon rise : %d:%d:%d\n",id,m,y);
	TAstroConvertToHMS(astro,ss,&id,&m,&y);

	printf("Moon set  : %d:%d:%d\n",id,m,y);
	printf("Moon distance from earth : %f km\n",di);
	printf("Illuminated part of the Moon : %f \n",light);
	printf("Pos Angel of illumitated Part : %f\n",mpangle);
	printf("New Moon for %d.%d: %d.%d.%d\n",db.tdb_Month,db.tdb_Year,moonphase.new.tdb_Day,
			moonphase.new.tdb_Month, moonphase.new.tdb_Year);
	printf("First Quarter: %d.%d.%d\n",moonphase.first.tdb_Day,
			moonphase.first.tdb_Month, moonphase.first.tdb_Year);
	printf("Full Moon: %d.%d.%d\n",moonphase.full.tdb_Day,
			moonphase.full.tdb_Month, moonphase.full.tdb_Year);
	printf("Last Quarter: %d.%d.%d\n",moonphase.last.tdb_Day,
			moonphase.last.tdb_Month, moonphase.last.tdb_Year);
	
	otags[0].tti_Tag = TDT_Sunrise;
	otags[0].tti_Value = (TTAG) &sr;
	otags[1].tti_Tag = TDT_Sunset;
	otags[1].tti_Value = (TTAG) &ss;
	otags[2].tti_Tag = TDT_Sundist;
	otags[2].tti_Value = (TTAG) &di;
	otags[3].tti_Tag = TTAG_DONE;

	TAstroGetFacts(astro,itags,otags);
	TAstroConvertToHMS(astro,sr,&id,&m,&y);
	printf("Sun rise : %d:%d:%d\n",id,m,y);
	TAstroConvertToHMS(astro,ss,&id,&m,&y);
	printf("Sun set  : %d:%d:%d\n",id,m,y);
	printf("Sun distance from earth : %f km\n\n",di);
}

/*****************************************************************************/
TVOID test11(TAPTR astro, TDATE *when)
{
	TTAGITEM tags[19];
	struct TDateBox db;
	struct TDateBox dt[18];
	
	TTimeUnpackDate(TTimeBase, when, &db, TDB_YEAR);

	printf("Some flexible holidays for %d:\n",db.tdb_Year);
	tags[0].tti_Tag = TEVT_Easter;
	tags[0].tti_Value = (TTAG) &dt[0];
	tags[1].tti_Tag = TEVT_AshWnd;
	tags[1].tti_Value = (TTAG) &dt[1];
	tags[2].tti_Tag = TEVT_GoodFrd;
	tags[2].tti_Value = (TTAG) &dt[2];
	tags[3].tti_Tag = TEVT_Ascension;
	tags[3].tti_Value = (TTAG) &dt[3];
	tags[4].tti_Tag = TEVT_Pentecost;
	tags[4].tti_Value = (TTAG) &dt[4];
	tags[5].tti_Tag = TEVT_CorpChrist;
	tags[5].tti_Value = (TTAG) &dt[5];
	tags[6].tti_Tag = TEVT_HerzJesu;
	tags[6].tti_Value = (TTAG) &dt[6];
	tags[7].tti_Tag = TEVT_Penance;
	tags[7].tti_Value = (TTAG) &dt[7];
	tags[8].tti_Tag = TEVT_FirstAdvt;
	tags[8].tti_Value = (TTAG) &dt[8];
	tags[9].tti_Tag = TEVT_MotherDay;
	tags[9].tti_Value = (TTAG) &dt[9];
	tags[10].tti_Tag = TEVT_Passion;
	tags[10].tti_Value = (TTAG) &dt[10];
	tags[11].tti_Tag = TEVT_Palm;
	tags[11].tti_Value = (TTAG) &dt[11];
	tags[12].tti_Tag = TEVT_Thanks;
	tags[12].tti_Value = (TTAG) &dt[12];
	tags[13].tti_Tag = TEVT_Laetare;
	tags[13].tti_Value = (TTAG) &dt[13];
	tags[14].tti_Tag = TEVT_Trinity;
	tags[14].tti_Value = (TTAG) &dt[14];
	tags[15].tti_Tag = TEVT_DeadSun;
	tags[15].tti_Value = (TTAG) &dt[15];
	tags[16].tti_Tag = TEVT_BeginDST;
	tags[16].tti_Value = (TTAG) &dt[16];
	tags[17].tti_Tag = TEVT_EndDST;
	tags[17].tti_Value = (TTAG) &dt[17];
	tags[18].tti_Tag = TTAG_DONE;
	
	TAstroGetFlexEvents(astro, db.tdb_Year, tags);	

	printf("Easter is %d.%d.%d \n",dt[0].tdb_Day, dt[0].tdb_Month,dt[0].tdb_Year);
	printf("Ash Wednesday (Aschermittwoch) is %d.%d.%d\n",
			dt[1].tdb_Day, dt[1].tdb_Month,dt[1].tdb_Year);
	printf("Laetare Sunday (Mid-Lent) is %d.%d.%d\n",
			dt[13].tdb_Day, dt[13].tdb_Month,dt[13].tdb_Year);
	printf("Good Friday (Karfreitag) is %d.%d.%d\n",
			dt[2].tdb_Day, dt[2].tdb_Month,dt[2].tdb_Year);
	printf("Ascension (Christi Himmelfahrt) is %s, %d.%d.%d\n",
			DayNames[dt[3].tdb_WDay],dt[3].tdb_Day, dt[3].tdb_Month,dt[3].tdb_Year);
	printf("Pentecost (Pfingst Sonntag) is %d.%d.%d\n",
			dt[4].tdb_Day, dt[4].tdb_Month,dt[4].tdb_Year);
	printf("Trinity Sunday (Dreifaltigkeitssonntag) is %d.%d.%d\n",
			dt[14].tdb_Day, dt[14].tdb_Month,dt[14].tdb_Year);
	printf("Corp Christi (Fronleichnam) is %s, %d.%d.%d\n",
			DayNames[dt[5].tdb_WDay],dt[5].tdb_Day, dt[5].tdb_Month,dt[5].tdb_Year);
	printf("Herz-Jesu-Tag is %s, %d.%d.%d\n",
			DayNames[dt[6].tdb_WDay],dt[6].tdb_Day, dt[6].tdb_Month,dt[6].tdb_Year);
	printf("Penance (Buﬂ- und Bettag) is %s, %d.%d.%d\n",
			DayNames[dt[7].tdb_WDay],dt[7].tdb_Day, dt[7].tdb_Month,dt[7].tdb_Year);
	printf("1. Advent is %d.%d.%d\n",dt[8].tdb_Day, dt[8].tdb_Month,dt[8].tdb_Year);
	printf("Mother Day is %s, %d.%d.%d\n",
			DayNames[dt[9].tdb_WDay],dt[9].tdb_Day, dt[9].tdb_Month,dt[9].tdb_Year);
	printf("Passions Sunday is %d.%d.%d\n",dt[10].tdb_Day, dt[10].tdb_Month,dt[10].tdb_Year);
	printf("Palm Sunday is %d.%d.%d\n",dt[11].tdb_Day, dt[11].tdb_Month,dt[11].tdb_Year);
	printf("Erntedank is %d.%d.%d\n",dt[12].tdb_Day, dt[12].tdb_Month,dt[12].tdb_Year);
	printf("Sunday in commemoration of the dead (Totensonntag) is %d.%d.%d\n",
			dt[15].tdb_Day,dt[15].tdb_Month,dt[15].tdb_Year);
	printf("Begin Daylight saving time is %d.%d.%d\n",dt[16].tdb_Day, dt[16].tdb_Month,dt[16].tdb_Year);
	printf("End Daylight saving time is %d.%d.%d\n",dt[17].tdb_Day, dt[17].tdb_Month,dt[17].tdb_Year);

}
TVOID test12(TAPTR astro, TDATE *when)
{
	struct TDTEclipse e;
	struct TDateBox db;	
	TINT y;
	
	TTimeUnpackDate(TTimeBase, when, &db, TDB_YEAR);
	y = db.tdb_Year;
	
	/* calc sun eclipses for a year */
	TTimeMakeDate(TTimeBase, when, 15,12,y-1, TNULL); /* to catch the first eclipse */
	TTimeUnpackDate(TTimeBase, when, &db, TDB_YEAR|TDB_MONTH|TDB_DAY);
	printf("\nSun Eclipses for %d:  \n",y);
	do{
		TAstroNextEclipse(astro,&db,&e,TDT_SUN);
		if(e.event.tdb_Year == y)
			printf("%d.%d.%d  Max(UT): %d:%d Type: %s  Visible: %s\n",
			e.event.tdb_Day,e.event.tdb_Month,e.event.tdb_Year,
			e.event.tdb_Hour,e.event.tdb_Minute,
			EclipseTypes[e.type],VisibleTypes[e.visible]);
		TTimeMakeDate(TTimeBase, when, e.event.tdb_Day,e.event.tdb_Month,e.event.tdb_Year, TNULL); 
		TTimeUnpackDate(TTimeBase, when, &db, TDB_YEAR|TDB_MONTH|TDB_DAY);
	}while(e.event.tdb_Year<=y);
	
	/* calc mon eclipses for a year */
	TTimeMakeDate(TTimeBase, when, 15,12,y-1, TNULL); /* to catch the first eclipse */
	TTimeUnpackDate(TTimeBase, when, &db, TDB_YEAR|TDB_MONTH|TDB_DAY);
	printf("Moon Eclipses for %d:  \n",y);
	do{
		TAstroNextEclipse(astro,&db,&e,TDT_MOON);
		if(e.event.tdb_Year == y)
			printf("%d.%d.%d  Max(UT): %d:%d Type: %s  \n",
			e.event.tdb_Day,e.event.tdb_Month,e.event.tdb_Year,
			e.event.tdb_Hour,e.event.tdb_Minute,
			EclipseTypes[e.type]);
		TTimeMakeDate(TTimeBase, when, e.event.tdb_Day,e.event.tdb_Month,e.event.tdb_Year, TNULL); 
		TTimeUnpackDate(TTimeBase, when, &db, TDB_YEAR|TDB_MONTH|TDB_DAY);
	}while(e.event.tdb_Year<=y);

}
/*****************************************************************************/

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TTimeBase = TExecOpenModule(TExecBase, "time", 0, TNULL);
	if (TTimeBase)
	{
		treq = TTimeAllocTimeRequest(TTimeBase, TNULL);
		treq2 = TTimeAllocTimeRequest(TTimeBase, TNULL);
		if (treq && treq2)
		{
			TDATE now;
			TAPTR astro;
			struct TDateBox dbox;
			TINT tzsec;
	
			printf("-------------- time tests --------------\n\n");
			test10();

			printf("-------------- date tests --------------\n\n");
			test0();
			test1();
			test2();
			test3();
			test4();
			test5();
			test6();
			test7();
	
			printf("\n------------- current date -------------\n\n");
			
			TTimeGetDate(TTimeBase, treq, &now, &tzsec);
			TTimeUnpackDate(TTimeBase, &now, &dbox, TDB_ALL);
			printf("Universal date: %d.%d.%d - time: %d:%d:%d\n",
				dbox.tdb_Day, dbox.tdb_Month, dbox.tdb_Year,
				dbox.tdb_Hour, dbox.tdb_Minute, dbox.tdb_Sec);
				
			printf("We are seconds west of GMT: %d\n", tzsec);
	
			TTimeGetDate(TTimeBase, treq, &now, TNULL);
			TTimeUnpackDate(TTimeBase, &now, &dbox, TDB_ALL);
			printf("Local date: %d.%d.%d - time: %d:%d:%d\n\n",
				dbox.tdb_Day, dbox.tdb_Month, dbox.tdb_Year,
				dbox.tdb_Hour, dbox.tdb_Minute, dbox.tdb_Sec);
			
			printmonthascii(dbox.tdb_Month, dbox.tdb_Year);
	
			printf("\n");
	
			astro = TExecOpenModule(TExecBase, "astro", 0, TNULL);
			if (astro)
			{
				printf("-------------- astro tests -------------\n\n");
	
				test8(astro);
				test9(astro, &now);
				test11(astro, &now);
				test12(astro, &now);
	
				TExecCloseModule(TExecBase, astro);
			}
			else printf("*** Could not open astro module.\n");
		}
		else printf("*** could not open time request\n");

		if (treq)  TTimeFreeTimeRequest(TTimeBase, treq);
		if (treq2) TTimeFreeTimeRequest(TTimeBase, treq2);

		TExecCloseModule(TExecBase, TTimeBase);
	}
	else printf("*** Could not open date module.\n");
}
