
/*
**	$Id: iohnd_ps2io.c,v 1.18 2006/02/24 14:06:19 tmueller Exp $
**
**	Largely based on the findings and code by Gustavo Scotti and
**	Marcus R. Brown, adapted to the semantics of TEKlib I/O handlers
**	by Timm S. Mueller <tmueller at neoscientists.org>
*/

#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/io.h>

#include <tek/debug.h>
#include <tek/teklib.h>

#include <tek/mod/io.h>
#include <tek/mod/ioext.h>
#include <tek/mod/ps2/ps2io.h>

#include <sifrpc.h>
#include <kernel.h>

#include <unistd.h>

#define MOD_VERSION		0
#define MOD_REVISION	3

/*****************************************************************************/

#define UNCACHED(p)			((TAPTR) (((TUINTPTR)(p)) | 0x20000000))
#define IS_UNCACHED(p)		(((TUINTPTR)(p)) & 0x20000000)

/*****************************************************************************/

#define O_RDONLY			0x0001
#define O_WRONLY			0x0002
#define O_RDWR				0x0003
#define O_NBLOCK			0x0010
#define O_APPEND			0x0100
#define O_CREAT				0x0200
#define O_TRUNC				0x0400
#define O_NOWAIT			0x8000

#define	IOBUFSIZE_DEFAULT	16384

#define PIO_PATH_MAX		256

#define PIO_WAIT			0
#define PIO_NOWAIT			1

enum pio_functions
{
	PIO_F_OPEN = 0,
	PIO_F_CLOSE,
	PIO_F_READ,
	PIO_F_WRITE,
	PIO_F_LSEEK,
	PIO_F_IOCTL,
	PIO_F_REMOVE,
	PIO_F_MKDIR,
	PIO_F_RMDIR,
	PIO_F_DOPEN,
	PIO_F_DCLOSE,
	PIO_F_DREAD,
	PIO_F_GETSTAT,
	PIO_F_CHSTAT,
	PIO_F_FORMAT,
	PIO_F_ADDDRV,
	PIO_F_DELDRV
};

/* The updated modules shipped with licensed games changed the size of
   the buffers from 16 to 64.  */

typedef struct
{
	TUINT size1;
	TUINT size2;
	TAPTR dest1;
	TAPTR dest2;
	TUINT8 buf1[16];
	TUINT8 buf2[16];

} pio_read_data_t;

/*****************************************************************************/

typedef struct
{
	struct TModule pio_Module;	/* Module header */
								/* aligned */
	TAPTR pio_IOTask;			/* task to perform I/O operations */
	TAPTR pio_UtilBase;			/* ptr to Utility module base */
	TAPTR pio_TimeBase;			/* ptr to Time module base */
	TAPTR pio_HndLock;			/* locking for handler base */
	TUINT pio_RefCount;			/* handler reference counter */
	TAPTR pio_TimeReq;			/* Time Request structure */
	TINT pio_ComplSema;			/* Completion semaphore */
	TINT pio_BlockMode;			/* Blocking mode */
								/* aligned */
	TUINT _intr_data[32 + 16];
	SifRpcClientData_t pio_ClientData;		/* for file I/O */
	SifRpcClientData_t pio_ModClientData;	/* for module loading */
	TUINT *pio_IntrData;		/* ptr to _intr_data, 64 bytes aligned */

} TMOD_HND;

#define TExecBase TGetExecBase(mod)
#define TUtilBase mod->pio_UtilBase
#define TTimeBase mod->pio_TimeBase

typedef struct
{
	TINT result[16];			/* aligned */

	union
	{
		struct { TINT mode; TUINT8 name[PIO_PATH_MAX]; } open;
		struct { TINT fd; } close;
		struct { TINT fd; TAPTR buf; TUINT len; pio_read_data_t *data; } read;
		struct { TINT fd; TAPTR buf; TUINT len; TUINT mis;
			TUINT8 aligned[16]; } write;
		struct { TINT fd; TINT offset; TUINT whence; } seek;
		struct { TUINT8 unknown1[8]; TUINT8 name[252]; TUINT8 unknown2[252]; } loadirx;

	} arg;						/* aligned */

	TINT fileno;
	TSTRPTR name;

} FLOCK;

typedef struct
{
	TMOD_HND *mod;
	TDATE date;
	TSTRPTR name;
	TUINT size;
	TUINT type;
	TINT numattr;

} EXAMINE;


