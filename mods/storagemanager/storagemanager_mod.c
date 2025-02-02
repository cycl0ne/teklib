
/*
**	storage manager module
**
**	this is a 'common', i.e. a cross-platform module.
**  opening this module creates a manager instance capable 
**  of holding different storage classes.
**  each storage class provides thread safe access to a
**  resizable array of one or more elements of a particular
**  size specified at the storage class' registration time. 
**  an external id specified by the user may be used to
**  group and identify the type of data held by a storage 
**  class when it is registered dynamically at run time
*/

#include <tek/exec.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/debug.h>
#include <tek/mod/storagemanager.h>

#define MOD_VERSION		0
#define MOD_REVISION	4


typedef struct _TStorageClass
{

	TUINT8 *dataPointer;
	TUINT instanceSize;

  TVOID (*constructor)(TAPTR);
  TVOID (*destructor)(TAPTR);

	TUINT externalID;

	TUINT numElements;

	TUINT numAllocated;
	TUINT numElementsGrow;

	TUINT flags;
}
TSTORAGECLASS;

typedef struct _TModStorageManger
{
	struct TModule module;				/* module header */

	TAPTR memoryManager;
	TAPTR repository;

	union
	{
		TUINT id;
		TAPTR atom;
	}
	ID;

	TAPTR util;
	TAPTR exec;

}	TMOD_STORAGEMANAGER;

/*
**	module prototypes
*/

static TCALLBACK TVOID mod_unload(TMOD_STORAGEMANAGER * mod);

static TCALLBACK TMOD_STORAGEMANAGER *mod_open
 (TMOD_STORAGEMANAGER * mod, TAPTR tsk, TTAGITEM * tags);

static TCALLBACK TVOID mod_close
 (TMOD_STORAGEMANAGER * mod, TAPTR tsk);

static TMODAPI TSTORAGE storagemanager_registerClass
 (TMOD_STORAGEMANAGER * mgr, TAPTR tsk, 
  TUINT instanceSize, TCALLBACK TVOID(*constructor)(TAPTR), TCALLBACK TVOID(*destructior)(TAPTR),
  TUINT externalId, 
  TUINT initialCapacity, TUINT numElementsGrow);


static TMODAPI TVOID storagemanager_destroyClass
 (TMOD_STORAGEMANAGER * mgr, TAPTR tak, TSTORAGE id);

static TMODAPI TAPTR storagemanager_openClass
 (TMOD_STORAGEMANAGER * mgr, TAPTR tak, TSTORAGE id, TUINT mode);

static TMODAPI TVOID storagemanager_closeClass
 (TMOD_STORAGEMANAGER * mgr, TAPTR tak, TSTORAGE id);

static TMODAPI TAPTR storagemanager_setClassElementsCount
 (TMOD_STORAGEMANAGER * mgr, TAPTR tsk, TSTORAGE id, TUINT numElements);

static TMODAPI TINT storagemanager_getClassElementsCount
 (TMOD_STORAGEMANAGER * mgr, TAPTR tak, TSTORAGE id);

static TSTRPTR myitoa(TSTRPTR buf, TINT i);

/* 
**	tek_init_<modname>
**	module initializations (not instance-specific)
*/

TMODENTRY TUINT tek_init_storagemanager
 (TAPTR tsk, TMOD_STORAGEMANAGER * mod, TUINT16 version, TTAGITEM * tags)
{
	if (!mod)
	{
		if (version != 0xffff)	/* first call */
		{
			if (version <= MOD_VERSION)	/* version check */
			{
				return sizeof(TMOD_STORAGEMANAGER);	/* return module positive size */
			}
		}
		else					/* second call */
		{
			return sizeof(TAPTR) * 6;	/* return module negative size */
		}
	}
	else						/* third call */
	{
		mod->exec = TGetExecBase(mod);

		/* version */
		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;

		/* unload */
		mod->module.tmd_DestroyFunc = (TDFUNC) mod_unload;

		/* constructor/destructor */
		mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;

		/* put module vectors in front */
		((TAPTR *) mod)[-1] = (TAPTR) storagemanager_registerClass;
		((TAPTR *) mod)[-2] = (TAPTR) storagemanager_destroyClass;
		((TAPTR *) mod)[-3] = (TAPTR) storagemanager_openClass;
		((TAPTR *) mod)[-4] = (TAPTR) storagemanager_closeClass;
		((TAPTR *) mod)[-5] = (TAPTR) storagemanager_setClassElementsCount;
		((TAPTR *) mod)[-6] = (TAPTR) storagemanager_getClassElementsCount;

		/* make sure the ID of the template instance is zero */
		mod->ID.atom = TExecLockAtom
      (mod->exec, "StrgMgrInstCnt", TATOMF_CREATE | TATOMF_NAME);
		TExecSetAtomData(mod->exec, mod->ID.atom, 0);
		TExecUnlockAtom(mod->exec, (TAPTR) mod->ID.atom, TATOMF_KEEP);

		return TTRUE;
	}

	return 0;
}

