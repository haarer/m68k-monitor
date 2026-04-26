#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <platform.h>
#include <commands.h>

extern struct _reent reent_main;

static char linebuf[64];
static int linepos;

static void putstr(const char *s)
{
    while (*s) {
        v_uartPutch(*s++);
    }
}

static void putnl(void)
{
    v_uartPutch('\r');
    v_uartPutch('\n');
}

static void prompt(void)
{
    putstr("MON> ");
}

static char *skip_ws(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

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