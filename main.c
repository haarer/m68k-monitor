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
 *   - Cursor Up/Down (ESC[A / ESC[B) navigate command history
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <platform.h>
#include <commands.h>

extern struct _reent reent_main;

#define HISTORY_SIZE 10

static char linebuf[64];
static int linepos;

static char history[HISTORY_SIZE][64];
static int history_count;
static int history_pos;

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
 * @brief Redraw the current line
 * Clears the line and re-displays prompt + buffer content
 */
static void redraw_line(void)
{
    int i;
    putstr("\r\b");
    prompt();
    for (i = 0; i < linepos; i++) {
        v_uartPutch(linebuf[i]);
    }
}

/**
 * @brief Add a command to the history buffer
 * @param cmd Command string to add
 */
static void history_add(const char *cmd)
{
    int len;
    if (cmd[0] == '\0') return;

    len = strlen(cmd);
    if (len >= 64) len = 63;

    if (history_count < HISTORY_SIZE) {
        strncpy(history[history_count], cmd, len);
        history[history_count][len] = '\0';
        history_count++;
    } else {
        memmove(history, history + 1, (HISTORY_SIZE - 1) * sizeof(history[0]));
        strncpy(history[HISTORY_SIZE - 1], cmd, len);
        history[HISTORY_SIZE - 1][len] = '\0';
    }

    history_pos = history_count;
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
    int esc_seq;

    init_main();

    putstr("\r\n");
    putstr("MC68331 Monitor v0.1\r\n");
    putstr("Type 'help' for commands\r\n");
    putnl();

    linepos = 0;
    history_count = 0;
    history_pos = 0;

    for (;;) {
        prompt();

        while (1) {
            ch = i_uartGetch();

            if (ch == 0x1B) {
                esc_seq = i_uartGetch();
                if (esc_seq == '[') {
                    esc_seq = i_uartGetch();

                    if (esc_seq == 'A') {
                        if (history_pos > 0) {
                            history_pos--;
                            strncpy(linebuf, history[history_pos], sizeof(linebuf) - 1);
                            linebuf[sizeof(linebuf) - 1] = '\0';
                            linepos = strlen(linebuf);
                            redraw_line();
                        }
                    } else if (esc_seq == 'B') {
                        if (history_pos < history_count) {
                            history_pos++;
                            if (history_pos < history_count) {
                                strncpy(linebuf, history[history_pos], sizeof(linebuf) - 1);
                                linebuf[sizeof(linebuf) - 1] = '\0';
                                linepos = strlen(linebuf);
                            } else {
                                linebuf[0] = '\0';
                                linepos = 0;
                            }
                            redraw_line();
                        }
                    }
                }
                continue;
            }

            v_uartPutch(ch);

            if (ch == '\r') {
                putnl();
                linebuf[linepos] = '\0';
                history_add(linebuf);
                break;
            }
            if (ch == '\b' || ch == 0x7f) {
                if (linepos > 0) {
                    linepos--;
                    redraw_line();
                }
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