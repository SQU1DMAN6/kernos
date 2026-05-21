OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)
SECTIONS
{
    . = 0x100000;
    
    .multiboot : {
        KEEP(*(.multiboot))
    }

    . = ALIGN(8);

    .text      : { *(.text)      }
    .rodata    : { *(.rodata*)   }
    .data      : { *(.data)      }
    .bss       : { *(COMMON) *(.bss) }
}
