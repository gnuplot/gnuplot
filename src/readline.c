/* GNUPLOT - readline.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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


/*
 * AUTHORS
 *
 *   Original Software:
 *     Tom Tkacik
 *
 *   Msdos port and some enhancements:
 *     Gershon Elber and many others.
 *
 *   Adapted to work with UTF-8 encoding.
 *     Ethan A Merritt  April 2011
 */

#include <signal.h>

#include "stdfn.h"
#include "readline.h"

#include "alloc.h"
#include "gp_hist.h"
#include "plot.h"
#include "util.h"
#include "encoding.h"
#include "term_api.h"
#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif
#ifdef WGP_CONSOLE
#include "win/winmain.h"
#endif

/*
 * adaptor routine for gnu libreadline
 * to allow multiplexing terminal and mouse input
 */
#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
int
getc_wrapper(FILE* fp)
{
    int c;

    while (1) {
	errno = 0;
#ifdef USE_MOUSE
	/* EAM June 2020:
	 * For 20 years this was conditional on interactive.
	 *    if (term && term->waitforinput && interactive)
	 * Now I am suspicious that this was the cause of dropped
	 * characters when mixing piped input with mousing,
	 * e.g. Bugs 2134, 2279
	 */
	if (term && term->waitforinput) {
	    c = term->waitforinput(0);
	}
	else
#endif
#if defined(WGP_CONSOLE)
	    c = ConsoleGetch();
#else
	if (fp)
	    c = getc(fp);
	else
	    c = getchar(); /* HAVE_LIBEDITLINE */
	if (c == EOF && errno == EINTR)
	    continue;
#endif
	return c;
    }
}
#endif /* HAVE_LIBREADLINE || HAVE_LIBEDITLINE */

#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_SIGNAL_HANDLER)
/*
 * The signal handler of libreadline sets a flag when SIGTSTP is received
 * but does not suspend until this flag is checked by other library
 * routines.  Since gnuplot's term->waitforinput() + getc_wrapper()
 * replace these other routines, we must do the test and suspend ourselves.
 */
void
wrap_readline_signal_handler()
{
    int sig;

    /* FIXME:	At the moment, there is no portable way to invoke the
	    signal handler. */
    extern void _rl_signal_handler(int);

# ifdef HAVE_READLINE_PENDING_SIGNAL
    sig = rl_pending_signal();
# else
    /* XXX: We assume all versions of readline have this... */
    extern int volatile _rl_caught_signal;
    sig = _rl_caught_signal;
# endif

    if (sig) _rl_signal_handler(sig);
}

#endif /* defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_SIGNAL_HANDLER) */


#ifdef READLINE

/* This is a small portable version of GNU's readline that does not require
 * any terminal capabilities except backspace and space overwrites a character.
 * It is not the BASH or GNU EMACS version of READLINE due to Copyleft
 * restrictions.
 * Configuration option:   ./configure --with-readline=builtin
 */

/* NANO-EMACS line editing facility
 * printable characters print as themselves (insert not overwrite)
 * ^A moves to the beginning of the line
 * ^B moves back a single character
 * ^E moves to the end of the line
 * ^F moves forward a single character
 * ^K kills from current position to the end of line
 * ^P moves back through history
 * ^N moves forward through history
 * ^H deletes the previous character
 * ^D deletes the current character, or EOF if line is empty
 * ^L redraw line in case it gets trashed
 * ^U kills the entire line
 * ^W deletes previous full or partial word
 * ^V disables interpretation of the following key
 * LF and CR return the entire line regardless of the cursor position
 * DEL deletes previous or current character (configuration dependent)
 * TAB will perform filename completion
 * ^R start a backward-search of the history
 * EOF with an empty line returns (char *)NULL
 *
 * all other characters are ignored
 */

#ifdef HAVE_SYS_IOCTL_H
/* For ioctl() prototype under Linux (and BeOS?) */
# include <sys/ioctl.h>
#endif

/* replaces the previous kludge in configure */
#if defined(HAVE_TERMIOS_H) && defined(HAVE_TCGETATTR)
# define TERMIOS
#else /* not HAVE_TERMIOS_H && HAVE_TCGETATTR */
# ifdef HAVE_SGTTY_H
#  define SGTTY
# endif
#endif /* not HAVE_TERMIOS_H && HAVE_TCGETATTR */

#if !defined(MSDOS) && !defined(_WIN32)

/*
 * Set up structures using the proper include file
 */
# if defined(_IBMR2) || defined(alliant)
#  define SGTTY
# endif

/*  submitted by Francois.Dagorn@cicb.fr */
# ifdef SGTTY
#  include <sgtty.h>
static struct sgttyb orig_termio, rl_termio;
/* define terminal control characters */
static struct tchars s_tchars;
#  ifndef VERASE
#   define VERASE    0
#  endif			/* not VERASE */
#  ifndef VEOF
#   define VEOF      1
#  endif			/* not VEOF */
#  ifndef VKILL
#   define VKILL     2
#  endif			/* not VKILL */
#  ifdef TIOCGLTC		/* available only with the 'new' line discipline */
static struct ltchars s_ltchars;
#   ifndef VWERASE
#    define VWERASE   3
#   endif			/* not VWERASE */
#   ifndef VREPRINT
#    define VREPRINT  4
#   endif			/* not VREPRINT */
#   ifndef VSUSP
#    define VSUSP     5
#   endif			/* not VSUP */
#  endif			/* TIOCGLTC */
#  ifndef NCCS
#   define NCCS      6
#  endif			/* not NCCS */

# else				/* not SGTTY */

/* SIGTSTP defines job control
 * if there is job control then we need termios.h instead of termio.h
 * (Are there any systems with job control that use termio.h?  I hope not.)
 */
