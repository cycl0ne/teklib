
/*
**	$Id: interval.c,v 1.3 2006/09/10 01:06:45 tmueller Exp $
**	teklib/src/time/tests/interval.c - Time module unit test
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

/* default duration in seconds */
#define DEF_DURATION	20
/* default implementation */
#define DEF_IMPL		"device"

/* argument template */
#define TEMPLATE "-d=DURATION/N,-i=IMPL/K,-l=LIST/S,-h=HELP/S"
enum { ARG_DURATION, ARG_IMPL, ARG_LIST, ARG_HELP, ARG_NUM };
TTAG args[ARG_NUM];

TAPTR TExecBase;
TAPTR TUtilBase;

/*****************************************************************************/
/*
**	Interval timer structures
*/

struct ITimer
{
	struct THandle handle;
	TAPTR replyport;
	struct TList reqs; /* list of requests (device implementation only) */
	struct TTask *task; /* management task (task implementation only) */
};

struct ITimeMsg
{
	struct TNode node;
 	void (*func)(struct ITimeMsg *msg);
	TAPTR userdata;
	TTIME interval;
	TBOOL restart;
	TTIME timeout;
	struct TTimeRequest *treq; /* I/O request (device implementation only) */
};

typedef void (*ITIMERFUNC)(struct ITimeMsg *msg);

/*****************************************************************************/
/*
**	Task implementation
*/

static void task_timerfunc(struct TTask *task)
{
	struct TNode *nnode, *node;
	TAPTR port = TGetUserPort(task);
	TUINT portsig = TGetPortSignal(port);
	TTIME waitt, nowt;
	struct ITimeMsg *msg;
	TUINT sig;
	struct TList reqs;
	TInitList(&reqs);

	do
	{
		while ((msg = (struct ITimeMsg *) TGetMsg(port)))
			TAddTail(&reqs, &msg->node);

		waitt.tdt_Int64 = 0x7fffffffffffffffULL;
		TGetSystemTime(&nowt);

		for (node = reqs.tlh_Head; (nnode = node->tln_Succ); node = nnode)
		{
			msg = (struct ITimeMsg *) node;
			if (TCmpTime(&msg->timeout, &nowt) < 0)
			{
				/* reply finished request to caller */
				TRemove(node);
				TReplyMsg(msg);
				continue;
			}
			/* get new next waittime (absolute) */
			if (TCmpTime(&msg->timeout, &waitt) < 0)
				waitt = msg->timeout;
		}

		/* make waittime relative and wait */
		TSubTime(&waitt, &nowt);
		sig = TWaitTime(&waitt, portsig | TTASK_SIG_ABORT);

	} while (!(sig & TTASK_SIG_ABORT));

	while ((msg = (struct ITimeMsg *) TRemHead(&reqs)))
		TFree(msg);
}

static THOOKENTRY TTAG
task_destroytimer(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct ITimer *timer = obj;
 	TSignal(timer->task, TTASK_SIG_ABORT);
	TDestroy((struct THandle *) timer->task);
	TFree(timer);
	return 0;
}

static THOOKENTRY TTAG
timer_dispatch(struct THook *hook, TAPTR task, TTAG msg)
{
	switch (msg)
	{
		case TMSG_INITTASK:
			return TTRUE;
		case TMSG_RUNTASK:
			task_timerfunc(task);
			break;
	}
	return 0;
}

static struct ITimer *task_timer_create(TAPTR replyport)
{
	struct ITimer *t = TAlloc(TNULL, sizeof(struct ITimer));
	if (t)
	{
		struct THook taskhook;
		TInitHook(&taskhook, timer_dispatch, TNULL);
		t->handle.thn_Hook.thk_Entry = task_destroytimer;
		t->replyport = replyport;
		t->task = TCreateTask(&taskhook, TNULL);
		if (t->task)
			return t;
		TDestroy(&t->handle);
	}
	return TNULL;
}

static TBOOL task_timer_set(struct ITimer *timer, TINT sec, TINT usec,
	ITIMERFUNC func, TBOOL restart, TAPTR udata)
{
	struct ITimeMsg *msg = TAllocMsg0(sizeof(struct ITimeMsg));
	if (!msg)
		return TFALSE;
	msg->func = func;
	msg->userdata = udata;
	msg->interval.tdt_Int64 = sec * 1000000ULL + usec;
	msg->restart = restart;
	TGetSystemTime(&msg->timeout);
	TAddTime(&msg->timeout, &msg->interval);
	TPutMsg(TGetUserPort(timer->task), timer->replyport, msg);
	return TTRUE;
}

