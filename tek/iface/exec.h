#ifndef _TEK_IFACE_EXEC_H
#define _TEK_IFACE_EXEC_H

/*
**	teklib/tek/iface/exec.h - exec interface
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/proto/exec.h>

struct TExecIFace
{
	struct TInterface IFace;
	TMODCALL struct THALBase *(*GetHALBase)(TAPTR TExecBase);
	TMODCALL struct TModule *(*OpenModule)(TAPTR TExecBase, TSTRPTR name, TUINT16 version, TTAGITEM *tags);
	TMODCALL void (*CloseModule)(TAPTR TExecBase, struct TModule *mod);
	TMODCALL void (*CopyMem)(TAPTR TExecBase, TAPTR src, TAPTR dst, TUINT len);
	TMODCALL void (*FillMem)(TAPTR TExecBase, TAPTR dst, TUINT len, TUINT8 val);
	TMODCALL struct TMemManager *(*CreateMemManager)(TAPTR TExecBase, TAPTR object, TUINT type, TTAGITEM *tags);
	TMODCALL TAPTR (*Alloc)(TAPTR TExecBase, struct TMemManager *mm, TSIZE size);
	TMODCALL TAPTR (*Alloc0)(TAPTR TExecBase, struct TMemManager *mm, TSIZE size);
	TMODCALL struct TInterface *(*QueryInterface)(TAPTR TExecBase, struct TModule *mod, TSTRPTR name, TUINT16 version, TTAGITEM *tags);
	TMODCALL void (*Free)(TAPTR TExecBase, TAPTR mem);
	TMODCALL TAPTR (*Realloc)(TAPTR TExecBase, TAPTR mem, TSIZE nsize);
	TMODCALL struct TMemManager *(*GetMemManager)(TAPTR TExecBase, TAPTR mem);
	TMODCALL TSIZE (*GetSize)(TAPTR TExecBase, TAPTR mem);
	TMODCALL struct TLock *(*CreateLock)(TAPTR TExecBase, TTAGITEM *tags);
	TMODCALL void (*Lock)(TAPTR TExecBase, struct TLock *lock);
	TMODCALL void (*Unlock)(TAPTR TExecBase, struct TLock *lock);
	TMODCALL TUINT (*AllocSignal)(TAPTR TExecBase, TUINT sig);
	TMODCALL void (*FreeSignal)(TAPTR TExecBase, TUINT sig);
	TMODCALL void (*Signal)(TAPTR TExecBase, struct TTask *task, TUINT sig);
	TMODCALL TUINT (*SetSignal)(TAPTR TExecBase, TUINT newsig, TUINT sigmask);
	TMODCALL TUINT (*Wait)(TAPTR TExecBase, TUINT sig);
	TMODCALL TBOOL (*StrEqual)(TAPTR TExecBase, TSTRPTR s1, TSTRPTR s2);
	TMODCALL struct TMsgPort *(*CreatePort)(TAPTR TExecBase, TTAGITEM *tags);
	TMODCALL void (*PutMsg)(TAPTR TExecBase, struct TMsgPort *port, struct TMsgPort *replyport, TAPTR msg);
	TMODCALL TAPTR (*GetMsg)(TAPTR TExecBase, struct TMsgPort *port);
	TMODCALL void (*AckMsg)(TAPTR TExecBase, TAPTR msg);
	TMODCALL void (*ReplyMsg)(TAPTR TExecBase, TAPTR msg);
	TMODCALL void (*DropMsg)(TAPTR TExecBase, TAPTR msg);
	TMODCALL TUINT (*SendMsg)(TAPTR TExecBase, struct TMsgPort *port, TAPTR msg);
	TMODCALL TAPTR (*WaitPort)(TAPTR TExecBase, struct TMsgPort *port);
	TMODCALL TUINT (*GetPortSignal)(TAPTR TExecBase, struct TMsgPort *port);
	TMODCALL struct TMsgPort *(*GetUserPort)(TAPTR TExecBase, struct TTask *task);
	TMODCALL struct TMsgPort *(*GetSyncPort)(TAPTR TExecBase, struct TTask *task);
	TMODCALL TAPTR (*CreateTask)(TAPTR TExecBase, struct THook *a, TTAGITEM *tags);
	TMODCALL TAPTR (*FindTask)(TAPTR TExecBase, TSTRPTR name);
	TMODCALL TAPTR (*GetTaskData)(TAPTR TExecBase, struct TTask *task);
	TMODCALL TAPTR (*SetTaskData)(TAPTR TExecBase, struct TTask *task, TAPTR data);
	TMODCALL TAPTR (*GetTaskMemManager)(TAPTR TExecBase, struct TTask *task);
	TMODCALL TAPTR (*AllocMsg)(TAPTR TExecBase, TSIZE size);
	TMODCALL TAPTR (*AllocMsg0)(TAPTR TExecBase, TSIZE size);
	TMODCALL struct TAtom *(*LockAtom)(TAPTR TExecBase, TAPTR atom, TUINT mode);
	TMODCALL void (*UnlockAtom)(TAPTR TExecBase, TAPTR atom, TUINT mode);
	TMODCALL TTAG (*GetAtomData)(TAPTR TExecBase, TAPTR atom);
	TMODCALL TTAG (*SetAtomData)(TAPTR TExecBase, TAPTR atom, TTAG data);
	TMODCALL struct TMemPool *(*CreatePool)(TAPTR TExecBase, TTAGITEM *tags);
	TMODCALL TAPTR (*AllocPool)(TAPTR TExecBase, struct TMemPool *pool, TSIZE size);
	TMODCALL void (*FreePool)(TAPTR TExecBase, struct TMemPool *pool, TAPTR mem, TSIZE size);
	TMODCALL TAPTR (*ReallocPool)(TAPTR TExecBase, struct TMemPool *pool, TAPTR mem, TSIZE oize, TSIZE nsize);
	TMODCALL void (*PutIO)(TAPTR TExecBase, struct TIORequest *ioreq);
	TMODCALL TINT (*WaitIO)(TAPTR TExecBase, struct TIORequest *ioreq);
	TMODCALL TINT (*DoIO)(TAPTR TExecBase, struct TIORequest *ioreq);
	TMODCALL TINT (*CheckIO)(TAPTR TExecBase, struct TIORequest *ioreq);
	TMODCALL TINT (*AbortIO)(TAPTR TExecBase, struct TIORequest *ioreq);
	TMODCALL TBOOL (*AddModules)(TAPTR TExecBase, struct TModInitNode *im, TUINT flags);
	TMODCALL TBOOL (*RemModules)(TAPTR TExecBase, struct TModInitNode *im, TUINT flags);
	TMODCALL TAPTR (*AllocTimeRequest)(TAPTR TExecBase, TTAGITEM *tags);
	TMODCALL void (*FreeTimeRequest)(TAPTR TExecBase, TAPTR req);
	TMODCALL void (*GetSystemTime)(TAPTR TExecBase, TTIME *t);
	TMODCALL TINT (*GetUniversalDate)(TAPTR TExecBase, TDATE *dt);
	TMODCALL TINT (*GetLocalDate)(TAPTR TExecBase, TDATE *dt);
	TMODCALL TUINT (*WaitTime)(TAPTR TExecBase, TTIME *t, TUINT sig);
	TMODCALL TUINT (*WaitDate)(TAPTR TExecBase, TDATE *dt, TUINT sig);
};

#endif /* _TEK_IFACE_EXEC */
