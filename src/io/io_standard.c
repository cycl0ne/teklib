
/* 
**	$Id: io_standard.c,v 1.2 2006/09/10 14:40:37 tmueller Exp $
**	teklib/src/io/io_standard.c - "standard" IO operations
*/

#include "io_mod.h"

/*****************************************************************************/
/*
**	oldcode = io_SetIOErr(io, errorcode)
*/

EXPORT TINT
io_SetIOErr(TMOD_IO *io, TINT errorcode)
{
	TAPTR exec = TGetExecBase(io);
	struct TTask *self = TExecFindTask(exec, TNULL);
	TINT olderr = self->tsk_IOErr;
	self->tsk_IOErr = errorcode;
	return olderr;
}

/*****************************************************************************/
/*
**	error = io_ioerr(io)
*/

EXPORT TINT
io_GetIOErr(TMOD_IO *io)
{
	TAPTR exec = TGetExecBase(io);
	struct TTask *self = TExecFindTask(exec, TNULL);
	return self->tsk_IOErr;
}

/*
**	pos = seek_internal(io, handle, offs, offs_hi, mode)
*/

static TUINT
seek_internal(TMOD_IO *io, struct TIOPacket *iomsg, TINT offs, TINT *offs_hi,
	TINT mode)
{
	TAPTR exec = TGetExecBase(io);
	
	iomsg->io_Req.io_Command = TIOCMD_SEEK;
	iomsg->io_Op.Seek.Offs = offs;
	iomsg->io_Op.Seek.OffsHi = offs_hi;
	iomsg->io_Op.Seek.Mode = mode;

	TExecDoIO(exec, (struct TIORequest *) iomsg);
	if (iomsg->io_Op.Seek.Result == 0xffffffff)
		io_SetIOErr(io, iomsg->io_Req.io_Error);
	return iomsg->io_Op.Seek.Result;
}

/*
**	success = writebuffer(io, iomsg, buffer, len)
*/

static TBOOL
writebuffer(TMOD_IO *io, struct TIOPacket *iomsg, TUINT8 *buf, TINT len)
{
	if (buf)
	{
		while (len)
		{
			TINT wr = io_Write(io, iomsg, buf, len);
			if (wr < 0) 
				return TFALSE;
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
io_getiobuf(TMOD_IO *io, struct TIOPacket *iomsg, TBOOL getnew)
{
	TIOBUF *iobuf = TNULL;

	if (iomsg)
	{
		iobuf = iomsg->io_Buffer;
		if (!iobuf && getnew)
		{
			TAPTR exec = TGetExecBase(io);
			iobuf = iomsg->io_Buffer = 
				TExecAlloc(exec, io->mmu, iomsg->io_BufSize + sizeof(TIOBUF));

			if (iobuf)
			{
				iobuf->pos = 0;
				iobuf->fill = 0;
				/* use handler recommendations */
				iobuf->size = iomsg->io_BufSize;
				iobuf->mode = iomsg->io_BufFlags;
			}
			else
				io_SetIOErr(io, TIOERR_NOT_ENOUGH_MEMORY);
		}
	}

	return iobuf;
}

/*****************************************************************************/
/*
**	fh = io_open(io, name, mode, tags)
*/

EXPORT TAPTR
io_OpenFile(TMOD_IO *io, TSTRPTR name, TUINT mode, TTAGITEM *tags)
{
	TSTRPTR namepart;
	struct TIOPacket *iomsg;

	iomsg = io_ObtainMsg(io, name, &namepart);
	if (iomsg)
	{
		TAPTR exec = TGetExecBase(io);
		
		iomsg->io_Req.io_Command = TIOCMD_OPEN;
		iomsg->io_Op.Open.Name = namepart;
		iomsg->io_Op.Open.Mode = mode;
		iomsg->io_Op.Open.Tags = tags;
		
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Op.Open.Result)
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
**	io_close(io, fh)
*/

EXPORT TBOOL
io_CloseFile(TMOD_IO *io, struct TIOPacket *iomsg)
{
	TINT err = TIOERR_BAD_ARGUMENTS;

	if (iomsg)
	{
		TAPTR exec = TGetExecBase(io);
		io_Flush(io, iomsg);
		iomsg->io_Req.io_Command = TIOCMD_CLOSE;
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Op.Close.Result)
			err = 0;
		else
			err = iomsg->io_Req.io_Error;
		io_ReleaseMsg(io, iomsg);
	}
	
	if (err == 0)
		return TTRUE;
	io_SetIOErr(io, err);
	return TFALSE;
}

/*****************************************************************************/
/*
**	rdlen = io_read(io, fh, buf, len)
*/

EXPORT TINT
io_Read(TMOD_IO *io, struct TIOPacket *iomsg, TAPTR buf, TINT len)
{
	if (iomsg && (!buf == !len))
	{
		TAPTR exec;
		
		if (len == 0)
			return 0;
	
		iomsg->io_Req.io_Command = TIOCMD_READ;
		iomsg->io_Op.Read.Buf = buf;
		iomsg->io_Op.Read.Len = len;
		exec = TGetExecBase(io);
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Op.Read.RdLen < 0)
			io_SetIOErr(io, iomsg->io_Req.io_Error);

		return iomsg->io_Op.Read.RdLen;
	}

	io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);
	return -1;
}

/*****************************************************************************/
/*
**	wrlen = io_write(io, fh, buf, len)
*/

EXPORT TINT
io_Write(TMOD_IO *io, struct TIOPacket *iomsg, TAPTR buf, TINT len)
{
	if (iomsg && (!buf == !len))
	{
		TAPTR exec;
		
		if (len == 0)
			return 0;
	
		exec = TGetExecBase(io);
		iomsg->io_Req.io_Command = TIOCMD_WRITE;
		iomsg->io_Op.Write.Buf = buf;
		iomsg->io_Op.Write.Len = len;
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Op.Write.WrLen < 0)
			io_SetIOErr(io, iomsg->io_Req.io_Error);

		return iomsg->io_Op.Write.WrLen;
	}

	io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);
	return -1;
}