/* final unload of the "big mama instance" */

static TCALLBACK TVOID mod_unload(TMOD_STORAGEMANAGER * mod)
{
	TExecLockAtom(mod->exec, mod->ID.atom, TATOMF_DESTROY);
}

/* private functions (the messy stuff) */

static TBOOL _storageclass_init
 (TMOD_STORAGEMANAGER * mgr, TSTORAGECLASS * sc,
  TUINT instanceSize, TVOID(*constructor)(TAPTR), TVOID(*destructor)(TAPTR),
  TUINT externalId,
  TUINT initialCapacity, TUINT numElementsGrow)
{
	TAPTR aptr;
	
	aptr = TExecAlloc
    (mgr->exec, mgr->memoryManager, instanceSize * initialCapacity);
	if (aptr == TNULL)
		return TFALSE;

	sc->instanceSize = instanceSize;
  sc->constructor  = constructor;
  sc->destructor   = destructor;
  sc->externalID   = externalId;

	sc->dataPointer  = aptr;
	sc->numElements  = 0;
  
	sc->numAllocated    = initialCapacity;
	sc->numElementsGrow = numElementsGrow;

	return TTRUE;
}

static TAPTR _storageclass_checkoutAtom
  (TMOD_STORAGEMANAGER * mgr, TAPTR tsk, TSTORAGE id)
{
	TSTORAGECLASS *repository;
	TAPTR classAtom;

	/* lock repository for reading */
	TExecLockAtom(mgr->exec, mgr->repository, TATOMF_SHARED | TATOMF_KEEP);
	repository = (TSTORAGECLASS *) TExecGetAtomData(mgr->exec, mgr->repository);

	/* valid storage handle ?  no -> unlock repository and exit */
	if ((id < 0) || (id >= repository->numElements))
	{
		TExecUnlockAtom(mgr->exec, mgr->repository, TATOMF_KEEP);
		return TNULL;
	}

	/* get class atom to return and unlock repository */
	classAtom = ((TAPTR *) repository->dataPointer)[id];
	TExecUnlockAtom(mgr->exec, mgr->repository, TATOMF_KEEP);

	return classAtom;
}

static TUINT _storageclass_setElementsCount
  (TMOD_STORAGEMANAGER * mgr, TSTORAGECLASS * sc, TUINT numElements)
{
	TUINT numAllocated = sc->numAllocated;
	TUINT step = sc->numElementsGrow;
	TUINT8 *aptr = sc->dataPointer;
  TUINT i;

	/* need to realloc */
	if ((numElements > numAllocated) || ((numAllocated > step) && 
      (numElements < (numAllocated - step))))
  {
	  /* adjust size */
	  if (numElements > numAllocated)
      numAllocated += step * ( (numElements-numAllocated) / step + 1 );
    else if ((numAllocated > step) && (numElements < (numAllocated - step)))
		  numAllocated -= step * ( (numAllocated-numElements) / step );

	  /* realloc */
	  aptr = TExecRealloc
    (mgr->exec, sc->dataPointer, numAllocated * sc->instanceSize);
	  if (aptr == TNULL)
		return 0;
  }

  /* write back - call constructor/destructor if present */
  sc->dataPointer = aptr;
  
  if ((numElements > sc->numElements) && (sc->constructor != TNULL))
    for (i = numElements - sc->numElements, 
         aptr += sc->numElements * sc->instanceSize; 
         --i != (TUINT) -1;
         aptr += sc->instanceSize) 
      (*sc->constructor)(aptr);

  else if (sc->destructor != TNULL)
    for (i = sc->numElements - numElements, 
         aptr += (sc->numElements-1) * sc->instanceSize; 
         --i != (TUINT) -1;
         aptr -= sc->instanceSize)
      (*sc->destructor)(aptr); 

	sc->numAllocated = numAllocated;
	sc->numElements = numElements;

	return numElements;
}

/* public api implementation */

