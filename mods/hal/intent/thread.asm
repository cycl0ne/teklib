
*
* $Header: /cvs/teklib/teklib/mods/hal/intent/thread.asm,v 1.2 2003/12/18 10:04:28 mlukat Exp $
*

.include 'tao'

	record payload
		void	[hit_Init]
	endrecord

tool 'lib/tek/hal/thread',VP,TF_MAIN,4096,sizeof(void[])
	ent	- : -

	qcall	lib/argcargv,(- : p0,i~)
	if	p0 eq NULL,true
		cpy	[gp],p0
		qcall	lib/tek/hal/setuserdata,(p0 : -)
		gos	[(payload[])p0.hit_Init],(p0 : -)
	endif
	qcall	lib/exit,(0 : -)
	noret
toolend


tool 'lib/tek/hal/setuserdata'
	ent	p0 : -

	switch
		cpy	[(PROC[])gp.userdata],p1
		cpy	p0,[(PROC[])gp.userdata]
		breakif	p0 eq p1

		qcall	sys/kn/mem/free,(p1 : -)
	endswitch
	ret
toolend


tool 'lib/tek/hal/getuserdata'
	ent	- : p0

	cpy	[(PROC[])gp.userdata],p0
	ret
toolend
.end
