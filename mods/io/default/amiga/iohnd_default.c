
/*
**	$Id: iohnd_default.c,v 1.6 2005/09/07 23:59:59 tmueller Exp $
**	Amiga implementation of the 'default' I/O handler.
**	it is responsible for abstraction from the host file system.
*/

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

#include <exec/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/dos.h>
#include <proto/timer.h>

/*****************************************************************************/

#define MOD_VERSION		0
#define MOD_REVISION	2

typedef struct
{
	TMODL module;

	struct DosLibrary *dosbase;

	TAPTR util;
	TAPTR time;

	struct TTimeRequest *treq;

	TAPTR lock;
	TUINT refcount;
	TAPTR task;
	TAPTR mmu;

} TMOD_HND;

#define SysBase *((struct ExecBase **) 4L)
#define DOSBase mod->dosbase

#define TExecBase		TGetExecBase(mod)
#define TUtilBase		mod->util
#define TTimeBase		mod->time

typedef struct
{
	TMOD_HND *mod;
	TUINT flags;
	TSTRPTR aminame;
	TUINT lockmode;
	BPTR desc;

} FLOCK;

typedef struct
{
	struct FileInfoBlock *fib;
	TMOD_HND *mod;

	TLIST devlist;
	TLIST devlist_done;

	TDATE date;
	TSTRPTR name;
	TUINT size;
	TUINT type;
	TINT numattr;

} EXAMINE;

#define FL_NONE		0x0000
#define FL_LOCK		0x0001
#define FL_FILE		0x0002
#define FL_ISDIR	0x0004
#define FL_ISROOT	0x0008

typedef struct TIOPacket TIOMSG;

#define	IOBUFSIZE_DEFAULT	1024

/*****************************************************************************/

static TCALLBACK TVOID mod_destroy(TMOD_HND *mod);
static TCALLBACK TIOMSG *mod_open(TMOD_HND *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_HND *mod, TAPTR task);
static TTASKENTRY TVOID mod_task(TAPTR task);
static TBOOL hnd_init(TMOD_HND *mod);

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

static TVOID
closeall(TMOD_HND *mod)
{
	if (mod->task)
	{
		TSignal(mod->task, TTASK_SIG_ABORT);
		TDestroy(mod->task);
		mod->task = TNULL;
	}
	CloseLibrary((struct Library *) mod->dosbase);
	if (mod->treq)
	{
		TCloseModule(mod->treq->ttr_Req.io_Device);
		TFree(mod->treq);
	}
	TCloseModule(mod->time);
	TCloseModule(mod->util);
}

static TCALLBACK TIOMSG *
mod_open(TMOD_HND *mod, TAPTR task, TTAGITEM *tags)
{
	TIOMSG *msg;

	msg = TAllocMsg0(sizeof(TIOMSG));
	if (msg)
	{
		TBOOL success = TFALSE;

		TLock(mod->lock);
		
		if (!mod->task)
		{			
			mod->util = TOpenModule("util", 0, TNULL);
			mod->time = TOpenModule("time", 0, TNULL);
			mod->treq = TOpenModule("timer.device", 0, TNULL);
			mod->dosbase = (struct DosLibrary *) OpenLibrary("dos.library", 0);
			
			if (mod->util && mod->time && mod->treq && mod->dosbase)
			{
				TTAGITEM tasktags[2];
				tasktags[0].tti_Tag = TTask_UserData;
				tasktags[0].tti_Value = (TTAG) mod;
				tasktags[1].tti_Tag = TTAG_DONE;
	
				/* create handler task */

				mod->task = TCreateTask((TTASKFUNC) mod_task, TNULL, tasktags);
				if (mod->task)
				{
					success = hnd_init(mod);
				}
			}
			
			if (!success)
			{
				closeall(mod);
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
			closeall(mod);
		}
	}
	TUnlock(mod->lock);
}

/*****************************************************************************/
/*
**	free list
*/

static TVOID freelist(TMOD_HND *mod, TLIST *list)
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
	TBOOL success = TTRUE;
	struct DosList *dl;

	dl = LockDosList(LDF_VOLUMES | LDF_ASSIGNS | LDF_READ);

	while ((dl = NextDosEntry(dl, LDF_VOLUMES | LDF_ASSIGNS)))
	{
		TSTRPTR name = (TSTRPTR) BADDR(dl->dol_Name) + 1;
		TINT l = TStrLen(name);
		THNDL *n = TAlloc(mod->mmu, sizeof(THNDL) + l + 1);
		if (n)
		{
			TStrCpy((TSTRPTR) (n + 1), name);
			n->thn_Data = n + 1;
			TAddTail(list, (TNODE *) n);
			continue;
		}

		freelist(mod, list);
		success = TFALSE;
		break;
	}

	UnLockDosList(LDF_VOLUMES | LDF_ASSIGNS | LDF_READ);
	
	return success;
}

