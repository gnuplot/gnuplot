#ifndef lint
static char *RCSid() { return RCSid("$Id: set.c,v 1.255 2007/12/06 06:27:07 sfeam Exp $"); }
#endif

/* GNUPLOT - set.c */

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
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 */

#include "setshow.h"

#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "contour.h"
#include "datafile.h"
#include "fit.h"
#include "gadgets.h"
#include "gp_hist.h"
#include "gp_time.h"
#include "hidden3d.h"
#include "misc.h"
/* #include "parse.h" */
#include "plot.h"
#include "plot2d.h"
#include "plot3d.h"
#include "tables.h"
#include "term_api.h"
#include "util.h"
#include "variable.h"
#include "pm3d.h"
#include "getcolor.h"
#include <ctype.h>

static palette_color_mode pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_NONE;

static void set_angles __PROTO((void));
static void set_arrow __PROTO((void));
static int assign_arrow_tag __PROTO((void));
static void set_autoscale __PROTO((void));
static void set_bars __PROTO((void));
static void set_border __PROTO((void));
static void set_boxwidth __PROTO((void));
static void set_clabel __PROTO((void));
static void set_clip __PROTO((void));
static void set_cntrparam __PROTO((void));
static void set_contour __PROTO((void));
static void set_dgrid3d __PROTO((void));
static void set_decimalsign __PROTO((void));
static void set_dummy __PROTO((void));
static void set_encoding __PROTO((void));
static void set_fit __PROTO((void));
static void set_format __PROTO((void));
static void set_grid __PROTO((void));
static void set_hidden3d __PROTO((void));
#ifdef GNUPLOT_HISTORY
static void set_historysize __PROTO((void));
#endif
static void set_isosamples __PROTO((void));
static void set_key __PROTO((void));
static void set_keytitle __PROTO((void));
static void set_label __PROTO((void));
static int assign_label_tag __PROTO((void));
static void set_loadpath __PROTO((void));
static void set_fontpath __PROTO((void));
static void set_locale __PROTO((void));
static void set_logscale __PROTO((void));
#ifdef GP_MACROS
static void set_macros __PROTO((void));
#endif
static void set_mapping __PROTO((void));
static void set_margin __PROTO((t_position *));
static void set_missing __PROTO((void));
static void set_separator __PROTO((void));
static void set_datafile_commentschars __PROTO((void));
#ifdef USE_MOUSE
static void set_mouse __PROTO((void));
#endif
static void set_offsets __PROTO((void));
static void set_origin __PROTO((void));
static void set_output __PROTO((void));
static void set_parametric __PROTO((void));
static void set_pm3d __PROTO((void));
static void set_palette __PROTO((void));
static void set_colorbox __PROTO((void));
static void set_pointsize __PROTO((void));
static void set_polar __PROTO((void));
static void set_print __PROTO((void));
#ifdef EAM_OBJECTS
static void set_object __PROTO((void));
static void set_rectangle __PROTO((int));
#endif
static void set_samples __PROTO((void));
static void set_size __PROTO((void));
static void set_style __PROTO((void));
static void set_surface __PROTO((void));
static void set_table __PROTO((void));
static void set_terminal __PROTO((void));
static void set_termoptions __PROTO((void));
static void set_tics __PROTO((void));
static void set_ticscale __PROTO((void));
static void set_timefmt __PROTO((void));
static void set_timestamp __PROTO((void));
static void set_view __PROTO((void));
static void set_zero __PROTO((void));
static void set_timedata __PROTO((AXIS_INDEX));
static void set_range __PROTO((AXIS_INDEX));
static void set_xyplane __PROTO((void));
static void set_ticslevel __PROTO((void));
static void set_zeroaxis __PROTO((AXIS_INDEX));
static void set_allzeroaxis __PROTO((void));


/******** Local functions ********/

static void set_xyzlabel __PROTO((text_label * label));
static void load_tics __PROTO((AXIS_INDEX axis));
static void load_tic_user __PROTO((AXIS_INDEX axis));
static void load_tic_series __PROTO((AXIS_INDEX axis));
static void load_offsets __PROTO((double *a, double *b, double *c, double *d));

static void set_linestyle __PROTO((void));
static int assign_linestyle_tag __PROTO((void));
static void set_arrowstyle __PROTO((void));
static int assign_arrowstyle_tag __PROTO((void));
static int looks_like_numeric __PROTO((char *));
static int set_tic_prop __PROTO((AXIS_INDEX));
static char *fill_numbers_into_string __PROTO((char *pattern));

static void check_palette_grayscale __PROTO((void));
static int set_palette_defined __PROTO((void));
static void set_palette_file __PROTO((void));
static void set_palette_function __PROTO((void));
#ifdef EAM_HISTOGRAMS
static void parse_histogramstyle __PROTO((histogram_style *hs, 
		t_histogram_type def_type, int def_gap));
#endif

static struct position default_position
	= {first_axes, first_axes, first_axes, 0., 0., 0.};

#ifdef BACKWARDS_COMPATIBLE
static void set_nolinestyle __PROTO((void));
#endif

/******** The 'set' command ********/
void
set_command()
{
    static char GPFAR setmess[] =
    "valid set options:  [] = choose one, {} means optional\n\n\
\t'angles', 'arrow', 'autoscale', 'bars', 'border', 'boxwidth',\n\
\t'clabel', 'clip', 'cntrparam', 'colorbox', 'contour', 'decimalsign',\n\
\t'dgrid3d', 'dummy', 'encoding', 'fit', 'format', 'grid',\n\
\t'hidden3d', 'historysize', 'isosamples', 'key', 'label', 'locale',\n\
\t'logscale', 'macros', '[blrt]margin', 'mapping', 'mouse', 'multiplot',\n\
\t'offsets', 'origin', 'output', 'palette', 'parametric', 'pm3d',\n\
\t'pointsize', 'polar', 'print', '[rtuv]range', 'samples', 'size',\n\
\t'style', 'surface', 'terminal', 'tics', 'ticscale', 'ticslevel',\n\
\t'timestamp', 'timefmt', 'title', 'view', 'xyplane', '[xyz]{2}data',\n\
\t'[xyz]{2}label', '[xyz]{2}range', '{no}{m}[xyz]{2}tics',\n\
\t'[xyz]{2}[md]tics', '{[xyz]{2}}zeroaxis', 'zero'";

    c_token++;

#ifdef BACKWARDS_COMPATIBLE

    /* retain backwards compatibility to the old syntax for now
     * Oh, such ugliness ...
     */

    if (almost_equals(c_token,"da$ta")) {
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"set style data\"");
	if (!almost_equals(++c_token,"s$tyle"))
	    int_error(c_token,"expecting keyword 'style'");
	else
	    data_style = get_style();
    } else if (almost_equals(c_token,"fu$nction")) {
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"set style function\"");
	if (!almost_equals(++c_token,"s$tyle"))
	    int_error(c_token,"expecting keyword 'style'");
	else {
	    enum PLOT_STYLE temp_style = get_style();

	    if (temp_style & PLOT_STYLE_HAS_ERRORBAR)
		int_error(c_token, "style not usable for function plots, left unchanged");
#ifdef EAM_HISTOGRAMS
	    else if (temp_style == HISTOGRAMS)
		int_error(c_token, "style not usable for function plots, left unchanged");
#endif
	    else
		func_style = temp_style;
	}
    } else if (almost_equals(c_token,"li$nestyle") || equals(c_token, "ls" )) {
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"set style line\"");
	c_token++;
	set_linestyle();
    } else if (almost_equals(c_token,"noli$nestyle") || equals(c_token, "nols" )) {
	c_token++;
	set_nolinestyle();
    } else if (gp_input_line[token[c_token].start_index] == 'n' &&
	       gp_input_line[token[c_token].start_index+1] == 'o') {
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"unset\"");
	token[c_token].start_index += 2;
	token[c_token].length -= 2;
	c_token--;
	unset_command();
    } else if (almost_equals(c_token,"miss$ing")) {
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"set datafile missing\"");
	set_missing();
    } else {

#else	/* Milder form of backwards compatibility */
	/* Allow "set no{foo}" rather than "unset foo" */ 
    if (gp_input_line[token[c_token].start_index] == 'n' &&
	       gp_input_line[token[c_token].start_index+1] == 'o') {
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"unset\"");
	token[c_token].start_index += 2;
	token[c_token].length -= 2;
	c_token--;
	unset_command();
    } else {

#endif /* BACKWARDS_COMPATIBLE */

	switch(lookup_table(&set_tbl[0],c_token)) {
	case S_ANGLES:
	    set_angles();
	    break;
	case S_ARROW:
	    set_arrow();
	    break;
	case S_AUTOSCALE:
	    set_autoscale();
	    break;
	case S_BARS:
	    set_bars();
	    break;
	case S_BORDER:
	    set_border();
	    break;
	case S_BOXWIDTH:
	    set_boxwidth();
	    break;
	case S_CLABEL:
	    set_clabel();
	    break;
	case S_CLIP:
	    set_clip();
	    break;
	case S_CNTRPARAM:
	    set_cntrparam();
	    break;
	case S_CONTOUR:
	    set_contour();
	    break;
	case S_DGRID3D:
	    set_dgrid3d();
	    break;
	case S_DECIMALSIGN:
	    set_decimalsign();
	    break;
	case S_DUMMY:
	    set_dummy();
	    break;
	case S_ENCODING:
	    set_encoding();
	    break;
	case S_FIT:
	    set_fit();
	    break;
	case S_FONTPATH:
	    set_fontpath();
	    break;
	case S_FORMAT:
	    set_format();
	    break;
	case S_GRID:
	    set_grid();
	    break;
	case S_HIDDEN3D:
	    set_hidden3d();
	    break;
	case S_HISTORYSIZE:
#ifdef GNUPLOT_HISTORY
	    set_historysize();
#else
	    int_error(c_token, "Command 'set historysize' requires history support.");
#endif
	    break;
	case S_ISOSAMPLES:
	    set_isosamples();
	    break;
	case S_KEY:
	    set_key();
	    break;
	case S_KEYTITLE:
	    set_keytitle();
	    break;
	case S_LABEL:
	    set_label();
	    break;
	case S_LOADPATH:
	    set_loadpath();
	    break;
	case S_LOCALE:
	    set_locale();
	    break;
	case S_LOGSCALE:
	    set_logscale();
	    break;
#ifdef GP_MACROS
	case S_MACROS:
	    set_macros();
	    break;
#endif
	case S_MAPPING:
	    set_mapping();
	    break;
	case S_BMARGIN:
	    set_margin(&bmargin);
	    break;
	case S_LMARGIN:
	    set_margin(&lmargin);
	    break;
	case S_RMARGIN:
	    set_margin(&rmargin);
	    break;
	case S_TMARGIN:
	    set_margin(&tmargin);
	    break;
	case S_DATAFILE:
	    if (almost_equals(++c_token,"miss$ing"))
		set_missing();
	    else if (almost_equals(c_token,"sep$arator"))
		set_separator();
	    else if (almost_equals(c_token,"com$mentschars"))
		set_datafile_commentschars();
#ifdef BINARY_DATA_FILE
	    else if (almost_equals(c_token,"bin$ary"))
		df_set_datafile_binary();
#endif
	    else if (almost_equals(c_token,"fort$ran")) {
		df_fortran_constants = TRUE;
		c_token++;
	    } else if (almost_equals(c_token,"nofort$ran")) {
		df_fortran_constants = FALSE;
		c_token++;
	    } else
		int_error(c_token,"expecting datafile modifier");
	    break;
#ifdef USE_MOUSE
	case S_MOUSE:
	    set_mouse();
	    break;
#endif
	case S_MULTIPLOT:
	    term_start_multiplot();
	    break;
	case S_OFFSETS:
	    set_offsets();
	    break;
	case S_ORIGIN:
	    set_origin();
	    break;
	case SET_OUTPUT:
	    set_output();
	    break;
	case S_PARAMETRIC:
	    set_parametric();
	    break;
	case S_PM3D:
	    set_pm3d();
	    break;
	case S_PALETTE:
	    set_palette();
	    break;
	case S_COLORBOX:
	    set_colorbox();
	    break;
	case S_POINTSIZE:
	    set_pointsize();
	    break;
	case S_POLAR:
	    set_polar();
	    break;
	case S_PRINT:
	    set_print();
	    break;
#ifdef EAM_OBJECTS
	case S_OBJECT:
	    set_object();
	    break;
#endif
	case S_SAMPLES:
	    set_samples();
	    break;
	case S_SIZE:
	    set_size();
	    break;
	case S_STYLE:
	    set_style();
	    break;
	case S_SURFACE:
	    set_surface();
	    break;
	case S_TABLE:
	    set_table();
	    break;
	case S_TERMINAL:
	    set_terminal();
	    break;
	case S_TERMOPTIONS:
	    set_termoptions();
	    break;
	case S_TICS:
	    set_tics();
	    break;
	case S_TICSCALE:
	    set_ticscale();
	    break;
	case S_TIMEFMT:
	    set_timefmt();
	    break;
	case S_TIMESTAMP:
	    set_timestamp();
	    break;
	case S_TITLE:
	    set_xyzlabel(&title);
	    break;
	case S_VIEW:
	    set_view();
	    break;
	case S_ZERO:
	    set_zero();
	    break;
/* FIXME */
	case S_MXTICS:
	case S_NOMXTICS:
	case S_XTICS:
	case S_NOXTICS:
	case S_XDTICS:
	case S_NOXDTICS:
	case S_XMTICS:
	case S_NOXMTICS:
	    set_tic_prop(FIRST_X_AXIS);
	    break;
	case S_MYTICS:
	case S_NOMYTICS:
	case S_YTICS:
	case S_NOYTICS:
	case S_YDTICS:
	case S_NOYDTICS:
	case S_YMTICS:
	case S_NOYMTICS:
	    set_tic_prop(FIRST_Y_AXIS);
	    break;
	case S_MX2TICS:
	case S_NOMX2TICS:
	case S_X2TICS:
	case S_NOX2TICS:
	case S_X2DTICS:
	case S_NOX2DTICS:
	case S_X2MTICS:
	case S_NOX2MTICS:
	    set_tic_prop(SECOND_X_AXIS);
	    break;
	case S_MY2TICS:
	case S_NOMY2TICS:
	case S_Y2TICS:
	case S_NOY2TICS:
	case S_Y2DTICS:
	case S_NOY2DTICS:
	case S_Y2MTICS:
	case S_NOY2MTICS:
	    set_tic_prop(SECOND_Y_AXIS);
	    break;
	case S_MZTICS:
	case S_NOMZTICS:
	case S_ZTICS:
	case S_NOZTICS:
	case S_ZDTICS:
	case S_NOZDTICS:
	case S_ZMTICS:
	case S_NOZMTICS:
	    set_tic_prop(FIRST_Z_AXIS);
	    break;
	case S_MCBTICS:
	case S_NOMCBTICS:
	case S_CBTICS:
	case S_NOCBTICS:
	case S_CBDTICS:
	case S_NOCBDTICS:
	case S_CBMTICS:
	case S_NOCBMTICS:
	    set_tic_prop(COLOR_AXIS);
	    break;
	case S_XDATA:
	    set_timedata(FIRST_X_AXIS);
	    /* HBB 20000506: the old cod this this, too, although it
	     * serves no useful purpose, AFAICS */
	    /* HBB 20010201: Changed implementation to fix bug
	     * (c_token advanced too far) */
	    axis_array[T_AXIS].is_timedata
	      = axis_array[U_AXIS].is_timedata
	      = axis_array[FIRST_X_AXIS].is_timedata;
	    break;
	case S_YDATA:
	    set_timedata(FIRST_Y_AXIS);
	    /* dito */
	    axis_array[V_AXIS].is_timedata
	      = axis_array[FIRST_X_AXIS].is_timedata;
	    break;
	case S_ZDATA:
	    set_timedata(FIRST_Z_AXIS);
	    break;
	case S_CBDATA:
	    set_timedata(COLOR_AXIS);
	    break;
	case S_X2DATA:
	    set_timedata(SECOND_X_AXIS);
	    break;
	case S_Y2DATA:
	    set_timedata(SECOND_Y_AXIS);
	    break;
	case S_XLABEL:
	    set_xyzlabel(&axis_array[FIRST_X_AXIS].label);
	    break;
	case S_YLABEL:
	    set_xyzlabel(&axis_array[FIRST_Y_AXIS].label);
	    break;
	case S_ZLABEL:
	    set_xyzlabel(&axis_array[FIRST_Z_AXIS].label);
	    break;
	case S_CBLABEL:
	    set_xyzlabel(&axis_array[COLOR_AXIS].label);
	    break;
	case S_X2LABEL:
	    set_xyzlabel(&axis_array[SECOND_X_AXIS].label);
	    break;
	case S_Y2LABEL:
	    set_xyzlabel(&axis_array[SECOND_Y_AXIS].label);
	    break;
	case S_XRANGE:
	    set_range(FIRST_X_AXIS);
	    break;
	case S_X2RANGE:
	    set_range(SECOND_X_AXIS);
	    break;
	case S_YRANGE:
	    set_range(FIRST_Y_AXIS);
	    break;
	case S_Y2RANGE:
	    set_range(SECOND_Y_AXIS);
	    break;
	case S_ZRANGE:
	    set_range(FIRST_Z_AXIS);
	    break;
	case S_CBRANGE:
	    set_range(COLOR_AXIS);
	    break;
	case S_RRANGE:
	    set_range(R_AXIS);
	    break;
	case S_TRANGE:
	    set_range(T_AXIS);
	    break;
	case S_URANGE:
	    set_range(U_AXIS);
	    break;
	case S_VRANGE:
	    set_range(V_AXIS);
	    break;
	case S_XZEROAXIS:
	    set_zeroaxis(FIRST_X_AXIS);
	    break;
	case S_YZEROAXIS:
	    set_zeroaxis(FIRST_Y_AXIS);
	    break;
	case S_ZZEROAXIS:
	    set_zeroaxis(FIRST_Z_AXIS);
	    break;
	case S_X2ZEROAXIS:
	    set_zeroaxis(SECOND_X_AXIS);
	    break;
	case S_Y2ZEROAXIS:
	    set_zeroaxis(SECOND_Y_AXIS);
	    break;
	case S_ZEROAXIS:
	    set_allzeroaxis();
	    break;
	case S_XYPLANE:
	    set_xyplane();
	    break;
	case S_TICSLEVEL:
	    set_ticslevel();
	    break;
	default:
	    int_error(c_token, setmess);
	    break;
	}

    }
    update_gpval_variables(0); /* update GPVAL_ inner variables */

}


/* process 'set angles' command */
static void
set_angles()
{
    c_token++;
    if (END_OF_COMMAND) {
	/* assuming same as defaults */
	ang2rad = 1;
    } else if (almost_equals(c_token, "r$adians")) {
	c_token++;
	ang2rad = 1;
    } else if (almost_equals(c_token, "d$egrees")) {
	c_token++;
	ang2rad = DEG2RAD;
    } else
	int_error(c_token, "expecting 'radians' or 'degrees'");

    if (polar && axis_array[T_AXIS].set_autoscale) {
	/* set trange if in polar mode and no explicit range */
	axis_array[T_AXIS].set_min = 0;
	axis_array[T_AXIS].set_max = 2 * M_PI / ang2rad;
    }
}


