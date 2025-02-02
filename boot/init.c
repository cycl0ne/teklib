
/*
**	$Id: init.c,v 1.8 2005/09/13 02:41:00 tmueller Exp $
**	teklib/boot/init.c - TEKlib startup procedure
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "init.h"

/*****************************************************************************/
/*
**	initappatoms - application-specific properties
*/

static TAPTR
newatom(TAPTR exec, TSTRPTR name, TAPTR ptr)
{
	TAPTR atom = TExecLockAtom(exec, name, TATOMF_CREATE | TATOMF_NAME);
	if (atom)
	{
		TExecSetAtomData(exec, atom, (TTAG) ptr);
		TExecUnlockAtom(exec, atom, TATOMF_KEEP);
	}
	return atom;
}

static TINT
addatomfromtag(TAPTR exec, TAPTR *atomp, TSTRPTR name, TTAG data)
{
	if (data)
	{
		*atomp = newatom(exec, name, (TAPTR) data);
		if (*atomp == TNULL) return 1;		
	}
	else
	{
		*atomp = TNULL;
	}
	return 0;
}

static TBOOL
initappatoms(INITDATA *data, TTAGITEM *tags)
{
	TAPTR exec = data->execbase;
	TINT error = 0;

	error += addatomfromtag(exec, &data->atom_imods, "sys.imods",
		TGetTag(tags, TExecBase_ModInit, TNULL));
	error += addatomfromtag(exec, &data->atom_argv, "sys.argv",
		TGetTag(tags, TExecBase_ArgV, TNULL));
	error += addatomfromtag(exec, &data->atom_args, "sys.arguments",
		TGetTag(tags, TExecBase_Arguments, TNULL));
	error += addatomfromtag(exec, &data->atom_progname, "sys.progname",
		TGetTag(tags, TExecBase_ProgName, TNULL));
	error += addatomfromtag(exec, &data->atom_retvalp, "sys.returnvalue",
		TGetTag(tags, TExecBase_RetValP, TNULL));
	error += ((data->atom_unique = 
		newatom(exec, "sys.uniqueid", &data->uniqueID)) == TNULL);

	return (TBOOL) (error == 0);
}

/*****************************************************************************/
/*
**	halbase = openhalmodule(init, tags)
**	closehalmodule(init)
*/

static TAPTR
openhalmodule(INITDATA *data, TTAGITEM *usertags)
{
	data->halbase = TNULL;
	data->halhalmod = boot_loadmodule(data->boot, data->progdir,
		data->moddir, "hal", usertags);
	if (data->halhalmod)
	{
		/* get HAL module entrypoint */
		data->halentry = boot_getentry(data->boot, data->halhalmod,
			"tek_init_hal");
		if (data->halentry)
		{
			/* prefer user tags */
			data->haltags[0].tti_Tag = TTAG_GOSUB;
			data->haltags[0].tti_Value = (TTAG) usertags;

			/* ptr to HAL module base */
			data->haltags[1].tti_Tag = TExecBase_HAL;
			data->haltags[1].tti_Value = (TTAG) &data->halbase;

			/* applicaton path */
			data->haltags[2].tti_Tag = TExecBase_ProgDir;
			data->haltags[2].tti_Value = (TTAG) data->progdir;

			/* TEKlib system directory */
			data->haltags[3].tti_Tag = TExecBase_SysDir;
			data->haltags[3].tti_Value = (TTAG) data->sysdir;

			/* TEKlib module directory */
			data->haltags[4].tti_Tag = TExecBase_ModDir;
			data->haltags[4].tti_Value = (TTAG) data->moddir;
			
			/* Boot handle */
			data->haltags[5].tti_Tag = TExecBase_BootHnd;
			data->haltags[5].tti_Value = (TTAG) data->boot;
			
			data->haltags[6].tti_Tag = TTAG_DONE;

			/* call module entry. this will place a pointer to the HALbase
			into the variable being pointed to by TExecBase_HAL */
			
			boot_callmod(data->boot, data->halhalmod, data->halentry, TNULL,
				(TAPTR) 1, 0, data->haltags);
			if (data->halbase) return data->halbase;
		}
		boot_closemodule(data->boot, data->halhalmod);
	}
	return TNULL;
}

