/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#ifndef _INCL_ERRORS
#define _INCL_ERRORS

#include <stdbool.h>

typedef enum { ERR_NONE=0, ERR_OUTOFRANGE, ERR_INPUT, ERR_CUSTOM } md_error_t;

/* program-wide error variable */
extern md_error_t global_md_error;

/* global_md_error_str returns an error message for the given error */
const char* global_md_error_str(md_error_t err);

/* clear_global_md_error sets error value to indicate no error */
void clear_global_md_error();

/* set an error with custom message msg */
void md_error_custom(const char* msg);

/* if an error is set, exit_on_error will print the error message to stderr and exit the program */
void exit_on_error();

/* report a bug in line line of source file file, quit if quit is true. Roger Roger! */
void md_bug(const char* file, int line, bool quit);

#endif
