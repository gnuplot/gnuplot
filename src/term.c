#ifndef lint
static char *RCSid() { return RCSid("$Id: term.c,v 1.51 2003/02/18 16:38:11 broeker Exp $"); }
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

#include "term_api.h"

#include "alloc.h"
#include "axis.h"
#include "bitmap.h"
#include "command.h"
#include "driver.h"
#include "graphics.h"
#include "help.h"
#include "plot.h"
#include "tables.h"
#include "term.h"
#include "util.h"
#include "version.h"
#include "misc.h"

#ifdef USE_MOUSE
#include "mouse.h"
static int save_mouse_state = 1;
#endif

#ifdef _Windows
FILE *open_printer __PROTO((void));	/* in wprinter.c */
void close_printer __PROTO((FILE * outfile));
# ifdef __MSC__
#  include <malloc.h>
# else
#  include <alloc.h>
# endif				/* MSC */
#endif /* _Windows */

enum { UNSET = -1, no = 0, yes = 1 }; /* FIXME HBB 20001031: should this be here? */

static int termcomp __PROTO((const generic * a, const generic * b));

/* Externally visible variables */
/* the central instance: the current terminal's interface structure */
struct termentry *term = NULL;	/* unknown */

/* ... and its options string */
char term_options[MAX_LINE_LEN+1] = "";

/* the 'output' file name and handle */
char *outstr = NULL;		/* means "STDOUT" */
FILE *gpoutfile;

#ifdef PM3D
/* Output file where the PostScript output goes to. See term_api.h for more
   details.
*/
FILE *postscript_gpoutfile = 0;
#endif

/* true if terminal has been initialized */
TBOOLEAN term_initialised;

/* true if in multiplot mode */
TBOOLEAN multiplot = FALSE;

/* flag variable to disable enhanced output of filenames, mainly */
TBOOLEAN ignore_enhanced_text = FALSE;

/* text output encoding, for terminals that support it */
enum set_encoding_id encoding;
/* table of encoding names, for output of the setting */
const char *encoding_names[] = {
    "default", "iso_8859_1", "iso_8859_2", "cp437", "cp850", "cp852", NULL };
/* 'set encoding' options */
const struct gen_table set_encoding_tbl[] =
{
    { "def$ault", S_ENC_DEFAULT },
    { "iso$_8859_1", S_ENC_ISO8859_1 },
    { "iso_8859_2", S_ENC_ISO8859_2 },
    { "cp4$37", S_ENC_CP437 },
    { "cp8$50", S_ENC_CP850 },
    { "cp8$52", S_ENC_CP852 },
    { NULL, S_ENC_INVALID }
};

/* HBB 20020225: moved here, from ipc.h, where it never should have
 * been. */
#ifdef PIPE_IPC
/* HBB 20020225: currently not used anywhere outside term.c --> make
 * it static */
static int ipc_back_fd = IPC_BACK_CLOSED;
int isatty_state = 1;
#endif

/* Internal variables */

/* true if terminal is in graphics mode */
static TBOOLEAN term_graphics = FALSE;

/* we have suspended the driver, in multiplot mode */
static TBOOLEAN term_suspended = FALSE;

/* true if? */
static TBOOLEAN opened_binary = FALSE;

/* true if require terminal to be initialized */
static TBOOLEAN term_force_init = FALSE;

/* internal pointsize for do_point */
static double term_pointsize=1;

/* Internal prototypes: */

static void term_suspend __PROTO((void));
static void term_close_output __PROTO((void));
static void null_linewidth __PROTO((double));

static void do_point __PROTO((unsigned int x, unsigned int y, int number));
static void do_pointsize __PROTO((double size));
static void line_and_point __PROTO((unsigned int x, unsigned int y, int number));
static void do_arrow __PROTO((unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, TBOOLEAN head));

static void UP_redirect __PROTO((int called));

static int null_text_angle __PROTO((int ang));
static int null_justify_text __PROTO((enum JUSTIFY just));
static int null_scale __PROTO((double x, double y));
static void options_null __PROTO((void));
static void UNKNOWN_null __PROTO((void));
static void MOVE_null __PROTO((unsigned int, unsigned int));
static void LINETYPE_null __PROTO((int));
static void PUTTEXT_null __PROTO((unsigned int, unsigned int, const char *));
static int set_font_null __PROTO((const char *s));


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

