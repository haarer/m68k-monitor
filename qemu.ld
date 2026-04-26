OUTPUT_ARCH(m68k)
STARTUP(crt0_qemu.o)
SEARCH_DIR(.)
SEARCH_DIR(/workspace/toolchain68k/toolchain-m68k-elf-current/m68k-elf/lib)

INPUT(libc.a libnosys.a)

__DYNAMIC = 0;

MEMORY
{
    ram    (rwx) : ORIGIN = 0x40000000, LENGTH = 64M
}

PROVIDE (__stack = 0x41000000);
PROVIDE (__heap_limit = __stack);
PROVIDE (__start = __text_start);

SECTIONS
{
    .vector :
    {
        __vec_start = .;
        *(.vector)
        . = ALIGN(4);
    } > ram

    .text :
    {
        __text_start = . ;
        *(.text)
        *(.text.*)
        . = ALIGN (16);

        *(.eh_frame)
        . = ALIGN (16);

        *(.gnu.linkonce.t.*)

        . = ALIGN(0x4);
        __CTOR_LIST__ = .;
        ___CTOR_LIST__ = .;
        LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
        *(.ctors)
        LONG(0)
        __CTOR_END__ = .;
        __DTOR_LIST__ = .;
        ___DTOR_LIST__ = .;
        LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
        *(.dtors)
        LONG(0)
        __DTOR_END__ = .;
        *(.rodata .rodata.*)
        *(.gcc_except_table)

        . = ALIGN(0x2);

        *(.lit)

        . = ALIGN(16);
        __end_text = .;
        etext = .;
    } > ram

    .data : AT ( ADDR( .text ) + SIZEOF( .text ) )
    {
        __data_start = .;
        *(.shdata)
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN (16);
        _edata = .;
        __data_end = .;
    } > ram

    .bss :
    {
        . = ALIGN(0x4);
        __bss_start = . ;
        *(.shbss)
        *(.bss)
        *(COMMON)
        __bss_end =  ALIGN (0x8);
        __end = __bss_end;
        end = __bss_end;
    } > ram

    .stab 0 (NOLOAD) :
    {
        *(.stab)
    }

    .stabstr 0 (NOLOAD) :
    {
        *(.stabstr)
    }

    .gdbdata :
    {
        *(.gdbdata)
    } > ram
}