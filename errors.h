/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#ifndef _INCL_ERRORS
#define _INCL_ERRORS

typedef enum { ERR_NONE=0, ERR_OUTOFRANGE } global_md_errors;

extern global_md_errors global_md_error;

const char* global_md_errors_str(int err);

void clear_global_md_error();

#endif
