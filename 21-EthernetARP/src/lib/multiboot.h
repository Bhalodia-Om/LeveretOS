#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>


// We need to make a structure for GRUB, which will then give us a pointer to it, in ebx (boot.s).
// It has many fields, but we only need the memory map, so we name the fields up to the mmap ones.

struct MultibootInfo {
    uint32_t flags;          // which of the other fields are valid.
    uint32_t mem_lower;      // KB of low memory.
    uint32_t mem_upper;      // KB of high memory.
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;    // total size of the memory map, in bytes.
    uint32_t mmap_addr;      // address of the first memory-map entry.
    // More fields follow, but we don't use them yet.
} __attribute__((packed));

// One entry in the memory map. GRUB will give us a list, of these that describe a region of the physical memory.

struct MultibootMmapEntry {
    uint32_t size;           // size of THIS entry (used to move to the next one)
    uint64_t addr;           // start address of the region
    uint64_t len;            // length of the region, in bytes
    uint32_t type;           // 1 = usable RAM; anything else = reserved
} __attribute__((packed));

// The type value for usable RAM. so we can use MULTIBOOT_MEMORY_AVAILABLE instead of 1.
#define MULTIBOOT_MEMORY_AVAILABLE 1

#endif // MULTIBOOT_H