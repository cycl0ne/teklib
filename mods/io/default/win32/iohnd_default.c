
/*
**	$Id: iohnd_default.c,v 1.6 2005/11/07 22:24:22 tmueller Exp $
**	Win32 implementation of the 'default' I/O handler.
**	it is responsible for abstraction from the host file system.
*/

#include <windows.h>

#include <tek/exec.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>
#include <tek/proto/hal.h>
#include <tek/inline/io.h>
#include <tek/mod/io.h>
#include <tek/mod/ioext.h>
#include <tek/mod/time.h>

/*****************************************************************************/

#define MOD_VERSION		0
#define MOD_REVISION	2

typedef struct
{
	TMODL module;

	TAPTR util;
	TAPTR time;
	struct TTimeRequest *treq;

	TAPTR lock;
	TUINT refcount;
	TAPTR task;
	TAPTR mmu;

} TMOD_HND;

#define TExecBase		TGetExecBase(mod)
#define TUtilBase		mod->util
#define TTimeBase		mod->time

typedef struct
{
	TMOD_HND *mod;
	TUINT flags;
	TSTRPTR winname;
	TUINT lockmode;
	HANDLE filehandle;
	HANDLE dirfindhandle;			/* find handle for directories */
	WIN32_FIND_DATA dirfinddata;	/* find data for directories */

} FLOCK;

typedef struct
{
	TMOD_HND *mod;
	
	FLOCK *flock;					/* back-ptr */
	HANDLE filefindhandle;			/* find handle for files */
	WIN32_FIND_DATA filefinddata;	/* find data for files */
	TLIST devlist;					/* list of device names (A, C, D...) */
	TLIST devlist_done;				/* list of processed device names */

	TSTRPTR tempdrivename;		
	TUINT size;
	TUINT sizehi;
	TUINT type;
	struct TDateBox datebox;
	TSTRPTR name;
	TINT numattr;
	TBOOL havesizehi;

} EXAMINE;

#define FL_NONE		0x0000
#define FL_LOCK		0x0001
#define FL_FILE		0x0002
#define FL_ISDIR	0x0004
#define FL_ISROOT	0x0008

typedef struct TIOPacket TIOMSG;

#define	IOBUFSIZE_DEFAULT			4096

/*****************************************************************************/

static TCALLBACK TVOID mod_destroy(TMOD_HND *mod);
static TCALLBACK TIOMSG *mod_open(TMOD_HND *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_HND *mod, TAPTR task);
static TTASKENTRY TVOID mod_task(TAPTR task);
static TVOID hnd_init(TMOD_HND *mod);

static TMODAPI TVOID beginio(TMOD_HND *mod, TIOMSG *msg);
static TMODAPI TINT abortio(TMOD_HND *mod, TIOMSG *msg);

/*****************************************************************************/
/*
**	module init/exit
*/

TMODENTRY TUINT
tek_init_iohnd_default(TAPTR task, TMOD_HND *mod, TUINT16 version,
	TTAGITEM *tags)
{
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * 2;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_HND);	/* positive size */

		return 0;
	}

	mod->lock = TCreateLock(TNULL);
	if (mod->lock)
	{		
		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;
		mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;
		mod->module.tmd_DestroyFunc = (TDFUNC) mod_destroy;

		#ifdef TDEBUG
			mod->mmu = TCreateMMU(TNULL, TMMUT_Tracking | TMMUT_TaskSafe, TNULL);
		#endif

		((TAPTR *) mod)[-1] = (TAPTR) beginio;
		((TAPTR *) mod)[-2] = (TAPTR) abortio;

		return TTRUE;
	}

	return 0;
}

static TCALLBACK TVOID
mod_destroy(TMOD_HND *mod)
{
	TDestroy(mod->mmu);
	TDestroy(mod->lock);
}

/*****************************************************************************/
/*
**	module open/close
*/

