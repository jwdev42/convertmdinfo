/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#include "wrappers.h"
#include <stdio.h>
#include <string.h>

#define ERROR_OOM "FATAL: Memory allocation error.\n"

/* memfail prints an error message that no memory could be allocated and exits.
 */
static void memfail() {
    fprintf(stderr, ERROR_OOM);
    exit(EXIT_FAILURE);
}

/* wrapper for malloc that exits the program on error. */
void *md_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL)
        memfail();
    return ptr;
}

void *md_calloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL)
        memfail();
    return ptr;
}

/* wrapper for realloc that exits the program on error. */
void *md_realloc(void *ptr, size_t size) {
    void *ret_ptr = realloc(ptr, size);
    if (ret_ptr == NULL)
        memfail();
    return ret_ptr;
}

/* reimplementation of strdup since it is not a standard function. Also wraps
 * malloc. */
char *md_strdup(const char *str) {
    char *newstr = md_malloc(strlen(str) + 1);
    strcpy(newstr, str);
    return newstr;
}
