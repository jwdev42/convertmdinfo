/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#ifndef _INCL_EVAL
#define _INCL_EVAL

#include "cmdline.h"
#include "mdinfo.h"
#include <stdbool.h>

typedef enum {
    EVAL_UNDEFINED, /*unknown switch*/
    EVAL_GLOBAL,    /* switch is compatible with every class of command line
                       switches */
    EVAL_PRIMARY,   /* switch is part of a manual specification of display
                       metadata */
    EVAL_FFMPEG,    /* switch is related to an interaction with ffmpeg */
} eval_class;

typedef struct eval_container {
    eval_class type;
    /* global options */
    char *output_file;
    /* manual metadata input */
    disp_meta *col;
    disp_lum *lum;
    /* ffmpeg options */
    char *ffinput;
    bool ffdynamic;
} eval_container;

eval_container *eval_container_alloc();
void eval_container_free(eval_container *ct);

eval_container *eval_cmdline(eval_container *ct, cmdline_switch *sw);

#endif
