#include "framebuffer.h"
#include "kernos8x16.h"
#include "terminal.h"
#include <stdint.h>

framebuffer_t framebuffer = {0};

void framebuffer_init(
    uint32_t *address,
    uint32_t width,
    uint32_t height,
    uint32_t pitch,
    uint8_t bpp
)
{
    framebuffer.address = address;
    framebuffer.width = width;
    framebuffer.height = height;
    framebuffer.pitch = pitch;
    framebuffer.bpp = bpp;
}

void init_terminal_bounds()
{
    // TERM_W = framebuffer.width / KERNOS_FONT_WIDTH;
    // TERM_H = framebuffer.height / KERNOS_FONT_HEIGHT;
}

void put_pixel(uint32_t x, uint32_t y, uint32_t colour)
{
    if (!framebuffer.address || framebuffer.width == 0 || framebuffer.height == 0)
        return;

    if (x >= framebuffer.width || y >= framebuffer.height)
        return;

    uint8_t *base = (uint8_t *)framebuffer.address;
    uint8_t *pixel = base + (y * framebuffer.pitch) + (x * (framebuffer.bpp / 8));

    *(uint32_t *)pixel = colour;
}
