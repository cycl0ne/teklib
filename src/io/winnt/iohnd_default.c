
/*
**	$Id: iohnd_default.c,v 1.1 2006/08/25 21:23:42 tmueller Exp $
**	Win32 implementation of the 'default' I/O handler.
**	It is responsible for abstraction from the host file system.
**
**	TODO: Lock directories using CreateFile/FILE_FLAG_BACKUP_SEMANTICS
*/

#include <tek/exec.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/proto/exec.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/proto/hal.h>
#include <tek/inline/io.h>
#include <tek/mod/io.h>
#include <tek/mod/ioext.h>
#include <tek/mod/time.h>

/*****************************************************************************/

#define IODEF_VERSION	0
#define IODEF_REVISION	3

typedef struct
{
	struct TModule iod_Module;
	struct TExecBase *iod_ExecBase;
	struct TUtilBase *iod_UtilBase;
	struct TLock *iod_Lock;
	struct TTask *iod_IOTask;
	struct TMemManager *iod_MemManager;
	TUINT iod_RefCount;

} IODEF_MOD;

typedef struct
{
	IODEF_MOD *mod;
	TUINT flags;
	TSTRPTR winname;
	TUINT lockmode;
	HANDLE filehandle;
	HANDLE dirfindhandle;			/* find handle for directories */
	WIN32_FIND_DATA dirfinddata;	/* find data for directories */

} IODEF_LOCK;

typedef struct
{
	IODEF_MOD *mod;

	IODEF_LOCK *flock;				/* back-ptr */
	HANDLE filefindhandle;			/* find handle for files */
	WIN32_FIND_DATA filefinddata;	/* find data for files */
	struct TList devlist;			/* list of device names (A, C, D...) */
	struct TList devlist_done;		/* list of processed device names */

	TSTRPTR tempdrivename;
	TUINT size;
	TUINT sizehi;
	TUINT type;
	struct TDateBox datebox;
	TSTRPTR name;
	TINT numattr;
	TBOOL havesizehi;

} IODEF_EXAMINE;

#define IODEFL_NONE		0x0000
#define IODEFL_LOCK		0x0001
#define IODEFL_FILE		0x0002
#define IODEFL_ISDIR	0x0004
#define IODEFL_ISROOT	0x0008

#define	IODEF_BUFSIZE_DEFAULT	4096

/*****************************************************************************/

static THOOKENTRY TTAG iodef_mod_dispatch(struct THook *hook, TAPTR obj,
	TTAG msg);
static struct TIOPacket *iodef_mod_open(IODEF_MOD *mod, TTAGITEM *tags);
static void iodef_mod_close(IODEF_MOD *mod);
static void iodef_mod_task(struct TTask *task);
static void iodef_init(IODEF_MOD *mod);

static TMODAPI void iodef_beginio(IODEF_MOD *mod, struct TIOPacket *msg);
static TMODAPI TINT iodef_abortio(IODEF_MOD *mod, struct TIOPacket *msg);

/*****************************************************************************/

static TAPTR
iodef_alloc(IODEF_MOD *mod, TUINT size)
{
	return TExecAlloc(mod->iod_ExecBase, mod->iod_MemManager, size);
}

static void
iodef_freelist(IODEF_MOD *mod, struct TList *list)
{
	struct TNode *n;
	while ((n = TRemHead(list)))
		TExecFree(mod->iod_ExecBase, n);
}

static void
iodef_closeall(IODEF_MOD *mod)
{
	if (mod->iod_IOTask)
	{
		TExecSignal(mod->iod_ExecBase, mod->iod_IOTask, TTASK_SIG_ABORT);
		TDestroy((struct THandle *) mod->iod_IOTask);
		mod->iod_IOTask = TNULL;
	}
	TExecCloseModule(mod->iod_ExecBase, (struct TModule *) mod->iod_UtilBase);
}

/*****************************************************************************/
/*
**	module init/exit
*/

TMODENTRY TUINT
tek_init_iohnd_default(struct TTask *task, struct TModule *ioh,
	TUINT16 version, TTAGITEM *tags)
{
	IODEF_MOD *mod = (IODEF_MOD *) ioh;

	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * 8;	/* negative size */

		if (version <= IODEF_VERSION)
			return sizeof(IODEF_MOD);	/* positive size */

		return 0;
	}

	mod->iod_ExecBase = TGetExecBase(mod);
	mod->iod_Lock = TExecCreateLock(mod->iod_ExecBase, TNULL);
	if (mod->iod_Lock)
	{
		mod->iod_Module.tmd_Version = IODEF_VERSION;
		mod->iod_Module.tmd_Revision = IODEF_REVISION;

		mod->iod_Module.tmd_Handle.thn_Hook.thk_Entry = iodef_mod_dispatch;
		mod->iod_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;

		#ifdef TDEBUG
		mod->iod_MemManager = TExecCreateMemManager(mod->iod_ExecBase, TNULL,
			TMMT_Tracking | TMMT_TaskSafe, TNULL);
		#endif

		((TAPTR *) mod)[-3] = (TAPTR) iodef_beginio;
		((TAPTR *) mod)[-4] = (TAPTR) iodef_abortio;

		return TTRUE;
	}

	return 0;
}


static THOOKENTRY TTAG
iodef_mod_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	IODEF_MOD *mod = (IODEF_MOD *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			TDestroy((struct THandle *) mod->iod_MemManager);
			TDestroy((struct THandle *) mod->iod_Lock);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) iodef_mod_open(mod, obj);
		case TMSG_CLOSEMODULE:
			iodef_mod_close(obj);
			break;
		case TMSG_INITTASK:
			return TTRUE;
		case TMSG_RUNTASK:
			iodef_mod_task(obj);
	}
	return 0;
}

