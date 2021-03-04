/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */

#include "ffmpeg.h"
#include "errors.h"
#include "mdinfo.h"
#include "wrappers.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/mastering_display_metadata.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FRAMES 24

typedef struct ffbucket {
	AVFormatContext *fmt_ctx;
	AVCodecContext *dec_ctx;
	AVCodec *decoder;
	AVFrame *frame;
	AVPacket *pkt;
} ffbucket;

static ffbucket *ffbucket_alloc() {
	ffbucket *bucket = md_malloc(sizeof(ffbucket));
	bucket->fmt_ctx = NULL;
	bucket->dec_ctx = NULL;
	bucket->decoder = NULL;
	bucket->frame = NULL;
	bucket->pkt = NULL;
	return bucket;
}

static void ffbucket_free(ffbucket *bucket) {
	if (bucket->pkt) {
		av_packet_unref(bucket->pkt);
		av_packet_free(&bucket->pkt);
	} if (bucket->frame)
		av_frame_free(&bucket->frame);
	if (bucket->dec_ctx)
		avcodec_free_context(&bucket->dec_ctx);
	if (bucket->fmt_ctx)
		avformat_close_input(&bucket->fmt_ctx);
	free(bucket);
}

static point *conv_meta_point(AVMasteringDisplayMetadata *ffmeta, int index) {
    if (index > 2 || index < 0) /* prevent index out of range */
        md_bug(__FILE__, __LINE__, true);
    point *p = md_malloc(sizeof(point));
    p->x = av_q2d(ffmeta->display_primaries[index][0]);
    p->y = av_q2d(ffmeta->display_primaries[index][1]);
    return p;
}

static void conv_meta(disp_meta *meta, AVMasteringDisplayMetadata *ffmeta) {
    if (!ffmeta->has_primaries)
        /* values in ffmeta must be set */
        md_bug(__FILE__, __LINE__, true);
    meta->r = conv_meta_point(ffmeta, 0); /* red channel */
    meta->g = conv_meta_point(ffmeta, 1); /* green channel */
    meta->b = conv_meta_point(ffmeta, 2); /* blue channel */

    /* white point */
    meta->wp = md_malloc(sizeof(point));
    meta->wp->x = av_q2d(ffmeta->white_point[0]);
    meta->wp->y = av_q2d(ffmeta->white_point[1]);
}

static void conv_lum(disp_lum *lum, AVMasteringDisplayMetadata *ffmeta) {
    if (!ffmeta->has_luminance)
        /* values in ffmeta must be set */
        md_bug(__FILE__, __LINE__, true);
    lum->min = av_q2d(ffmeta->min_luminance);
    lum->max = av_q2d(ffmeta->max_luminance);
}

