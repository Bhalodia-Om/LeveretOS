// This file was completely reworked.
#include "keyboard.h"
#include "vga.h"   // putchar (echo typed characters)

// shift_held is internally used by the keyboard, so nothing else needs it.
static bool shift_held = false;

// The scancode to character table (declared extern in keyboard.h).
const char scancode_to_char[128] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`',
    0,   '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,   '*', 0,   ' ',
};

// The line buffer the interrupt handler fills as the user types. 
// This is shared between the interrupt handler and read_line, as they are "volatile", aka the compiler should not assume they will stay the same, and can change at any time.

static char line_buffer[128];
static volatile size_t line_length = 0;
static volatile bool   line_ready  = false;

// This is called by the keyboard interrupt handler in idt.cpp for each scancode. This keeps the interrupt short, where we track shift, echo the character, and on enter mark the line ready.
// We the line is marked ready, read_line can wake up and run.
void keyboard_handle_scancode(uint8_t scancode) {
    if (scancode == 0x2A) { shift_held = true;  return; }   // left shift pressed
    if (scancode == 0xAA) { shift_held = false; return; }   // left shift released
    if (scancode & 0x80)  { return; }                       // ignore other key releases

    char c = scancode_to_char[scancode];
    if (c == 0) return;
    if (shift_held && c >= 'a' && c <= 'z') c -= 0x20;       // make it uppercase

    if (c == '\n') {                        // Enter: finish the line
        putchar('\n');
        line_buffer[line_length] = '\0';
        line_ready = true;
    } else if (c == '\b') {                 // Backspace: erase one char
        if (line_length > 0) {
            line_length--;
            putchar('\b');
        }
    } else if (line_length < sizeof(line_buffer) - 1) { // Check to see if there is room in the buffer, to make sure we don't read into unowned memory and corrupt stuff.
        line_buffer[line_length++] = c;     // store it
        putchar(c);                         // echo it to the screen
    }
}

// Wait for a full line, and then copy it into the caller buffer. This will sleep with hit, until the interrupt handler returns line_ready.
void read_line(char* buffer, size_t max) {
    line_length = 0;
    line_ready  = false;

    while (!line_ready) {
        asm volatile ("hlt");   // sleep until a keypress interrupt wakes us.
    }

    size_t i = 0;
    while (i < max - 1 && line_buffer[i] != '\0') {
        buffer[i] = line_buffer[i]; // Pass our line to the buffer variable pointer.
        i++;
    }
    buffer[i] = '\0';
}
