/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#ifndef _INCL_CMDLINE
#define _INCL_CMDLINE

#include <stdlib.h>

typedef struct cmdline_switch {
    char *id;
    char **args;
    size_t argc;
    struct cmdline_switch *prev;
    struct cmdline_switch *next;
} cmdline_switch;

cmdline_switch *cmdline_parse(int argc, char **argv);

/* recursively frees all cmdline_switch structs */
void cmdline_free(cmdline_switch *sw);

/* return amount of parsed command line switches, always backtraces to the first
 * element */
size_t cmdline_elements(cmdline_switch *sw);
#endif
