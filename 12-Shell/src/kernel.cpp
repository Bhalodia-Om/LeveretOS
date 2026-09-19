#include "vga.h"
#include "gdt.h"      
#include "idt.h"       // our interrupt table
#include "pic.h"       // pic_remap
#include "shell.h"     // the shell loop

// --- Kernel entry ---
extern "C" void kernel_main() {
    gdt_init();      // segments
    idt_init();      // interrupt table (includes the keyboard entry)
    pic_remap();     // move IRQs to 32-47

    text_color = make_color(VGA_WHITE, VGA_BLACK);
    clear_screen();

    print("LeveretOS - Lesson 12: Shell\n\n");

    asm volatile ("sti");   

    shell_run();            // hand control to the shell, which never returns
}
