
/*
**	$Id: io_mod.c,v 1.6 2006/11/11 14:19:09 tmueller Exp $
**	teklib/src/hash/hash_mod.c - Io module
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "io_mod.h"
#include <tek/mod/exec.h>

static THOOKENTRY TTAG io_dispatch(struct THook *hook, TAPTR obj, TTAG msg);
static TMOD_IO *io_open(TMOD_IO *mod, TTAGITEM *tags);
static void io_close(TMOD_IO *mod);
static void io_exit(TMOD_IO *mod);
static const TMFPTR io_vectors[IO_NUMVECTORS];

/*****************************************************************************/
/*
**	module init function
*/

TMODENTRY TUINT
tek_init_io(struct TTask *task, struct TModule *mod, TUINT16 version,
	TTAGITEM *tags)
{
	TMOD_IO *io = (TMOD_IO *) mod;
	TAPTR exec;

	if (io == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * IO_NUMVECTORS; /* negative size */

		if (version <= IO_VERSION)
			return sizeof(TMOD_IO);	/* positive size */

		return 0;
	}

	exec = TGetExecBase(io);
	io->modlock = TExecCreateLock(exec, TNULL);
	if (io->modlock)
	{
		TInitList(&io->devicelist);
		TInitList(&io->cdlocklist);
		TInitList(&io->stdfhlist);

		io->module.tmd_Version = IO_VERSION;
		io->module.tmd_Revision = IO_REVISION;
		io->module.tmd_Handle.thn_Hook.thk_Entry = io_dispatch;
		io->module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;

		#ifdef TDEBUG
		io->mmu = TExecCreateMemManager(exec, TNULL,
			TMMT_Tracking | TMMT_TaskSafe, TNULL);
		#endif

		TInitVectors(&io->module, io_vectors, IO_NUMVECTORS);
		return TTRUE;
	}

	return 0;
}

/*****************************************************************************/
/*
**	module exit function
*/

static THOOKENTRY TTAG
io_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	TMOD_IO *mod = (TMOD_IO *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			TDestroy(mod->mmu);
			TDestroy(mod->modlock);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) io_open(mod, obj);
		case TMSG_CLOSEMODULE:
			io_close(obj);
	}
	return 0;
}

/*****************************************************************************/
/*
**	instance open
*/

static TMOD_IO *
io_open(TMOD_IO *io, TTAGITEM *tags)
{
	TAPTR exec = TGetExecBase(io);
	TExecLock(exec, io->modlock);
	io->refcount++;
	TExecUnlock(exec, io->modlock);
	return io;
}

/*****************************************************************************/
/*
**	instance close
*/

static void
io_close(TMOD_IO *io)
{
	TAPTR exec = TGetExecBase(io);
	TExecLock(exec, io->modlock);
	if (--io->refcount == 0)
		io_exit(io);
	TExecUnlock(exec, io->modlock);
}

/*****************************************************************************/
/*
**	function vector table
*/

static const TMFPTR
io_vectors[IO_NUMVECTORS] =
{
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) io_LockFile,
	(TMFPTR) io_UnlockFile,
	(TMFPTR) io_OpenFile,
	(TMFPTR) io_CloseFile,
	(TMFPTR) io_Read,
	(TMFPTR) io_Write,
	(TMFPTR) io_Flush,
	(TMFPTR) io_Seek,
	(TMFPTR) io_FPutC,
	(TMFPTR) io_FGetC,
	(TMFPTR) io_FEoF,
	(TMFPTR) io_FRead,
	(TMFPTR) io_FWrite,
	(TMFPTR) io_Examine,
	(TMFPTR) io_ExNext,
	(TMFPTR) io_ChangeDir,
	(TMFPTR) io_ParentDir,
	(TMFPTR) io_NameOf,
	(TMFPTR) io_DupLock,
	(TMFPTR) io_OpenFromLock,
	(TMFPTR) io_AddPart,
	(TMFPTR) io_AssignLate,
	(TMFPTR) io_AssignLock,
	(TMFPTR) io_Rename,
	(TMFPTR) io_MakeDir,
	(TMFPTR) io_Delete,
	(TMFPTR) io_SetIOErr,
	(TMFPTR) io_GetIOErr,
	(TMFPTR) io_ObtainMsg,
	(TMFPTR) io_ReleaseMsg,
	(TMFPTR) io_Fault,
	(TMFPTR) io_WaitChar,
	(TMFPTR) io_IsInteractive,
	(TMFPTR) io_OutputFH,
	(TMFPTR) io_InputFH,
	(TMFPTR) io_ErrorFH,
	(TMFPTR) io_MakeName,
	(TMFPTR) io_Mount,
	(TMFPTR) io_FUngetC,
	(TMFPTR) io_FPutS,
	(TMFPTR) io_FGetS,
	(TMFPTR) io_SetDate,
};