/* process a 'set arrow' command */
/* set arrow {tag} {from x,y} {to x,y} {{no}head} ... */
/* allow any order of options - pm 25.11.2001 */
static void
set_arrow()
{
    struct arrow_def *this_arrow = NULL;
    struct arrow_def *new_arrow = NULL;
    struct arrow_def *prev_arrow = NULL;
    TBOOLEAN duplication = FALSE;
    TBOOLEAN set_start = FALSE;
    TBOOLEAN set_end = FALSE;
    int save_token;
    int tag;
    
    c_token++;

    /* get tag */
    if (almost_equals(c_token, "back$head") || equals(c_token, "front")
	    || equals(c_token, "from") || equals(c_token, "size")
	    || equals(c_token, "to") || equals(c_token, "rto")
	    || equals(c_token, "filled") || equals(c_token, "empty")
	    || equals(c_token, "as") || equals(c_token, "arrowstyle")
	    || almost_equals(c_token, "head$s") || equals(c_token, "nohead")) {
	tag = assign_arrow_tag();

    } else
	tag = int_expression();

    if (tag <= 0)
	int_error(c_token, "tag must be > 0");

    /* OK! add arrow */
    if (first_arrow != NULL) {	/* skip to last arrow */
	for (this_arrow = first_arrow; this_arrow != NULL;
	     prev_arrow = this_arrow, this_arrow = this_arrow->next)
	    /* is this the arrow we want? */
	    if (tag <= this_arrow->tag)
		break;
    }
    if (this_arrow == NULL || tag != this_arrow->tag) {
	new_arrow = gp_alloc(sizeof(struct arrow_def), "arrow");
	if (prev_arrow == NULL)
	    first_arrow = new_arrow;
	else
	    prev_arrow->next = new_arrow;
	new_arrow->tag = tag;
	new_arrow->next = this_arrow;
	this_arrow = new_arrow;

	this_arrow->start = default_position;
	this_arrow->end = default_position;

	default_arrow_style(&(new_arrow->arrow_properties));
    }

    while (!END_OF_COMMAND) {

	/* get start position */
	if (equals(c_token, "from")) {
	    if (set_start) { duplication = TRUE; break; }
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "start coordinates expected");
	    /* get coordinates */
	    get_position(&this_arrow->start);
	    set_start = TRUE;
	    continue;
	}

	/* get end or relative end position */
	if (equals(c_token, "to") || equals(c_token,"rto")) {
	    if (set_end) { duplication = TRUE; break; }
	    this_arrow->relative = (equals(c_token,"rto")) ? TRUE : FALSE;
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "end coordinates expected");
	    /* get coordinates */
	    get_position(&this_arrow->end);
	    set_end = TRUE;
	    continue;
	}

	/* Allow interspersed style commands */
	save_token = c_token;
	arrow_parse(&this_arrow->arrow_properties, TRUE);
	if (save_token != c_token)
	    continue;

	if (!END_OF_COMMAND)
	    int_error(c_token, "wrong argument in set arrow");

    } /* while (!END_OF_COMMAND) */

    if (duplication)
	int_error(c_token, "duplicate or contradictory arguments");

}


/* assign a new arrow tag
 * arrows are kept sorted by tag number, so this is easy
 * returns the lowest unassigned tag number
 */
static int
assign_arrow_tag()
{
    struct arrow_def *this_arrow;
    int last = 0;		/* previous tag value */

    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next)
	if (this_arrow->tag == last + 1)
	    last++;
	else
	    break;

    return (last + 1);
}


/* process 'set autoscale' command */
static void
set_autoscale()
{
    char min_string[20], max_string[20];

    c_token++;
    if (END_OF_COMMAND) {
	INIT_AXIS_ARRAY(set_autoscale , AUTOSCALE_BOTH);
	return;
    } else if (equals(c_token, "xy") || equals(c_token, "yx")) {
	axis_array[FIRST_X_AXIS].set_autoscale =
	    axis_array[FIRST_Y_AXIS].set_autoscale =  AUTOSCALE_BOTH;
	c_token++;
	return;
    } else if (equals(c_token, "fix")) {
	int a = 0;
	while (a < AXIS_ARRAY_SIZE) {
	    axis_array[a].set_autoscale |= AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX;
	    a++;
	}
	c_token++;
	return;
    } else if (almost_equals(c_token, "ke$epfix")) {
	int a = 0;
	while (a < AXIS_ARRAY_SIZE)
	    axis_array[a++].set_autoscale |= AUTOSCALE_BOTH;
	c_token++;
	return;
    }

    /* save on replication with a macro */
#define PROCESS_AUTO_LETTER(axis)					\
    do {								\
	AXIS *this = axis_array + axis;					\
									\
	if (equals(c_token, axis_defaults[axis].name)) {		\
	    this->set_autoscale = AUTOSCALE_BOTH;			\
	    ++c_token;							\
	    return;							\
	}								\
	sprintf(min_string, "%smi$n", axis_defaults[axis].name);	\
	if (almost_equals(c_token, min_string)) {			\
	    this->set_autoscale |= AUTOSCALE_MIN;			\
	    ++c_token;							\
	    return;							\
	}								\
	sprintf(max_string, "%sma$x", axis_defaults[axis].name);	\
	if (almost_equals(c_token, max_string)) {			\
	    this->set_autoscale |= AUTOSCALE_MAX;			\
	    ++c_token;							\
	    return;							\
	}								\
	sprintf(min_string, "%sfix", axis_defaults[axis].name);		\
	if (equals(c_token, min_string)) {				\
	    this->set_autoscale |= AUTOSCALE_FIXMIN | AUTOSCALE_FIXMAX;	\
	    ++c_token;							\
	    return;							\
	}								\
	sprintf(min_string, "%sfixmi$n", axis_defaults[axis].name);	\
	if (almost_equals(c_token, min_string)) {			\
	    this->set_autoscale |= AUTOSCALE_FIXMIN;			\
	    ++c_token;							\
	    return;							\
	}								\
	sprintf(max_string, "%sfixma$x", axis_defaults[axis].name);	\
	if (almost_equals(c_token, max_string)) {			\
	    this->set_autoscale |= AUTOSCALE_FIXMAX;			\
	    ++c_token;							\
	    return;							\
	}								\
    } while(0)

    PROCESS_AUTO_LETTER(R_AXIS);
    PROCESS_AUTO_LETTER(T_AXIS);
    PROCESS_AUTO_LETTER(U_AXIS);
    PROCESS_AUTO_LETTER(V_AXIS);
    PROCESS_AUTO_LETTER(FIRST_X_AXIS);
    PROCESS_AUTO_LETTER(FIRST_Y_AXIS);
    PROCESS_AUTO_LETTER(FIRST_Z_AXIS);
    PROCESS_AUTO_LETTER(SECOND_X_AXIS);
    PROCESS_AUTO_LETTER(SECOND_Y_AXIS);
    PROCESS_AUTO_LETTER(COLOR_AXIS);
    /* came here only if nothing found: */
	int_error(c_token, "Invalid range");
}


/* process 'set bars' command */
static void
set_bars()
{

    int save_token = ++c_token;

    while (!END_OF_COMMAND) {
	if (almost_equals(c_token,"s$mall")) {
	    bar_size = 0.0;
	    ++c_token;
	} else if (almost_equals(c_token,"l$arge")) {
	    bar_size = 1.0;
	    ++c_token;
	} else if (almost_equals(c_token,"full$width")) {
	    bar_size = -1.0;
	    ++c_token;
	} else if (equals(c_token,"front")) {
	    bar_layer = LAYER_FRONT;
	    ++c_token;
	} else if (equals(c_token,"back")) {
	    bar_layer = LAYER_BACK;
	    ++c_token;
	} else {
	    bar_size = real_expression();
	}
    }

    if (save_token == c_token)
	bar_size = 1.0;
	
}


/* process 'set border' command */
static void
set_border()
{
    c_token++;
    if(END_OF_COMMAND){
	draw_border = 31;
	border_layer = 1;
	border_lp = default_border_lp;
    }
    
    while (!END_OF_COMMAND) {
	if (equals(c_token,"front")) {
	    border_layer = 1;
	    c_token++;
	} else if (equals(c_token,"back")) {
	    border_layer = 0;
	    c_token++;
	} else {
	    int save_token = c_token;
	    lp_parse(&border_lp, TRUE, FALSE);
	    if (save_token != c_token)
		continue;
	    draw_border = int_expression();
	}
    }
}


/* process 'set boxwidth' command */
static void
set_boxwidth()
{
    c_token++;
    if (END_OF_COMMAND) {
	boxwidth = -1.0;
	boxwidth_is_absolute = TRUE;
    } else {
	boxwidth = real_expression();
    }
    if (END_OF_COMMAND)
	return;
    else {
	if (almost_equals(c_token, "a$bsolute"))
	    boxwidth_is_absolute = TRUE;
	else if (almost_equals(c_token, "r$elative"))
	    boxwidth_is_absolute = FALSE;
	else
	    int_error(c_token, "expecting 'absolute' or 'relative' ");
    }
    c_token++;
}

/* process 'set clabel' command */
static void
set_clabel()
{
    char *new_format;

    c_token++;
    label_contours = TRUE;
    if ((new_format = try_to_get_string())) {
	strncpy(contour_format, new_format, sizeof(contour_format));
	free(new_format);
    }
}


/* process 'set clip' command */
static void
set_clip()
{
    c_token++;
    if (END_OF_COMMAND)
	/* assuming same as points */
	clip_points = TRUE;
    else if (almost_equals(c_token, "p$oints"))
	clip_points = TRUE;
    else if (almost_equals(c_token, "o$ne"))
	clip_lines1 = TRUE;
    else if (almost_equals(c_token, "t$wo"))
	clip_lines2 = TRUE;
    else
	int_error(c_token, "expecting 'points', 'one', or 'two'");
    c_token++;
}


/* process 'set cntrparam' command */
static void
set_cntrparam()
{
    c_token++;
    if (END_OF_COMMAND) {
	/* assuming same as defaults */
	contour_pts = DEFAULT_NUM_APPROX_PTS;
	contour_kind = CONTOUR_KIND_LINEAR;
	contour_order = DEFAULT_CONTOUR_ORDER;
	contour_levels = DEFAULT_CONTOUR_LEVELS;
	contour_levels_kind = LEVELS_AUTO;
    } else if (almost_equals(c_token, "p$oints")) {
	c_token++;
	contour_pts = int_expression();
    } else if (almost_equals(c_token, "li$near")) {
	c_token++;
	contour_kind = CONTOUR_KIND_LINEAR;
    } else if (almost_equals(c_token, "c$ubicspline")) {
	c_token++;
	contour_kind = CONTOUR_KIND_CUBIC_SPL;
    } else if (almost_equals(c_token, "b$spline")) {
	c_token++;
	contour_kind = CONTOUR_KIND_BSPLINE;
    } else if (almost_equals(c_token, "le$vels")) {
	c_token++;

	free_dynarray(&dyn_contour_levels_list);
	init_dynarray(&dyn_contour_levels_list, sizeof(double), 5, 10);

	/*  RKC: I have modified the next two:
	 *   to use commas to separate list elements as in xtics
	 *   so that incremental lists start,incr[,end]as in "
	 */
	if (almost_equals(c_token, "di$screte")) {
	    contour_levels_kind = LEVELS_DISCRETE;
	    c_token++;
	    if(END_OF_COMMAND)
		int_error(c_token, "expecting discrete level");
	    else
		*(double *)nextfrom_dynarray(&dyn_contour_levels_list) =
		    real_expression();

	    while(!END_OF_COMMAND) {
		if (!equals(c_token, ","))
		    int_error(c_token,
			      "expecting comma to separate discrete levels");
		c_token++;
		*(double *)nextfrom_dynarray(&dyn_contour_levels_list) =
		    real_expression();
	    }
	    contour_levels = dyn_contour_levels_list.end;
	} else if (almost_equals(c_token, "in$cremental")) {
	    int i = 0;  /* local counter */

	    contour_levels_kind = LEVELS_INCREMENTAL;
	    c_token++;
	    contour_levels_list[i++] = real_expression();
	    if (!equals(c_token, ","))
		int_error(c_token,
			  "expecting comma to separate start,incr levels");
	    c_token++;
	    if((contour_levels_list[i++] = real_expression()) == 0)
		int_error(c_token, "increment cannot be 0");
	    if(!END_OF_COMMAND) {
		if (!equals(c_token, ","))
		    int_error(c_token,
			      "expecting comma to separate incr,stop levels");
		c_token++;
		/* need to round up, since 10,10,50 is 5 levels, not four,
		 * but 10,10,49 is four
		 */
		dyn_contour_levels_list.end = i;
		contour_levels = (int) ( (real_expression()-contour_levels_list[0])/contour_levels_list[1] + 1.0);
	    }
	} else if (almost_equals(c_token, "au$to")) {
	    contour_levels_kind = LEVELS_AUTO;
	    c_token++;
	    if(!END_OF_COMMAND)
		contour_levels = int_expression();
	} else {
	    if(contour_levels_kind == LEVELS_DISCRETE)
		int_error(c_token, "Levels type is discrete, ignoring new number of contour levels");
	    contour_levels = int_expression();
	}
    } else if (almost_equals(c_token, "o$rder")) {
	int order;
	c_token++;
	order = int_expression();
	if ( order < 2 || order > MAX_BSPLINE_ORDER )
	    int_error(c_token, "bspline order must be in [2..10] range.");
	contour_order = order;
    } else
	int_error(c_token, "expecting 'linear', 'cubicspline', 'bspline', 'points', 'levels' or 'order'");
}


/* process 'set contour' command */
static void
set_contour()
{
    c_token++;
    if (END_OF_COMMAND)
	/* assuming same as points */
	draw_contour = CONTOUR_BASE;
    else {
	if (almost_equals(c_token, "ba$se"))
	    draw_contour = CONTOUR_BASE;
	else if (almost_equals(c_token, "s$urface"))
	    draw_contour = CONTOUR_SRF;
	else if (almost_equals(c_token, "bo$th"))
	    draw_contour = CONTOUR_BOTH;
	else
	    int_error(c_token, "expecting 'base', 'surface', or 'both'");
	c_token++;
    }
}


/* process 'set dgrid3d' command */
static void
set_dgrid3d()
{
    int local_vals[3];
    int i;
    TBOOLEAN was_comma = TRUE;

    c_token++;
    local_vals[0] = dgrid3d_row_fineness;
    local_vals[1] = dgrid3d_col_fineness;
    local_vals[2] = dgrid3d_norm_value;

    for (i = 0; i < 3 && !(END_OF_COMMAND);) {
	if (equals(c_token,",")) {
	    if (was_comma) i++;
	    was_comma = TRUE;
	    c_token++;
	} else {
	    if (!was_comma)
		int_error(c_token, "',' expected");
	    local_vals[i] = real_expression();
	    i++;
	    was_comma = FALSE;
	}
    }

    if (local_vals[0] < 2 || local_vals[0] > 1000)
	int_error(c_token, "Row size must be in [2:1000] range; size unchanged");
    if (local_vals[1] < 2 || local_vals[1] > 1000)
	int_error(c_token, "Col size must be in [2:1000] range; size unchanged");
    if (local_vals[2] < 1 || local_vals[2] > 100)
	int_error(c_token, "Norm must be in [1:100] range; norm unchanged");

    dgrid3d_row_fineness = local_vals[0];
    dgrid3d_col_fineness = local_vals[1];
    dgrid3d_norm_value = local_vals[2];
    dgrid3d = TRUE;
}


/* process 'set decimalsign' command */
static void
set_decimalsign()
{
    c_token++;

    /* Clear current setting */
	free(decimalsign);
	decimalsign=NULL;

    if (END_OF_COMMAND) {
#ifdef HAVE_LOCALE_H
	free(numeric_locale);
	numeric_locale = NULL;
	setlocale(LC_NUMERIC,"C");
    } else if (equals(c_token,"locale")) {
	char *newlocale = NULL;
	c_token++;
	newlocale = try_to_get_string();
	if (!newlocale)
	    newlocale = getenv("LC_ALL");
	if (!newlocale)
	    newlocale = getenv("LC_NUMERIC");
	if (!newlocale)
	    newlocale = getenv("LANG");
	if (!setlocale(LC_NUMERIC, newlocale ? newlocale : ""))
	    int_error(c_token-1, "Could not find requested locale");
	decimalsign = gp_strdup(localeconv()->decimal_point);
	fprintf(stderr,"decimal_sign in locale is %s\n", decimalsign);
	/* Save this locale for later use, but return to "C" for now */
	free(numeric_locale);
	numeric_locale = newlocale;
	setlocale(LC_NUMERIC,"C");
#endif
    } else if (!(decimalsign = try_to_get_string()))
	int_error(c_token, "expecting string");
}

/* process 'set dummy' command */
static void
set_dummy()
{
    c_token++;
    if (END_OF_COMMAND)
	int_error(c_token, "expecting dummy variable name");
    else {
	if (!equals(c_token,","))
	    copy_str(set_dummy_var[0],c_token++, MAX_ID_LEN);
	if (!END_OF_COMMAND && equals(c_token,",")) {
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "expecting second dummy variable name");
	    copy_str(set_dummy_var[1],c_token++, MAX_ID_LEN);
	}
    }
}


/* process 'set encoding' command */
static void
set_encoding()
{
    c_token++;

    if (END_OF_COMMAND) {
	encoding = S_ENC_DEFAULT;
#ifdef HAVE_LOCALE_H
    } else if (equals(c_token,"locale")) {
	setlocale(LC_CTYPE,"");
	c_token++;
#endif
    } else {
	int temp = lookup_table(&set_encoding_tbl[0],c_token);

	if (temp == S_ENC_INVALID)
	    int_error(c_token, "expecting one of 'default', 'utf8', 'iso_8859_1', 'iso_8859_2', 'iso_8859_9', 'iso_8859_15', 'cp437', 'cp850', 'cp852', 'koi8r', 'koi8u', or 'locale'");
	encoding = temp;
	c_token++;
    }
}


/* process 'set fit' command */
static void
set_fit()
{
    c_token++;

    while (!END_OF_COMMAND) {
	if (almost_equals(c_token, "log$file")) {
	    c_token++;
	    if (END_OF_COMMAND) {
		if (fitlogfile != NULL)
		    free(fitlogfile);
		fitlogfile=NULL;
	    } else if (!(fitlogfile = try_to_get_string()))
		int_error(c_token, "expecting string");
#if GP_FIT_ERRVARS
	} else if (almost_equals(c_token, "err$orvariables")) {
	    fit_errorvariables = TRUE;
	    c_token++;
	} else if (almost_equals(c_token, "noerr$orvariables")) {
	    fit_errorvariables = FALSE;
	    c_token++;
#endif /* GP_FIT_ERRVARS */
	} else {
	    int_error(c_token,
		      "unknown --- expected 'logfile' or [no]errorvariables");
	}
    } /* while (!end) */
}


/* process 'set format' command */
static void
set_format()
{
    TBOOLEAN set_for_axis[AXIS_ARRAY_SIZE] = AXIS_ARRAY_INITIALIZER(FALSE);
    int axis;

    c_token++;
    if ((axis = lookup_table(axisname_tbl, c_token)) >= 0) {
	set_for_axis[axis] = TRUE;
	c_token++;
    } else if (equals(c_token,"xy") || equals(c_token,"yx")) {
	set_for_axis[FIRST_X_AXIS]
	    = set_for_axis[FIRST_Y_AXIS]
	    = TRUE;
	c_token++;
    } else if (isstring(c_token) || END_OF_COMMAND) {
	/* Assume he wants all */
	for (axis = 0; axis < AXIS_ARRAY_SIZE; axis++)
	    set_for_axis[axis] = TRUE;
    }

    if (END_OF_COMMAND) {
	SET_DEFFORMAT(FIRST_X_AXIS , set_for_axis);
	SET_DEFFORMAT(FIRST_Y_AXIS , set_for_axis);
	SET_DEFFORMAT(FIRST_Z_AXIS , set_for_axis);
	SET_DEFFORMAT(SECOND_X_AXIS, set_for_axis);
	SET_DEFFORMAT(SECOND_Y_AXIS, set_for_axis);
	SET_DEFFORMAT(COLOR_AXIS   , set_for_axis);
    } else {
	if (!isstring(c_token))
	    int_error(c_token, "expecting format string");
	else {

#define SET_FORMATSTRING(axis)						      \
	    if (set_for_axis[axis]) {					      \
		quote_str(axis_array[axis].formatstring,c_token, MAX_ID_LEN); \
		axis_array[axis].format_is_numeric =			      \
		    looks_like_numeric(axis_array[axis].formatstring);	      \
	    }
	    SET_FORMATSTRING(FIRST_X_AXIS);
	    SET_FORMATSTRING(FIRST_Y_AXIS);
	    SET_FORMATSTRING(FIRST_Z_AXIS);
	    SET_FORMATSTRING(SECOND_X_AXIS);
	    SET_FORMATSTRING(SECOND_Y_AXIS);
	    SET_FORMATSTRING(COLOR_AXIS);
#undef SET_FORMATSTRING

	    c_token++;
	}
    }
}


/* process 'set grid' command */

