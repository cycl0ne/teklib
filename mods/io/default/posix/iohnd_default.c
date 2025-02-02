
/*
**	$Id: iohnd_default.c,v 1.18 2005/11/06 22:01:47 tmueller Exp $
**	POSIX implementation of the 'default' I/O handler.
**	it is responsible for abstraction from the host file system.
*/

/* provide 64bit off_t */
#define _FILE_OFFSET_BITS 64

#include <tek/exec.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/time.h>
#include <tek/inline/io.h>
#include <tek/proto/hal.h>
#include <tek/mod/io.h>
#include <tek/mod/ioext.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <utime.h>

/*****************************************************************************/

#define MAX_PATH_LEN	PATH_MAX

#define MOD_VERSION		0
#define MOD_REVISION	3

typedef struct
{
	struct TModule module;
	TAPTR util;
	TAPTR time;
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
	TSTRPTR unixname;
	TUINT lockmode;
	int desc;
} FLOCK;

typedef struct
{
	TMOD_HND *mod;
	FLOCK *flock;
	DIR *dir;
	struct stat stat;
	TSTRPTR fullname;
	TINT pathlen;
	TINT fnbuflen;
	TDATE date;
	TUINT type;
	off_t size;
	TINT numattr;
	TBOOL havesizehi;
} EXAMINE;

#define FL_NONE		0x0000
#define FL_LOCK		0x0001
#define FL_FILE		0x0002
#define FL_ISDIR	0x0004

typedef struct TIOPacket TIOMSG;

#define	IOBUFSIZE_DEFAULT	4096

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
			mod->mmu = TCreateMMU(TNULL, 
				TMMUT_Tracking | TMMUT_TaskSafe, TNULL);
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
			
			if (mod->util && mod->time)
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

			msg->io_Req.io_Device = (struct TModule *) mod;
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

			TCloseModule(mod->time);
			TCloseModule(mod->util);
		}
	}
	TUnlock(mod->lock);
}

/*****************************************************************************/
/* 
**	ioerr = geterror(errno)
**	translate host errno to TEKlib ioerr
*/

static TINT 
geterror(int err)
{
	switch (err)
	{

	#ifdef ETXTBSY
		case ETXTBSY:
	#endif
		case EBUSY:
			return TIOERR_OBJECT_IN_USE;
	
		case ENOSPC:
			return TIOERR_DISK_FULL;
	
		case ENFILE:
		case EMFILE:
		case ENOMEM:
			return TIOERR_NOT_ENOUGH_MEMORY;

		case EPERM:
		case EACCES:
			return TIOERR_ACCESS_DENIED;

		case ENAMETOOLONG:
			return TIOERR_INVALID_NAME;

		case ENOENT:
			return TIOERR_OBJECT_NOT_FOUND;

		case EMLINK:		
		case ELOOP:
			return TIOERR_TOO_MANY_LEVELS;

		case EROFS:
			return TIOERR_DISK_WRITE_PROTECTED;
		
		case EEXIST:
			return TIOERR_OBJECT_EXISTS;

		case EFBIG:
			return TIOERR_OBJECT_TOO_LARGE;
		
		case ENXIO:
		case ENODEV:
		case EISDIR:
		case ENOTDIR:
			return TIOERR_OBJECT_WRONG_TYPE;
	
		case ENOTEMPTY:
			return TIOERR_DIRECTORY_NOT_EMPTY;

		case EXDEV:
			return TIOERR_NOT_SAME_DEVICE;
			
		case EIO:
			return TIOERR_DISK_CORRUPT;
	
		case EOVERFLOW:
			return TIOERR_OUT_OF_RANGE;
			
		default:
		case EFAULT:
		case EBADF:
		case EINVAL:
			perror("fehler");
			tdbprintf1(99,"unhandled host error: %d\n", err);
			tdbfatal(99);
			
		case 0:
			return 0;
	}
}

/* 
**	getunixname
*/

static TSTRPTR
getunixname(TMOD_HND *mod, TSTRPTR name)
{
	TSTRPTR unixname;
	TINT l = TStrLen(name);
	
	unixname = TAlloc(mod->mmu, l + 2);
	if (unixname)
	{
		unixname[0] = 0;
		if (name[0] != '/')
		{
			TStrCpy(unixname, "/");
		}
		TStrCat(unixname, name);
	}
	return unixname;
}

/* 
**	getcurrentdir
*/

