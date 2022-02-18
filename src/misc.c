/* GNUPLOT - misc.c */

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

#include "misc.h"

#include "alloc.h"
#include "command.h"
#include "datablock.h"
#include "encoding.h"
#include "graphics.h"
#include "plot.h"
#include "tables.h"
#include "util.h"
#include "variable.h"
#include "axis.h"
#include "scanner.h"		/* so that scanner() can count curly braces */
#include "setshow.h"
#ifdef _WIN32
# include <fcntl.h>
# if defined(__WATCOMC__) || defined(_MSC_VER)
#  include <io.h>        /* for setmode() */
# endif
#endif

static void prepare_call(int calltype);

/* State information for load_file(), to recover from errors
 * and properly handle recursive load_file calls
 */
LFS *lf_head = NULL;		/* NULL if not in load_file */

/* these are global so that plot.c can load them for the -c option */
int call_argc;
char *call_args[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static char *argname[] = {"ARG0","ARG1","ARG2","ARG3","ARG4","ARG5","ARG6","ARG7","ARG8","ARG9"};

/* Used to report failure on file open */
static char *failed_file_name = NULL;

/* Used by postscript terminal if a font file is found by loadpath_fopen() */
char *loadpath_fontname = NULL;

static void
prepare_call(int calltype)
{
    struct udvt_entry *udv;
    struct value *ARGV;
    int argindex;
    int argv_size;

    /* call_args[] will hold arguments as strings.
     * argval[] will be a private copy of numeric arguments as udvs.
     * Later we will fill ARGV[] from one or the other.
     */
    struct value argval[9];
    for (argindex = 0; argindex < 9; argindex++)
	argval[argindex].type = NOTDEFINED;

    if (calltype == 2) {
	call_argc = 0;
	while (!END_OF_COMMAND && call_argc < 9) {
	    call_args[call_argc] = try_to_get_string();
	    if (!call_args[call_argc]) {
		int save_token = c_token;

		/* This catches call "file" STRINGVAR (expression) */
		if (type_udv(c_token) == STRING) {
		    call_args[call_argc] = gp_strdup(add_udv(c_token)->udv_value.v.string_val);
		    c_token++;

		/* Evaluate a parenthesized expression or a bare numeric user variable
		 * and store the result in a string
		 */
		} else if (equals(c_token, "(")
			|| (type_udv(c_token) == INTGR || type_udv(c_token) == CMPLX)) {
		    char val_as_string[32];
		    struct value a;
		    const_express(&a);
		    argval[call_argc] = a;
		    switch(a.type) {
			case CMPLX: /* FIXME: More precision? Some way to provide a format? */
				sprintf(val_as_string, "%g", a.v.cmplx_val.real);
				call_args[call_argc] = gp_strdup(val_as_string);
				break;
			default:
				int_error(save_token, "Unrecognized argument type");
				break;
			case INTGR:
				sprintf(val_as_string, PLD, a.v.int_val);
				call_args[call_argc] = gp_strdup(val_as_string);
				break;
		    }

		/* Old (pre version 5) style wrapping of bare tokens as strings
		 * is still used for storing numerical constants ARGn but not ARGV[n]
		 */
		} else {
		    double temp;
		    char *endptr;
		    m_capture(&call_args[call_argc], c_token, c_token);
		    c_token++;
		    temp = strtod(call_args[call_argc], &endptr);
		    if (endptr != call_args[call_argc] && *endptr == '\0')
			Gcomplex(&argval[call_argc], temp, 0.0);
		}
	    }
	    call_argc++;
	}
	lf_head->c_token = c_token;
	if (!END_OF_COMMAND)
	    int_error(++c_token, "too many arguments for 'call <file>'");

    } else if (calltype == 5) {
	/* lf_push() moved our call arguments from call_args[] to lf->call_args[] */
	/* call_argc was determined at program entry */
	for (argindex = 0; argindex < 10; argindex++) {
	    call_args[argindex] = lf_head->call_args[argindex];
	    lf_head->call_args[argindex] = NULL;	/* just to be safe */
	}

    } else {
	/* "load" command has no arguments */
	call_argc = 0;
    }

    /* Old-style "call" arguments were referenced as $0 ... $9 and $#.
     * New-style has ARG0 = script-name, ARG1 ... ARG9 and ARGC
     * Version 5.3 adds ARGV[n]
     */
    udv = add_udv_by_name("ARGC");
    Ginteger(&(udv->udv_value), call_argc);

    udv = add_udv_by_name("ARG0");
    gpfree_string(&(udv->udv_value));
    Gstring(&(udv->udv_value), gp_strdup(lf_head->name));

    udv = add_udv_by_name("ARGV");
    free_value(&(udv->udv_value));

    argv_size = GPMIN(call_argc, 9);
    udv->udv_value.type = ARRAY;
    ARGV = udv->udv_value.v.value_array = gp_alloc((argv_size + 1) * sizeof(t_value), "array state");
    ARGV[0].v.int_val = argv_size;
    ARGV[0].type = NOTDEFINED;

    for (argindex = 1; argindex <= 9; argindex++) {
	char *argstring = call_args[argindex-1];

	udv = add_udv_by_name(argname[argindex]);
	gpfree_string(&(udv->udv_value));
	Gstring(&(udv->udv_value), argstring ? gp_strdup(argstring) : gp_strdup(""));

	if (argindex > argv_size)
	    continue;
	if (argval[argindex-1].type == NOTDEFINED)
	    Gstring(&ARGV[argindex], gp_strdup(udv->udv_value.v.string_val));
	else
	    ARGV[argindex] = argval[argindex-1];
    }
}

/*
 * calltype indicates whether load_file() is called from
 * (1) the "load" command, no arguments substitution is done
 * (2) the "call" command, arguments are substituted for $0, $1, etc.
 * (3) on program entry to load initialization files (acts like "load")
 * (4) to execute script files given on the command line (acts like "load")
 * (5) to execute a single script file given with -c (acts like "call")
 * (6) "load $datablock" (EXPERIMENTAL)
 */
void
load_file(FILE *fp, char *name, int calltype)
{
    int len;

    int start, left;
    int more;
    int stop = FALSE;
    udvt_entry *gpval_lineno = NULL;
    char **datablock_input_line = NULL;

    /* Support for "load $datablock" */
    if (calltype == 6)
	datablock_input_line = get_datablock(name);

    if (!fp && !datablock_input_line) {
	failed_file_name = name;
	int_error(NO_CARET, "Cannot load input from '%s'", failed_file_name);
    }

    /* Provide a user-visible copy of the current line number in the input file */
    gpval_lineno = add_udv_by_name("GPVAL_LINENO");
    Ginteger(&gpval_lineno->udv_value, 0);

    lf_push(fp, name, NULL); /* save state for errors and recursion */

    if (fp == stdin) {
	/* DBT 10-6-98  go interactive if "-" named as load file */
	interactive = TRUE;
	while (!com_line());
	(void) lf_pop();
	return;
    }

    /* We actually will read from a file */
    prepare_call(calltype);

    /* things to do after lf_push */
    inline_num = 0;
    /* go into non-interactive mode during load */
    /* will be undone below, or in load_file_error */
    interactive = FALSE;

    while (!stop) {	/* read all lines in file */
	left = gp_input_line_len;
	start = 0;
	more = TRUE;

	/* read one logical line */
	while (more) {
	    if (fp && fgets(&(gp_input_line[start]), left, fp) == (char *) NULL) {
		/* EOF in input file */
		stop = TRUE;
		gp_input_line[start] = '\0';
		more = FALSE;
	    } else if (!fp && datablock_input_line && (*datablock_input_line == NULL)) {
		/* End of input datablock */
		stop = TRUE;
		gp_input_line[start] = '\0';
		more = FALSE;
	    } else {
		/* Either we successfully read a line from input file fp
		 * or we are about to copy a line from a datablock.
		 * Either way we have to process line-ending '\' as a 
		 * continuation request.
		 */
		if (!fp && datablock_input_line) {
		    strncpy(&(gp_input_line[start]), *datablock_input_line, left);
		    datablock_input_line++;
		}
		inline_num++;
		gpval_lineno->udv_value.v.int_val = inline_num;	/* User visible copy */
		if ((len = strlen(gp_input_line)) == 0)
		    continue;
		--len;
		if (gp_input_line[len] == '\n') {	/* remove any newline */
		    gp_input_line[len] = '\0';
		    /* Look, len was 1-1 = 0 before, take care here! */
		    if (len > 0)
			--len;
		    if (gp_input_line[len] == '\r') {	/* remove any carriage return */
			gp_input_line[len] = NUL;
			if (len > 0)
			    --len;
		    }
		} else if (len + 2 >= left) {
		    extend_input_line();
		    left = gp_input_line_len - len - 1;
		    start = len + 1;
		    continue;	/* don't check for '\' */
		}
		if (gp_input_line[len] == '\\') {
		    /* line continuation */
		    start = len;
		    left = gp_input_line_len - start;
		} else {
		    /* EAM May 2011 - handle multi-line bracketed clauses {...}.
		     * Introduces a requirement for scanner.c and scanner.h
		     * This code is redundant with part of do_line(),
		     * but do_line() assumes continuation lines come from stdin.
		     */

		    /* macros in a clause are problematic, as they are */
		    /* only expanded once even if the clause is replayed */
		    string_expand_macros();

		    /* Strip off trailing comment and count curly braces */
		    num_tokens = scanner(&gp_input_line, &gp_input_line_len);
		    if (gp_input_line[token[num_tokens].start_index] == '#') {
			gp_input_line[token[num_tokens].start_index] = NUL;
			start = token[num_tokens].start_index;
			left = gp_input_line_len - start;
		    }
		    /* Read additional lines if necessary to complete a
		     * bracketed clause {...}
		     */
		    if (curly_brace_count < 0)
			int_error(NO_CARET, "Unexpected }");
		    if (curly_brace_count > 0) {
			if ((len + 4) > gp_input_line_len)
			    extend_input_line();
			strcat(gp_input_line,";\n");
			start = strlen(gp_input_line);
			left = gp_input_line_len - start;
			continue;
		    }

		    more = FALSE;
		}

	    }
	}

	/* If we hit a 'break' or 'continue' statement in the lines just processed */
	if (iteration_early_exit())
	    continue;

	/* process line */
	if (strlen(gp_input_line) > 0) {
	    screen_ok = FALSE;	/* make sure command line is echoed on error */
	    if (do_line())
		stop = TRUE;
	}
    }

    /* pop state */
    (void) lf_pop();		/* also closes file fp */
}

/* pop from load_file state stack
   FALSE if stack was empty
   called by load_file and load_file_error */
TBOOLEAN
lf_pop()
{
    LFS *lf;
    int argindex;
    struct udvt_entry *udv;

    if (lf_head == NULL)
	return (FALSE);

    lf = lf_head;
    if (lf->fp == NULL || lf->fp == stdin)
	/* Do not close stdin in the case that "-" is named as a load file */
	;
#if defined(PIPES)
    else if (lf->name != NULL && lf->name[0] == '<')
	pclose(lf->fp);
#endif
    else
	fclose(lf->fp);

    /* call arguments are not relevant when invoked from do_string_and_free */
    if (lf->cmdline == NULL) {
	for (argindex = 0; argindex < 10; argindex++) {
	    if (call_args[argindex])
		free(call_args[argindex]);
	    call_args[argindex] = lf->call_args[argindex];
	}
	call_argc = lf->call_argc;

	/* Restore ARGC and ARG0 ... ARG9 */
	if ((udv = get_udv_by_name("ARGC"))) {
	    Ginteger(&(udv->udv_value), call_argc);
	}

	if ((udv = get_udv_by_name("ARG0"))) {
	    gpfree_string(&(udv->udv_value));
	    Gstring(&(udv->udv_value),
		(lf->prev && lf->prev->name) ? gp_strdup(lf->prev->name) : gp_strdup(""));
	}

	for (argindex = 1; argindex <= 9; argindex++) {
	    if ((udv = get_udv_by_name(argname[argindex]))) {
		gpfree_string(&(udv->udv_value));
		if (!call_args[argindex-1])
		    udv->udv_value.type = NOTDEFINED;
		else
		    Gstring(&(udv->udv_value), gp_strdup(call_args[argindex-1]));
	    }
	}

	if ((udv = get_udv_by_name("ARGV")) && udv->udv_value.type == ARRAY) {
	    struct value *ARGV;
	    int argv_size = lf->argv[0].v.int_val;

	    gpfree_array(&(udv->udv_value));
	    udv->udv_value.type = ARRAY;
	    ARGV = udv->udv_value.v.value_array = gp_alloc((argv_size + 1) * sizeof(t_value), "array state");
	    for (argindex = 0; argindex <= argv_size; argindex++)
		ARGV[argindex] = lf->argv[argindex];
	}

    }

    interactive = lf->interactive;
    inline_num = lf->inline_num;
    add_udv_by_name("GPVAL_LINENO")->udv_value.v.int_val = inline_num;
    if_depth = lf->if_depth;
    if_condition = lf->if_condition;
    if_open_for_else = lf->if_open_for_else;

    /* Restore saved input state and free the copy */
    if (lf->tokens) {
	num_tokens = lf->num_tokens;
	c_token = lf->c_token;
	assert(token_table_size >= lf->num_tokens+1);
	memcpy(token, lf->tokens,
	       (lf->num_tokens+1) * sizeof(struct lexical_unit));
	free(lf->tokens);
    }
    if (lf->input_line) {
	strcpy(gp_input_line, lf->input_line);
	free(lf->input_line);
    }
    free(lf->name);
    free(lf->cmdline);

    lf_head = lf->prev;
    free(lf);
    return (TRUE);
}

/* lf_push is called from two different contexts:
 *    load_file passes fp and file name (3rd param NULL)
 *    do_string_and_free passes cmdline (1st and 2nd params NULL)
 * In either case the routines lf_push/lf_pop save and restore state
 * information that may be changed by executing commands from a file
 * or from the passed command line.
 */
void
lf_push(FILE *fp, char *name, char *cmdline)
{
    LFS *lf;
    int argindex;

    lf = (LFS *) gp_alloc(sizeof(LFS), (char *) NULL);
    if (lf == (LFS *) NULL) {
	if (fp != (FILE *) NULL)
	    (void) fclose(fp);	/* it won't be otherwise */
	int_error(c_token, "not enough memory to load file");
    }
    lf->fp = fp;		/* save this file pointer */
    lf->name = name;
    lf->cmdline = cmdline;

    lf->interactive = interactive;	/* save current state */
    lf->inline_num = inline_num;	/* save current line number */
    lf->call_argc = call_argc;

    /* Call arguments are irrelevant if invoked from do_string_and_free */
    if (cmdline == NULL) {
	struct udvt_entry *udv;
	/* Save ARG0 through ARG9 */
	for (argindex = 0; argindex < 10; argindex++) {
	    lf->call_args[argindex] = call_args[argindex];
	    call_args[argindex] = NULL;	/* initially no args */
	}
	/* Save ARGV[] */
	lf->argv[0].v.int_val = 0;
	lf->argv[0].type = NOTDEFINED;
	if ((udv = get_udv_by_name("ARGV")) && udv->udv_value.type == ARRAY) {
	    /* When called from the command line (-c option) call_argc correctly
	     * enumerates the entities on the command line, but they were not
	     * previously saved in ARGV.
	     */
	    int saved_args = udv->udv_value.v.value_array[0].v.int_val;
	    for (argindex = 0; argindex <= call_argc; argindex++) {
		if (argindex > saved_args)
		    break;
		lf->argv[argindex] = udv->udv_value.v.value_array[argindex];
		if (lf->argv[argindex].type == STRING)
		    lf->argv[argindex].v.string_val =
			gp_strdup(lf->argv[argindex].v.string_val);
	    }
	}
    }
    lf->depth = lf_head ? lf_head->depth+1 : 0;	/* recursion depth */
    if (lf->depth > STACK_DEPTH)
	int_error(NO_CARET, "load/eval nested too deeply");
    lf->if_depth = if_depth;
    lf->if_open_for_else = if_open_for_else;
    lf->if_condition = if_condition;
    lf->c_token = c_token;
    lf->num_tokens = num_tokens;
    lf->tokens = gp_alloc((num_tokens+1) * sizeof(struct lexical_unit),
			  "lf tokens");
    memcpy(lf->tokens, token, (num_tokens+1) * sizeof(struct lexical_unit));
    lf->input_line = gp_strdup(gp_input_line);

    lf->prev = lf_head;		/* link to stack */
    lf_head = lf;
}

/* used for reread  vsnyder@math.jpl.nasa.gov */
FILE *
lf_top()
{
    if (lf_head == (LFS *) NULL)
	return ((FILE *) NULL);
    return (lf_head->fp);
}

/* called from main */
void
load_file_error()
{
    /* clean up from error in load_file */
    /* pop off everything on stack */
    free(failed_file_name);
    failed_file_name = NULL;
    while (lf_pop());
}

FILE *
loadpath_fopen(const char *filename, const char *mode)
{
    FILE *fp;

    /* The global copy of fullname is only for the benefit of post.trm's
     * automatic fontfile conversion via a constructed shell command.
     * FIXME: There was a Feature Request to export the directory path
     * in which a loaded file was found to a user-visible variable for the
     * lifetime of that load.  This is close but without the lifetime.
     */
    free(loadpath_fontname);
    loadpath_fontname = NULL;

#if defined(PIPES)
    if (*filename == '<') {
	restrict_popen();
	if ((fp = popen(filename + 1, "r")) == (FILE *) NULL)
	    return (FILE *) 0;
    } else
#endif /* PIPES */
    if ((fp = fopen(filename, mode)) == (FILE *) NULL) {
	/* try 'loadpath' variable */
	char *fullname = NULL, *path;

	while ((path = get_loadpath()) != NULL) {
	    /* length of path, dir separator, filename, \0 */
	    fullname = gp_realloc(fullname, strlen(path) + 1 + strlen(filename) + 1, "loadpath_fopen");
	    strcpy(fullname, path);
	    PATH_CONCAT(fullname, filename);
	    if ((fp = fopen(fullname, mode)) != NULL) {
		/* free(fullname); */
		loadpath_fontname = fullname;
		fullname = NULL;
		/* reset loadpath internals!
		 * maybe this can be replaced by calling get_loadpath with
		 * a NULL argument and some loadpath_handler internal logic */
		while (get_loadpath());
		break;
	    }
	}

	free(fullname);
    }

#ifdef _WIN32
    if (fp != NULL)
	_setmode(_fileno(fp), _O_BINARY);
#endif
    return fp;
}

/* Push current terminal.
 * Called 1. in main(), just after init_terminal(),
 *        2. from load_rcfile(),
 *        3. anytime by user command "set term push".
 */
static char *push_term_name = NULL;
static char *push_term_opts = NULL;

void
push_terminal(int is_interactive)
{
    if (term) {
	free(push_term_name);
	free(push_term_opts);
	push_term_name = gp_strdup(term->name);
	push_term_opts = gp_strdup(term_options);
	if (is_interactive)
	    fprintf(stderr, "   pushed terminal %s %s\n", push_term_name, push_term_opts);
    } else {
	if (is_interactive)
	    fputs("\tcurrent terminal type is unknown\n", stderr);
    }
}

/* Pop the terminal.
 * Called anytime by user command "set term pop".
 */
void
pop_terminal()
{
    if (push_term_name != NULL) {
	char *s;
	int i = strlen(push_term_name) + 11;
	if (push_term_opts) {
	    /* do_string() does not like backslashes -- thus remove them */
	    for (s=push_term_opts; *s; s++)
		if (*s=='\\' || *s=='\n') *s=' ';
	    i += strlen(push_term_opts);
	}
	s = gp_alloc(i, "pop");
	i = interactive;
	interactive = 0;
	sprintf(s,"set term %s %s", push_term_name, (push_term_opts ? push_term_opts : ""));
	do_string_and_free(s);
	interactive = i;
	if (interactive)
	    fprintf(stderr,"   restored terminal is %s %s\n", term->name, ((*term_options) ? term_options : ""));
    } else
	fprintf(stderr,"No terminal has been pushed yet\n");
}


/* Parse a plot style. Used by 'set style {data|function}' and by (s)plot.  */
enum PLOT_STYLE
get_style()
{
    /* defined in plot.h */
    enum PLOT_STYLE ps;

    c_token++;

    ps = lookup_table(&plotstyle_tbl[0], c_token);

    c_token++;

    if (ps == PLOT_STYLE_NONE)
	int_error(c_token, "unrecognized plot type");

    return ps;
}

/* Parse options for style filledcurves and fill fco accordingly.
 * If no option given, set it to FILLEDCURVES_DEFAULT.
 */
void
get_filledcurves_style_options(filledcurves_opts *fco)
{
    enum filledcurves_opts_id p;
    fco->closeto = FILLEDCURVES_DEFAULT;
    fco->oneside = 0;

    while ((p = lookup_table(&filledcurves_opts_tbl[0], c_token)) != -1) {
	fco->closeto = p;
	c_token++;

	if (p == FILLEDCURVES_ABOVE) {
	    fco->oneside = 1;
	    continue;
	} else if (p == FILLEDCURVES_BELOW) {
	    fco->oneside = -1;
	    continue;
	}

	fco->at = 0;
	if (!equals(c_token, "="))
	    return;
	/* parameter required for filledcurves x1=... and friends */
	if (p < FILLEDCURVES_ATXY)
	    fco->closeto += 4;
	c_token++;
	fco->at = real_expression();
	if (p != FILLEDCURVES_ATXY)
	    return;
	/* two values required for FILLEDCURVES_ATXY */
	if (!equals(c_token, ","))
	    int_error(c_token, "syntax is xy=<x>,<y>");
	c_token++;
	fco->aty = real_expression();
    }
    return;
}

/* Print filledcurves style options to a file (used by 'show' and 'save'
 * commands).
 */
void
filledcurves_options_tofile(filledcurves_opts *fco, FILE *fp)
{
    if (fco->closeto == FILLEDCURVES_DEFAULT)
	return;
    if (fco->oneside)
	fputs(fco->oneside > 0 ? "above " : "below ", fp);
    if (fco->closeto == FILLEDCURVES_CLOSED) {
	fputs("closed", fp);
	return;
    }
    if (fco->closeto <= FILLEDCURVES_Y2) {
	fputs(filledcurves_opts_tbl[fco->closeto].key, fp);
	return;
    }
    if (fco->closeto <= FILLEDCURVES_ATY2) {
	fprintf(fp, "%s=%g", filledcurves_opts_tbl[fco->closeto - 4].key, fco->at);
	return;
    }
    if (fco->closeto == FILLEDCURVES_ATXY) {
	fprintf(fp, "xy=%g,%g", fco->at, fco->aty);
	return;
    }
}

TBOOLEAN
need_fill_border(struct fill_style_type *fillstyle)
{
    struct lp_style_type p;
    p.pm3d_color = fillstyle->border_color;

    if (p.pm3d_color.type == TC_LT) {
	/* Doesn't want a border at all */
	if (p.pm3d_color.lt == LT_NODRAW)
	    return FALSE;
	load_linetype(&p, p.pm3d_color.lt+1);
    }

    /* Wants a border in a new color */
    if (p.pm3d_color.type != TC_DEFAULT)
	apply_pm3dcolor(&p.pm3d_color);

    return TRUE;
}

int
parse_dashtype(struct t_dashtype *dt)
{
    int res = DASHTYPE_SOLID;
    int j = 0;
    int k = 0;
    char *dash_str = NULL;

    /* Erase any previous contents */
    memset(dt, 0, sizeof(struct t_dashtype));

    /* Fill in structure based on keyword ... */
    if (equals(c_token, "solid")) {
	res = DASHTYPE_SOLID;
	c_token++;

    /* Or numerical pattern consisting of pairs solid,empty,solid,empty... */
    } else if (equals(c_token, "(")) {
	c_token++;
	while (!END_OF_COMMAND) {
	    if (j >= DASHPATTERN_LENGTH) {
		int_error(c_token, "too many pattern elements");
	    }
	    dt->pattern[j++] = real_expression();	/* The solid portion */
	    if (!equals(c_token++, ","))
		int_error(c_token, "expecting comma");
	    dt->pattern[j++] = real_expression();	/* The empty portion */
	    if (equals(c_token, ")"))
		break;
	    if (!equals(c_token++, ","))
		int_error(c_token, "expecting comma");
	}

	if (!equals(c_token, ")"))
	    int_error(c_token, "expecting , or )");

	c_token++;
	res = DASHTYPE_CUSTOM;

    /* Or string representing pattern elements ... */
    } else if ((dash_str = try_to_get_string())) {
	int leading_space = 0;
#define DSCALE 10.
	while (dash_str[j] && (k < DASHPATTERN_LENGTH || dash_str[j] == ' ')) {
	    /* .      Dot with short space
	     * -      Dash with regular space
	     * _      Long dash with regular space
	     * space  Don't add new dash, just increase last space */
	    switch (dash_str[j]) {
	    case '.':
		dt->pattern[k++] = 0.2 * DSCALE;
		dt->pattern[k++] = 0.5 * DSCALE;
		break;
	    case '-':
		dt->pattern[k++] = 1.0 * DSCALE;
		dt->pattern[k++] = 1.0 * DSCALE;
		break;
	    case '_':
		dt->pattern[k++] = 2.0 * DSCALE;
		dt->pattern[k++] = 1.0 * DSCALE;
		break;
	    case ' ':
		if (k == 0)
		    leading_space++;
		else
		    dt->pattern[k-1] += 1.0 * DSCALE;
		break;
	    default:
		int_error(c_token - 1, "expecting one of . - _ or space");
	    }
	    j++;
	}
	/* Move leading space, if any, to the end */
	if (k > 0)
	    dt->pattern[k-1] += leading_space * DSCALE;
#undef  DSCALE

	/* truncate dash_str if we ran out of space in the array representation */
	dash_str[j] = '\0';
	safe_strncpy(dt->dstring, dash_str, sizeof(dt->dstring));
	free(dash_str);
	if (k == 0)
	    res = DASHTYPE_SOLID;
	else
	    res = DASHTYPE_CUSTOM;

    /* Or index of previously defined dashtype */
    } else {
	res = int_expression();
	if (res < 0)
	    int_error(c_token - 1, "dashtype must be non-negative");
	if (res == 0)
	    res = DASHTYPE_AXIS;
	else
	    res = res - 1;
    }

    return res;
}

/*
 * destination_class tells us whether we are filling in a line style ('set style line'),
 * a persistent linetype ('set linetype') or an ad hoc set of properties for a single
 * use ('plot ... lc foo lw baz').
 * allow_point controls whether we accept a point attribute in this lp_style.
 */
int
lp_parse(struct lp_style_type *lp, lp_class destination_class, TBOOLEAN allow_point)
{
    /* keep track of which options were set during this call */
    int set_lt = 0, set_pal = 0, set_lw = 0;
    int set_pt = 0, set_ps  = 0, set_pi = 0;
    int set_pn = 0;
    int set_dt = 0;
    int new_lt = 0;

    /* EAM Mar 2010 - We don't want properties from a user-defined default
     * linetype to override properties explicitly set here.  So fill in a
     * local lp_style_type as we go and then copy over the specifically
     * requested properties on top of the default ones.
     */
    struct lp_style_type newlp = *lp;

    if ((destination_class == LP_ADHOC)
    && (almost_equals(c_token, "lines$tyle") || equals(c_token, "ls"))) {
	c_token++;
	lp_use_properties(lp, int_expression());
    }

    while (!END_OF_COMMAND) {

	/* This special case is to flag an attempt to "set object N lt <lt>",
	 * which would otherwise be accepted but ignored, leading to confusion
	 * FIXME:  Couldn't this be handled at a higher level?
	 */
	if ((destination_class == LP_NOFILL)
	&&  (equals(c_token,"lt") || almost_equals(c_token,"linet$ype"))) {
	    int_error(c_token, "object linecolor must be set using fillstyle border");
	}

	if (almost_equals(c_token, "linet$ype") || equals(c_token, "lt")) {
	    if (set_lt++)
		break;
	    if (destination_class == LP_TYPE)
		int_error(c_token, "linetype definition cannot use linetype");
	    c_token++;
	    if (almost_equals(c_token, "rgb$color")) {
		if (set_pal++)
		    break;
		c_token--;
		parse_colorspec(&(newlp.pm3d_color), TC_RGB);
	    } else
		/* both syntaxes allowed: 'with lt pal' as well as 'with pal' */
		if (almost_equals(c_token, "pal$ette")) {
		    if (set_pal++)
			break;
		    c_token--;
		    parse_colorspec(&(newlp.pm3d_color), TC_Z);
		} else if (equals(c_token,"bgnd")) {
		    *lp = background_lp;
		    c_token++;
		} else if (equals(c_token,"black")) {
		    *lp = default_border_lp;
		    c_token++;
		} else if (equals(c_token,"nodraw")) {
		    lp->l_type = LT_NODRAW;
		    c_token++;
		} else {
		    /* These replace the base style */
		    new_lt = int_expression();
		    lp->l_type = new_lt - 1;
		    /* user may prefer explicit line styles */
		    if (prefer_line_styles && (destination_class != LP_STYLE))
			lp_use_properties(lp, new_lt);
		    else
			load_linetype(lp, new_lt);
		}
	} /* linetype, lt */

	/* both syntaxes allowed: 'with lt pal' as well as 'with pal' */
	if (almost_equals(c_token, "pal$ette")) {
	    if (set_pal++)
		break;
	    c_token--;
	    parse_colorspec(&(newlp.pm3d_color), TC_Z);
	    continue;
	}

	/* This is so that "set obj ... lw N fc <colorspec>" doesn't eat up the
	 * fc colorspec as a line property.  We need to parse it later as a
	 * _fill_ property. Also prevents "plot ... fc <col1> fs <foo> lw <baz>"
	 * from generating an error claiming redundant line properties.
	 */
	if ((destination_class == LP_NOFILL || destination_class == LP_ADHOC)
	&&  (equals(c_token,"fc") || almost_equals(c_token,"fillc$olor"))
	&&  (!almost_equals(c_token+1, "pal$ette"))
	   )
	    break;

	if (equals(c_token,"lc") || almost_equals(c_token,"linec$olor")
	||  equals(c_token,"fc") || almost_equals(c_token,"fillc$olor")
	   ) {
	    if (set_pal++)
		break;
	    c_token++;
	    if (almost_equals(c_token, "rgb$color") || isstring(c_token)) {
		c_token--;
		parse_colorspec(&(newlp.pm3d_color), TC_RGB);
	    } else if (almost_equals(c_token, "pal$ette")) {
		c_token--;
		parse_colorspec(&(newlp.pm3d_color), TC_Z);
	    } else if (equals(c_token,"bgnd")) {
		newlp.pm3d_color.type = TC_LT;
		newlp.pm3d_color.lt = LT_BACKGROUND;
		c_token++;
	    } else if (equals(c_token,"black")) {
		newlp.pm3d_color.type = TC_LT;
		newlp.pm3d_color.lt = LT_BLACK;
		c_token++;
	    } else if (almost_equals(c_token, "var$iable")) {
		c_token++;
		newlp.l_type = LT_COLORFROMCOLUMN;
		newlp.pm3d_color.type = TC_LINESTYLE;
	    } else {
		/* Pull the line colour from a default linetype, but */
		/* only if we are not in the middle of defining one! */
		if (destination_class != LP_STYLE) {
		    struct lp_style_type temp;
		    load_linetype(&temp, int_expression());
		    newlp.pm3d_color = temp.pm3d_color;
		} else {
		    newlp.pm3d_color.type = TC_LT;
		    newlp.pm3d_color.lt = int_expression() - 1;
		}
	    }
	    continue;
	}

	if (almost_equals(c_token, "linew$idth") || equals(c_token, "lw")) {
	    if (set_lw++)
		break;
	    c_token++;
	    newlp.l_width = real_expression();
	    if (newlp.l_width < 0)
		newlp.l_width = 0;
	    continue;
	}

	if (equals(c_token,"bgnd")) {
	    if (set_lt++)
		break;;
	    c_token++;
	    *lp = background_lp;
	    continue;
	}

	if (equals(c_token,"black")) {
	    if (set_lt++)
		break;;
	    c_token++;
	    *lp = default_border_lp;
	    continue;
	}

	if (almost_equals(c_token, "pointt$ype") || equals(c_token, "pt")) {
	    if (allow_point) {
		char *symbol;
		if (set_pt++)
		    break;
		c_token++;
		if ((symbol = try_to_get_string())) {
		    newlp.p_type = PT_CHARACTER;
		    truncate_to_one_utf8_char(symbol);
		    safe_strncpy(newlp.p_char, symbol, sizeof(newlp.p_char));
		    free(symbol);
		} else if (almost_equals(c_token, "var$iable") && (destination_class == LP_ADHOC)) {
		    newlp.p_type = PT_VARIABLE;
		    c_token++;
		} else {
		    newlp.p_type = int_expression() - 1;
		}
	    } else {
		int_warn(c_token, "No pointtype specifier allowed, here");
		c_token += 2;
	    }
	    continue;
	}

	if (almost_equals(c_token, "points$ize") || equals(c_token, "ps")) {
	    if (allow_point) {
		if (set_ps++)
		    break;
		c_token++;
		if (almost_equals(c_token, "var$iable")) {
		    newlp.p_size = PTSZ_VARIABLE;
		    c_token++;
		} else if (almost_equals(c_token, "def$ault")) {
		    newlp.p_size = PTSZ_DEFAULT;
		    c_token++;
		} else {
		    newlp.p_size = real_expression();
		    if (newlp.p_size < 0)
			newlp.p_size = 0;
		}
	    } else {
		int_warn(c_token, "No pointsize specifier allowed, here");
		c_token += 2;
	    }
	    continue;
	}

	if (almost_equals(c_token, "pointi$nterval") || equals(c_token, "pi")) {
	    c_token++;
	    if (allow_point) {
		newlp.p_interval = int_expression();
		set_pi = 1;
	    } else {
		int_warn(c_token, "No pointinterval specifier allowed here");
		int_expression();
	    }
	    continue;
	}

	if (almost_equals(c_token, "pointn$umber") || equals(c_token, "pn")) {
	    c_token++;
	    if (allow_point) {
		newlp.p_number = int_expression();
		set_pn = 1;
	    } else {
		int_warn(c_token, "No pointnumber specifier allowed here)");
		int_expression();
	    }
	    continue;
	}

	if (almost_equals(c_token, "dasht$ype") || equals(c_token, "dt")) {
	    int tmp;
	    if (set_dt++)
		break;
	    c_token++;
	    tmp = parse_dashtype(&newlp.custom_dash_pattern);
	    /* Pull the dashtype from the list of already defined dashtypes, */
	    /* but only if it we didn't get an explicit one back from parse_dashtype */
	    if (tmp == DASHTYPE_AXIS)
		lp->l_type = LT_AXIS;
	    if (tmp >= 0)
		tmp = load_dashtype(&newlp.custom_dash_pattern, tmp + 1);
	    newlp.d_type = tmp;
	    continue;
	}


	/* caught unknown option -> quit the while(1) loop */
	break;
    }

    if (set_lt > 1 || set_pal > 1 || set_lw > 1 || set_pt > 1 || set_ps > 1 || set_dt > 1
    || (set_pi + set_pn > 1))
	int_error(c_token, "duplicate or conflicting arguments in style specification");

    if (set_pal) {
	lp->pm3d_color = newlp.pm3d_color;
	/* hidden3d uses this to decide that a single color surface is wanted */
	lp->flags |= LP_EXPLICIT_COLOR;
    } else {
	lp->flags &= ~LP_EXPLICIT_COLOR;
    }
    if (set_lw)
	lp->l_width = newlp.l_width;
    if (set_pt) {
	lp->p_type = newlp.p_type;
	memcpy(lp->p_char, newlp.p_char, sizeof(newlp.p_char));
    }
    if (set_ps)
	lp->p_size = newlp.p_size;
    if (set_pi) {
	lp->p_interval = newlp.p_interval;
	lp->p_number = 0;
    }
    if (set_pn) {
	lp->p_number = newlp.p_number;
	lp->p_interval = 0;
    }
    if (newlp.l_type == LT_COLORFROMCOLUMN)
	lp->l_type = LT_COLORFROMCOLUMN;
    if (set_dt) {
	lp->d_type = newlp.d_type;
	lp->custom_dash_pattern = newlp.custom_dash_pattern;
    }


    return new_lt;
}

/* <fillstyle> = {empty | solid {<density>} | pattern {<n>}} {noborder | border {<lt>}} */
const struct gen_table fs_opt_tbl[] = {
    {"e$mpty", FS_EMPTY},
    {"s$olid", FS_SOLID},
    {"p$attern", FS_PATTERN},
    {NULL, -1}
};

void
parse_fillstyle(struct fill_style_type *fs)
{
    TBOOLEAN set_fill = FALSE;
    TBOOLEAN set_border = FALSE;
    TBOOLEAN transparent = FALSE;

    if (END_OF_COMMAND)
	return;
    if (!equals(c_token, "fs") && !almost_equals(c_token, "fill$style"))
	return;
    c_token++;

    while (!END_OF_COMMAND) {
	int i;

	if (almost_equals(c_token, "trans$parent")) {
	    transparent = TRUE;
	    fs->filldensity = 50;
	    c_token++;
	    continue;
	}

	i = lookup_table(fs_opt_tbl, c_token);
	switch (i) {
	    default:
		 break;

	    case FS_EMPTY:
	    case FS_SOLID:
	    case FS_PATTERN:

		if (set_fill && fs->fillstyle != i)
		    int_error(c_token, "conflicting option");
		fs->fillstyle = i;
		set_fill = TRUE;
		c_token++;

		if (!transparent)
			fs->filldensity = 100;

		if (might_be_numeric(c_token)) {
		    if (fs->fillstyle == FS_SOLID) {
			/* user sets 0...1, but is stored as an integer 0..100 */
			fs->filldensity = 100.0 * real_expression() + 0.5;
			if (fs->filldensity < 0)
			    fs->filldensity = 0;
			if (fs->filldensity > 100)
			    fs->filldensity = 100;
		    } else if (fs->fillstyle == FS_PATTERN) {
			fs->fillpattern = int_expression();
			if (fs->fillpattern < 0)
			    fs->fillpattern = 0;
		    }
		}
		continue;
	}

	if (almost_equals(c_token, "bo$rder")) {
	    if (set_border && fs->border_color.lt == LT_NODRAW)
		int_error(c_token, "conflicting option");
	    fs->border_color.type = TC_DEFAULT;
	    set_border = TRUE;
	    c_token++;
	    if (END_OF_COMMAND)
		continue;
	    if (equals(c_token,"-") || isanumber(c_token)) {
		fs->border_color.type = TC_LT;
		fs->border_color.lt = int_expression() - 1;
	    } else if (equals(c_token,"lc") || almost_equals(c_token,"linec$olor")) {
		parse_colorspec(&fs->border_color, TC_Z);
	    } else if (equals(c_token,"rgb")
		   ||  equals(c_token,"lt") || almost_equals(c_token,"linet$ype")) {
		c_token--;
		parse_colorspec(&fs->border_color, TC_Z);
	    }
	    continue;
	} else if (almost_equals(c_token, "nobo$rder")) {
	    if (set_border && fs->border_color.lt != LT_NODRAW)
		int_error(c_token, "conflicting option");
	    fs->border_color.type = TC_LT;
	    fs->border_color.lt = LT_NODRAW;
	    set_border = TRUE;
	    c_token++;
	    continue;
	}

	/* Keyword must belong to someone else */
	break;
    }
    if (transparent) {
	if (fs->fillstyle == FS_SOLID)
	    fs->fillstyle = FS_TRANSPARENT_SOLID;
	else if (fs->fillstyle == FS_PATTERN)
	    fs->fillstyle = FS_TRANSPARENT_PATTERN;
    }
}

/*
 * Parse the sub-options of text color specification
 *   { def$ault | lt <linetype> | pal$ette { cb <val> | frac$tion <val> | z }
 * The ordering of alternatives shown in the line above is kept in the symbol definitions
 * TC_DEFAULT TC_LT TC_LINESTYLE TC_RGB TC_CB TC_FRAC TC_Z TC_VARIABLE (0 1 2 3 4 5 6 7)
 * and the "options" parameter to parse_colorspec limits legal input to the
 * corresponding point in the series. So TC_LT allows only default or linetype
 * coloring, while TC_Z allows all coloring options up to and including pal z
 */
void
parse_colorspec(struct t_colorspec *tc, int options)
{
    c_token++;
    if (END_OF_COMMAND)
	int_error(c_token, "expected colorspec");
    if (almost_equals(c_token,"def$ault")) {
	c_token++;
	tc->type = TC_DEFAULT;
    } else if (equals(c_token,"bgnd")) {
	c_token++;
	tc->type = TC_LT;
	tc->lt = LT_BACKGROUND;
    } else if (equals(c_token,"black")) {
	c_token++;
	tc->type = TC_LT;
	tc->lt = LT_BLACK;
    } else if (equals(c_token,"lt") || almost_equals(c_token, "linet$ype")) {
	struct lp_style_type lptemp;
	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token, "expected linetype");
	tc->type = TC_LT;
	tc->lt = int_expression()-1;
	if (tc->lt < LT_BACKGROUND) {
	    tc->type = TC_DEFAULT;
	    int_warn(c_token,"illegal linetype");
	}

	/*
	 * July 2014 - translate linetype into user-defined linetype color.
	 * This is a CHANGE!
	 */
	load_linetype(&lptemp, tc->lt + 1);
	*tc = lptemp.pm3d_color;
    } else if (options <= TC_LT) {
	tc->type = TC_DEFAULT;
	int_error(c_token, "only tc lt <n> possible here");
    } else if (equals(c_token,"ls") || almost_equals(c_token,"lines$tyle")) {
	c_token++;
	tc->type = TC_LINESTYLE;
	tc->lt = real_expression();
    } else if (almost_equals(c_token,"rgb$color")) {
	c_token++;
	tc->type = TC_RGB;
	if (almost_equals(c_token, "var$iable")) {
	    tc->value = -1.0;
	    c_token++;
	} else {
	    tc->value = 0.0;
	    tc->lt = parse_color_name();
	}
    } else if (almost_equals(c_token,"pal$ette")) {
	c_token++;
	if (equals(c_token,"z")) {
	    /* The actual z value is not yet known, fill it in later */
	    if (options >= TC_Z) {
		tc->type = TC_Z;
	    } else {
		tc->type = TC_DEFAULT;
		int_error(c_token, "palette z not possible here");
	    }
	    c_token++;
	} else if (equals(c_token,"cb")) {
	    tc->type = TC_CB;
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "expected cb value");
	    tc->value = real_expression();
	} else if (almost_equals(c_token,"frac$tion")) {
	    tc->type = TC_FRAC;
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "expected palette fraction");
	    tc->value = real_expression();
	    if (tc->value < 0. || tc->value > 1.0)
		int_error(c_token, "palette fraction out of range");
	} else {
	    /* END_OF_COMMAND or palette <blank> */
	    if (options >= TC_Z)
		tc->type = TC_Z;
	}
    } else if (options >= TC_VARIABLE && almost_equals(c_token,"var$iable")) {
	tc->type = TC_VARIABLE;
	c_token++;

    /* New: allow to skip the rgb keyword, as in  'plot $foo lc "blue"' */
    } else if (isstring(c_token)) {
	tc->type = TC_RGB;
	tc->lt = parse_color_name();

    } else {
	int_error(c_token, "colorspec option not recognized");
    }
}

