#include "rpi-thread.h"
#include "cycle-count.h"
#include "cycle-util.h"

enum {
    // should not be here!
    in_pin = 21,
    out_pin = 20 ,

    tsop_cycle = 9210,
#if 1
    burst_1 = 100 * tsop_cycle,
    quiet_1 = 4*burst_1,
    burst_0 = 200 * tsop_cycle,
    quiet_0 = 4*burst_0
#else
    burst_1 = 40 * tsop_cycle,
    quiet_1 = 4*burst_1,
    burst_0 = 80 * tsop_cycle,
    quiet_0 = 4*burst_0
#endif
};

enum {
    PKT_HDR = 0xfeedface,
    PKT_HDR_ACK = 0xdeadbeef,
    PKT_DATA = 0x11223344,
    PKT_DATA_ACK = 0x55667788
};

int tsop_read_bit(unsigned pin);
int staff_tsop_read_bit(unsigned pin);

void ir_put8(unsigned pin, uint8_t c);
uint8_t ir_get8(unsigned pin);

void staff_ir_put8(unsigned pin, uint8_t c);
uint8_t staff_ir_get8(unsigned pin);

void ir_put32(unsigned pin, uint32_t x);
uint32_t ir_get32(unsigned pin);
void staff_ir_put32(unsigned pin, uint32_t x);
uint32_t staff_ir_get32(unsigned pin);

int ir_send_pkt(void *data, uint32_t nbytes);
int ir_recv_pkt(void *data, uint32_t max_nbytes);

int staff_ir_send_pkt(void *data, uint32_t nbytes);
int staff_ir_recv_pkt(void *data, uint32_t max_nbytes);
