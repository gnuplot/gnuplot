#ifndef lint
static char *RCSid() { return RCSid("$Id: readline.c,v 1.19.4.2 2000/07/26 18:52:58 broeker Exp $"); }
#endif

/* GNUPLOT - readline.c */

/*[
 * Copyright 1986 - 1993, 1998   Thomas Williams, Colin Kelley
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
 */

#include <signal.h>

#include "readline.h"

#include "alloc.h"
#include "gp_hist.h"
#include "plot.h"
#include "util.h"
#include "term_api.h"

#if defined(HAVE_LIBREADLINE)
/* #include <readline/readline.h> --- HBB 20000508: now included by readline.h*/
/* #include <readline/history.h> --- HBB 20000508: now included by gp_hist */


#if defined(USE_MOUSE) && !defined(OS2)
static char* line_buffer;
static int line_complete;

/**
 * called by libreadline if the input
 * was typed (not from the ipc).
 */
void
LineCompleteHandler(char* ptr)
{
    rl_callback_handler_remove();
    line_buffer = ptr;
    line_complete = 1;
}

int
getc_wrapper(FILE* fp /* should be stdin, supplied by readline */)
{
    if (term && term->waitforinput && interactive)
	return term->waitforinput();
    else
	return getc(fp);
}
#endif

#endif /* HAVE_LIBREADLINE */

char*
readline_ipc(const char* prompt)
{
#if defined(USE_MOUSE) && defined(HAVE_LIBREADLINE) && !defined(OS2)
    /* use readline's alternate interface.
     * this requires rl_library_version > 2.2
     * TODO: make a check in configure.in */

    rl_callback_handler_install((char*) prompt, LineCompleteHandler);
    rl_getc_function = getc_wrapper;

    for (line_complete = 0; !line_complete; /* EMPTY */) {
	rl_callback_read_char(); /* stdin */
    }
    return line_buffer;
#else
    return readline((const char*) prompt);
#endif
}


#if defined(READLINE) && !defined(HAVE_LIBREADLINE)

/* a small portable version of GNU's readline
 * this is not the BASH or GNU EMACS version of READLINE due to Copyleft 
 * restrictions
 * do not need any terminal capabilities except backspace,
 * and space overwrites a character
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
 * ^H and DEL delete the previous character
 * ^D deletes the current character, or EOF if line is empty
 * ^L/^R redraw line in case it gets trashed
 * ^U kills the entire line
 * ^W kills last word
 * LF and CR return the entire line regardless of the cursor postition
 * EOF with an empty line returns (char *)NULL
 *
 * all other characters are ignored
 */

#ifdef HAVE_SYS_IOCTL_H
/* For ioctl() prototype under Linux (and BeOS?) */
# include <sys/ioctl.h>
#endif

/* replaces the previous klugde in configure */
#if defined(HAVE_TERMIOS_H) && defined(HAVE_TCGETATTR)
# define TERMIOS
#else /* not HAVE_TERMIOS_H && HAVE_TCGETATTR */
# ifdef HAVE_SGTTY_H
#  define SGTTY
# endif
#endif /* not HAVE_TERMIOS_H && HAVE_TCGETATTR */

#if !defined(MSDOS) && !defined(ATARI) && !defined(MTOS) && !defined(_Windows) && !defined(DOS386) && !defined(OSK)

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
static int ansi_getc __PROTO((void));

#else /* MSDOS or ATARI or MTOS or _Windows or DOS386 or OSK */

# ifdef _Windows
#  include <windows.h>
#  include "win/wtext.h"
#  include "win/winmain.h"
#  define TEXTUSER 0xf1
#  define TEXTGNUPLOT 0xf0
#  define special_getc() msdos_getch()
static char msdos_getch __PROTO((void));	/* HBB 980308: PROTO'ed it */
# endif				/* _Windows */

# if defined(MSDOS) || defined(DOS386)
/* MSDOS specific stuff */
#  ifdef DJGPP
#   include <pc.h>
#  endif			/* DJGPP */
#  ifdef __EMX__
#   include <conio.h>
#  endif			/* __EMX__ */
#  define special_getc() msdos_getch()
static char msdos_getch();
# endif				/* MSDOS || DOS386 */

# ifdef OSK
#  include <sgstat.h>
#  include <modes.h>

#  define STDIN	0
static int term_set = 0;	/* =1 if new_settings is set */

static struct _sgs old_settings;	/* old terminal settings        */
static struct _sgs new_settings;	/* new terminal settings        */

