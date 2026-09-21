#include "paging.h"
#include "pmm.h"    // alloc_frame, to get RAM for the tables
#include <stdint.h>

// A page directory and every page table are arrays of 1024 32-bit entries.
// The top 20 bits in each entry represents the physical address it points to, 4 KB-aligned, so the low 12 bits are always 0.
// This way the low bits can be used for flags.
//   bit 0 = present  (this entry is valid/mapped)
//   bit 1 = writable (can be written, not just read)
//   We use 0x3 for everything we map, or present and writable.
#define PAGE_PRESENT  0x1
#define PAGE_WRITABLE 0x2

// Pointer for the page directory. 1024 entries of 4 MB address space. Must be 4 KB-aligned for the CPU, done by allocating a frame.
static uint32_t* page_directory = nullptr;      // nullptr means not set to point to anything yet. Ironic.

// Load the directories address into cr3 (control register 3, for page tables), then set the paging bit, bit 31, in cr0 (control register for CPU features)
// Turn paging on. Must be assembly, as direct influence on CPU control registers.
static void enable_paging(uint32_t page_directory_phys) {
    asm volatile ("mov %0, %%cr3" : : "r"(page_directory_phys));  // Tell the CPU where the directory is.
    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));                   // Read cr0.
    cr0 |= 0x80000000;                                            // Set bit 31 (paging enable).
    asm volatile ("mov %0, %%cr0" : : "r"(cr0));                  // Write it back, as paging is now ON.
}

void paging_init() {
    // Get one frame (4 KB) for the page directory itself.
    page_directory = (uint32_t*)alloc_frame();

    // Every directory entry starts as "not present", or 0. Any address whose entry is not present will fault, which it should for unmapped memory.
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0;
    }

    // --- Identity-map the first 4 MB for kernel processes (virtual address is equal to physical address.)
    // One page table can contain the entire kernel and VGA.
    uint32_t* first_table = (uint32_t*)alloc_frame();
    for (int i = 0; i < 1024; i++) {
        // Map virtual page i to physical frame i. Page i covers address i * 4096 (0x1000).
        first_table[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    // Put the table as directory entry 0, covering virtual addresses from 0 to 4MB.
    page_directory[0] = ((uint32_t)first_table) | PAGE_PRESENT | PAGE_WRITABLE;

    // Turn the paging switch on, using the directories physical address.
    enable_paging((uint32_t)page_directory);
}