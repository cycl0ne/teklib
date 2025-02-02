
/*
**	$Id: io_names.c,v 1.6 2006/11/11 14:19:09 tmueller Exp $
**	teklib/src/io/io_names.c - Io namespace management
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "io_mod.h"
#include <tek/mod/exec.h>

/*****************************************************************************/
/*
**	len = addpathpart(path1, path2, destbuf_end)
**
**	taking into account all occuring path delimiters, add path2 to path1,
**	render the result to destbuf. if destbuf is NULL, calculate the length
**	only. destbuf_end must point to the end of the buffer, i.e. to where
**	the trailing zero byte would be expected. an error is indicated with
**	a return value of -1.
*/

static TINT
io_addpathpart(TSTRPTR p1, TSTRPTR p2, TSTRPTR dp)
{
	TINT len = 0, eatc = 0, slc = 0, l1, c;
	TSTRPTR p;

	p = p1;
	while (*p) p++;
	l1 = p - p1;

	p = p2;
	while (*p) p++;

	while (p != p1)
	{
		if (p == p2)
		{
			/* start of second path reached, continue at end of first */

			if (!l1)
				break;

			p = p1 + l1;
			if (p[-1] != '/' && p[-1] != ':')
			{
				/* need a slash between them */

				/*c = '/';*/
				slc++;
				continue;
			}
		}

		c = *--p;

		if (c == '/')
			slc++;
		else if (c == ':')
		{
			if (eatc > 1)
				slc += eatc - 1;

			while (slc)
			{
				len++;
				if (dp)
					*--dp = '/';
				slc--;
			}

			len++;
			if (dp)
				*--dp = c;

			eatc = 0;
			slc = 0;
		}
		else
		{
			if (slc > 0)
			{
				if (eatc > 0)
					eatc--;
				if (slc > 1)
					eatc += slc - 1;
				if (!eatc)
				{
					if (len)
					{
						len++;
						if (dp)
							*--dp = '/';
					}
				}
				slc = 0;
			}

			if (!eatc)
			{
				len++;
				if (dp)
					*--dp = c;
			}
		}
	}

	if (eatc > 1)
		return -1; /* unresolved eat count, error */

	while (slc)
	{
		len++;
		if (dp)
			*--dp = '/';
		slc--;
	}

	return len;
}


/*****************************************************************************/
/*
**	len = io_addpart(io, path1, path2, buf, buflen)
**
**	add path2 to path1, taking into account all kinds of path
**	delimiters, render the resulting path to buf. if buf and buflen
**	are zero, determine the length only. the length returned does
**	NOT include the extra byte for the string's trailing zero.
**	if buflen is too small or an error occurs, -1 is returned.
*/

EXPORT TINT
io_AddPart(TMOD_IO *io, TSTRPTR path1, TSTRPTR path2, TSTRPTR buf, TINT buflen)
{
	TINT rlen = -1;
	TINT err = TIOERR_BAD_ARGUMENTS;
	if (!buf == !buflen)
	{
		err = TIOERR_INVALID_NAME;
		if (path2)
		{
			TSTRPTR temp = io_strchr(path2, ':');
			if (temp)
			{
				if (!io_strchr(temp + 1, ':'))
				{
					err = 0;
					rlen = io_strlen(path2);
					if (buf)
					{
						if (rlen < buflen)
							io_strcpy(buf, path2);
						else
							err = TIOERR_LINE_TOO_LONG;
					}
				}
			}
			else if (path1)
			{
				rlen = io_addpathpart(path1, path2, TNULL);
				if (rlen >= 0)
				{
					err = 0;
					if (buf)
					{
						if (rlen < buflen)
						{
							io_addpathpart(path1, path2, buf + rlen);
							buf[rlen] = 0;
						}
						else
							err = TIOERR_LINE_TOO_LONG;
					}
				}
			}
		}
	}
	io_SetIOErr(io, err);
	return rlen;
}

