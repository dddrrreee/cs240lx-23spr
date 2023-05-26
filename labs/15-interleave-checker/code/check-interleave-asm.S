#include "rpi-asm.h"

@ user-level trampoline to call system call.
@
@ save lr b/c hardware will trash it (since we are at SUPER initially)
MK_FN(syscall_invoke_asm)
    push {lr}   @ we won't need these at user level.
    swi 0
    pop {lr}    @ we won't need these at user level.
    bx lr

@ if you want to override, need to implement your own
@ prefetch_abort_trampoline
.align 5; 
.globl interleave_full_vec
interleave_full_vec:
    b unhandled_reset
    b unhandled_undefined_instruction
    b swi_trampoline
    b staff_prefetch_abort_trampoline
    b unhandled_data_abort
    b unhandled_interrupt
    b unhandled_fiq


@ we do the swi asm in a bit weird way: you can redo if don't like.
MK_FN(swi_trampoline)
    @ runs at system level (p.a2-5): assumes we have a sp
    push {r3-r12,lr}

    @ move the values into the right place
    mov   r2, r1
    mov   r1, r0

    sub   r0, lr, #4
    mov   r3, sp

    bl    syscall_vector

    pop {r3-r12,lr}
    movs  pc, lr   @ lr holds address of instruction after the SWI

@ this is from 140e: write the code to switch to user space given an
@ array of registers.
MK_FN(switchto_user_asm)
    b staff_switchto_user_asm