#  if defined(SIGTSTP) || defined(TERMIOS)
#   ifndef TERMIOS
#    define TERMIOS
#   endif			/* not TERMIOS */
#   include <termios.h>
/* Added by Robert Eckardt, RobertE@beta.TP2.Ruhr-Uni-Bochum.de */
#   ifdef ISC22
#    ifndef ONOCR		/* taken from sys/termio.h */
#     define ONOCR 0000020	/* true at least for ISC 2.2 */
#    endif			/* not ONOCR */
#    ifndef IUCLC
#     define IUCLC 0001000
#    endif			/* not IUCLC */
#   endif			/* ISC22 */
#   if !defined(IUCLC)
     /* translate upper to lower case not supported */
#    define IUCLC 0
#   endif			/* not IUCLC */

static struct termios orig_termio, rl_termio;
#  else				/* not SIGSTP || TERMIOS */
#   include <termio.h>
static struct termio orig_termio, rl_termio;
/* termio defines NCC instead of NCCS */
#   define NCCS    NCC
#  endif			/* not SIGTSTP || TERMIOS */
# endif				/* SGTTY */

/* ULTRIX defines VRPRNT instead of VREPRINT */
# if defined(VRPRNT) && !defined(VREPRINT)
#  define VREPRINT VRPRNT
# endif				/* VRPRNT */

/* define characters to use with our input character handler */
static char term_chars[NCCS];

static int term_set = 0;	/* =1 if rl_termio set */

#define special_getc() ansi_getc()
static int ansi_getc(void);
#define DEL_ERASES_CURRENT_CHAR

#else /* MSDOS or _WIN32 */

# ifdef _WIN32
#  include <windows.h>
#  include "win/winmain.h"
#  include "win/wcommon.h"
#  define TEXTUSER 0xf1
#  define TEXTGNUPLOT 0xf0
#  ifdef WGP_CONSOLE
#   define special_getc() win_getch()
static int win_getch(void);
#  else
    /* The wgnuplot text window will suppress intermediate
       screen updates in 'suspend' mode and only redraw the
       input line after 'resume'. */
#   define SUSPENDOUTPUT TextSuspend(&textwin)
#   define RESUMEOUTPUT TextResume(&textwin)
#   define special_getc() msdos_getch()
static int msdos_getch(void);
#  endif /* WGP_CONSOLE */
#  define DEL_ERASES_CURRENT_CHAR
# endif				/* _WIN32 */

# if defined(MSDOS)
/* MSDOS specific stuff */
#  ifdef DJGPP
#   include <pc.h>
#  endif			/* DJGPP */
#  if defined(__EMX__) || defined (__WATCOMC__)
#   include <conio.h>
#  endif			/* __EMX__ */
#  define special_getc() msdos_getch()
static int msdos_getch();
#  define DEL_ERASES_CURRENT_CHAR
# endif				/* MSDOS */

#endif /* MSDOS or _WIN32 */

#ifdef OS2
# if defined( special_getc )
#  undef special_getc()
# endif				/* special_getc */
# define special_getc() os2_getch()
static int msdos_getch(void);
static int os2_getch(void);
#  define DEL_ERASES_CURRENT_CHAR
#endif /* OS2 */


/* initial size and increment of input line length */
#define MAXBUF	1024
#define BACKSPACE '\b'   /* ^H */
#define SPACE	' '
#define NEWLINE	'\n'

#define MAX_COMPLETIONS 50

#ifndef SUSPENDOUTPUT
#define SUSPENDOUTPUT
#define RESUMEOUTPUT
#endif

static char *cur_line;		/* current contents of the line */
static size_t line_len = 0;
static size_t cur_pos = 0;	/* current position of the cursor */
static size_t max_pos = 0;	/* maximum character position */

static TBOOLEAN search_mode = FALSE;
static const char search_prompt[] = "search '";
static const char search_prompt2[] = "': ";
static struct hist * search_result = NULL;
static int search_result_width = 0;	/* on-screen width of the search result */

static void fix_line(void);
static void redraw_line(const char *prompt);
static void clear_line(const char *prompt);
static void clear_eoline(const char *prompt);
static void delete_previous_word(void);
static void copy_line(char *line);
static void set_termio(void);
static void reset_termio(void);
static int user_putc(int ch);
static int user_puts(char *str);
static int backspace(void);
static void extend_cur_line(void);
static void step_forward(void);
static void delete_forward(void);
static void delete_backward(void);
static int char_seqlen(void);
#if defined(HAVE_DIRENT)
static char *fn_completion(size_t anchor_pos, int direction);
static void tab_completion(TBOOLEAN forward);
#endif
static void switch_prompt(const char * old_prompt, const char * new_prompt);
static int do_search(int dir);
static void print_search_result(const struct hist * result);

#ifndef _WIN32
static int mbwidth(const char *c);
#endif
static int strwidth(const char * str);

/* user_putc and user_puts should be used in the place of
 * fputc(ch,stderr) and fputs(str,stderr) for all output
 * of user typed characters.  This allows MS-Windows to
 * display user input in a different color.
 */
static int
user_putc(int ch)
{
    int rv;
#if defined(_WIN32) && !defined(WGP_CONSOLE)
    TextAttr(&textwin, TEXTUSER);
#endif
    rv = fputc(ch, stderr);
#if defined(_WIN32) && !defined(WGP_CONSOLE)
    TextAttr(&textwin, TEXTGNUPLOT);
#endif
    return rv;
}

static int
user_puts(char *str)
{
    int rv;
#if defined(_WIN32) && !defined(WGP_CONSOLE)
    TextAttr(&textwin, TEXTUSER);
#endif
    rv = fputs(str, stderr);
#if defined(_WIN32) && !defined(WGP_CONSOLE)
    TextAttr(&textwin, TEXTGNUPLOT);
#endif
    return rv;
}


#if !defined(_WIN32)
/* EAM FIXME
 * This test is intended to determine if the current character, of which
 * we have only seen the first byte so far, will require twice the width
 * of an ascii character.  The test catches glyphs above unicode 0x3000,
 * which is roughly the set of CJK characters.
 * It should be replaced with a more accurate test.
 */
static int
mbwidth(const char *c)
{
    switch (encoding) {

    case S_ENC_UTF8:
	return ((unsigned char)(*c) >= 0xe3 ? 2 : 1);

    case S_ENC_SJIS:
	/* Assume all double-byte characters have double-width. */
	return is_sjis_lead_byte(*c) ? 2 : 1;

    default:
	return 1;
    }
}
#endif

