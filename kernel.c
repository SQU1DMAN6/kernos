#include "terminal.h"
#include "idt.h"
#include "pic.h"
#include "memory.h"
#include "vfs.h"
#include "framebuffer.h"
#include "timer.h"
#include <stdint.h>
#include <stddef.h>

struct mb_tag {
    uint32_t type;
    uint32_t size;
};

struct mb_framebuffer_tag {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
} __attribute__((packed));

extern void keyboard_handler(void);

static void init_framebuffer(uint32_t mb_info)
{
    uint8_t *ptr = (uint8_t *)mb_info;

    uint32_t total_size = *(uint32_t *)ptr;
    ptr += 8;

    uint8_t *end = (uint8_t *)mb_info + total_size;

    while (ptr < end) {
        struct mb_tag *tag = (struct mb_tag *)ptr;

        if (tag->type == 0)
            break;

        if (tag->type == 8) {
            uint8_t *t = (uint8_t *)tag;

            uint64_t addr = *(uint64_t *)(t + 8);
            uint32_t pitch = *(uint32_t *)(t + 16);
            uint32_t width = *(uint32_t *)(t + 20);
            uint32_t height = *(uint32_t *)(t + 24);
            uint8_t bpp = *(uint8_t *)(t + 28);

            framebuffer_init(
                (uint32_t *)(uintptr_t)addr,
                width,
                height,
                pitch,
                bpp
            );

            return;
        }

        ptr += (tag->size + 7) & ~7;
    }
}

void kmain(uint32_t magic, uint32_t mb_info)
{
    (void)magic;
    
    init_framebuffer(mb_info);

    clear_screen();

    kprint("Booting Kernos...\n");

    heap_init();

    kprint("[  OK  ] Heap initialisation successful\n");

    fs_init();

    kprint("[  OK  ] File system initialisation successful\n");

    idt_init();

    kprint("[  OK  ] IDT initialisation successful\n");

    timer_init();

    kprint("[  OK  ] Timer initialisation successful\n");

    __asm__ __volatile__("sti");

    kprint("[  OK  ] System is stable\n");

    kb_init();

    kprint("[  OK  ] Keyboard initialisation successful\n");

    kprintln();
    kprint("         Welcome to Kernos, written by Quan Thai\n");
    kprint("         Type `help` for information on available commands\n\n");


    shell_prompt();

    while (1) {
        render_terminal();
        render_cursor();
        __asm__("hlt");
    }
}
