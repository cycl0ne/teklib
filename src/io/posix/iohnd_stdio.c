
/*
**	$Id: iohnd_stdio.c,v 1.6 2006/11/11 14:19:10 tmueller Exp $
**	teklib/src/io/posix/iohnd_stdio.c - POSIX stdio handler implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/io.h>

#include <tek/mod/io.h>
#include <tek/mod/ioext.h>
#include <errno.h>
#include <unistd.h>

/*****************************************************************************/

#define IOSTD_VERSION		0
#define IOSTD_REVISION		3

typedef struct
{
	struct TModule iostd_Module;
	TAPTR iostd_ExecBase;
	TAPTR iostd_UtilBase;
	TAPTR iostd_Lock;
	TUINT iostd_RefCount;
} IOSTD_MOD;

typedef struct
{
	int iostd_FileNo;
	TSTRPTR iostd_Name;
} IOSTD_FLOCK;

#define	IOSTD_BUFSIZE_DEFAULT	512

static THOOKENTRY TTAG iostd_mod_dispatch(struct THook *hook, TAPTR obj,
	TTAG msg);
static struct TIOPacket *iostd_mod_open(IOSTD_MOD *mod, TTAGITEM *tags);
static void iostd_mod_close(IOSTD_MOD *mod);

static TMODAPI void iostd_beginio(IOSTD_MOD *mod, struct TIOPacket *msg);
static TMODAPI TINT iostd_abortio(IOSTD_MOD *mod, struct TIOPacket *msg);

/*****************************************************************************/
/*
**	module init/exit
*/

TMODENTRY TUINT
tek_init_iohnd_stdio(struct TTask *task, struct TModule *stdio,
	TUINT16 version, TTAGITEM *tags)
{
	IOSTD_MOD *mod = (IOSTD_MOD *) stdio;

	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TMFPTR) * 8;	/* negative size */

		if (version <= IOSTD_VERSION)
			return sizeof(IOSTD_MOD);	/* positive size */

		return 0;
	}

	mod->iostd_ExecBase = TGetExecBase(mod);
	mod->iostd_Lock = TExecCreateLock(mod->iostd_ExecBase, TNULL);
	if (mod->iostd_Lock)
	{
		mod->iostd_Module.tmd_Version = IOSTD_VERSION;
		mod->iostd_Module.tmd_Revision = IOSTD_REVISION;
		mod->iostd_Module.tmd_Handle.thn_Hook.thk_Entry = iostd_mod_dispatch;
		mod->iostd_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;

		((TMFPTR *) mod)[-3] = (TMFPTR) iostd_beginio;
		((TMFPTR *) mod)[-4] = (TMFPTR) iostd_abortio;

		return TTRUE;
	}

	return TFALSE;
}

static THOOKENTRY TTAG
iostd_mod_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	IOSTD_MOD *mod = (IOSTD_MOD *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			TDestroy(mod->iostd_Lock);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) iostd_mod_open(mod, obj);
		case TMSG_CLOSEMODULE:
			iostd_mod_close(obj);
	}
	return 0;
}

/*****************************************************************************/
/*
**	module open/close
*/

static struct TIOPacket *
iostd_mod_open(IOSTD_MOD *mod, TTAGITEM *tags)
{
	struct TIOPacket *msg = TExecAllocMsg0(mod->iostd_ExecBase,
		sizeof(struct TIOPacket) + sizeof(IOSTD_FLOCK));
	if (msg)
	{
		TExecLock(mod->iostd_ExecBase, mod->iostd_Lock);

		if (mod->iostd_RefCount == 0)
			mod->iostd_UtilBase =
				TExecOpenModule(mod->iostd_ExecBase, "util", 0, TNULL);

		if (mod->iostd_UtilBase)
		{
			mod->iostd_RefCount++;

			/* insert recommended buffer settings */
			msg->io_BufSize = IOSTD_BUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_LINE;
			msg->io_Req.io_Device = (struct TModule *) mod;
		}
		else
		{
			TExecCloseModule(mod->iostd_ExecBase, mod->iostd_UtilBase);
			TExecFree(mod->iostd_ExecBase, msg);
			msg = TNULL;
		}

		TExecUnlock(mod->iostd_ExecBase, mod->iostd_Lock);
	}

	return msg;
}

static void
iostd_mod_close(IOSTD_MOD *mod)
{
	TExecLock(mod->iostd_ExecBase, mod->iostd_Lock);
	if (--mod->iostd_RefCount == 0)
		TExecCloseModule(mod->iostd_ExecBase, mod->iostd_UtilBase);
	TExecUnlock(mod->iostd_ExecBase, mod->iostd_Lock);
}

