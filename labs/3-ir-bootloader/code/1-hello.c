#include "rpi.h"
#include "ir-put-get.h"

// send hello
void xmit_hello_thread(void *nul) {
    const char *hello = "hello world\n";
    output("xmit thread: about to send <%s>\n", hello);

    for(const char *p = hello; *p; p++)
        ir_put8(out_pin, *p);

    output("xmit: done!\n");
}

// receive hello
void recv_hello_thread(void *nul) {
    unsigned i = 0;
    char buf[1024+1];
    for(; i < sizeof buf - 1; i++) {
        if((buf[i] = ir_get8(in_pin)) == '\n')
            break;
    }
    assert(i < sizeof buf);
    buf[i++] = 0;
    output("GOT=<%s>\n",buf);
}

void notmain(void) {
    caches_enable();
    gpio_set_input(in_pin);
    gpio_set_pullup(in_pin);

    gpio_set_output(out_pin);
    gpio_write(out_pin, 0);

    rpi_fork(recv_hello_thread,0);
    rpi_fork(xmit_hello_thread,0);
    rpi_thread_start();
    output("SUCCESS: done\n");
}
