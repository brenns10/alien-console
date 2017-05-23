/**
 * alien-console: a clone of the Alien: Isolation "personal terminal"
 * Copyright (c) 2017 Stephen Brennan. Released under the Revised BSD License.
 */
#include <stdlib.h>
#include <sys/stat.h>

#include <ncurses.h>

#include "alien-console.h"

char *config_file(int argc, char *argv[])
{
	struct stat s;
	if (argc >= 2) {
		return argv[1];
	} else if (stat(SYSTEM_CONFIG, &s) == 0) {
		return SYSTEM_CONFIG;
	} else if (stat(DEFAULT_CONFIG, &s) == 0) {
		return DEFAULT_CONFIG;
	}
	fprintf(stderr, "alien-console: no configuration file\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	int rv = 0;
	struct pt_params params;
	char *config;

	config = config_file(argc, argv);
	rv = parse_config(config, &params);
	if (rv < 0) {
		mark_error();
		goto exit;
	}

	/* ncurses initialization */
	initscr();            /* initialize curses */
	cbreak();             /* pass key presses to program, but not signals */
	noecho();             /* don't echo key presses to screen */
	keypad(stdscr, TRUE); /* allow arrow keys */
	timeout(-1);          /* block on getch() */
	curs_set(0);          /* set the cursor to invisible */


	rv = splash(&params.splash); /* display splash screen */
	if (rv < 0) {
		mark_error();
		goto cleanup;
	}

	rv = personal_terminal(&params); /* display pt (main loop) */
	if (rv < 0) {
		mark_error();
		goto cleanup;
	}

cleanup:
	cleanup_config(&params);

	// Deinitialize NCurses
	wclear(stdscr);
	endwin();
exit:
	if (rv < 0) {
		report_error(stderr);
		return rv;
	}
	return 0;
}
