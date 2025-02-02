
#ifndef _TEK_MOD_EXEC_MOD_H
#define _TEK_MOD_EXEC_MOD_H

/*
**	$Id: exec_mod.h,v 1.10 2005/09/13 02:41:58 tmueller Exp $
**	teklib/mods/exec/exec_mod.h - Exec module internal definitions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/exec.h>
#include <tek/mod/ioext.h>
#include <tek/proto/hal.h>

/*****************************************************************************/

#define MOD_VERSION		3
#define MOD_REVISION	1
#define MOD_NUMVECTORS	77

/*****************************************************************************/

#ifndef LOCAL
#define LOCAL
#endif

#ifndef EXPORT
#define EXPORT	TMODAPI
#endif

/*****************************************************************************/
/* 
**	Device calls
*/

#define TBeginIO(dev,msg)	\
	(*(((TMODCALL TVOID(**)(TAPTR,TAPTR))(dev))[-1]))(dev,msg)
#define TAbortIO(dev,msg)	\
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR))(dev))[-2]))(dev,msg)

/*****************************************************************************/
/* 
**	Internal
*/

LOCAL TBOOL exec_strequal(TSTRPTR s1, TSTRPTR s2);

LOCAL TBOOL exec_initmmu(TEXECBASE *exec, struct TMemManager *mmu,
	TAPTR allocator, TUINT mmutype, struct TTagItem *tags);
LOCAL TBOOL exec_initmemhead(union TMemHead *mh, TAPTR mem, TUINT size,
	TUINT flags, TINT bytealign);
LOCAL TBOOL exec_initport(TEXECBASE *exec, struct TMsgPort *port,
	struct TTask *task, TUINT prefsignal);
LOCAL TUINT exec_allocsignal(TEXECBASE *exec, struct TTask *task,
	TUINT signals);
LOCAL TVOID exec_freesignal(TEXECBASE *exec, struct TTask *task,
	TUINT signals);
LOCAL TBOOL exec_initlock(TEXECBASE *exec, struct TLock *lock);
LOCAL TAPTR exec_getmsg(TEXECBASE *exec, struct TMsgPort *port);
LOCAL TVOID exec_returnmsg(TEXECBASE *exec, TAPTR mem, TUINT status);
LOCAL TUINT exec_sendmsg(TEXECBASE *exec, struct TTask *task,
	struct TMsgPort *port, TAPTR mem);
LOCAL TVOID exec_panic(TEXECBASE *exec, TSTRPTR suggestion);

/*****************************************************************************/
/* 
**	External
*/

EXPORT TBOOL exec_DoExec(TEXECBASE *exec, struct TTagItem *tags);
EXPORT struct TTask *exec_CreateSysTask(TEXECBASE *exec, TTASKFUNC func,
	struct TTagItem *tags);
EXPORT TUINT exec_AllocSignal(TEXECBASE *exec, TUINT signals);
EXPORT TVOID exec_FreeSignal(TEXECBASE *exec, TUINT signal);
EXPORT struct TMsgPort *exec_CreatePort(TEXECBASE *exec,
	struct TTagItem *tags);
EXPORT TVOID exec_CloseModule(TEXECBASE *exec, struct TModule *module);
EXPORT struct TTask *exec_CreateTask(TEXECBASE *exec, TTASKFUNC function,
	TINITFUNC initfunc, struct TTagItem *tags);
EXPORT struct TAtom *exec_LockAtom(TEXECBASE *exec, TAPTR atom, TUINT mode);
EXPORT struct TMemManager *exec_CreateMMU(TEXECBASE *exec, TAPTR allocator,
	TUINT mmutype, struct TTagItem *tags);
EXPORT TUINT exec_SendMsg(TEXECBASE *exec, struct TMsgPort *port, TAPTR msg);
EXPORT TUINT exec_SetSignal(TEXECBASE *exec, TUINT newsignals, TUINT sigmask);
EXPORT TVOID exec_Signal(TEXECBASE *exec, struct TTask *task, TUINT signals);
EXPORT TUINT exec_Wait(TEXECBASE *exec, TUINT sigmask);
EXPORT TVOID exec_UnlockAtom(TEXECBASE *exec, struct TAtom *atom, TUINT mode);
EXPORT TVOID exec_Lock(TEXECBASE *exec, struct TLock *lock);
EXPORT TVOID exec_Unlock(TEXECBASE *exec, struct TLock *lock);
EXPORT TVOID exec_AckMsg(TEXECBASE *exec, TAPTR msg);
EXPORT TVOID exec_DropMsg(TEXECBASE *exec, TAPTR msg);
EXPORT TVOID exec_Free(TEXECBASE *exec, TAPTR mem);
EXPORT struct TTask *exec_FindTask(TEXECBASE *exec, TSTRPTR name);
EXPORT TAPTR exec_GetMsg(TEXECBASE *exec, struct TMsgPort *port);
EXPORT TVOID exec_CopyMem(TEXECBASE *exec, TAPTR from, TAPTR to,
	TUINT numbytes);
EXPORT TVOID exec_FillMem(TEXECBASE *exec, TAPTR dest, TUINT numbytes,
	TUINT8 fillval);
EXPORT TAPTR exec_AllocMMU(TEXECBASE *exec, struct TMemManager *mmu,
	TUINT size);
