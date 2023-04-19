#ifndef __ARMV6_PMU_H__
#define __ARMV6_PMU_H__

#include "bit-support.h"
#include "asm-helpers.h"

// use the <cp_asm> macro to define the different 
// get/set PMU instructions.  i called them:
//  pmu_cycle [to get the cycle counter]
//  pmu_event0 : to get and set event0
//  pmu_event1 : to get and set event1
//  pmu_control : to get and set the control reg.

// p 3-134 arm1176.pdf: write the PMU control register
static inline void pmu_control_wr(uint32_t in) {
    
    // 3-134 says to clear these.

    // you have to *set* these bits to clear the event.
    in = bits_set(in, 8, 10, 0b111);
    assert(bits_get(in, 8,10) == 0b111);

    pmu_control_set(in);
}

// get the type of event0 by reading the type
// field from the PMU control register and 
// returning it.
static inline uint32_t pmu_type0(void) {
    todo("return the current type of event0");
}

// set PMU event0 as <type> and enable it.
static inline void pmu_enable0(uint32_t type) {
    todo("enable event 0");

    assert(pmu_type0() == type);
}

// get the type of event1 by reading the type
// field from the PMU control register and 
// returning it.
static inline uint32_t pmu_type1(void) {
    todo("return the current type of event1");
}

// set event1 as <type> and enable it.
static inline void pmu_enable1(uint32_t type) {
    assert((type & 0xff) == type);
    todo("enable event 1");
    assert(pmu_type1() == type);
}

#endif