static void
set_grid()
{
    TBOOLEAN explicit_change = FALSE;
    c_token++;
#define	GRID_MATCH(axis, string)				\
	    if (almost_equals(c_token, string+2)) {		\
		if (string[2] == 'm')				\
		    axis_array[axis].gridminor = TRUE;		\
		else						\
		    axis_array[axis].gridmajor = TRUE;		\
		explicit_change = TRUE;				\
		++c_token;					\
	    } else if (almost_equals(c_token, string)) {	\
		if (string[2] == 'm')				\
		    axis_array[axis].gridminor = FALSE;		\
		else						\
		    axis_array[axis].gridmajor = FALSE;		\
		explicit_change = TRUE;				\
		++c_token;					\
	    }
    while (!END_OF_COMMAND) {
	GRID_MATCH(FIRST_X_AXIS, "nox$tics")
	else GRID_MATCH(FIRST_Y_AXIS, "noy$tics")
	else GRID_MATCH(FIRST_Z_AXIS, "noz$tics")
	else GRID_MATCH(SECOND_X_AXIS, "nox2$tics")
	else GRID_MATCH(SECOND_Y_AXIS, "noy2$tics")
	else GRID_MATCH(FIRST_X_AXIS, "nomx$tics")
	else GRID_MATCH(FIRST_Y_AXIS, "nomy$tics")
	else GRID_MATCH(FIRST_Z_AXIS, "nomz$tics")
	else GRID_MATCH(SECOND_X_AXIS, "nomx2$tics")
	else GRID_MATCH(SECOND_Y_AXIS, "nomy2$tics")
	else GRID_MATCH(COLOR_AXIS, "nocb$tics")
	else GRID_MATCH(COLOR_AXIS, "nomcb$tics")
	else if (almost_equals(c_token,"po$lar")) {
	    if (!some_grid_selected()) {
		/* grid_selection = GRID_X; */
		axis_array[FIRST_X_AXIS].gridmajor = TRUE;
	    }
	    c_token++;
	    if (END_OF_COMMAND) {
		polar_grid_angle = 30*DEG2RAD;
	    } else {
		/* get radial interval */
		polar_grid_angle = ang2rad*real_expression();
	    }
	} else if (almost_equals(c_token,"nopo$lar")) {
	    polar_grid_angle = 0; /* not polar grid */
	    c_token++;
	} else if (equals(c_token,"back")) {
	    grid_layer = 0;
	    c_token++;
	} else if (equals(c_token,"front")) {
	    grid_layer = 1;
	    c_token++;
	} else if (almost_equals(c_token,"layerd$efault")) {
	    grid_layer = -1;
	    c_token++;
	} else
	    break; /* might be a linetype */
    }

    if (!END_OF_COMMAND) {
	int old_token = c_token;

	lp_parse(&grid_lp, TRUE, FALSE);
	if (c_token == old_token) { /* nothing parseable found... */
	    grid_lp.l_type = int_expression() - 1;
	}

	/* probably just  set grid <linetype> */

	if (END_OF_COMMAND) {
	    memcpy(&mgrid_lp,&grid_lp,sizeof(struct lp_style_type));
	} else {
	    if (equals(c_token,","))
		c_token++;
	    old_token = c_token;
	    lp_parse(&mgrid_lp, TRUE, FALSE);
	    if (c_token == old_token) {
		mgrid_lp.l_type = int_expression() - 1;
	    }
	}
    }

    if (!explicit_change && !some_grid_selected()) {
	/* no axis specified, thus select default grid */
	axis_array[FIRST_X_AXIS].gridmajor = TRUE;
	axis_array[FIRST_Y_AXIS].gridmajor = TRUE;
    }
}


/* process 'set hidden3d' command */
static void
set_hidden3d()
{
    c_token++;
#ifdef LITE
    printf(" Hidden Line Removal Not Supported in LITE version\n");
#else
    /* HBB 970618: new parsing engine for hidden3d options */
    set_hidden3doptions();
    hidden3d = TRUE;
#endif
}


#ifdef GNUPLOT_HISTORY
/* process 'set historysize' command */
static void
set_historysize()
{
    c_token++;

    gnuplot_history_size = int_expression();
    if (gnuplot_history_size < 0) {
	gnuplot_history_size = 0;
    }
}
#endif


/* process 'set isosamples' command */
static void
set_isosamples()
{
    int tsamp1, tsamp2;

    c_token++;
    tsamp1 = abs(int_expression());
    tsamp2 = tsamp1;
    if (!END_OF_COMMAND) {
	if (!equals(c_token,","))
	    int_error(c_token, "',' expected");
	c_token++;
	tsamp2 = abs(int_expression());
    }
    if (tsamp1 < 2 || tsamp2 < 2)
	int_error(c_token, "sampling rate must be > 1; sampling unchanged");
    else {
	struct curve_points *f_p = first_plot;
	struct surface_points *f_3dp = first_3dplot;

	first_plot = NULL;
	first_3dplot = NULL;
	cp_free(f_p);
	sp_free(f_3dp);

	iso_samples_1 = tsamp1;
	iso_samples_2 = tsamp2;
    }
}


/* When plotting an external key, the margin and l/r/t/b/c are
   used to determine one of twelve possible positions.  They must
   be defined appropriately in the case where stack direction
   determines exact position. */
static void
set_key_position_from_stack_direction(legend_key *key)
{
    if (key->stack_dir == GPKEY_VERTICAL) {
	switch(key->hpos) {
	case LEFT:
	    key->margin = GPKEY_LMARGIN;
	    break;
	case CENTRE:
	    if (key->vpos == JUST_TOP)
		key->margin = GPKEY_TMARGIN;
	    else
		key->margin = GPKEY_BMARGIN;
	    break;
	case RIGHT:
	    key->margin = GPKEY_RMARGIN;
	    break;
	}
    } else {
	switch(key->vpos) {
	case JUST_TOP:
	    key->margin = GPKEY_TMARGIN;
	    break;
	case JUST_CENTRE:
	    if (key->hpos == LEFT)
		key->margin = GPKEY_LMARGIN;
	    else
		key->margin = GPKEY_RMARGIN;
	    break;
	case JUST_BOT:
	    key->margin = GPKEY_BMARGIN;
	    break;
	}
    }
}


/* process 'set key' command */
static void
set_key()
{
    TBOOLEAN vpos_set = FALSE, hpos_set = FALSE, reg_set = FALSE, sdir_set = FALSE;
    char *vpos_warn = "Multiple vertical position settings";
    char *hpos_warn = "Multiple horizontal position settings";
    char *reg_warn = "Multiple location region settings";
    char *sdir_warn = "Multiple stack direction settings";
    legend_key *key = &keyT;

    c_token++;
    key->visible = TRUE;

#ifdef BACKWARDS_COMPATIBLE
    if (END_OF_COMMAND) {
	free(key->font);
	reset_key();
	if (interactive)
	    int_warn(c_token, "deprecated syntax, use \"set key default\"");
    }
#endif

    while (!END_OF_COMMAND) {
	switch(lookup_table(&set_key_tbl[0],c_token)) {
	case S_KEY_ON:
	    key->visible = TRUE;
	    break;
	case S_KEY_OFF:
	    key->visible = FALSE;
	    break;
	case S_KEY_DEFAULT:
	    free(key->font);
	    reset_key();
	    break;
	case S_KEY_TOP:
	    if (vpos_set)
		int_warn(c_token, vpos_warn);
	    key->vpos = JUST_TOP;
	    vpos_set = TRUE;
	    break;
	case S_KEY_BOTTOM:
	    if (vpos_set)
		int_warn(c_token, vpos_warn);
	    key->vpos = JUST_BOT;
	    vpos_set = TRUE;
	    break;
	case S_KEY_LEFT:
	    if (hpos_set)
		int_warn(c_token, hpos_warn);
	    key->hpos = LEFT;
	    hpos_set = TRUE;
	    break;
	case S_KEY_RIGHT:
	    if (hpos_set)
		int_warn(c_token, hpos_warn);
	    key->hpos = RIGHT;
	    hpos_set = TRUE;
	    break;
	case S_KEY_CENTER:
	    if (!vpos_set) key->vpos = JUST_CENTRE;
	    if (!hpos_set) key->hpos = CENTRE;
	    if (vpos_set || hpos_set)
		vpos_set = hpos_set = TRUE;
	    break;
	case S_KEY_VERTICAL:
	    if (sdir_set)
		int_warn(c_token, sdir_warn);
	    key->stack_dir = GPKEY_VERTICAL;
	    sdir_set = TRUE;
	    break;
	case S_KEY_HORIZONTAL:
	    if (sdir_set)
		int_warn(c_token, sdir_warn);
	    key->stack_dir = GPKEY_HORIZONTAL;
	    sdir_set = TRUE;
	    break;
	case S_KEY_OVER:
	    if (reg_set)
		int_warn(c_token, reg_warn);
	    /* Fall through */
	case S_KEY_ABOVE:
	    if (!hpos_set)
		key->hpos = CENTRE;
	    if (!sdir_set)
		key->stack_dir = GPKEY_HORIZONTAL;
	    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
	    key->margin = GPKEY_TMARGIN;
	    reg_set = TRUE;
	    break;
	case S_KEY_UNDER:
	    if (reg_set)
		int_warn(c_token, reg_warn);
	    /* Fall through */
	case S_KEY_BELOW:
	    if (!hpos_set)
		key->hpos = CENTRE;
	    if (!sdir_set)
		key->stack_dir = GPKEY_HORIZONTAL;
	    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
	    key->margin = GPKEY_BMARGIN;
	    reg_set = TRUE;
	    break;
	case S_KEY_INSIDE:
	    if (reg_set)
		int_warn(c_token, reg_warn);
	    key->region = GPKEY_AUTO_INTERIOR_LRTBC;
	    reg_set = TRUE;
	    break;
	case S_KEY_OUTSIDE:
#ifdef BACKWARDS_COMPATIBLE
	    if (!hpos_set)
		key->hpos = RIGHT;
	    if (!sdir_set)
		key->stack_dir = GPKEY_VERTICAL;
#endif
	    if (reg_set)
		int_warn(c_token, reg_warn);
	    key->region = GPKEY_AUTO_EXTERIOR_LRTBC;
	    reg_set = TRUE;
	    break;
	case S_KEY_TMARGIN:
	    if (reg_set)
		int_warn(c_token, reg_warn);
	    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
	    key->margin = GPKEY_TMARGIN;
	    reg_set = TRUE;
	    break;
	case S_KEY_BMARGIN:
	    if (reg_set)
		int_warn(c_token, reg_warn);
	    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
	    key->margin = GPKEY_BMARGIN;
	    reg_set = TRUE;
	    break;
	case S_KEY_LMARGIN:
	    if (reg_set)
		int_warn(c_token, reg_warn);
	    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
	    key->margin = GPKEY_LMARGIN;
	    reg_set = TRUE;
	    break;
	case S_KEY_RMARGIN:
	    if (reg_set)
		int_warn(c_token, reg_warn);
	    key->region = GPKEY_AUTO_EXTERIOR_MARGIN;
	    key->margin = GPKEY_RMARGIN;
	    reg_set = TRUE;
	    break;
	case S_KEY_LLEFT:
	    key->just = GPKEY_LEFT;
	    break;
	case S_KEY_RRIGHT:
	    key->just = GPKEY_RIGHT;
	    break;
	case S_KEY_REVERSE:
	    key->reverse = TRUE;
	    break;
	case S_KEY_NOREVERSE:
	    key->reverse = FALSE;
	    break;
	case S_KEY_INVERT:
	    key->invert = TRUE;
	    break;
	case S_KEY_NOINVERT:
	    key->invert = FALSE;
	    break;
	case S_KEY_ENHANCED:
	    key->enhanced = TRUE;
	    break;
	case S_KEY_NOENHANCED:
	    key->enhanced = FALSE;
	    break;
	case S_KEY_BOX:
	    c_token++;
	    key->box.l_type = LT_BLACK;
	    if (!END_OF_COMMAND) {
		int old_token = c_token;
		lp_parse(&key->box, TRUE, FALSE);
		if (old_token == c_token && isanumber(c_token)) {
		    key->box.l_type = int_expression() - 1;
		    c_token++;
		}
	    }
	    c_token--;  /* is incremented after loop */
	    break;
	case S_KEY_NOBOX:
	    key->box.l_type = LT_NODRAW;
	    break;
	case S_KEY_SAMPLEN:
	    c_token++;
	    key->swidth = real_expression();
	    c_token--; /* it is incremented after loop */
	    break;
	case S_KEY_SPACING:
	    c_token++;
	    key->vert_factor = real_expression();
	    if (key->vert_factor < 0.0)
		key->vert_factor = 0.0;
	    c_token--; /* it is incremented after loop */
	    break;
	case S_KEY_WIDTH:
	    c_token++;
	    key->width_fix = real_expression();
	    c_token--; /* it is incremented after loop */
	    break;
	case S_KEY_HEIGHT:
	    c_token++;
	    key->height_fix = real_expression();
	    c_token--; /* it is incremented after loop */
	    break;
	case S_KEY_AUTOTITLES:
	    if (almost_equals(++c_token, "col$umnheader"))
		key->auto_titles = COLUMNHEAD_KEYTITLES;
	    else {
		key->auto_titles = FILENAME_KEYTITLES;
		c_token--;
	    }
	    break;
	case S_KEY_NOAUTOTITLES:
	    key->auto_titles = NOAUTO_KEYTITLES;
	    break;
	case S_KEY_TITLE:
	    {
		char *s;
		c_token++;
		if ((s = try_to_get_string())) {
		    strncpy(key->title,s,sizeof(key->title));
		    free(s);
		} else
		    key->title[0] = '\0';
		c_token--;
	    }
	    break;

	case S_KEY_FONT:
	    c_token++;
	    /* Make sure they've specified a font */
	    if (!isstringvalue(c_token))
		int_error(c_token,"expected font");
	    else {
		free(key->font);
		key->font = try_to_get_string();
		c_token--;
	    }
	    break;
	case S_KEY_TEXTCOLOR:
	    {
	    struct t_colorspec lcolor = DEFAULT_COLORSPEC;
	    parse_colorspec(&lcolor, TC_FRAC);
	    key->textcolor = lcolor;
	    }
	    c_token--;
	    break;

	case S_KEY_MANUAL:
	    c_token++;
#ifdef BACKWARDS_COMPATIBLE
	case S_KEY_INVALID:
	default:
#endif
	    if (reg_set)
		int_warn(c_token, reg_warn);
	    get_position(&key->user_pos);
	    key->region = GPKEY_USER_PLACEMENT;
	    reg_set = TRUE;
	    c_token--;  /* will be incremented again soon */
	    break;
#ifndef BACKWARDS_COMPATIBLE
	case S_KEY_INVALID:
	default:
	    int_error(c_token, "unknown key option");
	    break;
#endif
	}
	c_token++;
    }

    if (key->region == GPKEY_AUTO_EXTERIOR_LRTBC)
	set_key_position_from_stack_direction(key);
    else if (key->region == GPKEY_AUTO_EXTERIOR_MARGIN) {
	if (vpos_set && (key->margin == GPKEY_TMARGIN || key->margin == GPKEY_BMARGIN))
	    int_warn(NO_CARET,
		     "ignoring top/center/bottom; incompatible with tmargin/bmargin.");
	else if (hpos_set && (key->margin == GPKEY_LMARGIN || key->margin == GPKEY_RMARGIN))
	    int_warn(NO_CARET,
		     "ignoring left/center/right; incompatible with lmargin/tmargin.");
    }
}


/* process 'set keytitle' command */
static void
set_keytitle()
{
    legend_key *key = &keyT;

    c_token++;
    if (END_OF_COMMAND) {	/* set to default */
	key->title[0] = NUL;
    } else {
	char *s;
	if ((s = try_to_get_string())) {
	    strncpy(key->title,s,sizeof(key->title));
	    free(s);
	}
    }
}

/* process 'set label' command */
/* set label {tag} {"label_text"{,<value>{,...}}} {<label options>} */
/* EAM Mar 2003 - option parsing broken out into separate routine */
static void
set_label()
{
    struct text_label *this_label = NULL;
    struct text_label *new_label = NULL;
    struct text_label *prev_label = NULL;
    struct value a;
    int tag;

    c_token++;
    
    /* get tag */
    if (!END_OF_COMMAND
	/* FIXME - Are these tests really still needed? */
	&& !isstringvalue(c_token)
	&& !equals(c_token, "at")
	&& !equals(c_token, "left")
	&& !equals(c_token, "center")
	&& !equals(c_token, "centre")
	&& !equals(c_token, "right")
	&& !equals(c_token, "front")
	&& !equals(c_token, "back")
	&& !almost_equals(c_token, "rot$ate")
	&& !almost_equals(c_token, "norot$ate")
	&& !equals(c_token, "lt")
	&& !almost_equals(c_token, "linet$ype")
	&& !equals(c_token, "pt")
	&& !almost_equals(c_token, "pointt$ype")
	&& !equals(c_token, "tc")
	&& !almost_equals(c_token, "text$color")
	&& !equals(c_token, "font")) {

	/* must be an expression, but is it a tag or is it the label itself? */
	int save_token = c_token;
	const_express(&a);
	if (a.type == STRING) {
	    c_token = save_token;
	    tag = assign_label_tag();
	    gpfree_string(&a);
	} else
	    tag = (int) real(&a);

    } else
	tag = assign_label_tag();	/* default next tag */

    if (tag <= 0)
	int_error(c_token, "tag must be > zero");

    if (first_label != NULL) {	/* skip to last label */
	for (this_label = first_label; this_label != NULL;
	     prev_label = this_label, this_label = this_label->next)
	    /* is this the label we want? */
	    if (tag <= this_label->tag)
		break;
    }
    /* Insert this label into the list if it is a new one */
    if (this_label == NULL || tag != this_label->tag) {
	struct position default_offset = { character, character, character, 
					   0., 0., 0. };
	new_label = new_text_label(tag);
	new_label->offset = default_offset;
	if (prev_label == NULL)
	    first_label = new_label;
	else
	    prev_label->next = new_label;
	new_label->next = this_label;
	this_label = new_label;
    }

    if (!END_OF_COMMAND) {
	char* text;
	parse_label_options( this_label );
	text = try_to_get_string();
	if (text)
	    this_label->text = text;

	/* HBB 20001021: new functionality. If next token is a ','
	 * treat it as a numeric expression whose value is to be
	 * sprintf()ed into the label string (which contains an
	 * appropriate %f format string) */
	/* EAM Oct 2004 - this is superseded by general string variable
	 * handling, but left in for backward compatibility */
	if (!END_OF_COMMAND && equals(c_token, ","))
	    this_label->text = fill_numbers_into_string(this_label->text);
    }

    /* Now parse the label format and style options */
    parse_label_options( this_label );
}


/* assign a new label tag
 * labels are kept sorted by tag number, so this is easy
 * returns the lowest unassigned tag number
 */
static int
assign_label_tag()
{
    struct text_label *this_label;
    int last = 0;		/* previous tag value */

    for (this_label = first_label; this_label != NULL;
	 this_label = this_label->next)
	if (this_label->tag == last + 1)
	    last++;
	else
	    break;

    return (last + 1);
}


/* process 'set loadpath' command */
static void
set_loadpath()
{
    /* We pick up all loadpath elements here before passing
     * them on to set_var_loadpath()
     */
    char *collect = NULL;

    c_token++;
    if (END_OF_COMMAND) {
	clear_loadpath();
    } else while (!END_OF_COMMAND) {
	if (isstring(c_token)) {
	    int len;
	    char *ss = gp_alloc(token_len(c_token), "tmp storage");
	    len = (collect? strlen(collect) : 0);
	    quote_str(ss,c_token,token_len(c_token));
	    collect = gp_realloc(collect, len+1+strlen(ss)+1, "tmp loadpath");
	    if (len != 0) {
		strcpy(collect+len+1,ss);
		*(collect+len) = PATHSEP;
	    }
	    else
		strcpy(collect,ss);
	    free(ss);
	    ++c_token;
	} else {
	    int_error(c_token, "expected string");
	}
    }
    if (collect) {
	set_var_loadpath(collect);
	free(collect);
    }
}


/* process 'set fontpath' command */
static void
set_fontpath()
{
    /* We pick up all fontpath elements here before passing
     * them on to set_var_fontpath()
     */
    char *collect = NULL;

    c_token++;
    if (END_OF_COMMAND) {
	clear_fontpath();
    } else while (!END_OF_COMMAND) {
	if (isstring(c_token)) {
	    int len;
	    char *ss = gp_alloc(token_len(c_token), "tmp storage");
	    len = (collect? strlen(collect) : 0);
	    quote_str(ss,c_token,token_len(c_token));
	    collect = gp_realloc(collect, len+1+strlen(ss)+1, "tmp fontpath");
	    if (len != 0) {
		strcpy(collect+len+1,ss);
		*(collect+len) = PATHSEP;
	    }
	    else
		strcpy(collect,ss);
	    free(ss);
	    ++c_token;
	} else {
	    int_error(c_token, "expected string");
	}
    }
    if (collect) {
	set_var_fontpath(collect);
	free(collect);
    }
}


/* process 'set locale' command */
static void
set_locale()
{
    char *s;

    c_token++;
    if (END_OF_COMMAND) {
	init_locale();
    } else if ((s = try_to_get_string())) {
	set_var_locale(s);
	free(s);
    } else
	int_error(c_token, "expected string");
}


