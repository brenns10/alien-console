/**
 * Definitions for alien-console.
 * Copyright (c) 2017 Stephen Brennan. Released under the Revised BSD License.
 */
#ifndef ALIEN_CONSOLE_H
#define ALIEN_CONSOLE_H

#include <stdio.h>

/*
 * UTILITES
 */
#define nelem(x) (sizeof(x) / sizeof(x[0]))
int count_lines(char *);

/*
 * CONFIGURATION
 */
#define SYSTEM_CONFIG "/etc/alien-console/alien-console.conf"
#define DEFAULT_CONFIG "/usr/share/alien-console/alien-console.conf"
struct pt_entry {
	char *folder;
	char *title;
	char *content_file;
};

struct splash_params {
	char *filename;
	char *tagline;
	char *copyright;
};

struct pt_params {
	struct splash_params splash;
	struct pt_entry entries[4];
	int num_entries;
};

int parse_config(const char *filename, struct pt_params *params);
void cleanup_config(struct pt_params *params);

/*
 * SPLASH SCREEN
 */


int splash(const struct splash_params *params);

/*
 * PERSONAL TERMINAL
 */
int personal_terminal(struct pt_params *params);

/*
 * ERROR HANDLING (see error.c for an overview)
 */
enum error {
	/* UPDATES TO THIS TABLE MUST UPDATE error_values TABLE IN error.c */
	ESYS         = -1,
	NO_ERROR     = 0,
	ENARROW      = 1,
	ESHORT       = 2,
	EBIGFILE     = 3,
	EBIGTEXT     = 4,
	ECONFREAD    = 5,
	ECONFPARSE   = 6,
	ECONFSET     = 7,
	EMEM         = 8,
	E2MANY       = 9,
};

const char *error_string(void);
int get_error(void);
#define error_is_set() (get_error() != NO_ERROR)
void clear_error(void);
void report_error(FILE *f);

void _mark_error(const char *file, const char *func, int line);
#define mark_error() _mark_error(__FILE__, __func__, __LINE__)

void _set_error(int error, const char *file, const char *func, int line);
#define set_error(error) _set_error(error, __FILE__, __func__, __LINE__)

#endif /* ALIEN_CONSOLE_H */
