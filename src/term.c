#ifndef lint
static char *RCSid() { return RCSid("$Id: term.c,v 1.184.2.3 2009/07/05 00:10:34 sfeam Exp $"); }
#endif

/* GNUPLOT - term.c */

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
  * term_initialise()  : optional. Prepare the terminal for first
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
#include "getcolor.h"

#ifdef USE_MOUSE
#include "mouse.h"
#endif

#ifdef _Windows
FILE *open_printer __PROTO((void));     /* in wprinter.c */
void close_printer __PROTO((FILE * outfile));
# ifdef __MSC__
#  include <malloc.h>
#  include <io.h>
# else
#  include <alloc.h>
# endif                         /* MSC */
#endif /* _Windows */

static int termcomp __PROTO((const generic * a, const generic * b));

/* Externally visible variables */
/* the central instance: the current terminal's interface structure */
struct termentry *term = NULL;  /* unknown */

/* ... and its options string */
char term_options[MAX_LINE_LEN+1] = "";

/* the 'output' file name and handle */
char *outstr = NULL;            /* means "STDOUT" */
FILE *gpoutfile;

/* Output file where the PostScript output goes to. See term_api.h for more
   details.
*/
FILE *gppsfile = 0;

/* true if terminal has been initialized */
TBOOLEAN term_initialised;

/* true if in multiplot mode */
TBOOLEAN multiplot = FALSE;

/* text output encoding, for terminals that support it */
enum set_encoding_id encoding;
/* table of encoding names, for output of the setting */
const char *encoding_names[] = {
    "default", "iso_8859_1", "iso_8859_2", "iso_8859_9", "iso_8859_15",
    "cp437", "cp850", "cp852", "cp1250", "cp1254", "koi8r", "koi8u", 
    "utf8", NULL };
/* 'set encoding' options */
const struct gen_table set_encoding_tbl[] =
{
    { "def$ault", S_ENC_DEFAULT },
    { "utf$8", S_ENC_UTF8 },
    { "iso$_8859_1", S_ENC_ISO8859_1 },
    { "iso_8859_2", S_ENC_ISO8859_2 },
    { "iso_8859_9", S_ENC_ISO8859_9 },
    { "iso_8859_15", S_ENC_ISO8859_15 },
    { "cp4$37", S_ENC_CP437 },
    { "cp850", S_ENC_CP850 },
    { "cp852", S_ENC_CP852 },
    { "cp1250", S_ENC_CP1250 },
    { "cp1254", S_ENC_CP1254 },
    { "koi8$r", S_ENC_KOI8_R },
    { "koi8$u", S_ENC_KOI8_U },
    { NULL, S_ENC_INVALID }
};

const char *arrow_head_names[4] = 
    {"nohead", "head", "backhead", "heads"};

/* HBB 20020225: moved here, from ipc.h, where it never should have
 * been. */
#ifdef PIPE_IPC
/* HBB 20020225: currently not used anywhere outside term.c --> make
 * it static */
static SELECT_TYPE_ARG1 ipc_back_fd = IPC_BACK_CLOSED;
#endif

/* resolution in dpi for converting pixels to size units */
int gp_resolution = 72;

/* Support for enhanced text mode. Declared extern in term_api.h */
char  enhanced_text[MAX_LINE_LEN+1] = "";
char *enhanced_cur_text = NULL;
double enhanced_fontscale = 1.0;
char enhanced_escape_format[16] = "";
double enhanced_max_height = 0.0, enhanced_min_height = 0.0;
/* flag variable to disable enhanced output of filenames, mainly. */
TBOOLEAN ignore_enhanced_text = FALSE;

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
static struct termentry *change_term __PROTO((const char *name, int length));

static void null_linewidth __PROTO((double));
static void do_point __PROTO((unsigned int x, unsigned int y, int number));
static void do_pointsize __PROTO((double size));
static void line_and_point __PROTO((unsigned int x, unsigned int y, int number));
static void do_arrow __PROTO((unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, int head));

static void UP_redirect __PROTO((int called));

static int null_text_angle __PROTO((int ang));
static int null_justify_text __PROTO((enum JUSTIFY just));
static int null_scale __PROTO((double x, double y));
static void options_null __PROTO((void));
static void UNKNOWN_null __PROTO((void));
static void MOVE_null __PROTO((unsigned int, unsigned int));
static void LINETYPE_null __PROTO((int));
static void PUTTEXT_null __PROTO((unsigned int, unsigned int, const char *));

/* Used by terminals and by shared routine parse_term_size() */
typedef enum {
    PIXELS,
    INCHES,
    CM
} size_units;
static size_units parse_term_size __PROTO((float *xsize, float *ysize, size_units def_units));

/*
 * Bookkeeping and support routine for 'set multiplot layout <rows>, <cols>'
 * July 2004
 * Volker Dobler <v.dobler@web.de>
 */

static void mp_layout_size_and_offset __PROTO((void));

enum set_multiplot_id {
    S_MULTIPLOT_LAYOUT,
    S_MULTIPLOT_COLUMNSFIRST, S_MULTIPLOT_ROWSFIRST, S_MULTIPLOT_SCALE,
    S_MULTIPLOT_DOWNWARDS, S_MULTIPLOT_UPWARDS,
    S_MULTIPLOT_OFFSET, S_MULTIPLOT_TITLE, S_MULTIPLOT_INVALID
};

static const struct gen_table set_multiplot_tbl[] =
{
    { "lay$out", S_MULTIPLOT_LAYOUT },
    { "col$umnsfirst", S_MULTIPLOT_COLUMNSFIRST },
    { "row$sfirst", S_MULTIPLOT_ROWSFIRST },
    { "down$wards", S_MULTIPLOT_DOWNWARDS },
    { "up$wards", S_MULTIPLOT_UPWARDS },
    { "sca$le", S_MULTIPLOT_SCALE },
    { "off$set", S_MULTIPLOT_OFFSET },
    { "ti$tle", S_MULTIPLOT_TITLE },
    { NULL, S_MULTIPLOT_INVALID }
};

# define MP_LAYOUT_DEFAULT {          \
    FALSE,	/* auto_layout */         \
    0, 0,	/* num_rows, num_cols */  \
    FALSE,	/* row_major */           \
    TRUE,	/* downwards */           \
    0, 0,	/* act_row, act_col */    \
    1, 1,	/* xscale, yscale */      \
    0, 0,	/* xoffset, yoffset */    \
    0,0,0,0,	/* prev_ sizes and offsets */ \
    EMPTY_LABELSTRUCT, 0.0 \
}

static struct {
    TBOOLEAN auto_layout;  /* automatic layout if true */
    int num_rows;          /* number of rows in layout */
    int num_cols;          /* number of columns in layout */
    TBOOLEAN row_major;    /* row major mode if true, column major else */
    TBOOLEAN downwards;    /* prefer downwards or upwards direction */
    int act_row;           /* actual row in layout */
    int act_col;           /* actual column in layout */
    double xscale;         /* factor for horizontal scaling */
    double yscale;         /* factor for vertical scaling */
    double xoffset;        /* horizontal shift */
    double yoffset;        /* horizontal shift */
    double prev_xsize, prev_ysize, prev_xoffset, prev_yoffset;
			   /* values before 'set multiplot layout' */
    text_label title;    /* goes above complete set of plots */
    double title_height;   /* fractional height reserved for title */
} mp_layout = MP_LAYOUT_DEFAULT;


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

#if defined(__WATCOMC__) || defined(__MSC__)
# include <io.h>        /* for setmode() */
#endif

/* This is needed because the unixplot library only writes to stdout,
 * but GNU plotutils libplot.a doesn't */
#if defined(UNIXPLOT) && !defined(GNUGRAPH)
static FILE save_stdout;
#endif
static int unixplot = 0;

