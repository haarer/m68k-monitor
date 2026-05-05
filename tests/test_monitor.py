#!/usr/bin/env python3
"""
Test suite for m68k-monitor using QEMU.
Tests all user commands: help, md, mw, mf, mc

Runs QEMU once and connects via TCP serial.
"""

import subprocess
import time
import sys
import os
import signal
import socket
import select
import threading

# Colors
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
NC = '\033[0m'

QEMU_PORT = 1235
TIMEOUT = 10
PASS = 0
FAIL = 0
QEMU_PID = None
SOCKET = None
READ_BUFFER = ""


def cleanup():
    """Clean up QEMU process and socket."""
    global QEMU_PID, SOCKET

    if SOCKET:
        try:
            SOCKET.close()
        except Exception:
            pass
        SOCKET = None

    if QEMU_PID:
        try:
            os.kill(QEMU_PID, signal.SIGTERM)
            time.sleep(0.5)
            os.kill(QEMU_PID, signal.SIGKILL)
        except (ProcessLookupError, ChildProcessError):
            pass
        QEMU_PID = None


def log_pass(test_name):
    global PASS
    print(f"  {test_name}... {GREEN}PASS{NC}")
    PASS += 1


def log_fail(test_name, details=""):
    global FAIL
    print(f"  {test_name}... {RED}FAIL{NC}")
    if details:
        print(f"    {YELLOW}{details}{NC}")
    FAIL += 1


