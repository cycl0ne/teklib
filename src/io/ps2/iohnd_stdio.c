
/*
**	$Id: iohnd_stdio.c,v 1.1 2005/05/11 21:34:38 tmueller Exp $
**	iohnd_stdio - PS2 implementaton of the stdio handler
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/mod/ioext.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>

/*****************************************************************************/

#define MOD_VERSION		0
#define MOD_REVISION	1

typedef struct
{
	struct TModule thnd_Module;
	TAPTR thnd_UtilBase;
	TAPTR thnd_IOBase;
	TAPTR thnd_Lock;
	TUINT thnd_RefCount;
	TAPTR thnd_PS2OutFile;

} TMOD_HND;

#define TExecBase TGetExecBase(mod)
#define TUtilBase mod->thnd_UtilBase
#define TIOBase mod->thnd_IOBase

typedef struct
{
	TINT type;
	TSTRPTR name;

} FLOCK;

enum { TYPE_OUT, TYPE_IN, TYPE_ERR };

#define	IOBUFSIZE_DEFAULT	512

typedef struct TIOPacket TIOMSG;

static TIOMSG *mod_open(TMOD_HND *mod, TTAGITEM *tags);
static TVOID mod_close(TMOD_HND *mod);

static TMODAPI TVOID beginio(TMOD_HND *mod, TIOMSG *msg);
static TMODAPI TINT abortio(TMOD_HND *mod, TIOMSG *msg);

/*****************************************************************************/
/*
**	module init/exit
*/

static THOOKENTRY TTAG
mod_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	TMOD_HND *mod = (TMOD_HND *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			TDestroy(mod->thnd_Lock);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) mod_open(mod, obj);
		case TMSG_CLOSEMODULE:
			mod_close(obj);
	}
	return 0;
}

TMODENTRY TUINT
tek_init_iohnd_stdio(struct TTask *task, TMOD_HND *mod, TUINT16 version,
	TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * 8;				/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_HND);				/* positive size */

		return 0;
	}

	mod->thnd_Lock = TCreateLock(TNULL);
	if (mod->thnd_Lock)
	{
		mod->thnd_Module.tmd_Version = MOD_VERSION;
		mod->thnd_Module.tmd_Revision = MOD_REVISION;
		mod->thnd_Module.tmd_Handle.thn_Hook.thk_Entry = mod_dispatch;
		mod->thnd_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;

		((TAPTR *) mod)[-3] = (TAPTR) beginio;
		((TAPTR *) mod)[-4] = (TAPTR) abortio;

		return TTRUE;
	}

	return 0;
}

/*****************************************************************************/
/*
**	module open/close
*/

static TIOMSG *
mod_open(TMOD_HND *mod, TTAGITEM *tags)
{
	TIOMSG *msg;

	msg = TAllocMsg0(sizeof(TIOMSG) + sizeof(FLOCK));
	if (msg)
	{
		TLock(mod->thnd_Lock);

		if (mod->thnd_RefCount == 0)
		{
			TUtilBase = TOpenModule("util", 0, TNULL);
			TIOBase = TOpenModule("io", 0, TNULL);
			if (TIOBase)
			{
				mod->thnd_PS2OutFile = TOpenFile("ps2io:out",
					TFMODE_READWRITE, TNULL);
			}
		}

		if (TUtilBase && mod->thnd_PS2OutFile)
		{
			mod->thnd_RefCount++;

			msg->io_Req.io_Device = (struct TModule *) mod;

			/* insert recommended buffer settings */
			msg->io_BufSize = IOBUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_LINE;
		}
		else
		{
			if (TIOBase)
			{
				TCloseFile(mod->thnd_PS2OutFile);
				TCloseModule(TIOBase);
			}
			TCloseModule(TUtilBase);
			TFree(msg);
			msg = TNULL;
		}

		TUnlock(mod->thnd_Lock);
	}

	return msg;
}

static TVOID
mod_close(TMOD_HND *mod)
{
	TLock(mod->thnd_Lock);
	if (--mod->thnd_RefCount == 0)
	{
		if (TIOBase)
		{
			TCloseFile(mod->thnd_PS2OutFile);
			TCloseModule(TIOBase);
		}
		TCloseModule(TUtilBase);
	}
	TUnlock(mod->thnd_Lock);
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
		fl->type = TYPE_OUT;
		fl->name = "out";
		msg->io_FLock = fl;
	}
	else if (!TStrCaseCmp(msg->io_Op.Open.Name, "err"))
	{
		FLOCK *fl = (FLOCK *) (msg + 1);
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
	msg->io_Op.Write.WrLen = TWrite(mod->thnd_PS2OutFile,
		msg->io_Op.Write.Buf, msg->io_Op.Write.Len);
	msg->io_Req.io_Error = TGetIOErr();
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
**	Revision 1.1  2005/05/11 21:34:38  tmueller
**	added ps2 stdio handler
**
*/
