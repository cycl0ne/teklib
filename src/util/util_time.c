
#include <tek/inline/util.h>
#include "util_mod.h"

static const TUINT8 util_mdays[] =
{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*****************************************************************************/
/*
**	isleap = util_isleapyear(date, y)
**	Check if year is a leap year
*/

EXPORT TBOOL util_isleapyear(struct TUtilBase *TUtilBase, TUINT y)
{
	if (y & 3) return TFALSE;
	if (y < 1582) return TTRUE;
	if (y % 100) return TTRUE;
	if (y % 400) return TFALSE;
	return TTRUE;
}

/*****************************************************************************/
/*
**	valid = util_isvaliddate(date, d, m, y)
**	Check if a date is valid
*/

EXPORT TBOOL util_isvaliddate(struct TUtilBase *TUtilBase, TUINT d, TUINT m,
	TUINT y)
{
	if (m == 2 && d == 29)
		return TIsLeapYear(y);
	return (m >= 1 && m <= 12 && d >= 1 && d <= util_mdays[m - 1]);
}

/*****************************************************************************/
/*
**	util_ydaytodm(date, yday, year, pd, pm)
**	Calculate a day and month from a yearday and year
*/

EXPORT TBOOL util_ydaytodm(struct TUtilBase *TUtilBase, TUINT n, TUINT y,
	TUINT *pd, TUINT *pm)
{
	TUINT m;
	TUINT ndt = 0;
	TUINT odt = 0;
	if (n < 1)
		return TFALSE;
	for (m = 1; m <= 12; ++m)
	{
		TINT md = util_mdays[m - 1];
		if (m == 2 && TIsLeapYear(y))
			md++;
		odt = ndt;
		ndt += md;
		if (n <= ndt)
		{
			if (pd) *pd = n - odt;
			if (pm) *pm = m;
			return TTRUE;
		}
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	yday = util_dmytoyday(date, d, m, y)
**	Calculate the yearday from a date
*/

EXPORT TUINT util_dmytoyday(struct TUtilBase *TUtilBase, TUINT d, TUINT m,
	TUINT y)
{
	TINT k, n;
	k = 2 - TIsLeapYear(y);
	k *= (m + 9) / 12;
	n = 275 * m / 9;
	n += d - 30 - k;
	return n;
}

/*****************************************************************************/
/*
**	day = util_mytoday(m, y)
**	Convert month and year to days since 1.1.1601
*/

EXPORT TUINT util_mytoday(struct TUtilBase *TUtilBase, TUINT m, TUINT y)
{
	TINT a = (14 - m) / 12;
	TINT d;
	y = y + 4800 - a;
	m = m + 12 * a - 3;
	d = (153 * m + 2) / 5;
	d += 365 * y;
	d += y / 4;
	d -= y / 100;
	d += y / 400;
	d -= 2337859;	/* shift to 1. 1. 1601 */
	return d;
}

/*****************************************************************************/
/*
**	util_datetodmy(date, pD, pM, pY, pT)
*/

EXPORT void util_datetodmy(struct TUtilBase *TUtilBase, TDATE *td, TUINT *pD,
	TUINT *pM, TUINT *pY, TTIME *pT)
{
	TUINT64 J = td->tdt_Int64;
	TUINT j = J / 86400000000ULL + 2337858;
	TUINT g = j / 146097;
	TUINT dg = j % 146097;
	TUINT c = (dg / 36524 + 1) * 3 / 4;
	TUINT dc = dg - c * 36524;
	TUINT b = dc / 1461;
	TUINT db = dc % 1461;
	TUINT a = (db / 365 + 1) * 3 / 4;
	TUINT da = db - a * 365;
	TUINT y = g * 400 + c * 100 + b * 4 + a;
	TUINT m = (da * 5 + 308) / 153 - 2;
	TUINT d = da - (m + 4) * 153 / 5 + 122;
	TUINT Y = y - 4800 + (m + 2) / 12;
	TUINT M = (m + 2) % 12 + 1;
	TUINT D = d + 1;

	if (!TIsValidDate(D, M, Y))
	{
		D = 1;
		if (++M > 12)
		{
			M = 1;
			Y++;
		}
	}

	if (pD) *pD = D;
	if (pM) *pM = M;
	if (pY) *pY = Y;
	if (pT) pT->tdt_Int64 = J % 86400000000ULL;
}

/*****************************************************************************/
/*
**	d = util_getweekday(d, m, y)
*/

EXPORT TUINT util_getweekday(struct TUtilBase *TUtilBase, TUINT d, TUINT m,
	TUINT y)
{
	if (m < 3)
	{
		m += 12;
		y--;
	}

	return (d
		+ 2 * m + 6 * (m + 1) / 10
		+ y
		+ y / 4
		- y / 100
		+ y / 400
		+ 1) % 7;
}

/*****************************************************************************/
/*
**	week = util_getweeknumber(date, d, m, y)
**	Calculate a date's week number
*/

EXPORT TUINT util_getweeknumber(struct TUtilBase *TUtilBase, TUINT d, TUINT m,
	TUINT y)
{
	TINT J, Jb, dd;
	TUINT w;

	J = TMYToDay(m, y) + d;
	Jb = TMYToDay(1, y) + 1;
	dd = (Jb + 3) % 7;
	w = (J + dd - Jb + 4) / 7;

	if (((w >= 1 && w <= 52) || (w == 53 && d == 6)) ||
		(w == 53 && d == 6 && TIsLeapYear(y)))
		return w;

	if (w == 53)
		return 1;	/* first week of next year */

	if (w == 0)
	{
		Jb = TMYToDay(1, y - 1) + 1;
		dd = (Jb + 3) % 7;
		w = (J + dd - Jb + 4) / 7;
	}

	return w;
}

/*****************************************************************************/
/*
**	valid = util_packdate(date, datebox, td)
**	pack a datebox structure to a TDATE
*/

EXPORT TBOOL util_packdate(struct TUtilBase *TUtilBase, struct TDateBox *db,
	TDATE *td)
{
	TBOOL success = TFALSE;
	if (db)
	{
		TUINT16 f = db->tdb_Fields;
		TINT y = db->tdb_Year;
		TUINT m = 0, d = 0;

		if ((f & (TDB_YEAR | TDB_MONTH | TDB_DAY)) ==
			(TDB_YEAR | TDB_MONTH | TDB_DAY))
		{
			d = db->tdb_Day;
			m = db->tdb_Month;
			success = TIsValidDate(d, m, y);
		}
		else if ((f & (TDB_YEAR|TDB_YDAY)) == (TDB_YEAR|TDB_YDAY))
		{
			d = db->tdb_YDay;
			success = TYDayToDM(d, y, &d, &m);
		}

		if (success)
		{
			TUINT64 x = TMYToDay(m, y) + d;
			x *= 86400000000ULL;
			if (f & TDB_HOUR)
				x += (TUINT64) db->tdb_Hour * 3600000000UL;
			if (f & TDB_MINUTE)
				x += db->tdb_Minute * 60000000;
			if (f & TDB_SEC)
				x += db->tdb_Sec * 1000000;
			if (f & TDB_USEC)
				x += db->tdb_USec;
			if (td)
				td->tdt_Int64 = x;
		}
	}
	return success;
}

/*****************************************************************************/
/*
**	util_unpackdate(date, datebox, tdate, fields)
**	unpack a TDATE to a datebox structure
*/

EXPORT void util_unpackdate(struct TUtilBase *TUtilBase, TDATE *td,
	struct TDateBox *db, TUINT16 rf)
{
	if (td && db)
	{
		TUINT16 wf = TDB_YEAR|TDB_MONTH|TDB_DAY;
		TUINT d, m;
		TUINT64 df;
		TTIME dt;

		TDateToDMY(td, &d, &m, &db->tdb_Year, &dt);
		db->tdb_Month = m;
		db->tdb_Day = d;
		df = dt.tdt_Int64;

		if (rf & (TDB_HOUR | TDB_MINUTE | TDB_SEC | TDB_USEC))
		{
			db->tdb_Hour = df / 3600000000UL;
			df %= 3600000000UL;
			wf |= TDB_HOUR;
			if (rf & (TDB_MINUTE | TDB_SEC | TDB_USEC))
			{
				db->tdb_Minute = df / 60000000;
				df %= 60000000;
				wf |= TDB_MINUTE;
				if (rf & (TDB_SEC | TDB_USEC))
				{
					db->tdb_Sec = df / 1000000;
					df %= 1000000;
					wf |= TDB_SEC;
					if (rf & TDB_USEC)
					{
						db->tdb_USec = df;
						wf |= TDB_USEC;
					}
				}
			}
		}

		if (rf & TDB_WEEK)
			db->tdb_Week = TGetWeekNumber(d, m, db->tdb_Year);

		if (rf & TDB_YDAY)
			db->tdb_YDay = TDMYToYDay(d, m, db->tdb_Year);

		if (rf & TDB_WDAY)
			db->tdb_WDay = TGetWeekDay(d, m, db->tdb_Year);

		db->tdb_Fields = wf | rf;
	}
}