/*****************************************************************************/
/*
**	ioerr = geterror(errno)
**	translate host errno to TEKlib ioerr
*/

static TINT
iostd_geterror(int err)
{
	switch (err)
	{
		default:
			TDBPRINTF(20,("unhandled error: %d\n", err));
			TDBFATAL();

		case EIO:
			return TIOERR_DISK_CORRUPT;

		case EISDIR:
		case EBADF:
			return TIOERR_OBJECT_WRONG_TYPE;
	}

	return 0;
}

/*****************************************************************************/
/*
**	iostd_open
*/

static void
iostd_open(IOSTD_MOD *mod, struct TIOPacket *msg)
{
	IOSTD_FLOCK *fl = (IOSTD_FLOCK *) (msg + 1);
	msg->io_Op.Open.Result = TTRUE;
	msg->io_FLock = fl;

	if (!TStrCaseCmp(msg->io_Op.Open.Name, "out"))
	{
		fl->iostd_FileNo = STDOUT_FILENO;
		fl->iostd_Name = "out";
	}
	else if (!TStrCaseCmp(msg->io_Op.Open.Name, "in"))
	{
		fl->iostd_FileNo = STDIN_FILENO;
		fl->iostd_Name = "in";
	}
	else if (!TStrCaseCmp(msg->io_Op.Open.Name,
		"err"))
	{
		fl->iostd_FileNo = STDERR_FILENO;
		fl->iostd_Name = "err";
		msg->io_BufFlags = TIOBUF_LINE;
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_NOT_FOUND;
		msg->io_Op.Open.Result = TFALSE;
		msg->io_FLock = TNULL;
	}
}

/*****************************************************************************/
/*
**	iostd_close
*/

static void
iostd_close(IOSTD_MOD *mod, struct TIOPacket *msg)
{
	msg->io_Op.Close.Result = TTRUE;
}

/*****************************************************************************/
/*
**	iostd_read
*/

static void
iostd_read(IOSTD_MOD *mod, struct TIOPacket *msg)
{
	TINT res = -1;
	IOSTD_FLOCK *fl = msg->io_FLock;

	if (fl->iostd_FileNo == STDIN_FILENO)
	{
		res = (TINT) read(fl->iostd_FileNo, msg->io_Op.Read.Buf,
			msg->io_Op.Read.Len);
		if (res == -1)
			msg->io_Req.io_Error = iostd_geterror(errno);
	}
	else
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;

	msg->io_Op.Read.RdLen = res;
}

/*****************************************************************************/
/*
**	iostd_write
*/

static void
iostd_write(IOSTD_MOD *mod, struct TIOPacket *msg)
{
	TINT res = -1;
	IOSTD_FLOCK *fl = msg->io_FLock;

	if (fl->iostd_FileNo == STDOUT_FILENO || fl->iostd_FileNo == STDERR_FILENO)
	{
		res = (TINT) write(fl->iostd_FileNo, msg->io_Op.Write.Buf,
			msg->io_Op.Write.Len);
		if (res == -1)
			msg->io_Req.io_Error = iostd_geterror(errno);
	}
	else
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;

	msg->io_Op.Write.WrLen = res;
}

/*****************************************************************************/
/*
**	iostd_docmd
*/

static void
iostd_docmd(IOSTD_MOD *mod, struct TIOPacket *msg)
{
	msg->io_Req.io_Error = 0;

	switch (msg->io_Req.io_Command)
	{
		default:
			msg->io_Req.io_Error = TIOERR_UNKNOWN_COMMAND;
			break;

		case TIOCMD_OPEN:
			iostd_open(mod, msg);
			break;

		case TIOCMD_CLOSE:
			iostd_close(mod, msg);
			break;

		case TIOCMD_READ:
			iostd_read(mod, msg);
			break;

		case TIOCMD_WRITE:
			iostd_write(mod, msg);
			break;
	}
}

/*****************************************************************************/
/*
**	beginio(msg->io_Device, msg)
*/

static TMODAPI void
iostd_beginio(IOSTD_MOD *mod, struct TIOPacket *msg)
{
	iostd_docmd(mod, msg);

	if (msg->io_Req.io_Flags & TIOF_QUICK)
	{
		/* done synchronously */
		msg->io_Req.io_Flags &= ~TIOF_QUICK;
	}
	else
	{
		/* fake asynchronoucy: reply to ourselves */
		TExecReplyMsg(mod->iostd_ExecBase, msg);
	}
}

/*****************************************************************************/
/*
**	abortio(msg->io_Device, msg)
*/

static TMODAPI TINT
iostd_abortio(IOSTD_MOD *mod, struct TIOPacket *msg)
{
	/* not supported */
	return -1;
}
