
/*
**	$Id: iohnd_stdio.c,v 1.2 2005/05/13 14:52:27 tmueller Exp $
**	teklib/mods/io/stdio/amiga/iohnd_stdio.c - Amiga stdio implementaton
**
**	TODO
**	* stderr: Open("CONSOLE:", MODE_NEWFILE)?
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/mod/ioext.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>

#include <exec/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/dos.h>

/*****************************************************************************/

#define MOD_VERSION		0
#define MOD_REVISION	1

void kprintf(char *, ...);

typedef struct
{
	struct TModule thnd_Module;
	TAPTR thnd_UtilBase;
	TAPTR thnd_Lock;
	TUINT thnd_RefCount;
	TAPTR thnd_AmigaDosBase;
} TMOD_HND;

#define SysBase *((struct ExecBase **) 4L)
#define DOSBase mod->thnd_AmigaDosBase

#define TExecBase TGetExecBase(mod)
#define TUtilBase mod->thnd_UtilBase

typedef struct
{
	TINT type;
	TSTRPTR name;
	BPTR fileno;
	TUINT8 printbuf[256];
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

	mod->thnd_Lock = TCreateLock(TNULL);
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

	msg = TAllocMsg0(sizeof(TIOMSG) + sizeof(FLOCK));
	if (msg)
	{
		TLock(mod->thnd_Lock);
		
		if (mod->thnd_RefCount == 0)
		{
			TUtilBase = TOpenModule("util", 0, TNULL);
			DOSBase = (struct DosLibrary *) OpenLibrary("dos.library", 0);
		}
		
		if (TUtilBase && DOSBase)
		{
			mod->thnd_RefCount++;

			msg->io_Req.io_Device = (struct TModule *) mod;

			/* insert recommended buffer settings */
			msg->io_BufSize = IOBUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_LINE;
		}
		else
		{
			CloseLibrary(DOSBase);
			TCloseModule(TUtilBase);
			TFree(msg);
			msg = TNULL;
		}

		TUnlock(mod->thnd_Lock);
	}

	return msg;
}

static TCALLBACK TVOID 
mod_close(TMOD_HND *mod, TAPTR task)
{
	TLock(mod->thnd_Lock);
	if (--mod->thnd_RefCount == 0)
	{
		CloseLibrary(DOSBase);
		TCloseModule(TUtilBase);
	}
	TUnlock(mod->thnd_Lock);
}

/*****************************************************************************/
/*
**	ioerr = geterror(mod)
**	translate host ioerr to TEKlib ioerr
*/

static TINT geterror(TMOD_HND *mod)
{
	LONG err = IoErr();
	switch (err)
	{
		case ERROR_INVALID_COMPONENT_NAME:
			return TIOERR_INVALID_NAME;
	
		case ERROR_OBJECT_IN_USE:
			return TIOERR_OBJECT_IN_USE;
	
		case ERROR_DISK_FULL:
			return TIOERR_DISK_FULL;
	
		case ERROR_NO_FREE_STORE:
			return TIOERR_NOT_ENOUGH_MEMORY;

		case ERROR_DISK_WRITE_PROTECTED:
			return TIOERR_DISK_WRITE_PROTECTED;
			
		case ERROR_READ_PROTECTED:
		case ERROR_WRITE_PROTECTED:
		case ERROR_DELETE_PROTECTED:
			return TIOERR_ACCESS_DENIED;

		case ERROR_DIR_NOT_FOUND:
		case ERROR_OBJECT_NOT_FOUND:
			return TIOERR_OBJECT_NOT_FOUND;

		case ERROR_TOO_MANY_LEVELS:
			return TIOERR_TOO_MANY_LEVELS;

		case ERROR_OBJECT_EXISTS:
			return TIOERR_OBJECT_EXISTS;

		case ERROR_OBJECT_TOO_LARGE:
			return TIOERR_OBJECT_TOO_LARGE;
		
		case ERROR_FILE_NOT_OBJECT:
		case ERROR_OBJECT_WRONG_TYPE:
			return TIOERR_OBJECT_WRONG_TYPE;
	
		case ERROR_DIRECTORY_NOT_EMPTY:
			return TIOERR_DIRECTORY_NOT_EMPTY;

		case ERROR_RENAME_ACROSS_DEVICES:
			return TIOERR_NOT_SAME_DEVICE;

		case ERROR_SEEK_ERROR:
			return TIOERR_OUT_OF_RANGE;

		case ERROR_DEVICE_NOT_MOUNTED:
		case ERROR_NO_DISK:
			return TIOERR_DISK_NOT_READY;

		case ERROR_DISK_NOT_VALIDATED:
		case ERROR_NOT_A_DOS_DISK:
			return TIOERR_DISK_CORRUPT;
			
		case ERROR_NO_MORE_ENTRIES:
			return TIOERR_NO_MORE_ENTRIES;
			
		default:
			tdbprintf1(99,"unhandled host error: %d\n", err);
			tdbfatal(99);
			
		case 0:
			return 0;
	}
}

