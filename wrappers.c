/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#include <stdio.h>
#include "wrappers.h"

#define ERROR_OOM "FATAL: Memory allocation error.\n"

/* wrapper for malloc that exits the program on error */
void* md_malloc(size_t size) {
	void* ptr = malloc(size);
	if (ptr == NULL) {
		fprintf(stderr, ERROR_OOM);
		exit(EXIT_FAILURE);
	}
	return ptr;
}

/* wrapper for realloc that exits the program on error */
void* md_realloc(void *ptr, size_t size) {
	void* ret_ptr = realloc(ptr, size);
	if (ptr == NULL) {
		fprintf(stderr, ERROR_OOM);
		exit(EXIT_FAILURE);
	}
	return ret_ptr;
}