#if defined(MSDOS) || defined(WIN32) || defined(WIN16)
# if defined(__DJGPP__) || defined (__TURBOC__)
#  include <io.h>
# endif
# include <fcntl.h>
# ifndef O_BINARY
#  ifdef _O_BINARY
#   define O_BINARY _O_BINARY
#  else
#   define O_BINARY O_BINARY_is_not_defined
#  endif
# endif
#endif

#ifdef __EMX__
#include <io.h>
#include <fcntl.h>
#endif

/* This is needed because the unixplot library only writes to stdout,
 * but GNU plotutils libplot.a doesn't */
#if defined(UNIXPLOT) && !defined(GNUGRAPH)
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
static TBOOLEAN output_pipe_open = FALSE;
#endif /* PIPES */

static void
term_close_output()
{
    FPRINTF((stderr, "term_close_output\n"));

    opened_binary = FALSE;

    if (!outstr)		/* ie using stdout */
	return;

#if defined(PIPES)
    if (output_pipe_open) {
	(void) pclose(gpoutfile);
	output_pipe_open = FALSE;
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
void
term_set_output(dest)
char *dest;
{
    FILE *f = NULL;

    FPRINTF((stderr, "term_set_output\n"));
    assert(dest == NULL || dest != outstr);

    if (multiplot) {
	fputs("In multiplotmode you can't change the output\n", stderr);
	return;
    }
    if (term && term_initialised) {
	(*term->reset) ();
	term_initialised = FALSE;
#ifdef PM3D
	/* switch off output to special postscript file (if used) */
	postscript_gpoutfile = 0;
#endif
    }
    if (dest == NULL) {		/* stdout */
	UP_redirect(4);
	term_close_output();
    } else {

#if defined(PIPES)
	if (*dest == '|') {
	    if ((f = popen(dest + 1, POPEN_MODE)) == (FILE *) NULL)
		os_error(c_token, "cannot create pipe; output not changed");
	    else
		output_pipe_open = TRUE;
	} else {
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
		os_error(c_token, "cannot open printer temporary file; output may have changed");
	} else
#endif

	{
#if defined (MSDOS)
	    if (outstr && (0 == stricmp(outstr, dest))) {
		/* On MSDOS, you cannot open the same file twice and
		 * then close the first-opened one and keep the second
		 * open, it seems. If you do, you get lost clusters
		 * (connection to the first version of the file is
		 * lost, it seems). */
		/* FIXME: this is not yet safe enough. You can fool it by
		 * specifying the same output file in two different ways
		 * (relative vs. absolute path to file, e.g.) */
		term_close_output();
	    }
#endif
            if (term && (term->flags & TERM_BINARY))
		f = FOPEN_BINARY(dest);
            else
		f = fopen(dest, "w");

	    if (f == (FILE *) NULL)
		os_error(c_token, "cannot open file; output not changed");
	}
#if defined(PIPES)
	}
#endif
	
	term_close_output();
	gpoutfile = f;
	outstr = dest;
	opened_binary = (term && (term->flags & TERM_BINARY));
	UP_redirect(1);
    }
}

void
term_init()
{
    FPRINTF((stderr, "term_init()\n"));

    if (!term)
	int_error(NO_CARET, "No terminal defined");

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
#if defined(MSDOS) || defined (_Windows) || defined(OS2)
# ifdef _Windows
    else if (!outstr && (term->flags & TERM_BINARY))
# else
    else if (!outstr && !interactive && (term->flags & TERM_BINARY))
# endif
	{
	    /* binary to stdout in non-interactive session... */
	    fflush(stdout);
	    setmode(fileno(stdout), O_BINARY);
	}
#endif


    if (!term_initialised || term_force_init) {
	FPRINTF((stderr, "- calling term->init()\n"));
	(*term->init) ();
	term_initialised = TRUE;
    }
}


void
term_start_plot()
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

void
term_end_plot()
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

#ifdef USE_MOUSE
    recalc_statusline();
    update_ruler();
#endif
}

