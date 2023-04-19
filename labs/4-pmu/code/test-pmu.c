#include "rpi.h"
#include "pmu.h"

void measure_calls(const char *msg, unsigned n) {
    output("%s:\n", msg);
    pmu_enable0(PMU_CALL_CNT);
    pmu_enable1(PMU_RET_CNT);
    pmu_clear0();
    pmu_clear1();

    while(n-->0)
        GET32(0);

    unsigned t0 = pmu_event0_get();
    unsigned t1 = pmu_event1_get();
    output("\ttotal calls=%d, total returns =%d\n", t0,t1);
}

void measure_pmu_calls(pmu_t p, const char *msg, unsigned n) {
    output("%s:\n", msg);

    pmu_start();
    while(n-->0)
        GET32(0);

    unsigned t0 = pmu_event0_get();
    unsigned t1 = pmu_event1_get();
    output("\ttotal %s=%d, total %s=%d\n", p.pmu0, t0, p.pmu1, t1);
}

void notmain(void) {
    // see cache-support.S
    void flush_caches(void);
    flush_caches();

    // call/ret doesn't seem to work without cache.
    assert(!caches_is_enabled());
    measure_calls("no cache: doesn't work?", 10);

    caches_enable();
    measure_calls("cache [expect 10]", 10);

    // wrap it up a bit nicer.
    pmu_t p = pmu_mk(PMU_CALL_CNT, PMU_RET_CNT);
    measure_pmu_calls(p, "cache [expect 10]", 10);


    output("disabling caches again\n");
    caches_disable();
    assert(!caches_is_enabled());

    p = pmu_mk(PMU_BRANCH_MISPREDICT, PMU_BRANCH_EXECUTED);
    measure_pmu_calls(p, "no cache branches", 10);

    caches_enable();
    measure_pmu_calls(p, "cache branches", 10);
}
