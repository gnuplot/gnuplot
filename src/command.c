#ifndef lint
static char *RCSid() { return RCSid("$Id: command.c,v 1.14 1999/06/14 19:20:58 lhecking Exp $"); }
#endif

/* GNUPLOT - command.c */

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
 * Changes:
 * 
 * Feb 5, 1992  Jack Veenstra   (veenstra@cs.rochester.edu) Added support to
 * filter data values read from a file through a user-defined function before
 * plotting. The keyword "thru" was added to the "plot" command. Example
 * syntax: f(x) = x / 100 plot "test.data" thru f(x) This example divides all
 * the y values by 100 before plotting. The filter function processes the
 * data before any log-scaling occurs. This capability should be generalized
 * to filter x values as well and a similar feature should be added to the
 * "splot" command.
 * 
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * April 1999 Franz Bakan (bakan@ukezyk.desy.de)
 * Added code to support mouse-input from OS/2 PM window
 * Works with gnuplot's readline routine, it does not work with GNU readline
 * Changes marked by USE_MOUSE
 * May 1999, update by Petr Mikulik: use gnuplot's pid in share mem name
 *
 */

#include "plot.h"
#include "setshow.h"
#include "fit.h"
#include "binary.h"

/* GNU readline
 * Only required by two files directly,
 * so I don't put this into a header file. -lh
 */
#ifdef HAVE_LIBREADLINE
# include <readline/readline.h>
# include <readline/history.h>
#endif

#ifdef OS2
extern int PM_pause(char *);            /* term/pm.trm */
extern int ExecuteMacro(char *, int);   /* plot.c */
extern TBOOLEAN CallFromRexx;           /* plot.c */
# ifdef USE_MOUSE
PVOID input_from_PM_Terminal = NULL;
char mouseShareMemName[40] = "";
#  define INCL_DOSMEMMGR
#  include <os2.h>
# endif /* USE_MOUSE */
#endif /* OS2 */

#ifndef _Windows
# include "help.h"
#else
static int winsystem __PROTO((char *));
#endif /* _Windows */

#ifdef _Windows
# include <windows.h>
# ifdef __MSC__
#  include <malloc.h>
# else
#  include <alloc.h>
#  include <dir.h>		/* setdisk() */
# endif				/* !MSC */
# include "win/wgnuplib.h"
extern TW textwin;
extern LPSTR winhelpname;
extern void screen_dump(void);	/* in term/win.trm */
extern int Pause(LPSTR mess);	/* in winmain.c */
#endif /* _Windows */

#ifdef VMS
int vms_vkid;			/* Virtual keyboard id */
int vms_ktid;			/* key table id, for translating keystrokes */
#endif /* VMS */

/* static prototypes */
static int command __PROTO((void));
static int read_line __PROTO((const char *prompt));
static void do_shell __PROTO((void));
static void do_help __PROTO((int toplevel));
static void do_system __PROTO((void));
static int changedir __PROTO((char *path));
#ifdef AMIGA_AC_5
static void getparms __PROTO((char *, char **));
#endif

struct lexical_unit *token;
int token_table_size;

/* TRUE if command just typed; becomes FALSE whenever we
 * send some other output to screen.  If FALSE, the command line
 * will be echoed to the screen before the ^ error message.
 */
TBOOLEAN screen_ok;

char *input_line;
size_t input_line_len;
int inline_num;			/* input line number */

/* jev -- for passing data thru user-defined function */
/* NULL means no dummy vars active */
struct udft_entry ydata_func;

struct udft_entry *dummy_func;

/* current dummy vars */
char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];

/* support for replot command */
char *replot_line;
int plot_token;			/* start of 'plot' command */

/* If last plot was a 3d one. */
TBOOLEAN is_3d_plot = FALSE;

/* input data, parsing variables */
#ifdef AMIGA_SC_6_1
__far int num_tokens, c_token;
#else
int num_tokens, c_token;
#endif

/* support for dynamic size of input line */
void
extend_input_line()
{
    if (input_line_len == 0) {
	/* first time */
	input_line = gp_alloc(MAX_LINE_LEN, "input_line");
	input_line_len = MAX_LINE_LEN;
	input_line[0] = NUL;

#if defined(USE_MOUSE) && defined(OS2)
	sprintf( mouseShareMemName, "\\SHAREMEM\\GP%i_Mouse_Input", getpid() );
	if (DosAllocSharedMem((PVOID) & input_from_PM_Terminal,
			      mouseShareMemName,
			      MAX_LINE_LEN,
			      PAG_WRITE | PAG_COMMIT))
	    fputs("command.c: DosAllocSharedMem ERROR\n",stderr);
#endif /* USE_MOUSE && OS2 */

    } else {
	input_line = gp_realloc(input_line, input_line_len + MAX_LINE_LEN, "extend input line");
	input_line_len += MAX_LINE_LEN;
	FPRINTF((stderr, "extending input line to %d chars\n", input_line_len));
    }
}


void
extend_token_table()
{
    if (token_table_size == 0) {
	/* first time */
	token = (struct lexical_unit *) gp_alloc(MAX_TOKENS * sizeof(struct lexical_unit), "token table");
	token_table_size = MAX_TOKENS;
    } else {
	token = gp_realloc(token, (token_table_size + MAX_TOKENS) * sizeof(struct lexical_unit), "extend token table");
	token_table_size += MAX_TOKENS;
	FPRINTF((stderr, "extending token table to %d elements\n", token_table_size));
    }
}


void
init_memory()
{
    extend_input_line();
    extend_token_table();
    replot_line = gp_alloc(1, "string");
    *replot_line = NUL;
}