/*****************************************************************************/
/*
**	ioerr = geterror(mod)
**	translate host ioerr to TEKlib ioerr
*/

static TINT
geterror(TMOD_HND *mod)
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
**	aminame = getaminame(tekname)
**		c				->	c:
**		foo/bar/bla/	->	foo:bar/bla
*/

static TSTRPTR
getaminame(TMOD_HND *mod, TSTRPTR name)
{
	TSTRPTR aminame = TNULL;
	TINT len;

	len = TStrLen(name);
	if (len)
	{
		aminame = TAlloc(mod->mmu, len + 2);
		if (aminame)
		{
			TSTRPTR p, d;
			TINT8 c;
			
			p = TStrChr(name, '/');
			if (p)
			{
				len = p - name;			/* foo/bar -> 3 */
				p++;					/*     ^   		*/
			}
			else
			{
				p = name + len;			/* foo  */
			}							/*    ^ */
		
			TStrNCpy(aminame, name, len);
			
			d = aminame;
			d[len] = ':';
			
			do
			{
				c = *p++;
				d[++len] = c;
			} while (c);
			
			/* erase trailing slash, it's not needed */
			
			if (d[len - 1] == '/')
			{
				d[--len] = 0;
			}
			
			tdbprintf2(5,"name created: %s -> %s\n", name, aminame);
		}
	}
	
	return aminame;
}

/*****************************************************************************/
/*
**	tekname = gettekname(mod, mmu, aminame, extra_space_in_front)
**		foo:bar/bla/fasel
**	->	foo/bar/bla/fasel
**		c:
**	->	c
*/

static TSTRPTR 
gettekname(TMOD_HND *mod, TAPTR mmu, TSTRPTR aminame, TINT addspace)
{
	TSTRPTR tekname = TNULL;
	TSTRPTR p;
	
	p = TStrChr(aminame, ':');
	if (p)
	{
		TINT len = TStrLen(aminame);
		tekname = TAlloc(mmu, len + 1 + addspace);
		if (tekname)
		{
			TINT8 c;
			TSTRPTR d = tekname + addspace;
			
			len = p - aminame;
			TStrNCpy(d, aminame, len);

			d += len;
			p++;
			
			if (*p)
			{
				*d++ = '/';
			}

			do
			{
				c = *p++;
				*d++ = c;
	
			} while (c);
		}
	}

	return tekname;
}

/*****************************************************************************/
/*
**	getcurrentdir
*/

static TSTRPTR getcurrentdir(TMOD_HND *mod)
{
	TSTRPTR name;
	TINT len = 16;

	while ((name = TAlloc(mod->mmu, len)))
	{
		GetCurrentDirName(name, len);
		/* The autodocs for GetCurrentDir() are misleading at best! */
		if (IoErr() == ERROR_LINE_TOO_LONG)
		{
			TFree(name);
			len <<= 1;
			continue;
		}
		break;
	}

	return name;
}

/*****************************************************************************/
/*
**	success = addassign(mod, assign, amipath)
*/