void
term_start_multiplot()
{

    c_token++;
    FPRINTF((stderr, "term_start_multiplot()\n"));
    if (multiplot)
	term_end_multiplot();

    multiplot = TRUE;
    term_start_plot();

#ifdef USE_MOUSE
    /* save the state of mouse_setting.on and
     * disable mouse; call UpdateStatusline()
     * to turn of eventually statusline */
    save_mouse_state = mouse_setting.on;
    mouse_setting.on = 0;
    UpdateStatusline();
#endif
}

void
term_end_multiplot()
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
#ifdef USE_MOUSE
    /* restore the state of mouse_setting.on;
     * call UpdateStatusline() to turn on
     * eventually statusline */
    mouse_setting.on = save_mouse_state;
    UpdateStatusline();
#endif
}



static void
term_suspend()
{
    FPRINTF((stderr, "term_suspend()\n"));
    if (term_initialised && !term_suspended && term->suspend) {
	FPRINTF((stderr, "- calling term->suspend()\n"));
	(*term->suspend) ();
	term_suspended = TRUE;
    }
}

void
term_reset()
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
#ifdef PM3D
	/* switch off output to special postscript file (if used) */
	postscript_gpoutfile = 0;
#endif
    }
}

void
term_apply_lp_properties(lp)
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



void
term_check_multiplot_okay(f_interactive)
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
	int_error(NO_CARET, "This terminal does not support multiplot");
    else
	int_error(NO_CARET, "Must set output to a file or put all multiplot commands on one input line");
}


void
write_multiline(x, y, text, hor, vert, angle, font)
    unsigned int x, y;
    char *text;
    JUSTIFY hor;		/* horizontal ... */
    VERT_JUSTIFY vert;		/* ... and vertical just - text in hor direction despite angle */
    int angle;			/* assume term has already been set for this */
    const char *font;		/* NULL or "" means use default */
{
    register struct termentry *t = term;
    char *p = text;

    if (!p)
	return;

    /* EAM 9-Feb-2003 - Set font before calculating sizes */
    if (font && *font)
	(*t->set_font) (font);

    if (vert != JUST_TOP) {
	/* count lines and adjust y */
	int lines = 0;		/* number of linefeeds - one fewer than lines */
	while (*p++) {
	    if (*p == '\n')
		++lines;
	}
	if (angle)
	    x -= (vert * lines * t->v_char) / 2;
	else
	    y += (vert * lines * t->v_char) / 2;
    }

    for (;;) {			/* we will explicitly break out */

	if ((text != NULL) && (p = strchr(text, '\n')) != NULL)
	    *p = 0;		/* terminate the string */

	if ((*t->justify_text) (hor)) {
	    (*t->put_text) (x, y, text);
	} else {
	    int fix = hor * (t->h_char) * strlen(text) / 2;
	    if (angle)
		(*t->put_text) (x, y - fix, text);
	    else
		(*t->put_text) (x - fix, y, text);
	}
	if (angle == TEXT_VERTICAL)
	    x += t->v_char;
	else if (-angle == TEXT_VERTICAL)
	    x -= t->v_char;
	else
	    y -= t->v_char;

	if (!p)
	    break;
	else {
	    /* put it back */
	    *p = '\n';
	}

	text = p + 1;
    }				/* unconditional branch back to the for(;;) - just a goto ! */

    if (font && *font)
	(*t->set_font) ("");

}


static void
do_point(x, y, number)
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

static void
do_pointsize(size)
double size;
{
    term_pointsize = (size >= 0 ? size : 1);
}


/*
 * general point routine
 */
static void
line_and_point(x, y, number)
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

int curr_arrow_headlength;    /* access head length + angle without changing API */
double curr_arrow_headangle;  /* angle in degrees */
double curr_arrow_headbackangle;  /* angle in degrees */
TBOOLEAN curr_arrow_headfilled; /* arrow head filled or not */

static void
do_arrow(sx, sy, ex, ey, head)
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
#ifdef PM3D
    gpiPoint filledhead[5];
