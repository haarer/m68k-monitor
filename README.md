# MC68331 UART Monitor

Proof-of-concept bare-metal monitor for MC68331 controlled over UART.

## MVP Commands

- `help` - list commands
- `md <addr> <len>` - dump memory
- `mw <addr> <value>` - write memory  
- `mf <addr> <len> <value>` - fill memory

## Memory Map

| Address Range | Size | Description |
|------------|------|-----------|
| 0x000000 - 0x0FFFFF | 1MB | Flash (not used in RAM mode) |
| 0x100000 - 0x1003FF | 1KB | Vector Table (loaded to RAM) |
| 0x100400 - 0x1DFFF | ~112KB | RAM (text, data) |
| 0x1E000 - 0x20FFF | 192KB | RAM (data, bss, heap) |
| 0x20FFFC | 4B | Stack top |

### Linker Sections

- `.text` - Code: 0x100400+
- `.data` - Initialized data: 0x1E000
- `.bss` - Uninitialized data: 0x1E060+
- `.ramvec` - Vector table: 0x100000

## Exception Vector Table

Located at 0x100000 (copied from ROM vectors at startup):

| Vector | Address | Description |
|--------|---------|-------------|
| 0x000 | SP | Initial Stack Pointer |
| 0x004 | PC | Initial Program Counter (reset) |
| 0x008 | Bus Error |
| 0x00C | Address Error |
| 0x010 | Illegal Instruction |
| 0x014 | Zero Divide |
| 0x018 | CHK Instruction |
| 0x01C | TRAP Instruction |
| 0x020 | Privilege Violation |
| 0x024 | Trace |
| ... | ... | Reserved / User interrupts |
| 0x060 | Spurious Interrupt |
| 0x064-0x07F | Level 1-7 IRQs |
| 0x080-0x0FF | TRAP 0-15 |
| 0x100-0x1FF | User interrupts |

## Hardware

- CPU: MC68331 (CPU32 core)
- Clock: 16.777 MHz (PLL from 32.768 kHz crystal)
- UART: 19200 baud, 8N1 (no parity, 8 bits, 1 stop)
- RAM: External SRAM via chip selects

## Exception Vector Table

Full vector table defined in `vector.S`. Key vectors:

| Vector | Offset | Description |
|--------|--------|-------------|
| 0 | 0x000 | Initial Stack Pointer (SP) |
| 1 | 0x004 | Initial Program Counter (PC) / Reset |
| 2 | 0x008 | Bus Error |
| 3 | 0x00C | Address Error |
| 4 | 0x010 | Illegal Instruction |
| 5 | 0x014 | Zero Divide |
| 6 | 0x018 | CHK Instruction |
| 7 | 0x01C | TRAP Instruction |
| 8 | 0x020 | Privilege Violation |
| 9 | 0x024 | Trace |
| ... | ... | ... |
| 24 | 0x060 | Spurious Interrupt |
| 25-31 | 0x064-0x07F | IRQ Level 1-7 |
| 32-47 | 0x080-0x0BF | TRAP 0-15 |
| 48-127 | 0x0C0-0x1FF | User-defined interrupts |

All unused/reserved vectors point to infinite loops by default.
Vectors are copied from Flash to RAM at startup (see `#ramvec` section).

## Building

```bash
export PATH=/workspace/toolchain68k/toolchain-m68k-elf-current/bin:$PATH
make clean all
```

## Output Files

- `m68k-monitor.elf` - ELF executable
- `m68k-monitor.bin` - Binary image (9,328 bytes)
- `m68k-monitor.hex` - Intel HEX
- `m68k-monitor.srec` - Motorola S-Record

## Architecture

- Bare metal with newlib stdio (polling-based, no interrupts in MVP)
- UART at 19200 baud (8N1)
- Command line interpreter with line buffer (64 bytes)

## UART Protocol

- 19200 baud
- 8 data bits
- No parity
- 1 stop bit
- No hardware flow control

Connection: RX/TX pins on QSM module