/*****************************************************************************/
/*
**	pos = io_seek(io, handle, offs, offs_hi, mode)
**	returns new absolute position, -1 on error
*/

EXPORT TUINT
io_Seek(TMOD_IO *io, struct TIOPacket *iomsg, TINT offs, TINT *offs_hi,
	TINT mode)
{
	if (iomsg)
	{
		if (io_Flush(io, iomsg))
			return seek_internal(io, iomsg, offs, offs_hi, mode);
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);
	
	return 0xffffffff;
}

/*****************************************************************************/
/*
**	success = io_flush(io, handle)
*/

EXPORT TBOOL
io_Flush(TMOD_IO *io, struct TIOPacket *iomsg)
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
				if (res == 0xffffffff && io_GetIOErr(io))
					return TFALSE;
			}
			else if (mode == TIOBUF_WRITE)
			{
				if (!writebuffer(io, iomsg, 
					(TUINT8 *) (iobuf + 1), iobuf->pos))
					return TFALSE;
			}

			iobuf->pos = 0;
			iobuf->fill = 0;
			iobuf->mode &= ~TIOBUF_ACCESSMODE;
		}

		return TTRUE;
	}

	io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);
	return TFALSE;
}

/*****************************************************************************/
/*
**	char = io_fputc(io, handle, char)
**	returns char, TEOF on error
*/

EXPORT TINT
io_FPutC(TMOD_IO *io, struct TIOPacket *iomsg, TINT c)
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
					if (res == 0xffffffff && io_GetIOErr(io))
						return TEOF;
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
						return TEOF;
			 		iobuf->pos = 0;
				}
	
				return c;
			}
		}
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return TEOF;
}

/*****************************************************************************/
/*
**	char = io_fgetc(io, handle)
**	returns TEOF on error
*/

EXPORT TINT
io_FGetC(TMOD_IO *io, struct TIOPacket *iomsg)
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
					return TEOF;
				iobuf->pos = 0;
				iobuf->fill = 0;
				iobuf->mode &= ~TIOBUF_ACCESSMODE;
			}

			iobuf->mode |= TIOBUF_READ;

			if (iobuf->fill == 0 || iobuf->pos == iobuf->fill)
			{
				if (iobuf->pos == iobuf->fill)
					iobuf->pos = 0;

				iobuf->fill = io_Read(io, iomsg,
					(TUINT8 *) (iobuf + 1), iobuf->size);
				if (iobuf->fill <= 0)
					return TEOF;

				iobuf->pos = 0;
			}

			return (TINT) ((TUINT8 *) (iobuf + 1))[iobuf->pos++];
		}
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return TEOF;
}

/*****************************************************************************/
/*
**	success = io_feof(io, handle)
*/