typedef struct TIOPacket TIOMSG;

static void mod_destroy(TMOD_HND *mod);
static TIOMSG *mod_open(TMOD_HND *mod, TTAGITEM *tags);
static void mod_close(TMOD_HND *mod);
static void mod_task(struct TTask *task);
static TBOOL mod_init(struct TTask *task);

static TMODAPI void beginio(TMOD_HND *mod, TIOMSG *msg);
static TMODAPI TINT abortio(TMOD_HND *mod, TIOMSG *msg);

/*****************************************************************************/
/*
**	module init/exit
*/

static void
mod_destroy(TMOD_HND *mod)
{
	TDestroy(mod->pio_HndLock);
}

static THOOKENTRY TTAG
mod_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	TMOD_HND *mod = (TMOD_HND *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			mod_destroy(mod);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) mod_open(mod, obj);
		case TMSG_CLOSEMODULE:
			mod_close(obj);
	}
	return 0;
}

TMODENTRY TUINT
tek_init_iohnd_ps2io(struct TTask *task, TMOD_HND *mod, TUINT16 version,
	TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)					/* first call */
		{
			if (version <= MOD_VERSION)			/* version check */
			{
				return sizeof(TMOD_HND);		/* module positive size */
			}
		}
		else									/* second call */
		{
			return sizeof(TAPTR) * 8;
		}
	}
	else										/* third call */
	{
		mod->pio_HndLock = TExecCreateLock(TExecBase, TNULL);
		if (mod->pio_HndLock)
		{
			mod->pio_Module.tmd_Version = MOD_VERSION;
			mod->pio_Module.tmd_Revision = MOD_REVISION;
			mod->pio_Module.tmd_Handle.thn_Hook.thk_Entry = mod_dispatch;
			mod->pio_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;

			((TAPTR *) mod)[-3] = (TAPTR) beginio;
			((TAPTR *) mod)[-4] = (TAPTR) abortio;

			return TTRUE;
		}
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

	msg = TExecAllocMsg0(TExecBase, sizeof(TIOMSG) + sizeof(FLOCK) + 64);
	if (msg)
	{
		TExecLock(TExecBase, mod->pio_HndLock);

		if (!mod->pio_IOTask)
		{
			mod->pio_UtilBase = TExecOpenModule(TExecBase, "util", 0, TNULL);
			mod->pio_TimeBase = TExecOpenModule(TExecBase, "time", 0, TNULL);

			if (mod->pio_UtilBase && mod->pio_TimeBase)
			{
				mod->pio_TimeReq = TTimeAllocRequest(TTimeBase, TNULL);
				if (mod->pio_TimeReq)
				{
					TTAGITEM tasktags[2];
					tasktags[0].tti_Tag = TTask_UserData;
					tasktags[0].tti_Value = (TTAG) mod;
					tasktags[1].tti_Tag = TTAG_DONE;

					/* create handler task */
					mod->pio_IOTask = TExecCreateTask(TExecBase,
						mod_task, mod_init, tasktags);
					if (mod->pio_IOTask == TNULL)
					{
						TTimeFreeRequest(TTimeBase, mod->pio_TimeReq);
						mod->pio_TimeReq = TNULL;
					}
				}
			}
		}

		if (mod->pio_IOTask)
		{
			mod->pio_RefCount++;

			/* insert recommended buffer settings */
			msg->io_BufSize = IOBUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_FULL;

			msg->io_Req.io_Device = (struct TModule *) mod;
		}
		else
		{
			TExecCloseModule(TExecBase, mod->pio_TimeBase);
			mod->pio_TimeBase = TNULL;
			TExecCloseModule(TExecBase, mod->pio_UtilBase);
			mod->pio_UtilBase = TNULL;
			TExecFree(TExecBase, msg);
			msg = TNULL;
		}

		TExecUnlock(TExecBase, mod->pio_HndLock);
	}

	return msg;
}