#endif
    int xm = 0, ym = 0;

    /* Calculate and draw arrow heads.
     * Draw no head for arrows with length = 0, or, to be more specific,
     * length < DBL_EPSILON, because len_arrow will almost always be != 0.
     */
    if (head && fabs(len_arrow) >= DBL_EPSILON) {
	int x1, y1, x2, y2;
	if (curr_arrow_headlength <= 0) {
	    /* arrow head with the default size */
	    /* now calc the head_coeff */
	    double coeff_shortest = len_tic * HEAD_SHORT_LIMIT / len_arrow;
	    double coeff_longest = len_tic * HEAD_LONG_LIMIT / len_arrow;
	    double head_coeff = GPMAX(coeff_shortest,
				      GPMIN(HEAD_COEFF, coeff_longest));
	    /* we put the arrowhead marks at 15 degrees to line */
	    x1 = (int) ((COS15 * dx - SIN15 * dy) * head_coeff);
	    y1 = (int) ((SIN15 * dx + COS15 * dy) * head_coeff);
	    x2 = (int) ((COS15 * dx + SIN15 * dy) * head_coeff);
	    y2 = (int) ((-SIN15 * dx + COS15 * dy) * head_coeff);
	    /* backangle defaults to 90 deg */
	    xm = (int) ((x1 + x2)/2);
	    ym = (int) ((y1 + y2)/2);
	} else {
	    /* the arrow head with the length + angle specified explicitly */
	    double alpha = curr_arrow_headangle * DEG2RAD;
	    double beta = curr_arrow_headbackangle * DEG2RAD;
	    double phi = atan2(-dy,-dx); /* azimuthal angle of the vector */
	    double backlen = curr_arrow_headlength * sin(alpha) / sin(beta);
	    double dx2, dy2;
	    /* anticlock-wise head segment */
	    x1 = -(int)(curr_arrow_headlength * cos( alpha - phi ));
	    y1 =  (int)(curr_arrow_headlength * sin( alpha - phi ));
	    /* clock-wise head segment */
	    dx2 = -curr_arrow_headlength * cos( phi + alpha );
	    dy2 = -curr_arrow_headlength * sin( phi + alpha );
	    x2 = (int) (dx2);
	    y2 = (int) (dy2);
	    /* back point */
	    xm = (int) (dx2 + backlen * cos( phi + beta ));
	    ym = (int) (dy2 + backlen * sin( phi + beta ));
	}
#ifdef PM3D
	if (curr_arrow_headfilled==2) {
	    /* draw filled forward arrow head */
	    filledhead[0].x = ex + xm;
	    filledhead[0].y = ey + ym;
	    filledhead[1].x = ex + x1;
	    filledhead[1].y = ey + y1;
	    filledhead[2].x = ex;
	    filledhead[2].y = ey;
	    filledhead[3].x = ex + x2;
	    filledhead[3].y = ey + y2;
	    filledhead[4].x = ex + xm;
	    filledhead[4].y = ey + ym;
	    (*t->filled_polygon) (5, filledhead);
	}
#endif
	/* draw outline of forward arrow head */
	if (curr_arrow_headfilled!=0) {
	    (*t->move) (ex + xm, ey + ym);
	    (*t->vector) (ex + x1, ey + y1);
	    (*t->vector) (ex, ey);
	    (*t->vector) (ex + x2, ey + y2);
	    (*t->vector) (ex + xm, ey + ym);
	} else {
	    (*t->move) (ex + x1, ey + y1);
	    (*t->vector) (ex, ey);
	    (*t->vector) (ex + x2, ey + y2);
	}
	if (head == 2) { /* backward arrow head */
#ifdef PM3D
	    if (curr_arrow_headfilled==2) {
		/* draw filled backward arrow head */
		filledhead[0].x = sx - xm;
		filledhead[0].y = sy - ym;
		filledhead[1].x = sx - x1;
		filledhead[1].y = sy - y1;
		filledhead[2].x = sx;
		filledhead[2].y = sy;
		filledhead[3].x = sx - x2;
		filledhead[3].y = sy - y2;
		filledhead[4].x = sx - xm;
		filledhead[4].y = sy - ym;
		(*t->filled_polygon) (5, filledhead);
	    }
#endif
	    /* draw outline of backward arrow head */
	    if (curr_arrow_headfilled!=0) {
		(*t->move) ( sx - xm, sy - ym);
		(*t->vector) ( sx - x2, sy - y2);
		(*t->vector) (sx, sy);
		(*t->vector) (sx - x1, sy - y1);
		(*t->vector) ( sx - xm, sy - ym);
	    } else {
		(*t->move) ( sx - x2, sy - y2);
		(*t->vector) (sx, sy);
		(*t->vector) (sx - x1, sy - y1);
	    }
	}
    }
    /* Draw the line for the arrow. */
    if ( (head == 2) && 
	 (fabs(len_arrow) >= DBL_EPSILON) && (curr_arrow_headfilled!=0) )
	(*t->move) (sx - xm, sy - ym);
    else
	(*t->move) (sx, sy);
    if ( head && 
	 (fabs(len_arrow) >= DBL_EPSILON) && (curr_arrow_headfilled!=0) )
	(*t->vector) (ex + xm, ey + ym);
    else
	(*t->vector) (ex, ey);
}