int
com_line()
{
    if (multiplot) {
	/* calls int_error() if it is not happy */
	term_check_multiplot_okay(interactive);

	if (read_line("multiplot> "))
	    return (1);
    } else {
	if (read_line(PROMPT))
	    return (1);
    }

    /* So we can flag any new output: if false at time of error,
     * we reprint the command line before printing caret.
     * TRUE for interactive terminals, since the command line is typed.
     * FALSE for non-terminal stdin, so command line is printed anyway.
     * (DFK 11/89)
     */
    screen_ok = interactive;

    if (do_line())
	return (1);
    else
	return (0);
}


int
do_line()
{
    /* Line continuation has already been handled
     * by read_line() */
    char *inlptr = input_line;

    /* Skip leading whitespace */
    while (isspace((int) *inlptr))
	inlptr++;

    if (inlptr != input_line) {
	/* If there was leading whitespace, copy the actual
	 * command string to the front. use memmove() because
	 * source and target may overlap */
	memmove(input_line, inlptr, strlen(inlptr));
	/* Terminate resulting string */
	input_line[strlen(inlptr)] = NUL;
    }
    FPRINTF((stderr, "Input line: \"%s\"\n", input_line));

    /* also used in load_file */
    if (is_system(input_line[0])) {
	do_system();
	if (interactive)	/* 3.5 did it unconditionally */
	    (void) fputs("!\n", stderr);	/* why do we need this ? */
	return (0);
    }

    num_tokens = scanner(&input_line, &input_line_len);
    c_token = 0;
    while (c_token < num_tokens) {
	if (command())
	    return (1);
	if (c_token < num_tokens) {	/* something after command */
	    if (equals(c_token, ";"))
		c_token++;
	    else
		int_error(c_token, "';' expected");
	}
    }
    return (0);
}


void
define()
{
    register int start_token;	/* the 1st token in the function definition */
    register struct udvt_entry *udv;
    register struct udft_entry *udf;

    if (equals(c_token + 1, "(")) {
	/* function ! */
	int dummy_num = 0;
	struct at_type *at_tmp;
	char save_dummy[MAX_NUM_VAR][MAX_ID_LEN+1];
	memcpy(save_dummy, c_dummy_var, sizeof(save_dummy));
	start_token = c_token;
	do {
	    c_token += 2;	/* skip to the next dummy */
	    copy_str(c_dummy_var[dummy_num++], c_token, MAX_ID_LEN);
	} while (equals(c_token + 1, ",") && (dummy_num < MAX_NUM_VAR));
	if (equals(c_token + 1, ","))
	    int_error(c_token + 2, "function contains too many parameters");
	c_token += 3;		/* skip (, dummy, ) and = */
	if (END_OF_COMMAND)
	    int_error(c_token, "function definition expected");
	udf = dummy_func = add_udf(start_token);
	if ((at_tmp = perm_at()) == (struct at_type *) NULL)
	    int_error(start_token, "not enough memory for function");
	if (udf->at)		/* already a dynamic a.t. there */
	    free((char *) udf->at);	/* so free it first */
	udf->at = at_tmp;	/* before re-assigning it. */
	memcpy(c_dummy_var, save_dummy, sizeof(save_dummy));
	m_capture(&(udf->definition), start_token, c_token - 1);
	dummy_func = NULL;	/* dont let anyone else use our workspace */
    } else {
	/* variable ! */
	start_token = c_token;
	c_token += 2;
	udv = add_udv(start_token);
	(void) const_express(&(udv->udv_value));
	udv->udv_undef = FALSE;
    }
}


