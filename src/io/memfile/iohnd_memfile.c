
/*
**	$Id: iohnd_memfile.c,v 1.2 2005/09/11 09:14:06 tmueller Exp $
**	Driver for mounting a device in a block of memory
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/mod/io.h>
#include <tek/mod/ioext.h>

/*****************************************************************************/

#define MOD_VERSION			0
#define MOD_REVISION		1

typedef struct
{
	struct TModule mod_Module;
	TAPTR mod_UtilBase;
	TAPTR mod_Lock;
	TAPTR mod_MM;
	TUINT mod_RefCount;
	TBOOL mod_Initialized;

} TMOD_HND;

typedef struct
{
	TSTRPTR name;
	TAPTR atom;
	TUINT8 *address;
	TINT pos;
	TINT size;

} FLOCK;

typedef struct
{
	TMOD_HND *mod;
	TSTRPTR name;
	TUINT size;
	TINT numattr;

} EXAMINE;

#define TExecBase		TGetExecBase(mod)
#define TUtilBase		mod->mod_UtilBase

typedef struct TIOPacket TIOMSG;

#define	IOBUFSIZE_DEFAULT	512

static TIOMSG *mod_open(TMOD_HND *mod, TTAGITEM *tags);
static TVOID mod_close(TMOD_HND *mod);
static TMODAPI TVOID beginio(TMOD_HND *mod, TIOMSG *msg);
static TMODAPI TINT abortio(TMOD_HND *mod, TIOMSG *msg);
static TBOOL hnd_init(TMOD_HND *mod, TTAGITEM *tags);

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
			TDestroy(mod->mod_Lock);
			TDestroy(mod->mod_MM);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) mod_open(mod, obj);
		case TMSG_CLOSEMODULE:
			mod_close(obj);
	}
	return 0;
}

TMODENTRY TUINT
tek_init_iohnd_memfile(struct TTask *task, TMOD_HND *mod, TUINT16 version, TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * 8;		/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_HND);		/* positive size */

		return 0;
	}

	mod->mod_Lock = TCreateLock(TNULL);
	if (mod->mod_Lock)
	{
		mod->mod_MM = TCreateMemManager(TNULL,
			TMMT_Tracking | TMMT_TaskSafe, TNULL);

		mod->mod_Module.tmd_Version = MOD_VERSION;
		mod->mod_Module.tmd_Revision = MOD_REVISION;
		mod->mod_Module.tmd_Handle.thn_Hook.thk_Entry = mod_dispatch;
		mod->mod_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;

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
	TIOMSG *msg = TAllocMsg0(sizeof(TIOMSG) + sizeof(FLOCK));
	if (msg)
	{
		TLock(mod->mod_Lock);

		if (!mod->mod_Initialized)
		{
			TUtilBase = TOpenModule("util", 0, TNULL);
			if (TUtilBase)
			{
				mod->mod_Initialized = hnd_init(mod, tags);
			}

			if (!mod->mod_Initialized)
			{
				TCloseModule(mod->mod_UtilBase);
			}
		}

		if (mod->mod_Initialized)
		{
			mod->mod_RefCount++;

			/* insert recommended buffer settings */
			msg->io_BufSize = IOBUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_NONE;

			msg->io_Req.io_Device = (struct TModule *) mod;
		}
		else
		{
			TFree(msg);
			msg = TNULL;
		}

		TUnlock(mod->mod_Lock);
	}

	return msg;
}

static TVOID
mod_close(TMOD_HND *mod)
{
	TLock(mod->mod_Lock);
	if (mod->mod_Initialized)
	{
		if (--mod->mod_RefCount == 0)
		{
			TCloseModule(TUtilBase);
		}
	}
	TUnlock(mod->mod_Lock);
}

static TBOOL
hnd_init(TMOD_HND *mod, TTAGITEM *tags)
{
	return TTRUE;
}

/*****************************************************************************/

static TVOID
hnd_open(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = (FLOCK *) (msg + 1);
	msg->io_Op.Open.Result = TFALSE;

	switch (msg->io_Op.Open.Mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;

		case TFMODE_READONLY:
			break;
	}

	fl->name = TStrDup(mod->mod_MM, msg->io_Op.Open.Name);
	if (fl->name)
	{
		fl->atom = TLockAtom(msg->io_Op.Open.Name, TATOMF_NAME | TATOMF_SHARED);
		if (fl->atom)
		{
			fl->address = (TAPTR) TGetAtomData(fl->atom);
			fl->size = TGetSize(fl->address);
			msg->io_FLock = (TAPTR) fl;
			msg->io_Req.io_Error = 0;
			msg->io_Op.Open.Result = TTRUE;
			return;
		}

		msg->io_Req.io_Error = TIOERR_OBJECT_NOT_FOUND;
		TFree(fl->name);
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	}
}

