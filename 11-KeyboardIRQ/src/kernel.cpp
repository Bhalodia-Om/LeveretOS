#include "vga.h"
#include "gdt.h"      
#include "idt.h"       // our interrupt table
#include "pic.h"       // pic_remap

// --- Kernel entry ---
extern "C" void kernel_main() {
// --- New Code: Lesson 11 ---
    // The order here matters, as each new step depends on the one before.
    gdt_init();      // segments
    idt_init();      // interrupt table (now includes the keyboard entry)
    pic_remap();     // move IRQs to 32-47

    
    text_color = make_color(VGA_WHITE, VGA_BLACK);
    clear_screen();

    print("LeveretOS - Lesson 11: Keyboard Through Interrupts\n\n");

    asm volatile ("sti"); // Set up the sti after everything else, as order matters. Now it's safe for keypresses to fire without ruinning any of the setup.

    for (;;) {  // Nothing left to do, so hit puts the CPU to sleep until an interrupt, like a key press, wakes it. 
        asm volatile ("hlt");   // sleep until the next interrupt
    }
    // Now we're not wasting CPU power to poll at all times!!! Only took like 400 lines...
}