static TCALLBACK TMOD_STORAGEMANAGER *mod_open
  (TMOD_STORAGEMANAGER * mod, TAPTR tsk, TTAGITEM * tags)
{
	TMOD_STORAGEMANAGER *self;
	TSTORAGECLASS *repository;
	TUINT ID;
	TINT8 tmpStr[256];

	self = TNewInstance(mod, mod->module.tmd_PosSize, mod->module.tmd_NegSize);	
	if (!self) return TNULL;

	self->util = TExecOpenModule(mod->exec, "util", 0, TNULL);
	if (!self->util)
		goto ____failure_mod_open;

	#ifdef TDEBUG
		self->memoryManager = TExecCreateMMU
      (mod->exec, TNULL, TMMUT_Tracking | TMMUT_TaskSafe, TNULL);
	#endif

	/* get a unique id for our instance */
	TExecLockAtom(mod->exec, (TAPTR) mod->ID.atom, TATOMF_KEEP);
	ID = (TUINT) TExecGetAtomData(mod->exec, mod->ID.atom);
	self->ID.id = ID++;
	TExecSetAtomData(mod->exec, mod->ID.atom, (TTAG) ID);
	TExecUnlockAtom(mod->exec, mod->ID.atom, TATOMF_KEEP);

	/* allocate classes repository */
	repository = (TSTORAGECLASS *) TExecAlloc
    (mod->exec, self->memoryManager, sizeof(TSTORAGECLASS));
	if (repository == TNULL)
		goto __failure_mod_open;

	/* initialize it */
	if (!_storageclass_init(self, repository, sizeof(TAPTR),TNULL,TNULL,0,1,1))
		goto _failure_mod_open;

	/* tie it to an atom */
	/*sprintf(tmpStr,"StrgMgr%d.Repository",self->ID.id); */
	TUtilStrCpy(self->util, tmpStr, "StrgMgr");
	myitoa(tmpStr + 7, self->ID.id);
	TUtilStrCat(self->util, tmpStr, ".Repository");
	self->repository = TExecLockAtom
    (mod->exec, tmpStr, TATOMF_NAME | TATOMF_CREATE);
	if (self->repository == TNULL)
		goto _failure_mod_open;
	TExecSetAtomData(mod->exec, self->repository, (TTAG) repository);
	TExecUnlockAtom(mod->exec, self->repository, TATOMF_KEEP);

	/* return new instance */
	return self;

  _failure_mod_open:
	TExecFree(mod->exec, repository);
  __failure_mod_open:
	TDestroy(self->memoryManager);
	TExecCloseModule(mod->exec, self->util);
  ____failure_mod_open:
	TFreeInstance(self);
	return TNULL;
}

static TCALLBACK TVOID mod_close(TMOD_STORAGEMANAGER * mgr, TAPTR tsk)
{
	TAPTR exec = TGetExecBase(mgr);
	TSTORAGECLASS *repository;
	TAPTR repAtom;
	TAPTR *atoms;
	TUINT i;

	/* get a lock on the repository */
	repAtom = mgr->repository;
	TExecLockAtom(exec, repAtom, TATOMF_KEEP);
	repository = (TSTORAGECLASS *) TExecGetAtomData(mgr->exec, repAtom);
	atoms = (TAPTR *) repository->dataPointer;

	/* destroy all registered classes */
	for (i = 0; i < repository->numElements; i++)
	{
		if (atoms[i] != TNULL)
			storagemanager_destroyClass(mgr, tsk, i);
	}

	/* free everything */
	TExecFree(exec, atoms);
	TExecFree(exec, repository);
	TDestroy(mgr->memoryManager);
	TExecCloseModule(exec, mgr->util);
	TFreeInstance(mgr);

	/* destroy the atom */
	TExecUnlockAtom(exec, repAtom, TATOMF_DESTROY);
}

