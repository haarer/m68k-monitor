# MC68331 UART Monitor

Bare-metal monitor for MC68331 (CPU32) with UART console.

## Build

```bash
make all VARIANT=realhw   # Build for MC68331 hardware
make all VARIANT=qemu     # Build for QEMU virt machine
```

## Toolchain

Requires m68k-elf toolchain built from https://github.com/haarer/toolchain68k

Current versions:
- GCC 15.2.0
- binutils 2.46.0
- GDB 17.1
- newlib 4.6.0

### Toolchain Path

Pre-installed at `/opt/toolchain-m68k-elf-current/bin`:

```bash
export PATH=/opt/toolchain-m68k-elf-current/bin:$PATH
```

Tools: `m68k-elf-gcc`, `m68k-elf-gdb`, `m68k-elf-objdump`, `m68k-elf-nm`, etc.

---

## Commands

All addresses and values are in hexadecimal format.

### `help`
Show this help message with all available commands.

```
MON> help
MC68331 Monitor v0.1
Commands:
help           - show this help
md <addr> <len> - dump memory
mw <addr> <val> - write memory
mf <addr> <len> <val> - fill memory
```

### `md <addr> <len>` - Memory Dump
Dump memory contents starting at `addr` for `len` bytes.

- `addr` - Starting address (hex)
- `len` - Number of bytes to dump (hex)

Output format: 16 bytes per line with address prefix
```
MON> md 100000 20
00100000: 4e 56 00 00 4e b9 00 10  00 00 4e 5e 4e 75 00 00
00100010: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
```

### `mw <addr> <val>` - Memory Write
Write a 16-bit value to memory address.

- `addr` - Target address (hex, must be 2-byte aligned)
- `val` - 16-bit value to write (hex)

```
MON> mw 100100 dead
Wrote dead to 00100100
```

Note: Writes 2 bytes (16-bit word) at the specified address.

### `mf <addr> <len> <val>` - Memory Fill
Fill a block of memory with a 16-bit value.

- `addr` - Starting address (hex, must be 2-byte aligned)
- `len` - Number of 16-bit words to fill (hex)
- `val` - 16-bit fill value (hex)

```
MON> mf 100000 10 beef
Filled 0010 words at 00100000 with beef
```

Note: Fills `len` words (2 × len bytes) with the specified 16-bit value.

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
| RAM | 0x00000000 | 64MB (origin in linker script) |
| Goldfish TTY | 0xFF008000 | Serial console (stdin/stdout) |
| CPU | m68020 | No MMU |

### Memory Map (from qemu.ld)

```
0x00000000 - 0x00FFFFFF  RAM (64MB total)
0x00000000              Vector table (.vector section)
0x00000000+             Code (.text section)
                        Data (.data section)
                        BSS (.bss section)
0x01000000              Stack top (__stack)
```

### Run in QEMU

```bash
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -serial mon:stdio -display none
```

### Linker Script

- Base: `qemu.ld`
- Vectors: `.vector` section at 0x00000000
- Stack: 0x01000000 (16MB boundary)

---

## Debugging with GDB

### Start QEMU with GDB Server

```bash
# Option 1: Run and connect immediately
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -serial mon:stdio -display none -s

# Option 2: Stopped at start (wait for GDB)
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -serial mon:stdio -display none -s -S
```

### Connect GDB

```bash
export PATH=/opt/toolchain-m68k-elf-current/bin:$PATH

m68k-elf-gdb m68k-monitor.elf -ex "target remote localhost:1234"
```

### Common GDB Commands

```bash
# Connection
target remote localhost:1234    # Connect to QEMU

# Execution control
continue                       # Run until breakpoint
c                              # Short for continue
step                           # Step one instruction
next                           # Step one instruction (skip calls)
finish                         # Run until function returns
interrupt                     # Stop running program

# Breakpoints
break *0x400006d8             # Break at address
break init_main               # Break at function
info breakpoints             # List breakpoints
delete <num>                 # Delete breakpoint

# Inspection
info registers               # Show all registers
info registers pc             # Show PC only
x/20i $pc                    # Disassemble 20 instructions at PC
x/4x 0x40000FF0              # Examine 4 words at address
print $d0                     # Print register value
backtrace                     # Show call stack

# Source-level debugging
list main                     # Show source around main
list                          # Continue listing
step                          # Step one source line
next                          # Step one source line (skip calls)
```

### Example Debugging Session

```bash
# Terminal 1: Start QEMU
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -serial mon:stdio -display none -s -S &

# Terminal 2: Debug with GDB
m68k-elf-gdb m68k-monitor.elf << 'EOF'
target remote localhost:1234
set tcp connect-timeout 30
break init_main
continue
step
step
step
info registers pc
continue
EOF
```

---

## VS Code Debugging (QEMU variant)

### Required Plugin

- **C/C++** by Microsoft (`ms-vscode.cpptools`) — provides the `cppdbg` debugger

### Setup

VS Code configuration is already included in `.vscode/`:
- `launch.json` — debug configurations
- `tasks.json` — build tasks and QEMU start/stop
- `settings.json` — IntelliSense with toolchain includes

### Build

Press `Ctrl+Shift+B` and select:
- `build-qemu` — build for QEMU
- `build-realhw` — build for hardware

### Debug

1. Select **Run and Debug** (`Ctrl+Shift+D`)
2. Choose `QEMU: Debug m68k-monitor` from the dropdown
3. Press `F5`

This automatically:
- Starts QEMU with GDB server (`-s -S`)
- Connects `m68k-elf-gdb` to `localhost:1234`
- Sets architecture to `m68k:68020`
- Stops at entry point

### Attach to Running QEMU

To manually start QEMU first:
```bash
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -serial mon:stdio -display none -s -S
```
Then in VS Code, select `QEMU: Attach to running GDB server` and press `F5`.

### Useful Debug Actions

| Action | Shortcut |
|--------|----------|
| Continue | `F5` |
| Step Over | `F10` |
| Step Into | `F11` |
| Toggle Breakpoint | `F9` |
| View Variables | Debug sidebar |
| View Registers | Debug sidebar → Registers |

---

### Known Issues

- Use GDB for debugging verification

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