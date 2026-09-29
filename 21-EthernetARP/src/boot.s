; boot.s, the first thing the bootloader runs. Hands control to the kernel.

; --- Multiboot header constants ---
MBALIGN  equ 1 << 0
MEMINFO  equ 1 << 1
MBFLAGS  equ MBALIGN | MEMINFO
MAGIC    equ 0x1BADB002
CHECKSUM equ - (MAGIC + MBFLAGS)

; --- The multiboot header ---
section .multiboot
align 4
    dd MAGIC
    dd MBFLAGS
    dd CHECKSUM

; --- The stack ---
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

; --- Entry point ---
section .text
global _start

_start:
    ; --- Step 1: give the CPU a working stack ---
    mov esp, stack_top

    ; --- Step 2: jump into our C++ code ---
    extern kernel_main
    push ebx                      ; We need to pass GRUB's multiboot info pointer to kernel_main, to get a map of reserved and usable memory.
    call kernel_main

    ; --- Step 3: if the kernel ever returns, stop safely ---
.hang:
    cli
    hlt
    jmp .hang
