
/*
**	$Id: iohnd_default.c,v 1.6 2006/11/11 14:19:10 tmueller Exp $
**	teklib/src/io/posix/iohnd_default.c - Default I/O handler implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

/* provide 64bit off_t */
#define _FILE_OFFSET_BITS 64

#include <tek/exec.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/string.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/inline/io.h>
#include <tek/proto/hal.h>
#include <tek/mod/time.h>
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
#include <sys/wait.h>

/*****************************************************************************/

#define IODEF_PATH_MAX	PATH_MAX

#define IODEF_VERSION	0
#define IODEF_REVISION	4

typedef struct
{
	struct TModule module;
	TAPTR iodef_ExecBase;
	TAPTR iodef_UtilBase;
	TAPTR iodef_Lock;
	TUINT iodef_RefCount;
	TAPTR iodef_Task;
	TAPTR iodef_MemManager;
} IODEF_MOD;

typedef struct
{
	IODEF_MOD *mod;
	TUINT flags;
	TSTRPTR unixname;
	TUINT lockmode;
	int desc;
} IODEF_FLOCK;

typedef struct
{
	IODEF_MOD *mod;
	IODEF_FLOCK *flock;
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
} IODEF_EXAMINE;

#define FL_NONE		0x0000
#define FL_LOCK		0x0001
#define FL_FILE		0x0002
#define FL_ISDIR	0x0004

#define	IODEF_BUFSIZE_DEFAULT	4096

/*****************************************************************************/

static THOOKENTRY TTAG iodef_mod_dispatch(struct THook *hook, TAPTR obj,
	TTAG msg);
static struct TIOPacket *iodef_mod_open(IODEF_MOD *mod, TTAGITEM *tags);
static void iodef_mod_close(IODEF_MOD *mod);
static void iodef_task(struct TTask *task);
static void iodef_init(IODEF_MOD *mod);

static TMODAPI void iodef_beginio(IODEF_MOD *mod, struct TIOPacket *msg);
static TMODAPI TINT iodef_abortio(IODEF_MOD *mod, struct TIOPacket *msg);

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

	mod->iodef_ExecBase = TGetExecBase(mod);
	mod->iodef_Lock = TExecCreateLock(mod->iodef_ExecBase, TNULL);
	if (mod->iodef_Lock)
	{
		mod->module.tmd_Version = IODEF_VERSION;
		mod->module.tmd_Revision = IODEF_REVISION;
		mod->module.tmd_Handle.thn_Hook.thk_Entry = iodef_mod_dispatch;
		mod->module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;

		#ifdef TDEBUG
			mod->iodef_MemManager = TExecCreateMemManager(mod->iodef_ExecBase,
				TNULL, TMMT_Tracking | TMMT_TaskSafe, TNULL);
		#endif

		((TMFPTR *) mod)[-3] = (TMFPTR) iodef_beginio;
		((TMFPTR *) mod)[-4] = (TMFPTR) iodef_abortio;

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
			TDestroy(mod->iodef_MemManager);
			TDestroy(mod->iodef_Lock);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) iodef_mod_open(mod, obj);
		case TMSG_CLOSEMODULE:
			iodef_mod_close(obj);
			break;
		case TMSG_INITTASK:
			return TTRUE;
		case TMSG_RUNTASK:
			iodef_task(obj);
			break;
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
	struct TIOPacket *msg;

	msg = TExecAllocMsg0(mod->iodef_ExecBase, sizeof(struct TIOPacket));
	if (msg)
	{
		TExecLock(mod->iodef_ExecBase, mod->iodef_Lock);

		if (!mod->iodef_Task)
		{
			mod->iodef_UtilBase = TExecOpenModule(mod->iodef_ExecBase, "util",
				0, TNULL);
			if (mod->iodef_UtilBase)
			{
				TTAGITEM tasktags[2];
				tasktags[0].tti_Tag = TTask_UserData;
				tasktags[0].tti_Value = (TTAG) mod;
				tasktags[1].tti_Tag = TTAG_DONE;
				mod->iodef_Task = TExecCreateTask(mod->iodef_ExecBase,
					&mod->module.tmd_Handle.thn_Hook, tasktags);
				if (mod->iodef_Task)
					iodef_init(mod);
			}
			else
				TExecCloseModule(mod->iodef_ExecBase, mod->iodef_UtilBase);
		}

		if (mod->iodef_Task)
		{
			mod->iodef_RefCount++;

			/* insert recommended buffer settings */
			msg->io_BufSize = IODEF_BUFSIZE_DEFAULT;
			msg->io_BufFlags = TIOBUF_FULL;

			msg->io_Req.io_Device = (struct TModule *) mod;
		}
		else
		{
			TExecFree(mod->iodef_ExecBase, msg);
			msg = TNULL;
		}

		TExecUnlock(mod->iodef_ExecBase, mod->iodef_Lock);
	}

	return msg;
}

