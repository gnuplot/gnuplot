#ifndef lint
static char *RCSid() { return RCSid("$Id: history.c,v 1.2.2.1 2000/05/03 21:26:11 joze Exp $"); }
#endif

/* GNUPLOT - history.c */

/*[
 * Copyright 1986 - 1993, 1999   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

#include "gp_hist.h"

#include "alloc.h"
#include "util.h"

#if defined(READLINE) && !defined(HAVE_LIBREADLINE)

/*
 *   In add_history(), do not store duplicated entries:
 *     Petr Mikulik
 *
 */

/* history code from readline.c by Petr Mikulik (?) */

struct hist *history = NULL;     /* no history yet */
struct hist *cur_entry = NULL;

/* add line to the history */
void
add_history(line)
char *line;
{
    struct hist *entry;

    entry = history;
    while (entry != NULL) {
	/* Don't store duplicate entries */
	if (!strcmp(entry->line, line)) {
	    /* cmd lines are equal, relink entry that was found last */
	    if (entry->next == NULL) {
		/* previous command repeated, no change */
		return;
	    }
	    if (entry->prev == NULL) {
		/* current cmd line equals the first in the history */
		(entry->next)->prev = NULL;
		history->next = entry;
		entry->prev = history;
		entry->next = NULL;
		history = entry;
		return;
	    }
	    /* bridge over entry's vacancy, then move it to the end */
	    (entry->prev)->next = entry->next;
	    (entry->next)->prev = entry->prev;
	    entry->prev = history;
	    history->next = entry;
	    entry->next = NULL;
	    history = entry;
	    return;
	}
	entry = entry->prev;
    }				/* end of not-storing duplicated entries */

    entry = (struct hist *) gp_alloc(sizeof(struct hist), "history");
/*    entry->line = gp_alloc(strlen(line) + 1, "history");
      strcpy(entry->line, line); */
    entry->line = gp_strdup(line);

    entry->prev = history;
    entry->next = NULL;
    if (history != NULL) {
	history->next = entry;
    }
    history = entry;
}


/*
 * New functions for browsing the history. They are called from command.c
 * when the user runs the 'history' command
 */

/* write <n> last entries of the history to the file <filename>
 * Input parameters:
 *    n > 0 ... write only <n> last entries; otherwise all entries
 *    filename == NUL ... write to stdout; otherwise to the filename
*/
void
write_history_n(n, filename)
const int n;
const char *filename;
{
    struct hist *entry = history, *start = NULL;
    FILE *out = stdout;
    int hist_entries = 0;
    int hist_index = 1;

    if (entry == NULL)
	return;			/* no history yet */

    /* find the beginning of the history and count nb of entries */
    while (entry->prev != NUL) {
	hist_entries++;
	if (n > 0 && n == hist_entries)	/* listing will start from this entry */
	    start = entry;
	entry = entry->prev;
    }
    if (start != NULL) {
	entry = start;
	hist_index = hist_entries - n + 1;
    }
    /* now write the history */
    if (filename != NULL)
	out = fopen(filename, "w");
    while (entry != NULL) {
	/* don't add line numbers when writing to file
	 * to make file loadable */
	if (filename)
	    fprintf(out, "%s\n", entry->line);
	else
	    fprintf(out, "%5i  %s\n", hist_index++, entry->line);
	entry = entry->next;
    }
    if (filename != NULL)
	fclose(out);
}


/* obviously the same routine as in GNU readline, according to code from
 * plot.c:#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
 */
void
write_history(filename)
char *filename;
{
    write_history_n(0, filename);
}


/* finds and returns a command from the history which starts with <cmd>
 * (ignores leading spaces in <cmd>)
 * Returns NULL if nothing found
 */
char *
history_find(cmd)
char *cmd;
{
    struct hist *entry = history;
    size_t len;
    char *line;

    if (entry == NULL)
	return NULL;		/* no history yet */
    if (*cmd == '"')
	cmd++;			/* remove surrounding quotes */
    if (!*cmd)
	return NULL;

    len = strlen(cmd);

    if (cmd[len - 1] == '"')
	cmd[--len] = 0;
    if (!*cmd)
	return NULL;

    /* search through the history */
    while (entry->prev != NULL) {
	line = entry->line;
	while (isspace((int) *line))
	    line++;		/* skip leading spaces */
	if (!strncmp(cmd, line, len))	/* entry found */
	    return line;
	entry = entry->prev;
    }
    return NULL;
}


/* finds and print all occurencies of commands from the history which
 * start with <cmd>
 * (ignores leading spaces in <cmd>)
 * Returns 1 on success, 0 if no such entry exists
 */
int
history_find_all(cmd)
char *cmd;
{
    struct hist *entry = history;
    int hist_index = 1;
    char res = 0;
    int len;
    char *line;

    if (entry == NULL)
	return 0;		/* no history yet */
    if (*cmd == '"')
	cmd++;			/* remove surrounding quotes */
    if (!*cmd)
	return 0;
    len = strlen(cmd);
    if (cmd[len - 1] == '"')
	cmd[--len] = 0;
    if (!*cmd)
	return 0;
    /* find the beginning of the history */
    while (entry->prev != NULL)
	entry = entry->prev;
    /* search through the history */
    while (entry != NULL) {
	line = entry->line;
	while (isspace((int) *line))
	    line++;		/* skip leading spaces */
	if (!strncmp(cmd, line, len)) {	/* entry found */
	    printf("%5i  %s\n", hist_index, line);
	    res = 1;
	}
	entry = entry->next;
	hist_index++;
    }
    return res;
}

