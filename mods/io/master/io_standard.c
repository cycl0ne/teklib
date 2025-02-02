
/* 
**	$Id: io_standard.c,v 1.7 2005/09/13 01:11:46 tmueller Exp $
**	standard IO operations
*/

#include "io_mod.h"

/*****************************************************************************/
/*
**	oldcode = io_seterr(io, errorcode)
*/

EXPORT TINT
io_seterr(TMOD_IO *io, TINT errorcode)
{
	struct TTask *self = TFindTask(TNULL);
	TINT olderr = self->tsk_IOErr;
	self->tsk_IOErr = errorcode;
	return olderr;
}

/*****************************************************************************/
/*
**	error = io_ioerr(io)
*/

EXPORT TINT
io_ioerr(TMOD_IO *io)
{
	struct TTask *self = TFindTask(TNULL);
	return self->tsk_IOErr;
}

/*
**	pos = seek_internal(io, handle, offs, offs_hi, mode)
*/

static TUINT
seek_internal(TMOD_IO *io, TIOMSG *iomsg, TINT offs, TINT *offs_hi, TINT mode)
{
	iomsg->io_Req.io_Command = TIOCMD_SEEK;
	iomsg->io_Op.Seek.Offs = offs;
	iomsg->io_Op.Seek.OffsHi = offs_hi;
	iomsg->io_Op.Seek.Mode = mode;

	TDoIO((struct TIORequest *) iomsg);
	if (iomsg->io_Op.Seek.Result == 0xffffffff)
	{
		io_seterr(io, iomsg->io_Req.io_Error);
	}
	return iomsg->io_Op.Seek.Result;
}

/*
**	success = writebuffer(io, iomsg, buffer, len)
*/

static TBOOL
writebuffer(TMOD_IO *io, TIOMSG *iomsg, TUINT8 *buf, TINT len)
{
	if (buf)
	{
		while (len)
		{
			TINT wr = io_write(io, iomsg, buf, len);
			if (wr < 0) return TFALSE;
			buf += wr;
			len -= wr;
		}
	}
	return TTRUE;
}

/*
**	iobuf = io_getiobuf(io, iomsg, getnew)
*/

static TIOBUF *
io_getiobuf(TMOD_IO *io, TIOMSG *iomsg, TBOOL getnew)
{
	TIOBUF *iobuf = TNULL;

	if (iomsg)
	{
		iobuf = iomsg->io_Buffer;
		if (!iobuf && getnew)
		{
			iobuf = iomsg->io_Buffer = 
				TAlloc(io->mmu, iomsg->io_BufSize + sizeof(TIOBUF));

			if (iobuf)
			{
				iobuf->pos = 0;
				iobuf->fill = 0;
				/* use handler recommendations */
				iobuf->size = iomsg->io_BufSize;
				iobuf->mode = iomsg->io_BufFlags;
			}
			else
			{
				io_seterr(io, TIOERR_NOT_ENOUGH_MEMORY);
			}
		}
	}

	return iobuf;
}

/*****************************************************************************/
/*
**	fh = io_open(io, name, mode, tags)
*/

EXPORT TAPTR
io_open(TMOD_IO *io, TSTRPTR name, TUINT mode, TTAGITEM *tags)
{
	TSTRPTR namepart;
	TIOMSG *iomsg;

	iomsg = io_obtainmsg(io, name, &namepart);
	if (iomsg)
	{
		iomsg->io_Req.io_Command = TIOCMD_OPEN;
		iomsg->io_Op.Open.Name = namepart;
		iomsg->io_Op.Open.Mode = mode;
		iomsg->io_Op.Open.Tags = tags;
		
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Op.Open.Result)
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
**	io_close(io, fh)
*/

EXPORT TBOOL
io_close(TMOD_IO *io, TIOMSG *iomsg)
{
	TINT err = TIOERR_BAD_ARGUMENTS;

	if (iomsg)
	{
		io_flush(io, iomsg);
		iomsg->io_Req.io_Command = TIOCMD_CLOSE;
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Op.Close.Result)
		{
			err = 0;
		}
		else
		{
			err = iomsg->io_Req.io_Error;
		}

		io_releasemsg(io, iomsg);
	}
	
	if (err == 0) return TTRUE;
	io_seterr(io, err);
	return TFALSE;
}

/*****************************************************************************/
/*
**	rdlen = io_read(io, fh, buf, len)
*/