/*
**	fullpath = addpart(io, path1, path2)
**	internal version
**
**	add path2 to path1, taking into account all kinds of path
**	delimiters. returns a new path, or TNULL for failure. the
**	resulting string must be freed using TFree().
*/

static TSTRPTR
io_addpart(TMOD_IO *io, TSTRPTR path1, TSTRPTR path2)
{
	TSTRPTR newp = TNULL;
	TINT err = TIOERR_INVALID_NAME;

	if (path2)
	{
		TSTRPTR temp = io_strchr(path2, ':');
		if (temp)
		{
			if (!io_strchr(temp + 1, ':'))
			{
				newp = io_strdup(io, io->mmu, path2);
				if (!newp)
					err = TIOERR_NOT_ENOUGH_MEMORY;
			}
		}
		else if (path1)
		{
			TINT len = io_addpathpart(path1, path2, TNULL);
			if (len >= 0)
			{
				TAPTR exec = TGetExecBase(io);
				newp = TExecAlloc(exec, io->mmu, len + 1);
				if (newp)
				{
					io_addpathpart(path1, path2, newp + len);
					newp[len] = 0;
					err = 0;
				}
				else
					err = TIOERR_NOT_ENOUGH_MEMORY;
			}
		}
	}

	if (err)
		io_SetIOErr(io, err);

	return newp;
}

/*
**	devnode = finddevice(io, devname, namelen)
**	find a named device. note: the caller must
**	be owner of io->devicelist
*/

