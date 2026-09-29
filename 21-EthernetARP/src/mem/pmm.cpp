#include "pmm.h"
#include "multiboot.h"

// Each frame is 4 KB. But we need it in bytes.
#define FRAME_SIZE 4096

// The bitmap is one bit per frame, 1 being used, and 0 being free. We size it up for the full 4 GB of RAM.
// In total we have 1,048,576 frames. 1,048,576 bits / 32 bits per uin32_t = 32,768 words. Well not really words, but a 32-bit chunk.
#define MAX_FRAMES   (0xFFFFFFFF / FRAME_SIZE + 1)   // frames in a 4 GB space.
#define BITMAP_WORDS (MAX_FRAMES / 32)               // uint32_t words needed.

static uint32_t frame_bitmap[BITMAP_WORDS];   // the bitmap itself is in .bss.
static uint32_t total_frames = 0;             // frames that exist as usable RAM.
static uint32_t used_frames  = 0;             // frames currently reserved.

// The kernel's own memory must never be handed out. The linker gives us this symbol at the very end of the kernel in the memory. 
// Everything before it is kernel. Would be pretty bad if applications read over the kernel data. REALLY bad...
extern "C" uint8_t kernel_end;

// --- Bitmap helpers: set / clear / test one frame's bit ---

// A frame number picks a word (frame / 32) and a bit within the word (frame % 32).
static void bitmap_set(uint32_t frame) {
    frame_bitmap[frame / 32] |= (1u << (frame % 32));   // |= OR gate. Forces the bit to 1, leaving all others the same.
}
static void bitmap_clear(uint32_t frame) {              
    frame_bitmap[frame / 32] &= ~(1u << (frame % 32));  // ~ flips the bits of the mask, getting every bit except the one selected. Then set the selected bit to 0.
}
static bool bitmap_test(uint32_t frame) {
    return frame_bitmap[frame / 32] & (1u << (frame % 32)); // Check if a bit is ON / used, and return result as a boolean.
}

void pmm_init(uint32_t multiboot_info_addr) {
    MultibootInfo* mboot = (MultibootInfo*)multiboot_info_addr; // Point to the struct that GRUB wrote.

    // We will start with everything masked, then free regions marked as usable. Safer than the reverse. Anything not free stays reserved.
    for (uint32_t i = 0; i < BITMAP_WORDS; i++) {
        frame_bitmap[i] = 0xFFFFFFFF;
    }

    // Go through GRUB's memory map. Each entry describes one region. For usable regions, free every frame in the region.
    uint32_t offset = 0;
    while (offset < mboot->mmap_length) {                                                   // Keep going until we reach the end.
        MultibootMmapEntry* entry = (MultibootMmapEntry*)(mboot->mmap_addr + offset);       // Treat the bytes at mapstart (+offset) as one entry.
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {                                    // Only go through regions GRUB marked as usable.
            uint32_t start = (uint32_t)entry->addr;                                         // First address in the region.
            uint32_t end   = (uint32_t)(entry->addr + entry->len);                          // One past the last address of the region.
            // Free each 4 KB frame that falls fully inside this region.                
            for (uint32_t addr = start; addr + FRAME_SIZE <= end; addr += FRAME_SIZE) {     // Go frame by frame, stopping if a frame would be past the end of the region.
                uint32_t frame = addr / FRAME_SIZE;                                         // turn the address into a frame number.
                if (bitmap_test(frame)) {                                                   // If the frame was marked used,
                    bitmap_clear(frame);                                                    // now make it free.
                    total_frames++;                                                         // One more frame that is marked usable.
                }
            }
        }
        offset += entry->size + 4;                                                          // Jump to the next GRUB entry.
    }

    // Now we need to re-reserve the kernel memory. Everything from address 0 to the kernel_end. kernel code and data must never been handed out.
    uint32_t kernel_last_frame = ((uint32_t)&kernel_end) / FRAME_SIZE;
    for (uint32_t frame = 0; frame <= kernel_last_frame; frame++) {
        if (!bitmap_test(frame)) {   // if it was free
            bitmap_set(frame);       // mark it used
            used_frames++;           // one usable frame is now reserved.
        }
    }
}

uint32_t alloc_frame() {
    // Find the first free frame with a 0 bit and hand it out.
    for (uint32_t frame = 0; frame < MAX_FRAMES; frame++) {
        if (!bitmap_test(frame)) {
            bitmap_set(frame);
            used_frames++;
            return frame * FRAME_SIZE;   // The frame's physical address.
        }
    }
    return 0;   // We are out of memory.
}

void free_frame(uint32_t addr) {        // Clear the frame.
    uint32_t frame = addr / FRAME_SIZE;
    if (bitmap_test(frame)) {
        bitmap_clear(frame);
        used_frames--;
    }
}

uint32_t pmm_total_frames() { return total_frames; }
uint32_t pmm_used_frames()  { return used_frames; }