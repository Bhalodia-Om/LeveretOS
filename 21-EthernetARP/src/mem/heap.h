#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>   // for size_t

// The heap will give out small, sized, chunks of memory to programs. The PMM only gave 4 KB frames, too large for something that may be 5 bytes.
// The heap will sit on top of the PMM. IT will take a big region of ram, and size it for callers.

// Set up the heap once at boot, before anyone calls malloc.
void heap_init();

// Take size bytes, and return a pointer to usable memory of that size, or a nullptr if there is none.
void* malloc(size_t size);

// Free a block so it can be reused. ptr must be a pointer given out by malloc, or we risk damaging unowned memory.
void free(void* ptr);

// Get total values for the shell
uint32_t heap_total_bytes();
uint32_t heap_used_bytes();

#endif // HEAP_H