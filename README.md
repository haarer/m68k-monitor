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

### Run in QEMU

```bash
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -display none
```

### Linker Script

- Base: `qemu.ld`
- Vectors: `.vector` section at 0x40000000

---

## Debugging with GDB

### Start QEMU with GDB Server

```bash
# Option 1: Run and connect immediately
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -display none -s

# Option 2: Stopped at start (wait for GDB)
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -display none -s -S
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
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -display none -s -S &

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

For **VS Code Community (OSS)** — install from [Open VSX Registry](https://open-vsx.org/):
- **Native Debug** by Augmented Startup (`webfreak.debug`)

For **VS Code (Microsoft)** — install from Marketplace:
- **C/C++** by Microsoft (`ms-vscode.cpptools`)

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
qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -display none -s -S
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

- Serial output may not appear in QEMU (Goldfish TTY not fully connected)
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