#ifndef _TEK_INLINE_IO_H
#define _TEK_INLINE_IO_H

/*
**	$Id: io.h,v 1.5.2.1 2005/12/04 22:30:14 tmueller Exp $
**	teklib/tek/inline/io.h - io inline calls
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/proto/io.h>

#define TLockFile(name,mode,tags) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TUINT,TTAGITEM *))(TIOBase))[-9]))(TIOBase,name,mode,tags)

#define TUnlockFile(lock) \
	(*(((TMODCALL TVOID(**)(TAPTR,TAPTR))(TIOBase))[-10]))(TIOBase,lock)

#define TOpenFile(name,mode,tags) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TUINT,TTAGITEM *))(TIOBase))[-11]))(TIOBase,name,mode,tags)

#define TCloseFile(fh) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR))(TIOBase))[-12]))(TIOBase,fh)

#define TRead(fh,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TAPTR,TINT))(TIOBase))[-13]))(TIOBase,fh,buf,len)

#define TWrite(fh,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TAPTR,TINT))(TIOBase))[-14]))(TIOBase,fh,buf,len)

#define TFlush(fh) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR))(TIOBase))[-15]))(TIOBase,fh)

#define TSeek(fh,offs,offshi,mode) \
	(*(((TMODCALL TUINT(**)(TAPTR,TAPTR,TINT,TINT *,TINT))(TIOBase))[-16]))(TIOBase,fh,offs,offshi,mode)

#define TFPutC(fh,c) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TINT))(TIOBase))[-17]))(TIOBase,fh,c)

#define TFGetC(fh) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR))(TIOBase))[-18]))(TIOBase,fh)

#define TFEoF(fh) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR))(TIOBase))[-19]))(TIOBase,fh)

#define TFRead(fh,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TAPTR,TINT))(TIOBase))[-20]))(TIOBase,fh,buf,len)

#define TFWrite(fh,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TAPTR,TINT))(TIOBase))[-21]))(TIOBase,fh,buf,len)

#define TExamine(lock,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TTAGITEM *))(TIOBase))[-22]))(TIOBase,lock,tags)

#define TExNext(lock,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TTAGITEM *))(TIOBase))[-23]))(TIOBase,lock,tags)

#define TChangeDir(lock) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TAPTR))(TIOBase))[-24]))(TIOBase,lock)

#define TParentDir(lock) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TAPTR))(TIOBase))[-25]))(TIOBase,lock)

#define TNameOf(lock,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TSTRPTR,TINT))(TIOBase))[-26]))(TIOBase,lock,buf,len)

#define TDupLock(lock) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TAPTR))(TIOBase))[-27]))(TIOBase,lock)

#define TOpenFromLock(lock) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TAPTR))(TIOBase))[-28]))(TIOBase,lock)

#define TAddPart(p1,p2,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR,TSTRPTR,TINT))(TIOBase))[-29]))(TIOBase,p1,p2,buf,len)

#define TAssignLate(name,path) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TSTRPTR))(TIOBase))[-30]))(TIOBase,name,path)

#define TAssignLock(name,lock) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TAPTR))(TIOBase))[-31]))(TIOBase,name,lock)

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
	(*(((TMODCALL TVOID(**)(TAPTR,TAPTR))(TIOBase))[-38]))(TIOBase,packet)

#define TFault(err,buf,len,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TSTRPTR,TINT,TTAGITEM *))(TIOBase))[-39]))(TIOBase,err,buf,len,tags)

#define TWaitChar(fh,timeout) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR,TINT))(TIOBase))[-40]))(TIOBase,fh,timeout)

#define TIsInteractive(fh) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR))(TIOBase))[-41]))(TIOBase,fh)

#define TOutputFH() \
	(*(((TMODCALL TAPTR(**)(TAPTR))(TIOBase))[-42]))(TIOBase)

#define TInputFH() \
	(*(((TMODCALL TAPTR(**)(TAPTR))(TIOBase))[-43]))(TIOBase)

#define TErrorFH() \
	(*(((TMODCALL TAPTR(**)(TAPTR))(TIOBase))[-44]))(TIOBase)

#define TMakeName(name,dest,dlen,mode,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR,TINT,TINT,TTAGITEM *))(TIOBase))[-45]))(TIOBase,name,dest,dlen,mode,tags)

#define TMount(name,action,tags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TINT,TTAGITEM *))(TIOBase))[-46]))(TIOBase,name,action,tags)

#define TFUngetC(fh,c) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TINT))(TIOBase))[-47]))(TIOBase,fh,c)

#define TFPutS(fh,s) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TSTRPTR))(TIOBase))[-48]))(TIOBase,fh,s)

#define TFGetS(fh,buf,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TAPTR,TSTRPTR,TINT))(TIOBase))[-49]))(TIOBase,fh,buf,len)

#define TSetFileDate(name,date) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TDATE *))(TIOBase))[-50]))(TIOBase,name,date)

#endif /* _TEK_INLINE_IO_H */
