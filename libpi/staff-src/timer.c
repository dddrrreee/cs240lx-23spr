#include "rpi.h"

// no dev barrier: 
uint32_t timer_get_usec_raw(void) {
    return GET32(0x20003004);
}

// in usec.  the lower 32-bits of the usec 
// counter: if you investigate in the broadcom 
// doc can see how to get the high 32-bits too.
uint32_t timer_get_usec(void) {
    dev_barrier();
    uint32_t u = timer_get_usec_raw();
    dev_barrier();
    return u;
}

// call rpi_wait() while timer not expired ---
// client can override this.
void delay_us(uint32_t us) {
    uint32_t s = timer_get_usec();
    while((timer_get_usec() - s) < us)
        rpi_wait();
}

// delay in milliseconds
void delay_ms(uint32_t ms) {
    delay_us(ms*1000);
}

// delay in seconds
void delay_sec(uint32_t sec) {
    delay_ms(sec*1000);
}