static int
strwidth(const char * str)
{
#if !defined(_WIN32)
    int width = 0;
    int i = 0;

    switch (encoding) {
    case S_ENC_UTF8:
	while (str[i]) {
	    const char *ch = &str[i++];
	    if ((*ch & 0xE0) == 0xC0) {
		i += 1;
	    } else if ((*ch & 0xF0) == 0xE0) {
		i += 2;
	    } else if ((*ch & 0xF8) == 0xF0) {
		i += 3;
	    }
	    width += mbwidth(ch);
	}
	break;
    case S_ENC_SJIS:
	/* Assume all double-byte characters have double-width. */
	width = gp_strlen(str);
	break;
    default:
	width = strlen(str);
    }
    return width;
#else
    /* double width characters are handled in the backend */
    return gp_strlen(str);
#endif
}

static int
isdoublewidth(size_t pos)
{
#if defined(_WIN32)
    /* double width characters are handled in the backend */
    return FALSE;
#else
    return mbwidth(cur_line + pos) > 1;
#endif
}

/*
 * Determine length of multi-byte sequence starting at current position
 */
static int
char_seqlen()
{
    switch (encoding) {

    case S_ENC_UTF8: {
	int i = cur_pos;
	do {i++;}
	while (((cur_line[i] & 0xc0) != 0xc0)
	       && ((cur_line[i] & 0x80) != 0)
	       && (i < max_pos));
	return (i - cur_pos);
    }

    case S_ENC_SJIS:
	return is_sjis_lead_byte(cur_line[cur_pos]) ? 2 : 1;

    default:
	return 1;
    }
}

/*
 * Back up over one multi-byte character sequence immediately preceding
 * the current position.  Non-destructive.  Affects both cur_pos and screen cursor.
 */
static int
backspace()
{
    switch (encoding) {

    case S_ENC_UTF8: {
	int seqlen = 0;
	do {
	    cur_pos--;
	    seqlen++;
	} while (((cur_line[cur_pos] & 0xc0) != 0xc0)
	         && ((cur_line[cur_pos] & 0x80) != 0)
		 && (cur_pos > 0)
	        );

	if (   ((cur_line[cur_pos] & 0xc0) == 0xc0)
	    || isprint((unsigned char)cur_line[cur_pos])
	   )
	    user_putc(BACKSPACE);
	if (isdoublewidth(cur_pos))
	    user_putc(BACKSPACE);
	return seqlen;
    }

    case S_ENC_SJIS: {
	/* With S-JIS you cannot always determine if a byte is a single byte or part
	   of a double-byte sequence by looking of an arbitrary byte in a string.
	   Always test from the start of the string instead.
	*/
	int i;
        int seqlen = 1;

	for (i = 0; i < cur_pos; i += seqlen) {
	    seqlen = is_sjis_lead_byte(cur_line[i]) ? 2 : 1;
	}
	cur_pos -= seqlen;
	user_putc(BACKSPACE);
	if (isdoublewidth(cur_pos))
	    user_putc(BACKSPACE);
	return seqlen;
    }

    default:
	cur_pos--;
	user_putc(BACKSPACE);
	return 1;
    }
}

/*
 * Step forward over one multi-byte character sequence.
 * We don't assume a non-destructive forward space, so we have
 * to redraw the character as we go.
 */
static void
step_forward()
{
    int i, seqlen;

    switch (encoding) {

    case S_ENC_UTF8:
    case S_ENC_SJIS:
	seqlen = char_seqlen();
	for (i = 0; i < seqlen; i++)
	    user_putc(cur_line[cur_pos++]);
	break;

    default:
	user_putc(cur_line[cur_pos++]);
	break;
    }
}

/*
 * Delete the character we are on and collapse all subsequent characters back one
 */
static void
delete_forward()
{
    if (cur_pos < max_pos) {
	size_t i;
	int seqlen = char_seqlen();
	max_pos -= seqlen;
	for (i = cur_pos; i < max_pos; i++)
	    cur_line[i] = cur_line[i + seqlen];
	cur_line[max_pos] = '\0';
	fix_line();
    }
}

/*
 * Delete the previous character and collapse all subsequent characters back one
 */
static void
delete_backward()
{
    if (cur_pos > 0) {
	size_t i;
	int seqlen = backspace();
	max_pos -= seqlen;
	for (i = cur_pos; i < max_pos; i++)
	    cur_line[i] = cur_line[i + seqlen];
	cur_line[max_pos] = '\0';
	fix_line();
    }
}

static void
extend_cur_line()
{
    char *new_line;

    /* extend input line length */
    new_line = (char *) gp_realloc(cur_line, line_len + MAXBUF, NULL);
    if (!new_line) {
	reset_termio();
	int_error(NO_CARET, "Can't extend readline length");
    }
    cur_line = new_line;
    line_len += MAXBUF;
    FPRINTF((stderr, "\nextending readline length to %d chars\n", line_len));
}


