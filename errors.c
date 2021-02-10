/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#include "errors.h"

global_md_errors global_md_error = ERR_NONE;

const char* global_errors_str(global_md_errors err) {
	switch(err) {
		case ERR_NONE:
			return "No error";
		case ERR_OUTOFRANGE:
			return "Number out of range";
	}
	return "Undefined error";
}

void clear_global_md_error() {
	global_md_error = ERR_NONE;
}
