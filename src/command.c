/* GNUPLOT - command.c */

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
 * Changes marked by USE_MOUSE
 *
 * May 1999, update by Petr Mikulik
 * Use gnuplot's pid in shared mem name
 *
 * August 1999 Franz Bakan and Petr Mikulik
 * Encapsulating read_line into a thread, acting on input when thread or
 * gnupmdrv posts an event semaphore. Thus mousing works even when gnuplot
 * is used as a plotting device (commands passed via pipe).
 *
 * May 2011 Ethan A Merritt
 * Introduce block structure defined by { ... }, which may span multiple lines.
 * In order to have the entire block available at one time we now count
 * +/- curly brackets during input and keep extending the current input line
 * until the net count is zero.  This is done in do_line() for interactive
 * input, and load_file() for non-interactive input.
 */

#include "command.h"

#include "axis.h"

#include "alloc.h"
#include "datablock.h"
#include "eval.h"
#include "fit.h"
#include "datafile.h"
#include "getcolor.h"
#include "gp_hist.h"
#include "gp_time.h"
#include "misc.h"
#include "parse.h"
#include "plot.h"
#include "plot2d.h"
#include "plot3d.h"
#include "readline.h"
#include "save.h"
#include "scanner.h"
#include "setshow.h"
#include "stats.h"
#include "tables.h"
#include "term_api.h"
#include "util.h"
#include "variable.h"
#include "external.h"

#ifdef USE_MOUSE
# include "mouse.h"
int paused_for_mouse = 0;
#endif

#define PROMPT "gnuplot> "

#ifdef OS2_IPC
# define INCL_DOSMEMMGR
# define INCL_DOSPROCESS
# define INCL_DOSSEMAPHORES
# include <os2.h>
static char *input_line_SharedMem = NULL; /* pointer to the shared memory for mouse messages */
static HEV semInputReady = 0;      /* mouse event semaphore */
static TBOOLEAN thread_rl_Running = FALSE;  /* running status */
static int thread_rl_RetCode = -1; /* return code from readline input thread */
#endif /* OS2_IPC */

#ifndef _WIN32
# include "help.h"
#endif

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# ifdef _MSC_VER
#  include <malloc.h>
#  include <direct.h>          /* getcwd() */
# else
#  include <alloc.h>
# endif
# include <htmlhelp.h>
# include "win/winmain.h"
#endif /* _WIN32 */

#ifdef __DJGPP__
# include <pc.h>		/* getkey() */
# define useconds_t unsigned
#endif

#ifdef __WATCOMC__
# include <conio.h>		/* for getch() */
#endif

#ifdef VMS
int vms_vkid;			/* Virtual keyboard id */
int vms_ktid;			/* key table id, for translating keystrokes */
#endif /* VMS */


/* static prototypes */
static void command(void);
static TBOOLEAN is_array_assignment(void);
static int changedir(char *path);
static char* fgets_ipc(char* dest, int len);
static char* gp_get_string(char *, size_t, const char *);
static int read_line(const char *prompt, int start);
static void do_system(const char *);
static void test_palette_subcommand(void);
static int find_clause(int *, int *);
static int report_error(int ierr);

static int expand_1level_macros(void);

struct lexical_unit *token;
int token_table_size;


char *gp_input_line;
size_t gp_input_line_len;
int inline_num;			/* input line number */

struct udft_entry *dummy_func;

/* support for replot command */
char *replot_line;
int plot_token;			/* start of 'plot' command */

/* flag to disable `replot` when some data are sent through stdin;
 * used by mouse/hotkey capable terminals */
TBOOLEAN replot_disabled = FALSE;

/* output file for the print command */
FILE *print_out = NULL;
struct udvt_entry *print_out_var = NULL;
char *print_out_name = NULL;

/* input data, parsing variables */
int num_tokens, c_token;

int if_depth = 0;
TBOOLEAN if_condition = FALSE;
TBOOLEAN if_open_for_else = FALSE;

static int clause_depth = 0;

/* support for 'break' and 'continue' commands */
static int iteration_depth = 0;
static TBOOLEAN requested_break = FALSE;
static TBOOLEAN requested_continue = FALSE;

/* set when an "exit" command is encountered */
static int command_exit_requested = 0;

/* support for dynamic size of input line */
void
extend_input_line()
{
    if (gp_input_line_len == 0) {
	/* first time */
	gp_input_line = gp_alloc(MAX_LINE_LEN, "gp_input_line");
	gp_input_line_len = MAX_LINE_LEN;
	gp_input_line[0] = NUL;
    } else {
	gp_input_line = gp_realloc(gp_input_line, gp_input_line_len + MAX_LINE_LEN,
				"extend input line");
	gp_input_line_len += MAX_LINE_LEN;
	FPRINTF((stderr, "extending input line to %d chars\n",
		 gp_input_line_len));
    }
}

/* constant by which token table grows */
#define MAX_TOKENS 400

void
extend_token_table()
{
    if (token_table_size == 0) {
	/* first time */
	token = (struct lexical_unit *) gp_alloc(MAX_TOKENS * sizeof(struct lexical_unit), "token table");
	token_table_size = MAX_TOKENS;
	/* HBB: for checker-runs: */
	memset(token, 0, MAX_TOKENS * sizeof(*token));
    } else {
	token = gp_realloc(token, (token_table_size + MAX_TOKENS) * sizeof(struct lexical_unit), "extend token table");
	memset(token+token_table_size, 0, MAX_TOKENS * sizeof(*token));
	token_table_size += MAX_TOKENS;
	FPRINTF((stderr, "extending token table to %d elements\n", token_table_size));
    }
}


#ifdef OS2_IPC
void
thread_read_line(void *arg)
{
    (void) arg;
    thread_rl_Running = TRUE;
    thread_rl_RetCode = read_line(PROMPT, 0);
    thread_rl_Running = FALSE;
    DosPostEventSem(semInputReady);
}


void
os2_ipc_setup(void)
{
    APIRET rc;
    char semInputReadyName[40];
    char mouseSharedMemName[40];

    /* create input event semaphore */
    sprintf(semInputReadyName, "\\SEM32\\GP%i_Input_Ready", getpid());
    rc = DosCreateEventSem(semInputReadyName, &semInputReady, 0, 0);
    if (rc != 0)
	fputs("DosCreateEventSem error\n", stderr);

    /* allocate shared memory */
    sprintf(mouseSharedMemName, "\\SHAREMEM\\GP%i_Mouse_Input", getpid());
    rc = DosAllocSharedMem((PPVOID) &input_line_SharedMem,
		mouseSharedMemName, MAX_LINE_LEN,
		PAG_READ | PAG_WRITE | PAG_COMMIT);
    if (rc != 0)
	fputs("DosAllocSharedMem ERROR\n", stderr);
    else
	*input_line_SharedMem = 0;
}


int
os2_ipc_dispatch_event(void)
{
    if (input_line_SharedMem == NULL || !*input_line_SharedMem)
	return 0;

    if (*input_line_SharedMem == '%') {
	struct gp_event_t ge;

	/* copy event data immediately */
	memcpy(&ge, input_line_SharedMem + 1, sizeof(ge));
	*input_line_SharedMem = 0; /* discard the event data */
	thread_rl_RetCode = 0;

	/* process event */
	do_event(&ge);

	/* end pause mouse? */
	if ((ge.type == GE_buttonrelease) && (paused_for_mouse & PAUSE_CLICK) &&
	    (((ge.par1 == 1) && (paused_for_mouse & PAUSE_BUTTON1)) ||
	     ((ge.par1 == 2) && (paused_for_mouse & PAUSE_BUTTON2)) ||
	     ((ge.par1 == 3) && (paused_for_mouse & PAUSE_BUTTON3)))) {
	    paused_for_mouse = 0;
	}
	if ((ge.type == GE_keypress) && (paused_for_mouse & PAUSE_KEYSTROKE) &&
	    (ge.par1 != NUL)) {
	    paused_for_mouse = 0;
	}
	return 0;
    }
    if (*input_line_SharedMem &&
        strstr(input_line_SharedMem, "plot") != NULL &&
        (strcmp(term->name, "pm") != 0 && strcmp(term->name, "x11") != 0)) {
	/* avoid plotting if terminal is not PM or X11 */
	fprintf(stderr, "\n\tCommand(s) ignored for other than PM and X11 terminals\a\n");
	if (interactive)
	    fputs(PROMPT, stderr);
	*input_line_SharedMem = 0; /* discard the event data */
	return 0;
    }
    strcpy(gp_input_line, input_line_SharedMem);
    input_line_SharedMem[0] = 0;
    thread_rl_RetCode = 0;
    return 1;
}
#endif /* OS2_IPC */


