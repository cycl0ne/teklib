
/*
**	$Id: iohnd_stdio.c,v 1.1 2006/08/25 21:23:42 tmueller Exp $
**	iohnd_stdio - WinNT implementaton of the stdio handler
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/mod/io.h>
#include <tek/mod/ioext.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/io.h>

/*****************************************************************************/

#define IOSTD_VERSION		0
#define IOSTD_REVISION		1

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
	HANDLE fileno;
	TSTRPTR name;
	TINT type;

} IOSTD_FLOCK;

enum { IOSTD_TYPE_OUT, IOSTD_TYPE_IN, IOSTD_TYPE_ERR };

#define	IOSTD_IOBUFSIZE_DEFAULT	512

typedef struct TIOPacket TIOMSG;

static THOOKENTRY TTAG iostd_mod_dispatch(struct THook *hook, TAPTR obj,
	TTAG msg);
static struct TIOPacket *iostd_mod_open(IOSTD_MOD *mod, TTAGITEM *tags);
static void iostd_mod_close(IOSTD_MOD *mod);

static TMODAPI void iostd_beginio(IOSTD_MOD *mod, TIOMSG *msg);
static TMODAPI TINT iostd_abortio(IOSTD_MOD *mod, TIOMSG *msg);

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
			return sizeof(TAPTR) * 8;

		if (version <= IOSTD_VERSION)
			return sizeof(IOSTD_MOD);

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

		((TAPTR *) mod)[-3] = (TAPTR) iostd_beginio;
		((TAPTR *) mod)[-4] = (TAPTR) iostd_abortio;

		return TTRUE;
	}

	return 0;
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
	TIOMSG *msg;

	msg = TExecAllocMsg0(mod->iostd_ExecBase, sizeof(TIOMSG) + sizeof(IOSTD_FLOCK));
	if (msg)
	{
		TExecLock(mod->iostd_ExecBase, mod->iostd_Lock);

		if (mod->iostd_RefCount == 0)
			mod->iostd_UtilBase = TExecOpenModule(mod->iostd_ExecBase, "util", 0, TNULL);

		if (mod->iostd_UtilBase)
		{
			mod->iostd_RefCount++;

			msg->io_Req.io_Device = (struct TModule *) mod;

			/* insert recommended buffer settings */
			msg->io_BufSize = IOSTD_IOBUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_LINE;
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
**	ioerr = geterror()
**	translate host error to TEKlib ioerr
*/

static TINT geterror(void)
{
	DWORD err = GetLastError();
	switch (err)
	{
		case ERROR_SUCCESS:
			return 0;

		case ERROR_SEEK:
		case ERROR_INVALID_PARAMETER:
			return TIOERR_OUT_OF_RANGE;

		case ERROR_SEEK_ON_DEVICE:
			return TIOERR_OBJECT_WRONG_TYPE;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return TIOERR_OBJECT_NOT_FOUND;

		case ERROR_WRITE_PROTECT:
			return TIOERR_DISK_WRITE_PROTECTED;

		case ERROR_ACCESS_DENIED:
		case ERROR_OPEN_FAILED:
		case ERROR_INVALID_ACCESS:
			return TIOERR_ACCESS_DENIED;

		case ERROR_TOO_MANY_OPEN_FILES:
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
		case ERROR_BUFFER_OVERFLOW:
			return TIOERR_NOT_ENOUGH_MEMORY;

		case ERROR_NOT_SAME_DEVICE:
			return TIOERR_NOT_SAME_DEVICE;

		case ERROR_DISK_FULL:
		case ERROR_HANDLE_DISK_FULL:
			return TIOERR_DISK_FULL;

		case ERROR_DIR_NOT_EMPTY:
			return TIOERR_DIRECTORY_NOT_EMPTY;

		case ERROR_BUSY:
		case ERROR_PATH_BUSY:
			return TIOERR_OBJECT_IN_USE;

		case ERROR_ALREADY_EXISTS:
			return TIOERR_OBJECT_EXISTS;

		case ERROR_FILENAME_EXCED_RANGE:
		case ERROR_INVALID_NAME:
		case ERROR_BAD_PATHNAME:
		case ERROR_DIRECTORY:
			return TIOERR_INVALID_NAME;

		case ERROR_NO_MEDIA_IN_DRIVE:
		case ERROR_MEDIA_CHANGED:
		case ERROR_INVALID_DRIVE:
		case ERROR_UNRECOGNIZED_VOLUME:
		case ERROR_NOT_READY:
		case ERROR_NOT_DOS_DISK:
		case ERROR_DRIVE_LOCKED:
			return TIOERR_DISK_NOT_READY;

		case ERROR_READ_FAULT:
		case ERROR_WRITE_FAULT:
		case ERROR_FILE_CORRUPT:
		case ERROR_DISK_CORRUPT:
		case ERROR_DISK_OPERATION_FAILED:
			return TIOERR_DISK_CORRUPT;

		case ERROR_FILE_EXISTS:
			return TIOERR_OBJECT_EXISTS;

		case ERROR_TOO_MANY_LINKS:
			return TIOERR_TOO_MANY_LEVELS;

		default:
			TDBPRINTF(99,("unhandled error\n"));
			TDBFATAL();
	}

	return 0;
}

/*****************************************************************************/
/*
**	hnd_open
*/

static void
hnd_open(IOSTD_MOD *mod, TIOMSG *msg)
{
	msg->io_Op.Open.Result = TTRUE;

	if (!TStrCaseCmp(msg->io_Op.Open.Name, "out"))
	{
		IOSTD_FLOCK *fl = (IOSTD_FLOCK *) (msg + 1);
		fl->fileno = GetStdHandle(STD_OUTPUT_HANDLE);
		fl->type = IOSTD_TYPE_OUT;
		fl->name = "out";
		msg->io_FLock = fl;
	}
	else if (!TStrCaseCmp(msg->io_Op.Open.Name, "in"))
	{
		IOSTD_FLOCK *fl = (IOSTD_FLOCK *) (msg + 1);
		fl->fileno = GetStdHandle(STD_INPUT_HANDLE);
		fl->type = IOSTD_TYPE_IN;
		fl->name = "in";
		msg->io_FLock = fl;
	}
	else if (!TStrCaseCmp(msg->io_Op.Open.Name, "err"))
	{
		IOSTD_FLOCK *fl = (IOSTD_FLOCK *) (msg + 1);
		fl->fileno = GetStdHandle(STD_ERROR_HANDLE);
		fl->name = "err";
		fl->type = IOSTD_TYPE_ERR;
		msg->io_FLock = fl;
		msg->io_BufFlags = TIOBUF_LINE;
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_NOT_FOUND;
		msg->io_Op.Open.Result = TFALSE;
	}
}

/*****************************************************************************/
/*
**	hnd_close
*/

static void hnd_close(IOSTD_MOD *mod, TIOMSG *msg)
{
	msg->io_Op.Close.Result = TTRUE;
}

/*****************************************************************************/
/*
**	hnd_read
*/

static void
hnd_read(IOSTD_MOD *mod, TIOMSG *msg)
{
	TINT res = -1;
	IOSTD_FLOCK *fl = msg->io_FLock;

	if (fl->type == IOSTD_TYPE_IN)
	{
		TUINT rdlen;
		TBOOL success = ReadFile(fl->fileno, msg->io_Op.Read.Buf,
			msg->io_Op.Read.Len, (LPDWORD) &rdlen, TNULL);
		if (success)
			res = rdlen;
		else
			msg->io_Req.io_Error = geterror();
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
	}

	msg->io_Op.Read.RdLen = res;
}

/*****************************************************************************/
/*
**	hnd_write
*/

static void
hnd_write(IOSTD_MOD *mod, TIOMSG *msg)
{
	TINT res = -1;
	IOSTD_FLOCK *fl = msg->io_FLock;

	if (fl->type == IOSTD_TYPE_OUT || fl->type == IOSTD_TYPE_ERR)
	{
		TUINT wrlen;
		TBOOL success = WriteFile(fl->fileno, msg->io_Op.Write.Buf,
			msg->io_Op.Write.Len, (LPDWORD) &wrlen, TNULL);
		if (success)
			res = wrlen;
		else
			msg->io_Req.io_Error = geterror();
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
	}

	msg->io_Op.Write.WrLen = res;
}

/*****************************************************************************/
/*
**	hnd_docmd
*/

static void
hnd_docmd(IOSTD_MOD *mod, TIOMSG *msg)
{
	msg->io_Req.io_Error = 0;

	switch (msg->io_Req.io_Command)
	{
		default:
			msg->io_Req.io_Error = TIOERR_UNKNOWN_COMMAND;
			break;

		case TIOCMD_OPEN:
			hnd_open(mod, msg);
			break;

		case TIOCMD_CLOSE:
			hnd_close(mod, msg);
			break;

		case TIOCMD_READ:
			hnd_read(mod, msg);
			break;

		case TIOCMD_WRITE:
			hnd_write(mod, msg);
			break;
	}
}

/*****************************************************************************/
/*
**	beginio(msg->io_Device, msg)
*/

static TMODAPI void
iostd_beginio(IOSTD_MOD *mod, TIOMSG *msg)
{
	hnd_docmd(mod, msg);

	if (msg->io_Req.io_Flags & TIOF_QUICK)
	{
		/* done synchronously */
		msg->io_Req.io_Flags &= ~TIOF_QUICK;
	}
	else
	{
		/* fake asynchronoucy: ackmsg to ourselves */
		TExecReplyMsg(mod->iostd_ExecBase, msg);
	}
}

/*****************************************************************************/
/*
**	abortio(msg->io_Device, msg)
*/

static TMODAPI TINT
iostd_abortio(IOSTD_MOD *mod, TIOMSG *msg)
{
	/* not supported */
	return -1;
}
