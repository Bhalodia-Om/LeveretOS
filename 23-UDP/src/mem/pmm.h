#ifndef PMM_H
#define PMM_H

#include <stdint.h>

// The PHysical Memory Manager will track which 4 KB frames of RAM are free, or which are used.
// pmm_init reads GRUB's memory map, via the multiboot info pointer, to get what ram exists. Then we mark it all used, then free the usable regions.

void pmm_init(uint32_t multiboot_info_addr);

// Allocate one free 4 KB frame. This returns its physical address, or 0 if there are no free frames.
uint32_t alloc_frame();

// Return a frame back to unallocate and reuse it. addr must be a frame address from alloc_frame.

// Counters, so the shell's "mem" command can report usage.
uint32_t pmm_total_frames();
uint32_t pmm_used_frames();

#endif // PMM_H
