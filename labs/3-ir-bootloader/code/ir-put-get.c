#include "rpi.h"
#include "rpi-thread.h"
#include "cycle-count.h"
#include "cycle-util.h"
#include "ir-put-get.h"

#include "fast-hash32.h"


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
