#ifndef lint
static char *RCSid() { return RCSid("$Id: show.c,v 1.114 2003/11/13 08:18:16 mikulik Exp $"); }
#endif

/* GNUPLOT - show.c */

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
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 */

#include "setshow.h"

#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "contour.h"
#include "datafile.h"
#include "eval.h"
#include "fit.h"
#include "gp_time.h"
#include "graphics.h"
#include "hidden3d.h"
#include "misc.h"
#include "gp_hist.h"
#include "plot.h" /* for gnuplot_history_size */
#include "plot2d.h"
#include "plot3d.h"
#include "tables.h"
#include "util.h"
#include "term_api.h"
#include "variable.h"
#include "version.h"
#ifdef USE_MOUSE
# include "mouse.h"
#endif

#ifdef PM3D
# include "color.h"
# include "pm3d.h"
# include "getcolor.h"
#endif

#include <ctype.h>

/******** Local functions ********/

static void show_at __PROTO((void));
static void disp_at __PROTO((struct at_type *, int));
static void show_all __PROTO((void));
static void show_autoscale __PROTO((void));
static void show_bars __PROTO((void));
static void show_border __PROTO((void));
static void show_boxwidth __PROTO((void));
#if USE_ULIG_FILLEDBOXES
static void show_fillstyle __PROTO((void));
#endif /* USE_ULIG_FILLEDBOXES */
static void show_clip __PROTO((void));
static void show_contour __PROTO((void));
static void show_dgrid3d __PROTO((void));
static void show_label_contours __PROTO((void));
static void show_mapping __PROTO((void));
static void show_dummy __PROTO((void));
static void show_format __PROTO((void));
static void show_styles __PROTO((const char *name, enum PLOT_STYLE style));
static void show_style __PROTO((void));
static void show_grid __PROTO((void));
static void show_zeroaxis __PROTO((AXIS_INDEX));
static void show_label __PROTO((int tag));
static void show_keytitle __PROTO((void));
static void show_key __PROTO((void));
static void show_logscale __PROTO((void));
static void show_offsets __PROTO((void));
static void show_margin __PROTO((void));
static void show_output __PROTO((void));
static void show_parametric __PROTO((void));
#ifdef PM3D
static void show_pm3d __PROTO((void));
static void show_palette __PROTO((void));
static void show_palette_rgbformulae __PROTO((void));
static void show_palette_fit2rgbformulae __PROTO((void));
static void show_palette_palette __PROTO((void));
static void show_palette_gradient __PROTO((void));
static void show_palette_colornames __PROTO((void));
static void show_colorbox __PROTO((void));
#endif
static void show_pointsize __PROTO((void));
static void show_encoding __PROTO((void));
static void show_decimalsign __PROTO((void));
static void show_fit __PROTO((void));
static void show_polar __PROTO((void));
static void show_print __PROTO((void));
static void show_angles __PROTO((void));
static void show_samples __PROTO((void));
static void show_isosamples __PROTO((void));
static void show_view __PROTO((void));
static void show_surface __PROTO((void));
static void show_hidden3d __PROTO((void));
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
static void show_historysize __PROTO((void));
#endif
static void show_size __PROTO((void));
static void show_origin __PROTO((void));
static void show_term __PROTO((void));
#ifdef PM3D
static void show_tics __PROTO((TBOOLEAN showx, TBOOLEAN showy, TBOOLEAN showz, TBOOLEAN showx2, TBOOLEAN showy2, TBOOLEAN showcb));
#else
static void show_tics __PROTO((TBOOLEAN showx, TBOOLEAN showy, TBOOLEAN showz, TBOOLEAN showx2, TBOOLEAN showy2));
#endif
static void show_mtics __PROTO((AXIS_INDEX));
static void show_timestamp __PROTO((void));
static void show_range __PROTO((AXIS_INDEX axis));
static void show_xyzlabel __PROTO((const char *name, const char *suffix, label_struct * label));
static void show_title __PROTO((void));
static void show_axislabel __PROTO((AXIS_INDEX));
static void show_data_is_timedate __PROTO((AXIS_INDEX));
static void show_timefmt __PROTO((void));
static void show_locale __PROTO((void));
static void show_loadpath __PROTO((void));
static void show_fontpath __PROTO((void));
static void show_zero __PROTO((void));
static void show_datafile __PROTO((void));
#ifdef USE_MOUSE
static void show_mouse __PROTO((void));
#endif
static void show_plot __PROTO((void));
static void show_variables __PROTO((void));

static void show_linestyle __PROTO((int tag));
static void show_arrowstyle __PROTO((int tag));
static void show_arrow __PROTO((int tag));

static void show_ticdef __PROTO((AXIS_INDEX));
static void show_position __PROTO((struct position * pos));
static void show_functions __PROTO((void));

static int var_show_all = 0;


/* following code segment appears over and over again */

#define SHOW_NUM_OR_TIME(x, axis) SAVE_NUM_OR_TIME(stderr, x, axis)

#define SHOW_ALL_NL { if (!var_show_all) (void) putc('\n',stderr); }

/******* The 'show' command *******/
void
show_command()
{
    /* show at is undocumented/hidden... */
    static char GPFAR showmess[] =
    "valid set options:  [] = choose one, {} means optional\n\n\
\t'all', 'angles', 'arrow', 'autoscale', 'bars', 'border', 'boxwidth', 'clip',\n\
\t'cntrparam', 'colorbox', 'contour', 'datafile', 'decimalsign','dgrid3d',\n\
\t'dummy', 'encoding', 'fit', 'fontpath', 'format', 'functions', 'grid',\n\
\t'hidden', 'isosamples', 'key', 'label', 'loadpath', 'locale', 'logscale',\n\
\t'mapping', 'margin', 'offsets', 'origin', 'output', 'plot',\n\
\t'palette', 'parametric', 'pm3d', 'pointsize', 'polar', 'print', '[rtuv]range',\n\
\t'samples', 'size', 'style', 'terminal', 'tics', 'timestamp',\n\
\t'timefmt', 'title', 'variables', 'version', 'view',\n\
\t'[xyz,cb]{2}label', '[xyz,cb]{2}range', '{m}[xyz,cb]{2}tics',\n\
\t'[xyz,cb]{2}[md]tics', '[xyz]{2}zeroaxis', '[xyz,cb]data', 'zero',\n\
\t'zeroaxis'";

    enum set_id token_found; 
    struct value a;
    int tag =0;

    c_token++;

    token_found = lookup_table(&set_tbl[0],c_token);

    /* rationalize c_token advancement stuff a bit: */
    if (token_found != S_INVALID)
	c_token++;

    switch(token_found) {
    case S_ACTIONTABLE:
	show_at();
	break;
    case S_ALL:
	show_all();
	break;
    case S_VERSION:
	show_version(stderr);
	break;
    case S_AUTOSCALE:
	show_autoscale();
	break;
    case S_BARS:
	show_bars();
	break;
    case S_BORDER:
	show_border();
	break;
    case S_BOXWIDTH:
	show_boxwidth();
	break;
    case S_CLIP:
	show_clip();
	break;
    case S_CLABEL:
	/* contour labels are shown with 'show contour' */
    case S_CONTOUR:
    case S_CNTRPARAM:
	show_contour();
	break;
    case S_DGRID3D:
	show_dgrid3d();
	break;
    case S_MAPPING:
	show_mapping();
	break;
    case S_DUMMY:
	show_dummy();
	break;
    case S_FORMAT:
	show_format();
	break;
    case S_FUNCTIONS:
	show_functions();
	break;
    case S_GRID:
	show_grid();
	break;
    case S_XZEROAXIS:
	show_zeroaxis(FIRST_X_AXIS);
	break;
    case S_ZEROAXIS:
	show_zeroaxis(FIRST_X_AXIS);
	/* FALLTHROUGH */
    case S_YZEROAXIS:
	show_zeroaxis(FIRST_Y_AXIS);
	break;
    case S_X2ZEROAXIS:
	show_zeroaxis(SECOND_X_AXIS);
	break;
    case S_Y2ZEROAXIS:
	show_zeroaxis(SECOND_Y_AXIS);
	break;

#define CHECK_TAG_GT_ZERO					\
	if (!END_OF_COMMAND) {					\
	    tag = real(const_express(&a));			\
	    if (tag <= 0)					\
		int_error(c_token, "tag must be > zero");	\
	}							\
	(void) putc('\n',stderr);

    case S_LABEL:
	CHECK_TAG_GT_ZERO;
	show_label(tag);
	break;
    case S_ARROW:
	CHECK_TAG_GT_ZERO;
	show_arrow(tag);
	break;
#ifdef BACKWARDS_COMPATIBLE
    case S_LINESTYLE:
	CHECK_TAG_GT_ZERO;
	show_linestyle(tag);
	break;
#endif
    case S_KEYTITLE:
	show_keytitle();
	break;
    case S_KEY:
	show_key();
	break;
    case S_LOGSCALE:
	show_logscale();
	break;
    case S_OFFSETS:
	show_offsets();
	break;

    case S_LMARGIN:		/* HBB 20010525: handle like 'show margin' */
    case S_RMARGIN:
    case S_TMARGIN:
    case S_BMARGIN:
    case S_MARGIN:
	show_margin();
	break;

    case S_OUTPUT:
	show_output();
	break;
    case S_PARAMETRIC:
	show_parametric();
	break;
#ifdef PM3D
    case S_PM3D:
	show_pm3d();
	break;
    case S_PALETTE:
	show_palette();
	break;
    case S_COLORBOX:
	show_colorbox();
	break;
#endif
    case S_POINTSIZE:
	show_pointsize();
	break;
    case S_DECIMALSIGN:
	show_decimalsign();
	break;
    case S_ENCODING:
	show_encoding();
	break;
    case S_FIT:
	show_fit();
	break;
    case S_FONTPATH:
	show_fontpath();
	break;
    case S_POLAR:
	show_polar();
	break;
    case S_PRINT:
	show_print();
	break;
    case S_ANGLES:
	show_angles();
	break;
    case S_SAMPLES:
	show_samples();
	break;
    case S_ISOSAMPLES:
	show_isosamples();
	break;
    case S_VIEW:
	show_view();
	break;
#ifdef BACKWARDS_COMPATIBLE
    case S_DATA:
	/* HBB 20010525: re-implement old 'show data style' command */
	/* FIXME: 'show function style' is gone completely */
	if (almost_equals(c_token, "st$yle")) {
	    show_styles("Data", data_style);
	    c_token++;
	} else 
	    int_error(c_token, "keyword 'style' expected after 'show data'");
	break;
#endif
    case S_STYLE:
	show_style();
	break;
    case S_SURFACE:
	show_surface();
	break;
    case S_HIDDEN3D:
	show_hidden3d();
	break;
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
    case S_HISTORYSIZE:
	show_historysize();
	break;
#endif
    case S_SIZE:
	show_size();
	break;
    case S_ORIGIN:
	show_origin();
	break;
    case S_TERMINAL:
	show_term();
	break;
    case S_TICS:
    case S_TICSLEVEL:
    case S_TICSCALE:
#ifdef PM3D
	show_tics(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);
#else
	show_tics(TRUE, TRUE, TRUE, TRUE, TRUE);
#endif
	break;
    case S_MXTICS:
	show_mtics(FIRST_X_AXIS);
	break;
    case S_MYTICS:
	show_mtics(FIRST_Y_AXIS);
	break;
    case S_MZTICS:
	show_mtics(FIRST_Z_AXIS);
	break;
#ifdef PM3D
    case S_MCBTICS:
	show_mtics(COLOR_AXIS);
	break;
#endif
    case S_MX2TICS:
	show_mtics(SECOND_X_AXIS);
	break;
    case S_MY2TICS:
	show_mtics(SECOND_Y_AXIS);
	break;
    case S_TIMESTAMP:
	show_timestamp();
	break;
    case S_RRANGE:
	show_range(R_AXIS);
	break;
    case S_TRANGE:
	show_range(T_AXIS);
	break;
    case S_URANGE:
	show_range(U_AXIS);
	break;
    case S_VRANGE:
	show_range(V_AXIS);
	break;
    case S_XRANGE:
	show_range(FIRST_X_AXIS);
	break;
    case S_YRANGE:
	show_range(FIRST_Y_AXIS);
	break;
    case S_X2RANGE:
	show_range(SECOND_X_AXIS);
	break;
    case S_Y2RANGE:
	show_range(SECOND_Y_AXIS);
	break;
    case S_ZRANGE:
	show_range(FIRST_Z_AXIS);
	break;
#ifdef PM3D
    case S_CBRANGE:
	show_range(COLOR_AXIS);
	break;
#endif
    case S_TITLE:
	show_title();
	break;
    case S_XLABEL:
	show_axislabel(FIRST_X_AXIS);
	break;
    case S_YLABEL:
	show_axislabel(FIRST_Y_AXIS);
	break;
    case S_ZLABEL:
	show_axislabel(FIRST_Z_AXIS);
	break;
#ifdef PM3D
    case S_CBLABEL:
	show_axislabel(COLOR_AXIS);
	break;
#endif
    case S_X2LABEL:
	show_axislabel(SECOND_X_AXIS);
	break;
    case S_Y2LABEL:
	show_axislabel(SECOND_Y_AXIS);
	break;
    case S_XDATA:
	show_data_is_timedate(FIRST_X_AXIS);
	break;
    case S_YDATA:
	show_data_is_timedate(FIRST_Y_AXIS);
	break;
    case S_X2DATA:
	show_data_is_timedate(SECOND_X_AXIS);
	break;
    case S_Y2DATA:
	show_data_is_timedate(SECOND_Y_AXIS);
	break;
    case S_ZDATA:
	show_data_is_timedate(FIRST_Z_AXIS);
	break;
#ifdef PM3D
    case S_CBDATA:
	show_data_is_timedate(COLOR_AXIS);
	break;
#endif
    case S_TIMEFMT:
	show_timefmt();
	break;
    case S_LOCALE:
	show_locale();
	break;
    case S_LOADPATH:
	show_loadpath();
	break;
    case S_ZERO:
	show_zero();
	break;
    case S_DATAFILE:
	show_datafile();
	break;
#ifdef USE_MOUSE
    case S_MOUSE:
	show_mouse();
	break;
#endif
    case S_PLOT:
	show_plot();
	if (!END_OF_COMMAND) {
	    if (almost_equals(c_token, "a$dd2history")) {
		c_token++;
		add_history(replot_line);
	    }
	}
	break;
    case S_VARIABLES:
	show_variables();
	break;
/* FIXME: get rid of S_*DTICS, S_*MTICS cases */
    case S_XTICS:
    case S_XDTICS:
    case S_XMTICS:
#ifdef PM3D
	show_tics(TRUE, FALSE, FALSE, TRUE, FALSE, FALSE);
#else
	show_tics(TRUE, FALSE, FALSE, TRUE, FALSE);
#endif
	break;
    case S_YTICS:
    case S_YDTICS:
    case S_YMTICS:
#ifdef PM3D
	show_tics(FALSE, TRUE, FALSE, FALSE, TRUE, FALSE);
#else
	show_tics(FALSE, TRUE, FALSE, FALSE, TRUE);
#endif
	break;
    case S_ZTICS:
    case S_ZDTICS:
    case S_ZMTICS:
#ifdef PM3D
	show_tics(FALSE, FALSE, TRUE, FALSE, FALSE, FALSE);
#else
	show_tics(FALSE, FALSE, TRUE, FALSE, FALSE);
#endif
	break;
#ifdef PM3D
    case S_CBTICS:
    case S_CBDTICS:
    case S_CBMTICS:
	show_tics(FALSE, FALSE, FALSE, FALSE, FALSE, TRUE);
	break;
#endif
    case S_X2TICS:
    case S_X2DTICS:
    case S_X2MTICS:
#ifdef PM3D
	show_tics(FALSE, FALSE, FALSE, TRUE, FALSE, FALSE);
#else
	show_tics(FALSE, FALSE, FALSE, TRUE, FALSE);
#endif
	break;
    case S_Y2TICS:
    case S_Y2DTICS:
    case S_Y2MTICS:
#ifdef PM3D
	show_tics(FALSE, FALSE, FALSE, FALSE, TRUE, FALSE);
#else
	show_tics(FALSE, FALSE, FALSE, FALSE, TRUE);
#endif
	break;

#ifdef BACKWARDS_COMPATIBLE
	/* HBB 20010522: avoid triggering the 'default' parse error
	 * message for these commands --- they don't really exist, and
	 * shouldn't cause that message to appear */
    case S_NOMX2TICS:
    case S_NOMXTICS:
    case S_NOMY2TICS:
    case S_NOMYTICS:
    case S_NOMZTICS:
#ifdef PM3D
    case S_NOCBTICS:
    case S_NOMCBTICS:
    case S_NOCBDTICS:
    case S_NOCBMTICS:
#endif
    case S_NOX2DTICS:
    case S_NOX2MTICS:
    case S_NOX2TICS:
    case S_NOXDTICS:
    case S_NOXMTICS:
    case S_NOXTICS:
    case S_NOY2DTICS:
    case S_NOY2MTICS:
    case S_NOY2TICS:
    case S_NOYDTICS:
    case S_NOYMTICS:
    case S_NOYTICS:
    case S_NOZDTICS:
    case S_NOZMTICS:
    case S_NOZTICS:
	int_error(c_token,
		  "'show' does not accept the 'no...' type of 'set' options");
	/* FALLTHROUGH into S_INVALID! */
#endif /* BACKWARDS_COMPATIBLE */

    case S_MULTIPLOT:
	fprintf(stderr,"multiplot mode is %s\n", multiplot ? "on" : "off");
	break;

    case S_TERMOPTIONS:
	fprintf(stderr,"Terminal options are '%s'\n",
		(term_options && *term_options) ? term_options : "[none]");
	break;

    /* HBB 20010525: 'set commands' that don't have an
     * accompanying 'show' version, for no particular reason: */
    /* --- such case now, all implemented. */

    case S_INVALID:
	int_error(c_token, showmess);
    default:
	int_error(c_token, "internal parser error -- report as a bug");
    }

    screen_ok = FALSE;
    (void) putc('\n', stderr);

}