#define NICE_LINE               0
#define POINT_TYPES             6

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

    if (!outstr)                /* ie using stdout */
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
    if (gpoutfile != gppsfile)
	fclose(gpoutfile);

    gpoutfile = stdout;         /* Don't dup... */
    free(outstr);
    outstr = NULL;

    if (gppsfile)
	fclose(gppsfile);
    gppsfile = NULL;
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
term_set_output(char *dest)
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
	/* switch off output to special postscript file (if used) */
	gppsfile = NULL;
    }
    if (dest == NULL) {         /* stdout */
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
	    close_printer(gpoutfile);   /* close printer immediately if open */
	    gpoutfile = stdout; /* and reset output to stdout */
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
term_initialise()
{
    FPRINTF((stderr, "term_initialise()\n"));

    if (!term)
	int_error(NO_CARET, "No terminal defined");

    /* check if we have opened the output file in the wrong mode
     * (text/binary), if set term comes after set output
     * This was originally done in change_term, but that
     * resulted in output files being truncated
     */

    if (outstr && (term->flags & TERM_NO_OUTPUTFILE)) {
	if (interactive)
	    fprintf(stderr,"Closing %s\n",outstr);
	term_close_output();
    }

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
            FPRINTF((stderr, "term_initialise: reopening \"%s\" as %s\n",
		     outstr, term->flags & TERM_BINARY ? "binary" : "text"));
	    strcpy(temp, outstr);
	    term_set_output(temp);      /* will free outstr */
	    if (temp != outstr) {
		if (temp)
		    free(temp);
		temp = outstr;
	    }
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
        term_initialise();

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

    /* Sync point for epslatex text positioning */
    if (term->layer)
	(term->layer)(TERM_LAYER_RESET);

    /* Because PostScript plots may be viewed out of order, make sure */
    /* Each new plot makes no assumption about the previous palette.  */
    if (term->flags & TERM_IS_POSTSCRIPT)
	invalidate_palette();

    /* Set canvas size to full range of current terminal coordinates */
	canvas.xleft  = 0;
	canvas.xright = term->xmax - 1;
	canvas.ybot   = 0;
	canvas.ytop   = term->ymax - 1;

}

void
term_end_plot()
{
    FPRINTF((stderr, "term_end_plot()\n"));

    if (!term_initialised)
	return;

    /* Sync point for epslatex text positioning */
    if (term->layer)
	(term->layer)(TERM_LAYER_END_TEXT);
    
    if (!multiplot) {
	FPRINTF((stderr, "- calling term->text()\n"));
	(*term->text) ();
	term_graphics = FALSE;
    } else {
	if (mp_layout.auto_layout) {
	    if (mp_layout.row_major) {
		mp_layout.act_row++;
		if (mp_layout.act_row == mp_layout.num_rows ) {
		    mp_layout.act_row = 0;
		    mp_layout.act_col++;
		    if (mp_layout.act_col == mp_layout.num_cols ) {
			/* int_warn(NO_CARET,"will overplot first plot"); */
			mp_layout.act_col = 0;
		    }
		}
	    } else { /* column-major */
		mp_layout.act_col++;
		if (mp_layout.act_col == mp_layout.num_cols ) {
		    mp_layout.act_col = 0;
		    mp_layout.act_row++;
		    if (mp_layout.act_row == mp_layout.num_rows ) {
			/* int_warn(NO_CARET,"will overplot first plot"); */
			mp_layout.act_col = 0;
		    }
		}
	    }
	    mp_layout_size_and_offset();
	}
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
    FPRINTF((stderr, "term_start_multiplot()\n"));

    c_token++;
    if (multiplot)
	term_end_multiplot();

    term_start_plot();

    mp_layout.auto_layout = FALSE;

    /* Parse options (new in version 4.1 */
    while (!END_OF_COMMAND) {
	char *s;

	if (almost_equals(c_token, "ti$tle")) {
	    c_token++;
	    if ((s = try_to_get_string())) {
		free(mp_layout.title.text);
		mp_layout.title.text = s;
 	    }
 	    continue;
       }

       if (equals(c_token, "font")) {
	    c_token++;
 	    if ((s = try_to_get_string())) {
 		free(mp_layout.title.font);
 		mp_layout.title.font = s;
	    }
	    continue;
	}

	if (almost_equals(c_token, "lay$out")) {
	    if (mp_layout.auto_layout)
		int_error(c_token, "too many layout commands");
	    else
		mp_layout.auto_layout = TRUE;

	    c_token++;
	    if (END_OF_COMMAND) {
		int_error(c_token,"expecting '<num_cols>,<num_rows>'");
	    }
	    
	    /* read row,col */
	    mp_layout.num_rows = int_expression();
	    if (END_OF_COMMAND || !equals(c_token,",") )
		int_error(c_token, "expecting ', <num_cols>'");

	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "expecting <num_cols>");
	    mp_layout.num_cols = int_expression();
	
	    /* remember current values of the plot size */
	    mp_layout.prev_xsize = xsize;
	    mp_layout.prev_ysize = ysize;
	    mp_layout.prev_xoffset = xoffset;
	    mp_layout.prev_yoffset = yoffset;
	
	    mp_layout.act_row = 0;
	    mp_layout.act_col = 0;

	    continue;
	}

	/* The remaining options are only valid for auto-layout mode */
	if (!mp_layout.auto_layout)
	    int_error(c_token, "only valid as part of an auto-layout command");

	switch(lookup_table(&set_multiplot_tbl[0],c_token)) {
	    case S_MULTIPLOT_COLUMNSFIRST:
		mp_layout.row_major = TRUE;
		c_token++;
		break;
	    case S_MULTIPLOT_ROWSFIRST:
		mp_layout.row_major = FALSE;
		c_token++;
		break;
	    case S_MULTIPLOT_DOWNWARDS:
		mp_layout.downwards = TRUE;
		c_token++;
		break;
	    case S_MULTIPLOT_UPWARDS:
		mp_layout.downwards = FALSE;
		c_token++;
		break;
	    case S_MULTIPLOT_SCALE:
		c_token++;
		mp_layout.xscale = real_expression();
		mp_layout.yscale = mp_layout.xscale;
		if (!END_OF_COMMAND && equals(c_token,",") ) {
		    c_token++;
		    if (END_OF_COMMAND) {
			int_error(c_token, "expecting <yscale>");
		    }
		    mp_layout.yscale = real_expression();
		}
		break;
	    case S_MULTIPLOT_OFFSET:
		c_token++;
		mp_layout.xoffset = real_expression();
		mp_layout.yoffset = mp_layout.xoffset;
		if (!END_OF_COMMAND && equals(c_token,",") ) {
		    c_token++;
		    if (END_OF_COMMAND) {
			int_error(c_token, "expecting <yoffset>");
		    }
		    mp_layout.yoffset = real_expression();
		}
		break;
	    default:
		int_error(c_token,"invalid or duplicate option");
		break;
	}
    }

    /* If we reach here, then the command has been successfully parsed */
    multiplot = TRUE;
    fill_gpval_integer("GPVAL_MULTIPLOT", 1);

    /* Place overall title before doing anything else */
    if (mp_layout.title.text) {
	double tmpx, tmpy;
	unsigned int x, y;
	char *p = mp_layout.title.text;

	map_position_r(&(mp_layout.title.offset), &tmpx, &tmpy, "mp title");
	x = term->xmax  / 2 + tmpx;
	y = term->ymax - term->v_char + tmpy;;

	ignore_enhanced(mp_layout.title.noenhanced);
	apply_pm3dcolor(&(mp_layout.title.textcolor), term);
	write_multiline(x, y, mp_layout.title.text,
			CENTRE, JUST_TOP, 0, mp_layout.title.font);
	reset_textcolor(&(mp_layout.title.textcolor), term);
	ignore_enhanced(FALSE);

	/* Calculate fractional height of title compared to entire page */
	/* If it would fill the whole page, forget it! */
	for (y=2; *p; p++)
	    if (*p == '\n')
		y++;
	mp_layout.title_height = (double)(y * term->v_char) / (double)term->ymax;
	if (mp_layout.title_height > 0.9)
	    mp_layout.title_height = 0.05;
    } else {
	mp_layout.title_height = 0.0;
    }
    
    mp_layout_size_and_offset();

#ifdef USE_MOUSE
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
    fill_gpval_integer("GPVAL_MULTIPLOT", 0);
    /* reset plot size and origin to values before 'set multiplot layout' */
    if (mp_layout.auto_layout) {
	xsize = mp_layout.prev_xsize;
	ysize = mp_layout.prev_ysize;
	xoffset = mp_layout.prev_xoffset;
	yoffset = mp_layout.prev_yoffset;
    }
    /* reset automatic multiplot layout */
    mp_layout.auto_layout = FALSE;
    mp_layout.xscale = mp_layout.yscale = 1.0;
    mp_layout.xoffset = mp_layout.yoffset = 0.0;
    if (mp_layout.title.text) {
	free(mp_layout.title.text);
	mp_layout.title.text = NULL;
    }

    term_end_plot();
#ifdef USE_MOUSE
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

#ifdef USE_MOUSE
    /* Make sure that ^C will break out of a wait for 'pause mouse' */
    paused_for_mouse = 0;
#endif

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
	/* switch off output to special postscript file (if used) */
	gppsfile = NULL;
    }
}

void
term_apply_lp_properties(struct lp_style_type *lp)
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
	if (lp->p_size < 0)
	    (*term->pointsize) (pointsize);
	else
	    (*term->pointsize) (lp->p_size);
    }
    /*  _first_ set the line width, _then_ set the line type !

     *  The linetype might depend on the linewidth in some terminals.
     */
    (*term->linewidth) (lp->l_width);

    /* Apply "linetype", which can include both color and dot/dash */
    (*term->linetype) (lp->l_type);
    /* Possibly override the linetype color with a fancier colorspec */
    if (lp->use_palette)
	apply_pm3dcolor(&lp->pm3d_color, term);
}



