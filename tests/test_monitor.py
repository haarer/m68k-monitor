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

    # Clear any pending input
    try:
        SOCKET.setblocking(0)
        while select.select([SOCKET], [], [], 0.1)[0]:
            SOCKET.recv(4096)
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