static void
mod_close(TMOD_HND *mod)
{
	TExecLock(TExecBase, mod->pio_HndLock);
	if (mod->pio_IOTask)
	{
		if (--mod->pio_RefCount == 0)
		{
			TExecSignal(TExecBase, mod->pio_IOTask, TTASK_SIG_ABORT);
			TDestroy(mod->pio_IOTask);
			mod->pio_IOTask = TNULL;

			if (mod->pio_TimeBase)
			{
				TTimeFreeRequest(TTimeBase, mod->pio_TimeReq);
				TExecCloseModule(TExecBase, mod->pio_TimeBase);
			}
			TExecCloseModule(TExecBase, mod->pio_UtilBase);
		}
	}
	TExecUnlock(TExecBase, mod->pio_HndLock);
}

/*****************************************************************************/
/*
**	ps2io:	"host/xxx"	->		"host:xxx"
**			"host"		->		"host:"
**			"cdrom/xxx"	->		"cdrom0:\xxx"
**			"cdrom"		->		"cdrom0:\"
*/

static TSTRPTR
gethostname(TMOD_HND *mod, TSTRPTR name)
{
	TSTRPTR hostname = TNULL;
	TINT len = TUtilStrLen(TUtilBase, name);

	if (TUtilStrCaseCmp(TUtilBase, name, "cdrom") == 0)
	{
		/* "cdrom" -> "cdrom0:\" */
		hostname = TExecAlloc(TExecBase, TNULL, len + 4);
		if (hostname)
		{
			TUtilStrCpy(TUtilBase, hostname, "cdrom0:\\");
		}
	}
	else if (TUtilStrNCaseCmp(TUtilBase, name, "cdrom/", 6) == 0)
	{
		/* "cdrom/xxx" -> "cdrom0:\xxx" */
		hostname = TExecAlloc(TExecBase, TNULL, len + 3);
		if (hostname)
		{
			TINT c;
			TSTRPTR s, d;
			TUtilStrCpy(TUtilBase, hostname, "cdrom0:\\");
			s = name + 6;
			d = hostname + 8;
			do
			{
				c = *s++;
				if (c == '/') c = '\\';
				else if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
				*d++ = c;
			} while (c);
		}
	}
	else if (TUtilStrCaseCmp(TUtilBase, name, "host") == 0)
	{
		/* "host" -> "host:" */
		hostname = TExecAlloc(TExecBase, TNULL, len + 2);
		if (hostname)
		{
			TUtilStrCpy(TUtilBase, hostname, "host:");
		}
	}
	else if (TUtilStrNCaseCmp(TUtilBase, name, "host/", 5) == 0)
	{
		/* "host/xxx" -> "host:xxx" */
		hostname = TExecAlloc(TExecBase, TNULL, len + 1);
		if (hostname)
		{
			TUtilStrCpy(TUtilBase, hostname, "host:");
			TUtilStrCat(TUtilBase, hostname, name + 5);
		}
	}
	else if (TUtilStrCaseCmp(TUtilBase, name, "out") == 0)
	{
		/* "out" -> "out:" */
		hostname = TExecAlloc(TExecBase, TNULL, len + 2);
		if (hostname)
		{
			TUtilStrCpy(TUtilBase, hostname, "out:");
		}
	}

	if (hostname)
	{
		TDBPRINTF(5,("name created: %s\n", hostname));
	}

	return hostname;
}

/*****************************************************************************/

static void
pio_intr(TMOD_HND *mod)
{
	iSignalSema(mod->pio_ComplSema);
}

/*****************************************************************************/
/*
**	hnd_lock
**	for now, grant locks opportunistically
**	(but Examine and OpenFromLock can't be implemented that way)
*/

static void
hnd_lock(TMOD_HND *mod, TIOMSG *msg)
{
	/* align flock portion of the I/O message to 64 bytes */
	FLOCK *flock = (FLOCK *) ((((TUINTPTR) (msg + 1)) + 63) & ~63);
	msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	flock->name = gethostname(mod, msg->io_Op.Lock.Name);
	if (flock->name)
	{
		msg->io_FLock = flock;
		msg->io_Req.io_Error = 0;
		msg->io_Op.Lock.Result = TTRUE;
		return;
	}
	msg->io_Op.Lock.Result = TFALSE;
}

static void
hnd_unlock(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock = msg->io_FLock;
	TExecFree(TExecBase, flock->name);
	msg->io_Req.io_Error = 0;
}

/*****************************************************************************/
/*
**	hnd_open
*/