int
com_line()
{
    if (multiplot) {
	/* calls int_error() if it is not happy */
	term_check_multiplot_okay(interactive);

	if (read_line("multiplot> ", 0))
	    return (1);
    } else {

#if defined(OS2_IPC) && defined(USE_MOUSE)
	ULONG u;

	if (!thread_rl_Running) {
	    int res;

	    // Discard pending mouse events to avoid spurious side effects.
	    // This seems only necessary since we do no handle mouse events
	    // anywhere else (like in pause, during load, etc.).
	    input_line_SharedMem[0] = 0;
	    DosResetEventSem(semInputReady, &u);

	    res = _beginthread(thread_read_line, NULL, 32768, NULL);
	    if (res == -1)
		fputs("error command.c could not begin thread\n", stderr);
	}
	/* wait until a line is read or gnupmdrv makes shared mem available */
	DosWaitEventSem(semInputReady, SEM_INDEFINITE_WAIT);
	DosResetEventSem(semInputReady, &u);
	if (thread_rl_Running) {
	    /* input thread still running, this must be a "mouse" event */
	    if (os2_ipc_dispatch_event() == 0)
		return 0;
	}
	if (thread_rl_RetCode)
	    return 1;
#else	/* The normal case */
	if (read_line(PROMPT, 0))
	    return 1;
#endif	/* defined(OS2_IPC) && defined(USE_MOUSE) */
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
    /* Line continuation has already been handled by read_line().
     * Expand any string variables in the current input line.
     */
    string_expand_macros();

    /* Remove leading whitespace */
    {
	char *inlptr = gp_input_line;
	while (isspace((unsigned char) *inlptr))
	    inlptr++;
	if (inlptr != gp_input_line) {
	    memmove(gp_input_line, inlptr, strlen(inlptr));
	    gp_input_line[strlen(inlptr)] = NUL;
	}
    }

    /* Leading '!' indicates a shell command that bypasses normal gnuplot
     * tokenization and parsing.  This doesn't work inside a bracketed clause.
     */
    if (is_system(*gp_input_line)) {
	do_system(gp_input_line + 1);
	return (0);
    }

    /* Strip off trailing comment */
    if (strchr(gp_input_line, '#')) {
	num_tokens = scanner(&gp_input_line, &gp_input_line_len);
	if (gp_input_line[token[num_tokens].start_index] == '#')
	    gp_input_line[token[num_tokens].start_index] = NUL;
    }

    if_depth = 0;

    num_tokens = scanner(&gp_input_line, &gp_input_line_len);

    /*
     * Expand line if necessary to contain a complete bracketed clause {...}
     * Insert a ';' after current line and append the next input line.
     * NB: This may leave an "else" condition on the next line.
     */
    if (curly_brace_count < 0)
	int_error(NO_CARET,"Unexpected }");

    while (curly_brace_count > 0) {
	if (lf_head && lf_head->depth > 0) {
	    /* This catches the case that we are inside a "load foo" operation
	     * and therefore requesting interactive input is not an option.
	     * FIXME: or is it?
	     */
	    int_error(NO_CARET, "Syntax error: missing block terminator }");
	}
	else if (interactive || noinputfiles) {
	    /* If we are really in interactive mode and there are unterminated blocks,
	     * then we want to display a "more>" prompt to get the rest of the block.
	     * However, there are two more cases that must be dealt here:
	     * One is when commands are piped to gnuplot - on the command line,
	     * the other is when commands are piped to gnuplot which is opened
	     * as a slave process. The test for noinputfiles is for the latter case.
	     * If we didn't have that test here, unterminated blocks sent via a pipe
	     * would trigger the error message in the else branch below. */
	    int retval;
	    strcat(gp_input_line,";");
	    retval = read_line("more> ", strlen(gp_input_line));
	    if (retval)
		int_error(NO_CARET, "Syntax error: missing block terminator }");
	    /* Expand any string variables in the current input line */
	    string_expand_macros();

	    num_tokens = scanner(&gp_input_line, &gp_input_line_len);
	    if (gp_input_line[token[num_tokens].start_index] == '#')
		gp_input_line[token[num_tokens].start_index] = NUL;
	}
	else {
	    /* Non-interactive mode here means that we got a string from -e.
	     * Having curly_brace_count > 0 means that there are at least one
	     * unterminated blocks in the string.
	     * Likely user error, so we die with an error message. */
	    int_error(NO_CARET, "Syntax error: missing block terminator }");
	}
    }

    c_token = 0;
    while (c_token < num_tokens) {
	command();
	if (command_exit_requested) {
	    command_exit_requested = 0;	/* yes this is necessary */
	    return 1;
	}
	if (iteration_early_exit()) {
	    c_token = num_tokens;
	    break;
	}
	if (c_token < num_tokens) {	/* something after command */
	    if (equals(c_token, ";")) {
		c_token++;
	    } else if (equals(c_token, "{")) {
		begin_clause();
	    } else if (equals(c_token, "}")) {
		end_clause();
	    } else
		int_error(c_token, "unexpected or unrecognized token: %s",
		    token_to_string(c_token));
	}
    }

    /* This check allows event handling inside load/eval/while statements */
    check_for_mouse_events();
    return (0);
}


void
do_string(const char *s)
{
    char *cmdline = gp_strdup(s);
    do_string_and_free(cmdline);
}

void
do_string_and_free(char *cmdline)
{
#ifdef USE_MOUSE
    if (display_ipc_commands())
	fprintf(stderr, "%s\n", cmdline);
#endif

    lf_push(NULL, NULL, cmdline); /* save state for errors and recursion */
    while (gp_input_line_len < strlen(cmdline) + 1)
	extend_input_line();
    strcpy(gp_input_line, cmdline);
    screen_ok = FALSE;
    command_exit_requested = do_line();

    /* "exit" is supposed to take us out of the current file from a
     * "load <file>" command.  But the LFS stack holds both files and
     * bracketed clauses, so we have to keep popping until we hit an
     * actual file.
     */
    if (command_exit_requested) {
	while (lf_head && !lf_head->name) {
	    FPRINTF((stderr,"pop one level of non-file LFS\n"));
	    lf_pop();
	}
    } else
	lf_pop();
}


#ifdef USE_MOUSE
void
toggle_display_of_ipc_commands()
{
    if (mouse_setting.verbose)
	mouse_setting.verbose = 0;
    else
	mouse_setting.verbose = 1;
}

int
display_ipc_commands()
{
    return mouse_setting.verbose;
}

void
do_string_replot(const char *s)
{
    do_string(s);

    if (volatile_data && (E_REFRESH_NOT_OK != refresh_ok)) {
	if (display_ipc_commands())
	    fprintf(stderr, "refresh\n");
	refresh_request();

    } else if (!replot_disabled)
	replotrequest();

    else
	int_warn(NO_CARET, "refresh not possible and replot is disabled");
}

void
restore_prompt()
{
    if (interactive) {
#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDITLINE)
#  if !defined(MISSING_RL_FORCED_UPDATE_DISPLAY)
	rl_forced_update_display();
#  else
	rl_redisplay();
#  endif
#else
	fputs(PROMPT, stderr);
	fflush(stderr);
#endif
    }
}
#endif /* USE_MOUSE */


void
define()
{
    int start_token;	/* the 1st token in the function definition */
    struct udvt_entry *udv;
    struct udft_entry *udf;
    struct value result;

    if (equals(c_token + 1, "(")) {
	/* function ! */
	int dummy_num = 0;
	struct at_type *at_tmp;
	char *tmpnam;
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
	udf->dummy_num = dummy_num;
	if ((at_tmp = perm_at()) == (struct at_type *) NULL)
	    int_error(start_token, "not enough memory for function");
	if (udf->at)		/* already a dynamic a.t. there */
	    free_at(udf->at);	/* so free it first */
	udf->at = at_tmp;	/* before re-assigning it. */
	memcpy(c_dummy_var, save_dummy, sizeof(save_dummy));
	m_capture(&(udf->definition), start_token, c_token - 1);
	dummy_func = NULL;	/* dont let anyone else use our workspace */

	/* Save function definition in a user-accessible variable */
	tmpnam = gp_alloc(8+strlen(udf->udf_name), "varname");
	strcpy(tmpnam, "GPFUN_");
	strcat(tmpnam, udf->udf_name);
	fill_gpval_string(tmpnam, udf->definition);
	free(tmpnam);

    } else {
	/* variable ! */
	char *varname = gp_input_line + token[c_token].start_index;
	if (!strncmp(varname, "GPVAL_", 6)
	||  !strncmp(varname, "GPFUN_", 6)
	||  !strncmp(varname, "MOUSE_", 6))
	    int_error(c_token, "Cannot set internal variables GPVAL_ GPFUN_ MOUSE_");
	start_token = c_token;
	c_token += 2;
	udv = add_udv(start_token);
	(void) const_express(&result);
	/* Prevents memory leak if the variable name is re-used */
	free_value(&udv->udv_value);
	udv->udv_value = result;
    }
}


void
undefine_command()
{
    char key[MAX_ID_LEN+1];
    TBOOLEAN wildcard;

    c_token++;               /* consume the command name */

    while (!END_OF_COMMAND) {
	/* copy next var name into key */
	copy_str(key, c_token, MAX_ID_LEN);

	/* Peek ahead - must do this, because a '*' is returned as a
	   separate token, not as part of the 'key' */
	wildcard = equals(c_token+1,"*");
	if (wildcard)
	    c_token++;

	/* The '$' starting a data block name is a separate token */
	else if (*key == '$')
	    copy_str(&key[1], ++c_token, MAX_ID_LEN-1);

	/* Other strange stuff on command line */
	else if (!isletter(c_token))
	    int_error(c_token, "Not a variable name");

	/* This command cannot deal with array elements or functions */
	if (equals(c_token+1, "[") || equals(c_token+1, "("))
	    int_error(c_token, "Cannot undefine function or array element");

	/* ignore internal variables */
	if (strncmp(key, "GPVAL_", 6) && strncmp(key, "MOUSE_", 6))
	    del_udv_by_name( key, wildcard );

	c_token++;
    }
}


static void
command()
{
    int i;

    for (i = 0; i < MAX_NUM_VAR; i++)
	c_dummy_var[i][0] = NUL;	/* no dummy variables */

    if (is_definition(c_token))
	define();
    else if (is_array_assignment())
	;
    else
	(*lookup_ftable(&command_ftbl[0],c_token))();

    return;
}


/* process the 'raise' or 'lower' command */
void
raise_lower_command(int lower)
{
    ++c_token;

    if (END_OF_COMMAND) {
	if (lower) {
#ifdef OS2
	    pm_lower_terminal_window();
#endif
#ifdef X11
	    x11_lower_terminal_group();
#endif
#ifdef _WIN32
	    win_lower_terminal_group();
#endif
#ifdef WXWIDGETS
	    wxt_lower_terminal_group();
#endif
	} else {
#ifdef OS2
	    pm_raise_terminal_window();
#endif
#ifdef X11
	    x11_raise_terminal_group();
#endif
#ifdef _WIN32
	    win_raise_terminal_group();
#endif
#ifdef WXWIDGETS
	    wxt_raise_terminal_group();
#endif
	}
	return;
    } else {
	int number;
	int negative = equals(c_token, "-");

	if (negative || equals(c_token, "+")) c_token++;
	if (!END_OF_COMMAND && isanumber(c_token)) {
	    number = real_expression();
	    if (negative)
	    number = -number;
	    if (lower) {
#ifdef OS2
		pm_lower_terminal_window();
#endif
#ifdef X11
		x11_lower_terminal_window(number);
#endif
#ifdef _WIN32
		win_lower_terminal_window(number);
#endif
#ifdef WXWIDGETS
		wxt_lower_terminal_window(number);
#endif
	    } else {
#ifdef OS2
		pm_raise_terminal_window();
#endif
#ifdef X11
		x11_raise_terminal_window(number);
#endif
#ifdef _WIN32
		win_raise_terminal_window(number);
#endif
#ifdef WXWIDGETS
		wxt_raise_terminal_window(number);
#endif
	    }
	    ++c_token;
	    return;
	}
    }
    if (lower)
	int_error(c_token, "usage: lower {plot_id}");
    else
	int_error(c_token, "usage: raise {plot_id}");
}

void
raise_command(void)
{
    raise_lower_command(0);
}

void
lower_command(void)
{
    raise_lower_command(1);
}


/*
 * Arrays are declared using the syntax
 *    array A[size] { = [ element, element, ... ] }
 * where size is an integer and space is reserved for elements A[1] through A[size]
 * The size itself is stored in A[0].v.int_val.A
 * The list of initial values is optional.
 * Any element that is not initialized is set to NOTDEFINED.
 *
 * Elements in an existing array can be accessed like any other gnuplot variable.
 * Each element can be one of INTGR, CMPLX, STRING.
 */
void
array_command()
{
    int nsize = 0;	/* Size of array when we leave */
    int est_size = 0;	/* Estimated size */
    struct udvt_entry *array;
    struct value *A;
    int i;

    /* Create or recycle a udv containing an array with the requested name */
    if (!isletter(++c_token))
	int_error(c_token, "illegal variable name");
    array = add_udv(c_token);
    free_value(&array->udv_value);
    c_token++;

    if (equals(c_token, "[")) {
	c_token++;
	nsize = int_expression();
	if (!equals(c_token++,"]"))
	    int_error(c_token-1, "expecting array[size>0]");
    } else if (equals(c_token, "=") && equals(c_token+1, "[")) {
	/* Estimate size of array by counting commas in the initializer */
	for ( i = c_token+2; i < num_tokens; i++) {
	    if (equals(i,",") || equals(i,"]"))
		est_size++;
	    if (equals(i,"]"))
		break;
	}
	nsize = est_size;
    }
    if (nsize <= 0)
	int_error(c_token-1, "expecting array[size>0]");

    array->udv_value.v.value_array = gp_alloc((nsize+1) * sizeof(t_value), "array_command");
    array->udv_value.type = ARRAY;

    /* Element zero of the new array is not visible but contains the size */
    A = array->udv_value.v.value_array;
    A[0].v.int_val = nsize;
    for (i = 0; i <= nsize; i++) {
	A[i].type = NOTDEFINED;
    }

    /* Initializer syntax:   array A[10] = [x,y,z,,"foo",] */
    if (equals(c_token, "=")) {
	int initializers = 0;
	if (!equals(++c_token, "["))
	    int_error(c_token, "expecting Array[size] = [x,y,...]");
	c_token++;
	for (i = 1; i <= nsize; i++) {
	    if (equals(c_token, "]"))
		break;
	    if (equals(c_token, ",")) {
		initializers++;
		c_token++;
		continue;
	    }
	    const_express(&A[i]);
	    initializers++;
	    if (equals(c_token, "]"))
		break;
	    if (equals(c_token, ","))
		c_token++;
	    else
		int_error(c_token, "expecting Array[size] = [x,y,...]");
	}
	c_token++;
	/* If the size is determined by the number of initializers */
	if (A[0].v.int_val == 0)
	    A[0].v.int_val = initializers;
    }

    return;
}

/*
 * Check for command line beginning with
 *    Array[<expr>] = <expr>
 * This routine is modeled on command.c:define()
 */
TBOOLEAN
is_array_assignment()
{
    udvt_entry *udv;
    struct value newvalue;
    int index;
    TBOOLEAN looks_OK = FALSE;
    int brackets;

    if (!isletter(c_token) || !equals(c_token+1, "["))
	return FALSE;

    /* There are other legal commands where the 2nd token is [
     * e.g.  "plot [min:max] foo"
     * so we check that the closing ] is immediately followed by =.
     */
    for (index=c_token+2, brackets=1; index < num_tokens; index++) {
	if (equals(index,";"))
	    return FALSE;
	if (equals(index,"["))
	    brackets++;
	if (equals(index,"]"))
	    brackets--;
	if (brackets == 0) {
	    if (!equals(index+1,"="))
		return FALSE;
	    looks_OK = TRUE;
	    break;
	}
    }
    if (!looks_OK)
	return FALSE;

    udv = add_udv(c_token);
    if (udv->udv_value.type != ARRAY)
	int_error(c_token, "Not a known array");

    /* Evaluate index */
    c_token += 2;
    index = int_expression();
    if (index <= 0 || index > udv->udv_value.v.value_array[0].v.int_val)
	int_error(c_token, "array index out of range");
    if (!equals(c_token, "]") || !equals(c_token+1, "="))
	int_error(c_token, "Expecting Arrayname[<expr>] = <expr>");

    /* Evaluate right side of assignment */
    c_token += 2;
    (void) const_express(&newvalue);
    udv->udv_value.v.value_array[index] = newvalue;

    return TRUE;
}



#ifdef USE_MOUSE

/* process the 'bind' command */
/* EAM - rewritten 2015 */
void
bind_command()
{
    char* lhs = NULL;
    char* rhs = NULL;
    TBOOLEAN allwindows = FALSE;
    ++c_token;

    if (almost_equals(c_token,"all$windows")) {
	allwindows = TRUE;
	c_token++;
    }

    /* get left hand side: the key or key sequence
     * either (1) entire sequence is in quotes
     * or (2) sequence goes until the first whitespace
     */
    if (END_OF_COMMAND) {
	; /* Fall through */
    } else if ((lhs = try_to_get_string())) {
	FPRINTF((stderr,"Got bind quoted lhs = \"%s\"\n",lhs));
    } else {
	char *first = gp_input_line + token[c_token].start_index;
	int size = strcspn(first, " \";");
	lhs = gp_alloc(size + 1, "bind_command->lhs");
	strncpy(lhs, first, size);
	lhs[size] = '\0';
	FPRINTF((stderr,"Got bind unquoted lhs = \"%s\"\n",lhs));
	while (gp_input_line + token[c_token].start_index < first+size)
	    c_token++;
    }

    /* get right hand side: the command to bind
     * either (1) quoted command
     * or (2) the rest of the line
     */
    if (END_OF_COMMAND) {
	; /* Fall through */
    } else if ((rhs = try_to_get_string())) {
	FPRINTF((stderr,"Got bind quoted rhs = \"%s\"\n",rhs));
    } else {
	int save_token = c_token;
	while (!END_OF_COMMAND)
	    c_token++;
	m_capture( &rhs, save_token, c_token-1 );
	FPRINTF((stderr,"Got bind unquoted rhs = \"%s\"\n",rhs));
    }

    /* bind_process() will eventually free lhs / rhs ! */
    bind_process(lhs, rhs, allwindows);

}
#endif /* USE_MOUSE */

/*
 * 'break' and 'continue' commands act as in the C language.
 * Skip to the end of current loop iteration and (for break)
 * do not iterate further
 */
void
break_command()
{
    c_token++;
    if (iteration_depth == 0)
	return;
    /* Skip to end of current iteration */
    c_token = num_tokens;
    /* request that subsequent iterations should be skipped also */
    requested_break = TRUE;
}

void
continue_command()
{
    c_token++;
    if (iteration_depth == 0)
	return;
    /* Skip to end of current clause */
    c_token = num_tokens;
    /* request that remainder of this iteration be skipped also */
    requested_continue = TRUE;
}

TBOOLEAN
iteration_early_exit()
{
    return (requested_continue || requested_break);
}

/*
 * Command parser functions
 */

/* process the 'call' command */
void
call_command()
{
    char *save_file = NULL;

    c_token++;
    save_file = try_to_get_string();

    if (!save_file)
	int_error(c_token, "expecting filename");
    gp_expand_tilde(&save_file);

    /* Argument list follows filename */
    load_file(loadpath_fopen(save_file, "r"), save_file, 2);
}


/* process the 'cd' command */
void
changedir_command()
{
    char *save_file = NULL;

    c_token++;
    save_file = try_to_get_string();
    if (!save_file)
	int_error(c_token, "expecting directory name");

    gp_expand_tilde(&save_file);
    if (changedir(save_file))
	int_error(c_token, "Can't change to this directory");
    else
	update_gpval_variables(5);
    free(save_file);
}


/* process the 'clear' command */
void
clear_command()
{

    term_start_plot();

    if (multiplot && term->fillbox) {
	int xx1 = xoffset * term->xmax;
	int yy1 = yoffset * term->ymax;
	unsigned int width = xsize * term->xmax;
	unsigned int height = ysize * term->ymax;
	(*term->fillbox) (0, xx1, yy1, width, height);
    }
    term_end_plot();

    screen_ok = FALSE;
    c_token++;

}

/* process the 'evaluate' command */
void
eval_command()
{
    char *command;
    c_token++;
    command = try_to_get_string();
    if (!command)
	int_error(c_token, "Expected command string");
    do_string_and_free(command);
}


/* process the 'exit' and 'quit' commands */
void
exit_command()
{
    /* If the command is "exit gnuplot" then do so */
    if (equals(c_token+1,"gnuplot"))
	gp_exit(EXIT_SUCCESS);

    if (equals(c_token+1,"status")) {
	int status;
	c_token += 2;
	status = int_expression();
	gp_exit(status);
    }

    /* exit error 'error message'  returns to the top command line */
    if (equals(c_token+1,"error")) {
	c_token += 2;
	int_error(NO_CARET, try_to_get_string());
    }

    /* else graphics will be tidied up in main */
    command_exit_requested = 1;
}


/* fit_command() is in fit.c */


/* help_command() is below */


/* process the 'history' command */
void
history_command()
{
#ifdef USE_READLINE
    c_token++;

    if (!END_OF_COMMAND && equals(c_token,"?")) {
	static char *search_str = NULL;  /* string from command line to search for */

	/* find and show the entries */
	c_token++;
	m_capture(&search_str, c_token, c_token);  /* reallocates memory */
	printf ("history ?%s\n", search_str);
	if (!history_find_all(search_str))
	    int_error(c_token,"not in history");
	c_token++;

    } else if (!END_OF_COMMAND && equals(c_token,"!")) {
	const char *line_to_do = NULL;  /* command returned by search	*/

	c_token++;
	if (isanumber(c_token)) {
	    int i = int_expression();
	    line_to_do = history_find_by_number(i);
	} else {
	    char *search_str = NULL;  /* string from command line to search for */
	    m_capture(&search_str, c_token, c_token);
	    line_to_do = history_find(search_str);
	    free(search_str);
	}
	if (line_to_do == NULL)
	    int_error(c_token, "not in history");

	/* Add the command to the history.
	   Note that history commands themselves are no longer added to the history. */
	add_history((char *) line_to_do);

	printf("  Executing:\n\t%s\n", line_to_do);
	do_string(line_to_do);
	c_token++;

    } else {
	int n = 0;		   /* print only <last> entries */
	char *tmp;
	TBOOLEAN append = FALSE;   /* rewrite output file or append it */
	static char *name = NULL;  /* name of the output file; NULL for stdout */

	TBOOLEAN quiet = history_quiet;
	if (!END_OF_COMMAND && almost_equals(c_token,"q$uiet")) {
	    /* option quiet to suppress history entry numbers */
	    quiet = TRUE;
	    c_token++;
	}
	/* show history entries */
	if (!END_OF_COMMAND && isanumber(c_token)) {
	    n = int_expression();
	}
	if ((tmp = try_to_get_string())) {
	    free(name);
	    name = tmp;
	    if (!END_OF_COMMAND && almost_equals(c_token, "ap$pend")) {
		append = TRUE;
		c_token++;
	    }
	}
	write_history_n(n, (quiet ? "" : name), (append ? "a" : "w"));
    }

#else
    c_token++;
    int_warn(NO_CARET, "This copy of gnuplot was built without support for command history.");
#endif /* defined(USE_READLINE) */
}

#define REPLACE_ELSE(tok)             \
do {                                  \
    int idx = token[tok].start_index; \
    token[tok].length = 1;            \
    gp_input_line[idx++] = ';'; /* e */  \
    gp_input_line[idx++] = ' '; /* l */  \
    gp_input_line[idx++] = ' '; /* s */  \
    gp_input_line[idx++] = ' '; /* e */  \
} while (0)

/* Make a copy of an input line substring delimited by { and } */
static char *
new_clause(int clause_start, int clause_end)
{
    char *clause = gp_alloc(clause_end - clause_start, "clause");
    memcpy(clause, &gp_input_line[clause_start+1], clause_end - clause_start);
    clause[clause_end - clause_start - 1] = '\0';
    return clause;
}

/* process the 'if' command */
void
if_command()
{
    double exprval;
    int end_token;

    if (!equals(++c_token, "("))	/* no expression */
	int_error(c_token, "expecting (expression)");
    exprval = real_expression();

    /*
     * EAM May 2011
     * New if {...} else {...} syntax can span multiple lines.
     * Isolate the active clause and execute it recursively.
     */
    if (equals(c_token,"{")) {
	/* Identify start and end position of the clause substring */
	char *clause = NULL;
	int if_start, if_end, else_start=0, else_end=0;
	int clause_start, clause_end;

	c_token = find_clause(&if_start, &if_end);

	if (equals(c_token,"else")) {
	    if (!equals(++c_token,"{"))
		int_error(c_token,"expected {else-clause}");
	    c_token = find_clause(&else_start, &else_end);
	}
	end_token = c_token;

	if (exprval != 0) {
	    clause_start = if_start;
	    clause_end = if_end;
	    if_condition = TRUE;
	} else {
	    clause_start = else_start;
	    clause_end = else_end;
	    if_condition = FALSE;
	}
	if_open_for_else = (else_start) ? FALSE : TRUE;

	if (if_condition || else_start != 0) {
	    clause = new_clause(clause_start, clause_end);
	    begin_clause();
	    do_string_and_free(clause);
	    end_clause();
	}

	if (iteration_early_exit())
	    c_token = num_tokens;
	else
	    c_token = end_token;

	return;
    }

    /*
     * EAM May 2011
     * Old if/else syntax (no curly braces) affects the rest of the current line.
     * Deprecate?
     */
    if (clause_depth > 0)
	int_error(c_token,"Old-style if/else statement encountered inside brackets");
    if_depth++;
    if (exprval != 0.0) {
	/* fake the condition of a ';' between commands */
	int eolpos = token[num_tokens - 1].start_index + token[num_tokens - 1].length;
	--c_token;
	token[c_token].length = 1;
	token[c_token].start_index = eolpos + 2;
	gp_input_line[eolpos + 2] = ';';
	gp_input_line[eolpos + 3] = NUL;

	if_condition = TRUE;
    } else {
	while (c_token < num_tokens) {
	    /* skip over until the next command */
	    while (!END_OF_COMMAND) {
		++c_token;
	    }
	    if (equals(++c_token, "else")) {
		/* break if an "else" was found */
		if_condition = FALSE;
		--c_token; /* go back to ';' */
		return;
	    }
	}
	/* no else found */
	c_token = num_tokens = 0;
    }
}

/* process the 'else' command */
void
else_command()
{
    int end_token;
   /*
    * EAM May 2011
    * New if/else syntax permits else clause to appear on a new line
    */
    if (equals(c_token+1,"{")) {
	int clause_start, clause_end;
	char *clause;

	if (if_open_for_else)
	    if_open_for_else = FALSE;
	else
	    int_error(c_token,"Invalid {else-clause}");

	c_token++;	/* Advance to the opening curly brace */
	end_token = find_clause(&clause_start, &clause_end);

	if (!if_condition) {
	    clause = new_clause(clause_start, clause_end);
	    begin_clause();
	    do_string_and_free(clause);
	    end_clause();
	}

	if (iteration_early_exit())
	    c_token = num_tokens;
	else
	    c_token = end_token;

	return;
    }


   /* EAM May 2011
    * The rest is only relevant to the old if/else syntax (no curly braces)
    */
    if (if_depth <= 0) {
	int_error(c_token, "else without if");
	return;
    } else {
	if_depth--;
    }

    if (TRUE == if_condition) {
	/* First part of line was true so
	 * discard the rest of the line. */
	c_token = num_tokens = 0;
    } else {
	REPLACE_ELSE(c_token);
	if_condition = TRUE;
    }
}

/* process commands of the form 'do for [i=1:N] ...' */
void
do_command()
{
    t_iterator *do_iterator;
    int do_start, do_end;
    int end_token;
    char *clause;

    c_token++;
    do_iterator = check_for_iteration();
    if (forever_iteration(do_iterator)) {
	cleanup_iteration(do_iterator);
	int_error(c_token-2, "unbounded iteration not accepted here");
    }

    if (!equals(c_token,"{")) {
	cleanup_iteration(do_iterator);
	int_error(c_token,"expecting {do-clause}");
    }
    end_token = find_clause(&do_start, &do_end);

    clause = new_clause(do_start, do_end);
    begin_clause();

    iteration_depth++;

    /* Sometimes the start point of a nested iteration is not within the
     * limits for all levels of nesting. In this case we need to advance
     * through the iteration to find the first good set of indices.
     * If we don't find one, forget the whole thing.
     */
    if (empty_iteration(do_iterator) && !next_iteration(do_iterator)) {
	strcpy(clause, ";");
    }

    do {
	requested_continue = FALSE;
	do_string(clause);

	if (command_exit_requested != 0)
	    requested_break = TRUE;
	if (requested_break)
	    break;
    } while (next_iteration(do_iterator));
    iteration_depth--;

    free(clause);
    end_clause();
    c_token = end_token;

    /* FIXME:  If any of the above exited via int_error() then this	*/
    /* cleanup never happens and we leak memory.  But do_iterator can	*/
    /* not be static or global because do_command() can recurse.	*/
    do_iterator = cleanup_iteration(do_iterator);
    requested_break = FALSE;
    requested_continue = FALSE;
}

/* process commands of the form 'while (foo) {...}' */
/* FIXME:  For consistency there should be an iterator associated
 * with this statement.
 */
void
while_command()
{
    int do_start, do_end;
    char *clause;
    int save_token, end_token;
    double exprval;

    c_token++;
    save_token = c_token;
    exprval = real_expression();

    if (!equals(c_token,"{"))
	int_error(c_token,"expecting {while-clause}");
    end_token = find_clause(&do_start, &do_end);

    clause = new_clause(do_start, do_end);
    begin_clause();

    iteration_depth++;
    while (exprval != 0) {
	requested_continue = FALSE;
	do_string(clause);
	if (command_exit_requested)
	    requested_break = TRUE;
	if (requested_break)
	    break;
	c_token = save_token;
	exprval = real_expression();
    };
    iteration_depth--;

    end_clause();
    free(clause);
    c_token = end_token;
    requested_break = FALSE;
    requested_continue = FALSE;
}

/*
 * set link [x2|y2] {via <expression1> {inverse <expression2>}}
 * set nonlinear <axis> via <expression1> inverse <expression2>
 * unset link [x2|y2]
 * unset nonlinear <axis>
 */
void
link_command()
{
    AXIS *primary_axis = NULL;
    AXIS *secondary_axis = NULL;
    TBOOLEAN linked = FALSE;
    int command_token = c_token;	/* points to "link" or "nonlinear" */

    c_token++;

    /* Set variable name accepatable for the via/inverse functions */
	strcpy(c_dummy_var[0], "x");
	strcpy(c_dummy_var[1], "y");
	if (equals(c_token, "z") || equals(c_token, "cb"))
	    strcpy(c_dummy_var[0], "z");
	if (equals(c_token, "r"))
	    strcpy(c_dummy_var[0], "r");

    /*
     * "set nonlinear" currently supports axes x x2 y y2 z r cb
     */
    if (equals(command_token,"nonlinear")) {
	AXIS_INDEX axis;
	if ((axis = lookup_table(axisname_tbl, c_token)) >= 0)
	    secondary_axis = &axis_array[axis];
	else
	    int_error(c_token,"not a valid nonlinear axis");
	primary_axis = get_shadow_axis(secondary_axis);
	/* Trap attempt to set an already-linked axis to nonlinear */
	/* This catches the sequence "set link y; set nonlinear y2" */
	if (secondary_axis->linked_to_primary && secondary_axis->linked_to_primary->index > 0)
	    int_error(NO_CARET,"must unlink axis before setting it to nonlinear");
	if (secondary_axis->linked_to_secondary && secondary_axis->linked_to_secondary->index > 0)
	    int_error(NO_CARET,"must unlink axis before setting it to nonlinear");
	/* Clear previous log status */
	secondary_axis->log = FALSE;
	secondary_axis->ticdef.logscaling = FALSE;

    /*
     * "set link" applies to either x|x2 or y|y2
     * Flag the axes as being linked, and copy the range settings
     * from the primary axis into the linked secondary axis
     */
    } else {
	if (almost_equals(c_token,"x$2")) {
	    primary_axis = &axis_array[FIRST_X_AXIS];
	    secondary_axis = &axis_array[SECOND_X_AXIS];
	} else if (almost_equals(c_token,"y$2")) {
	    primary_axis = &axis_array[FIRST_Y_AXIS];
	    secondary_axis = &axis_array[SECOND_Y_AXIS];
	} else {
	    int_error(c_token,"expecting x2 or y2");
	}
	/* This catches the sequence "set nonlinear x; set link x2" */
	if (primary_axis->linked_to_primary)
	    int_error(NO_CARET, "You must clear nonlinear x or y before linking it");
	/* This catches the sequence "set nonlinear x2; set link x2" */
	if (secondary_axis->linked_to_primary && secondary_axis->linked_to_primary->index <= 0)
	    int_error(NO_CARET, "You must clear nonlinear x2 or y2 before linking it");
    }
    c_token++;

    /* "unset link {x|y}" command */
    if (equals(command_token-1,"unset")) {
	primary_axis->linked_to_secondary = NULL;
	if (secondary_axis->linked_to_primary == NULL)
	    /* It wasn't linked anyhow */
	    return;
	else
	    secondary_axis->linked_to_primary = NULL;
	/* FIXME: could return here except for the need to free link_udf->at */
	linked = FALSE;
    } else {
	linked = TRUE;
    }

    /* Initialize the action tables for the mapping function[s] */
    if (!primary_axis->link_udf) {
	primary_axis->link_udf = gp_alloc(sizeof(udft_entry),"link_at");
	memset(primary_axis->link_udf, 0, sizeof(udft_entry));
    }
    if (!secondary_axis->link_udf) {
	secondary_axis->link_udf = gp_alloc(sizeof(udft_entry),"link_at");
	memset(secondary_axis->link_udf, 0, sizeof(udft_entry));
    }

    if (equals(c_token,"via")) {
	parse_link_via(secondary_axis->link_udf);
	if (almost_equals(c_token,"inv$erse")) {
	    parse_link_via(primary_axis->link_udf);
	} else {
	    int_warn(c_token,"inverse mapping function required");
	    linked = FALSE;
	}
    }

    else if (equals(command_token,"nonlinear") && linked) {
	int_warn(c_token,"via mapping function required");
	linked = FALSE;
    }

    if (equals(command_token,"nonlinear") && linked) {
	/* Save current user-visible axis range (note reversed order!) */
	struct udft_entry *temp = primary_axis->link_udf;
	primary_axis->link_udf = secondary_axis->link_udf;
	secondary_axis->link_udf = temp;
	secondary_axis->linked_to_primary = primary_axis;
	primary_axis->linked_to_secondary = secondary_axis;
	clone_linked_axes(secondary_axis, primary_axis);
    } else if (linked) {
	/* Clone the range information */
	secondary_axis->linked_to_primary = primary_axis;
	primary_axis->linked_to_secondary = secondary_axis;
	clone_linked_axes(primary_axis, secondary_axis);
    } else {
	free_at(secondary_axis->link_udf->at);
	secondary_axis->link_udf->at = NULL;
	free_at(primary_axis->link_udf->at);
	primary_axis->link_udf->at = NULL;
	/* Shouldn't be necessary, but it doesn't hurt */
	primary_axis->linked_to_secondary = NULL;
	secondary_axis->linked_to_primary = NULL;
    }

    if (secondary_axis->index == POLAR_AXIS)
	rrange_to_xy();
}

/* process the 'load' command */
void
load_command()
{
    /* These need to be local so that recursion works. */
    /* They will eventually be freed by lf_pop(). */
    FILE *fp;
    char *save_file;

    c_token++;
    save_file = try_to_get_string();
    if (!save_file)
	int_error(c_token, "expecting filename");
    gp_expand_tilde(&save_file);
    fp = strcmp(save_file, "-") ? loadpath_fopen(save_file, "r") : stdout;
    load_file(fp, save_file, 1);
}


/* null command */
void
null_command()
{
    return;
}

/* Clauses enclosed by curly brackets:
 * do for [i = 1:N] { a; b; c; }
 * if (<test>) {
 *    line1;
 *    line2;
 * } else {
 *    ...
 * }
 */

/* Find the start and end character positions within gp_input_line
 * bounding a clause delimited by {...}.
 * Assumes that c_token indexes the opening left curly brace.
 * Returns the index of the first token after the closing curly brace.
 */
int
find_clause(int *clause_start, int *clause_end)
{
    int i, depth;

    *clause_start = token[c_token].start_index;
    for (i=++c_token, depth=1; i<num_tokens; i++) {
	if (equals(i,"{"))
	    depth++;
	else if (equals(i,"}"))
	    depth--;
	if (depth == 0)
	    break;
    }
    *clause_end = token[i].start_index;

    return (i+1);
}

void
begin_clause()
{
    clause_depth++;
    c_token++;
    return;
}

void
end_clause()
{
    if (clause_depth == 0)
	int_error(c_token, "unexpected }");
    else
	clause_depth--;
    c_token++;
    return;
}

void
clause_reset_after_error()
{
    if (clause_depth)
	FPRINTF((stderr,"CLAUSE RESET after error at depth %d\n",clause_depth));
    clause_depth = 0;
    iteration_depth = 0;
}

/* helper routine to multiplex mouse event handling with a timed pause command */
void
timed_pause(double sleep_time)
{
#if defined(HAVE_USLEEP) && defined(USE_MOUSE) && !defined(_WIN32)
    if (term->waitforinput)		/* If the terminal supports it */
	while (sleep_time > 0.05) {	/* we poll 20 times a second */
	    usleep(50000);		/* Sleep for 50 msec */
	    check_for_mouse_events();
	    sleep_time -= 0.05;
	}
    usleep((useconds_t)(sleep_time * 1e6));
    check_for_mouse_events();
#else
    GP_SLEEP(sleep_time);
#endif
}

/* process the 'pause' command */
#define EAT_INPUT_WITH(slurp) do {int junk=0; do {junk=slurp;} while (junk != EOF && junk != '\n');} while (0)


void
pause_command()
{
    int text = 0;
    double sleep_time;
    static char *buf = NULL;

    c_token++;

#ifdef USE_MOUSE
    paused_for_mouse = 0;
    if (equals(c_token, "mouse")) {
	sleep_time = -1;
	c_token++;

/*	EAM FIXME - This is not the correct test; what we really want */
/*	to know is whether or not the terminal supports mouse feedback */
/*	if (term_initialised) { */
	if (mouse_setting.on && term) {
	    struct udvt_entry *current;
	    int end_condition = 0;

	    while (!(END_OF_COMMAND)) {
		if (almost_equals(c_token, "key$press")) {
		    end_condition |= PAUSE_KEYSTROKE;
		    c_token++;
		} else if (equals(c_token, ",")) {
		    c_token++;
		} else if (equals(c_token, "any")) {
		    end_condition |= PAUSE_ANY;
		    c_token++;
		} else if (equals(c_token, "button1")) {
		    end_condition |= PAUSE_BUTTON1;
		    c_token++;
		} else if (equals(c_token, "button2")) {
		    end_condition |= PAUSE_BUTTON2;
		    c_token++;
		} else if (equals(c_token, "button3")) {
		    end_condition |= PAUSE_BUTTON3;
		    c_token++;
		} else if (equals(c_token, "close")) {
		    end_condition |= PAUSE_WINCLOSE;
		    c_token++;
		} else
		    break;
	    }

	    if (end_condition)
		paused_for_mouse = end_condition;
	    else
		paused_for_mouse = PAUSE_CLICK;

	    /* Set the pause mouse return codes to -1 */
	    current = add_udv_by_name("MOUSE_KEY");
	    Ginteger(&current->udv_value,-1);
	    current = add_udv_by_name("MOUSE_BUTTON");
	    Ginteger(&current->udv_value,-1);
	} else
	    int_warn(NO_CARET, "Mousing not active");
    } else
#endif
	sleep_time = real_expression();

    if (END_OF_COMMAND) {
	free(buf); /* remove the previous message */
	buf = gp_strdup("paused"); /* default message, used in Windows GUI pause dialog */
    } else {
	char *tmp = try_to_get_string();
	if (!tmp)
	    int_error(c_token, "expecting string");
	else {
#ifdef _WIN32
	    free(buf);
	    buf = tmp;
	    if (sleep_time >= 0) {
		fputs(buf, stderr);
	    }
#elif defined(OS2)
	    free(buf);
	    buf = tmp;
	    if (strcmp(term->name, "pm") != 0 || sleep_time >= 0)
		fputs(buf, stderr);
#else /* Not _WIN32 or OS2 */
	    free(buf);
	    buf = tmp;
	    fputs(buf, stderr);
#endif
	    text = 1;
	}
    }

    if (sleep_time < 0) {
#if defined(_WIN32)
	ctrlc_flag = FALSE;
# if defined(WGP_CONSOLE) && defined(USE_MOUSE)
	if (!paused_for_mouse || !MousableWindowOpened()) {
	    int junk = 0;
	    if (buf) {
		/* Use of fprintf() triggers a bug in MinGW + SJIS encoding */
		fputs(buf, stderr); fputs("\n", stderr);
	    }
	    /* Note: cannot use EAT_INPUT_WITH here
	     * FIXME: probably term->waitforinput is always correct, not just for qt
	     */
	    if (!strcmp(term->name,"qt"))
		term->waitforinput(0);
	    else
		do {
		    junk = getch();
		    if (ctrlc_flag)
			bail_to_command_line();
		} while (junk != EOF && junk != '\n' && junk != '\r');
	} else /* paused_for_mouse */
# endif /* !WGP_CONSOLE */
	{
	    if (!Pause(buf)) /* returns false if Ctrl-C or Cancel was pressed */
		bail_to_command_line();
	}
#elif defined(OS2) && defined(USE_MOUSE)
	if (isatty(fileno(stdin)) && strcmp(term->name, "pm") == 0) {
	    int rc = PM_pause(buf);
	    if (rc == 0) {
		/* if (!CallFromRexx)
		 * would help to stop REXX programs w/o raising an error message
		 * in RexxInterface() ...
		 */
		bail_to_command_line();
	    } else if (rc == 2) {
		fputs(buf, stderr);
		text = 1;
		EAT_INPUT_WITH(fgetc(stdin));
	    }
	} else {
	    if (strcmp(term->name, "pm") == 0)
		fputs(buf, stderr);
	    EAT_INPUT_WITH(fgetc(stdin));
	    fputc('\n', stderr);
	}
#else /* !(_WIN32 || OS2) */
# ifdef USE_MOUSE
	if (term && term->waitforinput) {
	    /* It does _not_ work to do EAT_INPUT_WITH(term->waitforinput()) */
	    term->waitforinput(0);
	} else
# endif /* USE_MOUSE */
# ifdef MSDOS
	{
	    int junk;
	    /* cannot use EAT_INPUT_WITH here */
	    do {
#  ifdef __DJGPP__
		/* We use getkey() since with DJGPP 2.05 and gcc 7.2,
		   getchar() requires two keystrokes. */
		junk = getkey();
#  else
		junk = getch();
#  endif
		/* Check if Ctrl-C was pressed */
		if (junk == 0x03)
		    bail_to_command_line();
	    } while (junk != EOF && junk != '\n' && junk != '\r');
	    fputc('\n', stderr);
	}
# else
	    EAT_INPUT_WITH(fgetc(stdin));
# endif

#endif /* !(_WIN32 || OS2) */
    }
    if (sleep_time > 0)
	timed_pause(sleep_time);

    if (text != 0 && sleep_time >= 0)
	fputc('\n', stderr);
    screen_ok = FALSE;

}


/* process the 'plot' command */
void
plot_command()
{
    plot_token = c_token++;
    plotted_data_from_stdin = FALSE;
    refresh_nplots = 0;
    SET_CURSOR_WAIT;
#ifdef USE_MOUSE
    plot_mode(MODE_PLOT);
    add_udv_by_name("MOUSE_X")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_Y")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_X2")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_Y2")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_BUTTON")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_SHIFT")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_ALT")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_CTRL")->udv_value.type = NOTDEFINED;