#  define special_getc() ansi_getc()
static int ansi_getc __PROTO((void));

/* On OS9 a '\n' is a character 13 and '\r' == '\n'. This gives troubles
   here, so we need a new putc wich handles this correctly and print a
   character 10 on each place we want a '\n'.
 */
#  undef putc			/* Undefine the macro for putc */

static int
putc(c, fp)
char c;
FILE *fp;
{
    write(fileno(fp), &c, 1);
    if (c == '\012') {		/* A normal ASCII '\n' */
	c = '\r';
	write(fileno(fp), &c, 1);
    }
}

# endif				/* OSK */

# if defined(ATARI) || defined(MTOS)
#  define special_getc() tos_getch()
# endif				/* ATARI || MTOS */

#endif /* MSDOS or ATARI or MTOS or _Windows or DOS386 or OSK */

#ifdef OS2
# if defined( special_getc )
#  undef special_getc()
# endif				/* special_getc */
# define special_getc() msdos_getch()
static char msdos_getch __PROTO((void));	/* HBB 980308: PROTO'ed it */
#endif /* OS2 */


/* initial size and increment of input line length */
#define MAXBUF	1024
/* ^H */
#define BACKSPACE 0x08
#define SPACE	' '

#ifdef OSK
# define NEWLINE	'\012'
#else /* OSK */
# define NEWLINE	'\n'
#endif /* not OSK */

static char *cur_line;		/* current contents of the line */
static size_t line_len = 0;
static size_t cur_pos = 0;	/* current position of the cursor */
static size_t max_pos = 0;	/* maximum character position */

static void fix_line __PROTO((void));
static void redraw_line __PROTO((const char *prompt));
static void clear_line __PROTO((const char *prompt));
static void clear_eoline __PROTO((void));
static void copy_line __PROTO((char *line));
static void set_termio __PROTO((void));
static void reset_termio __PROTO((void));
static int ansi_getc __PROTO((void));
static int user_putc __PROTO((int ch));
static int user_puts __PROTO((char *str));
static void backspace __PROTO((void));
static void extend_cur_line __PROTO((void));


/* user_putc and user_puts should be used in the place of
 * fputc(ch,stderr) and fputs(str,stderr) for all output
 * of user typed characters.  This allows MS-Windows to 
 * display user input in a different color.
 */
static int
user_putc(ch)
int ch;
{
    int rv;
#ifdef _Windows
    TextAttr(&textwin, TEXTUSER);
#endif
    rv = fputc(ch, stderr);
#ifdef _Windows
    TextAttr(&textwin, TEXTGNUPLOT);
#endif
    return rv;
}

static int
user_puts(str)
char *str;
{
    int rv;
#ifdef _Windows
    TextAttr(&textwin, TEXTUSER);
#endif
    rv = fputs(str, stderr);
#ifdef _Windows
    TextAttr(&textwin, TEXTGNUPLOT);
#endif
    return rv;
}

/* This function provides a centralized non-destructive backspace capability
 * M. Castro
 */
static void
backspace()
{
    user_putc(BACKSPACE);
}

static void
extend_cur_line()
{
    char *new_line;

    /* extent input line length */
    new_line = gp_realloc(cur_line, line_len + MAXBUF, NULL);
    if (!new_line) {
	reset_termio();
	int_error(NO_CARET, "Can't extend readline length");
    }
    cur_line = new_line;
    line_len += MAXBUF;
    FPRINTF((stderr, "\nextending readline length to %d chars\n", line_len));
}