static void
hnd_open(TMOD_HND *mod, TIOMSG *msg)
{
	/* align flock portion of the I/O message to 64 bytes */
	FLOCK *flock = (FLOCK *) ((((TUINTPTR) (msg + 1)) + 63) & ~63);
	TINT hostmode;

	msg->io_Op.Open.Result = TFALSE;

	switch (msg->io_Op.Open.Mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;

		case TFMODE_READONLY:
			hostmode = O_RDONLY;
			break;

		case TFMODE_OLDFILE:
			hostmode = O_RDWR;
			break;

		case TFMODE_NEWFILE:
			hostmode = O_RDWR | O_CREAT | O_TRUNC;
			break;

		case TFMODE_READWRITE:
			hostmode = O_RDWR | O_CREAT;
	}

	msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;

	flock->name = gethostname(mod, msg->io_Op.Open.Name);
	if (flock->name)
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_NOT_FOUND;

		/* intercept predefined fileno */
		if (TUtilStrCmp(TUtilBase, flock->name, "out:") == 0)
		{
			flock->fileno = 1;
			msg->io_Req.io_Error = 0;
		}
		else
		{
			flock->arg.open.mode = hostmode;
			TUtilStrNCpy(TUtilBase, flock->arg.open.name,
				flock->name, PIO_PATH_MAX);

			WaitSema(mod->pio_ComplSema);

			if (SifCallRpc(&mod->pio_ClientData, PIO_F_OPEN, mod->pio_BlockMode,
				&flock->arg.open, sizeof(flock->arg.open), flock->result, 4,
				(SifRpcEndFunc_t) pio_intr, mod) >= 0)
			{
				if (flock->result[0] >= 0)
				{
					flock->fileno = flock->result[0];
					msg->io_Req.io_Error = 0;
				}
			}
		}

		if (msg->io_Req.io_Error == 0)
		{
			msg->io_FLock = flock;
			msg->io_Op.Open.Result = TTRUE;
			TDBPRINTF(5,("open name: %s succeeded: %d\n",
				flock->name, flock->fileno));
			return;
		}

		TDBPRINTF(10,("open name: %s failed: %d\n",
			flock->name, flock->fileno));

	#if 0
		if (SifCallRpc(&mod->pio_ClientData, PIO_F_OPEN, mod->pio_BlockMode,
			&flock->arg.open, sizeof(flock->arg.open), flock->result, 4,
			(SifRpcEndFunc_t) pio_intr, mod) >= 0)
		{
			/* a succeeded RPC call doesn't mean the file open succeeded */
			if (flock->result[0] >= 0)
			{
				flock->fileno = flock->result[0];
				msg->io_FLock = flock;
				msg->io_Req.io_Error = 0;
				msg->io_Op.Open.Result = TTRUE;

				TDBPRINTF(5,("open name: %s succeeded: %d\n",
					flock->name, flock->fileno));

				return;
			}
		}
	#endif

		TExecFree(TExecBase, flock->name);
	}
}

/*****************************************************************************/
/*
**	hnd_close
*/

static void
hnd_close(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock = msg->io_FLock;

	if (flock->fileno != 1)
	{
		flock->arg.close.fd = flock->fileno;

		WaitSema(mod->pio_ComplSema);

		msg->io_Op.Close.Result =
			(SifCallRpc(&mod->pio_ClientData, PIO_F_CLOSE, 0,
				&flock->arg.close, sizeof(flock->arg.close),
				flock->result, 4, (SifRpcEndFunc_t) pio_intr, mod) >= 0);

		TExecFree(TExecBase, flock->name);

		TDBPRINTF(2,("close %d result %d\n",
			flock->fileno, msg->io_Op.Close.Result));
	}
}

/*****************************************************************************/
/*
**	hnd_read
*/

static void
pio_read_intr(TMOD_HND *mod)
{
	pio_read_data_t *data = UNCACHED(mod->pio_IntrData);

	if (data->dest1 && data->size1)
		TExecCopyMem(TExecBase, data->buf1, data->dest1, data->size1);

	if (data->dest2 && data->size2)
		TExecCopyMem(TExecBase, data->buf2, data->dest2, data->size2);

	iSignalSema(mod->pio_ComplSema);
}