static int
command()
{
    FILE *fp;
    int i;
    char *sv_file = NULL;

    for (i = 0; i < MAX_NUM_VAR; i++)
	c_dummy_var[i][0] = NUL;	/* no dummy variables */

    if (is_definition(c_token))
	define();
    else if (almost_equals(c_token, "h$elp") || equals(c_token, "?")) {
	c_token++;
	do_help(1);
    } else if (equals(c_token, "testtime")) {
	/* given a format and a time string, exercise the time code */
	char *format = NULL, *string = NULL;
	struct tm tm;
	double secs;
	if (isstring(++c_token)) {
	    m_quote_capture(&format, c_token, c_token);
	    if (isstring(++c_token)) {
		m_quote_capture(&string, c_token, c_token);
		memset(&tm, 0, sizeof(tm));
		gstrptime(string, format, &tm);
		secs = gtimegm(&tm);
		fprintf(stderr, "internal = %f - %d/%d/%d::%d:%d:%d , wday=%d, yday=%d\n",
			secs, tm.tm_mday, tm.tm_mon + 1, tm.tm_year,
			tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_wday,
			tm.tm_yday);
		memset(&tm, 0, sizeof(tm));
		ggmtime(&tm, secs);
		gstrftime(string, 159, format, secs);
		fprintf(stderr, "convert back \"%s\" - %d/%d/%d::%d:%d:%d , wday=%d, yday=%d\n",
			string, tm.tm_mday, tm.tm_mon + 1, tm.tm_year,
			tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_wday,
			tm.tm_yday);
		free(string);
		++c_token;
	    }
	    free(format);
	}
    } else if (almost_equals(c_token, "test")) {
	c_token++;
	test_term();
    } else if (almost_equals(c_token, "scr$eendump")) {
	c_token++;
#ifdef _Windows
	screen_dump();
#else
	fputs("screendump not implemented\n", stderr);
#endif
    } else if (almost_equals(c_token, "pa$use")) {
	struct value a;
	int sleep_time, text = 0;
	char buf[MAX_LINE_LEN+1];

	c_token++;
	sleep_time = (int) real(const_express(&a));
	buf[0] = NUL;
	if (!(END_OF_COMMAND)) {
	    if (!isstring(c_token))
		int_error(c_token, "expecting string");
	    else {
		quote_str(buf, c_token, MAX_LINE_LEN);
		++c_token;
#ifdef _Windows
		if (sleep_time >= 0)
#elif defined(OS2)
		if (strcmp(term->name, "pm") != 0 || sleep_time >= 0)
#elif defined(MTOS)
		    if (strcmp(term->name, "mtos") != 0 || sleep_time >= 0)
#endif /* _Windows */
			fputs(buf, stderr);
		text = 1;
	    }
	}
	if (sleep_time < 0) {
#ifdef _Windows
	    if (!Pause(buf))
		bail_to_command_line();
#elif defined(OS2)
	    if (strcmp(term->name, "pm") == 0 && sleep_time < 0) {
		int rc;
		if ((rc = PM_pause(buf)) == 0) {
		    /* if (!CallFromRexx)
		     * would help to stop REXX programs w/o raising an error message
		     * in RexxInterface() ...
		     */
		    bail_to_command_line();
		} else if (rc == 2) {
		    fputs(buf, stderr);
		    text = 1;
		    (void) fgets(buf, MAX_LINE_LEN, stdin);
		}
	    }
#elif defined(_Macintosh)
	    if (strcmp(term->name, "macintosh") == 0 && sleep_time < 0)
		Pause(sleep_time);
#elif defined(MTOS)
	    if (strcmp(term->name, "mtos") == 0) {
		int MTOS_pause(char *buf);
		int rc;
		if ((rc = MTOS_pause(buf)) == 0)
		    bail_to_command_line();
		else if (rc == 2) {
		    fputs(buf, stderr);
		    text = 1;
		    (void) fgets(buf, MAX_LINE_LEN, stdin);
		}
	    } else if (strcmp(term->name, "atari") == 0) {
		char *readline(char *);
		char *line = readline("");
		if (line)
		    free(line);
	    } else
		(void) fgets(buf, MAX_LINE_LEN, stdin);
#elif defined(ATARI)
	    if (strcmp(term->name, "atari") == 0) {
		char *readline(char *);
		char *line = readline("");
		if (line)
		    free(line);
	    } else
		(void) fgets(buf, MAX_LINE_LEN, stdin);
#else /* !(_Windows || OS2 || _Macintosh || MTOS || ATARI) */
	    (void) fgets(buf, MAX_LINE_LEN, stdin);
	    /* Hold until CR hit. */
#endif
	}
	if (sleep_time > 0)
	    GP_SLEEP(sleep_time);

	if (text != 0 && sleep_time >= 0)
	    fputc('\n', stderr);
	screen_ok = FALSE;
    } else if (almost_equals(c_token, "pr$int")) {
	int need_space = 0;	/* space printed between two expressions only */
	screen_ok = FALSE;
	do {
	    ++c_token;
	    if (isstring(c_token)) {
		char *s = NULL;
		m_quote_capture(&s, c_token, c_token);
		fputs(s, stderr);
		need_space = 0;
		free(s);
		++c_token;
	    } else {
		struct value a;
		(void) const_express(&a);
		if (need_space)
		    putc(' ', stderr);
		need_space = 1;
		disp_value(stderr, &a);
	    }
	} while (!END_OF_COMMAND && equals(c_token, ","));

	(void) putc('\n', stderr);
    } else if (almost_equals(c_token, "fit")) {
	++c_token;
	do_fit();
    } else if (almost_equals(c_token, "up$date")) {
	char *opfname = NULL;  /* old parameter filename */
	char *npfname = NULL;  /* new parameter filename */

	if (!isstring(++c_token))
	    int_error(c_token, "Parameter filename expected");
	m_quote_capture(&opfname, c_token, c_token);
	++c_token;
	if (!(END_OF_COMMAND)) {
	    if (!isstring(c_token))
		int_error(c_token, "New parameter filename expected");
	    else {
		m_quote_capture(&npfname, c_token, c_token);
		++c_token;
	    }
	}
	update(opfname, npfname);
	free(npfname);
	free(opfname);
    } else if (almost_equals(c_token, "p$lot")) {
	plot_token = c_token++;
	SET_CURSOR_WAIT;
	plotrequest();
	SET_CURSOR_ARROW;
    } else if (almost_equals(c_token, "sp$lot")) {
	plot_token = c_token++;
	SET_CURSOR_WAIT;
	plot3drequest();
	SET_CURSOR_ARROW;
    } else if (almost_equals(c_token, "rep$lot")) {
	if (replot_line[0] == NUL)
	    int_error(c_token, "no previous plot");
	c_token++;
	SET_CURSOR_WAIT;
	replotrequest();
	SET_CURSOR_ARROW;
    } else if (almost_equals(c_token, "se$t"))
	set_command();
    else if (almost_equals(c_token, "res$et"))
	reset_command();
    else if (almost_equals(c_token, "sh$ow"))
	show_command();
    else if (almost_equals(c_token, "cl$ear")) {
	term_start_plot();

	if (multiplot && term->fillbox) {
	    unsigned int x1 = (unsigned int) (xoffset * term->xmax);
	    unsigned int y1 = (unsigned int) (yoffset * term->ymax);
	    unsigned int width = (unsigned int) (xsize * term->xmax);
	    unsigned int height = (unsigned int) (ysize * term->ymax);
	    (*term->fillbox) (0, x1, y1, width, height);
	}
	term_end_plot();

	screen_ok = FALSE;
	c_token++;
    } else if (almost_equals(c_token, "she$ll")) {
	do_shell();
	screen_ok = FALSE;
	c_token++;
    } else if (almost_equals(c_token, "sa$ve")) {
	if (almost_equals(++c_token, "f$unctions")) {
	    if (!isstring(++c_token))
		int_error(c_token, "expecting filename");
	    else {
		m_quote_capture(&sv_file, c_token, c_token);
		gp_expand_tilde(&sv_file);
		save_functions(fopen(sv_file, "w"));
	    }
	} else if (almost_equals(c_token, "v$ariables")) {
	    if (!isstring(++c_token))
		int_error(c_token, "expecting filename");
	    else {
		m_quote_capture(&sv_file, c_token, c_token);
		gp_expand_tilde(&sv_file);
		save_variables(fopen(sv_file, "w"));
	    }
	} else if (almost_equals(c_token, "s$et")) {
	    if (!isstring(++c_token))
		int_error(c_token, "expecting filename");
	    else {
		m_quote_capture(&sv_file, c_token, c_token);
		gp_expand_tilde(&sv_file);
		save_set(fopen(sv_file, "w"));
	    }
	} else if (isstring(c_token)) {
	    m_quote_capture(&sv_file, c_token, c_token);
	    gp_expand_tilde(&sv_file);
	    save_all(fopen(sv_file, "w"));
	} else {
	    int_error(c_token, "filename or keyword 'functions', 'variables', or 'set' expected");
	}
	c_token++;
    } else if (almost_equals(c_token, "l$oad")) {
	if (!isstring(++c_token))
	    int_error(c_token, "expecting filename");
	else {
	    m_quote_capture(&sv_file, c_token, c_token);
	    gp_expand_tilde(&sv_file);
	    /* load_file(fp=fopen(sv_file, "r"), sv_file, FALSE); OLD
	     * DBT 10/6/98 handle stdin as special case
	     * passes it on to load_file() so that it gets
	     * pushed on the stack and recusion will work, etc
	     */
	    fp = strcmp(sv_file, "-") ? loadpath_fopen(sv_file, "r") : stdin;
	    load_file(fp, sv_file, FALSE);
	    /* input_line[] and token[] now destroyed! */
	    c_token = num_tokens = 0;
	}
    } else if (almost_equals(c_token, "ca$ll")) {
	if (!isstring(++c_token))
	    int_error(c_token, "expecting filename");
	else {
	    m_quote_capture(&sv_file, c_token, c_token);
	    gp_expand_tilde(&sv_file);
	    /* Argument list follows filename */
	    load_file(loadpath_fopen(sv_file, "r"), sv_file, TRUE);
	    /* input_line[] and token[] now destroyed! */
	    c_token = num_tokens = 0;
	}
    } else if (almost_equals(c_token, "if")) {
	double exprval;
	struct value t;
	if (!equals(++c_token, "("))	/* no expression */
	    int_error(c_token, "expecting (expression)");
	exprval = real(const_express(&t));
	if (exprval != 0.0) {
	    /* fake the condition of a ';' between commands */
	    int eolpos = token[num_tokens - 1].start_index + token[num_tokens - 1].length;
	    --c_token;
	    token[c_token].length = 1;
	    token[c_token].start_index = eolpos + 2;
	    input_line[eolpos + 2] = ';';
	    input_line[eolpos + 3] = NUL;
	} else
	    c_token = num_tokens = 0;
    } else if (almost_equals(c_token, "rer$ead")) {
	fp = lf_top();
	if (fp != (FILE *) NULL)
	    rewind(fp);
	c_token++;
    } else if (almost_equals(c_token, "cd")) {
	if (!isstring(++c_token))
	    int_error(c_token, "expecting directory name");
	else {
	    m_quote_capture(&sv_file, c_token, c_token);
	    gp_expand_tilde(&sv_file);
	    if (changedir(sv_file)) {
		int_error(c_token, "Can't change to this directory");
	    }
	    c_token++;
	}
    } else if (almost_equals(c_token, "pwd")) {
	sv_file = (char *) gp_alloc(PATH_MAX, "print current dir");
	if (sv_file) {
	    GP_GETCWD(sv_file, PATH_MAX);
	    fprintf(stderr, "%s\n", sv_file);
	    free(sv_file);
	    sv_file = NULL;
	}
	c_token++;
    } else if (almost_equals(c_token, "ex$it") ||
	       almost_equals(c_token, "q$uit")) {
	/* graphics will be tidied up in main */
	return (1);
#if 0
    } else if(equals_alias(c_token) == 0) {
	/* expand alias; set num_tokens to 0; reprocess */
#endif
    } else if (!equals(c_token, ";")) {		/* null statement */
#ifdef OS2
	if (_osmode == OS2_MODE) {
	    if (token[c_token].is_token) {
		int rc;
		rc = ExecuteMacro(input_line + token[c_token].start_index,
				  token[c_token].length);
		if (rc == 0) {
		    c_token = num_tokens = 0;
		    return (0);
		}
	    }
	}
#endif
	int_error(c_token, "invalid command");
    }
    if (sv_file)
	free(sv_file);

    return (0);
}