static TMODAPI TSTORAGE storagemanager_registerClass
	(TMOD_STORAGEMANAGER * mgr, 
   TAPTR tsk, 
   TUINT instanceSize,
   TCALLBACK TVOID (*constructor)(TAPTR),
   TCALLBACK TVOID (*destructor)(TAPTR),
   TUINT externalId, 
   TUINT initialCapacity, 
   TUINT numElementsGrow)
{
	TSTORAGECLASS *repository;
	TAPTR *repAtoms;
	TAPTR classAtom;
	TSTORAGECLASS *sc;

	TINT8 tmpStr[256];
	TUINT i, j;

	/* get exclusive access to repository */
	TExecLockAtom(mgr->exec, mgr->repository, TATOMF_KEEP);
	repository = (TSTORAGECLASS *) TExecGetAtomData(mgr->exec, mgr->repository);

	/* grow the main repository */
	i = _storageclass_setElementsCount
    (mgr, repository, repository->numElements + 1);

	if (i == 0)
		goto __failure_registerClass;
	i -= 1;

	/* instantiate storage class type info and initialize */
	sc = (TSTORAGECLASS *) TExecAlloc
    (mgr->exec, mgr->memoryManager, sizeof(TSTORAGECLASS));
	if (sc == TNULL)
		goto __failure_registerClass;

	if (!_storageclass_init(mgr, sc, 
                          instanceSize, constructor, destructor, externalId, 
                          initialCapacity, numElementsGrow))
		goto _failure_registerClass;

	/* register an atom for the storage class */
	/*sprintf(tmpStr,"StrgMgr%d.Class%d",mgr->ID.id,i); */
	TUtilStrCpy(mgr->util, tmpStr, "StrgMgr");
	myitoa(tmpStr + 7, mgr->ID.id);
	TUtilStrCat(mgr->util, tmpStr, ".Class");
	j = TUtilStrLen(mgr->util, tmpStr);
	myitoa(tmpStr + j, i);
	classAtom = TExecLockAtom(mgr->exec, tmpStr, TATOMF_CREATE | TATOMF_NAME);
	TExecSetAtomData(mgr->exec, classAtom, (TTAG) sc);
	TExecUnlockAtom(mgr->exec, classAtom, TATOMF_KEEP);

	/* put the atom in the repository */
	repAtoms = (TAPTR *) repository->dataPointer;
	repAtoms[i] = classAtom;

	/* release the repository atom and return handle */
	TExecUnlockAtom(mgr->exec, mgr->repository, TATOMF_KEEP);
	return (TSTORAGE) i;

  _failure_registerClass:
	TExecFree(mgr->exec, sc);
  __failure_registerClass:
	TExecUnlockAtom(mgr->exec, mgr->repository, TATOMF_KEEP);
	return -1;
}

static TMODAPI TVOID storagemanager_destroyClass
  (TMOD_STORAGEMANAGER * mgr, TAPTR tsk, TSTORAGE id)
{
	TAPTR exec = TGetExecBase(mgr);
	TAPTR classAtom;
	TSTORAGECLASS *repository;
	TSTORAGECLASS *victim;
  TUINT8 *aptr;
  TUINT i;

	/* lock repository */
	TExecLockAtom(exec, mgr->repository, TATOMF_KEEP);
	repository = (TSTORAGECLASS *) TExecGetAtomData(mgr->exec, mgr->repository);

	/* valid storage handle ?  no -> unlock repository and exit */
	if ((id < 0) || (id >= repository->numElements))
		goto _failure_destroyClass;

	/* get class atom to return and unlock repository */
	classAtom = ((TAPTR *) repository->dataPointer)[id];

	/* already destroyed ? */
	if (classAtom == TNULL)
		goto _failure_destroyClass;

	/* set the repository entry to TNULL and unlock the repository */
	((TAPTR *) repository->dataPointer)[id] = TNULL;
	TExecUnlockAtom(exec, mgr->repository, TATOMF_KEEP);

	/* get the class instance */
	TExecLockAtom(exec, classAtom, TATOMF_KEEP);
	victim = (TSTORAGECLASS *) TExecGetAtomData(mgr->exec, classAtom);

	/* destroy the class' atom */
	TExecUnlockAtom(exec, classAtom, TATOMF_DESTROY);

  /* call destructor if present on all existing elements */
  if (victim->destructor != TNULL)
    for (i = victim->numElements, aptr = victim->dataPointer 
                          + victim->instanceSize * (victim->numElements - 1);
         --i != (TUINT) -1;
         aptr -= victim->instanceSize)
      (*victim->destructor)(aptr); 

	/* free the resources and free the class */  
	TExecFree(exec, victim->dataPointer);
	TExecFree(exec, victim);

	return;

  _failure_destroyClass:
	TExecUnlockAtom(exec, mgr->repository, TATOMF_KEEP);
}

