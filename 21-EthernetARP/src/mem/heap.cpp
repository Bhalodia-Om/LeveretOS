#include "heap.h"
#include <stdint.h>
#include <stddef.h>

// We need to reserve a fixed slab of memory for the heap as a static array.
// This will be in the kernel's BSS (Block Started by Symbol), or uninitialized global memory. We don't need to PMM to hand us frames to it.
// We will allocated 1 MB for now.

#define HEAP_SIZE (1024 * 1024)   // 1 MB, in bytes.

static uint8_t heap_area[HEAP_SIZE];   // The raw bytes that the heap manages.

// We beed to make a struct for the BlockHeader. Will be stored inline right before the usable bytes. The next pointers point to the next header.
struct BlockHeader {
    size_t size;         // Number of usable bytes in this block, not including the header.
    bool   free;         // True = available.
    BlockHeader* next;   // The next block's header, or nullptr at the end.
};

// The first block in the chain. Will be set up by heap_init.
static BlockHeader* head = nullptr;

void heap_init() {
    // Make one free block across the entire heap.
    head = (BlockHeader*)heap_area;
    head->size = HEAP_SIZE - sizeof(BlockHeader);  // Usable space is the total region minus the header.
    head->free = true;
    head->next = nullptr;

}

void* malloc(size_t size) {
    // Walk through the linked list until we find the first free block big enough.
    BlockHeader* current = head;
    while (current != nullptr) {
        if (current->free && current->size >= size) {
            // If the block is much bigger than needed, split it, and turn the leftover into a new free block.
            // We need to make sure the leftover can atleast hold a header and at least 1 usable byte, otherwise theres no reason.
            if (current->size >= size + sizeof(BlockHeader) + 1) {
                // The new blocks header should sit right after the used bytes.
                uint8_t* raw = (uint8_t*)current;
                BlockHeader* leftover = (BlockHeader*)(raw + sizeof(BlockHeader) + size);
                
                leftover->size = current->size - size - sizeof(BlockHeader);
                leftover->free = true;
                leftover->next = current->next;

                current->size = size; // Shrink the current block to the requested size.
                current->next = leftover; // Link the new leftover block as the next block.
            }
            current->free = false; 

            // Return a pointer to the bytes after the header, as the caller should never see the header. 
            return (void*)((uint8_t*)current + sizeof(BlockHeader));
        }
        current = current->next;    // If size is too small, go to the next block.
    }
    return nullptr; // There are no blocks big enough.
}

void free(void* ptr) {
    if (ptr == nullptr) return;

    // The header is right before the ptr we gave out in malloc.
    BlockHeader* block = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));
    block->free = true;

    // Coalesce the block in front. This way we don't end up with a ton of small free blocks.
    if (block->next != nullptr && block->next->free) {
        block->size += sizeof(BlockHeader) + block->next->size;
        block->next = block->next->next;   // The next linked block should be moved from the abosrbed block to the next.

    }
}

uint32_t heap_total_bytes() {
    return HEAP_SIZE;
}

uint32_t heap_used_bytes() {
    // Sum the sizes of every block in use, and also all the headers.
    uint32_t used = 0;
    BlockHeader* current = head;
    while (current != nullptr) {
        used += sizeof(BlockHeader);
        if (!current->free) {
            used += current->size;
        }
        current = current->next;
    }
    return used;
}