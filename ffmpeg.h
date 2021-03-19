/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#ifndef _INCL_FFMPEG
#define _INCL_FFMPEG

#include "mdinfo.h"
#include <libavutil/frame.h>
#include <stdio.h>

typedef enum {
    FFRET_ERROR,    /* error reported */
    FFRET_CONTINUE, /* continue AVFrameSideData loop, more data from frame
                       expected */
    FFRET_BREAK,    /* break AVFrameSideData loop, done with this frame */
    FFRET_DONE,     /* do not decode further frames, gleaned all information */
} ff_return_t;

int ffmpeg_access_sidedata(const char *path, FILE *ostream,
                           ff_return_t (*recv_func)(FILE *, AVFrameSideData *),
                           uint64_t frame_limit);

int ffmpeg_recv_meta(const char *path, disp_meta *meta, disp_lum *lum);

ff_return_t ffmpeg_disp_meta(FILE *ostream, AVFrameSideData *sd);
#endif
