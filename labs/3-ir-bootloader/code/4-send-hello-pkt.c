// send "hello world" using a packet.
#include "rpi.h"
#include "ir-put-get.h"

// how much extra bytes
enum { extra_bytes = 31 };

void recv_pkt_thread(void *input) {
    static char data[1024];
    
    uint32_t start = timer_get_usec();

    unsigned n = ir_recv_pkt(data, sizeof data);
    if(!n)
        panic("recv failed!!\n");

    data[n] = 0;
    output("got: <%s>\n", data);

    uint32_t tot_usec = timer_get_usec() - start;
    uint32_t baud = (n*1000*1000)/tot_usec;
    output("total usec=%d: baud= %d bytes/sec\n", 
        tot_usec, baud);
}

void xmit_pkt_thread(void *input) {
    // send some extra
    char *hello = "hello world\n";
    ir_send_pkt(hello, strlen(hello)+ extra_bytes);
}

void notmain(void) {
    caches_enable();
    gpio_set_input(in_pin);
    gpio_set_pullup(in_pin);

    gpio_set_output(out_pin);
    gpio_write(out_pin, 0);

    rpi_fork(recv_pkt_thread,0);
    rpi_fork(xmit_pkt_thread,0);

    rpi_thread_start();
    output("done running\n");
}