#endif
    plotrequest();
    /* Clear "hidden" flag for any plots that may have been toggled off */
    if (term->modify_plots)
	term->modify_plots(MODPLOTS_SET_VISIBLE, -1);
    SET_CURSOR_ARROW;
}


void
print_set_output(char *name, TBOOLEAN datablock, TBOOLEAN append_p)
{
    if (print_out && print_out != stderr && print_out != stdout) {
#ifdef PIPES
	if (print_out_name[0] == '|') {
	    if (0 > pclose(print_out))
		perror(print_out_name);
	} else
#endif
	    if (0 > fclose(print_out))
		perror(print_out_name);
	print_out = stderr;
    }

    free(print_out_name);
    print_out_name = NULL;
    print_out_var = NULL;

    if (! name) {
	print_out = stderr;
	return;
    }

    if (strcmp(name, "-") == 0) {
	print_out = stdout;
	return;
    }

#ifdef PIPES
    if (name[0] == '|') {
	restrict_popen();
	print_out = popen(name + 1, "w");
	if (!print_out)
	    perror(name);
	else
	    print_out_name = name;
	return;
    }
#endif

    if (!datablock) {
	print_out = fopen(name, append_p ? "a" : "w");
	if (!print_out) {
	    perror(name);
	    return;
	}
    } else {
	print_out_var = add_udv_by_name(name);
	if (print_out_var == NULL) {
	    fprintf(stderr, "Error allocating datablock \"%s\"\n", name);
	    return;
	}
	if (print_out_var->udv_value.type != NOTDEFINED) {
	    gpfree_string(&print_out_var->udv_value);
	    if (!append_p)
		gpfree_datablock(&print_out_var->udv_value);
	    if (print_out_var->udv_value.type != DATABLOCK)
		print_out_var->udv_value.v.data_array = NULL;
	} else {
	    print_out_var->udv_value.v.data_array = NULL;
	}
	print_out_var->udv_value.type = DATABLOCK;
    }

    print_out_name = name;
}

