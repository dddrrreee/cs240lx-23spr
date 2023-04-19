// used to hold example C code that you want to see asm for.
#include "rpi.h"

void call_hello(void) {
    printk("hello world\n");
}

void notmain(void) {
}