EXPORT TINT
io_read(TMOD_IO *io, TIOMSG *iomsg, TAPTR buf, TINT len)
{
	if (iomsg && (!buf == !len))
	{
		if (len == 0) return 0;
	
		iomsg->io_Req.io_Command = TIOCMD_READ;
		iomsg->io_Op.Read.Buf = buf;
		iomsg->io_Op.Read.Len = len;
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Op.Read.RdLen < 0)
		{
			io_seterr(io, iomsg->io_Req.io_Error);
		}	

		return iomsg->io_Op.Read.RdLen;
	}

	io_seterr(io, TIOERR_BAD_ARGUMENTS);
	return -1;
}

/*****************************************************************************/
/*
**	wrlen = io_write(io, fh, buf, len)
*/

EXPORT TINT
io_write(TMOD_IO *io, TIOMSG *iomsg, TAPTR buf, TINT len)
{
	if (iomsg && (!buf == !len))
	{
		if (len == 0) return 0;
	
		iomsg->io_Req.io_Command = TIOCMD_WRITE;
		iomsg->io_Op.Write.Buf = buf;
		iomsg->io_Op.Write.Len = len;
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Op.Write.WrLen < 0)
		{
			io_seterr(io, iomsg->io_Req.io_Error);
		}

		return iomsg->io_Op.Write.WrLen;
	}

	io_seterr(io, TIOERR_BAD_ARGUMENTS);
	return -1;
}

/*****************************************************************************/
/*
**	pos = io_seek(io, handle, offs, offs_hi, mode)
**	returns new absolute position, -1 on error
*/

