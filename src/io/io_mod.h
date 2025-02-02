
#ifndef _TEK_MOD_IO_MOD_H
#define _TEK_MOD_IO_MOD_H

/*
**	$Id: io_mod.h,v 1.2 2006/09/10 14:51:00 tmueller Exp $
**	IO module internal definitions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/io.h>
#include <tek/mod/ioext.h>
#include <tek/mod/exec.h>
#include <tek/mod/time.h>
#include <tek/proto/exec.h>
#include <tek/debug.h>
#include <tek/teklib.h>

/*****************************************************************************/

#define IO_VERSION			0
#define IO_REVISION			3
#define IO_NUMVECTORS		50

/*****************************************************************************/

#ifndef LOCAL
#define LOCAL	TMODINTERN
#endif

#ifndef EXPORT
#define EXPORT	TMODAPI
#endif

/*****************************************************************************/

typedef struct
{
	struct TModule module;	/* module header */

	TAPTR modlock;		/* module base lock */
	TUINT refcount;		/* module reference counter */
	TAPTR mmu;			/* module's memory manager */

	struct TList devicelist;	/* list of handlers and assigns */
	struct TList cdlocklist;	/* currentdir locks that we maintain */
	struct TList stdfhlist;		/* list of standard filehandles */

} TMOD_IO;

typedef struct
{
	TUINT mode;
	TINT size;
	TINT pos;
	TINT fill;

} TIOBUF;

/* module name prefix: */
#define IOHNDNAME			"iohnd_"
#define IOHNDNAME_LEN		6
/* name of the default handler: */
#define IOHNDNAME_DEFAULT	"default"
#define IOHNDNAME_DEFLEN	7

/*****************************************************************************/
/*
**	devicenode structure. currently supported types and flags:
**
**	TDEVF_HANDLER	- identifies a handler (can reside on a physical
**		device; a link to it would be required in devnode->device).
**		a handler implements or is an abstraction from a file system
**		(or even a device, in some cases).
**	TDEVF_ASSIGN	- named alias to a directory.
**	TDEVF_DEFER		- late-binding; lock is TNULL while set
*/

struct TDeviceNode
{
	struct TNode node;
	/* device entry name (without colon): */
	TSTRPTR name;
	/* assigned path/name (with colon): */
	TSTRPTR altname;
	/* device root (handlers) or directory (assigns): */
	struct TIOPacket *lock;
	/* link to a exec device: */
	TAPTR device;
	/* see below: */
	TUINT flags;
};

#define TDEVF_MOUNT		0x0001
/* e.g. "sys" -> altname = ":opt/tek": */
#define TDEVF_ASSIGN	0x0002
/* late-binding; should work for both assigns and handlers: */
#define TDEVF_DEFER		0x0004

/*****************************************************************************/
/*
**	module API
*/

/* utils */

LOCAL TINT io_strlen(TSTRPTR s);
LOCAL TSTRPTR io_strchr(TSTRPTR s, TINT c);
LOCAL TSTRPTR io_strcpy(TSTRPTR d, TSTRPTR s);
LOCAL TSTRPTR io_strdup(TMOD_IO *io, TAPTR mmu, TSTRPTR s);
LOCAL TINT io_strncasecmp(TSTRPTR s1, TSTRPTR s2, TINT count);

/* names */

EXPORT struct TIOPacket *io_ObtainMsg(TMOD_IO *io, TSTRPTR name, TSTRPTR *namepart);
EXPORT void io_ReleaseMsg(TMOD_IO *io, struct TIOPacket *iomsg);
EXPORT TINT io_AddPart(TMOD_IO *io, TSTRPTR path1, TSTRPTR path2, TSTRPTR buf, TINT buflen);
EXPORT TBOOL io_AssignLate(TMOD_IO *io, TSTRPTR name, TSTRPTR path);
EXPORT TBOOL io_AssignLock(TMOD_IO *io, TSTRPTR name, struct TIOPacket *lock);
LOCAL struct TIOPacket *io_refhandler(TMOD_IO *io, struct TIOPacket *iomsg);
EXPORT TINT io_MakeName(TMOD_IO *io, TSTRPTR name, TSTRPTR destbuf, TINT destlen, TINT mode, TTAGITEM *tags);
EXPORT TBOOL io_Mount(TMOD_IO *io, TSTRPTR name, TINT action, TTAGITEM *tags);