char *
print_show_output()
{
    if (print_out_name)
	return print_out_name;
    if (print_out == stdout)
	return "<stdout>";
    if (!print_out || print_out == stderr || !print_out_name)
	return "<stderr>";
    return print_out_name;
}

/* 'printerr' is the same as 'print' except that output is always to stderr */
void
printerr_command()
{
    FILE *save_print_out = print_out;
    struct udvt_entry *save_print_out_var = print_out_var;

    print_out = stderr;
    print_out_var = NULL;
    print_command();
    print_out_var = save_print_out_var;
    print_out = save_print_out;
}

/* process the 'print' command */
void
print_command()
{
    struct value a;
    /* space is not needed for the first entry */
    TBOOLEAN need_space = FALSE;

    char *dataline = NULL;
    size_t size = 256;
    size_t len = 0;

    if (!print_out)
	print_out = stderr;
    if (print_out_var != NULL) { /* print to datablock */
	dataline = (char *) gp_alloc(size, "dataline");
	*dataline = NUL;
    }
    screen_ok = FALSE;
    do {
	++c_token;
	if (equals(c_token, "$") && isletter(c_token+1) && !equals(c_token+2,"[")) {
	    char *datablock_name = parse_datablock_name();
	    char **line = get_datablock(datablock_name);

	    /* Printing a datablock into itself would cause infinite recursion */
	    if (print_out_var && !strcmp(datablock_name, print_out_name))
		continue;
	    if (need_space && !print_out_var)
		fprintf(print_out, "\n");

	    while (line && *line) {
		if (print_out_var != NULL)
		    append_to_datablock(&print_out_var->udv_value, strdup(*line));
		else
		    fprintf(print_out, "%s\n", *line);
		line++;
	    }
	    need_space = FALSE;
	    continue;
	}

	/* All entries other than the first one on a line */
	if (need_space) {
	    if (dataline != NULL)
		len = strappend(&dataline, &size, len, " ");
	    else
		fputs(" ", print_out);
	}
	need_space = TRUE;

	if (type_udv(c_token) == ARRAY && !equals(c_token+1, "[")) {
	    struct value *array = add_udv(c_token++)->udv_value.v.value_array;
	    if (dataline != NULL) {
		int i;
		int arraysize = array[0].v.int_val;
		len = strappend(&dataline, &size, len, "[");
		for (i = 1; i <= arraysize; i++) {
		    if (array[i].type != NOTDEFINED)
			len = strappend(&dataline, &size, len, value_to_str(&array[i], TRUE));
		    if (i < arraysize)
			len = strappend(&dataline, &size, len, ",");
		}
		len = strappend(&dataline, &size, len, "]");
	    } else {
		save_array_content(print_out, array);
	    }
	    continue;
	}
	const_express(&a);
	if (a.type == STRING) {
	    if (dataline != NULL)
		len = strappend(&dataline, &size, len, a.v.string_val);
	    else
		fputs(a.v.string_val, print_out);
	    gpfree_string(&a);
	} else {
	    if (dataline != NULL)
		len = strappend(&dataline, &size, len, value_to_str(&a, FALSE));
	    else
		disp_value(print_out, &a, FALSE);
	}

    } while (!END_OF_COMMAND && equals(c_token, ","));

    if (dataline != NULL) {
	append_multiline_to_datablock(&print_out_var->udv_value, dataline);
    } else {
	(void) putc('\n', print_out);
	fflush(print_out);
    }
}


