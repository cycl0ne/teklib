#------------------------------------------------------------------------
#
#  $Id: crt0.S,v 1.1 2005/09/18 12:33:39 tmueller Exp $
#  teklib/boot/ps2/crt0.S - Early system startup 
#
#  Written by Gustavo Scotti <gustavo at scotti.com>
#  and Marcus R. Brown <mrbrown at 0xd6.org>
#  This file was derived from PS2lib - PS2 OpenSource Project
#
#  Adapted to teklib by Timm S. Mueller <tmueller at neoscientists.org>
#------------------------------------------------------------------------

#include <tek/mod/ps2/regs.h>

.set noat
.set noreorder

.global _start
.global	_exit

	.extern	_heap_size
	.extern	_stack
	.extern _stack_size

	.text

	nop
	nop

	.ent _start
_start:

# Clear bss elf segment (static uninitalised data)

zerobss:
	la	v0, _fbss
	la	v1, _end
loop:	nop
	nop
	nop
	sq	zero,(v0)
	sltu	at,v0,v1
	bne	at,zero,loop
	addiu	v0,v0,16

# Some program loaders (such as Pukklink) execute programs as a thread, but
# support passing argc and argv values via a0.  This also indicates that when
# the program terminates, control should be returned to the program loader
# instead of the PS2 browser.
	la	v0, _args_ptr
    	sw	a0,(v0)

# Setup a thread to use
	la	a0, _gp
	la	a1, _stack
	la	a2, _stack_size
	la	a3, _args
	la	t0, _root
	move	gp,a0
	addiu	v1,zero,60
	syscall			# RFU060(gp, stack, stack_size, args, root_func)

# align stack
	li	v1,~16383
	and	v0,v0,v1
	addiu	v0,v0,16
	move	sp, v0

# Heap
	addiu	v1,zero,61
	la	a0, _end
	la	a1, _heap_size
	syscall			# RFU061(heap_start, heap_size)
	nop

# Flush the data cache (no need to preserve regs for this call)
	li	v1, 0x64
	move	a0,zero
	syscall			# FlushCache(0) - Writeback data cache

# Jump main, now that environment and args are setup
	ei

# Check for arguments pased via ExecPS2 or LoadExecPS2
	la	v0, _args
	lw	v1, (v0)
	bnez	v1, 1f
	nop

# Otherwise check for arguments passed by a loader via a0 (_arg_ptr)
	la	v0, _args_ptr
	lw	v1, (v0)
	beqzl	v1, 2f
	addu	a0, zero, 0

	addiu	v0, v1, 4
1:
	lw	a0, (v0)
	addiu	a1, v0, 4
2:
	la	a2, _end	# heap
	jal	_tekmain
	nop
	.end	_start

	.ent	_exit
_exit:
# If we received our program arguments in a0, then we were executed by a
# loader, and we dont want to return to the browser.
	la	a0, _args_ptr
	lw	a1, (a0)
	beqz	a1, 1f
	move	a0, v0		# return code

	lw	a2, (a1)
	sw	zero, (a2)
	addiu	v1, zero, 36
	syscall			# ExitDeleteThread(void)

# Return to the browser via Exit()
1:
	addiu	v1, zero, 4
	syscall			# Exit(void)
	.end	_exit

# Call ExitThread()
	.ent	_root
_root:
	addiu	v1, zero, 35
	syscall
	.end	_root


	.bss
	.align	6
_args:
	.space	256+16*4+4
_args_ptr:
	.space	4

#------------------------------------------------------------------------
#
#  Revision History
#  $Log: crt0.S,v $
#  Revision 1.1  2005/09/18 12:33:39  tmueller
#  added
#
#
#