#if 0				/* oiginal routine */
#define ROOT2 (1.41421)		/* sqrt of 2 */

void
org_do_arrow(sx, sy, ex, ey, head)
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
static int
null_text_angle(ang)
int ang;
{
    return (ang == 0);
}

/* change justification of text.  
 * modes are LEFT (flush left), CENTRE (centred), RIGHT (flush right)
 */
static int
null_justify_text(just)
enum JUSTIFY just;
{
    return (just == LEFT);
}


/* Change scale of plot.
 * Parameters are x,y scaling factors for this plot.
 * Some terminals (eg latex) need to do scaling themselves.
 */
static int
null_scale(x, y)
    double x, y;
{
    (void) x;			/* avoid -Wunused warning */
    (void) y;
    return FALSE;		/* can't be done */
}

/* HBB 990829: unused --> commented out */
#if 0
int
do_scale(x, y)
double x;
double y;
{
    return TRUE;		/* can be done */
}
#endif /* commented out */

static void
options_null()
{
    term_options[0] = '\0';	/* we have no options */
}

static void
UNKNOWN_null()
{
}

static void
MOVE_null(x, y)
    unsigned int x, y;
{
    (void) x;			/* avoid -Wunused warning */
    (void) y;
}

static void
LINETYPE_null(t)
    int t;
{
    (void) t;			/* avoid -Wunused warning */
}

static void
PUTTEXT_null(x, y, s)
    unsigned int x, y;
    const char *s;
{
    (void) s;			/* avoid -Wunused warning */
    (void) x;
    (void) y;
}


static int
set_font_null(s)
    const char *s;
{
    (void) s;			/* avoid -Wunused warning */
    return FALSE;
}

static void
null_linewidth(s)
    double s;
{
    (void) s;			/* avoid -Wunused warning */
}


/* cast to get rid of useless warnings about UNKNOWN_null */
/* FIXME HBB 20010527: not used anywhere! */
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
static struct termentry term_tbl[] =
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

#define TERMCOUNT (sizeof(term_tbl)/sizeof(term_tbl[0]))

/* mainly useful for external code */
GP_INLINE int
term_count()
{
    return TERMCOUNT;
}

void
list_terms()
{
    register int i;
    char *line_buffer = gp_alloc(BUFSIZ, "list_terms");
    int sort_idxs[TERMCOUNT];

    /* sort terminal types alphabetically */
    for( i = 0; i < TERMCOUNT; i++ )
	sort_idxs[i] = i;
    qsort( sort_idxs, TERMCOUNT, sizeof(int), termcomp );
    /* now sort_idxs[] contains the sorted indices */
    
    StartOutput();
    strcpy(line_buffer, "\nAvailable terminal types:\n");
    OutLine(line_buffer);

    for (i = 0; i < TERMCOUNT; i++) {
	sprintf(line_buffer, "  %15s  %s\n",
		term_tbl[sort_idxs[i]].name,
                term_tbl[sort_idxs[i]].description);
	OutLine(line_buffer);
    }

    EndOutput();
    free(line_buffer);
}

static int
termcomp(arga, argb)
    const generic *arga, *argb;
{
    const int *a = arga;
    const int *b = argb;

    return( strcasecmp( term_tbl[*a].name, term_tbl[*b].name ) );
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
	int_error(c_token_arg, "terminal name expected");
    input_name = input_line + token[c_token_arg].start_index;
    t = change_term(input_name, token[c_token_arg].length);
    if (!t)
	int_error(c_token_arg, "unknown or ambiguous terminal type; type just 'set terminal' for a list");

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
const char *name;
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
void
init_terminal()
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
	    && env_term != (char *) NULL && strcmp(env_term, "next") == 0)
	    term_name = "next";
