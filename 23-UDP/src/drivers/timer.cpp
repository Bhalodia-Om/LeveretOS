#include "timer.h"
#include "io.h"    // outb

// The PIT's fixed base frequency, in hertz. It counts down from the divisor and fires every time it reaches zero. So the rate of fire is base / divisor.
static const uint32_t PIT_BASE = 1193182;

// The PIT ports are the channel 0 data port, and the command port.
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43

// Shared with the interrupt handler, so it needs to be volatile.
static volatile uint32_t ticks = 0;
static uint32_t frequency_hz = 0;

void timer_init(uint32_t frequency) {
    frequency_hz = frequency; 

    // Work out the divisor, base / desired rate.
    uint32_t divisor = PIT_BASE / frequency;

    // 0x36 = 00110110
    // bits 7-6 (00): channel 0,
    // bits 5-4 (11): data port is only 8 bits, so we need to send our 16 bit number in two writes. 11 means the lower bit will come before the high bit.
    // bits 3-1 (011): the operating mode for the PIT. We are setting it to mode 3, or the "square wave generator". Fires like a metronome.
    // bit  0   (0): tells the PIT we are giving the number in binary format.
    outb(PIT_COMMAND, 0x36);

    // Send the divisor in two bytes. First the low 8 bits, then high 8 bits.
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_tick() {
    ticks++;
}

uint32_t timer_ticks()     { return ticks; }
uint32_t timer_frequency() { return frequency_hz; }