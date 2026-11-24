#include "../include/kedr.h"
#include "../include/kernos8x16.h"

#include <stdint.h>

#define KEDR_FONT_WIDTH 8
#define KEDR_FONT_HEIGHT 16

#define KEDR_BLACK 0x000000
#define KEDR_WHITE 0xFFFFFF
#define KEDR_RED 0xFF0000
#define KEDR_GREEN 0x00FF00
#define KEDR_BLUE 0x0000FF

static kedr_config_t kedr;

// Convert an 8-bit colour component into the framebuffer's
// specified channel width and position.

static uint32_t kedr_pack_channel (
    uint8_t value, uint8_t position, uint8_t size
)
{
    uint32_t maximum = (1U << size) - 1U;
    uint32_t scaled =
        ((uint32_t)value * maximum + 127U) / 255U;

    return scaled << position;
}

// Convert conventional 0xRRGGBB into the framebuffer's
// actual RGB bit layout.

static uint32_t kedr_pack_colour (
    uint32_t colour
)
{
    uint8_t red = (colour >> 16) & 0xFF;
    uint8_t green = (colour >> 8) & 0xFF;
    uint8_t blue = colour & 0xFF;

    return kedr_pack_channel(red,
                             kedr.red_position,
                             kedr.red_size) |
           kedr_pack_channel(green,
                             kedr.green_position,
                             kedr.green_size) |
           kedr_pack_channel(blue,
                             kedr.blue_position,
                             kedr.blue_size);
}

static void kedr_put_pixel (
    uint32_t x, uint32_t y, uint32_t colour
)
{
    if (x >= kedr.width || y >= kedr.height)
    {
        return;
    }

    uint32_t offset = (y * kedr.pitch) / 4U + x;

    kedr.address[offset] = kedr_pack_colour(colour);
}

static void kedr_draw_rectangle (
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t colour
)
{
    if (x >= kedr.width || y >= kedr.height)
    {
        return;
    }

    if (width > kedr.width - x)
    {
        width = kedr.width - x;
    }

    if (height > kedr.height - y)
    {
        height = kedr.height - y;
    }

    for (uint32_t row = 0; row < height; row++)
    {
        for (uint32_t column = 0; column < width; column++)
        {
            kedr_put_pixel(x + column, y + row, colour);
        }
    }
}

static void kedr_draw_character (
    uint32_t x,
    uint32_t y,
    uint8_t character,
    uint32_t foreground,
    uint32_t background
)
{
    const uint8_t *glyph = kernos_font8x16[character];

    for (uint32_t row = 0; row < KEDR_FONT_HEIGHT; row++)
    {
        uint8_t bits = glyph[row];

        for (uint32_t column = 0; column < KEDR_FONT_WIDTH;
             column++)
        {
            uint32_t colour = (bits & (0x80U >> column))
                                  ? foreground
                                  : background;

            kedr_put_pixel(x + column, y + row, colour);
        }
    }
}

static void kedr_draw_string (
    uint32_t x,
    uint32_t y,
    const char *string,
    uint32_t foreground,
    uint32_t background
)
{
    uint32_t start_x = x;

    while (*string)
    {
        if (*string == '\n')
        {
            x = start_x;
            y += KEDR_FONT_HEIGHT;
            string++;
            continue;
        }

        if (x + KEDR_FONT_WIDTH > kedr.width)
        {
            break;
        }

        if (y + KEDR_FONT_HEIGHT > kedr.height)
        {
            break;
        }

        kedr_draw_character(
            x, y, (uint8_t)*string, foreground, background
        );

        x += KEDR_FONT_WIDTH;
        string++;
    }
}

static int kedr_valid_config (
    const kedr_config_t *config
)
{
    if (!config || !config->address)
    {
        return 0;
    }

    if (config->width == 0 || config->height == 0)
    {
        return 0;
    }

    if (config->width > UINT32_MAX / 4U)
    {
        return 0;
    }

    if (config->pitch == 0 || config->pitch % 4U != 0)
    {
        return 0;
    }

    if (config->pitch < config->width * 4U)
    {
        return 0;
    }

    if ((uint64_t)config->height * config->pitch >
        UINT32_MAX)
    {
        return 0;
    }

    if (config->red_size == 0 || config->red_size > 8 ||
        config->green_size == 0 || config->green_size > 8 ||
        config->blue_size == 0 || config->blue_size > 8)
    {
        return 0;
    }

    if (config->red_position + config->red_size > 32 ||
        config->green_position + config->green_size > 32 ||
        config->blue_position + config->blue_size > 32)
    {
        return 0;
    }

    return 1;
}

int kedr_run (
    const kedr_config_t *config
)
{
    if (!kedr_valid_config(config))
    {
        return 0;
    }

    kedr = *config;

    // Time to fill the display using KEDR's own
    // pixel-writing thingy

    kedr_draw_rectangle(
        0, 0, kedr.width, kedr.height, KEDR_BLACK
    );

    // And to draw three large colour testing regions. To
    // know that KEDR is actually up and not LARPing.

    uint32_t test_width = kedr.width / 6U;
    uint32_t test_height = kedr.height / 6U;
    uint32_t test_y = 64U;

    kedr_draw_rectangle(
        16U, test_y, test_width, test_height, KEDR_RED
    );

    kedr_draw_rectangle(32U + test_width,
                        test_y,
                        test_width,
                        test_height,
                        KEDR_GREEN);

    kedr_draw_rectangle(48U + test_width * 2U,
                        test_y,
                        test_width,
                        test_height,
                        KEDR_BLUE);

    // Now we will render some diagnostic test using our
    // classic Kernos font.

    kedr_draw_string(16U,
                     16U,
                     "=== KERNOS EARLY DIAGNOSTICS ===",
                     KEDR_WHITE,
                     KEDR_BLUE);

    kedr_draw_string(16U,
                     48U,
                     "KEDR v0.1 initialised",
                     KEDR_WHITE,
                     KEDR_BLACK);

    kedr_draw_string(16U,
                     test_y + test_height + 24U,
                     "RED GREEN BLUE TEST REGIONS",
                     KEDR_WHITE,
                     KEDR_BLACK);

    kedr_draw_string(16U,
                     test_y + test_height + 56U,
                     "DIRECT FRAMEBUFFER WRITES COMPLETED",
                     KEDR_GREEN,
                     KEDR_BLACK);

    // Temporary: Halt the kernel so bare-metal knows kmain
    // is being reached
    __asm__ volatile("cli\n"
                     "1:\n"
                     "hlt\n"
                     "jmp 1b\n" ::
                         : "memory");

    return 1;
}
