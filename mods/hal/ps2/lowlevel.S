
#------------------------------------------------------------------------
#
#  $Id: lowlevel.S,v 1.2 2005/09/18 11:27:22 tmueller Exp $
#  teklib/mods/hal/ps2/lowlevel.S - stack utils
#
#  Written by Franciska Schulze <fschulze at neoscientists.org>
#  See copyright notice in teklib/COPYRIGHT
#------------------------------------------------------------------------

#include <tek/mod/ps2/regs.h>

	.text
	.p2align 4

#------------------------------------------------------------------------
	.global	alignsp
#------------------------------------------------------------------------
# Registers:
#	a0 - function ptr for subfunc(udata)
#	a1 - udata
#	a2 - alignment mask
#	t0 - work copy of sp_new
#
# Stack Layout:
#
#	|///////////////|
#	|///////////////|
#	================== sp_old
#	:		:
#	:		:
#	|---------------|
#	|      ra       |
#	|---------------|
#	|    sp_old     |
#	================= aligned
#	|    userdata   |
#	|---------------|
#	|///////////////|
#	|///////////////|
#	:		:
#	:		:
#

	.ent alignsp
	.set push
	.set noreorder
	.set mips3
	
alignsp:	# calc sp_new
		add	t0,sp,zero
		addi	t0,-8
		and	t0,t0,a2
		
		# store sp_old, ra
		sw	ra,8(t0)
		sw	sp,4(t0)

		# set sp to sp_new
		add	sp,t0,zero
		sw	a1,0(sp)
		addi	sp,sp,-16

		# call func
		add	t1,zero,a0
		add	a0,zero,a1
		jalr	t1
		nop
		
		# restore sp_old, ra
		lw	ra,24(sp)
		lw	sp,20(sp)
				
		jr	ra
		nop
	
	.set pop
	.end alignsp

#------------------------------------------------------------------------
	.global	GetThreadPrivate
#------------------------------------------------------------------------
# Registers:
#	> a0	alignment mask
#	< v0	aligned stack ptr (void **userdata)

	.ent GetThreadPrivate
	.set push
	.set noreorder
	.set mips3
	
GetThreadPrivate:

		and	v0,a0,sp
		not 	a0
		add 	v0,v0,a0
		addi 	v0,v0,-3
		jr	ra
		nop
	
	.set pop
	.end GetThreadPrivate


#------------------------------------------------------------------------
#
#  Revision History
#  $Log: lowlevel.S,v $
#  Revision 1.2  2005/09/18 11:27:22  tmueller
#  added authors
#
#  Revision 1.1  2005/03/13 20:01:39  fschulze
#
#
