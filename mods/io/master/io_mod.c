
/* 
**	$Id: io_mod.c,v 1.5 2005/09/08 03:31:19 tmueller Exp $
**	IO module setup
*/

#include "io_mod.h"
#include <tek/mod/exec.h>

static TCALLBACK TMOD_IO *mod_open(TMOD_IO *mod, TAPTR task, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_IO *mod, TAPTR task);
static TCALLBACK TVOID mod_destroy(TMOD_IO *mod);
static const TAPTR mod_vectors[MOD_NUMVECTORS];
static TVOID mod_exit(TMOD_IO *mod);

/*****************************************************************************/
/*
**	module init function
*/

TMODENTRY TUINT
tek_init_io(TAPTR task, TMOD_IO *io, TUINT16 version, TTAGITEM *tags)
{
	if (io == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * MOD_NUMVECTORS;	/* negative size */

		if (version <= MOD_VERSION)
			return sizeof(TMOD_IO);	/* positive size */

		return 0;
	}

	io->modlock = TCreateLock(TNULL);
	if (io->modlock)
	{
		TInitList(&io->devicelist);
		TInitList(&io->cdlocklist);
		TInitList(&io->stdfhlist);
	
		io->module.tmd_Version = MOD_VERSION;
		io->module.tmd_Revision = MOD_REVISION;
		io->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		io->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;
		io->module.tmd_DestroyFunc = (TDFUNC) mod_destroy;
		io->module.tmd_Flags |= TMODF_EXTENDED;
		
		#ifdef TDEBUG
		io->mmu = TCreateMMU(TNULL, TMMUT_Tracking | TMMUT_TaskSafe, TNULL);
		#endif

		TInitVectors(io, (TAPTR *) mod_vectors, MOD_NUMVECTORS);
		return TTRUE;
	}

	return 0;
}

/*****************************************************************************/
/*
**	module exit function
*/

static TCALLBACK TVOID 
mod_destroy(TMOD_IO *mod)
{
	TDestroy(mod->mmu);
	TDestroy(mod->modlock);
}

/*****************************************************************************/
/*
**	instance open
*/

static TCALLBACK TMOD_IO *
mod_open(TMOD_IO *io, TAPTR task, TTAGITEM *tags)
{
	TLock(io->modlock);
	io->refcount++;
	TUnlock(io->modlock);
	return io;
}

/*****************************************************************************/
/*
**	instance close
*/

static TCALLBACK TVOID
mod_close(TMOD_IO *io, TAPTR task)
{
	TLock(io->modlock);
	if (--io->refcount == 0) mod_exit(io);
	TUnlock(io->modlock);
}

/*****************************************************************************/
/*
**	function vector table
*/

static const TAPTR
mod_vectors[MOD_NUMVECTORS] =
{
	(TAPTR) TNULL,		/* reserved */
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,
	(TAPTR) TNULL,

	(TAPTR) io_lock,
	(TAPTR) io_unlock,
	(TAPTR) io_open,
	(TAPTR) io_close,
	(TAPTR) io_read,
	(TAPTR) io_write,
	(TAPTR) io_flush,
	(TAPTR) io_seek,
	(TAPTR) io_fputc,
	(TAPTR) io_fgetc,
	(TAPTR) io_feof,
	(TAPTR) io_fread,
	(TAPTR) io_fwrite,
	(TAPTR) io_examine,
	(TAPTR) io_exnext,
	(TAPTR) io_changedir,
	(TAPTR) io_parentdir,
	(TAPTR) io_nameof,
	(TAPTR) io_duplock,
	(TAPTR) io_openfromlock,
	(TAPTR) io_addpart,
	(TAPTR) io_assignlate,
	(TAPTR) io_assignlock,
	(TAPTR) io_rename,
	(TAPTR) io_makedir,
	(TAPTR) io_delete,
	(TAPTR) io_seterr,
	(TAPTR) io_ioerr,
	(TAPTR) io_obtainmsg,
	(TAPTR) io_releasemsg,
	(TAPTR) io_fault,
	(TAPTR) io_waitchar,
	(TAPTR) io_isinteractive,
	(TAPTR) io_outputfh,
	(TAPTR) io_inputfh,
	(TAPTR) io_errorfh,
	(TAPTR) io_makename,
	(TAPTR) io_mount,
	(TAPTR) io_fungetc,
	(TAPTR) io_fputs,
	(TAPTR) io_fgets,
	(TAPTR) io_setdate,
};