void
done(status)
int status;
{
    term_reset();
    exit(status);
}


static int
changedir(path)
char *path;
{
#if defined(MSDOS) || defined(WIN16) || defined(ATARI) || defined(DOS386)
# if defined(__ZTC__)
    unsigned dummy;		/* it's a parameter needed for dos_setdrive */
# endif

    /* first deal with drive letter */

    if (isalpha(path[0]) && (path[1] == ':')) {
	int driveno = toupper(path[0]) - 'A';	/* 0=A, 1=B, ... */

# if defined(ATARI)
	(void) Dsetdrv(driveno);
# endif

# if defined(__ZTC__)
	(void) dos_setdrive(driveno + 1, &dummy);
# endif

# if (defined(MSDOS) && defined(__EMX__)) || defined(__MSC__)
	(void) _chdrive(driveno + 1);
# endif


/* HBB: recent versions of DJGPP also have setdisk():,
 * so I del'ed the special code */
# if ((defined(MSDOS) || defined(_Windows)) && defined(__TURBOC__)) || defined(DJGPP)
	(void) setdisk(driveno);
# endif
	path += 2;		/* move past drive letter */
    }
    /* then change to actual directory */
    if (*path)
	if (chdir(path))
	    return 1;

    return 0;			/* should report error with setdrive also */

#elif defined(WIN32)
    return !(SetCurrentDirectory(path));
#elif defined(__EMX__) && defined(OS2)
    return _chdir2(path);
#else
    return chdir(path);
#endif /* MSDOS, ATARI etc. */
}


