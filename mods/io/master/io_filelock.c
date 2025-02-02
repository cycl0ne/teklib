
/* 
**	$Id: io_filelock.c,v 1.7 2005/09/13 01:12:03 tmueller Exp $
**	filesystem operations
*/

#include "io_mod.h"
#include <tek/mod/exec.h>

/*****************************************************************************/
/* 
**	the internal version of changedir does not track currentdir locks.
**	duplock() depends on changedir(), and new tasks call duplock()
**	to inherit their current directory from their parent. the parent's
**	current directory lock, when duplicated, would end up linked twice
**	into the current directory list, which is fatal.
*/

static TIOMSG *io_changedir_simple(TMOD_IO *io, TIOMSG *newcd)
{
	struct TTask *self = TFindTask(TNULL);
	TIOMSG *oldcd;

	TLock(io->modlock);
	oldcd = self->tsk_CurrentDir;

	if (oldcd) self->tsk_CurrentDir = TNULL;
	
	if (newcd) self->tsk_CurrentDir = newcd;

	TUnlock(io->modlock);

	return oldcd;
}

/*****************************************************************************/
/*
**	outputfh = io_outputfh(io)
**	get current task's output file handle
*/

EXPORT TAPTR io_outputfh(TMOD_IO *io)
{
	struct TTask *self = TFindTask(TNULL);
	if (!self->tsk_FHOut)
	{
		self->tsk_FHOut = io_open(io, "stdio:out", TFMODE_OLDFILE, TNULL);
		if (self->tsk_FHOut)
		{
			self->tsk_Flags |= TTASKF_CLOSEOUTPUT;
			TAddTail(&io->stdfhlist, self->tsk_FHOut);
			tdbprintf(5,"added outfh to stdfhlist\n");
		}
	}
	return self->tsk_FHOut;
}

/*****************************************************************************/
/*
**	inputfh = io_inputfh(io)
**	get current task's input file handle
*/

EXPORT TAPTR io_inputfh(TMOD_IO *io)
{
	struct TTask *self = TFindTask(TNULL);
	if (!self->tsk_FHIn)
	{
		self->tsk_FHIn = io_open(io, "stdio:in", TFMODE_OLDFILE, TNULL);
		if (self->tsk_FHIn)
		{
			self->tsk_Flags |= TTASKF_CLOSEINPUT;
			TAddTail(&io->stdfhlist, self->tsk_FHIn);
			tdbprintf(5,"added infh to stdfhlist\n");
		}
	}
	return self->tsk_FHIn;
}

/*****************************************************************************/
/*
**	errorfh = io_errorfh(io)
**	get current task's error file handle
*/

EXPORT TAPTR io_errorfh(TMOD_IO *io)
{
	struct TTask *self = TFindTask(TNULL);
	if (!self->tsk_FHErr)
	{
		self->tsk_FHErr = io_open(io, "stdio:err", TFMODE_OLDFILE, TNULL);
		if (self->tsk_FHErr)
		{
			self->tsk_Flags |= TTASKF_CLOSEOUTPUT;
			TAddTail(&io->stdfhlist, self->tsk_FHErr);
			tdbprintf(5,"added errorfh to stdfhlist\n");
		}
	}
	return self->tsk_FHErr;
}


/*
**	lock = lockpath(io, lock, relpath)
**	create a new lock relative to a given lock
*/

