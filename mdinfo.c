/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "errors.h"
#include "mdinfo.h"

/* Helper function for ffmpeg_to_x265. Check global_md_error after using this function */
point_x265 point_to_point_x265(point p) {
	const double divisor = 0.00002;
	clear_global_md_error();
	point_x265 ip;
	int bigx = (int) (p.x / divisor);
	int bigy = (int) (p.y / divisor);
	if (bigx < 0 || bigx > UINT16_MAX || bigy < 0 || bigy > UINT16_MAX) {
		global_md_error = ERR_OUTOFRANGE;
		ip.x = 0;
		ip.y = 0;
	} else {
		ip.x = (uint16_t) bigx;
		ip.y = (uint16_t) bigy;
	}
	return ip;
}

/* Helper function for ffmpeg_to_x265. Check global_md_error after using this function */
uint32_t lum_to_x265(double lum) {
	const double divisor = 0.00002;
	clear_global_md_error();
	int64_t big = (int64_t) (lum / divisor);
	if (big < 0 || big > UINT32_MAX) {
		global_md_error = ERR_OUTOFRANGE;
		return 0;
	}
	return (uint32_t)big;
}

disp_meta_x265* ffmpeg_to_x265(disp_meta_ffmpeg* ffmpeg_disp, disp_lum_ffmpeg* ffmpeg_lum) {
	disp_meta_x265* meta = malloc(sizeof(disp_meta_x265));
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
	char* str = malloc(82);
	sprintf(str, "G(%u,%u)B(%u,%u)R(%u,%u)WP(%u,%u)L(%u,%u)",
		meta->g.x, meta->g.y, meta->b.x, meta->b.y, meta->r.x, meta->r.y,
		meta->wp.x, meta->wp.y, meta->max_luminance, meta->min_luminance);
	return str;
}
