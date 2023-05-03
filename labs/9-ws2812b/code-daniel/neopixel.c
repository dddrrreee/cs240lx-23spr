/*
 * "higher-level" neopixel stuff.
 * our model:
 *  1. you write to an array with as many entries as there are pixels as much as you 
 *     want
 *  2. you call <neopix_flush> when you want to externalize this.
 *
 * Note:
 *  - out of bound writes are ignored.
 *  - do not allow interrupts during the flush!
 */
#include "rpi.h"
#include "neopixel.h"
#include "WS2812B.h"

void neopix_flush(neo_t h) { 
    todo("implement this");
}

neo_t neopix_init(uint8_t pin, unsigned npixel) {
    todo("implement this");
}

// this just makes the definition visible in other modules.
void neopix_sendpixel(neo_t h, uint8_t r, uint8_t g, uint8_t b) {
    todo("implement this");
}

// doesn't need to be here.
void neopix_fast_clear(neo_t h, unsigned n) {
    todo("impleent this");
}

void neopix_clear(neo_t h) {
    todo("impleent this");
}

// set pixel <pos> in <h> to {r,g,b}
void neopix_write(neo_t h, uint32_t pos, uint8_t r, uint8_t g, uint8_t b) {
    unimplemented();
}