/* process 'show actiontable|at' command
 * not documented
 */
static void
show_at()
{
    (void) putc('\n', stderr);
    disp_at(temp_at(), 0);
    c_token++;
}


/* called by show_at() */
static void
disp_at(curr_at, level)
struct at_type *curr_at;
int level;
{
    register int i, j;
    register union argument *arg;

    for (i = 0; i < curr_at->a_count; i++) {
	(void) putc('\t', stderr);
	for (j = 0; j < level; j++)
	    (void) putc(' ', stderr);   /* indent */

	/* print name of instruction */

	fputs(ft[(int) (curr_at->actions[i].index)].f_name, stderr);
	arg = &(curr_at->actions[i].arg);

	/* now print optional argument */

	switch (curr_at->actions[i].index) {
	case PUSH:
	    fprintf(stderr, " %s\n", arg->udv_arg->udv_name);
	    break;
	case PUSHC:
	    (void) putc(' ', stderr);
	    disp_value(stderr, &(arg->v_arg));
	    (void) putc('\n', stderr);
	    break;
	case PUSHD1:
	    fprintf(stderr, " %c dummy\n",
	    arg->udf_arg->udf_name[0]);
	    break;
	case PUSHD2:
	    fprintf(stderr, " %c dummy\n",
	    arg->udf_arg->udf_name[1]);
	    break;
	case CALL:
	    fprintf(stderr, " %s", arg->udf_arg->udf_name);
	    if (level < 6) {
		if (arg->udf_arg->at) {
		    (void) putc('\n', stderr);
		    disp_at(arg->udf_arg->at, level + 2);       /* recurse! */
		} else
		    fputs(" (undefined)\n", stderr);
	    } else
		(void) putc('\n', stderr);
	    break;
	case CALLN:
	    fprintf(stderr, " %s", arg->udf_arg->udf_name);
	    if (level < 6) {
		if (arg->udf_arg->at) {
		    (void) putc('\n', stderr);
		    disp_at(arg->udf_arg->at, level + 2);       /* recurse! */
		} else
		    fputs(" (undefined)\n", stderr);
	    } else
		(void) putc('\n', stderr);
	    break;
	case JUMP:
	case JUMPZ:
	case JUMPNZ:
	case JTERN:
	    fprintf(stderr, " +%d\n", arg->j_arg);
	    break;
	case DOLLARS:
	    fprintf(stderr, " %d\n", arg->v_arg.v.int_val);
	    break;
	default:
	    (void) putc('\n', stderr);
	}
    }
}


/* process 'show all' command */
static void
show_all()
{
    var_show_all = 1;

    show_version(stderr);
    show_autoscale();
    show_bars();
    show_border();
    show_boxwidth();
    show_clip();
    show_label_contours();
    show_contour();
    show_dgrid3d();
    show_mapping();
    show_dummy();
    show_format();
    show_style();
    show_grid();
    show_zeroaxis(FIRST_X_AXIS);
    show_zeroaxis(FIRST_Y_AXIS);
    show_label(0);
    show_arrow(0);
/*
    show_linestyle(0);
*/
    show_keytitle();
    show_key();
    show_logscale();
    show_offsets();
    show_margin();
    show_output();
    show_print();
    show_parametric();
#ifdef PM3D
	show_palette();
    show_colorbox();
	show_pm3d();
#endif
    show_pointsize();
    show_encoding();
    show_decimalsign();
    show_fit();
    show_polar();
    show_angles();
    show_samples();
    show_isosamples();
    show_view();
    show_surface();
    show_hidden3d();
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
    show_historysize();
#endif
    show_size();
    show_origin();
    show_term();
#ifdef PM3D
    show_tics(TRUE,TRUE,TRUE,TRUE,TRUE,TRUE);
#else
    show_tics(TRUE,TRUE,TRUE,TRUE,TRUE);
#endif
    show_mtics(FIRST_X_AXIS);
    show_mtics(FIRST_Y_AXIS);
    show_mtics(FIRST_Z_AXIS);
    show_mtics(SECOND_X_AXIS);
    show_mtics(SECOND_Y_AXIS);
    show_xyzlabel("", "time", &timelabel);
    if (parametric || polar) {
	if (!is_3d_plot)
	    show_range(T_AXIS);
	else {
	    show_range(U_AXIS);
	    show_range(V_AXIS);
	}
    }
    show_range(FIRST_X_AXIS);
    show_range(FIRST_Y_AXIS);
    show_range(SECOND_X_AXIS);
    show_range(SECOND_Y_AXIS);
    show_range(FIRST_Z_AXIS);
    show_title();
    show_axislabel(FIRST_X_AXIS );
    show_axislabel(FIRST_Y_AXIS );
    show_axislabel(FIRST_Z_AXIS );
    show_axislabel(SECOND_X_AXIS);
    show_axislabel(SECOND_Y_AXIS);
    show_data_is_timedate(FIRST_X_AXIS);
    show_data_is_timedate(FIRST_Y_AXIS);
    show_data_is_timedate(SECOND_X_AXIS);
    show_data_is_timedate(SECOND_Y_AXIS);
    show_data_is_timedate(FIRST_Z_AXIS);
    show_timefmt();
    show_loadpath();
    show_fontpath();
    show_locale();
    show_zero();
    show_datafile();
#ifdef USE_MOUSE
    show_mouse();
#endif
    show_plot();
    show_variables();
    show_functions();

    var_show_all = 0;
}