static TCALLBACK TIOMSG *
mod_open(TMOD_HND *mod, TAPTR task, TTAGITEM *tags)
{
	TIOMSG *msg;

	msg = TAllocMsg0(sizeof(TIOMSG));
	if (msg)
	{
		TLock(mod->lock);
		
		if (!mod->task)
		{			
			mod->util = TOpenModule("util", 0, TNULL);
			mod->time = TOpenModule("time", 0, TNULL);
			mod->treq = TOpenModule("timer.device", 0, TNULL);
			
			if (mod->util && mod->time && mod->treq)
			{
				TTAGITEM tasktags[2];
				tasktags[0].tti_Tag = TTask_UserData;
				tasktags[0].tti_Value = (TTAG) mod;
				tasktags[1].tti_Tag = TTAG_DONE;
	
				/* create handler task */

				mod->task = TCreateTask((TTASKFUNC) mod_task, TNULL, tasktags);
				if (mod->task)
				{
					hnd_init(mod);
				}
			}
			else
			{
				if (mod->treq)
				{
					TCloseModule(mod->treq->ttr_Req.io_Device);
					TFree(mod->treq);
				}
				TCloseModule(mod->time);
				TCloseModule(mod->util);
			}
		}
		
		if (mod->task)
		{
			mod->refcount++;

			/* insert recommended buffer settings */
			msg->io_BufSize = IOBUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_FULL;

			msg->io_Req.io_Device = (TMODL *) mod;
		}
		else
		{
			TFree(msg);
			msg = TNULL;
		}

		TUnlock(mod->lock);
	}

	return msg;
}

static TCALLBACK TVOID
mod_close(TMOD_HND *mod, TAPTR task)
{
	TLock(mod->lock);
	if (mod->task)
	{
		if (--mod->refcount == 0)
		{
			TSignal(mod->task, TTASK_SIG_ABORT);
			TDestroy(mod->task);
			mod->task = TNULL;

			TCloseModule(mod->treq->ttr_Req.io_Device);
			TFree(mod->treq);
			TCloseModule(mod->time);
			TCloseModule(mod->util);
		}
	}
	TUnlock(mod->lock);
}

/*****************************************************************************/
/*
**	free list
*/

static TVOID 
freelist(TMOD_HND *mod, TLIST *list)
{
	TNODE *n;
	while ((n = TRemHead(list)))
	{
		TFree(n);					
	}
}

/*****************************************************************************/
/*
**	add available devices to list
*/

static TBOOL
getdevlist(TMOD_HND *mod, TLIST *list)
{
	TSTRPTR buf;
	DWORD blen;
	TBOOL success = TFALSE;

	blen = GetLogicalDriveStrings(0, NULL);
	if (blen > 0)
	{
		TSTRPTR devlist = buf = TAlloc(mod->mmu, blen);
		if (buf)
		{
			if (GetLogicalDriveStrings(blen, buf) == blen - 1)
			{
				success = TTRUE;
				while (*buf)
				{
					TSTRPTR p = buf;
					THNDL *n;
					TINT l;
					l = TStrLen(p);
					buf += l + 1;
					if (l > 2 && !TStrCmp(p + l - 2, ":\\")) l -= 2;
					n = TAlloc(mod->mmu, sizeof(THNDL) + l + 1);
					if (n)
					{
						TCopyMem(p, n + 1, l);
						((TSTRPTR)(n+1))[l] = 0;
						n->thn_Data = n + 1;
						TAddTail(list, (TNODE *) n);
						continue;
					}
					
					freelist(mod, list);
					success = TFALSE;
					break;
				}
			}
			TFree(devlist);
		}
	}
	return success;
}

/*****************************************************************************/
/*
**	errordialog
*/

static TVOID 
errordialog(DWORD err)
{
	LPVOID buf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &buf, 0, NULL);
	tdbprintf1(20,"error in messagebox: %d\n", (TUINT) err);
	MessageBox(NULL, buf, "iohnd_default Debug", MB_OK | MB_ICONINFORMATION);
	LocalFree(buf);
}

/*****************************************************************************/
/*
**	ioerr = geterror()
**	translate host error to TEKlib ioerr
*/

