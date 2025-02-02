#ifndef _TEK_ANSICALL_STORAGEMANAGER_H
#define _TEK_ANSICALL_STORAGEMANAGER_H

/*
**	$Id: storagemanager.h,v 1.2 2005/09/13 02:44:20 tmueller Exp $
**	teklib/tek/ansicall/storagemanager.h - storagemanager module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define storagemanager_registerClass(storagemanager,task,instanceSize,constructor,destructor,extID,initialCapacity,numElementsGrow) \
	(*(((TMODCALL TSTORAGE(**)(TAPTR,TAPTR,TUINT,TAPTR,TAPTR,TUINT,TUINT,TUINT))(storagemanager))[-1]))(storagemanager,task,instanceSize,constructor,destructor,extID,initialCapacity,numElementsGrow)

#define storagemanager_destroyClass(storagemanager,task,class) \
	(*(((TMODCALL TVOID(**)(TAPTR,TAPTR,TSTORAGE))(storagemanager))[-2]))(storagemanager,task,class)

#define storagemanager_openClass(storagemanager,task,class,mode) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TAPTR,TSTORAGE,TUINT))(storagemanager))[-3]))(storagemanager,task,class,mode)

#define storagemanager_closeClass(storagemanager,task,class) \
	(*(((TMODCALL TVOID(**)(TAPTR,TAPTR,TSTORAGE))(storagemanager))[-4]))(storagemanager,task,class)

#define storagemanager_setClassElementsCount(storagemanager,task,class,count) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TAPTR,TSTORAGE,TUINT))(storagemanager))[-5]))(storagemanager,task,class,count)

#define storagemanager_getClassElementsCount(storagemanager,task,class) \
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR,TSTORAGE))(storagemanager))[-6]))(storagemanager,task,class)

#endif /* _TEK_ANSICALL_STORAGEMANAGER_H */