/*****************************************************************************/
/*
**	mod_exit
*/

static TVOID
mod_exit(TMOD_IO *io)
{
	TIOMSG *iomsg;
	struct TDeviceNode *dn;

	/* close standard file handles */
	
	while ((iomsg = (TIOMSG *) TRemHead(&io->stdfhlist)))
	{
		tdbprintf1(5,"closing fh in stdfhlist: %08x\n", (TUINT)(TUINTPTR) iomsg);
		io_close(io, iomsg);
	}

	/* release current directory locks */
	
	while ((iomsg = (TIOMSG *) TRemHead(&io->cdlocklist)))
	{
		tdbprintf1(5,"releasing lock in cdlocklist: %08x\n", (TUINT)(TUINTPTR) iomsg);
		io_unlock(io, iomsg);
	}

	/* release assigns and mounts */

	while ((dn = (struct TDeviceNode *) TRemHead(&io->devicelist)))
	{
		tdbprintf1(5,"releasing device list entry: %s\n", dn->name);
		io_unlock(io, dn->lock);
		TFree(dn);		
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
		if (d == c) return s;
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
	TSTRPTR s2 = TAlloc(mmu, l + 1);
	if (s2)
	{
		if (l) TCopyMem(s, s2, l);
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
			TINT8 t1 = *s1, t2 = *s2;
			TINT8 c1, c2;

			if (t1 >= 'a' && t1 <= 'z') t1 -= 'a' - 'A';
			if (t2 >= 'a' && t2 <= 'z') t2 -= 'a' - 'A';

			do
			{
				if ((c1 = t1))
				{
					t1 = *s1++;
					if (t1 >= 'a' && t1 <= 'z') t1 -= 'a' - 'A';
				}

				if ((c2 = t2))
				{
					t2 = *s2++;
					if (t2 >= 'a' && t2 <= 'z') t2 -= 'a' - 'A';
				}

				if (!c1 || !c2) break;

			} while (count-- && c1 == c2);

			return ((TINT) c1 - (TINT) c2);
		}

		return 1;
	}

	if (s2) return -1;

	return 0;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: io_mod.c,v $
**	Revision 1.5  2005/09/08 03:31:19  tmueller
**	strchr/strrchr char argument changed to TINT
**	
**	Revision 1.4  2005/09/08 00:01:37  tmueller
**	mod API extended; cleanup; util dependency removed
**	
**	Revision 1.3  2004/07/03 03:29:54  tmueller
**	added io_setdate()
**	
**	Revision 1.2  2004/04/18 14:11:42  tmueller
**	TTAG changed from TAPTR to TUINTPTR; atomdata changed from TAPTR to TTAG
**	
**	Revision 1.1.1.1  2003/12/11 07:18:41  tmueller
**	Krypton import
**	
**	Revision 1.6  2003/10/19 03:38:18  tmueller
**	Changed exec structure field names: Node, List, Handle, Module, ModuleEntry
**	
**	Revision 1.5  2003/10/18 21:17:37  tmueller
**	Adapted to the changed mod->handle field names
**	
**	Revision 1.4  2003/07/11 19:36:14  tmueller
**	added io_fgets, io_fputs, io_fungetc
**	
**	Revision 1.3  2003/07/07 20:18:31  tmueller
**	Added io_fungetc()
**	
**	Revision 1.2  2003/05/11 14:11:34  tmueller
**	Updated I/O master and default implementations to extended definitions
**	
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**	
**	Revision 1.7  2003/03/07 21:14:32  bifat
**	io_makename() added for conversion from HOST to TEK naming conventions.
**	Load of name handling problems have been fixed.
**	
**	Revision 1.6  2003/03/01 05:25:14  bifat
**	POSIX implementation of the default IO handler now works with Elate, too
**	
**	Revision 1.5  2003/02/09 13:53:20  tmueller
**	added io_outputfh and io_errorfh, added automatic closedown of stdio
**	filehandles
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
