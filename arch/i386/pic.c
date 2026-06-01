#include "pic.h"
#include "io.h"

void pic_remap_and_mask(void)
{
    // ICW1 - begin initialisation
    write_port(0x20, 0x11);
    write_port(0xA0, 0x11);

    // ICW2 - remap the offset address of IDT.
    write_port(0x21, 0x20);
    write_port(0xA1, 0x28);

    // ICW3 - setup cascading.
    write_port(0x21, 0x04);  // Master: slave is on IRQ2
    write_port(0xA1, 0x02);  // Slave: cascade identity = 2

    // ICW4 - environment information (8086/88 mode)
    write_port(0x21, 0x01);
    write_port(0xA1, 0x01);

    // Mask all interrupts initially
    write_port(0x21, 0xFF);
    write_port(0xA1, 0xFF);
}

void kb_init(void)
{
    // Enable keyboard IRQ1 on master (bit 1 cleared), mask all slave IRQs.
    write_port(0x21, 0xFC);
    write_port(0xA1, 0xFF);
}
