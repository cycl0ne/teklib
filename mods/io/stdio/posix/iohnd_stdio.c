
/*
**	$Id: iohnd_stdio.c,v 1.3 2004/09/09 19:31:58 tmueller Exp $
**	iohnd_stdio - POSIX implementaton of the stdio handler
*/

#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/io.h>
#include <tek/debug.h>
#include <tek/teklib.h>

#include <tek/mod/io.h>
#include <tek/mod/ioext.h>
#include <errno.h>
#include <unistd.h>

#define MOD_VERSION		0
#define MOD_REVISION	2

typedef struct
{
	struct TModule module;
	TAPTR util;
	TAPTR lock;
	TUINT refcount;

} TMOD_HND;

#define TExecBase TGetExecBase(mod)

typedef struct
{
	int fileno;
	TSTRPTR name;

} FLOCK;

#define	IOBUFSIZE_DEFAULT	512

typedef struct TIOPacket TIOMSG;

static TCALLBACK TVOID mod_destroy(TMOD_HND *mod);
static TCALLBACK TIOMSG *mod_open(TMOD_HND *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_HND *mod, TAPTR task);

static TMODAPI TVOID beginio(TMOD_HND *mod, TIOMSG *msg);
static TMODAPI TINT abortio(TMOD_HND *mod, TIOMSG *msg);

/**************************************************************************
**
**	module init/exit
*/

TMODENTRY TUINT tek_init_iohnd_stdio(TAPTR task, TMOD_HND *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)					/* first call */
		{
			if (version <= MOD_VERSION)			/* version check */
			{
				return sizeof(TMOD_HND);		/* return module positive size */
			}
		}
		else									/* second call */
		{
			return sizeof(TAPTR) * 2;
		}
	}
	else										/* third call */
	{
		mod->lock = TExecCreateLock(TExecBase, TNULL);
		if (mod->lock)
		{		
			mod->module.tmd_Version = MOD_VERSION;
			mod->module.tmd_Revision = MOD_REVISION;
			mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
			mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;
			mod->module.tmd_DestroyFunc = (TDFUNC) mod_destroy;

			((TAPTR *) mod)[-1] = (TAPTR) beginio;
			((TAPTR *) mod)[-2] = (TAPTR) abortio;

			return TTRUE;
		}
	}

	return 0;
}

static TCALLBACK TVOID mod_destroy(TMOD_HND *mod)
{
	TDestroy(mod->lock);
}


/**************************************************************************
**
**	module open/close
*/

static TCALLBACK TIOMSG *mod_open(TMOD_HND *mod, TAPTR task, TTAGITEM *tags)
{
	TIOMSG *msg;

	msg = TExecAllocMsg0(TExecBase, sizeof(TIOMSG) + sizeof(FLOCK));
	if (msg)
	{
		TExecLock(TExecBase, mod->lock);
		
		if (mod->refcount == 0)
		{
			mod->util = TExecOpenModule(TExecBase, "util", 0, TNULL);
		}
		
		if (mod->util)
		{
			mod->refcount++;

			/* insert recommended buffer settings */
			msg->io_BufSize = IOBUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_LINE;
	
			msg->io_Req.io_Device = (struct TModule *) mod;
		}
		else
		{
			TExecCloseModule(TExecBase, mod->util);
			TExecFree(TExecBase, msg);
			msg = TNULL;
		}

		TExecUnlock(TExecBase, mod->lock);
	}

	return msg;
}


static TCALLBACK TVOID mod_close(TMOD_HND *mod, TAPTR task)
{
	TExecLock(TExecBase, mod->lock);
	if (--mod->refcount == 0)
	{
		TExecCloseModule(TExecBase, mod->util);
	}
	TExecUnlock(TExecBase, mod->lock);
}


/**************************************************************************
**
**	ioerr = geterror(errno)
**	translate host errno to TEKlib ioerr
*/

static TINT geterror(int err)
{
	switch (err)
	{
		default:
			tdbprintf1(20,"unhandled error: %d\n", err);
			tdbfatal(99);

		case EIO:
			return TIOERR_DISK_CORRUPT;

		case EISDIR:
		case EBADF:
			return TIOERR_OBJECT_WRONG_TYPE;
	}

	return 0;
}


/**************************************************************************
**
**	hnd_open
*/

