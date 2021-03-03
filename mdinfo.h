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

typedef struct disp_meta {
    point *r;
    point *g;
    point *b;
    point *wp;
} disp_meta;

/*constructor for disp_meta*/
disp_meta *disp_meta_alloc();

/*destructor for disp_meta*/
void disp_meta_free(disp_meta *meta);

/*sets an error if the struct is incomplete*/
void disp_meta_verify(disp_meta *meta);

typedef struct disp_lum {
    double min;
    double max;
} disp_lum;

/*constructor for disp_lum*/
disp_lum *disp_lum_alloc();

/*destructor for disp_lum*/
void disp_lum_free(disp_lum *lum);

/*sets an error if the struct is incomplete or invalid*/
void disp_lum_verify(disp_lum *lum);

typedef struct disp_meta_x265 {
    point_x265 *r;
    point_x265 *g;
    point_x265 *b;
    point_x265 *wp;
    uint32_t min_luminance;
    uint32_t max_luminance;
} disp_meta_x265;

/*constructor for disp_meta_x265*/
disp_meta_x265 *disp_meta_x265_alloc();

/*destructor for disp_meta_x265*/
void disp_meta_x265_free(disp_meta_x265 *meta);

/* meta_to_x265 converts hdr display metadata to a format that can be used with
 * x265. In case of an error the function returns NULL and sets global_md_error
 * to something else than ERR_NONE.
 * If the returned pointer is not NULL it must be freed if it is not needed
 * anymore. */
disp_meta_x265 *meta_to_x265(disp_meta *meta, disp_lum *lum);

/* x265_str produces a string that is usable with x265's "--master-display"
 * command line option. The returned string must be freed after being used. */
char *x265_str(disp_meta_x265 *meta);
#endif