#if defined(HAVE_DIRENT)
static char *
fn_completion(size_t anchor_pos, int direction)
{
    static char * completions[MAX_COMPLETIONS];
    static int n_completions = 0;
    static int completion_idx = 0;

    if (direction == 0) {
	/* new completion */
	DIR * dir;
	char * start, * path;
	char * t, * search;
	char * name = NULL;
	size_t nlen;

	if (n_completions != 0) {
	    /* new completion, cleanup first */
	    int i;
	    for (i = 0; i < n_completions; i++)
		free(completions[i]);
	    memset(completions, 0, sizeof(completions));
	    n_completions = 0;
	    completion_idx = 0;
	}

	/* extract path to complete */
	start = cur_line + anchor_pos;
	if (anchor_pos > 0) {
	    /* first, look for a quote to start the string */
	    for ( ; start >= cur_line; start--) {
	        if ((*start == '"') || (*start == '\'')) {
		    start++;
		    /* handle pipe commands */
		    if ((*start == '<') || (*start == '|'))
			start++;
		    break;
		}
	    }
	    /* if not found, search for a space or a system command '!' instead */
	    if (start <= cur_line) {
		for (start = cur_line + anchor_pos; start >= cur_line; start--) {
		    if ((*start == ' ') || (*start == '!')) {
			start++;
			break;
		    }
		}
	    }
	    if (start < cur_line)
		start = cur_line;

	    path = strndup(start, cur_line - start + anchor_pos);
	    gp_expand_tilde(&path);
	} else {
	    path = gp_strdup("");
	}

	/* separate directory and (partial) file directory name */
	t = strrchr(path, DIRSEP1);
#if DIRSEP2 != NUL
	if (t == NULL) t = strrchr(path, DIRSEP2);
#endif
	if (t == NULL) {
	    /* name... */
	    search = gp_strdup(".");
	    name = strdup(path);
	} else if (t == path) {
	    /* root dir: /name... */
	    search = strndup(path, 1);
	    nlen = cur_pos - (t - path) - 1;
	    name = strndup(t + 1, nlen);
	} else {
	    /* normal case: dir/dir/name... */
	    search = strndup(path, t - path);
	    nlen = cur_pos - (t - path) - 1;
	    name = strndup(t + 1, nlen);
	}
	nlen = strlen(name);
	free(path);

	n_completions = 0;
	if ((dir = opendir(search))) {
	    struct dirent * entry;
	    while ((entry = readdir(dir)) != NULL) {
		/* ignore files and directories starting with a dot */
		if (entry->d_name[0] == '.')
		    continue;

		/* skip entries which don't match */
		if (nlen > 0)
		    if (strncmp(entry->d_name, name, nlen) != 0) continue;

		completions[n_completions] = gp_strdup(entry->d_name + nlen);
		n_completions++;

		/* limit number of completions */
		if (n_completions == MAX_COMPLETIONS) break;
	    }
	    closedir(dir);
	    free(search);
	    if (name) free(name);
	    if (n_completions > 0)
		return completions[0];
	    else
		return NULL;
	}
	free(search);
	if (name) free(name);
    } else {
	/* cycle trough previous results */
	if (n_completions > 0) {
	    if (direction > 0)
		completion_idx = (completion_idx + 1) % n_completions;
	    else
		completion_idx = (completion_idx + n_completions - 1) % n_completions;
	    return completions[completion_idx];
	} else
	    return NULL;
    }
    return NULL;
}


static void
tab_completion(TBOOLEAN forward)
{
    size_t i;
    char * completion;
    size_t completion_len;
    static size_t last_tab_pos = -1;
    static size_t last_completion_len = 0;
    int direction;

    /* detect tab cycling */
    if ((last_tab_pos + last_completion_len) != cur_pos) {
	last_completion_len = 0;
	last_tab_pos = cur_pos;
	direction = 0; /* new completion */
    } else {
	direction = (forward ? 1 : -1);
    }

    /* find completion */
    completion = fn_completion(last_tab_pos, direction);
    if (!completion) return;

    /* make room for new completion */
    completion_len = strlen(completion);
    if (completion_len > last_completion_len)
	while (max_pos + completion_len - last_completion_len + 1 > line_len)
	    extend_cur_line();

    SUSPENDOUTPUT;
    /* erase from last_tab_pos to eol */
    while (cur_pos > last_tab_pos)
	backspace();
    while (cur_pos < max_pos) {
	user_putc(SPACE);
	if (isdoublewidth(cur_pos))
	    user_putc(SPACE);
	cur_pos += char_seqlen();
    }

    /* rewind to last_tab_pos */
    while (cur_pos > last_tab_pos)
	backspace();

    /* insert completion string */
    if (max_pos > (last_tab_pos - last_completion_len))
	memmove(cur_line + last_tab_pos + completion_len,
		cur_line + last_tab_pos + last_completion_len,
		max_pos  - last_tab_pos - last_completion_len);
    memcpy(cur_line + last_tab_pos, completion, completion_len);
    max_pos += completion_len - last_completion_len;
    cur_line[max_pos] = NUL;

    /* draw new completion */
    for (i = 0; i < completion_len; i++)
	user_putc(cur_line[last_tab_pos+i]);
    cur_pos += completion_len;
    fix_line();
    RESUMEOUTPUT;

    /* remember this completion */
    last_tab_pos  = cur_pos - completion_len;
    last_completion_len = completion_len;
}

#endif /* HAVE_DIRENT */