/*****************************************************************************/
/*
**	module open/close
*/

static struct TIOPacket *
iodef_mod_open(IODEF_MOD *mod, TTAGITEM *tags)
{
	struct TIOPacket *msg = TExecAllocMsg0(mod->iod_ExecBase,
		sizeof(struct TIOPacket));
	if (msg)
	{
		struct TExecBase *TExecBase = TGetExecBase(mod);
		TLock(mod->iod_Lock);
		if (mod->iod_IOTask == TNULL)
		{
			mod->iod_UtilBase =
				(struct TUtilBase *) TOpenModule("util", 0, TNULL);
			if (mod->iod_UtilBase)
			{
				TTAGITEM tasktags[2];
				tasktags[0].tti_Tag = TTask_UserData;
				tasktags[0].tti_Value = (TTAG) mod;
				tasktags[1].tti_Tag = TTAG_DONE;
				mod->iod_IOTask =
					TCreateTask(&mod->iod_Module.tmd_Handle.thn_Hook,
					tasktags);
				if (mod->iod_IOTask)
					iodef_init(mod);
			}
			else
				iodef_closeall(mod);
		}

		if (mod->iod_IOTask)
		{
			/* insert recommended buffer settings */
			msg->io_BufSize = IODEF_BUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_FULL;
			msg->io_Req.io_Device = (struct TModule *) mod;
			mod->iod_RefCount++;
		}
		else
		{
			TFree(msg);
			msg = TNULL;
		}

		TUnlock(mod->iod_Lock);
	}

	return msg;
}

static void iodef_mod_close(IODEF_MOD *mod)
{
	TExecLock(mod->iod_ExecBase, mod->iod_Lock);
	if (mod->iod_IOTask && --mod->iod_RefCount == 0)
		iodef_closeall(mod);
	TExecUnlock(mod->iod_ExecBase, mod->iod_Lock);
}

/*****************************************************************************/
/*
**	add available devices to list
*/

static TBOOL
iodef_getdevlist(IODEF_MOD *mod, struct TList *list)
{
	TSTRPTR buf;
	DWORD blen;
	TBOOL success = TFALSE;

	blen = GetLogicalDriveStrings(0, NULL);
	if (blen > 0)
	{
		TSTRPTR devlist = buf = iodef_alloc(mod, blen);
		if (buf)
		{
			if (GetLogicalDriveStrings(blen, buf) == blen - 1)
			{
				success = TTRUE;
				while (*buf)
				{
					TSTRPTR p = buf;
					struct THandle *n;
					TINT l = TStrLen(p);
					buf += l + 1;
					if (l > 2 &&
						!TStrCmp(p + l - 2, ":\\"))
						l -= 2;
					n = iodef_alloc(mod, sizeof(struct THandle) + l + 1);
					if (n)
					{
						TExecCopyMem(mod->iod_ExecBase, p, n + 1, l);
						((TSTRPTR)(n + 1))[l] = 0;
						n->thn_Hook.thk_Data = n + 1;
						TAddTail(list, (struct TNode *) n);
						continue;
					}

					iodef_freelist(mod, list);
					success = TFALSE;
					break;
				}
			}
			TExecFree(mod->iod_ExecBase, devlist);
		}
	}
	return success;
}

/*****************************************************************************/
/*
**	ioerr = geterror()
**	translate host error to TEKlib ioerr
*/

static TINT
iodef_geterror(void)
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
		case ERROR_SHARING_VIOLATION:
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
			#ifdef TDEBUG
			{
				LPVOID buf;
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &buf, 0, NULL);
				TDBPRINTF(20,("error in messagebox: %d\n", (TUINT) err));
				MessageBox(NULL, buf, "iohnd_default Debug",
					MB_OK | MB_ICONINFORMATION);
				LocalFree(buf);
			}
			#endif
			TDBFATAL();
	}

	return 0;
}

/*****************************************************************************/
/*
**	winname = getwinname(mod, tekname, winextra)
**		"foo/bar/bla/fasel/", NULL
**	->	"foo:\bar\bla\fasel"
**		"foo", ""
**	->	"foo:"
**		"foo/bar/bla/fasel/", "\\*"
**	->	"foo:\bar\bla\fasel\*"
*/

static TSTRPTR
iodef_getwinname(IODEF_MOD *mod, TSTRPTR name, TSTRPTR winextra)
{
	TSTRPTR winname = TNULL;
	TINT len = TStrLen(name);
	if (len)
	{
		TINT elen = TStrLen(winextra);
		winname = iodef_alloc(mod, len + elen + 3);
		if (winname)
		{
			TSTRPTR p, d;
			TINT c;

			p = TStrChr(name, '/');
			if (p)
				len = p++ - name;
			else
				p = name + len;

			TStrNCpy(winname, name, len);

			d = winname;
			d[len++] = ':';
			d[len] = '\\';

			do
			{
				c = *p++;
				if (c == '/')
					c = '\\';
				d[++len] = c;

			} while (c);

			/* erase trailing winslash */
			if (d[len - 1] == '\\')
				d[--len] = 0;

			TStrCpy(d + len, winextra);
		}
	}

	return winname;
}

/*****************************************************************************/
/*
**	winname = getcurrentdir(mod)
*/