long
parse_color_name()
{
    char *string;
    long color = -2;

    /* Terminal drivers call this after seeing a "background" option */
    if (almost_equals(c_token,"rgb$color") && almost_equals(c_token-1,"back$ground"))
	c_token++;
    if ((string = try_to_get_string())) {
	int iret;
	iret = lookup_table_nth(pm3d_color_names_tbl, string);
	if (iret >= 0)
	    color = pm3d_color_names_tbl[iret].value;
	else if (string[0] == '#')
	    iret = sscanf(string,"#%lx",&color);
	else if (string[0] == '0' && (string[1] == 'x' || string[1] == 'X'))
	    iret = sscanf(string,"%lx",&color);
	free(string);
	if (color == -2)
	    int_error(c_token, "unrecognized color name and not a string \"#AARRGGBB\" or \"0xAARRGGBB\"");
    } else {
	color = int_expression();
    }

    return (unsigned int)(color);
}

/* arrow parsing...
 *
 * allow_as controls whether we are allowed to accept arrowstyle in
 * the current context [ie not when doing a  set style arrow command]
 */

void
arrow_use_properties(struct arrow_style_type *arrow, int tag)
{
    /*  This function looks for an arrowstyle defined by 'tag' and
     *  copies its data into the structure 'ap'. */
    struct arrowstyle_def *this;

    /* If a color has already been set for this arrow, keep it */
    struct t_colorspec save_colorspec = arrow->lp_properties.pm3d_color;

    /* Default if requested style is not found */
    default_arrow_style(arrow);

    this = first_arrowstyle;
    while (this != NULL) {
	if (this->tag == tag) {
	    *arrow = this->arrow_properties;
	    break;
	} else {
	    this = this->next;
	}
    }

    /* tag not found: */
    if (!this || this->tag != tag)
	int_warn(NO_CARET,"arrowstyle %d not found", tag);

    /* Restore original color if the style doesn't specify one */
    if (arrow->lp_properties.pm3d_color.type == TC_DEFAULT)
	arrow->lp_properties.pm3d_color = save_colorspec;
}

