#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stddef.h>
#include <stdint.h>

// Read_line waits for a full typed line (blocking until Enter). Defined in keyboard.cpp.
void read_line(char* buffer, size_t max);

// Called by the keyboard interrupt handler (idt.cpp) for each scancode.
void keyboard_handle_scancode(uint8_t scancode);

// The scancode to character table, defined in keyboard.cpp.
extern const char scancode_to_char[128];

#endif // KEYBOARD_H