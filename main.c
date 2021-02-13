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
	disp_meta_ffmpeg meta_ff;
	disp_lum_ffmpeg lum;
	eval_cmdline(sw, &meta_ff, &lum);
	exit_on_error();
	
	disp_meta_x265* meta_x265 = ffmpeg_to_x265(&meta_ff, &lum);
	exit_on_error();
	char* display_str = x265_str(meta_x265);
	printf("%s\n", display_str);
	free(display_str);
	free(meta_x265);
	return 0;
}
