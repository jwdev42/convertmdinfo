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
    }
    if (bucket->frame)
        av_frame_free(&bucket->frame);
    if (bucket->dec_ctx)
        avcodec_free_context(&bucket->dec_ctx);
    if (bucket->fmt_ctx)
        avformat_close_input(&bucket->fmt_ctx);
    free(bucket);
}

static int fferror(ffbucket *bucket, const char *msg) {
    md_error_custom(msg);
    if (bucket)
        ffbucket_free(bucket);
    return -1;
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

int ffmpeg_access_sidedata(const char *path, FILE *ostream,
                           ff_return_t (*recv_func)(FILE *, AVFrameSideData *),
                           uint64_t frame_limit) {
    /* initialize */
    ffbucket *bucket = ffbucket_alloc();

    /* enable for debugging */
    // av_log_set_level(AV_LOG_DEBUG);

    /* open file */
    if (avformat_open_input(&bucket->fmt_ctx, path, NULL, NULL) != 0)
        return fferror(bucket, "ffmpeg could not open file");

    if (avformat_find_stream_info(bucket->fmt_ctx, NULL) < 0)
        return fferror(bucket, "ffmpeg could not retreive stream info");

    /* find video stream */
    int video_id = av_find_best_stream(bucket->fmt_ctx, AVMEDIA_TYPE_VIDEO, 0,
                                       -1, NULL, 0);
    if (video_id < 0)
        return fferror(bucket, "No video stream in input file");

    /* verify hevc codec */
    AVCodecParameters *codec_par = bucket->fmt_ctx->streams[video_id]->codecpar;
    if (codec_par->codec_id != AV_CODEC_ID_HEVC)
        return fferror(bucket,
                       "Video stream in input file is not an HEVC stream");

    bucket->decoder = avcodec_find_decoder(codec_par->codec_id);
    if (bucket->decoder == NULL)
        return fferror(bucket, "Could not open ffmpeg HEVC decoder");

    bucket->dec_ctx = avcodec_alloc_context3(bucket->decoder);
    /* copy stream header information to codec context */
    if (avcodec_parameters_to_context(bucket->dec_ctx, codec_par) < 0)
        return fferror(bucket,
                       "Could not copy codec parameters to codec context");

    /* open AVCodecContext */
    if (avcodec_open2(bucket->dec_ctx, bucket->decoder, NULL) < 0)
        return fferror(bucket, "Could not initialize ffmpeg AVCodecContext");

    bucket->pkt = av_packet_alloc();
    av_init_packet(bucket->pkt);
    bucket->pkt->data = NULL;
    bucket->pkt->size = 0;
    bucket->pkt->stream_index = video_id;
    bucket->frame = av_frame_alloc();
    bool ffabort = false;
    uint64_t fc = 0; /* frame counter */
    while (true) {
        if (av_read_frame(bucket->fmt_ctx, bucket->pkt) < 0) {
            break; /* end of stream or error */
        }
        if (frame_limit > 0) {
            if (fc >= frame_limit)
                break; /* frame limit reached */
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
                return fferror(bucket,
                               "avcodec_send_packet returned decoding error");
            }
        }
        while (true) {
            int frame_status =
                avcodec_receive_frame(bucket->dec_ctx, bucket->frame);
            if (frame_status != 0) {
                if (frame_status == AVERROR(EAGAIN)) {
                    if (send_status == AVERROR(EAGAIN)) {
                        /* this condition leads to undefined behaviour in ffmpeg
                         * if not catched */
                        return fferror(
                            bucket,
                            "avcodec_send_packet and avcodec_receive_frame "
                            "both returned EAGAIN");
                    }
                    break;
                } else if (frame_status == AVERROR_EOF)
                    break;
                else if (frame_status < 0) {
                    return fferror(
                        bucket,
                        "avcodec_receive_frame returned decoding error");
                    return -1;
                } else
                    md_bug(__FILE__, __LINE__, true);
            }
            for (int i = 0; i < bucket->frame->nb_side_data; i++) {
                AVFrameSideData *sd = bucket->frame->side_data[i];
                ff_return_t ret = recv_func(ostream, sd);
                if (ret == FFRET_ERROR || ret == FFRET_DONE) {
                    ffabort = true;
                    break;
                } else if (ret == FFRET_BREAK)
                    break;
            }
            if (ffabort)
                break;
            av_frame_unref(bucket->frame);
        }
        if (ffabort)
            break;
        av_packet_unref(bucket->pkt);
        av_init_packet(bucket->pkt);
    }

    if (!ffabort)
        return fferror(bucket,
                       "Video stream does not contain the desired side data");

    ffbucket_free(bucket);
    return 0;
}

ff_return_t ffmpeg_disp_meta(FILE *ostream, AVFrameSideData *sd) {
    if (sd->type == AV_FRAME_DATA_MASTERING_DISPLAY_METADATA) {
        /* these are the droids we are looking for */
        AVMasteringDisplayMetadata *ffmeta =
            (AVMasteringDisplayMetadata *)sd->data;
        if (ffmeta->has_primaries && ffmeta->has_luminance) {
            disp_meta *col = disp_meta_alloc();
            disp_lum *lum = disp_lum_alloc();
            conv_meta(col, ffmeta);
            conv_lum(lum, ffmeta);
            disp_meta_x265 *x265 = meta_to_x265(col, lum);
            if (x265 == NULL) {
                free(col);
                free(lum);
                return FFRET_ERROR;
            }
            fprintf(ostream, "%s\n", x265_str(x265));
            free(x265);
            free(col);
            free(lum);
            return FFRET_DONE;
        } else {
            md_error_custom("Incomplete mastering display metadata");
            return FFRET_ERROR;
        }
    }
    return FFRET_CONTINUE;
}
