/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */

#include "eval.h"
#include "cmdline.h"
#include "errors.h"
#include "ffmpeg.h"
#include "mdinfo.h"
#include "wrappers.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    EVAL_UNDEFINED, /*unknown switch*/
    EVAL_GLOBAL,    /* switch is compatible with every class of command line
                       switches */
    EVAL_PRIMARY,   /* switch is part of a manual specification of display
                       metadata */
    EVAL_FFMPEG,    /* switch is related to an interaction with ffmpeg */
} eval_class;

/* forward declarations */
static void eval_err_undefined(cmdline_switch *sw);
static void eval_err_class_mismatch(cmdline_switch *sw);
static int eval_check_conflict(eval_class *history, int offset);
static disp_meta *eval_cmdline_internal(cmdline_switch *sw, eval_class *history,
                                        int i, disp_meta *meta, disp_lum *lum);

/* returns true if the input string represents a floating-point value of zero */
static bool is_zero(const char *input) {
    size_t len = strlen(input);
    if (len == 0)
        return false;
    bool dot = false;
    for (size_t i = 0; i < len; i++) {
        if (i == 0) {
            if (input[i] == '-' || input[i] == '0')
                continue;
            else
                return false;
        } else {
            if (input[i] == '.') {
                if (dot || i == len - 1)
                    return false;
                if (input[i - 1] != '0' || input[i + 1] != '0')
                    return false;
                dot = true;
                continue;
            } else if (input[i] == '0')
                continue;
            else
                return false;
        }
    }
    return true;
}

static double parse_double(const char *input) {
    double d = atof(input);
    if (d == 0 &&
        !is_zero(
            input)) { /* input could not be parsed as floating-point value */
        size_t bufsize = 17 + strlen(input) + 1;
        char *buf = md_malloc(bufsize);
        snprintf(buf, bufsize, "Invalid decimal: %s", input);
        md_error_custom(buf);
    }
    return d;
}

static point *eval_point(char **input, int elements) {

    if (elements != 2) {
        global_md_error = ERR_INPUT;
        return NULL;
    }
    double x = parse_double(input[0]);
    if (global_md_error != ERR_NONE)
        return NULL;
    double y = parse_double(input[1]);
    if (global_md_error != ERR_NONE)
        return NULL;
    point *p = md_malloc(sizeof(point));
    p->x = x;
    p->y = y;
    return p;
}

static double eval_lum(char **input, int elements) {
    if (elements != 1) {
        global_md_error = ERR_INPUT;
        return 0;
    }
    return parse_double(input[0]);
}

static disp_meta *eval_ffmpeg(char **input, int elements, disp_meta *meta,
                              disp_lum *lum) {
    if (elements != 1) {
        global_md_error = ERR_INPUT;
        return NULL;
    }
    if (ffmpeg_recv_meta(input[0], meta, lum) < 0)
        return NULL;
    return meta;
}
/*
disp_meta *eval_cmdline(cmdline_switch *sw, disp_meta *meta, disp_lum *lum) {
    if (sw == NULL)
        return meta; // base case
    if (!strcmp("-r", sw->id))
        meta->r = eval_point(sw->args, sw->argc);
    else if (!strcmp("-g", sw->id))
        meta->g = eval_point(sw->args, sw->argc);
    else if (!strcmp("-b", sw->id))
        meta->b = eval_point(sw->args, sw->argc);
    else if (!strcmp("-wp", sw->id))
        meta->wp = eval_point(sw->args, sw->argc);
    else if (!strcmp("-lmin", sw->id))
        lum->min = eval_lum(sw->args, sw->argc);
    else if (!strcmp("-lmax", sw->id))
        lum->max = eval_lum(sw->args, sw->argc);
    else if (!strcmp("-i", sw->id))
        return eval_ffmpeg(sw->args, sw->argc, meta, lum);
    if (global_md_error != ERR_NONE)
        return NULL;
    return eval_cmdline(sw->next, meta, lum);
}
*/

disp_meta *eval_cmdline(cmdline_switch *sw, disp_meta *meta, disp_lum *lum) {
    eval_class *history = md_calloc(cmdline_elements(sw), sizeof(eval_class));
    disp_meta *ret = eval_cmdline_internal(sw, history, 0, meta, lum);
    free(history);
    return ret;
}

static disp_meta *eval_cmdline_internal(cmdline_switch *sw, eval_class *history,
                                        int i, disp_meta *meta, disp_lum *lum) {
    if (sw == NULL)
        return meta; // base case
    if (!strcmp("-r", sw->id)) {
        *history = EVAL_PRIMARY;
        meta->r = eval_point(sw->args, sw->argc);
    } else if (!strcmp("-g", sw->id)) {
        *history = EVAL_PRIMARY;
        meta->g = eval_point(sw->args, sw->argc);
    } else if (!strcmp("-b", sw->id)) {
        *history = EVAL_PRIMARY;
        meta->b = eval_point(sw->args, sw->argc);
    } else if (!strcmp("-wp", sw->id)) {
        *history = EVAL_PRIMARY;
        meta->wp = eval_point(sw->args, sw->argc);
    } else if (!strcmp("-lmin", sw->id)) {
        *history = EVAL_PRIMARY;
        lum->min = eval_lum(sw->args, sw->argc);
    } else if (!strcmp("-lmax", sw->id)) {
        *history = EVAL_PRIMARY;
        lum->max = eval_lum(sw->args, sw->argc);
    } else if (!strcmp("-i", sw->id)) {
        *history = EVAL_FFMPEG;
        return eval_ffmpeg(sw->args, sw->argc, meta, lum);
    } else if (!strcmp("-dynamic", sw->id)) {
        *history = EVAL_FFMPEG;
        /* TODO: Implement dynamic metadata scraping */
    } else if (!strcmp("-o", sw->id)) {
        *history = EVAL_GLOBAL;
        /* TODO: Implement output file support */
    } else
        eval_err_undefined(sw);
    if (eval_check_conflict(history, i))
        eval_err_class_mismatch(sw);
    if (global_md_error != ERR_NONE)
        return NULL;
    return eval_cmdline_internal(sw->next, ++history, ++i, meta, lum);
}

static int eval_check_conflict(eval_class *history, int offset) {
    if (offset < 0)
        md_bug(__FILE__, __LINE__, true);
    if (*history == EVAL_GLOBAL)
        return 0;
    eval_class *start = history - offset;
    for (int i = 0; i < offset + 1; i++) {
        eval_class c = start[i];
        if (c == EVAL_GLOBAL)
            continue;
        else if (c != *history)
            return -1;
    }
    return 0;
}

/* thrown if an unknown command line switch is encountered */
static void eval_err_undefined(cmdline_switch *sw) {
    const size_t len = 128;
    char *msg = md_malloc(len);
    snprintf(msg, len, "Unknown command line switch \"%s\"", sw->id);
    md_error_custom(msg);
}

static void eval_err_class_mismatch(cmdline_switch *sw) {
    const size_t len = 128;
    char *msg = md_malloc(len);
    snprintf(msg, len, "Class mismatch for switch \"%s\"", sw->id);
    md_error_custom(msg);
}