static TSTRPTR
iodef_getcurrentdir(IODEF_MOD *mod)
{
	TSTRPTR name = TNULL;
	TINT len = GetCurrentDirectory(0, NULL);
	if (len)
	{
		name = iodef_alloc(mod, len);
		if (name)
			GetCurrentDirectory(len, name);
	}
	return name;
}

/*****************************************************************************/
/*
**	tekname = gettekname(mod, mmu, winname, extra_chars_in_front)
**		foo:\bar\bla\fasel
**	->	foo/bar/bla/fasel
**		c:\
**	->	c
*/

static TSTRPTR
iodef_gettekname(IODEF_MOD *mod, TAPTR mmu, TSTRPTR winname, TINT addspace)
{
	TSTRPTR tekname = TNULL;
	TSTRPTR p;

	p = TStrChr(winname, ':');
	if (p)
	{
		TINT len = TStrLen(winname);
		tekname = iodef_alloc(mod, len + addspace);
		if (tekname)
		{
			TSTRPTR d = tekname + addspace;
			TINT c;

			len = p - winname;
			TStrNCpy(d, winname, len);

			d += len;
			p++;

			do
			{
				c = *p++;
				if (c == '\\')
					c = '/';
				*d++ = c;

			} while (c);
		}
	}

	return tekname;
}

/*****************************************************************************/
/*
**	success = addassign(mod, assign, winpath)
*/

