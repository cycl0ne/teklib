
/*
**	teklib/src/util/tests/timetest.c
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

TAPTR TExecBase;
TAPTR TUtilBase;

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		TDATE dt;
		struct TDateBox db;
		TUINT D, M, Y;

		TGetLocalDate(&dt);

		TUnpackDate(&dt, &db, TDB_ALL);

		printf("year: %d\n"
			"yday: %d\n"
			"month: %d\n"
			"week: %d\n"
			"wday: %d\n"
			"day: %d\n"
			"hour: %d\n"
			"minute: %d\n"
			"sec: %d\n"
			"usec: %d\n",
			db.tdb_Year,
			db.tdb_YDay,
			db.tdb_Month,
			db.tdb_Week,
			db.tdb_WDay,
			db.tdb_Day,
			db.tdb_Hour,
			db.tdb_Minute,
			db.tdb_Sec,
			db.tdb_USec);

		TDateToDMY(&dt, &D, &M, &Y, TNULL);

		printf("%d %d %d\n", D, M, Y);

		TCloseModule(TUtilBase);
	}
}
