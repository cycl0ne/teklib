
#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

TCALLBACK TINT mysort(TAPTR userdata, TTAG data1, TTAG data2);

/*****************************************************************************/

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TINT t1[20], t2[20];
	TINT i;

	TAPTR TExecBase = TGetExecBase(task);
	TAPTR TUtilBase = TOpenModule("util", 0, TNULL);
	TINT seed = 123;

	if (TUtilBase)
	{
		for(i=0;i<20;i++)
		{
			t1[i] = (seed = TGetRand(seed)) & 255;
			t2[i]=t1[i];
		}

		TQSort(t2,20,sizeof(TINT),mysort, TNULL);

		for(i=0;i<20;i++)
		{
			printf("%d\t%d\n",t1[i],t2[i]);
		}

		TCloseModule(TUtilBase);
	}
}

TCALLBACK TINT mysort(TAPTR userdata, TTAG data1, TTAG data2)
{
	TINT a = *(TINT *) data1;
	TINT b = *(TINT *) data2;

	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
}
