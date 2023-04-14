#include "rpi.h"
#include "ir-put-get.h"

#include "../small-prog/code-hello.h"

void bootload_server(void *input) {
    output("about to send nbytes=%d\n", sizeof hello);
    ir_send_pkt(&hello, sizeof hello + hello.nbytes);
}

void bootload_client(void *input) {
    static char data[1024];
    unsigned n = ir_recv_pkt(data, sizeof data);
    if(!n)
        panic("recv failed!!\n");

    struct prog *p = (void*)data;
    output("got: name=<%s> nbytes=%d, first word=%x\n", 
        p->name, p->nbytes, *(unsigned*)p->code);

    struct bin_header *h = (void*)p->code;

    if(h->cookie != 0x12345678)
        panic("bad cookie: %x\n", h->cookie);
    output("success: %x\n", h->cookie);

    unsigned tot = h->code_nbytes + h->header_nbytes;
    output("about to copy to addr=%x, nbytes=%d\n", h->link_addr, tot);
    memcpy((void*)h->link_addr,  h, tot);
    output("about to call: %x!\n", h->link_addr);
    BRANCHTO(h->link_addr);
    not_reached();
}


void notmain(void) {
    caches_enable();
    gpio_set_input(in_pin);
    gpio_set_pullup(in_pin);

    gpio_set_output(out_pin);
    gpio_write(out_pin, 0);

    rpi_fork(bootload_client,0);
    rpi_fork(bootload_server,0);

    rpi_thread_start();
    output("done running\n");
}
