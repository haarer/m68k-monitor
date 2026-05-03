#ifndef COMMANDS_H
#define COMMANDS_H

typedef struct {
    const char *name;
    const char *help;
    int (*func)(int argc, char *argv[]);
} cmd_t;

int cmd_help(int argc, char *argv[]);
int cmd_md(int argc, char *argv[]);
int cmd_mw(int argc, char *argv[]);
int cmd_mf(int argc, char *argv[]);
int cmd_mc(int argc, char *argv[]);

#define NUM_COMMANDS 5

static const cmd_t commands[NUM_COMMANDS] = {
    {"help", "help           - show this help", cmd_help},
    {"md",  "md <addr> <len> - dump memory", cmd_md},
    {"mw",  "mw <addr> <val> - write memory", cmd_mw},
    {"mf",  "mf <addr> <len> <val> - fill memory", cmd_mf},
    {"mc",  "mc <src> <dst> <len> - copy memory", cmd_mc},
};

#endif