void
replotrequest()
{
    if (equals(c_token, "["))
	int_error(c_token, "cannot set range with replot");

    /* do not store directly into the replot_line string, until the
     * new plot line has been successfully plotted. This way,
     * if user makes a typo in a replot line, they do not have
     * to start from scratch. The replot_line will be committed
     * after do_plot has returned, whence we know all is well
     */
    if (END_OF_COMMAND) {
	char *rest_args = &input_line[token[c_token].start_index];
	size_t replot_len = strlen(replot_line);
	size_t rest_len = strlen(rest_args);

	/* preserve commands following 'replot ;' */
	/* move rest of input line to the start
	 * necessary because of realloc() in extend_input_line() */
	memmove(input_line,rest_args,rest_len+1);
	/* reallocs if necessary */
	while (input_line_len < replot_len+rest_len+1)
	    extend_input_line();
	/* move old rest args off begin of input line to
	 * make space for replot_line */
	memmove(input_line+replot_len,input_line,rest_len+1);
	/* copy previous plot command to start of input line */
	strncpy(input_line, replot_line, replot_len);
    } else {
	char *replot_args = NULL;	/* else m_capture will free it */
	int last_token = num_tokens - 1;

	/* length = length of old part + length of new part + ',' + \0 */
	size_t newlen = strlen(replot_line) + token[last_token].start_index +
	token[last_token].length - token[c_token].start_index + 2;

	m_capture(&replot_args, c_token, last_token);	/* might be empty */
	while (input_line_len < newlen)
	    extend_input_line();
	strcpy(input_line, replot_line);
	strcat(input_line, ",");
	strcat(input_line, replot_args);
	free(replot_args);
    }
    plot_token = 0;		/* whole line to be saved as replot line */

    screen_ok = FALSE;
    num_tokens = scanner(&input_line, &input_line_len);
    c_token = 1;		/* skip the 'plot' part */
    if (is_3d_plot)
	plot3drequest();
    else
	plotrequest();
}


/* Support for input, shell, and help for various systems */

#ifdef VMS

# include <descrip.h>
# include <rmsdef.h>
# include <smgdef.h>
# include <smgmsg.h>

extern lib$get_input(), lib$put_output();
extern smg$read_composed_line();
extern sys$putmsg();
extern lbr$output_help();
extern lib$spawn();

int vms_len;

unsigned int status[2] = { 1, 0 };

static char Help[MAX_LINE_LEN+1] = "gnuplot";

$DESCRIPTOR(prompt_desc, PROMPT);
/* temporary fix until change to variable length */
struct dsc$descriptor_s line_desc =
{0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL};

$DESCRIPTOR(help_desc, Help);
$DESCRIPTOR(helpfile_desc, "GNUPLOT$HELP");

/* please note that the vms version of read_line doesn't support variable line
   length (yet) */

static int
read_line(prompt)
const char *prompt;
{
    int more, start = 0;
    char expand_prompt[40];

    prompt_desc.dsc$w_length = strlen(prompt);
    prompt_desc.dsc$a_pointer = prompt;
    (void) strcpy(expand_prompt, "_");
    (void) strncat(expand_prompt, prompt, 38);
    do {
	line_desc.dsc$w_length = MAX_LINE_LEN - start;
	line_desc.dsc$a_pointer = &input_line[start];
	switch (status[1] = smg$read_composed_line(&vms_vkid, &vms_ktid, &line_desc, &prompt_desc, &vms_len)) {
	case SMG$_EOF:
	    done(EXIT_SUCCESS);	/* ^Z isn't really an error */
	    break;
	case RMS$_TNS:		/* didn't press return in time */
	    vms_len--;		/* skip the last character */
	    break;		/* and parse anyway */
	case RMS$_BES:		/* Bad Escape Sequence */
	case RMS$_PES:		/* Partial Escape Sequence */
	    sys$putmsg(status);
	    vms_len = 0;	/* ignore the line */
	    break;
	case SS$_NORMAL:
	    break;		/* everything's fine */
	default:
	    done(status[1]);	/* give the error message */
	}
	start += vms_len;
	input_line[start] = NUL;
	inline_num++;
	if (input_line[start - 1] == '\\') {
	    /* Allow for a continuation line. */
	    prompt_desc.dsc$w_length = strlen(expand_prompt);
	    prompt_desc.dsc$a_pointer = expand_prompt;
	    more = 1;
	    --start;
	} else {
	    line_desc.dsc$w_length = strlen(input_line);
	    line_desc.dsc$a_pointer = input_line;
	    more = 0;
	}
    } while (more);
    return 0;
}


# ifdef NO_GIH
static void
do_help(toplevel)
int toplevel;			/* not used for VMS version */
{
    int first = c_token;
    while (!END_OF_COMMAND)
	++c_token;

    strcpy(Help, "GNUPLOT ");
    capture(Help + 8, first, c_token - 1, sizeof(Help) - 9);
    help_desc.dsc$w_length = strlen(Help);
    if ((vaxc$errno = lbr$output_help(lib$put_output, 0, &help_desc,
				      &helpfile_desc, 0, lib$get_input)) != SS$_NORMAL)
	os_error(NO_CARET, "can't open GNUPLOT$HELP");
}
# endif				/* NO_GIH */


static void
do_shell()
{
    if ((vaxc$errno = lib$spawn()) != SS$_NORMAL) {
	os_error(NO_CARET, "spawn error");
    }
}


static void
do_system()
{
    /* input_line[0] = ' ';     an embarrassment, but... */

    /* input_line is filled by read_line or load_file, but 
     * line_desc length is set only by read_line; adjust now
     */
    line_desc.dsc$w_length = strlen(input_line) - 1;
    line_desc.dsc$a_pointer = &input_line[1];

    if ((vaxc$errno = lib$spawn(&line_desc)) != SS$_NORMAL)
	os_error(NO_CARET, "spawn error");

    (void) putc('\n', stderr);

}
#endif /* VMS */


