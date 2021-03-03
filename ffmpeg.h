/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#ifndef _INCL_FFMPEG
#define _INCL_FFMPEG

#include "mdinfo.h"

int ffmpeg_recv_meta(const char* path, disp_meta* meta, disp_lum* lum);

#endif
