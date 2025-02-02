
/*
**	Memory alignment test
*/

#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

TAPTR TExecBase;
TAPTR TUtilBase;
TINT seed = 123;

#define NUMSLOTS 100
#define SIZEMIN 1
#define SIZEMAX 127
#define MAXALI 11
#define NUMTEST 10000

TAPTR slots[NUMSLOTS];
TUINT align[MAXALI];

static TVOID
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

TVOID runtest(TAPTR mmu)
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

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		TTAGITEM tags[3];
		TAPTR mmu;

		mmu = TNULL;
		{		
			printf("MMU-Type: TNULL\n");
			runtest(mmu);
		}

		mmu = TCreateMMU(TNULL, TMMUT_Debug, TNULL);
		{
			printf("MMU-Type: TMMUT_Debug\n");
			runtest(mmu);
			TDestroy(mmu);
		}
	
		mmu = TCreateMMU(TNULL, TMMUT_Message, TNULL);
		{
			printf("MMU-Type: TMMUT_Message\n");
			runtest(mmu);
			TDestroy(mmu);
		}
		
		mmu = TCreateMMU(TNULL, TMMUT_Tracking, TNULL);
		{
			printf("MMU-Type: TMMUT_Tracking\n");
			runtest(mmu);
			TDestroy(mmu);
		}

		mmu = TCreateMMU(TNULL, TMMUT_Pooled, TNULL);
		{
			printf("MMU-Type: TMMUT_Pooled\n");
			runtest(mmu);
			TDestroy(mmu);
		}

		tags[0].tti_Tag = TMem_StaticSize;
		tags[0].tti_Value = (TTAG) 100000;
		tags[1].tti_Tag = TTAG_DONE;
		mmu = TCreateMMU(TNULL, TMMUT_Static, tags);
		{
			printf("MMU-Type: TMMUT_Static\n");
			runtest(mmu);
			TDestroy(mmu);
		}

		TCloseModule(TUtilBase);
	}
}