static TVOID
hnd_close(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;
	TUnlockAtom(fl->atom, TATOMF_KEEP);
	msg->io_Op.Close.Result = TTRUE;
}

static TVOID
hnd_read(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;
	TINT len = msg->io_Op.Read.Len;
	if (fl->pos + len > fl->size) len = fl->size - fl->pos;
	if (len > 0)
	{
		TCopyMem(fl->address + fl->pos, msg->io_Op.Read.Buf, len);
		fl->pos += len;
	}

	msg->io_Op.Read.RdLen = len;
}

static TVOID
hnd_seek(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;
	TINT offs = msg->io_Op.Seek.Offs;

	msg->io_Op.Seek.Result = 0xffffffff;

	if (msg->io_Op.Seek.OffsHi)
	{
		TINT offs_hi = *msg->io_Op.Seek.OffsHi;
		if (offs_hi > 0 || offs_hi < -1)
		{
			msg->io_Req.io_Error = TIOERR_OUT_OF_RANGE;
			return;
		}
	}

	switch (msg->io_Op.Seek.Mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;

		case TFPOS_BEGIN:
			fl->pos = offs;
			break;

		case TFPOS_CURRENT:
			fl->pos += offs;
			break;

		case TFPOS_END:
			fl->pos = fl->size - offs;
			break;
	}

	fl->pos = TCLAMP(0, fl->pos, fl->size);
	if (msg->io_Op.Seek.OffsHi) *msg->io_Op.Seek.OffsHi = 0;
	msg->io_Op.Seek.Result = (TUINT) fl->pos;
}

/*****************************************************************************/
/*
**	hnd_examine
**	examine object. if successful, place a handler-specific examination
**	handle into the iomsg - this is how the IO module knows whether an
**	object has been examined already. note that both files and locks must
**	be supported.
*/

static THOOKENTRY TTAG
examinetags(struct THook *hook, TAPTR obj, TTAG msg)
{
	EXAMINE *e = hook->thk_Data;
	TTAGITEM *ti = obj;
	switch ((TUINT) ti->tti_Tag)
	{
		default:
			return TTRUE;
		case TFATTR_Size:
			*((TINT *) ti->tti_Value) = e->size;
			break;
		case TFATTR_Name:
			*((TSTRPTR *) ti->tti_Value) = e->name;
			break;
	}
	e->numattr++;
	return TTRUE;
}

static TVOID
hnd_examine(TMOD_HND *mod, TIOMSG *msg)
{
	EXAMINE *e = msg->io_Examine;
	msg->io_Op.Examine.Result = -1;

	if (!e)
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
		e = TExecAlloc0(TExecBase, TNULL, sizeof(EXAMINE));
		if (e)
		{
			FLOCK *fl = msg->io_FLock;

			e->mod = mod;
			e->name = fl->name;
			e->size = fl->size;
			msg->io_Examine = e;
		}
	}

	if (e)
	{
		struct THook hook;
		e->numattr = 0;
		TInitHook(&hook, examinetags, e);
		TForEachTag(msg->io_Op.Examine.Tags, &hook);
		msg->io_Op.Examine.Result = e->numattr;
		msg->io_Req.io_Error = 0;
	}
}

/*****************************************************************************/
/*
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

		case TIOCMD_SEEK:
			hnd_seek(mod, msg);
			break;

		case TIOCMD_EXAMINE:
			hnd_examine(mod, msg);
			break;
	}
}

/*****************************************************************************/
/*
**	beginio(msg->io_Device, msg)
**	abortio(msg->io_Device, msg)
*/

static TMODAPI TVOID
beginio(TMOD_HND *mod, TIOMSG *msg)
{
	/* always synchronous */
	msg->io_Req.io_Flags &= ~TIOF_QUICK;
	hnd_docmd(mod, msg);
}

static TMODAPI TINT
abortio(TMOD_HND *mod, TIOMSG *msg)
{
	/* not supported */
	return -1;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: iohnd_memfile.c,v $
**	Revision 1.2  2005/09/11 09:14:06  tmueller
**	cosmetic
**
**	Revision 1.1  2005/08/11 18:42:04  tmueller
**	added
**
*/