static TVOID hnd_open(TMOD_HND *mod, TIOMSG *msg)
{
	msg->io_Op.Open.Result = TTRUE;

	if (!TUtilStrCaseCmp(mod->util, msg->io_Op.Open.Name, "out"))
	{
		FLOCK *fl = (FLOCK *) (msg + 1);
		fl->fileno = STDOUT_FILENO;
		fl->name = "out";
		msg->io_FLock = fl;
	}
	else if (!TUtilStrCaseCmp(mod->util, msg->io_Op.Open.Name, "in"))
	{
		FLOCK *fl = (FLOCK *) (msg + 1);
		fl->fileno = STDIN_FILENO;
		fl->name = "in";
		msg->io_FLock = fl;
	}
	else if (!TUtilStrCaseCmp(mod->util, msg->io_Op.Open.Name, "err"))
	{
		FLOCK *fl = (FLOCK *) (msg + 1);
		fl->fileno = STDERR_FILENO;
		fl->name = "err";
		msg->io_FLock = fl;
		msg->io_BufFlags = TIOBUF_LINE;
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_NOT_FOUND;
		msg->io_Op.Open.Result = TFALSE;
	}
}


/**************************************************************************
**
**	hnd_close
*/

static TVOID hnd_close(TMOD_HND *mod, TIOMSG *msg)
{
	msg->io_Op.Close.Result = TTRUE;
}


/**************************************************************************
**
**	hnd_read
*/

static TVOID hnd_read(TMOD_HND *mod, TIOMSG *msg)
{
	TINT res = -1;
	FLOCK *fl = msg->io_FLock;
	
	if (fl->fileno == STDIN_FILENO)
	{
		res = (TINT) read(fl->fileno, msg->io_Op.Read.Buf, msg->io_Op.Read.Len);
		if (res == -1)
		{
			msg->io_Req.io_Error = geterror(errno);
		}
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
	}

	msg->io_Op.Read.RdLen = res;
}


/**************************************************************************
**
**	hnd_write
*/

static TVOID hnd_write(TMOD_HND *mod, TIOMSG *msg)
{
	TINT res = -1;
	FLOCK *fl = msg->io_FLock;

	if (fl->fileno == STDOUT_FILENO || fl->fileno == STDERR_FILENO)
	{
		res = (TINT) write(fl->fileno, msg->io_Op.Write.Buf, msg->io_Op.Write.Len);
		if (res == -1)
		{
			msg->io_Req.io_Error = geterror(errno);
		}
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
	}

	msg->io_Op.Write.WrLen = res;
}


/**************************************************************************
**
**	hnd_docmd
*/

static TVOID hnd_docmd(TMOD_HND *mod, TIOMSG *msg)
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


/**************************************************************************
**
**	beginio(msg->io_Device, msg)
*/

static TMODAPI TVOID beginio(TMOD_HND *mod, TIOMSG *msg)
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


/**************************************************************************
**
**	abortio(msg->io_Device, msg)
*/

static TMODAPI TINT abortio(TMOD_HND *mod, TIOMSG *msg)
{
	/* not supported */
	return -1;
}


/*
**	Revision History
**	$Log: iohnd_stdio.c,v $
**	Revision 1.3  2004/09/09 19:31:58  tmueller
**	stderr handle is now line-buffered by default
**	
**	Revision 1.2  2003/12/22 22:55:19  tmueller
**	Renamed field names in I/O packets to uppercase
**	
**	Revision 1.1.1.1  2003/12/11 07:18:39  tmueller
**	Krypton import
**	
**	Revision 1.7  2003/10/19 03:38:18  tmueller
**	Changed exec structure field names: Node, List, Handle, Module, ModuleEntry
**	
**	Revision 1.6  2003/10/18 21:17:37  tmueller
**	Adapted to the changed mod->handle field names
**	
**	Revision 1.5  2003/10/16 17:39:54  tmueller
**	Default stdout buffer mode is now TIOBUF_LINE
**	
**	Revision 1.4  2003/07/11 19:33:10  tmueller
**	added posix stdio handler
**	
**	Revision 1.3  2003/05/11 14:11:34  tmueller
**	Updated I/O master and default implementations to extended definitions
**	
**	Revision 1.2  2003/03/22 05:22:38  tmueller
**	io_seek() now handles 64bit offsets. The function prototype changed. The
**	return value is now unsigned. See the documentation of io_seek() for the
**	implications of the API change. TIOERR_SEEK_ERROR has been removed,
**	TIOERR_OUT_OF_RANGE was added.
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.1  2003/01/20 21:44:20  tmueller
**	added
**	
*/