#endif /* NeXT */

#ifdef __BEOS__
	env_term = getenv("TERM");
	if (term_name == (char *) NULL
	    && env_term != (char *) NULL && strcmp(env_term, "beterm") == 0)
	    term_name = "be";
#endif /* BeOS */

#ifdef SUN
	env_term = getenv("TERM");	/* try $TERM */
	if (term_name == (char *) NULL
	    && env_term != (char *) NULL && strcmp(env_term, "sun") == 0)
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
	    && env_term != (char *) NULL && strcmp(env_term, "xterm") == 0)
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
/* amai: Note that we do some checks above and now overwrite any
   results. Perhaps we may disable checks above!? */
#ifdef X11
/* WINDOWID is set in sessions like xterm, etc.
   DISPLAY is also mandatory. */
	env_term = getenv("WINDOWID");
	display  = getenv("DISPLAY");
	if ((env_term != (char *) NULL) && (display != (char *) NULL))
	    term_name = "x11";
	else
#endif		/* X11 */
	    term_name = "pm";
#endif /*OS2 */

/* set linux terminal only if LINUX_setup was successfull, if we are on X11
   LINUX_setup has failed, also if we are logged in by network */
#ifdef LINUXVGA
	if (LINUX_graphics_allowed)
#ifdef VGAGL
	    term_name = "vgagl";
#else
	    term_name = "linux";
#endif
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
char *
ztc_init()
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


#if defined(UNIXPLOT) && !defined(GNUGRAPH)
static void
UP_redirect(caller)
int caller;
/*
 * Unixplot can't really write to gpoutfile--it wants to write to stdout.
 * This is normally ok, but the original design of gnuplot gives us
 * little choice.  Originally users of unixplot had to anticipate
 * their needs and redirect all I/O to a file...  Not very gnuplot-like.
 *
 * caller:  1 - called from SET OUTPUT "FOO.OUT"
 * 2 - called from SET TERM UNIXPLOT
 * 3 - called from SET TERM other
 * 4 - called from SET OUTPUT
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
	/* closepl(); */ /* This is called by the term. */
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
#else /* !UNIXPLOT || GNUGRAPH */
/*
 * This is always defined so we don't have to have command.c know if it
 * is there or not.
 */
static void
UP_redirect(caller)
int caller;
{
    caller = caller;		/* to stop Turbo C complaining 
				   * about caller not being used */
}
#endif /* !UNIXPLOT || GNUGRAPH */