char *
readline(const char *prompt)
{
    int cur_char;
    char *new_line;
    TBOOLEAN next_verbatim = FALSE;
    char *prev_line;

    /* start with a string of MAXBUF chars */
    if (line_len != 0) {
	free(cur_line);
	line_len = 0;
    }
    cur_line = (char *) gp_alloc(MAXBUF, "readline");
    line_len = MAXBUF;

    /* set the termio so we can do our own input processing */
    set_termio();

    /* print the prompt */
    fputs(prompt, stderr);
    cur_line[0] = '\0';
    cur_pos = 0;
    max_pos = 0;

    /* move to end of history */
    while (next_history());

    /* init global variables */
    search_mode = FALSE;

    /* get characters */
    for (;;) {

	cur_char = special_getc();

	/* Accumulate ascii (7bit) printable characters
	 * and all leading 8bit characters.
	 */
	if (((isprint((unsigned char)cur_char)
	      || (((cur_char & 0x80) != 0) && (cur_char != EOF))
	     ) && (cur_char != '\t')) /* TAB is a printable character in some locales */
	    || next_verbatim
	    ) {
	    size_t i;

	    if (max_pos + 1 >= line_len) {
		extend_cur_line();
	    }
	    for (i = max_pos; i > cur_pos; i--) {
		cur_line[i] = cur_line[i - 1];
	    }
	    user_putc(cur_char);

	    cur_line[cur_pos] = cur_char;
	    cur_pos += 1;
	    max_pos += 1;
	    cur_line[max_pos] = '\0';

	    if (cur_pos < max_pos) {
		switch (encoding) {
		case S_ENC_UTF8:
		    if ((cur_char & 0xc0) == 0) {
			next_verbatim = FALSE;
			fix_line(); /* Normal ascii character */
		    } else if ((cur_char & 0xc0) == 0xc0) {
			; /* start of a multibyte sequence. */
		    } else if (((cur_char & 0xc0) == 0x80) &&
			 ((unsigned char)(cur_line[cur_pos-2]) >= 0xe0)) {
			; /* second byte of a >2 byte sequence */
		    } else {
			/* Last char of multi-byte sequence */
			next_verbatim = FALSE;
			fix_line();
		    }
		    break;

		case S_ENC_SJIS: {
		    /* S-JIS requires a state variable */
		    static int mbwait = 0;

		    if (mbwait == 0) {
		        if (!is_sjis_lead_byte(cur_char)) {
			    /* single-byte character */
			    next_verbatim = FALSE;
			    fix_line();
			} else {
			    /* first byte of a double-byte sequence */
			    ;
			}
		    } else {
			/* second byte of a double-byte sequence */
			mbwait = 0;
			next_verbatim = FALSE;
			fix_line();
		    }
		}

		default:
		    next_verbatim = FALSE;
		    fix_line();
		    break;
		}
	    } else {
		static int mbwait = 0;

		next_verbatim = FALSE;
		if (search_mode) {
		    /* Only update the search at the end of a multi-byte sequence. */
		    if (mbwait == 0) {
			if (encoding == S_ENC_SJIS)
			    mbwait = is_sjis_lead_byte(cur_char) ? 1 : 0;
			if (encoding == S_ENC_UTF8) {
			    char ch = cur_char;
			    if (ch & 0x80)
				while ((ch = (ch << 1)) & 0x80)
				    mbwait++;
			}
		    } else {
			mbwait--;
		    }
		    if (!mbwait)
			do_search(-1);
		}
	    }

	/* ignore special characters in search_mode */
	} else if (!search_mode) {

	if (0) {
	    ;

	/* else interpret unix terminal driver characters */
#ifdef VERASE
	} else if (cur_char == term_chars[VERASE]) {	/* ^H */
	    delete_backward();
#endif /* VERASE */
#ifdef VEOF
	} else if (cur_char == term_chars[VEOF]) {	/* ^D? */
	    if (max_pos == 0) {
		reset_termio();
		return ((char *) NULL);
	    }
	    delete_forward();
#endif /* VEOF */
#ifdef VKILL
	} else if (cur_char == term_chars[VKILL]) {	/* ^U? */
	    clear_line(prompt);
#endif /* VKILL */
#ifdef VWERASE
	} else if (cur_char == term_chars[VWERASE]) {	/* ^W? */
	    delete_previous_word();
#endif /* VWERASE */
#ifdef VREPRINT
#if 0 /* conflict with reverse-search */
	} else if (cur_char == term_chars[VREPRINT]) {	/* ^R? */
	    putc(NEWLINE, stderr);	/* go to a fresh line */
	    redraw_line(prompt);
#endif
#endif /* VREPRINT */
#ifdef VSUSP
	} else if (cur_char == term_chars[VSUSP]) {
	    reset_termio();
	    kill(0, SIGTSTP);

	    /* process stops here */

	    set_termio();
	    /* print the prompt */
	    redraw_line(prompt);
#endif /* VSUSP */
	} else {
	    /* do normal editing commands */
	    /* some of these are also done above */
	    switch (cur_char) {
	    case EOF:
		reset_termio();
		return ((char *) NULL);
	    case 001:		/* ^A */
		while (cur_pos > 0)
		    backspace();
		break;
	    case 002:		/* ^B */
		if (cur_pos > 0)
		    backspace();
		break;
	    case 005:		/* ^E */
		while (cur_pos < max_pos) {
		    user_putc(cur_line[cur_pos]);
		    cur_pos += 1;
		}
		break;
	    case 006:		/* ^F */
		if (cur_pos < max_pos) {
		    step_forward();
		}
		break;
#if defined(HAVE_DIRENT)
	    case 011:		/* ^I / TAB */
		tab_completion(TRUE); /* next tab completion */
		break;
	    case 034:		/* remapped by wtext.c or ansi_getc from Shift-Tab */
		tab_completion(FALSE); /* previous tab completion */
		break;
#endif
	    case 013:		/* ^K */
		clear_eoline(prompt);
		max_pos = cur_pos;
		break;
	    case 020:		/* ^P */
		if (previous_history() != NULL) {
		    clear_line(prompt);
		    copy_line(current_history()->line);
		}
		break;
	    case 016:		/* ^N */
		clear_line(prompt);
		if (next_history() != NULL) {
		    copy_line(current_history()->line);
		} else {
		    cur_pos = max_pos = 0;
		}
		break;
	    case 022:		/* ^R */
		prev_line = strdup(cur_line);
		switch_prompt(prompt, search_prompt);
		while (next_history()); /* seek to end of history */
		search_result = NULL;
		search_result_width = 0;
		search_mode = TRUE;
		print_search_result(NULL);
		break;
	    case 014:		/* ^L */
		putc(NEWLINE, stderr);	/* go to a fresh line */
		redraw_line(prompt);
		break;
#ifndef DEL_ERASES_CURRENT_CHAR
	    case 0177:		/* DEL */
	    case 023:		/* Re-mapped from CSI~3 in ansi_getc() */
#endif
	    case 010:		/* ^H */
		delete_backward();
		break;
	    case 004:		/* ^D */
		/* Also catch asynchronous termination signal on Windows */
		if (max_pos == 0 || terminate_flag) {
		    reset_termio();
		    return NULL;
		}
		/* intentionally omitting break */
#ifdef DEL_ERASES_CURRENT_CHAR
	    case 0177:		/* DEL */
	    case 023:		/* Re-mapped from CSI~3 in ansi_getc() */
#endif
		delete_forward();
		break;
	    case 025:		/* ^U */
		clear_line(prompt);
		break;
	    case 026:		/* ^V */
		next_verbatim = TRUE;
		break;
	    case 027:		/* ^W */
		delete_previous_word();
		break;
	    case '\n':		/* ^J */
	    case '\r':		/* ^M */
		cur_line[max_pos + 1] = '\0';
#ifdef OS2
		while (cur_pos < max_pos) {
		    user_putc(cur_line[cur_pos]);
		    cur_pos += 1;
		}
#endif
		putc(NEWLINE, stderr);

		/* Shrink the block down to fit the string ?
		 * if the alloc fails, we still own block at cur_line,
		 * but this shouldn't really fail.
		 */
		new_line = (char *) gp_realloc(cur_line, strlen(cur_line) + 1,
					       "line resize");
		if (new_line)
		    cur_line = new_line;
		/* else we just hang on to what we had - it's not a problem */

		line_len = 0;
		FPRINTF((stderr, "Resizing input line to %d chars\n", strlen(cur_line)));
		reset_termio();
		return (cur_line);
	    default:
		break;
	    }
	}

	} else {  /* search-mode */
#ifdef VERASE
	    if (cur_char == term_chars[VERASE]) {	/* ^H */
		delete_backward();
		do_search(-1);
	    } else
#endif /* VERASE */
	    {
	    switch (cur_char) {
	    case 022:		/* ^R */
		/* search next */
		previous_history();
		if (do_search(-1) == -1)
		    next_history();
		break;
	    case 023:		/* ^S */
		/* search previous */
		next_history();
		if (do_search(1) == -1)
		    previous_history();
		break;
		break;
	    case '\n':		/* ^J */
	    case '\r':		/* ^M */
		/* accept */
		switch_prompt(search_prompt, prompt);
		if (search_result != NULL)
		    copy_line(search_result->line);
		free(prev_line);
		search_result_width = 0;
		search_mode = FALSE;
		break;
#ifndef DEL_ERASES_CURRENT_CHAR
	    case 0177:		/* DEL */
	    /* FIXME: conflict! */
	    //case 023:		/* Re-mapped from CSI~3 in ansi_getc() */
#endif
	    case 010:		/* ^H */
		delete_backward();
		do_search(1);
		break;
	    default:
		/* abort, restore previous input line */
		switch_prompt(search_prompt, prompt);
		copy_line(prev_line);
		free(prev_line);
		search_result_width = 0;
		search_mode = FALSE;
		break;
	    }
	    }
	}
    }
}


