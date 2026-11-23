#include "../../include/serial.h"
#include <stdint.h>

#define COM1_BASE 0x3F8

#define SERIAL_DATA (COM1_BASE + 0)
#define SERIAL_INTERRUPT (COM1_BASE + 1)
#define SERIAL_FIFO (COM1_BASE + 2)
#define SERIAL_LINE (COM1_BASE + 3)
#define SERIAL_MODEM (COM1_BASE + 4)
#define SERIAL_STATUS (COM1_BASE + 5)

static inline void outb (
    uint16_t port, uint8_t value
)
{
    __asm__ volatile("outb %0, %1"
                     :
                     : "a"(value), "Nd"(port));
}

static inline uint8_t inb (
    uint16_t port
)
{
    uint8_t value;

    __asm__ volatile("inb %1, %0"
                     : "=a"(value)
                     : "Nd"(port));

    return value;
}

void serial_init (
    void
)
{
    // Disable UART interrupts
    outb(SERIAL_INTERRUPT, 0x00);

    // Enable divisor latch access
    outb(SERIAL_LINE, 0x80);

    // Divisor 3 = 38400 baud with 115200 Hz base...
    // buzzwords.
    outb(SERIAL_DATA, 0x03);
    outb(SERIAL_INTERRUPT, 0x00);

    // 8 data bits, no parity, one stop bit.
    outb(SERIAL_LINE, 0x03);

    // Enable FIFO
    outb(SERIAL_FIFO, 0xC7);

    // Enable DTR, RTS, and OUT2
    outb(SERIAL_MODEM, 0x0B);
}

void serial_write_char (
    char c
)
{
    uint32_t timeout = 100000;

    while (timeout--)
    {
        if (inb(SERIAL_STATUS) & 0x20)
        {
            outb(SERIAL_DATA, (uint8_t)c);
            return;
        }
    }
}

void serial_write_string (
    const char *str
)
{
    if (!str)
    {
        return;
    }

    while (*str)
    {
        serial_write_char(*str++);
    }
}

void serial_write_hex32 (
    uint32_t value
)
{
    static const char digits[] = "0123456789ABCDEF";

    serial_write_string("0x");

    for (int i = 7; i >= 0; i--)
    {
        uint32_t digit = (value >> (i * 4)) & 0xF;
        serial_write_char(digits[digit]);
    }
}

void serial_write_hex64 (
    uint64_t value
)
{
    static const char digits[] = "0123456789ABCDEF";

    serial_write_string("0x");

    for (int i = 15; i >= 0; i--)
    {
        uint32_t digit = (value >> (i * 4)) & 0xF;
        serial_write_char(digits[digit]);
    }
}