static TVOID
closehalmodule(INITDATA *data)
{
	TDestroy(data->halbase);
	boot_closemodule(data->boot, data->halhalmod);
}

/*****************************************************************************/
/*
**	execbase = openexecmodule(init, tags)
**	closeexecmodule(init)
*/

static TAPTR 
openexecmodule(INITDATA *data, TTAGITEM *usertags)
{
	/* load exec module */
	data->halexecmod = boot_loadmodule(data->boot, data->progdir,
		data->moddir, "exec", usertags);
	if (data->halexecmod)
	{
		/* get exec module entrypoint */
		data->execentry = boot_getentry(data->boot, data->halexecmod,
			"tek_init_exec");
		if (data->execentry)
		{
			TUINT psize, nsize;
			TUINT16 version;
			struct TModule *execbase;
		
			/* get version and size */
			version = (TUINT16) TGetTag(data->haltags,
				TExecBase_Version, 0);
			psize = boot_callmod(data->boot, data->halexecmod,
				data->execentry, TNULL, TNULL, version, data->haltags);
			nsize = boot_callmod(data->boot, data->halexecmod, data->execentry,
				TNULL, TNULL, 0xffff, data->haltags);

			/* get memory for the exec module */
			execbase = boot_alloc(data->boot, nsize + psize);
			if (execbase)
			{
				/* initialize execbase */
				execbase = (struct TModule *) (((TINT8 *) execbase) + nsize);

				if (boot_callmod(data->boot, data->halexecmod, data->execentry,
					TNULL, execbase, 0, data->haltags))
				{
					data->execbase = execbase;
					return execbase;
				}
				boot_free(data->boot, ((TINT8 *) execbase) - nsize, nsize + psize);
			}
		}
		boot_closemodule(data->boot, data->halexecmod);
	}
	data->execbase = TNULL;
	return TNULL;
}

static TVOID
closeexecmodule(INITDATA *data)
{
	struct TModule *execbase = (struct TModule *) data->execbase;
	TINT nsize = execbase->tmd_NegSize;
	TDestroy(execbase);
	boot_free(data->boot, ((TINT8 *) execbase) - nsize, execbase->tmd_NegSize + nsize);
	boot_closemodule(data->boot, data->halexecmod);
}

/*****************************************************************************/
/* 
**	Perform an Execbase context ("exec", "ramlib")
*/

static TTASKENTRY TVOID
execfunc(TAPTR task)
{
	TAPTR exec = TGetExecBase(task);
	TExecDoExec(exec, TNULL);
}

/*****************************************************************************/
/*
**	Apptask destructor
**	call previously saved task destructor, signal execbase to abort,
**	closedown execbase, shutdown exec and HAL modules, free all memory
*/

static TVOID
destroyatom(TAPTR exec, TAPTR atom, TSTRPTR name)
{
	if (TExecLockAtom(exec, atom,
		TATOMF_TRY | TATOMF_DESTROY) != atom)
	{
		tdbprintf1(20,"atom '%s' is still in use\n", name);
	}
}

static TCALLBACK TVOID 
destroyapptask(TAPTR apptask)
{
	TAPTR exec = TGetExecBase(apptask);
	TAPTR exectask = TExecFindTask(exec, TTASKNAME_EXEC);
	INITDATA *data = TExecGetTaskData(exec, exectask);
	TAPTR boot = data->boot;
	
	destroyatom(exec, data->atom_unique, "sys.unique");
	destroyatom(exec, data->atom_retvalp, "sys.returnvalue");
	destroyatom(exec, data->atom_progname, "sys.progname");
	destroyatom(exec, data->atom_args, "sys.arguments");
	destroyatom(exec, data->atom_argv, "sys.argv");
	destroyatom(exec, data->atom_imods, "sys.imods");

	TExecSignal(data->execbase, data->exectask, TTASK_SIG_ABORT);
	TExecSignal(data->execbase, data->iotask, TTASK_SIG_ABORT);
	TDestroy(data->exectask);
	TDestroy(data->iotask);

	(*data->orgapptaskdestructor)(apptask);

	closeexecmodule(data);
	closehalmodule(data);

	boot_freevec(boot, data->progdir);
	boot_freevec(boot, data->sysdir);
	boot_freevec(boot, data->moddir);
	boot_free(boot, data, sizeof(INITDATA));
	
	boot_exit(boot);
}