void
term_check_multiplot_okay(TBOOLEAN f_interactive)
{
    FPRINTF((stderr, "term_multiplot_okay(%d)\n", f_interactive));

    if (!term_initialised)
	return;                 /* they've not started yet */

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
    /* uh oh: they're not allowed to be in multiplot here */

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
write_multiline(
    unsigned int x, unsigned int y,
    char *text,
    JUSTIFY hor,                /* horizontal ... */
    VERT_JUSTIFY vert,          /* ... and vertical just - text in hor direction despite angle */
    int angle,                  /* assume term has already been set for this */
    const char *font)           /* NULL or "" means use default */
{
    struct termentry *t = term;
    char *p = text;

    if (!p)
	return;

    /* EAM 9-Feb-2003 - Set font before calculating sizes */
    if (font && *font && t->set_font)
	(*t->set_font) (font);

    if (vert != JUST_TOP) {
	/* count lines and adjust y */
	int lines = 0;          /* number of linefeeds - one fewer than lines */
	while (*p) {
	    if (*p++ == '\n')
		++lines;
	}
	if (angle)
	    x -= (vert * lines * t->v_char) / 2;
	else
	    y += (vert * lines * t->v_char) / 2;
    }

    for (;;) {                  /* we will explicitly break out */

	if ((text != NULL) && (p = strchr(text, '\n')) != NULL)
	    *p = 0;             /* terminate the string */

	if ((*t->justify_text) (hor)) {
	    if (on_page(x, y))
		(*t->put_text) (x, y, text);
	} else {
	    int fix = hor * t->h_char * estimate_strlen(text) / 2;
	    if (angle) {
		if (on_page(x, y - fix))
		    (*t->put_text) (x, y - fix, text);
	    }
	    else {
		if (on_page(x - fix, y))
		    (*t->put_text) (x - fix, y, text);
	    }
	}
	if (angle == 90 || angle == TEXT_VERTICAL)
	    x += t->v_char;
	else if (angle == -90 || angle == -TEXT_VERTICAL)
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
    }                           /* unconditional branch back to the for(;;) - just a goto ! */

    if (font && *font && t->set_font)
	(*t->set_font) ("");

}


static void
do_point(unsigned int x, unsigned int y, int number)
{
    int htic, vtic;
    struct termentry *t = term;

    if (number < 0) {           /* do dot */
	(*t->move) (x, y);
	(*t->vector) (x, y);
	return;
    }
    number %= POINT_TYPES;
    /* should be in term_tbl[] in later version */
    htic = (term_pointsize * t->h_tic / 2);
    vtic = (term_pointsize * t->v_tic / 2);

    /* point types 1..4 are same as in postscript, png and x11
       point types 5..6 are "similar"
       (note that (number) equals (pointtype-1)
    */
    switch (number) {
    case 4:                     /* do diamond */
	(*t->move) (x - htic, y);
	(*t->vector) (x, y - vtic);
	(*t->vector) (x + htic, y);
	(*t->vector) (x, y + vtic);
	(*t->vector) (x - htic, y);
	(*t->move) (x, y);
	(*t->vector) (x, y);
	break;
    case 0:                     /* do plus */
	(*t->move) (x - htic, y);
	(*t->vector) (x - htic, y);
	(*t->vector) (x + htic, y);
	(*t->move) (x, y - vtic);
	(*t->vector) (x, y - vtic);
	(*t->vector) (x, y + vtic);
	break;
    case 3:                     /* do box */
	(*t->move) (x - htic, y - vtic);
	(*t->vector) (x - htic, y - vtic);
	(*t->vector) (x + htic, y - vtic);
	(*t->vector) (x + htic, y + vtic);
	(*t->vector) (x - htic, y + vtic);
	(*t->vector) (x - htic, y - vtic);
	(*t->move) (x, y);
	(*t->vector) (x, y);
	break;
    case 1:                     /* do X */
	(*t->move) (x - htic, y - vtic);
	(*t->vector) (x - htic, y - vtic);
	(*t->vector) (x + htic, y + vtic);
	(*t->move) (x - htic, y + vtic);
	(*t->vector) (x - htic, y + vtic);
	(*t->vector) (x + htic, y - vtic);
	break;
    case 5:                     /* do triangle */
	(*t->move) (x, y + (4 * vtic / 3));
	(*t->vector) (x - (4 * htic / 3), y - (2 * vtic / 3));
	(*t->vector) (x + (4 * htic / 3), y - (2 * vtic / 3));
	(*t->vector) (x, y + (4 * vtic / 3));
	(*t->move) (x, y);
	(*t->vector) (x, y);
	break;
    case 2:                     /* do star */
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
do_pointsize(double size)
{
    term_pointsize = (size >= 0 ? size : 1);
}


/*
 * general point routine
 */
static void
line_and_point(unsigned int x, unsigned int y, int number)
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

#define COS15 (0.96593)         /* cos of 15 degree */
#define SIN15 (0.25882)         /* sin of 15 degree */

#define HEAD_LONG_LIMIT  (2.0)  /* long  limit of arrowhead length */
#define HEAD_SHORT_LIMIT (0.3)  /* short limit of arrowhead length */
				/* their units are the "tic" length */

#define HEAD_COEFF  (0.3)       /* default value of head/line length ratio */

int curr_arrow_headlength; /* access head length + angle without changing API */
double curr_arrow_headangle;    /* angle in degrees */
double curr_arrow_headbackangle;  /* angle in degrees */
int curr_arrow_headfilled;      /* arrow head filled or not */

static void
do_arrow(
    unsigned int usx, unsigned int usy,   /* start point */
    unsigned int uex, unsigned int uey,   /* end point (point of arrowhead) */
    int headstyle)
{
    /* Clipping and angle calculations do not work if coords are unsigned! */
    int sx = (int)usx;
    int sy = (int)usy;
    int ex = (int)uex;
    int ey = (int)uey;

    struct termentry *t = term;
    float len_tic = ((double) (t->h_tic + t->v_tic)) / 2.0;
    /* average of tic sizes */
    /* (dx,dy) : vector from end to start */
    double dx = sx - ex;
    double dy = sy - ey;
    double len_arrow = sqrt(dx * dx + dy * dy);
    gpiPoint filledhead[5];
    int xm = 0, ym = 0;
    BoundingBox *clip_save;
    t_arrow_head head = (t_arrow_head)((headstyle < 0) ? -headstyle : headstyle);
	/* negative headstyle means draw heads only, no shaft */

    /* FIXME: The plan is to migrate calling routines to call via            */
    /* draw_clip_arrow() in which case we would not need to clip again here. */
    clip_save = clip_area;
    if (term->flags & TERM_CAN_CLIP)
	clip_area = NULL;
    else
	clip_area = &canvas;

    /* Calculate and draw arrow heads.
     * Draw no head for arrows with length = 0, or, to be more specific,
     * length < DBL_EPSILON, because len_arrow will almost always be != 0.
     */
    if ((head != NOHEAD) && fabs(len_arrow) >= DBL_EPSILON) {
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

	if (head & END_HEAD) {
	    if (curr_arrow_headfilled==2 && !clip_point(ex,ey)) {
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
		filledhead->style = FS_OPAQUE;
		if (t->filled_polygon)
		    (*t->filled_polygon) (5, filledhead);
	    }
	    /* draw outline of forward arrow head */
	    if (clip_point(ex,ey))
		;
	    else if (curr_arrow_headfilled!=0) {
		draw_clip_line(ex+xm, ey+ym, ex+x1, ey+y1);
		draw_clip_line(ex+x1, ey+y1, ex, ey);
		draw_clip_line(ex, ey, ex+x2, ey+y2);
		draw_clip_line(ex+x2, ey+y2, ex+xm, ey+ym);
	    } else {
		draw_clip_line(ex+x1, ey+y1, ex, ey);
		draw_clip_line(ex, ey, ex+x2, ey+y2);
	    }
	}

	/* backward arrow head */
	if ((head & BACKHEAD) && !clip_point(sx,sy)) { 
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
		filledhead->style = FS_OPAQUE;
		if (t->filled_polygon)
		    (*t->filled_polygon) (5, filledhead);
	    }
	    /* draw outline of backward arrow head */
	    if (curr_arrow_headfilled!=0) {
		draw_clip_line(sx-xm, sy-ym, sx-x2, sy-y2);
		draw_clip_line(sx-x2, sy-y2, sx, sy);
		draw_clip_line(sx, sy, sx-x1, sy-y1);
		draw_clip_line(sx-x1, sy-y1, sx-xm, sy-ym);
	    } else {
		draw_clip_line(sx-x2, sy-y2, sx, sy);
		draw_clip_line(sx, sy, sx-x1, sy-y1);
	    }
	}
    }

    /* Draw the line for the arrow. */
    if (headstyle >= 0) {
	if ((head & BACKHEAD)
	&&  (fabs(len_arrow) >= DBL_EPSILON) && (curr_arrow_headfilled!=0) ) {
	    sx -= xm;
	    sy -= ym;
	}
	if ((head & END_HEAD)
	&&  (fabs(len_arrow) >= DBL_EPSILON) && (curr_arrow_headfilled!=0) ) {
	    ex += xm;
	    ey += ym;
	}
	if (clip_line(&sx, &sy, &ex, &ey))
	    draw_clip_line(sx, sy, ex, ey);
    }

    /* Restore previous clipping box */
    clip_area = clip_save;
	
}

#ifdef EAM_OBJECTS
/* Generic routine for drawing circles or circular arcs.          */
/* If this feature proves useful, we can add a new terminal entry */
/* point term->arc() to the API and let terminals either provide  */
/* a private implemenation or use this generic one.               */

void
do_arc( 
    unsigned int cx, unsigned int cy, /* Center */
    double radius, /* Radius */
    double arc_start, double arc_end, /* Limits of arc in degress */
    int style)
{
    gpiPoint vertex[120];
    int i, segments;
    double aspect;

    /* Always draw counterclockwise */
    while (arc_end < arc_start)
	arc_end += 360.;

    /* Choose how many segments to draw for this arc */
#   define INC 5.
    segments = (arc_end - arc_start) / INC;

    /* Calculate the vertices */
    aspect = (double)term->v_tic / (double)term->h_tic;
    vertex[0].style = style;
    for (i=0; i<segments; i++) {
	vertex[i].x = cx + cos(DEG2RAD * (arc_start + i*INC)) * radius;
	vertex[i].y = cy + sin(DEG2RAD * (arc_start + i*INC)) * radius * aspect;
    }
#   undef INC
    vertex[segments].x = cx + cos(DEG2RAD * arc_end) * radius;
    vertex[segments].y = cy + sin(DEG2RAD * arc_end) * radius * aspect;
    if (fabs(arc_end - arc_start) > .1 
    &&  fabs(arc_end - arc_start) < 359.9) {
	vertex[++segments].x = cx;
	vertex[segments].y = cy;
	vertex[++segments].x = vertex[0].x;
	vertex[segments].y = vertex[0].y;
    }

    if (style) {
	/* Fill in the center */
	if (term->filled_polygon)
	    term->filled_polygon(segments+1, vertex);
    } else {
	/* Draw the arc */
	for (i=0; i<segments; i++)
	    draw_clip_line( vertex[i].x, vertex[i].y,
		vertex[i+1].x, vertex[i+1].y );
    }
}
#endif /* EAM_OBJECTS */



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
null_text_angle(int ang)
{
    return (ang == 0);
}

/* change justification of text.
 * modes are LEFT (flush left), CENTRE (centred), RIGHT (flush right)
 */
static int
null_justify_text(enum JUSTIFY just)
{
    return (just == LEFT);
}


/* Change scale of plot.
 * Parameters are x,y scaling factors for this plot.
 * Some terminals (eg latex) need to do scaling themselves.
 */
static int
null_scale(double x, double y)
{
    (void) x;                   /* avoid -Wunused warning */
    (void) y;
    return FALSE;               /* can't be done */
}

static void
options_null()
{
    term_options[0] = '\0';     /* we have no options */
}

static void
UNKNOWN_null()
{
}

static void
MOVE_null(unsigned int x, unsigned int y)
{
    (void) x;                   /* avoid -Wunused warning */
    (void) y;
}

static void
LINETYPE_null(int t)
{
    (void) t;                   /* avoid -Wunused warning */
}

static void
PUTTEXT_null(unsigned int x, unsigned int y, const char *s)
{
    (void) s;                   /* avoid -Wunused warning */
    (void) x;
    (void) y;
}


static void
null_linewidth(double s)
{
    (void) s;                   /* avoid -Wunused warning */
}


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

#include "term.h"

};

#define TERMCOUNT (sizeof(term_tbl) / sizeof(term_tbl[0]))
#if 0 /* UNUSED */
/* mainly useful for external code */
int
term_count()
{
    return TERMCOUNT;
}
#endif

void
list_terms()
{
    int i;
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

/* Return string with all terminal names.
   Note: caller must free the returned names after use.
*/
char*
get_terminals_names()
{
    int i;
    char *buf = gp_alloc(TERMCOUNT*15, "all_term_names"); /* max 15 chars per name */
    char *names;
    int sort_idxs[TERMCOUNT];

    /* sort terminal types alphabetically */
    for( i = 0; i < TERMCOUNT; i++ )
	sort_idxs[i] = i;
    qsort( sort_idxs, TERMCOUNT, sizeof(int), termcomp );
    /* now sort_idxs[] contains the sorted indices */

    strcpy(buf, " "); /* let the string have leading and trailing " " in order to search via strstrt(GPVAL_TERMINALS, " png "); */
    for (i = 0; i < TERMCOUNT; i++)
	sprintf(buf+strlen(buf), "%s ", term_tbl[sort_idxs[i]].name);
    names = gp_alloc(strlen(buf)+1, "all_term_names2");
    strcpy(names, buf);
    free(buf);
    return names;
}

static int
termcomp(const generic *arga, const generic *argb)
{
    const int *a = arga;
    const int *b = argb;

    return( strcasecmp( term_tbl[*a].name, term_tbl[*b].name ) );
}

/* set_term: get terminal number from name on command line
 * will change 'term' variable if successful
 */
struct termentry *
set_term()
{
    struct termentry *t = NULL;
    char *input_name = NULL;

    if (!END_OF_COMMAND) {
	input_name = gp_input_line + token[c_token].start_index;
    	t = change_term(input_name, token[c_token].length);
	if (!t && isstringvalue(c_token)) {
	    input_name = try_to_get_string();  /* Cannot fail if isstringvalue succeeded */
	    t = change_term(input_name, strlen(input_name));
	    free(input_name);
	} else {
    	    c_token++;
	}
    }

    if (!t)
	int_error(c_token-1, "unknown or ambiguous terminal type; type just 'set terminal' for a list");

    /* otherwise the type was changed */
    return (t);
}

/* change_term: get terminal number from name and set terminal type
 *
 * returns NULL for unknown or ambiguous, otherwise is terminal
 * driver pointer
 */
static struct termentry *
change_term(const char *origname, int length)
{
    int i;
    struct termentry *t = NULL;
    TBOOLEAN ambiguous = FALSE;

    /* For backwards compatibility only */
    char *name = (char *)origname;
    if (!strncmp(origname,"X11",length)) {
	name = "x11";
	length = 3;
    }

    for (i = 0; i < TERMCOUNT; i++) {
	if (!strncmp(name, term_tbl[i].name, length)) {
	    if (t)
		ambiguous = TRUE;
	    t = term_tbl + i;
	    /* Exact match is always accepted */
	    if (length == strlen(term_tbl[i].name)) {
		ambiguous = FALSE;
		break;
	    }
	}
    }

    if (!t || ambiguous)
	return (NULL);

    /* Success: set terminal type now */

    term = t;
    term_initialised = FALSE;

    if (term->scale != null_scale)
	fputs("Warning: scale interface is not null_scale - may not work with multiplot\n", stderr);

    /* check that optional fields are initialised to something */
    if (term->text_angle == 0)
	term->text_angle = null_text_angle;
    if (term->justify_text == 0)
	term->justify_text = null_justify_text;
    if (term->point == 0)
	term->point = do_point;
    if (term->arrow == 0)
	term->arrow = do_arrow;
    if (term->pointsize == 0)
	term->pointsize = do_pointsize;
    if (term->linewidth == 0)
	term->linewidth = null_linewidth;
    if (term->tscale <= 0)
	term->tscale = 1.0;

    /* Special handling for unixplot term type */
    if (!strncmp("unixplot", term->name, 8)) {
	UP_redirect(2);         /* Redirect actual stdout for unixplots */
    } else if (unixplot) {
	UP_redirect(3);         /* Put stdout back together again. */
    }
    if (interactive)
	fprintf(stderr, "Terminal type set to '%s'\n", term->name);

    /* Invalidate any terminal-specific structures that may be active */
    invalidate_palette();

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
    char *env_term = NULL;      /* from TERM environment var */
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
	env_term = getenv("TERM");      /* try $TERM */
	if (term_name == (char *) NULL
	    && env_term != (char *) NULL && strcmp(env_term, "sun") == 0)
	    term_name = "sun";
#endif /* SUN */

#ifdef WXWIDGETS
	if (term_name == (char *) NULL)
		term_name = "wxt";
#endif

#ifdef _Windows
	/* let the wxWidgets terminal be the default when available */
	if (term_name == (char *) NULL)
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
# endif                         /* APOLLO */
#endif /* GPR    */

#if defined(__APPLE__) && defined(__MACH__) && defined(HAVE_LIBAQUATERM)
	/* Mac OS X with AquaTerm installed */
	term_name = "aqua";
#endif

#ifdef X11
	env_term = getenv("TERM");      /* try $TERM */
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
#endif          /* X11 */
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
	int namelength = strlen(term_name);
	struct udvt_entry *name = add_udv_by_name("GNUTERM");

	Gstring(&name->udv_value, gp_strdup(term_name));
	name->udv_undef = FALSE;

	if (strchr(term_name,' '))
	    namelength = strchr(term_name,' ') - term_name;

	/* Force the terminal to initialize default fonts, etc.	This prevents */
	/* segfaults and other strangeness if you set GNUTERM to "post" or    */
	/* "png" for example. However, calling X11_options() is expensive due */
	/* to the fork+execute of gnuplot_x11 and x11 can tolerate not being  */
	/* initialized until later.                                           */
	/* Note that gp_input_line[] is blank at this point.	              */
	if (change_term(term_name, namelength)) {
	    if (strcmp(term->name,"x11"))
		term->options();
	    return;
	}
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
static void
UP_redirect(int caller)
{
#if defined(UNIXPLOT) && !defined(GNUGRAPH)
    switch (caller) {
    case 1:
	/* Don't save, just replace stdout w/gpoutfile (save was already done). */
	if (unixplot)
	    *(stdout) = *(gpoutfile);   /* Copy FILE structure */
	break;
    case 2:
	if (!unixplot) {
	    fflush(stdout);
	    save_stdout = *(stdout);
	    *(stdout) = *(gpoutfile);   /* Copy FILE structure */
	    unixplot = 1;
	}
	break;
    case 3:
	/* New terminal in use--put stdout back to original. */
	/* closepl(); */ /* This is called by the term. */
	fflush(stdout);
	*(stdout) = save_stdout;        /* Copy FILE structure */
	unixplot = 0;
	break;
    case 4:
	/*  User really wants to go to normal output... */
	if (unixplot) {
	    fflush(stdout);
	    *(stdout) = save_stdout;    /* Copy FILE structure */
	}
	break;
    } /* switch() */
#else /* !UNIXPLOT || GNUGRAPH */
    (void) caller;              /* avoid -Wunused warning */
#endif /* !UNIXPLOT || GNUGRAPH */
}

/* test terminal by drawing border and text */
/* called from command test */
void
test_term()
{
    struct termentry *t = term;
    const char *str;
    int x, y, xl, yl, i;
    unsigned int xmax_t, ymax_t;
    char label[MAX_ID_LEN];
    int key_entry_height;
    int p_width;

    term_start_plot();
    screen_ok = FALSE;
    xmax_t = (unsigned int) (t->xmax * xsize);
    ymax_t = (unsigned int) (t->ymax * ysize);

    p_width = pointsize * t->h_tic;
    key_entry_height = pointsize * t->v_tic * 1.25;
    if (key_entry_height < t->v_char)
	key_entry_height = t->v_char;

    /* Sync point for epslatex text positioning */
    if (term->layer)
	(term->layer)(TERM_LAYER_FRONTTEXT);

    /* border linetype */
    (*t->linewidth) (1.0);
    (*t->linetype) (LT_BLACK);
    (*t->move) (0, 0);
    (*t->vector) (xmax_t - 1, 0);
    (*t->vector) (xmax_t - 1, ymax_t - 1);
    (*t->vector) (0, ymax_t - 1);
    (*t->vector) (0, 0);
    (*t->linetype)(0);
    (void) (*t->justify_text) (LEFT);
    (*t->put_text) (t->h_char * 5, ymax_t - t->v_char * 1.5, "Terminal Test");
#ifdef USE_MOUSE
    if (t->set_ruler) {
	(*t->put_text) (t->h_char * 5, ymax_t - t->v_char * 3, "Mouse and hotkeys are supported, hit: h r m 6");
    }
#endif
    (*t->linetype)(LT_BLACK);
    (*t->linetype) (LT_AXIS);
    (*t->move) (xmax_t / 2, 0);
    (*t->vector) (xmax_t / 2, ymax_t - 1);
    (*t->move) (0, ymax_t / 2);
    (*t->vector) (xmax_t - 1, ymax_t / 2);
    /* test width and height of characters */
    (*t->linetype) (3);
    (*t->move) (xmax_t / 2 - t->h_char * 10, ymax_t / 2 + t->v_char / 2);
    (*t->vector) (xmax_t / 2 + t->h_char * 10, ymax_t / 2 + t->v_char / 2);
    (*t->vector) (xmax_t / 2 + t->h_char * 10, ymax_t / 2 - t->v_char / 2);
    (*t->vector) (xmax_t / 2 - t->h_char * 10, ymax_t / 2 - t->v_char / 2);
    (*t->vector) (xmax_t / 2 - t->h_char * 10, ymax_t / 2 + t->v_char / 2);
    (*t->put_text) (xmax_t / 2 - t->h_char * 10, ymax_t / 2,
		    "12345678901234567890");
    (*t->put_text) (xmax_t / 2 - t->h_char * 10, ymax_t / 2 + t->v_char * 1.4,
		    "test of character width:");
    (*t->linetype) (LT_BLACK);
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
    (*t->linetype)(1);
    str = "rotated ce+ntred text";
    if ((*t->text_angle) (TEXT_VERTICAL)) {
	if ((*t->justify_text) (CENTRE))
	    (*t->put_text) (t->v_char,
			    ymax_t / 2, str);
	else
	    (*t->put_text) (t->v_char,
			    ymax_t / 2 - strlen(str) * t->h_char / 2, str);
	(*t->justify_text) (LEFT);
	str = " rotated by +45 deg";
	(*t->text_angle)(45);
	(*t->put_text)(t->v_char * 3, ymax_t / 2, str);
	(*t->justify_text) (LEFT);
	str = " rotated by -45 deg";
	(*t->text_angle)(-45);
	(*t->put_text)(t->v_char * 2, ymax_t / 2, str);
#ifdef HAVE_GD_PNG
	if (!strcmp(t->name, "png") || !strcmp(t->name, "gif") || !strcmp(t->name, "jpeg")) {
	    (*t->text_angle)(0);
	    str = "this terminal supports text rotation only for truetype fonts";
	    (*t->put_text)(t->v_char * 2 + t->h_char * 4, ymax_t / 2 - t->v_char * 2, str);
	}
#endif
    } else {
	(void) (*t->justify_text) (LEFT);
	(*t->put_text) (t->h_char * 2, ymax_t / 2 - t->v_char * 2, "can't rotate text");
    }
    (void) (*t->justify_text) (LEFT);
    (void) (*t->text_angle) (0);
    (*t->linetype)(LT_BLACK);

    /* test tic size */
    (*t->linetype)(4);
    (*t->move) ((unsigned int) (xmax_t / 2 + t->h_tic * (1 + axis_array[FIRST_X_AXIS].ticscale)), (unsigned int) ymax_t - 1);
    (*t->vector) ((unsigned int) (xmax_t / 2 + t->h_tic * (1 + axis_array[FIRST_X_AXIS].ticscale)),
		  (unsigned int) (ymax_t - axis_array[FIRST_X_AXIS].ticscale * t->v_tic));
    (*t->move) ((unsigned int) (xmax_t / 2), (unsigned int) (ymax_t - t->v_tic * (1 + axis_array[FIRST_X_AXIS].ticscale)));
    (*t->vector) ((unsigned int) (xmax_t / 2 + axis_array[FIRST_X_AXIS].ticscale * t->h_tic),
		  (unsigned int) (ymax_t - t->v_tic * (1 + axis_array[FIRST_X_AXIS].ticscale)));
    /* HBB 19990530: changed this to use right-justification, if possible... */
    str = "show ticscale";
    if ((*t->justify_text) (RIGHT))
	(*t->put_text) ((unsigned int) (xmax_t / 2 - 1* t->h_char),
			(unsigned int) (ymax_t - (t->v_tic * 2 + t->v_char / 2)),
		    str);
    else
	(*t->put_text) ((unsigned int) (xmax_t / 2 - (strlen(str)+1)     * t->h_char),
			(unsigned int) (ymax_t - (t->v_tic * 2 + t->v_char / 2)),
			str);
    (void) (*t->justify_text) (LEFT);
    (*t->linetype)(LT_BLACK);

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
    x = xmax_t * .375;
    y = ymax_t * .250;
    xl = t->h_tic * 7;
    yl = t->v_tic * 7;
    i = curr_arrow_headfilled;
    curr_arrow_headfilled = 0;
    (*t->arrow) (x, y, x + xl, y, END_HEAD);
    curr_arrow_headfilled = 1;
    (*t->arrow) (x, y, x - xl, y, END_HEAD);
    curr_arrow_headfilled = 2;
    (*t->arrow) (x, y, x, y + yl, END_HEAD);
    curr_arrow_headfilled = 1;		/* Was 3, but no terminals implement it */
    (*t->arrow) (x, y, x, y - yl, END_HEAD);
    curr_arrow_headfilled = i;
    xl = t->h_tic * 5;
    yl = t->v_tic * 5;
    (*t->arrow) (x - xl, y - yl, x + xl, y + yl, END_HEAD | BACKHEAD);
    (*t->arrow) (x - xl, y + yl, x, y, NOHEAD);
    curr_arrow_headfilled = 1;		/* Was 3, but no terminals implement it */
    (*t->arrow) (x, y, x + xl, y - yl, BACKHEAD);

    /* test line widths */
    (void) (*t->justify_text) (LEFT);
    xl = xmax_t / 10;
    yl = ymax_t / 25;
    x = xmax_t * .075;
    y = yl;

    for (i=1; i<7; i++) {
	(*t->linewidth) ((float)(i)); (*t->linetype)(LT_BLACK);
	(*t->move) (x, y); (*t->vector) (x+xl, y);
	sprintf(label,"  lw %1d%c",i,0);
	(*t->put_text) (x+xl, y, label);
	y += yl;
    }
    (*t->put_text) (x, y, "linewidth");

    /* test fill patterns */
    x = xmax_t * 0.5;
    y = 0;
    xl = xmax_t / 40;
    yl = ymax_t / 8;
    (*t->linewidth) ((float)(1));
    (*t->linetype)(LT_BLACK);
    (*t->justify_text) (CENTRE);
    (*t->put_text)(x+xl*7, yl+t->v_char*1.5, "pattern fill");
    for (i=0; i<10; i++) {
	int style = ((i<<4) + FS_PATTERN);
	if (t->fillbox)
	    (*t->fillbox) ( style, x, y, xl, yl );
	(*t->move)  (x,y);
	(*t->vector)(x,y+yl);
	(*t->vector)(x+xl,y+yl);
	(*t->vector)(x+xl,y);
	(*t->vector)(x,y);
	sprintf(label,"%2d",i);
	(*t->put_text)(x+xl/2, y+yl+t->v_char*0.5, label);
	x += xl * 1.5;
    }

    {
	int cen_x = (int)(0.75 * xmax_t);
	int cen_y = (int)(0.83 * ymax_t);
	int radius = xmax_t / 20;

	(*t->linetype)(2);
	/* test pm3d -- filled_polygon(), but not set_color() */
	if (t->filled_polygon) {
#define NUMBER_OF_VERTICES 6
	    int n = NUMBER_OF_VERTICES;
	    gpiPoint corners[NUMBER_OF_VERTICES+1];
#undef  NUMBER_OF_VERTICES
	    int i;

	    for (i = 0; i < n; i++) {
		corners[i].x = cen_x + radius * cos(2*M_PI*i/n);
		corners[i].y = cen_y + radius * sin(2*M_PI*i/n);
	    }
	    corners[n].x = corners[0].x;
	    corners[n].y = corners[0].y;
	    corners->style = FS_OPAQUE;
	    term->filled_polygon(n+1, corners);
	    str = "(color) filled polygon:";
	} else
	    str = "filled polygons not supported";
	(*t->linetype)(LT_BLACK);
	i = ((*t->justify_text) (CENTRE)) ? 0 : t->h_char * strlen(str) / 2;
	(*t->put_text) (cen_x + i, cen_y + radius + t->v_char * 0.5, str);
	(*t->linetype)(LT_BLACK);
    }

    term_end_plot();
}

#if 0
# if defined(MSDOS)||defined(g)||defined(OS2)||defined(_Windows)||defined(DOS386)

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
	    regs.h.ah = 0x44;   /* ioctl */
	    regs.h.al = 0;      /* get device info */
	    regs.x.bx = fileno(gpoutfile);
	    intdos(&regs, &regs);
	    regs.h.dl |= 0x20;  /* binary (no ^Z intervention) */
	    regs.h.dh = 0;
	    regs.h.ah = 0x44;   /* ioctl */
	    regs.h.al = 1;      /* set device info */
	    intdos(&regs, &regs);
	}
#   endif /* !_Windows */
#  endif /* TURBOC && MSDOS */
    }
}

