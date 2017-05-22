/**
 * alien-console: splash screen
 * Copyright (c) 2017 Stephen Brennan. Released under the Revised BSD License.
 *
 * This is intended to display ASCII art along with a progress bar. Expecting
 * me to handle tabs or anything more complex than spaces and printable
 * characters is just nonsense.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ncurses.h>

#include "alien-console.h"

/* Splash layout -------------------------------------------------------
   [ at least a line of blank space at top]
   SPLASH CONTENTS SPLASH CONTENTS SPLASH CONTENTS SPLASH CONTENTS
   TAGLINE ALIGNED @ RIGHT OF SPLASH CONTENTS
   [ at least a line of blank space in here ]
   ######### progress bar ######
   COPYRIGHT LINE (centered)
   * ------------------------------------------------------------------
   */
struct splash_layout {
	int maxx, maxy; /* from ncurses */
	int splash_lines;
	int splash_width;
	int top; /* amount of blank space at the top */
	int bottom; /* amount of blank space below splash */
	int splash_start_x; /* where to start printing splash */
	int tagline_start_x; /* where to start printing tagline */
	int copyright_start_x; /* where to start printing copyright */
};

/* presumably a splash should have <= 12 lines with <= 80 characters / line */
char splash_contents[1024];

/**
 * Returns the number of characters in the longest line of splash. Does not
 * handle things like tabs or other nonsense. splash must be NUL-terminated.
 */
static int max_line_width(char *splash)
{
	int max_line_width = 0;
	int line_width = 0;
	for (; *splash != '\0'; splash++) {
		if (*splash == '\n') {
			if (line_width > max_line_width)
				max_line_width = line_width;
			line_width = 0;
		} else {
			line_width++;
		}
	}
	return max_line_width;
}

/**
 * Returns the number of lines in splash.
 */
int count_lines(char *splash)
{
	int lines = 1;
	for (; *splash != '\0'; splash++) {
		if (*splash == '\n')
			lines++;
	}
	return lines;
}

/**
 * Sleep for a short time between printing progress bar.
 */
static void splash_sleep(int maxx)
{
	/* sleep for ~5 seconds total during load */
	double nsec_per_tick = (5.0 / maxx) * 1000000000;
	struct timespec slp;
	int millis_fuzz = rand() % 20 - 10;
	slp.tv_sec = 0;
	slp.tv_nsec = (unsigned long) nsec_per_tick;
	slp.tv_nsec += 1000000 * millis_fuzz;
	nanosleep(&slp, NULL);
}

/**
 * Loads the splash file into the statically allocated splash buffer.
 */
static int splash_load(char *filename)
{
	FILE *splash_file;
	size_t bytes;

	splash_file = fopen(filename, "r");
	if (!splash_file) {
		set_error(ESYS);
		return -1;
	}

	bytes = fread(splash_contents, 1, sizeof(splash_contents), splash_file);
	if (bytes >= sizeof(splash_contents) - 1) {
		set_error(EBIGFILE);
		return -1;
	}
	if (ferror(splash_file)) {
		set_error(ESYS);
		return -1;
	}
	fclose(splash_file);

	/* ensure we have a trailing newline and null terminator */
	if (splash_contents[bytes-1] != '\n') {
		splash_contents[bytes++] = '\n';
	}
	splash_contents[bytes] = '\0';

	return 0;
}

static int splash_compute_layout(const struct splash_params *params,
                                 struct splash_layout *layout)
{
	layout->splash_lines = count_lines(splash_contents);
	layout->splash_width = max_line_width(splash_contents);
	getmaxyx(stdscr, layout->maxy, layout->maxx);

	if (layout->splash_lines + 5 > layout->maxy) {
		set_error(ESHORT);
		return -1;
	}
	if (layout->splash_width > layout->maxx) {
		set_error(ENARROW);
		return -1;
	}
	if ((int)strlen(params->copyright) > layout->maxx) {
		set_error(EBIGTEXT);
	}

	layout->top = layout->maxy - (layout->splash_lines + 3);
	layout->bottom = layout->top / 2;
	layout->top = layout->top - layout->bottom;

	layout->splash_start_x = (layout->maxx - layout->splash_width) / 2;
	int splash_end_x = layout->splash_start_x + layout->splash_width;
	layout->tagline_start_x = splash_end_x - strlen(params->tagline);
	if (layout->tagline_start_x < 0) {
		set_error(EBIGTEXT);
		return -1;
	}

	layout->copyright_start_x = (layout->maxx - strlen(params->copyright))/2;
	return 0;
}

void splash_display(const struct splash_params *params,
                    const struct splash_layout *layout)
{
	int line_length, y, x;
	char *splash_ptr = splash_contents;
	y = layout->top;
	while (*splash_ptr != '\0') {
		line_length = strchr(splash_ptr, '\n') - splash_ptr;
		mvaddnstr(y, layout->splash_start_x, splash_ptr, line_length);
		y++;
		splash_ptr += line_length + 1;
	}
	y = layout->top + layout->splash_lines; /* just in case */
	mvaddstr(y, layout->tagline_start_x, params->tagline);
	y += 2 + layout->bottom;
	mvaddstr(y, layout->copyright_start_x, params->copyright);
	move(y - 1, 0);
	x = 0;
	refresh();
	while (x < layout->maxx) {
		splash_sleep(layout->maxx);
		addch(' ' | A_REVERSE);
		x++;
		refresh();
	}
	splash_sleep(layout->maxx); /* for good measure */
}

int splash(const struct splash_params *params)
{
	struct splash_layout layout;

	if (splash_load(params->filename) < 0) {
		mark_error();
		return -1;
	}

	if (splash_compute_layout(params, &layout) < 0) {
		mark_error();
		return -1;
	}

	splash_display(params, &layout);
	return 0;
}