static TBOOL
iodef_addassign(IODEF_MOD *mod, TAPTR TIOBase, TSTRPTR assign, TSTRPTR winpath)
{
	if (assign && winpath)
	{
		TSTRPTR t = iodef_gettekname(mod, mod->iod_MemManager, winpath, 1);
		if (t)
		{
			t[0] = ':';
			TAssignLate(assign, t);
			TExecFree(mod->iod_ExecBase, t);
			return TTRUE;
		}
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	iodef_init
**	add late-binding assigns, such as CURRENTDIR:, PROGDIR: and SYS:.
**	note: calls to iobase are allowed here, because in the instance
**	open function we're running in the same task context as the module
**	opener, who already holds locks to the base.
*/

static void
iodef_init(IODEF_MOD *mod)
{
	TAPTR TIOBase = TExecOpenModule(mod->iod_ExecBase, "io", 0, TNULL);
	if (TIOBase)
	{
		TAPTR hal = TExecGetHALBase(mod->iod_ExecBase);
		TCHR buf[2048];
		TINT res;
		TSTRPTR name;

		/* add CURRENTDIR: */
		name = iodef_getcurrentdir(mod);
		iodef_addassign(mod, TIOBase, "CURRENTDIR", name);
		TExecFree(mod->iod_ExecBase, name);

		/* add PROGDIR: */
		name = (TSTRPTR) THALGetAttr(hal, TExecBase_ProgDir, TNULL);
		iodef_addassign(mod, TIOBase, "PROGDIR", name);

		/* add SYS: */
		name = (TSTRPTR) THALGetAttr(hal, TExecBase_SysDir, TNULL);
		iodef_addassign(mod, TIOBase, "SYS", name);

		/* add ENVARC: */
		TAssignLate("ENVARC", "SYS:prefs/env-archive");

		/* add T: */
		res = ExpandEnvironmentStrings("%TMP%", buf, sizeof(buf));
		if (res > 0 && res <= sizeof(buf))
			iodef_addassign(mod, TIOBase, "T", buf);

		TExecCloseModule(mod->iod_ExecBase, TIOBase);
	}
}

/*****************************************************************************/
/*
**	iodef_open
*/

static void
iodef_open(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_LOCK *flock;
	DWORD accessmode, sharemode, createmode;
	TUINT lockmode;

	msg->io_Op.Open.Result = TFALSE;

	switch (msg->io_Op.Open.Mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;

		case TFMODE_READONLY:
			accessmode = GENERIC_READ;
			sharemode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			createmode = OPEN_EXISTING;
			lockmode = TFLOCK_NONE;
			break;

		case TFMODE_OLDFILE:
			accessmode = GENERIC_READ | GENERIC_WRITE;
			sharemode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			createmode = OPEN_EXISTING;
			lockmode = TFLOCK_NONE;
			break;

		case TFMODE_NEWFILE:
			accessmode = GENERIC_READ | GENERIC_WRITE;
			sharemode = 0;
			createmode = CREATE_ALWAYS;
			lockmode = TFLOCK_WRITE;
			break;

		case TFMODE_READWRITE:
			accessmode = GENERIC_READ | GENERIC_WRITE;
			sharemode = FILE_SHARE_READ;
			createmode = OPEN_ALWAYS;
			lockmode = TFLOCK_READ;
			break;
	}

	msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	flock = iodef_alloc(mod, sizeof(IODEF_LOCK));
	if (flock)
	{
		flock->winname = iodef_getwinname(mod, msg->io_Op.Open.Name, TNULL);
		if (flock->winname)
		{
			flock->filehandle = CreateFile(flock->winname, accessmode,
				sharemode, NULL, createmode, FILE_ATTRIBUTE_NORMAL, NULL);
			if (flock->filehandle != INVALID_HANDLE_VALUE)
			{
				flock->dirfindhandle = INVALID_HANDLE_VALUE;
				flock->flags = IODEFL_FILE;
				flock->lockmode = lockmode;
				flock->mod = mod;
				msg->io_FLock = flock;
				msg->io_Op.Open.Result = TTRUE;
				msg->io_Req.io_Error = 0;
				return;
			}
			msg->io_Req.io_Error = iodef_geterror();
			TExecFree(mod->iod_ExecBase, flock->winname);
		}
		TExecFree(mod->iod_ExecBase, flock);
	}
}

/*****************************************************************************/
/*
**	iodef_lock
*/

static void
iodef_lock(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_LOCK *flock;
	DWORD accessmode, sharemode, createmode;
	TUINT lockmode;
	TUINT mode = msg->io_Op.Lock.Mode;

	msg->io_Op.Lock.Result = TFALSE;

	switch (mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;

		case TFLOCK_READ:
			accessmode = GENERIC_READ /* | GENERIC_WRITE*/;
			sharemode = FILE_SHARE_READ;
			createmode = OPEN_EXISTING;
			lockmode = TFLOCK_READ;
			break;

		case TFLOCK_WRITE:
			accessmode = GENERIC_READ | GENERIC_WRITE;
			sharemode = 0;
			createmode = OPEN_EXISTING;
			lockmode = TFLOCK_WRITE;
			break;
	}

	msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	flock = iodef_alloc(mod, sizeof(IODEF_LOCK));
	if (flock)
	{
		flock->filehandle = INVALID_HANDLE_VALUE;
		flock->dirfindhandle = INVALID_HANDLE_VALUE;
		if (msg->io_Op.Lock.Name[0] == 0)
		{
			/* empty string; lock on the filesystem root */
			flock->flags = IODEFL_LOCK | IODEFL_ISDIR | IODEFL_ISROOT;
			flock->lockmode = lockmode;
			flock->mod = mod;
			msg->io_FLock = flock;
			msg->io_Op.Lock.Result = TTRUE;
			msg->io_Req.io_Error = 0;
			return;
		}

		flock->winname = iodef_getwinname(mod, msg->io_Op.Lock.Name, TNULL);
		if (flock->winname)
		{
			/* first try to lock it as a directory */
			TSTRPTR winfindname =
				iodef_getwinname(mod, msg->io_Op.Lock.Name, "\\*");
			if (winfindname)
			{
				flock->dirfindhandle = FindFirstFile(winfindname,
					&flock->dirfinddata);
				TExecFree(mod->iod_ExecBase, winfindname);
				if (flock->dirfindhandle != INVALID_HANDLE_VALUE)
				{
					flock->flags = IODEFL_LOCK | IODEFL_ISDIR;
					flock->lockmode = lockmode;
					flock->mod = mod;
					msg->io_FLock = flock;
					msg->io_Op.Lock.Result = TRUE;
					msg->io_Req.io_Error = 0;
					return;
				}
			}

			/* try to open a file instead */
			flock->filehandle = CreateFile(flock->winname, accessmode,
				sharemode, NULL, createmode, FILE_ATTRIBUTE_NORMAL, NULL);
			if (flock->filehandle != INVALID_HANDLE_VALUE)
			{
				flock->flags = IODEFL_LOCK;
				flock->lockmode = lockmode;
				flock->mod = mod;
				msg->io_FLock = flock;
				msg->io_Op.Lock.Result = TTRUE;
				msg->io_Req.io_Error = 0;
				return;
			}
			msg->io_Req.io_Error = iodef_geterror();
			TExecFree(mod->iod_ExecBase, flock->winname);
		}

		TExecFree(mod->iod_ExecBase, flock);
	}
}

/*****************************************************************************/
/*
**	iodef_close - applies to both locks and files
*/

static void
iodef_close(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_EXAMINE *e = msg->io_Examine;
	IODEF_LOCK *f = msg->io_FLock;

	if (e)
	{
		iodef_freelist(mod, &e->devlist);
		iodef_freelist(mod, &e->devlist_done);
		if (e->filefindhandle)
			FindClose(e->filefindhandle);
		TExecFree(mod->iod_ExecBase, e->tempdrivename);
		TExecFree(mod->iod_ExecBase, e);
	}

	if (f->filehandle != INVALID_HANDLE_VALUE)
	{
		if (!CloseHandle(f->filehandle))
			msg->io_Req.io_Error = iodef_geterror();
	}

	if (f->dirfindhandle != INVALID_HANDLE_VALUE)
		FindClose(f->dirfindhandle);

	TExecFree(mod->iod_ExecBase, f->winname);
	TExecFree(mod->iod_ExecBase, f);
}

/*****************************************************************************/
/*
**	iodef_write
*/

static void
iodef_write(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_LOCK *fl = msg->io_FLock;
	TINT res = -1;

	if (fl->flags & IODEFL_FILE)
	{
		DWORD wrlen = 0;
		if (WriteFile(fl->filehandle, msg->io_Op.Write.Buf,
			msg->io_Op.Write.Len, &wrlen, NULL))
			res = (TINT) wrlen;
		else
			msg->io_Req.io_Error = iodef_geterror();
	}
	else
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;

	msg->io_Op.Write.WrLen = res;
}

/*****************************************************************************/
/*
**	iodef_read
*/

static void
iodef_read(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_LOCK *fl = msg->io_FLock;
	TINT res = -1;

	if (fl->flags & IODEFL_FILE)
	{
		DWORD rdlen = 0;
		if (ReadFile(fl->filehandle, msg->io_Op.Read.Buf,
			msg->io_Op.Read.Len, &rdlen, NULL))
			res = (TINT) rdlen;
		else
			msg->io_Req.io_Error = iodef_geterror();
	}
	else
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;

	msg->io_Op.Read.RdLen = res;
}

/*****************************************************************************/
/*
**	examinefinddata(mod, examine, winfinddata)
*/

static void
iodef_examinefinddata(IODEF_MOD *mod, IODEF_EXAMINE *e, LPWIN32_FIND_DATA fp)
{
	SYSTEMTIME st;

	e->size = fp->nFileSizeLow;
	e->sizehi = fp->nFileSizeHigh;
	e->name = fp->cFileName;

	if (fp->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		e->type = TFTYPE_Directory;
	else
		e->type = TFTYPE_File;

	FileTimeToSystemTime(&fp->ftLastWriteTime, &st);
	e->datebox.tdb_Year = st.wYear;
	e->datebox.tdb_Month = (TUINT8) st.wMonth;
	e->datebox.tdb_Day = (TUINT8) st.wDay;
	e->datebox.tdb_Hour = (TUINT8) st.wHour;
	e->datebox.tdb_Minute = (TUINT8) st.wMinute;
	e->datebox.tdb_USec = (st.wSecond * 1000 + st.wMilliseconds) * 1000;
	e->datebox.tdb_Fields =
		TDB_YEAR|TDB_MONTH|TDB_DAY|TDB_HOUR|TDB_MINUTE|TDB_USEC;
}

/*****************************************************************************/
/*
**	examinetags(examine, tagitem)
**	fill examination info into user-supplied tags. this
**	is called back by TForEachTag()
*/

static THOOKENTRY TTAG
iodef_examinetags(struct THook *hook, TAPTR obj, TTAG msg)
{
	IODEF_EXAMINE *e = hook->thk_Data;
	TTAGITEM *ti = obj;

	switch ((TUINT) ti->tti_Tag)
	{
		default:
			return TTRUE;

		case TFATTR_Name:
			*((TSTRPTR *) ti->tti_Value) = e->name;
			break;

		case TFATTR_Size:
			if (!e->havesizehi && e->size >= 0xffffffff)
			{
				*((TUINT *) ti->tti_Value) = 0xffffffff;
				break;
			}
			*((TUINT *) ti->tti_Value) = e->size & 0xffffffff;
			break;

		case TFATTR_SizeHigh:
			*((TUINT *) ti->tti_Value) = e->sizehi;
			break;

		case TFATTR_Type:
			*((TINT *) ti->tti_Value) = e->type;
			break;

		case TFATTR_Date:
		{
			IODEF_MOD *mod = e->mod;
			TUtilPackDate(mod->iod_UtilBase, &e->datebox,
				(TDATE *) ti->tti_Value);
			break;
		}

		case TFATTR_DateBox:
			*((struct TDateBox *) ti->tti_Value) = e->datebox;
			break;
	}

	e->numattr++;
	return TTRUE;
}

/*****************************************************************************/
/*
**	iodef_examine
**	examine object. if successful, place a handler-specific examination
**	handle into the iomsg - this is how the IO module knows whether an
**	object has been examined already. note that both files and locks must
**	be supported.
*/

static void
iodef_examine(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_LOCK *fl = msg->io_FLock;
	IODEF_EXAMINE *e = msg->io_Examine;
	msg->io_Op.Examine.Result = -1;

	if (!e)
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;

		e = TExecAlloc0(mod->iod_ExecBase, mod->iod_MemManager,
			sizeof(IODEF_EXAMINE));
		if (e)
		{
			TBOOL success = TFALSE;

			e->mod = mod;
			TInitList(&e->devlist);
			TInitList(&e->devlist_done);

			if (fl->flags & IODEFL_ISROOT)
			{
				if (iodef_getdevlist(mod, &e->devlist))
				{
					TDATE date;
					e->name = "";
					e->size = 0;
					e->type = TFTYPE_Directory;
					TExecLock(mod->iod_ExecBase, mod->iod_Lock);
					TExecGetLocalDate(mod->iod_ExecBase, &date);
					TExecUnlock(mod->iod_ExecBase, mod->iod_Lock);
					TUtilUnpackDate(mod->iod_UtilBase, &date,
						&e->datebox, TDB_ALL);
					success = TTRUE;
				}
			}
			else
			{
				TBOOL isdrivename = TFALSE;
				if (fl->flags & IODEFL_ISDIR)
				{
					TINT l = TStrLen(fl->winname);
					if (l > 1 && fl->winname[l - 1] == ':')
					{
						isdrivename = TTRUE;
						/* we need a copy without the colon */
						e->tempdrivename = TUtilStrNDup(mod->iod_UtilBase,
							mod->iod_MemManager, fl->winname, l - 1);
					}
				}

				if (isdrivename)
				{
					/* this path is a drive name and colon only, like "c:" */
					if (e->tempdrivename)
					{
						TDATE date;
						e->name = e->tempdrivename;
						e->size = 0;
						e->type = TFTYPE_Directory | TFTYPE_Volume;
						TExecLock(mod->iod_ExecBase, mod->iod_Lock);
						TExecGetLocalDate(mod->iod_ExecBase, &date);
						TExecUnlock(mod->iod_ExecBase, mod->iod_Lock);
						TUtilUnpackDate(mod->iod_UtilBase, &date, &e->datebox,
							TDB_ALL);
						success = TTRUE;
					}
					else
						msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
				}
				else
				{
					/* regular files and directories need a findhandle */
					e->filefindhandle = FindFirstFile(fl->winname,
						&e->filefinddata);
					if (e->filefindhandle != INVALID_HANDLE_VALUE)
					{
						iodef_examinefinddata(mod, e, &e->filefinddata);
						success = TTRUE;
					}
					else
						msg->io_Req.io_Error = iodef_geterror();
				}
			}

			if (success)
			{
				e->flock = fl;
				msg->io_Examine = e;
			}
			else
			{
				TExecFree(mod->iod_ExecBase, e);
				return;
			}
		}
	}

	if (e)
	{
		struct THook hook;

		e->havesizehi =
			TGetTag(msg->io_Op.Examine.Tags, TFATTR_SizeHigh, TNULL);
		e->numattr = 0;

		TInitHook(&hook, iodef_examinetags, e);
		TForEachTag(msg->io_Op.Examine.Tags, &hook);

		msg->io_Op.Examine.Result = e->numattr;
		msg->io_Req.io_Error = 0;
	}
}

/*****************************************************************************/
/*
**	iodef_exnext
**	examine next entry. only directory locks are supported.
**	note: the IO module guarantees that the directory lock
**	has been examined once before an exnext is issued.
*/

static void
iodef_exnext(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_EXAMINE *e = msg->io_Examine;
	IODEF_LOCK *fl = e->flock;
	struct THook hook;
	msg->io_Op.Examine.Result = -1;

	if (!(fl->flags & IODEFL_ISDIR))
	{
		/* not a directory lock */
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
		return;
	}

	if (fl->flags & IODEFL_ISROOT)
	{
		/* link next node from devlist to devlist_done, examine it */
		struct THandle *h = (struct THandle *) TRemHead(&e->devlist);
		if (h)
		{
			e->name = h->thn_Hook.thk_Data;
			e->size = 0;
			e->type = TFTYPE_Directory | TFTYPE_Volume;
			TAddTail(&e->devlist_done, (struct TNode *) h);
			e->numattr = 0;
			e->havesizehi =
				TGetTag(msg->io_Op.Examine.Tags, TFATTR_SizeHigh, TNULL);

			TInitHook(&hook, iodef_examinetags, e);
			TForEachTag(msg->io_Op.Examine.Tags, &hook);

			msg->io_Op.Examine.Result = e->numattr;
			return;
		}
		/* else no more entries */
	}
	else
	{
		while (FindNextFile(fl->dirfindhandle, &fl->dirfinddata))
		{
			/* the ".." entry does not exist in our world */
			if (!TStrCmp(
				fl->dirfinddata.cFileName, "..")) continue;
			iodef_examinefinddata(mod, e, &fl->dirfinddata);
			e->numattr = 0;
			e->havesizehi =
				TGetTag(msg->io_Op.Examine.Tags, TFATTR_SizeHigh, TNULL);

			TInitHook(&hook, iodef_examinetags, e);
			TForEachTag(msg->io_Op.Examine.Tags, &hook);

			msg->io_Op.Examine.Result = e->numattr;
			return;
		}
	}

	msg->io_Req.io_Error = TIOERR_NO_MORE_ENTRIES;
}

/*****************************************************************************/
/*
**	iodef_openfromlock
**	turn a lock into an open filehandle
*/

static void
iodef_openfromlock(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_LOCK *fl = msg->io_FLock;
	if (fl->flags & (IODEFL_ISDIR | IODEFL_FILE))
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
		msg->io_Op.OpenFromLock.Result = TFALSE;
	}
	else
	{
		fl->flags = IODEFL_FILE;
		msg->io_Op.OpenFromLock.Result = TTRUE;
	}
}

/*****************************************************************************/
/*
**	iodef_seek
**	seek in a file
*/

static void
iodef_seek(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_LOCK *fl = msg->io_FLock;
	DWORD method, res;

	msg->io_Op.Seek.Result = 0xffffffff;

	switch (msg->io_Op.Seek.Mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;

		case TFPOS_BEGIN:
			method = FILE_BEGIN;
			break;

		case TFPOS_CURRENT:
			method = FILE_CURRENT;
			break;

		case TFPOS_END:
			method = FILE_END;
			break;
	}

	if (!(fl->flags & IODEFL_FILE))
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
		return;
	}

	if (msg->io_Op.Seek.OffsHi)
	{
		res = SetFilePointer(fl->filehandle, msg->io_Op.Seek.Offs,
			(PLONG) msg->io_Op.Seek.OffsHi, method);
		if (res == 0xffffffff)
		{
			TINT err = iodef_geterror();
			if (err)
			{
				msg->io_Req.io_Error = err;
				return;
			}
		}
	}
	else
	{
		res = SetFilePointer(fl->filehandle, msg->io_Op.Seek.Offs,
			NULL, method);
		if (res == 0xffffffff)
		{
			msg->io_Req.io_Error = iodef_geterror();
			return;
		}
	}

	msg->io_Op.Seek.Result = res;
}