/* process 'show version' command */
void
show_version(fp)
    FILE *fp;
{
    /* If printed to a file, we prefix everything with
     * a hash mark to comment out the version information.
     */
    char prefix[6];		/* "#    " */
    char *p = prefix;

    prefix[0] = '#';
    prefix[1] = prefix[2] = prefix[3] = prefix[4] = ' ';
    prefix[5] = NUL;

    if (fp == stderr) {
	/* No hash mark - let p point to the trailing '\0' */
	p += sizeof(prefix) - 1;
    } else {
#ifdef BINDIR
# ifdef X11
	fprintf(fp, "#!%s/gnuplot -persist\n#\n", BINDIR);
#  else
	fprintf(fp, "#!%s/gnuplot\n#\n", BINDIR);
# endif				/* not X11 */
#endif /* BINDIR */
    }
    fprintf(fp, "%s\n\
%s\t%s\n\
%s\tVersion %s patchlevel %s\n\
%s\tlast modified %s\n\
%s\tSystem: %s %s\n\
%s\n\
%s\t%s\n\
%s\tThomas Williams, Colin Kelley and many others\n\
%s\n\
%s\tThis is a pre-version of gnuplot 4.0. Please refer to the documentation\n\
%s\tfor command syntax changes. The old syntax will be accepted throughout\n\
%s\tthe 4.0 series, but all save files use the new syntax.\n\
%s\n\
%s\tType `help` to access the on-line reference manual\n\
%s\tThe gnuplot FAQ is available from\n\
%s\t\t%s\n\
%s\n\
%s\tSend comments and requests for help to <%s>\n\
%s\tSend bugs, suggestions and mods to <%s>\n\
%s\n",
	    p,			/* empty line */
	    p, PROGRAM,
	    p, gnuplot_version, gnuplot_patchlevel,
	    p, gnuplot_date,
	    p, os_name, os_rel,
	    p,			/* empty line */
	    p, gnuplot_copyright,
	    p,			/* authors */
	    p,			/* empty line */
	    p,			/* 4.0 info */
	    p,			/* 4.0 info */
	    p,			/* 4.0 info */
	    p,			/* empty line */
	    p,			/* Type `help` */
	    p,			/* FAQ is at */
	    p, faq_location,
	    p,			/* empty line */
	    p, help_email,
	    p, bug_email,
	    p);			/* empty line */


    /* show version long */
    if (almost_equals(c_token, "l$ong")) {
	char *helpfile = NULL;

	c_token++;

	fputs("Compile options:\n", stderr);

	{
	    /* The following code could be a lot simpler if
	     * it wasn't for Borland's broken compiler ...
	     */
	    const char *rdline, *gnu_rdline, *libgd, *libpng, *linuxvga,
			*nocwdrc, *x11, *use_mouse, *unixplot, *gnugraph;

	    rdline =
#ifdef READLINE
		"+READLINE  "
#else
		"-READLINE  "
#endif
		, gnu_rdline =
#ifdef HAVE_LIBREADLINE
		"+LIBREADLINE  "
# ifdef GNUPLOT_HISTORY
		"+HISTORY  "
# else
		"-HISTORY  "
# endif
#else
		"-LIBREADLINE  "
#endif
		, libgd =
#ifdef HAVE_LIBGD
# ifdef HAVE_GD_PNG
		"+GD_PNG  "
# endif
# ifdef HAVE_GD_JPEG
		"+GD_JPEG  "
# endif
# ifdef HAVE_GD_GIF
		"+GD_GIF  "
# endif
# ifdef HAVE_GD_TTF
		"+GD_TTF  "
# endif
		""
#else
		"-LIBGD  "
#endif
		, libpng =
#if defined(HAVE_LIBPNG) && !(defined(HAVE_GD_PNG) || defined(HAVE_GD_JPEG))
		"+PNG  "
#else
		"-PNG  "
#endif
		, linuxvga =
#ifdef LINUXVGA
		"+LINUXVGA  "
#else
		""
#endif
		, nocwdrc =
#ifdef NOCWDRC
		"+NOCWDRC  "
#else
		"-NOCWDRC  "
#endif
		, x11 =

#ifdef X11
		"+X11  "
#else
		""
#endif
		, use_mouse =

#ifdef USE_MOUSE
		"+USE_MOUSE  "
#else
		""
#endif
		, unixplot =
#ifdef UNIXPLOT
		"+UNIXPLOT  "
#else
		""
#endif
		, gnugraph =
#ifdef GNUGRAPH
		"+GNUGRAPH  "
#else
		""
#endif
		;
	    fprintf(stderr, "%s%s%s%s%s%s%s%s%s%s\n\n", rdline, gnu_rdline,
		    libgd, libpng, linuxvga, nocwdrc, x11, use_mouse,
		    unixplot, gnugraph);
	}

	if ((helpfile = getenv("GNUHELP")) == NULL) {
#if defined(ATARI) || defined(MTOS)
	    if ((helpfile = user_gnuplotpath) == NULL) {
		helpfile = HELPFILE;
	    }
#else
	    helpfile = HELPFILE;
#endif
	}

#ifdef X11
	{
	    char *driverdir = getenv("GNUPLOT_DRIVER_DIR");

	    if (driverdir == NULL) 
		driverdir = X11_DRIVER_DIR;
	    fprintf(stderr, "\
DRIVER_DIR   = \"%s\"\n", driverdir);
	}
#endif

	fprintf(stderr, "\
HELPFILE     = \"%s\"\n\
CONTACT      = <%s>\n\
HELPMAIL     = <%s>\n", helpfile, bug_email, help_email);

    }
}


/* process 'show autoscale' command */
static void
show_autoscale()
{
    SHOW_ALL_NL;

#define SHOW_AUTOSCALE(axis) {						      \
	t_autoscale ascale = axis_array[axis].set_autoscale;		      \
									      \
	fprintf(stderr, "\t%s: %s%s%s%s%s, ",				      \
		axis_defaults[axis].name,				      \
		(ascale & AUTOSCALE_BOTH) ? "ON" : "OFF",		      \
		((ascale & AUTOSCALE_BOTH) == AUTOSCALE_MIN) ? " (min)" : "", \
		((ascale & AUTOSCALE_BOTH) == AUTOSCALE_MAX) ? " (max)" : "", \
		(ascale & AUTOSCALE_FIXMIN) ? " (fixmin)" : "",		      \
		(ascale & AUTOSCALE_FIXMAX) ? " (fixmax)" : "");	      \
    }

    fputs("\tautoscaling is ", stderr);
    if (parametric) {
	if (is_3d_plot) {
	    SHOW_AUTOSCALE(T_AXIS);
	} else {
	    SHOW_AUTOSCALE(U_AXIS);
	    SHOW_AUTOSCALE(V_AXIS);
	}
    }

    if (polar) {
	SHOW_AUTOSCALE(R_AXIS)
    }

    SHOW_AUTOSCALE(FIRST_X_AXIS );
    SHOW_AUTOSCALE(FIRST_Y_AXIS );
    fputs("\n\t               ", stderr);
    SHOW_AUTOSCALE(SECOND_X_AXIS);
    SHOW_AUTOSCALE(SECOND_Y_AXIS);
    fputs("\n\t               ", stderr);
    SHOW_AUTOSCALE(FIRST_Z_AXIS );
#ifdef PM3D
    SHOW_AUTOSCALE(COLOR_AXIS);
#endif
#undef SHOW_AUTOSCALE

}


/* process 'show bars' command */
static void
show_bars()
{
    SHOW_ALL_NL;

    /* I really like this: "terrorbars" ;-) */
    if (bar_size > 0.0)
	fprintf(stderr, "\terrorbars are plotted with bars of size %f\n",
		bar_size);
    else
	fputs("\terrors are plotted without bars\n", stderr);
}


/* process 'show border' command */
static void
show_border()
{
    SHOW_ALL_NL;

    fprintf(stderr, "\tborder is %sdrawn %d\n", draw_border ? "" : "not ",
	    draw_border);
    /* HBB 980609: added facilities to specify border linestyle */
    fprintf(stderr, "\tBorder drawn with linetype %d, linewidth %.3f\n",
	    border_lp.l_type + 1, border_lp.l_width);
}


/* process 'show boxwidth' command */
static void
show_boxwidth()
{
    SHOW_ALL_NL;

    if (boxwidth < 0.0)
	fputs("\tboxwidth is auto\n", stderr);
    else {
#if USE_ULIG_RELATIVE_BOXWIDTH
	fprintf(stderr, "\tboxwidth is %g %s\n", boxwidth,
		(boxwidth_is_absolute) ? "absolute" : "relative");
#else
	fprintf(stderr, "\tboxwidth is %g\n", boxwidth);
#endif /* USE_ULIG_RELATIVE_BOXWIDTH */
    }
}


#if USE_ULIG_FILLEDBOXES
/* process 'show fillstyle' command */
static void
show_fillstyle()
{
    SHOW_ALL_NL;

    switch(default_fillstyle.fillstyle) {
    case FS_SOLID:
        fprintf(stderr,
	    "\tFill style is solid colour with density %f", 
	    default_fillstyle.filldensity/100.0);
        break;
    case FS_PATTERN:
        fprintf(stderr,
	    "\tFill style uses patterns starting at %d",
	    default_fillstyle.fillpattern);
        break;
    default:
        fprintf(stderr, "\tFill style is empty");
    }
    switch(default_fillstyle.border_linetype) {
    case LT_NODRAW:
	fprintf(stderr," with no border\n");
	break;
    case LT_UNDEFINED:
	fprintf(stderr," with border\n");
	break;
    default:
	fprintf(stderr," with border linetype %d\n",default_fillstyle.border_linetype+1);
	break;
    }
}
#endif /* USE_ULIG_FILLEDBOXES */


/* process 'show clip' command */
static void
show_clip()
{
    SHOW_ALL_NL;

    fprintf(stderr, "\tpoint clip is %s\n", (clip_points) ? "ON" : "OFF");

    if (clip_lines1)
	fputs("\tdrawing and clipping lines between inrange and outrange points\n", stderr);
    else
	fputs("\tnot drawing lines between inrange and outrange points\n", stderr);

    if (clip_lines2)
	fputs("\tdrawing and clipping lines between two outrange points\n", stderr);
    else
	fputs("\tnot drawing lines between two outrange points\n", stderr);
}


/* process 'show cntrparam|contour' commands */
static void
show_contour()
{
    SHOW_ALL_NL;

    fprintf(stderr, "\tcontour for surfaces are %s",
	    (draw_contour) ? "drawn" : "not drawn\n");

    if (draw_contour) {
	fprintf(stderr, " in %d levels on ", contour_levels);
	switch (draw_contour) {
	case CONTOUR_BASE:
	    fputs("grid base\n", stderr);
	    break;
	case CONTOUR_SRF:
	    fputs("surface\n", stderr);
	    break;
	case CONTOUR_BOTH:
	    fputs("grid base and surface\n", stderr);
	    break;
	case CONTOUR_NONE:
	    /* should not happen --- be easy: don't complain... */
	    break;
	}
	switch (contour_kind) {
	case CONTOUR_KIND_LINEAR:
	    fputs("\t\tas linear segments\n", stderr);
	    break;
	case CONTOUR_KIND_CUBIC_SPL:
	    fprintf(stderr, "\t\tas cubic spline interpolation segments with %d pts\n", contour_pts);
	    break;
	case CONTOUR_KIND_BSPLINE:
	    fprintf(stderr, "\t\tas bspline approximation segments of order %d with %d pts\n", contour_order, contour_pts);
	    break;
	}
	switch (contour_levels_kind) {
	case LEVELS_AUTO:
	    fprintf(stderr, "\t\tapprox. %d automatic levels\n", contour_levels);
	    break;
	case LEVELS_DISCRETE:
	    {
		int i;
		fprintf(stderr, "\t\t%d discrete levels at ", contour_levels);
		fprintf(stderr, "%g", contour_levels_list[0]);
		for (i = 1; i < contour_levels; i++)
		    fprintf(stderr, ",%g ", contour_levels_list[i]);
		putc('\n', stderr);
		break;
	    }
	case LEVELS_INCREMENTAL:
	    fprintf(stderr, "\t\t%d incremental levels starting at %g, step %g, end %g\n", contour_levels, contour_levels_list[0],
		    contour_levels_list[1],
		    contour_levels_list[0] + (contour_levels - 1) * contour_levels_list[1]);
	    /* contour-levels counts both ends */
	    break;
	}
	/* fprintf(stderr,"\t\tcontour line types are %s\n", label_contours ? "varied" : "all the same"); */
	show_label_contours();
    }
}


