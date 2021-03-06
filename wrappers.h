/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#ifndef _INCL_WRAPPERS
#define _INCL_WRAPPERS

#include <stdlib.h>

void *md_malloc(size_t size);
void *md_calloc(size_t nmemb, size_t size);
void *md_realloc(void *ptr, size_t size);
char *md_strdup(const char *s);

#endif
