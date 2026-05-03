#!/usr/bin/env python3
"""
Test suite for m68k-monitor using QEMU.
Tests all user commands: help, md, mw, mf, mc
"""

import subprocess
import time
import sys
import os
import signal
import tempfile

# Colors
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
NC = '\033[0m'

TIMEOUT = 10
PASS = 0
FAIL = 0
QEMU_PID = None


def cleanup():
    """Clean up QEMU process."""
    global QEMU_PID
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


def run_test(commands, expected, test_name):
    """Run QEMU with commands and check for expected output.

    Uses a temporary file for input since QEMU processes file input correctly.
    """
    global QEMU_PID

    # Create input file with commands using \r as line terminator
    # (monitor expects \r to process commands)
    with tempfile.NamedTemporaryFile(mode='wb', suffix='.txt', delete=False) as f:
        for cmd in commands:
            f.write((cmd + '\r').encode())
        # Add a delay to keep QEMU alive longer
        f.write(b'sleep 1\r')
        input_file = f.name

    qemu_cmd = [
        'qemu-system-m68k', '-M', 'virt', '-cpu', 'm68020',
        '-kernel', '../m68k-monitor.elf',
        '-serial', 'stdio', '-monitor', 'none',
        '-nographic', '-display', 'none'
    ]

    try:
        with open(input_file, 'r') as f_in:
            proc = subprocess.Popen(
                qemu_cmd,
                stdin=f_in,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            QEMU_PID = proc.pid

            # Wait for output with timeout
            try:
                stdout, stderr = proc.communicate(timeout=TIMEOUT)
                output = stdout + stderr
            except subprocess.TimeoutExpired:
                proc.kill()
                stdout, stderr = proc.communicate()
                output = stdout + stderr

            # Check if expected string is in output
            if expected in output:
                log_pass(test_name)
                return True
            else:
                log_fail(test_name, f"Expected '{expected}' in output")
                if "--debug" in sys.argv:
                    print(f"    Output was: {repr(output[:500])}")
                return False

    except Exception as e:
        log_fail(test_name, str(e))
        return False
    finally:
        # Cleanup
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()
        QEMU_PID = None
        try:
            os.unlink(input_file)
        except Exception:
            pass


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
    return run_test(['mf 100000 10 beef'], 'Filled', 'mf command (memory fill)')


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
    print("m68k-monitor Test Suite (QEMU)")
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

    # Run tests
    print("Running tests...")
    print()

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