/* process 'set logscale' command */
static void
set_logscale()
{
    c_token++;
    if (END_OF_COMMAND) {
	INIT_AXIS_ARRAY(log,TRUE);
	INIT_AXIS_ARRAY(base, 10.0);
    } else {
	TBOOLEAN set_for_axis[AXIS_ARRAY_SIZE] = AXIS_ARRAY_INITIALIZER(FALSE);
	int axis;
	double newbase = 10;

	/* do reverse search because of "x", "x1", "x2" sequence in axisname_tbl */
	int i = 0;
	while (i < token[c_token].length) {
	    axis = lookup_table_nth_reverse(axisname_tbl, AXIS_ARRAY_SIZE,
		       gp_input_line + token[c_token].start_index + i);
	    if (axis < 0) {
		token[c_token].start_index += i;
		int_error(c_token, "unknown axis");
	    }
	    set_for_axis[axisname_tbl[axis].value] = TRUE;
	    i += strlen(axisname_tbl[axis].key);
	}
	c_token++;

	if (!END_OF_COMMAND) {
	    newbase = abs(real_expression());
	    if (newbase < 1.1)
		int_error(c_token,
			  "log base must be >= 1.1; logscale unchanged");
	}

	for (axis = 0; axis < AXIS_ARRAY_SIZE; axis++)
	    if (set_for_axis[axis]) {
		axis_array[axis].log = TRUE;
		axis_array[axis].base = newbase;
	}
    }

#ifdef VOLATILE_REFRESH
    /* Because the log scaling is applied during data input, a quick refresh */
    /* using existing stored data will not work if the log setting changes.  */
    refresh_ok = 0;
#endif
}

#ifdef GP_MACROS
static void
set_macros()
{
   c_token++;
   expand_macros = TRUE;
}
#endif

/* process 'set mapping3d' command */
static void
set_mapping()
{
    c_token++;
    if (END_OF_COMMAND)
	/* assuming same as points */
	mapping3d = MAP3D_CARTESIAN;
    else if (almost_equals(c_token, "ca$rtesian"))
	mapping3d = MAP3D_CARTESIAN;
    else if (almost_equals(c_token, "s$pherical"))
	mapping3d = MAP3D_SPHERICAL;
    else if (almost_equals(c_token, "cy$lindrical"))
	mapping3d = MAP3D_CYLINDRICAL;
    else
	int_error(c_token,
		  "expecting 'cartesian', 'spherical', or 'cylindrical'");
	c_token++;
}


/* process 'set {blrt}margin' command */
static void
set_margin(t_position *margin)
{
    margin->scalex = character;
    margin->x = -1;
    c_token++;

    if (END_OF_COMMAND)
	return;
    if (almost_equals(c_token,"sc$reen")) {
	margin->scalex = screen;
	c_token++;
    }
    margin->x = real_expression();
    if (margin->x < 0)
	margin->x = -1;
}

static void
set_separator()
{
    c_token++;
    if (END_OF_COMMAND) {
	df_separator = '\0';
	return;
    }
    if (almost_equals(c_token, "white$space"))
	df_separator = '\0';
    else if (!isstring(c_token))
	int_error(c_token, "expected \"<separator_char>\"");
    else if (equals(c_token, "\"\\t\"") || equals(c_token, "\'\\t\'"))
	df_separator = '\t';
    else if (gp_input_line[token[c_token].start_index]
	     != gp_input_line[token[c_token].start_index + 2])
	int_error(c_token, "extra chars after <separation_char>");
    else
	df_separator = gp_input_line[token[c_token].start_index + 1];
    c_token++;
}

static void
set_datafile_commentschars()
{
    char *s;

    c_token++;
	
    if (END_OF_COMMAND) {
	free(df_commentschars);
	df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
    } else if ((s = try_to_get_string())) {
	free(df_commentschars);
        df_commentschars = s;
    } else /* Leave it the way it was */
	int_error(c_token, "expected string with comments chars");
}

/* process 'set missing' command */
static void
set_missing()
{
    c_token++;
    if (END_OF_COMMAND) {
	free(missing_val);
	missing_val = NULL;
    } else if (!(missing_val = try_to_get_string()))
	int_error(c_token, "expected missing-value string");
}

#ifdef USE_MOUSE
static void
set_mouse()
{
    c_token++;
    mouse_setting.on = 1;

    while (!END_OF_COMMAND) {
	if (almost_equals(c_token, "do$ubleclick")) {
	    ++c_token;
	    mouse_setting.doubleclick = real_expression();
	    if (mouse_setting.doubleclick < 0)
		mouse_setting.doubleclick = 0;
	} else if (almost_equals(c_token, "nodo$ubleclick")) {
	    mouse_setting.doubleclick = 0; /* double click off */
	    ++c_token;
	} else if (almost_equals(c_token, "zoomco$ordinates")) {
	    mouse_setting.annotate_zoom_box = 1;
	    ++c_token;
	} else if (almost_equals(c_token, "nozoomco$ordinates")) {
	    mouse_setting.annotate_zoom_box = 0;
	    ++c_token;
	} else if (almost_equals(c_token, "po$lardistancedeg")) {
	    mouse_setting.polardistance = 1;
	    UpdateStatusline();
	    ++c_token;
	} else if (almost_equals(c_token, "polardistancet$an")) {
	    mouse_setting.polardistance = 2;
	    UpdateStatusline();
	    ++c_token;
	} else if (almost_equals(c_token, "nopo$lardistance")) {
	    mouse_setting.polardistance = 0;
	    UpdateStatusline();
	    ++c_token;
	} else if (equals(c_token, "labels")) {
	    mouse_setting.label = 1;
	    ++c_token;
	    /* check if the optional argument "<label options>" is present */
	    if (isstring(c_token)) {
		if (token_len(c_token) >= sizeof(mouse_setting.labelopts)) {
		    int_error(c_token, "option string too long");
		} else {
		    quote_str(mouse_setting.labelopts,
			c_token, token_len(c_token));
		}
		++c_token;
	    }
	} else if (almost_equals(c_token, "nola$bels")) {
	    mouse_setting.label = 0;
	    ++c_token;
	} else if (almost_equals(c_token, "ve$rbose")) {
	    mouse_setting.verbose = 1;
	    ++c_token;
	} else if (almost_equals(c_token, "nove$rbose")) {
	    mouse_setting.verbose = 0;
	    ++c_token;
	} else if (almost_equals(c_token, "zoomju$mp")) {
	    mouse_setting.warp_pointer = 1;
	    ++c_token;
	} else if (almost_equals(c_token, "nozoomju$mp")) {
	    mouse_setting.warp_pointer = 0;
	    ++c_token;
	} else if (almost_equals(c_token, "fo$rmat")) {
	    ++c_token;
	    if (isstring(c_token)) {
		if (token_len(c_token) >= sizeof(mouse_setting.fmt)) {
		    int_error(c_token, "format string too long");
		} else {
		    quote_str(mouse_setting.fmt, c_token, token_len(c_token));
		}
	    } else {
		int_error(c_token, "expecting string format");
	    }
	    ++c_token;
	} else if (almost_equals(c_token, "cl$ipboardformat")) {
	    ++c_token;
	    if (isstring(c_token)) {
		if (clipboard_alt_string)
		    free(clipboard_alt_string);
		clipboard_alt_string = gp_alloc
		    (token_len(c_token), "set->mouse->clipboardformat");
		quote_str(clipboard_alt_string, c_token, token_len(c_token));
		if (clipboard_alt_string && !strlen(clipboard_alt_string)) {
		    free(clipboard_alt_string);
		    clipboard_alt_string = (char*) 0;
		    if (MOUSE_COORDINATES_ALT == mouse_mode) {
			mouse_mode = MOUSE_COORDINATES_REAL;
		    }
		} else {
		    clipboard_mode = MOUSE_COORDINATES_ALT;
		}
		c_token++;
	    } else {
		int itmp = int_expression();
		if (itmp >= MOUSE_COORDINATES_REAL
		    && itmp <= MOUSE_COORDINATES_XDATETIME) {
		    if (MOUSE_COORDINATES_ALT == itmp
			&& !clipboard_alt_string) {
			fprintf(stderr,
			    "please 'set mouse clipboard <fmt>' first.\n");
		    } else {
			clipboard_mode = itmp;
		    }
		} else {
		    fprintf(stderr, "should be: %d <= clipboardformat <= %d\n",
			MOUSE_COORDINATES_REAL, MOUSE_COORDINATES_XDATETIME);
		}
	    }
	} else if (almost_equals(c_token, "mo$useformat")) {
	    ++c_token;
	    if (isstring(c_token)) {
		if (mouse_alt_string)
		    free(mouse_alt_string);
		mouse_alt_string = gp_alloc
		    (token_len(c_token), "set->mouse->mouseformat");
		quote_str(mouse_alt_string, c_token, token_len(c_token));
		if (mouse_alt_string && !strlen(mouse_alt_string)) {
		    free(mouse_alt_string);
		    mouse_alt_string = (char*) 0;
		    if (MOUSE_COORDINATES_ALT == mouse_mode) {
			mouse_mode = MOUSE_COORDINATES_REAL;
		    }
		} else {
		    mouse_mode = MOUSE_COORDINATES_ALT;
		}
		c_token++;
	    } else {
		int itmp = int_expression();
		if (itmp >= MOUSE_COORDINATES_REAL
		    && itmp <= MOUSE_COORDINATES_XDATETIME) {
		    if (MOUSE_COORDINATES_ALT == itmp && !mouse_alt_string) {
			fprintf(stderr,
			    "please 'set mouse mouseformat <fmt>' first.\n");
		    } else {
			mouse_mode = itmp;
		    }
		} else {
		    fprintf(stderr, "should be: %d <= mouseformat <= %d\n",
			MOUSE_COORDINATES_REAL, MOUSE_COORDINATES_XDATETIME);
		}
	    }
	} else if (almost_equals(c_token, "noru$ler")) {
	    c_token++;
	    set_ruler(FALSE, -1, -1);
	} else if (almost_equals(c_token, "ru$ler")) {
	    c_token++;
    	    if (END_OF_COMMAND || !equals(c_token, "at")) {
		set_ruler(TRUE, -1, -1);
	    } else { /* set mouse ruler at ... */
		struct position where;
		int x, y;
		c_token++;
		if (END_OF_COMMAND)
		    int_error(c_token, "expecting ruler coordinates");
		get_position(&where);
		map_position(&where, &x, &y, "ruler at");
		set_ruler(TRUE, (int)x, (int)y);
	    }
	} else {
	    if (!END_OF_COMMAND)
    		int_error(c_token, "wrong option");
	    break;
	}
    }
#ifdef OS2
    PM_update_menu_items();
#endif
}
#endif

/* process 'set offsets' command */
static void
set_offsets()
{
    c_token++;
    if (END_OF_COMMAND) {
	loff = roff = toff = boff = 0.0;  /* Reset offsets */
    } else {
	load_offsets (&loff,&roff,&toff,&boff);
    }
}


/* process 'set origin' command */
static void
set_origin()
{
    c_token++;
    if (END_OF_COMMAND) {
	xoffset = 0.0;
	yoffset = 0.0;
    } else {
	xoffset = real_expression();
	if (!equals(c_token,","))
	    int_error(c_token, "',' expected");
	c_token++;
	yoffset = real_expression();
    }
}


/* process 'set output' command */
static void
set_output()
{
    char *testfile;

    c_token++;
    if (multiplot)
	int_error(c_token, "you can't change the output in multiplot mode");

    if (END_OF_COMMAND) {	/* no file specified */
	term_set_output(NULL);
	if (outstr) {
	    free(outstr);
	    outstr = NULL; /* means STDOUT */
	}
    } else if ((testfile = try_to_get_string())) {
	gp_expand_tilde(&testfile);
	term_set_output(testfile);
	if (testfile != outstr) {
	    if (testfile)
		free(testfile);
	    testfile = outstr;
	}
	/* if we get here then it worked, and outstr now = testfile */
    } else
	int_error(c_token, "expecting filename");

    /* Invalidate previous palette */
    invalidate_palette();
	
}


/* process 'set print' command */
static void
set_print()
{
    TBOOLEAN append_p = FALSE;
    char *testfile = NULL;

    c_token++;
    if (END_OF_COMMAND) {	/* no file specified */
	print_set_output(NULL, append_p);
    } else if ((testfile = try_to_get_string())) {
	gp_expand_tilde(&testfile);
	if (!END_OF_COMMAND) {
	    if (equals(c_token, "append")) {
		append_p = TRUE;
		c_token++;
	    } else {
		int_error(c_token, "expecting keyword \'append\'");
	    }
	}
	print_set_output(testfile, append_p);
    } else
	int_error(c_token, "expecting filename");
}


/* process 'set parametric' command */
static void
set_parametric()
{
    c_token++;

    if (!parametric) {
	parametric = TRUE;
	if (!polar) { /* already done for polar */
	    strcpy (set_dummy_var[0], "t");
	    strcpy (set_dummy_var[1], "y");
	    if (interactive)
		(void) fprintf(stderr,"\n\tdummy variable is t for curves, u/v for surfaces\n");
	}
    }
}


/* is resetting palette enabled?
 * note: reset_palette() is disabled within 'test palette'
 */
int enable_reset_palette = 1;

/* default settings for palette */
void
reset_palette()
{
    if (!enable_reset_palette) return;
    sm_palette.colorMode = SMPAL_COLOR_MODE_RGB;
    sm_palette.formulaR = 7; sm_palette.formulaG = 5;
    sm_palette.formulaB = 15;
    sm_palette.positive = SMPAL_POSITIVE;
    sm_palette.ps_allcF = 0;
    sm_palette.use_maxcolors = 0;
    sm_palette.gradient_num = 0;
    free(sm_palette.gradient);
    sm_palette.gradient = NULL;
    free(sm_palette.color);
    sm_palette.color = NULL;
    sm_palette.cmodel = C_MODEL_RGB;
    sm_palette.gamma = 1.5;
    pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_NONE;
}



/* Process 'set palette defined' gradient specification */
/* Syntax
 *   set palette defined   -->  use default palette
 *   set palette defined ( <pos1> <colorspec1>, ... , <posN> <colorspecN> )
 *     <posX>  gray value, automatically rescaled to [0, 1]
 *     <colorspecX>   :=  { "<color_name>" | "<X-style-color>" |  <r> <g> <b> }
 *        <color_name>     predefined colors (see below)
 *        <X-style-color>  "#rrggbb" with 2char hex values for red, green, blue
 *        <r> <g> <b>      three values in [0, 1] for red, green and blue
 *   return 1 if named colors where used, 0 otherwise
 */
static int
set_palette_defined()
{
    double p=0, r=0, g=0, b=0;
    int num, named_colors=0;
    int actual_size=8;

    /* Invalidate previous gradient */
    invalidate_palette();

    free( sm_palette.gradient );
    sm_palette.gradient = gp_alloc( actual_size*sizeof(gradient_struct), "pm3d gradient" );

    if (END_OF_COMMAND) {
	/* lets use some default gradient */
	double pal[][4] = { {0.0, 0.05, 0.05, 0.2}, {0.1, 0, 0, 1},
			    {0.25, 0.7, 0.85, 0.9}, {0.4, 0, 0.75, 0},
			    {0.5, 1, 1, 0}, {0.7, 1, 0, 0},
			    {0.9, 0.6, 0.6, 0.6}, {1.0, 0.95, 0.95, 0.95} };
	int i;
	for( i=0; i<8; ++i ) {
	    sm_palette.gradient[i].pos = pal[i][0];
	    sm_palette.gradient[i].col.r = pal[i][1];
	    sm_palette.gradient[i].col.g = pal[i][2];
	    sm_palette.gradient[i].col.b = pal[i][3];
	}
	sm_palette.gradient_num = 8;
	sm_palette.cmodel = C_MODEL_RGB;
	return 0;
    }

    if ( !equals(c_token,"(") )
	int_error( c_token, "Expected ( to start gradient definition." );

    ++c_token;
    num = -1;

    while (!END_OF_COMMAND) {
	p = real_expression();
	if (isstring(c_token)) {
	    /* either color name or X-styel rgb value "#rrggbb" */
	    char col_buf[21];
	    quote_str(col_buf, c_token++, 20);
	    if (col_buf[0] == '#') {
		/* X-style specifier */
		int rr,gg,bb;
		if (sscanf( col_buf, "#%2x%2x%2x", &rr, &gg, &bb ) != 3 )
		    int_error( c_token-1,
			       "Unknown color specifier. Use '#rrggbb'." );
		r = (double)(rr)/255.;
		g = (double)(gg)/255.;
		b = (double)(bb)/255.;
	    }
	    else { /* some predefined names */
		/* Maybe we could scan the X11 rgb.txt file to look up color
		 * names?  Or at least move these definitions to some file
		 * which is included somehow during compilation instead
		 * hardcoding them. */
		/* Can't use lookupt_table() as it works for tokens only,
		   so we'll do it manually */
		const struct gen_table *tbl = pm3d_color_names_tbl;
		while (tbl->key) {
		    if (!strcmp(col_buf, tbl->key)) {
			r = (double)((tbl->value >> 16 ) & 255) / 255.;
			g = (double)((tbl->value >> 8 ) & 255) / 255.;
			b = (double)(tbl->value & 255) / 255.;
			break;
		    }
		    tbl++;
		}
		if (!tbl->key)
		    int_error( c_token-1, "Unknown color name." );
		named_colors = 1;
	    }
	} else {
	    /* numerical rgb, hsv, xyz, ... values  [0,1] */
	    r = real_expression();
	    if (r<0 || r>1 )  int_error(c_token-1,"Value out of range [0,1].");
	    g = real_expression();
	    if (g<0 || g>1 )  int_error(c_token-1,"Value out of range [0,1].");
	    b = real_expression();
	    if (b<0 || b>1 )  int_error(c_token-1,"Value out of range [0,1].");
	}
	++num;

	if ( num >= actual_size ) {
	    /* get more space for the gradient */
	    actual_size += 10;
	    sm_palette.gradient = gp_realloc( sm_palette.gradient,
			  actual_size*sizeof(gradient_struct),
			  "pm3d gradient" );
	}
	sm_palette.gradient[num].pos = p;
	sm_palette.gradient[num].col.r = r;
	sm_palette.gradient[num].col.g = g;
	sm_palette.gradient[num].col.b = b;
	if (equals(c_token,")") ) break;
	if ( !equals(c_token,",") )
	    int_error( c_token, "Expected comma." );
	++c_token;

    }

    sm_palette.gradient_num = num + 1;
    check_palette_grayscale();

    return named_colors;
}


/*  process 'set palette file' command
 *  load a palette from file, honor datafile modifiers
 */
static void
set_palette_file()
{
    int specs;
    double v[4];
    int i, j, actual_size;
    char *file_name;

    ++c_token;

    /* get filename */
    if (!(file_name = try_to_get_string()))
	int_error(c_token, "missing filename");

    df_set_plot_mode(MODE_QUERY);	/* Needed only for binary datafiles */
    specs = df_open(file_name, 4, NULL);
    free(file_name);

    if (specs > 0 && specs < 3)
	int_error( c_token, "Less than 3 using specs for palette");

    if (sm_palette.gradient) {
	free( sm_palette.gradient );
	sm_palette.gradient = 0;
    }
    actual_size = 10;
    sm_palette.gradient =
      gp_alloc( actual_size*sizeof(gradient_struct), "gradient" );

    i = 0;

#define VCONSTRAIN(x) ( (x)<0 ? 0 : ( (x)>1 ? 1: (x) ) )
    /* values are simply clipped to [0,1] without notice */
    while ((j = df_readline(v, 4)) != DF_EOF) {
	if (i >= actual_size) {
	  actual_size += 10;
	  sm_palette.gradient = (gradient_struct*)
	    gp_realloc( sm_palette.gradient,
			actual_size*sizeof(gradient_struct),
			"pm3d gradient" );
	}
	switch (j) {
	    case 3:
		sm_palette.gradient[i].col.r = VCONSTRAIN(v[0]);
		sm_palette.gradient[i].col.g = VCONSTRAIN(v[1]);
		sm_palette.gradient[i].col.b = VCONSTRAIN(v[2]);
		sm_palette.gradient[i].pos = i ;
		break;
	    case 4:
		sm_palette.gradient[i].col.r = VCONSTRAIN(v[1]);
		sm_palette.gradient[i].col.g = VCONSTRAIN(v[2]);
		sm_palette.gradient[i].col.b = VCONSTRAIN(v[3]);
		sm_palette.gradient[i].pos = v[0];
		break;
	    default:
		df_close();
		int_error(c_token, "Bad data on line %d", df_line_number);
		break;
	}
	++i;
    }
#undef VCONSTRAIN
    df_close();
    if (i==0)
	int_error( c_token, "No valid palette found" );

    sm_palette.gradient_num = i;
    check_palette_grayscale();

}