#ifdef _Windows
# ifdef NO_GIH
static void
do_help(toplevel)
int toplevel;			/* not used for windows */
{
    if (END_OF_COMMAND)
	WinHelp(textwin.hWndParent, (LPSTR) winhelpname, HELP_INDEX, (DWORD) NULL);
    else {
	char buf[128];
	int start = c_token++;
	while (!(END_OF_COMMAND))
	    c_token++;
	capture(buf, start, c_token - 1, 128);
	WinHelp(textwin.hWndParent, (LPSTR) winhelpname, HELP_PARTIALKEY, (DWORD) buf);
    }
}
# endif				/* NO_GIH */
#endif /* _Windows */


/*
 * do_help: (not VMS, although it would work) Give help to the user. It
 * parses the command line into helpbuf and supplies help for that string.
 * Then, if there are subtopics available for that key, it prompts the user
 * with this string. If more input is given, do_help is called recursively,
 * with argument 0.  Thus a more specific help can be supplied. This can be 
 * done repeatedly.  If null input is given, the function returns, effecting 
 * a backward climb up the tree.
 * David Kotz (David.Kotz@Dartmouth.edu) 10/89
 * drd - The help buffer is first cleared when called with toplevel=1. 
 * This is to fix a bug where help is broken if ^C is pressed whilst in the 
 * help.
 */

#ifndef NO_GIH
static void
do_help(toplevel)
int toplevel;
{
    static char *helpbuf = NULL;
    static char *prompt = NULL;
    int base;			/* index of first char AFTER help string */
    int len;			/* length of current help string */
    TBOOLEAN more_help;
    TBOOLEAN only;		/* TRUE if only printing subtopics */
    int subtopics;		/* 0 if no subtopics for this topic */
    int start;			/* starting token of help string */
    char *help_ptr;		/* name of help file */
# if defined(SHELFIND)
    static char help_fname[256] = "";	/* keep helpfilename across calls */
# endif

# if defined(ATARI) || defined(MTOS)
    char const *const ext[] = { NULL };
# endif

    if ((help_ptr = getenv("GNUHELP")) == (char *) NULL)
# ifndef SHELFIND
	/* if can't find environment variable then just use HELPFILE */

/* patch by David J. Liu for getting GNUHELP from home directory */
#  if (defined(__TURBOC__) && (defined(MSDOS) || defined(DOS386))) || defined(__DJGPP__)
	help_ptr = HelpFile;
#  else
#   if defined(ATARI) || defined(MTOS)
    {
	/* I hope findfile really can accept a NULL argument ... */
	if ((help_ptr = findfile(HELPFILE, user_gnuplotpath, ext)) == NULL)
	    help_ptr = findfile(HELPFILE, getenv("PATH"), ext);
	if (!help_ptr)
	    help_ptr = HELPFILE;
    }
#   else
    help_ptr = HELPFILE;
#   endif			/* ATARI || MTOS */
#  endif			/* __TURBOC__ */
/* end of patch  - DJL */

# else				/* !SHELFIND */
    /* try whether we can find the helpfile via shell_find. If not, just
       use the default. (tnx Andreas) */

    if (!strchr(HELPFILE, ':') && !strchr(HELPFILE, '/') &&
	!strchr(HELPFILE, '\\')) {
	if (strlen(help_fname) == 0) {
	    strcpy(help_fname, HELPFILE);
	    if (shel_find(help_fname) == 0) {
		strcpy(help_fname, HELPFILE);
	    }
	}
	help_ptr = help_fname;
    } else {
	help_ptr = HELPFILE;
    }
# endif				/* !SHELFIND */

    /* Since MSDOS DGROUP segment is being overflowed we can not allow such  */
    /* huge static variables (1k each). Instead we dynamically allocate them */
    /* on the first call to this function...                                 */
    if (helpbuf == NULL) {
	helpbuf = gp_alloc(MAX_LINE_LEN, "help buffer");
	prompt = gp_alloc(MAX_LINE_LEN, "help prompt");
	helpbuf[0] = prompt[0] = 0;
    }
    if (toplevel)
	helpbuf[0] = prompt[0] = 0;	/* in case user hit ^c last time */

    len = base = strlen(helpbuf);

    /* find the end of the help command */
    for (start = c_token; !(END_OF_COMMAND); c_token++);
    /* copy new help input into helpbuf */
    if (len > 0)
	helpbuf[len++] = ' ';	/* add a space */
    capture(helpbuf + len, start, c_token - 1, MAX_LINE_LEN - len);
    squash_spaces(helpbuf + base);	/* only bother with new stuff */
    lower_case(helpbuf + base);	/* only bother with new stuff */
    len = strlen(helpbuf);

    /* now, a lone ? will print subtopics only */
    if (strcmp(helpbuf + (base ? base + 1 : 0), "?") == 0) {
	/* subtopics only */
	subtopics = 1;
	only = TRUE;
	helpbuf[base] = NUL;	/* cut off question mark */
    } else {
	/* normal help request */
	subtopics = 0;
	only = FALSE;
    }

    switch (help(helpbuf, help_ptr, &subtopics)) {
    case H_FOUND:{
	    /* already printed the help info */
	    /* subtopics now is true if there were any subtopics */
	    screen_ok = FALSE;

	    do {
		if (subtopics && !only) {
		    /* prompt for subtopic with current help string */
		    if (len > 0)
			(void) sprintf(prompt, "Subtopic of %s: ", helpbuf);
		    else
			(void) strcpy(prompt, "Help topic: ");
		    read_line(prompt);
		    num_tokens = scanner(&input_line, &input_line_len);
		    c_token = 0;
		    more_help = !(END_OF_COMMAND);
		    if (more_help)
			/* base for next level is all of current helpbuf */
			do_help(0);
		} else
		    more_help = FALSE;
	    } while (more_help);

	    break;
	}
    case H_NOTFOUND:{
	    printf("Sorry, no help for '%s'\n", helpbuf);
	    break;
	}
    case H_ERROR:{
	    perror(help_ptr);
	    break;
	}
    default:{			/* defensive programming */
	    int_error(NO_CARET, "Impossible case in switch");
	    /* NOTREACHED */
	}
    }

    helpbuf[base] = NUL;	/* cut it off where we started */
}
#endif /* !NO_GIH */

