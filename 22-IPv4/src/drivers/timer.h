#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// The PIT (Programmable Interval Timer) fires a hardware interrupt at a steady rate.  timer_init sets that rate (in hertz) and unmasks it.
void timer_init(uint32_t frequency);

// Called by the timer interrupt handler (idt.cpp) each time the PIT fires. It raises the tick count by 1.
void timer_tick();

// The current tick count since boot, and the rate we set, so other code can turn ticks into seconds (seconds = ticks / frequency).
uint32_t timer_ticks();
uint32_t timer_frequency();

#endif // TIMER_H
