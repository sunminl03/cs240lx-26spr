// header-only neopixel implementation for small light strips.
// you could make it smaller by inlining <WS2812B.h> and collapsing
// the "abstraction" level.  probaby a better idea.
#ifndef __NEOPIXEL_H__
#define __NEOPIXEL_H__
#include "rpi.h"
#include "WS2812B.h"

// limited by the fact that we don't use an external power supply :)
#define MAX_NEOPIX 128

// from datasheet: r,g,b are 8-bit each (each pixel is 24 bit color)
struct neo_pixel {
    uint8_t r,g,b;
};

// you can add other information to this if you want: this gives us a way to 
// control multiple light arrays concurrently, and to use different brands.
typedef struct {
    uint8_t pin;      // output pin
    uint32_t npixel;  // number of pixesl.

    struct neo_pixel pixels[MAX_NEOPIX];
} neo_t;

// initialize neopix string of <npixels>: data sent on <pin>
static inline neo_t neopix_init(uint8_t pin, unsigned npixel) {
    neo_t h;
    assert(npixel < MAX_NEOPIX);
    memset(&h, 0, sizeof h);
    h.npixel = npixel;
    h.pin = pin;
    gpio_set_output(pin);
    return h;
}

// buffered write {r,g,b} into <h> at position <pos>: last write wins.  
// does not flush out to light array until you call <neopixel_flush>.
//
// ignores if out of bounds (or assert?)
static inline void 
neopix_write(neo_t *h, uint32_t pos, uint8_t r, uint8_t g, uint8_t b) {
    // silently clip
    if(pos >= h->npixel)
        return;
    // todo("write pixel value to position <pos> in <pixels>");
    h->pixels[pos].r = r;
    h->pixels[pos].g = g;
    h->pixels[pos].b = b;
}

// write the pixel out with [r,g,b] using <WS2812b.h>
void neopix_sendpixel(neo_t *h, uint8_t r, uint8_t g, uint8_t b) {
    pix_sendpixel(h->pin, r,g,b);
}

// do the work:
//  1. write all <npixel> pixels out using <neopix_sendpixel>
//  2. then flush.  
//  3. memset the array to 0 after.
static inline void neopix_flush(neo_t *h) {
    // todo("treset");
    for (int i = 0; i < h->npixel; i++) {
        neopix_sendpixel(h, h->pixels[i].r, h->pixels[i].g, h->pixels[i].b);
    }
    pix_flush(h->pin);
    memset(h->pixels, 0xff, h->npixel * sizeof h->pixels[0]);
}

// immediately write black/off (0,0,0) to every pixel upto 
// pixel number <upto>
static inline void neopix_fast_clear(neo_t *h, unsigned upto) {
    for(int i = 0; i < upto; i++)
        neopix_sendpixel(h,0,0,0);
    neopix_flush(h);
}

// same: but write all pixels in <h>
static inline void neopix_clear(neo_t *h) {
    neopix_fast_clear(h, h->npixel);
}

#endif
