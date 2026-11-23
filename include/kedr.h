#ifndef KEDR_H
#define KEDR_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t *address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;

    uint8_t red_position;
    uint8_t red_size;

    uint8_t green_position;
    uint8_t green_size;

    uint8_t blue_position;
    uint8_t blue_size;
} kedr_config_t;

// This thing runs the independent early framebuffer
// diagnostic.
//
// It returns 1 if the diagnostic completed,
// and 0 otherwise. Try to keep it simple.
int kedr_run (const kedr_config_t *config);

#endif
