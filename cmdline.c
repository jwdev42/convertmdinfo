/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */

#include <stdio.h>
#include "errors.h"
#include "wrappers.h"
#include "cmdline.h"

/* forward declarations */
static cmdline_switch* parse_switch(int pos, int argc, char** argv);
static int parse_arguments(cmdline_switch* sw, int pos, int argc, char** argv);

cmdline_switch* cmdline_parse(int argc, char** argv) {
	return parse_switch(1, argc, argv);
}

static int parse_arguments(cmdline_switch* sw, int pos, int argc, char** argv) {
	char** arguments = NULL;
	int elements = 0;
	for(int i = pos; i < argc; i++) {
		if (argv[i][0] == '-')
			break; /* end of arguments */
		elements++;
		if (elements == 1) {
			arguments = md_malloc(sizeof(char*));
		} else {
			arguments = md_realloc(arguments, sizeof(char*) * elements);
		}
		arguments[elements - 1] = md_strdup(argv[i]);
	}
	sw->argc = elements;
	sw->args = arguments;
	return 0;
}

static cmdline_switch* parse_switch(int pos, int argc, char** argv) {
	if (pos >= argc) {
		return NULL; /* base case */
	}
	if (argv[pos][0] != '-') {
		const size_t bufsize = 64;
		char* buf = md_malloc(bufsize);
		snprintf(buf, bufsize, "Command line parser: Expected switch at token %d", pos);
		md_error_custom(buf);
		return NULL;
	}
	cmdline_switch* sw = md_malloc(sizeof(cmdline_switch));
	sw->id = argv[pos];
	parse_arguments(sw, pos + 1, argc, argv);
	sw->next = parse_switch(pos + 1 + sw->argc, argc, argv);
	return sw;
}
