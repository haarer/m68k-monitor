/**
 * @file commands.c
 * @brief Monitor command implementations for m68k-monitor
 *
 * This file implements all user-facing commands for the MC68331 monitor.
 * Commands are registered in the commands[] array (defined at the end)
 * and invoked through the command-line interpreter in main.c.
 *
 * All numeric values (addresses, lengths, values) are parsed as
 * hexadecimal (base 16) using strtoul(..., 16).
 *
 * Memory access notes:
 *   - md: Accesses memory as bytes (unsigned char)
 *   - mw: Writes 16-bit words (unsigned short)
 *   - mf: Fills memory with 16-bit words
 *   - mc: Copies 16-bit words
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <commands.h>
#include <uart.h>

extern void v_uartPutch(unsigned int ch);

/**
 * @brief Output a null-terminated string via UART
 * @param s String to output
 */
static void putstr(const char *s)
{
    while (*s) {
        v_uartPutch(*s++);
    }
}

/**
 * @brief Output a hexadecimal value via UART
 * @param val Value to output
 * @param digits Number of hex digits to output (padded with leading zeros)
 */
static void puthex(unsigned long val, int digits)
{
    const char *hex = "0123456789abcdef";
    while (digits-- > 0) {
        v_uartPutch(hex[(val >> (digits * 4)) & 0xf]);
    }
}

/**
 * @brief Output CRLF (carriage return + line feed) via UART
 */
static void putnl(void)
{
    v_uartPutch('\r');
    v_uartPutch('\n');
}

/**
 * @brief Display help message with all available commands
 * @param argc Argument count (unused)
 * @param argv Argument vector (unused)
 * @return Always returns 0
 *
 * Shows monitor version and all registered commands with their help text.
 */
int cmd_help(int argc, char *argv[])
{
    int i;
    putstr("MC68331 Monitor v0.1\r\n");
    putstr("Commands:\r\n");
    for (i = 0; i < NUM_COMMANDS; i++) {
        putstr(commands[i].help);
        putnl();
    }
    return 0;
}

/**
 * @brief Memory dump command - dump memory contents as hex bytes
 * @param argc Argument count (must be >= 3)
 * @param argv Argument vector: argv[1]=addr, argv[2]=len
 * @return 0 on success, -1 on error
 *
 * Usage: md <addr> <len>
 *   addr: Starting address in hex
 *   len:  Number of bytes to dump in hex
 *
 * Output format: 16 bytes per line with 8-digit hex address prefix
 * Example: 00100000: 4e 56 00 00 4e b9 00 10  00 00 4e 5e 4e 75 00 00
 *
 * Memory is accessed as bytes (unsigned char).
 */
int cmd_md(int argc, char *argv[])
{
    unsigned long addr;
    unsigned long len;
    unsigned long i;
    unsigned char *p;

    if (argc < 3) {
        putstr("Usage: md <addr> <len>\r\n");
        return -1;
    }

    addr = strtoul(argv[1], NULL, 16);
    len = strtoul(argv[2], NULL, 16);

    p = (unsigned char *)addr;
    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            putnl();
            puthex(addr + i, 8);
            putstr(": ");
        }
        puthex(p[i], 2);
        putstr(" ");
        if ((i % 16) == 7) {
            putstr(" ");
        }
    }
    putnl();
    return 0;
}

/**
 * @brief Memory write command - write value to memory
 * @param argc Argument count (must be >= 3)
 * @param argv Argument vector: argv[1]=addr, argv[2]=val
 * @return 0 on success, -1 on error
 *
 * Usage: mw <addr> <val>
 *   addr: Target address in hex
 *   val:  value to write in hex (size auto-detected from hex digit count)
 *
 * Size detection:
 *   1-2 hex digits  -> 8-bit byte write (no alignment required)
 *   3-4 hex digits  -> 16-bit word write (2-byte aligned)
 *   5-8 hex digits  -> 32-bit longword write (4-byte aligned)
 *
 * Output: "Wrote <val> to <addr>"
 */
