/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */

#include <stdio.h>
#include <string.h>
#include "errors.h"
#include "wrappers.h"
#include "cmdline.h"

/* forward declarations */
static cmdline_switch* parse_switch(cmdline_switch* sw_first, int pos, int argc, char** argv);
static int parse_arguments(cmdline_switch* sw, int pos, int argc, char** argv);
static cmdline_switch* find_switch(cmdline_switch* sw, const char* s_search);

cmdline_switch* cmdline_parse(int argc, char** argv) {
	return parse_switch(NULL, 1, argc, argv);
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

static cmdline_switch* parse_switch(cmdline_switch* sw_first, int pos, int argc, char** argv) {
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
	
	/* backtrack to detect duplicate switches */
	if (find_switch(sw_first, argv[pos]) != NULL) {
		const size_t bufsize = 128;
		char* buf = md_malloc(bufsize);
		snprintf(buf, bufsize, "Command line parser: Duplicate command line switch \"%s\"", argv[pos]);
		md_error_custom(buf);
		return NULL;
	}
	
	cmdline_switch* sw = md_malloc(sizeof(cmdline_switch));
	sw->id = argv[pos];
	parse_arguments(sw, pos + 1, argc, argv);
	if (sw_first == NULL) {
		sw_first = sw;
	}
	sw->next = parse_switch(sw_first, pos + 1 + sw->argc, argc, argv);
	return sw;
}

/* searches for a particular command line switch and returns its object if found */
static cmdline_switch* find_switch(cmdline_switch* sw, const char* s_search) {
	if (sw == NULL)
		return NULL; //base case
	if (!strcmp(sw->id, s_search))
		return sw; //found match
	return find_switch(sw->next, s_search);
}