/*****************************************************************************/
/*
**	iodef_rename
*/

static void
iodef_rename(IODEF_MOD *mod, struct TIOPacket *msg)
{
	TSTRPTR srcname, dstname;

	msg->io_Op.Rename.Result = TFALSE;

	srcname = iodef_getwinname(mod, msg->io_Op.Rename.SrcName, TNULL);
	dstname = iodef_getwinname(mod, msg->io_Op.Rename.DstName, TNULL);

	if (srcname && dstname)
	{
		if (MoveFile(srcname, dstname))
			msg->io_Op.Rename.Result = TTRUE;
		else
			msg->io_Req.io_Error = iodef_geterror();
	}
	else
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;

	TExecFree(mod->iod_ExecBase, dstname);
	TExecFree(mod->iod_ExecBase, srcname);
}

/*****************************************************************************/
/*
**	iodef_makedir
*/

static void
iodef_makedir(IODEF_MOD *mod, struct TIOPacket *msg)
{
	TSTRPTR name = iodef_getwinname(mod, msg->io_Op.Lock.Name, TNULL);
	msg->io_Op.Lock.Result = TFALSE;
	if (name)
	{
		if (CreateDirectory(name, NULL))
			iodef_lock(mod, msg);
		else
			msg->io_Req.io_Error = iodef_geterror();
		TExecFree(mod->iod_ExecBase, name);
	}
	else
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
}

