/* GNUPLOT - history.c */

/*[
 * Copyright 1986 - 1993, 1999, 2004   Thomas Williams, Colin Kelley
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

#include <stdio.h>

#include "gp_hist.h"

#include "alloc.h"
#include "plot.h"
#include "util.h"

/* Public variables */

int gnuplot_history_size = HISTORY_SIZE;
TBOOLEAN history_quiet = FALSE;
TBOOLEAN history_full = FALSE;


#if defined(READLINE)

/* Built-in readline */

struct hist *history = NULL;	/* last entry in the history list, no history yet */
struct hist *cur_entry = NULL;
int history_length = 0;		/* number of entries in history list */
int history_base = 1;


/* add line to the history */
void
add_history(char *line)
{
    struct hist *entry;

    entry = (struct hist *) gp_alloc(sizeof(struct hist), "history");
    entry->line = gp_strdup(line);
    entry->data = NULL;

    entry->prev = history;
    entry->next = NULL;
    if (history != NULL)
	history->next = entry;
    else
	cur_entry = entry;
    history = entry;
    history_length++;
}


/* write history to a file
 */
int
write_history(char *filename)
{
    write_history_n(0, filename, "w");
    return 0;
}


/* routine to read history entries from a file
 */
void
read_history(char *filename)
{
#ifdef GNUPLOT_HISTORY
    gp_read_history(filename);
#endif
}


void
using_history(void)
{
    /* Nothing to do. */
}


void
clear_history(void)
{
    HIST_ENTRY * entry = history;

    while (entry != NULL) {
	HIST_ENTRY * prev = entry->prev;
	free(entry->line);
	free(entry);
	entry = prev;
    }

    history_length = 0;
    cur_entry = NULL;
    history = NULL;
}


int
where_history(void)
{
    struct hist *entry = history;  /* last_entry */
    int hist_index = history_length;

    if (entry == NULL)
	return 0;		/* no history yet */

    if (cur_entry == NULL)
	return history_length;

    /* find the current history entry and count backwards */
    while ((entry->prev != NULL) && (entry != cur_entry)) {
	entry = entry->prev;
	hist_index--;
    }

    if (hist_index > 0)
	hist_index--;

    return hist_index;
}


int
history_set_pos(int offset)
{
    struct hist *entry = history;  /* last_entry */
    int hist_index = history_length - 1;

    if ((offset < 0) || (offset > history_length) || (history == NULL))
	return 0;

    if (offset == history_length) {
	cur_entry = NULL;
	return 1;
    }

    /* seek backwards */
    while (entry != NULL) {
	if (hist_index == offset) {
	    cur_entry = entry;
	    return 1;
	}
	entry = entry->prev;
	hist_index--;
    }
    return 0;
}


HIST_ENTRY *
history_get(int offset)
{
    struct hist *entry = history;  /* last_entry */
    int hist_index = history_length - 1;
    int hist_ofs = offset - history_base;

   if ((hist_ofs < 0) || (hist_ofs >= history_length) || (history == NULL))
	return NULL;

    /* find the current history entry and count backwards */
    /* seek backwards */
    while (entry != NULL) {
	if (hist_index == hist_ofs)
	    return entry;
	entry = entry->prev;
	hist_index--;
    }

    return NULL;
}


HIST_ENTRY *
current_history(void)
{
    return cur_entry;
}


HIST_ENTRY *
previous_history(void)
{
    if (cur_entry == NULL)
	return (cur_entry = history);
    if ((cur_entry != NULL) && (cur_entry->prev != NULL))
	return (cur_entry = cur_entry->prev);
    else
	return NULL;
}


HIST_ENTRY *
next_history(void)
{
    if (cur_entry != NULL)
	return (cur_entry = cur_entry->next);
    else
	return NULL;
}


HIST_ENTRY *
replace_history_entry(int which, const char *line, histdata_t data)
{
    HIST_ENTRY * entry = history_get(which + 1);
    HIST_ENTRY * prev_entry;

    if (entry == NULL)
	return NULL;

    /* save contents: allocate new entry */
    prev_entry = (HIST_ENTRY *) malloc(sizeof(HIST_ENTRY));
    if (entry != NULL) {
	memset(prev_entry, 0, sizeof(HIST_ENTRY));
	prev_entry->line = entry->line;
	prev_entry->data = entry->data;
    }

    /* set new value */
    entry->line = gp_strdup(line);
    entry->data = data;

    return prev_entry;
}


