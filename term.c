#ifndef lint
static char *RCSid = "$Id: term.c,v 1.104 1998/06/18 14:55:19 ddenholm Exp $";
#endif

/* GNUPLOT - term.c */

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


 /* This module is responsible for looking after the terminal
  * drivers at the lowest level. Only this module (should)
  * know about all the various rules about interpreting
  * the terminal capabilities exported by the terminal
  * drivers in the table.
  *
  * Note that, as far as this module is concerned, a
  * terminal session lasts only until _either_ terminal
  * or output file changes. Before either is changed,
  * the terminal is shut down.
  * 
  * Entry points : (see also term/README)
  *
  * term_set_output() : called when  set output  invoked
  *
  * term_init()  : optional. Prepare the terminal for first
  *                use. It protects itself against subsequent calls.
  *
  * term_start_plot() : called at start of graph output. Calls term_init
  *                     if necessary
  *
  * term_apply_lp_properties() : apply linewidth settings
  *
  * term_end_plot() : called at the end of a plot
  *
  * term_reset() : called during int_error handling, to shut
  *                terminal down cleanly
  *
  * term_start_multiplot() : called by   set multiplot
  *
  * term_end_multiplot() : called by  set nomultiplot
  *
  * term_check_multiplot_okay() : called just before an interactive
  *                        prompt is issued while in multiplot mode,
  *                        to allow terminal to suspend if necessary,
  *                        Raises an error if interactive multiplot
  *                       is not supported.
  */

#include "plot.h"
#include "bitmap.h"
#include "setshow.h"
#include "driver.h"

#ifdef _Windows
FILE *open_printer __PROTO((void));	/* in wprinter.c */
void close_printer __PROTO((FILE * outfile));
# ifdef __MSC__
#  include <malloc.h>
# else
#  include <alloc.h>
# endif				/* MSC */
#endif /* _Windows */


/* true if terminal has been initialized */
static TBOOLEAN term_initialised;

/* true if terminal is in graphics mode */
static TBOOLEAN term_graphics = FALSE;

/* we have suspended the driver, in multiplot mode */
static TBOOLEAN term_suspended = FALSE;

/* true if? */
static TBOOLEAN opened_binary = FALSE;

/* true if require terminal to be initialized */
static TBOOLEAN term_force_init = FALSE;

extern FILE *gpoutfile;
extern char *outstr;
extern float xsize, ysize;

/* internal pointsize for do_point */
static double term_pointsize;

static void term_suspend __PROTO((void));
static void term_close_output __PROTO((void));
static void null_linewidth __PROTO((double));

void do_point __PROTO((unsigned int x, unsigned int y, int number));
void do_pointsize __PROTO((double size));
void line_and_point __PROTO((unsigned int x, unsigned int y, int number));
void do_arrow __PROTO((unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, int head));


#ifdef __ZTC__
char *ztc_init();
/* #undef TGIF */
#endif

#ifdef VMS
char *vms_init();
void vms_reset();
void term_mode_tek();
void term_mode_native();
void term_pasthru();
void term_nopasthru();
void fflush_binary();
# define FOPEN_BINARY(file) fopen(file, "wb", "rfm=fix", "bls=512", "mrs=512")
#else /* !VMS */
# define FOPEN_BINARY(file) fopen(file, "wb")
#endif /* !VMS */


/* This is needed because the unixplot library only writes to stdout. */
#if defined(UNIXPLOT) || defined(GNUGRAPH)
static FILE save_stdout;
#endif
static int unixplot = 0;

#if defined(MTOS) || defined(ATARI)
int aesid = -1;
#endif

#define NICE_LINE		0
#define POINT_TYPES		6

#ifndef DEFAULTTERM
# define DEFAULTTERM NULL
#endif

/* interface to the rest of gnuplot - the rules are getting
 * too complex for the rest of gnuplot to be allowed in
 */

#if defined(PIPES)
static TBOOLEAN pipe_open = FALSE;
#endif /* PIPES */

static void term_close_output()
{
    FPRINTF((stderr, "term_close_output\n"));

    opened_binary = FALSE;

    if (!outstr)		/* ie using stdout */
	return;

#if defined(PIPES)
    if (pipe_open) {
	(void) pclose(gpoutfile);
	pipe_open = FALSE;
    } else
#endif /* PIPES */
#ifdef _Windows
    if (stricmp(outstr, "PRN") == 0)
	close_printer(gpoutfile);
    else
#endif
	(void) fclose(gpoutfile);

    gpoutfile = stdout;		/* Don't dup... */
    free(outstr);
    outstr = NULL;
}

#ifdef OS2
# define POPEN_MODE ("wb")
#else
# define POPEN_MODE ("w")
#endif

/* assigns dest to outstr, so it must be allocated or NULL
 * and it must not be outstr itself !
 */
void term_set_output(dest)
char *dest;
{
    FILE *f;

    FPRINTF((stderr, "term_set_output\n"));
    assert(dest == NULL || dest != outstr);

    if (multiplot) {
	fputs("In multiplotmode you can't change the output\n", stderr);
	return;
    }
    if (term && term_initialised) {
	(*term->reset) ();
	term_initialised = FALSE;
    }
    if (dest == NULL) {		/* stdout */
	UP_redirect(4);
	term_close_output();
    } else {

#if defined(PIPES)
	if (*dest == '|') {
	    if ((f = popen(dest + 1, POPEN_MODE)) == (FILE *) NULL)
		    os_error("cannot create pipe; output not changed", c_token);
		else
		    pipe_open = TRUE;
	} else
#endif /* PIPES */

#ifdef _Windows
	if (outstr && stricmp(outstr, "PRN") == 0) {
	    /* we can't call open_printer() while printer is open, so */
	    close_printer(gpoutfile);	/* close printer immediately if open */
	    gpoutfile = stdout;	/* and reset output to stdout */
	    free(outstr);
	    outstr = NULL;
	}
	if (stricmp(dest, "PRN") == 0) {
	    if ((f = open_printer()) == (FILE *) NULL)
		os_error("cannot open printer temporary file; output may have changed", c_token);
	} else
#endif

	{
	    if (term && (term->flags & TERM_BINARY)) {
		f = FOPEN_BINARY(dest);
	    } else
		f = fopen(dest, "w");

	    if (f == (FILE *) NULL)
		os_error("cannot open file; output not changed", c_token);
	}
	term_close_output();
	gpoutfile = f;
	outstr = dest;
	opened_binary = (term && (term->flags & TERM_BINARY));
	UP_redirect(1);
    }
}