static void
iodef_mod_close(IODEF_MOD *mod)
{
	TExecLock(mod->iodef_ExecBase, mod->iodef_Lock);
	if (mod->iodef_Task)
	{
		if (--mod->iodef_RefCount == 0)
		{
			TExecSignal(mod->iodef_ExecBase, mod->iodef_Task, TTASK_SIG_ABORT);
			TDestroy(mod->iodef_Task);
			mod->iodef_Task = TNULL;
			TExecCloseModule(mod->iodef_ExecBase, mod->iodef_UtilBase);
		}
	}
	TExecUnlock(mod->iodef_ExecBase, mod->iodef_Lock);
}

/*****************************************************************************/
/*
**	ioerr = geterror(errno)
**	translate host errno to TEKlib ioerr
*/

static TINT
iodef_geterror(int err)
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
			TDBPRINTF(99,("unhandled host error: %d\n", err));
			TDBFATAL();

		case 0:
			return 0;
	}
}

/*
**	getunixname
*/

static TSTRPTR
iodef_getunixname(IODEF_MOD *mod, TSTRPTR name)
{
	TINT l = TStrLen(name);
	TSTRPTR unixname = TExecAlloc(mod->iodef_ExecBase, mod->iodef_MemManager,
		l + 2);
	if (unixname)
	{
		unixname[0] = 0;
		if (name[0] != '/')
			TStrCpy(unixname, "/");
		TStrCat(unixname, name);
	}
	return unixname;
}

/*
**	getcurrentdir
*/

static TSTRPTR
iodef_getcurrentdir(IODEF_MOD *mod)
{
	TSTRPTR name;
	TINT len = 16;

	while ((name = TExecAlloc(mod->iodef_ExecBase, mod->iodef_MemManager, len)))
	{
		if (getcwd(name, len) == TNULL)
		{
			TExecFree(mod->iodef_ExecBase, name);
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
**	iodef_init
**	add late-binding assigns, such as CURRENTDIR:, PROGDIR: and SYS:.
**	note: calls to iobase are allowed here, because in the instance
**	open function we're running in the same task context as the module
**	opener, who already holds locks to the base.
*/

static void
iodef_init(IODEF_MOD *mod)
{
	TAPTR hal = TExecGetHALBase(mod->iodef_ExecBase);
	TAPTR TIOBase;

	TIOBase = TExecOpenModule(mod->iodef_ExecBase, "io", 0, TNULL);
	if (TIOBase)
	{
		TSTRPTR unixname;

		/* add currentdir */

		unixname = iodef_getcurrentdir(mod);
		if (unixname)
		{
			unixname[0] = ':';
			TAssignLate("CURRENTDIR", unixname);
			TExecFree(mod->iodef_ExecBase, unixname);
		}

		/* add PROGDIR */

		unixname = (TSTRPTR) THALGetAttr(hal, TExecBase_ProgDir, TNULL);
		if (unixname)
		{
			unixname = TUtilStrDup(mod->iodef_UtilBase, mod->iodef_MemManager,
				unixname);
			if (unixname)
			{
				unixname[0] = ':';
				TAssignLate("PROGDIR", unixname);
				TExecFree(mod->iodef_ExecBase, unixname);
			}
		}

		/* add SYS */

		unixname = (TSTRPTR) THALGetAttr(hal, TExecBase_SysDir, TNULL);
		if (unixname)
		{
			unixname = TUtilStrDup(mod->iodef_UtilBase, mod->iodef_MemManager,
				unixname);
			if (unixname)
			{
				unixname[0] = ':';
				TAssignLate("SYS", unixname);
				TExecFree(mod->iodef_ExecBase, unixname);
			}
		}

		/* add ENVARC */

		TAssignLate("ENVARC", "SYS:prefs/env-archive");

		/* add T */
		TAssignLate("T", ":tmp");

		TExecCloseModule(mod->iodef_ExecBase, TIOBase);
	}
}

/*****************************************************************************/
/*
**	iodef_open
*/

static void
iodef_open(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_FLOCK *flock;
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
	flock = TExecAlloc(mod->iodef_ExecBase, mod->iodef_MemManager,
		sizeof(IODEF_FLOCK));
	if (flock)
	{
		flock->unixname = iodef_getunixname(mod, msg->io_Op.Open.Name);
		if (flock->unixname)
		{
			TDBPRINTF(TDB_INFO,("trying to open %s\n", flock->unixname));
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
					else
						msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
				}
				else
					msg->io_Req.io_Error = iodef_geterror(errno);
				close(flock->desc);
			}
			else
				msg->io_Req.io_Error = iodef_geterror(errno);
		}

		TExecFree(mod->iodef_ExecBase, flock->unixname);
		TExecFree(mod->iodef_ExecBase, flock);
	}
}

/*****************************************************************************/
/*
**	iodef_lock
*/

static void
iodef_lock(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_FLOCK *flock;
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
	flock = TExecAlloc(mod->iodef_ExecBase, mod->iodef_MemManager,
		sizeof(IODEF_FLOCK));
	if (flock)
	{
		flock->unixname = iodef_getunixname(mod, msg->io_Op.Lock.Name);
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
					if (isdir)
						flock->flags |= FL_ISDIR;
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

			msg->io_Req.io_Error = iodef_geterror(errno);
		}

		TExecFree(mod->iodef_ExecBase, flock->unixname);
		TExecFree(mod->iodef_ExecBase, flock);
	}
}

