#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <commands.h>
#include <uart.h>

extern void v_uartPutch(unsigned int ch);

static void putstr(const char *s)
{
    while (*s) {
        v_uartPutch(*s++);
    }
}

static void puthex(unsigned long val, int digits)
{
    const char *hex = "0123456789abcdef";
    while (digits-- > 0) {
        v_uartPutch(hex[(val >> (digits * 4)) & 0xf]);
    }
}

static void putnl(void)
{
    v_uartPutch('\r');
    v_uartPutch('\n');
}

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

    addr = strtoul(argv[1], NULL, 0);
    len = strtoul(argv[2], NULL, 0);

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

int cmd_mw(int argc, char *argv[])
{
    unsigned long addr;
    unsigned long val;

    if (argc < 3) {
        putstr("Usage: mw <addr> <value>\r\n");
        return -1;
    }

    addr = strtoul(argv[1], NULL, 0);
    val = strtoul(argv[2], NULL, 0);

    *(unsigned short *)addr = (unsigned short)val;
    putstr("Wrote ");
    puthex(val, 4);
    putstr(" to ");
    puthex(addr, 8);
    putnl();
    return 0;
}

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

    addr = strtoul(argv[1], NULL, 0);
    len = strtoul(argv[2], NULL, 0);
    val = strtoul(argv[3], NULL, 0);

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

    src = strtoul(argv[1], NULL, 0);
    dst = strtoul(argv[2], NULL, 0);
    len = strtoul(argv[3], NULL, 0);

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