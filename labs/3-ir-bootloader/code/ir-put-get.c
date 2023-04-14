#include "rpi.h"
#include "rpi-thread.h"
#include "cycle-count.h"
#include "cycle-util.h"
#include "ir-put-get.h"

#include "fast-hash32.h"

// wait for <n_cycles>, yielding on each check.
        // we don't need the start stuff for waiting since 
static inline void wait_ncycles(unsigned n_cycles) {
    unsigned start = cycle_cnt_read();
    while((cycle_cnt_read() - start) < n_cycles)
        rpi_yield();
}

// set GPIO <pin> = <v> and wait <ncycles>
static inline void
write_for_ncycles(unsigned pin, unsigned v, unsigned ncycles) {
    gpio_write(pin, v);     // inline this?
    wait_ncycles(ncycles);
}

// write 0,1,0,1 etc at 38khz, interleaving with other thread.
static inline void tsop_write_cyc(unsigned pin, unsigned n_cycle) {
    unsigned n = 0;
    unsigned start = cycle_cnt_read();

    // tsop expects a 38khz signal --- this means 38,000 transitions
    // between hi (1) and lo (0) per second.
    //
    // common mistake: setting to hi or lo 38k times per sec rather 
    // than both hi+low (which is 2x as frequent).
    //
    // we use cycles rather than usec for increased precision, but 
    // at the rate the IR works, usec might be ok.
    do {
        // does it matter that we get rid of <start>?  i think
        // should put it back.
        write_for_ncycles(pin, 1, tsop_cycle);
        write_for_ncycles(pin, 0, tsop_cycle);
    } while((cycle_cnt_read() - start) < n_cycle);
}

// modified protocol to write a single 0 or 1.
//
//   to write a 0:
//      1. cycle for burst_0 (so receiver reads 0 for <burst_0> cycles)
//      2. do nothing for <quiet_0> cycles (so IR receiver can reset)
//   to write a 1:
//      1. cycle for burst_1 (so receiver reads 0 for <burst_1> cycles)
//      2. do nothing for <quiet_1> cycles (so IR receiver can reset)
static inline void tsop_write(unsigned pin, unsigned v) {
    if(!v) {
        tsop_write_cyc(pin, burst_0);
        wait_ncycles(quiet_0);
    } else {
        tsop_write_cyc(pin, burst_1);
        wait_ncycles(quiet_1);
    }
}

// read a bit:
//   1. record how long IR receiver it down.
//   2. if closer to <burst_0> cycles, return 0.
//   3. if closer to <burst_1> cycles, return 1.
//
// suggested modifications:
//  - panic if read 0 for "too long" (e.g., to catch
//    a bad jumper.
//  - ignore very short 1s (which can happen if the sender
//    screws up transmission timing).
int tsop_read_bit(unsigned pin) {
    return staff_tsop_read_bit(pin);
}

void ir_put8(unsigned pin, uint8_t c) {
    staff_ir_put8(pin,c);
}

uint8_t ir_get8(unsigned pin) {
    return staff_ir_get8(pin);
}

void ir_put32(unsigned pin, uint32_t x) {
    staff_ir_put32(pin,x);
}

uint32_t ir_get32(unsigned pin) {
    return staff_ir_get32(pin);
}

// bad form to have in/out pins globally.
int ir_send_pkt(void *data, uint32_t nbytes) {
    return staff_ir_send_pkt(data,nbytes);
}

int ir_recv_pkt(void *data, uint32_t max_nbytes) {
    return staff_ir_recv_pkt(data,max_nbytes);
}