HIST_ENTRY *
remove_history(int which)
{
    HIST_ENTRY * entry;

    entry = history_get(which + history_base);
    if (entry == NULL)
	return NULL;

    /* remove entry from chain */
    if (entry->prev != NULL)
	entry->prev->next = entry->next;
    if (entry->next != NULL)
	entry->next->prev = entry->prev;
    else
	history = entry->prev; /* last entry */

    if (cur_entry == entry)
	cur_entry = entry->prev;

    /* adjust length */
    history_length--;

    return entry;
}
#endif


#if defined(READLINE) || defined(HAVE_LIBEDITLINE)
histdata_t 
free_history_entry(HIST_ENTRY *histent)
{
    histdata_t data;

    if (histent == NULL)
	return NULL;

    data = histent->data;
    free((void *)(histent->line));
    free(histent);
    return data;
}
#endif


#if defined(READLINE) || defined(HAVE_WINEDITLINE)
int
history_search(const char *string, int direction)
{
    int start;
    HIST_ENTRY *entry;
    /* Work-around for WinEditLine: */
    int once = 1; /* ensure that we try seeking at least one position */
    char * pos;

    start = where_history();
    entry = current_history();
    while (((entry != NULL) && entry->line != NULL) || once) {
	if ((entry != NULL) && (entry->line != NULL) && ((pos = strstr(entry->line, string)) != NULL))
	    return (pos - entry->line);
	if (direction < 0) 
	    entry = previous_history(); 
	else
	    entry = next_history();
	once = 0;
    }
    /* not found */
    history_set_pos(start);
    return -1;
}


int
history_search_prefix(const char *string, int direction)
{
    int start;
    HIST_ENTRY * entry;
    /* Work-around for WinEditLine: */
    int once = 1; /* ensure that we try seeking at least one position */
    size_t len = strlen(string);

    start = where_history();
    entry = current_history();
    while (((entry != NULL) && entry->line != NULL) || once) {
	if ((entry != NULL) && (entry->line != NULL) && (strncmp(entry->line, string, len) == 0))
	    return 0;
	if (direction < 0) 
	    entry = previous_history(); 
	else
	    entry = next_history();
	once = 0;
    }
    /* not found */
    history_set_pos(start);
    return -1;
}
#endif

#ifdef GNUPLOT_HISTORY

/* routine to read history entries from a file,
 * this complements write_history and is necessary for
 * saving of history when we are not using libreadline
 */
int
gp_read_history(const char *filename)
{
    FILE *hist_file;

    if ((hist_file = fopen( filename, "r" ))) {
    	while (!feof(hist_file)) {
	    char line[MAX_LINE_LEN + 1];
	    char *pline;

	    pline = fgets(line, MAX_LINE_LEN, hist_file);
	    if (pline != NULL) {
		/* remove trailing linefeed */
		if ((pline = strrchr(line, '\n')))
		    *pline = '\0';
		if ((pline = strrchr(line, '\r')))
		    *pline = '\0';

		/* skip leading whitespace */
		pline = line;
		while (isspace((unsigned char) *pline))
		    pline++;

		/* avoid adding empty lines */
		if (*pline)
		    add_history(pline);
	    }
	}
	fclose(hist_file);
	return 0;
    } else {
	return errno;
    }
}
#endif


#ifdef USE_READLINE

/* Save history to file, or write to stdout or pipe.
 * For pipes, only "|" works, pipes starting with ">" get a strange 
 * filename like in the non-readline version.
 *
 * Peter Weilbacher, 28Jun2004
 */