/* Process a 'set palette function' command.
 *  Three functions with fixed dummy variable gray are registered which
 *  map gray to the different color components.
 *  If ALLOW_DUMMY_VAR_FOR_GRAY is set:
 *    A different dummy variable may proceed the formulae in quotes.
 *    This syntax is different from the usual '[u=<start>:<end>]', but
 *    as <start> and <end> are fixed to 0 and 1 you would have to type
 *    always '[u=]' which looks strange, especially as just '[u]'
 *    wouldn't work.
 *  If unset:  dummy variable is fixed to 'gray'.
 */
static void
set_palette_function()
{
    int start_token;
    char saved_dummy_var[MAX_ID_LEN+1];

    ++c_token;
    strncpy( saved_dummy_var, c_dummy_var[0], MAX_ID_LEN );

    /* set dummy variable */
#ifdef ALLOW_DUMMY_VAR_FOR_GRAY
    if (isstring(c_token)) {
	quote_str( c_dummy_var[0], c_token, MAX_ID_LEN );
	++c_token;
    }
    else
#endif /* ALLOW_DUMMY_VAR_FOR_GRAY */
    strncpy( c_dummy_var[0], "gray", MAX_ID_LEN );


    /* Afunc */
    start_token = c_token;
    if (sm_palette.Afunc.at) {
	free_at( sm_palette.Afunc.at );
	sm_palette.Afunc.at = NULL;
    }
    dummy_func = &sm_palette.Afunc;
    sm_palette.Afunc.at = perm_at();
    if (! sm_palette.Afunc.at)
	int_error(start_token, "not enough memory for function");
    m_capture(&(sm_palette.Afunc.definition), start_token, c_token-1);
    dummy_func = NULL;
    if (!equals(c_token,","))
	int_error(c_token,"Expected comma" );
    ++c_token;

    /* Bfunc */
    start_token = c_token;
    if (sm_palette.Bfunc.at) {
	free_at( sm_palette.Bfunc.at );
	sm_palette.Bfunc.at = NULL;
    }
    dummy_func = &sm_palette.Bfunc;
    sm_palette.Bfunc.at = perm_at();
    if (! sm_palette.Bfunc.at)
	int_error(start_token, "not enough memory for function");
    m_capture(&(sm_palette.Bfunc.definition), start_token, c_token-1);
    dummy_func = NULL;
    if (!equals(c_token,","))
	int_error(c_token,"Expected comma" );
    ++c_token;

    /* Cfunc */
    start_token = c_token;
    if (sm_palette.Cfunc.at) {
	free_at( sm_palette.Cfunc.at );
	sm_palette.Cfunc.at = NULL;
    }
    dummy_func = &sm_palette.Cfunc;
    sm_palette.Cfunc.at = perm_at();
    if (! sm_palette.Cfunc.at)
	int_error(start_token, "not enough memory for function");
    m_capture(&(sm_palette.Cfunc.definition), start_token, c_token-1);
    dummy_func = NULL;

    strncpy( c_dummy_var[0], saved_dummy_var, MAX_ID_LEN );
}


/*
 *  Normalize gray scale of gradient to fill [0,1] and
 *  complain if gray values are not strictly increasing.
 *  Maybe automatic sorting of the gray values could be a
 *  feature.
 */
static void
check_palette_grayscale()
{
    int i;
    double off, f;

    /* check if gray values are sorted */
    for (i=0; i<sm_palette.gradient_num-1; ++i ) {
	if (sm_palette.gradient[i].pos > sm_palette.gradient[i+1].pos) {
	    int_error( c_token, "Gray scale not sorted in gradient." );
	}
    }

    /* fit gray axis into [0:1]:  subtract offset and rescale */
    off = sm_palette.gradient[0].pos;
    f = 1.0 / ( sm_palette.gradient[sm_palette.gradient_num-1].pos-off );
    for (i=1; i<sm_palette.gradient_num-1; ++i ) {
	sm_palette.gradient[i].pos = f*(sm_palette.gradient[i].pos-off);
    }

    /* paranoia on the first and last entries */
    sm_palette.gradient[0].pos = 0.0;
    sm_palette.gradient[sm_palette.gradient_num-1].pos = 1.0;
}


#define SCAN_RGBFORMULA(formula) do {								       \
    c_token++;											       \
    i = int_expression();										       \
    if (abs(i) >= sm_palette.colorFormulae)							       \
	int_error(c_token,									       \
		  "color formula out of range (use `show palette rgbformulae' to display the range)"); \
    formula = i;										       \
} while(0)

#define CHECK_TRANSFORM  do {							  \
    if (transform_defined)							  \
	int_error(c_token,							  \
		  "Use either `rgbformulae`, `defined`, `file` or `formulae`." ); \
    transform_defined = 1;							  \
}  while(0)

/* Process 'set palette' command */
static void
set_palette()
{
    int transform_defined, named_color;
    transform_defined = named_color = 0;
    c_token++;

    if (END_OF_COMMAND) /* reset to default settings */
	reset_palette();
    else { /* go through all options of 'set palette' */
	for ( ; !END_OF_COMMAND; c_token++ ) {
	    switch (lookup_table(&set_palette_tbl[0],c_token)) {
	    /* positive and negative picture */
	    case S_PALETTE_POSITIVE: /* "pos$itive" */
		sm_palette.positive = SMPAL_POSITIVE;
		continue;
	    case S_PALETTE_NEGATIVE: /* "neg$ative" */
		sm_palette.positive = SMPAL_NEGATIVE;
		continue;
	    /* Now the options that determine the palette of smooth colours */
	    /* gray or rgb-coloured */
	    case S_PALETTE_GRAY: /* "gray" */
		sm_palette.colorMode = SMPAL_COLOR_MODE_GRAY;
		continue;
	    case S_PALETTE_GAMMA: /* "gamma" */
		++c_token;
		sm_palette.gamma = real_expression();
		--c_token;
		continue;
	    case S_PALETTE_COLOR: /* "col$or" */
		if (pm3d_last_set_palette_mode != SMPAL_COLOR_MODE_NONE) {
		    sm_palette.colorMode = pm3d_last_set_palette_mode;
		} else {
		sm_palette.colorMode = SMPAL_COLOR_MODE_RGB;
		}
		continue;
	    /* rgb color mapping formulae: rgb$formulae r,g,b (3 integers) */
	    case S_PALETTE_RGBFORMULAE: { /* "rgb$formulae" */
		int i;

		CHECK_TRANSFORM;
		SCAN_RGBFORMULA( sm_palette.formulaR );
		if (!equals(c_token,",")) { c_token--; continue; }
		SCAN_RGBFORMULA( sm_palette.formulaG );
		if (!equals(c_token,",")) { c_token--; continue; }
		SCAN_RGBFORMULA( sm_palette.formulaB );
		c_token--;
		sm_palette.colorMode = SMPAL_COLOR_MODE_RGB;
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_RGB;
		continue;
	    } /* rgbformulae */
	    case S_PALETTE_DEFINED: { /* "def$ine" */
		CHECK_TRANSFORM;
		++c_token;
		named_color = set_palette_defined();
		sm_palette.colorMode = SMPAL_COLOR_MODE_GRADIENT;
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
		continue;
	    }
	    case S_PALETTE_FILE: { /* "file" */
		CHECK_TRANSFORM;
		set_palette_file();
		sm_palette.colorMode = SMPAL_COLOR_MODE_GRADIENT;
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_GRADIENT;
		--c_token;
		continue;
	    }
	    case S_PALETTE_FUNCTIONS: { /* "func$tions" */
		CHECK_TRANSFORM;
		set_palette_function();
		sm_palette.colorMode = SMPAL_COLOR_MODE_FUNCTIONS;
		pm3d_last_set_palette_mode = SMPAL_COLOR_MODE_FUNCTIONS;
		--c_token;
		continue;
	    }
	    case S_PALETTE_MODEL: { /* "mo$del" */
		int model;

		++c_token;
		if (END_OF_COMMAND)
		    int_error( c_token, "Expected color model." );
		model = lookup_table(&color_model_tbl[0],c_token);
		if (model == -1)
		    int_error(c_token,"Unknown color model.");
		sm_palette.cmodel = model;
		continue;
	    }
	    /* ps_allcF: write all rgb formulae into PS file? */
	    case S_PALETTE_NOPS_ALLCF: /* "nops_allcF" */
		sm_palette.ps_allcF = 0;
		continue;
	    case S_PALETTE_PS_ALLCF: /* "ps_allcF" */
		sm_palette.ps_allcF = 1;
		continue;
	    /* max colors used */
	    case S_PALETTE_MAXCOLORS: { /* "maxc$olors" */
		int i;

		c_token++;
		i = int_expression();
		if (i<0) int_error(c_token,"non-negative number required");
		sm_palette.use_maxcolors = i;
		--c_token;
		continue;
	    }
	    } /* switch over palette lookup table */
	    int_error(c_token,"invalid palette option");
	} /* end of while !end of command over palette options */
    } /* else(arguments found) */

    if (named_color && sm_palette.cmodel != C_MODEL_RGB && interactive)
	int_warn(NO_CARET,
		 "Named colors will produce strange results if not in color mode RGB." );

    /* Invalidate previous palette */
    invalidate_palette();
}

#undef CHECK_TRANSFORM
#undef SCAN_RGBFORMULA

/* process 'set colorbox' command */
static void
set_colorbox()
{
    c_token++;

    if (END_OF_COMMAND) /* reset to default position */
	color_box.where = SMCOLOR_BOX_DEFAULT;
    else { /* go through all options of 'set colorbox' */
	for ( ; !END_OF_COMMAND; c_token++ ) {
	    switch (lookup_table(&set_colorbox_tbl[0],c_token)) {
	    /* vertical or horizontal color gradient */
	    case S_COLORBOX_VERTICAL: /* "v$ertical" */
		color_box.rotation = 'v';
		continue;
	    case S_COLORBOX_HORIZONTAL: /* "h$orizontal" */
		color_box.rotation = 'h';
		continue;
	    /* color box where: default position */
	    case S_COLORBOX_DEFAULT: /* "def$ault" */
		color_box.where = SMCOLOR_BOX_DEFAULT;
		continue;
	    /* color box where: position by user */
	    case S_COLORBOX_USER: /* "u$ser" */
		color_box.where = SMCOLOR_BOX_USER;
		continue;
	    /* color box layer: front or back */
	    case S_COLORBOX_FRONT: /* "fr$ont" */
		color_box.layer = LAYER_FRONT;
		continue;
	    case S_COLORBOX_BACK: /* "ba$ck" */
		color_box.layer = LAYER_BACK;
		continue;
	    /* border of the color box */
	    case S_COLORBOX_BORDER: /* "bo$rder" */

		color_box.border = 1;
		c_token++;

		if (!END_OF_COMMAND) {
		    /* expecting a border line type */
		    color_box.border_lt_tag = int_expression();
		    if (color_box.border_lt_tag <= 0) {
			color_box.border_lt_tag = 0;
			int_error(c_token, "tag must be strictly positive (see `help set style line')");
		    }
		    --c_token;
		}
		continue;
	    case S_COLORBOX_BDEFAULT: /* "bd$efault" */
		color_box.border_lt_tag = -1; /* use default border */
		continue;
	    case S_COLORBOX_NOBORDER: /* "nobo$rder" */
		color_box.border = 0;
		continue;
	    /* colorbox origin */
	    case S_COLORBOX_ORIGIN: /* "o$rigin" */
		c_token++;
		if (END_OF_COMMAND) {
		    int_error(c_token, "expecting screen value [0 - 1]");
		} else {
		    get_position_default(&color_box.origin, screen);
		}
		c_token--;
		continue;
	    /* colorbox size */
	    case S_COLORBOX_SIZE: /* "s$ize" */
		c_token++;
		if (END_OF_COMMAND) {
		    int_error(c_token, "expecting screen value [0 - 1]");
		} else {
		    get_position_default(&color_box.size, screen);
		}
		c_token--;
		continue;
	    } /* switch over colorbox lookup table */
	    int_error(c_token,"invalid colorbox option");
	} /* end of while !end of command over colorbox options */
    if (color_box.where == SMCOLOR_BOX_NO) /* default: draw at default position */
	color_box.where = SMCOLOR_BOX_DEFAULT;
    }
}


/* process 'set pm3d' command */
static void
set_pm3d()
{
    int c_token0 = ++c_token;

    if (END_OF_COMMAND) { /* assume default settings */
	pm3d_reset(); /* sets pm3d.implicit to PM3D_IMPLICIT and pm3d.where to "s" */
	pm3d.implicit = PM3D_IMPLICIT; /* for historical reasons */
    }
    else { /* go through all options of 'set pm3d' */
	for ( ; !END_OF_COMMAND; c_token++ ) {
	    switch (lookup_table(&set_pm3d_tbl[0],c_token)) {
	    /* where to plot */
	    case S_PM3D_AT: /* "at" */
		c_token++;
		if (get_pm3d_at_option(&pm3d.where[0]))
		    return; /* error */
		c_token--;
#if 1
		if (c_token == c_token0+1)
		    /* for historical reasons: if "at" is the first option of pm3d,
		     * like "set pm3d at X other_opts;", then implicit is switched on */
		    pm3d.implicit = PM3D_IMPLICIT;
#endif
		continue;
	    case S_PM3D_INTERPOLATE: /* "interpolate" */
		c_token++;
		if (END_OF_COMMAND) {
		    int_error(c_token, "expecting step values i,j");
		} else {
		    pm3d.interp_i = int_expression();
		    if(pm3d.interp_i < 1)
			pm3d.interp_i = 1;
		    if (!equals(c_token,","))
			int_error(c_token, "',' expected");
		    c_token++;
		    pm3d.interp_j = int_expression();
		    if (pm3d.interp_j < 1)
			pm3d.interp_j = 1;
		    c_token--;
                }
		continue;
	    /* forward and backward drawing direction */
	    case S_PM3D_SCANSFORWARD: /* "scansfor$ward" */
		pm3d.direction = PM3D_SCANS_FORWARD;
		continue;
	    case S_PM3D_SCANSBACKWARD: /* "scansback$ward" */
		pm3d.direction = PM3D_SCANS_BACKWARD;
		continue;
	    case S_PM3D_SCANS_AUTOMATIC: /* "scansauto$matic" */
		pm3d.direction = PM3D_SCANS_AUTOMATIC;
		continue;
	    case S_PM3D_DEPTH: /* "dep$thorder" */
		pm3d.direction = PM3D_DEPTH;
		continue;
	    /* flush scans: left, right or center */
	    case S_PM3D_FLUSH:  /* "fl$ush" */
		c_token++;
		if (almost_equals(c_token, "b$egin"))
		    pm3d.flush = PM3D_FLUSH_BEGIN;
		else if (almost_equals(c_token, "c$enter"))
		    pm3d.flush = PM3D_FLUSH_CENTER;
		else if (almost_equals(c_token, "e$nd"))
		    pm3d.flush = PM3D_FLUSH_END;
		else
		    int_error(c_token,"expecting flush 'begin', 'center' or 'end'");
		continue;
	    /* clipping method */
	    case S_PM3D_CLIP_1IN: /* "clip1$in" */
		pm3d.clip = PM3D_CLIP_1IN;
		continue;
	    case S_PM3D_CLIP_4IN: /* "clip4$in" */
		pm3d.clip = PM3D_CLIP_4IN;
		continue;
	    /* setup everything for plotting a map */
	    case S_PM3D_MAP: /* "map" */
		pm3d.where[0] = 'b'; pm3d.where[1] = 0; /* set pm3d at b */
		data_style = PM3DSURFACE;
		func_style = PM3DSURFACE;
		splot_map = TRUE;
		continue;
	    /* flushing triangles */
	    case S_PM3D_FTRIANGLES: /* "ftr$iangles" */
		pm3d.ftriangles = 1;
		continue;
	    case S_PM3D_NOFTRIANGLES: /* "noftr$iangles" */
		pm3d.ftriangles = 0;
		continue;
	    /* pm3d-specific hidden line overwrite */
	    case S_PM3D_HIDDEN: { /* "hi$dden3d" */
		c_token++;
		pm3d.hidden3d_tag = int_expression();
		--c_token;
		if (pm3d.hidden3d_tag <= 0) {
		    pm3d.hidden3d_tag = 0;
		    int_error(c_token,"tag must be strictly positive (see `help set style line')");
		}
		}
		continue;
	    case S_PM3D_NOHIDDEN: /* "nohi$dden3d" */
		pm3d.hidden3d_tag = 0;
		continue;
	    case S_PM3D_SOLID: /* "so$lid" */
	    case S_PM3D_NOTRANSPARENT: /* "notr$ansparent" */
#if PM3D_HAVE_SOLID
		pm3d.solid = 1;

#else
		if (interactive)
		    int_warn(c_token, "Deprecated syntax --- ignored");
#endif
		continue;
	    case S_PM3D_NOSOLID: /* "noso$lid" */
	    case S_PM3D_TRANSPARENT: /* "tr$ansparent" */
#if PM3D_HAVE_SOLID
		pm3d.solid = 0;
		continue;
#else
		if (interactive)
		    int_warn(c_token, "Deprecated syntax --- ignored");
#endif
	    case S_PM3D_IMPLICIT: /* "i$mplicit" */
	    case S_PM3D_NOEXPLICIT: /* "noe$xplicit" */
		pm3d.implicit = PM3D_IMPLICIT;
		continue;
	    case S_PM3D_NOIMPLICIT: /* "noi$mplicit" */
	    case S_PM3D_EXPLICIT: /* "e$xplicit" */
		pm3d.implicit = PM3D_EXPLICIT;
		continue;
	    case S_PM3D_WHICH_CORNER: /* "corners2color" */
		c_token++;
		if (equals(c_token, "mean"))
		    pm3d.which_corner_color = PM3D_WHICHCORNER_MEAN;
		else if (equals(c_token, "geomean"))
		    pm3d.which_corner_color = PM3D_WHICHCORNER_GEOMEAN;
		else if (equals(c_token, "median"))
		    pm3d.which_corner_color = PM3D_WHICHCORNER_MEDIAN;
		else if (equals(c_token, "min"))
		    pm3d.which_corner_color = PM3D_WHICHCORNER_MIN;
		else if (equals(c_token, "max"))
		    pm3d.which_corner_color = PM3D_WHICHCORNER_MAX;
		else if (equals(c_token, "c1"))
		    pm3d.which_corner_color = PM3D_WHICHCORNER_C1;
		else if (equals(c_token, "c2"))
		    pm3d.which_corner_color = PM3D_WHICHCORNER_C2;
		else if (equals(c_token, "c3"))
		    pm3d.which_corner_color = PM3D_WHICHCORNER_C3;
		else if (equals(c_token, "c4"))
		    pm3d.which_corner_color = PM3D_WHICHCORNER_C4;
		else
		    int_error(c_token,"expecting 'mean', 'geomean', 'median', 'c1', 'c2', 'c3' or 'c4'");
		continue;
	    } /* switch over pm3d lookup table */
	    int_error(c_token,"invalid pm3d option");
	} /* end of while !end of command over pm3d options */
	if (PM3D_SCANS_AUTOMATIC == pm3d.direction
	    && PM3D_FLUSH_BEGIN != pm3d.flush) {
	    pm3d.direction = PM3D_SCANS_FORWARD;
#if 0
	    /* be silent, don't print this warning */
	    /* Rather FIXME that this combination is supported? Shouldn't be
	       so big problem, I guess, just it is not implemented. */
	    fprintf(stderr, "pm3d: `scansautomatic' and `flush %s' are incompatible\n",
		PM3D_FLUSH_END == pm3d.flush ? "end": "center");
	    fputs("   => setting `scansforward'\n", stderr);
#endif
	}
    }
}


/* process 'set pointsize' command */
static void
set_pointsize()
{
    c_token++;
    if (END_OF_COMMAND)
	pointsize = 1.0;
    else
	pointsize = real_expression();
    if(pointsize <= 0) pointsize = 1;
}


/* process 'set polar' command */
static void
set_polar()
{
    c_token++;

    if (!polar) {
	if (!parametric) {
	    if (interactive)
		(void) fprintf(stderr,"\n\tdummy variable is t for curves\n");
	    strcpy (set_dummy_var[0], "t");
	}
	polar = TRUE;
	if (axis_array[T_AXIS].set_autoscale) {
	    /* only if user has not set a range manually */
	    axis_array[T_AXIS].set_min = 0.0;
	    /* 360 if degrees, 2pi if radians */
	    axis_array[T_AXIS].set_max = 2 * M_PI / ang2rad;
	}
    }
}

