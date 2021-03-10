/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */

#include "cmdline.h"
#include "errors.h"
#include "wrappers.h"
#include <stdio.h>
#include <string.h>

/* forward declarations */
static cmdline_switch *parse_switch(cmdline_switch *sw, int pos, int argc,
                                    char **argv);
static int parse_arguments(cmdline_switch *sw, int pos, int argc, char **argv);
static cmdline_switch *find_switch(cmdline_switch *sw, const char *s_search);
static cmdline_switch *first_switch(cmdline_switch *sw);
static size_t count_elements(cmdline_switch *sw, size_t e);

/* constructor for cmdline_switch, only used internally */
static cmdline_switch *cmdline_switch_alloc() {
    cmdline_switch *sw = md_malloc(sizeof(cmdline_switch));
    sw->id = NULL;
    sw->args = NULL;
    sw->argc = 0;
    sw->prev = NULL;
    sw->next = NULL;
    return sw;
}

void cmdline_free(cmdline_switch *sw) {
    if (sw == NULL)
        return;
    cmdline_free(sw->next);
    if (sw->args != NULL) {
        for (size_t i = 0; i < sw->argc; i++)
            free(sw->args[i]);
        free(sw->args);
    }
    free(sw);
}

cmdline_switch *cmdline_parse(int argc, char **argv) {
    return parse_switch(NULL, 1, argc, argv);
}

static int parse_arguments(cmdline_switch *sw, int pos, int argc, char **argv) {
    char **arguments = NULL;
    size_t elements = 0;
    for (int i = pos; i < argc; i++) {
        if (argv[i][0] == '-')
            break; /* end of arguments */
        elements++;
        if (elements == 1) {
            arguments = md_malloc(sizeof(char *));
        } else {
            arguments = md_realloc(arguments, sizeof(char *) * elements);
        }
        arguments[elements - 1] = md_strdup(argv[i]);
    }
    sw->argc = elements;
    sw->args = arguments;
    return 0;
}

static cmdline_switch *parse_switch(cmdline_switch *sw, int pos, int argc,
                                    char **argv) {
    if (pos >= argc) {
        return NULL; /* base case */
    }
    if (argv[pos][0] != '-') {
        const size_t bufsize = 64;
        char *buf = md_malloc(bufsize);
        snprintf(buf, bufsize,
                 "Command line parser: Expected switch at token %d", pos);
        md_error_custom(buf);
        return NULL;
    }

    /* backtrack to detect duplicate switches */
    if (find_switch(first_switch(sw), argv[pos]) != NULL) {
        const size_t bufsize = 128;
        char *buf = md_malloc(bufsize);
        snprintf(buf, bufsize,
                 "Command line parser: Duplicate command line switch \"%s\"",
                 argv[pos]);
        md_error_custom(buf);
        return NULL;
    }

    cmdline_switch *sw_prev = NULL;
    if (sw != NULL)
        sw_prev = sw;
    sw = cmdline_switch_alloc();
    sw->id = argv[pos];
    parse_arguments(sw, pos + 1, argc, argv);
    sw->prev = sw_prev;
    sw->next = parse_switch(sw, pos + 1 + sw->argc, argc, argv);
    return sw;
}

/* searches for a particular command line switch and returns its object if found
 */
static cmdline_switch *find_switch(cmdline_switch *sw, const char *s_search) {
    if (sw == NULL)
        return NULL; // base case
    if (!strcmp(sw->id, s_search))
        return sw; // found match
    return find_switch(sw->next, s_search);
}

/* returns first switch of the list */
static cmdline_switch *first_switch(cmdline_switch *sw) {
    if (sw == NULL)
        return NULL;
    if (sw->prev == NULL)
        return sw;
    return first_switch(sw->prev);
}

size_t cmdline_elements(cmdline_switch *sw) {
    if (sw == NULL)
        return 0;
    cmdline_switch *start = first_switch(sw);
    return count_elements(start, 0);
}

static size_t count_elements(cmdline_switch *sw, size_t e) {
    if (sw == NULL)
        return e;
    e++;
    return count_elements(sw->next, e);
}
