# MC68331 UART Monitor

Bare-metal monitor for MC68331 (CPU32) with UART console.

## Build

```bash
make all VARIANT=realhw   # Build for MC68331 hardware
make all VARIANT=qemu      # Build for QEMU virt machine
make run-qemu            # Build and run in QEMU
```

## Commands

- `help`              - list commands
- `md <addr> <len>`  - dump memory (hex)
- `mw <addr> <val>`  - write memory
- `mf <addr> <len> <val>` - fill memory

---

## VARIANT: realhw

Target: Motorola MC68331 (CPU32 core) on custom hardware

### Hardware

| Component | Address | Notes |
|-----------|---------|-------|
| RAM | 0x100000+ | 192KB external SRAM |
| QSM (UART) | 0xFFFC00 | QSM module registers |
| Baud | 19200 | 8N1 (no parity, 8 bits, 1 stop) |

### Memory Map

```
0x000000 - 0x0FFFFF  Flash (1MB, not used in RAM mode)
0x00100000 - 0x001003FF  Vector Table (RAM, copied from Flash)
0x00100400+  Code + Data
0x0020FFFC  Stack Top
```

### Linker Script

- Base: `ram.ld`
- Vectors: `.vector` section at 0x100000

---

## VARIANT: qemu

Target: QEMU m68k virt machine

### QEMU Configuration

| Component | Address | Notes |
|-----------|---------|-------|
| RAM | 0x40000000 | 64MB simulated |
| Goldfish TTY | 0xFF008000 | Serial console |
| CPU | m68020 | No MMU |

### Run

```bash
make run-qemu
# or manually:
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -display none
```

### Known Issues

- Serial output may not appear in QEMU console
- Use GDB debugging for verification

### Linker Script

- Base: `qemu.ld`
- Vectors: `.vector` section at 0x40000000

---

## Output Files

| File | Format | Use |
|------|--------|-----|
| `m68k-monitor.elf` | ELF | GDB debugging |
| `m68k-monitor.hex` | Intel HEX | Flash programmer |
| `m68k-monitor.srec` | Motorola S-Record | Legacy programmers |
| `m68k-monitor.bin` | Raw binary | Direct flash |

---

## Architecture

- Bare metal (no OS)
- newlib stdio (polling-based UART, no interrupts in MVP)
- Command line interpreter with 64-byte line buffer
- Vector table in RAM (copied at startup for realhw)