static TINT
geterror(void)
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
			#ifdef TDEBUG
				errordialog(err);
			#endif
			tdbfatal(99);
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
getwinname(TMOD_HND *mod, TSTRPTR name, TSTRPTR winextra)
{
	TSTRPTR winname = TNULL;
	TINT len;

	len = TStrLen(name);
	if (len)
	{
		TINT elen = TStrLen(winextra);
		winname = TAlloc(mod->mmu, len + elen + 3);
		if (winname)
		{
			TSTRPTR p, d;
			TINT8 c;
	
			p = TStrChr(name, '/');
			if (p)
			{
				len = p - name;
				p++;	
			}
			else
			{
				p = name + len;
			}
	
			TStrNCpy(winname, name, len);

			d = winname;
			d[len++] = ':';
			d[len] = '\\';
			
			do
			{
				c = *p++;
				if (c == '/') c = '\\';
				d[++len] = c;
		
			} while (c);
			
			/* erase trailing winslash */
			if (d[len - 1] == '\\')
			{
				d[--len] = 0;
			}
			
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
getcurrentdir(TMOD_HND *mod)
{
	TSTRPTR name = TNULL;
	TINT len;
	
	len = GetCurrentDirectory(0, NULL);
	if (len)
	{
		name = TAlloc(mod->mmu, len);
		if (name)
		{
			GetCurrentDirectory(len, name);
		}
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
gettekname(TMOD_HND *mod, TAPTR mmu, TSTRPTR winname, TINT addspace)
{
	TSTRPTR tekname = TNULL;
	TSTRPTR p;
	
	p = TStrChr(winname, ':');
	if (p)
	{
		TINT len = TStrLen(winname);
		tekname = TAlloc(mmu, len + addspace);
		if (tekname)
		{
			TINT8 c;
			TSTRPTR d = tekname + addspace;
			
			len = p - winname;
			TStrNCpy(d, winname, len);

			d += len;
			p++;

			do
			{
				c = *p++;
				if (c == '\\') c = '/';
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
addassign(TMOD_HND *mod, TAPTR TIOBase, TSTRPTR assign, TSTRPTR winpath)
{
	TBOOL success = TFALSE;
	
	if (assign && winpath)
	{
		TSTRPTR t = gettekname(mod, mod->mmu, winpath, 1);
		if (t)
		{
			t[0] = ':';
			TAssignLate(assign, t);
			TFree(t);
			success = TTRUE;
		}
	}

	return success;
}

/*****************************************************************************/
/*
**	hnd_init
**	add late-binding assigns, such as CURRENTDIR:, PROGDIR: and SYS:.
**	note: calls to iobase are allowed here, because in the instance
**	open function we're running in the same task context as the module
**	opener, who already holds locks to the base.
*/

static TVOID
hnd_init(TMOD_HND *mod)
{
	TAPTR TIOBase = TOpenModule("io", 0, TNULL);
	if (TIOBase)
	{	
		TAPTR hal = TGetHALBase();
		TSTRPTR name;
		
		/* add currentdir */
		name = getcurrentdir(mod);
		if (name)
		{
			addassign(mod, TIOBase, "CURRENTDIR", name);
			TFree(name);
		}

		/* add PROGDIR: */
		name = (TSTRPTR) THALGetAttr(hal, TExecBase_ProgDir, TNULL);
		addassign(mod, TIOBase, "PROGDIR", name);

		/* add SYS: */
		name = (TSTRPTR) THALGetAttr(hal, TExecBase_SysDir, TNULL);
		addassign(mod, TIOBase, "SYS", name);

		/* add ENVARC: */
		TAssignLate("ENVARC", "SYS:prefs/env-archive");
		
		TCloseModule(TIOBase);
	}
}

/*****************************************************************************/
/*
**	hnd_open
*/

static TVOID
hnd_open(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock;
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
	flock = TAlloc(mod->mmu, sizeof(FLOCK));
	if (flock)
	{
		flock->winname = getwinname(mod, msg->io_Op.Open.Name, TNULL);
		if (flock->winname)
		{
			flock->filehandle = CreateFile(flock->winname, accessmode, sharemode, NULL, 
				createmode, FILE_ATTRIBUTE_NORMAL, NULL);

			if (flock->filehandle != INVALID_HANDLE_VALUE)
			{
				flock->dirfindhandle = INVALID_HANDLE_VALUE;
				flock->flags = FL_FILE;
				flock->lockmode = lockmode;
				flock->mod = mod;
				msg->io_FLock = flock;
				msg->io_Op.Open.Result = TTRUE;
				msg->io_Req.io_Error = 0;
				return;
			}
			
			msg->io_Req.io_Error = geterror();
			TFree(flock->winname);
		}
		
		TFree(flock);
	}
}

/*****************************************************************************/
/*
**	hnd_lock
*/

static TVOID
hnd_lock(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock;
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
			accessmode = GENERIC_READ | GENERIC_WRITE;
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
	flock = TAlloc(mod->mmu, sizeof(FLOCK));
	if (flock)
	{
		flock->filehandle = INVALID_HANDLE_VALUE;
		flock->dirfindhandle = INVALID_HANDLE_VALUE;
	
		if (msg->io_Op.Lock.Name[0] == 0)
		{
			/* empty string; lock on the filesystem root */
			
			flock->flags = FL_LOCK | FL_ISDIR | FL_ISROOT;
			flock->lockmode = lockmode;
			flock->mod = mod;
			msg->io_FLock = flock;
			msg->io_Op.Lock.Result = TTRUE;
			msg->io_Req.io_Error = 0;
			return;
		}
		
		flock->winname = getwinname(mod, msg->io_Op.Lock.Name, TNULL);
		
		if (flock->winname)
		{
			/* first try to lock it as a directory */
			
			TSTRPTR winfindname = getwinname(mod, msg->io_Op.Lock.Name, "\\*");
			if (winfindname)
			{
				flock->dirfindhandle = FindFirstFile(winfindname, &flock->dirfinddata);
				TFree(winfindname);	
				if (flock->dirfindhandle != INVALID_HANDLE_VALUE)
				{
					flock->flags = FL_LOCK | FL_ISDIR;
					flock->lockmode = lockmode;
					flock->mod = mod;
					msg->io_FLock = flock;
					msg->io_Op.Lock.Result = TRUE;
					msg->io_Req.io_Error = 0;
					return;
				}
			}

			/* try to open a file instead */
			
			flock->filehandle = CreateFile(flock->winname, accessmode, sharemode, 
				NULL, createmode, FILE_ATTRIBUTE_NORMAL, NULL);
				
			if (flock->filehandle != INVALID_HANDLE_VALUE)
			{
				flock->flags = FL_LOCK;
				flock->lockmode = lockmode;
				flock->mod = mod;
				msg->io_FLock = flock;
				msg->io_Op.Lock.Result = TTRUE;
				msg->io_Req.io_Error = 0;
				return;
			}
			
			msg->io_Req.io_Error = geterror();
			TFree(flock->winname);
		}
		
		TFree(flock);
	}
}

/*****************************************************************************/
/*
**	hnd_close
**	- applies to both locks and files.
*/

static TVOID
hnd_close(TMOD_HND *mod, TIOMSG *msg)
{
	EXAMINE *e = msg->io_Examine;
	FLOCK *f = msg->io_FLock;
	
	if (e)
	{
		freelist(mod, &e->devlist);
		freelist(mod, &e->devlist_done);
		if (e->filefindhandle) FindClose(e->filefindhandle);
		TFree(e->tempdrivename);
		TFree(e);
	}

	if (f->filehandle != INVALID_HANDLE_VALUE)
	{
		if (!CloseHandle(f->filehandle))
		{
			msg->io_Req.io_Error = geterror();
		}
	}
	
	if (f->dirfindhandle != INVALID_HANDLE_VALUE) FindClose(f->dirfindhandle);

	TFree(f->winname);
	TFree(f);
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
	
	if (fl->flags & FL_FILE)
	{
		DWORD wrlen = 0;
		if (WriteFile(fl->filehandle, msg->io_Op.Write.Buf, msg->io_Op.Write.Len, &wrlen, NULL))
		{
			res = (TINT) wrlen;
		}
		else
		{
			msg->io_Req.io_Error = geterror();
		}
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
	}

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
	TINT res = -1;
	
	if (fl->flags & FL_FILE)
	{
		DWORD rdlen = 0;
		if (ReadFile(fl->filehandle, msg->io_Op.Read.Buf, msg->io_Op.Read.Len, &rdlen, NULL))
		{
			res = (TINT) rdlen;
		}
		else
		{
			msg->io_Req.io_Error = geterror();
		}
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
	}

	msg->io_Op.Read.RdLen = res;
}

/*****************************************************************************/
/*
**	examinefinddata(mod, examine, winfinddata)
*/

static TVOID
examinefinddata(TMOD_HND *mod, EXAMINE *e, LPWIN32_FIND_DATA fp)
{
	SYSTEMTIME st;
	
	e->size = fp->nFileSizeLow;
	e->sizehi = fp->nFileSizeHigh;
	e->name = fp->cFileName;
	
	if (fp->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		e->type = TFTYPE_Directory;
	}
	else
	{
		e->type = TFTYPE_File;
	}
	
	FileTimeToSystemTime(&fp->ftLastWriteTime, &st);
	e->datebox.tdb_Year = st.wYear;
	e->datebox.tdb_Month = (TUINT8) st.wMonth;
	e->datebox.tdb_Day = (TUINT8) st.wDay;
	e->datebox.tdb_Hour = (TUINT8) st.wHour;
	e->datebox.tdb_Minute = (TUINT8) st.wMinute;
	e->datebox.tdb_USec = (st.wSecond * 1000 + st.wMilliseconds) * 1000;
	e->datebox.tdb_Fields = TDB_YEAR|TDB_MONTH|TDB_DAY|TDB_HOUR|TDB_MINUTE|TDB_USEC;
}

/*****************************************************************************/
/*
**	examinetags(examine, tagitem)
**	fill examination info into user-supplied tags. this
**	is called back by TForEachTag()
*/

static TCALLBACK TBOOL
examinetags(EXAMINE *e, TTAGITEM *ti)
{
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
			TMOD_HND *mod = e->mod;
			TPackDate(&e->datebox, (TDATE *) ti->tti_Value);
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
**	hnd_examine
**	examine object. if successful, place a handler-specific examination
**	handle into the iomsg - this is how the IO module knows whether an
**	object has been examined already. note that both files and locks must
**	be supported.
*/

static TVOID
hnd_examine(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;
	EXAMINE *e = msg->io_Examine;
	msg->io_Op.Examine.Result = -1;

	if (!e)
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;

		e = TAlloc0(mod->mmu, sizeof(EXAMINE));
		if (e)
		{
			TBOOL success = TFALSE;

			e->mod = mod;
			TInitList(&e->devlist);
			TInitList(&e->devlist_done);
			
			if (fl->flags & FL_ISROOT)
			{
				if (getdevlist(mod, &e->devlist))
				{
					TDATE date;
					e->name = "";
					e->size = 0;
					e->type = TFTYPE_Directory;
					TLock(mod->lock);
					TGetDate(mod->treq, &date, TNULL);
					TUnlock(mod->lock);
					TUnpackDate(&date, &e->datebox, TDB_ALL);
					success = TTRUE;
				}
			}
			else
			{
				TBOOL isdrivename = TFALSE;

				if (fl->flags & FL_ISDIR)
				{
					TINT l = TStrLen(fl->winname);
					if (l > 1 && fl->winname[l - 1] == ':')
					{
						isdrivename = TTRUE;
						/* we need a copy without the colon */
						e->tempdrivename = TStrNDup(mod->mmu, fl->winname, l - 1);
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
						TLock(mod->lock);
						TGetDate(mod->treq, &date, TNULL);
						TUnlock(mod->lock);
						TUnpackDate(&date, &e->datebox, TDB_ALL);
						success = TTRUE;
					}
					else
					{
						msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
					}
				}
				else
				{
					/* regular files and directories need a findhandle */
	
					e->filefindhandle = FindFirstFile(fl->winname, &e->filefinddata);
					if (e->filefindhandle != INVALID_HANDLE_VALUE)
					{
						examinefinddata(mod, e, &e->filefinddata);
						success = TTRUE;
					}
					else
					{
						msg->io_Req.io_Error = geterror();
					}
				}
			}

			if (success)
			{
				e->flock = fl;		
				msg->io_Examine = e;
			}
			else
			{
				TFree(e);
				return;
			}
		}
	}

	if (e)
	{
		e->havesizehi = 
			TGetTag(msg->io_Op.Examine.Tags, TFATTR_SizeHigh, TNULL);
		e->numattr = 0;
		TForEachTag(msg->io_Op.Examine.Tags, (TTAGFOREACHFUNC) examinetags, e);
		msg->io_Op.Examine.Result = e->numattr;
		msg->io_Req.io_Error = 0;
	}
}

/*****************************************************************************/
/*
**	hnd_exnext
**	examine next entry. only directory locks are supported.
**	note: the IO module guarantees that the directory lock
**	has been examined once before an exnext is issued.
*/

static TVOID
hnd_exnext(TMOD_HND *mod, TIOMSG *msg)
{
	EXAMINE *e = msg->io_Examine;
	FLOCK *fl = e->flock;
	msg->io_Op.Examine.Result = -1;
	
	if (!(fl->flags & FL_ISDIR))
	{
		/* not a directory lock */
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
		return;
	}
	
	if (fl->flags & FL_ISROOT)
	{
		/* link next node from devlist to devlist_done, examine it */
		THNDL *h = (THNDL *) TRemHead(&e->devlist);
		if (h)
		{
			e->name = h->thn_Data;
			e->size = 0;
			e->type = TFTYPE_Directory | TFTYPE_Volume;
			TAddTail(&e->devlist_done, (TNODE *) h);
			e->numattr = 0;
			e->havesizehi = 
				TGetTag(msg->io_Op.Examine.Tags, TFATTR_SizeHigh, TNULL);
			TForEachTag(msg->io_Op.Examine.Tags, (TTAGFOREACHFUNC) examinetags, e);
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
			if (!TStrCmp(fl->dirfinddata.cFileName, "..")) continue;

			examinefinddata(mod, e, &fl->dirfinddata);
			e->numattr = 0;
			e->havesizehi = 
				TGetTag(msg->io_Op.Examine.Tags, TFATTR_SizeHigh, TNULL);
			TForEachTag(msg->io_Op.Examine.Tags, (TTAGFOREACHFUNC) examinetags, e);
			msg->io_Op.Examine.Result = e->numattr;
			return;
		}
	}

	msg->io_Req.io_Error = TIOERR_NO_MORE_ENTRIES;
}

/*****************************************************************************/
/*
**	hnd_openfromlock
**	turn a lock into an open filehandle
*/

static TVOID
hnd_openfromlock(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;

	if (fl->flags & (FL_ISDIR | FL_FILE))
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
		msg->io_Op.OpenFromLock.Result = TFALSE;
	}
	else
	{
		fl->flags = FL_FILE;
		msg->io_Op.OpenFromLock.Result = TTRUE;
	}
}

/*****************************************************************************/
/*
**	hnd_seek
**	seek in a file
*/

static TVOID
hnd_seek(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;
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

	if (!(fl->flags & FL_FILE))
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
		return;
	}

	if (msg->io_Op.Seek.OffsHi)
	{
		res = SetFilePointer(fl->filehandle, msg->io_Op.Seek.Offs, (PLONG) msg->io_Op.Seek.OffsHi, method);
		if (res == 0xffffffff)
		{
			TINT err = geterror();
			if (err)
			{
				msg->io_Req.io_Error = err;
				return;
			}
		}
	}
	else
	{
		res = SetFilePointer(fl->filehandle, msg->io_Op.Seek.Offs, NULL, method);
		if (res == 0xffffffff)
		{
			msg->io_Req.io_Error = geterror();
			return;
		}
	}

	msg->io_Op.Seek.Result = res;
}

/*****************************************************************************/
/*
**	hnd_rename
*/

static TVOID
hnd_rename(TMOD_HND *mod, TIOMSG *msg)
{
	TSTRPTR srcname, dstname;

	msg->io_Op.Rename.Result = TFALSE;

	srcname = getwinname(mod, msg->io_Op.Rename.SrcName, TNULL);
	dstname = getwinname(mod, msg->io_Op.Rename.DstName, TNULL);

	if (srcname && dstname)
	{
		if (MoveFile(srcname, dstname))
		{
			msg->io_Op.Rename.Result = TTRUE;
		}
		else
		{
			msg->io_Req.io_Error = geterror();
		}
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	}

	TFree(dstname);
	TFree(srcname);
}

/*****************************************************************************/
/*
**	hnd_makedir
*/

static TVOID
hnd_makedir(TMOD_HND *mod, TIOMSG *msg)
{
	TSTRPTR name;
	
	msg->io_Op.Lock.Result = TFALSE;
	name = getwinname(mod, msg->io_Op.Lock.Name, TNULL);
	if (name)
	{
		if (CreateDirectory(name, NULL))
		{
			hnd_lock(mod, msg);
		}
		else
		{
			msg->io_Req.io_Error = geterror();
		}
		TFree(name);
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	}
}

/*****************************************************************************/
/*
**	hnd_deleteobject
*/

static TVOID
hnd_delete(TMOD_HND *mod, TIOMSG *msg)
{
	TSTRPTR name;
	
	msg->io_Op.Delete.Result = TFALSE;
	name = getwinname(mod, msg->io_Op.Delete.Name, TNULL);
	if (name)
	{
		if (DeleteFile(name))
		{
			msg->io_Op.Delete.Result = TTRUE;
		}
		else
		{
			msg->io_Req.io_Error = geterror();
			if (msg->io_Req.io_Error == TIOERR_ACCESS_DENIED)
			{
				if (RemoveDirectory(name))
				{
					msg->io_Op.Delete.Result = TTRUE;
					msg->io_Req.io_Error = 0;
				}
				else
				{
					msg->io_Req.io_Error = geterror();
				}
			}
		}
		TFree(name);
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	}
}

/*****************************************************************************/
/*
**	hnd_makename
*/

static TVOID
hnd_makename(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock = msg->io_FLock;
	TSTRPTR name = msg->io_Op.MakeName.Name;
	TSTRPTR dest = msg->io_Op.MakeName.Dest;

	TSTRPTR src, temp;
	TINT len, len2;
	TINT res = -1;
	
	msg->io_Op.MakeName.Result = -1;
	msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;

	switch (msg->io_Op.MakeName.Mode)
	{
		default:
			return;
		
		case TPPF_HOST2TEK:
			break;
	}

	len2 = TStrLen(name);
	
	if (TStrChr(name, ':'))
	{
		src = TNULL;		/* absolute */
		len = 0;
	}
	else
	{
		src = flock->winname;
		len = TStrLen(src) + 1;
	}
	
	temp = TAlloc(mod->mmu, len + len2 + 1);
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
			TSTRPTR winabsname = TAlloc(mod->mmu, len + 1);
			if (winabsname)
			{
				if (GetFullPathName(temp, len + 1, winabsname, TNULL))
				{
					tekname = gettekname(mod, mod->mmu, winabsname, 1);
				}
				
				if (tekname)
				{
					tekname[0] = ':';
				}
				else
				{
					TFree(tekname);
					tekname = TNULL;
				}
				
				TFree(winabsname);
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

			TFree(tekname);
		}
		else
		{
			res = -1;
			msg->io_Req.io_Error = geterror();
		}
			
		TFree(temp);
	}
	
	msg->io_Op.MakeName.Result = res;
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
	
		case TIOCMD_LOCK:
			hnd_lock(mod, msg);
			break;

		case TIOCMD_UNLOCK:
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
	
		case TIOCMD_EXNEXT:
			hnd_exnext(mod, msg);
			break;
	
		case TIOCMD_OPENFROMLOCK:
			hnd_openfromlock(mod, msg);
			break;

		case TIOCMD_RENAME:
			hnd_rename(mod, msg);
			break;

		case TIOCMD_MAKEDIR:
			hnd_makedir(mod, msg);
			break;

		case TIOCMD_DELETE:
			hnd_delete(mod, msg);
			break;

		case TIOCMD_MAKENAME:
			hnd_makename(mod, msg);
			break;
	}
}

/*****************************************************************************/
/*
**	handler task
*/

static TTASKENTRY TVOID
mod_task(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TMOD_HND *mod = TExecGetTaskData(exec, task);
	TAPTR port = TGetUserPort(task);
	TUINT portsig = TGetPortSignal(port);
	TIOMSG *msg;
	TUINT sigs;

	do
	{
		sigs = TWait(portsig | TTASK_SIG_ABORT);
		if (sigs & portsig)
		{
			while ((msg = TGetMsg(port)))
			{
				hnd_docmd(mod, msg);				
				TReplyMsg(msg);
			}
		}

	} while (!(sigs & TTASK_SIG_ABORT));
}

/*****************************************************************************/
/*
**	beginio(msg->io_Device, msg)
*/

static TMODAPI TVOID
beginio(TMOD_HND *mod, TIOMSG *msg)
{
	if (msg->io_Req.io_Flags & TIOF_QUICK)
	{
		/* do synchronously */
		msg->io_Req.io_Flags &= ~TIOF_QUICK;
		hnd_docmd(mod, msg);
	}
	else
	{
		/* do asynchronously */
		TPutMsg(TGetUserPort(mod->task), 
			msg->io_Req.io_ReplyPort, msg);
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