def start_qemu():
    """Start QEMU with TCP serial, once for all tests."""
    global QEMU_PID

    qemu_cmd = [
        'qemu-system-m68k', '-M', 'virt', '-cpu', 'm68020',
        '-kernel', '../m68k-monitor.elf',
        '-serial', f'tcp::{QEMU_PORT},server,nowait',
        '-monitor', 'none',
        '-display', 'none', '-nographic'
    ]

    try:
        proc = subprocess.Popen(
            qemu_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        QEMU_PID = proc.pid
        print(f"QEMU started (PID: {QEMU_PID})")

        # Wait for QEMU to start and open port
        for i in range(20):
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(1)
                s.connect(('127.0.0.1', QEMU_PORT))
                s.close()
                return True
            except (ConnectionRefusedError, OSError):
                time.sleep(0.5)

        print(f"{RED}QEMU did not start listening on port {QEMU_PORT}{NC}")
        return False

    except Exception as e:
        print(f"{RED}Failed to start QEMU: {e}{NC}")
        return False


def connect_tcp():
    """Connect to QEMU's TCP serial port."""
    global SOCKET

    try:
        SOCKET = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        SOCKET.settimeout(TIMEOUT)
        SOCKET.connect(('127.0.0.1', QEMU_PORT))
        print("Connected to QEMU serial via TCP")
        return True
    except Exception as e:
        print(f"{RED}Failed to connect to QEMU TCP serial: {e}{NC}")
        return False


def send_command(cmd):
    """Send a command via TCP socket."""
    global SOCKET

    # Send command with \r (monitor expects \r to process line)
    SOCKET.sendall((cmd + '\r').encode())
    time.sleep(0.3)


def read_output(timeout=TIMEOUT):
    """Read from socket until we see the prompt 'MON> '."""
    global SOCKET, READ_BUFFER

    output = ""
    start_time = time.time()

    while time.time() - start_time < timeout:
        ready, _, _ = select.select([SOCKET], [], [], 0.5)
        if ready:
            try:
                data = SOCKET.recv(4096).decode('utf-8', errors='replace')
                if data:
                    output += data
                    if 'MON> ' in output:
                        break
                else:
                    break
            except socket.timeout:
                break
            except Exception:
                break

    return output


def run_test(commands, expected, test_name):
    """Run a test by sending commands and checking output."""

    # Clear any pending input — drain until buffer is empty
    try:
        SOCKET.setblocking(0)
        drained = True
        while drained:
            drained = False
            while select.select([SOCKET], [], [], 0.2)[0]:
                SOCKET.recv(4096)
                drained = True
        SOCKET.setblocking(1)
    except Exception:
        pass

    # Send all commands
    for cmd in commands:
        send_command(cmd)

    # Read response until prompt
    output = read_output()

    # Check if expected string is in output
    if expected in output:
        log_pass(test_name)
        return True
    else:
        log_fail(test_name, f"Expected '{expected}' in output")
        if "--debug" in sys.argv:
            print(f"    Output was: {repr(output[:500])}")
        return False


def generate_srec(record_type, addr, data_bytes):
    """Generate a valid SREC record string with correct checksum.
    
    Args:
        record_type: '1', '2', or '3' (S1=16-bit, S2=24-bit, S3=32-bit addr)
        addr: Target load address (integer)
        data_bytes: List of byte values (0-255) to embed
    
    Returns:
        SREC record string with valid checksum
    """
    addr_bits = {'1': 16, '2': 24, '3': 32}[record_type]
    addr_hex_len = addr_bits // 4
    count = (addr_bits // 8) + len(data_bytes) + 1
    addr_str = format(addr, '0{}X'.format(addr_hex_len))
    data_str = ''.join(format(b, '02X') for b in data_bytes)
    payload = addr_str + data_str
    payload_bytes = [int(payload[i:i+2], 16) for i in range(0, len(payload), 2)]
    checksum = (-sum(payload_bytes)) & 0xFF
    return 'S{}{:02X}{}{}{:02X}'.format(record_type, count, addr_str, data_str, checksum)


def test_help():
    """Test the help command."""
    return run_test(['help'], 'Commands:', 'help command')


def test_md():
    """Test the md (memory dump) command."""
    return run_test(['md 0 10'], '00000000', 'md command (memory dump)')


def test_mw():
    """Test the mw (memory write) command."""
    return run_test(['mw 100000 0xdead'], 'Wrote dead', 'mw command (memory write)')


def test_mf():
    """Test the mf (memory fill) command."""
    return run_test(['mf 100000 10 0xbeef'], 'Filled', 'mf command (memory fill)')


def test_mc():
    """Test the mc (memory copy) command."""
    return run_test(['mc 100000 100100 10'], 'Copied', 'mc command (memory copy)')


def test_mw_verify():
    """Test write and verify with memory dump."""
    return run_test(
        ['mw 100000 0x1234', 'md 100000 2'],
        '1234',
        'mw then md verify'
    )


def test_mf_verify():
    """Test fill and verify with memory dump."""
    return run_test(
        ['mf 100000 4 0xaaaa', 'md 100000 8'],
        'aaaa',
        'mf then md verify'
    )


def test_mc_verify():
    """Test copy and verify with memory dump."""
    return run_test(
        ['mf 100000 4 0x5555', 'mc 100000 100200 4', 'md 100200 8'],
        '5555',
        'mc then md verify'
    )


def test_invalid_command():
    """Test invalid command handling."""
    return run_test(['invalid_cmd_xyz'], 'Unknown command', 'invalid command handling')


def test_missing_args():
    """Test command with missing arguments."""
    return run_test(['mw 100000'], 'Usage:', 'missing arguments (mw)')


def test_srec_basic():
    """Test basic SREC command - single S3 record loads and reports success."""
    srec_line = generate_srec('3', 0x100000, [0xAB, 0xCD])
    commands = [
        'srec 200000',
        srec_line,
        '',
    ]
    return run_test(commands, 'Loaded record at address 200000', 'srec basic S3 record')


def test_srec_s1_record():
    """Test S1 record (16-bit address) loads successfully."""
    srec_line = generate_srec('1', 0x0000, [0x11, 0x22, 0x33])
    commands = [
        'srec 200000',
        srec_line,
        '',
    ]
    return run_test(commands, 'Loaded record', 'srec S1 record (16-bit addr)')


def test_srec_s2_record():
    """Test S2 record (24-bit address) loads successfully."""
    srec_line = generate_srec('2', 0x000100, [0xDE, 0xAD])
    commands = [
        'srec 200000',
        srec_line,
        '',
    ]
    return run_test(commands, 'Loaded record', 'srec S2 record (24-bit addr)')


def test_srec_data_verify_s3():
    """Test S3 record data is actually written to correct address and verify with md."""
    addr = 0x200000
    data = [0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE]
    srec_line = generate_srec('3', addr, data)
    hex_dump = ' '.join(format(b, '02x') for b in data)
    commands = [
        'srec 200000',
        srec_line,
        '',
        f'md {addr:X} {len(data)}',
    ]
    return run_test(commands, hex_dump, 'srec S3 data verify via md')

def test_srec_data_verify_s1():
    """Test S1 record data is actually written to correct address and verify with md."""
    addr = 0x0100
    data = [0xAA, 0xBB, 0xCC]
    srec_line = generate_srec('1', addr, data)
    hex_dump = ' '.join(format(b, '02x') for b in data)
    commands = [
        'srec 200000',
        srec_line,
        '',
        f'md {addr:X} {len(data)}',
    ]
    return run_test(commands, hex_dump, 'srec S1 data verify via md')


def test_srec_multi_record():
    """Test loading multiple SREC records and verify all data lands correctly."""
    addr_base = 0x200200
    data1 = [0x01, 0x02, 0x03]
    data2 = [0x04, 0x05, 0x06]
    srec1 = generate_srec('3', addr_base, data1)
    srec2 = generate_srec('3', addr_base + 3, data2)
    combined = ' '.join(format(b, '02x') for b in data1 + data2)
    commands = [
        'srec 200000',
        srec1,
        srec2,
        '',
        f'md {addr_base:X} {len(data1) + len(data2)}',
    ]
    return run_test(commands, combined, 'srec multi-record verify')


def test_srec_zero_bytes():
    """Test that zero-valued bytes are written correctly."""
    addr = 0x200300
    data = [0x00, 0x00, 0x00, 0x00, 0x00]
    srec_line = generate_srec('3', addr, data)
    hex_dump = ' '.join(format(b, '02x') for b in data)
    commands = [
        'srec 200000',
        srec_line,
        '',
        f'md {addr:X} {len(data)}',
    ]
    return run_test(commands, hex_dump, 'srec zero bytes written')


def test_srec_full_range_bytes():
    """Test that extreme byte values (0x00, 0x01, 0xFF) are written correctly."""
    addr = 0x200400
    data = [0x00, 0x01, 0xFF, 0x80, 0x7F]
    srec_line = generate_srec('3', addr, data)
    hex_dump = ' '.join(format(b, '02x') for b in data)
    commands = [
        'srec 200000',
        srec_line,
        '',
        f'md {addr:X} {len(data)}',
    ]
    return run_test(commands, hex_dump, 'srec full-range byte values')


def test_srec_missing_args():
    """Test srec command with missing address argument."""
    commands = [
        'srec',
    ]
    return run_test(commands, 'Usage:', 'srec missing args')


def test_srec_invalid_record():
    """Test that an invalid SREC record line is rejected gracefully."""
    commands = [
        'srec 200000',
        'INVALIDLINE',
        '',
    ]
    return run_test(commands, 'Error parsing line', 'srec invalid record rejected')


def test_srec_overwrite_verify():
    """Test that srec data overwrites existing memory, then verify."""
    addr = 0x200500
    # First fill with a known pattern via mf
    # Then srec-write different data, then verify the srec data won
    data = [0xAA, 0xBB]
    srec_line = generate_srec('3', addr, data)
    hex_dump = ' '.join(format(b, '02x') for b in data)
    commands = [
        f'mf {addr:X} 2 0x0000',
        'srec 200000',
        srec_line,
        '',
        f'md {addr:X} {len(data)}',
    ]
    return run_test(commands, hex_dump, 'srec overwrite then verify')


def test_srec_upload_completed():
    """Test that the upload completion message is shown."""
    srec_line = generate_srec('3', 0x200600, [0x42])
    commands = [
        'srec 200000',
        srec_line,
        '',
    ]
    return run_test(commands, 'S-Record upload completed', 'srec upload completed message')


def test_srec_bad_checksum():
    """Test that an SREC line with a wrong checksum is rejected."""
    good = generate_srec('3', 0x200700, [0x11, 0x22])
    bad = good[:-2] + 'FF'  # overwrite the checksum with garbage
    commands = [
        'srec 200000',
        bad,
        '',
    ]
    return run_test(commands, 'Error parsing line', 'srec bad checksum rejected')


def test_srec_count_too_large():
    """Test that an SREC line whose count exceeds actual data length is rejected."""
    good = generate_srec('3', 0x200800, [0x11, 0x22])
    # count field is bytes 2-3 (hex). Inflate count by 2 (one extra data byte)
    count_hex = int(good[2:4], 16) + 1
    inflated = good[:2] + format(count_hex, '02X') + good[4:]
    commands = [
        'srec 200000',
        inflated,
        '',
    ]
    return run_test(commands, 'Error parsing line', 'srec count too large rejected')


def test_srec_count_too_small():
    """Test that an SREC line whose count is less than actual data length is rejected."""
    good = generate_srec('3', 0x200900, [0x11, 0x22])
    # Shrink count by 1 (one fewer data byte claimed)
    count_hex = int(good[2:4], 16) - 1
    shrunken = good[:2] + format(count_hex, '02X') + good[4:]
    commands = [
        'srec 200000',
        shrunken,
        '',
    ]
    return run_test(commands, 'Error parsing line', 'srec count too small rejected')


def main():
    global PASS, FAIL

    print("=" * 60)
    print("m68k-monitor Test Suite (QEMU + TCP)")
    print("=" * 60)
    print()

    # Check if QEMU is available
    try:
        subprocess.run(['qemu-system-m68k', '--version'],
                       capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print(f"{RED}Error: qemu-system-m68k not found{NC}")
        sys.exit(1)

    # Check if monitor binary exists
    if not os.path.exists('../m68k-monitor.elf'):
        print(f"{RED}Error: m68k-monitor.elf not found{NC}")
        print("Please build the project first with: make all VARIANT=qemu")
        sys.exit(1)

    # Start QEMU once
    print("Starting QEMU...")
    if not start_qemu():
        sys.exit(1)

    # Connect to TCP serial
    print("Connecting to QEMU serial...")
    if not connect_tcp():
        cleanup()
        sys.exit(1)

    # Wait for monitor to boot and show prompt
    print("Waiting for monitor to boot...")
    time.sleep(2)

    # Clear initial output (boot messages)
    try:
        SOCKET.setblocking(0)
        while select.select([SOCKET], [], [], 0.2)[0]:
            SOCKET.recv(4096)
        SOCKET.setblocking(1)
    except Exception:
        pass

    print("Monitor ready.")
    print()
    print("Running tests...")
    print()

    # Run all tests
    test_help()
    test_md()
    test_mw()
    test_mf()
    test_mc()
    test_mw_verify()
    test_mf_verify()
    test_mc_verify()
    test_invalid_command()
    test_missing_args()
    test_srec_basic()
    test_srec_s1_record()
    test_srec_s2_record()
    test_srec_data_verify_s3()
    test_srec_data_verify_s1()
    test_srec_multi_record()
    test_srec_zero_bytes()
    test_srec_full_range_bytes()
    test_srec_missing_args()
    test_srec_invalid_record()
    test_srec_overwrite_verify()
    test_srec_upload_completed()
    test_srec_bad_checksum()
    test_srec_count_too_large()
    test_srec_count_too_small()

    # Cleanup
    cleanup()

    # Summary
    print()
    print("=" * 60)
    print("Test Results:")
    print(f"  {GREEN}Passed: {PASS}{NC}")
    print(f"  {RED}Failed: {FAIL}{NC}")
    print("=" * 60)

    if FAIL == 0:
        print(f"{GREEN}All tests passed!{NC}")
        sys.exit(0)
    else:
        print(f"{RED}Some tests failed!{NC}")
        sys.exit(1)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\nInterrupted by user")
        cleanup()
        sys.exit(1)