/*****************************************************************************/
/*
**	iodef_deleteobject
*/

static void
iodef_delete(IODEF_MOD *mod, struct TIOPacket *msg)
{
	TSTRPTR name = iodef_getwinname(mod, msg->io_Op.Delete.Name, TNULL);
	msg->io_Op.Delete.Result = TFALSE;
	if (name)
	{
		if (DeleteFile(name))
			msg->io_Op.Delete.Result = TTRUE;
		else
		{
			msg->io_Req.io_Error = iodef_geterror();
			if (msg->io_Req.io_Error == TIOERR_ACCESS_DENIED)
			{
				if (RemoveDirectory(name))
				{
					msg->io_Op.Delete.Result = TTRUE;
					msg->io_Req.io_Error = 0;
				}
				else
					msg->io_Req.io_Error = iodef_geterror();
			}
		}
		TExecFree(mod->iod_ExecBase, name);
	}
	else
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
}

/*****************************************************************************/
/*
**	iodef_setdate
*/

static void
iodef_setdate(IODEF_MOD *mod, struct TIOPacket *msg)
{
	TSTRPTR fname = iodef_getwinname(mod, msg->io_Op.SetDate.Name, TNULL);
	msg->io_Op.SetDate.Result = TFALSE;
	if (fname)
	{
		HANDLE fh = CreateFile(fname, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (fh != INVALID_HANDLE_VALUE)
		{
			LARGE_INTEGER ft;
			ft.QuadPart = msg->io_Op.SetDate.Date->tdt_Int64 * 10;
			if (SetFileTime(fh, NULL, NULL, (LPFILETIME) &ft))
				msg->io_Op.SetDate.Result = TTRUE;
			else
				msg->io_Req.io_Error = iodef_geterror();
			CloseHandle(fh);
		}
		else
			msg->io_Req.io_Error = iodef_geterror();

		TExecFree(mod->iod_ExecBase, fname);
	}
	else
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
}

/*****************************************************************************/
/*
**	iodef_makename
*/

static void
iodef_makename(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_LOCK *flock = msg->io_FLock;
	TSTRPTR name = msg->io_Op.MakeName.Name;
	TSTRPTR dest = msg->io_Op.MakeName.Dest;
	TINT res = -1;

	msg->io_Op.MakeName.Result = -1;
	msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;

	if (msg->io_Op.MakeName.Mode == TPPF_HOST2TEK)
	{
		TSTRPTR src, temp;
		TINT len, len2 = TStrLen(name);

		if (TStrChr(name, ':'))
		{
			src = TNULL; /* absolute */
			len = 0;
		}
		else
		{
			src = flock->winname;
			len = TStrLen(src) + 1;
		}

		temp = iodef_alloc(mod, len + len2 + 1);
		if (temp)
		{
			TSTRPTR tekname = TNULL;

			if (src)
			{
				TStrCpy(temp, src);
				temp[len - 1] = '\\';
			}

			TStrCpy(temp + len, name);

			len = GetFullPathName(temp, 0, NULL, 0);
			if (len)
			{
				TSTRPTR winabsname = iodef_alloc(mod, len + 1);
				if (winabsname)
				{
					if (GetFullPathName(temp, len + 1, winabsname, TNULL))
						tekname = iodef_gettekname(mod, mod->iod_MemManager,
							winabsname, 1);

					if (tekname)
						tekname[0] = ':';
					else
					{
						TExecFree(mod->iod_ExecBase, tekname);
						tekname = TNULL;
					}

					TExecFree(mod->iod_ExecBase, winabsname);
				}
			}

			if (tekname)
			{
				tekname[0] = ':';
				res = TStrLen(tekname);
				if (dest)
				{
					if (res < msg->io_Op.MakeName.DLen)
					{
						TStrCpy(dest, tekname);
						dest[0] = ':';
					}
					else
					{
						res = -1;
						msg->io_Req.io_Error = TIOERR_LINE_TOO_LONG;
					}
				}
				TExecFree(mod->iod_ExecBase, tekname);
			}
			else
			{
				res = -1;
				msg->io_Req.io_Error = iodef_geterror();
			}
			TExecFree(mod->iod_ExecBase, temp);
		}
	}
	else if (msg->io_Op.MakeName.Mode == TPPF_TEK2HOST)
	{
		TSTRPTR winname = iodef_getwinname(mod, name, TNULL);
		if (winname)
		{
			res = TStrLen(winname);
			if (res < msg->io_Op.MakeName.DLen)
			{
				if (dest)
					TStrCpy(dest, winname);
			}
			else
			{
				res = -1;
				msg->io_Req.io_Error = TIOERR_LINE_TOO_LONG;
			}
			TExecFree(mod->iod_ExecBase, winname);
		}
	}

	msg->io_Op.MakeName.Result = res;
}

/*****************************************************************************/
/*
**	iodef_execute
**	execute command with the lock as currentdir
*/

static TSTRPTR
iodef_makecmd(IODEF_MOD *mod, TSTRPTR cmdline, TSTRPTR args)
{
	if (cmdline && args)
	{
		TINT len = TStrLen(cmdline);
		TSTRPTR ncmdline = TExecRealloc(mod->iod_ExecBase, cmdline,
			len + 2 + TStrLen(args));
		if (ncmdline)
		{
			ncmdline[len] = ' ';
			TStrCpy(ncmdline + len + 1, args);
			cmdline = ncmdline;
		}
		else
		{
			TExecFree(mod->iod_ExecBase, cmdline);
			cmdline = TNULL;
		}
	}
	return cmdline;
}

static void
iodef_execute(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_LOCK *flock = msg->io_FLock;
	msg->io_Op.Execute.Result = -1;
	if (flock && (flock->flags & IODEFL_ISDIR))
	{
		TSTRPTR cmdline =
			iodef_getwinname(mod, msg->io_Op.Execute.Command, TNULL);
		cmdline = iodef_makecmd(mod, cmdline, msg->io_Op.Execute.Args);
		if (cmdline)
		{
			PROCESS_INFORMATION pi;
			STARTUPINFO si;
			GetStartupInfo(&si);

			if (CreateProcess(NULL, cmdline,
				NULL, NULL, FALSE, 0, NULL, flock->winname, &si, &pi))
			{
				DWORD ret;
				WaitForSingleObjectEx(pi.hThread, INFINITE, FALSE);
				if (GetExitCodeProcess(pi.hProcess, &ret))
					msg->io_Op.Execute.Result = ret;
				else
					msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			}
			else
				msg->io_Req.io_Error = iodef_geterror();

			TExecFree(mod->iod_ExecBase, cmdline);
		}
		else
			msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	}
	else
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
}

/*****************************************************************************/
/*
**	iodef_docmd
*/

static void
iodef_docmd(IODEF_MOD *mod, struct TIOPacket *msg)
{
	msg->io_Req.io_Error = 0;

	switch (msg->io_Req.io_Command)
	{
		default:
			msg->io_Req.io_Error = TIOERR_UNKNOWN_COMMAND;
			break;

		case TIOCMD_OPEN:
			iodef_open(mod, msg);
			break;

		case TIOCMD_LOCK:
			iodef_lock(mod, msg);
			break;

		case TIOCMD_UNLOCK:
		case TIOCMD_CLOSE:
			iodef_close(mod, msg);
			break;

		case TIOCMD_READ:
			iodef_read(mod, msg);
			break;

		case TIOCMD_WRITE:
			iodef_write(mod, msg);
			break;

		case TIOCMD_SEEK:
			iodef_seek(mod, msg);
			break;

		case TIOCMD_EXAMINE:
			iodef_examine(mod, msg);
			break;

		case TIOCMD_EXNEXT:
			iodef_exnext(mod, msg);
			break;

		case TIOCMD_OPENFROMLOCK:
			iodef_openfromlock(mod, msg);
			break;

		case TIOCMD_RENAME:
			iodef_rename(mod, msg);
			break;

		case TIOCMD_MAKEDIR:
			iodef_makedir(mod, msg);
			break;

		case TIOCMD_DELETE:
			iodef_delete(mod, msg);
			break;

		case TIOCMD_SETDATE:
			iodef_setdate(mod, msg);
			break;

		case TIOCMD_MAKENAME:
			iodef_makename(mod, msg);
			break;

		case TIOCMD_EXECUTE:
			iodef_execute(mod, msg);
			break;
	}
}

/*****************************************************************************/
/*
**	handler task
*/

static void iodef_mod_task(struct TTask *task)
{
	TAPTR exec = TGetExecBase(task);
	IODEF_MOD *mod = TExecGetTaskData(exec, task);
	TAPTR port = TExecGetUserPort(mod->iod_ExecBase, task);
	TUINT portsig = TExecGetPortSignal(mod->iod_ExecBase, port);
	struct TIOPacket *msg;
	TUINT sigs;

	do
	{
		sigs = TExecWait(mod->iod_ExecBase, portsig | TTASK_SIG_ABORT);
		if (sigs & portsig)
		{
			while ((msg = TExecGetMsg(mod->iod_ExecBase, port)))
			{
				iodef_docmd(mod, msg);
				TExecReplyMsg(mod->iod_ExecBase, msg);
			}
		}

	} while (!(sigs & TTASK_SIG_ABORT));
}

/*****************************************************************************/
/*
**	beginio(msg->io_Device, msg)
*/

static TMODAPI void
iodef_beginio(IODEF_MOD *mod, struct TIOPacket *msg)
{
	if (msg->io_Req.io_Flags & TIOF_QUICK)
	{
		/* do synchronously */
		msg->io_Req.io_Flags &= ~TIOF_QUICK;
		iodef_docmd(mod, msg);
	}
	else
	{
		/* do asynchronously */
		TExecPutMsg(mod->iod_ExecBase,
			TExecGetUserPort(mod->iod_ExecBase, mod->iod_IOTask),
			msg->io_Req.io_ReplyPort, msg);
	}
}

/*****************************************************************************/
/*
**	abortio(msg->io_Device, msg)
*/

static TMODAPI TINT
iodef_abortio(IODEF_MOD *mod, struct TIOPacket *msg)
{
	/* not supported */
	return -1;
}
