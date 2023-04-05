// figure out the value of <pc> register (r15) when read with a
// <mov> by using some code and reasoning about where stuff is.  
// 
// in general, its better to use the arch manual, but we do it 
// this way to get used to kicking the tires.
#include "rpi.h"

/*
 * from .list file:
 * 00008038 <pc_val_get>:
 *  8038:   e1a0000f    mov r0, pc
 *  803c:   e12fff1e    bx  lr
 */
void derive_add(void) {
    asm volatile ("add r0, r0, r0");
    asm volatile ("add r0, r0, r1");
    asm volatile ("add r0, r0, r2");
    asm volatile ("add r0, r0, r3");
    asm volatile ("add r0, r0, r4");
    asm volatile ("add r0, r0, r5");
    asm volatile ("add r0, r0, r6");
    asm volatile ("add r0, r0, r7");
    asm volatile ("add r0, r0, r8");
    asm volatile ("add r0, r0, r9");
    asm volatile ("add r0, r0, r10");
    asm volatile ("add r0, r0, r11");
    asm volatile ("add r0, r0, r12");
    asm volatile ("add r0, r0, r13");
    asm volatile ("add r0, r0, r14");
    asm volatile ("add r0, r0, r15");
}
void derive_end(void);

void notmain() { 
    uint32_t *inst = (void*)derive_add;
    output("inst\thex encoding\tbinary encoding\n");
    for(int i = 0; i < 15; i++) 
        output("%d:\t[%x]\t[%b]\n", i,inst[i], inst[i]);

    // solve.
    output("-------------------------------------\n");
    output("solve for changed bits\n");
    uint32_t always_0 = ~0;
    uint32_t always_1 = ~0;
    for(int i = 0; i < 15; i++) {
        always_0 &= ~inst[i];
        always_1 &= inst[i];
    }

    uint32_t changed = ~(always_0 | always_1);
    for(unsigned i = 0; i < 32; i++) {
        if(changed & (1<<i))
            output("bit=%d changed: part of source 2 register\n",i);
    }

    output("-------------------------------------\n");
    output("going to try cheating!\n");
    // we cheat and assume contiguous and r0...r15
    always_0 &= ~inst[0];
    always_0 &= ~inst[15];
    always_1 &= inst[0];
    always_1 &= inst[15];

    changed = ~(always_0 | always_1);
    for(unsigned i = 0; i < 32; i++) {
        if(changed & (1<<i))
            output("bit=%d changed: part of source 2 register\n",i);
    }
}