static void task_timer_dispatch(struct ITimer *timer)
{
	struct ITimeMsg *msg;
	while ((msg = (struct ITimeMsg *) TGetMsg(timer->replyport)))
	{
		(*msg->func)(msg);
		if (msg->restart)
		{
			TAddTime(&msg->timeout, &msg->interval);
			TPutMsg(TGetUserPort(timer->task), timer->replyport, msg);
		}
		else
			TFree(msg);
	}
}

/*****************************************************************************/
/*
**	direct timer.device I/O implementation
*/

static THOOKENTRY TTAG
device_destroytimer(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct ITimer *timer = obj;
	struct TNode *nnode, *node;
	for (node = timer->reqs.tlh_Head; (nnode = node->tln_Succ); node = nnode)
	{
		struct ITimeMsg *msg = (struct ITimeMsg *) node;
		struct TTimeRequest *tr = msg->treq;
		TAbortIO((struct TIORequest *) tr);
		TWaitIO((struct TIORequest *) tr);
		TFreeTimeRequest(tr);
		TFree(msg);
	}
	TFree(timer);
	return 0;
}

static struct ITimer *device_timer_create(TAPTR replyport)
{
	struct ITimer *t = TAlloc(TNULL, sizeof(struct ITimer));
	if (t)
	{
		TInitList(&t->reqs);
		t->replyport = replyport;
		t->handle.thn_Hook.thk_Entry = device_destroytimer;
		return t;
	}
	return TNULL;
}

static TBOOL device_timer_set(struct ITimer *timer, TINT sec, TINT usec,
	ITIMERFUNC func, TBOOL restart, TAPTR udata)
{
	struct ITimeMsg *msg = TAllocMsg0(sizeof(struct ITimeMsg));
	if (!msg)
		return TFALSE;

	/* get timerequest (= timer.device instance) */
	msg->treq = TAllocTimeRequest(TNULL);
	if (!msg->treq)
	{
		TFree(msg);
		return TFALSE;
	}

	/* prepare message */
	msg->func = func;
	msg->userdata = udata;
	msg->interval.tdt_Int64 = sec * 1000000ULL + usec;
	msg->restart = restart;
	TGetSystemTime(&msg->timeout);
	TAddTime(&msg->timeout, &msg->interval);

	/* prepare timerequest */
	msg->treq->ttr_Data.ttr_Time = msg->interval;
	msg->treq->ttr_Req.io_ReplyPort = timer->replyport;
	msg->treq->ttr_Req.io_Command = TTREQ_WAITTIME;

	/* send to device */
	TPutIO((struct TIORequest *) msg->treq);

	/* insert to queue */
	TAddTail(&timer->reqs, &msg->node);

	return TTRUE;
}

static void device_timer_dispatch(struct ITimer *timer)
{
	struct TNode *nnode, *node = timer->reqs.tlh_Head;
	for (; (nnode = node->tln_Succ); node = nnode)
	{
		struct ITimeMsg *msg = (struct ITimeMsg *) node;
		if (TCheckIO((struct TIORequest *) msg->treq))
		{
			TWaitIO((struct TIORequest *) msg->treq);
			(*msg->func)(msg);
			if (msg->restart)
			{
				TTIME nowt;
				TGetSystemTime(&nowt);
				TAddTime(&msg->timeout, &msg->interval);
				msg->treq->ttr_Data.ttr_Time = msg->timeout;
				TSubTime(&msg->treq->ttr_Data.ttr_Time, &nowt);
				TPutIO((struct TIORequest *) msg->treq);
			}
			else
			{
				TRemove(node);
				TCloseModule(msg->treq->ttr_Req.io_Device);
				TFree(msg->treq);
				TFree(msg);
			}
		}
	}
}

/*****************************************************************************/
/*
**	Interval timer interface
*/

/* available implementations */
enum { ITIMER_TASK, ITIMER_DEVICE, ITIMER_NUM };

