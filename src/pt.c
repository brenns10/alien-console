/**
 * alien-console: personal terminal screen
 * Copyright (C) 2017 Stephen Brennan. Released under the Revised BSD License.
 *
 * This module emulates the personal terminal screen of Alien Isolation. See
 * img/real-main.jpg for a screenshot of the original.
 *
 * +------------------------------------------------------------------------+
 * | PERSONAL TERMINAL                                                      |
 * |                                                                        |
 * |                          +-++-----------------------------------------+|
 * |                          | || CONTENT TITLE (window, boxed)           ||
 * | FOLDERS                  | |+-----------------------------------------+|
 * |+------------------------+| |                                           |
 * || FOLDER BOX #1          || |+-----------------------------------------+|
 * || 24x4 (incl. border)    || || CONTENT TEXT (window, boxed)            ||
 * |+------------------------+| ||                                         ||
 * |+------------------------+| ||
 *
 *   etc etc etc etc
 * +------------------------------------------------------------------------+
 *
 * The interfaces here are fairly simple: the folder boxes, the elbow connector,
 * the content title, and the content text all have windows. Each one has a
 * function which will redraw it and refresh it (but not doupdate()). The init
 * function creates all windows, draws all the static stuff, and uses each draw
 * function to draw everything for the first time. Then the loop function simply
 * waits for keypresses and calls corresponding functions. These guys just
 * update the state and then redraw only the things that changed.
 */
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include "alien-console.h"

#define Y_PERSONAL_TERMINAL 0
#define X_PERSONAL_TERMINAL 0

#define Y_FOLDERS 4
#define X_FOLDERS 1
#define Y_FOLDER_BOX 5
#define X_FOLDER_BOX 0
#define H_FOLDER_BOX 4
#define W_FOLDER_BOX 20
#define N_FOLDER_BOX 4

#define Y_ELBOW_WINDOW 2
#define X_ELBOW_WINDOW W_FOLDER_BOX
#define W_ELBOW_WINDOW 3
#define H_ELBOW_WINDOW (N_FOLDER_BOX * H_FOLDER_BOX + 3)

#define Y_CONTENT_TITLE 2
#define X_CONTENT_TITLE (W_FOLDER_BOX + W_ELBOW_WINDOW)
#define H_CONTENT_TITLE 3

#define Y_CONTENT_TEXT 6
#define X_CONTENT_TEXT X_CONTENT_TITLE
#define CONTENT_TEXT_MIN_WIDTH 40

#define MIN_HEIGHT (X_FOLDER_BOX + N_FOLDER_BOX * H_FOLDER_BOX + 1)
#define MIN_WIDTH (X_CONTENT_TEXT + CONTENT_TEXT_MIN_WIDTH)
/* bottom text is 59 characters, so we're limited by layout, not text */

struct folder_entry {
	char *folder;
	char *title;
	char *text;
	int lines;
};

struct personal_terminal {
	int maxy, maxx;
	WINDOW *content_title;
	WINDOW *content_text;
	WINDOW *elbow_box;
	WINDOW *folder_box[N_FOLDER_BOX];
	struct folder_entry folder_entries[N_FOLDER_BOX];
	unsigned int selected;
	unsigned int scroll;
};

/**
 * Insert newlines at spaces in the string so that lines are no longer than the
 * width of the box (subtracting 2 to account for the box lines).
 */
static int wrap_folder_entry(struct personal_terminal *pt,
                             struct folder_entry *entry)
{
	int maxy, maxx, width, i;
	int last_space = -1, line_length = 0;
	getmaxyx(pt->content_text, maxy, maxx);
	(void) maxy; /* unused */
	width = maxx - 2;

	for (i = 0; entry->text[i]; i++) {
		if (entry->text[i] == ' ') {
			line_length++;
			last_space = i;
		} else if (entry->text[i] == '\n') {
			line_length = 0;
			last_space = -1;
		} else {
			line_length++;
		}
		if (line_length > width) {
			if (last_space == -1) {
				set_error(EBIGTEXT);
				return -1;
			}
			entry->text[last_space] = '\n';
			line_length = i - last_space;
			last_space = -1;
		}
	}
	entry->lines = count_lines(entry->text);
	return 0;
}

/**
 * Draws the content box according to the state of the terminal. Hopefully,
 * you've wrapped the text already!
 */
static void draw_content_text(struct personal_terminal *pt)
{
	int maxy, maxx, nlines, i;
	unsigned int scroll = pt->scroll;
	char *str = pt->folder_entries[pt->selected].text;
	wclear(pt->content_text);
	box(pt->content_text, 0, 0);
	getmaxyx(pt->content_text, maxy, maxx);
	(void) maxx; /* unused */
	nlines = maxy - 2;

	/* move to line */
	while (scroll > 0 && str) {
		scroll--;
		str = strchr(str, '\n') + 1;
	}

	/* scrolled past content. try not to do this */
	if (!str) {
		wnoutrefresh(pt->content_text);
		return;
	}

	/* now do the printing */
	for (i = 0; i < nlines && str; i++) {
		char *newstr = strchr(str, '\n');
		if (!newstr) {
			/* no newline before end of string, but there could be
			 * more text, so let's print this string and then exit
			 */
			mvwaddstr(pt->content_text, 1 + i, 1, str);
			str = newstr;
		} else {
			/* the normal case */
			mvwaddnstr(pt->content_text, 1 + i, 1, str, newstr - str);
			str = newstr + 1;
		}
	}
	wnoutrefresh(pt->content_text);
	return;
}