/* called by show_contour() */
static void
show_label_contours()
{
    if (label_contours)
	fprintf(stderr, "\tcontour line types are varied & labeled with format '%s'\n", contour_format);
    else
	fputs("\tcontour line types are all the same\n", stderr);
}


/* process 'show dgrid3d' command */
static void
show_dgrid3d()
{
    SHOW_ALL_NL;

    if (dgrid3d)
	fprintf(stderr, "\
\tdata grid3d is enabled for mesh of size %dx%d, norm=%d\n",
		dgrid3d_row_fineness,
		dgrid3d_col_fineness,
		dgrid3d_norm_value);
    else
	fputs("\tdata grid3d is disabled\n", stderr);
}


/* process 'show mapping' command */
static void
show_mapping()
{
    SHOW_ALL_NL;

    fputs("\tmapping for 3-d data is ", stderr);

    switch (mapping3d) {
    case MAP3D_CARTESIAN:
	fputs("cartesian\n", stderr);
	break;
    case MAP3D_SPHERICAL:
	fputs("spherical\n", stderr);
	break;
    case MAP3D_CYLINDRICAL:
	fputs("cylindrical\n", stderr);
	break;
    }
}


/* process 'show dummy' command */
static void
show_dummy()
{
    SHOW_ALL_NL;

    fprintf(stderr, "\tdummy variables are \"%s\" and \"%s\"\n",
	    set_dummy_var[0], set_dummy_var[1]);
}


/* process 'show format' command */
static void
show_format()
{
    SHOW_ALL_NL;

    fprintf(stderr, "\ttic format is:\n");
#define SHOW_FORMAT(_axis)						\
    fprintf(stderr, "\t  %s-axis: \"%s\"\n", axis_defaults[_axis].name,	\
	    conv_text(axis_array[_axis].formatstring));
    SHOW_FORMAT(FIRST_X_AXIS );
    SHOW_FORMAT(FIRST_Y_AXIS );
    SHOW_FORMAT(SECOND_X_AXIS);
    SHOW_FORMAT(SECOND_Y_AXIS);
    SHOW_FORMAT(FIRST_Z_AXIS );
#ifdef PM3D
    SHOW_FORMAT(COLOR_AXIS);
#endif
#undef SHOW_FORMAT
}


/* process 'show style' sommand */
static void
show_style()
{
    struct value a;
    int tag = 0;

    switch(lookup_table(&show_style_tbl[0],c_token)){
    case SHOW_STYLE_DATA:
	SHOW_ALL_NL;
	show_styles("Data",data_style);
	c_token++;
	break;
    case SHOW_STYLE_FUNCTION:
	SHOW_ALL_NL;
	show_styles("Functions", func_style);
	c_token++;
	break;
    case SHOW_STYLE_LINE:
	c_token++;
	CHECK_TAG_GT_ZERO;
	show_linestyle(tag);
	break;
#if USE_ULIG_FILLEDBOXES
    case SHOW_STYLE_FILLING:
	show_fillstyle();
	c_token++;
	break;
#endif /* USE_ULIG_FILLEDBOXES */
    case SHOW_STYLE_ARROW:
	c_token++;
	CHECK_TAG_GT_ZERO;
	show_arrowstyle(tag);
	break;
    default:
	/* show all styles */
	show_styles("Data",data_style);
	show_styles("Functions", func_style);
	show_linestyle(0);
#if USE_ULIG_FILLEDBOXES
	show_fillstyle();
#endif /* USE_ULIG_FILLEDBOXES */
	show_arrowstyle(0);
	break;
    }
}

/* called by show_data() and show_func() */
static void
show_styles(name, style)
    const char *name;
    enum PLOT_STYLE style;
{

    fprintf(stderr, "\t%s are plotted with ", name);
    /* perhaps these need to be put into a constant array ? */
    switch (style) {
    case LINES:
	fputs("lines\n", stderr);
	break;
    case POINTSTYLE:
	fputs("points\n", stderr);
	break;
    case IMPULSES:
	fputs("impulses\n", stderr);
	break;
    case LINESPOINTS:
	fputs("linespoints\n", stderr);
	break;
    case DOTS:
	fputs("dots\n", stderr);
	break;

    case YERRORLINES:
	fputs("yerrorlines\n", stderr);
	break;
    case XERRORLINES:
	fputs("xerrorlines\n", stderr);
	break;
    case XYERRORLINES:
	fputs("xyerrorlines\n", stderr);
	break;

    case YERRORBARS:
	fputs("yerrorbars\n", stderr);
	break;
    case XERRORBARS:
	fputs("xerrorbars\n", stderr);
	break;
    case XYERRORBARS:
	fputs("xyerrorbars\n", stderr);
	break;
    case BOXES:
	fputs("boxes\n", stderr);
	break;
#ifdef PM3D
    case FILLEDCURVES:
	fputs("filledcurves ", stderr);
	filledcurves_options_tofile( (!strcmp(name,"Data")) ? 
		&filledcurves_opts_data : &filledcurves_opts_func, stderr );
	fputc('\n', stderr);
	break;
#endif
    case BOXERROR:
	fputs("boxerrorbars\n", stderr);
	break;
    case BOXXYERROR:
	fputs("boxxyerrorbars\n", stderr);
	break;
    case STEPS:
	fputs("steps\n", stderr);
	break;
    case FSTEPS:
	fputs("fsteps\n", stderr);
	break;
    case HISTEPS:
	fputs("histeps\n", stderr);
	break;
    case VECTOR:
	fputs("vector\n", stderr);
	break;
    case FINANCEBARS:
	fputs("financebars\n", stderr);
	break;
    case CANDLESTICKS:
	fputs("candlesticks\n", stderr);
	break;
#ifdef PM3D
    case PM3DSURFACE:
	fputs("pm3d\n", stderr);
	break;
#endif
    }
}


/* called by show_func() */
static void
show_functions()
{
    register struct udft_entry *udf = first_udf;

    fputs("\n\tUser-Defined Functions:\n", stderr);

    while (udf) {
	if (udf->definition)
	    fprintf(stderr, "\t%s\n", udf->definition);
	else
	    fprintf(stderr, "\t%s is undefined\n", udf->udf_name);
	udf = udf->next_udf;
    }
}


/* process 'show grid' command */
static void
show_grid()
{
    SHOW_ALL_NL;

    if (! some_grid_selected()) {
	fputs("\tgrid is OFF\n", stderr);
	return;
    }

#if 0
    /* Old method of accessing grid choices */
    fprintf(stderr, "\t%s grid drawn at%s%s%s%s%s%s%s%s%s%s%s%s tics\n",
	    (polar_grid_angle != 0) ? "Polar" : "Rectangular",
	    grid_selection & GRID_X ? " x" : "",
	    grid_selection & GRID_Y ? " y" : "",
	    grid_selection & GRID_Z ? " z" : "",
	    grid_selection & GRID_X2 ? " x2" : "",
	    grid_selection & GRID_Y2 ? " y2" : "",
	    grid_selection & GRID_MX ? " mx" : "",
	    grid_selection & GRID_MY ? " my" : "",
	    grid_selection & GRID_MZ ? " mz" : "",
	    grid_selection & GRID_MX2 ? " mx2" : "",
	    grid_selection & GRID_MY2 ? " my2" : "",
#ifdef PM3D
	    grid_selection & GRID_CB ? " cb" : "",
	    grid_selection & GRID_MCB ? " mcb" : ""
#else
	    "", ""
#endif /* PM3D */
	    );
#else 
    /* HBB 20010806: new storage method for grid options: */
    fprintf(stderr, "\t%s grid drawn at", 
	    (polar_grid_angle != 0) ? "Polar" : "Rectangular");
#define SHOW_GRID(axis)						\
    if (axis_array[axis].gridmajor)				\
	fprintf(stderr, " %s", axis_defaults[axis].name);	\
    if (axis_array[axis].gridminor)				\
	fprintf(stderr, " m%s", axis_defaults[axis].name);
    SHOW_GRID(FIRST_X_AXIS );
    SHOW_GRID(FIRST_Y_AXIS );
    SHOW_GRID(SECOND_X_AXIS);
    SHOW_GRID(SECOND_Y_AXIS);
    SHOW_GRID(FIRST_Z_AXIS );
#ifdef PM3D
    SHOW_GRID(COLOR_AXIS);
#endif
#undef SHOW_GRID
    fputs(" tics\n", stderr);
#endif /* 0/1 */

    fprintf(stderr, "\
\tMajor grid drawn with linetype %d, linewidth %.3f\n\
\tMinor grid drawn with linetype %d, linewidth %.3f\n",
	    grid_lp.l_type + 1, grid_lp.l_width,
	    mgrid_lp.l_type + 1, mgrid_lp.l_width);

    if (polar_grid_angle)
	fprintf(stderr, "\tGrid radii drawn every %f %s\n",
		polar_grid_angle / ang2rad,
		(ang2rad == 1.0) ? "radians" : "degrees");

    fprintf(stderr, "\tGrid drawn at %s\n", (grid_layer==-1) ? "default layer" : ((grid_layer==0) ? "back" : "front"));
}


/* process 'show {x|y}zeroaxis' command */
static void
show_zeroaxis(axis)
    AXIS_INDEX axis;
{
    SHOW_ALL_NL;

    if (axis_array[axis].zeroaxis.l_type > L_TYPE_NODRAW)
	fprintf(stderr, "\
\t%szeroaxis is drawn with linestyle %d, linewidth %.3f\n",
		axis_defaults[axis].name,
		axis_array[axis].zeroaxis.l_type + 1,
		axis_array[axis].zeroaxis.l_width);
    else
	fprintf(stderr, "\t%szeroaxis is OFF\n", axis_defaults[axis].name);

    if ((axis / SECOND_AXES) == 0) {
	/* this is a 'first' axis. To output secondary axis, call self
	 * recursively: */
	show_zeroaxis(axis + SECOND_AXES);
}
}

/* */
static void
show_label(tag)
int tag;			/* 0 means show all */
{
    struct text_label *this_label;
    TBOOLEAN showed = FALSE;

    for (this_label = first_label; this_label != NULL;
	 this_label = this_label->next) {
	if (tag == 0 || tag == this_label->tag) {
	    showed = TRUE;
	    fprintf(stderr, "\tlabel %d \"%s\" at ",
		    this_label->tag, 
		    (this_label->text==NULL) ? "" : conv_text(this_label->text));
	    show_position(&this_label->place);
	    switch (this_label->pos) {
	    case LEFT:{
		    fputs(" left", stderr);
		    break;
		}
	    case CENTRE:{
		    fputs(" centre", stderr);
		    break;
		}
	    case RIGHT:{
		    fputs(" right", stderr);
		    break;
		}
	    }
	    if (this_label->rotate)
	    	fprintf(stderr, " rotated by %d degrees (if possible)", this_label->rotate); 
	    else
	    	fprintf(stderr, " not rotated");
	    fprintf(stderr, " %s ", this_label->layer ? "front" : "back");
	    if (this_label->font != NULL)
		fprintf(stderr, " font \"%s\"", this_label->font);
	    if (this_label->textcolor.type) {
	    	fprintf(stderr, " textcolor");
		switch(this_label->textcolor.type) {
		case TC_LT:   fprintf(stderr," lt %d", this_label->textcolor.lt+1); 
			      break;
		case TC_Z:    fprintf(stderr," palette z");
			      break;
		case TC_CB:   fprintf(stderr," palette cb %g", this_label->textcolor.value);
			      break;
		case TC_FRAC: fprintf(stderr," palette fraction %4.2f", this_label->textcolor.value);
			      break;
		}
	    }
	    if (this_label->lp_properties.pointflag == 0)
		fprintf(stderr, " nopoint");
	    else {
		fprintf(stderr, " point with color of linetype %d pointtype %d pointsize %g offset %g,%g",
		    this_label->lp_properties.l_type+1,
		    this_label->lp_properties.p_type+1,
		    this_label->lp_properties.p_size,
		    this_label->hoffset, this_label->voffset);
	    }

	    /* Entry font added by DJL */
	    fputc('\n', stderr);
	}
    }
    if (tag > 0 && !showed)
	int_error(c_token, "label not found");
}