static const struct ITimerIFace
{
	TSTRPTR Name;
	struct ITimer *(*Create)(TAPTR replyport);
	TBOOL (*Set)(struct ITimer *timer, TINT sec, TINT usec, ITIMERFUNC func,
		TBOOL restart, TAPTR udata);
	void (*Dispatch)(struct ITimer *timer);
}
ITimerImpl[] =
{
	{ "task", task_timer_create, task_timer_set, task_timer_dispatch },
	{ "device", device_timer_create, device_timer_set, device_timer_dispatch },
};

/*****************************************************************************/
/*
**	Run test
*/

TAPTR replyport = TNULL;
TTIME starttime;

void testfunc(struct ITimeMsg *msg)
{
	TTIME dt;
	TGetSystemTime(&dt);
	TSubTime(&dt, &starttime);
	printf("%2.3fs", (TFLOAT) dt.tdt_Int64 / 1000000);
	printf(" (interval: %ds - %s)\n",
		(TINT) (msg->interval.tdt_Int64 / 1000000),
		msg->restart ? "restart" : "oneshot");
}

void runtest(const struct ITimerIFace *iface, struct ITimer *timer)
{
	TUINT replysig = TGetPortSignal(replyport);
	TUINT sig;
	TDATE date;
	TTIME dt;
	TINT duration = *(TINT *) args[ARG_DURATION];

	printf("Test duration is %d seconds.\n", duration);
	printf("Using %s timer implementation\n", iface->Name);
	printf("-------------------------------\n");

	TGetSystemTime(&starttime);

	/* setup test expiration date */
	dt.tdt_Int64 = duration * 1000000;
	TGetUniversalDate(&date);
	TAddDate(&date, 0, &dt);

	/* setup timers */
	iface->Set(timer, 1, 0, testfunc, TTRUE, TNULL);
	iface->Set(timer, 3, 0, testfunc, TTRUE, TNULL);
	iface->Set(timer, 5, 0, testfunc, TTRUE, TNULL);
	iface->Set(timer, 7, 0, testfunc, TFALSE, TNULL);

	for (;;)
	{
		sig = TWaitDate(&date, replysig);
		if (!sig) break;
		iface->Dispatch(timer);
	}
}

/*****************************************************************************/
/*
**	Main
*/

void TEKMain(struct TTask *task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	if (TUtilBase)
	{
		TAPTR argh;
		TINT def_duration = DEF_DURATION;

		args[ARG_DURATION] = (TTAG) &def_duration;
		args[ARG_IMPL] = (TTAG) DEF_IMPL;
		args[ARG_LIST] = TFALSE;
		args[ARG_HELP] = TFALSE;

		argh = TParseArgV(TEMPLATE, TGetArgV() + 1, args);
		if (argh && !args[ARG_HELP])
		{
			TINT i;
			if (args[ARG_LIST])
			{
				printf("List of possible timer implementations:\n");
				printf("---------------------------------------\n");
				for (i = 0; i < ITIMER_NUM; ++i)
					puts(ITimerImpl[i].Name);
			}
			else
			{
				TSTRPTR impl = (TSTRPTR) args[ARG_IMPL];
				const struct ITimerIFace *iface = TNULL;
				for (i = 0; i < ITIMER_NUM; ++i)
					if (!TStrCaseCmp(impl, ITimerImpl[i].Name))
						iface = &ITimerImpl[i];
				if (!iface)
				{
					printf("Implementation `%s' not found.\n", impl);
					printf("Use -l to see the available implementations.\n");
				}
				else
				{
					replyport = TCreatePort(TNULL);
					if (replyport)
					{
						struct ITimer *timer = iface->Create(replyport);
						if (timer)
						{
							runtest(iface, timer);
							TDestroy(&timer->handle);
						}
						TDestroy(replyport);
					}
				}
			}
		}
		else
		{
			printf("Usage: interval %s\n", TEMPLATE);
			printf("Interval timer test.\n");
			printf("-d=DURATION/N - duration of test in seconds\n");
			printf("-i=IMPL/K   - implementation to use [default: `%s']\n",
				DEF_IMPL);
			printf("-l=LIST/S   - list possible implementations\n");
			printf("-h=HELP/S   - this help\n");
		}
		TDestroy(argh);
	}
	else
		printf("Could not open all modules.\n");

	TCloseModule(TUtilBase);
}
