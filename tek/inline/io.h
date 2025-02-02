#ifndef _TEK_INLINE_IO_H
#define _TEK_INLINE_IO_H

/*
**	$Id: io.h $
**	teklib/tek/inline/io.h - io inline calls
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/proto/io.h>

#define TLockFile(name,mode,tags) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TSTRPTR,TUINT,TTAGITEM *))(TIOBase))[-9]))(TIOBase,name,mode,tags)

#define TUnlockFile(lock) \
	(*(((TMODCALL void(**)(TAPTR,TFILE *))(TIOBase))[-10]))(TIOBase,lock)

#define TOpenFile(name,mode,tags) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TSTRPTR,TUINT,TTAGITEM *))(TIOBase))[-11]))(TIOBase,name,mode,tags)

#define TCloseFile(a) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TFILE *))(TIOBase))[-12]))(TIOBase,a)

#define TRead(a,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TAPTR,TINT))(TIOBase))[-13]))(TIOBase,a,buf,len)

#define TWrite(a,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TAPTR,TINT))(TIOBase))[-14]))(TIOBase,a,buf,len)

#define TFlush(a) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TFILE *))(TIOBase))[-15]))(TIOBase,a)

#define TSeek(a,offs,offshi,mode) \
	(*(((TMODCALL TUINT(**)(TAPTR,TFILE *,TINT,TINT *,TINT))(TIOBase))[-16]))(TIOBase,a,offs,offshi,mode)

#define TFPutC(a,c) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TINT))(TIOBase))[-17]))(TIOBase,a,c)

#define TFGetC(a) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *))(TIOBase))[-18]))(TIOBase,a)

#define TFEoF(a) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TFILE *))(TIOBase))[-19]))(TIOBase,a)

#define TFRead(a,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TAPTR,TINT))(TIOBase))[-20]))(TIOBase,a,buf,len)

#define TFWrite(a,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TAPTR,TINT))(TIOBase))[-21]))(TIOBase,a,buf,len)

#define TExamine(a,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TTAGITEM *))(TIOBase))[-22]))(TIOBase,a,tags)

#define TExNext(a,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TTAGITEM *))(TIOBase))[-23]))(TIOBase,a,tags)

#define TChangeDir(a) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TFILE *))(TIOBase))[-24]))(TIOBase,a)

#define TParentDir(a) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TFILE *))(TIOBase))[-25]))(TIOBase,a)

#define TNameOf(a,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TSTRPTR,TINT))(TIOBase))[-26]))(TIOBase,a,buf,len)

#define TDupLock(a) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TFILE *))(TIOBase))[-27]))(TIOBase,a)

#define TOpenFromLock(a) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TFILE *))(TIOBase))[-28]))(TIOBase,a)

#define TAddPart(p1,p2,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR,TSTRPTR,TINT))(TIOBase))[-29]))(TIOBase,p1,p2,buf,len)

#define TAssignLate(name,path) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TSTRPTR))(TIOBase))[-30]))(TIOBase,name,path)

#define TAssignLock(name,lock) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TFILE *))(TIOBase))[-31]))(TIOBase,name,lock)

#define TRename(name,newname) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TSTRPTR))(TIOBase))[-32]))(TIOBase,name,newname)

#define TMakeDir(name,tags) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TTAGITEM *))(TIOBase))[-33]))(TIOBase,name,tags)

#define TDeleteFile(name) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR))(TIOBase))[-34]))(TIOBase,name)

#define TSetIOErr(newerr) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(TIOBase))[-35]))(TIOBase,newerr)

#define TGetIOErr() \
	(*(((TMODCALL TINT(**)(TAPTR))(TIOBase))[-36]))(TIOBase)

#define TObtainPacket(path,namepart) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TSTRPTR *))(TIOBase))[-37]))(TIOBase,path,namepart)

#define TReleasePacket(packet) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(TIOBase))[-38]))(TIOBase,packet)

#define TFault(err,buf,len,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TSTRPTR,TINT,TTAGITEM *))(TIOBase))[-39]))(TIOBase,err,buf,len,tags)

#define TWaitChar(a,timeout) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TFILE *,TINT))(TIOBase))[-40]))(TIOBase,a,timeout)

#define TIsInteractive(a) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TFILE *))(TIOBase))[-41]))(TIOBase,a)

#define TOutputFH() \
	(*(((TMODCALL TFILE *(**)(TAPTR))(TIOBase))[-42]))(TIOBase)

#define TInputFH() \
	(*(((TMODCALL TFILE *(**)(TAPTR))(TIOBase))[-43]))(TIOBase)

#define TErrorFH() \
	(*(((TMODCALL TFILE *(**)(TAPTR))(TIOBase))[-44]))(TIOBase)

#define TMakeName(name,dest,dlen,mode,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR,TINT,TINT,TTAGITEM *))(TIOBase))[-45]))(TIOBase,name,dest,dlen,mode,tags)

#define TMount(name,action,tags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TINT,TTAGITEM *))(TIOBase))[-46]))(TIOBase,name,action,tags)

#define TFUngetC(a,c) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TINT))(TIOBase))[-47]))(TIOBase,a,c)

#define TFPutS(a,s) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TSTRPTR))(TIOBase))[-48]))(TIOBase,a,s)

#define TFGetS(a,buf,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TFILE *,TSTRPTR,TINT))(TIOBase))[-49]))(TIOBase,a,buf,len)

#define TSetFileDate(name,date,tags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TDATE *,TTAGITEM *))(TIOBase))[-50]))(TIOBase,name,date,tags)

#endif /* _TEK_INLINE_IO_H */