static void
show_arrow(tag)
int tag;			/* 0 means show all */
{
    struct arrow_def *this_arrow;
    TBOOLEAN showed = FALSE;

    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next) {
	if (tag == 0 || tag == this_arrow->tag) {
	    showed = TRUE;
	    fprintf(stderr, "\tarrow %d, linetype %d, linewidth %.3f %s %s %s\n\t  from ",
		    this_arrow->tag,
		    this_arrow->arrow_properties.lp_properties.l_type + 1,
		    this_arrow->arrow_properties.lp_properties.l_width,
		    this_arrow->arrow_properties.head ? (this_arrow->arrow_properties.head==2 ? " both heads " : "") : " (nohead)",
		    ( (this_arrow->arrow_properties.head_filled==2) ? "filled" :
		      ( (this_arrow->arrow_properties.head_filled==1) ? "empty" :
			"nofilled" )),
		    this_arrow->arrow_properties.layer ? "front" : "back");
	    show_position(&this_arrow->start);
	    fputs(this_arrow->relative ? " rto " : " to ", stderr);
	    show_position(&this_arrow->end);
	    if (this_arrow->arrow_properties.head_length > 0) {
		static char *msg[] =
		{"(first x axis) ", "(second x axis) ", "(graph units) ", "(screen units) "};
		fprintf(stderr,"\n\t  arrow head: length %s%g, angle %g deg", 
		   this_arrow->arrow_properties.head_lengthunit == first_axes ? "" : msg[this_arrow->arrow_properties.head_lengthunit],
		   this_arrow->arrow_properties.head_length,
                   this_arrow->arrow_properties.head_angle);
		if (this_arrow->arrow_properties.head_filled!=0)
		    fprintf(stderr,", backangle %g deg", 
			    this_arrow->arrow_properties.head_backangle);
	    }
	    putc('\n', stderr);
	}
    }
    if (tag > 0 && !showed)
	int_error(c_token, "arrow not found");
}


/* process 'show keytitle' command */
static void
show_keytitle()
{
    legend_key *key = &keyT;
    SHOW_ALL_NL;

    fprintf(stderr, "\tkeytitle is \"%s\"\n", conv_text(key->title));
}


/* process 'show key' command */
static void
show_key()
{
    legend_key *key = &keyT;
    char *str = gp_alloc(30, "show_key");

    SHOW_ALL_NL;

    if (!(key->visible)) {
	fputs("\tkey is OFF\n", stderr);
	return;
    } 

    switch (key->flag) {
    case KEY_AUTO_PLACEMENT:
	if (key->vpos == TUNDER) {
	    strcpy(str, "below");
	} else if (key->vpos == TTOP) {
	    strcpy(str, "top");
	} else {
	    strcpy(str, "bottom");
	}
	if (key->hpos == TOUT) {
	    strcpy(str, "outside (right)");
	} else if (key->hpos == TLEFT) {
	    strcat(str, " left");
	} else {
	    strcat(str, " right");
	}
	if (key->vpos != TUNDER && key->hpos != TOUT) {
	    strcat(str, " corner");
	}
	fprintf(stderr, "\tkey is ON, position: %s\n", str);
	free(str);
	break;
    case KEY_USER_PLACEMENT:
	fputs("\tkey is at ", stderr);
	show_position(&key->user_pos);
	putc('\n', stderr);
	break;
    }

	fprintf(stderr, "\tkey is %s justified, %sreversed, %senhanced and ",
		key->just == JLEFT ? "left" : "right",
		key->reverse ? "" : "not ",
		key->enhanced ? "" : "not ");
	if (key->box.l_type > L_TYPE_NODRAW)
	    fprintf(stderr, "boxed\n\twith linetype %d, linewidth %.3f\n",
		    key->box.l_type + 1, key->box.l_width);
	else
	    fprintf(stderr, "not boxed\n");
	fprintf(stderr, "\tsample length is %g characters\n\
\tvertical spacing is %g characters\n\
\twidth adjustment is %g characters\n\
\theight adjustment is %g characters\n\
\tcurves are%s automatically titled\n\
\tkey title is \"%s\"\n",
		    key->swidth,
		    key->vert_factor,
		    key->width_fix,
		    key->height_fix,
                    key->auto_titles ? "" : " not",
		    key->title);

}


/* */
static void
show_position(pos)
struct position *pos;
{
    static const char *msg[] = { "(first axes) ", "(second axes) ",
				 "(graph units) ", "(screen units) " };

    assert(first_axes == 0 && second_axes == 1 && graph == 2 && screen == 3);

    fprintf(stderr, "(%s%g, %s%g, %s%g)",
	    pos->scalex == first_axes ? "" : msg[pos->scalex], pos->x,
	    pos->scaley == pos->scalex ? "" : msg[pos->scaley], pos->y,
	    pos->scalez == pos->scaley ? "" : msg[pos->scalez], pos->z);

}


/* process 'show logscale' command */
static void
show_logscale()
{
    int count = 0;

    SHOW_ALL_NL;

#define SHOW_LOG(axis)							\
    {									\
	if (axis_array[axis].log) 					\
	    fprintf(stderr, "%s %s (base %g)",				\
		    !count++ ? "\tlogscaling" : " and",			\
		    axis_defaults[axis].name,axis_array[axis].base);	\
    }
    SHOW_LOG(FIRST_X_AXIS );
    SHOW_LOG(FIRST_Y_AXIS );
    SHOW_LOG(FIRST_Z_AXIS );
    SHOW_LOG(SECOND_X_AXIS);
    SHOW_LOG(SECOND_Y_AXIS);
#ifdef PM3D
    SHOW_LOG(COLOR_AXIS );
#endif
#undef SHOW_LOG

    if (count == 0)
	fputs("\tno logscaling\n", stderr);
    else if (count == 1)
	fputs(" only\n", stderr);
    else
	putc('\n', stderr);
}


/* process 'show offsets' command */
static void
show_offsets()
{
    SHOW_ALL_NL;

    fprintf(stderr, "\toffsets are %g, %g, %g, %g\n", loff, roff, toff, boff);
}


/* process 'show margin' command */
static void
show_margin()
{
    SHOW_ALL_NL;

    if (lmargin >= 0)
	fprintf(stderr, "\tlmargin is set to %d\n", lmargin);
    else
	fputs("\tlmargin is computed automatically\n", stderr);
    if (bmargin >= 0)
	fprintf(stderr, "\tbmargin is set to %d\n", bmargin);
    else
	fputs("\tbmargin is computed automatically\n", stderr);
    if (rmargin >= 0)
	fprintf(stderr, "\trmargin is set to %d\n", rmargin);
    else
	fputs("\trmargin is computed automatically\n", stderr);
    if (tmargin >= 0)
	fprintf(stderr, "\ttmargin is set to %d\n", tmargin);
    else
	fputs("\ttmargin is computed automatically\n", stderr);
}


/* process 'show output' command */
static void
show_output()
{
    SHOW_ALL_NL;

    if (outstr)
	fprintf(stderr, "\toutput is sent to '%s'\n", outstr);
    else
	fputs("\toutput is sent to STDOUT\n", stderr);
}


/* process 'show print' command */
static void
show_print()
{
    SHOW_ALL_NL;

    fprintf(stderr, "\tprint output is sent to '%s'\n", print_show_output());
}


/* process 'show parametric' command */
static void
show_parametric()
{
    SHOW_ALL_NL;
    fprintf(stderr, "\tparametric is %s\n", (parametric) ? "ON" : "OFF");
}

#ifdef PM3D

static void
show_palette_rgbformulae()
{
    int i;
    fprintf(stderr,"\t  * there are %i available rgb color mapping formulae:",
	    sm_palette.colorFormulae);
    /* print the description of the color formulae */
    i = 0;
    while ( *(ps_math_color_formulae[2*i]) ) {
	if (i % 3 == 0)
	    fputs("\n\t    ", stderr);
	fprintf(stderr, "%2i: %-15s",i,ps_math_color_formulae[2*i+1]);
	i++;
    }
    fputs("\n", stderr);
    fputs("\t  * negative numbers mean inverted=negative colour component\n",
	    stderr);
    fprintf(stderr,
	    "\t  * thus the ranges in `set pm3d rgbformulae' are -%i..%i\n",
	    sm_palette.colorFormulae-1,sm_palette.colorFormulae-1);
    ++c_token;
}


static void
show_palette_fit2rgbformulae()
{
#define rgb_distance(r,g,b) ((r)*(r) + (g)*(g) + (b)*(b))
    int pts = 32; /* resolution: nb of points in the discrete raster for comparisons */
    int i, p, r, g, b, ir, ig, ib, rMin, gMin, bMin;
    int maxFormula = sm_palette.colorFormulae - 1; /* max formula number */
    double gray, dist, distMin;
    rgb_color *currRGB;
    int *formulaeSeq;
    double **formulae;
    ++c_token;
    if (sm_palette.colorMode == SMPAL_COLOR_MODE_RGB && sm_palette.cmodel == C_MODEL_RGB) {
	fprintf(stderr, "\tCurrent palette is\n\t    set palette rgbformulae %i,%i,%i\n", sm_palette.formulaR, sm_palette.formulaG, sm_palette.formulaB);
	return;
    }
    /* allocate and fill R, G, B values rastered on pts points */
    currRGB = (rgb_color*)gp_alloc(pts * sizeof(rgb_color), "RGB pts");
    for (p = 0; p < pts; p++) {
	gray = (double)p / (pts - 1);
	rgb1_from_gray(gray, &(currRGB[p]));
    }
    /* organize sequence of rgb formulae */
    formulaeSeq = gp_alloc((2*maxFormula+1) * sizeof(int), "formulaeSeq");
    for (i = 0; i <= maxFormula; i++)
	formulaeSeq[i] = i;
    for (i = 1; i <= maxFormula; i++)
	formulaeSeq[maxFormula+i] = -i;
    /* allocate and fill all +-formulae on the interval of given number of points */
    formulae = gp_alloc((2*maxFormula+1) * sizeof(double*), "formulae");
    for (i = 0; i < 2*maxFormula+1; i++) {
	formulae[i] = gp_alloc(pts * sizeof(double), "formulae pts");
	for (p = 0; p < pts; p++) {
	    double gray = (double)p / (pts - 1);
	    formulae[i][p] = GetColorValueFromFormula(formulaeSeq[i], gray);
	}
    }
    /* Now go over all rastered formulae, compare them to the current one, and
       find the minimal distance.
     */
    distMin = VERYLARGE;
    for (ir = 0; ir <	 2*maxFormula+1; ir++) {
	for (ig = 0; ig < 2*maxFormula+1; ig++) {
	    for (ib = 0; ib < 2*maxFormula+1; ib++) {
		dist = 0; /* calculate distance of the two rgb profiles */
		for (p = 0; p < pts; p++) {
		double tmp = rgb_distance(
			    currRGB[p].r - formulae[ir][p], 
			    currRGB[p].g - formulae[ig][p], 
			    currRGB[p].b - formulae[ib][p] );
		    dist += tmp;
		}
		if (dist < distMin) {
		    distMin = dist;
		    rMin = formulaeSeq[ir];
		    gMin = formulaeSeq[ig];
		    bMin = formulaeSeq[ib];
		}
	    }
	}
    }
    fprintf(stderr, "\tThe best match of the current palette corresponds to\n\t    set palette rgbformulae %i,%i,%i\n", rMin, gMin, bMin);
#undef rgb_distance
    for (i = 0; i < 2*maxFormula+1; i++)
	free(formulae[i]);
    free(formulae);
    free(formulaeSeq);
    free(currRGB);
}


