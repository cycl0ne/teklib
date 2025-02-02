
#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/iface/exec.h>

TAPTR TExecBase;
struct TExecIFace *TExecIFace;
TAPTR mem;

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TExecIFace = (struct TExecIFace *)
		TQueryInterface(TExecBase, "exec", 1, TNULL);
	if (TExecIFace)
	{
		printf("have interface: %p\n", TExecIFace);
		mem = TExecIFace->Alloc(TExecBase, TNULL, 1000);
		if (mem)
		{
			printf("have mem, size: %d\n", (TUINT) TExecIFace->GetSize(TExecBase, mem));
			TExecIFace->Free(TExecBase, mem);
		}
		TDestroy(&TExecIFace->IFace.tif_Handle);
	}
}
