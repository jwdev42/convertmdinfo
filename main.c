/* This file is part of convertmdinfo, (c) 2021 Joerg Walter */

#include <stdio.h>
#include <stdlib.h>
#include "cmdline.h"
#include "eval.h"
#include "mdinfo.h"
#include "errors.h"

static void print_usage() {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "convertmdinfo -r %%d %%d -g %%d %%d -b %%d %%d -wp %%d %%d -lmin %%d -lmax %%d\n");
}

int main(int argc, char** argv) {
	/* parse command line */
	cmdline_switch* sw = cmdline_parse(argc, argv);
	exit_on_error();
	if (sw == NULL) {
		print_usage();
		return 0;
	}
	
	/* evaluate command line */
	disp_meta* meta = disp_meta_alloc();
	disp_lum* lum = disp_lum_alloc();
	eval_cmdline(sw, meta, lum);
	exit_on_error();
	
	/* free command line parser memory */
	cmdline_free(sw);
	
	/*verify command line input */
	disp_meta_verify(meta);
	exit_on_error();
	disp_lum_verify(lum);
	exit_on_error();
	
	/*convert & print */
	disp_meta_x265* meta_x265 = meta_to_x265(meta, lum);
	exit_on_error();
	char* display_str = x265_str(meta_x265);
	printf("%s\n", display_str);
	
	/* cleanup */
	free(display_str);
	disp_meta_x265_free(meta_x265);
	disp_lum_free(lum);
	disp_meta_free(meta);
	return 0;
}
