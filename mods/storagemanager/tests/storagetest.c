
/*
**	tests/storagetest.c
**	test for the storage manager module
*/

#include <stdio.h>
#include <stdlib.h>

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/storagemanager.h>

/* these are my test structures */

typedef struct
{
	TUINT anInt;
	TUINT anotherInt;
	TUINT wellAnotherInt;
}
userStruct1;

typedef struct
{
	TFLOAT aFloat;
	TFLOAT anotherFloat;
	TINT someSignedInts[3];
}
userStruct2;

TVOID createUserStruct2(TAPTR inst)
{
  userStruct2* obj;
  tdbprintf(2, "constructor\n");
  obj = (userStruct2*) inst;
  obj->aFloat = 77.2f;
  obj->anotherFloat = 42.0f;
}

TVOID destroyUserStruct2(TAPTR inst)
{
  tdbprintf(2, "destructor\n");
}


/* param structure */

typedef struct
{
	TAPTR mgr;
	TSTORAGE cl1;
	TSTORAGE cl2;
}
params;

TTASKENTRY TVOID storagetest(TAPTR task)
{
	TAPTR TExecBase = TGetExecBase(task);
	userStruct1 *u1;
	userStruct2 *u2;
	params *p = TExecGetTaskData(TExecBase, task);
	TAPTR mgr;
	TSTORAGE clh;
	TINT i, j;
	TINT idx;
	TAPTR ptr;

	/* open class 1 for read/write access and add some bullshit instances */
	ptr = storagemanager_openClass(p->mgr, task, p->cl1, TSTORAGEF_RW);
	tdbprintf1(2, "ptr: %i\n", (TUINT)(TUINTPTR)ptr);
	idx = storagemanager_getClassElementsCount(p->mgr, task, p->cl1);
	tdbprintf1(2, "idx: %i\n", idx);
	u1 = storagemanager_setClassElementsCount(p->mgr, task, p->cl1, idx + 11);
	for (i = idx; i < idx + 11; i++)
	{
		tdbprintf1(2, "Adding element %d of class1.\n", i);

		u1[i].anInt = i - idx;
		u1[i].anotherInt = i;
		u1[i].wellAnotherInt = idx + 1;
	}
	storagemanager_closeClass(p->mgr, task, p->cl1);

	/* open class 1 for read-only access and class 2 for read/write */
	u1 = storagemanager_openClass(p->mgr, task, p->cl1, TSTORAGEF_RO);
	storagemanager_openClass(p->mgr, task, p->cl2, TSTORAGEF_RW);

	/* add instances to class 2 based on what we read from the first type */
	idx = storagemanager_getClassElementsCount(p->mgr, task, p->cl2);
	u2 = storagemanager_setClassElementsCount(p->mgr, task, p->cl2, idx + 11);
	for (i = 0; i < 11; i++)
	{
		tdbprintf1(2, "Adding element %d of class2.\n", i + idx);

		u2[i + idx].aFloat = (TFLOAT)i + idx / u1[i].wellAnotherInt;
		u2[i + idx].anotherFloat = 1 / u2[i + idx].aFloat;
		for (j = 0; j < 3; j++)
		{
			u2[i + idx].someSignedInts[j] = u1[i].wellAnotherInt + j + i;
		}
	}

	storagemanager_closeClass(p->mgr, task, p->cl2);
	storagemanager_closeClass(p->mgr, task, p->cl1);

	if ((mgr = TExecOpenModule(TExecBase, "storagemanager", 0, TNULL)))
	{
		tdbprintf(2, "created a new nanger instance, adding two classes...\n");
		clh = storagemanager_registerClass
      (p->mgr, task, sizeof(userStruct1), TNULL, TNULL, 0, 10, 10);
		storagemanager_registerClass
      (p->mgr, task, sizeof(userStruct2), TNULL, TNULL , 0, 10, 12);
		tdbprintf(2, "destroying the first class...\n");
		storagemanager_destroyClass(mgr, task, clh);
		tdbprintf(2, "destroying manager.\n");
		TExecCloseModule(TExecBase, mgr);
	}
	else
	{
		printf("storage manager open failed\n");
	}
}

#define NUMTASKS 100

TTASKENTRY TVOID TEKMain(TAPTR task)
{
	TAPTR TExecBase = TGetExecBase(task);

	userStruct1 *u1;
	userStruct2 *u2;

	TTAGITEM tags[2];

	TAPTR tasks[NUMTASKS];

	TUINT idx, i, j;
	params *p;

	tdbprintf(20, "started\n\n");
	p = (params *) TExecAlloc(TExecBase, TNULL, sizeof(params));

	/* get a manager instance and register two classes */
	p->mgr = TExecOpenModule(TExecBase, "storagemanager", 0, TNULL);
	tdbprintf(20, "have manager\n");
	p->cl1 = storagemanager_registerClass
    (p->mgr, task, sizeof(userStruct1), TNULL, TNULL, 0, 10, 10);
	tdbprintf(20, "registered class 1\n");
	p->cl2 = storagemanager_registerClass
    (p->mgr, task, sizeof(userStruct2), createUserStruct2, destroyUserStruct2, 
     0, 10, 12);
	tdbprintf(20, "registered class 2\n");

	tags[0].tti_Tag = TTask_UserData;
	tags[0].tti_Value = (TTAG) p;
	tags[1].tti_Tag = TTAG_DONE;

	for (i = 0; i < NUMTASKS; ++i)
	{
		tasks[i] = TExecCreateTask(TExecBase, storagetest, TNULL, tags);
	}

	for (i = 0; i < NUMTASKS; ++i)
	{
		TDestroy(tasks[i]);
	}

	/* display the data in the manager */
	u1 = storagemanager_openClass(p->mgr, task, p->cl1, TSTORAGEF_RO);
	idx = storagemanager_getClassElementsCount(p->mgr, task, p->cl1);
	for (i = 0; i < idx; i++) tdbprintf6(2, 
    "u1[%d].anInt = %d\nu1[%d].anotherInt = %d\nu1[%d].wellAnotherInt = %d\n,",
			i, u1[i].anInt, i, u1[i].anotherInt, i, u1[i].wellAnotherInt);
	storagemanager_closeClass(p->mgr, task, p->cl1);

	u2 = storagemanager_openClass(p->mgr, task, p->cl2, TSTORAGEF_RO);
	idx = storagemanager_getClassElementsCount(p->mgr, task, p->cl2);
	for (i = 0; i < idx; i++)
	{
		tdbprintf4(2, "u2[%d].aFloat = %f\nu2[%d].anotherFloat = %f\n,", 
      i, u2[i].aFloat, i, u2[i].anotherFloat);
		for (j = 0; j < 3; j++) tdbprintf3(2, 
      "u2[%d].someSignedInts[%d] = %d\n", i, j, u2[i].someSignedInts[j]);
	}
	storagemanager_closeClass(p->mgr, task, p->cl2);
	tdbprintf(20, "Destroying manager\n");
	TExecCloseModule(TExecBase, p->mgr);
	TExecFree(TExecBase, p);

	tdbprintf(20, "bye\n");
}