/* test terminal by drawing border and text */
/* called from command test */
void
test_term()
{
    register struct termentry *t = term;
    const char *str;
    int x, y, xl, yl, i;
    unsigned int xmax_t, ymax_t;
    char label[MAX_ID_LEN];
    int key_entry_height;
    int p_width;

    c_token++;

    term_start_plot();
    screen_ok = FALSE;
    xmax_t = (unsigned int) (t->xmax * xsize);
    ymax_t = (unsigned int) (t->ymax * ysize);

    p_width = pointsize * (t->h_tic);
    key_entry_height = pointsize * (t->v_tic) * 1.25;
    if (key_entry_height < (t->v_char))
	key_entry_height = (t->v_char);

    /* border linetype */
    (*t->linewidth) (1.0);
    (*t->linetype) (LT_BLACK);
    (*t->move) (0, 0);
    (*t->vector) (xmax_t - 1, 0);
    (*t->vector) (xmax_t - 1, ymax_t - 1);
    (*t->vector) (0, ymax_t - 1);
    (*t->vector) (0, 0);
    (void) (*t->justify_text) (LEFT);
    (*t->put_text) (t->h_char * 5, ymax_t - t->v_char * 3, "Terminal Test");
    (*t->linetype) (LT_AXIS);
    (*t->move) (xmax_t / 2, 0);
    (*t->vector) (xmax_t / 2, ymax_t - 1);
    (*t->move) (0, ymax_t / 2);
    (*t->vector) (xmax_t - 1, ymax_t / 2);
    /* test width and height of characters */
    (*t->linetype) (LT_BLACK);
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
    if ((*t->text_angle) (TEXT_VERTICAL)) {
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
    (*t->move) ((unsigned int) (xmax_t / 2 + t->h_tic * (1 + ticscale)), (unsigned) ymax_t);
    (*t->vector) ((unsigned int) (xmax_t / 2 + t->h_tic * (1 + ticscale)),
		  (unsigned int) (ymax_t - ticscale * t->v_tic));
    (*t->move) ((unsigned int) (xmax_t / 2), (unsigned int) (ymax_t - t->v_tic * (1 + ticscale)));
    (*t->vector) ((unsigned int) (xmax_t / 2 + ticscale * t->h_tic),
                  (unsigned int) (ymax_t - t->v_tic * (1 + ticscale)));
    /* HBB 19990530: changed this to use right-justification, if possible... */
    if ((*t->justify_text) (RIGHT))
	(*t->put_text) ((unsigned int) (xmax_t / 2 - 1* t->h_char),
			(unsigned int) (ymax_t - (t->v_tic * 2 + t->v_char / 2)),
		    "test tics");
    else
	(*t->put_text) ((unsigned int) (xmax_t / 2 - 10 * t->h_char),
			(unsigned int) (ymax_t - (t->v_tic * 2 + t->v_char / 2)),
			"test tics");
    (void) (*t->justify_text) (LEFT);

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
    x = xmax_t / 3;
    y = ymax_t / 3;
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

    /* test line widths */
    (void) (*t->justify_text) (LEFT);
    x = xmax_t * .075;
    y = t->v_tic * 5;
    xl = t->h_tic * 15;
   
    for (i=1; i<7; i++) {
	(*t->linewidth) ((float)(i)); (*t->linetype)(LT_BLACK);
	(*t->move) (x, y); (*t->vector) (x+xl, y);
	sprintf(label,"  lw %1d%c",i,0);
	(*t->put_text) (x+xl, y, label);
	y += t->v_tic * 5;
    }
    (*t->put_text) (x, y, "linewidth");

#ifdef USE_ULIG_FILLEDBOXES
    /* test fill patterns */
    x = xmax_t * 0.5;
    y = 0;
    xl = t->h_tic * 3;
    yl = t->v_tic * 12;
    (*t->linewidth) ((float)(1));
    (*t->linetype)(LT_BLACK);
    (*t->justify_text) (CENTRE);
    (*t->put_text)(x+xl*7, yl+t->v_tic*5, "pattern fill");
    for (i=0; i<10; i++) {
	int style = ((i<<4) + FS_PATTERN);
	(*t->fillbox) ( style, x, y, xl, yl );
	(*t->move)  (x,y);
	(*t->vector)(x,y+yl);
	(*t->vector)(x+xl,y+yl);
	(*t->vector)(x+xl,y);
	(*t->vector)(x,y);
	sprintf(label,"%2d",i);
	(*t->put_text)(x+xl/2, y+yl+t->v_tic*2, label);
	x += xl * 1.5;
    }
#endif

    term_end_plot();
}

#if 0
# if defined(MSDOS)||defined(g)||defined(MTOS)||defined(OS2)||defined(_Windows)||defined(DOS386)
/* output for some terminal types must be binary to stop non Unix computers
   changing \n to \r\n. 
   If the output is not STDOUT, the following code reopens gpoutfile 
   with binary mode. */
void
reopen_binary()
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
		os_error(NO_CARET, "cannot reopen file with binary type; output unknown");
	    } else {
		os_error(NO_CARET, "cannot reopen file with binary type; output reset to ascii");
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
# endif				/* !VWS_MAX */
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

char *
vms_init()
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

void
vms_reset()
/* set terminal to original state */
{
    int i;
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, old_char_buf, 12, 0, 0, 0, 0);
    for (i = 0; i < 3; ++i)
	cur_char_buf[i] = old_char_buf[i];
    sys$dassgn(chan);
}

void
term_mode_tek()
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

void
term_mode_native()
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

void
term_pasthru()
/* set terminal mode pasthru */
{
    if (gpoutfile != stdout)
	return;			/* don't modify if not stdout */
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    cur_char_buf[2] |= TT2$M_PASTHRU;
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
    sys$dassgn(chan);
}

void
term_nopasthru()
/* set terminal mode nopasthru */
{
    if (gpoutfile != stdout)
	return;			/* don't modify if not stdout */
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    cur_char_buf[2] &= ~TT2$M_PASTHRU;
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
    sys$dassgn(chan);
}

void
fflush_binary()
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
