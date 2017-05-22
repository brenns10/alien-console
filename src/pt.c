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

#include "alien-console.h"

#define Y_PERSONAL_TERMINAL 0
#define X_PERSONAL_TERMINAL 0

#define Y_FOLDERS 4
#define X_FOLDERS 0
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
	unsigned int selected;
};

static void draw_selected_elbow(struct personal_terminal *pt)
{
	unsigned int i;
	wattron(pt->elbow_box, A_BOLD);
	mvwaddch(pt->elbow_box, 1, 2, ACS_HLINE);
	mvwaddch(pt->elbow_box, 1, 1, ACS_ULCORNER);
	mvwaddch(pt->elbow_box, 2, 1, ACS_VLINE);
	for (i = 0; i < N_FOLDER_BOX * W_FOLDER_BOX; i++) {
		if (i == pt->selected * H_FOLDER_BOX + 1) {
			mvwaddch(pt->elbow_box, 3 + i, 1, ACS_LRCORNER);
			mvwaddch(pt->elbow_box, 3 + 1, 0, ACS_HLINE);
			wattroff(pt->elbow_box, A_BOLD);
			return;
		} else {
			mvwaddch(pt->elbow_box, 3 + i, 1, ACS_VLINE);
		}
	}
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
	pt->selected = 0;

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
	wnoutrefresh(pt->elbow_box);

	/* draw the folder boxes */
	for (i = 0; i < N_FOLDER_BOX; i++) {
		pt->folder_box[i] = newwin(H_FOLDER_BOX, W_FOLDER_BOX,
		                           Y_FOLDER_BOX + i * H_FOLDER_BOX,
		                           X_FOLDER_BOX);
		wattron(pt->folder_box[i], (i == pt->selected ? A_BOLD : A_DIM));
		box(pt->folder_box[i], 0, 0);
		wattroff(pt->folder_box[i], (i == pt->selected ? A_BOLD : A_DIM));
		wnoutrefresh(pt->folder_box[i]);
	}

	doupdate();
	sleep(5);
}

int personal_terminal(void)
{
	struct personal_terminal pt;

	if (init_personal_terminal(&pt) < 0) {
		mark_error();
		return -1;
	}

	return 0;
}
