#include "check-interleave.h"
#include "pi-sys-lock.h"

uint32_t sp_user_get(void) {
    volatile uint32_t x[3] = {0};

    asm volatile("stm %0, {sp,lr}^" :: "r"(x));
    asm volatile("nop");
    printk("shadow lr=%p, shadow sp = %p\n", x[1], x[0]);
    return x[0];
}

// <pc> should point to the system call instruction.
//      can see the encoding on a3-29:  lower 24 bits hold the encoding.
// <r0> = the first argument passed to the system call.
// 
// system call numbers:
//  <1> - set spsr to the value of <r0>
int syscall_vector(unsigned pc, uint32_t sys_num, uint32_t r0, uint32_t sp) {
    uint32_t inst = *(uint32_t*)pc;

    assert(mode_get(cpsr_get()) == SUPER_MODE);

    switch(sys_num) {
    case SYS_RESUME:
    {
            uint32_t cpsr = cpsr_get();
            uint32_t spsr = spsr_get();
            printk("DONE: cur mode: %s, saved mode =%s, switch-to mode=%s\n",
                    mode_str(cpsr), mode_str(spsr), mode_str(r0));

            printk("DONE: pc = %x, saved sp=%p\n", pc, sp_user_get());
            output("DONE: going to resume!: sp=%p, inst=%b, sys_num=%d, r0=%x\n", sp, inst, sys_num,r0);
#if 0
            void exec_resume(uint32_t sp);
            // this won't work b/c your lr will be trashed: should be ok, right?
            exec_resume(sp);
#endif
            // panic("should not get here\n");
            if(mode_get(r0) != SUPER_MODE)
                panic("should only be switching to super!\n");

            // have to change the *saved* spsr :) :)
            spsr_set(r0);
            assert(spsr_get() == r0);
            return 0;
    }
    case SYS_TRYLOCK:
    {
            todo("implement this\n");
            return 1;
    }
    case SYS_TEST:
        printk("running empty syscall with arg=%d\n", r0);
        return SYS_TEST;
    default:
        panic("illegal system call = %d, pc=%x, r0=%d!\n", sys_num, pc, r0);
    }
}