# endif /* MSDOS || g || ... */
#endif /* 0 */

#ifdef VMS
/* these are needed to modify terminal characteristics */
# ifndef VWS_XMAX
   /* avoid duplicate warning; VWS includes these */
#  include <descrip.h>
#  include <ssdef.h>
# endif                         /* !VWS_MAX */
# include <iodef.h>
# include <ttdef.h>
# include <tt2def.h>
# include <dcdef.h>
# include <stat.h>
# include <fab.h>
/* If you use WATCOM C or a very strict ANSI compiler, you may have to
 * delete or comment out the following 3 lines: */
# ifndef TT2$M_DECCRT3          /* VT300 not defined as of VAXC v2.4 */
#  define TT2$M_DECCRT3 0X80000000
# endif
static unsigned short chan;
static int old_char_buf[3], cur_char_buf[3];
$DESCRIPTOR(sysoutput_desc, "SYS$OUTPUT");

/* Look first for decw$display (decterms do regis).  Determine if we
 * have a regis terminal and save terminal characteristics */
char *
vms_init()
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

/* set terminal to original state */
void
vms_reset()
{
    int i;

    sys$assign(&sysoutput_desc, &chan, 0, 0);
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, old_char_buf, 12, 0, 0, 0, 0);
    for (i = 0; i < 3; ++i)
	cur_char_buf[i] = old_char_buf[i];
    sys$dassgn(chan);
}

