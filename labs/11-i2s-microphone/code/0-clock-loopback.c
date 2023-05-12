// use loop back on fs and clk to make sure we get the expected period.
#include "rpi.h"
#include "i2s.h"

// connect BCLK in in_clk and FS to in_fs with jumpers.
enum { in_clk = 16, in_fs = 17 };

static inline uint32_t gpio_read_raw(unsigned pin) {
    enum { GPIO_BASE = 0x20200000 };
    volatile uint32_t *gpio_lev0 = (void*)(GPIO_BASE + 0x34);

    return ((*gpio_lev0 >> pin)&1);
}

static int
measure_v(
    const char *clk_str,        // string giving source
    uint32_t in,            // input loopback pin
    uint32_t v,
    uint32_t expected,      // what we expect (cycles)
    uint32_t epsilon,       // how much error we accept (cycles)
    int die_p)              // if we should die if miss tolerance
{
    uint32_t s,e;

    // while == v spin [since it's just partial]
    while(v == gpio_read(in))
        ;

    // while have != v spin [since not what we want]
    while(v != gpio_read(in))
        ;

    // start timing.
    s = cycle_cnt_read();
    while(v == gpio_read(in))
        ;
    e = cycle_cnt_read();

    // see if its within tolerances
    unsigned got = e-s;
    unsigned diff;

    if(got > expected)
        diff = got - expected;
    else
        diff = expected - got;

    if(diff > epsilon)
        output("ERROR:");
    else
        output("SUCCESS:");

    output("%s=%d: total cycles = %d [diff=%d], expect about %d +/- %d\n",
        clk_str, v, got, diff, expected, epsilon);

    if(diff > epsilon) {
        if(die_p)
            panic("exceeded tolerance by %d!\n", diff);
        return 0;
    }
    return 1;
}



static int
measure_clock(
    const char *clk_str,        // string giving source
    uint32_t in,            // input loopback pin
    uint32_t expected,      // what we expect (cycles)
    uint32_t epsilon,       // how much error we accept (cycles)
    int die_p)              // if we should die if miss tolerance
{
    uint32_t s,e,v;

    // wait until changes for the first time.
    v = gpio_read_raw(in);
    while(v == gpio_read_raw(in))
        ;

    // start timing.
    // wait for a comple {up, down} or {down, up}
    s = cycle_cnt_read();
    v = 1-v;
    while(v == gpio_read_raw(in))
        ;

    v = 1-v;
    while(v == gpio_read_raw(in))
        ;

    e = cycle_cnt_read();

    // see if its within tolerances
    unsigned got = e-s;
    unsigned diff;

    if(got > expected)
        diff = got - expected;
    else
        diff = expected - got;
        
    if(diff > epsilon)
        output("ERROR:");
    else
        output("SUCCESS:");

    output("%s: total cycles = %d [diff=%d], expect about %d +/- %d\n",
        clk_str, got, diff, expected, epsilon);

    if(diff > epsilon) {
        if(die_p)
            panic("exceeded tolerance by %d!\n", diff);
        return 0;
    }

    return 1;
}

void notmain(void) {
    caches_enable();

    output("----------------------------------------------------\n");
    output("loopback test for BCLK (pin=%d) connect to GPIO input (%d)\n", 
            pcm_clk, in_clk);

    gpio_set_pulldown(in_clk);
    gpio_set_input(in_clk);
    gpio_set_pulldown(in_fs);
    gpio_set_input(in_fs);

    i2s_init(44100);

    enum { N = 8 };
    /* 
       from the README: our clock is 
            44.1Khz * 64 = 2,822,400 cycles per second.

       so we expect each sample to be roughly:

        = ARM cycles per second / cycles per second
        = 700 MHz  / (44.1khz * 64)
        = 700*1000*1000 / (44100 * 64)
        = 248 cycles

       you should try other clock periods to make sure they work!
     */
    enum { BCLK_PERIOD = 248 };
    
    // check that the pcm clock works.
    for(unsigned i = 0; i < N; i++)
        measure_clock("pcm_clk", in_clk, BCLK_PERIOD, 200, 0);

    output("----------------------------------------------------\n");
    output("loopback test for FS (pin=%d) connect to GPIO input (%d)\n", 
            pcm_fs, in_fs);

    // this error is big.  don't understand atm.
    for(unsigned i = 0; i < 8; i++)
        measure_clock("pcm_fs", in_fs, BCLK_PERIOD*64, 200, 0);

#if 0
    for(unsigned i = 0; i < 8; i++)
        measure_v("pcm_fs", in_fs, 0, BCLK_PERIOD*64/2, 200, 0);
    for(unsigned i = 0; i < 8; i++)
        measure_v("pcm_fs", in_fs, 1, BCLK_PERIOD*64/2, 200, 0);
#endif
}