static TBOOL
addassign(TMOD_HND *mod, TAPTR TIOBase, TSTRPTR assign, TSTRPTR amipath)
{
	TBOOL success = TFALSE;
	
	if (assign && amipath)
	{
		/* resolve path first */
		BPTR lock;
		struct Process *pr = (struct Process *) FindTask(NULL);
		APTR oldwptr = pr->pr_WindowPtr;
		pr->pr_WindowPtr = (APTR) 0xffffffff;
		lock = Lock(amipath, ACCESS_READ);
		pr->pr_WindowPtr = oldwptr;
		if (lock)
		{
			char buf[1024];
			if (NameFromLock(lock, buf, sizeof(buf)))
			{
				TSTRPTR t = gettekname(mod, mod->mmu, buf, 1);
				if (t)
				{
					t[0] = ':';
					tdbprintf2(2,"addassign: %s -> %s\n", buf, t);
					TAssignLate(assign, t);
					TFree(t);
					success = TTRUE;
				}
			}
			UnLock(lock);
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

static TBOOL hnd_init(TMOD_HND *mod)
{
	TBOOL success = TFALSE;
	TAPTR TIOBase = TOpenModule("io", 0, TNULL);
	
	if (TIOBase)
	{	
		TSTRPTR name = getcurrentdir(mod);
		if (name)
		{
			/* add currentdir */
			success = addassign(mod, TIOBase, "CURRENTDIR", name);
			TFree(name);
			
			if (success)
			{
				TAPTR hal = TGetHALBase();

				/* add PROGDIR */
				name = (TSTRPTR) THALGetAttr(hal, TExecBase_ProgDir, TNULL);
				addassign(mod, TIOBase, "PROGDIR", name);

				/* add SYS */				
				name = (TSTRPTR) THALGetAttr(hal, TExecBase_SysDir, TNULL);
				addassign(mod, TIOBase, "SYS", name);

				/* add ENVARC */
				TAssignLate("ENVARC", "SYS:prefs/env-archive");
			}
		}
		TCloseModule(TIOBase);
	}
	return success;
}

/*****************************************************************************/
/*
**	hnd_open
*/

static TVOID hnd_open(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock;
	LONG hostmode;
	TUINT lockmode;
	
	msg->io_Op.Open.Result = TFALSE;

	switch (msg->io_Op.Open.Mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;
			
		case TFMODE_READONLY:
			hostmode = MODE_OLDFILE;
			lockmode = TFLOCK_NONE;
			break;

		case TFMODE_OLDFILE:
			hostmode = MODE_OLDFILE;
			lockmode = TFLOCK_NONE;
			break;
			
		case TFMODE_NEWFILE:
			hostmode = MODE_NEWFILE;
			lockmode = TFLOCK_WRITE;
			break;

		case TFMODE_READWRITE:
			hostmode = MODE_READWRITE;
			lockmode = TFLOCK_READ;
	}
	
	msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	flock = TAlloc(mod->mmu, sizeof(FLOCK));
	if (flock)
	{
		flock->aminame = getaminame(mod, msg->io_Op.Open.Name);
		if (flock->aminame)
		{
			tdbprintf1(5,"trying open %s\n", flock->aminame);
			flock->desc = Open(flock->aminame, hostmode);
			if (flock->desc)
			{
				flock->flags = FL_FILE;
				flock->lockmode = lockmode;
				flock->mod = mod;
				msg->io_FLock = flock;
				msg->io_Op.Open.Result = TTRUE;
				msg->io_Req.io_Error = 0;
				return;
			}
			msg->io_Req.io_Error = geterror(mod);
			TFree(flock->aminame);
		}
		TFree(flock);
	}
}

/*****************************************************************************/
/*
**	hnd_lock
*/

static TVOID hnd_lock(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock;
	TUINT mode = msg->io_Op.Lock.Mode;
	LONG hostmode;

	msg->io_Op.Lock.Result = TFALSE;

	switch (mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;
			
		case TFLOCK_WRITE:
			hostmode = ACCESS_WRITE;
			break;
			
		case TFLOCK_READ:
			hostmode = ACCESS_READ;
			break;
	}

	msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;	
	flock = TAlloc(mod->mmu, sizeof(FLOCK));
	if (flock)
	{
		if (msg->io_Op.Lock.Name[0] == 0)
		{
			/* empty string; lock on the filesystem root */
			flock->aminame = TNULL;
			flock->desc = TNULL;
			flock->flags = FL_LOCK | FL_ISDIR | FL_ISROOT;
			goto havelock;
		}

		flock->aminame = getaminame(mod, msg->io_Op.Lock.Name);
		if (flock->aminame)
		{
			flock->desc = Lock(flock->aminame, hostmode);
			if (flock->desc)
			{
				flock->flags = FL_LOCK;
havelock:		flock->lockmode = mode;
				msg->io_FLock = flock;
				msg->io_Op.Lock.Result = TTRUE;
				msg->io_Req.io_Error = 0;
				return;
			}
			msg->io_Req.io_Error = geterror(mod);
			TFree(flock->aminame);
		}
		TFree(flock);
	}
}

/*****************************************************************************/
/*
**	hnd_close
**	- applies to both locks and files.
*/

static TVOID hnd_close(TMOD_HND *mod, TIOMSG *msg)
{
	EXAMINE *e = msg->io_Examine;
	FLOCK *f = msg->io_FLock;
	
	if (e)
	{
		freelist(mod, &e->devlist);
		freelist(mod, &e->devlist_done);
		FreeDosObject(DOS_FIB, e->fib);
		TFree(e);
	}

	if (f->flags & FL_LOCK)
	{
		UnLock(f->desc);
	}
	else if (f->flags & FL_FILE)
	{
		if (!Close(f->desc))
		{
			msg->io_Req.io_Error = geterror(mod);
		}
	}

	TFree(f->aminame);
	TFree(f);
}

/*****************************************************************************/
/*
**	hnd_write
*/

static TVOID hnd_write(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;
	TINT res = -1;
	
	if (fl->flags & FL_FILE)
	{
		res = (TINT) 
			Write(fl->desc, msg->io_Op.Write.Buf, msg->io_Op.Write.Len);
		if (res == -1)
		{
			msg->io_Req.io_Error = geterror(mod);
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

static TVOID hnd_read(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;
	TINT res = -1;
	
	if (fl->flags & FL_FILE)
	{
		res = (TINT) Read(fl->desc, msg->io_Op.Read.Buf, msg->io_Op.Read.Len);
		if (res == -1)
		{
			msg->io_Req.io_Error = geterror(mod);
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
**	examinefib(mod, examine, fileinfoblock)
*/

static TVOID examinefib(TMOD_HND *mod, EXAMINE *e, struct FileInfoBlock *fib)
{
	TDOUBLE jd;

	e->size = fib->fib_Size;
	e->name = fib->fib_FileName;

	jd = fib->fib_Date.ds_Tick;
	jd /= 3000;
	jd += fib->fib_Date.ds_Minute;
	jd /= 1440;
	jd += fib->fib_Date.ds_Days;
	jd += 2443509.5;		/* 1.1.1978 in Julian */
	TJulianToDate(jd, &e->date);

	e->type = (fib->fib_DirEntryType > 0) ? TFTYPE_Directory : TFTYPE_File;
	tdbprintf1(2,"examined: %s\n", e->name);
}

/*****************************************************************************/
/*
**	examinetags(examine, tagitem)
**	fill examination info into user-supplied tags. this
**	is called back by TForEachTag()
*/

static TCALLBACK TBOOL examinetags(EXAMINE *e, TTAGITEM *ti)
{
	switch ((TUINT) ti->tti_Tag)
	{
		default:
			return TTRUE;

		case TFATTR_Name:
			*((TSTRPTR *) ti->tti_Value) = e->name;
			break;
		case TFATTR_Size:
			*((TINT *) ti->tti_Value) = e->size;
			break;
		case TFATTR_SizeHigh:
			*((TUINT *) ti->tti_Value) = 0;
			break;
		case TFATTR_Type:
			*((TINT *) ti->tti_Value) = e->type;
			break;
		case TFATTR_Date:
			*((TDATE *) ti->tti_Value) = e->date;
			break;
		case TFATTR_DateBox:
		{
			TMOD_HND *mod = e->mod;
			TUnpackDate(&e->date, (struct TDateBox *) ti->tti_Value, TDB_ALL);
			break;
		}
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
**
**	amiga-specific note: other than on other platforms, we do not
**	initially know whether a lock is a file or directory. we put the
**	missing flag into the flock->flags field here.
*/

static TVOID hnd_examine(TMOD_HND *mod, TIOMSG *msg)
{
	EXAMINE *e = msg->io_Examine;
	msg->io_Op.Examine.Result = -1;

	if (!e)
	{
		FLOCK *fl = msg->io_FLock;

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
					e->name = "";
					e->size = 0;
					e->type = TFTYPE_Directory;
					TLock(mod->lock);
					TGetDate(mod->treq, &e->date, TNULL);
					TUnlock(mod->lock);
					success = TTRUE;
				}
			}
			else
			{
				e->fib = AllocDosObject(DOS_FIB, NULL);
				if (e->fib)
				{
					if (fl->flags & FL_LOCK)
					{
						success = Examine(fl->desc, e->fib);
					}
					else if (fl->flags & FL_FILE)
					{
						success = ExamineFH(fl->desc, e->fib);
					}
				
					if (success)
					{
						if (e->fib->fib_DirEntryType > 0)
						{
							fl->flags |= FL_ISDIR;
						}
						examinefib(mod, e, e->fib);
					}
					else
					{
						msg->io_Req.io_Error = geterror(mod);
					}
				}
			}
				
			if (success)
			{
				msg->io_Examine = e;
			}
			else
			{
				if (e->fib) FreeDosObject(DOS_FIB, e->fib);
				TFree(e);
				return;
			}
		}
	}

	if (e)
	{
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
**	has been examined before an exnext is issued.
*/

static TVOID hnd_exnext(TMOD_HND *mod, TIOMSG *msg)
{
	EXAMINE *e = msg->io_Examine;
	FLOCK *fl = msg->io_FLock;
	msg->io_Op.Examine.Result = -1;

	if (!(fl->flags & FL_ISDIR))
	{
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
			TForEachTag(msg->io_Op.Examine.Tags,
				(TTAGFOREACHFUNC) examinetags, e);
			msg->io_Op.Examine.Result = e->numattr;
		}
		/* else no more entries */
	}
	else
	{
		if (ExNext(fl->desc, e->fib))
		{
			examinefib(mod, e, e->fib);
			e->numattr = 0;
			TForEachTag(msg->io_Op.Examine.Tags,
				(TTAGFOREACHFUNC) examinetags, e);
			msg->io_Op.Examine.Result = e->numattr;
		}
		else
		{
			msg->io_Req.io_Error = geterror(mod);
		}
	}
}

/*****************************************************************************/
/*
**	hnd_openfromlock
**	turn a lock into an open filehandle
*/

static TVOID hnd_openfromlock(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *fl = msg->io_FLock;

	msg->io_Op.OpenFromLock.Result = TFALSE;

	if (fl->flags & (FL_ISDIR | FL_FILE))
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
	}
	else
	{
		BPTR fh = OpenFromLock(fl->desc);
		if (fh)
		{
			fl->desc = fh;
			msg->io_Op.OpenFromLock.Result = TTRUE;
			fl->flags = FL_FILE;
		}
		else
		{
			msg->io_Req.io_Error = geterror(mod);
		}
	}
}

/*****************************************************************************/
/*
**	hnd_seek
**	seek in a file
*/

static TVOID hnd_seek(TMOD_HND *mod, TIOMSG *msg)
{
	LONG mode, res;
	FLOCK *fl = msg->io_FLock;

	msg->io_Op.Seek.Result = 0xffffffff;
	
	switch (msg->io_Op.Seek.Mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;

		case TFPOS_BEGIN:
			mode = OFFSET_BEGINNING;
			break;

		case TFPOS_CURRENT:
			mode = OFFSET_CURRENT;
			break;

		case TFPOS_END:
			mode = OFFSET_END;
			break;
	}

	if (!(fl->flags & FL_FILE))
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
		return;
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

	res = Seek(fl->desc, (LONG) msg->io_Op.Seek.Offs, mode);
	if (res >= 0)
	{
		/* AmigaDOS returns the previous position, not the new one */

		switch (mode)
		{
			case OFFSET_BEGINNING:
				msg->io_Op.Seek.Result = (TUINT) msg->io_Op.Seek.Offs;
				return;

			case OFFSET_CURRENT:
				res += msg->io_Op.Seek.Offs;
				msg->io_Op.Seek.Result = (TUINT) TMAX(0, res);
				return;

			case OFFSET_END:
				res = Seek(fl->desc, 0, OFFSET_CURRENT);
				if (res >= 0)
				{
					msg->io_Op.Seek.Result = (TUINT) res;
					return;
				}
		}
	}

	msg->io_Req.io_Error = geterror(mod);
}

/*****************************************************************************/
/*
**	hnd_rename
*/

static TVOID hnd_rename(TMOD_HND *mod, TIOMSG *msg)
{
	TSTRPTR srcname, dstname;

	msg->io_Op.Rename.Result = TFALSE;

	srcname = getaminame(mod, msg->io_Op.Rename.SrcName);
	dstname = getaminame(mod, msg->io_Op.Rename.DstName);

	if (srcname && dstname)
	{
		if (Rename(srcname, dstname))
		{
			msg->io_Op.Rename.Result = TTRUE;
		}
		else
		{
			msg->io_Req.io_Error = geterror(mod);
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

static TVOID hnd_makedir(TMOD_HND *mod, TIOMSG *msg)
{
	TSTRPTR aminame;
	
	msg->io_Op.Lock.Result = TFALSE;
	aminame = getaminame(mod, msg->io_Op.Lock.Name);
	if (aminame)
	{
		BPTR lock = CreateDir(aminame);
		if (lock)
		{
			UnLock(lock);	/* this is dead dirty... */
			hnd_lock(mod, msg);
		}
		else
		{
			msg->io_Req.io_Error = geterror(mod);
		}
		TFree(aminame);
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	}
}

/*****************************************************************************/
/*
**	hnd_delete
*/

static TVOID hnd_delete(TMOD_HND *mod, TIOMSG *msg)
{
	TSTRPTR aminame;
	
	msg->io_Op.Delete.Result = TFALSE;
	aminame = getaminame(mod, msg->io_Op.Delete.Name);
	if (aminame)
	{
		if (DeleteFile(aminame))
		{
			msg->io_Op.Delete.Result = TTRUE;
		}
		else
		{
			msg->io_Req.io_Error = geterror(mod);
		}
		TFree(aminame);
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

static TVOID hnd_makename(TMOD_HND *mod, TIOMSG *msg)
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
		src = flock->aminame;
		len = TStrLen(src) + 1;
	}
	
	temp = TAlloc(mod->mmu, len + len2 + 1);
	if (temp)
	{
		TSTRPTR tekname;

		if (src)
		{
			TStrCpy(temp, src);
			temp[len - 1] = '/';
		}

		TStrCpy(temp + len, name);

		tekname = gettekname(mod, mod->mmu, temp, 1);
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
			msg->io_Req.io_Error = geterror(mod);
		}
			
		TFree(temp);
	}
	
	msg->io_Op.MakeName.Result = res;
}

/*****************************************************************************/
/*
**	hnd_docmd
*/

TVOID hnd_docmd(TMOD_HND *mod, TIOMSG *msg)
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

static TTASKENTRY TVOID mod_task(TAPTR task)
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

static TMODAPI TVOID beginio(TMOD_HND *mod, TIOMSG *msg)
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
		TPutMsg(TGetUserPort(mod->task), msg->io_Req.io_ReplyPort, msg);
	}
}

/*****************************************************************************/
/*
**	abortio(msg->io_Device, msg)
*/

static TMODAPI TINT abortio(TMOD_HND *mod, TIOMSG *msg)
{
	/* not supported */
	return -1;
}