EXPORT TUINT
io_seek(TMOD_IO *io, TIOMSG *iomsg, TINT offs, TINT *offs_hi, TINT mode)
{
	if (iomsg)
	{
		if (io_flush(io, iomsg))
		{
			return seek_internal(io, iomsg, offs, offs_hi, mode);
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}	
	
	return 0xffffffff;
}

/*****************************************************************************/
/*
**	success = io_flush(io, handle)
*/

EXPORT TBOOL
io_flush(TMOD_IO *io, TIOMSG *iomsg)
{
	if (iomsg)
	{
		TIOBUF *iobuf = io_getiobuf(io, iomsg, TFALSE);
		if (iobuf)
		{
			TUINT mode = iobuf->mode & TIOBUF_ACCESSMODE;

			if (mode == TIOBUF_READ)
			{
				TINT offs = iobuf->pos - iobuf->fill;
				TUINT res = offs < 0 ? 0xffffffff : 0;
				res = seek_internal(io, iomsg, offs, (TINT *) &res,
					TFPOS_CURRENT);
				if (res == 0xffffffff && io_ioerr(io))
				{
					return TFALSE;
				}
			}
			else if (mode == TIOBUF_WRITE)
			{
				if (!writebuffer(io, iomsg, 
					(TUINT8 *) (iobuf + 1), iobuf->pos))
				{
					return TFALSE;
				}
			}

			iobuf->pos = 0;
			iobuf->fill = 0;
			iobuf->mode &= ~TIOBUF_ACCESSMODE;
		}

		return TTRUE;
	}

	io_seterr(io, TIOERR_BAD_ARGUMENTS);
	return TFALSE;
}

/*****************************************************************************/
/*
**	char = io_fputc(io, handle, char)
**	returns char, TEOF on error
*/

EXPORT TINT
io_fputc(TMOD_IO *io, TIOMSG *iomsg, TINT c)
{
	if (iomsg)
	{
		if (c != TEOF)
		{
			TIOBUF *iobuf = io_getiobuf(io, iomsg, TTRUE);
			if (iobuf)
			{
				if ((iobuf->mode & TIOBUF_ACCESSMODE) == TIOBUF_READ)
				{
					TINT offs = iobuf->pos - iobuf->fill;
					TUINT res = offs < 0 ? 0xffffffff : 0;
					res = seek_internal(io, iomsg, offs,
						(TINT *) &res, TFPOS_CURRENT);
					if (res == 0xffffffff && io_ioerr(io))
					{
						return TEOF;
					}
					iobuf->pos = 0;
					iobuf->mode &= ~TIOBUF_ACCESSMODE;
				}
				iobuf->mode |= TIOBUF_WRITE;
	
				((TUINT8 *)(iobuf + 1))[iobuf->pos++] = (TUINT8) c;
	
				if ((iobuf->pos == iobuf->size) ||
					(((iobuf->mode & TIOBUF_BUFMODE) == TIOBUF_LINE) &&
					((c == 10) || (c == 12))) ||
					((iobuf->mode & TIOBUF_BUFMODE) == TIOBUF_NONE))
				{
					if (!writebuffer(io, iomsg,
						(TUINT8 *) (iobuf + 1), iobuf->pos))
					{
						return TEOF;
					}
			 		iobuf->pos = 0;
				}
	
				return c;
			}
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return TEOF;
}

/*****************************************************************************/
/*
**	char = io_fgetc(io, handle)
**	returns TEOF on error
*/

EXPORT TINT
io_fgetc(TMOD_IO *io, TIOMSG *iomsg)
{
	if (iomsg)
	{
		TIOBUF *iobuf = io_getiobuf(io, iomsg, TTRUE);
		if (iobuf)
		{
			if ((iobuf->mode & TIOBUF_ACCESSMODE) == TIOBUF_WRITE)
			{
				if (!writebuffer(io, iomsg, (TUINT8 *) (iobuf + 1),
					iobuf->pos))
				{
					return TEOF;
				}
				iobuf->pos = 0;
				iobuf->fill = 0;
				iobuf->mode &= ~TIOBUF_ACCESSMODE;
			}

			iobuf->mode |= TIOBUF_READ;

			if (iobuf->fill == 0 || iobuf->pos == iobuf->fill)
			{
				if (iobuf->pos == iobuf->fill)
				{
					iobuf->pos = 0;
				}

				iobuf->fill = io_read(io, iomsg,
					(TUINT8 *) (iobuf + 1), iobuf->size);
				if (iobuf->fill <= 0)
				{
					return TEOF;
				}

				iobuf->pos = 0;
			}

			return (TINT) ((TUINT8 *) (iobuf + 1))[iobuf->pos++];
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return TEOF;
}

/*****************************************************************************/
/*
**	success = io_feof(io, handle)
*/

EXPORT TBOOL
io_feof(TMOD_IO *io, TIOMSG *iomsg)
{
	if (iomsg)
	{
		TIOBUF *iobuf = io_getiobuf(io, iomsg, TTRUE);
		if (iobuf)
		{
			TINT c = io_fgetc(io, iomsg);
			if (c == TEOF) return TTRUE;
			iobuf->pos--;		/* push back character */
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	char = io_fungetc(io, handle, char)
*/

EXPORT TINT
io_fungetc(TMOD_IO *io, TIOMSG *iomsg, TINT c)
{
	if (iomsg)
	{
		TIOBUF *iobuf = io_getiobuf(io, iomsg, TTRUE);
		if (iobuf)
		{
			if (iobuf->pos == 0) return TEOF;
			
			if (c == -1)
			{
				c = (TINT) ((TUINT8 *)(iobuf + 1))[--iobuf->pos];
			}
			else
			{
				((TUINT8 *)(iobuf + 1))[iobuf->pos--] = c;
			}

			return c;
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return -1;
}

/*****************************************************************************/
/*
**	error = io_fputs(io, fh, string)
*/

EXPORT TINT
io_fputs(TMOD_IO *io, TIOMSG *iomsg, TSTRPTR s)
{
	if (iomsg)
	{
		TINT c, error = 0, n = 0;
		while ((c = *s++))
		{
			error = io_fputc(io, iomsg, c);
			if (error == TEOF) break;
			n++;
		}
		
		if (error != TEOF) return n;
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return TEOF;
}

/*****************************************************************************/
/*
**	buf = io_fgets(io, fh, buf, len)
*/

EXPORT TSTRPTR
io_fgets(TMOD_IO *io, TIOMSG *iomsg, TSTRPTR buf, TINT len)
{
	if (iomsg && buf && len > 0)
	{
		TINT c;
		TINT rdlen = 0;

		io_seterr(io, 0);
		
		while (rdlen < len - 1 && (c = io_fgetc(io, iomsg)) != TEOF)
		{
			buf[rdlen++] = c;
			if (c == 10) break;
		}
		
		buf[rdlen] = 0;
		if (rdlen > 0 && io_ioerr(io) == 0)
		{
			return buf;
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	wrlen = io_fread(io, file, buf, len)
*/

EXPORT TINT
io_fread(TMOD_IO *io, TIOMSG *iomsg, TUINT8 *buf, TINT len)
{
	TINT rdlen = -1;

	if (iomsg && buf)
	{
		rdlen = 0;
		while (len)
		{
			TINT c = io_fgetc(io, iomsg);
			if (c == TEOF)
			{
				if (io_ioerr(io)) rdlen = -1; 
				break;
			}
			buf[rdlen] = (TUINT8) c;
			rdlen++;
			len--;
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}

	return rdlen;
}

/*****************************************************************************/
/*
**	wrlen = io_fwrite(io, file, buf, len)
*/

EXPORT TINT
io_fwrite(TMOD_IO *io, TIOMSG *iomsg, TUINT8 *buf, TINT len)
{
	TINT wrlen = -1;

	if (iomsg && buf)
	{
		wrlen = 0;
		while (len)
		{
			if (io_fputc(io, iomsg, buf[wrlen]) == TEOF)
			{
				if (io_ioerr(io)) wrlen = -1; 
				break;
			}
			wrlen++;
			len--;
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}
	
	return wrlen;
}

/*****************************************************************************/
/*
**	isavail = io_waitchar(io, file, timeout)
*/

EXPORT TBOOL
io_waitchar(TMOD_IO *io, TIOMSG *iomsg, TINT timeout)
{
	TBOOL isavail = TFALSE;
	
	if (iomsg)
	{
		iomsg->io_Req.io_Command = TIOCMD_WAITCHAR;
		iomsg->io_Op.WaitChar.TimeOut = timeout;
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Req.io_Error == 0)
		{
			isavail = iomsg->io_Op.WaitChar.Result;
		}
		else
		{
			io_seterr(io, iomsg->io_Req.io_Error);
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}
	
	return isavail;
}

/*****************************************************************************/
/*
**	itis = io_isinteractive(io, file)
*/

EXPORT TBOOL
io_isinteractive(TMOD_IO *io, TIOMSG *iomsg)
{
	TBOOL interactive = TFALSE;
	
	if (iomsg)
	{
		iomsg->io_Req.io_Command = TIOCMD_INTERACTIVE;
		TDoIO((struct TIORequest *) iomsg);
		if (iomsg->io_Req.io_Error == 0)
		{
			interactive = iomsg->io_Op.Interactive.Result;
		}
		else
		{
			io_seterr(io, iomsg->io_Req.io_Error);
		}
	}
	else
	{
		io_seterr(io, TIOERR_BAD_ARGUMENTS);
	}
	
	return interactive;
}

/*****************************************************************************/
/*
**	len = io_fault(io, errcode, buf, buflen, tags)
**
**	return an english representation of an IO error code.
**	returns the length of the string, not including the
**	extra byte for the string's trailing zero. buf and buflen
**	may be NULL, in which case only the length will be
**	returned. if buflen is too small or err is not known,
**	returns -1. by convention, error code descriptions are
**	no longer than 60 characters.
*/

EXPORT TINT
io_fault(TMOD_IO *io, TINT err, TSTRPTR buf, TINT buflen, TTAGITEM *tags)
{
	if (err && (!buf == !buflen))
	{
		TSTRPTR text;
		TINT len;

		switch (err)
		{
			case 0:
				return 0;

			default:
				return -1;

			case TIOERR_OUT_OF_RANGE:
				text = "Out of range";
				break;
			case TIOERR_LINE_TOO_LONG:
				text = "Line too long";
				break;
			case TIOERR_NOT_ENOUGH_MEMORY:
				text = "Not enough memory";
				break;
			case TIOERR_UNKNOWN_COMMAND:
				text = "Command unknown to handler";
				break;
			case TIOERR_BAD_ARGUMENTS:
				text = "Bad arguments";
				break;
			case TIOERR_INVALID_NAME:
				text = "Invalid name";
				break;

			case TIOERR_DISK_FULL:
				text = "Disk full";
				break;
			case TIOERR_DISK_WRITE_PROTECTED:
				text = "Disk write protected";
				break;
			case TIOERR_ACCESS_DENIED:
				text = "Access denied";
				break;
			case TIOERR_TOO_MANY_LEVELS:
				text = "Too many levels";
				break;
				
			case TIOERR_OBJECT_NOT_FOUND:
				text = "Object not found";
				break;
			case TIOERR_OBJECT_WRONG_TYPE:
				text = "Object wrong type";
				break;
			case TIOERR_OBJECT_EXISTS:
				text = "Object exists";
				break;
			case TIOERR_OBJECT_TOO_LARGE:
				text = "Object too large";
				break;
			case TIOERR_OBJECT_IN_USE:
				text = "Object in use";
				break;
				
			case TIOERR_DISK_NOT_READY:
				text = "Disk not ready";
				break;
			case TIOERR_DISK_CORRUPT:
				text = "Disk corrupt";
				break;

			case TIOERR_NO_MORE_ENTRIES:
				text = "No more entries in directory";
				break;
			case TIOERR_NOT_SAME_DEVICE:
				text = "Not same device";
				break;
			case TIOERR_DIRECTORY_NOT_EMPTY:
				text = "Directory not empty";
				break;
			case TIOERR_DEVICE_OPEN_FAILED:
				text = "Device open failed";
				break;
		}
		
		len = io_strlen(text);

		if (!buf)
		{
			return len;
		}
		
		if (buflen > len)
		{
			io_strcpy(buf, text);
			return len;
		}
		
	}
	
	return -1;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: io_standard.c,v $
**	Revision 1.7  2005/09/13 01:11:46  tmueller
**	corrected ungetc, fputs returns number of chars
**	
**	Revision 1.6  2005/09/08 00:01:37  tmueller
**	mod API extended; cleanup; util dependency removed
**	
**	Revision 1.5  2004/06/27 22:29:42  tmueller
**	io_lock() and io_open() did not set ioerr correctly
**	
**	Revision 1.4  2004/06/26 09:49:44  tmueller
**	io_fgets(): io_error condition was not reset. fixed
**	
**	Revision 1.3  2004/06/26 06:47:32  tmueller
**	io_fgets used 12 instead of 10 for termination. fixed
**	
**	Revision 1.2  2003/12/22 22:55:19  tmueller
**	Renamed field names in I/O packets to uppercase
**	
**	Revision 1.1.1.1  2003/12/11 07:18:44  tmueller
**	Krypton import
**	
**	Revision 1.8  2003/10/16 17:38:57  tmueller
**	Applied changed exec structure names. Minor optimization in name resolution
**	
**	Revision 1.7  2003/07/11 19:36:14  tmueller
**	added io_fgets, io_fputs, io_fungetc
**	
**	Revision 1.6  2003/07/07 20:18:31  tmueller
**	Added io_fungetc()
**	
**	Revision 1.5  2003/05/11 14:11:34  tmueller
**	Updated I/O master and default implementations to extended definitions
**	
**	Revision 1.4  2003/03/22 05:35:19  tmueller
**	Warnings removed, Amiga implementation of io_seek() fixed
**	
**	Revision 1.3  2003/03/22 05:22:38  tmueller
**	io_seek() now handles 64bit offsets. The function prototype changed. The
**	return value is now unsigned. See the documentation of io_seek() for the
**	implications of the API change. TIOERR_SEEK_ERROR has been removed,
**	TIOERR_OUT_OF_RANGE was added.
**	
**	Revision 1.2  2003/03/12 16:36:46  dtrompetter
**	*** empty log message ***
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.7  2003/03/04 02:34:58  bifat
**	The return value of io_examine() and io_exnext() changed.
**	
**	Revision 1.6  2003/03/02 15:47:03  bifat
**	Some packet types extended with additional tag arguments. io_addpart() API
**	changed. io_lock(), io_open(), io_makedir() and io_fault() got additional
**	taglist arguments. Added TIOERR_LINE_TOO_LONG error code.
**	
**	Revision 1.5  2003/03/01 21:22:24  bifat
**	Massive cleanup and simplification of error codes. Now fits very well to
**	the different platforms.
**	
**	Revision 1.4  2003/02/09 13:28:32  tmueller
**	added io_outputfh(), added stdin/stdout/stderr fields to exec tasks,
**	cleaned up child task inheritance of current directory, added task tags
**	TTask_OutputFH etc. for passing stdio filehandles to newly created tasks.
**	
**	Revision 1.3  2003/02/02 23:08:43  tmueller
**	Locks no longer have a field with a backpointer to a task that "owns" them
**	as a currentdir. That doesn't work if locks are passed around across tasks,
**	and it wasn't very useful either.
**	
**	Revision 1.2  2003/01/21 22:28:02  tmueller
**	added io_waitchar, io_isinteractive, more packets
**	
**	Revision 1.1  2003/01/20 21:29:56  tmueller
**	added
**	
*/
