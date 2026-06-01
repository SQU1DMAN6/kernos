#include "timer.h"
#include "io.h"

volatile unsigned int timer_ticks = 0;

void timer_handler_main(void)
{
    timer_ticks++;

    write_port(0x20, 0x20);
}

void timer_init(void)
{
    unsigned int divisor = 1193180 / 100;

    write_port(0x43, 0x36);

    write_port(0x40, divisor & 0xFF);
    write_port(0x40, (divisor >> 8) & 0xFF);
}