void term_init()
{
    FPRINTF((stderr, "term_init()\n"));

    if (!term)
	int_error("No terminal defined", NO_CARET);

    /* check if we have opened the output file in the wrong mode
     * (text/binary), if set term comes after set output
     * This was originally done in change_term, but that
     * resulted in output files being truncated
     */

    if (outstr &&
	(((term->flags & TERM_BINARY) && !opened_binary) ||
	 ((!(term->flags & TERM_BINARY) && opened_binary)))) {
	/* this is nasty - we cannot just term_set_output(outstr)
	 * since term_set_output will first free outstr and we
	 * end up with an invalid pointer. I think I would
	 * prefer to defer opening output file until first plot.
	 */
	char *temp = gp_alloc(strlen(outstr) + 1, "temp file string");
	if (temp) {
	    FPRINTF((stderr, "term_init : reopening \"%s\" as %s\n",
		     outstr, term->flags & TERM_BINARY ? "binary" : "text"));
	    strcpy(temp, outstr);
	    term_set_output(temp);	/* will free outstr */
	} else
	    fputs("Cannot reopen output file in binary", stderr);
	/* and carry on, hoping for the best ! */
    }
#ifdef OS2
    else if (!outstr && !interactive && (term->flags & TERM_BINARY)) {
	/* binary (eg gif) to stdout in a non-interactive session */
	fflush(stdout);		// _fsetmode requires an empty buffer

	_fsetmode(stdout, "b");
    }
#endif


    if (!term_initialised || term_force_init) {
	FPRINTF((stderr, "- calling term->init()\n"));
	(*term->init) ();
	term_initialised = TRUE;
    }
}


void term_start_plot()
{
    FPRINTF((stderr, "term_start_plot()\n"));

    if (!term_initialised)
	term_init();

    if (!term_graphics) {
	FPRINTF((stderr, "- calling term->graphics()\n"));
	(*term->graphics) ();
	term_graphics = TRUE;
    } else if (multiplot && term_suspended) {
	if (term->resume) {
	    FPRINTF((stderr, "- calling term->resume()\n"));
	    (*term->resume) ();
	}
	term_suspended = FALSE;
    }
}

void term_end_plot()
{
    FPRINTF((stderr, "term_end_plot()\n"));

    if (!term_initialised)
	return;

    if (!multiplot) {
	FPRINTF((stderr, "- calling term->text()\n"));
	(*term->text) ();
	term_graphics = FALSE;
    }
#ifdef VMS
    if (opened_binary)
	fflush_binary();
    else
#endif /* VMS */

	(void) fflush(gpoutfile);
}

void term_start_multiplot()
{
    FPRINTF((stderr, "term_start_multiplot()\n"));
    if (multiplot)
	term_end_multiplot();

    multiplot = TRUE;
    term_start_plot();
}

void term_end_multiplot()
{
    FPRINTF((stderr, "term_end_multiplot()\n"));
    if (!multiplot)
	return;

    if (term_suspended) {
	if (term->resume)
	    (*term->resume) ();
	term_suspended = FALSE;
    }
    multiplot = FALSE;

    term_end_plot();
}



static void term_suspend()
{
    FPRINTF((stderr, "term_suspend()\n"));
    if (term_initialised && !term_suspended && term->suspend) {
	FPRINTF((stderr, "- calling term->suspend()\n"));
	(*term->suspend) ();
	term_suspended = TRUE;
    }
}

void term_reset()
{
    FPRINTF((stderr, "term_reset()\n"));

    if (!term_initialised)
	return;

    if (term_suspended) {
	if (term->resume) {
	    FPRINTF((stderr, "- calling term->resume()\n"));
	    (*term->resume) ();
	}
	term_suspended = FALSE;
    }
    if (term_graphics) {
	(*term->text) ();
	term_graphics = FALSE;
    }
    if (term_initialised) {
	(*term->reset) ();
	term_initialised = FALSE;
    }
}

void term_apply_lp_properties(lp)
struct lp_style_type *lp;
{
    /*  This function passes all the line and point properties to the
     *  terminal driver and issues the corresponding commands.
     *
     *  Alas, sometimes it might be necessary to give some help to
     *  this function by explicitly issuing additional '(*term)(...)'
     *  commands.
     */

    if (lp->pointflag) {
	/* change points, too
	 * Currently, there is no 'pointtype' function.  For points
	 * there is a special function also dealing with (x,y) co-
	 * ordinates.
	 */
	(*term->pointsize) (lp->p_size);
    }
    /*  _first_ set the line width, _then_ set the line type !

     *  The linetype might depend on the linewidth in some terminals.
     */
    (*term->linewidth) (lp->l_width);
    (*term->linetype) (lp->l_type);
}