/*****************************************************************************/
/*
**	iodef_close
**	- applies to both locks and files.
*/

static void
iodef_close(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_EXAMINE *e = msg->io_Examine;
	IODEF_FLOCK *f = msg->io_FLock;

	if (e)
	{
		if (e->dir)
			closedir(e->dir);
		TExecFree(mod->iodef_ExecBase, e->fullname);
		TExecFree(mod->iodef_ExecBase, e);
	}

	if (close(f->desc) != 0)
		msg->io_Req.io_Error = iodef_geterror(errno);

	TExecFree(mod->iodef_ExecBase, f->unixname);
	TExecFree(mod->iodef_ExecBase, f);
}

/*****************************************************************************/
/*
**	iodef_write
*/

static void
iodef_write(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_FLOCK *fl = msg->io_FLock;
	TINT res = -1;

	if (fl->flags & FL_FILE)
	{
		res = (TINT)
			write(fl->desc, msg->io_Op.Write.Buf, msg->io_Op.Write.Len);
		if (res == -1)
			msg->io_Req.io_Error = iodef_geterror(errno);
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
	IODEF_FLOCK *fl = msg->io_FLock;
	TINT res = -1;

	if (fl->flags & FL_FILE)
	{
		res = (TINT) read(fl->desc, msg->io_Op.Read.Buf, msg->io_Op.Read.Len);
		if (res == -1)
			msg->io_Req.io_Error = iodef_geterror(errno);
	}
	else
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;

	msg->io_Op.Read.RdLen = res;
}

/*****************************************************************************/
/*
**	examinestat(mod, examine, stat)
*/

static void
iodef_examinestat(IODEF_MOD *mod, IODEF_EXAMINE *e, struct stat *sp)
{
	TUINT64 t = e->stat.st_mtime;
	t *= 1000000;
	t += 11644473600000000ULL;
	e->date.tdt_Int64 = t;

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
			IODEF_MOD *mod = e->mod;
			TUtilUnpackDate(mod->iodef_UtilBase, &e->date,
				(struct TDateBox *) ti->tti_Value, TDB_ALL);
			break;
		}
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
	IODEF_FLOCK *fl = msg->io_FLock;
	IODEF_EXAMINE *e = msg->io_Examine;

	msg->io_Op.Examine.Result = -1;

	if (!e)
	{
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
		e = TExecAlloc0(mod->iodef_ExecBase, mod->iodef_MemManager,
			sizeof(IODEF_EXAMINE));
		if (e)
		{
			if (fstat(fl->desc, &e->stat) == 0)
			{
				if (fl->flags & FL_ISDIR)
				{
					e->dir = opendir(fl->unixname);
					if (!e->dir)
						goto fail;
				}

				e->flock = fl;

				/* prepare string buffer for the pathname length, rounded up,
				** with some extra space for the filenames to be examined */

				if (TStrCmp("/", e->flock->unixname))
					e->pathlen = TStrLen(e->flock->unixname);
				else
					e->pathlen = 0;

				e->fnbuflen = (e->pathlen + 63) & ~63;	/* round up... */
				e->fnbuflen += 64;	/* add some extra space */
				e->fullname = TExecAlloc(mod->iodef_ExecBase,
					mod->iodef_MemManager, e->fnbuflen);
				if (!e->fullname)
					goto fail2;

				TStrCpy(e->fullname, e->flock->unixname);
				TStrCpy(e->fullname + e->pathlen, "/");
				msg->io_Examine = e;

				iodef_examinestat(mod, e, &e->stat);
				e->mod = mod;
			}
			else
			{
fail:			msg->io_Req.io_Error = iodef_geterror(errno);
fail2:			TExecFree(mod->iodef_ExecBase, e);
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

		/*TForEachTag(msg->io_Op.Examine.Tags,
			(TTAGFOREACHFUNC) iodef_examinetags, e);*/

		msg->io_Op.Examine.Result = e->numattr;
		msg->io_Req.io_Error = 0;
	}
}

/*****************************************************************************/
/*
**	iodef_exnext
**	examine next entry. only directory locks are supported.
**	note: the IO module guarantees that the directory lock
**	has been examined before an exnext is issued.
*/

static void
iodef_exnext(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_EXAMINE *e = msg->io_Examine;
	struct THook hook;
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
			TINT c;
			TSTRPTR fname = de->d_name;

			/* "." and ".." do not exist in our world */

			if (fname[0] == '.')
			{
				if (fname[1] == 0)
					continue;
				if (fname[1] == '.' && fname[2] == 0)
					continue;
			}

			/* unfortunately we must reject files with ":" in their names,
			too. use the opportunity to determine the string length. */

			fnlen = 0;
calclen:	c = fname[fnlen];
			if (c == ':')
				continue;
			if (c)
			{
				fnlen++;
				goto calclen;
			}

			if (fnlen + e->pathlen + 2 > e->fnbuflen)
			{
				TSTRPTR newfnbuf;
				e->fnbuflen = (fnlen + e->pathlen + 2 + 63) & ~63;
				newfnbuf = TExecRealloc(mod->iodef_ExecBase, e->fullname,
					e->fnbuflen);
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
				if (errno == ENOENT || errno == EPERM)
				{
					/* assuming it's a dead softlink (or one we can't follow),
					silently ignore it. */
					continue;
				}

				TDBPRINTF(10,("exnext %s:\n", e->fullname));
				msg->io_Req.io_Error = iodef_geterror(errno);
				break;
			}

			iodef_examinestat(mod, e, &e->stat);
			e->numattr = 0;
			e->havesizehi =
				TGetTag(msg->io_Op.Examine.Tags, TFATTR_SizeHigh, TNULL);

			TInitHook(&hook, iodef_examinetags, e);
			TForEachTag(msg->io_Op.Examine.Tags, &hook);
			msg->io_Op.Examine.Result = e->numattr;
			msg->io_Req.io_Error = 0;
			return;
		}
		else
			msg->io_Req.io_Error = TIOERR_NO_MORE_ENTRIES;

		break;
	}
}

