/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdline.h"
#include "mdinfo.h"
#include "errors.h"
#include "wrappers.h"
#include "eval.h"
#include "ffmpeg.h"

/* returns true if the input string represents a floating-point value of zero */
static bool is_zero(const char* input) {
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
				if (input[i -1] != '0' || input[i+1] != '0')
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

static double parse_double(const char* input) {
	double d = atof(input);
	if (d == 0 && !is_zero(input)) { /* input could not be parsed as floating-point value */
		size_t bufsize = 17 + strlen(input) + 1;
		char* buf = md_malloc(bufsize);
		snprintf(buf, bufsize, "Invalid decimal: %s", input);
		md_error_custom(buf);
	}
	return d;
}

static point* eval_point(char** input, int elements) {
	
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
	point* p = md_malloc(sizeof(point));
	p->x = x;
	p->y = y;
	return p;
}

static double eval_lum(char** input, int elements) {
	if (elements != 1) {
		global_md_error = ERR_INPUT;
		return 0;
	}
	return parse_double(input[0]);
}

static disp_meta* eval_ffmpeg(char** input, int elements) {
	if (elements != 1) {
		global_md_error = ERR_INPUT;
		return NULL;
	}
	return ffmpeg_recv_meta(input[0]);
}

disp_meta* eval_cmdline(cmdline_switch* sw, disp_meta* meta, disp_lum* lum) {
	if (sw == NULL)
		return meta; //base case
	if (!strcmp("-r", sw->id))
		meta->r = eval_point(sw->args, sw->argc);
	else if (!strcmp("-g", sw->id))
		meta->g = eval_point(sw->args, sw->argc);
	else if (!strcmp("-b", sw->id))
		meta->b = eval_point(sw->args, sw->argc);
	else if (!strcmp("-wp", sw->id))
		meta->wp = eval_point(sw->args, sw->argc);
	else if (!strcmp("-lmin", sw->id))
		lum->min = eval_lum(sw->args, sw->argc);
	else if (!strcmp("-lmax", sw->id))
		lum->max = eval_lum(sw->args, sw->argc);
	else if (!strcmp("-i", sw->id))
		return eval_ffmpeg(sw->args, sw->argc);
	if (global_md_error != ERR_NONE)
		return NULL;
	return eval_cmdline(sw->next, meta, lum);
}
