// use loop back on fs and clk to make sure we get the expected period.
#include "rpi.h"
#include "i2s.h"

void notmain(void) {
    caches_enable();

    output("----------------------------------------------------\n");
    output("now hook up the i2s mic not in loopback\n");
    i2s_init(44100);

    enum { N = 64, DISCARD = 256 };
    unsigned nsample = 0;

    struct mic_vals {
        long v;
        uint32_t tot;
    } r[N];

    uint32_t last=0;

    // discard the first samples.
    for(int i = 0; i < DISCARD; i++) {
        i2s_get32();
        last = cycle_cnt_read();
    }

    unsigned start = last;

    // very crude measure of how many samples per sec
    for(int i = 0; i < N; i++) {
        uint32_t v = i2s_get32();
        uint32_t e = cycle_cnt_read();

        r[i].tot = e - last;
        r[i].v = v;
        last = e;
    }

    unsigned end = cycle_cnt_read();

    // how many samples per second
    //   cycles per second = 700M.
    // what fraction of a second we took to do N cycles:
    //   700M / total cycles
    // so how many samples per sec =
    //  =  N * (700M / tot_cycles))
    //
    // e.g., cycles=253952, N=16 gives
    // 16 * (700*1000*1000 / 253952.) = 44102.8225806

    output("samples per second= %d (tot cycle = %d, N=%d)\n",
            N * ((700*1000*1000)/(end-start)),
            end-start,N);

    for(int i = 0; i<N; i++) {
        output("read=%d: raw val=%x, mic val=%d [ncycle=%d]\n",
                i,
                r[i].v, 
                i2s_to_mic_val(r[i].v),
                r[i].tot);
    }
}