static void
hnd_read(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock = msg->io_FLock;
	TAPTR buf = msg->io_Op.Read.Buf;
	TINT len = msg->io_Op.Read.Len;

	WaitSema(mod->pio_ComplSema);

	flock->arg.read.fd = flock->fileno;
	flock->arg.read.buf = buf;
	flock->arg.read.len = len;
	flock->arg.read.data = (pio_read_data_t *) mod->pio_IntrData;

	if (!IS_UNCACHED(buf)) SifWriteBackDCache(buf, len);
	SifWriteBackDCache(mod->pio_IntrData, 128);
	SifWriteBackDCache(&flock->arg.read, sizeof(flock->arg.read));

	if (SifCallRpc(&mod->pio_ClientData, PIO_F_READ, mod->pio_BlockMode,
		&flock->arg.read, sizeof(flock->arg.read),
		flock->result, 4, (SifRpcEndFunc_t) pio_read_intr, mod) < 0)
	{
		msg->io_Req.io_Error = TIOERR_DISK_CORRUPT;
		return;
	}

	msg->io_Op.Read.RdLen = flock->result[0];

	TDBPRINTF(2,("read fd %d size %d to %08x result: %d\n",
		flock->fileno, len, (TUINT) buf, msg->io_Op.Read.RdLen));
}

/*****************************************************************************/
/*
**	hnd_write
*/

static void
hnd_write(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock = msg->io_FLock;
	TAPTR buf = msg->io_Op.Write.Buf;
	TINT len = msg->io_Op.Write.Len;
	TUINT mis;

	WaitSema(mod->pio_ComplSema);

	flock->arg.write.fd = flock->fileno;
	flock->arg.write.buf = buf;
	flock->arg.write.len = len;

	/* copy unaligned (16 byte) portion into argument */
	mis = (TUINT) buf & 15;
	if (mis)
	{
		mis = 16 - mis;
		if (mis > len) mis = len;
	}
	flock->arg.write.mis = mis;
	if (mis) TExecCopyMem(TExecBase, buf, flock->arg.write.aligned, mis);

	if (!IS_UNCACHED(buf)) SifWriteBackDCache(buf, len);

	if (SifCallRpc(&mod->pio_ClientData, PIO_F_WRITE, mod->pio_BlockMode,
		&flock->arg.write, sizeof(flock->arg.write),
		flock->result, 4, (SifRpcEndFunc_t) pio_intr, mod) < 0)
	{
		msg->io_Req.io_Error = TIOERR_DISK_CORRUPT;
		return;
	}

	msg->io_Op.Write.WrLen = flock->result[0];
}

/*****************************************************************************/
/*
**	hnd_seek
**	seek in a file
*/

static void
hnd_seek(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock = msg->io_FLock;
	TINT whence;

	msg->io_Op.Seek.Result = 0xffffffff;

	switch (msg->io_Op.Seek.Mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;

		case TFPOS_BEGIN:
			whence = SEEK_SET;
			break;

		case TFPOS_CURRENT:
			whence = SEEK_CUR;
			break;

		case TFPOS_END:
			whence = SEEK_END;
			break;
	}

	if (msg->io_Op.Seek.OffsHi)
	{
		TINT offs_hi = *msg->io_Op.Seek.OffsHi;
		if (offs_hi > 0 || offs_hi < -1)
		{
			msg->io_Req.io_Error = TIOERR_OUT_OF_RANGE;
			return;
		}
	}

	WaitSema(mod->pio_ComplSema);

	flock->arg.seek.fd = flock->fileno;
	flock->arg.seek.offset = msg->io_Op.Seek.Offs;
	flock->arg.seek.whence = whence;

	if (SifCallRpc(&mod->pio_ClientData, PIO_F_LSEEK, 0,
		&flock->arg.seek, sizeof(flock->arg.seek),
		flock->result, 4, (SifRpcEndFunc_t) pio_intr, mod) < 0)
	{
		msg->io_Req.io_Error = TIOERR_DISK_CORRUPT;
		return;
	}

	msg->io_Op.Seek.Result = flock->result[0];

	TDBPRINTF(2,("fd %d seek result: %d\n",
		flock->fileno, flock->result[0]));
}

/**************************************************************************
**
**	examinetags(examine, tagitem)
**	fill examination info into user-supplied tags. this
**	is called back by TForEachTag()
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
	}

	e->numattr++;
	return TTRUE;
}

/**************************************************************************
**
**	hnd_examine
**	examine object. if successful, place a handler-specific examination
**	handle into the iomsg - this is how the IO module knows whether an
**	object has been examined already. note that both files and locks must
**	be supported.
**
**	the only attribute that can be examined at the moment is filesize
*/

