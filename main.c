/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */

#include "cmdline.h"
#include "errors.h"
#include "eval.h"
#include "ffmpeg.h"
#include "mdinfo.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void print_usage() {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr,
            "convertmdinfo -r %%d %%d -g %%d %%d -b %%d %%d -wp %%d %%d "
            "-lmin %%d -lmax %%d\n");
}

static FILE *open_output_stream(const char *filename) {
    if (filename == NULL)
        return stdout;
    FILE *ostream = fopen(filename, "w");
    if (ostream == NULL) {
        md_error_custom(strerror(errno));
        return NULL;
    }
    return ostream;
}

static void close_output_stream(FILE *stream) {
    if (fclose(stream) == EOF)
        md_error_custom(strerror(errno));
}

int manual_metadata_input(eval_container *ct, FILE *ostream) {
    /*verify command line input */
    disp_meta_verify(ct->col);
    exit_on_error();
    disp_lum_verify(ct->lum);
    exit_on_error();

    /*convert & print */
    disp_meta_x265 *meta_x265 = meta_to_x265(ct->col, ct->lum);
    exit_on_error();
    char *display_str = x265_str(meta_x265);
    fprintf(ostream, "%s\n", display_str);

    free(display_str);
    disp_meta_x265_free(meta_x265);
    return 0;
}

int process_ffmpeg_input(eval_container *ct, FILE *ostream) {
    if (ct->ffinput == NULL) {
        md_error_custom("No input file specified for ffmpeg");
        return -1;
    }
    if (ct->ffdynamic) {
        md_error_custom("dynamic metadata support is not implemented yet");
        return -1;
    }
    return ffmpeg_access_sidedata(ct->ffinput, ostream, &ffmpeg_disp_meta, 24);
}

int main(int argc, char **argv) {
    /* parse command line */
    cmdline_switch *sw = cmdline_parse(argc, argv);
    exit_on_error();
    if (sw == NULL) {
        print_usage();
        return 0;
    }

    /* evaluate command line */
    eval_container *ct = eval_container_alloc();
    eval_cmdline(ct, sw);
    exit_on_error();

    /* free command line parser memory */
    cmdline_free(sw);

    if (ct->type == EVAL_UNDEFINED || ct->type == EVAL_GLOBAL) {
        print_usage();
    } else {

        FILE *ostream = open_output_stream(ct->output_file);
        exit_on_error();

        switch (ct->type) {
        case EVAL_PRIMARY:
            manual_metadata_input(ct, ostream);
            break;
        case EVAL_FFMPEG:
            process_ffmpeg_input(ct, ostream);
            break;
        default:
            md_bug(__FILE__, __LINE__, false);
        }
        exit_on_error();

        /* cleanup */
        if (ostream != stdout) {
            close_output_stream(ostream);
            exit_on_error();
        }
    }
    /* cleanup */
    eval_container_free(ct);
    return 0;
}