/*****************************************************************************/
/*
**	iodef_openfromlock
**	turn a lock into an open filehandle
*/

static void
iodef_openfromlock(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_FLOCK *fl = msg->io_FLock;

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
**	iodef_seek
**	seek in a file
*/

static void
iodef_seek(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_FLOCK *fl = msg->io_FLock;
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
		offs_hi = (off_t) *msg->io_Op.Seek.OffsHi;
	else
		offs_hi = 0;

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
		msg->io_Req.io_Error = iodef_geterror(errno);
		return;
	}

	if (sizeof(off_t) >= 8)
	{
		offs_hi = res >> 32;
		if (msg->io_Op.Seek.OffsHi)
			*msg->io_Op.Seek.OffsHi = (TINT) offs_hi;
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
**	iodef_rename
*/

static void
iodef_rename(IODEF_MOD *mod, struct TIOPacket *msg)
{
	TSTRPTR srcname, dstname;

	msg->io_Op.Rename.Result = TFALSE;

	srcname = iodef_getunixname(mod, msg->io_Op.Rename.SrcName);
	dstname = iodef_getunixname(mod, msg->io_Op.Rename.DstName);

	if (srcname && dstname)
	{
		if (rename(srcname, dstname) == 0)
			msg->io_Op.Rename.Result = TTRUE;
		else
			msg->io_Req.io_Error = iodef_geterror(errno);
	}
	else
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;

	TExecFree(mod->iodef_ExecBase, dstname);
	TExecFree(mod->iodef_ExecBase, srcname);
}

/*****************************************************************************/
/*
**	iodef_makedir
**	returns a shared lock on a successfully created directory.
*/

static void
iodef_makedir(IODEF_MOD *mod, struct TIOPacket *msg)
{
	TSTRPTR unixname = iodef_getunixname(mod, msg->io_Op.Lock.Name);
	msg->io_Op.Lock.Result = TFALSE;
	if (unixname)
	{
		if (mkdir(unixname, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) == 0)
			iodef_lock(mod, msg);
		else
			msg->io_Req.io_Error = iodef_geterror(errno);
		TExecFree(mod->iodef_ExecBase, unixname);
	}
	else
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
}

/*****************************************************************************/
/*
**	iodef_delete
*/

static void
iodef_delete(IODEF_MOD *mod, struct TIOPacket *msg)
{
	TSTRPTR unixname = iodef_getunixname(mod, msg->io_Op.Delete.Name);
	msg->io_Op.Delete.Result = TFALSE;
	if (unixname)
	{
		if (remove(unixname) == 0)
			msg->io_Op.Delete.Result = TTRUE;
		else
			msg->io_Req.io_Error = iodef_geterror(errno);
		TExecFree(mod->iodef_ExecBase, unixname);
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
	TSTRPTR unixname = iodef_getunixname(mod, msg->io_Op.SetDate.Name);
	msg->io_Op.SetDate.Result = TFALSE;
	if (unixname)
	{
		struct utimbuf ut, *utp = TNULL;
		TUINT64 t = msg->io_Op.SetDate.Date->tdt_Int64;
		t -= 11644473600000000ULL;
		t /= 1000000;
		ut.actime = (time_t) t;
		ut.modtime = (time_t) t;
		utp = &ut;
		if (utime(unixname, utp) == 0)
			msg->io_Op.SetDate.Result = TTRUE;
		else
			msg->io_Req.io_Error = iodef_geterror(errno);
		TExecFree(mod->iodef_ExecBase, unixname);
	}
	else
		msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
}

/*****************************************************************************/
/*
**	iodef_makename
*/

static TBOOL
iodef_resolvepath(IODEF_MOD *mod, TSTRPTR src, TSTRPTR dest)
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
				if (dc == 2)
					eac++;
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
				if (wc)
					break;

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
					*dp++ = '.';
					dc--;
				}
				dc = 0;

				*dp++ = c;
				break;
		}
	}

	/* unresolved eatcount */
	if (eac)
		return TFALSE;

	/* resolve remaining slash */
	if (slc)
		*dp++ = '/';

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

