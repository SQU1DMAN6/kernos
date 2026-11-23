#include "include/framebuffer.h"
#include "include/idt.h"
#include "include/kedr.h"
#include "include/memory.h"
#include "include/pic.h"
#include "include/serial.h"
#include "include/terminal.h"
#include "include/timer.h"
#include "include/vfs.h"
#include <stddef.h>
#include <stdint.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

struct mb_tag
{
    uint32_t type;
    uint32_t size;
};

struct mb_framebuffer_tag
{
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;

    // Present for framebuffer_type = 1 , direct RGB.
    uint8_t red_field_position;
    uint8_t red_mask_size;
    uint8_t green_field_position;
    uint8_t green_mask_size;
    uint8_t blue_field_position;
    uint8_t blue_mask_size;
} __attribute__((packed));

extern void keyboard_handler (void);

static int init_framebuffer (
    uint32_t mb_info
)
{
    uint8_t *ptr = (uint8_t *)(uintptr_t)mb_info;

    serial_write_string(
        "[ BOOT ] Multiboot information at "
    );
    serial_write_hex32(mb_info);
    serial_write_char('\n');

    if (mb_info == 0)
    {
        serial_write_string(
            "[ERROR] Null Multiboot information pointer\n"
        );
        return 0;
    }

    uint32_t total_size = *(uint32_t *)ptr;

    if (total_size < 16)
    {
        serial_write_string(
            "[ERROR] Invalic multiboot information size\n"
        );
        return 0;
    }

    if (mb_info > UINT32_MAX - total_size)
    {
        serial_write_string("[ERROR] Multiboot information "
                            "address overflow\n");
        return 0;
    }

    uint8_t *end = ptr + total_size;
    ptr += 8;

    serial_write_string(
        "[ BOOT ] Multiboot information size: "
    );
    serial_write_hex32(total_size);
    serial_write_char('\n');

    while (ptr <= end - 8)
    {
        struct mb_tag *tag = (struct mb_tag *)ptr;

        if (tag->size < 8 ||
            tag->size > (uint32_t)(end - ptr))
        {
            serial_write_string(
                "[ERROR] Invalid Multiboot tag size\n"
            );
            return 0;
        }

        if (tag->type == 0)
        {
            break;
        }

        if (tag->type == 8)
        {
            if (tag->size <
                sizeof(struct mb_framebuffer_tag))
            {
                serial_write_string(
                    "[ERROR] Framebuffer tag too small\n"
                );
                return 0;
            }

            struct mb_framebuffer_tag *fb =
                (struct mb_framebuffer_tag *)tag;

            serial_write_string(
                "[ BOOT ] Framebuffer address: "
            );
            serial_write_hex64(fb->framebuffer_addr);
            serial_write_char('\n');

            serial_write_string(
                "[ BOOT ] Framebuffer width: "
            );
            serial_write_hex32(fb->framebuffer_width);
            serial_write_char('\n');

            serial_write_string(
                "[ BOOT ] Framebuffer height: "
            );
            serial_write_hex32(fb->framebuffer_height);
            serial_write_char('\n');

            serial_write_string(
                "[ BOOT ] Framebuffer pitch: "
            );
            serial_write_hex32(fb->framebuffer_pitch);
            serial_write_char('\n');

            serial_write_string(
                "[ BOOT ] Framebuffer BPP: "
            );
            serial_write_hex32(fb->framebuffer_bpp);
            serial_write_char('\n');

            serial_write_string(
                "[ BOOT ] Framebuffer type: "
            );
            serial_write_hex32(fb->framebuffer_type);
            serial_write_char('\n');

            if (fb->framebuffer_addr == 0 ||
                fb->framebuffer_addr > UINT32_MAX)
            {
                serial_write_string(
                    "[ERROR] Framebuffer address is "
                    "outside the 32-bit address space\n"
                );
                return 0;
            }

            if (fb->framebuffer_width == 0 ||
                fb->framebuffer_height == 0)
            {
                serial_write_string(
                    "[ERROR] Invalud framebuffer "
                    "dimensions\n"
                );
                return 0;
            }

            if (fb->framebuffer_bpp != 32)
            {
                serial_write_string(
                    "[ERROR] Unsupported framebuffer "
                    "colour depth\n"
                );
                return 0;
            }

            if (fb->framebuffer_type != 1)
            {
                serial_write_string("[ERROR] Framebuffer "
                                    "is not direct RGB\n");
                return 0;
            }

            if (fb->framebuffer_width > UINT32_MAX / 4)
            {
                serial_write_string(
                    "[ERROR] Framebuffer width overflow\n"
                );
                return 0;
            }

            if (fb->framebuffer_pitch <
                fb->framebuffer_width * 4)
            {
                serial_write_string("[ERROR] Framebuffer "
                                    "pitch is too small\n");
                return 0;
            }

            serial_write_string(
                "[ BOOT ] Starting KEDR diagnostics\n"
            );

            kedr_config_t kedr_config = {
                .address = (volatile uint32_t *)(uintptr_t)
                               fb->framebuffer_addr,
                .width = fb->framebuffer_width,
                .height = fb->framebuffer_height,
                .pitch = fb->framebuffer_pitch,

                .red_position = fb->red_field_position,
                .red_size = fb->red_mask_size,

                .green_position = fb->green_field_position,
                .green_size = fb->green_mask_size,

                .blue_position = fb->blue_field_position,
                .blue_size = fb->blue_mask_size};

            if (kedr_run(&kedr_config))
            {
                serial_write_string(
                    "[ BOOT ] KEDR diagnostic completed\n"
                );
            }
            else
            {
                serial_write_string(
                    "[ERROR] KEDR rejected framebuffer "
                    "configuration\n"
                );
            }

            // Now we'll init the normal framebuffer driver
            // after KEDR is done cooking
            framebuffer_init(
                (uint32_t *)(uintptr_t)fb->framebuffer_addr,
                fb->framebuffer_width,
                fb->framebuffer_height,
                fb->framebuffer_pitch,
                fb->framebuffer_bpp
            );

            serial_write_string(
                "[ BOOT ] Framebuffer driver initialised\n"
            );

            return 1;
        }

        uint32_t next_tag = (tag->size + 7) & ~7U;

        if (next_tag < tag->size ||
            next_tag > (uint32_t)(end - ptr))
        {
            serial_write_string(
                "[ERROR] Multiboot tag alignment overflow\n"
            );
            return 0;
        }

        ptr += next_tag;
    }

    serial_write_string(
        "[ERROR] No framebuffer tag found\n"
    );
    return 0;
}

