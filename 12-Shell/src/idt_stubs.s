; Assembly stubs for the 32 CPU exceptions and hardware interrupt flags.

extern interrupt_handler

; --- Macro, exception with no error code (push a fake 0) ---
%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0
    push dword %1
    jmp isr_common
%endmacro

; --- Macro, exception with an error code (CPU already pushed it) ---
%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1
    jmp isr_common
%endmacro

; ERR ones are 8, 10, 11, 12, 13, 14, 17; the rest NOERR.
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; --- New Code: Lesson 11 ---
; --- Hardware IRQ stub ---
; The keyboard is IRQ1, which is now interrupt 33. IRQs never push an error code, so we push a fake 0, and then the interrupt number (33).
; We then jump to the shared handler. We give it its own name, so that idt.cpp can find it.

global irq1 ; set name
irq1:
    push dword 0          ; fake error code
    push dword 33         ; interrupt number 33 from remapped IRQ1 for keyboard
    jmp isr_common        ; jump to the same shared routine as the CPU exceptions

; --- The following code is changed. ---
; --- Shared routine for the exception, and now the interrupt stubs ---
; Stack has [error code][interrupt number] from the stub, plus the CPU's current activity (EIP/CS/EFLAGS).
; We first save the registers, pass the interrupt number to the c++ dispatcher, restore to the previous activity, clean up, and return from the interrupt.
; using pusha/popa we now save the registers, or values the CPU was working with at the moment. This way we can recover easily from an interrupt, like CPU.
isr_common:
    pusha                        ; save all general registers.
    mov eax, [esp + 32]          ; interrupt number sits past the 8 saved regs (8*4 = 32).
    push eax                     ; pass it as the argument to interrupt_handler.
    call interrupt_handler
    add esp, 4                   ; remove the argument we pushed.
    popa                         ; restore the saved registers.
    add esp, 8                   ; drop the [error code][interrupt number] the stub pushed.
    iret                         ; return from interrupt, resume what was interrupted.
