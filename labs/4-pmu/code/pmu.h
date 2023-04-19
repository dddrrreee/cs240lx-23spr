#ifndef __PMU_H__
#define __PMU_H__
#include "armv6-pmu.h"

// define the different PMU types.
typedef enum {
#   define XX(name,val,string) name = val,
#   include "pmu-types.h"
#   undef XX
} pmu_type_t;

// define the different strings used
static inline const char *pmu_str(pmu_type_t ty) {
    switch(ty) {
#   define XX(name,val,string) case name: return string;
#   include "pmu-types.h"
#   undef XX
    default: panic("unhandled type %d\n", ty);
    }
}

// bundle the three counters into one structure to make
// things a bit easier.
typedef struct {
    pmu_type_t type0;
    const char *pmu0;
    uint32_t tot0;

    pmu_type_t type1;
    const char *pmu1;
    uint32_t tot1;

    uint32_t tot_cyc;

} pmu_t;
 
static inline pmu_t
pmu_mk(pmu_type_t type0, pmu_type_t type1) {
    pmu_t p = (pmu_t) { 
        .type0 = type0,
        .pmu0 = pmu_str(type0),
        .type1 = type1,
        .pmu1 = pmu_str(type1),
    };
    pmu_enable0(type0);
    pmu_enable1(type1);
    return p;
}

// clear event counts
#define pmu_clear0() pmu_event0_set(0)
#define pmu_clear1() pmu_event1_set(0)

// clear all the counters.
static inline void pmu_start(void) {
    pmu_cycle_set(0);
    pmu_event0_set(0);
    pmu_event1_set(0);
}

// could see different values depending on different
// orders of reads.  the cleanest method is to read a
// single counter.
//
// the cleanest measurement will be event 0, then event 1,
// then the cycle count (can measure null statement to 
// correct).
#define PMU_MEASURE(pmu, stmt) ({           \
    pmu_clear();                            \
    stmt;                                   \
    uint32_t tot0 = pmu_event0_get();       \
    uint32_t tot1 = pmu_event1_get();       \
    uint32_t tot_cyc = cycle_cnt_read();    \
    pmu.tot0 = tot0;                        \
    pmu.tot1 = tot1;                        \
    pmu.tot_cyc = tot_cyc;                  \
    pmu;                                    \
})


static inline void 
pmu_print(const char *msg, pmu_t p) {
    printk("%s\n", msg);
    printk("    %s = %d\n", p.pmu0, p.tot0);
    printk("    %s = %d\n", p.pmu1, p.tot1);
    printk("    cycles = %d\n", p.tot_cyc);
}

#endif
