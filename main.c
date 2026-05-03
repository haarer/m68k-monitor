/**
 * @file main.c
 * @brief Main entry point and command-line interpreter for m68k-monitor
 *
 * This is the main loop of the MC68331 monitor. It initializes
 * the hardware, displays the welcome banner, and enters a command
 * processing loop.
 *
 * The main loop:
 *   1. Displays "MON> " prompt
 *   2. Reads characters from UART (blocking)
 *   3. Buffers input until carriage return (\r)
 *   4. Parses the line into argc/argv
 *   5. Looks up and executes the matching command
 *
 * Line editing:
 *   - Backspace (\b or 0x7f) deletes last character
 *   - Carriage return (\r) terminates input
 *   - Only printable characters (>= ' ') are accepted
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <platform.h>
#include <commands.h>

extern struct _reent reent_main;

static char linebuf[64];  /**< Input line buffer (64 bytes max) */
static int linepos;          /**< Current position in linebuf */

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
 * @brief Display the command prompt
 * Shows "MON> " to indicate ready for input
 */
static void prompt(void)
{
    putstr("MON> ");
}

/**
 * @brief Skip whitespace characters
 * @param s String to process
 * @return Pointer to first non-whitespace character
 */
static char *skip_ws(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

/**
 * @brief Parse a line into argc/argv format
 * @param line Input line to parse (modified in-place)
 * @param argv Array to store argument pointers (max 16)
 * @return Number of arguments parsed
 *
 * Arguments are separated by spaces/tabs.
 * The line buffer is modified to NULL-terminate each argument.
 */
static int parse_line(char *line, char *argv[])
{
    int argc = 0;
    char *p = line;
    char *start;

    p = skip_ws(p);
    if (*p == '\0') return 0;

    while (*p && argc < 16) {
        start = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
        argv[argc++] = start;
        p = skip_ws(p);
    }
    return argc;
}

/**
 * @brief Find a command by name
 * @param name Command name to search for
 * @return Command index in commands[] array, or -1 if not found
 */
static int find_command(const char *name)
{
    int i;
    for (i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(name, commands[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

void v_uartInit(void);
void init_main(void);

/**
 * @brief Main entry point for m68k-monitor
 * @param argc Argument count (unused)
 * @param argv Argument vector (unused)
 * @return 0 on exit (never reached in monitor mode)
 *
 * Initializes hardware and enters the main command loop.
 * The loop displays a prompt, reads a line of input, and
 * executes the corresponding command.
 */
int main(int argc, char *argv[])
{
    int ch;
    int i;
    int cmd_idx;
    char *cmd_argv[16];

    init_main();

    putstr("\r\n");
    putstr("MC68331 Monitor v0.1\r\n");
    putstr("Type 'help' for commands\r\n");
    putnl();

    linepos = 0;

    for (;;) {
        prompt();

        while (1) {
            ch = i_uartGetch();
            v_uartPutch(ch);
            if (ch == '\r') {
                putnl();
                linebuf[linepos] = '\0';
                break;
            }
            if (ch == '\b' || ch == 0x7f) {
                if (linepos > 0) linepos--;
                continue;
            }
            if (ch >= ' ' && linepos < sizeof(linebuf) - 1) {
                linebuf[linepos++] = ch;
            }
        }

        linepos = 0;
        if (linebuf[0] == '\0') continue;

        i = parse_line(linebuf, cmd_argv);
        if (i == 0) continue;

        cmd_idx = find_command(cmd_argv[0]);
        if (cmd_idx < 0) {
            putstr("Unknown command: ");
            putstr(cmd_argv[0]);
            putnl();
            continue;
        }

        commands[cmd_idx].func(i, cmd_argv);
    }

    return 0;
}