/*****************************************************************************/
/*
**	Create application task in current context
*/

TLIBAPI TAPTR 
TEKCreate(TTAGITEM *usertags)
{
	INITDATA *data;
	TAPTR boot = boot_init(usertags);
	if (boot == TNULL) return TNULL;
	data = boot_alloc(boot, sizeof(INITDATA));
	if (data)
	{
		data->boot = boot;
		data->progdir = boot_getprogdir(boot, usertags);
		data->sysdir = boot_getsysdir(boot, usertags);
		data->moddir = boot_getmoddir(boot, usertags);
		
		/* load host abstraction module */
		if (openhalmodule(data, usertags))
		{
			/* load exec module */
			if (openexecmodule(data, usertags))
			{	
				/* place application task into current context */
				data->exectags[0].tti_Tag = TTask_Name;
				data->exectags[0].tti_Value = (TTAG) TTASKNAME_ENTRY;
				data->exectags[1].tti_Tag = TTask_UserData;
				data->exectags[1].tti_Value = (TTAG) data;
				data->exectags[2].tti_Tag = TTAG_MORE;
				data->exectags[2].tti_Value = (TTAG) usertags;
				data->apptask = TExecCreateSysTask(data->execbase, TNULL, 
					data->exectags);
				if (data->apptask)
				{
					/* fill in missing fields in execbase */
					data->execbase->tmd_HALMod = data->halexecmod;
					data->execbase->tmd_InitTask = data->apptask;

					/* create ramlib task */
					data->exectags[0].tti_Value = (TTAG) TTASKNAME_RAMLIB;
					data->iotask = TExecCreateSysTask(data->execbase,
						execfunc, data->exectags);
					if (data->iotask)
					{
						/* create execbase task */
						data->exectags[0].tti_Value = (TTAG) TTASKNAME_EXEC;
						data->exectask = TExecCreateSysTask(data->execbase,
							execfunc, data->exectags);
						if (data->exectask)
						{
							/* this is the 'backdoor' for the remaining
							** initializations in the entrytask context */

							if (TExecDoExec(data->execbase, TNULL))
							{
								/* overwrite apptask destructor */
								data->orgapptaskdestructor = 
									data->apptask->thn_DestroyFunc;
								data->apptask->thn_DestroyFunc = 
									(TDFUNC) destroyapptask;
								initappatoms(data, usertags);
								/* application is running */
								return data->apptask;
							}

							TExecSignal(data->execbase, data->exectask,
								TTASK_SIG_ABORT);
							TDestroy(data->exectask);
						}
						TExecSignal(data->execbase, data->iotask,
							TTASK_SIG_ABORT);
						TDestroy(data->iotask);
					}
					TDestroy(data->apptask);
				}
				closeexecmodule(data);
			} else tdbprintf(20,"could not open Exec module\n");
			closehalmodule(data);
		} else tdbprintf(20,"could not open HAL module\n");

		boot_freevec(boot, data->progdir);
		boot_freevec(boot, data->sysdir);
		boot_freevec(boot, data->moddir);
		boot_free(boot, data, sizeof(INITDATA));
	}

	boot_exit(boot);
	return TNULL;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: init.c,v $
**	Revision 1.8  2005/09/13 02:41:00  tmueller
**	updated copyright reference
**	
**	Revision 1.7  2005/04/01 18:36:22  tmueller
**	A boot-specific object handle is now passed to all boot functions and later
**	handed over to the HAL module, where it helps to abstract from HW resources
**	
**	Revision 1.6  2004/07/04 17:16:14  tmueller
**	added sys.arguments and sys.progname atoms
**	
**	Revision 1.5  2004/06/11 17:47:49  tmueller
**	Atoms are now destroyed in TATOMF_TRY mode, failure is reported verbosely
**	
**	Revision 1.4  2004/05/05 00:45:19  tmueller
**	If argv unsupported, the sys.argv atom now contains an empty array
**	
**	Revision 1.3  2004/04/18 13:57:53  tmueller
**	arguments in parseargv, atomdata, gettag changed from TAPTR to TTAG
**	
**	Revision 1.2  2003/12/12 15:02:17  tmueller
**	Added and fixed some comments
**	
**	Revision 1.1.1.1  2003/12/11 07:18:20  tmueller
**	Krypton import
*/
