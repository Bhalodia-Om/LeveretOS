#include "pic.h"
#include "io.h"      // outb

// Define the two PIC (Programmable Interrupt Controller) chips. Master handles IRQ 0-7, and the slave handles IRQ 8-15, which are chained into the master via IRQ2.
// Each chip has a command port and a data port. 

#define PIC1_COMMAND 0x20   // master command port.
#define PIC1_DATA    0x21   // master data port.
#define PIC2_COMMAND 0xA0   // slave command port.
#define PIC2_DATA    0xA1   // slave data port.

// The byte/port to send the End of interrupt command, or the master command port.
#define PIC_EOI      0x20   // the "End Of Interrupt" command byte.

// Move the PIC's interrupts off 0-15, which are reserved for CPU exceptions to 32-47.
// We do this by sending 4 "Initialization Command Words" (ICWs) in a fixed order.

void pic_remap() {
    // start initialization with ICW1. 0X11 = "begin init" and wait for an ICW4 to arrive.
    // We send this to both command ports. 
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // ICW2 is the actual remap, telling the PIC which CPU interrupt number to map the interrupts to. Master -> 0x20 (32), so from 32-39. Slave -> 0x28 (40), so from 40-47.
    // Send this to the DATA ports.
    outb(PIC1_DATA, 0x20);   // master starts at 32
    outb(PIC2_DATA, 0x28);   // slave starts at 40

    //ICW3 tells the two PIC's how they are wired together. 0x04, with bit 2 set means theres a slave attached to the IRQ2 line. 0x02 means this controller is the slave connected to the master's IRQ2.
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    //ICW4 passes 0x01, or 8086/8088. This represents the mode the CPU runs in.
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    outb(PIC1_DATA, 0xFC);   // master: only IRQ1 (keyboard) and IRQ0 (Timer) allowed
    outb(PIC2_DATA, 0xFF);   // slave: all masked
}

// Tell the PIC we have finished handling an interrupt, so it can send the next one.
// If the interrupt came from the slave (IRQ 8-15), BOTH PICs need the EOI, because the slave's interrupts pass through the master. Otherwise only the master needs it.
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);   // tell the slave
    }
    outb(PIC1_COMMAND, PIC_EOI);       // always tell the master
}