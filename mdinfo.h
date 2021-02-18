/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#ifndef _INCL_MDINFO
#define _INCL_MDINFO

#include <stdint.h>

typedef struct point {
	double x;
	double y;
} point;

typedef struct point_x265 {
	uint16_t x;
	uint16_t y;
} point_x265;

typedef struct disp_meta_ffmpeg {
	point* r;
	point* g;
	point* b;
	point* wp;
} disp_meta_ffmpeg;

/*constructor for disp_meta_ffmpeg*/
disp_meta_ffmpeg* disp_meta_ffmpeg_alloc();

/*destructor for disp_meta_ffmpeg*/
void disp_meta_ffmpeg_free(disp_meta_ffmpeg* meta);

/*sets an error if the struct is incomplete*/
void disp_meta_ffmpeg_verify(disp_meta_ffmpeg* meta);

typedef struct disp_lum_ffmpeg {
	double min;
	double max;
} disp_lum_ffmpeg;

/*constructor for disp_lum_ffmpeg*/
disp_lum_ffmpeg* disp_lum_ffmpeg_alloc();

/*destructor for disp_lum_ffmpeg*/
void disp_lum_ffmpeg_free(disp_lum_ffmpeg* lum);

/*sets an error if the struct is incomplete or invalid*/
void disp_lum_ffmpeg_verify(disp_lum_ffmpeg* lum);

typedef struct disp_meta_x265 {
	point_x265* r;
	point_x265* g;
	point_x265* b;
	point_x265* wp;
	uint32_t min_luminance;
	uint32_t max_luminance;
} disp_meta_x265;

/*constructor for disp_meta_x265*/
disp_meta_x265* disp_meta_x265_alloc();

/*destructor for disp_meta_x265*/
void disp_meta_x265_free(disp_meta_x265* meta);

/* ffmpeg_to_x265 converts ffmpegs representation of hdr display metadata to a format that can be used with x265.
 * In case of an error the function returns NULL and sets global_md_error to something else than ERR_NONE.
 * If the returned pointer is not NULL it must be freed if it is not needed anymore. */
disp_meta_x265* ffmpeg_to_x265(disp_meta_ffmpeg* ffmpeg_disp, disp_lum_ffmpeg* ffmpeg_lum);

/* x265_str produces a string that is usable with x265's "--master-display" command line option.
 * The returned string must be freed after being used. */
char* x265_str(disp_meta_x265* meta);
#endif