void
arrow_parse(
    struct arrow_style_type *arrow,
    TBOOLEAN allow_as)
{
    int set_layer=0, set_line=0, set_head=0;
    int set_headsize=0, set_headfilled=0;

    /* Use predefined arrow style */
    if (allow_as && (almost_equals(c_token, "arrows$tyle") ||
		     equals(c_token, "as"))) {
	c_token++;
	if (almost_equals(c_token, "var$iable")) {
	    arrow->tag = AS_VARIABLE;
	    c_token++;
	} else {
	    arrow_use_properties(arrow, int_expression());
	}
	return;
    }

    /* No predefined arrow style; read properties from command line */
    /* avoid duplicating options */
    while (!END_OF_COMMAND) {
	if (equals(c_token, "nohead")) {
	    if (set_head++)
		break;
	    c_token++;
	    arrow->head = NOHEAD;
	    continue;
	}
	if (equals(c_token, "head")) {
	    if (set_head++)
		break;
	    c_token++;
	    arrow->head = END_HEAD;
	    continue;
	}
	if (equals(c_token, "backhead")) {
	    if (set_head++)
		break;
	    c_token++;
	    arrow->head = BACKHEAD;
	    continue;
	}
	if (equals(c_token, "heads")) {
	    if (set_head++)
		break;
	    c_token++;
	    arrow->head = BACKHEAD | END_HEAD;
	    continue;
	}

	if (almost_equals(c_token, "nobo$rder")) {
	    if (set_headfilled++)
		break;
	    c_token++;
	    arrow->headfill = AS_NOBORDER;
	    continue;
	}
	if (almost_equals(c_token, "fill$ed")) {
	    if (set_headfilled++)
		break;
	    c_token++;
	    arrow->headfill = AS_FILLED;
	    continue;
	}
	if (almost_equals(c_token, "empty")) {
	    if (set_headfilled++)
		break;
	    c_token++;
	    arrow->headfill = AS_EMPTY;
	    continue;
	}
	if (almost_equals(c_token, "nofill$ed")) {
	    if (set_headfilled++)
		break;
	    c_token++;
	    arrow->headfill = AS_NOFILL;
	    continue;
	}

	if (equals(c_token, "size")) {
	    struct position hsize;
	    if (set_headsize++)
		break;
	    hsize.scalex = hsize.scaley = hsize.scalez = first_axes;
	    /* only scalex used; scaley is angle of the head in [deg] */
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "head size expected");
	    get_position(&hsize);
	    arrow->head_length = hsize.x;
	    arrow->head_lengthunit = hsize.scalex;
	    arrow->head_angle = hsize.y;
	    arrow->head_backangle = hsize.z;
	    /* invalid backangle --> default of 90.0 degrees */
	    if (arrow->head_backangle <= arrow->head_angle)
		arrow->head_backangle = 90.0;

	    /* Assume adjustable size but check for 'fixed' instead */
	    arrow->head_fixedsize = FALSE;
	    continue;
	}

	if (almost_equals(c_token, "fix$ed")) {
	    arrow->head_fixedsize = TRUE;
	    c_token++;
	    continue;
	}

	if (equals(c_token, "back")) {
	    if (set_layer++)
		break;
	    c_token++;
	    arrow->layer = LAYER_BACK;
	    continue;
	}
	if (equals(c_token, "front")) {
	    if (set_layer++)
		break;
	    c_token++;
	    arrow->layer = LAYER_FRONT;
	    continue;
	}

	/* pick up a line spec - allow ls, but no point. */
	{
	    int stored_token = c_token;
	    lp_parse(&arrow->lp_properties, LP_ADHOC, FALSE);
	    if (stored_token == c_token || set_line++)
		break;
	    continue;
	}

	/* unknown option caught -> quit the while(1) loop */
	break;
    }

    if (set_layer>1 || set_line>1 || set_head>1 || set_headsize>1 || set_headfilled>1)
	int_error(c_token, "duplicated arguments in style specification");
}

void
get_image_options(t_image *image)
{
    if (almost_equals(c_token, "pix$els") || equals(c_token, "failsafe")) {
	c_token++;
	image->fallback = TRUE;
    }

}