/**
 * Draws the elbow box, which contains the little elbow connector that joins
 * the selected folder with the content title.
 */
static void draw_elbow_box(struct personal_terminal *pt)
{
	unsigned int i;
	wclear(pt->elbow_box);
	wattron(pt->elbow_box, A_BOLD);
	mvwaddch(pt->elbow_box, 1, 2, ACS_HLINE);
	mvwaddch(pt->elbow_box, 1, 1, ACS_ULCORNER);
	mvwaddch(pt->elbow_box, 2, 1, ACS_VLINE);
	for (i = 0; i < N_FOLDER_BOX * W_FOLDER_BOX; i++) {
		if (i == pt->selected * H_FOLDER_BOX + 1) {
			mvwaddch(pt->elbow_box, 3 + i, 1, ACS_LRCORNER);
			mvwaddch(pt->elbow_box, 3 + i, 0, ACS_HLINE);
			wattroff(pt->elbow_box, A_BOLD);
			wnoutrefresh(pt->elbow_box);
			return;
		} else {
			mvwaddch(pt->elbow_box, 3 + i, 1, ACS_VLINE);
		}
	}
	wnoutrefresh(pt->elbow_box);
}

/**
 * Draws the content title. Not much special here.
 */
static void draw_content_title(struct personal_terminal *pt)
{
	wclear(pt->content_title);
	box(pt->content_title, 0, 0);
	mvwaddch(pt->content_title, 1, 0, ACS_RTEE);
	mvwaddstr(pt->content_title, 1, 1,
	          pt->folder_entries[pt->selected].title);
	wnoutrefresh(pt->content_title);
}

/**
 * Draws the folder box. This never gets cleared, and as a result we can skip
 * the overhead of drawing the text each time, and simply do the outline. This
 * function will insert a connector for the elbow at the appropriate location.
 * It will also select the appropriate attributes for printing.
 */
static void draw_folder_box_outline(struct personal_terminal *pt,
                                    unsigned int i)
{
	int attr = (i == pt->selected ? A_BOLD : A_DIM);
	wattron(pt->folder_box[i], attr);
	box(pt->folder_box[i], 0, 0);
	if (pt->selected == i) {
		mvwaddch(pt->folder_box[i], 1, W_FOLDER_BOX - 1, ACS_LTEE);
	}
	wattroff(pt->folder_box[i], attr);
	wnoutrefresh(pt->folder_box[i]);
}

/**
 * Select a new folder, index i. If the folder is out of range, don't bother.
 * This will call the appropriate redrawing code.
 */
static void select_folder(struct personal_terminal *pt, int i)
{
	if (i < 0 || i >= N_FOLDER_BOX)
		return;


	unsigned int old = pt->selected;
	pt->selected = (unsigned int) i;
	pt->scroll = 0;

	draw_elbow_box(pt);
	draw_content_text(pt);
	draw_content_title(pt);
	draw_folder_box_outline(pt, old);
	draw_folder_box_outline(pt, pt->selected);
}

/**
 * Do a scroll up operation (if possible) and then redraw.
 */
static void scroll_up(struct personal_terminal *pt)
{
	if (pt->scroll <= 0)
		return;

	pt->scroll -= 1;
	draw_content_text(pt);
}

/**
 * Do a scroll down operation (if possible) and then redraw.
 */
static void scroll_down(struct personal_terminal *pt)
{
	int maxy, maxx, height;
	getmaxyx(pt->content_text, maxy, maxx);
	(void)maxx; /* unused */
	height = maxy - 2;
	if ((int)pt->scroll + height >= pt->folder_entries[pt->selected].lines) {
		return;
	}
	pt->scroll += 1;
	draw_content_text(pt);
}

/**
 * Initialize the curses resources and do a first draw of the personal terminal.
 */