static TSTRPTR
getcurrentdir(TMOD_HND *mod)
{
	TSTRPTR name;
	TINT len = 16;

	while ((name = TAlloc(mod->mmu, len)))
	{
		if (getcwd(name, len) == TNULL)
		{
			TFree(name);
			if (errno == ERANGE)
			{
				len <<= 1;
				continue;
			}
			name = TNULL;
		}
		break;
	}

	return name;
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
	TAPTR hal = TGetHALBase();
	TAPTR TIOBase;
	
	TIOBase = TOpenModule("io", 0, TNULL);
	if (TIOBase)
	{	
		TSTRPTR unixname;
		
		/* add currentdir */

		unixname = getcurrentdir(mod);
		if (unixname)
		{
			unixname[0] = ':';
			TAssignLate("CURRENTDIR", unixname);
			TFree(unixname);
		}

		/* add PROGDIR */		

		unixname = (TSTRPTR) THALGetAttr(hal, TExecBase_ProgDir, TNULL);
		if (unixname)
		{
			unixname = TStrDup(mod->mmu, unixname);
			if (unixname)
			{
				unixname[0] = ':';
				TAssignLate("PROGDIR", unixname);
				TFree(unixname);
			}
		}
		
		/* add SYS */

		unixname = (TSTRPTR) THALGetAttr(hal, TExecBase_SysDir, TNULL);
		if (unixname)
		{
			unixname = TStrDup(mod->mmu, unixname);
			if (unixname)
			{
				unixname[0] = ':';
				TAssignLate("SYS", unixname);
				TFree(unixname);
			}
		}

		/* add ENVARC */

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
	int hostmode;
	TUINT lockmode;
	
	msg->io_Op.Open.Result = TFALSE;

	switch (msg->io_Op.Open.Mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;
			
		case TFMODE_READONLY:
			hostmode = O_RDONLY;
			lockmode = TFLOCK_NONE;
			break;

		case TFMODE_OLDFILE:
			hostmode = O_RDWR;
			lockmode = TFLOCK_NONE;
			break;
			
		case TFMODE_NEWFILE:
			hostmode = O_RDWR | O_CREAT | O_TRUNC;
			lockmode = TFLOCK_WRITE;
			break;

		case TFMODE_READWRITE:
			hostmode = O_RDWR | O_CREAT;
			lockmode = TFLOCK_READ;
	}
	
	msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	flock = TAlloc(mod->mmu, sizeof(FLOCK));
	if (flock)
	{
		flock->unixname = getunixname(mod, msg->io_Op.Open.Name);
		if (flock->unixname)
		{
			tdbprintf1(5,"trying open %s\n", flock->unixname);
			flock->desc = 
				open(flock->unixname, hostmode, S_IRUSR|S_IWUSR|S_IRGRP);
			if (flock->desc != -1)
			{
				struct stat stat;
				if (fstat(flock->desc, &stat) == 0)
				{
					if (!S_ISDIR(stat.st_mode))
					{
						flock->flags = FL_FILE;
						flock->lockmode = lockmode;
						flock->mod = mod;
						msg->io_FLock = flock;
						msg->io_Op.Open.Result = TTRUE;
						msg->io_Req.io_Error = 0;
						return;
					}
					else msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
				}
				else msg->io_Req.io_Error = geterror(errno);
				close(flock->desc);
			}
			else msg->io_Req.io_Error = geterror(errno);
		}

		TFree(flock->unixname);
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
	TUINT mode = msg->io_Op.Lock.Mode;

	msg->io_Op.Lock.Result = TFALSE;

	switch (mode)
	{
		default:
			msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;
			return;
			
		case TFLOCK_WRITE:
		case TFLOCK_READ:
			break;
	}

	msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;	
	flock = TAlloc(mod->mmu, sizeof(FLOCK));
	if (flock)
	{
		flock->unixname = getunixname(mod, msg->io_Op.Lock.Name);
		if (flock->unixname)
		{
			flock->desc = open(flock->unixname, O_RDWR);
			flock->flags = FL_LOCK;

			if (flock->desc == -1)
			{
				if (/*errno == EISDIR &&*/ mode == TFLOCK_READ)
				{
					TBOOL isdir = (errno == EISDIR);
					/* retry readonly */
					flock->desc = open(flock->unixname, O_RDONLY);
					if (isdir) flock->flags |= FL_ISDIR;
				}
			}
			
			if (flock->desc != -1)
			{
				flock->lockmode = mode;
				flock->mod = mod;
				msg->io_FLock = flock;
				msg->io_Op.Lock.Result = TTRUE;
				msg->io_Req.io_Error = 0;
				return;
			}
			
			msg->io_Req.io_Error = geterror(errno);
		}
		
		TFree(flock->unixname);
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
		if (e->dir) closedir(e->dir);
		TFree(e->fullname);
		TFree(e);
	}

	if (close(f->desc) != 0)
	{
		msg->io_Req.io_Error = geterror(errno);
	}

	TFree(f->unixname);
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
		res = (TINT) 
			write(fl->desc, msg->io_Op.Write.Buf, msg->io_Op.Write.Len);
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
		res = (TINT) read(fl->desc, msg->io_Op.Read.Buf, msg->io_Op.Read.Len);
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

/*****************************************************************************/
/*
**	examinestat(mod, examine, stat)
*/

static TVOID
examinestat(TMOD_HND *mod, EXAMINE *e, struct stat *sp)
{
	TDOUBLE jd;
	
	jd = e->stat.st_mtime;
	jd /= 86400;
	jd += 2440587.5;

	TJulianToDate(jd, &e->date);

	if (S_ISREG(e->stat.st_mode))
	{
		e->type = TFTYPE_File;
		e->size = e->stat.st_size;
	}
	else if (S_ISDIR(e->stat.st_mode))
	{
		e->type = TFTYPE_Directory;
		e->size = 0;
	}
	else
	{
		e->type = TFTYPE_Unknown;
		e->size = e->stat.st_size;
	}
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
			*((TSTRPTR *) ti->tti_Value) = e->fullname + e->pathlen + 1;
			break;
		case TFATTR_Size:
			if (sizeof(off_t) >= 8)
			{
				if (!e->havesizehi)
				{
					if (e->size >= 0xffffffff)
					{
						*((TUINT *) ti->tti_Value) = 0xffffffff;
						break;
					}
				}
			}
			*((TUINT *) ti->tti_Value) = e->size & 0xffffffff;
			break;
		case TFATTR_SizeHigh:
			*((TUINT *) ti->tti_Value) = e->size >> 32;
			break;
		case TFATTR_Type:
			*((TUINT *) ti->tti_Value) = e->type;
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
			if (fstat(fl->desc, &e->stat) == 0)
			{
				if (fl->flags & FL_ISDIR)
				{
					e->dir = opendir(fl->unixname);
					if (!e->dir) goto fail;
				}
	
				e->flock = fl;
	
				/* prepare string buffer for the pathname length, rounded up,
				** with some extra space for the filenames to be examined */
	
				if (TStrCmp("/", e->flock->unixname))
				{			
					e->pathlen = TStrLen(e->flock->unixname);
				}
				else
				{
					e->pathlen = 0;
				}
	
				e->fnbuflen = (e->pathlen + 63) & ~63;		/* round up... */
				e->fnbuflen += 64;					/* and some extra space */
				e->fullname = TAlloc(mod->mmu, e->fnbuflen);
				if (!e->fullname) goto fail2;
	
				TStrCpy(e->fullname, e->flock->unixname);
				TStrCpy(e->fullname + e->pathlen, "/");
				msg->io_Examine = e;
				
				examinestat(mod, e, &e->stat);
				e->mod = mod;
			}
			else
			{
fail:			msg->io_Req.io_Error = geterror(errno);
fail2:			TFree(e);
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
**	has been examined before an exnext is issued.
*/

static TVOID
hnd_exnext(TMOD_HND *mod, TIOMSG *msg)
{
	EXAMINE *e = msg->io_Examine;
	msg->io_Op.Examine.Result = -1;

	if (!(e->flock->flags & FL_ISDIR))
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
		return;
	}

	for (;;)
	{
		struct dirent *de = readdir(e->dir);
		if (de)
		{
			TINT fnlen;
			TINT8 c;
			TSTRPTR fname = de->d_name;

			/* "." and ".." do not exist in our world */
			
			if (fname[0] == '.')
			{
				if (fname[1] == 0) continue;
				if (fname[1] == '.')
				{
					if (fname[2] == 0) continue;
				}
			}

			/* unfortunately we must reject files with ":" in their names,
			too. use the opportunity to determine the string length. */
			
			fnlen = 0;
calclen:	c = fname[fnlen];
			if (c == ':') continue;
			if (c)
			{
				fnlen++;
				goto calclen;
			}

			if (fnlen + e->pathlen + 2 > e->fnbuflen)
			{
				TSTRPTR newfnbuf;
				e->fnbuflen = (fnlen + e->pathlen + 2 + 63) & ~63;
				newfnbuf = TRealloc(e->fullname, e->fnbuflen);
				if (!newfnbuf)
				{
					msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
					break;
				}
				e->fullname = newfnbuf;
			}
			
			TStrCpy(e->fullname + e->pathlen + 1, fname);
			
			if (stat(e->fullname, &e->stat))
			{
				if (errno == ENOENT)
				{
					/* assuming it's a dead softlink - silently ignore it. */
					continue;
				}
			
				tdbprintf1(10,"exnext %s:\n", e->fullname);
				msg->io_Req.io_Error = geterror(errno);
				break;
			}

			examinestat(mod, e, &e->stat);
			e->numattr = 0;
			e->havesizehi = 
				TGetTag(msg->io_Op.Examine.Tags, TFATTR_SizeHigh, TNULL);
			TForEachTag(msg->io_Op.Examine.Tags, 
				(TTAGFOREACHFUNC) examinetags, e);
			msg->io_Op.Examine.Result = e->numattr;
			msg->io_Req.io_Error = 0;
			return;
		}
		else msg->io_Req.io_Error = TIOERR_NO_MORE_ENTRIES;
		
		break;
	}
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
	int whence;
	off_t offset, res, offs_hi;

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

	if (!(fl->flags & FL_FILE))
	{
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
		return;
	}

	if (msg->io_Op.Seek.OffsHi)
	{
		offs_hi = (off_t) *msg->io_Op.Seek.OffsHi;
	}
	else
	{
		offs_hi = 0;
	}
	
	if (sizeof(off_t) >= 8)
	{
		offset = offs_hi << 32;
		offset |= ((off_t) msg->io_Op.Seek.Offs) & 0xffffffff;
	}
	else
	{
		if (offs_hi > 0 || offs_hi < -1)
		{
			msg->io_Req.io_Error = TIOERR_OUT_OF_RANGE;
			return;
		}
		offset = (off_t) msg->io_Op.Seek.Offs;
	}

	res = lseek(fl->desc, offset, whence);
	
	if (res == -1)
	{
		msg->io_Req.io_Error = geterror(errno);		
		return;
	}
	
	if (sizeof(off_t) >= 8)
	{
		offs_hi = res >> 32;
		if (msg->io_Op.Seek.OffsHi)
		{
			*msg->io_Op.Seek.OffsHi = (TINT) offs_hi;
		}
		else
		{
			if (offs_hi > 0 || offs_hi < -1)
			{
				/* cannot return result properly */
				msg->io_Req.io_Error = TIOERR_OUT_OF_RANGE;
				return;
			}
		}
		res &= 0xffffffff;
	}
	
	msg->io_Op.Seek.Result = (TUINT) res;
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

	srcname = getunixname(mod, msg->io_Op.Rename.SrcName);
	dstname = getunixname(mod, msg->io_Op.Rename.DstName);

	if (srcname && dstname)
	{
		if (rename(srcname, dstname) == 0)
		{
			msg->io_Op.Rename.Result = TTRUE;
		}
		else
		{
			tdbprintf2(5,"rename %s -> %s failed\n", srcname, dstname);
			msg->io_Req.io_Error = geterror(errno);		
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
**	returns a shared lock on a successfully created directory.
*/

static TVOID
hnd_makedir(TMOD_HND *mod, TIOMSG *msg)
{
	TSTRPTR unixname;

	msg->io_Op.Lock.Result = TFALSE;

	unixname = getunixname(mod, msg->io_Op.Lock.Name);
	if (unixname)
	{
		if (mkdir(unixname, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) == 0)		/* mode: 755 */
		{
			hnd_lock(mod, msg);
		}
		else
		{
			msg->io_Req.io_Error = geterror(errno);
		}
		TFree(unixname);
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

static TVOID
hnd_delete(TMOD_HND *mod, TIOMSG *msg)
{
	TSTRPTR unixname;
	
	msg->io_Op.Delete.Result = TFALSE;
	unixname = getunixname(mod, msg->io_Op.Delete.Name);
	if (unixname)
	{
		tdbprintf1(2,"delete: %s\n", unixname);
		if (remove(unixname) == 0)
		{
			msg->io_Op.Delete.Result = TTRUE;
		}
		else
		{
			tdbprintf1(5,"delete %s failed\n", unixname);
			msg->io_Req.io_Error = geterror(errno);
		}
		TFree(unixname);
	}
	else
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	}
}

/*****************************************************************************/
/*
**	hnd_setdate
*/

static TVOID
hnd_setdate(TMOD_HND *mod, TIOMSG *msg)
{
	TSTRPTR unixname;
	
	msg->io_Op.SetDate.Result = TFALSE;
	unixname = getunixname(mod, msg->io_Op.SetDate.Name);
	if (unixname)
	{
		struct utimbuf ut, *utp = TNULL;

		if (msg->io_Op.SetDate.Date != TNULL)
		{
			TDOUBLE x = msg->io_Op.SetDate.Date->tdt_Day.tdtt_Double;
			x -= 2440587.5;	/* days since 1.1.1970 */
			x *= 86400;		/* secs since 1.1.1970 */
			ut.actime = x;
			ut.modtime = x;
			utp = &ut;
		}
		
		if (utime(unixname, utp) == 0)
		{
			msg->io_Op.Delete.Result = TTRUE;
		}
		else
		{
			tdbprintf1(5,"setdate %s failed\n", unixname);
			msg->io_Req.io_Error = geterror(errno);
		}

		TFree(unixname);
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

static TBOOL
resolvepath(TMOD_HND *mod, TSTRPTR src, TSTRPTR dest)
{
	TINT len = TStrLen(src);
	TSTRPTR sp = src + len;
	TSTRPTR dp = dest;
	TINT dc = 0, slc = 0, eac = 0, wc = 0;
	TINT i, c;
	
	while (len--)	
	{
		c = *(--sp);

		switch (c)
		{
			case '/':
				if (dc == 2) eac++;
				dc = 0;
				slc = 1;
				wc = 0;
				break;

			case '.':
				if (slc)
				{
					dc++;
					break;
				}
				
				/* fallthru: */

			default:
				if (wc) break;
			
				if (slc)
				{
					slc = 0;
					
					if (eac > 0)
					{
						/* resolve one eatcount */
						eac--;
						/* now wait for next path part */
						wc = 1;
						break;
					}
					
					*dp++ = '/';
				}
				
				while (dc == 2 || dc == 1)
				{
					*dp++ = '.'; dc--;
				}
				dc = 0;
				
				*dp++ = c;
				break;
		}
	}
	
	/* unresolved eatcount */
	if (eac) return TFALSE;
	
	/* resolve remaining slash */
	if (slc) *dp++ = '/';
	
	*dp = 0;
	
	len = dp - dest;
	for (i = 0; i < len / 2; ++i)
	{
		TUINT8 t = dest[i];
		dest[i] = dest[len - i - 1]; 
		dest[len - i - 1] = t;
	}

	return TTRUE;
}

static TVOID 
hnd_makename(TMOD_HND *mod, TIOMSG *msg)
{
	FLOCK *flock = msg->io_FLock;
	TSTRPTR name = msg->io_Op.MakeName.Name;
	TSTRPTR dest = msg->io_Op.MakeName.Dest;

	TSTRPTR src, temp, temp2;
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
	if (name[0] == '/')
	{
		src = TNULL;		/* absolute */
		len = 0;
	}
	else					/* relative */
	{
		src = flock->unixname;
		len = TStrLen(src) + 1;
	}
		
	
	temp = TAlloc(mod->mmu, len + len2 + 2);
	temp2 = TAlloc(mod->mmu, len + len2 + 1);
	if (temp && temp2)
	{
		if (src)
		{
			TStrCpy(temp, src);
			temp[len - 1] = '/';
			temp[len] = 0;
		}

		TStrCpy(temp + len, name);
		tdbprintf1(5,"resolving %s\n", temp);
		
		/*if (realpath(temp, mod->maxpath) == mod->maxpath)*/
		if (resolvepath(mod, temp, temp2))
		{
			tdbprintf1(5,"name resolved: %s\n", temp2);
			res = TStrLen(temp2);
			if (dest)
			{
				if (res < msg->io_Op.MakeName.DLen)
				{
					TStrCpy(dest, temp2);
					dest[0] = ':';
				}
				else
				{
					res = -1;
					msg->io_Req.io_Error = TIOERR_LINE_TOO_LONG;
				}
			}
		}
		else
		{
			res = -1;
			msg->io_Req.io_Error = geterror(errno);
			msg->io_Req.io_Error = TIOERR_INVALID_NAME;
		}
	}
	TFree(temp2);
	TFree(temp);
	
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

		case TIOCMD_SETDATE:
			hnd_setdate(mod, msg);
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
		TPutMsg(TGetUserPort(mod->task), msg->io_Req.io_ReplyPort, msg);
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

