#include "rpi.h"
#include "pmu.h"

void count_inst(const char *msg);

void notmain(void) {
    count_inst("count instructions [no cache]");
    count_inst("count instructions [no cache]");


    caches_enable();

    count_inst("count instructions [cache] weird i get 13 inst here.");
    count_inst("count instructions [cache]");
    count_inst("count instructions [cache]");
    count_inst("count instructions [cache]");
}

void count_inst(const char *msg) {
    output("%s:\n", msg);
    pmu_enable0(PMU_INST_CNT);
    pmu_clear0();
    asm volatile("nop");    // 5
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");

    asm volatile("nop");    // 5
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");

    unsigned t0 = pmu_event0_get();
    output("\ttotal instructions=%d\n", t0);
}

