#include "rpi.h"
#include "bit-support.h"
#include "i2s.h"

struct pcm_div {
    uint32_t divi, divf, mash;
};

// lets us hardcode and be really sure we did the right calculation.
struct pcm_div clock_to_div(uint32_t clock) {
    switch(clock) {
    case 44100: return (struct pcm_div) { .divi = 6, .divf = 3288, .mash=3 };
    default: panic("add the div for clock=%d\n", clock);
    }
}

void pcm_clock_init(uint32_t clock) {
    dev_barrier();

    struct pcm_div div = clock_to_div(clock);

    // 1.turn off the pcm clock.
    // 2.wait til not busy.
    // 3. set the control and divisor
    todo("implement this\n");

    dev_barrier();
}

static void pcm_check_initial(void) {
    dev_barrier();
    pcm_t *p = (void*)0x20203000;

    // from the datasheet: p134
    if(!bit_is_on(p->cs_a, 21))
        panic("default not set?\n");
    if(!bit_is_on(p->cs_a, 19))
        panic("default not set?\n");
    if(!bit_is_on(p->cs_a, 17))
        panic("default not set?\n");

    // from the datasheet: p134
    if(bits_get(p->dreg_a, 24,30) != 0x10)
        panic("default not set?\n");
    if(bits_get(p->dreg_a, 16,22) != 0x30)
        panic("default not set?\n");
    if(bits_get(p->dreg_a, 8,14) != 0x30)
        panic("default not set?\n");
    if(bits_get(p->dreg_a, 0,6) != 0x20)
        panic("default not set?\n");

    // p 133
    if(p->txc_a != 0)
        panic("default not set?\n");

    // p 135
    if(p->inten_a != 0)
        panic("default not set?\n");

    if(p->intstc_a != 0)
        panic("default not set?\n");
    if(p->grey != 0)
        panic("default not set?\n");
}

void i2s_init(uint32_t clock) {
    todo("set the gpio pins\n");

    // this checks the invial values from the datasheet.
    pcm_check_initial();

    pcm_clock_init(clock);

    // set cs_a
    //
    // we want:
    //  - master mode
    //  - 48000 clock
    //  - 32 bit frames (T)
    //  - sync low for T/2, sync high for T/2 (sync = 50% duty cycle)
    //  - RX
    //  - no interrupt.
    //  - no grey
    // * configure before you do enable [datasheet is confusing]


    // page 129
    // mode bits:
    // these we don't set but are all 0s:
    //  - don't disable clock [28=0]
    //  - no decimation(?) [27=0]
    //  - disable pdm [26=0]
    //  - don't received packed [25=0]
    //  - don't xmit packed [24=0]
    //  - don't invert clock [22=0]
    //  - don't invert frame [20=0]
    //
    // 
    // these we do set [many 0s]:
    //  - FSM[23]: master mode [23=0]
    //  - FLEN[19:10]: frame length = 24 bits, one bit per clock.
    //    [23 << 10]
    //  - FSLEN[9:0]: frame sync length [one down or one up: so half FLEN]
    //    [12]

    todo("set mode_a\n");


    // page 131: receive config: rxc_a
    todo("set rxc_a\n");

    todo("set cs_a\n");
    dev_barrier();
}

uint32_t i2s_get32(void) {
    todo("read from fifo if data is present\n");
}