void term_check_multiplot_okay(f_interactive)
TBOOLEAN f_interactive;
{
    FPRINTF((stderr, "term_multiplot_okay(%d)\n", f_interactive));

    if (!term_initialised)
	return;			/* they've not started yet */

    /* make sure that it is safe to issue an interactive prompt
     * it is safe if
     *   it is not an interactive read, or
     *   the terminal supports interactive multiplot, or
     *   we are not writing to stdout and terminal doesn't
     *     refuse multiplot outright
     */
    if (!f_interactive || (term->flags & TERM_CAN_MULTIPLOT) ||
	((gpoutfile != stdout) && !(term->flags & TERM_CANNOT_MULTIPLOT))
	) {
	/* it's okay to use multiplot here, but suspend first */
	term_suspend();
	return;
    }
    /* uh oh : they're not allowed to be in multiplot here */

    term_end_multiplot();

    /* at this point we know that it is interactive and that the
     * terminal can either only do multiplot when writing to
     * to a file, or it does not do multiplot at all
     */

    if (term->flags & TERM_CANNOT_MULTIPLOT)
	int_error("This terminal does not support multiplot", NO_CARET);
    else
	int_error("Must set output to a file or put all multiplot commands on one input line", NO_CARET);
}

void do_point(x, y, number)
unsigned int x, y;
int number;
{
    register int htic, vtic;
    register struct termentry *t = term;

    if (number < 0) {		/* do dot */
	(*t->move) (x, y);
	(*t->vector) (x, y);
	return;
    }
    number %= POINT_TYPES;
    /* should be in term_tbl[] in later version */
    htic = (term_pointsize * t->h_tic / 2);
    vtic = (term_pointsize * t->v_tic / 2);

    switch (number) {
    case 0:			/* do diamond */
	(*t->move) (x - htic, y);
	(*t->vector) (x, y - vtic);
	(*t->vector) (x + htic, y);
	(*t->vector) (x, y + vtic);
	(*t->vector) (x - htic, y);
	(*t->move) (x, y);
	(*t->vector) (x, y);
	break;
    case 1:			/* do plus */
	(*t->move) (x - htic, y);
	(*t->vector) (x - htic, y);
	(*t->vector) (x + htic, y);
	(*t->move) (x, y - vtic);
	(*t->vector) (x, y - vtic);
	(*t->vector) (x, y + vtic);
	break;
    case 2:			/* do box */
	(*t->move) (x - htic, y - vtic);
	(*t->vector) (x - htic, y - vtic);
	(*t->vector) (x + htic, y - vtic);
	(*t->vector) (x + htic, y + vtic);
	(*t->vector) (x - htic, y + vtic);
	(*t->vector) (x - htic, y - vtic);
	(*t->move) (x, y);
	(*t->vector) (x, y);
	break;
    case 3:			/* do X */
	(*t->move) (x - htic, y - vtic);
	(*t->vector) (x - htic, y - vtic);
	(*t->vector) (x + htic, y + vtic);
	(*t->move) (x - htic, y + vtic);
	(*t->vector) (x - htic, y + vtic);
	(*t->vector) (x + htic, y - vtic);
	break;
    case 4:			/* do triangle */
	(*t->move) (x, y + (4 * vtic / 3));
	(*t->vector) (x - (4 * htic / 3), y - (2 * vtic / 3));
	(*t->vector) (x + (4 * htic / 3), y - (2 * vtic / 3));
	(*t->vector) (x, y + (4 * vtic / 3));
	(*t->move) (x, y);
	(*t->vector) (x, y);
	break;
    case 5:			/* do star */
	(*t->move) (x - htic, y);
	(*t->vector) (x - htic, y);
	(*t->vector) (x + htic, y);
	(*t->move) (x, y - vtic);
	(*t->vector) (x, y - vtic);
	(*t->vector) (x, y + vtic);
	(*t->move) (x - htic, y - vtic);
	(*t->vector) (x - htic, y - vtic);
	(*t->vector) (x + htic, y + vtic);
	(*t->move) (x - htic, y + vtic);
	(*t->vector) (x - htic, y + vtic);
	(*t->vector) (x + htic, y - vtic);
	break;
    }
}

void do_pointsize(size)
double size;
{
    term_pointsize = (size >= 0 ? size : 1);
}


/*
 * general point routine
 */
void line_and_point(x, y, number)
unsigned int x, y;
int number;
{
    /* temporary(?) kludge to allow terminals with bad linetypes 
       to make nice marks */

    (*term->linetype) (NICE_LINE);
    do_point(x, y, number);
}

/* 
 * general arrow routine
 *
 * I set the angle between the arrowhead and the line 15 degree.
 * The length of arrowhead varies depending on the line length 
 * within the the range [0.3*(the-tic-length), 2*(the-tic-length)].
 * No head is printed if the arrow length is zero.
 *
 *            Yasu-hiro Yamazaki(hiro@rainbow.physics.utoronto.ca)
 *            Jul 1, 1993
 */

#define COS15 (0.96593)		/* cos of 15 degree */
#define SIN15 (0.25882)		/* sin of 15 degree */

#define HEAD_LONG_LIMIT  (2.0)	/* long  limit of arrowhead length */
#define HEAD_SHORT_LIMIT (0.3)	/* short limit of arrowhead length */
				/* their units are the "tic" length */

#define HEAD_COEFF  (0.3)	/* default value of head/line length ratio */

void do_arrow(sx, sy, ex, ey, head)
unsigned int sx, sy;		/* start point */
unsigned int ex, ey;		/* end point (point of arrowhead) */
TBOOLEAN head;
{
    register struct termentry *t = term;
    float len_tic = ((double) (t->h_tic + t->v_tic)) / 2.0;
    /* average of tic sizes */
    /* (dx,dy) : vector from end to start */
    double dx = (double) sx - (double) ex;
    double dy = (double) sy - (double) ey;
    double len_arrow = sqrt(dx * dx + dy * dy);

    /* draw the line for the arrow. That's easy. */
    (*t->move) (sx, sy);
    (*t->vector) (ex, ey);

    /* no head for arrows whih length = 0
     * or, to be more specific, length < DBL_EPSILON,
     * because len_arrow will almost always be != 0
     */
    if (head && fabs(len_arrow) >= DBL_EPSILON) {
	/* now calc the head_coeff */
	double coeff_shortest = len_tic * HEAD_SHORT_LIMIT / len_arrow;
	double coeff_longest = len_tic * HEAD_LONG_LIMIT / len_arrow;
	double head_coeff = GPMAX(coeff_shortest,
				  GPMIN(HEAD_COEFF, coeff_longest));
	/* now draw the arrow head. */
	/* we put the arrowhead marks at 15 degrees to line */
	int x, y;		/* one endpoint */

	x = (int) (ex + (COS15 * dx - SIN15 * dy) * head_coeff);
	y = (int) (ey + (SIN15 * dx + COS15 * dy) * head_coeff);
	(*t->move) (x, y);
	(*t->vector) (ex, ey);

	x = (int) (ex + (COS15 * dx + SIN15 * dy) * head_coeff);
	y = (int) (ey + (-SIN15 * dx + COS15 * dy) * head_coeff);
	(*t->vector) (x, y);
    }
}