#ifndef VMS

static void
do_system()
{
# ifdef AMIGA_AC_5
    static char *parms[80];
    getparms(input_line + 1, parms);
    fexecv(parms[0], parms);
# elif (defined(ATARI) && defined(__GNUC__))
/* || (defined(MTOS) && defined(__GNUC__)) */
    /* use preloaded shell, if available */
    short (*shell_p) (char *command);
    void *ssp;

    ssp = (void *) Super(NULL);
    shell_p = *(short (**)(char *)) 0x4f6;
    Super(ssp);

    /* this is a bit strange, but we have to have a single if */
    if (shell_p)
	(*shell_p) (input_line + 1);
    else
	system(input_line + 1);
# elif defined(_Windows)
    winsystem(input_line + 1);
# else /* !(AMIGA_AC_5 || ATARI && __GNUC__ || _Windows) */
/* (am, 19980929)
 * OS/2 related note: cmd.exe returns 255 if called w/o argument.
 * i.e. calling a shell by "!" will always end with an error message.
 * A workaround has to include checking for EMX,OS/2, two environment
 *  variables,...
 */
    system(input_line + 1);
# endif /* !(AMIGA_AC_5 || ATARI&&__GNUC__ || _Windows) */
}


# ifdef AMIGA_AC_5
/******************************************************************************
 * Parses the command string (for fexecv use) and  converts the first token
 * to lower case                                                 
 *****************************************************************************/
static void
getparms(command, parms)
char *command;
char **parms;
{
    static char strg0[256];
    register int i = 0, j = 0, k = 0;		/* A bunch of indices */

    while (*(command + j) != NUL) {	/* Loop on string characters */
	parms[k++] = strg0 + i;
	while (*(command + j) == ' ')
	    ++j;
	while (*(command + j) != ' ' && *(command + j) != NUL) {
	    if (*(command + j) == '"')	/* Get quoted string */
		for (*(strg0 + (i++)) = *(command + (j++));
		     *(command + j) != '"';
		     *(strg0 + (i++)) = *(command + (j++)));
	    *(strg0 + (i++)) = *(command + (j++));
	}
	*(strg0 + (i++)) = NUL;	/* NUL terminate every token */
    }
    parms[k] = NUL;

    for (k = strlen(strg0) - 1; k >= 0; --k)	/* Convert to lower case */
	*(strg0 + k) >= 'A' && *(strg0 + k) <= 'Z' ? *(strg0 + k) |= 32 : *(strg0 + k);
}

# endif				/* AMIGA_AC_5 */


# if defined(READLINE) || defined(HAVE_LIBREADLINE)
/* keep some compilers happy */
static char *rlgets __PROTO((char *s, size_t n, char *prompt));

static char *
rlgets(s, n, prompt)
char *s;
size_t n;
char *prompt;
{
    static char *line = (char *) NULL;
    static int leftover = -1;	/* index of 1st char leftover from last call */

    if (leftover == -1) {
	/* If we already have a line, first free it */
	if (line != (char *) NULL) {
	    free(line);
	    line = NULL;
	    /* so that ^C or int_error during readline() does
	     * not result in line being free-ed twice */
	}
	line = readline((interactive) ? prompt : "");
	leftover = 0;
	/* If it's not an EOF */
	if (line && *line) {
#ifdef HAVE_LIBREADLINE
	    HIST_ENTRY *temp;

	    /* Must always be called at this point or
	     * 'temp' has the wrong value. */
	    using_history();
	    temp = previous_history();

	    if (temp == 0 || strcmp(temp->line, line) != 0)
		add_history(line);

#else /* !HAVE_LIBREADLINE */
	    add_history(line);
#endif
	}
    }
    if (line) {
	safe_strncpy(s, line + leftover, n);
	leftover += strlen(s);
	if (line[leftover] == NUL)
	    leftover = -1;
	return s;
    }
    return NULL;
}
# endif				/* READLINE || HAVE_LIBREADLINE */


# if defined(MSDOS) || defined(_Windows) || defined(DOS386)
static void
do_shell()
{
    if (user_shell) {
#  if defined(_Windows)
	if (WinExec(user_shell, SW_SHOWNORMAL) <= 32)
#  elif defined(DJGPP)
	    if (system(user_shell) == -1)
#  else
		if (spawnl(P_WAIT, user_shell, NULL) == -1)
#  endif			/* !(_Windows || DJGPP) */
		    os_error(NO_CARET, "unable to spawn shell");
    }
}

# elif defined(AMIGA_SC_6_1)

static void
do_shell()
{
    if (user_shell) {
	if (system(user_shell))
	    os_error(NO_CARET, "system() failed");
    }
    (void) putc('\n', stderr);
}

#  elif defined(OS2)

static void
do_shell()
{
    if (user_shell) {
	if (system(user_shell) == -1)
	    os_error(NO_CARET, "system() failed");

    }
    (void) putc('\n', stderr);
}

#  else				/* !OS2 */

/* plain old Unix */

#define EXEC "exec "
static void
do_shell()
{
    static char exec[100] = EXEC;

    if (user_shell) {
	if (system(safe_strncpy(&exec[sizeof(EXEC) - 1], user_shell,
				sizeof(exec) - sizeof(EXEC) - 1)))
	    os_error(NO_CARET, "system() failed");
    }
    (void) putc('\n', stderr);
}

# endif				/* !MSDOS */

/* read from stdin, everything except VMS */

# if !defined(READLINE) && !defined(HAVE_LIBREADLINE)
#  if (defined(MSDOS) || defined(DOS386)) && !defined(_Windows) && !defined(__EMX__) && !defined(DJGPP)

/* if interactive use console IO so CED will work */