/* process the 'pwd' command */
void
pwd_command()
{
    char *save_file = NULL;

    save_file = gp_alloc(PATH_MAX, "print current dir");
    if (GP_GETCWD(save_file, PATH_MAX) == NULL)
	fprintf(stderr, "<invalid>\n");
    else
	fprintf(stderr, "%s\n", save_file);
    free(save_file);
    c_token++;
}


/* EAM April 2007
 * The "refresh" command replots the previous graph without going back to read
 * the original data. This allows zooming or other operations on data that was
 * only transiently available in the input stream.
 */
void
refresh_command()
{
    c_token++;
    refresh_request();
}

void
refresh_request()
{
    AXIS_INDEX axis;

    if (   ((first_plot == NULL) && (refresh_ok == E_REFRESH_OK_2D))
	|| ((first_3dplot == NULL) && (refresh_ok == E_REFRESH_OK_3D))
	|| (!*replot_line && (refresh_ok == E_REFRESH_NOT_OK))
       )
	int_error(NO_CARET, "no active plot; cannot refresh");

    if (refresh_ok == E_REFRESH_NOT_OK) {
	int_warn(NO_CARET, "cannot refresh from this state. trying full replot");
	replotrequest();
	return;
    }

    /* The margins from "set offset" were already applied;
     * don't reapply them here
     */
    retain_offsets = TRUE;

    /* Restore the axis range/scaling state from original plot
     * Dima Kogan April 2018
     */
    for (axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
	AXIS *this_axis = &axis_array[axis];
	if ((this_axis->set_autoscale & AUTOSCALE_MIN)
	&&  (this_axis->writeback_min < VERYLARGE))
	    this_axis->set_min = this_axis->writeback_min;
	else
	    this_axis->min = this_axis->set_min;
	if ((this_axis->set_autoscale & AUTOSCALE_MAX)
	&&  (this_axis->writeback_max > -VERYLARGE))
	    this_axis->set_max = this_axis->writeback_max;
	else
	    this_axis->max = this_axis->set_max;

	if (this_axis->linked_to_secondary)
	    clone_linked_axes(this_axis, this_axis->linked_to_secondary);
	else if (this_axis->linked_to_primary) {
	    if (this_axis->linked_to_primary->autoscale != AUTOSCALE_BOTH)
	    clone_linked_axes(this_axis, this_axis->linked_to_primary);
	}
    }

    if (refresh_ok == E_REFRESH_OK_2D) {
	refresh_bounds(first_plot, refresh_nplots);
	do_plot(first_plot, refresh_nplots);
	update_gpval_variables(1);
    } else if (refresh_ok == E_REFRESH_OK_3D) {
	refresh_3dbounds(first_3dplot, refresh_nplots);
	do_3dplot(first_3dplot, refresh_nplots, 0);
	update_gpval_variables(1);
    } else
	int_error(NO_CARET, "Internal error - refresh of unknown plot type");

}

/* process the 'replot' command */
void
replot_command()
{
    if (!*replot_line)
	int_error(c_token, "no previous plot");

    if (volatile_data && (refresh_ok != E_REFRESH_NOT_OK) && !replot_disabled) {
	FPRINTF((stderr,"volatile_data %d refresh_ok %d plotted_data_from_stdin %d\n",
		volatile_data, refresh_ok, plotted_data_from_stdin));
	refresh_command();
	return;
    }

    /* Disable replot for some reason; currently used by the mouse/hotkey
       capable terminals to avoid replotting when some data come from stdin,
       i.e. when  plotted_data_from_stdin==1  after plot "-".
    */
    if (replot_disabled) {
	replot_disabled = FALSE;
	bail_to_command_line(); /* be silent --- don't mess the screen */
    }
    if (!term) /* unknown terminal */
	int_error(c_token, "use 'set term' to set terminal type first");

    c_token++;
    SET_CURSOR_WAIT;
    if (term->flags & TERM_INIT_ON_REPLOT)
	term->init();
    replotrequest();
    SET_CURSOR_ARROW;
}