void
write_history_list(const int num, const char *const filename, const char *mode)
{
    const HIST_ENTRY *list_entry;
    FILE *out = stdout;
    int is_pipe = 0;
    int is_file = 0;
    int is_quiet = 0;
    int i, istart;

    if (filename && filename[0]) {
	/* good filename given and not quiet */
#ifdef PIPES
	if (filename[0] == '|') {
	    restrict_popen();
	    out = popen(filename + 1, "w");
	    is_pipe = 1;
	} else
#endif
	{
	    if (!(out = fopen(filename, mode))) {
		int_warn(NO_CARET, "Cannot open file to save history, using standard output.\n");
		out = stdout;
	    } else {
		is_file = 1;
	    }
	}
    } else if (filename && !filename[0]) {
	is_quiet = 1;
    }

    /* Determine starting point and output in loop. */
    if (num > 0)
	istart = history_length - num - 1;
    else
	istart = 0;
    if (istart < 0 || istart > history_length)
	istart = 0;

    for (i = istart; (list_entry = history_get(i + history_base)); i++) {
	/* don't add line numbers when writing to file to make file loadable */
	if (!is_file && !is_quiet)
	    fprintf(out, "%5i   %s\n", i + history_base, list_entry->line);
	else
	    fprintf(out, "%s\n", list_entry->line);
    }

#ifdef PIPES
    if (is_pipe) pclose(out);
#endif
    if (is_file) fclose(out);
}


/* This is the function getting called in command.c */
void
write_history_n(const int n, const char *filename, const char *mode)
{
    write_history_list(n, filename, mode);
}
#endif


#ifdef USE_READLINE

/* finds and returns a command from the history list by number */
const char *
history_find_by_number(int n)
{
    if (0 < n && n < history_length)
	return history_get(n)->line;
    else
	return NULL;
}


/* finds and returns a command from the history which starts with <cmd>
 * Returns NULL if nothing found
 *
 * Peter Weilbacher, 28Jun2004
 */
const char *
history_find(char *cmd)
{
    int len;

    /* remove quotes */
    if (*cmd == '"')
	cmd++;
    if (!*cmd)
	return NULL;
    len = strlen(cmd);
    if (cmd[len - 1] == '"')
	cmd[--len] = NUL;
    if (!*cmd)
	return NULL;

    /* Start at latest entry */
#if !defined(HAVE_LIBEDITLINE)
    history_set_pos(history_length);
#else
    while (previous_history());
#endif

    /* Anchored backward search for prefix */
    if (history_search_prefix(cmd, -1) == 0)
	return current_history()->line;
    return NULL;
}


/* finds and print all occurencies of commands from the history which
 * start with <cmd>
 * Returns the number of found entries on success,
 * and 0 if no such entry exists
 *
 * Peter Weilbacher 28Jun2004
 */
int
history_find_all(char *cmd)
{
    int len;
    int found;
    int ret;
    int number = 0; /* each entry found increases this */

    /* remove quotes */
    if (*cmd == '"')
	cmd++;
    if (!*cmd)
	return 0;
    len = strlen(cmd);
    if (cmd[len - 1] == '"')
	cmd[--len] = 0;
    if (!*cmd)
	return 0;

    /* Output matching history entries in chronological order (not backwards
     * so we have to start at the beginning of the history list.
     */
#if !defined(HAVE_LIBEDITLINE)
    ret = history_set_pos(0);
    if (ret == 0) {
	fprintf(stderr, "ERROR (history_find_all): could not rewind history\n");
	return 0;
    }
#else /* HAVE_LIBEDITLINE */
    /* libedit's history_set_pos() does not work properly,
       so we manually go to the oldest entry. Note that directions
       are reversed. */
    while (next_history());
#endif
    do {
	found = history_search_prefix(cmd, 1); /* Anchored backward search for prefix */
	if (found == 0) {
	    number++;
#if !defined(HAVE_LIBEDITLINE)
	    printf("%5i  %s\n", where_history() + history_base, current_history()->line);
	    /* Advance one step or we find always the same entry. */
	    if (next_history() == NULL)
		break; /* finished if stepping didn't work */
#else /* HAVE_LIBEDITLINE */
	    /* libedit's history indices are reversed wrt GNU readline */
	    printf("%5i  %s\n", history_length - where_history() + history_base, current_history()->line);
	    /* Advance one step or we find always the same entry. */
	    if (!previous_history())
		break; /* finished if stepping didn't work */
#endif
	} /* (found == 0) */
    } while (found > -1);

    return number;
}

#endif /* READLINE */