#elif defined(HAVE_LIBREADLINE)

#include <readline/history.h>

/*
 * Interface between 'gnuplot' and GNU 'readline-4.0'.
 * NOTE:  'show_version()' should report the used readline version too.
 *        There are more version around. '2.0, 2.1, 2.2, 4.0' 
 */
/*
 * Copyright (C) 1999 Thomas Walter
 *
 * This can be used by anybody
 *
 * But you cannot / may not put it under another Copyright
 */

static void do_history_list(FILE * out_stream);

int history_list_save(const int quantity, const char *const place);

int
history_list_save(const int quantity, const char *const place)
{
    /*
     * Input:
     * ======
     * quantity = 0   : list the complete history
     * quantity > 0   : list the recent 'quantity' of elements
     *                  if it is <= total size
     *
     * place = NULL   : write to 'stdout'.
     * place = STRING : write into a file with the name given by 'place'
     *
     * Output:
     * =======
     *  nothing
     *
     * Return:
     * =======
     *  error code : 0 == no error
     */

#if UNIX_LIKE
#define WRITE_MODE "w"
#else
#define WRITE_MODE "wb"
#endif

    int error_code = 0;
    FILE *place_fp = stdout;

    if (place != NULL) {
	place_fp = fopen(place, WRITE_MODE);
	if (place_fp == NULL) {
	    /* Fall back to 'stdout' */
	    place_fp = stdout;
	    error_code = 1;
	}
    }

    /* if (place != NULL) */
    /*
       * Real code goes here
     */
    if (history_is_stifled()) {
	unstifle_history();
    }

    if (quantity > 0) {
	stifle_history(quantity);
    }

    do_history_list(place_fp);

    /* Always leave with an unstifled history */
    if (history_is_stifled()) {
	unstifle_history();
    }
    /*
     * End fo real code
     */

    if (!error_code) {
	if (place != NULL) {
	    fclose(place_fp);
	}
    }
    /* if (!error_code) */
    return (error_code);
}

static void
do_history_list(FILE * out_stream)
{
    HIST_ENTRY **the_list;
    int i;

    the_list = history_list();
    if (the_list) {
	for (i = 0; the_list[i]; i++) {
	    fprintf(out_stream, "%d: %s\n", i + history_base, the_list[i]->line);
	}
    }				/* if (the_list) */
}

/*
 * Callable interfaces:
 */
void
write_history_n(const int n, const char *filename)
{
    /* dicard error code */
    (void) history_list_save(n, filename);
}

char *
history_find(char *cmd)
{
    int found;
    char *ret_val = NULL;
    int current_idx = where_history() + history_base;;

    printf("h_f: Hi user, read the manual of 'GNU readline-4.0'.  Better use 'CONTROL-R'.\n");

    /* printf ("searching for '%s'\n", cmd); */
    /*
     * 'cmd' may be quoted, kill them brute force
     * better would be to call 'm_quote_capture' or 'm_capture' in 'command.c', but
     * equals(c_token, "\"") does not what I want.
     */
    if (cmd[0] == '"') {
	int len = strlen(cmd);
	cmd[len - 1] = '\0';
	cmd++;
    }
    /* printf ("searching for '%s'\n", cmd); */

    do {
	/* idx = history_search_prefix (cmd, 1); *//* Anchored forward search */
	found = history_search_prefix(cmd, -1);	/* Anchored backward search */
	if (found == 0) {
	    int idx = where_history() + history_base;
	    HIST_ENTRY *he_found = current_history();
	    ret_val = he_found->line;
	    /* fprintf (stdout, "c_idx = %d  %d: %s\n", current_idx, idx, ret_val); */
	    replace_history_entry(current_idx, ret_val, NULL);
	    if (idx > 0) {
		break;		/* finished */
	    }			/* !(idx > 0) */
	}			/* if (found == 0) */
    }
    while (found > -1);
    /*  add_history (ret_val); */

    return (ret_val);
}

int
history_find_all(char *cmd)
{
    int found;
    int ret_val = 0;		/* each found entry increases this */

    printf("h_f_a: Hi user, read the manual of 'GNU readline-4.0'.  Better use 'CONTROL-R'.\n");

    /* printf ("searching for '%s'\n", cmd); */
    /*
     * 'cmd' may be quoted, kill them brute force
     * better would be to call 'm_quote_capture' or 'm_capture' in 'command.c', but
     * euqals(c_token, "\"") does not what I want.
     */
    if (cmd[0] == '"') {
	int len = strlen(cmd);
	cmd[len - 1] = '\0';
	cmd++;
    }
    /* printf ("searching for '%s'\n", cmd); */

    do {
	/* idx = history_search_prefix (cmd, 1); *//* Anchored forward search */
	found = history_search_prefix(cmd, -1);	/* Anchored backward search */
	/* fprintf (stdout, "Was gefunden %d\n", found); */
	if (found == 0) {
	    int idx = where_history() + history_base;
#if 0
	    HIST_ENTRY *he_found = current_history();
#endif
	    ret_val++;		/* DEBUG if (ret_val > 5) return (ret_val); */
	    /* fprintf (stdout, "%d: %s\n", idx, he_found->line); */
	    if (idx > 0) {
		history_set_pos(idx - 1);	/* go one step back or you find always the same entry. */
	    } else {		/* !(idx > 0) */

		break;		/* finished */
	    }			/* !(idx > 0) */
	}			/* if (found == 0) */
    }
    while (found > -1);

    return (ret_val);
}

#endif /* READLINE && !HAVE_LIBREADLINE */
