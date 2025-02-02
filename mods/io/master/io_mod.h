
#ifndef _TEK_MOD_IO_MOD_H
#define _TEK_MOD_IO_MOD_H

/* 
**	$Id: io_mod.h,v 1.5 2005/09/08 03:31:19 tmueller Exp $
**	IO module internal definitions
*/

#include <tek/mod/io.h>
#include <tek/mod/ioext.h>
#include <tek/mod/exec.h>
#include <tek/inline/exec.h>
#include <tek/debug.h>
#include <tek/teklib.h>

#define MOD_VERSION			0
#define MOD_REVISION		3
#define MOD_NUMVECTORS		50

/*****************************************************************************/

typedef struct TIOPacket TIOMSG;

typedef struct
{
	struct TModule module;	/* module header */

	TAPTR modlock;			/* module base lock */
	TUINT refcount;			/* module reference counter */
	TAPTR mmu;				/* module's memory manager */

	TLIST devicelist;		/* list of handlers and assigns */
	TLIST cdlocklist;		/* currentdir locks that we maintain */
	TLIST stdfhlist;		/* list of standard filehandles */

} TMOD_IO;

#define TExecBase TGetExecBase(io)

/*
**	iobuffer
*/

typedef struct
{
	TUINT mode;
	TUINT size;
	TINT pos;
	TINT fill;

} TIOBUF;

/* 
**	names
*/

#define IOHNDNAME			"iohnd_"		/* module name prefix */
#define IOHNDNAME_LEN		6
#define IOHNDNAME_DEFAULT	"default"		/* name of the default handler */
#define IOHNDNAME_DEFLEN	7

/*****************************************************************************/
/*
**	devicenode structure. currently supported types and flags:
**
**	TDEVF_HANDLER	- identifies a handler (can reside on a physical
**		device; a link to it would be required in devnode->device).
**		a handler implements or is an abstraction from a file system
**		(or even a device, in some cases).
**	TDEVF_ASSIGN	- named alias to a directory.
**	TDEVF_DEFER		- late-binding; lock is TNULL while set
*/

struct TDeviceNode
{
	TNODE node;
	TSTRPTR name;			/* device entry name (without colon) */
	TSTRPTR altname;		/* assigned path/name (with colon) */
	TIOMSG *lock;			/* device root (handlers) or directory (assigns) */
	TAPTR device;			/* link to a exec device */
	TUINT flags;			/* see below */
};

#define TDEVF_MOUNT		0x0001
#define TDEVF_ASSIGN	0x0002		/* e.g. "sys" -> altname = ":opt/tek" */
#define TDEVF_DEFER		0x0004		/* late-binding; should work for both assigns and handlers */

/*****************************************************************************/
/* 
**	module API
*/

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

#ifndef LOCAL
#define LOCAL
#endif

/* utils */

LOCAL TINT io_strlen(TSTRPTR s);
LOCAL TSTRPTR io_strchr(TSTRPTR s, TINT c);
LOCAL TSTRPTR io_strcpy(TSTRPTR d, TSTRPTR s);
LOCAL TSTRPTR io_strdup(TMOD_IO *io, TAPTR mmu, TSTRPTR s);
LOCAL TINT io_strncasecmp(TSTRPTR s1, TSTRPTR s2, TINT count);

/* names */

EXPORT TIOMSG *io_obtainmsg(TMOD_IO *io, TSTRPTR name, TSTRPTR *namepart);
EXPORT TVOID io_releasemsg(TMOD_IO *io, TIOMSG *iomsg);
EXPORT TINT io_addpart(TMOD_IO *io, TSTRPTR path1, TSTRPTR path2, TSTRPTR buf, TINT buflen);
EXPORT TBOOL io_assignlate(TMOD_IO *io, TSTRPTR name, TSTRPTR path);
EXPORT TBOOL io_assignlock(TMOD_IO *io, TSTRPTR name, TIOMSG *lock);
LOCAL TIOMSG *io_refhandler(TMOD_IO *io, TIOMSG *iomsg);
EXPORT TINT io_makename(TMOD_IO *io, TSTRPTR name, TSTRPTR destbuf, TINT destlen, TINT mode, TTAGITEM *tags);
EXPORT TBOOL io_mount(TMOD_IO *io, TSTRPTR name, TINT action, TTAGITEM *tags);

/* filelock */