static TAPTR lockpath(TMOD_IO *io, TIOMSG *lock, TSTRPTR relpath)
{
	TIOMSG *newlock = TNULL;

	if (lock)
	{
		TIOMSG *oldcd = io_changedir_simple(io, lock);
		newlock = io_lock(io, relpath, TFLOCK_READ, TNULL);
		io_changedir_simple(io, oldcd);
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return newlock;
}

/*****************************************************************************/
/*
**	lock = io_duplock(io, lock)
*/

EXPORT TAPTR io_duplock(TMOD_IO *io, TIOMSG *lock)
{
	return lockpath(io, lock, "");
}

/*****************************************************************************/
/*
**	lock = io_parentdir(io, lock)
*/

EXPORT TAPTR io_parentdir(TMOD_IO *io, TIOMSG *lock)
{
	return lockpath(io, lock, "/");
}

/*****************************************************************************/
/*
**	lock = io_lock(io, name, mode, tags)
*/

EXPORT TAPTR io_lock(TMOD_IO *io, TSTRPTR name, TUINT mode, TTAGITEM *tags)
{
	TSTRPTR namepart;
	TIOMSG *iomsg = io_obtainmsg(io, name, &namepart);

	if (iomsg)
	{
		iomsg->io_Req.io_Command = TIOCMD_LOCK;
		iomsg->io_Op.Lock.Name = namepart;
		iomsg->io_Op.Lock.Mode = mode;
		iomsg->io_Op.Lock.Tags = tags;
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Op.Lock.Result)
		{
			io_seterr(io, 0);
			return iomsg;
		}

		io_seterr(io, iomsg->io_Req.io_Error);
		io_releasemsg(io, iomsg);
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	io_unlock(io, lock)
*/

EXPORT TVOID io_unlock(TMOD_IO *io, TIOMSG *iomsg)
{
	if (iomsg)
	{
		iomsg->io_Req.io_Command = TIOCMD_UNLOCK;
		TDoIO((struct TIORequest *) iomsg);		/* error? */
		io_releasemsg(io, iomsg);
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}
}

/*****************************************************************************/
/*
**	lock = io_openfromlock(io, lock)
*/

EXPORT TAPTR io_openfromlock(TMOD_IO *io, TIOMSG *iomsg)
{
	if (iomsg)
	{
		iomsg->io_Req.io_Command = TIOCMD_OPENFROMLOCK;
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Op.OpenFromLock.Result)
		{
			return iomsg;
		}

		io_seterr(io, iomsg->io_Req.io_Error);
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	success = io_examine(io, lock, tags)
*/

EXPORT TINT io_examine(TMOD_IO *io, TIOMSG *iomsg, TTAGITEM *tags)
{
	TINT numattr = -1;

	if (iomsg)
	{
		if (!tags && iomsg->io_Examine) return 0;	/* ok, examined already */

		iomsg->io_Req.io_Command = TIOCMD_EXAMINE;
		iomsg->io_Op.Examine.Tags = tags;
		TDoIO((struct TIORequest *) iomsg);
		
		numattr = iomsg->io_Op.Examine.Result;
		if (numattr == -1)
		{
			io_seterr(io, iomsg->io_Req.io_Error);
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return numattr;
}

/*****************************************************************************/
/*
**	success = io_exnext(io, lock, tags)
*/

EXPORT TINT io_exnext(TMOD_IO *io, TIOMSG *iomsg, TTAGITEM *tags)
{
	TINT numattr = -1;

	if (iomsg)
	{
		/* ensure examination object is available */
		if (io_examine(io, iomsg, TNULL) == 0)
		{
			iomsg->io_Req.io_Command = TIOCMD_EXNEXT;
			iomsg->io_Op.Examine.Tags = tags;
			TDoIO((struct TIORequest *) iomsg);

			numattr = iomsg->io_Op.Examine.Result;
			if (numattr == -1)
			{
				io_seterr(io, iomsg->io_Req.io_Error);
			}
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return numattr;
}

/*****************************************************************************/
/*
**	len = io_nameof(io, lock, buf, buflen)
**
**	get an object's fully qualified path and name. if buf and buflen
**	are zero, determine the length only. the length returned does
**	NOT include an extra byte for the string's trailing zero.
**	if buflen is too small or an error occurs, -1 is returned.
*/

EXPORT TINT io_nameof(TMOD_IO *io, TIOMSG *iomsg, TSTRPTR buf, TINT buflen)
{
	TINT err = TIOERR_BAD_ARGUMENTS;
	if (iomsg && (!buf == !buflen))
	{
		if (!iomsg->io_Req.io_Name)
		{
			tdbprintf(99,"NO NAME FOUND\n");
			tdbfatal(99);

		#if 0
			lock->action = TIOCMD_NAMEOF;
			/* memory object to allocate name from */
			lock->io_Op.nameof.mmu = io->mmu;
			TSendMsg(io->exec, lock->port, lock);
			lock->name = lock->io_Op.nameof.result;
			if (!lock->name) err = lock->ioerr;
		#endif
		}
	
		if (iomsg->io_Req.io_Name)
		{
			TINT len = io_strlen(iomsg->io_Req.io_Name);
	
			if (!buf)
			{
				return len;
			}
			
			if (buflen > len)
			{
				io_strcpy(buf, iomsg->io_Req.io_Name);
				return len;
			}
			
			err = TIOERR_LINE_TOO_LONG;
		}
	}

	io_seterr(io, err);
	return -1;
}

/*****************************************************************************/
/*
**	oldlock = io_changedir(io, newlock)
**
**	make newlock the new current directory. the old current
**	directory lock is returned. note:
**	- from the user's point of view, newlock is relinquished.
**	  it will be under system control.
**	- in return, the user becomes responsible for oldlock.
*/

EXPORT TIOMSG *io_changedir(TMOD_IO *io, TIOMSG *newcd)
{
	struct TTask *self = TFindTask(TNULL);
	TIOMSG *oldcd;

	TLock(io->modlock);
	oldcd = self->tsk_CurrentDir;

	if (oldcd)
	{
		tdbprintf2(2,"currentdir in thread %08x removed: %08x\n",
			(TUINT)(TUINTPTR) self, (TUINT)(TUINTPTR) oldcd);
		TRemove((TNODE *) oldcd);
		self->tsk_CurrentDir = TNULL;
	}
	
	if (newcd)
	{
		TAddTail(&io->cdlocklist, (TNODE *) newcd);
		self->tsk_CurrentDir = newcd;
		tdbprintf2(2,"new currentdir in thread %08x set: %08x\n",
			(TUINT)(TUINTPTR) self, (TUINT)(TUINTPTR) newcd);
	}

	TUnlock(io->modlock);

	return oldcd;
}

/*****************************************************************************/
/*
**	success = io_rename(io, oldname, newname)
**	rename an object
*/

EXPORT TBOOL io_rename(TMOD_IO *io, TSTRPTR oldname, TSTRPTR newname)
{
	TBOOL success = TFALSE;
	TSTRPTR srcname, dstname;
	TIOMSG *srcmsg, *dstmsg;

	srcmsg = io_obtainmsg(io, oldname, &srcname);
	if (srcmsg)
	{
		dstmsg = io_obtainmsg(io, newname, &dstname);
		if (dstmsg)
		{
			/*	if (srcmsg->io_Req.io_Unit == dstmsg->io_Req.io_Unit)
				we don't have "units" anymore; device instances ARE what make
				them unique! */

			if (srcmsg->io_Req.io_Device == dstmsg->io_Req.io_Device)
			{
				srcmsg->io_Req.io_Command = TIOCMD_RENAME;
				srcmsg->io_Op.Rename.SrcName = srcname;
				srcmsg->io_Op.Rename.DstName = dstname;

				TDoIO((struct TIORequest *) srcmsg);
				if (srcmsg->io_Op.Rename.Result)
				{
					success = TTRUE;
				}
				else
				{
					io_seterr(io, srcmsg->io_Req.io_Error);
				}
			}
			else
			{
				io_seterr(io, TIOERR_NOT_SAME_DEVICE);
			}

			io_releasemsg(io, dstmsg);
		}
		io_releasemsg(io, srcmsg);
	}

	return success;
}

/*****************************************************************************/
/*
**	lock = io_makedir(io, name, tags)
**	create a directory, return a shared lock on it.
*/

EXPORT TAPTR io_makedir(TMOD_IO *io, TSTRPTR name, TTAGITEM *tags)
{
	TSTRPTR namepart;
	TIOMSG *iomsg;
	
	iomsg = io_obtainmsg(io, name, &namepart);
	if (iomsg)
	{
		iomsg->io_Req.io_Command = TIOCMD_MAKEDIR;
		iomsg->io_Op.Lock.Name = namepart;
		iomsg->io_Op.Lock.Mode = TFLOCK_READ;
		iomsg->io_Op.Lock.Tags = tags;
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Op.Lock.Result)
		{
			return iomsg;
		}
		io_seterr(io, iomsg->io_Req.io_Error);
		io_releasemsg(io, iomsg);
	}
	
	return TNULL;
}

/*****************************************************************************/
/*
**	success = io_delete(io, name)
**	delete file or directory. note: by definition, directories
**	must be empty before they can be deleted. the handler is
**	expected to take care of this.
*/

EXPORT TBOOL io_delete(TMOD_IO *io, TSTRPTR name)
{
	TSTRPTR namepart;
	TIOMSG *iomsg;
	TBOOL success = TFALSE;
	
	iomsg = io_obtainmsg(io, name, &namepart);
	if (iomsg)
	{
		iomsg->io_Req.io_Command = TIOCMD_DELETE;
		iomsg->io_Op.Delete.Name = namepart;
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Op.Delete.Result)
		{
			success = TTRUE;
		}
		else
		{
			io_seterr(io, iomsg->io_Req.io_Error);
		}

		io_releasemsg(io, iomsg);
	}

	return success;
}

/*****************************************************************************/
/*
**	success = io_setdate(io, name, date)
**	create a directory, return a shared lock on it.
*/

EXPORT TBOOL io_setdate(TMOD_IO *io, TSTRPTR name, TDATE *date)
{
	TSTRPTR namepart;
	TIOMSG *iomsg;
	TBOOL success = TFALSE;
	
	iomsg = io_obtainmsg(io, name, &namepart);
	if (iomsg)
	{
		iomsg->io_Req.io_Command = TIOCMD_SETDATE;
		iomsg->io_Op.SetDate.Name = namepart;
		iomsg->io_Op.SetDate.Date = date;
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Op.SetDate.Result)
		{
			success = TTRUE;
		}
		else
		{
			io_seterr(io, iomsg->io_Req.io_Error);
		}
		io_releasemsg(io, iomsg);
	}

	return success;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: io_filelock.c,v $
**	Revision 1.7  2005/09/13 01:12:03  tmueller
**	commentary corrected
**	
**	Revision 1.6  2005/09/08 00:01:37  tmueller
**	mod API extended; cleanup; util dependency removed
**	
**	Revision 1.5  2004/07/03 03:29:54  tmueller
**	added io_setdate()
**	
**	Revision 1.4  2004/06/27 22:29:42  tmueller
**	io_lock() and io_open() did not set ioerr correctly
**	
**	Revision 1.3  2004/04/18 14:11:42  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.2  2003/12/22 22:55:19  tmueller
**	Renamed field names in I/O packets to uppercase
**	
**	Revision 1.1.1.1  2003/12/11 07:18:40  tmueller
**	Krypton import
**	
**	Revision 1.5  2003/10/16 17:38:57  tmueller
**	Applied changed exec structure names. Minor optimization in name resolution
**	
**	Revision 1.4  2003/08/29 22:02:18  tmueller
**	Serious cyclic dependency resolved between io_changedir() and io_duplock(),
**	when a child task is created and inherits its current directory from its
**	parent; A new internal changedir() function is used for duplock(), that
**	does not track currentdir locks in the I/O module's cdlocklist.
**	
**	Revision 1.3  2003/07/11 19:36:14  tmueller
**	added io_fgets, io_fputs, io_fungetc
**	
**	Revision 1.2  2003/05/11 14:11:34  tmueller
**	Updated I/O master and default implementations to extended definitions
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.10  2003/03/04 02:34:58  bifat
**	The return value of io_examine() and io_exnext() changed.
**	
**	Revision 1.9  2003/03/03 02:22:30  bifat
**	io_makedir() now returns a shared lock on the created directory
**	
**	Revision 1.8  2003/03/02 15:47:02  bifat
**	Some packet types extended with additional tag arguments. io_addpart() API
**	changed. io_lock(), io_open(), io_makedir() and io_fault() got additional
**	taglist arguments. Added TIOERR_LINE_TOO_LONG error code.
**	
**	Revision 1.7  2003/03/01 21:22:24  bifat
**	Massive cleanup and simplification of error codes. Now fits very well to
**	the different platforms.
**	
**	Revision 1.6  2003/03/01 05:25:14  bifat
**	POSIX implementation of the default IO handler now works with Elate, too
**	
**	Revision 1.5  2003/03/01 04:46:47  bifat
**	minor changes
**	
**	Revision 1.4  2003/02/09 13:53:20  tmueller
**	added io_outputfh and io_errorfh, added automatic closedown of stdio
**	filehandles
**	
**	Revision 1.3  2003/02/09 13:28:32  tmueller
**	added io_outputfh(), added stdin/stdout/stderr fields to exec tasks,
**	cleaned up child task inheritance of current directory, added task tags
**	TTask_OutputFH etc. for passing stdio filehandles to newly created tasks.
**	
**	Revision 1.2  2003/02/02 23:08:43  tmueller
**	Locks no longer have a field with a backpointer to a task that "owns" them
**	as a currentdir. That doesn't work if locks are passed around across tasks,
**	and it wasn't very useful either.
**	
**	Revision 1.1  2003/01/20 21:29:56  tmueller
**	added
**	
*/
