// engler: very simple concurrency checker for cs240lx
#include "check-interleave.h"

// used to communicate with the breakpoint handler.
static volatile checker_t *checker = 0;
int brk_verbose_p = 0;

// called on each single step exception: 
// if we have run switch_on_inst_n instructions then:
//  1. run B().
//     - if B() succeeds, then run the rest of A() to completion.  
//     - if B() returns 0, it couldn't complete w current state:
//       resume A() and switch on next instruction.
static void ss_handler(uint32_t regs[17]) {
    uint32_t pc = regs[15];

    brk_debug("prefetch abort at %p\n", pc);
    if(!brkpt_fault_p())
        panic("impossible: should get no other faults\n");


    volatile checker_t *c = checker;

    // not exactly right: we consider each fault 1 inst.
    if(c->inst_count == c->switch_on_inst_n) {
        // you probably want to comment this out :)
        output("about to switch from %p to %p\n", pc, c->B);


        // run B() completion: won't trap b/c invoke in privileged mode
        if(!c->B((void*)c)) {
            output("B failed on step=%d: going to try next instruction\n", c->inst_count);
            c->switch_on_inst_n++;
            c->skips++;

        } else {
            // done.
            brkpt_mismatch_stop();
            c->switch_addr = pc;
            c->switched_p = 1;
            c->nswitches++;

            output("mode=%s: done running B, going to return to A @ %p\n", mode_str(cpsr_get()), pc);
            
            return;
        }
    } 
    assert(c->inst_count < c->switch_on_inst_n);
    assert(!c->switched_p);
    c->inst_count++;
    brk_debug("setting up a mismatch on %p\n", pc);
    brkpt_mismatch_set(pc);
    brk_debug("done setting up a mismatch on %p\n", pc);
}

// we do it like this so the handler can just return.  i had a bug 
// where i missed doing a full cswitch
void single_step_handler_full(uint32_t regs[17]) {
    ss_handler(regs);
    switchto_user_asm(regs);
}

static inline uint32_t sp_get(void) {
    uint32_t sp = 0;
    asm volatile("mov %0, sp" : "=r" (sp));
    return sp;
}

// iteratively run c->A(), doing 1 constext switch on instruction 1, then
// 2, ... calling B().
static unsigned check_one_cswitch(checker_t *c) {
    uint32_t cpsr_old = cpsr_get();
    uint32_t cpsr_user = mode_set(cpsr_old, USER_MODE);

    checker = c;
    c->interleaving_p = 1;
    do {
        // initialize the state on each run.
        c->init(c);

        // 0 should always mismatch
        brk_debug("trial=%d: about to set mismatch brkpt for A\n", c->ntrials);

        // start mismatching.  
        brkpt_mismatch_start();

        // switch on the next instruction: note, b/c of non-determ,
        // this does not mean we do the same prefix as previous trials.
        c->switch_on_inst_n++;
        c->inst_count = 0;
        c->switched_p = 0;

        // for sanity checking.
        uint32_t old_sp = sp_get();

        
        output("going to start iter = %d\n", c->switch_on_inst_n);

        // this will run c->A(c);
        user_trampoline_ret(cpsr_user, (void (*)(void*)) c->A, c);

        // make sure sp got restored (weak test)
        uint32_t new_sp = sp_get();
        assert(old_sp == new_sp);

        // we didn't switch: should be able to invoke B()
        if(!c->switched_p) {
            if(!c->B(c))
                panic("B failed: error in its code\n");
        }

        if(!c->check(c)) {
            output("ERROR: check failed when switched on address [%p], after [%d] instructions\n",
                c->switch_addr, c->switch_on_inst_n);
            c->nerrors++;
        }

        c->ntrials ++;

    // keep doing while we've switched in A()  otherwise no more 
    // instructions.
    } while(c->switched_p);

    printk("done!  did c->nswitches=%d, skips=%d, ntrials=%d, nerrors=%d\n", 
                    c->nswitches, c->skips, c->ntrials, c->nerrors);

    // make sure that the cpsr is set correctly.
    output("expected mode = %s (%x)\n", mode_str(cpsr_old), cpsr_old);
    output("current mode = %s (%x)\n", mode_str(cpsr_get()), cpsr_get());

    // turn off mismatch
    brkpt_mismatch_stop();

    assert(mode_get(cpsr_old) == mode_get(cpsr_get()));

    c->interleaving_p = 0;

    // when we get back here, it might have breakpoints enabled: disable them.
    return c->nerrors == 0;
}

// should not fail: testing sequentially.
// if the code is non-deterministic, or the init / check
// routines are busted, this could find it.
static unsigned check_sequential(checker_t *c) {
    return 1;
    // we don't count sequential trials.
    unsigned i;
    for(i = 0; i < 100; i++) {
        c->init(c);

        c->A(c);
        if(!c->B(c))
            panic("B should not fail\n");
        if(!c->check(c))
            panic("check failed sequentially: code is broken\n");

#if 0
        // if AB commuted, we could do this: won't be true in general.
        c->init(c);
        c->B(c);
        c->A(c);
        if(!c->check(c))
            panic("check failed sequentially: code is broken\n");
#endif
    }
    return 1;
}

// zero out all the fields that might be left over from previous run.
static void check_zero(checker_t *c) {
    c->nswitches =
    c->switch_on_inst_n =
    c-> inst_count =
    c->switch_addr = 
    c->switched_p =
    c->ntrials =
    c->nerrors =
    c->skips =
    c->interleaving_p = 0;
}

// check that running the two routines A and B in <c> with
// a single context switch on each instruction in A give 
// the same result as running AB without a context switch
int check(checker_t *c) {
    uint32_t *v = vector_base_get();
    extern uint32_t  interleave_full_vec[];

    if(!v)
        vector_base_set(interleave_full_vec);
    else if(v != interleave_full_vec)
        panic("exceptions setup with different vector: need to handle this\n");

    check_zero(c);
    return check_sequential(c) && check_one_cswitch(c);
}