/* filelock */

EXPORT TAPTR io_DupLock(TMOD_IO *io, struct TIOPacket *lock);
EXPORT TAPTR io_ParentDir(TMOD_IO *io, struct TIOPacket *lock);
EXPORT TAPTR io_LockFile(TMOD_IO *io, TSTRPTR name, TUINT mode, TTAGITEM *tags);
EXPORT void io_UnlockFile(TMOD_IO *io, struct TIOPacket *lock);
EXPORT TAPTR io_OpenFromLock(TMOD_IO *io, struct TIOPacket *lock);
EXPORT TINT io_Examine(TMOD_IO *io, struct TIOPacket *iomsg, TTAGITEM *tags);
EXPORT TINT io_ExNext(TMOD_IO *io, struct TIOPacket *iomsg, TTAGITEM *tags);
EXPORT TINT io_NameOf(TMOD_IO *io, struct TIOPacket *iomsg, TSTRPTR buf, TINT len);
EXPORT struct TIOPacket *io_ChangeDir(TMOD_IO *io, struct TIOPacket *newcd);
EXPORT TBOOL io_Rename(TMOD_IO *io, TSTRPTR oldname, TSTRPTR newname);
EXPORT TAPTR io_MakeDir(TMOD_IO *io, TSTRPTR name, TTAGITEM *tags);
EXPORT TBOOL io_Delete(TMOD_IO *io, TSTRPTR name);
EXPORT TAPTR io_OutputFH(TMOD_IO *io);
EXPORT TAPTR io_InputFH(TMOD_IO *io);
EXPORT TAPTR io_ErrorFH(TMOD_IO *io);
EXPORT TBOOL io_SetDate(TMOD_IO *io, TSTRPTR name, TDATE *date, TTAGITEM *tags);

/* standard */

EXPORT TINT io_SetIOErr(TMOD_IO *io, TINT errorcode);
EXPORT TINT io_GetIOErr(TMOD_IO *io);
EXPORT TAPTR io_OpenFile(TMOD_IO *io, TSTRPTR name, TUINT mode, TTAGITEM *tags);
EXPORT TBOOL io_CloseFile(TMOD_IO *io, struct TIOPacket *fh);
EXPORT TINT io_Read(TMOD_IO *io, struct TIOPacket *iomsg, TAPTR buf, TINT len);
EXPORT TINT io_Write(TMOD_IO *io, struct TIOPacket *iomsg, TAPTR buf, TINT len);
EXPORT TUINT io_Seek(TMOD_IO *io, struct TIOPacket *iomsg, TINT offs, TINT *offs_hi, TINT mode);
EXPORT TBOOL io_Flush(TMOD_IO *io, struct TIOPacket *iomsg);
EXPORT TINT io_FPutC(TMOD_IO *io, struct TIOPacket *iomsg, TINT c);
EXPORT TINT io_FGetC(TMOD_IO *io, struct TIOPacket *iomsg);
EXPORT TBOOL io_FEoF(TMOD_IO *io, struct TIOPacket *iomsg);
EXPORT TINT io_FRead(TMOD_IO *io, struct TIOPacket *iomsg, TUINT8 *buf, TINT len);
EXPORT TINT io_FWrite(TMOD_IO *io, struct TIOPacket *iomsg, TUINT8 *buf, TINT len);
EXPORT TINT io_Fault(TMOD_IO *io, TINT err, TSTRPTR buf, TINT buflen, TTAGITEM *tags);
EXPORT TBOOL io_WaitChar(TMOD_IO *io, struct TIOPacket *file, TINT timeout);
EXPORT TBOOL io_IsInteractive(TMOD_IO *io, struct TIOPacket *file);
EXPORT TINT io_FUngetC(TMOD_IO *io, struct TIOPacket *iomsg, TINT c);
EXPORT TINT io_FPutS(TMOD_IO *io, struct TIOPacket *iomsg, TSTRPTR s);
EXPORT TSTRPTR io_FGetS(TMOD_IO *io, struct TIOPacket *iomsg, TSTRPTR buf, TINT len);

#endif
