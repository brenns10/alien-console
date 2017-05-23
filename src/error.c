/**
 * alien-console: error handling utilities.
 * Copyright (c) 2017 Stephen Brennan. Released under the Revised BSD License.
 *
 * The error handling system here is simple and totally thread-unsafe. Hopefully
 * we won't need threading.
 *
 * If an error is encountered, it's going to be inconvenient to report it on
 * ncurses windows. So, this error handling system makes it so that we can track
 * an error and propagate it back up the call chain until we get back to main().
 *
 * Functions call set_error(ERROR_CODE) when something happens, and then return
 * an error value. For sanity's sake, just return -1 on failure, and 0 on
 * success.
 *
 * When an error check shows an error occurred, simply use mark_error() before
 * returning your error code.
 *
 * Finally, when main() receives an error, it simply jumps to the ncurses
 * cleanup code, and then runs report_error() to generate a traceback on the
 * screen.
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "alien-console.h"

struct error_entry {
	const char *file;
	const char *func;
	int line;
};

/* Max stack depth is 32. Should be fine. */
static struct error_entry error_stack[32];
static int current_error = 0;
static int error_stack_idx = 0;
static bool overflow = false;

static const char *error_values[] = {
	"No error",
	"Terminal is not wide enough",
	"Terminal is not tall enough",
	"File exceeds statically allocated buffer",
	"The user-provided text is too big",
	"Configuration read error",
	"Configuration parse error",
	"Configuration setting was not found or wrong type",
	"Memory allocation error",
	"Too many elements in the folder entry list",
};

/**
 * Return the string error description. Works for any case (no error, app error,
 * or system error).
 */
const char *error_string(void)
{
	if (current_error < 0) {
		return strerror(-current_error);
	} else {
		return error_values[current_error];
	}
}

/**
 * Return the current error code. 0 means success. > 0 means application defined
 * error. < 0 means system error.
 */
int get_error(void)
{
	return current_error;
}

/**
 * Reset the error state. This means you handled the error.
 */
void clear_error(void)
{
	error_stack_idx = 0;
	current_error = 0;
	overflow = false;
}

/**
 * Pushes a "frame" onto the error entry stack. This is a helper for the macro
 * mark_error(), which is what you will usually use.
 */
void _mark_error(const char *file, const char *func, int line)
{
	if (error_stack_idx == nelem(error_stack)) {
		overflow = true;
		return;
	}
	error_stack[error_stack_idx].file = file;
	error_stack[error_stack_idx].func = func;
	error_stack[error_stack_idx].line = line;
	error_stack_idx++;
}

/**
 * Marks an error. This will clear any old errors and report the new one. This
 * is a helper for set_error() macro, which you will usually use.
 */
void _set_error(int error, const char *file, const char *func, int line)
{
	clear_error();

	if (error < 0) {
		/* use system error */
		current_error = -errno;
	} else {
		current_error = error;
	}

	_mark_error(file, func, line);
}

/**
 * Report error. This prints a traceback to f (presumably stderr, but I'm not
 * here to judge).
 */
void report_error(FILE *f)
{
	int i;

	if (!error_is_set())
		return;

	fprintf(f, "Traceback (most recent call last)\n");

	if (overflow)
		fprintf(f, "  (recent stack frames overflowed)\n");

	for (i = error_stack_idx - 1; i >= 0; i--) {
		fprintf(f, "%s:%d: in %s()\n", error_stack[i].file,
		        error_stack[i].line, error_stack[i].func);
	}
	fprintf(f, "Error: %s\n", error_string());
}
