/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */
#ifndef _INCL_CMDLINE
#define _INCL_CMDLINE

typedef struct cmdline_switch {
	char* id;
	char** args;
	int argc;
	struct cmdline_switch* next;
} cmdline_switch;

cmdline_switch* cmdline_parse(int argc, char **argv);

#endif