EXPORT TBOOL
io_FEoF(TMOD_IO *io, struct TIOPacket *iomsg)
{
	if (iomsg)
	{
		TIOBUF *iobuf = io_getiobuf(io, iomsg, TTRUE);
		if (iobuf)
		{
			TINT c = io_FGetC(io, iomsg);
			if (c == TEOF)
				return TTRUE;
			iobuf->pos--;		/* push back character */
		}
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return TFALSE;
}

/*****************************************************************************/
/*
**	char = io_fungetc(io, handle, char)
*/

EXPORT TINT
io_FUngetC(TMOD_IO *io, struct TIOPacket *iomsg, TINT c)
{
	if (iomsg)
	{
		TIOBUF *iobuf = io_getiobuf(io, iomsg, TTRUE);
		if (iobuf)
		{
			if (iobuf->pos == 0) return TEOF;
			
			if (c == -1)
				c = (TINT) ((TUINT8 *)(iobuf + 1))[--iobuf->pos];
			else
				((TUINT8 *)(iobuf + 1))[--iobuf->pos] = c;

			return c;
		}
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return -1;
}

/*****************************************************************************/
/*
**	error = io_fputs(io, fh, string)
*/

EXPORT TINT
io_FPutS(TMOD_IO *io, struct TIOPacket *iomsg, TSTRPTR s)
{
	if (iomsg)
	{
		TINT c, error = 0, n = 0;
		while ((c = *s++))
		{
			error = io_FPutC(io, iomsg, c);
			if (error == TEOF)
				break;
			n++;
		}
		
		if (error != TEOF)
			return n;
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return TEOF;
}

/*****************************************************************************/
/*
**	buf = io_fgets(io, fh, buf, len)
*/

EXPORT TSTRPTR
io_FGetS(TMOD_IO *io, struct TIOPacket *iomsg, TSTRPTR buf, TINT len)
{
	if (iomsg && buf && len > 0)
	{
		TINT c;
		TINT rdlen = 0;

		io_SetIOErr(io, 0);
		
		while (rdlen < len - 1 && (c = io_FGetC(io, iomsg)) != TEOF)
		{
			buf[rdlen++] = c;
			if (c == 10)
				break;
		}
		
		buf[rdlen] = 0;
		if (rdlen > 0 && io_GetIOErr(io) == 0)
			return buf;
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return TNULL;
}

/*****************************************************************************/
/*
**	wrlen = io_fread(io, file, buf, len)
*/

EXPORT TINT
io_FRead(TMOD_IO *io, struct TIOPacket *iomsg, TUINT8 *buf, TINT len)
{
	TINT rdlen = -1;

	if (iomsg && buf)
	{
		io_SetIOErr(io, 0);
		rdlen = 0;
		while (len)
		{
			TINT c = io_FGetC(io, iomsg);
			if (c == TEOF)
			{
				if (io_GetIOErr(io))
					rdlen = -1; 
				break;
			}
			buf[rdlen] = (TUINT8) c;
			rdlen++;
			len--;
		}
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);

	return rdlen;
}

/*****************************************************************************/
/*
**	wrlen = io_fwrite(io, file, buf, len)
*/

EXPORT TINT
io_FWrite(TMOD_IO *io, struct TIOPacket *iomsg, TUINT8 *buf, TINT len)
{
	TINT wrlen = -1;

	if (iomsg && buf)
	{
		wrlen = 0;
		while (len)
		{
			if (io_FPutC(io, iomsg, buf[wrlen]) == TEOF)
			{
				if (io_GetIOErr(io))
					wrlen = -1; 
				break;
			}
			wrlen++;
			len--;
		}
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);
	
	return wrlen;
}

/*****************************************************************************/
/*
**	isavail = io_waitchar(io, file, timeout)
*/

EXPORT TBOOL
io_WaitChar(TMOD_IO *io, struct TIOPacket *iomsg, TINT timeout)
{
	TBOOL isavail = TFALSE;
	
	if (iomsg)
	{
		TAPTR exec = TGetExecBase(io);
		iomsg->io_Req.io_Command = TIOCMD_WAITCHAR;
		iomsg->io_Op.WaitChar.TimeOut = timeout;
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Req.io_Error == 0)
			isavail = iomsg->io_Op.WaitChar.Result;
		else
			io_SetIOErr(io, iomsg->io_Req.io_Error);
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);
	
	return isavail;
}

/*****************************************************************************/
/*
**	itis = io_isinteractive(io, file)
*/

EXPORT TBOOL
io_IsInteractive(TMOD_IO *io, struct TIOPacket *iomsg)
{
	TBOOL interactive = TFALSE;
	
	if (iomsg)
	{
		TAPTR exec = TGetExecBase(io);
		iomsg->io_Req.io_Command = TIOCMD_INTERACTIVE;
		TExecDoIO(exec, (struct TIORequest *) iomsg);
		if (iomsg->io_Req.io_Error == 0)
			interactive = iomsg->io_Op.Interactive.Result;
		else
			io_SetIOErr(io, iomsg->io_Req.io_Error);
	}
	else
		io_SetIOErr(io, TIOERR_BAD_ARGUMENTS);
	
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
io_Fault(TMOD_IO *io, TINT err, TSTRPTR buf, TINT buflen, TTAGITEM *tags)
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
			return len;
		
		if (buflen > len)
		{
			io_strcpy(buf, text);
			return len;
		}
	}
	
	return -1;
}

