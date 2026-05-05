# MC68331 UART Monitor

Bare-metal monitor for MC68331 (CPU32) with UART console.

## About This Project

This is a **proof of concept project** demonstrating the use of [opencode](https://opencode.ai) for embedded system development.

Using [opencode](https://opencode.ai) together open-source toolchain (GCC, binutils, GDB, QEMU) works very well for most embedded development tasks, allowing me to focus on what I actually want to build.


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

## Command Specification

All addresses and values are in hexadecimal format.

### `help`
Show this help message with all available commands.

```
MON> help
MC68331 Monitor v0.1
Commands:
help           - show this help
md <addr> <len> - dump memory
mw <addr> <val> [<val> ...] - write memory
mf <addr> <len> <val> - fill memory
mc <src> <dst> <len> - copy memory
srec <addr>    - load S-Record data
```

### `md <addr> <len>` - Memory Dump
Dump memory contents starting at `addr` for `len` bytes.
Memory is accessed byte wise.

- `addr` - Starting address (hex)
- `len` - Number of bytes to dump (hex)

Output format: 16 bytes per line with address prefix
```
MON> md 100000 20
00100000: 4e 56 00 00 4e b9 00 10  00 00 4e 5e 4e 75 00 00
00100010: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
```

### `mw <addr> <val> [<val> ...]` - Memory Write
Write one or more values to memory address.
The write size is auto-detected from the first value's hex digit count:
- 1–2 hex digits: 8-bit byte write (no alignment required)
- 3–4 hex digits: 16-bit word write (address must be 2-byte aligned)
- 5–8 hex digits: 32-bit longword write (address must be 4-byte aligned)

All values must match the size of the first value.
The address increments by the write size after each value.

- `addr` - Target address (hex)
- `val` - one or more values to write (hex)

```
MON> mw 100100 ff
Wrote 0001 byte to 00100100

MON> mw 100100 dead
Wrote 0001 word to 00100100

MON> mw 100100 deadbeef
Wrote 0001 longword to 00100100

MON> mw 100100 aa bb cc dd
Wrote 0004 bytes to 00100100

MON> mw 100100 1234 5678 9abc
Wrote 0003 words to 00100100

MON> mw 100100 01020304 05060708
Wrote 0002 longwords to 00100100
```

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

### `mc <src> <dst> <len>` - Memory Copy
Copy a block of memory from source to destination.

- `src` - Source address (hex, must be 2-byte aligned)
- `dst` - Destination address (hex, must be 2-byte aligned)
- `len` - Number of 16-bit words to copy (hex)

```
MON> mc 100000 100100 20
Copied 0020 words from 00100000 to 00100100
```

Note: Copies `len` words (2 × len bytes) from source to destination. Regions may overlap.

### `srec <addr>` - Load S-Record Data
Load Motorola S-Record formatted data into memory. The `<addr>` argument is informational (displayed in status messages); actual load addresses are determined by the S-Record records themselves.

- `addr` - Informational address in hex (used in status output)

Supports S1 (16-bit), S2 (24-bit), and S3 (32-bit) address records. Enter S-record lines one by one, terminated by a blank line or Ctrl+D.

Example:
```
MON> srec 100000
Enter Motorola S-Record data:
Lines should be in format: S<type><count><address><data><checksum>
End with blank line or Ctrl+D (ASCII 0x04)

S3091000000000000000000000000000
S3051000090000000000000000000000
S3071000120000000000000000000000

S-Record upload completed.
```

**Record types:**

| Type | Address Width | Use Case |
|------|--------------|----------|
| S1 | 16-bit (0x0000–0xFFFF) | Low memory, small targets |
| S2 | 24-bit (0x000000–0xFFFFFF) | Medium memory range |
| S3 | 32-bit (full range) | Full address space |

Each record is acknowledged with `Loaded record at address <addr>` or `Error parsing line: <line>` on failure. Records are validated for correct checksum and consistent field lengths before loading.

---

## Command History

The monitor maintains a history of the last 10 executed commands.

### Navigation

| Key | Action |
|-----|--------|
| Cursor Up (`ESC[A`) | Move backward in history (shows older commands) |
| Cursor Down (`ESC[B`) | Move forward in history (shows newer commands) |

When at the newest entry, pressing Cursor Down clears the line.
When at the oldest entry, pressing Cursor Up has no effect.

Empty lines are not stored in history.
History is not persisted across resets.

### Example

```
MON> help
MC68331 Monitor v0.1
...

MON> md 100000 10
00100000: 4e 56 00 00 ...

MON> mw 100100 dead
Wrote 0001 word to 00100100

MON> ← press Cursor Up →
MON> mw 100100 dead
← press Cursor Up →
MON> md 100000 10
← press Cursor Up →
MON> help
← press Cursor Down →
MON> md 100000 10
← press Cursor Down →
MON> mw 100100 dead
← press Cursor Down →
MON>
```

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

## Test Suite

The project includes an automated test suite that verifies all monitor commands using QEMU emulation.

### Location
```
tests/test_monitor.py
```

### Requirements
- Python 3.12+
- QEMU with m68k support (`qemu-system-m68k`)
- Virtual environment with dependencies (auto-created)

### Running Tests

**Option 1: Using make (recommended)**
```bash
cd /workspace/m68k-monitor
make test
```

**Option 2: Manual execution**
```bash
# Create virtual environment and install dependencies
cd /workspace/m68k-monitor
python3 -m venv venv
source venv/bin/activate
pip install pexpect

# Build for QEMU
make all VARIANT=qemu

# Run tests
cd tests
python3 test_monitor.py
```

### Test Architecture
- QEMU is started once at the beginning of the test suite
- QEMU's serial is exposed via TCP port 1235
- Python connects to QEMU via TCP socket
- All tests run through a single TCP connection
- QEMU is terminated during cleanup

### Test Coverage

The test suite validates all user commands, history navigation, and data integrity:

| # | Command | Description |
|---|---------|-------------|
| 1 | `help` | Verify help displays all commands |
| 2 | `md` | Memory dump at address 0 |
| 3 | `mw` | Write 16-bit word to memory |
| 4 | `mw` byte | Write 8-bit byte to memory |
| 5 | `mw` byte + `md` | Byte write then verify with dump |
| 6 | `mw` byte odd addr | Byte write on unaligned address |
| 7 | `mw` longword | Write 32-bit longword to memory |
| 8 | `mw` longword + `md` | Longword write then verify with dump |
| 9 | `mw` word align error | Reject word write on unaligned address |
| 10 | `mw` longword align error | Reject longword write on unaligned address |
| 11 | `mw` value too large | Reject value exceeding 8 hex digits |
| 12 | `mw` invalid value | Reject non-hex characters |
| 13 | `mw` multi byte | Write multiple bytes then verify |
| 14 | `mw` multi word | Write multiple words then verify |
| 15 | `mw` multi longword | Write multiple longwords then verify |
| 16 | `mw` multi byte output | Verify plural "bytes" in output |
| 17 | `mw` multi word output | Verify plural "words" in output |
| 18 | `mw` mixed sizes | Reject mixing byte/word/longword values |
| 19 | `mw` multi byte offset | Each byte at correct sequential offset |
| 20 | `mw` multi word offset | Each word at correct +2 offset |
| 21 | `mw` multi longword offset | Each longword at correct +4 offset |
| 22 | `mw` multi alignment | Detect alignment error mid-sequence |
| 23 | `history` basic | Cursor Up recalls previous command |
| 24 | `history` two entries | Cursor Up cycles through multiple entries |
| 25 | `history` empty | Cursor Up with no history is harmless |
| 26 | `history` down | Cursor Down moves forward in history |
| 27 | `mf` | Fill memory with pattern |
| 28 | `mc` | Copy memory block |
| 29 | `mw` + `md` | Write then verify with dump |
| 30 | `mf` + `md` | Fill then verify with dump |
| 31 | `mc` + `md` | Copy then verify with dump |
| 32 | invalid cmd | Error handling for unknown commands |
| 33 | `mw` missing args | Usage message for missing arguments |
| 34 | `srec` S3 | Basic S3 record load |
| 35 | `srec` S1 | S1 record (16-bit address) load |
| 36 | `srec` S2 | S2 record (24-bit address) load |
| 37 | `srec` S3 + `md` | S3 data integrity verification |
| 38 | `srec` S1 + `md` | S1 data integrity verification |
| 39 | `srec` multi + `md` | Multi-record sequential load + verify |
| 40 | `srec` zeros | Zero-valued bytes written correctly |
| 41 | `srec` extremes | Full-range byte values (0x00–0xFF) |
| 42 | `srec` no args | Usage message for missing arguments |
| 43 | `srec` invalid | Invalid record rejected gracefully |
| 44 | `srec` overwrite | Data overwrites existing memory |
| 45 | `srec` completed | Upload completion message shown |
| 46 | `srec` bad checksum | Wrong checksum rejected |
| 47 | `srec` count too large | Count exceeds data length rejected |
| 48 | `srec` count too small | Count less than data length rejected |

### Output Example

```
============================================================
m68k-monitor Test Suite (QEMU + TCP)
============================================================

Starting QEMU...
QEMU started (PID: 3239)
Connecting to QEMU serial...
Connected to QEMU serial via TCP
Waiting for monitor to boot...
Monitor ready.

Running tests...

  help command... PASS
  md command (memory dump)... PASS
  mw command (memory write)... PASS
  mw byte write (8-bit)... PASS
  mw byte write then md verify... PASS
  mw byte write odd address... PASS
  mw longword write (32-bit)... PASS
  mw longword write then md verify... PASS
  mw word alignment error... PASS
  mw longword alignment error... PASS
  mw value too large... PASS
  mw invalid hex value... PASS
  mw multi byte write then verify... PASS
  mw multi word write then verify... PASS
  mw multi longword write then verify... PASS
  mw multi byte output... PASS
  mw multi word output... PASS
  mw mixed sizes rejected... PASS
  mw multi byte each offset correct... PASS
  mw multi word each offset correct... PASS
  mw multi longword each offset correct... PASS
  mw multi alignment error first value... PASS
  history basic recall... PASS
  history two entries cycle... PASS
  history empty safe... PASS
  history down forward... PASS
  mf command (memory fill)... PASS
  mc command (memory copy)... PASS
  mw then md verify... PASS
  mf then md verify... PASS
  mc then md verify... PASS
  invalid command handling... PASS
  missing arguments (mw)... PASS
  srec basic S3 record... PASS
  srec S1 record (16-bit addr)... PASS
  srec S2 record (24-bit addr)... PASS
  srec S3 data verify via md... PASS
  srec S1 data verify via md... PASS
  srec multi-record verify... PASS
  srec zero bytes written... PASS
  srec full-range byte values... PASS
  srec missing args... PASS
  srec invalid record rejected... PASS
  srec overwrite then verify... PASS
  srec upload completed message... PASS
  srec bad checksum rejected... PASS
  srec count too large rejected... PASS
  srec count too small rejected... PASS

============================================================
Test Results:
  Passed: 48
  Failed: 0
============================================================
All tests passed!
```

### Notes
- The test uses QEMU's virt machine with m68020 CPU
- QEMU's serial is exposed via TCP port 1235; tests connect via TCP socket
- SREC records are generated with correct checksums at runtime
- Data integrity is verified by loading via SREC then reading back with `md`
- All numeric values are parsed as hexadecimal (base 16)

### TCP Socket Buffering

The test suite shares a single TCP connection across all tests. The `srec` command produces substantial output (prompts, echoed input, per-record acknowledgments, completion banner), which can leave residual data in the TCP receive buffer. If not fully drained, stale output bleeds into the next test's response, causing false negatives or false positives.

Each test begins by draining the socket buffer until no data is available for 0.2 seconds, ensuring a clean state between tests. This is necessary because TCP buffers can hold multiple 4KB chunks, and a single `recv()` call is insufficient to flush them all.

---

## Architecture

- Bare metal (no OS)
- newlib stdio (polling-based UART, no interrupts in MVP)
- Command line interpreter with 64-byte line buffer
- Command history buffer (10 entries, navigated with Cursor Up/Down)
- Vector table in RAM (copied at startup for realhw)
- UART input flush before srec read loop (prevents Goldfish TTY echo feedback)
- S-Record parser supports S1/S2/S3 record types with checksum validation

---

## UART Input Handling

The Goldfish TTY in QEMU echoes written characters back into the read buffer. The monitor flushes pending input before starting interactive read loops (e.g., `srec`) to prevent stale echoed data from being interpreted as user input. This is handled by `v_uartFlushInput()` in the platform-specific UART driver.