static void
iodef_makename(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_FLOCK *flock = msg->io_FLock;
	TSTRPTR name = msg->io_Op.MakeName.Name;
	TSTRPTR dest = msg->io_Op.MakeName.Dest;
	TINT res = -1;

	msg->io_Op.MakeName.Result = -1;
	msg->io_Req.io_Error = TIOERR_BAD_ARGUMENTS;

	if (msg->io_Op.MakeName.Mode == TPPF_HOST2TEK)
	{
		TSTRPTR src, temp, temp2;
		TINT len, len2 = TStrLen(name);
		if (name[0] == '/')
		{
			src = TNULL; /* absolute */
			len = 0;
		}
		else /* relative */
		{
			src = flock->unixname;
			len = TStrLen(src) + 1;
		}

		temp = TExecAlloc(mod->iodef_ExecBase, mod->iodef_MemManager,
			len + len2 + 2);
		temp2 = TExecAlloc(mod->iodef_ExecBase, mod->iodef_MemManager,
			len + len2 + 1);
		if (temp && temp2)
		{
			if (src)
			{
				TStrCpy(temp, src);
				temp[len - 1] = '/';
				temp[len] = 0;
			}

			TStrCpy(temp + len, name);

			/* realpath() is evil, we use our own */
 			if (iodef_resolvepath(mod, temp, temp2))
			{
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
				msg->io_Req.io_Error = iodef_geterror(errno);
				msg->io_Req.io_Error = TIOERR_INVALID_NAME;
			}
		}
		TExecFree(mod->iodef_ExecBase, temp2);
		TExecFree(mod->iodef_ExecBase, temp);
	}
	else if (msg->io_Op.MakeName.Mode == TPPF_TEK2HOST)
	{
		TSTRPTR unixname = iodef_getunixname(mod, name);
		if (unixname)
		{
			res = TStrLen(unixname);
			if (res < msg->io_Op.MakeName.DLen)
			{
				if (dest)
					TStrCpy(dest, unixname);
			}
			else
			{
				res = -1;
				msg->io_Req.io_Error = TIOERR_LINE_TOO_LONG;
			}
			TExecFree(mod->iodef_ExecBase, unixname);
		}
	}

	msg->io_Op.MakeName.Result = res;
}

