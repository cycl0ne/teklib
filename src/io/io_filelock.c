
/*
**	$Id: io_filelock.c,v 1.2 2006/09/10 14:40:37 tmueller Exp $
**	teklib/src/io/io_filelock.c - operations based on filesystem locks
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

static struct TIOPacket *io_changedir_simple(TMOD_IO *io,
	struct TIOPacket *newcd)
{
	TAPTR exec = TGetExecBase(io);
	struct TTask *self = TExecFindTask(exec, TNULL);
	struct TIOPacket *oldcd;

	TExecLock(exec, io->modlock);
	oldcd = self->tsk_CurrentDir;

	if (oldcd)
		self->tsk_CurrentDir = TNULL;

	if (newcd)
		self->tsk_CurrentDir = newcd;

	TExecUnlock(exec, io->modlock);

	return oldcd;
}

/*****************************************************************************/
/*
**	outputfh = io_outputfh(io)
**	get current task's output file handle
*/

EXPORT TAPTR io_OutputFH(TMOD_IO *io)
{
	TAPTR exec = TGetExecBase(io);
	struct TTask *self = TExecFindTask(exec, TNULL);
	if (!self->tsk_FHOut)
	{
		self->tsk_FHOut = io_OpenFile(io, "stdio:out", TFMODE_OLDFILE, TNULL);
		if (self->tsk_FHOut)
		{
			self->tsk_Flags |= TTASKF_CLOSEOUTPUT;
			TAddTail(&io->stdfhlist, self->tsk_FHOut);
			TDBPRINTF(3,("added outfh to stdfhlist\n"));
		}
	}
	return self->tsk_FHOut;
}

/*****************************************************************************/
/*
**	inputfh = io_inputfh(io)
**	get current task's input file handle
*/

EXPORT TAPTR io_InputFH(TMOD_IO *io)
{
	TAPTR exec = TGetExecBase(io);
	struct TTask *self = TExecFindTask(exec, TNULL);
	if (!self->tsk_FHIn)
	{
		self->tsk_FHIn = io_OpenFile(io, "stdio:in", TFMODE_OLDFILE, TNULL);
		if (self->tsk_FHIn)
		{
			self->tsk_Flags |= TTASKF_CLOSEINPUT;
			TAddTail(&io->stdfhlist, self->tsk_FHIn);
			TDBPRINTF(3,("added infh to stdfhlist\n"));
		}
	}
	return self->tsk_FHIn;
}

/*****************************************************************************/
/*
**	errorfh = io_errorfh(io)
**	get current task's error file handle
*/

EXPORT TAPTR io_ErrorFH(TMOD_IO *io)
{
	TAPTR exec = TGetExecBase(io);
	struct TTask *self = TExecFindTask(exec, TNULL);
	if (!self->tsk_FHErr)
	{
		self->tsk_FHErr = io_OpenFile(io, "stdio:err", TFMODE_OLDFILE, TNULL);
		if (self->tsk_FHErr)
		{
			self->tsk_Flags |= TTASKF_CLOSEOUTPUT;
			TAddTail(&io->stdfhlist, self->tsk_FHErr);
			TDBPRINTF(3,("added errorfh to stdfhlist\n"));
		}
	}
	return self->tsk_FHErr;
}


/*
**	lock = lockpath(io, lock, relpath)
**	create a new lock relative to a given lock
*/

