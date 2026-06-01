#ifndef TIMER_H
#define TIMER_H

extern volatile unsigned int timer_ticks;

void timer_init(void);
void timer_handler_main(void);

#endif