#ifdef EAM_OBJECTS
/* 
 * Process 'set object <tag> rectangle' command
 * set object {tag} rectangle {from <bottom_left> {to|rto} <top_right>}
 *                     {{at|center} <xcen>,<ycen> size <w>,<h>}
 *                     {fc|fillcolor <colorspec>} {lw|linewidth <lw>}
 *                     {fs <fillstyle>} {front|back|behind}
 *                     {default}
 * EAM Jan 2005
 */

static void
set_object()
{
    int tag;

    /* The next token must either be a tag or the object type */
    c_token++;
    if (almost_equals(c_token, "rect$angle"))
	tag = -1; /* We'll figure out what it really is later */
    else {
	tag = int_expression();
	if (tag <= 0)
	    int_error(c_token, "tag must be > zero");
    }

    if (almost_equals(c_token, "rect$angle")) {
	set_rectangle(tag);

    } else if (tag > 0) {
	/* Look for existing object with this tag */
	t_object *this_object = first_object;
	for (; this_object != NULL; this_object = this_object->next)
	     if (tag == this_object->tag)
		break;
	if (this_object && tag == this_object->tag
			&& this_object->object_type == OBJ_RECTANGLE) {
	    c_token--;
	    set_rectangle(tag);
	} else
	    int_error(c_token, "unknown object");

    } else
	int_error(c_token, "unrecognized object type");

}

static t_object *
new_object(int tag, int object_type)
{
    t_object *new = gp_alloc(sizeof(struct object), "object");
    if (object_type == OBJ_RECTANGLE) {
	t_object def = DEFAULT_RECTANGLE_STYLE;
	*new = def;
	new->tag = tag;
	new->object_type = object_type;
	new->lp_properties.l_type = LT_DEFAULT; /* Use default rectangle color */
	new->fillstyle.fillstyle = FS_DEFAULT;  /* and default fill style */
    } else
	fprintf(stderr,"object initialization failure\n");
    return new;
}

static void
set_rectangle(int tag)
{
    t_rectangle *this_rect = NULL;
    t_object *this_object = NULL;
    t_object *new_obj = NULL;
    t_object *prev_object = NULL;
    TBOOLEAN got_fill = FALSE;
    TBOOLEAN got_lt = FALSE;
    TBOOLEAN got_lw = FALSE;
    TBOOLEAN got_corners = FALSE;
    TBOOLEAN got_center = FALSE;
    double lw = 1.0;

    c_token++;
    
    /* We are setting the default, not any particular rectangle */
    if (tag < -1) {
	this_object = &default_rectangle;
	this_rect = &default_rectangle.o.rectangle;
	c_token--;

    } else {
	/* Look for existing rectangle with this tag */
	for (this_object = first_object; this_object != NULL;
	     prev_object = this_object, this_object = this_object->next)
	     /* is this the rect we want? */
	     if (0 < tag  &&  tag <= this_object->tag)
		break;

	/* Insert this rect into the list if it is a new one */
	if (this_object == NULL || tag != this_object->tag) {
	    if (tag == -1)
		tag = (prev_object) ? prev_object->tag+1 : 1;
	    new_obj = new_object(tag, OBJ_RECTANGLE);
	    if (prev_object == NULL)
		first_object = new_obj;
	    else
		prev_object->next = new_obj;
	    new_obj->next = this_object;
	    this_object = new_obj;
	}
	this_rect = &this_object->o.rectangle;
    }

    while (!END_OF_COMMAND) {
	int save_token = c_token;

	if (equals(c_token,"from")) {
	    /* Read in the bottom left and upper right corners */
	    c_token++;
	    get_position(&this_rect->bl);
	    if (equals(c_token,"to")) {
		c_token++;
		get_position(&this_rect->tr);
	    } else if (equals(c_token,"rto")) {
		c_token++;
		get_position_default(&this_rect->tr,this_rect->bl.scalex);
		if (this_rect->bl.scalex != this_rect->tr.scalex
		||  this_rect->bl.scaley != this_rect->tr.scaley)
		    int_error(c_token,"relative coordinates must match in type");
		this_rect->tr.x += this_rect->bl.x;
		this_rect->tr.y += this_rect->bl.y;
	    } else
		int_error(c_token,"Expecting to or rto");
	    got_corners = TRUE;
	    this_rect->type = 0;
	    continue;

	} else if (equals(c_token,"at") || almost_equals(c_token,"cen$ter")) {
	    /* Read in the center position */
	    c_token++;
	    get_position(&this_rect->center);
	    got_center = TRUE;
	    this_rect->type = 1;
	    continue;
	} else if (equals(c_token,"size")) {
	    /* Read in the width and height */
	    c_token++;
	    get_position(&this_rect->extent);
	    got_center = TRUE;
	    this_rect->type = 1;
	    continue;

	} else if (equals(c_token,"front")) {
	    this_object->layer = 1;
	    c_token++;
	    continue;
	} else if (equals(c_token,"back")) {
	    this_object->layer = 0;
	    c_token++;
	    continue;
	} else if (equals(c_token,"behind")) {
	    this_object->layer = -1;
	    c_token++;
	    continue;
	} else if (almost_equals(c_token,"def$ault")) {
	    if (tag < 0) {
		int_error(c_token,
		    "Invalid command - did you mean 'unset style rectangle'?");
	    } else {
		this_object->lp_properties.l_type = LT_DEFAULT;
		this_object->fillstyle.fillstyle = FS_DEFAULT;
	    }
	    got_fill = got_lt = TRUE;
	    c_token++;
	    continue;
	}

	/* Now parse the style options; default to whatever the global style is  */
	if (!got_fill) {
	    if (new_obj)
		parse_fillstyle(&this_object->fillstyle, default_rectangle.fillstyle.fillstyle,
			default_rectangle.fillstyle.filldensity, default_rectangle.fillstyle.fillpattern,
			default_rectangle.fillstyle.border_linetype);
	    else
		parse_fillstyle(&this_object->fillstyle, this_object->fillstyle.fillstyle,
			this_object->fillstyle.filldensity, this_object->fillstyle.fillpattern,
			this_object->fillstyle.border_linetype);
	    if (c_token != save_token) {
		got_fill = TRUE;
		continue;
	    }
	}

	/* Parse the colorspec */
	if (!got_lt) {
	    if (equals(c_token,"fc") || almost_equals(c_token,"fillc$olor")) {
		this_object->lp_properties.use_palette = TRUE;
		this_object->lp_properties.l_type = LT_BLACK; /* Anything but LT_DEFAULT */
		parse_colorspec(&this_object->lp_properties.pm3d_color, TC_FRAC);
		if (this_object->lp_properties.pm3d_color.type == TC_DEFAULT)
		    this_object->lp_properties.l_type = LT_DEFAULT;
	    }

	    if (c_token != save_token) {
		got_lt = TRUE;
		continue;
	    }
	}

	/* And linewidth */
	if (!got_lw) {
	    if (equals(c_token,"lw") || almost_equals(c_token,"linew$idth")) {
		c_token++;
		lw = real_expression();
	    }
	    if (c_token != save_token) {
		got_lw = TRUE;
		continue;
	    }
	}

	int_error(c_token, "Unrecognized or duplicate option");
    }

    if (got_lw)
	this_object->lp_properties.l_width = lw;

    if (got_center && got_corners)
	int_error(NO_CARET,"Inconsistent options");

}
#endif

/* process 'set samples' command */
static void
set_samples()
{
    int tsamp1, tsamp2;

    c_token++;
    tsamp1 = abs(int_expression());
    tsamp2 = tsamp1;
    if (!END_OF_COMMAND) {
	if (!equals(c_token,","))
	    int_error(c_token, "',' expected");
	c_token++;
	tsamp2 = abs(int_expression());
    }
    if (tsamp1 < 2 || tsamp2 < 2)
	int_error(c_token, "sampling rate must be > 1; sampling unchanged");
    else {
	struct surface_points *f_3dp = first_3dplot;

	first_3dplot = NULL;
	sp_free(f_3dp);

	samples_1 = tsamp1;
	samples_2 = tsamp2;
    }
}


/* process 'set size' command */
static void
set_size()
{
    c_token++;
    if (END_OF_COMMAND) {
	xsize = 1.0;
	ysize = 1.0;
    } else {
	if (almost_equals(c_token, "sq$uare")) {
	    aspect_ratio = 1.0;
	    ++c_token;
	} else if (almost_equals(c_token,"ra$tio")) {
	    ++c_token;
	    aspect_ratio = real_expression();
	} else if (almost_equals(c_token, "nora$tio") || almost_equals(c_token, "nosq$uare")) {
	    aspect_ratio = 0.0;
	    ++c_token;
	}

	if (!END_OF_COMMAND) {
	    xsize = real_expression();
	    if (equals(c_token,",")) {
		c_token++;
		ysize = real_expression();
	    } else {
		ysize = xsize;
	    }
	}
    }
    if (xsize <= 0 || ysize <=0) {
	xsize = ysize = 1.0;
	int_error(NO_CARET,"Illegal value for size");
    }
}


/* process 'set style' command */
static void
set_style()
{
    c_token++;

    switch(lookup_table(&show_style_tbl[0],c_token)){
    case SHOW_STYLE_DATA:
	data_style = get_style();
	if (data_style == FILLEDCURVES) {
	    get_filledcurves_style_options(&filledcurves_opts_data);
	    if (!filledcurves_opts_data.opt_given) /* default value */
		filledcurves_opts_data.closeto = FILLEDCURVES_CLOSED;
	}
	break;
    case SHOW_STYLE_FUNCTION:
	{
	    enum PLOT_STYLE temp_style = get_style();

	    if (temp_style & PLOT_STYLE_HAS_ERRORBAR
#ifdef EAM_DATASTRINGS
	       || (temp_style == LABELPOINTS)
#endif
#ifdef EAM_HISTOGRAMS
		|| (temp_style == HISTOGRAMS)
#endif
	        )
		int_error(c_token, "style not usable for function plots, left unchanged");
	    else
		func_style = temp_style;
	    if (func_style == FILLEDCURVES) {
		get_filledcurves_style_options(&filledcurves_opts_func);
		if (!filledcurves_opts_func.opt_given) /* default value */
		    filledcurves_opts_func.closeto = FILLEDCURVES_CLOSED;
	    }
	    break;
	}
    case SHOW_STYLE_LINE:
	set_linestyle();
	break;
    case SHOW_STYLE_FILLING:
	parse_fillstyle( &default_fillstyle,
			default_fillstyle.fillstyle,
			default_fillstyle.filldensity,
			default_fillstyle.fillpattern,
			default_fillstyle.border_linetype);
	break;
    case SHOW_STYLE_ARROW:
	set_arrowstyle();
	break;
#ifdef EAM_OBJECTS
    case SHOW_STYLE_RECTANGLE:
	c_token++;
	set_rectangle(-2);
	break;
#endif
#ifdef EAM_HISTOGRAMS
    case SHOW_STYLE_HISTOGRAM:
	parse_histogramstyle(&histogram_opts,HT_CLUSTERED,histogram_opts.gap);
	break;
#endif
    case SHOW_STYLE_INCREMENT:
	c_token++;
	if (END_OF_COMMAND || almost_equals(c_token,"def$ault"))
	    prefer_line_styles = FALSE;
	else if (almost_equals(c_token,"u$serstyles"))
	    prefer_line_styles = TRUE;
	c_token++;
	break;
    default:
	int_error(c_token,
		  "expecting 'data', 'function', 'line', 'fill' or 'arrow'" );
    }
}


/* process 'set surface' command */
static void
set_surface()
{
    c_token++;
    draw_surface = TRUE;
}


/* process 'set table' command */
static void
set_table()
{
    char *tablefile;

    c_token++;

    if (table_outfile) {
	fclose(table_outfile);
	table_outfile = NULL;
    }

    if ((tablefile = try_to_get_string())) {
    /* 'set table "foo"' creates a new output file */
	if (!(table_outfile = fopen(tablefile, "w")))
	   os_error(c_token, "cannot open table output file");
	free(tablefile);
    }

    table_mode = TRUE;

}


/* process 'set terminal' comamnd */
static void
set_terminal()
{
    c_token++;

    if (multiplot)
	int_error(c_token, "You can't change the terminal in multiplot mode");

    if (END_OF_COMMAND) {
	list_terms();
	screen_ok = FALSE;
	return;
    }

#ifdef BACKWARDS_COMPATIBLE
    if (equals(c_token,"table")) {
	set_table();
	if (interactive)
	    int_warn(NO_CARET,"The command 'set term table' is deprecated.\n\t Please use 'set table \"outfile\"' instead.\n");
	return;
    } else
	table_mode = FALSE;
#endif

    /* `set term push' */
    if (equals(c_token,"push")) {
	push_terminal(interactive);
	c_token++;
	return;
    } /* set term push */

#ifdef EXTENDED_COLOR_SPECS
    /* each terminal is supposed to turn this on, probably
     * somewhere when the graphics is initialized */
    supply_extended_color_specs = 0;
#endif
#ifdef USE_MOUSE
    event_reset((void *)1);   /* cancel zoombox etc. */
#endif
    term_reset();

    /* `set term pop' */
    if (equals(c_token,"pop")) {
	pop_terminal();
	c_token++;
	return;
    } /* set term pop */

    /* `set term <normal terminal>' */
    term = 0; /* in case set_term() fails */
    term = set_term(c_token);
    c_token++;
    /* get optional mode parameters
     * not all drivers reset the option string before
     * strcat-ing to it, so we reset it for them
     */
    *term_options = 0;
    if (term)
	(*term->options)();
    if (interactive && *term_options)
	fprintf(stderr,"Options are '%s'\n",term_options);
}


/* 
 * Accept a single terminal option to apply to the current terminal if
 * possible.  The options are intended to be limited to those which apply
 * to a large number of terminals.  It would be nice also to limit it to
 * options for which we can test in advance to see if the terminal will
 * support it; that allows us to silently ignore the command rather than
 * issuing an error when the current terminal would not be affected anyhow.
 *
 * If necessary, the code in term->options() can detect that it was called
 * from here because in this case (c_token == 2), whereas when called from 
 * 'set term foo ...' it will see (c_token == 3).
 */

static void
set_termoptions()
{
    int save_end_of_line = num_tokens;
    c_token++;

    if (END_OF_COMMAND || !term) {
	return;
    } else if (almost_equals(c_token,"enh$anced")
           ||  almost_equals(c_token,"noenh$anced")) {
	num_tokens = GPMIN(num_tokens,c_token+1);
	if (term->enhanced_open) {
	    *term_options = 0;
	    (term->options)();
	} else
	    c_token++;
    } else if (almost_equals(c_token,"font")
           ||  almost_equals(c_token,"fname")) {
	num_tokens = GPMIN(num_tokens,c_token+2);
	if (term->set_font) {
	    *term_options = 0;
	    (term->options)();
	} else
	    c_token += 2;
    } else if (!strcmp(term->name,"gif") && equals(c_token,"delay") && num_tokens==4) {
	*term_options = 0;
	(term->options)();
    } else {
	int_error(c_token,"This option cannot be changed using 'set termoption'");
    }
    num_tokens = save_end_of_line;
}


/* process 'set tics' command */
static void
set_tics()
{
    unsigned int i = 0;
    TBOOLEAN axisset = FALSE;
    TBOOLEAN mirror_opt = FALSE; /* set to true if (no)mirror option specified) */

    ++c_token;

    if (END_OF_COMMAND) {
	for (i = 0; i < AXIS_ARRAY_SIZE; ++i)
	    axis_array[i].tic_in = TRUE;
    }

    while (!END_OF_COMMAND) {
	if (almost_equals(c_token, "ax$is")) {
	    axisset = TRUE;
	    for (i = 0; i < AXIS_ARRAY_SIZE; ++i) {
		axis_array[i].ticmode &= ~TICS_ON_BORDER;
		axis_array[i].ticmode |= TICS_ON_AXIS;
	    }
	    ++c_token;
	} else if (almost_equals(c_token, "bo$rder")) {
	    for (i = 0; i < AXIS_ARRAY_SIZE; ++i) {
		axis_array[i].ticmode &= ~TICS_ON_AXIS;
		axis_array[i].ticmode |= TICS_ON_BORDER;
	    }
	    ++c_token;
	} else if (almost_equals(c_token, "mi$rror")) {
	    for (i = 0; i < AXIS_ARRAY_SIZE; ++i)
		axis_array[i].ticmode |= TICS_MIRROR;
    	    mirror_opt = TRUE;
	    ++c_token;
	} else if (almost_equals(c_token, "nomi$rror")) {
	    for (i = 0; i < AXIS_ARRAY_SIZE; ++i)
		axis_array[i].ticmode &= ~TICS_MIRROR;
	    mirror_opt = TRUE;
	    ++c_token;
	} else if (almost_equals(c_token,"in$wards")) {
	    for (i = 0; i < AXIS_ARRAY_SIZE; ++i)
		axis_array[i].tic_in = TRUE;
	    ++c_token;
	} else if (almost_equals(c_token,"out$wards")) {
	    for (i = 0; i < AXIS_ARRAY_SIZE; ++i)
		axis_array[i].tic_in = FALSE;
	    ++c_token;
	} else if (almost_equals(c_token, "sc$ale")) {
	    ++c_token;
	    if (almost_equals(c_token, "def$ault")) {
		for (i = 0; i < AXIS_ARRAY_SIZE; ++i) {
		    axis_array[i].ticscale = 1.0;
		    axis_array[i].miniticscale = 0.5;
		}
		++c_token;
	    } else {
		double lticscale, lminiticscale;
		lticscale = real_expression();
		if (equals(c_token, ",")) {
		    ++c_token;
		    lminiticscale = real_expression();
		} else
		    lminiticscale = 0.5 * lticscale;
		for (i = 0; i < AXIS_ARRAY_SIZE; ++i) {
		    axis_array[i].ticscale = lticscale;
		    axis_array[i].miniticscale = lminiticscale;
		}
	    }
	} else if (almost_equals(c_token, "ro$tate")) {
	    axis_array[i].tic_rotate = TEXT_VERTICAL;
	    ++c_token;
	    if (equals(c_token, "by")) {
		int langle;
		++c_token;
		langle = int_expression();
		for (i = 0; i < AXIS_ARRAY_SIZE; ++i)
		    axis_array[i].tic_rotate = langle;
	    }
	} else if (almost_equals(c_token, "noro$tate")) {
	    for (i = 0; i < AXIS_ARRAY_SIZE; ++i)
		axis_array[i].tic_rotate = 0;
	    ++c_token;
	} else if (almost_equals(c_token, "off$set")) {
	    struct position lpos;
	    ++c_token;
	    get_position_default(&lpos, character);
	    for (i = 0; i < AXIS_ARRAY_SIZE; ++i)
		axis_array[i].ticdef.offset = lpos;
	} else if (almost_equals(c_token, "nooff$set")) {
	    struct position tics_nooffset =
		{ character, character, character, 0., 0., 0.};
	    ++c_token;
	    for (i = 0; i < AXIS_ARRAY_SIZE; ++i)
		axis_array[i].ticdef.offset = tics_nooffset;
	} else if (almost_equals(c_token, "f$ont")) {
	    ++c_token;
	    /* Make sure they've specified a font */
	    if (!isstringvalue(c_token))
		int_error(c_token,"expected font");
	    else {
		char *lfont = try_to_get_string();
		for (i = 0; i < AXIS_ARRAY_SIZE; ++i) {
		    free(axis_array[i].ticdef.font);
		    axis_array[i].ticdef.font = gp_strdup(lfont);
		}
		free(lfont);
	    }
	} else if (equals(c_token,"tc") ||
		   almost_equals(c_token,"text$color")) {
	    struct t_colorspec lcolor;
	    parse_colorspec(&lcolor, TC_FRAC);
	    for (i = 0; i < AXIS_ARRAY_SIZE; ++i)
		axis_array[i].ticdef.textcolor = lcolor;
	} else if (equals(c_token,"front")) {
	    grid_layer = 1;
	    ++c_token;
	} else if (equals(c_token,"back")) {
	    grid_layer = 0;
	    ++c_token;
	} else if (!END_OF_COMMAND) {
	    int_error(c_token, "extraneous arguments in set tics");
	}
    }

    /* if tics are off and not set by axis, reset to default (border) */
    for (i = 0; i < AXIS_ARRAY_SIZE; ++i) {
	if (((axis_array[i].ticmode & TICS_MASK) == NO_TICS) && (!axisset)) {
	    if ((i == SECOND_X_AXIS) || (i == SECOND_Y_AXIS))
		continue; /* don't switch on secondary axes by default */
	    axis_array[i].ticmode = TICS_ON_BORDER;
	    if ((mirror_opt == FALSE) && ((i == FIRST_X_AXIS) || (i == FIRST_Y_AXIS) || (i == COLOR_AXIS))) {
		axis_array[i].ticmode |= TICS_MIRROR;
	    }
	}
    }
}