int cmd_mw(int argc, char *argv[])
{
    unsigned long addr;
    unsigned long val;
    int hex_len;
    char *endptr;

    if (argc < 3) {
        putstr("Usage: mw <addr> <value>\r\n");
        return -1;
    }

    addr = strtoul(argv[1], NULL, 16);

    /* Determine hex digit count to auto-detect write size */
    hex_len = strlen(argv[2]);

    if (hex_len <= 2) {
        /* 8-bit byte write */
        val = strtoul(argv[2], &endptr, 16);
        if (*endptr != '\0') {
            putstr("Error: invalid value\r\n");
            return -1;
        }
        *(unsigned char *)addr = (unsigned char)val;
        putstr("Wrote ");
        puthex(val, hex_len);
        putstr(" to ");
        puthex(addr, 8);
        putnl();
    } else if (hex_len <= 4) {
        /* 16-bit word write */
        if (addr & 0x01) {
            putstr("Error: address not 2-byte aligned\r\n");
            return -1;
        }
        val = strtoul(argv[2], &endptr, 16);
        if (*endptr != '\0') {
            putstr("Error: invalid value\r\n");
            return -1;
        }
        *(unsigned short *)addr = (unsigned short)val;
        putstr("Wrote ");
        puthex(val, hex_len);
        putstr(" to ");
        puthex(addr, 8);
        putnl();
    } else if (hex_len <= 8) {
        /* 32-bit longword write */
        if (addr & 0x03) {
            putstr("Error: address not 4-byte aligned\r\n");
            return -1;
        }
        val = strtoul(argv[2], &endptr, 16);
        if (*endptr != '\0') {
            putstr("Error: invalid value\r\n");
            return -1;
        }
        *(unsigned long *)addr = val;
        putstr("Wrote ");
        puthex(val, hex_len);
        putstr(" to ");
        puthex(addr, 8);
        putnl();
    } else {
        putstr("Error: value too large (max 8 hex digits)\r\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Memory fill command - fill memory with 16-bit pattern
 * @param argc Argument count (must be >= 4)
 * @param argv Argument vector: argv[1]=addr, argv[2]=len, argv[3]=val
 * @return 0 on success, -1 on error
 *
 * Usage: mf <addr> <len> <val>
 *   addr: Starting address in hex (must be 2-byte aligned)
 *   len:  Number of 16-bit words to fill in hex
 *   val:  16-bit fill value in hex
 *
 * Fills len 16-bit words (len * 2 bytes) with the specified value.
 * Output: "Filled <len> words at <addr> with <val>"
 */
int cmd_mf(int argc, char *argv[])
{
    unsigned long addr;
    unsigned long len;
    unsigned long val;
    unsigned long i;

    if (argc < 4) {
        putstr("Usage: mf <addr> <len> <value>\r\n");
        return -1;
    }

    addr = strtoul(argv[1], NULL, 16);
    len = strtoul(argv[2], NULL, 16);
    val = strtoul(argv[3], NULL, 16);

    for (i = 0; i < len; i++) {
        ((unsigned short *)addr)[i] = (unsigned short)val;
    }
    putstr("Filled ");
    puthex(len, 4);
    putstr(" words at ");
    puthex(addr, 8);
    putstr(" with ");
    puthex(val, 4);
    putnl();
    return 0;
}

/**
 * @brief Memory copy command - copy block of memory
 * @param argc Argument count (must be >= 4)
 * @param argv Argument vector: argv[1]=src, argv[2]=dst, argv[3]=len
 * @return 0 on success, -1 on error
 *
 * Usage: mc <src> <dst> <len>
 *   src:  Source address in hex (must be 2-byte aligned)
 *   dst:  Destination address in hex (must be 2-byte aligned)
 *   len:  Number of 16-bit words to copy in hex
 *
 * Copies len 16-bit words (len * 2 bytes) from source to destination.
 * Overlapping regions are handled correctly (like memmove).
 * Output: "Copied <len> words from <src> to <dst>"
 */
int cmd_mc(int argc, char *argv[])
{
    unsigned long src;
    unsigned long dst;
    unsigned long len;
    unsigned long i;

    if (argc < 4) {
        putstr("Usage: mc <src> <dst> <len>\r\n");
        return -1;
    }

    src = strtoul(argv[1], NULL, 16);
    dst = strtoul(argv[2], NULL, 16);
    len = strtoul(argv[3], NULL, 16);

    for (i = 0; i < len; i++) {
        ((unsigned short *)dst)[i] = ((unsigned short *)src)[i];
    }
    putstr("Copied ");
    puthex(len, 4);
    putstr(" words from ");
    puthex(src, 8);
    putstr(" to ");
    puthex(dst, 8);
    putnl();
    return 0;
}