/* process the 'reread' command */
void
reread_command()
{
    FILE *fp = lf_top();

    if (fp != (FILE *) NULL)
	rewind(fp);
    c_token++;
}


/* process the 'save' command */
void
save_command()
{
    FILE *fp;
    char *save_file = NULL;
    int what;

    c_token++;
    what = lookup_table(&save_tbl[0], c_token);

    switch (what) {
	case SAVE_FUNCS:
	case SAVE_SET:
	case SAVE_TERMINAL:
	case SAVE_VARS:
	case SAVE_FIT:
	    c_token++;
	    break;
	default:
	    break;
    }

    save_file = try_to_get_string();
    if (!save_file)
	    int_error(c_token, "expecting filename");
#ifdef PIPES
    if (save_file[0]=='|') {
	restrict_popen();
	fp = popen(save_file+1,"w");
    } else
#endif
    {
    gp_expand_tilde(&save_file);
#ifdef _WIN32
    fp = strcmp(save_file,"-") ? loadpath_fopen(save_file,"w") : stdout;
#else
    fp = strcmp(save_file,"-") ? fopen(save_file,"w") : stdout;
#endif
    }

    if (!fp)
	os_error(c_token, "Cannot open save file");

    switch (what) {
	case SAVE_FUNCS:
	    save_functions(fp);
	break;
    case SAVE_SET:
	    save_set(fp);
	break;
    case SAVE_TERMINAL:
	    save_term(fp);
	break;
    case SAVE_VARS:
	    save_variables(fp);
	break;
    case SAVE_FIT:
	    save_fit(fp);
	break;
    default:
	    save_all(fp);
    }

    if (stdout != fp) {
#ifdef PIPES
	if (save_file[0] == '|')
	    (void) pclose(fp);
	else
#endif
	    (void) fclose(fp);
    }

    free(save_file);
}


/* process the 'screendump' command */
void
screendump_command()
{
    c_token++;
#ifdef _WIN32
    screen_dump();
#else
    fputs("screendump not implemented\n", stderr);
#endif
}


/* set_command() is in set.c */

/* 'shell' command is processed by do_shell(), see below */

/* show_command() is in show.c */


/* process the 'splot' command */
void
splot_command()
{
    plot_token = c_token++;
    plotted_data_from_stdin = FALSE;
    refresh_nplots = 0;
    SET_CURSOR_WAIT;
#ifdef USE_MOUSE
    plot_mode(MODE_SPLOT);
    add_udv_by_name("MOUSE_X")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_Y")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_X2")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_Y2")->udv_value.type = NOTDEFINED;
    add_udv_by_name("MOUSE_BUTTON")->udv_value.type = NOTDEFINED;
#endif
    plot3drequest();
    /* Clear "hidden" flag for any plots that may have been toggled off */
    if (term->modify_plots)
	term->modify_plots(MODPLOTS_SET_VISIBLE, -1);
    SET_CURSOR_ARROW;
}

/* process the 'stats' command */
void
stats_command()
{
#ifdef USE_STATS
    statsrequest();
#else
    int_error(NO_CARET,"This copy of gnuplot was not configured with support for the stats command");
#endif
}

/* process the 'system' command */
void
system_command()
{
    char *cmd;
    ++c_token;
    cmd = try_to_get_string();
    do_system(cmd);
    free(cmd);
}


/*
 * process the 'test palette' command
 * 1) Write a sequence of plot commands + set commands into a temp file
 * 2) Create a datablock with palette values
 * 3) Load the temp file to plot from the datablock
 *    The set commands then act to restore the initial state
 */
static void
test_palette_subcommand()
{
    enum {test_palette_colors = 256};
    struct udvt_entry *datablock;
    char *save_replot_line;
    TBOOLEAN save_is_3d_plot;
    int i;

    static const char pre1[] = "\
reset;\
uns border; se tics scale 0;\
se cbtic 0,0.1,1 mirr format '' scale 1;\
se xr[0:1];se yr[0:1];se zr[0:1];se cbr[0:1];\
set colorbox hor user orig 0.05,0.02 size 0.925,0.12;";

    static const char pre2[] = "\
se lmarg scre 0.05;se rmarg scre 0.975; se bmarg scre 0.22; se tmarg scre 0.86;\
se grid; se xtics 0,0.1;se ytics 0,0.1;\
se key top right at scre 0.975,0.975 horizontal \
title 'R,G,B profiles of the current color palette';";

    static const char pre3[] = "\
p NaN lc palette notit,\
$PALETTE u 1:2 t 'red' w l lt 1 lc rgb 'red',\
'' u 1:3 t 'green' w l lt 1 lc rgb 'green',\
'' u 1:4 t 'blue' w l lt 1 lc rgb 'blue',\
'' u 1:5 t 'NTSC' w l lt 1 lc rgb 'black'\
\n";

    FILE *f = tmpfile();

#if defined(_MSC_VER) || defined(__MINGW32__)
    /* On Vista/Windows 7 tmpfile() fails. */
    if (!f) {
	char buf[PATH_MAX];
	/* We really want the "ANSI" version */
	GetTempPathA(sizeof(buf), buf);
	strcat(buf, "gnuplot-pal.tmp");
	f = fopen(buf, "w+");
    }
#endif

    while (!END_OF_COMMAND)
	c_token++;
    if (!f)
	int_error(NO_CARET, "cannot write temporary file");

    /* Store R/G/B/Int curves in a datablock */
    datablock = add_udv_by_name("$PALETTE");
    if (datablock->udv_value.type != NOTDEFINED)
	gpfree_datablock(&datablock->udv_value);
    datablock->udv_value.type = DATABLOCK;
    datablock->udv_value.v.data_array = NULL;

    /* Part of the purpose for writing these values into a datablock */
    /* is so that the user can read them back if desired.  But data  */
    /* will be read back using the current numeric locale, so for    */
    /* consistency we must also use the locale when creating it.     */
    set_numeric_locale();
    for (i = 0; i < test_palette_colors; i++) {
	char dataline[64];
	rgb_color rgb;
	double ntsc;
	double z = (double)i / (test_palette_colors - 1);
	double gray = (sm_palette.positive == SMPAL_NEGATIVE) ? 1. - z : z;
	rgb1_from_gray(gray, &rgb);
	ntsc = 0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b;
	sprintf(dataline, "%0.4f %0.4f %0.4f %0.4f %0.4f %c",
		z, rgb.r, rgb.g, rgb.b, ntsc, '\0');
	append_to_datablock(&datablock->udv_value, strdup(dataline));
    }
    reset_numeric_locale();

    /* commands to setup the test palette plot */
    enable_reset_palette = 0;
    save_replot_line = gp_strdup(replot_line);
    save_is_3d_plot = is_3d_plot;
    fputs(pre1, f);
    fputs(pre2, f);
    fputs(pre3, f);

    /* save current gnuplot 'set' status because of the tricky sets
     * for our temporary testing plot.
     */
    save_set(f);

    /* execute all commands from the temporary file */
    rewind(f);
    load_file(f, NULL, 1); /* note: it does fclose(f) */

    /* enable reset_palette() and restore replot line */
    enable_reset_palette = 1;
    free(replot_line);
    replot_line = save_replot_line;
    is_3d_plot = save_is_3d_plot;
}

/* process the 'test' command */
void
test_command()
{
    int what;
    int save_token = c_token++;

    if (!term) /* unknown terminal */
	int_error(c_token, "use 'set term' to set terminal type first");

    what = lookup_table(&test_tbl[0], c_token);
    switch (what) {
	default:
	    if (!END_OF_COMMAND)
		int_error(c_token, "unrecognized test option");
	    /* otherwise fall through to test_term */
	case TEST_TERMINAL: test_term(); break;
	case TEST_PALETTE: test_palette_subcommand(); break;
    }

    /* prevent annoying error messages if there was no previous plot */
    /* and the "test" window is resized. */
    if (!replot_line || !(*replot_line)) {
	m_capture( &replot_line, save_token, c_token );
    }
}

/* toggle a single plot on/off from the command line
 * (only possible for qt, wxt, x11, win)
 */
void
toggle_command()
{
    int plotno = -1;
    char *plottitle = NULL;
    TBOOLEAN foundit = FALSE;

    c_token++;

    if (equals(c_token, "all")) {
	c_token++;

    } else if ((plottitle = try_to_get_string()) != NULL) {
	struct curve_points *plot;
	int last = strlen(plottitle) - 1;
	if (refresh_ok == E_REFRESH_OK_2D)
	    plot = first_plot;
	else if (refresh_ok == E_REFRESH_OK_3D)
	    plot = (struct curve_points *)first_3dplot;
	else
	    plot = NULL;
	if (last >= 0) {
	    for (plotno = 0; plot != NULL; plot = plot->next, plotno++) {
		if (plot->title)
		    if (!strcmp(plot->title, plottitle)
		    ||  (plottitle[last] == '*'
			&& !strncmp(plot->title, plottitle, last))) {
			foundit = TRUE;
			break;
		    }
	    }
	}
	free(plottitle);
	if (!foundit) {
	    int_warn(NO_CARET,"Did not find a plot with that title");
	    return;
	}

    } else {
	plotno = int_expression() - 1;
    }

    if (term->modify_plots)
	term->modify_plots(MODPLOTS_INVERT_VISIBILITIES, plotno);
}

void
update_command()
{
    int_error(NO_CARET, "DEPRECATED command 'update', please use 'save fit' instead");
}

/* the "import" command is only implemented if support is configured for */
/* using functions from external shared objects as plugins. */
void
import_command()
{
    int start_token = c_token;

#ifdef HAVE_EXTERNAL_FUNCTIONS
    struct udft_entry *udf;

    int dummy_num = 0;
    char save_dummy[MAX_NUM_VAR][MAX_ID_LEN+1];

    if (!equals(++c_token + 1, "("))
	int_error(c_token, "Expecting function template");

    memcpy(save_dummy, c_dummy_var, sizeof(save_dummy));
    do {
	c_token += 2;	/* skip to the next dummy */
	copy_str(c_dummy_var[dummy_num++], c_token, MAX_ID_LEN);
    } while (equals(c_token + 1, ",") && (dummy_num < MAX_NUM_VAR));
    if (equals(++c_token, ","))
	int_error(c_token + 1, "function contains too many parameters");
    if (!equals(c_token++, ")"))
	int_error(c_token, "missing ')'");

    if (!equals(c_token, "from"))
	int_error(c_token, "Expecting 'from <sharedobj>'");
    c_token++;

    udf = dummy_func = add_udf(start_token+1);
    udf->dummy_num = dummy_num;
    free_at(udf->at);	/* In case there was a previous function by this name */

    udf->at = external_at(udf->udf_name);
    memcpy(c_dummy_var, save_dummy, sizeof(save_dummy));
    dummy_func = NULL;	/* dont let anyone else use our workspace */

    if (!udf->at)
	int_error(NO_CARET, "failed to load external function");

    /* Don't copy the definition until we know it worked */
    m_capture(&(udf->definition), start_token, c_token - 1);

#else
    while (!END_OF_COMMAND)
	c_token++;
    int_error(start_token, "This copy of gnuplot does not support plugins");
#endif
}

/* process invalid commands and, on OS/2, REXX commands */
void
invalid_command()
{
    int save_token = c_token;
#ifdef OS2
    if (token[c_token].is_token) {
      int rc;
      rc = ExecuteMacro(gp_input_line + token[c_token].start_index,
	      token[c_token].length);
      if (rc == 0) {
	 c_token = num_tokens = 0;
	 return;
      }
    }
#endif

    /* Skip the rest of the command; otherwise we're left pointing to */
    /* the middle of a command we already know is not valid.          */
    while (!END_OF_COMMAND)
	c_token++;
    int_error(save_token, "invalid command");
}


/*
 * Auxiliary routines
 */

/* used by changedir_command() */
static int
changedir(char *path)
{
#if defined(MSDOS)
    /* first deal with drive letter */

    if (isalpha((unsigned char)path[0]) && (path[1] == ':')) {
	int driveno = toupper((unsigned char)path[0]) - 'A';	/* 0=A, 1=B, ... */

# if defined(__EMX__) || defined(__WATCOMC__)
	(void) _chdrive(driveno + 1);
# elif defined(__DJGPP__)
	(void) setdisk(driveno);
# endif
	path += 2;		/* move past drive letter */
    }
    /* then change to actual directory */
    if (*path)
	if (chdir(path))
	    return 1;

    return 0;			/* should report error with setdrive also */

#elif defined(_WIN32)
    LPWSTR pathw = UnicodeText(path, encoding);
    int ret = !SetCurrentDirectoryW(pathw);
    free(pathw);
    return ret;
#elif defined(__EMX__) && defined(OS2)
    return _chdir2(path);
#else
    return chdir(path);
#endif /* MSDOS etc. */
}