/* process 'set ticscale' command */
static void
set_ticscale()
{
    double lticscale, lminiticscale;
    unsigned int i;

    int_warn(c_token,
	     "Deprecated syntax - please use 'set tics scale' keyword");

    ++c_token;
    if (END_OF_COMMAND) {
	lticscale = 1.0;
	lminiticscale = 0.5;
    } else {
	lticscale = real_expression();
	if (END_OF_COMMAND) {
	    lminiticscale = lticscale*0.5;
	} else {
	    if (equals(c_token, ","))
		++c_token;
	    lminiticscale = real_expression();
	}
    }
    for (i = 0; i < AXIS_ARRAY_SIZE; ++i) {
	axis_array[i].ticscale = lticscale;
	axis_array[i].miniticscale = lminiticscale;
    }
}


/* process 'set ticslevel' command */
/* is datatype 'time' relevant here ? */
static void
set_ticslevel()
{
    c_token++;
    xyplane.z = real_expression();
    xyplane.absolute = FALSE;
}


/* process 'set xyplane' command */
/* is datatype 'time' relevant here ? */
static void
set_xyplane()
{
    if (equals(++c_token, "at")) {
	c_token++;
	xyplane.z = real_expression();
	xyplane.absolute = TRUE;
    } else
	int_error(c_token, "use 'ticslevel' for relative placement");
}


/* Process 'set timefmt' command */
/* HBB 20000507: changed this to a per-axis setting. I.e. you can now
 * have separate timefmt parse strings, different axes */
static void
set_timefmt()
{
    int axis;

    c_token++;
    if (END_OF_COMMAND) {
	/* set all axes to default */
	for (axis = 0; axis < AXIS_ARRAY_SIZE; axis++)
	    strcpy(axis_array[axis].timefmt,TIMEFMT);
    } else {
	if ((axis = lookup_table(axisname_tbl, c_token)) >= 0) {
	    c_token++;
	if (isstring(c_token)) {
		quote_str(axis_array[axis].timefmt,c_token, MAX_ID_LEN);
		c_token++;
	    } else {
		int_error(c_token, "time format string expected");
	}
	} else if (isstring(c_token)) {
	    /* set the given parse string for all current timedata axes: */
	    for (axis = 0; axis < AXIS_ARRAY_SIZE; axis++)
		quote_str(axis_array[axis].timefmt, c_token, MAX_ID_LEN);
	c_token++;
	} else {
	    int_error(c_token, "time format string expected");
	}
    }
}


/* process 'set timestamp' command */
static void
set_timestamp()
{
    TBOOLEAN got_format = FALSE;
    char *new;

    c_token++;

    while (!END_OF_COMMAND) {

	if (almost_equals(c_token,"t$op")) {
	    timelabel_bottom = FALSE;
	    c_token++;
	    continue;
	} else if (almost_equals(c_token, "b$ottom")) {
	    timelabel_bottom = TRUE;
	    c_token++;
	    continue;
	}

	if (almost_equals(c_token,"r$otate")) {
	    timelabel_rotate = TRUE;
	    c_token++;
	    continue;
	} else if (almost_equals(c_token, "n$orotate")) {
	    timelabel_rotate = FALSE;
	    c_token++;
	    continue;
	}

	if (almost_equals(c_token,"off$set")) {
	    c_token++;
	    get_position_default(&(timelabel.offset),character);
	    continue;
	}

	if (equals(c_token,"font")) {
	    c_token++;
	    new = try_to_get_string();
	    free(timelabel.font);
	    timelabel.font = new;
	    continue;
	}

	if (!got_format && ((new = try_to_get_string()))) {
	    /* we have a format string */
	    free(timelabel.text);
	    timelabel.text = new;
	    got_format = TRUE;
	    continue;
	}

#ifdef BACKWARDS_COMPATIBLE
	/* The "font" keyword is new (v4.1), for backward compatibility we don't enforce it */
	if (!END_OF_COMMAND && ((new = try_to_get_string()))) {
	    free(timelabel.font);
	    timelabel.font = new;
	    continue;
	}
	/* The "offset" keyword is new (v4.1); for backward compatibility we don't enforce it */
	get_position_default(&(timelabel.offset),character);
#endif

    }

    if (!(timelabel.text))
	timelabel.text = gp_strdup(DEFAULT_TIMESTAMP_FORMAT);

}


/* process 'set view' command */
static void
set_view()
{
    int i;
    TBOOLEAN was_comma = TRUE;
    static const char errmsg1[] = "rot_%c must be in [0:%d] degrees range; view unchanged";
    static const char errmsg2[] = "%sscale must be > 0; view unchanged";
    double local_vals[4];

    c_token++;
    if (equals(c_token,"map")) {
	    splot_map = TRUE;
	    c_token++;
	    return;
    };

    if (splot_map == TRUE) {
	splot_map_deactivate();
	splot_map = FALSE; /* default is no map */
    }

    local_vals[0] = surface_rot_x;
    local_vals[1] = surface_rot_z;
    local_vals[2] = surface_scale;
    local_vals[3] = surface_zscale;
    for (i = 0; i < 4 && !(END_OF_COMMAND);) {
	if (equals(c_token,",")) {
	    if (was_comma) i++;
	    was_comma = TRUE;
	    c_token++;
	} else {
	    if (!was_comma)
		int_error(c_token, "',' expected");
	    local_vals[i] = real_expression();
	    i++;
	    was_comma = FALSE;
	}
    }

    if (local_vals[0] < 0 || local_vals[0] > 180)
	int_error(c_token, errmsg1, 'x', 180);
    if (local_vals[1] < 0 || local_vals[1] > 360)
	int_error(c_token, errmsg1, 'z', 360);
    if (local_vals[2] < 1e-6)
	int_error(c_token, errmsg2, "");
    if (local_vals[3] < 1e-6)
	int_error(c_token, errmsg2, "z");

    surface_rot_x = local_vals[0];
    surface_rot_z = local_vals[1];
    surface_scale = local_vals[2];
    surface_zscale = local_vals[3];

}


/* process 'set zero' command */
static void
set_zero()
{
    struct value a;
    c_token++;
    zero = magnitude(const_express(&a));
}


/* process 'set {x|y|z|x2|y2}data' command */
static void
set_timedata(AXIS_INDEX axis)
{
    c_token++;
    if(END_OF_COMMAND) {
	axis_array[axis].is_timedata = FALSE;
    } else {
	if ((axis_array[axis].is_timedata = almost_equals(c_token,"t$ime")))
	    c_token++;
}
}


static void
set_range(AXIS_INDEX axis)
{
    c_token++;

    if (splot_map)
	splot_map_deactivate();

    if(almost_equals(c_token,"re$store")) { /* ULIG */
	c_token++;
	axis_array[axis].set_min = get_writeback_min(axis);
	axis_array[axis].set_max = get_writeback_max(axis);
	axis_array[axis].set_autoscale = AUTOSCALE_NONE;
    } else {
	if (!equals(c_token,"["))
	    int_error(c_token, "expecting '[' or 'restore'");
	c_token++;
	axis_array[axis].set_autoscale =
	    load_range(axis,
		       &axis_array[axis].set_min,&axis_array[axis].set_max,
		       axis_array[axis].set_autoscale);
	if (!equals(c_token,"]"))
	    int_error(c_token, "expecting ']'");
	c_token++;
	if (almost_equals(c_token, "rev$erse")) {
	    ++c_token;
	    axis_array[axis].range_flags |= RANGE_REVERSE;
	} else if (almost_equals(c_token, "norev$erse")) {
	    ++c_token;
	    axis_array[axis].range_flags &= ~RANGE_REVERSE;
	}
	if (almost_equals(c_token, "wr$iteback")) {
	    ++c_token;
	    axis_array[axis].range_flags |= RANGE_WRITEBACK;
	} else if (almost_equals(c_token, "nowri$teback")) {
	    ++c_token;
	    axis_array[axis].range_flags &= ~RANGE_WRITEBACK;
	}
    }
    if (splot_map)
	splot_map_activate();
}

/* process 'set {xyz}zeroaxis' command */
static void
set_zeroaxis(AXIS_INDEX axis)
{

    c_token++;
    if (END_OF_COMMAND)
	axis_array[axis].zeroaxis.l_type = -1;
    else {
	int old_token = c_token;
	axis_array[axis].zeroaxis.l_type = LT_AXIS;
	lp_parse(&axis_array[axis].zeroaxis, TRUE, FALSE);
	if (old_token == c_token)
	    axis_array[axis].zeroaxis.l_type = int_expression() - 1;
	}

}

/* process 'set zeroaxis' command */
static void
set_allzeroaxis()
{
    set_zeroaxis(FIRST_X_AXIS);
    axis_array[FIRST_Y_AXIS].zeroaxis = axis_array[FIRST_X_AXIS].zeroaxis;
#ifndef BACKWARDS_COMPATIBLE
    axis_array[FIRST_Z_AXIS].zeroaxis = axis_array[FIRST_X_AXIS].zeroaxis;
#endif
}

/*********** Support functions for set_command ***********/

/*
 * The set.c PROCESS_TIC_PROP macro has the following characteristics:
 *   (a) options must in the correct order
 *   (b) 'set xtics' (no option) resets only the interval (FREQ)
 *       {it will also negate NO_TICS, see (d)}
 *   (c) changing any property also resets the interval to automatic
 *   (d) set no[xy]tics; set [xy]tics changes border to nomirror, rather
 *       than to the default, mirror.
 *   (e) effect of 'set no[]tics; set []tics border ...' is compiler
 *       dependent;  if '!(TICS)' is evaluated first, 'border' is an
 *       undefined variable :-(
 *
 * This function replaces the macro, and introduces a new option
 * 'au$tofreq' to give somewhat different behaviour:
 *   (a) no change
 *   (b) 'set xtics' (no option) only affects NO_TICS;  'autofreq' resets
 *       the interval calulation to automatic
 *   (c) the interval mode is not affected by changing some other option
 *   (d) if NO_TICS, set []tics will restore defaults (borders, mirror
 *       where appropriate)
 *   (e) if (NO_TICS), border option is processed.
 *
 *  A 'default' option could easily be added to reset all options to
 *  the initial values - mostly book-keeping.
 *
 *  To retain tic properties after setting no[]tics may also be
 *  straightforward (save value as negative), but requires changes
 *  in other code ( e.g. for  'if (xtics)', use 'if (xtics > 0)'
 */

/*    generates PROCESS_TIC_PROP strings from tic_side, e.g. "x2"
 *  STRING, NOSTRING, MONTH, NOMONTH, DAY, NODAY, MINISTRING, NOMINI
 *  "nox2t$ics"     "nox2m$tics"  "nox2d$tics"    "nomx2t$ics"
 */

static int
set_tic_prop(AXIS_INDEX axis)
{
    int match = 0;		/* flag, set by matching a tic command */
    char nocmd[12];		/* fill w/ "no"+axis_name+suffix */
    char *cmdptr, *sfxptr;

    (void) strcpy(nocmd, "no");
    cmdptr = &nocmd[2];
    (void) strcpy(cmdptr, axis_defaults[axis].name);
    sfxptr = &nocmd[strlen(nocmd)];
    (void) strcpy(sfxptr, "t$ics");	/* STRING */

    if (almost_equals(c_token, cmdptr)) {
	TBOOLEAN axisset = FALSE;
	TBOOLEAN mirror_opt = FALSE; /* set to true if (no)mirror option specified) */
	axis_array[axis].ticdef.def.mix = FALSE;
	match = 1;
	++c_token;
	do {
	    if (almost_equals(c_token, "ax$is")) {
		axisset = TRUE;
		axis_array[axis].ticmode &= ~TICS_ON_BORDER;
		axis_array[axis].ticmode |= TICS_ON_AXIS;
		++c_token;
	    } else if (almost_equals(c_token, "bo$rder")) {
		axis_array[axis].ticmode &= ~TICS_ON_AXIS;
		axis_array[axis].ticmode |= TICS_ON_BORDER;
		++c_token;
	    } else if (almost_equals(c_token, "mi$rror")) {
		axis_array[axis].ticmode |= TICS_MIRROR;
		mirror_opt = TRUE;
		++c_token;
	    } else if (almost_equals(c_token, "nomi$rror")) {
		axis_array[axis].ticmode &= ~TICS_MIRROR;
		mirror_opt = TRUE;
		++c_token;
	    } else if (almost_equals(c_token, "in$wards")) {
		axis_array[axis].tic_in = TRUE;
		++c_token;
	    } else if (almost_equals(c_token, "out$wards")) {
		axis_array[axis].tic_in = FALSE;
		++c_token;
	    } else if (almost_equals(c_token, "sc$ale")) {
		++c_token;
		if (almost_equals(c_token, "def$ault")) {
		    axis_array[axis].ticscale = 1.0;
		    axis_array[axis].miniticscale = 0.5;
		    ++c_token;
		} else {
		    axis_array[axis].ticscale = real_expression();
		    if (equals(c_token, ",")) {
			++c_token;
			axis_array[axis].miniticscale = real_expression();
		    } else
			axis_array[axis].miniticscale =
			    0.5 * axis_array[axis].ticscale;
		}
	    } else if (almost_equals(c_token, "ro$tate")) {
		axis_array[axis].tic_rotate = TEXT_VERTICAL;
		++c_token;
		if (equals(c_token, "by")) {
		    c_token++;
		    axis_array[axis].tic_rotate = int_expression();
		}
	    } else if (almost_equals(c_token, "noro$tate")) {
		axis_array[axis].tic_rotate = 0;
		++c_token;
	    } else if (almost_equals(c_token, "off$set")) {
		++c_token;
		get_position_default(&axis_array[axis].ticdef.offset,
				     character);
	    } else if (almost_equals(c_token, "nooff$set")) {
		struct position tics_nooffset =
		    { character, character, character, 0., 0., 0.};
		++c_token;
		axis_array[axis].ticdef.offset = tics_nooffset;
	    } else if (almost_equals(c_token, "f$ont")) {
		++c_token;
		/* Make sure they've specified a font */
		if (!isstringvalue(c_token))
		    int_error(c_token,"expected font");
		else {
		    free(axis_array[axis].ticdef.font);
		    /* FIXME: protect against int_error() bail from try_to_get_string */
		    axis_array[axis].ticdef.font = NULL;
		    axis_array[axis].ticdef.font = try_to_get_string();
		}
	    } else if (equals(c_token,"tc") ||
		       almost_equals(c_token,"text$color")) {
		parse_colorspec(&axis_array[axis].ticdef.textcolor, TC_FRAC);
	    } else if (almost_equals(c_token, "au$tofreq")) {
		/* auto tic interval */
		++c_token;
		if (!axis_array[axis].ticdef.def.mix) {
		    free_marklist(axis_array[axis].ticdef.def.user);
		    axis_array[axis].ticdef.def.user = NULL;
		}
		axis_array[axis].ticdef.type = TIC_COMPUTED;
	    } else if (equals(c_token,"add")) {
		++c_token;
		axis_array[axis].ticdef.def.mix = TRUE;
	    } else if (!END_OF_COMMAND) {
		load_tics(axis);
	    }
	} while (!END_OF_COMMAND);

	/* if tics are off and not set by axis, reset to default (border) */
	if (((axis_array[axis].ticmode & TICS_MASK) == NO_TICS) && (!axisset)) {
	    axis_array[axis].ticmode = TICS_ON_BORDER;
	    if ((mirror_opt == FALSE) && ((axis == FIRST_X_AXIS) || (axis == FIRST_Y_AXIS) || (axis == COLOR_AXIS))) {
		axis_array[axis].ticmode |= TICS_MIRROR;
	    }
	}

    }

    if (almost_equals(c_token, nocmd)) {	/* NOSTRING */
	axis_array[axis].ticmode &= ~TICS_MASK;
	c_token++;
	match = 1;
    }
/* other options */

    (void) strcpy(sfxptr, "m$tics");	/* MONTH */
    if (almost_equals(c_token, cmdptr)) {
	if (!axis_array[axis].ticdef.def.mix) {
	    free_marklist(axis_array[axis].ticdef.def.user);
	    axis_array[axis].ticdef.def.user = NULL;
	}
	axis_array[axis].ticdef.type = TIC_MONTH;
	++c_token;
	match = 1;
    }
    if (almost_equals(c_token, nocmd)) {	/* NOMONTH */
	axis_array[axis].ticdef.type = TIC_COMPUTED;
	++c_token;
	match = 1;
    }
    (void) strcpy(sfxptr, "d$tics");	/* DAYS */
    if (almost_equals(c_token, cmdptr)) {
	match = 1;
	if (!axis_array[axis].ticdef.def.mix) {
	    free_marklist(axis_array[axis].ticdef.def.user);
	    axis_array[axis].ticdef.def.user = NULL;
	}
	axis_array[axis].ticdef.type = TIC_DAY;
	++c_token;
    }
    if (almost_equals(c_token, nocmd)) {	/* NODAYS */
	axis_array[axis].ticdef.type = TIC_COMPUTED;
	++c_token;
	match = 1;
    }
    *cmdptr = 'm';
    (void) strcpy(cmdptr + 1, axis_defaults[axis].name);
    (void) strcat(cmdptr, "t$ics");	/* MINISTRING */

    if (almost_equals(c_token, cmdptr)) {
	c_token++;
	match = 1;
	if (END_OF_COMMAND) {
	    axis_array[axis].minitics = MINI_AUTO;
	} else if (almost_equals(c_token, "def$ault")) {
	    axis_array[axis].minitics = MINI_DEFAULT;
	    ++c_token;
	} else {
	    axis_array[axis].mtic_freq = floor(real_expression());
	    axis_array[axis].minitics = MINI_USER;
	}
    }
    if (almost_equals(c_token, nocmd)) {	/* NOMINI */
	axis_array[axis].minitics = FALSE;
	c_token++;
	match = 1;
    }
    return (match);
}

/* process a 'set {x/y/z}label command */
/* set {x/y/z}label {label_text} {offset {x}{,y}} {<fontspec>} {<textcolor>} */
static void
set_xyzlabel(text_label *label)
{
    char *text = NULL;

    c_token++;
    if (END_OF_COMMAND) {	/* no label specified */
	free(label->text);
	label->text = NULL;
	return;
    }

    parse_label_options(label);

    if (!END_OF_COMMAND) {
	text = try_to_get_string();
	if (text) {
	    free(label->text);
	    label->text = text;
	}
#ifdef BACKWARDS_COMPATIBLE
	if (isanumber(c_token) || equals(c_token, "-")) {
	    /* Parse offset with missing keyword "set xlabel 'foo' 1,2 "*/
	    get_position_default(&(label->offset),character);
	}
#endif
    }

    parse_label_options(label);

}



/* 'set style line' command */
/* set style line {tag} {linetype n} {linewidth x} {pointtype n} {pointsize x} */
static void
set_linestyle()
{
    struct linestyle_def *this_linestyle = NULL;
    struct linestyle_def *new_linestyle = NULL;
    struct linestyle_def *prev_linestyle = NULL;
    struct lp_style_type loc_lp = DEFAULT_LP_STYLE_TYPE;
    int tag;

    c_token++;

    /* get tag */
    if (!END_OF_COMMAND) {
	/* must be a tag expression! */
	tag = int_expression();
	if (tag <= 0)
	    int_error(c_token, "tag must be > zero");
    } else
	tag = assign_linestyle_tag();	/* default next tag */

    /* Default style is based on linetype with the same tag id */
    loc_lp.l_type = tag - 1;
    loc_lp.p_type = tag - 1;

    /* Check if linestyle is already defined */
    if (first_linestyle != NULL) {	/* skip to last linestyle */
	for (this_linestyle = first_linestyle; this_linestyle != NULL;
	     prev_linestyle = this_linestyle,
	     this_linestyle = this_linestyle->next)
	    /* is this the linestyle we want? */
	    if (tag <= this_linestyle->tag)
		break;
    }

    if (this_linestyle == NULL || tag != this_linestyle->tag) {
	new_linestyle = gp_alloc(sizeof(struct linestyle_def), "linestyle");
	if (prev_linestyle != NULL)
	    prev_linestyle->next = new_linestyle;	/* add it to end of list */
	else
	    first_linestyle = new_linestyle;	/* make it start of list */
	new_linestyle->tag = tag;
	new_linestyle->next = this_linestyle;
	new_linestyle->lp_properties = loc_lp;
	this_linestyle = new_linestyle;
    }

    /* Reset to default values */
    if (END_OF_COMMAND)
	this_linestyle->lp_properties = loc_lp;
    else if (almost_equals(c_token, "def$ault")) {
	this_linestyle->lp_properties = loc_lp;
	c_token++;
    } else
	/* pick up a line spec; dont allow ls, do allow point type */
	lp_parse(&this_linestyle->lp_properties, FALSE, TRUE);

    if (!END_OF_COMMAND)
	int_error(c_token,"Extraneous arguments to set style line");

}

