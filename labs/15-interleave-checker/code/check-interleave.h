#ifndef __INTERLEAVE_CHECK_H__
#define __INTERLEAVE_CHECK_H__
// engler: simple concurrency interleaving checking for cs240lx

#include "rpi.h"
#include "rpi-interrupts.h"
#include "rpi-constants.h"
#include "cpsr-util.h"
#include "breakpoint.h"
#include "vector-base.h"


// simple debug macro: can turn it off/on by calling <brk_verbose({0,1})>
#define brk_debug(args...) if(brk_verbose_p) debug(args)

extern int brk_verbose_p;
static inline void brk_verbose(int on_p) { brk_verbose_p = on_p; }


// defines the concurrency interface: 
//  - two routines A() and B() where A is switched once and B() 
//    always runs to completion.
//      - B() returns 0 if it can't complete.
//      - returns != 0 otherwise.
//  - init() initializes the state
//  - chec() returns 1 if the state is legal at the end.
typedef struct checker { 
    /************************************************************
     * these fields are set by the user
     */
    // if you need state.
    volatile void *state;

    // can use nested functions if you want to pass data.
    void (*A)(struct checker *);
    int (*B)(struct checker *);

    // initialize the state.
    void (*init)(struct checker *c);
    // check that the state is valid.
    int  (*check)(struct checker *c);

    /************************************************************
     * these fields are for internal use by the checker.
     */

    // total number of switches we have done for all checking.
    volatile unsigned nswitches;

    /* the fields below are used by each individual run */

    // context switch from A->B on instruction <n>.
    // starts at 1 (run 1 instruction and switch)
    // then      2 (run 2 instructions and switch)
    // then      3 (run 3 instructions and switch)
    // ...
    // finished when we run and do not switch.
    //
    // lots of speed opportunities here.
    // 
    // Extension: handle more than one switch.
    // Extension: only switch when A does a store.
    //            all other instructions should not
    //            have an impact (I *believe* need to 
    //            check through the ARM carefully).
    volatile unsigned switch_on_inst_n; 

    // total number of instructions (faults) we ran so far on a given
    // run: used by the interrupt handler to see if we should 
    // switch to B() (when inst_count == switch_on_inst_n)
    volatile uint32_t inst_count;

    // the address we switched on this time: used for error reporting
    // when you get a mismatch.
    volatile uint32_t switch_addr;

    // number of times we've skipped instructions b/c B failed.
    volatile uint32_t skips;

    // set by the breakpoint handler when a switch occurs.
    // reset on each checking run: if you run and does not get 
    // set, you are done.
    volatile unsigned switched_p;    

    // records the total number of trials and errors.
    unsigned ntrials;
    unsigned nerrors;

    // set when we start doing interleave checking.
    unsigned interleaving_p;
} checker_t;

// check the routines A and B pointed to in <c>
// returns 1 if was successful, 0 otherwise.
//  total trials and errors can be pulled from <c>
int check(checker_t *c);

void user_trampoline_ret(uint32_t cpsr_user, void (*fn)(void*), checker_t *c);

#include "syscalls.h"
void switchto_user_asm(uint32_t regs[16]);


#endif
