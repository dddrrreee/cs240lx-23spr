// ping pong a word back and forth.
#include "rpi.h"
#include "ir-put-get.h"

static void ping_pong(void *is_server) {
    unsigned send = 0;

    char *name = is_server ? "server" : "client";

    // ugh: can't ping pong without multiple devices.
    if(is_server) {
        output("%s: about to send a 32-bit 0\n", name);

        send = 1;
        ir_put32(out_pin, send);
    } 

    enum { ntrials = 127 };
    // we expect to get back what we send + 1
    for(unsigned i = 0; i < ntrials; i++) {
        unsigned got = ir_get32(in_pin);
        if(got != send+1)
            panic("%s: failed: expected %d, got=%d\n", 
                name, send+1, got);

        send = got + 1;
        output("%s: success: received=%d, sending=%d\n", 
                name, got, send);
        ir_put32(out_pin, send);
    }
    output("%s: done!\n", name);
}

void notmain(void) {
    caches_enable();
    gpio_set_input(in_pin);
    gpio_set_pullup(in_pin);

    gpio_set_output(out_pin);
    gpio_write(out_pin, 0);

    int is_server;
    rpi_fork(ping_pong,0)->annot = "client";
    rpi_fork(ping_pong,&is_server)->annot = "server";

    rpi_thread_start();
    output("done running\n");
}
