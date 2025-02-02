
#ifdef TSYS_MORPHOS

#include <emul/emulinterface.h>

static ULONG HookStub(void)
{
	struct Hook *hook = (struct Hook *) REG_A0;
	APTR object = (APTR) REG_A2;
	APTR message = (APTR) REG_A1;
	HOOKENTRY func = (HOOKENTRY) hook->h_SubEntry;
	return (*func)(hook->h_Data, object, message);
};

static struct EmulLibEntry HookStubGate =
{
	TRAP_LIB, 0, (void (*)(void)) HookStub
};

void InitHook(struct Hook *hook, HOOKENTRY func, APTR data)
{
	hook->h_Entry = (HOOKFUNC) &HookStubGate;
	hook->h_SubEntry = (HOOKFUNC) func;
	hook->h_Data = data;
}

#else /* TSYS_MORPHOS */

#ifdef __SASC
	#define HOOKENT			__asm
	#define REG(reg,arg)	register __ ## reg arg
#endif

#ifdef __GNUC__
	#define HOOKENT
	#define REG(reg,arg)	arg __asm( #reg )
#endif

static HOOKENT ULONG HookStub(
	REG(a0, struct Hook *hook),
	REG(a2, APTR object),
	REG(a1, APTR message))
{
	HOOKENTRY func = hook->h_SubEntry;
	return (*func)(hook->h_Data, object, message);
}

void InitHook(struct Hook *hook, HOOKENTRY func, APTR data)
{
	hook->h_Entry = (ULONG (*)()) HookStub;
	hook->h_SubEntry = func;
	hook->h_Data = data;
}

#endif /* TSYS_MORPHOS */