#if 0				/* oiginal routine */
#define ROOT2 (1.41421)		/* sqrt of 2 */

void org_do_arrow(sx, sy, ex, ey, head)
int sx, sy;			/* start point */
int ex, ey;			/* end point (point of arrowhead) */
TBOOLEAN head;
{
    register struct termentry *t = term;
    int len = (t->h_tic + t->v_tic) / 2;	/* arrowhead size = avg of tic sizes */

    /* draw the line for the arrow. That's easy. */
    (*t->move) (sx, sy);
    (*t->vector) (ex, ey);

    if (head) {
	/* now draw the arrow head. */
	/* we put the arrowhead marks at 45 degrees to line */
	if (sx == ex) {
	    /* vertical line, special case */
	    int delta = ((float) len / ROOT2 + 0.5);
	    if (sy < ey)
		delta = -delta;	/* up arrow goes the other way */
	    (*t->move) (ex - delta, ey + delta);
	    (*t->vector) (ex, ey);
	    (*t->vector) (ex + delta, ey + delta);
	} else {
	    int dx = sx - ex;
	    int dy = sy - ey;
	    double coeff = len / sqrt(2.0 * ((double) dx * (double) dx
					     + (double) dy * (double) dy));
	    int x, y;		/* one endpoint */

	    x = (int) (ex + (dx + dy) * coeff);
	    y = (int) (ey + (dy - dx) * coeff);
	    (*t->move) (x, y);
	    (*t->vector) (ex, ey);

	    x = (int) (ex + (dx - dy) * coeff);
	    y = (int) (ey + (dy + dx) * coeff);
	    (*t->vector) (x, y);
	}
    }
}

#endif /* original routine */


#define TERM_PROTO
#define TERM_BODY
#define TERM_PUBLIC static

#include "term.h"

#undef TERM_PROTO
#undef TERM_BODY
#undef TERM_PUBLIC


/* Dummy functions for unavailable features */
/* return success if they asked for default - this simplifies code
 * where param is passed as a param. Client can first pass it here,
 * and only if it fails do they have to see what was trying to be done
 */

/* change angle of text.  0 is horizontal left to right.
   * 1 is vertical bottom to top (90 deg rotate)  
 */
int null_text_angle(ang)
int ang;
{
    return (ang == 0);
}

/* change justification of text.  
 * modes are LEFT (flush left), CENTRE (centred), RIGHT (flush right)
 */
int null_justify_text(just)
enum JUSTIFY just;
{
    return (just == LEFT);
}


/* Change scale of plot.
 * Parameters are x,y scaling factors for this plot.
 * Some terminals (eg latex) need to do scaling themselves.
 */
int null_scale(x, y)
double x;
double y;
{
    return FALSE;		/* can't be done */
}

int do_scale(x, y)
double x;
double y;
{
    return TRUE;		/* can be done */
}

void options_null()
{
    term_options[0] = '\0';	/* we have no options */
}

void UNKNOWN_null()
{
}

void MOVE_null(x, y)
unsigned int x, y;
{
}

void LINETYPE_null(t)
int t;
{
}

void PUTTEXT_null(x, y, s)
unsigned int x, y;
char *s;
{
}


int set_font_null(s)
char *s;
{
    return FALSE;
}

static void null_linewidth(s)
double s;
{
}


/* cast to get rid of useless warnings about UNKNOWN_null */
typedef void (*void_fp) __PROTO((void));


/* setup the magic macros to compile in the right parts of the
 * terminal drivers included by term.h
 */

#define TERM_TABLE
#define TERM_TABLE_START(x) ,{
#define TERM_TABLE_END(x)   }


/*
 * term_tbl[] contains an entry for each terminal.  "unknown" must be the
 *   first, since term is initialized to 0.
 */
struct termentry term_tbl[] =
{
    {"unknown", "Unknown terminal type - not a plotting device",
     100, 100, 1, 1,
     1, 1, options_null, UNKNOWN_null, UNKNOWN_null,
     UNKNOWN_null, null_scale, UNKNOWN_null, MOVE_null, MOVE_null,
     LINETYPE_null, PUTTEXT_null}
    ,
    {"table", "Dump ASCII table of X Y [Z] values to output",
     100, 100, 1, 1,
     1, 1, options_null, UNKNOWN_null, UNKNOWN_null,
     UNKNOWN_null, null_scale, UNKNOWN_null, MOVE_null, MOVE_null,
     LINETYPE_null, PUTTEXT_null}
#include "term.h"

};

#define TERMCOUNT (sizeof(term_tbl)/sizeof(struct termentry))

void list_terms()
{
    register int i;
    char line_buffer[BUFSIZ];

    StartOutput();
    sprintf(line_buffer,"\nAvailable terminal types:\n");
    OutLine(line_buffer);

    for (i = 0; i < TERMCOUNT; i++) {
	sprintf(line_buffer,"  %15s  %s\n",
		term_tbl[i].name, term_tbl[i].description);
	OutLine(line_buffer);
    }

    EndOutput();
}


/* set_term: get terminal number from name on command line
 * will change 'term' variable if successful
 */