/* used by replot_command() */
void
replotrequest()
{
    /* do not store directly into the replot_line string until the
     * new plot line has been successfully plotted. This way,
     * if user makes a typo in a replot line, they do not have
     * to start from scratch. The replot_line will be committed
     * after do_plot has returned, whence we know all is well
     */
    if (END_OF_COMMAND) {
	char *rest_args = &gp_input_line[token[c_token].start_index];
	size_t replot_len = strlen(replot_line);
	size_t rest_len = strlen(rest_args);

	/* preserve commands following 'replot ;' */
	/* move rest of input line to the start
	 * necessary because of realloc() in extend_input_line() */
	memmove(gp_input_line,rest_args,rest_len+1);
	/* reallocs if necessary */
	while (gp_input_line_len < replot_len+rest_len+1)
	    extend_input_line();
	/* move old rest args off begin of input line to
	 * make space for replot_line */
	memmove(gp_input_line+replot_len,gp_input_line,rest_len+1);
	/* copy previous plot command to start of input line */
	memcpy(gp_input_line, replot_line, replot_len);
    } else {
	char *replot_args = NULL;	/* else m_capture will free it */
	int last_token = num_tokens - 1;

	/* length = length of old part + length of new part + ", " + \0 */
	size_t newlen = strlen(replot_line) + token[last_token].start_index
		      + token[last_token].length - token[c_token].start_index + 3;

	m_capture(&replot_args, c_token, last_token);	/* might be empty */
	while (gp_input_line_len < newlen)
	    extend_input_line();
	strcpy(gp_input_line, replot_line);
	strcat(gp_input_line, ", ");
	strcat(gp_input_line, replot_args);
	free(replot_args);
    }
    plot_token = 0;		/* whole line to be saved as replot line */
    SET_REFRESH_OK(E_REFRESH_NOT_OK, 0);		/* start of replot will destroy existing data */

    screen_ok = FALSE;
    num_tokens = scanner(&gp_input_line, &gp_input_line_len);
    c_token = 1;	/* Skip the "plot" token */

    if (almost_equals(0,"test")) {
	c_token = 0;
	test_command();
    } else if (almost_equals(0,"s$plot"))
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
# include <ssdef.h>

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

/* HBB 990829: confirmed this to be used on VMS, only --> moved into
 * the VMS-specific section */
void
done(int status)
{
    term_reset();
    gp_exit(status);
}

/* VMS-only version of read_line */
static int
read_line(const char *prompt, int start)
{
    int more;
    char expand_prompt[40];

    current_prompt = prompt;	/* HBB NEW 20040727 */

    prompt_desc.dsc$w_length = strlen(prompt);
    prompt_desc.dsc$a_pointer = (char *) prompt;
    strcpy(expand_prompt, "_");
    strncat(expand_prompt, prompt, sizeof(expand_prompt) - 2);
    do {
	line_desc.dsc$w_length = MAX_LINE_LEN - start;
	line_desc.dsc$a_pointer = &gp_input_line[start];
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
	gp_input_line[start] = NUL;
	inline_num++;
	if (gp_input_line[start - 1] == '\\') {
	    /* Allow for a continuation line. */
	    prompt_desc.dsc$w_length = strlen(expand_prompt);
	    prompt_desc.dsc$a_pointer = expand_prompt;
	    more = 1;
	    --start;
	} else {
	    line_desc.dsc$w_length = strlen(gp_input_line);
	    line_desc.dsc$a_pointer = gp_input_line;
	    more = 0;
	}
    } while (more);
    return 0;
}


# ifdef NO_GIH
void
help_command()
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


void
do_shell()
{
    screen_ok = FALSE;
    c_token++;

    if ((vaxc$errno = lib$spawn()) != SS$_NORMAL) {
	os_error(NO_CARET, "spawn error");
    }
}


static void
do_system(const char *cmd)
{

     if (!cmd)
	return;

    /* gp_input_line is filled by read_line or load_file, but
     * line_desc length is set only by read_line; adjust now
     */
    line_desc.dsc$w_length = strlen(cmd);
    line_desc.dsc$a_pointer = (char *) cmd;

    if ((vaxc$errno = lib$spawn(&line_desc)) != SS$_NORMAL)
	os_error(NO_CARET, "spawn error");

    (void) putc('\n', stderr);

}
#endif /* VMS */


#ifdef NO_GIH
#ifdef _WIN32
void
help_command()
{
    HWND parent;

    c_token++;
    parent = GetDesktopWindow();

    /* open help file if necessary */
    help_window = HtmlHelp(parent, winhelpname, HH_GET_WIN_HANDLE, (DWORD_PTR)NULL);
    if (help_window == NULL) {
	help_window = HtmlHelp(parent, winhelpname, HH_DISPLAY_TOPIC, (DWORD_PTR)NULL);
	if (help_window == NULL) {
	    fprintf(stderr, "Error: Could not open help file \"" TCHARFMT "\"\n", winhelpname);
	    return;
	}
    }
    if (END_OF_COMMAND) {
	/* show table of contents */
	HtmlHelp(parent, winhelpname, HH_DISPLAY_TOC, (DWORD_PTR)NULL);
    } else {
	/* lookup topic in index */
	HH_AKLINK link;
	char buf[128];
#ifdef UNICODE
	WCHAR wbuf[128];
#endif
	int start = c_token;
	while (!(END_OF_COMMAND))
	    c_token++;
	capture(buf, start, c_token - 1, sizeof(buf));
	link.cbStruct =     sizeof(HH_AKLINK) ;
	link.fReserved =    FALSE;
#ifdef UNICODE
	MultiByteToWideChar(WinGetCodepage(encoding), 0, buf, sizeof(buf), wbuf, sizeof(wbuf) / sizeof(WCHAR));
	link.pszKeywords =  wbuf;
#else
	link.pszKeywords =  buf;
#endif
	link.pszUrl =       NULL;
	link.pszMsgText =   NULL;
	link.pszMsgTitle =  NULL;
	link.pszWindow =    NULL;
	link.fIndexOnFail = TRUE;
	HtmlHelp(parent, winhelpname, HH_KEYWORD_LOOKUP, (DWORD_PTR)&link);
    }
}
#else  /* !_WIN32 */
#ifndef VMS
void
help_command()
{
    while (!(END_OF_COMMAND))
	c_token++;
    fputs("This gnuplot was not built with inline help\n", stderr);
}
#endif /* VMS */
#endif /* _WIN32 */
#endif /* NO_GIH */


/*
 * help_command: (not VMS, although it would work) Give help to the user. It
 * parses the command line into helpbuf and supplies help for that string.
 * Then, if there are subtopics available for that key, it prompts the user
 * with this string. If more input is given, help_command is called
 * recursively, with argument 0.  Thus a more specific help can be supplied.
 * This can be done repeatedly.  If null input is given, the function returns,
 * effecting a backward climb up the tree.
 * David Kotz (David.Kotz@Dartmouth.edu) 10/89
 * drd - The help buffer is first cleared when called with toplevel=1.
 * This is to fix a bug where help is broken if ^C is pressed whilst in the
 * help.
 * Lars - The "int toplevel" argument is gone. I have converted it to a
 * static variable.
 *
 * FIXME - helpbuf is never free()'d
 */

#ifndef NO_GIH
void
help_command()
{
    static char *helpbuf = NULL;
    static char *prompt = NULL;
    static int toplevel = 1;
    int base;			/* index of first char AFTER help string */
    int len;			/* length of current help string */
    TBOOLEAN more_help;
    TBOOLEAN only;		/* TRUE if only printing subtopics */
    TBOOLEAN subtopics;		/* 0 if no subtopics for this topic */
    int start;			/* starting token of help string */
    char *help_ptr;		/* name of help file */
# if defined(SHELFIND)
    static char help_fname[256] = "";	/* keep helpfilename across calls */
# endif

    if ((help_ptr = getenv("GNUHELP")) == (char *) NULL)
# ifndef SHELFIND
	/* if can't find environment variable then just use HELPFILE */
#  if defined(MSDOS) || defined(OS2)
	help_ptr = HelpFile;
#  else
	help_ptr = HELPFILE;
#  endif

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

    /* if called recursively, toplevel == 0; toplevel must == 1 if called
     * from command() to get the same behaviour as before when toplevel
     * supplied as function argument
     */
    toplevel = 1;

    len = base = strlen(helpbuf);

    start = ++c_token;

    /* find the end of the help command */
    while (!(END_OF_COMMAND))
	c_token++;

    /* copy new help input into helpbuf */
    if (len > 0)
	helpbuf[len++] = ' ';	/* add a space */
    capture(helpbuf + len, start, c_token - 1, MAX_LINE_LEN - len);
    squash_spaces(helpbuf + base, 1);	/* only bother with new stuff */
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
		    if (len > 0) {
			strcpy (prompt, "Subtopic of ");
			strncat (prompt, helpbuf, MAX_LINE_LEN - 16);
			strcat (prompt, ": ");
		    } else
			strcpy(prompt, "Help topic: ");
		    read_line(prompt, 0);
		    num_tokens = scanner(&gp_input_line, &gp_input_line_len);
		    c_token = 0;
		    more_help = !(END_OF_COMMAND);
		    if (more_help) {
			c_token--;
			toplevel = 0;
			/* base for next level is all of current helpbuf */
			help_command();
		    }
		} else
		    more_help = FALSE;
	    } while (more_help);

	    break;
	}
    case H_NOTFOUND:
	printf("Sorry, no help for '%s'\n", helpbuf);
	break;
    case H_ERROR:
	perror(help_ptr);
	break;
    default:
	int_error(NO_CARET, "Impossible case in switch");
	break;
    }

    helpbuf[base] = NUL;	/* cut it off where we started */
}
#endif /* !NO_GIH */

#ifndef VMS

static void
do_system(const char *cmd)
{
    int ierr;

/* (am, 19980929)
 * OS/2 related note: cmd.exe returns 255 if called w/o argument.
 * i.e. calling a shell by "!" will always end with an error message.
 * A workaround has to include checking for EMX,OS/2, two environment
 *  variables,...
 */
    if (!cmd)
	return;
    restrict_popen();
#if defined(_WIN32) && !defined(WGP_CONSOLE)
    /* Open a console so we can see the command's output */
    WinOpenConsole();
#endif
#if defined(_WIN32) && !defined(HAVE_BROKEN_WSYSTEM)
    {
	LPWSTR wcmd = UnicodeText(cmd, encoding);
	ierr = _wsystem(wcmd);
	free(wcmd);
    }
#else
    ierr = system(cmd);
#endif
    report_error(ierr);
}

/* is_history_command:
   Test if line starts with an (abbreviated) history command.
   Modified copy of almost_equals() (util.c).
*/
static TBOOLEAN
is_history_command(const char *line)
{
    int i;
    int start = 0;
    int length = 0;
    int after = 0;
    const char str[] = "hi$story";

    /* skip leading whitespace */
    while (isblank((unsigned char) line[start]))
	start++;

    /* find end of "token" */
    while ((line[start + length] != NUL) && !isblank((unsigned char) line[start + length]))
	length++;

    for (i = 0; i < length + after; i++) {
	if (str[i] != line[start + i]) {
	    if (str[i] != '$')
		return FALSE;
	    else {
		after = 1;
		start--;	/* back up token ptr */
	    }
	}
    }

    /* i now beyond end of token string */

    return (after || str[i] == '$' || str[i] == NUL);
}


# ifdef USE_READLINE
/* keep some compilers happy */
static char *rlgets(char *s, size_t n, const char *prompt);

static char *
rlgets(char *s, size_t n, const char *prompt)
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
#  if defined(READLINE) || defined(HAVE_LIBREADLINE)
	    int found;
	    /* Initialize readline history functions */
	    using_history();

	    /* search in the history for entries containing line.
	     * They may have other tokens before and after line, hence
	     * the check on strcmp below. */
	    if (!is_history_command(line)) {
		if (!history_full) {
		    found = history_search(line, -1);
		    if (found != -1 && !strcmp(current_history()->line,line)) {
			/* this line is already in the history, remove the earlier entry */
			HIST_ENTRY *removed = remove_history(where_history());
			/* according to history docs we are supposed to free the stuff */
			if (removed) {
			    free(removed->line);
			    free(removed->data);
			    free(removed);
			}
		    }
		}
		add_history(line);
	    }
