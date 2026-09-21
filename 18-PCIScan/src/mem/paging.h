#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

// Set up paging:
// First build a page directory and page tables which indentity map the low, kernel, memory. We give it the same virtual and physical address.
// Next load the directory into the CPU, and turn paging on.
// After, the kernel will run the same, but every address will be translated.
void paging_init();

#endif // PAGING_H