static void hnd_examine(TMOD_HND *mod, TIOMSG *msg)
{
	EXAMINE *e = msg->io_Examine;
	msg->io_Op.Examine.Result = -1;

	if (!e)
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
		e = TExecAlloc0(TExecBase, TNULL, sizeof(EXAMINE));
		if (e)
		{
			TIOMSG temp;
			TINT cpos;
			TBOOL success = TTRUE;

			e->mod = mod;

			temp = *msg;
			msg->io_Req.io_Command = TIOCMD_SEEK;
			msg->io_Op.Seek.Offs = 0;
			msg->io_Op.Seek.OffsHi = TNULL;
			msg->io_Op.Seek.Mode = TFPOS_CURRENT;
			hnd_seek(mod, msg);
			cpos = msg->io_Op.Seek.Result;

			msg->io_Req.io_Command = TIOCMD_SEEK;
			msg->io_Op.Seek.Offs = 0;
			msg->io_Op.Seek.OffsHi = TNULL;
			msg->io_Op.Seek.Mode = TFPOS_END;
			hnd_seek(mod, msg);
			e->size = msg->io_Op.Seek.Result;

			msg->io_Req.io_Command = TIOCMD_SEEK;
			msg->io_Op.Seek.Offs = cpos;
			msg->io_Op.Seek.OffsHi = TNULL;
			msg->io_Op.Seek.Mode = TFPOS_BEGIN;
			hnd_seek(mod, msg);

			*msg = temp;

			if (success)
			{
				msg->io_Examine = e;
			}
			else
			{
				TExecFree(TExecBase, e);
				return;
			}
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
**	hnd_loadmodule
**	load IRX module
*/

static void
hnd_loadirx(TMOD_HND *mod, struct TPS2IOPacket *msg)
{
	FLOCK *flock = (FLOCK *) ((((TUINTPTR) (msg + 1)) + 63) & ~63);
	TSTRPTR name;

	msg->io_Op.LoadIRX.Result = TFALSE;
	msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;

	name = gethostname(mod, msg->io_Op.LoadIRX.Name);
	if (name)
	{
		WaitSema(mod->pio_ComplSema);

		msg->io_Req.io_Error = TIOERR_OBJECT_NOT_FOUND;
		TExecFillMem(TExecBase, &flock->arg.loadirx,
			sizeof(flock->arg.loadirx), 0);
		TUtilStrNCpy(TUtilBase, flock->arg.loadirx.name, name, 252);
		if (SifCallRpc(&mod->pio_ModClientData, 0, 0,
			&flock->arg.loadirx, sizeof(flock->arg.loadirx),
			flock->result, 4, (SifRpcEndFunc_t) pio_intr, mod) >= 0)
		{
			msg->io_Req.io_Error = 0;
			msg->io_Op.LoadIRX.Result = TTRUE;
		}

		TExecFree(TExecBase, name);
	}
}

/*****************************************************************************/
/*
**	hnd_docmd
*/

static void
hnd_docmd(TMOD_HND *mod, TIOMSG *msg)
{
	msg->io_Req.io_Error = 0;

	switch (msg->io_Req.io_Command)
	{
		default:
			msg->io_Req.io_Error = TIOERR_UNKNOWN_COMMAND;
			break;

		case TIOCMD_LOCK:
			hnd_lock(mod, msg);
			break;

		case TIOCMD_UNLOCK:
			hnd_unlock(mod, msg);
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

		case TIOCMD_SEEK:
			hnd_seek(mod, msg);
			break;

		case TIOCMD_EXAMINE:
			hnd_examine(mod, msg);
			break;

		case TIOCMD_LOADIRX:
			hnd_loadirx(mod, (struct TPS2IOPacket *) msg);
			break;
	}
}

/*****************************************************************************/
/*
**	handler task
*/

static TBOOL mod_init(struct TTask *task)
{
	TMOD_HND *mod = TExecGetTaskData(TGetExecBase(task), task);
	TTIME delay = { 0, 5000 };
	ee_sema_t compSema;
	TINT res;

	/* align intrdata block */
	mod->pio_IntrData = (TUINT *) ((((TUINTPTR) mod->_intr_data) + 63) & ~63);

	/* init clientdata for file I/O */
	while (((res = SifBindRpc(&mod->pio_ClientData, 0x80000001, 0)) >= 0) &&
		(mod->pio_ClientData.server == TNULL))
			TTimeDelay(TTimeBase, mod->pio_TimeReq, &delay);

	if (res < 0) return TFALSE;

	/* init clientdata for IRX loading */
	while (((res = SifBindRpc(&mod->pio_ModClientData, 0x80000006, 0)) >= 0) &&
		(mod->pio_ModClientData.server == TNULL))
			TTimeDelay(TTimeBase, mod->pio_TimeReq, &delay);

	if (res < 0) return TFALSE;

	compSema.init_count = 1;
	compSema.max_count = 1;
	compSema.option = 0;

	mod->pio_ComplSema = CreateSema(&compSema);
	if (mod->pio_ComplSema < 0) return TFALSE;

	mod->pio_BlockMode = PIO_WAIT;

	return TTRUE;
}

static void mod_task(struct TTask *task)
{
	TMOD_HND *mod = TExecGetTaskData(TGetExecBase(task), task);
	TAPTR port = TExecGetUserPort(TExecBase, task);
	TUINT portsig = TExecGetPortSignal(TExecBase, port);
	TIOMSG *msg;
	TUINT sigs;

	do
	{
		sigs = TExecWait(TExecBase, portsig | TTASK_SIG_ABORT);
		if (sigs & portsig)
		{
			while ((msg = TExecGetMsg(TExecBase, port)))
			{
				hnd_docmd(mod, msg);
				TExecReplyMsg(TExecBase, msg);
			}
		}

	} while (!(sigs & TTASK_SIG_ABORT));

	DeleteSema(mod->pio_ComplSema);
}

/*****************************************************************************/
/*
**	beginio(msg->io_Device, msg)
*/

static TMODAPI void
beginio(TMOD_HND *mod, TIOMSG *msg)
{
	/* do asynchronously via I/O task */
	TExecPutMsg(TExecBase, TExecGetUserPort(TExecBase, mod->pio_IOTask),
		msg->io_Req.io_ReplyPort, msg);
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
**	$Log: iohnd_ps2io.c,v $
**	Revision 1.18  2006/02/24 14:06:19  tmueller
**	debug macros corrected (again)
**
**	Revision 1.17  2006/02/24 14:04:03  tmueller
**	debug macros corrected
**
**	Revision 1.16  2005/11/11 10:07:45  tmueller
**	writebuffer in misalignment case corruption fixed
**
**	Revision 1.15  2005/09/18 11:27:22  tmueller
**	added authors
**
**	Revision 1.14  2005/08/31 09:46:48  tmueller
**	corrected return value of IRX loading
**
**	Revision 1.13  2005/07/30 20:59:01  tmueller
**	Implemented TIOCMD_EXAMINE (only TFATTR_Size is currently supported)
**
**	Revision 1.12  2005/07/16 09:39:55  fschulze
**	bug fixed in TIOCMD_SEEK
**
**	Revision 1.11  2005/07/15 21:41:48  fschulze
**	increased default buffer size
**
**	Revision 1.10  2005/07/01 22:21:41  fschulze
**	Added ps2io:cdrom device
**
**	Revision 1.9  2005/05/14 21:42:30  tmueller
**	unistd include was missing. fixed
**
**	Revision 1.8  2005/05/11 21:30:32  tmueller
**	added ps2io:out device
**
**	Revision 1.7  2005/05/11 00:33:11  tmueller
**	Files always opened successfully; treatment of retval from RPC call fixed
**
**	Revision 1.6  2005/04/22 14:12:39  fschulze
**	Added IOCMD_LOADIRX for loading IRX modules via common I/O namespace
**
**	Revision 1.5  2005/04/09 00:45:22  tmueller
**	ps2 I/O handler is now largely freestanding
**
**	Revision 1.4  2005/04/01 18:48:48  tmueller
**	Severe bug in memory management fixed; dependency from fileio lib removed
**
**	Revision 1.3  2005/03/21 11:12:17  fschulze
**	added fioClose() delay
**
**	Revision 1.2  2005/03/13 20:39:31  tmueller
**	added dummy hnd_lock and hnd_unlock
**
**	Revision 1.1.1.1  2005/01/23 13:57:59  fschulze
**	initial import
**
**	Revision 1.1  2005/01/22 16:13:47  tmueller
**	added
**
*/
