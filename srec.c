/**
 * @file srec.c
 * @brief S-Record parser and loader for m68k-monitor
 *
 * This file implements functions to parse Motorola S-Records and load 
 * binary data into memory. The format supports loading programs from 
 * external files via the serial console.
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
 * @brief Output CRLF (carriage return + line feed) via UART
 */
static void putnl(void)
{
    v_uartPutch('\r');
    v_uartPutch('\n');
}

/**
 * @brief Convert hex character to value (0-15)
 * @param c Hex character ('0'-'9', 'a'-'f', 'A'-'F')
 * @return Value of hex character
 */
static unsigned char hexchar_to_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0; // Invalid character
}

/**
 * @brief Convert hex string to unsigned long value
 * @param s String of hex characters 
 * @param len Number of hex characters to convert
 * @return Converted value
 */
static unsigned long hexstr_to_ulong(const char* s, int len)
{
    unsigned long val = 0;
    int i;
    for (i = 0; i < len && *s; i++) {
        val <<= 4;
        val |= hexchar_to_value(*s);
        s++;
    }
    return val;
}

/**
 * @brief Parse an S-Record line and load data into memory
 * @param line Input line to parse (S0, S1, S2, or S3 records)
 * @return 0 on success, -1 on error
 *
 * Supports:
 *   - S1 records: 16-bit address format (loads data at specified address)
 *   - S2 records: 24-bit address format (loads data at specified address)  
 *   - S3 records: 32-bit address format (loads data at specified address)
 *
 * Format:
 *   S<type><count><address><data><checksum>
 */
static int parse_srec_line(char* line)
{
    char type = line[1];

    if (*line != 'S' || !(type >= '0' && type <= '9')) {
        return -1;
    }

    // Parse count (2 hex chars at offset 2)
    unsigned long count = hexstr_to_ulong(line + 2, 2);

    int addr_len;
    unsigned long addr;

    switch (type) {
        case '1':
            addr_len = 4;
            break;
        case '2':
            addr_len = 6;
            break;
        case '3':
            addr_len = 8;
            break;
        default:
            return -1;
    }

    // Address starts at offset 4 (after "S<type><count>")
    char* addr_start = line + 4;
    addr = hexstr_to_ulong(addr_start, addr_len);

    // Data starts after address field
    unsigned long data_len = count - (addr_len / 2) - 1;
    char* data_start = addr_start + addr_len;

    unsigned long i;
    for (i = 0; i < data_len; i++) {
        unsigned long byte_val = hexstr_to_ulong(data_start + i * 2, 2);
        ((unsigned char*)addr)[i] = (unsigned char)byte_val;
    }

    return 0;
}

/**
 * @brief S-Record upload command - load binary data from serial input
 * @param argc Argument count (unused)
 * @param argv Argument vector (unused)
 * @return 0 on success, -1 on error
 *
 * Usage: srec <addr> 
 *   addr: Starting address in hex where to load the SREC data
 *
 * This command allows loading of Motorola S-Record files over serial.
 * The user types a series of S-record lines terminated by a blank line or Ctrl+D.
 */
int cmd_srec(int argc, char *argv[])
{
    unsigned long start_addr = 0;
    
    if (argc < 2) {
        putstr("Usage: srec <addr>\r\n");
        putstr("Load Motorola S-Record data into memory at address\r\n");
        return -1;
    }
    
    // Parse the starting address
    start_addr = strtoul(argv[1], NULL, 16);
    
    if (start_addr == 0) {
        putstr("Invalid address specified\r\n");
        return -1;
    }
    
    putstr("\r\nEnter Motorola S-Record data:\r\n");
    putstr("Lines should be in format: S<type><count><address><data><checksum>\r\n");
    putstr("End with blank line or Ctrl+D (ASCII 0x04)\r\n\r\n");
    
    char linebuf[256];
    int linepos = 0;
    int ch;
    
    // Process input one character at a time
    while (1) {
        ch = i_uartGetch();
        
        if (ch == '\r') {  // Handle CR, treat as LF for line ending
            v_uartPutch('\r');
            v_uartPutch('\n');
            
            if (linepos > 0) {
                linebuf[linepos] = '\0';
                
                // Check if this is a blank line to end input
                char* p = linebuf;
                while (*p == ' ' || *p == '\t') p++;
                if (*p == '\0') {
                    break;  // Blank line ends the SREC data entry
                }
                
                // Parse and load the S-Record line 
                if (parse_srec_line(linebuf) < 0) {
                    putstr("Error parsing line: ");
                    putstr(linebuf);
                    putnl();
                } else {
                    putstr("Loaded record at address ");
                    putstr(argv[1]);
                    putnl();
                }
            }
            
            // Reset for next line
            linepos = 0;
        } else if (ch == '\b' || ch == 0x7f) {  // Backspace handling
            if (linepos > 0) {
                v_uartPutch(ch);
                linepos--;
            }
        } else if (ch >= ' ') {  // Printable characters only
            if (linepos < sizeof(linebuf) - 1) {
                v_uartPutch(ch);
                linebuf[linepos++] = ch;
            }
        } else if (ch == 0x04) {  // Ctrl+D to end input
            break;
        }
    }
    
    putstr("\r\nS-Record upload completed.\r\n");
    return 0;
}