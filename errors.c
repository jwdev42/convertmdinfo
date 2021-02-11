/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#include <stddef.h>
#include "errors.h"

md_error_t global_md_error = ERR_NONE;
static const char* error_msg = NULL;

const char* global_md_error_str(md_error_t err) {
	switch(err) {
		case ERR_NONE:
			return "No error";
		case ERR_OUTOFRANGE:
			return "Number out of range";
		case ERR_CUSTOM:
			if (error_msg == NULL)
				return "Undefined custom error";
			return error_msg;
	}
	return "Undefined error";
}

void clear_global_md_error() {
	global_md_error = ERR_NONE;
	error_msg = NULL;
}

void md_error_custom(const char* msg) {
	global_md_error = ERR_CUSTOM;
	error_msg = msg;
}