static TMODAPI TAPTR storagemanager_openClass
 (TMOD_STORAGEMANAGER * mgr, TAPTR tsk, TSTORAGE id, TUINT mode)
{
	TAPTR classAtom;
	TSTORAGECLASS *sc;

	/* get class atom */
	classAtom = _storageclass_checkoutAtom(mgr, tsk, id);
	if (classAtom == TNULL)
		return TNULL;

	/* lock it - shared or exclusive (ro/rw) */
	TExecLockAtom(mgr->exec, classAtom, TATOMF_KEEP | mode);
	sc = (TSTORAGECLASS *) TExecGetAtomData(mgr->exec, classAtom);

	/* set write flag if we are in rw mode */
	if (mode == TSTORAGEF_RW)
	{
		sc->flags |= 1;
	}

	return sc->dataPointer;
}

static TMODAPI TVOID storagemanager_closeClass
 (TMOD_STORAGEMANAGER * mgr, TAPTR tsk, TSTORAGE id)
{
	TAPTR classAtom;
	TSTORAGECLASS *sc;

	/* get class atom */
	classAtom = _storageclass_checkoutAtom(mgr, tsk, id);
	if (classAtom == TNULL)
		return;

	/* try to lock it shared to see if we are the task who opened it */
	classAtom = TExecLockAtom(mgr->exec, classAtom, TATOMF_KEEP | TATOMF_SHARED);
	if (classAtom == TNULL)
		return;

	/* get class */
	sc = (TSTORAGECLASS *) TExecGetAtomData(mgr->exec, classAtom);

	/* unset write flag if we are in rw mode */
	if (sc->flags & 1)
	{
		sc->flags &= ~1;
	}

	/* release netsted and "official" lock */
	TExecUnlockAtom(mgr->exec, classAtom, TATOMF_KEEP);
	TExecUnlockAtom(mgr->exec, classAtom, TATOMF_KEEP);
}

static TMODAPI TAPTR storagemanager_setClassElementsCount
 (TMOD_STORAGEMANAGER * mgr, TAPTR tsk, TSTORAGE id, TUINT numElements)
{
	TAPTR classAtom;
	TSTORAGECLASS *sc;

	/* get class atom */
	classAtom = _storageclass_checkoutAtom(mgr, tsk, id);
	if (classAtom == TNULL)
		goto __failure_setClassElementsCount;

	/* try to get an exclusive lock */
	classAtom = TExecLockAtom(mgr->exec, classAtom, TATOMF_KEEP | TATOMF_TRY);
	if (classAtom == TNULL)
		goto __failure_setClassElementsCount;

	/* check whether the class was opened for writing before */
	sc = (TSTORAGECLASS *) TExecGetAtomData(mgr->exec, classAtom);
	if (!(sc->flags & 1))
		goto _failure_setClassElementsCount;

	/* release the nested lock */
	TExecUnlockAtom(mgr->exec, classAtom, TATOMF_KEEP);

	/* change the class */
	_storageclass_setElementsCount(mgr, sc, numElements);

	/* return new data pointer 'caus it might have been moved */
	return sc->dataPointer;

  _failure_setClassElementsCount:
	TExecUnlockAtom(mgr->exec, classAtom, TATOMF_KEEP);
  __failure_setClassElementsCount:
	return TNULL;
}

static TMODAPI TINT storagemanager_getClassElementsCount
 (TMOD_STORAGEMANAGER * mgr, TAPTR tsk, TSTORAGE id)
{
	TAPTR classAtom;
	TINT result;

	/* get class atom */
	classAtom = _storageclass_checkoutAtom(mgr, tsk, id);
	if (classAtom == TNULL)
		return -1;

	/* try to get a shared lock */
	classAtom = TExecLockAtom
    (mgr->exec, classAtom, TATOMF_KEEP | TATOMF_SHARED | TATOMF_TRY);
	if (classAtom == TNULL)
		return -1;

	/* get the value we are looking for and unlock the (probably nested) lock */
	result = 
    ((TSTORAGECLASS *) TExecGetAtomData(mgr->exec, classAtom))->numElements;
	TExecUnlockAtom(mgr->exec, classAtom, TATOMF_KEEP);

	return result;
}

static TSTRPTR myitoa2(TSTRPTR buf, TUINT i)
{
	if (i >= 10) buf = myitoa2(buf, i/10);
	*buf++ = i%10+48;
	return buf;
}

static TSTRPTR myitoa(TSTRPTR buf, TINT i)
{
	if (i < 0)
	{
		*buf++ = '-';
		i = -i;
	}
	if (i >= 10) buf = myitoa2(buf, (TUINT) i/10);
	*buf++ = i % 10 + 48;
	*buf = 0;
	return buf;
}