EXPORT TAPTR io_duplock(TMOD_IO *io, TIOMSG *lock);
EXPORT TAPTR io_parentdir(TMOD_IO *io, TIOMSG *lock);
EXPORT TAPTR io_lock(TMOD_IO *io, TSTRPTR name, TUINT mode, TTAGITEM *tags);
EXPORT TVOID io_unlock(TMOD_IO *io, TIOMSG *lock);
EXPORT TAPTR io_openfromlock(TMOD_IO *io, TIOMSG *lock);
EXPORT TINT io_examine(TMOD_IO *io, TIOMSG *iomsg, TTAGITEM *tags);
EXPORT TINT io_exnext(TMOD_IO *io, TIOMSG *iomsg, TTAGITEM *tags);
EXPORT TINT io_nameof(TMOD_IO *io, TIOMSG *iomsg, TSTRPTR buf, TINT len);
EXPORT TIOMSG *io_changedir(TMOD_IO *io, TIOMSG *newcd);
EXPORT TBOOL io_rename(TMOD_IO *io, TSTRPTR oldname, TSTRPTR newname);
EXPORT TAPTR io_makedir(TMOD_IO *io, TSTRPTR name, TTAGITEM *tags);
EXPORT TBOOL io_delete(TMOD_IO *io, TSTRPTR name);
EXPORT TAPTR io_outputfh(TMOD_IO *io);
EXPORT TAPTR io_inputfh(TMOD_IO *io);
EXPORT TAPTR io_errorfh(TMOD_IO *io);
EXPORT TBOOL io_setdate(TMOD_IO *io, TSTRPTR name, TDATE *date);

/* standard */

EXPORT TINT io_seterr(TMOD_IO *io, TINT errorcode);
EXPORT TINT io_ioerr(TMOD_IO *io);
EXPORT TAPTR io_open(TMOD_IO *io, TSTRPTR name, TUINT mode, TTAGITEM *tags);
EXPORT TBOOL io_close(TMOD_IO *io, TIOMSG *fh);
EXPORT TINT io_read(TMOD_IO *io, TIOMSG *iomsg, TAPTR buf, TINT len);
EXPORT TINT io_write(TMOD_IO *io, TIOMSG *iomsg, TAPTR buf, TINT len);
EXPORT TUINT io_seek(TMOD_IO *io, TIOMSG *iomsg, TINT offs, TINT *offs_hi, TINT mode);
EXPORT TBOOL io_flush(TMOD_IO *io, TIOMSG *iomsg);
EXPORT TINT io_fputc(TMOD_IO *io, TIOMSG *iomsg, TINT c);
EXPORT TINT io_fgetc(TMOD_IO *io, TIOMSG *iomsg);
EXPORT TBOOL io_feof(TMOD_IO *io, TIOMSG *iomsg);
EXPORT TINT io_fread(TMOD_IO *io, TIOMSG *iomsg, TUINT8 *buf, TINT len);
EXPORT TINT io_fwrite(TMOD_IO *io, TIOMSG *iomsg, TUINT8 *buf, TINT len);
EXPORT TINT io_fault(TMOD_IO *io, TINT err, TSTRPTR buf, TINT buflen, TTAGITEM *tags);
EXPORT TBOOL io_waitchar(TMOD_IO *io, TIOMSG *file, TINT timeout);
EXPORT TBOOL io_isinteractive(TMOD_IO *io, TIOMSG *file);
EXPORT TINT io_fungetc(TMOD_IO *io, TIOMSG *iomsg, TINT c);
EXPORT TINT io_fputs(TMOD_IO *io, TIOMSG *iomsg, TSTRPTR s);
EXPORT TSTRPTR io_fgets(TMOD_IO *io, TIOMSG *iomsg, TSTRPTR buf, TINT len);

/*****************************************************************************/
/*
**	Revision History
**	$Log: io_mod.h,v $
**	Revision 1.5  2005/09/08 03:31:19  tmueller
**	strchr/strrchr char argument changed to TINT
**	
**	Revision 1.4  2005/09/08 00:01:37  tmueller
**	mod API extended; cleanup; util dependency removed
**	
**	Revision 1.3  2004/07/03 03:29:54  tmueller
**	added io_setdate()
**	
**	Revision 1.2  2003/12/12 15:03:57  tmueller
**	Updated minor version number
**	
**	Revision 1.1.1.1  2003/12/11 07:18:42  tmueller
**	Krypton import
**	
**	Revision 1.6  2003/07/11 19:36:14  tmueller
**	added io_fgets, io_fputs, io_fungetc
**	
**	Revision 1.5  2003/07/07 20:18:31  tmueller
**	Added io_fungetc()
**	
**	Revision 1.4  2003/05/11 14:11:34  tmueller
**	Updated I/O master and default implementations to extended definitions
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
**	Revision 1.8  2003/03/07 21:14:32  bifat
**	io_makename() added for conversion from HOST to TEK naming conventions.
**	Load of name handling problems have been fixed.
**	
**	Revision 1.7  2003/03/04 02:34:58  bifat
**	The return value of io_examine() and io_exnext() changed.
**	
**	Revision 1.6  2003/03/03 02:22:30  bifat
**	io_makedir() now returns a shared lock on the created directory
**	
**	Revision 1.5  2003/03/02 15:47:03  bifat
**	Some packet types extended with additional tag arguments. io_addpart() API
**	changed. io_lock(), io_open(), io_makedir() and io_fault() got additional
**	taglist arguments. Added TIOERR_LINE_TOO_LONG error code.
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
**	Revision 1.2  2003/01/21 22:28:02  tmueller
**	added io_waitchar, io_isinteractive, more packets
**	
**	Revision 1.1  2003/01/20 21:29:56  tmueller
**	added
**	
*/

#endif