static void
show_palette_palette()
{
    int colors, i;
    struct value a;
    double gray, r, g, b;
    rgb_color rgb1;
    rgb255_color rgb255;
    int how = 0; /* How to print table: 0: default large; 1: rgb 0..1; 2: integers 0..255 */
    FILE *f;

    c_token++;
    if (END_OF_COMMAND)
	int_error(c_token,"palette size required");
    colors = (int) real(const_express(&a));
    if (colors<2) colors = 128;
    if (!END_OF_COMMAND) {
	if (almost_equals(c_token, "f$loat")) /* option: print r,g,b floats 0..1 values */
	    how = 1;
	else if (almost_equals(c_token, "i$nt")) /* option: print only integer 0..255 values */
	    how = 2;
    else
	    int_error(c_token, "expecting no option or int or float");
	c_token++;
    }

    f = (print_out) ? print_out : stderr;
    fprintf(stderr, "%s palette with %i discrete colors", 
	    (sm_palette.colorMode == SMPAL_COLOR_MODE_GRAY) ? "Gray" : "Color", colors);
    if (print_out) fprintf(stderr," saved to \"%s\".", print_out_name);
	else fprintf(stderr, ".\n");

    for (i = 0; i < colors; i++) {
	/* colours equidistantly from [0,1]  */
	gray = (double)i / (colors - 1); 
	if (sm_palette.positive == SMPAL_NEGATIVE)
	    /* needed, since printing without call to set_color()  */
	    gray = 1 - gray;
	rgb1_from_gray(gray, &rgb1);
	rgb255_from_rgb1(rgb1, &rgb255);

	switch (how) {
	    case 1:
		fprintf(f, "%0.4f\t%0.4f\t%0.4f\n", rgb1.r, rgb1.g, rgb1.b);
		break;
	    case 2:
		fprintf(f, "%i\t%i\t%i\n", 
			(int)rgb255.r, (int)rgb255.g, (int)rgb255.b);
		break;
	    default:
		fprintf(f,
		"%3i. gray=%0.4f, (r,g,b)=(%0.4f,%0.4f,%0.4f), #%02x%02x%02x = %3i %3i %3i\n",
		i, gray, rgb1.r, rgb1.g, rgb1.b,
		(int)rgb255.r, (int)rgb255.g, (int)rgb255.b,
		(int)rgb255.r, (int)rgb255.g, (int)rgb255.b );
    }
}
}


static void
show_palette_gradient()
{
    int i;
    double gray,r,g,b;

    ++c_token;
    if (sm_palette.colorMode != SMPAL_COLOR_MODE_GRADIENT) {
        fputs( "\tcolor mapping *not* done by defined gradient.\n", stderr );
	return;
    }

    for( i=0; i<sm_palette.gradient_num; ++i ) {
        gray = sm_palette.gradient[i].pos;
        r = sm_palette.gradient[i].col.r;
        g = sm_palette.gradient[i].col.g;
        b = sm_palette.gradient[i].col.b;
        fprintf(stderr, 
 "%3i. gray=%0.4f, (r,g,b)=(%0.4f,%0.4f,%0.4f), #%02x%02x%02x = %3i %3i %3i\n",
		i, gray, r,g,b,
                (int)(255*r+.5),(int)(255*g+.5),(int)(255*b+.5),
                (int)(255*r+.5),(int)(255*g+.5),(int)(255*b+.5) );
	}
}


static void
show_palette_colornames()
{
    const struct gen_table *tbl = pm3d_color_names_tbl;
    int i=0;
    fputs( "\tList of known color names:", stderr );
    while (tbl->key) {
#if 0
	/* Print only color names, table with 4 columns */
	if (i%4 == 0) fputs( "\n  ", stderr );
	fprintf( stderr, "%-18s ", tbl->key );
#else
	/* Print color names and their rgb values, table with 1 column */
	int r = ((tbl->value >> 16 ) & 255);
	int g = ((tbl->value >> 8 ) & 255);
	int b = (tbl->value & 255);

	fprintf( stderr, "\n  %-18s ", tbl->key );
	fprintf(stderr, "#%02x%02x%02x = %3i %3i %3i", r,g,b, r,g,b);
#endif
	++tbl;
	++i;
    }
    fputs( "\n", stderr );
    ++c_token;
}


static void
show_palette()
{
    /* no option given, i.e. "show palette" */
    if (END_OF_COMMAND) {
	fprintf(stderr,"\tpalette is %s\n",
	    sm_palette.colorMode == SMPAL_COLOR_MODE_GRAY ? "GRAY" : "COLOR");

	switch( sm_palette.colorMode ) {
	  case SMPAL_COLOR_MODE_GRAY: break;
	  case SMPAL_COLOR_MODE_RGB:
	    fprintf(stderr,"\trgb color mapping by rgbformulae are %i,%i,%i\n",
		    sm_palette.formulaR, sm_palette.formulaG, 
		    sm_palette.formulaB);
	    break;
	  case SMPAL_COLOR_MODE_GRADIENT:
	    fputs( "\tcolor mapping by defined gradient\n", stderr );
	    break;
	  case SMPAL_COLOR_MODE_FUNCTIONS:
	    fputs("\tcolor maping is done by user defined functions\n",stderr);
	    if (sm_palette.Afunc.at && sm_palette.Afunc.definition) 
	        fprintf( stderr, "\t  A-formula: %s\n", 
			 sm_palette.Afunc.definition);
	    if (sm_palette.Bfunc.at && sm_palette.Bfunc.definition)
	        fprintf( stderr, "\t  B-formula: %s\n", 
			 sm_palette.Bfunc.definition);
	    if (sm_palette.Cfunc.at && sm_palette.Cfunc.definition)
	        fprintf( stderr, "\t  C-formula: %s\n", 
			 sm_palette.Cfunc.definition);
	    break;
	  default:
	    fprintf( stderr, "%s:%d oops: Unknown color mode '%c'.\n",
		     __FILE__, __LINE__, (char)(sm_palette.colorMode) );
	}
	fprintf(stderr,"\tfigure is %s\n",
	    sm_palette.positive == SMPAL_POSITIVE ? "POSITIVE" : "NEGATIVE");
	fprintf( stderr, 
           "\tall color formulae ARE%s written into output postscript file\n",
		 sm_palette.ps_allcF == 0 ? " NOT" : "");
	fputs("\tallocating ", stderr);
	if (sm_palette.use_maxcolors)
	    fprintf(stderr,"MAX %i",sm_palette.use_maxcolors);
	else
	    fputs("ALL remaining", stderr);
	fputs(" color positions for discrete palette terminals\n", stderr);
	fputs( "\tColor-Model: ", stderr );
	switch( sm_palette.cmodel ) {
	case C_MODEL_RGB: fputs( "RGB\n", stderr ); break;
	case C_MODEL_HSV: fputs( "HSV\n", stderr ); break;
	case C_MODEL_CMY: fputs( "CMY\n", stderr ); break;
	case C_MODEL_YIQ: fputs( "YIQ\n", stderr ); break;
	case C_MODEL_XYZ: fputs( "XYZ\n", stderr ); break;
	default:
	  fprintf( stderr, "%s:%d ooops: Unknown color mode '%c'.\n",
		   __FILE__, __LINE__, (char)(sm_palette.cmodel) );
	}
	fprintf(stderr,"\tgamma is %.4g\n", sm_palette.gamma );
	return;
    }

    if (almost_equals(c_token, "pal$ette")) {
        /* 'show palette palette <n>' */
        show_palette_palette();
	return;
    }
    else if (almost_equals(c_token, "gra$dient")) {
        /* 'show palette gradient' */
        show_palette_gradient();
	return;
    }
    else if (almost_equals(c_token, "rgbfor$mulae" )) {
        /* 'show palette rgbformulae' */
        show_palette_rgbformulae();
	return;
    }
    else if (almost_equals(c_token, "color$names" )) {
        /* 'show palette colornames' */
        show_palette_colornames();
	return;
    }
    else if (almost_equals(c_token, "fit2rgb$formulae" )) {
        /* 'show palette fit2rgbformulae' */
	show_palette_fit2rgbformulae();
	return;
    }
    else { /* wrong option to "show palette" */
        int_error( c_token, "Required 'show palette' or 'show palette gradient' or\n\t 'show palette palette <n>' or 'show palette rgbformulae' or\n\t 'show palette colornames'.");
    }
}


static void
show_colorbox()
{
    c_token++;
    if (color_box.border) {
	fputs("\tcolor box with border, ", stderr);
	if (color_box.border_lt_tag >= 0)
	    fprintf(stderr,"line type %d is ", color_box.border_lt_tag);
	else
	    fputs("DEFAULT line type is ", stderr);
    } else {
	fputs("\tcolor box without border is ", stderr);
    }
    switch (color_box.where) {
	case SMCOLOR_BOX_NO:
	    fputs("NOT drawn\n", stderr);
	    break;
	case SMCOLOR_BOX_DEFAULT:
	    fputs("drawn at DEFAULT position\n", stderr);
	    break;
	case SMCOLOR_BOX_USER:
	    fputs("drawn at USER position:\n", stderr);
	    fprintf(stderr,"\t\torigin: %g, %g\n", color_box.xorigin, color_box.yorigin);
	    fprintf(stderr,"\t\tsize  : %g, %g\n", color_box.xsize  , color_box.ysize  );
	    break;
	default: /* should *never* happen */
	    int_error(NO_CARET, "Argh !");
    }
    fprintf(stderr,"\tcolor gradient is %s in the color box\n",
	color_box.rotation == 'v' ? "VERTICAL" : "HORIZONTAL");
}


static void
show_pm3d()
{
    c_token++;
    if (!pm3d.where[0]) {
	fputs("\tpm3d is OFF\n", stderr);
	return;
    }
    fputs("\tpm3d plotted at ", stderr);
    { int i=0;
	for ( ; pm3d.where[i]; i++ ) {
	    if (i>0) fputs(", then ", stderr);
	    switch (pm3d.where[i]) {
		case PM3D_AT_BASE: fputs("BOTTOM", stderr); break;
		case PM3D_AT_SURFACE: fputs("SURFACE", stderr); break;
		case PM3D_AT_TOP: fputs("TOP", stderr); break;
	    }
	}
	fputs("\n", stderr);
    }
    if (pm3d.direction != PM3D_SCANS_AUTOMATIC) {
	fprintf(stderr,"\ttaking scans in %s direction\n",
	    pm3d.direction == PM3D_SCANS_FORWARD ? "FORWARD" : "BACKWARD");
    } else {
	fputs("\ttaking scans direction automatically\n", stderr);
    }
    fputs("\tsubsequent scans with different nb of pts are ", stderr);
    if (pm3d.flush == PM3D_FLUSH_CENTER) fputs("CENTERED\n", stderr);
    else fprintf(stderr,"flushed from %s\n",
	pm3d.flush == PM3D_FLUSH_BEGIN ? "BEGIN" : "END");
    fprintf(stderr,"\tflushing triangles are %sdrawn\n",
	pm3d.ftriangles ? "" : "not ");
    fputs("\tclipping: ", stderr);
    if (pm3d.clip == PM3D_CLIP_1IN)
	fputs("at least 1 point of the quadrangle in x,y ranges\n", stderr);
    else
	fputs( "all 4 points of the quadrangle in x,y ranges\n", stderr);
    if (pm3d.hidden3d_tag) {
	fprintf(stderr,"\tpm3d-hidden3d is on an will use linestyle %d\n",
	    pm3d.hidden3d_tag);
    } else {
	fputs("\tpm3d-hidden3d is off\n", stderr);
    }
    if (pm3d.solid) {
	fputs("\tborders, tics and labels may be hidden by the surface\n", stderr);
    } else {
	fputs("\tsurface is transparent for borders, tics and labels\n", stderr);
    }
    fprintf(stderr,"\tpm3d style is %s\n", PM3D_IMPLICIT == pm3d.implicit ? "implicit" : "explicit");
    fprintf(stderr,"\tquadrangle color according to ");
    switch (pm3d.which_corner_color) {
	case PM3D_WHICHCORNER_MEAN: fputs("averaged 4 corners\n", stderr); break;
	case PM3D_WHICHCORNER_GEOMEAN: fputs("geometrical mean of 4 corners\n", stderr); break;
	case PM3D_WHICHCORNER_MEDIAN: fputs("median of 4 corners\n", stderr); break;
	default: fprintf(stderr, "corner %i\n", pm3d.which_corner_color - PM3D_WHICHCORNER_C1 + 1);
    }
}

