/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "mdinfo.h"
#include "errors.h"
#include "ffmpeg.h"

/*
static int find_hevc_stream(AVFormatContext* fmt_ctx) {
	AVCodec* dec = NULL;
	int id = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, 0, -1, &dec, 0);
}
*/

disp_meta* ffmpeg_recv_meta(const char* path) {
	/* initialize */
	av_register_all();
	AVFormatContext* fmt_ctx = NULL;
	
	/* open file */
	if (avformat_open_input(&fmt_ctx, path, NULL, NULL) != 0) {
		md_error_custom("ffmpeg could not open file");
		return NULL;
	}
	
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
		md_error_custom("ffmpeg could not retreive stream info");
		return NULL;
	}
	
	av_dump_format(fmt_ctx, 0, path, 0);
	
	disp_meta* meta = disp_meta_alloc();
	return meta;
}