/* set terminal mode to tektronix */
void
term_mode_tek()
{
    long status;

    if (gpoutfile != stdout)
	return;                 /* don't modify if not stdout */
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

/* set terminal mode back to native */
void
term_mode_native()
{
    int i;

    if (gpoutfile != stdout)
	return;                 /* don't modify if not stdout */
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, old_char_buf, 12, 0, 0, 0, 0);
    for (i = 0; i < 3; ++i)
	cur_char_buf[i] = old_char_buf[i];
    sys$dassgn(chan);
}

/* set terminal mode pasthru */
void
term_pasthru()
{
    if (gpoutfile != stdout)
	return;                 /* don't modify if not stdout */
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    cur_char_buf[2] |= TT2$M_PASTHRU;
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
    sys$dassgn(chan);
}

/* set terminal mode nopasthru */
void
term_nopasthru()
{
    if (gpoutfile != stdout)
	return;                 /* don't modify if not stdout */
    sys$assign(&sysoutput_desc, &chan, 0, 0);
    cur_char_buf[2] &= ~TT2$M_PASTHRU;
    sys$qiow(0, chan, IO$_SETMODE, 0, 0, 0, cur_char_buf, 12, 0, 0, 0, 0);
    sys$dassgn(chan);
}

void
fflush_binary()
{
    typedef short int INT16;    /* signed 16-bit integers */
    INT16 k;            /* loop index */

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

/*
 * This is an abstraction of the enhanced text mode originally written
 * for the postscript terminal driver by David Denholm and Matt Heffron.
 * I have split out a terminal-independent recursive syntax-parser
 * routine that can be shared by all drivers that want to add support
 * for enhanced text mode.
 *
 * A driver that wants to make use of this common framework must provide
 * three new entries in TERM_TABLE:
 *      void *enhanced_open   (char *fontname, double fontsize, double base,
 *                             TBOOLEAN widthflag, TBOOLEAN showflag,
 *                             int overprint)
 *      void *enhanced_writec (char c)
 *      void *enhanced_flush  ()
 *
 * Each driver also has a separate ENHXX_put_text() routine that replaces
 * the normal (term->put_text) routine while in enhanced mode.
 * This routine must initialize the following globals used by the shared code:
 *      enhanced_fontscale      converts font size to device resolution units
 *      enhanced_escape_format  used to process octal escape characters \xyz
 *
 * I bent over backwards to make the output of the revised code identical
 * to the output of the original postscript version.  That means there is
 * some cruft left in here (enhanced_max_height for one thing) that is
 * probably irrelevant to any new drivers using the code.
 *
 * Ethan A Merritt - November 2003
 */

#ifdef DEBUG_ENH
#define ENH_DEBUG(x) printf x;
#else
#define ENH_DEBUG(x)
#endif

void
do_enh_writec(int c)
{
    /* note: c is meant to hold a char, but is actually an int, for
     * the same reasons applying to putc() and friends */
    *enhanced_cur_text++ = c;
}

/*
 * Process a bit of string, and return the last character used.
 * p is start of string
 * brace is TRUE to keep processing to }, FALSE to do one character only
 * fontname & fontsize are obvious
 * base is the current baseline
 * widthflag is TRUE if the width of this should count,
 *              FALSE for zero width boxes
 * showflag is TRUE if this should be shown,
 *             FALSE if it should not be shown (like TeX \phantom)
 * overprint is 0 for normal operation,
 *              1 for the underprinted text (included in width calculation),
 *              2 for the overprinted text (not included in width calc)
 *              (overprinted text is centered horizontally on underprinted text
 */

char *
enhanced_recursion(
    char *p,
    TBOOLEAN brace,
    char *fontname,
    double fontsize,
    double base,
    TBOOLEAN widthflag,
    TBOOLEAN showflag,
    int overprint)
{
    ENH_DEBUG(("RECURSE WITH [%p] \"%s\", %d %s %.1f %.1f %d %d %d\n", p, p, brace, fontname, fontsize, base, widthflag, showflag, overprint));

    /* Start each recursion with a clean string */
    (term->enhanced_flush)();

    if (base + fontsize > enhanced_max_height) {
	enhanced_max_height = base + fontsize;
	ENH_DEBUG(("Setting max height to %.1f\n", enhanced_max_height));
    }

    if (base < enhanced_min_height) {
	enhanced_min_height = base;
	ENH_DEBUG(("Setting min height to %.1f\n", enhanced_min_height));
    }

    while (*p) {
	float shift;

	/*
	 * EAM Jun 2009 - treating bytes one at a time does not work for multibyte
	 * encodings, including utf-8. If we hit a byte with the high bit set, test
	 * whether it starts a legal UTF-8 sequence and if so copy the whole thing.  
	 * Other multibyte encodings are still a problem.
	 * Gnuplot's other defined encodings are all single-byte; for those we
	 * really do want to treat one byte at a time.
	 */
	if ((*p & 0x80) && (encoding == S_ENC_DEFAULT || encoding == S_ENC_UTF8)) {
	    unsigned long utf8char;
	    const char *nextchar = p;

	    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
	    if (utf8toulong(&utf8char, &nextchar)) {	/* Legal UTF8 sequence */
		while (p < nextchar)
		    (term->enhanced_writec)(*p++);
		p--;
	    } else {					/* Some other multibyte encoding? */
		(term->enhanced_writec)(*p);
	    }
	} else

	switch (*p) {
	case '}'  :
	    /*{{{  deal with it*/
	    if (brace)
		return (p);

	    fputs("enhanced text parser - spurious }\n", stderr);
	    break;
	    /*}}}*/

	case '_'  :
	case '^'  :
	    /*{{{  deal with super/sub script*/
	    shift = (*p == '^') ? 0.5 : -0.3;
	    (term->enhanced_flush)();
	    p = enhanced_recursion(p + 1, FALSE, fontname, fontsize * 0.8,
			      base + shift * fontsize, widthflag,
			      showflag, overprint);
	    break;
	    /*}}}*/
	case '{'  :
	    {
		char *savepos = NULL, save = 0;
		char *localfontname = fontname, ch;
		float f = fontsize, ovp;

		/*{{{  recurse (possibly with a new font) */

		ENH_DEBUG(("Dealing with {\n"));

		/* get vertical offset (if present) for overprinted text */
		while (*++p == ' ');
		if (overprint == 2) {
		    ovp = (float)strtod(p,&p);
		    if (term->flags & TERM_IS_POSTSCRIPT)
			base = ovp*f;
		    else
			base += ovp*f;
		}
		--p;            /* HBB 20001021: bug fix: 10^{2} broken */

		if (*++p == '/') {
		    /* then parse a fontname, optional fontsize */
		    while (*++p == ' ')
			;       /* do nothing */
		    if (*p=='-') {
			while (*++p == ' ')
			    ;   /* do nothing */
		    }
		    localfontname = p;
		    while ((ch = *p) > ' ' && ch != '=' && ch != '*' && ch != '}')
			++p;
		    save = *(savepos=p);
		    if (ch == '=') {
			*p++ = '\0';
			/*{{{  get optional font size*/
			ENH_DEBUG(("Calling strtod(\"%s\") ...", p));
			f = (float)strtod(p, &p);
			ENH_DEBUG(("Returned %.1f and \"%s\"\n", f, p));

			if (f == 0)
			    f = fontsize;
			else
			    f *= enhanced_fontscale;  /* remember the scaling */

			ENH_DEBUG(("Font size %.1f\n", f));
			/*}}}*/
		    } else if (ch == '*') {
			*p++ = '\0';
			/*{{{  get optional font size scale factor*/
			ENH_DEBUG(("Calling strtod(\"%s\") ...", p));
			f = (float)strtod(p, &p);
			ENH_DEBUG(("Returned %.1f and \"%s\"\n", f, p));

			if (f)
			    f *= fontsize;  /* apply the scale factor */
			else
			    f = fontsize;

			ENH_DEBUG(("Font size %.1f\n", f));
			/*}}}*/
		    } else if (ch == '}') {
			int_warn(NO_CARET,"bad syntax in enhanced text string");
			*p++ = '\0';
		    } else {
			*p++ = '\0';
			f = fontsize;
		    }

		    while (*p == ' ')
			++p;
		    if (! *localfontname)
			localfontname = fontname;
		}
		/*}}}*/

		ENH_DEBUG(("Before recursing, we are at [%p] \"%s\"\n", p, p));

		p = enhanced_recursion(p, TRUE, localfontname, f, base,
				  widthflag, showflag, overprint);

		ENH_DEBUG(("BACK WITH \"%s\"\n", p));

		(term->enhanced_flush)();

		if (savepos)
		    /* restore overwritten character */
		    *savepos = save;
		break;
	    } /* case '{' */
	case '@' :
	    /*{{{  phantom box - prints next 'char', then restores currentpoint */
	    (term->enhanced_flush)();
	    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, 3);
	    p = enhanced_recursion(++p, FALSE, fontname, fontsize, base,
			      widthflag, showflag, overprint);
	    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, 4);
	    break;
	    /*}}}*/

	case '&' :
	    /*{{{  character skip - skips space equal to length of character(s) */
	    (term->enhanced_flush)();

	    p = enhanced_recursion(++p, FALSE, fontname, fontsize, base,
			      widthflag, FALSE, overprint);
	    break;
	    /*}}}*/

	case '~' :
	    /*{{{ overprinted text */
	    /* the second string is overwritten on the first, centered
	     * horizontally on the first and (optionally) vertically
	     * shifted by an amount specified (as a fraction of the
	     * current fontsize) at the beginning of the second string

	     * Note that in this implementation neither the under- nor
	     * overprinted string can contain syntax that would result
	     * in additional recursions -- no subscripts,
	     * superscripts, or anything else, with the exception of a
	     * font definition at the beginning of the text */

	    (term->enhanced_flush)();
	    p = enhanced_recursion(++p, FALSE, fontname, fontsize, base,
			      widthflag, showflag, 1);
	    (term->enhanced_flush)();
	    p = enhanced_recursion(++p, FALSE, fontname, fontsize, base,
			      FALSE, showflag, 2);

	    overprint = 0;   /* may not be necessary, but just in case . . . */
	    break;
	    /*}}}*/

	case '('  :
	case ')'  :
	    /*{{{  an escape and print it */
	    /* special cases */
	    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
	    if (term->flags & TERM_IS_POSTSCRIPT)
		(term->enhanced_writec)('\\');
	    (term->enhanced_writec)(*p);
	    break;
	    /*}}}*/

	case '\\'  :
	    if (p[1]=='\\' || p[1]=='(' || p[1]==')') {
		(term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
		(term->enhanced_writec)('\\');

	    /*{{{  The enhanced mode always uses \xyz as an octal character representation
		   but each terminal type must give us the actual output format wanted.
		   pdf.trm wants the raw character code, which is why we use strtol();
		   most other terminal types want some variant of "\\%o". */
	    } else if (p[1] >= '0' && p[1] <= '7') {
		char *e, escape[16], octal[4] = {'\0','\0','\0','\0'};

		(term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
		octal[0] = *(++p);
		if (p[1] >= '0' && p[1] <= '7') {
		    octal[1] = *(++p);
		    if (p[1] >= '0' && p[1] <= '7')
			octal[2] = *(++p);
		}
		sprintf(escape, enhanced_escape_format, strtol(octal,NULL,8));
		for (e=escape; *e; e++) {
		    (term->enhanced_writec)(*e);
		}
		break;
	    } else if (term->flags & TERM_IS_POSTSCRIPT) {
		/* Shigeharu TAKENO  Aug 2004 - Needed in order for shift-JIS */
		/* encoding to work. If this change causes problems then we   */
		/* need a separate flag for shift-JIS and certain other 8-bit */
		/* character sets.                                            */
		/* EAM Nov 2004 - Nevertheless we must allow \ to act as an   */
		/* escape for a few enhanced mode formatting characters even  */
		/* though it corrupts certain Shift-JIS character sequences.  */
		if (strchr("^_@&~{}",p[1]) == NULL) {
		    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
		    (term->enhanced_writec)('\\');
		    (term->enhanced_writec)('\\');
		    break;
		}
	    }
	    ++p;

	    /* HBB 20030122: Avoid broken output if there's a \
	     * exactly at the end of the line */
	    if (*p == '\0') {
		fputs("enhanced text parser -- spurious backslash\n", stderr);
		break;
	    }

	    /* SVG requires an escaped '&' to be passed as something else */
	    /* FIXME: terminal-dependent code does not belong here */
	    if (*p == '&' && encoding == S_ENC_DEFAULT && !strcmp(term->name, "svg")) {
		(term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
		(term->enhanced_writec)('\376');
		break;
	    }

	    /* just go and print it (fall into the 'default' case) */
	    /*}}}*/
	default:
	    /*{{{  print it */
	    (term->enhanced_open)(fontname, fontsize, base, widthflag, showflag, overprint);
	    (term->enhanced_writec)(*p);
	    /*}}}*/
	} /* switch (*p) */

	/* like TeX, we only do one character in a recursion, unless it's
	 * in braces
	 */

	if (!brace) {
	    (term->enhanced_flush)();
	    return(p);  /* the ++p in the outer copy will increment us */
	}

	if (*p) /* only not true if { not terminated, I think */
	    ++p;
    } /* while (*p) */

    (term->enhanced_flush)();
    return p;
}

/* Called after the end of recursion to check for errors */
void
enh_err_check(const char *str)
{
    if (*str == '}')
	fputs("enhanced text mode parser - ignoring spurious }\n", stderr);
    else
	fprintf(stderr, "enhanced text mode parsing error - *str=0x%x\n",
		*str);
}

/* Helper function for multiplot auto layout to issue size and offest cmds */
static void
mp_layout_size_and_offset(void)
{
    if (!mp_layout.auto_layout) return;

    /* fprintf(stderr,"col==%d row==%d\n",mp_layout.act_col,mp_layout.act_row); */
    /* the 'set size' command */
    xsize = mp_layout.xscale / mp_layout.num_cols;
    ysize = mp_layout.yscale / mp_layout.num_rows;

    /* the 'set origin' command */
    xoffset = (double)(mp_layout.act_col) / mp_layout.num_cols;
    if (mp_layout.downwards)
	yoffset = 1.0 - (double)(mp_layout.act_row+1) / mp_layout.num_rows;
    else
	yoffset = (double)(mp_layout.act_row) / mp_layout.num_rows;
    /* fprintf(stderr,"xoffset==%g  yoffset==%g\n", xoffset,yoffset); */

    /* Allow a little space at the top for a title */
    if (mp_layout.title.text) {
	ysize *= (1.0 - mp_layout.title_height);
	yoffset *= (1.0 - mp_layout.title_height);
    }

    /* corrected for x/y-scaling factors and user defined offsets */
    xoffset -= (mp_layout.xscale-1)/(2*mp_layout.num_cols);
    yoffset -= (mp_layout.yscale-1)/(2*mp_layout.num_rows);
    /* fprintf(stderr,"  xoffset==%g  yoffset==%g\n", xoffset,yoffset); */
    xoffset += mp_layout.xoffset;
    yoffset += mp_layout.yoffset;
    /* fprintf(stderr,"  xoffset==%g  yoffset==%g\n", xoffset,yoffset); */
}

/*
 * Text strings containing control information for enhanced text mode
 * contain more characters than will actually appear in the output.
 * This makes it hard to estimate how much horizontal space on the plot
 * (e.g. in the key box) must be reserved to hold them.  To approximate
 * the eventual length we switch briefly to the dummy terminal driver
 * "estimate.trm" and then switch back to the current terminal.
 * If better, perhaps terminal-specific methods of estimation are
 * developed later they can be slotted into this one call site.
 */
int
estimate_strlen(char *text)
{
int len;

#ifdef GP_ENH_EST
    if (strchr(text,'\n') || (term->flags & TERM_ENHANCED_TEXT)) {
	struct termentry *tsave = term;
	term = &ENHest;
	term->put_text(0,0,text);
	len = term->xmax;
	FPRINTF((stderr,"Estimating length %d height %g for enhanced text string \"%s\"\n",
		len, (double)(term->ymax)/10., text));
	term = tsave;
    } else
#endif
	len = strlen(text);

    return len;
}

void
ignore_enhanced(TBOOLEAN flag)
{
    /* Force a return to the default font */
    if (flag && !ignore_enhanced_text) {
	ignore_enhanced_text = TRUE;
	if (term->set_font)
	    term->set_font("");
    }
    ignore_enhanced_text = flag;
}

/* Simple-minded test for whether the point (x,y) is in bounds for the current terminal.
 * Some terminals can do their own clipping, and can clip partial objects.
 * If the flag TERM_CAN_CLIP is set, we skip this relative crude test and let the
 * driver or the hardware handle clipping.
 */
TBOOLEAN
on_page(int x, int y)
{
    if (term->flags & TERM_CAN_CLIP)
	return TRUE;

    if ((0 < x && x < term->xmax) && (0 < y && y < term->ymax))
	return TRUE;

    return FALSE;
}

/* Utility routine for drivers to accept an explicit size for the 
 * output image.
 */
size_units
parse_term_size( float *xsize, float *ysize, size_units default_units )
{
    size_units units = default_units;

    if (END_OF_COMMAND)
	int_error(c_token, "size requires two numbers:  xsize, ysize");
    *xsize = real_expression();
    if (almost_equals(c_token,"in$ches")) {
	c_token++;
	units = INCHES;
    } else if (equals(c_token,"cm")) {
	c_token++;
	units = CM;
    }
    switch (units) {
    case INCHES:	*xsize *= gp_resolution; break;
    case CM:		*xsize *= (float)gp_resolution / 2.54; break;
    case PIXELS:
    default:		 break;
    }

    if (!equals(c_token++,","))
	int_error(c_token, "size requires two numbers:  xsize, ysize");
    *ysize = real_expression();
    if (almost_equals(c_token,"in$ches")) {
	c_token++;
	units = INCHES;
    } else if (equals(c_token,"cm")) {
	c_token++;
	units = CM;
    }
    switch (units) {
    case INCHES:	*ysize *= gp_resolution; break;
    case CM:		*ysize *= (float)gp_resolution / 2.54; break;
    case PIXELS:
    default:		 break;
    }

    if (*xsize < 1 || *ysize < 1)
	int_error(c_token, "size: out of range");

    return units;
}

/*
 * Wrappers for newpath and closepath
 */

void
newpath()
{
    if (term->path)
	(*term->path)(0);
}

void
closepath()
{
    if (term->path)
	(*term->path)(1);
}

/* Squeeze all fill information into the old style parameter.
 * The terminal drivers know how to extract the information.
 * We assume that the style (int) has only 16 bit, therefore we take
 * 4 bits for the style and allow 12 bits for the corresponding fill parameter.
 * This limits the number of styles to 16 and the fill parameter's
 * values to the range 0...4095, which seems acceptable.
 */
int
style_from_fill(struct fill_style_type *fs)
{
    int fillpar, style;

    switch( fs->fillstyle ) {
    case FS_SOLID:
    case FS_TRANSPARENT_SOLID:
	fillpar = fs->filldensity;
	style = ((fillpar & 0xfff) << 4) + fs->fillstyle;
	break;
    case FS_PATTERN:
    case FS_TRANSPARENT_PATTERN:
	fillpar = fs->fillpattern;
	style = ((fillpar & 0xfff) << 4) + fs->fillstyle;
	break;
    case FS_EMPTY:
    default:
	/* solid fill with background color */
	style = FS_EMPTY;
	break;
    }

    return style;
}


void
lp_use_properties(struct lp_style_type *lp, int tag)
{
    /*  This function looks for a linestyle defined by 'tag' and copies
     *  its data into the structure 'lp'.
     */

    struct linestyle_def *this;
    int save_pointflag = lp->pointflag;

    this = first_linestyle;
    while (this != NULL) {
	if (this->tag == tag) {
	    *lp = this->lp_properties;
	    lp->pointflag = save_pointflag;
	    /* FIXME - It would be nicer if this were always true already */
	    if (!lp->use_palette) {
		lp->pm3d_color.type = TC_LT;
		lp->pm3d_color.lt = lp->l_type;
	    }
	    return;
	} else {
	    this = this->next;
	}
    }

    /* No user-defined style with this tag; fall back to default line type. */
    /* NB: We assume that the remaining fields of lp have been initialized. */
    lp->l_type = tag - 1;
    lp->pm3d_color.type = TC_LT;
    lp->pm3d_color.lt = lp->l_type;
    lp->p_type = tag - 1;
}

