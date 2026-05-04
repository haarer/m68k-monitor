/**
 * @file commands.h
 * @brief Command interface for m68k-monitor
 *
 * This module defines the command structure and declares all user-facing
 * monitor commands for the MC68331 monitor.
 *
 * Supported commands:
 *   - help: Show help message
 *   - md:   Memory dump
 *   - mw:   Memory write
 *   - mf:   Memory fill
 *   - mc:   Memory copy
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

/**
 * @brief Command structure
 *
 * Each command has a name, help text, and a function pointer.
 * The function receives argc and argv (similar to main()).
 */
typedef struct {
    const char *name;       /**< Command name as typed by user */
    const char *help;       /**< Help text shown in help command */
    int (*func)(int argc, char *argv[]);  /**< Command implementation */
} cmd_t;

/* Command implementations */
int cmd_help(int argc, char *argv[]);
int cmd_md(int argc, char *argv[]);
int cmd_mw(int argc, char *argv[]);
int cmd_mf(int argc, char *argv[]);
int cmd_mc(int argc, char *argv[]);
int cmd_srec(int argc, char *argv[]);

#define NUM_COMMANDS 6

/**
 * @brief Command table
 *
 * This table is used by main.c to look up and execute commands.
 * The help text is displayed by the help command.
 */
static const cmd_t commands[NUM_COMMANDS] = {
    {"help", "help           - show this help", cmd_help},
    {"md",  "md <addr> <len> - dump memory", cmd_md},
    {"mw",  "mw <addr> <val> - write memory", cmd_mw},
    {"mf",  "mf <addr> <len> <val> - fill memory", cmd_mf},
    {"mc",  "mc <src> <dst> <len> - copy memory", cmd_mc},
    {"srec", "srec <addr>   - load S-Record data to memory", cmd_srec},
};

#endif /* COMMANDS_H */
