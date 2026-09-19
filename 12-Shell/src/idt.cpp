#include "idt.h"
#include "vga.h"      
#include "pic.h"       // pic_send_eoi
#include "io.h"        // inb, keyboard ports
#include "keyboard.h"  // scancode_to_char table
#include <stdint.h>

// --- IDT entry (gate) ---
struct IdtEntry {
    uint16_t offset_low;   // bits 0-15 of the handler's address
    uint16_t selector;     // which GDT code segment to run the handler in (0x08, our code segment)
    uint8_t  always_zero;  // reserved, must be 0
    uint8_t  flags;        // type + privilege + "present" bit (we use 0x8E)
    uint16_t offset_high;  // bits 16-31 of the handler's address
} __attribute__((packed));

// --- IDTR pointer ---
struct IdtPointer {
    uint16_t limit;     // size - 1
    uint32_t base;      // address
} __attribute__((packed));

static IdtEntry idt[256];
static IdtPointer idt_ptr;

// --- Fill one IDT slot ---
static void idt_set_entry(int i, uint32_t handler, uint16_t selector, uint8_t flags) {
    idt[i].offset_low  = handler & 0xFFFF;
    idt[i].offset_high = (handler >> 16) & 0xFFFF;
    idt[i].selector    = selector;
    idt[i].always_zero = 0;
    idt[i].flags       = flags;
}

// --- CPU exception names, indexed by number ---
static const char* exception_names[32] = {
    "Divide by zero", "Debug", "Non-maskable interrupt", "Breakpoint",
    "Overflow", "Bound range exceeded", "Invalid opcode", "Device not available",
    "Double fault", "Coprocessor segment overrun", "Invalid TSS", "Segment not present",
    "Stack-segment fault", "General protection fault", "Page fault", "Reserved",
    "x87 floating-point", "Alignment check", "Machine check", "SIMD floating-point",
    "Virtualization", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Security", "Reserved"
};

// --- Interrupt dispatcher ---
extern "C" void interrupt_handler(uint32_t interrupt_number) {
    if (interrupt_number < 32) {
        print("\n*** CPU EXCEPTION ***\n");
        print(exception_names[interrupt_number]);
        print("\nSystem halted.\n");
        for (;;) { asm volatile ("cli; hlt"); }
    }
    // --- Changed Code: Lesson 12 ---
    // Hand the scancode to keyboard.cpp, which builds the line buffer.
    // The handler stays small: just read the byte, pass it on, and send EOI.
    else if (interrupt_number == 33) {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        keyboard_handle_scancode(scancode);
        pic_send_eoi(1);
    }
}

// --- Stubs from idt_stubs.s ---
extern "C" {
    void isr0();  void isr1();  void isr2();  void isr3();
    void isr4();  void isr5();  void isr6();  void isr7();
    void isr8();  void isr9();  void isr10(); void isr11();
    void isr12(); void isr13(); void isr14(); void isr15();
    void isr16(); void isr17(); void isr18(); void isr19();
    void isr20(); void isr21(); void isr22(); void isr23();
    void isr24(); void isr25(); void isr26(); void isr27();
    void isr28(); void isr29(); void isr30(); void isr31();
    void irq1();   // keyboard IRQ stub (from idt_stubs.s)
    
}

void idt_init() {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;

    idt_set_entry(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_entry(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_entry(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_entry(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_entry(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_entry(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_entry(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_entry(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_entry(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_entry(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_entry(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_entry(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_entry(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_entry(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_entry(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_entry(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_entry(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_entry(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_entry(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_entry(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_entry(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_entry(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_entry(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_entry(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_entry(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_entry(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_entry(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_entry(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_entry(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_entry(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_entry(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_entry(31, (uint32_t)isr31, 0x08, 0x8E);

    idt_set_entry(33, (uint32_t)irq1, 0x08, 0x8E);   // keyboard (IRQ1)

    asm volatile ("lidt %0" : : "m"(idt_ptr));
}

