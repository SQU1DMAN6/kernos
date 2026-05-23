#include "power.h"
#include "io.h"

void reboot(void)
{
    // Disable CPU interrupts
    __asm__ __volatile__("cli");

    // Load an empty Interrupt Descriptor Table
    struct {
        unsigned short limit;
        unsigned int base;
    } __attribute__((packed)) idt = { 0, 0 };

    __asm__ __volatile("lidt %0" : : "m"(idt));

    // Trigger the interrupt
    __asm__ __volatile__("int $0x03");

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