struct termentry *
 set_term(c_token_arg)
int c_token_arg;
{
    register struct termentry *t = NULL;
    char *input_name;

    if (!token[c_token_arg].is_token)
	int_error("terminal name expected", c_token_arg);
    input_name = input_line + token[c_token_arg].start_index;
    t = change_term(input_name, token[c_token_arg].length);
    if (!t)
	int_error("unknown or ambiguous terminal type; type just 'set terminal' for a list",
		  c_token_arg);

    /* otherwise the type was changed */

    return (t);
}

/* change_term: get terminal number from name and set terminal type
 *
 * returns NULL for unknown or ambiguous, otherwise is terminal
 * driver pointer
 */
struct termentry *
 change_term(name, length)
char *name;
int length;
{
    int i;
    struct termentry *t = NULL;

    for (i = 0; i < TERMCOUNT; i++) {
	if (!strncmp(name, term_tbl[i].name, length)) {
	    if (t)
		return (NULL);	/* ambiguous */
	    t = term_tbl + i;
	}
    }

    if (!t)			/* unknown */
	return (NULL);

    /* Success: set terminal type now */

    term = t;
    term_initialised = FALSE;
    name = term->name;

    if (term->scale != null_scale)
	fputs("Warning : scale interface is not null_scale - may not work with multiplot\n", stderr);

    /* check that optional fields are initialised to something */
    if (term->text_angle == 0)
	term->text_angle = null_text_angle;
    if (term->justify_text == 0)
	term->justify_text = null_justify_text;
    if (term->point == 0)
	term->point = do_point;
    if (term->arrow == 0)
	term->arrow = do_arrow;
    if (term->set_font == 0)
	term->set_font = set_font_null;
    if (term->pointsize == 0)
	term->pointsize = do_pointsize;
    if (term->linewidth == 0)
	term->linewidth = null_linewidth;

    /* Special handling for unixplot term type */
    if (!strncmp("unixplot", name, 8)) {
	UP_redirect(2);		/* Redirect actual stdout for unixplots */
    } else if (unixplot) {
	UP_redirect(3);		/* Put stdout back together again. */
    }
    if (interactive)
	fprintf(stderr, "Terminal type set to '%s'\n", name);

    return (t);
}

/*
 * Routine to detect what terminal is being used (or do anything else
 * that would be nice).  One anticipated (or allowed for) side effect
 * is that the global ``term'' may be set. 
 * The environment variable GNUTERM is checked first; if that does
 * not exist, then the terminal hardware is checked, if possible, 
 * and finally, we can check $TERM for some kinds of terminals.
 * A default can be set with -DDEFAULTTERM=myterm in the Makefile
 * or #define DEFAULTTERM myterm in term.h
 */
/* thanks to osupyr!alden (Dave Alden) for the original GNUTERM code */
void init_terminal()
{
    char *term_name = DEFAULTTERM;
#if (defined(__TURBOC__) && defined(MSDOS) && !defined(_Windows)) || defined(NEXT) || defined(SUN) || defined(X11)
    char *env_term = NULL;	/* from TERM environment var */
#endif
#ifdef X11
    char *display = NULL;
#endif
    char *gnuterm = NULL;

    /* GNUTERM environment variable is primary */
    gnuterm = getenv("GNUTERM");
    if (gnuterm != (char *) NULL) {
	term_name = gnuterm;
    } else {

#ifdef __ZTC__
	term_name = ztc_init();
#endif

#ifdef VMS
	term_name = vms_init();
#endif /* VMS */

#ifdef NEXT
	env_term = getenv("TERM");
	if (term_name == (char *) NULL
	    && env_term != (char *) NULL && STREQ(term, "next"))
	    term_name = "next";
#endif /* NeXT */

#ifdef SUN
	env_term = getenv("TERM");	/* try $TERM */
	if (term_name == (char *) NULL
	    && env_term != (char *) NULL && STREQ(env_term, "sun"))
	    term_name = "sun";
#endif /* SUN */

#ifdef _Windows
	term_name = "win";
#endif /* _Windows */

#ifdef GPR
	/* find out whether stdout is a DM pad. See term/gpr.trm */
	if (gpr_isa_pad())
	    term_name = "gpr";
#else
# ifdef APOLLO
	/* find out whether stdout is a DM pad. See term/apollo.trm */
	if (apollo_isa_pad())
	    term_name = "apollo";
# endif				/* APOLLO */
#endif /* GPR    */

#ifdef X11
	env_term = getenv("TERM");	/* try $TERM */
	if (term_name == (char *) NULL
	    && env_term != (char *) NULL && STREQ(env_term, "xterm"))
	    term_name = "x11";
	display = getenv("DISPLAY");
	if (term_name == (char *) NULL && display != (char *) NULL)
	    term_name = "x11";
	if (X11_Display)
	    term_name = "x11";
#endif /* x11 */

#ifdef AMIGA
	term_name = "amiga";
#endif

#if defined(ATARI) || defined(MTOS)
	term_name = "atari";
#endif

#ifdef UNIXPC
	if (iswind() == 0) {
	    term_name = "unixpc";
	}
#endif /* unixpc */

#ifdef CGI
	if (getenv("CGIDISP") || getenv("CGIPRNT"))
	    term_name = "cgi";
#endif /*CGI */

#ifdef DJGPP
	term_name = "svga";
#endif

#ifdef GRASS
	term_name = "grass";
#endif

#ifdef OS2
/*      if (_osmode==OS2_MODE) term_name = "pm" ; else term_name = "emxvga"; */
# ifdef X11
/* This catch is hopefully ok ... */
	env_term = getenv("WINDOWID");
	display = getenv("DISPLAY");
	if ((env_term != (char *) NULL) && (display != (char *) NULL))
	    term_name = "x11";
	else
# endif				/* X11 */
	    term_name = "pm";
#endif /*OS2 */

/* set linux terminal only if LINUX_setup was successfull, if we are on X11
   LINUX_setup has failed, also if we are logged in by network */
#ifdef LINUXVGA
	if (LINUX_graphics_allowed)
	    term_name = "linux";
#endif /* LINUXVGA */
    }

    /* We have a name, try to set term type */
    if (term_name != NULL && *term_name != '\0') {
	if (change_term(term_name, (int) strlen(term_name)))
	    return;
	fprintf(stderr, "Unknown or ambiguous terminal name '%s'\n", term_name);
    }
    change_term("unknown", 7);
}