#  elif defined(HAVE_LIBEDITLINE)
	    if (!is_history_command(line)) {
		/* deleting history entries does not work, so suppress adjacent duplicates only */
		int found = 0;
		using_history();

		if (!history_full)
		    found = history_search(line, -1);
		if (found <= 0)
		    add_history(line);
	    }
#  endif
	}
    }
    if (line) {
	/* s will be NUL-terminated here */
	safe_strncpy(s, line + leftover, n);
	leftover += strlen(s);
	if (line[leftover] == NUL)
	    leftover = -1;
	return s;
    }
    return NULL;
}
# endif				/* USE_READLINE */


# if defined(MSDOS) || defined(_WIN32)

void
do_shell()
{
    screen_ok = FALSE;
    c_token++;

    if (user_shell) {
#  if defined(_WIN32)
	if (WinExec(user_shell, SW_SHOWNORMAL) <= 32)
#  elif defined(__DJGPP__)
	if (system(user_shell) == -1)
#  else
	if (spawnl(P_WAIT, user_shell, NULL) == -1)
#  endif /* !(_WIN32 || __DJGPP__) */
	    os_error(NO_CARET, "unable to spawn shell");
    }
}

#  elif defined(OS2)

void
do_shell()
{
    screen_ok = FALSE;
    c_token++;

    if (user_shell) {
	if (system(user_shell) == -1)
	    os_error(NO_CARET, "system() failed");
    }
    (void) putc('\n', stderr);
}

#  else				/* !OS2 */

/* plain old Unix */

#define EXEC "exec "
void
do_shell()
{
    static char exec[100] = EXEC;

    screen_ok = FALSE;
    c_token++;

    if (user_shell) {
	if (system(safe_strncpy(&exec[sizeof(EXEC) - 1], user_shell,
				sizeof(exec) - sizeof(EXEC) - 1)))
	    os_error(NO_CARET, "system() failed");
    }
    (void) putc('\n', stderr);
}

# endif				/* !MSDOS */

/* read from stdin, everything except VMS */

# ifndef USE_READLINE
#  if defined(MSDOS) && !defined(__EMX__) && !defined(__DJGPP__)

/* if interactive use console IO so CED will work */

#define PUT_STRING(s) cputs(s)
#define GET_STRING(s,l) ((interactive) ? cgets_emu(s,l) : fgets(s,l,stdin))


/* emulate a fgets like input function with DOS cgets */
char *
cgets_emu(char *str, int len)
{
    static char buffer[128] = "";
    static int leftover = 0;

    if (buffer[leftover] == NUL) {
	buffer[0] = 126;
	cgets(buffer);
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
# endif				/* !USE_READLINE */

/* this function is called for non-interactive operation. Its usage is
 * like fgets(), but additionally it checks for ipc events from the
 * terminals waitforinput() (if USE_MOUSE, and terminal is
 * mouseable). This function will be used when reading from a pipe.
 * fgets() reads in at most one less than size characters from stream and
 * stores them into the buffer pointed to by s.
 * Reading stops after an EOF or a newline.  If a newline is read, it is
 * stored into the buffer.  A '\0' is stored  after the last character in
 * the buffer. */
static char*
fgets_ipc(
    char *dest,			/* string to fill */
    int len)			/* size of it */
{
#ifdef USE_MOUSE
    if (term && term->waitforinput) {
	/* This a mouseable terminal --- must expect input from it */
	int c;			/* char gotten from waitforinput() */
	size_t i=0;		/* position inside dest */

	dest[0] = '\0';
	for (i=0; i < len-1; i++) {
	    c = term->waitforinput(0);
	    if ('\n' == c) {
		dest[i] = '\n';
		i++;
		break;
	    } else if (EOF == c) {
		dest[i] = '\0';
		return (char*) 0;
	    } else {
		dest[i] = c;
	    }
	}
	dest[i] = '\0';
	return dest;
    } else
#endif
	return fgets(dest, len, stdin);
}

/* get a line from stdin, and display a prompt if interactive */
static char*
gp_get_string(char * buffer, size_t len, const char * prompt)
{
# ifdef USE_READLINE
    if (interactive)
	return rlgets(buffer, len, prompt);
    else
	return fgets_ipc(buffer, len);
# else
    if (interactive)
	PUT_STRING(prompt);

    return GET_STRING(buffer, len);
# endif
}

/* Non-VMS version */
static int
read_line(const char *prompt, int start)
{
    TBOOLEAN more = FALSE;
    int last = 0;

    current_prompt = prompt;

    /* Once we start to read a new line, the tokens pointing into the old */
    /* line are no longer valid.  We used to _not_ clear things here, but */
    /* that lead to errors when a mouse-triggered replot request came in  */
    /* while a new line was being read.   Bug 3602388 Feb 2013.           */
    if (start == 0) {
	c_token = num_tokens = 0;
	gp_input_line[0] = '\0';
    }

    do {
	/* grab some input */
	if (gp_get_string(gp_input_line + start, gp_input_line_len - start,
		         ((more) ? ">" : prompt))
	    == (char *) NULL)
	{
	    /* end-of-file */
	    if (interactive)
		(void) putc('\n', stderr);
	    gp_input_line[start] = NUL;
	    inline_num++;
	    if (start > 0 && curly_brace_count == 0)	/* don't quit yet - process what we have */
		more = FALSE;
	    else
		return (1);	/* exit gnuplot */
	} else {
	    /* normal line input */
	    /* gp_input_line must be NUL-terminated for strlen not to pass the
	     * the bounds of this array */
	    last = strlen(gp_input_line) - 1;
	    if (last >= 0) {
		if (gp_input_line[last] == '\n') {	/* remove any newline */
		    gp_input_line[last] = NUL;
		    if (last > 0 && gp_input_line[last-1] == '\r')
			gp_input_line[--last] = NUL;
		    /* Watch out that we don't backup beyond 0 (1-1-1) */
		    if (last > 0)
			--last;
		} else if (last + 2 >= gp_input_line_len) {
		    extend_input_line();
		    /* read rest of line, don't print "> " */
		    start = last + 1;
		    more = TRUE;
		    continue;
		    /* else fall through to continuation handling */
		} /* if(grow buffer?) */
		if (gp_input_line[last] == '\\') {
		    /* line continuation */
		    start = last;
		    more = TRUE;
		} else
		    more = FALSE;
	    } else
		more = FALSE;
	}
    } while (more);
    return (0);
}

#endif /* !VMS */


/*
 * Walk through the input line looking for string variables preceded by @.
 * Replace the characters @<varname> with the contents of the string.
 * Anything inside quotes is not expanded.
 * Allow up to 3 levels of nested macros.
 */
void
string_expand_macros()
{
	if (expand_1level_macros() && expand_1level_macros()
	&&  expand_1level_macros() && expand_1level_macros())
	    int_error(NO_CARET, "Macros nested too deeply");
}

#define COPY_CHAR do {gp_input_line[o++] = *c; \
		  after_backslash = FALSE; } while (0)

int
expand_1level_macros()
{
    TBOOLEAN in_squote = FALSE;
    TBOOLEAN in_dquote = FALSE;
    TBOOLEAN after_backslash = FALSE;
    TBOOLEAN in_comment= FALSE;
    int   len;
    int   o = 0;
    int   nfound = 0;
    char *c;
    char *temp_string;
    char  temp_char;
    char *m;
    struct udvt_entry *udv;

    /* Most lines have no macros */
    if (!strchr(gp_input_line,'@'))
	return(0);

    temp_string = gp_alloc(gp_input_line_len,"string variable");
    len = strlen(gp_input_line);
    if (len >= gp_input_line_len) len = gp_input_line_len-1;
    strncpy(temp_string,gp_input_line,len);
    temp_string[len] = '\0';

    for (c=temp_string; len && c && *c; c++, len--) {
	switch (*c) {
	case '@':	/* The only tricky bit */
		if (!in_squote && !in_dquote && !in_comment && isalpha((unsigned char)c[1])) {
		    /* Isolate the udv key as a null-terminated substring */
		    m = ++c;
		    while (isalnum((unsigned char )*c) || (*c=='_')) c++;
		    temp_char = *c; *c = '\0';
		    /* Look up the key and restore the original following char */
		    udv = get_udv_by_name(m);
		    if (udv && udv->udv_value.type == STRING) {
			nfound++;
			m = udv->udv_value.v.string_val;
			FPRINTF((stderr,"Replacing @%s with \"%s\"\n",udv->udv_name,m));
			while (strlen(m) + o + len > gp_input_line_len)
			    extend_input_line();
			while (*m)
			    gp_input_line[o++] = (*m++);
		    } else {
			gp_input_line[o] = '\0';
			int_warn( NO_CARET, "%s is not a string variable",m);
		    }
		    *c-- = temp_char;
		} else
		    COPY_CHAR;
		break;

	case '"':
		if (!after_backslash)
		    in_dquote = !in_dquote;
		COPY_CHAR; break;
	case '\'':
		in_squote = !in_squote;
		COPY_CHAR; break;
	case '\\':
		if (in_dquote)
		    after_backslash = !after_backslash;
		gp_input_line[o++] = *c; break;
	case '#':
		if (!in_squote && !in_dquote)
		    in_comment = TRUE;
	default :
		COPY_CHAR; break;
	}
    }
    gp_input_line[o] = '\0';
    free(temp_string);

    if (nfound)
	FPRINTF((stderr,
		 "After string substitution command line is:\n\t%s\n",
		 gp_input_line));

    return(nfound);
}

/* much more than what can be useful */
#define MAX_TOTAL_LINE_LEN (1024 * MAX_LINE_LEN)

int
do_system_func(const char *cmd, char **output)
{

#if defined(VMS) || defined(PIPES)
    int c;
    FILE *f;
    int result_allocated, result_pos;
    char* result;
    int ierr = 0;
# if defined(VMS)
    int chan, one = 1;
    struct dsc$descriptor_s pgmdsc = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0};
    static $DESCRIPTOR(lognamedsc, "PLOT$MAILBOX");
# endif /* VMS */

    /* open stream */
# ifdef VMS
    pgmdsc.dsc$a_pointer = cmd;
    pgmdsc.dsc$w_length = strlen(cmd);
    if (!((vaxc$errno = sys$crembx(0, &chan, 0, 0, 0, 0, &lognamedsc)) & 1))
	os_error(NO_CARET, "sys$crembx failed");

    if (!((vaxc$errno = lib$spawn(&pgmdsc, 0, &lognamedsc, &one)) & 1))
	os_error(NO_CARET, "lib$spawn failed");

    if ((f = fopen("PLOT$MAILBOX", "r")) == NULL)
	os_error(NO_CARET, "mailbox open failed");
# else	/* everyone else */
    restrict_popen();
    if ((f = popen(cmd, "r")) == NULL)
	os_error(NO_CARET, "popen failed");
# endif	/* everyone else */

    /* get output */
    result_pos = 0;
    result_allocated = MAX_LINE_LEN;
    result = gp_alloc(MAX_LINE_LEN, "do_system_func");
    result[0] = NUL;
    while (1) {
	if ((c = getc(f)) == EOF)
	    break;
	/* result <- c */
	result[result_pos++] = c;
	if ( result_pos == result_allocated ) {
	    if ( result_pos >= MAX_TOTAL_LINE_LEN ) {
		result_pos--;
		int_warn(NO_CARET,
			 "*very* long system call output has been truncated");
		break;
	    } else {
		result = gp_realloc(result, result_allocated + MAX_LINE_LEN,
				    "extend in do_system_func");
		result_allocated += MAX_LINE_LEN;
	    }
	}
    }
    result[result_pos] = NUL;

    /* close stream */
    ierr = pclose(f);

    ierr = report_error(ierr);

    result = gp_realloc(result, strlen(result)+1, "do_system_func");
    *output = result;

    return ierr;

#else /* VMS || PIPES */

    int_warn(NO_CARET, "system() requires support for pipes");
    *output = gp_strdup("");
    return 0;

#endif /* VMS || PIPES */

}

static int
report_error(int ierr)
{
    int reported_error;

    /* FIXME:  This does not seem to report all reasonable errors correctly */
    if (ierr == -1 && errno != 0)
	reported_error = errno;
    else
	reported_error = WEXITSTATUS(ierr);

    fill_gpval_integer("GPVAL_SYSTEM_ERRNO", reported_error);
    if (reported_error == 127)
	fill_gpval_string("GPVAL_SYSTEM_ERRMSG", "command not found or shell failed");
    else
	fill_gpval_string("GPVAL_SYSTEM_ERRMSG", strerror(reported_error));

    return reported_error;
}