char *
readline(prompt)
const char *prompt;
{

    int cur_char;
    char *new_line;


    /* start with a string of MAXBUF chars */

    if (line_len != 0) {
	free(cur_line);
	line_len = 0;
    }
    cur_line = gp_alloc(MAXBUF, "readline");
    line_len = MAXBUF;

    /* set the termio so we can do our own input processing */
    set_termio();

    /* print the prompt */
    fputs(prompt, stderr);
    cur_line[0] = '\0';
    cur_pos = 0;
    max_pos = 0;
    cur_entry = NULL;

    /* get characters */
    for (;;) {

	cur_char = special_getc();

/*
 * The #define CHARSET7BIT should be used when one encounters problems with
 * 8bit characters that should not be entered on the commandline. I cannot
 * think on any reasonable example where this could happen, but what do I know?
 * After all, the unix world still ignores 8bit chars in most applications.
 *
 * Note that latin1 defines the chars 0x80-0x9f as control chars. For the
 * benefit of Atari, MSDOS, Windows and NeXT I have decided to ignore this,
 * since it would require more #ifs.
 *
 */

#ifdef CHARSET7BIT
	if (isprint(cur_char)) {
#else /* CHARSET7BIT */
	if (isprint(cur_char) || (((unsigned char) cur_char > 0x7f) &&
				  cur_char != EOF)) {
#endif /* CHARSET7BIT */
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
	    if (cur_pos < max_pos)
		fix_line();
	    cur_line[max_pos] = '\0';

	    /* else interpret unix terminal driver characters */
#ifdef VERASE
	} else if (cur_char == term_chars[VERASE]) {	/* DEL? */
	    if (cur_pos > 0) {
		size_t i;
		cur_pos -= 1;
		backspace();
		for (i = cur_pos; i < max_pos; i++)
		    cur_line[i] = cur_line[i + 1];
		max_pos -= 1;
		fix_line();
	    }
#endif /* VERASE */
#ifdef VEOF
	} else if (cur_char == term_chars[VEOF]) {	/* ^D? */
	    if (max_pos == 0) {
		reset_termio();
		return ((char *) NULL);
	    }
	    if ((cur_pos < max_pos) && (cur_char == 004)) {	/* ^D */
		size_t i;
		for (i = cur_pos; i < max_pos; i++)
		    cur_line[i] = cur_line[i + 1];
		max_pos -= 1;
		fix_line();
	    }
#endif /* VEOF */
#ifdef VKILL
	} else if (cur_char == term_chars[VKILL]) {	/* ^U? */
	    clear_line(prompt);
#endif /* VKILL */
#ifdef VWERASE
	} else if (cur_char == term_chars[VWERASE]) {	/* ^W? */
	    while ((cur_pos > 0) &&
		   (cur_line[cur_pos - 1] == SPACE)) {
		cur_pos -= 1;
		backspace();
	    }
	    while ((cur_pos > 0) &&
		   (cur_line[cur_pos - 1] != SPACE)) {
		cur_pos -= 1;
		backspace();
	    }
	    clear_eoline();
	    max_pos = cur_pos;
#endif /* VWERASE */
#ifdef VREPRINT
	} else if (cur_char == term_chars[VREPRINT]) {	/* ^R? */
	    putc(NEWLINE, stderr);	/* go to a fresh line */
	    redraw_line(prompt);
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
	    size_t i;
	    switch (cur_char) {
	    case EOF:
		reset_termio();
		return ((char *) NULL);
	    case 001:		/* ^A */
		while (cur_pos > 0) {
		    cur_pos -= 1;
		    backspace();
		}
		break;
	    case 002:		/* ^B */
		if (cur_pos > 0) {
		    cur_pos -= 1;
		    backspace();
		}
		break;
	    case 005:		/* ^E */
		while (cur_pos < max_pos) {
		    user_putc(cur_line[cur_pos]);
		    cur_pos += 1;
		}
		break;
	    case 006:		/* ^F */
		if (cur_pos < max_pos) {
		    user_putc(cur_line[cur_pos]);
		    cur_pos += 1;
		}
		break;
	    case 013:		/* ^K */
		clear_eoline();
		max_pos = cur_pos;
		break;
	    case 020:		/* ^P */
		if (history != NULL) {
		    if (cur_entry == NULL) {
			cur_entry = history;
			clear_line(prompt);
			copy_line(cur_entry->line);
		    } else if (cur_entry->prev != NULL) {
			cur_entry = cur_entry->prev;
			clear_line(prompt);
			copy_line(cur_entry->line);
		    }
		}
		break;
	    case 016:		/* ^N */
		if (cur_entry != NULL) {
		    cur_entry = cur_entry->next;
		    clear_line(prompt);
		    if (cur_entry != NULL)
			copy_line(cur_entry->line);
		    else
			cur_pos = max_pos = 0;
		}
		break;
	    case 014:		/* ^L */
	    case 022:		/* ^R */
		putc(NEWLINE, stderr);	/* go to a fresh line */
		redraw_line(prompt);
		break;
	    case 0177:		/* DEL */
	    case 010:		/* ^H */
		if (cur_pos > 0) {
		    cur_pos -= 1;
		    backspace();
		    for (i = cur_pos; i < max_pos; i++)
			cur_line[i] = cur_line[i + 1];
		    max_pos -= 1;
		    fix_line();
		}
		break;
	    case 004:		/* ^D */
		if (max_pos == 0) {
		    reset_termio();
		    return ((char *) NULL);
		}
		if (cur_pos < max_pos) {
		    for (i = cur_pos; i < max_pos; i++)
			cur_line[i] = cur_line[i + 1];
		    max_pos -= 1;
		    fix_line();
		}
		break;
	    case 025:		/* ^U */
		clear_line(prompt);
		break;
	    case 027:		/* ^W */
		while ((cur_pos > 0) &&
		       (cur_line[cur_pos - 1] == SPACE)) {
		    cur_pos -= 1;
		    backspace();
		}
		while ((cur_pos > 0) &&
		       (cur_line[cur_pos - 1] != SPACE)) {
		    cur_pos -= 1;
		    backspace();
		}
		clear_eoline();
		max_pos = cur_pos;
		break;
	    case '\n':		/* ^J */
#ifndef OSK
	    case '\r':		/* ^M */
#endif
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
    }
}

/* fix up the line from cur_pos to max_pos
 * do not need any terminal capabilities except backspace,
 * and space overwrites a character
 */
static void
fix_line()
{
    size_t i;

    /* write tail of string */
    for (i = cur_pos; i < max_pos; i++)
	user_putc(cur_line[i]);

    /* write a space at the end of the line in case we deleted one */
    user_putc(SPACE);

    /* backup to original position */
    for (i = max_pos + 1; i > cur_pos; i--)
	backspace();

}

/* redraw the entire line, putting the cursor where it belongs */
static void
redraw_line(prompt)
const char *prompt;
{
    size_t i;

    fputs(prompt, stderr);
    user_puts(cur_line);

    /* put the cursor where it belongs */
    for (i = max_pos; i > cur_pos; i--)
	backspace();
}

/* clear cur_line and the screen line */
static void
clear_line(prompt)
const char *prompt;
{
    size_t i;
    for (i = 0; i < max_pos; i++)
	cur_line[i] = '\0';

    for (i = cur_pos; i > 0; i--)
	backspace();

    for (i = 0; i < max_pos; i++)
	putc(SPACE, stderr);

    putc('\r', stderr);
    fputs(prompt, stderr);

    cur_pos = 0;
    max_pos = 0;
}

/* clear to end of line and the screen end of line */
static void
clear_eoline()
{
    size_t i;
    for (i = cur_pos; i < max_pos; i++)
	cur_line[i] = '\0';

    for (i = cur_pos; i < max_pos; i++)
	putc(SPACE, stderr);
    for (i = cur_pos; i < max_pos; i++)
	backspace();
}

/* copy line to cur_line, draw it and set cur_pos and max_pos */
static void
copy_line(line)
char *line;
{
    while (strlen(line) + 1 > line_len) {
	extend_cur_line();
    }
    strcpy(cur_line, line);
    user_puts(cur_line);
    cur_pos = max_pos = strlen(cur_line);
}

/* Convert ANSI arrow keys to control characters */
static int
ansi_getc()
{
    int c;

#ifdef USE_MOUSE
    if (term && term->waitforinput && interactive)
	c = term->waitforinput();
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
	    }
	}
    }
    return c;
}

