#include "small-libpi.h"

// no space for reboot.
void notmain(void) {
    putk("hello from bootloaded program!\n");
    clean_reboot();
}
