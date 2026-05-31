#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

typedef struct {
    uint32_t *address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
} framebuffer_t;

extern framebuffer_t framebuffer;

void framebuffer_init(
    uint32_t *address,
    uint32_t width,
    uint32_t height,
    uint32_t pitch,
    uint8_t bpp
);
void init_terminal_bounds();
void put_pixel(uint32_t x, uint32_t y, uint32_t colour);

#endif
