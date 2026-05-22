#include "idt.h"
#include "pic.h"
#include "io.h"

#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

extern void keyboard_handler(void);

struct IDT_entry {
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
};

static struct IDT_entry IDT[IDT_SIZE];

void idt_init(void)
{
    // Remap PICs first
    pic_remap_and_mask();

    unsigned long keyboard_address;
    struct idt_ptr {
        unsigned short limit;
        unsigned int base;
    } __attribute__((packed));

    struct idt_ptr idtp;

    // Populate the IDT entry of keyboard's interrupt
    keyboard_address = (unsigned long)keyboard_handler;
    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = INTERRUPT_GATE;
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

    // Fill the IDT descriptor
    idtp.limit = (sizeof(struct IDT_entry) * IDT_SIZE) - 1;
    idtp.base = (unsigned int)&IDT;

    load_idt((unsigned long*)&idtp);
}
