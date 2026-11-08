#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// The PIC (Programmable Interrupt Controller) is the controller between hardware devices, like a keyboard or timer, and the cpu. It will create interrupts for us.
// The problem is by default the IRQs fall on the CPU interrupt number os 0-15, colliding with our CPU exceptions.
// pic_remap moves them to 32-47 to avoid overlap.
void pic_remap();

// After handling a hardware interrupt, we need to tell the PIC we're done, or it will not send any more. This sends the End Of Interrupt signal.
void pic_send_eoi(uint8_t irq); // IRQ is the IRQ number of the interrupt, from (0-15).

#endif // PIC_H