/* assign a new linestyle tag
 * linestyles are kept sorted by tag number, so this is easy
 * returns the lowest unassigned tag number
 */
static int
assign_linestyle_tag()
{
    struct linestyle_def *this;
    int last = 0;		/* previous tag value */

    for (this = first_linestyle; this != NULL; this = this->next)
	if (this->tag == last + 1)
	    last++;
	else
	    break;

    return (last + 1);
}

/* delete linestyle from linked list started by first_linestyle.
 * called with pointers to the previous linestyle (prev) and the
 * linestyle to delete (this).
 * If there is no previous linestyle (the linestyle to delete is
 * first_linestyle) then call with prev = NULL.
 */
void
delete_linestyle(struct linestyle_def *prev, struct linestyle_def *this)
{
    if (this != NULL) {		/* there really is something to delete */
	if (prev != NULL)	/* there is a previous linestyle */
	    prev->next = this->next;
	else			/* this = first_linestyle so change first_linestyle */
	    first_linestyle = this->next;
	free(this);
    }
}


/* ======================================================== */
/* process a 'set arrowstyle' command */
/* set style arrow {tag} {nohead|head|backhead|heads} {size l,a{,b}} {{no}filled} {linestyle...} {layer n}*/
static void
set_arrowstyle()
{
    struct arrowstyle_def *this_arrowstyle = NULL;
    struct arrowstyle_def *new_arrowstyle = NULL;
    struct arrowstyle_def *prev_arrowstyle = NULL;
    struct arrow_style_type loc_arrow;
    int tag;

    default_arrow_style(&loc_arrow);

    c_token++;

    /* get tag */
    if (!END_OF_COMMAND) {
	/* must be a tag expression! */
	tag = int_expression();
	if (tag <= 0)
	    int_error(c_token, "tag must be > zero");
    } else
	tag = assign_arrowstyle_tag();	/* default next tag */

    /* search for arrowstyle */
    if (first_arrowstyle != NULL) {	/* skip to last arrowstyle */
	for (this_arrowstyle = first_arrowstyle; this_arrowstyle != NULL;
	     prev_arrowstyle = this_arrowstyle,
	     this_arrowstyle = this_arrowstyle->next)
	    /* is this the arrowstyle we want? */
	    if (tag <= this_arrowstyle->tag)
		break;
    }

    if (this_arrowstyle == NULL || tag != this_arrowstyle->tag) {
	/* adding the arrowstyle */
	new_arrowstyle = (struct arrowstyle_def *)
	    gp_alloc(sizeof(struct arrowstyle_def), "arrowstyle");
	default_arrow_style(&(new_arrowstyle->arrow_properties));
	if (prev_arrowstyle != NULL)
	    prev_arrowstyle->next = new_arrowstyle;	/* add it to end of list */
	else
	    first_arrowstyle = new_arrowstyle;	/* make it start of list */
	new_arrowstyle->tag = tag;
	new_arrowstyle->next = this_arrowstyle;
	this_arrowstyle = new_arrowstyle;
    }

    if (END_OF_COMMAND)
	this_arrowstyle->arrow_properties = loc_arrow;
    else if (almost_equals(c_token, "def$ault")) {
	this_arrowstyle->arrow_properties = loc_arrow;
	c_token++;
    } else
	/* pick up a arrow spec : dont allow arrowstyle */
	arrow_parse(&this_arrowstyle->arrow_properties, FALSE);

    if (!END_OF_COMMAND)
	int_error(c_token, "extraneous or out-of-order arguments in set arrowstyle");

}

/* assign a new arrowstyle tag
 * arrowstyles are kept sorted by tag number, so this is easy
 * returns the lowest unassigned tag number
 */
static int
assign_arrowstyle_tag()
{
    struct arrowstyle_def *this;
    int last = 0;		/* previous tag value */

    for (this = first_arrowstyle; this != NULL; this = this->next)
	if (this->tag == last + 1)
	    last++;
	else
	    break;

    return (last + 1);
}

/* For set [xy]tics... command */
static void
load_tics(AXIS_INDEX axis)
{
    if (equals(c_token, "(")) {	/* set : TIC_USER */
	c_token++;
	load_tic_user(axis);
    } else {			/* series : TIC_SERIES */
	load_tic_series(axis);
    }
}

/* load TIC_USER definition */
/* (tic[,tic]...)
 * where tic is ["string"] value [level]
 * Left paren is already scanned off before entry.
 */
static void
load_tic_user(AXIS_INDEX axis)
{
    char *ticlabel;
    double ticposition;

    /* Free any old tic labels */
    if (!axis_array[axis].ticdef.def.mix) {
	free_marklist(axis_array[axis].ticdef.def.user);
	axis_array[axis].ticdef.def.user = NULL;
    }

    while (!END_OF_COMMAND) {
	int ticlevel=0;
	int save_token;
	/* syntax is  (  {'format'} value {level} {, ...} )
	 * but for timedata, the value itself is a string, which
	 * complicates things somewhat
	 */

	/* has a string with it? */
	save_token = c_token;
	ticlabel = try_to_get_string();
	if (ticlabel && axis_array[axis].is_timedata
	    && (equals(c_token,",") || equals(c_token,")"))) {
	    c_token = save_token;
	    ticlabel = NULL;
	}

	/* in any case get the value */
	GET_NUM_OR_TIME(ticposition, axis);

	if (!END_OF_COMMAND &&
	    !equals(c_token, ",") &&
	    !equals(c_token, ")")) {
	  ticlevel = int_expression(); /* tic level */
	}

	/* add to list */
	add_tic_user(axis, ticlabel, ticposition, ticlevel);

	/* expect "," or ")" here */
	if (!END_OF_COMMAND && equals(c_token, ","))
	    c_token++;		/* loop again */
	else
	    break;		/* hopefully ")" */
    }

    if (END_OF_COMMAND || !equals(c_token, ")")) {
	free_marklist(axis_array[axis].ticdef.def.user);
	axis_array[axis].ticdef.def.user = NULL;
	int_error(c_token, "expecting right parenthesis )");
    }
    c_token++;
}

void
free_marklist(struct ticmark *list)
{
    while (list != NULL) {
	struct ticmark *freeable = list;
	list = list->next;
	if (freeable->label != NULL)
	    free(freeable->label);
	free(freeable);
    }
}

/* load TIC_SERIES definition */
/* [start,]incr[,end] */
static void
load_tic_series(AXIS_INDEX axis)
{
    double start, incr, end;
    int incr_token;

    struct ticdef *tdef = &axis_array[axis].ticdef;

    GET_NUM_OR_TIME(start, axis);

    if (!equals(c_token, ",")) {
	/* only step specified */
	incr = start;
	start = -VERYLARGE;
	end = VERYLARGE;
    } else {
	c_token++;
	incr_token = c_token;
	GET_NUM_OR_TIME(incr, axis);

	if (!equals(c_token, ",")) {
	    /* only step and increment specified */
	    end = VERYLARGE;
	} else {
	    c_token++;
	    GET_NUM_OR_TIME(end, axis);
	}

	if (start < end && incr <= 0)
	    int_error(incr_token, "increment must be positive");
	if (start > end && incr >= 0)
	    int_error(incr_token, "increment must be negative");
	if (start > end) {
	    /* put in order */
	    double numtics = floor((end * (1 + SIGNIF) - start) / incr);

	    end = start;
	    start = end + numtics * incr;
	    incr = -incr;
	}
    }

    if (!tdef->def.mix) { /* remove old list */
	free_marklist(tdef->def.user);
	tdef->def.user = NULL;
    }
    tdef->type = TIC_SERIES;
    tdef->def.series.start = start;
    tdef->def.series.incr = incr;
    tdef->def.series.end = end;
}

static void
load_offsets(double *a, double *b, double *c, double *d)
{
    *a = real_expression();	/* loff value */
    if (!equals(c_token, ","))
	return;

    c_token++;
    *b = real_expression();	/* roff value */
    if (!equals(c_token, ","))
	return;

    c_token++;
    *c = real_expression();	/* toff value */
    if (!equals(c_token, ","))
	return;

    c_token++;
    *d = real_expression();	/* boff value */
}

/* return 1 if format looks like a numeric format
 * ie more than one %{efg}, or %something-else
 */
/* FIXME HBB 20000430: as coded, this will only check the *first*
 * format string, not all of them. */
static int
looks_like_numeric(char *format)
{
    if (!(format = strchr(format, '%')))
	return 0;

    while (++format && (*format == ' '
			|| *format == '-'
			|| *format == '+'
			|| *format == '#'))
	;			/* do nothing */

    while (isdigit((unsigned char) *format)
	   || *format == '.')
	++format;

    return (*format == 'f' || *format == 'g' || *format == 'e');
}

#ifdef BACKWARDS_COMPATIBLE
/*
 * Backwards compatibility ...
 */
static void set_nolinestyle()
{
    struct linestyle_def *this, *prev;
    int tag;

    if (END_OF_COMMAND) {
	/* delete all linestyles */
	while (first_linestyle != NULL)
	    delete_linestyle((struct linestyle_def *) NULL, first_linestyle);
    } else {
	/* get tag */
	tag = int_expression();
	if (!END_OF_COMMAND)
	    int_error(c_token, "extraneous arguments to set nolinestyle");
	for (this = first_linestyle, prev = NULL;
	     this != NULL;
	     prev = this, this = this->next) {
	    if (this->tag == tag) {
		delete_linestyle(prev, this);
		return;         /* exit, our job is done */
	    }
	}
	int_error(c_token, "linestyle not found");
    }
}
#endif


/* HBB 20001021: new function: make label texts decoratable with numbers */
static char *
fill_numbers_into_string(char *pattern)
{
    size_t pattern_length = strlen(pattern) + 1;
    size_t newlen = pattern_length;
    char *output = gp_alloc(newlen, "fill_numbers output buffer");
    size_t output_end = 0;

    do {			/* loop over string/value pairs */
	double value;

	if (isstring(++c_token)) {
	    free(output);
	    free(pattern);
	    int_error(c_token, "constant expression expected");
	}

	/* assume it's a numeric expression, concatenate it to output
	 * string: parse value, enlarge output buffer, and gprintf()
	 * it. */
	value = real_expression();
	newlen += pattern_length + 30;
	output = gp_realloc(output, newlen, "fill_numbers next number");
	gprintf(output + output_end, newlen - output_end,
		pattern, 1.0, value);
	output_end += strlen(output + output_end);

	/* allow a string to follow, after another comma: */
	if (END_OF_COMMAND || !equals(c_token, ",")) {
	    /* no comma followed the number --> we're done. Jump out
	     * directly, as falling out of the while loop means
	     * something slightly different. */
	    free(pattern);
	    return output;
	}
	c_token++;

	if (!END_OF_COMMAND && isstring(c_token)) {
	    size_t length = token_len(c_token);

	    if (length >= pattern_length)
		pattern = gp_realloc(pattern, pattern_length = length,
				     "fill_numbers resize pattern");
	    quote_str(pattern, c_token, length);
	    c_token++;
	} else {
	    free(pattern);
	    free(output);
	    int_error(c_token, "string expected");
	} /* if (string after comma) */
    } while (!END_OF_COMMAND && equals(c_token, ","));

    /* came out here --> the last element was a string, not a number.
     * that means that there is a string in pattern which was not yet
     * copied to 'output' */
    output = gp_realloc(output, newlen += pattern_length,
			"fill_numbers closing");
    strcpy(output + output_end, pattern);
    free(pattern);
    return output;
}

/*
 * new_text_label() allocates and initializes a text_label structure.
 * This routine is also used by the plot and splot with labels commands.
 */
struct text_label *
new_text_label(int tag)
{
    struct text_label *new;
    struct position default_offset = { character, character, character,
				       0., 0., 0. };

    new = gp_alloc( sizeof(struct text_label), "text_label");
    new->next = NULL;
    new->tag = tag;
    new->place = default_position;
    new->pos = LEFT;
    new->rotate = 0;
    new->layer = 0;
    new->text = (char *)NULL;
    new->font = (char *)NULL;
    new->textcolor.type = TC_DEFAULT;
    new->lp_properties.pointflag = 0;
    new->lp_properties.p_type = 1;
    new->offset = default_offset;
    new->noenhanced = FALSE;

    return(new);
}

/*
 * Parse the sub-options for label style and placement.
 * This is called from set_label, and from plot2d and plot3d 
 * to handle options for 'plot with labels'
 */
void
parse_label_options( struct text_label *this_label )
{
    struct position pos;
    char *font = NULL;
    enum JUSTIFY just = LEFT;
    int rotate = 0;
    TBOOLEAN set_position = FALSE, set_just = FALSE,
	set_rot = FALSE, set_font = FALSE, set_offset = FALSE,
	set_layer = FALSE, set_textcolor = FALSE;
    int layer = 0;
    TBOOLEAN axis_label = (this_label->tag == -2);
    struct position offset = { character, character, character, 0., 0., 0. };
    t_colorspec textcolor = {TC_DEFAULT,0,0.0};
    struct lp_style_type loc_lp = DEFAULT_LP_STYLE_TYPE;
    loc_lp.pointflag = -2;

   /* Now parse the label format and style options */
    while (!END_OF_COMMAND) {
	/* get position */
	if (! set_position && equals(c_token, "at") && !axis_label) {
	    c_token++;
	    get_position(&pos);
	    set_position = TRUE;
	    continue;
	}

	/* get justification */
	if (! set_just) {
	    if (almost_equals(c_token, "l$eft")) {
		just = LEFT;
		c_token++;
		set_just = TRUE;
		continue;
	    } else if (almost_equals(c_token, "c$entre")
		       || almost_equals(c_token, "c$enter")) {
		just = CENTRE;
		c_token++;
		set_just = TRUE;
		continue;
	    } else if (almost_equals(c_token, "r$ight")) {
		just = RIGHT;
		c_token++;
		set_just = TRUE;
		continue;
	    }
	}

	/* get rotation (added by RCC) */
	if (! set_rot) {
	    if (almost_equals(c_token, "rot$ate")) {
		rotate = TEXT_VERTICAL;
		c_token++;
		set_rot = TRUE;
		if (equals(c_token, "by")) {
		    c_token++;
		    rotate = int_expression();
		}
		continue;
	    } else if (almost_equals(c_token, "norot$ate")) {
		rotate = 0;
		c_token++;
		set_rot = TRUE;
		continue;
	    }
	}

	/* get font (added by DJL) */
	if (! set_font && equals(c_token, "font")) {
	    c_token++;
	    if ((font = try_to_get_string())) {
		set_font = TRUE;
		continue;
	    } else
		int_error(c_token, "'fontname,fontsize' expected");
	}

	/* get front/back (added by JDP) */
	if (! set_layer && !axis_label) {
	    if (equals(c_token, "back")) {
		layer = 0;
		c_token++;
		set_layer = TRUE;
		continue;
	    } else if (equals(c_token, "front")) {
		layer = 1;
		c_token++;
		set_layer = TRUE;
		continue;
	    }
	}

	if (loc_lp.pointflag == -2 && !axis_label) {
	    if (almost_equals(c_token, "po$int")) {
		int stored_token = ++c_token;
		struct lp_style_type tmp_lp;
		loc_lp.pointflag = 1;
		tmp_lp = loc_lp;
		lp_parse(&tmp_lp, TRUE, TRUE);
		if (stored_token != c_token)
		    loc_lp = tmp_lp;
		continue;
	    } else if (almost_equals(c_token, "nopo$int")) {
		loc_lp.pointflag = 0;
		c_token++;
		continue;
	    }
	}

	if (! set_offset && almost_equals(c_token, "of$fset")) {
	    c_token++;
	    get_position_default(&offset,character);
	    set_offset = TRUE;
	    continue;
	}

	if ((equals(c_token,"tc") || almost_equals(c_token,"text$color"))
	    && ! set_textcolor ) {
	    parse_colorspec( &textcolor, TC_Z );
	    set_textcolor = TRUE;
	    continue;
	}

	/* EAM FIXME: Option to disable enhanced text processing currently not */
	/* documented for ordinary labels. */
	if (almost_equals(c_token,"noenh$anced")) {
	    this_label->noenhanced = TRUE;
	    c_token++;
	    continue;
	} else if (almost_equals(c_token,"enh$anced")) {
	    this_label->noenhanced = FALSE;
	    c_token++;
	    continue;
	}

	/* Coming here means that none of the previous 'if's struck
	 * its "continue" statement, i.e.  whatever is in the command
	 * line is forbidden by the 'set label' command syntax.
	 * On the other hand, 'plot with labels' may have additional stuff coming up.
	 */
	break;

    } /* while(!END_OF_COMMAND) */

    /* HBB 20011120: this chunk moved here, behind the while()
     * loop. Only after all options have been parsed it's safe to
     * overwrite the position if none has been specified. */
    if (!set_position)
	pos = default_position;

    /* OK! copy the requested options into the label */
	if (set_position)
	    this_label->place = pos;
	if (set_just)
	    this_label->pos = just;
	if (set_rot)
	    this_label->rotate = rotate;
	if (set_layer)
	    this_label->layer = layer;
	if (set_font)
	    this_label->font = font;
	if (set_textcolor)
	    memcpy(&(this_label->textcolor), &textcolor, sizeof(t_colorspec));
	if (loc_lp.pointflag >= 0)
	    memcpy(&(this_label->lp_properties), &loc_lp, sizeof(loc_lp));
	if (set_offset)
	    this_label->offset = offset;

    /* Make sure the z coord and the z-coloring agree */
    if (this_label->textcolor.type == TC_Z)
	this_label->textcolor.value = this_label->place.z;
}


#ifdef EAM_HISTOGRAMS
/* <histogramstyle> = {clustered {gap <n>} | rowstacked | columnstacked */
/*                     errorbars {gap <n>} {linewidth <lw>}}            */
/*                    {title <title_options>}                           */
static void
parse_histogramstyle( histogram_style *hs, 
		t_histogram_type def_type,
		int def_gap)
{
    text_label title_specs = EMPTY_LABELSTRUCT;

    /* Set defaults */
    hs->type  = def_type;
    hs->gap   = def_gap;

    if (END_OF_COMMAND)
	return;
    if (!equals(c_token,"hs") && !almost_equals(c_token,"hist$ogram"))
	return;
    c_token++;

    while (!END_OF_COMMAND) {
	if (almost_equals(c_token, "clust$ered")) {
	    hs->type = HT_CLUSTERED;
	    c_token++;
	} else if (almost_equals(c_token, "error$bars")) {
	    hs->type = HT_ERRORBARS;
	    c_token++;
	} else if (almost_equals(c_token, "rows$tacked")) {
	    hs->type = HT_STACKED_IN_LAYERS;
	    c_token++;
	} else if (almost_equals(c_token, "columns$tacked")) {
	    hs->type = HT_STACKED_IN_TOWERS;
	    c_token++;
	} else if (equals(c_token, "gap")) {
	    if (isanumber(++c_token))
		hs->gap = int_expression();
	    else
		int_error(c_token,"expected gap value");
	} else if (almost_equals(c_token, "ti$tle")) {
	    title_specs.offset = hs->title.offset;
	    set_xyzlabel(&title_specs);
	    memcpy(&hs->title.textcolor,&title_specs.textcolor,sizeof(t_colorspec));
	    hs->title.offset = title_specs.offset;
	    /* EAM FIXME - could allocate space and copy parsed font instead */
	    hs->title.font = axis_array[FIRST_X_AXIS].label.font;
	} else if ((equals(c_token,"lw") || almost_equals(c_token,"linew$idth"))
		  && (hs->type == HT_ERRORBARS)) {
	    c_token++;
	    hs->bar_lw = real_expression();
	    if (hs->bar_lw < 0)
		hs->bar_lw = 0;
	} else
	    /* We hit something unexpected */
	    break;
    }
}
#endif /* EAM_HISTOGRAMS */
