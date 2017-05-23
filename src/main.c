/**
 * alien-console: a clone of the Alien: Isolation "personal terminal"
 * Copyright (c) 2017 Stephen Brennan. Released under the Revised BSD License.
 */
#include <ncurses.h>

#include "alien-console.h"

int main(int argc, char *argv[])
{
	int rv = 0;
	struct pt_params params;
	char *config;

	/* ncurses initialization */
	initscr();            /* initialize curses */
	cbreak();             /* pass key presses to program, but not signals */
	noecho();             /* don't echo key presses to screen */
	keypad(stdscr, TRUE); /* allow arrow keys */
	timeout(-1);          /* block on getch() */
	curs_set(0);          /* set the cursor to invisible */

	if (argc < 2) {
		config = "/etc/alien-console.conf";
	} else {
		config = argv[1];
	}
	rv = parse_config(config, &params);
	if (rv < 0) {
		mark_error();
		goto exit;
	}

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
exit:
	wclear(stdscr);
	endwin();
	if (rv < 0) {
		report_error(stderr);
		return rv;
	}
	return 0;
}