#endif

/* process 'show pointsize' command */
static void
show_pointsize()
{
    SHOW_ALL_NL;
    fprintf(stderr, "\tpointsize is %g\n", pointsize);
}


/* process 'show encoding' command */
static void
show_encoding()
{
    SHOW_ALL_NL;
    fprintf(stderr, "\tencoding is %s\n", encoding_names[encoding]);
}


/* process 'show decimalsign' command */
static void
show_decimalsign()
{
    SHOW_ALL_NL;
    if (decimalsign!=NULL)
        fprintf(stderr, "\tdecimalsign is '%s'\n", decimalsign);
    else
        fprintf(stderr, "\tdecimalsign has default value (normally '.')\n");
}


/* process 'show fit' command */
static void
show_fit()
{
    SHOW_ALL_NL;

#ifdef GP_FIT_ERRVARS
    fprintf(stderr, "\
\tfit will%s place parameter errors in variables\n",
	    fit_errorvariables ? "" : " not");
#endif /* GP_FIT_ERRVARS */

    if (fitlogfile != NULL) {
        fprintf(stderr, "\
\tlog-file for fits is was set by the user to be \n\
\t'%s'\n", fitlogfile);
    } else {
	char *logfile = getfitlogfile();

	if (logfile) {
	    fprintf(stderr, "\
\tlog-file for fits is unchanged from the environment default of\n\
\t'%s'\n", logfile);
	    free(logfile);
	}
    }
}


/* process 'show polar' command */
static void
show_polar()
{
    SHOW_ALL_NL;
    fprintf(stderr, "\tpolar is %s\n", (polar) ? "ON" : "OFF");
}


/* process 'show angles' command */
static void
show_angles()
{
    SHOW_ALL_NL;

    fputs("\tAngles are in ", stderr);
    if (ang2rad == 1) {
	fputs("radians\n", stderr);
    } else {
	fputs("degrees\n", stderr);
    }
}


/* process 'show samples' command */
static void
show_samples()
{
    SHOW_ALL_NL;
    fprintf(stderr, "\tsampling rate is %d, %d\n", samples_1, samples_2);
}


/* process 'show isosamples' command */
static void
show_isosamples()
{
    SHOW_ALL_NL;
    fprintf(stderr, "\tiso sampling rate is %d, %d\n",
	    iso_samples_1, iso_samples_2);
}


/* process 'show view' command */
static void
show_view()
{
    SHOW_ALL_NL;
    fputs("\tview is ", stderr);
    if (splot_map == TRUE) {
	fputs("map\n", stderr);
	return;
    }
    fprintf(stderr, "%g rot_x, %g rot_z, %g scale, %g scale_z\n",
		surface_rot_x, surface_rot_z, surface_scale, surface_zscale);
}


/* process 'show surface' command */
static void
show_surface()
{
    SHOW_ALL_NL;
    fprintf(stderr, "\tsurface is %sdrawn\n", draw_surface ? "" : "not ");
}


/* process 'show hidden3d' command */
static void
show_hidden3d()
{
    SHOW_ALL_NL;

#ifdef LITE
    printf(" Hidden Line Removal Not Supported in LITE version\n");
#else
    fprintf(stderr, "\thidden surface is %s\n",
	    hidden3d ? "removed" : "drawn");
    show_hidden3doptions();
#endif /* LITE */
}


#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
/* process 'show historysize' command */
static void
show_historysize()
{
    if (gnuplot_history_size >= 0) {
	fprintf(stderr, "\thistory size: %ld\n", gnuplot_history_size);
    } else {
	fprintf(stderr, "\thistory will not be truncated.\n");
    }
}
#endif


/* process 'show size' command */
static void
show_size()
{
    SHOW_ALL_NL;

    fprintf(stderr, "\tsize is scaled by %g,%g\n", xsize, ysize);
    if (aspect_ratio > 0)
	fprintf(stderr, "\tTry to set aspect ratio to %g:1.0\n", aspect_ratio);
    else if (aspect_ratio == 0)
	fputs("\tNo attempt to control aspect ratio\n", stderr);
    else
	fprintf(stderr, "\tTry to set LOCKED aspect ratio to %g:1.0\n",
		-aspect_ratio);
}


/* process 'show origin' command */
static void
show_origin()
{
    SHOW_ALL_NL;
    fprintf(stderr, "\torigin is set to %g,%g\n", xoffset, yoffset);
}


/* process 'show term' command */
static void
show_term()
{
    SHOW_ALL_NL;

    if (term)
	fprintf(stderr, "   terminal type is %s %s\n",
		term->name, term_options);
    else
	fputs("\tterminal type is unknown\n", stderr);
}


/* process 'show tics|[xyzx2y2cb]tics' commands */
static void
#ifdef PM3D
show_tics(showx, showy, showz, showx2, showy2, showcb)
#else
show_tics(showx, showy, showz, showx2, showy2)
#endif
TBOOLEAN showx, showy, showz, showx2, showy2;
#ifdef PM3D
TBOOLEAN showcb;
#endif
{
    SHOW_ALL_NL;

    fprintf(stderr, "\ttics are %s, \
\tticslevel is %g\n\
\tmajor ticscale is %g and minor ticscale is %g\n",
	    (tic_in ? "IN" : "OUT"),
	    ticslevel,
	    ticscale, miniticscale);

    if (showx)
	show_ticdef(FIRST_X_AXIS);
    if (showx2)
	show_ticdef(SECOND_X_AXIS);
    if (showy)
	show_ticdef(FIRST_Y_AXIS);
    if (showy2)
	show_ticdef(SECOND_Y_AXIS);
    if (showz)
	show_ticdef(FIRST_Z_AXIS);
#ifdef PM3D
    if (showcb)
	show_ticdef(COLOR_AXIS);
#endif
    screen_ok = FALSE;
}


/* process 'show m[xyzx2y2cb]tics' commands */
static void
show_mtics(axis)
    AXIS_INDEX axis;
{
    switch (axis_array[axis].minitics) {
    case MINI_OFF:
	fprintf(stderr, "\tminor %stics are off\n", axis_defaults[axis].name);
	break;
    case MINI_DEFAULT:
	fprintf(stderr, "\
\tminor %stics are off for linear scales\n\
\tminor %stics are computed automatically for log scales\n", axis_defaults[axis].name, axis_defaults[axis].name);
	break;
    case MINI_AUTO:
	fprintf(stderr, "\tminor %stics are computed automatically\n", axis_defaults[axis].name);
	break;
    case MINI_USER:
	fprintf(stderr, "\
\tminor %stics are drawn with %d subintervals between major xtic marks\n",
		axis_defaults[axis].name, (int) axis_array[axis].mtic_freq);
	break;
    default:
	int_error(NO_CARET, "Unknown minitic type in show_mtics()");
    }
}


/* process 'show timestamp' command */
static void
show_timestamp()
{
    SHOW_ALL_NL;
    show_xyzlabel("", "time", &timelabel);
    fprintf(stderr, "\twritten in %s corner\n",
	    (timelabel_bottom ? "bottom" : "top"));
    if (timelabel_rotate)
	fputs("\trotated if the terminal allows it\n\t", stderr);
    else
	fputs("\tnot rotated\n\t", stderr);
}


/* process 'show [xyzx2y2rtuv]range' commands */
static void
show_range(axis)
    AXIS_INDEX axis;
{
    /* this probably ought to just invoke save_range(stderr) from misc.c
     * since I think it is identical
     */
    SHOW_ALL_NL;

    if (axis_array[axis].is_timedata)
	fprintf(stderr, "\tset %sdata time\n", axis_defaults[axis].name);
    fprintf(stderr, "\tset %srange [", axis_defaults[axis].name);
    if (axis_array[axis].set_autoscale & AUTOSCALE_MIN) {
	fputc('*', stderr);
    } else {
	SHOW_NUM_OR_TIME(axis_array[axis].set_min, axis);
    }
    fputs(" : ", stderr);
    if (axis_array[axis].set_autoscale & AUTOSCALE_MAX) {
	fputc('*', stderr);
    } else {
	SHOW_NUM_OR_TIME(axis_array[axis].set_max, axis);
    }
    fprintf(stderr, "] %sreverse %swriteback",
	    (axis_array[axis].range_flags & RANGE_REVERSE) ? "" : "no",
	    (axis_array[axis].range_flags & RANGE_WRITEBACK) ? "" : "no");

    if (axis_array[axis].set_autoscale & AUTOSCALE_BOTH) {
	/* add current (hidden) range as comments */
	fputs("  # (currently [", stderr);
	if (axis_array[axis].set_autoscale & AUTOSCALE_MIN) {
	    SHOW_NUM_OR_TIME(axis_array[axis].set_min, axis);
	}
	putc(':', stderr);
	if (axis_array[axis].set_autoscale & AUTOSCALE_MAX) {
	    SHOW_NUM_OR_TIME(axis_array[axis].set_max, axis);
	}
	fputs("] )\n", stderr);
	if (axis_array[axis].set_autoscale & AUTOSCALE_FIXMIN)
	    fputs("\t\tLower end will not be extended to ticstep boundary\n", stderr);
	if (axis_array[axis].set_autoscale & AUTOSCALE_FIXMAX)
	    fputs("\t\tUpper end will not be extended to ticstep boundary\n", stderr);
    } else {
	putc('\n', stderr);
    }
}


/* called by the functions below */
static void
show_xyzlabel(name, suffix, label)
    const char *name, *suffix;
    label_struct *label;
{
    fprintf(stderr, "\t%s%s is \"%s\", offset at %f, %f",
	    name, suffix, conv_text(label->text),
	    label->xoffset, label->yoffset);

    if (label->font[0])
	fprintf(stderr, ", using font \"%s\"", conv_text(label->font));

    if (label->textcolor.type == TC_LT)
    	fprintf(stderr,", textcolor lt %d", label->textcolor.lt+1);

    putc('\n', stderr);
}


/* process 'show title' command */
static void
show_title()
{
    SHOW_ALL_NL;
    show_xyzlabel("","title", &title);
}


/* process 'show {x|y|z|x2|y2}label' command */
static void
show_axislabel(axis)
    AXIS_INDEX axis;
{
    SHOW_ALL_NL;
    show_xyzlabel(axis_defaults[axis].name, "label", &axis_array[axis].label);
}


/* process 'show [xyzx2y2]data' commands */
static void
show_data_is_timedate(axis)
    AXIS_INDEX axis;
{
    SHOW_ALL_NL;
    fprintf(stderr, "\t%s is set to %s\n", axis_defaults[axis].name,
	    axis_array[axis].is_timedata ? "time" : "numerical");
}


/* process 'show timefmt' command */
static void
show_timefmt()
{
    int axis;

    SHOW_ALL_NL;

    if ((axis = lookup_table(axisname_tbl, c_token)) >= 0) {
	c_token++;
	fprintf(stderr, "\tread format for time on %s axis is \"%s\"\n",
		axis_defaults[axis].name,
		conv_text(axis_array[axis].timefmt));
    } else {
        /* show all currently active time axes' formats: */
	for (axis = 0; axis<AXIS_ARRAY_SIZE; axis++)
	    if (axis_array[axis].is_timedata) 
		fprintf(stderr,
			"\tread format for time on %s axis is \"%s\"\n",
			axis_defaults[axis].name,
			conv_text(axis_array[axis].timefmt));
    }
}


/* process 'show locale' command */
static void
show_locale()
{
    SHOW_ALL_NL;
    locale_handler(ACTION_SHOW,NULL);
}