#if defined(MSDOS) || defined(_Windows) || defined(DOS386) || defined(OS2)

/* Convert Arrow keystrokes to Control characters: */
static char
msdos_getch()
{
#ifdef DJGPP
    char c;
    int ch = getkey();
    c = (ch & 0xff00) ? 0 : ch & 0xff;
#else /* not DJGPP */
# ifdef OS2
    char c = getc(stdin);
# else				/* not OS2 */
    char c = getch();
# endif				/* not OS2 */
#endif /* not DJGPP */

    if (c == 0) {
#ifdef DJGPP
	c = ch & 0xff;
#else /* not DJGPP */
# ifdef OS2
	c = getc(stdin);
# else				/* not OS2 */
	c = getch();		/* Get the extended code. */
# endif				/* not OS2 */
#endif /* not DJGPP */
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
	    c = 004;
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

#endif /* MSDOS || _Windows || DOS386 || OS2 */


#if defined(ATARI) || defined(MTOS)

/* Convert Arrow keystrokes to Control characters: TOS version */

long poll_events(int);		/* from term/atariaes.trm */

/* this function is used in help.c as well. this means that the
 * program doesn't work without -DREADLINE (which would be the case
 * if help.c didn't use it as well, since no events would be processed)
 */
char
tos_getch()
{
    long rawkey;
    char c;
    int scan_code;
    static int in_help = 0;

    if (strcmp(term->name, "atari") == 0)
	poll_events(0);

    if (in_help) {
	switch (in_help) {
	case 1:
	case 5:
	    in_help++;
	    return 'e';
	case 2:
	case 6:
	    in_help++;
	    return 'l';
	case 3:
	case 7:
	    in_help++;
	    return 'p';
	case 4:
	    in_help = 0;
	    return 0x0d;
	case 8:
	    in_help = 0;
	    return ' ';
	}
    }
    if (strcmp(term->name, "atari") == 0) {
	do {
	    if (Bconstat(2))
		rawkey = Cnecin();
	    else
		rawkey = poll_events(1);
	} while (rawkey == 0);
    } else
	rawkey = Cnecin();

    c = (char) rawkey;
    scan_code = ((int) (rawkey >> 16)) & 0xff;	/* get the scancode */
    if (Kbshift(-1) & 0x00000007)
	scan_code |= 0x80;	/* shift or control ? */

    switch (scan_code) {
    case 0x62:			/* HELP         */
    case 0xe2:			/* shift HELP   */
	if (max_pos == 0) {
	    if (scan_code == 0x62) {
		in_help = 1;
	    } else {
		in_help = 5;
	    }
	    return 'h';
	} else {
	    return 0;
	}
    case 0x48:			/* Up Arrow */
	return 0x10;		/* ^P */
    case 0x50:			/* Down Arrow */
	return 0x0e;		/* ^N */
    case 0x4b:			/* Left Arrow */
	return 0x02;		/* ^B */
    case 0x4d:			/* Right Arrow */
	return 0x06;		/* ^F */
    case 0xcb:			/* Shift Left Arrow */
    case 0xf3:			/* Ctrl Left Arrow (TOS-bug ?) */
    case 0x47:			/* Home */
	return 0x01;		/* ^A */
    case 0xcd:			/* Shift Right Arrow */
    case 0xf4:			/* Ctrl Right Arrow (TOS-bug ?) */
    case 0xc7:			/* Shift Home */
    case 0xf7:			/* Ctrl Home */
	return 0x05;		/* ^E */
    case 0x61:			/* Undo - redraw line */
	return 0x0c;		/* ^L */
    default:
	if (c == 0x1b)
	    return 0x15;	/* ESC becomes ^U */
	if (c == 0x7f)
	    return 0x04;	/* Del becomes ^D */
	break;
    }
    return c;
}

#endif /* ATARI || MTOS */

  /* set termio so we can do our own input processing */
static void
set_termio()
{
#if !defined(MSDOS) && !defined(ATARI) && !defined(MTOS) && !defined(_Windows) && !defined(DOS386)
/* set termio so we can do our own input processing */
/* and save the old terminal modes so we can reset them later */
    if (term_set == 0) {
	/*
	 * Get terminal modes.
	 */
# ifndef OSK
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
# else				/* OSK */
	setbuf(stdin, (char *) 0);	/* Make stdin and stdout unbuffered */
	setbuf(stderr, (char *) 0);
	_gs_opt(STDIN, &new_settings);
# endif				/* OSK */

	/*
	 * Save terminal modes
	 */
# ifndef OSK
	rl_termio = orig_termio;
# else				/* OSK */
	_gs_opt(STDIN, &old_settings);
# endif				/* OSK */

	/*
	 * Set the modes to the way we want them
	 *  and save our input special characters
	 */
# ifndef OSK
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
# else				/* OSK */
	new_settings._sgs_echo = 0;	/* switch off terminal echo */
	new_settings._sgs_pause = 0;	/* inhibit page pause */
	new_settings._sgs_eofch = 0;	/* inhibit eof  */
	new_settings._sgs_kbich = 0;	/* inhibit ^C   */
	new_settings._sgs_kbach = 0;	/* inhibit ^E   */
# endif				/* OSK */

	/*
	 * Set the new terminal modes.
	 */
# ifndef OSK
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
# else				/* OSK */
	_ss_opt(STDIN, &new_settings);
# endif				/* OSK */
	term_set = 1;
    }
#endif /* not MSDOS && not ATARI && not MTOS && not _Windows && not DOS386 */
}

static void
reset_termio()
{
#if !defined(MSDOS) && !defined(ATARI) && !defined(MTOS) && !defined(_Windows) && !defined(DOS386)
/* reset saved terminal modes */
    if (term_set == 1) {
# ifndef OSK
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
# else				/* OSK */
	_ss_opt(STDIN, &old_settings);
# endif				/* OSK */
	term_set = 0;
    }
#endif /* not MSDOS && not ATARI && not MTOS && not _Windows && not DOS386 */
}

#endif /* READLINE && !HAVE_LIBREADLINE */
