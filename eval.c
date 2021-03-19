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

/* forward declarations */
static void eval_err_undefined(cmdline_switch *sw);
static void eval_err_class_mismatch(cmdline_switch *sw);

eval_container *eval_container_alloc() {
    eval_container *ct = md_malloc(sizeof(eval_container));
    ct->type = EVAL_UNDEFINED;
    ct->output_file = NULL;
    ct->col = disp_meta_alloc();
    ct->lum = disp_lum_alloc();
    ct->ffinput = NULL;
    ct->ffdynamic = false;
    return ct;
}

void eval_container_free(eval_container *ct) {
    if (ct->output_file)
        free(ct->output_file);
    if (ct->ffinput)
        free(ct->ffinput);
    if (ct->col)
        disp_meta_free(ct->col);
    if (ct->lum)
        disp_lum_free(ct->lum);
    free(ct);
}

/* set the type of the container, abort program on type conflict */
void eval_container_type(eval_container *ct, eval_class type,
                         cmdline_switch *sw) {
    switch (ct->type) {
    case EVAL_GLOBAL:
        return; /* ignore global args */
    case EVAL_UNDEFINED:
        ct->type = type;
        return;
    default:
        if (ct->type != type) {
            eval_err_class_mismatch(sw);
            exit_on_error();
        }
    }
}

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

char *eval_file(char **input, int elements) {
    if (elements != 1) {
        global_md_error = ERR_INPUT;
        return NULL;
    }
    return md_strdup(input[0]);
}

eval_container *eval_cmdline(eval_container *ct, cmdline_switch *sw) {
    if (sw == NULL)
        return ct; // base case
    if (!strcmp("-r", sw->id)) {
        eval_container_type(ct, EVAL_PRIMARY, sw);
        ct->col->r = eval_point(sw->args, sw->argc);
    } else if (!strcmp("-g", sw->id)) {
        eval_container_type(ct, EVAL_PRIMARY, sw);
        ct->col->g = eval_point(sw->args, sw->argc);
    } else if (!strcmp("-b", sw->id)) {
        eval_container_type(ct, EVAL_PRIMARY, sw);
        ct->col->b = eval_point(sw->args, sw->argc);
    } else if (!strcmp("-wp", sw->id)) {
        eval_container_type(ct, EVAL_PRIMARY, sw);
        ct->col->wp = eval_point(sw->args, sw->argc);
    } else if (!strcmp("-lmin", sw->id)) {
        eval_container_type(ct, EVAL_PRIMARY, sw);
        ct->lum->min = eval_lum(sw->args, sw->argc);
    } else if (!strcmp("-lmax", sw->id)) {
        eval_container_type(ct, EVAL_PRIMARY, sw);
        ct->lum->max = eval_lum(sw->args, sw->argc);
    } else if (!strcmp("-i", sw->id)) {
        eval_container_type(ct, EVAL_FFMPEG, sw);
        ct->ffinput = eval_file(sw->args, sw->argc);
    } else if (!strcmp("-dynamic", sw->id)) {
        eval_container_type(ct, EVAL_FFMPEG, sw);
        ct->ffdynamic = true;
    } else if (!strcmp("-o", sw->id)) {
        ct->output_file = eval_file(sw->args, sw->argc);
    } else
        eval_err_undefined(sw);
    if (global_md_error != ERR_NONE)
        return NULL;
    return eval_cmdline(ct, sw->next);
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
