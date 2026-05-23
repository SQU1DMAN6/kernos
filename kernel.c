#include "terminal.h"
#include "idt.h"
#include "pic.h"
#include "memory.h"

extern void keyboard_handler(void);

void kmain(void)
{
    clear_screen();
    kprint("Booting Kernos...\n");

    heap_init();

    kprint("[  OK  ] Heap initialisation successful\n");

    idt_init();

    kprint("[  OK  ] IDT initialisation successful\n");

    __asm__ __volatile__("sti");

    kprint("[  OK  ] System is stable\n");

    kb_init();

    kprint("[  OK  ] Keyboard initialisation successful\n");

    kprint("         Welcome to Kernos, written by Quan Thai\n\n");

    shell_prompt();

    while(1) {
        render_terminal();
        __asm__("hlt");
    }
}