#if 0
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
		TINT len = TExecGetSize(mod->iodef_ExecBase, cmdline);
		TSTRPTR ncmdline = TExecRealloc(mod->iodef_ExecBase, cmdline,
			len + 1 + TStrLen(args));
		if (ncmdline)
		{
			ncmdline[len - 1] = ' ';
			TStrCpy(ncmdline + len, args);
			cmdline = ncmdline;
		}
		else
		{
			TExecFree(mod->iodef_ExecBase, cmdline);
			cmdline = TNULL;
		}
	}
	return cmdline;
}

static void
iodef_execute(IODEF_MOD *mod, struct TIOPacket *msg)
{
	IODEF_FLOCK *flock = msg->io_FLock;
	msg->io_Op.Execute.Result = -1;
	if (flock && (flock->flags & FL_ISDIR))
	{
		TSTRPTR cmd = msg->io_Op.Execute.Command;
		int od = open(".", O_RDONLY);
		if (od != -1)
		{
			if (chdir(flock->unixname) == 0)
			{
				if (msg->io_Op.Execute.Flags == 0) /* absolute? */
					cmd = iodef_getunixname(mod, cmd);
				else
					cmd = TStrDup(TNULL, cmd);

				cmd = iodef_makecmd(mod, cmd, msg->io_Op.Execute.Args);
				if (cmd)
					msg->io_Op.Execute.Result = WEXITSTATUS(system(cmd));
				else
					msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;

				TExecFree(mod->iodef_ExecBase, cmd);
				fchdir(od);
			}
			else
				msg->io_Req.io_Error = TIOERR_OBJECT_NOT_FOUND;
		}
		else
			msg->io_Req.io_Error = TIOERR_NOT_ENOUGH_MEMORY;
	}
	else
		msg->io_Req.io_Error = TIOERR_OBJECT_WRONG_TYPE;
}
#endif

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

		case TIOCMD_MAKENAME:
			iodef_makename(mod, msg);
			break;

		case TIOCMD_SETDATE:
			iodef_setdate(mod, msg);
			break;

	#if 0
		case TIOCMD_EXECUTE:
			iodef_execute(mod, msg);
			break;
	#endif
	}
}

/*****************************************************************************/
/*
**	handler task
*/

static void iodef_task(struct TTask *task)
{
	IODEF_MOD *mod = TExecGetTaskData(TGetExecBase(task), task);
	TAPTR port = TExecGetUserPort(mod->iodef_ExecBase, task);
	TUINT portsig = TExecGetPortSignal(mod->iodef_ExecBase, port);
	struct TIOPacket *msg;
	TUINT sigs;

	do
	{
		sigs = TExecWait(mod->iodef_ExecBase, portsig | TTASK_SIG_ABORT);
		if (sigs & portsig)
		{
			while ((msg = TExecGetMsg(mod->iodef_ExecBase, port)))
			{
				iodef_docmd(mod, msg);
				TExecReplyMsg(mod->iodef_ExecBase, msg);
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
		TExecPutMsg(mod->iodef_ExecBase,
			TExecGetUserPort(mod->iodef_ExecBase, mod->iodef_Task),
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

