#include "shell.h"
#include "vga.h"       // print, putchar, clear_screen
#include "keyboard.h"  // read_line
#include "string.h"    // strcmp
#include "rtc.h"       // rtc_read
#include "pmm.h"       // pmm_total_frames, pmm_used_frames
#include "heap.h"      // malloc, free, heap_total_bytes, heap_used_bytes

// Print a number as two digits (7 -> 07), so that the clock can read like HH:MM:SS.
static void print_two(uint8_t n) {
    putchar('0' + (n / 10));   // tens digit
    putchar('0' + (n % 10));   // ones digit
}

// Run one command line, where we compare the typed text against set commands.
static void run_command(const char* line) {
    if (strcmp(line, "help") == 0) {                                                            // Show commands we have.
        print("Commands: help, clear, about, echo <text>, clock, mem, heaptest\n");
    } else if (strcmp(line, "clear") == 0) {                                                      // Clear screen.
        clear_screen();
    } else if (strcmp(line, "about") == 0) {                                                      // about the system. We can add other stuff here about our OS if we wish to.
        print("LeveretOS - a small OS built from scratch, one lesson at a time.\n");
        // "echo <text>": if the line starts with "echo ", print everything after it.
    } else if (line[0]=='e' && line[1]=='c' && line[2]=='h' && line[3]=='o' && line[4]==' ') {    // Can't use strcmp because we only need to check the first bit.
        print(line + 5);   // line + 5 skips past echo to the text.
        putchar('\n');
    } else if (line[0] == '\0') {
        // Empty line (only pressed Enter), so do nothing.
    } else if (strcmp(line, "clock") == 0) {                    // Show the current time from the RTC.
        RtcTime t = rtc_read();
        print_two(t.hour);
        putchar(':');
        print_two(t.minute);
        putchar(':');
        print_two(t.second);
        putchar('\n');
    } else if (strcmp(line, "mem") == 0) {                      // Show memory usage.
        uint32_t total = pmm_total_frames();
        uint32_t used  = pmm_used_frames();
        print("Usable RAM: ");
        print_int(total * 4);
        print(" KB, used: ");
        print_int(used * 4);
        print(" KB, free: ");
        print_int((total - used) * 4);
        print(" KB\n");
    } else if (strcmp(line, "heaptest") == 0) {                // Prove malloc / free work.
        print("heap total: ");
        print_int(heap_total_bytes());
        print(" bytes, used: ");
        print_int(heap_used_bytes());
        print(" bytes\n");

        // Ask for two blocks, to see if bytes climb.
        void* a = malloc(100);
        print("after malloc(100), used: ");
        print_int(heap_used_bytes());
        print(" bytes\n");

        void* b = malloc(200);
        print("after malloc(200), used: ");
        print_int(heap_used_bytes());
        print(" bytes\n");

        // Free the blocks to see bytes drop.
        free(a);
        print("after free(a),     used: ");
        print_int(heap_used_bytes());
        print(" bytes\n");
        free(b);
        print("after free(b),     used: ");
        print_int(heap_used_bytes());
        print(" bytes\n");
    } else {
        print("Unknown command: ");
        print(line);
        putchar('\n');
    }
}

// The shell loop. We print a prompt, wait for a line, and run it, repeating forever.
void shell_run() {
    char line[128]; // Buffer for the inputted line.

    print("Type 'help' for a list of commands.\n\n");

    while (true) {
        print("> ");                     // the prompt, showing that our shell is ready to accept a command.
        read_line(line, sizeof(line));   // wait until the user types a line and enters
        run_command(line);               // handle the command.
    }
}