#define PUT_STRING(s) cputs(s)
#define GET_STRING(s,l) ((interactive) ? cgets_emu(s,l) : fgets(s,l,stdin))

#   ifdef __TURBOC__
/* cgets implemented using dos functions */
/* Maurice Castro 22/5/91 */
static char *doscgets __PROTO((char *));

static char *
doscgets(s)
char *s;
{
    long datseg;

    /* protect and preserve segments - call dos to do the dirty work */
    datseg = _DS;

    _DX = FP_OFF(s);
    _DS = FP_SEG(s);
    _AH = 0x0A;
    geninterrupt(33);
    _DS = datseg;

    /* check for a carriage return and then clobber it with a null */
    if (s[s[1] + 2] == '\r')
	s[s[1] + 2] = 0;

    /* return the input string */
    return (&(s[2]));
}
#   endif			/* __TURBOC__ */

#   ifdef __ZTC__
void
cputs(char *s)
{
    register int i = 0;
    while (s[i] != NUL)
	bdos(0x02, s[i++], NULL);
}

char *
cgets(char *s)
{
    bdosx(0x0A, s, NULL);

    if (s[s[1] + 2] == '\r')
	s[s[1] + 2] = 0;

    /* return the input string */
    return (&(s[2]));
}
#   endif			/* __ZTC__ */

/* emulate a fgets like input function with DOS cgets */
char *
cgets_emu(str, len)
char *str;
int len;
{
    static char buffer[128] = "";
    static int leftover = 0;

    if (buffer[leftover] == NUL) {
	buffer[0] = 126;
#   ifdef __TURBOC__
	doscgets(buffer);
#   else
	cgets(buffer);
#   endif
	fputc('\n', stderr);
	if (buffer[2] == 26)
	    return NULL;
	leftover = 2;
    }
    safe_strncpy(str, buffer + leftover, len);
    leftover += strlen(str);
    return str;
}
#  else				/* !plain DOS */

#   define PUT_STRING(s) fputs(s, stderr)
#   define GET_STRING(s,l) fgets(s, l, stdin)

#  endif			/* !plain DOS */
# endif				/* !READLINE && !HAVE_LIBREADLINE) */

/* Non-VMS version */
static int
read_line(prompt)
const char *prompt;
{
    int start = 0;
    TBOOLEAN more = FALSE;
    int last = 0;

# if !defined(READLINE) && !defined(HAVE_LIBREADLINE)
    if (interactive)
	PUT_STRING(prompt);
# endif				/* no READLINE */
    do {
	/* grab some input */
# if defined(READLINE) || defined(HAVE_LIBREADLINE)
	if (((interactive)
	    ? rlgets(&(input_line[start]), input_line_len - start,
		     ((more) ? "> " : prompt))
	    : fgets(&(input_line[start]), input_line_len - start, stdin))
	    == (char *) NULL) {
# else /* !(READLINE || HAVE_LIBREADLINE) */
	if (GET_STRING(&(input_line[start]), input_line_len - start)
	    == (char *) NULL) {
# endif /* !(READLINE || HAVE_LIBREADLINE) */
	    /* end-of-file */
	    if (interactive)
		(void) putc('\n', stderr);
	    input_line[start] = NUL;
	    inline_num++;
	    if (start > 0)	/* don't quit yet - process what we have */
		more = FALSE;
	    else
		return (1);	/* exit gnuplot */
	} else {
	    /* normal line input */
	    last = strlen(input_line) - 1;
	    if (last >= 0) {
		if (input_line[last] == '\n') {		/* remove any newline */
		    input_line[last] = NUL;
		    /* Watch out that we don't backup beyond 0 (1-1-1) */
		    if (last > 0)
			--last;
		} else if (last + 2 >= input_line_len) {
		    extend_input_line();
		    start = last + 1;
		    more = TRUE;
		    continue;	/* read rest of line, don't print "> " */
		}
		if (input_line[last] == '\\') {		/* line continuation */
		    start = last;
		    more = TRUE;
		} else
		    more = FALSE;
	    } else
		more = FALSE;
	}
# if !defined(READLINE) && !defined(HAVE_LIBREADLINE)
	if (more && interactive)
	    PUT_STRING("> ");
# endif
    } while (more);
    return (0);
}

#endif /* !VMS */

#ifdef _Windows
/* there is a system like call on MS Windows but it is a bit difficult to 
   use, so we will invoke the command interpreter and use it to execute the 
   commands */
static int
winsystem(char *s)
{
    LPSTR comspec;
    LPSTR execstr;
    LPSTR p;

    /* get COMSPEC environment variable */
# ifdef WIN32
    char envbuf[81];
    GetEnvironmentVariable("COMSPEC", envbuf, 80);
    if (*envbuf == NUL)
	comspec = "\\command.com";
    else
	comspec = envbuf;
# else
    p = GetDOSEnvironment();
    comspec = "\\command.com";
    while (*p) {
	if (!strncmp(p, "COMSPEC=", 8)) {
	    comspec = p + 8;
	    break;
	}
	p += strlen(p) + 1;
    }
# endif
    /* if the command is blank we must use command.com */
    p = s;
    while ((*p == ' ') || (*p == '\n') || (*p == '\r'))
	p++;
    if (*p == NUL) {
	WinExec(comspec, SW_SHOWNORMAL);
    } else {
	/* attempt to run the windows/dos program via windows */
	if (WinExec(s, SW_SHOWNORMAL) <= 32) {
	    /* attempt to run it as a dos program from command line */
	    execstr = (char *) malloc(strlen(s) + strlen(comspec) + 6);
	    strcpy(execstr, comspec);
	    strcat(execstr, " /c ");
	    strcat(execstr, s);
	    WinExec(execstr, SW_SHOWNORMAL);
	    free(execstr);
	}
    }

    /* regardless of the reality return OK - the consequences of */
    /* failure include shutting down Windows */
    return (0);			/* success */
}
#endif /* _Windows */
