/* linker.ld, controls how the sections are laid out in the final kernel binary. */

ENTRY(_start)

SECTIONS
{
    . = 1M;                 /* load the kernel at 1 MB */

    .text ALIGN(4K) :
    {
        *(.multiboot)       /* multiboot header must come first */
        *(.text)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata)
    }

    .data ALIGN(4K) :
    {
        *(.data)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
    }

    kernel_end = .;     /* The address just past the end of the kernel, so PMM knows what to protect.*/
                        /* '.' is the next writing address, so the address after everything is the end of the kernel. */
}
