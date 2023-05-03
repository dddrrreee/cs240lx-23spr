#ifndef __WS2812B__
#define __WS2812B__
/*
 * engler: possible interface fo ws2812b light strip, cs240lx
 *
 * the low-level "driver" code for the WS2812B addressable LED light strip.  
 *      Datasheet in docs/WS2812B
 *
 * the higher level support stuff is in neopix.c
 * 
 * Key sentence  missing from most documents:
 *	Addressing is indirect: you don’t pass a NeoPixel’s address
 *	via SPI. Instead the order of data in the frame provides the
 *	addressing: the first three color values are taken by the first
 *	NeoPixel in line, the second set of values by the next NeoPixel,
 *	the third by the third and so on.
 *		--- https://developer.electricimp.com/resources/neopixels
 *
 * 	I.e., it's not a FIFO buffer, the first thing you write gets claimed
 *	by the first LED, etc.
 * 
 * to send a:
 *	- 0 = T0H, T0L
 * 	- 1 = T1H, T1L
 * 	- FLUSH = how long to wait with voltage low to flush it out.
 *
 * One way to communicate more information with a device is to use a fancier
 * protocol (e.g., SPI, I2C).  A simpler hack is to use time --- there's a lot 
 * of information you can encode in various numbers of nanoseconds.    
 *
 * The WS2812B uses time as follows:
 *      To send a 1 bit: write a 1 for T1H, then a 0 for T0H.
 *      To send a 0 bit: write a 1 for T1L, then a 0 for T0L.
 *
 * The protocol:
 *   - Each pixel needs 3 bytes of information (G,R,B) [in that order] for the color.  
 *   - The first 3 bytes you sent get claimed by the first pixel, the second 3 bytes 
 *     by the second, and so forth.
 *   - You then hold the communication pin low for FLUSH cycles to tell the pixel you
 *        are done.
 *  
 * The data sheet for WS2812B says the following timings: 
 *      T0H    0: .35us +- .15us
 *      T1H    1: .9us +- .15us
 *      T0L    0: .9us +- .15us
 *      T1L    1: .35us +- .15us
 *      Trst = 80us 
 *
 *  The above timings work. However, for long light strings, they can consume significant
 *  time.   Ideally we'd like to push the times as low as possible so that we can set/unset
 *  more pixels per second.   The following *seem* to work on the light strings I have,
 *  but could break on others (or could break on mine with the wrong set of pixels).   Use
 *  with caution:
 *      T0H = 200ns + eps
 *      T1H = 750ns + eps
 *      T0L = 750ns + eps
 *      T1L = 200ns + eps
 */
#include "cycle-count.h"

#define MHz 700UL


// you'll need to define these values in terms of cycles.
enum {
        T1H,        // Width of a 1 bit in ns
        T0H,        // Width of a 0 bit in ns
        // to send a 0: set pin high for T1L ns, then low for T0L ns.
        T1L,        // Width of a 1 bit in ns
        T0L,        // Width of a 0 bit in ns

        // to make the LED switch to the new values, old the pin low for FLUSH ns
        FLUSH    // how long to hold low to flush
};

// duplicate set_on/off so we can inline to reduce overhead.
// they have to run in < the delay we are shooting for.
static inline void gpio_set_on_raw(unsigned pin) {
    unimplemented();
}
static inline void gpio_set_off_raw(unsigned pin) {
    unimplemented();
}


static inline void delay_ncycles(unsigned start, unsigned ncycles)  {
    unimplemented();
}


// implement T1H from the datasheet (call write_1 with the right delay)
static inline void t1h(unsigned pin) {
    unimplemented();
}

// implement T0H from the datasheet (call write_0 with the right delay)
static inline void t0h(unsigned pin) {
    unimplemented();
}
// implement T1L from the datasheet.
static inline void t1l(unsigned pin) {
    unimplemented();
}
// implement T0L from the datasheed.
static inline void t0l(unsigned pin) {
    unimplemented();
}
// implement RESET from the datasheet.
static inline void treset(unsigned pin) {
    unimplemented();
}

// flush out the pixels.
static inline void pix_flush(unsigned pin) { 
    treset(pin); 
}

// send bytes [<r> red, <g> green, <b> blue out on pin <pin>.
static inline void 
pix_sendpixel(unsigned pin, uint8_t r, uint8_t g, uint8_t b) {
    unimplemented();
}
#endif