/*****************************************************************************/
/*
**	hnd_open
*/

static TVOID 
hnd_open(TMOD_HND *mod, TIOMSG *msg)
{
	msg->io_Op.Open.Result = TTRUE;
	if (!TStrCaseCmp(msg->io_Op.Open.Name, "out"))
	{
		FLOCK *fl = (FLOCK *) (msg + 1);
		fl->fileno = Output();
		fl->type = TYPE_OUT;
		fl->name = "out";
		msg->io_FLock = fl;
	}
	else if (!TStrCaseCmp(msg->io_Op.Open.Name, "in"))
	{
		FLOCK *fl = (FLOCK *) (msg + 1);
		fl->fileno = Input();
		fl->type = TYPE_IN;
		fl->name = "in";
		msg->io_FLock = fl;
	}
	else if (!TStrCaseCmp(msg->io_Op.Open.Name, "err"))
	{
		FLOCK *fl = (FLOCK *) (msg + 1);
		fl->fileno = TNULL;		/* indicates kprintf */
		fl->type = TYPE_ERR;
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
**	hnd_write
*/

static TVOID
hnd_write(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;
	TINT res = -1;
	if (fl->fileno)
	{
		res = Write(fl->fileno, msg->io_Op.Write.Buf, msg->io_Op.Write.Len);
		if (res == -1) msg->io_Req.io_Error = geterror(mod);
	}
	#if defined(TDEBUG)
	else
	{
		TINT wrlen = res = msg->io_Op.Write.Len;
		TUINT8 *p = msg->io_Op.Write.Buf;
		while (wrlen > 0)
		{
			TINT l = TMIN(wrlen, sizeof(fl->printbuf) - 1);
			TCopyMem(p, fl->printbuf, l);
			fl->printbuf[l] = 0;
			kprintf("%s", fl->printbuf);
			p += l;
			wrlen -= l;
		}
	}
	#endif
	msg->io_Op.Write.WrLen = res;
}

/*****************************************************************************/
/*
**	hnd_read
*/

static TVOID
hnd_read(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;
	TINT res = Read(fl->fileno, msg->io_Op.Read.Buf, msg->io_Op.Read.Len);
	if (res == -1) msg->io_Req.io_Error = geterror(mod);
	msg->io_Op.Read.RdLen = res;
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
	
		case TIOCMD_WRITE:
			hnd_write(mod, msg);
			break;

		case TIOCMD_READ:
			hnd_read(mod, msg);
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
		/* fake asynchronoucy: reply to ourselves */
		TReplyMsg(msg);
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
**	Revision 1.2  2005/05/13 14:52:27  tmueller
**	write to stderr returned wrong length. fixed
**	
**	Revision 1.1  2005/05/13 14:48:05  tmueller
**	amiga implementation added
**	
*/