/* process 'show loadpath' command */
static void
show_loadpath()
{
    SHOW_ALL_NL;
    loadpath_handler(ACTION_SHOW,NULL);
}


/* process 'show fontpath' command */
static void
show_fontpath()
{
    SHOW_ALL_NL;
    fontpath_handler(ACTION_SHOW,NULL);
}


/* process 'show zero' command */
static void
show_zero()
{
    SHOW_ALL_NL;
    fprintf(stderr, "\tzero is %g\n", zero);
}


/* process 'show datafile' command */
static void
show_datafile()
{
    SHOW_ALL_NL;

    if (missing_val == NULL)
	fputs("\tNo missing data string set for datafile\n", stderr);
    else
	fprintf(stderr, "\t\"%s\" in datafile is interpreted as missing value\n",
		missing_val);
    if (df_separator != '\0')
	fprintf(stderr, "\tdatafile fields separated by \"%c\"\n", df_separator);
    else
	fprintf(stderr, "\tdatafile fields separated by whitespace\n");
    fprintf(stderr, "\tComments chars are \"%s\"\n", df_commentschars);
}

#ifdef USE_MOUSE
/* process 'show mouse' command */
static void
show_mouse()
{
    SHOW_ALL_NL;
    if (mouse_setting.on) {
	fprintf(stderr, "\tmouse is on\n");
	if (mouse_setting.annotate_zoom_box) {
	    fprintf(stderr, "\tzoom coordinates will be drawn\n");
	} else {
	    fprintf(stderr, "\tno zoom coordinates will be drawn\n");
	}
	if (mouse_setting.polardistance) {
	    fprintf(stderr, "\tdistance to ruler will be show in polar coordinates\n");
	} else {
	    fprintf(stderr, "\tno polar distance to ruler will be shown\n");
	}
	if (mouse_setting.doubleclick > 0) {
	    fprintf(stderr, "\tdouble click resolution is %d ms\n",
		mouse_setting.doubleclick);
	} else {
	    fprintf(stderr, "\tdouble click resolution is off\n");
	}
	fprintf(stderr, "\tformatting numbers with \"%s\"\n",
	    mouse_setting.fmt);
	fprintf(stderr, "\tformat for Button 1 is %d\n", (int) clipboard_mode);
	if (clipboard_alt_string) {
	    fprintf(stderr, "\talternative format for Button 1 is '%s'\n",
		clipboard_alt_string);
	}
	fprintf(stderr, "\tformat for Button 2 is %d\n", (int) mouse_mode);
	if (mouse_alt_string) {
	    fprintf(stderr, "\talternative format for Button 2 is '%s'\n",
		mouse_alt_string);
	}
	if (mouse_setting.label) {
	    fprintf(stderr, "\tButton 2 draws labes with options \"%s\"\n",
		mouse_setting.labelopts);
	} else {
	    fprintf(stderr, "\tdrawing temporary annotation on Button 2\n");
	}
	fprintf(stderr, "\tzoomjump is %s\n",
	    mouse_setting.warp_pointer ? "on" : "off");
	fprintf(stderr, "\tcommunication commands will %sbe shown\n",
	    mouse_setting.verbose ? "" : "not ");
    } else {
	fprintf(stderr, "\tmouse is off\n");
    }
}
#endif

/* process 'show plot' command */
static void
show_plot()
{
    SHOW_ALL_NL;
    fprintf(stderr, "\tlast plot command was: %s\n", replot_line);
}


/* process 'show variables' command */
static void
show_variables()
{
    register struct udvt_entry *udv = first_udv;
    int len;

    SHOW_ALL_NL;

    fputs("\n\tVariables:\n", stderr);
    while (udv) {
	len = strcspn(udv->udv_name, " ");
	if (udv->udv_undef) {
	    FPRINTF((stderr, "\t%-*s is undefined\n", len, udv->udv_name));
	} else {
	    fprintf(stderr, "\t%-*s ", len, udv->udv_name);
	    fputs("= ", stderr);
	    disp_value(stderr, &(udv->udv_value));
	    (void) putc('\n', stderr);
	}
	udv = udv->next_udv;
    }
}


static void
show_linestyle(tag)
int tag;			/* 0 means show all */
{
    struct linestyle_def *this_linestyle;
    TBOOLEAN showed = FALSE;

    for (this_linestyle = first_linestyle; this_linestyle != NULL;
	 this_linestyle = this_linestyle->next) {
	if (tag == 0 || tag == this_linestyle->tag) {
	    showed = TRUE;
	    fprintf(stderr, "\tlinestyle %d, ", this_linestyle->tag);

#ifdef PM3D
	    if (this_linestyle->lp_properties.use_palette)
		fprintf(stderr, "linetype palette, ");
	    else
#endif
		fprintf(stderr, "linetype %d, ", this_linestyle->lp_properties.l_type + 1);

	    fprintf(stderr, "linewidth %.3f, pointtype %d, pointsize %.3f\n",
		    this_linestyle->lp_properties.l_width,
		    this_linestyle->lp_properties.p_type + 1,
		    this_linestyle->lp_properties.p_size);
	}
    }
    if (tag > 0 && !showed)
	int_error(c_token, "linestyle not found");
}


static void
show_arrowstyle(tag)
int tag;			/* 0 means show all */
{
    struct arrowstyle_def *this_arrowstyle;
    TBOOLEAN showed = FALSE;

    for (this_arrowstyle = first_arrowstyle; this_arrowstyle != NULL;
	 this_arrowstyle = this_arrowstyle->next) {
	if (tag == 0 || tag == this_arrowstyle->tag) {
	    showed = TRUE;
	    fprintf(stderr, "\tarrowstyle %d, ", this_arrowstyle->tag);
	    fflush(stderr);

	    fprintf(stderr, "\tlinetype %d, linewidth %.3f %s %s\n",
		    this_arrowstyle->arrow_properties.lp_properties.l_type+1,
		    this_arrowstyle->arrow_properties.lp_properties.l_width,
		    this_arrowstyle->arrow_properties.head ? 
		    (this_arrowstyle->arrow_properties.head==2 ? 
		     " both heads " : " one head ") : " nohead",
		    this_arrowstyle->arrow_properties.layer ? "front" : "back");
	    if (this_arrowstyle->arrow_properties.head > 0) {
		fprintf(stderr, "\t  arrow heads: %s, ",
		  ( (this_arrowstyle->arrow_properties.head_filled==2) ? "filled" :
		    ( (this_arrowstyle->arrow_properties.head_filled==1) ? "empty" :
		      "nofilled" )));
		if (this_arrowstyle->arrow_properties.head_length > 0) {
		    static char *msg[] =
			{"(first x axis) ", "(second x axis) ", 
			 "(graph units) ", "(screen units) "};
		    fprintf(stderr," length %s%g, angle %g deg", 
			    this_arrowstyle->arrow_properties.head_lengthunit == first_axes ? "" : msg[this_arrowstyle->arrow_properties.head_lengthunit],
			    this_arrowstyle->arrow_properties.head_length,
			    this_arrowstyle->arrow_properties.head_angle);
		    if (this_arrowstyle->arrow_properties.head_filled!=0)
			fprintf(stderr,", backangle %g deg", 
				this_arrowstyle->arrow_properties.head_backangle);
		    fprintf(stderr,"\n");
		}
		else
		    fprintf(stderr," (default length and angles)\n");
	    }
	}
    }
    if (tag > 0 && !showed)
	int_error(c_token, "arrowstyle not found");
}


/* called by show_tics */
static void
show_ticdef(axis)
AXIS_INDEX axis;
{
    register struct ticmark *t;

    const char *ticfmt = conv_text(axis_array[axis].formatstring);

    fprintf(stderr, "\t%s-axis tics:\t", axis_defaults[axis].name);
    switch (axis_array[axis].ticmode & TICS_MASK) {
    case NO_TICS:
	fputs("OFF\n", stderr);
	return;
    case TICS_ON_AXIS:
	fputs("on axis", stderr);
	if (axis_array[axis].ticmode & TICS_MIRROR)
	    fprintf(stderr, " and mirrored %s", (tic_in ? "OUT" : "IN"));
	break;
    case TICS_ON_BORDER:
	fputs("on border", stderr);
	if (axis_array[axis].ticmode & TICS_MIRROR)
	    fputs(" and mirrored on opposite border", stderr);
	break;
    }

    fprintf(stderr, "\n\t  labels are format \"%s\"", ticfmt);
    if (axis_array[axis].tic_rotate) {
	fprintf(stderr," rotated");
	fprintf(stderr," by %d",axis_array[axis].tic_rotate);
	fputs(" in 2D mode, terminal permitting.\n\t", stderr);
    } else
	fputs(" and are not rotated\n\t", stderr);

    switch (axis_array[axis].ticdef.type) {
    case TIC_COMPUTED:{
	    fputs("  intervals computed automatically\n", stderr);
	    break;
	}
    case TIC_MONTH:{
	    fputs("  Months computed automatically\n", stderr);
	    break;
	}
    case TIC_DAY:{
	    fputs("  Days computed automatically\n", stderr);
	    break;
	}
    case TIC_SERIES:{
	    fputs("  series", stderr);
	    if (axis_array[axis].ticdef.def.series.start != -VERYLARGE) {
		fputs(" from ", stderr);
		SHOW_NUM_OR_TIME(axis_array[axis].ticdef.def.series.start, axis);
	    }
	    fprintf(stderr, " by %g%s", axis_array[axis].ticdef.def.series.incr,
		    axis_array[axis].is_timedata ? " secs" : "");
	    if (axis_array[axis].ticdef.def.series.end != VERYLARGE) {
		fputs(" until ", stderr);
		SHOW_NUM_OR_TIME(axis_array[axis].ticdef.def.series.end, axis);
	    }
	    putc('\n', stderr);
	    break;
	}
    case TIC_USER:{
/* this appears to be unused? lh
	    int time;
	    time = (axis_array[axis].is_timedata);
 */
	    fputs("  list (", stderr);
	    for (t = axis_array[axis].ticdef.def.user; t != NULL; t = t->next) {
		if (t->label)
		    fprintf(stderr, "\"%s\" ", conv_text(t->label));
		SHOW_NUM_OR_TIME(t->position, axis);
		if (t->next)
		    fputs(", ", stderr);
	    }
	    fputs(")\n", stderr);
	    break;
	}
    default:{
	    int_error(NO_CARET, "unknown ticdef type in show_ticdef()");
	    /* NOTREACHED */
	}
    }
 
    if (axis_array[axis].ticdef.textcolor.type != TC_DEFAULT) {
        fprintf(stderr,"\t  textcolor lt %d\n",
            axis_array[axis].ticdef.textcolor.lt+1);
    }
 
    if (axis_array[axis].ticdef.font && *axis_array[axis].ticdef.font) {
        fprintf(stderr,"\t  font \"%s\"\n", axis_array[axis].ticdef.font);
    }
}


/* convert unprintable characters as \okt, tab as \t, newline \n .. */
char *
conv_text(t)
const char *t;
{
    static char *r = NULL, *s;

    if (t==NULL) return NULL;

    /* is this enough? */
    r = gp_realloc(r, 4 * (strlen(t) + 1), "conv_text buffer");

    s = r;

    while (*t != NUL) {
	switch (*t) {
	case '\t':
	    *s++ = '\\';
	    *s++ = 't';
	    break;
	case '\n':
	    *s++ = '\\';
	    *s++ = 'n';
	    break;
#ifndef OSK
	case '\r':
	    *s++ = '\\';
	    *s++ = 'r';
	    break;
#endif
	case '"':
	case '\\':
	    *s++ = '\\';
	    *s++ = *t;
	    break;

	default:{
		if (isprint((unsigned char)*t))
		    *s++ = *t;
		else {
		    *s++ = '\\';
		    sprintf(s, "%03o", *t);
		    while (*s != NUL)
			s++;
		}
	    }
	    break;
	}
	t++;
    }
    *s = NUL;
    return r;
}