static int
do_search(int dir)
{
    int ret = -1;

    if ((ret = history_search(cur_line, dir)) != -1)
	search_result = current_history();
    print_search_result(search_result);
    return ret;
}


void
print_search_result(const struct hist * result)
{
    int i, width = 0;

    SUSPENDOUTPUT;
    fputs(search_prompt2, stderr);
    if (result != NULL && result->line != NULL) {
	fputs(result->line, stderr);
	width = strwidth(result->line);
    }

    /* overwrite previous search result, and the line might
       just have gotten 1 double-width character shorter */
    for (i = 0; i < search_result_width - width + 2; i++)
	putc(SPACE, stderr);
    for (i = 0; i < search_result_width - width + 2; i++)
	putc(BACKSPACE, stderr);
    search_result_width = width;

    /* restore cursor position */
    for (i = 0; i < width; i++)
	putc(BACKSPACE, stderr);
    for (i = 0; i < strlen(search_prompt2); i++)
	putc(BACKSPACE, stderr);
    RESUMEOUTPUT;
}


static void
switch_prompt(const char * old_prompt, const char * new_prompt)
{
    int i, len;

    SUSPENDOUTPUT;

    /* clear search results (if any) */
    if (search_mode) {
	for (i = 0; i < search_result_width + strlen(search_prompt2); i++)
	    user_putc(SPACE);
	for (i = 0; i < search_result_width + strlen(search_prompt2); i++)
	    user_putc(BACKSPACE);
    }

    /* clear current line */
    clear_line(old_prompt);
    putc('\r', stderr);
    fputs(new_prompt, stderr);
    cur_pos = 0;

    /* erase remainder of previous prompt */
    len = GPMAX((int)strlen(old_prompt) - (int)strlen(new_prompt), 0);
    for (i = 0; i < len; i++)
	user_putc(SPACE);
    for (i = 0; i < len; i++)
	user_putc(BACKSPACE);
    RESUMEOUTPUT;
}


/* Fix up the line from cur_pos to max_pos.
 * Does not need any terminal capabilities except backspace,
 * and space overwrites a character
 */
static void
fix_line()
{
    size_t i;

    SUSPENDOUTPUT;
    /* write tail of string */
    for (i = cur_pos; i < max_pos; i++)
	user_putc(cur_line[i]);

    /* We may have just shortened the line by deleting a character.
     * Write a space at the end to over-print the former last character.
     * It needs 2 spaces in case the former character was double width.
     */
    user_putc(SPACE);
    user_putc(SPACE);
    if (search_mode) {
	for (i = 0; i < search_result_width; i++)
	    user_putc(SPACE);
	for (i = 0; i < search_result_width; i++)
	    user_putc(BACKSPACE);
    }
    user_putc(BACKSPACE);
    user_putc(BACKSPACE);

    /* Back up to original position */
    i = cur_pos;
    for (cur_pos = max_pos; cur_pos > i; )
	backspace();
    RESUMEOUTPUT;
}

/* redraw the entire line, putting the cursor where it belongs */
static void
redraw_line(const char *prompt)
{
    size_t i;

    SUSPENDOUTPUT;
    fputs(prompt, stderr);
    user_puts(cur_line);

    /* put the cursor where it belongs */
    i = cur_pos;
    for (cur_pos = max_pos; cur_pos > i; )
	backspace();
    RESUMEOUTPUT;
}