#ifdef __ZTC__
char *ztc_init()
{
    int g_mode;
    char *term_name = NULL;

    g_mode = fg_init();

    switch (g_mode) {
    case FG_NULL:
	fputs("Graphics card not detected or not supported.\n", stderr);
	exit(1);
    case FG_HERCFULL:
	term_name = "hercules";
	break;
    case FG_EGAMONO:
	term_name = "egamono";
	break;
    case FG_EGAECD:
	term_name = "egalib";
	break;
    case FG_VGA11:
	term_name = "vgamono";
	break;
    case FG_VGA12:
	term_name = "vgalib";
	break;
    case FG_VESA6A:
	term_name = "svgalib";
	break;
    case FG_VESA5:
	term_name = "ssvgalib";
	break;
    }
    fg_term();
    return (term_name);
}
#endif /* __ZTC__ */


/*
   This is always defined so we don't have to have command.c know if it
   is there or not.
 */
#if !(defined(UNIXPLOT) || defined(GNUGRAPH))
void UP_redirect(caller)
int caller;
{
    caller = caller;		/* to stop Turbo C complaining 
				   * about caller not being used */
}

#else /* UNIXPLOT || GNUGRAPH */
void UP_redirect(caller)
int caller;
/*
   Unixplot can't really write to gpoutfile--it wants to write to stdout.
   This is normally ok, but the original design of gnuplot gives us
   little choice.  Originally users of unixplot had to anticipate
   their needs and redirect all I/O to a file...  Not very gnuplot-like.

   caller:  1 - called from SET OUTPUT "FOO.OUT"
   2 - called from SET TERM UNIXPLOT
   3 - called from SET TERM other
   4 - called from SET OUTPUT
 */
{
    switch (caller) {
    case 1:
	/* Don't save, just replace stdout w/gpoutfile (save was already done). */
	if (unixplot)
	    *(stdout) = *(gpoutfile);	/* Copy FILE structure */
	break;
    case 2:
	if (!unixplot) {
	    fflush(stdout);
	    save_stdout = *(stdout);
	    *(stdout) = *(gpoutfile);	/* Copy FILE structure */
	    unixplot = 1;
	}
	break;
    case 3:
	/* New terminal in use--put stdout back to original. */
/* closepl(); *//* This is called by the term. */
	fflush(stdout);
	*(stdout) = save_stdout;	/* Copy FILE structure */
	unixplot = 0;
	break;
    case 4:
	/*  User really wants to go to normal output... */
	if (unixplot) {
	    fflush(stdout);
	    *(stdout) = save_stdout;	/* Copy FILE structure */
	}
	break;
    }
}
#endif /* UNIXPLOT || GNUGRAPH */