static struct TDeviceNode *
io_finddevice(TMOD_IO *io, TSTRPTR name, TINT nlen)
{
	struct TNode *nextnode, *node = io->devicelist.tlh_Head;
	while ((nextnode = node->tln_Succ))
	{
		struct TDeviceNode *dn = (struct TDeviceNode *) node;
		if (io_strncasecmp(dn->name, name, nlen) == 0 &&
			dn->name[nlen] == 0)
			return (struct TDeviceNode *) node;
		node = nextnode;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	iomsg = openhandler(io, name, namelen, version, tags)
**	open a handler. if name or namelen is NULL, attempt
**	to open the default handler.
*/

static struct TIOPacket *
io_openhandler(TMOD_IO *io, TSTRPTR name, TINT nlen, TUINT16 version,
	TTAGITEM *usertags)
{
	TAPTR exec = TGetExecBase(io);
	struct TIOPacket *iomsg = TNULL;
	TSTRPTR temp;

	if (!name || !nlen)
	{
		/* get default device */
		name = IOHNDNAME_DEFAULT;
		nlen = IOHNDNAME_DEFLEN;
	}

	temp = TExecAlloc(exec, io->mmu, nlen + IOHNDNAME_LEN + 1);
	if (temp)
	{
		TTAGITEM tags[2];
		TSTRPTR d;
		TINT c;

		tags[0].tti_Tag = TIOMount_IOBase; /* pass ptr to IOBase in Taglist */
		tags[0].tti_Value = (TTAG) io;
		tags[1].tti_Tag = TTAG_MORE;
		tags[1].tti_Value = (TTAG) usertags;

		io_strcpy(temp, IOHNDNAME);

		d = temp + IOHNDNAME_LEN;
		while (nlen-- && (c = *name++))
		{
			if (c >= 'A' && c <='Z') c += 'a'-'A';
			*d++ = c;
		}
		*d = 0;

		TDBPRINTF(4,("opening I/O handler %s\n", temp));

		iomsg = (struct TIOPacket *)
			TExecOpenModule(exec, temp, version, tags);
		if (!iomsg)
			io_SetIOErr(io, TIOERR_DEVICE_OPEN_FAILED);

		TExecFree(exec, temp);
	}
	else
		io_SetIOErr(io, TIOERR_NOT_ENOUGH_MEMORY);

	return iomsg;
}

/*****************************************************************************/
/*
**	iomsg = io_refhandler(io, iomsg)
**	get an additional reference to a handler;
**	returns another I/O message
*/

LOCAL struct TIOPacket *
io_refhandler(TMOD_IO *io, struct TIOPacket *iomsg)
{
	TAPTR exec = TGetExecBase(io);
	struct TIOPacket *newmsg = (struct TIOPacket *) TExecOpenModule(exec,
		iomsg->io_Req.io_Device->tmd_Handle.thn_Name, 0, TNULL);
	if (!newmsg)
		io_SetIOErr(io, TIOERR_DEVICE_OPEN_FAILED);
	return newmsg;
}

/*****************************************************************************/
/*
**	io_releasemsg(io, iomsg)
**	free I/O message and unreference handler
*/

EXPORT void
io_ReleaseMsg(TMOD_IO *io, struct TIOPacket *iomsg)
{
	TAPTR exec = TGetExecBase(io);
	TExecCloseModule(exec, iomsg->io_Req.io_Device);
	TExecFree(exec, iomsg->io_Req.io_Name);
	TExecFree(exec, iomsg->io_Buffer);
	TExecFree(exec, iomsg);
}

/*****************************************************************************/
/*
**	iomsg = io_obtainmsg(io, name, namepart)
**
**	resolve the supplied name and obtain an I/O message from the
**	corresponding handler. a pointer to the handler-independent part
**	of the name will be returned as well.
**
**	"foo:bar" will try to obtain the iomsg from the "foo" handler and
**	return "bar" as the namepart.
**
**	"foo/bar" will try to compose the full name from the current task's
**	current directory. if present, the resulting I/O message will be
**	obtained from the handler on which the current directory resides,
**	and the name part would result in e.g. "home/peter/foo/bar". if no
**	lock on a current directory is present in the current task context,
**	a lock to the current directory will be attempted from the default
**	handler first.
*/

static struct TIOPacket *
io_obtainhandler(TMOD_IO *io, struct TTask *self, TSTRPTR name,
	TSTRPTR *namepartp)
{
	TAPTR exec = TGetExecBase(io);
	struct TIOPacket *iomsg = TNULL;
	TSTRPTR temp;
	TINT nlen = 0;

	temp = io_strchr(name, ':');
	if (temp)
	{
		nlen = temp - name;	/* length of device part */
		name = io_addpart(io, name, ""); /* check and compress path/name */
		if (name)
		{
			struct TDeviceNode *devnode;
			devnode = io_finddevice(io, name, nlen); /* lookup in devlist */
			if (devnode)
			{
				if (devnode->flags & TDEVF_ASSIGN)
				{
					if (!devnode->lock)
					{
						/* try to resolve late-binding assign */

						/* this is recursive, so be careful */
						TRemove((struct TNode *) devnode);
						devnode->lock = io_LockFile(io, devnode->altname,
							TFLOCK_READ, TNULL);
						TAddHead(&io->devicelist, (struct TNode *) devnode);
					}

					if (devnode->lock)
					{
						devnode->flags &= ~TDEVF_DEFER;
						if (io_NameOf(io, devnode->lock, TNULL, 0))
						{
							TSTRPTR fullname = io_addpart(io,
								devnode->lock->io_Req.io_Name,
									name + nlen + 1);
							if (fullname)
							{
								TExecFree(exec, name);
								name = fullname;
								iomsg = io_refhandler(io, devnode->lock);
							}
						}
					}
				}
				else if (devnode->flags & TDEVF_MOUNT)
				{
					/* have a handler iomsg, i.e. a "root lock" */
					iomsg = io_refhandler(io, devnode->lock);
				}
			}
			else
			{
				/*	no matching entry. address a handler of that name directly.
				**	this allows fully abstract devices to mount themselves on
				**	demand; e.g. "foo:bar" will try to open a "foo" handler. */

				iomsg = io_openhandler(io, name, nlen, 0, TNULL);
			}

			if (iomsg)
			{
				iomsg->io_Req.io_Name = name;
				if (namepartp) *namepartp =
					io_strchr(name, ':') + 1;
				return iomsg;
			}

			TExecFree(exec, name);
		}
	}
	else
	{
		/* name is relative... */

		struct TIOPacket *currentdir = self->tsk_CurrentDir;
		TSTRPTR namepart = name;	/* save name */

		if (currentdir)
		{
			/*	... to the handler on which currentdir resides. */
			name = currentdir->io_Req.io_Device->tmd_Handle.thn_Name
				+ IOHNDNAME_LEN;
			nlen = io_strlen(name);
		}
		/* else we try to address the default handler */

		iomsg = io_openhandler(io, name, nlen, 0, TNULL);
		if (iomsg)
		{
			if (!currentdir)
			{
				/*
				**	this is a special convention for self-initialization.
				**	as long as we are started in a host operating system, only
				**	an initially addressed default handler can tell where we
				**	reside in the host file system.
				*/

				currentdir = io_LockFile(io, "CURRENTDIR:",
					TFLOCK_READ, TNULL);
				if (currentdir)
				{
					/*	link new currentdir to module's currentdir
					**	lock maintenance list */

					self->tsk_CurrentDir = currentdir;
					TDBPRINTF(2,("adding new lock to cdlocklist\n"));
					TAddTail(&io->cdlocklist, (struct TNode *) currentdir);
				}
			}

			if (currentdir)
			{
				if (io_NameOf(io, currentdir, TNULL, 0))
				{
					iomsg->io_Req.io_Name =
						io_addpart(io, currentdir->io_Req.io_Name, namepart);
					if (iomsg->io_Req.io_Name)
					{
						if (namepartp)
							*namepartp =
								io_strchr(iomsg->io_Req.io_Name, ':') + 1;
						return iomsg;
					}
				}
			}

			io_ReleaseMsg(io, iomsg);
		}
	}

	return TNULL;
}

EXPORT struct TIOPacket *
io_ObtainMsg(TMOD_IO *io, TSTRPTR name, TSTRPTR *namepartp)
{
	struct TIOPacket *iomsg = TNULL;
	if (name)
	{
		TAPTR exec = TGetExecBase(io);
		struct TTask *self = TExecFindTask(exec, TNULL);
		struct TIOPacket *initlock = TNULL;

		TExecLock(exec, io->modlock);

		/*	when these logical devices are requested, make sure that
		**	there is (or has been) a lock to the current directory.
		**	this will guarantee that the default handler is open, and
		**	that it had the opportunity to add them as late-binding
		**	assigns during initialization. */

		if (!self->tsk_CurrentDir &&
			(!io_strncasecmp("SYS:", name, 4) ||
			!io_strncasecmp("T:", name, 2) ||
			!io_strncasecmp("PROGDIR:", name, 8) ||
			!io_strncasecmp("ENVARC:", name, 7)))
			initlock = io_LockFile(io, "", TFLOCK_READ, TNULL);

		iomsg = io_obtainhandler(io, self, name, namepartp);

		if (initlock)
			io_UnlockFile(io, initlock);

		TExecUnlock(exec, io->modlock);
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return iomsg;
}

/*****************************************************************************/
/*
**	success = io_assignlate(io, name, path)
**	add a late-binding assign to the devicelist.
**	name must be without trailing ":".
*/

EXPORT TBOOL
io_AssignLate(TMOD_IO *io, TSTRPTR name, TSTRPTR path)
{
	TINT err = TIOERR_BAD_ARGUMENTS;

	if (name && path)
	{
		TAPTR exec = TGetExecBase(io);
		struct TDeviceNode *dn;
		TINT len1 = io_strlen(name);
		TINT len2 = io_strlen(path);

		TExecLock(exec, io->modlock);

		dn = io_finddevice(io, name, len1);
		if (!dn)
		{
			dn = TExecAlloc(exec, io->mmu,
				sizeof(struct TDeviceNode) + len1 + len2 + 2);
			if (dn)
			{
				dn->name = (TSTRPTR) (dn + 1);
				io_strcpy(dn->name, name);

				dn->altname = dn->name + len1 + 1;
				io_strcpy(dn->altname, path);

				dn->flags = TDEVF_ASSIGN | TDEVF_DEFER;
				dn->lock = TNULL;
				dn->device = TNULL;

				TAddTail(&io->devicelist, (struct TNode *) dn);

				err = 0;
			}
			else
				err = TIOERR_NOT_ENOUGH_MEMORY;
		}
		else
			err = TIOERR_OBJECT_EXISTS;

		TExecUnlock(exec, io->modlock);
	}

	if (err == 0)
		return TTRUE;
	io_SetIOErr(io, err);
	return TFALSE;
}

/*****************************************************************************/
/*
**	success = io_assignlock(io, name, lock)
**	add a lock as an assign to the devicelist. name must be
**	without trailing ":". passing TNULL as the lock cancels
**	outstanding assigns to the name. if successful, lock is
**	under system control and relinquished from the caller's
**	point of view
*/

EXPORT TBOOL
io_AssignLock(TMOD_IO *io, TSTRPTR name, struct TIOPacket *lock)
{
	TINT err = TIOERR_BAD_ARGUMENTS;

	if (name)
	{
		TAPTR exec = TGetExecBase(io);
		struct TDeviceNode *dn;
		TINT len = io_strlen(name);

		TExecLock(exec, io->modlock);

		dn = io_finddevice(io, name, len);
		if (!dn && lock)
		{
			dn = TExecAlloc(exec, io->mmu,
				sizeof(struct TDeviceNode) + len + 1);
			if (dn)
			{
				dn->name = (TSTRPTR) (dn + 1);
				io_strcpy(dn->name, name);

				dn->flags = TDEVF_ASSIGN;
				dn->altname = TNULL;
				dn->lock = lock;
				dn->device = TNULL;

				TAddTail(&io->devicelist, (struct TNode *) dn);
				err = 0;
			}
			else
				err = TIOERR_NOT_ENOUGH_MEMORY;
		}
		else if (lock == TNULL)
		{
			/* cancel an existing assign */
			if (dn)
			{
				TRemove((struct TNode *) dn);
				io_UnlockFile(io, dn->lock);
				TExecFree(exec, dn);
				err = 0;
			}
			else
				err = TIOERR_OBJECT_NOT_FOUND;
		}
		else
			err = TIOERR_OBJECT_EXISTS;

		TExecUnlock(exec, io->modlock);
	}

	if (err == 0)
		return TTRUE;
	io_SetIOErr(io, err);
	return TFALSE;
}

/*****************************************************************************/
/*
**	len = io_makename(io, name, destbuf, destlen, mode, tags)
**	convert a name, render the result to destbuf/destlen.
**	returns the length of the resulting string, or -1 in case
**	of an error.
*/

EXPORT TINT
io_MakeName(TMOD_IO *io, TSTRPTR name, TSTRPTR destbuf, TINT destlen,
	TINT mode, TTAGITEM *tags)
{
	TINT res = -1;
	TINT err = TIOERR_BAD_ARGUMENTS;
	TAPTR exec = TGetExecBase(io);
	if (mode == TPPF_HOST2TEK)
	{
		struct TIOPacket *cdlock = io_LockFile(io, "", TFLOCK_READ, TNULL);
		if (cdlock)
		{
			cdlock->io_Req.io_Command = TIOCMD_MAKENAME;
			cdlock->io_Op.MakeName.Name = name;
			cdlock->io_Op.MakeName.Dest = destbuf;
			cdlock->io_Op.MakeName.DLen = destlen;
			cdlock->io_Op.MakeName.Mode = mode;
			cdlock->io_Op.MakeName.Tags = tags;
			TExecDoIO(exec, (struct TIORequest *) cdlock);
			res = cdlock->io_Op.MakeName.Result;
			err = cdlock->io_Req.io_Error;
			io_UnlockFile(io, cdlock);
		}
		else
			err = io_GetIOErr(io);
	}
	else if (mode == TPPF_TEK2HOST)
	{
		/* purify path (still in TEKlib notation) */
		TSTRPTR fullname = io_addpart(io, name, "");
		if (fullname)
		{
			TSTRPTR namepart;
			/* address the responsible handler to resolve the rest */
			struct TIOPacket *iomsg = io_ObtainMsg(io, fullname, &namepart);
			if (iomsg)
			{
				iomsg->io_Req.io_Command = TIOCMD_MAKENAME;
				iomsg->io_Op.MakeName.Name = namepart;
				iomsg->io_Op.MakeName.Dest = destbuf;
				iomsg->io_Op.MakeName.DLen = destlen;
				iomsg->io_Op.MakeName.Mode = mode;
				iomsg->io_Op.MakeName.Tags = tags;
				TExecDoIO(exec, (struct TIORequest *) iomsg);
				res = iomsg->io_Op.MakeName.Result;
				err = iomsg->io_Req.io_Error;
				io_ReleaseMsg(io, iomsg);
			}
			else
				err = io_GetIOErr(io);

			TExecFree(exec, fullname);
		}
		else
			err = io_GetIOErr(io);
	}

	if (res < 0)
		io_SetIOErr(io, err);

	return res;
}

/*****************************************************************************/
/*
**	success = io_mount(io, name, action, tags)
**	tags are currently passed directly to the handler identified
**	by the TIOMount_Handler tag.
*/

EXPORT TBOOL
io_Mount(TMOD_IO *io, TSTRPTR name, TINT action, TTAGITEM *tags)
{
	TINT err = TIOERR_BAD_ARGUMENTS;

	switch (action)
	{
		default:
			name = TNULL;

		case TIOMNT_ADD:
		case TIOMNT_REMOVE:
			break;
	}

	if (name)
	{
		TAPTR exec = TGetExecBase(io);
		struct TDeviceNode *dn;
		TINT nlen;
		TSTRPTR p;

		p = io_strchr(name, ':');
		if (p)
			nlen = p - name;
		else
			nlen = io_strlen(name);

		TExecLock(exec, io->modlock);

		dn = io_finddevice(io, name, nlen);

		if (action == TIOMNT_ADD)
		{
			if (dn == TNULL)
			{
				TSTRPTR hndname =
					(TSTRPTR) TGetTag(tags, TIOMount_Handler, (TTAG) name);
				TUINT16 hndver =
					(TUINT16) (TUINT) TGetTag(tags, TIOMount_HndVersion, 0);

				TINT hlen = io_strlen(hndname);
				struct TIOPacket *lock = io_openhandler(io, hndname, hlen,
					hndver, tags);
				if (lock)
				{
					dn = TExecAlloc(exec, io->mmu,
						sizeof(struct TDeviceNode) + nlen + 1);
					if (dn)
					{
						dn->name = (TSTRPTR) (dn + 1);
						io_strcpy(dn->name, name);

						dn->flags = TDEVF_MOUNT;
						dn->lock = lock;
						dn->device = TNULL;
						dn->altname = TNULL;

						TAddTail(&io->devicelist, (struct TNode *) dn);
						err = 0;
					}
					else
					{
						io_ReleaseMsg(io, lock);
						err = TIOERR_NOT_ENOUGH_MEMORY;
					}
				}
				else
					err = TIOERR_DEVICE_OPEN_FAILED;
			}
			else
				err = TIOERR_OBJECT_EXISTS;
		}
		else
		{
			if (dn)
			{
				if (dn->flags & TDEVF_MOUNT)
				{
					TRemove((struct TNode *) dn);
					io_ReleaseMsg(io, dn->lock);
					TExecFree(exec, dn);
					err = 0;
				}
				else
					err = TIOERR_OBJECT_WRONG_TYPE;
			}
			else
				err = TIOERR_OBJECT_NOT_FOUND;
		}

		TExecUnlock(exec, io->modlock);
	}

	if (err == 0)
		return TTRUE;

	io_SetIOErr(io, err);
	return TFALSE;
}