/* clear cur_line and the screen line */
static void
clear_line(const char *prompt)
{
    SUSPENDOUTPUT;
    putc('\r', stderr);
    fputs(prompt, stderr);
    cur_pos = 0;

    while (cur_pos < max_pos) {
	user_putc(SPACE);
	if (isdoublewidth(cur_pos))
	    user_putc(SPACE);
	cur_pos += char_seqlen();
    }
    while (max_pos > 0)
	cur_line[--max_pos] = '\0';

    putc('\r', stderr);
    fputs(prompt, stderr);
    cur_pos = 0;
    RESUMEOUTPUT;
}

/* clear to end of line and the screen end of line */
static void
clear_eoline(const char *prompt)
{
    size_t save_pos = cur_pos;

    SUSPENDOUTPUT;
    while (cur_pos < max_pos) {
	user_putc(SPACE);
	if (isdoublewidth(cur_line[cur_pos]))
	    user_putc(SPACE);
	cur_pos += char_seqlen();
    }
    cur_pos = save_pos;
    while (max_pos > cur_pos)
	cur_line[--max_pos] = '\0';

    putc('\r', stderr);
    fputs(prompt, stderr);
    user_puts(cur_line);
    RESUMEOUTPUT;
}

/* delete the full or partial word immediately before cursor position */
static void
delete_previous_word()
{
    size_t save_pos = cur_pos;

    SUSPENDOUTPUT;
    /* skip whitespace */
    while ((cur_pos > 0) &&
	   (cur_line[cur_pos - 1] == SPACE)) {
	backspace();
    }
    /* find start of previous word */
    while ((cur_pos > 0) &&
	   (cur_line[cur_pos - 1] != SPACE)) {
	backspace();
    }
    if (cur_pos != save_pos) {
	size_t new_cur_pos = cur_pos;
	size_t m = max_pos - save_pos;

	/* erase to eol */
	while (cur_pos < max_pos) {
	    user_putc(SPACE);
	    if (isdoublewidth(cur_pos))
		user_putc(SPACE);
	    cur_pos += char_seqlen();
	}
	while (cur_pos > new_cur_pos)
	    backspace();

	/* overwrite previous word with trailing characters */
	memmove(cur_line + cur_pos, cur_line + save_pos, m);
	/* overwrite characters at end of string with NULs */
	memset(cur_line + cur_pos + m, NUL, save_pos - cur_pos);

	/* update display and line length */
	max_pos = cur_pos + m;
	fix_line();
    }
    RESUMEOUTPUT;
}

/* copy line to cur_line, draw it and set cur_pos and max_pos */
static void
copy_line(char *line)
{
    while (strlen(line) + 1 > line_len) {
	extend_cur_line();
    }
    strcpy(cur_line, line);
    user_puts(cur_line);
    cur_pos = max_pos = strlen(cur_line);
}

#if !defined(MSDOS) && !defined(_WIN32)
/* Convert ANSI arrow keys to control characters */
static int
ansi_getc()
{
    int c;

#ifdef USE_MOUSE
    /* EAM June 2020 why only interactive?
     * if (term && term->waitforinput && interactive)
     */
    if (term && term->waitforinput)
	c = term->waitforinput(0);
    else
#endif
    c = getc(stdin);

    if (c == 033) {
	c = getc(stdin);	/* check for CSI */
	if (c == '[') {
	    c = getc(stdin);	/* get command character */
	    switch (c) {
	    case 'D':		/* left arrow key */
		c = 002;
		break;
	    case 'C':		/* right arrow key */
		c = 006;
		break;
	    case 'A':		/* up arrow key */
		c = 020;
		break;
	    case 'B':		/* down arrow key */
		c = 016;
		break;
	    case 'F':		/* end key */
		c = 005;
		break;
	    case 'H':		/* home key */
		c = 001;
		break;
	    case 'Z':		/* shift-tab key */
		c = 034;	/* FS: non-standard! */
		break;
	    case '3':		/* DEL can be <esc>[3~ */
		getc(stdin);	/* eat the ~ */
		c = 023;	/* DC3 ^S NB: non-standard!! */
	    }
	}
    }
    return c;
}
#endif

#if defined(MSDOS) || defined(_WIN32) || defined(OS2)

#ifdef WGP_CONSOLE
static int
win_getch()
{
    if (term && term->waitforinput)
	return term->waitforinput(0);
    else
	return ConsoleGetch();
}
#else

/* Convert Arrow keystrokes to Control characters: */
static int
msdos_getch()
{
    int c;

#ifdef DJGPP
    /* no need to handle mouse input here: it's done in term->text() */
    int ch = getkey();
    c = (ch & 0xff00) ? 0 : ch & 0xff;
#elif defined (OS2)
    c = getc(stdin);
#else /* not OS2, not DJGPP*/
# if defined (USE_MOUSE)
    if (term && term->waitforinput && interactive)
	c = term->waitforinput(0);
    else
# endif /* not USE_MOUSE */
    c = getch();
#endif /* not DJGPP, not OS2 */

    if (c == 0) {
#ifdef DJGPP
	c = ch & 0xff;
#elif defined(OS2)
	c = getc(stdin);
#else /* not OS2, not DJGPP */
# if defined (USE_MOUSE)
    if (term && term->waitforinput && interactive)
	c = term->waitforinput(0);
    else
# endif /* not USE_MOUSE */
	c = getch();		/* Get the extended code. */
#endif /* not DJGPP, not OS2 */

	switch (c) {
	case 75:		/* Left Arrow. */
	    c = 002;
	    break;
	case 77:		/* Right Arrow. */
	    c = 006;
	    break;
	case 72:		/* Up Arrow. */
	    c = 020;
	    break;
	case 80:		/* Down Arrow. */
	    c = 016;
	    break;
	case 115:		/* Ctl Left Arrow. */
	case 71:		/* Home */
	    c = 001;
	    break;
	case 116:		/* Ctl Right Arrow. */
	case 79:		/* End */
	    c = 005;
	    break;
	case 83:		/* Delete */
	    c = 0177;
	    break;
	case 15:		/* BackTab / Shift-Tab */
	    c = 034;	/* FS: remap to non-standard code for tab-completion */
	    break;
	default:
	    c = 0;
	    break;
	}
    } else if (c == 033) {	/* ESC */
	c = 025;
    }
    return c;
}
#endif /* WGP_CONSOLE */
#endif /* MSDOS || _WIN32 || OS2 */

