/**
 * alien-console: configuration
 * Copyright (c) 2017 Stephen Brennan. Released under the Revised BSD License.
 */
#include <stdlib.h>
#include <string.h>

#include <libconfig.h>

#include "alien-console.h"

/**
 * Cleanup an entry.
 */
static void cleanup_pt_entry(struct pt_entry *entry)
{
	free(entry->folder);
	free(entry->title);
	free(entry->content_file);
}

/**
 * Parse a single PT folder entry item.
 */
static int parse_pt_entry(config_setting_t *setting, struct pt_entry *entry)
{
	const char *folder, *title, *content_file;

	/* so free won't fail */
	entry->folder = NULL;
	entry->title = NULL;
	entry->content_file = NULL;

	if (!config_setting_lookup_string(setting, "folder", &folder)) {
		set_error(ECONFSET);
		return -1;
	}

	if (!config_setting_lookup_string(setting, "title", &title)) {
		set_error(ECONFSET);
		return -1;
	}

	if (!config_setting_lookup_string(setting, "content_file", &content_file)) {
		set_error(ECONFSET);
		return -1;
	}

	entry->folder = strdup(folder);
	entry->title = strdup(title);
	entry->content_file = strdup(content_file);

	if (!entry->folder || !entry->title || !entry->content_file) {
		set_error(EMEM);
		cleanup_pt_entry(entry); /* in case any were allocated */
		return -1;
	}
	return 0;
}

/**
 * Parse a full PT config object, containing multiple (up to 4) entries.
 */
static int parse_pt_object(config_setting_t *setting, struct pt_params *params)
{
	int i, len, rv=-1;
	config_setting_t *entry_list, *entry;
	const char *filename, *tagline, *copyright;

	/* so free() won't fail */
	params->splash.filename = NULL;
	params->splash.tagline = NULL;
	params->splash.copyright = NULL;

	if (!config_setting_lookup_string(setting, "filename", &filename)) {
		set_error(ECONFSET);
		return -1;
	}

	if (!config_setting_lookup_string(setting, "tagline", &tagline)) {
		set_error(ECONFSET);
		return -1;
	}

	if (!config_setting_lookup_string(setting, "copyright", &copyright)) {
		set_error(ECONFSET);
		return -1;
	}
	params->splash.filename = strdup(filename);
	params->splash.tagline = strdup(tagline);
	params->splash.copyright = strdup(copyright);
	if (params->splash.filename || params->splash.tagline ||
	    params->splash.copyright) {
		set_error(EMEM);
		goto cleanup_strings;
	}

	entry_list = config_setting_lookup(setting, "entries");
	if (!entry_list || !config_setting_is_list(entry_list)) {
		set_error(ECONFSET);
		goto cleanup_strings;
	}

	len = config_setting_length(entry_list);
	if (len >= (int) nelem(params->entries)) {
		set_error(E2MANY);
		goto cleanup_strings;
	}
	params->num_entries = len;

	for (i = 0; i < len; i++) {
		entry = config_setting_get_elem(entry_list, i);
		if (!entry) {
			set_error(ECONFSET);
			goto cleanup_entries;
		}
		if (parse_pt_entry(entry, &params->entries[i]) < 0) {
			mark_error();
			goto cleanup_entries;
		}
	}
	rv = 0; /* success */
	goto exit;

cleanup_entries:
	while (--i >= 0) {
		cleanup_pt_entry(&params->entries[i]);
	}
cleanup_strings:
	free(params->splash.filename);
	free(params->splash.tagline);
	free(params->splash.copyright);
exit:
	return rv;
}

/**
 * Parse a config file. Right now that's just a personal_terminal key going to
 * a pt_object group. But it could change.
 */
int parse_config(const char *filename, struct pt_params *params)
{
	int rv = -1;
	config_t conf;
	config_setting_t *setting;

	config_init(&conf);
	if (!config_read_file(&conf, filename)) {
		if (config_error_type(&conf) == CONFIG_ERR_PARSE)
			fprintf(stderr, "in line %d: %s\n",
			        config_error_line(&conf),
			        config_error_text(&conf));
		else
			fprintf(stderr, "I/O error for %s\n", filename);
		set_error(ECONFIG);
		goto exit;
	}

	setting = config_lookup(&conf, "personal_terminal");
	if (!setting) {
		set_error(ECONFSET);
		goto exit;
	}

	if (parse_pt_object(setting, params) < 0) {
		mark_error();
		goto exit;
	}
	rv = 0; /* success */

exit:
	config_destroy(&conf);
	return rv;
}

/**
 * Cleanup a config object.
 */
void cleanup_config(struct pt_params *params)
{
	int i;
	free(params->splash.filename);
	free(params->splash.tagline);
	free(params->splash.copyright);
	for (i = 0; i < params->num_entries; i++) {
		cleanup_pt_entry(&params->entries[i]);
	}
}
