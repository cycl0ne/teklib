
/*
**	$Id: aligntest.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/exec/tests/aligntest.c - Exec module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TUINT seed = 123;

#define NUMSLOTS 100
#define SIZEMIN 1
#define SIZEMAX 127
#define MAXALI 11
#define NUMTEST 10000

TAPTR slots[NUMSLOTS];
TUINT align[MAXALI];

static void
alloctest(TAPTR mmu, TAPTR *slots, TUINT *align)
{
	TINT n = (seed = TGetRand(seed)) % NUMSLOTS;
	if (slots[n])
	{
		TFree(slots[n]);
		slots[n] = TNULL;
	}
	else
	{
		TINT s = SIZEMIN + (seed = TGetRand(seed)) % (SIZEMAX - SIZEMIN + 1);
		slots[n] = TAlloc(mmu, s);
		if (slots[n])
		{
			TUINTPTR adr = (TUINTPTR) slots[n];
			TUINT ali = 0;

			while ((adr & 1) == 0)
			{
				adr >>= 1;
				ali++;
			}

			ali = TMIN(ali, MAXALI - 1);
			align[ali]++;
		}
	}
}

void runtest(TAPTR mmu)
{
	TINT i;

	TFillMem(slots, sizeof(slots), 0);
	TFillMem(align, sizeof(align), 0);

	for (i = 0; i < NUMTEST; ++i) alloctest(mmu, slots, align);

	printf("number of allocations, distributed over alignment [bytes]\n");
	printf("--------------------------------------------------------------------\n");
	for (i = 0; i < MAXALI; ++i) printf("% 5d ", (1 << i));
	printf("(and higher)\n");
	for (i = 0; i < MAXALI; ++i) printf("% 5d ", align[i]);
	printf("\n--------------------------------------------------------------------\n");

	for (i = 0; i < NUMSLOTS; ++i) TFree(slots[i]);
}

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		TTAGITEM tags[3];
		TAPTR mmu;

		mmu = TNULL;
		{
			printf("MM-Type: TNULL\n");
			runtest(mmu);
		}

		mmu = TCreateMemManager(TNULL, TMMT_Debug, TNULL);
		{
			printf("MM-Type: TMMT_Debug\n");
			runtest(mmu);
			TDestroy(mmu);
		}

		mmu = TCreateMemManager(TNULL, TMMT_Message, TNULL);
		{
			printf("MM-Type: TMMT_Message\n");
			runtest(mmu);
			TDestroy(mmu);
		}

		mmu = TCreateMemManager(TNULL, TMMT_Tracking, TNULL);
		{
			printf("MM-Type: TMMT_Tracking\n");
			runtest(mmu);
			TDestroy(mmu);
		}

		mmu = TCreateMemManager(TNULL, TMMT_Pooled, TNULL);
		{
			printf("MM-Type: TMMT_Pooled\n");
			runtest(mmu);
			TDestroy(mmu);
		}

		tags[0].tti_Tag = TMem_StaticSize;
		tags[0].tti_Value = (TTAG) 100000;
		tags[1].tti_Tag = TTAG_DONE;
		mmu = TCreateMemManager(TNULL, TMMT_Static, tags);
		{
			printf("MM-Type: TMMT_Static\n");
			runtest(mmu);
			TDestroy(mmu);
		}

		TCloseModule(TUtilBase);
	}
}
