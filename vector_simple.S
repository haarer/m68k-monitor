#include "asm.h"

    .extern __stack
    .extern start

    .align 4
    .section  .vector
SYM(__vec_start):
    .long   __stack               | 0x000 Reset: Initial Stack Pointer
    .long   __start               | 0x004 Reset: Initial Program Counter
    .long   VecDefault            | 0x008 Bus Error
    .long   VecDefault            | 0x00C Address Error
    .long   VecDefault            | 0x010 Illegal Instruction
    .long   VecDefault            | 0x014 Zero Divide
    .long   VecDefault            | 0x018 CHK Instruction
    .long   VecDefault            | 0x01C TRAP Instruction
    .long   VecDefault            | 0x020 Privilege Violation
    .long   VecDefault            | 0x024 Trace
    .long   VecDefault            | 0x028 Unimplemented line-a opcode
    .long   VecDefault            | 0x02C Unimplemented line-b opcode
    .long   VecDefault            | 0x030 Hardware Breakpoint
    .long   VecDefault            | 0x034 reserved
    .long   VecDefault            | 0x038 Format Error
    .long   VecDefault            | 0x03C Uninitialized Interrupt
    .org    0x6C                   | 0x040 - 0x05C reserved
    .long   IRQSpurious           | 0x060 Spurious Interrupt
    .long   IRQDefault            | 0x064 Level 1 Interrupt
    .long   IRQDefault            | 0x068 Level 2 Interrupt
    .long   IRQDefault            | 0x06C Level 3 Interrupt
    .long   IRQDefault            | 0x070 Level 4 Interrupt
    .long   IRQDefault            | 0x074 Level 5 Interrupt
    .long   IRQDefault            | 0x078 Level 6 Interrupt
    .long   IRQDefault            | 0x07C Level 7 Interrupt
    .long   TrapDefault           | 0x080 TRAP 0
    .long   TrapDefault           | 0x084 TRAP 1
    .long   TrapDefault           | 0x088 TRAP 2
    .long   TrapDefault           | 0x08C TRAP 3
    .long   TrapDefault           | 0x090 TRAP 4
    .long   TrapDefault           | 0x094 TRAP 5
    .long   TrapDefault           | 0x098 TRAP 6
    .long   TrapDefault           | 0x09C TRAP 7
    .long   TrapDefault           | 0x0A0 TRAP 8
    .long   TrapDefault           | 0x0A4 TRAP 9
    .long   TrapDefault           | 0x0A8 TRAP 10
    .long   TrapDefault           | 0x0AC TRAP 11
    .long   TrapDefault           | 0x0B0 TRAP 12
    .long   TrapDefault           | 0x0B4 TRAP 13
    .long   TrapDefault           | 0x0B8 TRAP 14
    .long   TrapDefault           | 0x0BC TRAP 15
    .org    0x100                  | 0x0C0 - 0x0FC reserved
    .long   IRQDefault            | 0x100 User-Defined Interrupt 0
    .long   IRQDefault            | 0x104 User-Defined Interrupt 1
    .long   IRQDefault            | 0x108 User-Defined Interrupt 2
    .long   IRQDefault            | 0x10C User-Defined Interrupt 3
    .long   IRQDefault            | 0x110 User-Defined Interrupt 4
    .long   IRQDefault            | 0x114 User-Defined Interrupt 5
    .long   IRQDefault            | 0x118 User-Defined Interrupt 6
    .long   IRQDefault            | 0x11C User-Defined Interrupt 7
    .long   IRQDefault            | 0x120 User-Defined Interrupt 8
    .long   IRQDefault            | 0x124 User-Defined Interrupt 9
    .long   IRQDefault            | 0x128 User-Defined Interrupt 10
    .long   IRQDefault            | 0x12C User-Defined Interrupt 11
    .long   IRQDefault            | 0x130 User-Defined Interrupt 12
    .long   IRQDefault            | 0x134 User-Defined Interrupt 13
    .long   IRQDefault            | 0x138 User-Defined Interrupt 14
    .long   IRQDefault            | 0x13C User-Defined Interrupt 15
SYM(__vec_end):

    .section  .text

VecDefault:
    bra VecDefault

IRQDefault:
    bra IRQDefault

IRQSpurious:
    bra IRQSpurious

TrapDefault:
    bra TrapDefault