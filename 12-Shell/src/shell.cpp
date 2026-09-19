#include "shell.h"
#include "vga.h"       // print, putchar, clear_screen
#include "keyboard.h"  // read_line
#include "string.h"    // strcmp

// Run one command line, where we compare the typed text against set commands.
static void run_command(const char* line) {
    if (strcmp(line, "help") == 0) {                                                            // Show commands we have.
        print("Commands: help, clear, about, echo <text>\n");
    }
    else if (strcmp(line, "clear") == 0) {                                                      // Clear screen.
        clear_screen();
    }
    else if (strcmp(line, "about") == 0) {                                                      // about the system. We can add other stuff here about our OS if we wish to.
        print("LeveretOS - a small OS built from scratch, one lesson at a time.\n");
    }
    // "echo <text>": if the line starts with "echo ", print everything after it.
    else if (line[0]=='e' && line[1]=='c' && line[2]=='h' && line[3]=='o' && line[4]==' ') {    // Can't use strcmp because we only need to check the first bit.
        print(line + 5);   // line + 5 skips past echo to the text.
        putchar('\n');
    }
    else if (line[0] == '\0') {
        // Empty line (only pressed Enter), so do nothing.
    }
    else {
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