int ffmpeg_recv_meta(const char *path, disp_meta *meta, disp_lum *lum) {
    /* initialize */
    ffbucket *bucket = ffbucket_alloc();

    /* enable for debugging */
    // av_log_set_level(AV_LOG_DEBUG);

    /* open file */
    if (avformat_open_input(&bucket->fmt_ctx, path, NULL, NULL) != 0) {
        md_error_custom("ffmpeg could not open file");
        return -1;
    }

    if (avformat_find_stream_info(bucket->fmt_ctx, NULL) < 0) {
        md_error_custom("ffmpeg could not retreive stream info");
        return -1;
    }

    /* find video stream */
    int video_id =
        av_find_best_stream(bucket->fmt_ctx, AVMEDIA_TYPE_VIDEO, 0, -1, NULL, 0);
    if (video_id < 0) {
        md_error_custom("No video stream in input file");
        return -1;
    }

    /* verify hevc codec */
    AVCodecParameters *codec_par = bucket->fmt_ctx->streams[video_id]->codecpar;
    if (codec_par->codec_id != AV_CODEC_ID_HEVC) {
        md_error_custom("Video stream in input file is not an HEVC stream");
        return -1;
    }

    bucket->decoder = avcodec_find_decoder(codec_par->codec_id);
    if (bucket->decoder == NULL) {
        md_error_custom("Could not open ffmpeg HEVC decoder");
        return -1;
    }

    bucket->dec_ctx = avcodec_alloc_context3(bucket->decoder);
    /* copy stream header information to codec context */
    if (avcodec_parameters_to_context(bucket->dec_ctx, codec_par) < 0) {
        md_error_custom("Could not copy codec parameters to codec context");
        return -1;
    }
    /* open AVCodecContext */
    if (avcodec_open2(bucket->dec_ctx, bucket->decoder, NULL) < 0) {
        md_error_custom("Could not initialize ffmpeg AVCodecContext");
        return -1;
    }
    
    bucket->pkt = av_packet_alloc();
    av_init_packet(bucket->pkt);
    bucket->pkt->data = NULL;
    bucket->pkt->size = 0;
    bucket->pkt->stream_index = video_id;
    bucket->frame = av_frame_alloc();
    bool found = false;
    int fc = 0; /* frame counter */
    while (true) {
        if (av_read_frame(bucket->fmt_ctx, bucket->pkt) < 0 || fc >= MAX_FRAMES) {
            break; /* end of stream or error*/
        }
        if (bucket->pkt->stream_index != video_id) {
            av_packet_unref(bucket->pkt);
            continue;
        }
        fc++;
        /* send packet to decoder */
        int send_status = avcodec_send_packet(bucket->dec_ctx, bucket->pkt);
        if (send_status != 0) {
            if (send_status == AVERROR(ENOMEM))
                md_bug(__FILE__, __LINE__, true);
            else if (send_status == AVERROR(EINVAL))
                md_bug(__FILE__, __LINE__, true);
            else if (send_status == AVERROR(EAGAIN))
                md_bug(__FILE__, __LINE__, true);
            else if (send_status == AVERROR_EOF)
                md_bug(__FILE__, __LINE__, true);
            else {
                /* legitimate decoding error */
                md_error_custom("avcodec_send_packet returned decoding error");
                return -1;
            }
        }
        while (true) {
            int frame_status = avcodec_receive_frame(bucket->dec_ctx, bucket->frame);
            if (frame_status != 0) {
                if (frame_status == AVERROR(EAGAIN)) {
                    if (send_status == AVERROR(EAGAIN)) {
                        /* this condition leads to undefined behaviour in ffmpeg
                         * if not catched */
                        md_error_custom(
                            "avcodec_send_packet and avcodec_receive_frame "
                            "both returned EAGAIN");
                        return -1;
                    }
                    break;
                } else if (frame_status == AVERROR_EOF)
                    break;
                else if (frame_status < 0) {
                    md_error_custom(
                        "avcodec_receive_frame returned decoding error");
                    return -1;
                } else
                    md_bug(__FILE__, __LINE__, true);
            }
            for (int i = 0; i < bucket->frame->nb_side_data; i++) {
                AVFrameSideData *sd = bucket->frame->side_data[i];
                if (sd->type == AV_FRAME_DATA_MASTERING_DISPLAY_METADATA) {
                    /* these are the droids we are looking for */
                    AVMasteringDisplayMetadata *ffmeta =
                        (AVMasteringDisplayMetadata *)sd->data;
                    if (ffmeta->has_primaries && ffmeta->has_luminance) {
                        conv_meta(meta, ffmeta);
                        conv_lum(lum, ffmeta);
                        found = true;
                        break;
                    }
                }
            }
            if (found)
                break;
            av_frame_unref(bucket->frame);
        }
        if (found)
            break;
        av_packet_unref(bucket->pkt);
        av_init_packet(bucket->pkt);
    }
    ffbucket_free(bucket);
    if (!found) {
        md_error_custom("Video does not contain mastering display metadata");
        return -1;
    }
    return 0;
}
