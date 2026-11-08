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


; --- Hardware IRQ stubs ---

global irq1 ; set name
irq1:
    push dword 0         
    push dword 33        
    jmp isr_common        

; --- New Code: Lesson 13 ---

; The timer is IRQ0, which is interrupt 32 after our remap. Same as irq1, just different interrupt.
global irq0
irq0:
    push dword 0          ; fake error code
    push dword 32         ; interrupt number 32 = remapped IRQ0 = timer
    jmp isr_common


isr_common:
    pusha                        
    mov eax, [esp + 32]          
    push eax                    
    call interrupt_handler
    add esp, 4                   
    popa                        
    add esp, 8                  
    iret                        
