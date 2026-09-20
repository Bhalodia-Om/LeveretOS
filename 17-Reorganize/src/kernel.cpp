#include "vga.h"
#include "gdt.h"      
#include "idt.h"       // our interrupt table
#include "pic.h"       // pic_remap
#include "timer.h"     // timer_init
#include "shell.h"     // the shell loop
#include "pmm.h"        // pmm_init
#include "paging.h"    // paging_init
#include "heap.h"      // heap_init

// --- Kernel entry ---
extern "C" void kernel_main(uint32_t multiboot_info_addr) {             // Argument from push ebx, GRUBS multiboot info address pointer.
    gdt_init();      // segments
    idt_init();      // interrupt table (includes the keyboard entry)
    pic_remap();     // move IRQs to 32-47
    timer_init(100); // start the PIT              
    pmm_init(multiboot_info_addr);   // Set up the physical memory manager.
    paging_init();                   // Build page tables and turn paging on.
    heap_init();                     // Set up the heap (malloc/free).

    text_color = make_color(VGA_WHITE, VGA_BLACK);
    clear_screen();

    print("LeveretOS - Lesson 17: Reorganize\n\n");

    asm volatile ("sti");   
    shell_run();            // hand control to the shell, which never returns
}