static int init_personal_terminal(struct personal_terminal *pt)
{
	unsigned int i;

	clear();
	getmaxyx(stdscr, pt->maxy, pt->maxx);

	if (pt->maxy < MIN_HEIGHT) {
		set_error(ESHORT);
		return -1;
	} else if (pt->maxx < MIN_WIDTH) {
		set_error(ENARROW);
		return -1;
	}
	pt->selected = 0;
	pt->scroll = 0;

	/* The first stuff we draw never changes, and thus has no windows. */
	move(Y_PERSONAL_TERMINAL, X_PERSONAL_TERMINAL);
	attron(A_REVERSE);
	addstr("PERSONAL TERMINAL");
	attroff(A_REVERSE);
	chgat(-1, A_REVERSE, 0, NULL);

	mvaddstr(Y_FOLDERS, X_FOLDERS, "FOLDERS");

	move(pt->maxy - 1, 0);
	attron(A_DIM);
	addstr("UP, DOWN: select folder | LEFT, RIGHT: scroll | q: exit");
	attroff(A_DIM);

	wnoutrefresh(stdscr);

	/* the remaining stuff is redrawn regularly, and uses windows */
	pt->elbow_box = newwin(H_ELBOW_WINDOW, W_ELBOW_WINDOW, Y_ELBOW_WINDOW,
	                       X_ELBOW_WINDOW);
	pt->content_title = newwin(H_CONTENT_TITLE, pt->maxx - X_CONTENT_TITLE,
	                           Y_CONTENT_TITLE, X_CONTENT_TITLE);
	pt->content_text = newwin(pt->maxy - Y_CONTENT_TEXT - 1, /* for bar */
	                          pt->maxx - X_CONTENT_TEXT,
	                          Y_CONTENT_TEXT, X_CONTENT_TEXT);

	for (i = 0; i < N_FOLDER_BOX; i++) {
		pt->folder_box[i] = newwin(H_FOLDER_BOX, W_FOLDER_BOX,
		                           Y_FOLDER_BOX + i * H_FOLDER_BOX,
		                           X_FOLDER_BOX);
		mvwaddstr(pt->folder_box[i], 1, 1, pt->folder_entries[i].folder);
		draw_folder_box_outline(pt, i);

		/* and wrap text (this should move somewhere else) */
		if (wrap_folder_entry(pt, &pt->folder_entries[i]) < 0) {
			mark_error();
			return -1;
		}
	}
	draw_elbow_box(pt);
	draw_content_title(pt);
	draw_content_text(pt);

	doupdate();
	return 0;
}

/**
 * Handle key presses from the personal terminal.
 */
static void personal_terminal_loop(struct personal_terminal *pt)
{
	int key;
	while ((key = getch()) != 'q') {
		switch (key) {
		case KEY_UP:
			select_folder(pt, pt->selected - 1);
			break;
		case KEY_DOWN:
			select_folder(pt, pt->selected + 1);
			break;
		case KEY_LEFT:
			scroll_up(pt);
			break;
		case KEY_RIGHT:
			scroll_down(pt);
			break;
		}
		doupdate();
	}
}

#define CONTENT_BUFFER 512

/**
 * Load a personal_terminal file.
 */
static int pt_load_file(struct pt_params *params, struct personal_terminal *pt,
                        int i)
{
	size_t bufsize = CONTENT_BUFFER;
	size_t contents = 0;
	FILE *f = params->entries[i].content;
	char *buf = malloc(bufsize), *newbuf;

	if (!buf) {
		set_error(EMEM);
		return -1;
	}

	while (contents != bufsize) {
		contents += fread(buf + contents, 1, bufsize - contents, f);
		if (ferror(f)) {
			set_error(EMEM);
			free(buf);
			return -1;
		}
		if (contents == bufsize) {
			bufsize *= 2;
			newbuf = realloc(buf, bufsize);
			if (!newbuf) {
				free(buf);
				set_error(EMEM);
				return -1;
			}
			buf = newbuf;
		}
		if (feof(f))
			break;
	}

	/* yeah this is kinda important */
	buf[contents] = '\0';
	pt->folder_entries[i].text = buf;
	return 0;
}

/**
 * Load personal_terminal contents from config.
 */
int pt_load(struct pt_params *params, struct personal_terminal *pt)
{
	int i;
	for (i = 0; i < params->num_entries; i++) {
		pt->folder_entries[i].folder = params->entries[i].folder;
		pt->folder_entries[i].title = params->entries[i].title;
		if (pt_load_file(params, pt, i) < 0) {
			mark_error();
			goto err_cleanup;
		}
	}
	return 0;

err_cleanup:
	while (--i >= 0) {
		free(pt->folder_entries[i].text);
	}
	return -1;
}

/**
 * Run the whole personal terminal, start to finish. Right now this uses some
 * pre-coded folder entries. Later, I'm assuming that I will have some sort of
 * configuration so it doesn't have to be hand made.
 */
int personal_terminal(struct pt_params *params)
{
	int i;
	int rv = -1;
	struct personal_terminal pt;

	if (pt_load(params, &pt) < 0) {
		mark_error();
		return rv;
	}

	if (init_personal_terminal(&pt) < 0) {
		mark_error();
		goto exit;
	}

	personal_terminal_loop(&pt);
	rv = 0;

exit:
	/* change me when dynamic folder entries are allowed */
	i = 4;
	while (i-- > 0) {
		free(pt.folder_entries[i].text);
	}

	return rv;
}
