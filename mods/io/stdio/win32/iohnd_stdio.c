
/*
**	$Id: iohnd_stdio.c,v 1.2 2005/09/26 19:01:36 tschwinger Exp $
**	iohnd_stdio - Win32 implementaton of the stdio handler
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/mod/io.h>
#include <tek/mod/ioext.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/io.h>

#include <windows.h>

/*****************************************************************************/

#define MOD_VERSION		0
#define MOD_REVISION	1

typedef struct
{
	struct TModule thnd_Module;
	TAPTR thnd_UtilBase;
	TAPTR thnd_Lock;
	TUINT thnd_RefCount;

} TMOD_HND;

#define TExecBase TGetExecBase(mod)
#define TUtilBase mod->thnd_UtilBase

typedef struct
{
	HANDLE fileno;
	TSTRPTR name;
	TINT type;

} FLOCK;

enum { TYPE_OUT, TYPE_IN, TYPE_ERR };

#define	IOBUFSIZE_DEFAULT	512

typedef struct TIOPacket TIOMSG;

static TCALLBACK TVOID mod_destroy(TMOD_HND *mod);
static TCALLBACK TIOMSG *mod_open(TMOD_HND *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_HND *mod, TAPTR task);

static TMODAPI TVOID beginio(TMOD_HND *mod, TIOMSG *msg);
static TMODAPI TINT abortio(TMOD_HND *mod, TIOMSG *msg);

/*****************************************************************************/
/*
**	module init/exit
*/

TMODENTRY TUINT 
tek_init_iohnd_stdio(TAPTR task, TMOD_HND *mod, TUINT16 version,
	TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * 2;				/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_HND);				/* positive size */

		return 0;
	}

	mod->thnd_Lock = TExecCreateLock(TExecBase, TNULL);
	if (mod->thnd_Lock)
	{		
		mod->thnd_Module.tmd_Version = MOD_VERSION;
		mod->thnd_Module.tmd_Revision = MOD_REVISION;

		mod->thnd_Module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->thnd_Module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;
		mod->thnd_Module.tmd_DestroyFunc = (TDFUNC) mod_destroy;

		((TAPTR *) mod)[-1] = (TAPTR) beginio;
		((TAPTR *) mod)[-2] = (TAPTR) abortio;

		return TTRUE;
	}

	return 0;
}

static TCALLBACK TVOID 
mod_destroy(TMOD_HND *mod)
{
	TDestroy(mod->thnd_Lock);
}

/*****************************************************************************/
/*
**	module open/close
*/

static TCALLBACK TIOMSG *
mod_open(TMOD_HND *mod, TAPTR task, TTAGITEM *tags)
{
	TIOMSG *msg;

	msg = TExecAllocMsg0(TExecBase, sizeof(TIOMSG) + sizeof(FLOCK));
	if (msg)
	{
		TExecLock(TExecBase, mod->thnd_Lock);
		
		if (mod->thnd_RefCount == 0)
		{
			mod->thnd_UtilBase = TExecOpenModule(TExecBase, "util", 0, TNULL);
		}
		
		if (mod->thnd_UtilBase)
		{
			mod->thnd_RefCount++;

			msg->io_Req.io_Device = (struct TModule *) mod;

			/* insert recommended buffer settings */
			msg->io_BufSize = IOBUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_LINE;
		}
		else
		{
			TExecCloseModule(TExecBase, mod->thnd_UtilBase);
			TExecFree(TExecBase, msg);
			msg = TNULL;
		}

		TExecUnlock(TExecBase, mod->thnd_Lock);
	}

	return msg;
}

static TCALLBACK TVOID 
mod_close(TMOD_HND *mod, TAPTR task)
{
	TExecLock(TExecBase, mod->thnd_Lock);
	if (--mod->thnd_RefCount == 0)
	{
		TExecCloseModule(TExecBase, mod->thnd_UtilBase);
	}
	TExecUnlock(TExecBase, mod->thnd_Lock);
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
			tdbprintf(99,"unhandled error\n");
			tdbfatal(99);
	}
	
	return 0;
}

/*****************************************************************************/
/*
**	hnd_open
*/

static TVOID 
hnd_open(TMOD_HND *mod, TIOMSG *msg)
{
	msg->io_Op.Open.Result = TTRUE;

	if (!TUtilStrCaseCmp(TUtilBase, msg->io_Op.Open.Name, "out"))
	{
		FLOCK *fl = (FLOCK *) (msg + 1);
		fl->fileno = GetStdHandle(STD_OUTPUT_HANDLE);
		fl->type = TYPE_OUT;
		fl->name = "out";
		msg->io_FLock = fl;
	}
	else if (!TUtilStrCaseCmp(TUtilBase, msg->io_Op.Open.Name, "in"))
	{
		FLOCK *fl = (FLOCK *) (msg + 1);
		fl->fileno = GetStdHandle(STD_INPUT_HANDLE);
		fl->type = TYPE_IN;
		fl->name = "in";
		msg->io_FLock = fl;
	}
	else if (!TUtilStrCaseCmp(TUtilBase, msg->io_Op.Open.Name, "err"))
	{
		FLOCK *fl = (FLOCK *) (msg + 1);
		fl->fileno = GetStdHandle(STD_ERROR_HANDLE);
		fl->name = "err";
		fl->type = TYPE_ERR;
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

static TVOID hnd_close(TMOD_HND *mod, TIOMSG *msg)
{
	msg->io_Op.Close.Result = TTRUE;
}

/*****************************************************************************/
/*
**	hnd_read
*/

static TVOID 
hnd_read(TMOD_HND *mod, TIOMSG *msg)
{
	TINT res = -1;
	FLOCK *fl = msg->io_FLock;
	
	if (fl->type == TYPE_IN)
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

static TVOID
hnd_write(TMOD_HND *mod, TIOMSG *msg)
{
	TINT res = -1;
	FLOCK *fl = msg->io_FLock;

	if (fl->type == TYPE_OUT || fl->type == TYPE_ERR)
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

static TVOID
hnd_docmd(TMOD_HND *mod, TIOMSG *msg)
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

static TMODAPI TVOID
beginio(TMOD_HND *mod, TIOMSG *msg)
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
		TExecReplyMsg(TExecBase, msg);
	}
}

/*****************************************************************************/
/*
**	abortio(msg->io_Device, msg)
*/

static TMODAPI TINT
abortio(TMOD_HND *mod, TIOMSG *msg)
{
	/* not supported */
	return -1;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: iohnd_stdio.c,v $
**	Revision 1.2  2005/09/26 19:01:36  tschwinger
**	Silences some GCC warnings
**	
**	Revision 1.1  2005/05/11 21:34:05  tmueller
**	added win32 stdio handler
**	
*/
