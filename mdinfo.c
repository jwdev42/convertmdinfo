/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "errors.h"
#include "wrappers.h"
#include "mdinfo.h"

disp_meta_ffmpeg* disp_meta_ffmpeg_alloc() {
	disp_meta_ffmpeg* meta = md_malloc(sizeof(disp_meta_ffmpeg));
	meta->r = NULL;
	meta->g = NULL;
	meta->b = NULL;
	meta->wp = NULL;
	return meta;
}

void disp_meta_ffmpeg_free(disp_meta_ffmpeg* meta) {
	if (meta->r != NULL)
		free(meta->r);
	if (meta->g != NULL)
		free(meta->g);
	if (meta->b != NULL)
		free(meta->b);
	if (meta->wp != NULL)
		free(meta->wp);
	free(meta);
}

void disp_meta_ffmpeg_verify(disp_meta_ffmpeg* meta) {
	if (meta->r == NULL)
		md_error_custom("Red channel not set for master display");
	else if (meta->g == NULL)
		md_error_custom("Green channel not set for master display");
	else if (meta->b == NULL)
		md_error_custom("Blue channel not set for master display");
	else if (meta->wp == NULL)
		md_error_custom("White point not set for master display");
}

disp_lum_ffmpeg* disp_lum_ffmpeg_alloc() {
	disp_lum_ffmpeg* lum = md_malloc(sizeof(disp_lum_ffmpeg));
	lum->min = -666;
	lum->max = -666;
	return lum;
}

void disp_lum_ffmpeg_free(disp_lum_ffmpeg* lum) {
	free(lum);
}

void disp_lum_ffmpeg_verify(disp_lum_ffmpeg* lum) {
	if (lum->min < 0)
		md_error_custom("Minimum luminance value not set for master display");
	else if (lum->max < 0)
		md_error_custom("Maximum luminance value not set for master display");
	else if (lum->max < lum->min)
	md_error_custom("Minimum luminance cannot be greater than maximum luminance");
}

disp_meta_x265* disp_meta_x265_alloc() {
	disp_meta_x265* meta = md_malloc(sizeof(disp_meta_x265));
	meta->r = NULL;
	meta->g = NULL;
	meta->b = NULL;
	meta->wp = NULL;
	meta->min_luminance = 0;
	meta->max_luminance = 0;
	return meta;
}

void disp_meta_x265_free(disp_meta_x265* meta) {
	if (meta->r != NULL)
		free(meta->r);
	if (meta->g != NULL)
		free(meta->g);
	if (meta->b != NULL)
		free(meta->b);
	if (meta->wp != NULL)
		free(meta->wp);
	free(meta);
}

/* Helper function for ffmpeg_to_x265. Check global_md_error after using this function */
point_x265* point_to_point_x265(const point* p) {
	const double divisor = 0.00002;
	clear_global_md_error();
	point_x265* ip = NULL;
	int bigx = (int) (p->x / divisor + 0.5);
	int bigy = (int) (p->y / divisor + 0.5);
	if (bigx < 0 || bigx > UINT16_MAX || bigy < 0 || bigy > UINT16_MAX) {
		global_md_error = ERR_OUTOFRANGE;
		return NULL;
	} else {
		ip = md_malloc(sizeof(point_x265));
		ip->x = (uint16_t) bigx;
		ip->y = (uint16_t) bigy;
	}
	return ip;
}

/* Helper function for ffmpeg_to_x265. Check global_md_error after using this function */
uint32_t lum_to_x265(double lum) {
	const double divisor = 0.0001;
	clear_global_md_error();
	int64_t big = (int64_t) (lum / divisor + 0.5);
	if (big < 0 || big > UINT32_MAX) {
		global_md_error = ERR_OUTOFRANGE;
		return 0;
	}
	return (uint32_t)big;
}

disp_meta_x265* ffmpeg_to_x265(disp_meta_ffmpeg* ffmpeg_disp, disp_lum_ffmpeg* ffmpeg_lum) {
	disp_meta_x265* meta = disp_meta_x265_alloc();
	meta->r = point_to_point_x265(ffmpeg_disp->r);
	if (global_md_error != ERR_NONE)
		return NULL;
	meta->g = point_to_point_x265(ffmpeg_disp->g);
	if (global_md_error != ERR_NONE)
		return NULL;
	meta->b = point_to_point_x265(ffmpeg_disp->b);
	if (global_md_error != ERR_NONE)
		return NULL;
	meta->wp = point_to_point_x265(ffmpeg_disp->wp);
	if (global_md_error != ERR_NONE)
		return NULL;
	meta->min_luminance = lum_to_x265(ffmpeg_lum->min);
	if (global_md_error != ERR_NONE)
		return NULL;
	meta->max_luminance = lum_to_x265(ffmpeg_lum->max);
	if (global_md_error != ERR_NONE)
		return NULL;
	return meta;
}

char* x265_str(disp_meta_x265* meta) {
	/* longest producable string "G(12345,12345)B(12345,12345)R(12345,12345)WP(12345,12345)L(1234567890,1234567890)"
	 * has 81 characters */
	char* str = md_malloc(82);
	sprintf(str, "G(%u,%u)B(%u,%u)R(%u,%u)WP(%u,%u)L(%u,%u)",
		meta->g->x, meta->g->y, meta->b->x, meta->b->y, meta->r->x, meta->r->y,
		meta->wp->x, meta->wp->y, meta->max_luminance, meta->min_luminance);
	return str;
}