/*****************************************************************************/
/*
**	mod_exit
*/

static void
io_exit(TMOD_IO *io)
{
	TAPTR exec = TGetExecBase(io);
	struct TDeviceNode *dn;
	struct TIOPacket *iomsg;

	/* close standard file handles */

	while ((iomsg = (struct TIOPacket *) TRemHead(&io->stdfhlist)))
	{
		TDBPRINTF(4,("closing fh in stdfhlist: %p\n", iomsg));
		io_CloseFile(io, iomsg);
	}

	/* release current directory locks */

	while ((iomsg = (struct TIOPacket *) TRemHead(&io->cdlocklist)))
	{
		TDBPRINTF(4,("releasing lock in cdlocklist: %p\n", iomsg));
		io_UnlockFile(io, iomsg);
	}

	/* release assigns and mounts */

	while ((dn = (struct TDeviceNode *) TRemHead(&io->devicelist)))
	{
		TDBPRINTF(4,("releasing device list entry: %s\n", dn->name));
		io_UnlockFile(io, dn->lock);
		TExecFree(exec, dn);
	}
}

/*****************************************************************************/
/*
**	len = io_strlen(string)
**	Return length of a string
*/

LOCAL TINT
io_strlen(TSTRPTR s)
{
	if (s)
	{
		TSTRPTR p = s;
		while (*p) p++;
		return (TINT) (p - s);
	}
	return 0;
}

/*****************************************************************************/
/*
**	strptr = io_strchr(string, char)
**	Find a character in a string
*/

LOCAL TSTRPTR
io_strchr(TSTRPTR s, TINT c)
{
	TINT d;
	while ((d = *s))
	{
		if (d == c)
			return s;
		s++;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	p = io_strcpy(dest, source)
**	Copy string
*/

LOCAL TSTRPTR
io_strcpy(TSTRPTR d, TSTRPTR s)
{
	if (s && d)
	{
		TSTRPTR p = d;
		while ((*d++ = *s++));
		return p;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	s2 = io_strdup(iobase, mmu, string)
**	Duplicate string. Must be freed using TFree()
*/

LOCAL TSTRPTR
io_strdup(TMOD_IO *io, TAPTR mmu, TSTRPTR s)
{
	TUINT l = io_strlen(s);
	TAPTR exec = TGetExecBase(io);
	TSTRPTR s2 = TExecAlloc(exec, mmu, l + 1);
	if (s2)
	{
		if (l)
			TExecCopyMem(exec, s, s2, l);
		s2[l] = 0;
	}
	return s2;
}

/*****************************************************************************/
/*
**	result = io_strncasecmp(string1, string2, count)
**	Compare characters of strings without regard to case, length-limited,
**	with slightly extended semantics: Either or both strings may be TNULL
**	pointers. a TNULL string is 'less than' a non-TNULL string.
*/

LOCAL TINT
io_strncasecmp(TSTRPTR s1, TSTRPTR s2, TINT count)
{
	if (s1)
	{
		if (s2)
		{
			TINT t1 = *s1, t2 = *s2;
			TINT c1, c2;

			if (t1 >= 'a' && t1 <= 'z')
				t1 -= 'a' - 'A';
			if (t2 >= 'a' && t2 <= 'z')
				t2 -= 'a' - 'A';

			do
			{
				if ((c1 = t1))
				{
					t1 = *s1++;
					if (t1 >= 'a' && t1 <= 'z')
						t1 -= 'a' - 'A';
				}

				if ((c2 = t2))
				{
					t2 = *s2++;
					if (t2 >= 'a' && t2 <= 'z')
						t2 -= 'a' - 'A';
				}

				if (!c1 || !c2)
					break;

			} while (count-- && c1 == c2);

			return ((TINT) c1 - (TINT) c2);
		}

		return 1;
	}

	if (s2)
		return -1;

	return 0;
}
