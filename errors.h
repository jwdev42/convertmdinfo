/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#ifndef _INCL_ERRORS
#define _INCL_ERRORS

typedef enum { ERR_NONE=0, ERR_OUTOFRANGE } md_error_t;

/* program-wide error variable */
extern md_error_t global_md_error;

/* global_md_error_str returns an error message for the given error */
const char* global_md_error_str(md_error_t err);

/* clear_global_md_error sets error value to indicate no error */
void clear_global_md_error();

#endif