/* test terminal by drawing border and text */
/* called from command test */
void test_term()
{
    register struct termentry *t = term;
    char *str;
    int x, y, xl, yl, i;
    unsigned int xmax_t, ymax_t;
    char label[MAX_ID_LEN];
    int key_entry_height;
    int p_width;

    term_start_plot();
    screen_ok = FALSE;
    xmax_t = (unsigned int) (t->xmax * xsize);
    ymax_t = (unsigned int) (t->ymax * ysize);

    p_width = pointsize * (t->h_tic);
    key_entry_height = pointsize * (t->v_tic) * 1.25;
    if (key_entry_height < (t->v_char))
	key_entry_height = (t->v_char);

    /* border linetype */
    (*t->linetype) (-2);
    (*t->move) (0, 0);
    (*t->vector) (xmax_t - 1, 0);
    (*t->vector) (xmax_t - 1, ymax_t - 1);
    (*t->vector) (0, ymax_t - 1);
    (*t->vector) (0, 0);
    (void) (*t->justify_text) (LEFT);
    (*t->put_text) (t->h_char * 5, ymax_t - t->v_char * 3, "Terminal Test");
    /* axis linetype */
    (*t->linetype) (-1);
    (*t->move) (xmax_t / 2, 0);
    (*t->vector) (xmax_t / 2, ymax_t - 1);
    (*t->move) (0, ymax_t / 2);
    (*t->vector) (xmax_t - 1, ymax_t / 2);
    /* test width and height of characters */
    (*t->linetype) (-2);
    (*t->move) (xmax_t / 2 - t->h_char * 10, ymax_t / 2 + t->v_char / 2);
    (*t->vector) (xmax_t / 2 + t->h_char * 10, ymax_t / 2 + t->v_char / 2);
    (*t->vector) (xmax_t / 2 + t->h_char * 10, ymax_t / 2 - t->v_char / 2);
    (*t->vector) (xmax_t / 2 - t->h_char * 10, ymax_t / 2 - t->v_char / 2);
    (*t->vector) (xmax_t / 2 - t->h_char * 10, ymax_t / 2 + t->v_char / 2);
    (*t->put_text) (xmax_t / 2 - t->h_char * 10, ymax_t / 2,
		    "12345678901234567890");
    /* test justification */
    (void) (*t->justify_text) (LEFT);
    (*t->put_text) (xmax_t / 2, ymax_t / 2 + t->v_char * 6, "left justified");
    str = "centre+d text";
    if ((*t->justify_text) (CENTRE))
	(*t->put_text) (xmax_t / 2,
			ymax_t / 2 + t->v_char * 5, str);
    else
	(*t->put_text) (xmax_t / 2 - strlen(str) * t->h_char / 2,
			ymax_t / 2 + t->v_char * 5, str);
    str = "right justified";
    if ((*t->justify_text) (RIGHT))
	(*t->put_text) (xmax_t / 2,
			ymax_t / 2 + t->v_char * 4, str);
    else
	(*t->put_text) (xmax_t / 2 - strlen(str) * t->h_char,
			ymax_t / 2 + t->v_char * 4, str);
    /* test text angle */
    str = "rotated ce+ntred text";
    if ((*t->text_angle) (1)) {
	if ((*t->justify_text) (CENTRE))
	    (*t->put_text) (t->v_char,
			    ymax_t / 2, str);
	else
	    (*t->put_text) (t->v_char,
			    ymax_t / 2 - strlen(str) * t->h_char / 2, str);
    } else {
	(void) (*t->justify_text) (LEFT);
	(*t->put_text) (t->h_char * 2, ymax_t / 2 - t->v_char * 2, "Can't rotate text");
    }
    (void) (*t->justify_text) (LEFT);
    (void) (*t->text_angle) (0);
    /* test tic size */
    (*t->move) ((unsigned int) (xmax_t / 2 + t->h_tic * (1 + ticscale)), (unsigned) 0);
    (*t->vector) ((unsigned int) (xmax_t / 2 + t->h_tic * (1 + ticscale)), (unsigned int)
		  (ticscale * t->v_tic));
    (*t->move) ((unsigned int) (xmax_t / 2), (unsigned int) (t->v_tic * (1 + ticscale)));
    (*t->vector) ((unsigned int) (xmax_t / 2 + ticscale * t->h_tic), (unsigned int) (t->v_tic * (1
												 + ticscale)));
    (*t->put_text) ((unsigned int) (xmax_t / 2 - 10 * t->h_char), (unsigned int) (t->v_tic * 2 +
										  t->v_char / 2),
		    "test tics");

    /* test line and point types */
    x = xmax_t - t->h_char * 6 - p_width;
    y = ymax_t - key_entry_height;
    (*t->pointsize) (pointsize);
    for (i = -2; y > key_entry_height; i++) {
	(*t->linetype) (i);
	/*      (void) sprintf(label,"%d",i);  Jorgen Lippert
	   lippert@risoe.dk */
	(void) sprintf(label, "%d", i + 1);
	if ((*t->justify_text) (RIGHT))
	    (*t->put_text) (x, y, label);
	else
	    (*t->put_text) (x - strlen(label) * t->h_char, y, label);
	(*t->move) (x + t->h_char, y);
	(*t->vector) (x + t->h_char * 4, y);
	if (i >= -1)
	    (*t->point) (x + t->h_char * 5 + p_width / 2, y, i);
	y -= key_entry_height;
    }
    /* test some arrows */
    (*t->linewidth) (1.0);
    (*t->linetype) (0);
    x = xmax_t / 4;
    y = ymax_t / 4;
    xl = t->h_tic * 5;
    yl = t->v_tic * 5;
    (*t->arrow) (x, y, x + xl, y, TRUE);
    (*t->arrow) (x, y, x + xl / 2, y + yl, TRUE);
    (*t->arrow) (x, y, x, y + yl, TRUE);
    (*t->arrow) (x, y, x - xl / 2, y + yl, FALSE);
    (*t->arrow) (x, y, x - xl, y, TRUE);
    (*t->arrow) (x, y, x - xl, y - yl, TRUE);
    (*t->arrow) (x, y, x, y - yl, TRUE);
    (*t->arrow) (x, y, x + xl, y - yl, TRUE);

    term_end_plot();
}

#if 0
# if defined(MSDOS)||defined(g)||defined(MTOS)||defined(OS2)||defined(_Windows)||defined(DOS386)
/* output for some terminal types must be binary to stop non Unix computers
   changing \n to \r\n. 
   If the output is not STDOUT, the following code reopens gpoutfile 
   with binary mode. */
void reopen_binary()
{
    if (outstr) {
	(void) fclose(gpoutfile);
#  ifdef _Windows
	if (!stricmp(outstr, "PRN")) {
	    /* use temp file for windows */
	    (void) strcpy(filename, win_prntmp);
	}
#  endif
	if ((gpoutfile = fopen(filename, "wb")) == (FILE *) NULL) {
	    if ((gpoutfile = fopen(filename, "w")) == (FILE *) NULL) {
		os_error("cannot reopen file with binary type; output unknown",
			 NO_CARET);
	    } else {
		os_error("cannot reopen file with binary type; output reset to ascii",
			 NO_CARET);
	    }
	}
#  if defined(__TURBOC__) && defined(MSDOS)
#   ifndef _Windows
	if (!stricmp(outstr, "PRN")) {
	    /* Put the printer into binary mode. */
	    union REGS regs;
	    regs.h.ah = 0x44;	/* ioctl */
	    regs.h.al = 0;	/* get device info */
	    regs.x.bx = fileno(gpoutfile);
	    intdos(&regs, &regs);
	    regs.h.dl |= 0x20;	/* binary (no ^Z intervention) */
	    regs.h.dh = 0;
	    regs.h.ah = 0x44;	/* ioctl */
	    regs.h.al = 1;	/* set device info */
	    intdos(&regs, &regs);
	}
#   endif			/* !_Windows */
#  endif			/* TURBOC && MSDOS */
    }
}

# endif				/* MSDOS || g || MTOS || ... */
#endif /* 0 */

#ifdef VMS
/* these are needed to modify terminal characteristics */
# ifndef VWS_XMAX
   /* avoid duplicate warning; VWS includes these */
#  include <descrip.h>
#  include <ssdef.h>
# endif /* !VWS_MAX */
# include <iodef.h>
# include <ttdef.h>
# include <tt2def.h>
# include <dcdef.h>
# include <stat.h>
# include <fab.h>
/* If you use WATCOM C or a very strict ANSI compiler, you may have to 
 * delete or comment out the following 3 lines: */