static void debug_halt (
    const char *message
)
{
    serial_write_string("[ FATAL ]  ");
    serial_write_string(message);
    serial_write_char('\n');

    __asm__ __volatile__("cli");

    while (1)
    {
        __asm__ __volatile__("hlt");
    }
}

void kmain (
    uint32_t magic, uint32_t mb_info
)
{
    serial_write_string("[ BOOT ] Enterered kmain()\n");

    // Validate the Multiboot2 handover
    serial_write_string("[ BOOT ] Bootloader magic: ");
    serial_write_hex32(magic);
    serial_write_char('\n');

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        debug_halt("Invalid Multiboot2 bootloader magic");
    }

    // Initialise the framebuffer before using the terminal
    if (!init_framebuffer(mb_info))
    {
        debug_halt("Framebuffer initialisation failed");
    }

    // Test the first terminal output separately.
    serial_write_string(
        "[ TEST ] About to call first kprint()\n"
    );

    kprint("Booting Kernos...\n");

    serial_write_string(
        "[ TEST ] First kprint() returned!\n"
    );

    serial_write_string(
        "[ BOOT ] Starting heap initialisation\n"
    );
    heap_init();
    serial_write_string(
        "[ BOOT ] Heap initialisation successful\n"
    );
    kprint("[  OK  ] Heap initialisation successful\n");

    serial_write_string(
        "[ BOOT ] Starting filesystem initialisation\n"
    );
    fs_init();
    serial_write_string(
        "[ BOOT ] Filesystem init returned\n"
    );
    kprint(
        "[  OK  ] File system initialisation successful\n"
    );

    serial_write_string(
        "[ BOOT ] Starting IDT initialisation\n"
    );
    idt_init();
    serial_write_string(
        "[ BOOT ] IDT initialisation returned\n"
    );
    kprint("[  OK  ] IDT initialisation successful\n");

    serial_write_string(
        "[ BOOT ] Starting timer initialisation\n"
    );
    timer_init();
    serial_write_string(
        "[ BOOT ] Timer initialisation returned\n"
    );
    kprint("[  OK  ] Timer initialisation successful\n");

    serial_write_string("[ BOOT ] Enabling interrupts\n");
    __asm__ __volatile__("sti");
    kprint("[  OK  ] System is stable\n");

    serial_write_string(
        "[ BOOT ] Starting keyboard initialisation\n"
    );
    kb_init();
    serial_write_string(
        "[ BOOT ] Keyboard initialisation returned\n"
    );
    kprint("[  OK  ] Keyboard initialisation successful\n");

    kprintln();

    kprint(
        "        Welcome to Kernos, written by Quan Thai\n"
    );
    kprint("        Type 'help' for information on "
           "available commands\n\n");

    serial_write_string("[ BOOT ] Starting shell prompt\n");
    shell_prompt();
    serial_write_string("[ BOOT ] Shell prompt returned\n");

    serial_write_string(
        "[ BOOT ] Entering terminal loop\n"
    );

    while (1)
    {
        render_terminal();
        render_cursor();

        __asm__ __volatile__("hlt");
    }
}