#ifdef OS2
/* We need to call different procedures, dependent on the
   session type: VIO/window or an (XFree86) xterm */
static int
os2_getch() {
  static int IsXterm = 0;
  static int init = 0;

  if (!init) {
     if (getenv("WINDOWID")) {
        IsXterm = 1;
     }
     init = 1;
  }
  if (IsXterm) {
     return ansi_getc();
  } else {
     return msdos_getch();
  }
}
#endif /* OS2 */


  /* set termio so we can do our own input processing */
static void
set_termio()
{
#if !defined(MSDOS) && !defined(_WIN32)
/* set termio so we can do our own input processing */
/* and save the old terminal modes so we can reset them later */
    if (term_set == 0) {
	/*
	 * Get terminal modes.
	 */
#  ifdef SGTTY
	ioctl(0, TIOCGETP, &orig_termio);
#  else				/* not SGTTY */
#   ifdef TERMIOS
#    ifdef TCGETS
	ioctl(0, TCGETS, &orig_termio);
#    else			/* not TCGETS */
	tcgetattr(0, &orig_termio);
#    endif			/* not TCGETS */
#   else			/* not TERMIOS */
	ioctl(0, TCGETA, &orig_termio);
#   endif			/* TERMIOS */
#  endif			/* not SGTTY */

	/*
	 * Save terminal modes
	 */
	rl_termio = orig_termio;

	/*
	 * Set the modes to the way we want them
	 *  and save our input special characters
	 */
#  ifdef SGTTY
	rl_termio.sg_flags |= CBREAK;
	rl_termio.sg_flags &= ~(ECHO | XTABS);
	ioctl(0, TIOCSETN, &rl_termio);

	ioctl(0, TIOCGETC, &s_tchars);
	term_chars[VERASE] = orig_termio.sg_erase;
	term_chars[VEOF] = s_tchars.t_eofc;
	term_chars[VKILL] = orig_termio.sg_kill;
#   ifdef TIOCGLTC
	ioctl(0, TIOCGLTC, &s_ltchars);
	term_chars[VWERASE] = s_ltchars.t_werasc;
	term_chars[VREPRINT] = s_ltchars.t_rprntc;
	term_chars[VSUSP] = s_ltchars.t_suspc;

	/* disable suspending process on ^Z */
	s_ltchars.t_suspc = 0;
	ioctl(0, TIOCSLTC, &s_ltchars);
#   endif			/* TIOCGLTC */
#  else				/* not SGTTY */
	rl_termio.c_iflag &= ~(BRKINT | PARMRK | INPCK | IUCLC | IXON | IXOFF);
	rl_termio.c_iflag |= (IGNBRK | IGNPAR);

	/* rl_termio.c_oflag &= ~(ONOCR); Costas Sphocleous Irvine,CA */

	rl_termio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | NOFLSH);
#   ifdef OS2
	/* for emx: remove default terminal processing */
	rl_termio.c_lflag &= ~(IDEFAULT);
#   endif			/* OS2 */
	rl_termio.c_lflag |= (ISIG);
	rl_termio.c_cc[VMIN] = 1;
	rl_termio.c_cc[VTIME] = 0;

#   ifndef VWERASE
#    define VWERASE 3
#   endif			/* VWERASE */
	term_chars[VERASE] = orig_termio.c_cc[VERASE];
	term_chars[VEOF] = orig_termio.c_cc[VEOF];
	term_chars[VKILL] = orig_termio.c_cc[VKILL];
#   ifdef TERMIOS
	term_chars[VWERASE] = orig_termio.c_cc[VWERASE];
#    ifdef VREPRINT
	term_chars[VREPRINT] = orig_termio.c_cc[VREPRINT];
#    else			/* not VREPRINT */
#     ifdef VRPRNT
	term_chars[VRPRNT] = orig_termio.c_cc[VRPRNT];
#     endif			/* VRPRNT */
#    endif			/* not VREPRINT */
	term_chars[VSUSP] = orig_termio.c_cc[VSUSP];

	/* disable suspending process on ^Z */
	rl_termio.c_cc[VSUSP] = 0;
#   endif			/* TERMIOS */
#  endif			/* not SGTTY */

	/*
	 * Set the new terminal modes.
	 */
#  ifdef SGTTY
	ioctl(0, TIOCSLTC, &s_ltchars);
#  else				/* not SGTTY */
#   ifdef TERMIOS
#    ifdef TCSETSW
	ioctl(0, TCSETSW, &rl_termio);
#    else			/* not TCSETSW */
	tcsetattr(0, TCSADRAIN, &rl_termio);
#    endif			/* not TCSETSW */
#   else			/* not TERMIOS */
	ioctl(0, TCSETAW, &rl_termio);
#   endif			/* not TERMIOS */
#  endif			/* not SGTTY */
	term_set = 1;
    }
#endif /* not MSDOS && not _WIN32 */
}

static void
reset_termio()
{
#if !defined(MSDOS) && !defined(_WIN32)
/* reset saved terminal modes */
    if (term_set == 1) {
#  ifdef SGTTY
	ioctl(0, TIOCSETN, &orig_termio);
#   ifdef TIOCGLTC
	/* enable suspending process on ^Z */
	s_ltchars.t_suspc = term_chars[VSUSP];
	ioctl(0, TIOCSLTC, &s_ltchars);
#   endif			/* TIOCGLTC */
#  else				/* not SGTTY */
#   ifdef TERMIOS
#    ifdef TCSETSW
	ioctl(0, TCSETSW, &orig_termio);
#    else			/* not TCSETSW */
	tcsetattr(0, TCSADRAIN, &orig_termio);
#    endif			/* not TCSETSW */
#   else			/* not TERMIOS */
	ioctl(0, TCSETAW, &orig_termio);
#   endif			/* TERMIOS */
#  endif			/* not SGTTY */
	term_set = 0;
    }
#endif /* not MSDOS && not _WIN32 */
}

#endif /* READLINE */