# ifndef TT2$M_DECCRT3		/* VT300 not defined as of VAXC v2.4 */
#  define TT2$M_DECCRT3 0X80000000
# endif
static unsigned short chan;
static int old_char_buf[3], cur_char_buf[3];
$DESCRIPTOR(sysoutput_desc, "SYS$OUTPUT");

char *vms_init()
/*
 *  Look first for decw$display (decterms do regis)
 *  Determine if we have a regis terminal
 * and save terminal characteristics
 */
{
    /* Save terminal characteristics in old_char_buf and
       initialise cur_char_buf to current settings. */
    int i;
    if (getenv("DECW$DISPLAY"))
	return ("x11");
    atexit(vms_reset);
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    sys$qiow(0, chan, IO$_SENSEMODE, 0, 0, 0, old_char_buf, 12, 0, 0, 0, 0);
    for (i = 0; i < 3; ++i)
	cur_char_buf[i] = old_char_buf[i];
    sys$dassgn(chan);

    /* Test if terminal is regis */
    if ((cur_char_buf[2] & TT2$M_REGIS) == TT2$M_REGIS)
	return ("regis");
    return (NULL);
}

void vms_reset()
/* set terminal to original state */
{
    int i;
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, old_char_buf, 12, 0, 0, 0, 0);
    for (i = 0; i < 3; ++i)
	cur_char_buf[i] = old_char_buf[i];
    sys$dassgn(chan);
}

void term_mode_tek()
/* set terminal mode to tektronix */
{
    long status;
    if (gpoutfile != stdout)
	return;			/* don't modify if not stdout */
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    cur_char_buf[0] = 0x004A0000 | DC$_TERM | (TT$_TEK401X << 8);
    cur_char_buf[1] = (cur_char_buf[1] & 0x00FFFFFF) | 0x18000000;

    cur_char_buf[1] &= ~TT$M_CRFILL;
    cur_char_buf[1] &= ~TT$M_ESCAPE;
    cur_char_buf[1] &= ~TT$M_HALFDUP;
    cur_char_buf[1] &= ~TT$M_LFFILL;
    cur_char_buf[1] &= ~TT$M_MECHFORM;
    cur_char_buf[1] &= ~TT$M_NOBRDCST;
    cur_char_buf[1] &= ~TT$M_NOECHO;
    cur_char_buf[1] &= ~TT$M_READSYNC;
    cur_char_buf[1] &= ~TT$M_REMOTE;
    cur_char_buf[1] |= TT$M_LOWER;
    cur_char_buf[1] |= TT$M_TTSYNC;
    cur_char_buf[1] |= TT$M_WRAP;
    cur_char_buf[1] &= ~TT$M_EIGHTBIT;
    cur_char_buf[1] &= ~TT$M_MECHTAB;
    cur_char_buf[1] &= ~TT$M_SCOPE;
    cur_char_buf[1] |= TT$M_HOSTSYNC;

    cur_char_buf[2] &= ~TT2$M_APP_KEYPAD;
    cur_char_buf[2] &= ~TT2$M_BLOCK;
    cur_char_buf[2] &= ~TT2$M_DECCRT3;
    cur_char_buf[2] &= ~TT2$M_LOCALECHO;
    cur_char_buf[2] &= ~TT2$M_PASTHRU;
    cur_char_buf[2] &= ~TT2$M_REGIS;
    cur_char_buf[2] &= ~TT2$M_SIXEL;
    cur_char_buf[2] |= TT2$M_BRDCSTMBX;
    cur_char_buf[2] |= TT2$M_EDITING;
    cur_char_buf[2] |= TT2$M_INSERT;
    cur_char_buf[2] |= TT2$M_PRINTER;
    cur_char_buf[2] &= ~TT2$M_ANSICRT;
    cur_char_buf[2] &= ~TT2$M_AVO;
    cur_char_buf[2] &= ~TT2$M_DECCRT;
    cur_char_buf[2] &= ~TT2$M_DECCRT2;
    cur_char_buf[2] &= ~TT2$M_DRCS;
    cur_char_buf[2] &= ~TT2$M_EDIT;
    cur_char_buf[2] |= TT2$M_FALLBACK;

    status = sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
    if (status == SS$_BADPARAM) {
	/* terminal fallback utility not installed on system */
	cur_char_buf[2] &= ~TT2$M_FALLBACK;
	sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
    } else {
	if (status != SS$_NORMAL)
	    lib$signal(status, 0, 0);
    }
    sys$dassgn(chan);
}

void term_mode_native()
/* set terminal mode back to native */
{
    int i;
    if (gpoutfile != stdout)
	return;			/* don't modify if not stdout */
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, old_char_buf, 12, 0, 0, 0, 0);
    for (i = 0; i < 3; ++i)
	cur_char_buf[i] = old_char_buf[i];
    sys$dassgn(chan);
}

void term_pasthru()
/* set terminal mode pasthru */
{
    if (gpoutfile != stdout)
	return;			/* don't modify if not stdout */
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    cur_char_buf[2] |= TT2$M_PASTHRU;
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
    sys$dassgn(chan);
}

void term_nopasthru()
/* set terminal mode nopasthru */
{
    if (gpoutfile != stdout)
	return;			/* don't modify if not stdout */
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    cur_char_buf[2] &= ~TT2$M_PASTHRU;
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
    sys$dassgn(chan);
}

void fflush_binary()
{
    typedef short int INT16;	/* signed 16-bit integers */
    register INT16 k;		/* loop index */
    if (gpoutfile != stdout) {
	/* Stupid VMS fflush() raises error and loses last data block
	   unless it is full for a fixed-length record binary file.
	   Pad it here with NULL characters. */
	for (k = (INT16) ((*gpoutfile)->_cnt); k > 0; --k)
	    putc('\0', gpoutfile);
	fflush(gpoutfile);
    }
}
#endif /* VMS */
