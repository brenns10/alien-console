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
 */
#include <ncurses.h>
#include <string.h>

#include "alien-console.h"

#define Y_PERSONAL_TERMINAL 0
#define X_PERSONAL_TERMINAL 0

#define Y_FOLDERS 4
#define X_FOLDERS 1
#define Y_FOLDER_BOX 5
#define X_FOLDER_BOX 0
#define H_FOLDER_BOX 4
#define W_FOLDER_BOX 24
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

struct personal_terminal {
	int maxy, maxx;
	WINDOW *content_title;
	WINDOW *content_text;
	WINDOW *elbow_box;
	WINDOW *folder_box[N_FOLDER_BOX];
	struct folder_entry *folder_entries[N_FOLDER_BOX];
	unsigned int selected;
	unsigned int scroll;
};

struct folder_entry {
	char *folder;
	char *title;
	char *text;
	int lines;
};

char t0[] = "we've got fun and games!";
struct folder_entry eg0 = {
	.folder = "PERSONAL",
	.title  = "Welcome to the jungle",
	.text = t0,
};

char t1[] = (
	"I'm not entirely sure why we have shared messages.\n"
	"This is just going to be a really long-winded string literal "
	"that allows me to test such things as text wrapping, and "
	"scrolling.\n\n"
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus posuere libero at nulla dignissim porta. Etiam quam nibh, tempor et lectus id, viverra fringilla augue. Sed sed laoreet erat. Etiam tempor eget felis eget porta. Aenean purus arcu, venenatis et cursus non, imperdiet non diam. Etiam et scelerisque leo, non auctor ante. Suspendisse potenti.\n\n"
	"Duis faucibus justo in turpis elementum auctor. Nunc quis vulputate tortor, fermentum vehicula lorem. Cras ex ipsum, lacinia sit amet lacus et, tincidunt consequat dui. Integer sollicitudin dignissim augue vulputate fermentum. Suspendisse potenti. Phasellus gravida eu ipsum sed lacinia. Aliquam eget hendrerit sem. Pellentesque venenatis, tortor at dictum malesuada, lorem enim porttitor nulla, ut pellentesque dui orci sed massa. Maecenas vehicula eleifend dolor, et molestie ante congue non. Etiam mi purus, mattis nec eros non, rutrum rutrum ligula.\n\n"
	"Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Duis quis aliquam purus. Etiam mollis pulvinar justo, consectetur posuere tellus. Aliquam hendrerit, arcu sed vestibulum venenatis, massa urna sollicitudin ante, ut eleifend nisl ipsum lacinia nunc. Donec at arcu fringilla, sollicitudin dui non, molestie lorem. Phasellus risus justo, malesuada vitae urna at, pulvinar feugiat ante. Nulla ac lorem nec ipsum vehicula sodales. Vestibulum rutrum tortor quis ante scelerisque, et dictum orci sodales. Ut fermentum, nisl sed venenatis elementum, erat leo hendrerit sapien, vitae vulputate nulla lectus sed ligula. Sed turpis erat, laoreet et velit non, vulputate convallis orci. Ut eu interdum mi. Donec quis accumsan lacus, sit amet vestibulum nisl."
);
struct folder_entry eg1 = {
	.folder = "SHARED",
	.title  = "This is a shared message",
	.text = t1,
};

char t2[] = "If this were really a video game I could let you turn out the lights.";
struct folder_entry eg2 = {
	.folder = "UTILITY",
	.title  = "Turn out the lights",
	.text = t2,
};