static TAPTR lockpath(TMOD_IO *io, struct TIOPacket *lock, TSTRPTR relpath)
{
	struct TIOPacket *newlock = TNULL;

	if (lock)
	{
		struct TIOPacket *oldcd = io_changedir_simple(io, lock);
		newlock = io_LockFile(io, relpath, TFLOCK_READ, TNULL);
		io_changedir_simple(io, oldcd);
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return newlock;
}

/*****************************************************************************/
/*
**	lock = io_duplock(io, lock)
*/

EXPORT TAPTR io_DupLock(TMOD_IO *io, struct TIOPacket *lock)
{
	return lockpath(io, lock, "");
}

/*****************************************************************************/
/*
**	lock = io_parentdir(io, lock)
*/

EXPORT TAPTR io_ParentDir(TMOD_IO *io, struct TIOPacket *lock)
{
	return lockpath(io, lock, "/");
}

/*****************************************************************************/
/*
**	lock = io_lock(io, name, mode, tags)
*/

EXPORT TAPTR io_LockFile(TMOD_IO *io, TSTRPTR name, TUINT mode, TTAGITEM *tags)
{
	TSTRPTR namepart;
	struct TIOPacket *iomsg = io_ObtainMsg(io, name, &namepart);

	if (iomsg)
	{
		TAPTR exec = TGetExecBase(io);
		TSTRPTR *namepp = (TSTRPTR *) TGetTag(tags, TIOLock_NamePart, TNULL);
		if (namepp)
			*namepp = namepart;

		iomsg->io_Req.io_Command = TIOCMD_LOCK;
		iomsg->io_Op.Lock.Name = namepart;
		iomsg->io_Op.Lock.Mode = mode;
		iomsg->io_Op.Lock.Tags = tags;
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Op.Lock.Result)
		{
			io_SetIOErr(io, 0);
			return iomsg;
		}

		io_SetIOErr(io, iomsg->io_Req.io_Error);
		io_ReleaseMsg(io, iomsg);
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	io_unlock(io, lock)
*/

EXPORT void io_UnlockFile(TMOD_IO *io, struct TIOPacket *iomsg)
{
	if (iomsg)
	{
		TAPTR exec = TGetExecBase(io);
		iomsg->io_Req.io_Command = TIOCMD_UNLOCK;
		TExecDoIO(exec, (struct TIORequest *) iomsg); /* error? */
		io_ReleaseMsg(io, iomsg);
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);
}

/*****************************************************************************/
/*
**	lock = io_openfromlock(io, lock)
*/

EXPORT TAPTR io_OpenFromLock(TMOD_IO *io, struct TIOPacket *iomsg)
{
	if (iomsg)
	{
		TAPTR exec = TGetExecBase(io);
		iomsg->io_Req.io_Command = TIOCMD_OPENFROMLOCK;
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Op.OpenFromLock.Result)
			return iomsg;
		io_SetIOErr(io, iomsg->io_Req.io_Error);
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return TNULL;
}

/*****************************************************************************/
/*
**	success = io_examine(io, lock, tags)
*/

EXPORT TINT io_Examine(TMOD_IO *io, struct TIOPacket *iomsg, TTAGITEM *tags)
{
	TINT numattr = -1;

	if (iomsg)
	{
		TAPTR exec;

		if (!tags && iomsg->io_Examine)
			return 0; /* ok, examined already */

		iomsg->io_Req.io_Command = TIOCMD_EXAMINE;
		iomsg->io_Op.Examine.Tags = tags;
		exec = TGetExecBase(io);
		TExecDoIO(exec, (struct TIORequest *) iomsg);

		numattr = iomsg->io_Op.Examine.Result;
		if (numattr == -1)
			io_SetIOErr(io, iomsg->io_Req.io_Error);
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return numattr;
}

/*****************************************************************************/
/*
**	success = io_exnext(io, lock, tags)
*/

EXPORT TINT io_ExNext(TMOD_IO *io, struct TIOPacket *iomsg, TTAGITEM *tags)
{
	TINT numattr = -1;

	if (iomsg)
	{
		/* ensure examination object is available */
		if (io_Examine(io, iomsg, TNULL) == 0)
		{
			TAPTR exec = TGetExecBase(io);
			iomsg->io_Req.io_Command = TIOCMD_EXNEXT;
			iomsg->io_Op.Examine.Tags = tags;
			TExecDoIO(exec, (struct TIORequest *) iomsg);
			numattr = iomsg->io_Op.Examine.Result;
			if (numattr == -1)
				io_SetIOErr(io, iomsg->io_Req.io_Error);
		}
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

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

EXPORT TINT io_NameOf(TMOD_IO *io, struct TIOPacket *iomsg, TSTRPTR buf,
	TINT buflen)
{
	TINT err = TIOERR_BAD_ARGUMENTS;
	if (iomsg && (!buf == !buflen))
	{
		if (!iomsg->io_Req.io_Name)
		{
			TDBPRINTF(99,("no name found\n"));
			TDBFATAL();
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

	io_SetIOErr(io, err);
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

EXPORT struct TIOPacket *io_ChangeDir(TMOD_IO *io, struct TIOPacket *newcd)
{
	TAPTR exec = TGetExecBase(io);
	struct TTask *self = TExecFindTask(exec, TNULL);
	struct TIOPacket *oldcd;

	TExecLock(exec, io->modlock);
	oldcd = self->tsk_CurrentDir;

	if (oldcd)
	{
		TDBPRINTF(2,("currentdir in thread %08x removed: %08x\n",
			(TUINT)(TUINTPTR) self, (TUINT)(TUINTPTR) oldcd));
		TRemove((struct TNode *) oldcd);
		self->tsk_CurrentDir = TNULL;
	}

	if (newcd)
	{
		TAddTail(&io->cdlocklist, (struct TNode *) newcd);
		self->tsk_CurrentDir = newcd;
		TDBPRINTF(2,("new currentdir in thread %08x set: %08x\n",
			(TUINT)(TUINTPTR) self, (TUINT)(TUINTPTR) newcd));
	}

	TExecUnlock(exec, io->modlock);

	return oldcd;
}

/*****************************************************************************/
/*
**	success = io_rename(io, oldname, newname)
**	rename an object
*/

EXPORT TBOOL io_Rename(TMOD_IO *io, TSTRPTR oldname, TSTRPTR newname)
{
	TBOOL success = TFALSE;
	TSTRPTR srcname, dstname;
	struct TIOPacket *srcmsg, *dstmsg;

	srcmsg = io_ObtainMsg(io, oldname, &srcname);
	if (srcmsg)
	{
		dstmsg = io_ObtainMsg(io, newname, &dstname);
		if (dstmsg)
		{
			/*	if (srcmsg->io_Req.io_Unit == dstmsg->io_Req.io_Unit)
				we don't have "units" anymore; device instances ARE what make
				them unique! */

			if (srcmsg->io_Req.io_Device == dstmsg->io_Req.io_Device)
			{
				TAPTR exec = TGetExecBase(io);

				srcmsg->io_Req.io_Command = TIOCMD_RENAME;
				srcmsg->io_Op.Rename.SrcName = srcname;
				srcmsg->io_Op.Rename.DstName = dstname;

				TExecDoIO(exec, (struct TIORequest *) srcmsg);
				if (srcmsg->io_Op.Rename.Result)
					success = TTRUE;
				else
					io_SetIOErr(io, srcmsg->io_Req.io_Error);
			}
			else
				io_SetIOErr(io, TIOERR_NOT_SAME_DEVICE);

			io_ReleaseMsg(io, dstmsg);
		}
		io_ReleaseMsg(io, srcmsg);
	}

	return success;
}

/*****************************************************************************/
/*
**	lock = io_makedir(io, name, tags)
**	create a directory, return a shared lock on it.
*/

EXPORT TAPTR io_MakeDir(TMOD_IO *io, TSTRPTR name, TTAGITEM *tags)
{
	TSTRPTR namepart;
	struct TIOPacket *iomsg;

	iomsg = io_ObtainMsg(io, name, &namepart);
	if (iomsg)
	{
		TAPTR exec = TGetExecBase(io);
		iomsg->io_Req.io_Command = TIOCMD_MAKEDIR;
		iomsg->io_Op.Lock.Name = namepart;
		iomsg->io_Op.Lock.Mode = TFLOCK_READ;
		iomsg->io_Op.Lock.Tags = tags;
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Op.Lock.Result)
			return iomsg;
		io_SetIOErr(io, iomsg->io_Req.io_Error);
		io_ReleaseMsg(io, iomsg);
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

EXPORT TBOOL io_Delete(TMOD_IO *io, TSTRPTR name)
{
	TSTRPTR namepart;
	struct TIOPacket *iomsg;
	TBOOL success = TFALSE;

	iomsg = io_ObtainMsg(io, name, &namepart);
	if (iomsg)
	{
		TAPTR exec = TGetExecBase(io);
		iomsg->io_Req.io_Command = TIOCMD_DELETE;
		iomsg->io_Op.Delete.Name = namepart;
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Op.Delete.Result)
			success = TTRUE;
		else
			io_SetIOErr(io, iomsg->io_Req.io_Error);

		io_ReleaseMsg(io, iomsg);
	}

	return success;
}

/*****************************************************************************/
/*
**	success = io_setdate(io, name, date, tags)
*/

EXPORT TBOOL io_SetDate(TMOD_IO *io, TSTRPTR name, TDATE *date, TTAGITEM *tags)
{
	TSTRPTR namepart;
	struct TIOPacket *iomsg;
	TBOOL success = TFALSE;

	iomsg = io_ObtainMsg(io, name, &namepart);
	if (iomsg)
	{
		TAPTR exec = TGetExecBase(io);
		iomsg->io_Req.io_Command = TIOCMD_SETDATE;
		iomsg->io_Op.SetDate.Name = namepart;
		iomsg->io_Op.SetDate.Date = date;
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Op.SetDate.Result)
			success = TTRUE;
		else
			io_SetIOErr(io, iomsg->io_Req.io_Error);
		io_ReleaseMsg(io, iomsg);
	}

	return success;
}