EXPORT TAPTR exec_AllocMMU0(TEXECBASE *exec, struct TMemManager *mmu,
	TUINT size);
EXPORT TAPTR exec_OpenModule(TEXECBASE *exec, TSTRPTR modname, TUINT16 version,
	struct TTagItem *tags);
EXPORT TVOID exec_PutMsg(TEXECBASE *exec, struct TMsgPort *msgport,
	struct TMsgPort *replyport, TAPTR msg);
EXPORT TAPTR exec_Realloc(TEXECBASE *exec, TAPTR mem, TUINT newsize);
EXPORT TVOID exec_ReplyMsg(TEXECBASE *exec, TAPTR msg);
EXPORT TAPTR exec_WaitPort(TEXECBASE *exec, struct TMsgPort *port);
EXPORT TAPTR exec_GetTaskMMU(TEXECBASE *exec, struct TTask *task);
EXPORT TAPTR exec_GetUserPort(TEXECBASE *exec, struct TTask *task);
EXPORT TAPTR exec_GetSyncPort(TEXECBASE *exec, struct TTask *task);
EXPORT TAPTR exec_GetTaskData(TEXECBASE *exec, struct TTask *task);
EXPORT TAPTR exec_SetTaskData(TEXECBASE *exec, struct TTask *task, TAPTR data);
EXPORT TAPTR exec_GetHALBase(TEXECBASE *exec);
EXPORT TAPTR exec_AllocMsg(TEXECBASE *exec, TUINT size);
EXPORT TAPTR exec_AllocMsg0(TEXECBASE *exec, TUINT size);
EXPORT TAPTR exec_CreateLock(TEXECBASE *exec, struct TTagItem *tags);
EXPORT TUINT exec_GetPortSignal(TEXECBASE *exec, TAPTR port);
EXPORT TTAG exec_GetAtomData(TEXECBASE *exec, struct TAtom *atom);
EXPORT TTAG exec_SetAtomData(TEXECBASE *exec, struct TAtom *atom, TTAG data);
EXPORT TUINT exec_GetSize(TEXECBASE *exec, TAPTR mem);
EXPORT TAPTR exec_GetMMU(TEXECBASE *exec, TAPTR mem);
EXPORT TAPTR exec_CreatePool(TEXECBASE *exec, struct TTagItem *tags);
EXPORT TAPTR exec_AllocPool(TEXECBASE *exec, struct TMemPool *pool,
	TUINT size);
EXPORT TVOID exec_FreePool(TEXECBASE *exec, struct TMemPool *pool, TINT8 *mem,
	TUINT size);
EXPORT TAPTR exec_ReallocPool(TEXECBASE *exec, struct TMemPool *pool,
	TINT8 *oldmem, TUINT oldsize, TUINT newsize);
EXPORT TVOID exec_FillMem32(TEXECBASE *exec, TUINT *dest, TUINT len,
	TUINT fill);
EXPORT TVOID exec_PutIO(TEXECBASE *exec, struct TIORequest *ioreq);
EXPORT TINT exec_WaitIO(TEXECBASE *exec, struct TIORequest *ioreq);
EXPORT TINT exec_DoIO(TEXECBASE *exec, struct TIORequest *ioreq);
EXPORT TBOOL exec_CheckIO(TEXECBASE *exec, struct TIORequest *ioreq);
EXPORT TINT exec_AbortIO(TEXECBASE *exec, struct TIORequest *ioreq);

/*****************************************************************************/
/* 
**	External, private
*/

EXPORT TLIST *exec_LockPort(TEXECBASE *exec, struct TMsgPort *port);
EXPORT TVOID exec_UnlockPort(TEXECBASE *exec, struct TMsgPort *port);
EXPORT TVOID exec_InsertMsg(TEXECBASE *exec, struct TMsgPort *port, TAPTR msg,
	TAPTR predmsg, TUINT status);
EXPORT TVOID exec_RemoveMsg(TEXECBASE *exec, struct TMsgPort *port, TAPTR msg);
EXPORT TUINT exec_GetMsgStatus(TEXECBASE *exec, TAPTR mem);
EXPORT struct TMsgPort *exec_SetMsgReplyPort(TEXECBASE *exec, TAPTR mem,
	struct TMsgPort *rport);

/*****************************************************************************/
/*
**	Revision History
**	$Log: exec_mod.h,v $
**	Revision 1.10  2005/09/13 02:41:58  tmueller
**	updated copyright reference
**	
**	Revision 1.9  2005/09/07 23:56:46  tmueller
**	revision bumped
**	
**	Revision 1.8  2005/01/30 06:21:38  tmueller
**	added fixed-size memory pools
**	
**	Revision 1.7  2005/01/21 11:40:35  tmueller
**	added alignment option to memheaders
**	
**	Revision 1.6  2004/06/11 17:48:39  tmueller
**	TExecLockAtom() semantics corrected - Exec version bumped to v3
**	
**	Revision 1.5  2004/04/18 16:20:30  tmueller
**	API change: bumped to version 2
**	
**	Revision 1.4  2004/04/18 14:08:52  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.3  2003/12/22 23:00:40  tmueller
**	Added private function TExecSetMsgReplyPort()
**	
**	Revision 1.2  2003/12/12 03:43:49  tmueller
**	exec_panic() made LOCAL
**	
**	Revision 1.1.1.1  2003/12/11 07:19:02  tmueller
**	Krypton import
*/

#endif
