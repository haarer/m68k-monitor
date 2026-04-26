#include "asm.h"

    .title "crt0.S - QEMU variant"

    .extern SYM(main)
    .extern __stack

    .align 2
    .section  .text

__start:
    move.w  #0x2700, sr

    movec   vbr, d0
    move.l  #__vec_start, d1
    movec   d1, vbr

    move.l  #__stack, sp
    link    a6, #-8

    move.w  #0x2000, sr

    move.l  #0, -(sp)
    move.l  #0, -(sp)
    move.l  #0, -(sp)
    jsr     SYM(main)
    lea     (sp, 12), sp

__exit:
    bra __exit