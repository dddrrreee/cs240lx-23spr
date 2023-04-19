#include "rpi.h"
#include "pmu.h"


void measure_10_loads(const char *msg, int ty) {
    output("%s:\n", msg);

    pmu_enable0(ty);

/*
    80b4:   ee073f95    mcr 15, 0, r3, cr7, cr5, {4}
    80b8:   e5932000    ldr r2, [r3]
    80bc:   e5932000    ldr r2, [r3]
    80c0:   e5932000    ldr r2, [r3]
    80c4:   e5932000    ldr r2, [r3]
    80c8:   e5932000    ldr r2, [r3]
    80cc:   e5932000    ldr r2, [r3]
    80d0:   e5932000    ldr r2, [r3]
    80d4:   e5932000    ldr r2, [r3]
    80d8:   e5932000    ldr r2, [r3]
    80dc:   e5933000    ldr r3, [r3]
    80e0:   ee1f2f5c    mrc 15, 0, r2, cr15, cr12, {2}
*/

    pmu_clear0();

    *(volatile uint32_t *)0;
    *(volatile uint32_t *)0;
    *(volatile uint32_t *)0;
    *(volatile uint32_t *)0;
    *(volatile uint32_t *)0;

    *(volatile uint32_t *)0;
    *(volatile uint32_t *)0;
    *(volatile uint32_t *)0;
    *(volatile uint32_t *)0;
    *(volatile uint32_t *)0;

    unsigned t0 = pmu_event0_get();

    output("\t%s =%d\n", pmu_str(ty), t0);
}

void notmain(void) {
    measure_10_loads("should get 10 loads", PMU_EXPLICIT_DATA_ACCESS);
}