char t3[] = "This is a letter from me, Stephen.";
struct folder_entry eg3 = {
	.folder = "PERSONAL",
	.title  = "Hello!",
	.text = t3,
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
			if (line_length == -1) {
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
 * Write a folder entry text body. Uses the selected entry, and the current
 * scroll.
 */
static void write_folder_entry(struct personal_terminal *pt)
{
	int maxy, maxx, nlines, i;
	unsigned int scroll = pt->scroll;
	char *str = pt->folder_entries[pt->selected]->text;
	wclear(pt->content_text);
	box(pt->content_text, 0, 0);
	getmaxyx(pt->content_text, maxy, maxx);
	(void) maxx; /* unused */
	nlines = maxy - 2;

	/* move to line */
	while (scroll > 0 && str) {
		str = strchr(str, '\n');
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

static void draw_selected_elbow(struct personal_terminal *pt)
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
			mvwaddch(pt->elbow_box, 3 + 1, 0, ACS_HLINE);
			wattroff(pt->elbow_box, A_BOLD);
			wnoutrefresh(pt->elbow_box);
			return;
		} else {
			mvwaddch(pt->elbow_box, 3 + i, 1, ACS_VLINE);
		}
	}
	wnoutrefresh(pt->elbow_box);
}

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
	pt->selected = 1;
	pt->scroll = 0;

	/* draw personal terminal text in reverse video at top of screen */
	move(Y_PERSONAL_TERMINAL, X_PERSONAL_TERMINAL);
	attron(A_REVERSE);
	addstr("PERSONAL TERMINAL");
	attroff(A_REVERSE);
	chgat(-1, A_REVERSE, 0, NULL);

	/* draw folders text above where they will go */
	mvaddstr(Y_FOLDERS, X_FOLDERS, "FOLDERS");

	/* draw bottom bar with instructions */
	move(pt->maxy - 1, 0);
	attron(A_DIM);
	addstr("UP, DOWN: select folder | LEFT, RIGHT: scroll | ESC: exit");
	attroff(A_DIM);

	/* refresh stdscr but don't output to terminal until all is done */
	wnoutrefresh(stdscr);

	/* draw the content title box */
	pt->content_title = newwin(H_CONTENT_TITLE, pt->maxx - X_CONTENT_TITLE,
	                           Y_CONTENT_TITLE, X_CONTENT_TITLE);
	box(pt->content_title, 0, 0);
	wnoutrefresh(pt->content_title);

	/* draw the content text box */
	pt->content_text = newwin(pt->maxy - Y_CONTENT_TEXT - 1, /* for bar */
	                          pt->maxx - X_CONTENT_TEXT,
	                          Y_CONTENT_TEXT, X_CONTENT_TEXT);
	box(pt->content_text, 0, 0);
	wnoutrefresh(pt->content_text);

	/* draw the elbow box */
	pt->elbow_box = newwin(H_ELBOW_WINDOW, W_ELBOW_WINDOW, Y_ELBOW_WINDOW,
	                       X_ELBOW_WINDOW);
	draw_selected_elbow(pt);

	/* draw the folder boxes */
	for (i = 0; i < N_FOLDER_BOX; i++) {
		pt->folder_box[i] = newwin(H_FOLDER_BOX, W_FOLDER_BOX,
		                           Y_FOLDER_BOX + i * H_FOLDER_BOX,
		                           X_FOLDER_BOX);
		wattron(pt->folder_box[i], (i == pt->selected ? A_BOLD : A_DIM));
		box(pt->folder_box[i], 0, 0);
		wattroff(pt->folder_box[i], (i == pt->selected ? A_BOLD : A_DIM));
		mvwaddstr(pt->folder_box[i], 1, 1, pt->folder_entries[i]->folder);
		wnoutrefresh(pt->folder_box[i]);
		if (wrap_folder_entry(pt, pt->folder_entries[i]) < 0) {
			mark_error();
			return -1;
		}
	}
	write_folder_entry(pt);

	doupdate();
	getch();
	return 0;
}

int personal_terminal(void)
{
	struct personal_terminal pt;

	pt.folder_entries[0] = &eg0;
	pt.folder_entries[1] = &eg1;
	pt.folder_entries[2] = &eg2;
	pt.folder_entries[3] = &eg3;

	if (init_personal_terminal(&pt) < 0) {
		mark_error();
		return -1;
	}

	return 0;
}
