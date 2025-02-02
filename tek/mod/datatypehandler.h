
#ifndef _TEK_MOD_DATATYPES_H
#define	_TEK_MOD_DATATYPES_H

#include <tek/exec.h>

typedef struct
{
	TAPTR dth;
	TAPTR modul;
} datatypemodule;

/* class types */
enum { DTCLASS_PIC, DTCLASS_TEXT, DTCLASS_SOUND, DTCLASS_ANIM, DTCLASS_EXTENDED, DTCLASS_NONE };


/*	identify commands
	-----------------

	DTIC_USESUFFIX: if set, the suffix also will be used for identification
	DTIC_TOP:		move to top of file
	DTIC_END:		move to end of file
	DTIC_MOVE,num:	move number of bytes from current position in file
	DTIC_BYTE,byte:	compare with following byte
	DTIC_ORBYTE,numbytes,byte,byte,... : compare with this and also the following byte(s)
	DTIC_BYTES,numbytes,byte,byte,... : compare with number of following bytes
	DTIC_ORBYTES,numseqs,numbytes,byte,byte,... : compare with number of following bytesequences
	DTIC_SCANBYTES,numbytes,byte,byte,... : search for bytes from current position in file
	DTIC_DONE:		end of commandlist
*/
enum { DTIC_USESUFFIX, DTIC_TOP, DTIC_END, DTIC_MOVE, DTIC_BYTE, DTIC_ORBYTE, DTIC_BYTES, DTIC_ORBYTES, DTIC_SCANBYTES, DTIC_DONE };

/* struct containing the Identifydata */
typedef struct _DTIdentifyData
{
	TINT8 *datatypecodecname;
	TINT8 datatypeclass;
	TINT8 *datatypeidentdata;
	TINT8 *datatypesuffix;
	TINT8 *datatypefullname;
	TINT8 *datatypeshortname;
	TBOOL canwrite;

}DTIdentifyData;

/* struct containing the datatype description for listdatatypes */
typedef struct _DTDescData
{
	TINT8 *datatypecodecname;
	TINT8 *datatypefullname;
	TINT8 *datatypeshortname;
	TINT8 *datatypesuffix;
	TINT8 datatypeclass;
	TBOOL canwrite;
	TINT dtnum;

}DTDescData;

typedef struct _DTListItem
{
	struct TNode li_Node;		/* Node header */
	DTDescData *li_ddat;		/* identify data */
	TDFUNC li_DestroyFunc;		/* Destroy function for this handle */
	TAPTR li_ExecBase;			/* Exec base pointer */
	
}DTListItem;

/*	Tags for TDTHOpen
	----------------- */
#define TDOPENTAG_FILEHANDLE	(TTAG_USER+1)
#define TDOPENTAG_DTNAME		(TTAG_USER+2)

/*	Tags for getattrs 
	----------------- */
/* basic Tags */
#define TDTAG_CLASS				(TTAG_USER+1)		/* object class, i.e. picture or sound */
#define TDTAG_FULLNAME			(TTAG_USER+2)		/* full name of object type */
#define TDTAG_SHORTNAME			(TTAG_USER+3)		/* the short name, i.e. "ilbm" for a IFF picture */
#define TDTAG_COMMENT			(TTAG_USER+4)		/* file comment */
#define TDTAG_NUMSUBCLASSES		(TTAG_USER+5)		/* number of subclasses for compounded datatypes, i.e. animation with sound */
#define TDTAG_CANSAVE			(TTAG_USER+6)		/* indicates if datatype can write data */
#define TDTAG_COMPRESSION		(TTAG_USER+7)		/* maximum value for compression, if 0 no compression is supported */

/* attributes that may show up during operation */
#define TDTAG_END_OF_STREAM		(TTAG_USER+128)		/* End of stream detected */

/* picture Tags */
#define TDTAG_PICWIDTH			(TTAG_USER+256)
#define TDTAG_PICHEIGHT			(TTAG_USER+257)
#define TDTAG_PICDEPTH			(TTAG_USER+258)
#define TDTAG_PICBYTESPERROW	(TTAG_USER+259)
#define TDTAG_PICPIXELWIDTH		(TTAG_USER+260)
#define TDTAG_PICPIXELHEIGHT	(TTAG_USER+261)
#define TDTAG_PICFORMAT			(TTAG_USER+262)

/* more picture tags for writing pictures */
#define TDTAG_SAVEPICMAXWIDTH	(TTAG_USER+289)		/* maximum width supported */
#define TDTAG_SAVEPICMAXHEIGHT	(TTAG_USER+291)		/* maximum height supported */
#define TDTAG_SAVEPICMINDEPTH	(TTAG_USER+292)		/* minimum color depth (bitwise) supported */
#define TDTAG_SAVEPICMAXDEPTH	(TTAG_USER+293)		/* maximum color depth (bitwise) supported */

/* text Tags */
#define TDTAG_TEXTROWS			(TTAG_USER+512)

/* sound Tags */
#define TDTAG_SOUND_LENGTH		(TTAG_USER+768)		/* Number of samples */
#define TDTAG_SOUND_RATE		(TTAG_USER+769)		/* Hertz */
#define TDTAG_SOUND_CHANNELS	(TTAG_USER+770)		/* Number of channels */
#define TDTAG_SOUND_FORMAT		(TTAG_USER+771)		/* As defined in <tek/mod/audio.h> */

/* anim Tags */
#define TDTAG_ANIMTYP			(TTAG_USER+1024)
#define TDTAG_ANIMNUMFRAMES		(TTAG_USER+1025)
#define TDTAG_ANIMFRAMETIME		(TTAG_USER+1026)

/* base for extended datatypes */
#define TDTAG_USER				(TTAG_USER+2048)


/*	Flags for TDTAG_ANIMTYP
	----------------------- */
#define DTANIM_NODELTA			0x00000001
#define DTANIM_SINGLEDELTA		0x00000002
#define DTANIM_DOUBLEDELTA		0x00000003


/*	Tags for domethod
	----------------- */

/* basic Tags */
#define TDOTAG_GETDATATYP		(TTAG_USER+32768)
#define TDOTAG_GETDATA			(TTAG_USER+32769)

#define TDOTAG_SETDATATYP		(TTAG_USER+49152)
#define TDOTAG_SETDATA			(TTAG_USER+49153)
#define TDOTAG_COMPRESSION		(TTAG_USER+49154)
#define TDOTAG_WRITEFILE		(TTAG_USER+49155)

#define TDOTAG_RESTART			(TTAG_USER+49156)

/* extra picture Tags */
#define TDOTAG_GETPALETTE		(TTAG_USER+33024)

/* extra anim Tags */
#define TDOTAG_ANIMSTEP			(TTAG_USER+34048)

/* extra sound Tags */
#define TDOTAG_NUMSAMPLES		(TTAG_USER+36864)


/*	Flags for TDOTAG_ANIMSTEP
	------------------------- */
#define DTANIM_FORWARD			0x00000001
#define DTANIM_REWIND			0x00000002

/*	Tags for listdatatypes
	---------------------- */

#define TDFILTER_CLASS			(TTAG_USER+33792)
#define TDFILTER_SHORTNAME		(TTAG_USER+33793)
#define TDFILTER_CANSAVE		(TTAG_USER+33794)

#if 0
/*
**	convenience macros
*/

#define dth_new(exec,tags)		TOpenModule(exec, "datatypehandler", 0, tags)
#define dth_destroy(exec,dth)	TCloseModule(exec, dth)
#endif

#endif

