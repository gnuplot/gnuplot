#ifndef lint
static char *RCSid() { return RCSid("$Id: set.c,v 1.71 2002/01/22 15:52:24 mikulik Exp $"); }
#endif

/* GNUPLOT - set.c */

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
#ifdef USE_MOUSE
#   include "mouse.h"
#endif

#ifdef PM3D
#include "pm3d.h"
#endif

#include <ctype.h>		/* for isdigit() */

static void set_angles __PROTO((void));
static void set_arrow __PROTO((void));
static int assign_arrow_tag __PROTO((void));
static void set_autoscale __PROTO((void));
static void set_bars __PROTO((void));
static void set_border __PROTO((void));
static void set_boxwidth __PROTO((void));
#if USE_ULIG_FILLEDBOXES
static void set_fillstyle __PROTO((void));
#endif
static void set_clabel __PROTO((void));
static void set_clip __PROTO((void));
static void set_cntrparam __PROTO((void));
static void set_contour __PROTO((void));
static void set_dgrid3d __PROTO((void));
static void set_dummy __PROTO((void));
static void set_encoding __PROTO((void));
static void set_format __PROTO((void));
static void set_grid __PROTO((void));
static void set_hidden3d __PROTO((void));
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
static void set_historysize __PROTO((void));
#endif
static void set_isosamples __PROTO((void));
static void set_key __PROTO((void));
static void set_keytitle __PROTO((void));
static void set_label __PROTO((void));
static int assign_label_tag __PROTO((void));
static void set_loadpath __PROTO((void));
static void set_locale __PROTO((void));
static void set_logscale __PROTO((void));
static void set_mapping __PROTO((void));
static void set_bmargin __PROTO((void));
static void set_lmargin __PROTO((void));
static void set_rmargin __PROTO((void));
static void set_tmargin __PROTO((void));
static void set_missing __PROTO((void));
#ifdef USE_MOUSE
static void set_mouse __PROTO((void));
#endif
static void set_offsets __PROTO((void));
static void set_origin __PROTO((void));
static void set_output __PROTO((void));
static void set_parametric __PROTO((void));
#ifdef PM3D
static void set_pm3d __PROTO((void));
static void set_palette __PROTO((void));
static void set_colorbox __PROTO((void));
#endif
static void set_pointsize __PROTO((void));
static void set_polar __PROTO((void));
static void set_samples __PROTO((void));
static void set_size __PROTO((void));
static void set_style __PROTO((void));
static void set_surface __PROTO((void));
static void set_terminal __PROTO((void));
static void set_termoptions __PROTO((void));
static void set_tics __PROTO((void));
static void set_ticscale __PROTO((void));
static void set_ticslevel __PROTO((void));
static void set_timefmt __PROTO((void));
static void set_timestamp __PROTO((void));
static void set_view __PROTO((void));
static void set_zero __PROTO((void));
static void set_timedata __PROTO((AXIS_INDEX));
static void set_range __PROTO((AXIS_INDEX));
static void set_zeroaxis __PROTO((AXIS_INDEX));
static void set_allzeroaxis __PROTO((void));


/******** Local functions ********/

static void get_position __PROTO((struct position * pos));
static void get_position_type __PROTO((enum position_type * type, int *axes));
static void set_xyzlabel __PROTO((label_struct * label));
static void load_tics __PROTO((AXIS_INDEX axis));
static void load_tic_user __PROTO((AXIS_INDEX axis));
static void free_marklist __PROTO((struct ticmark * list));
static void load_tic_series __PROTO((AXIS_INDEX axis));
static void load_offsets __PROTO((double *a, double *b, double *c, double *d));

static void set_linestyle __PROTO((void));
static int assign_linestyle_tag __PROTO((void));
static int looks_like_numeric __PROTO((char *));
static int set_tic_prop __PROTO((AXIS_INDEX));
static char *fill_numbers_into_string __PROTO((char *pattern));

/* Backwards compatibility ... */
static void set_nolinestyle __PROTO((void));

/******** The 'set' command ********/
void
set_command()
{
    static char GPFAR setmess[] = 
    "valid set options:  [] = choose one, {} means optional\n\n\
\t'angles', 'arrow', 'autoscale', 'bars', 'border', 'boxwidth',\n\
\t'clabel', 'clip', 'cntrparam', 'colorbox', 'contour',\n\
\t'dgrid3d', 'dummy', 'encoding', 'format', 'grid',\n\
\t'hidden3d', 'historysize', 'isosamples', 'key', 'label',  'locale',\n\
\t'logscale', '[blrt]margin', 'mapping', 'missing', 'mouse',\n\
\t'multiplot', 'offsets', 'origin', 'output', 'palette', 'parametric',\n\
\t'pm3d', 'pointsize', 'polar', '[rtuv]range', 'samples', 'size',\n\
\t'style', 'surface', 'terminal', tics', 'ticscale', 'ticslevel',\n\
\t'timestamp', 'timefmt', 'title', 'view', '[xyz]{2}data',\n\
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
    } else if (input_line[token[c_token].start_index] == 'n' &&
	       input_line[token[c_token].start_index+1] == 'o') {
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
	case S_DUMMY:
	    set_dummy();
	    break;
	case S_ENCODING:
	    set_encoding();
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
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
	case S_HISTORYSIZE:
	    set_historysize();
	    break;
#endif
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
	case S_MAPPING:
	    set_mapping();
	    break;
	case S_BMARGIN:
	    set_bmargin();
	    break;
	case S_LMARGIN:
	    set_lmargin();
	    break;
	case S_RMARGIN:
	    set_rmargin();
	    break;
	case S_TMARGIN:
	    set_tmargin();
	    break;
	case S_MISSING:
	    set_missing();
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
	case S_OUTPUT:
	    set_output();
	    break;
	case S_PARAMETRIC:
	    set_parametric();
	    break;
#ifdef PM3D
	case S_PM3D:
	    set_pm3d();
	    break;
    case S_PALETTE:
	set_palette();
	break;
	case S_COLORBOX:
	    set_colorbox();
	break;
#endif
	case S_POINTSIZE:
	    set_pointsize();
	    break;
	case S_POLAR:
	    set_polar();
	    break;
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
	case S_TICSLEVEL:
	    set_ticslevel();
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
#ifdef PM3D
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
#endif
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
#ifdef PM3D
	case S_CBDATA:
	    set_timedata(COLOR_AXIS);
	    break;
#endif
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
#ifdef PM3D
	case S_CBLABEL:
	    set_xyzlabel(&axis_array[COLOR_AXIS].label);
	    break;
#endif
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
#ifdef PM3D
	case S_CBRANGE:
	    set_range(COLOR_AXIS);
	    break;
#endif
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
	case S_X2ZEROAXIS:
	    set_zeroaxis(SECOND_X_AXIS);
	    break;
	case S_Y2ZEROAXIS:
	    set_zeroaxis(SECOND_Y_AXIS);
	    break;
	case S_ZEROAXIS:
	    set_allzeroaxis();
	    break;
	default:
	    int_error(c_token, setmess);
	    break;
	}

#ifdef BACKWARDS_COMPATIBLE
    }
#endif

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
    struct value a;
    struct arrow_def *this_arrow = NULL;
    struct arrow_def *new_arrow = NULL;
    struct arrow_def *prev_arrow = NULL;
    struct position spos, epos, headsize;
    struct lp_style_type loc_lp = DEFAULT_LP_STYLE_TYPE;
    TBOOLEAN relative = FALSE;
    int tag;
    int head = 1;
    TBOOLEAN set_tag = FALSE, set_start = FALSE, set_end = FALSE;
    TBOOLEAN set_line = FALSE, set_headsize = FALSE, set_layer = FALSE;
    TBOOLEAN set_head = FALSE;
    TBOOLEAN duplication = FALSE;
    int layer = 0;
    /* remember current token number to see because it can be a tag: */
    int maybe_tag_coken = ++c_token;

    /* default values */
    spos.x = spos.y = spos.z = 0;
    spos.scalex = spos.scaley = spos.scalez = first_axes;
    epos.x = epos.y = epos.z = 0;
    epos.scalex = epos.scaley = epos.scalez = first_axes;
    headsize.x = 0; /* length being zero means the default head size */

    while (!END_OF_COMMAND) {

	/* get start position */
	if (equals(c_token, "from")) {
	    if (set_start) { duplication = TRUE; break; }
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "start coordinates expected");
	    /* get coordinates */
	    get_position(&spos);
	    set_start = TRUE;
	    continue;
	}

	/* get end or relative end position */
	if (equals(c_token, "to") || equals(c_token,"rto")) {
	    if (set_end) { duplication = TRUE; break; }
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "end coordinates expected");
	    /* get coordinates */
	    get_position(&epos);
	    relative = (equals(c_token,"rto")) ? TRUE : FALSE;
	    set_end = TRUE;
	    continue;
	}

	if (equals(c_token, "nohead")) {
	    if (set_head) { duplication = TRUE; break; }
	    c_token++;
	    head = 0;
	    set_head = TRUE;
	    continue;
	}
	if (equals(c_token, "head")) {
	    if (set_head) { duplication = TRUE; break; }
	    c_token++;
	    head = 1;
	    set_head = TRUE;
	    continue;
	}
	if (equals(c_token, "heads")) {
	    if (set_head) { duplication = TRUE; break; }
	    c_token++;
	    head = 2;
	    set_head = TRUE;
	    continue;
	}

	if (equals(c_token, "size")) {
	    if (set_headsize) { duplication = TRUE; break; }
	    headsize.scalex = headsize.scaley = headsize.scalez = first_axes;
	      /* only scalex used; scaley is angle of the head in [deg] */
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "head size expected");
	    get_position(&headsize);
	    set_headsize = TRUE;
	    continue;
	}

	if (equals(c_token, "back")) {
	    if (set_layer) { duplication = TRUE; break; }
	    c_token++;
	    layer = 0;
	    set_layer = TRUE;
	    continue;
	}
	if (equals(c_token, "front")) {
	    if (set_layer) { duplication = TRUE; break; }
	    c_token++;
	    layer = 1;
	    set_layer = TRUE;
	    continue;
	}

	/* pick up a line spec - allow ls, but no point. */
	/* HBB 20010807: Only set set_line to TRUE if there really was a
	 * lp spec at the end of this command. */
	{
	    int stored_token = c_token;

	    lp_parse(&loc_lp, 1, 0, 0, 0);
	    /* HBB 20010807: this is already taken care of by lp_parse()... */
	    /* loc_lp.pointflag = 0; */ /* standard value for arrows, don't use points */
	    if (stored_token != c_token) {
		set_line = TRUE;
		continue;
	    }
	}

	/* no option given, thus it could be the tag */
	if (maybe_tag_coken == c_token) {
	    /* must be a tag expression! */
	    tag = (int) real(const_express(&a));
	    if (tag <= 0)
		int_error(--c_token, "tag must be > zero");
	    set_tag = TRUE;
	    continue;
	}
    
	if (!END_OF_COMMAND)
	    int_error(c_token, "wrong argument in set arrow");

    } /* while (!END_OF_COMMAND) */

    if (duplication)
	int_error(c_token, "duplicated or contradicting arguments in set arrow");
    
    /* no tag given, the default is the next tag */
    if (!set_tag)
	tag = assign_arrow_tag();
    
    /* OK! add arrow */
    if (first_arrow != NULL) {	/* skip to last arrow */
	for (this_arrow = first_arrow; this_arrow != NULL;
	     prev_arrow = this_arrow, this_arrow = this_arrow->next)
	    /* is this the arrow we want? */
	    if (tag <= this_arrow->tag)
		break;
    }
    if (this_arrow != NULL && tag == this_arrow->tag) {
	/* changing the arrow */
	if (set_start) {
	    this_arrow->start = spos;
	}
	if (set_end) {
	    this_arrow->end = epos;
	    this_arrow->relative = relative;
	}
	if (set_head) 
	    this_arrow->head = head;
	if (set_layer) {
	    this_arrow->layer = layer;
	}
	if (set_headsize) {
	    this_arrow->headsize = headsize;
	}
	if (set_line) {
	    this_arrow->lp_properties = loc_lp;
	}
    } else {	/* adding the arrow */
	new_arrow = (struct arrow_def *)
	    gp_alloc(sizeof(struct arrow_def), "arrow");
	if (prev_arrow != NULL)
	    prev_arrow->next = new_arrow;	/* add it to end of list */
	else
	    first_arrow = new_arrow;	/* make it start of list */
	new_arrow->tag = tag;
	new_arrow->next = this_arrow;
	new_arrow->start = spos;
	new_arrow->end = epos;
	new_arrow->head = head;
	new_arrow->headsize = headsize;
	new_arrow->layer = layer;
	new_arrow->relative = relative;
	new_arrow->lp_properties = loc_lp;
    }
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
#ifdef PM3D
    PROCESS_AUTO_LETTER(COLOR_AXIS);
#endif
    /* came here only if nothing found: */
	int_error(c_token, "Invalid range");
}


/* process 'set bars' command */
static void
set_bars()
{
    struct value a;

    c_token++;
    if(END_OF_COMMAND) {
	bar_size = 1.0;
    } else if(almost_equals(c_token,"s$mall")) {
	bar_size = 0.0;
	++c_token;
    } else if(almost_equals(c_token,"l$arge")) {
	bar_size = 1.0;
	++c_token;
    } else {
	bar_size = real(const_express(&a));
    }
}


/* process 'set border' command */
static void
set_border()
{
    struct value a;

    c_token++;
    if(END_OF_COMMAND){
	draw_border = 31;
    } else {
	draw_border = (int)real(const_express(&a));
    }
    /* HBB 980609: add linestyle handling for 'set border...' */
    /* For now, you have to give a border bitpattern to be able to specify
     * a linestyle. Sorry for this, but the gnuplot parser really is too messy
     * for any other solution, currently */
    /* not for long, I hope. Lars */
    if(END_OF_COMMAND) {
	border_lp = default_border_lp;
    } else {
	lp_parse(&border_lp, 1, 0, -2, 0);
    }
}


/* process 'set boxwidth' command */
static void
set_boxwidth()
{
    struct value a;

    c_token++;
    if (END_OF_COMMAND) {
	boxwidth = -1.0;
#if USE_ULIG_RELATIVE_BOXWIDTH
        boxwidth_is_absolute = TRUE;
#endif /* USE_ULIG_RELATIVE_BOXWIDTH */
    } else {
	boxwidth = real(const_express(&a));
    }
#if USE_ULIG_RELATIVE_BOXWIDTH
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
#endif  /* USE_ULIG_RELATIVE_BOXWIDTH */
}

#if USE_ULIG_FILLEDBOXES
/* process 'set style filling' command (ULIG) */
static void
set_fillstyle()
{
    struct value a;

    c_token++;
    if (END_OF_COMMAND) {
	fillstyle = 1;
    } else if (almost_equals(c_token, "e$mpty")) {
	fillstyle = 0;
	c_token++;
    } else if (almost_equals(c_token, "s$olid")) {
	fillstyle = 1;
	c_token++;
	if (END_OF_COMMAND)
	    filldensity = 100;
	else {
	    /* user sets 0...1, but is stored as an integer 0..100 */
	    filldensity = 100.0 * real(const_express(&a)) + 0.5;
	    if( filldensity < 0 )
		filldensity = 0; 
	    if( filldensity > 100 )
		filldensity = 100;
	}
    } else if (almost_equals(c_token, "p$attern")) {
	fillstyle = 2;
	c_token++;
	if (END_OF_COMMAND)
	    fillpattern = 0;
	else {
	    fillpattern = real(const_express(&a));
	    if( fillpattern < 0 )
		fillpattern = 0;
	}
    } else
	int_error(c_token, "expecting 'empty', 'solid' or 'pattern'");
    
}
#endif /* USE_ULIG_FILLEDBOXES */

/* process 'set clabel' command */
static void
set_clabel()
{
    c_token++;
    label_contours = TRUE;
    if (isstring(c_token))
	quote_str(contour_format, c_token++, 30);
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
    struct value a;

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
	contour_pts = (int) real(const_express(&a));
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
		    real(const_express(&a));

	    while(!END_OF_COMMAND) {
		if (!equals(c_token, ","))
		    int_error(c_token,
			      "expecting comma to separate discrete levels");
		c_token++;
		*(double *)nextfrom_dynarray(&dyn_contour_levels_list) =
		    real(const_express(&a));
	    }
	    contour_levels = dyn_contour_levels_list.end;
	} else if (almost_equals(c_token, "in$cremental")) {
	    int i = 0;  /* local counter */

	    contour_levels_kind = LEVELS_INCREMENTAL;
	    c_token++;
	    contour_levels_list[i++] = real(const_express(&a));
	    if (!equals(c_token, ","))
		int_error(c_token,
			  "expecting comma to separate start,incr levels");
	    c_token++;
	    if((contour_levels_list[i++] = real(const_express(&a))) == 0)
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
		contour_levels = (int) ( (real(const_express(&a))-contour_levels_list[0])/contour_levels_list[1] + 1.0);
	    }
	} else if (almost_equals(c_token, "au$to")) {
	    contour_levels_kind = LEVELS_AUTO;
	    c_token++;
	    if(!END_OF_COMMAND)
		contour_levels = (int) real(const_express(&a));
	} else {
	    if(contour_levels_kind == LEVELS_DISCRETE)
		int_error(c_token, "Levels type is discrete, ignoring new number of contour levels");
	    contour_levels = (int) real(const_express(&a));
	}
    } else if (almost_equals(c_token, "o$rder")) {
	int order;
	c_token++;
	order = (int) real(const_express(&a));
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
    struct value a;
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
	    local_vals[i] = real(const_express(&a));
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
    int temp;

    c_token++;

    if(END_OF_COMMAND)
	temp = S_ENC_DEFAULT;
    else {
	temp = lookup_table(&set_encoding_tbl[0],c_token);

	if (temp == S_ENC_INVALID)
	    int_error(c_token, "expecting one of 'default', 'iso_8859_1', 'iso_8859_2', 'cp437', 'cp850' or 'cp852'");
	c_token++;
    }
    encoding = temp;
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
#ifdef PM3D
	SET_DEFFORMAT(COLOR_AXIS   , set_for_axis);
#endif
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
#ifdef PM3D
	    SET_FORMATSTRING(COLOR_AXIS);
#endif
#undef SET_FORMATSTRING

	    c_token++;
	}
    }
}


/* process 'set grid' command */

static void
set_grid()
{
    c_token++;
    if (END_OF_COMMAND && !some_grid_selected()) {
/*  	grid_selection = GRID_X|GRID_Y;  */
	axis_array[FIRST_X_AXIS].gridmajor = TRUE;
	axis_array[FIRST_Y_AXIS].gridmajor = TRUE;
    } else
	while (!END_OF_COMMAND) {
#if 0
            /* HBB 20010806: Old method of accessing grid choices */
#define GRID_MATCH(string, neg, mask)			\
	    if (almost_equals(c_token, string)) {	\
		grid_selection |= mask;			\
		++c_token;				\
	    } else if (almost_equals(c_token, neg)) {	\
		grid_selection &= ~(mask);		\
		++c_token;				\
	    }
	    GRID_MATCH("x$tics", "nox$tics", GRID_X)
	    else GRID_MATCH("y$tics", "noy$tics", GRID_Y)
	    else GRID_MATCH("z$tics", "noz$tics", GRID_Z)
	    else GRID_MATCH("x2$tics", "nox2$tics", GRID_X2)
	    else GRID_MATCH("y2$tics", "noy2$tics", GRID_Y2)
	    else GRID_MATCH("mx$tics", "nomx$tics", GRID_MX)
	    else GRID_MATCH("my$tics", "nomy$tics", GRID_MY)
	    else GRID_MATCH("mz$tics", "nomz$tics", GRID_MZ)
	    else GRID_MATCH("mx2$tics", "nomx2$tics", GRID_MX2)
	    else GRID_MATCH("my2$tics", "nomy2$tics", GRID_MY2)
#ifdef PM3D
	    else GRID_MATCH("cb$tics", "nocb$tics", GRID_CB)
	    else GRID_MATCH("mcb$tics", "nomcb$tics", GRID_MCB)
#endif
#else  
            /* HBB 20010806: new grid steering variable in axis struct */
#define GRID_MATCH(axis, string)				\
	    if (almost_equals(c_token, string+2)) {		\
		if (string[2] == 'm')				\
		    axis_array[axis].gridminor = TRUE;		\
		else						\
		    axis_array[axis].gridmajor = TRUE;		\
		++c_token;					\
	    } else if (almost_equals(c_token, string)) {	\
		if (string[2] == 'm')				\
		    axis_array[axis].gridminor = FALSE;		\
		else						\
		    axis_array[axis].gridmajor = FALSE;		\
		++c_token;					\
	    }
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
#ifdef PM3D
	    else GRID_MATCH(COLOR_AXIS, "nocb$tics")
	    else GRID_MATCH(COLOR_AXIS, "nomcb$tics")
#endif
#endif /* 1/0 */
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
		    struct value a;
		    polar_grid_angle = ang2rad*real(const_express(&a));
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
	struct value a;
	int old_token = c_token;

	lp_parse(&grid_lp,1,0,-1,1);
	if (c_token == old_token) { /* nothing parseable found... */
	    grid_lp.l_type = real(const_express(&a)) - 1;
	}

	if (! some_grid_selected()) {
	    /* grid_selection = GRID_X|GRID_Y; */
	    axis_array[FIRST_X_AXIS].gridmajor = TRUE;
	    axis_array[FIRST_Y_AXIS].gridmajor = TRUE;
	}
	/* probably just  set grid <linetype> */

	if (END_OF_COMMAND) {
	    memcpy(&mgrid_lp,&grid_lp,sizeof(struct lp_style_type));
	} else {
	    if (equals(c_token,",")) 
		c_token++;
	    old_token = c_token;
	    lp_parse(&mgrid_lp,1,0,-1,1);
	    if (c_token == old_token) {
		mgrid_lp.l_type = real(const_express(&a)) -1;
	    }
	}

	if (! some_grid_selected()) {
	    /* grid_selection = GRID_X|GRID_Y; */
	    axis_array[FIRST_X_AXIS].gridmajor = TRUE;
	    axis_array[FIRST_Y_AXIS].gridmajor = TRUE;
	}
	/* probably just  set grid <linetype> */
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


#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
/* process 'set historysize' command */
static void
set_historysize()
{
    struct value a;

    c_token++;

    gnuplot_history_size = (int) real(const_express(&a));
    if (gnuplot_history_size < 0) {
	gnuplot_history_size = 0;
    }
}
#endif


/* process 'set isosamples' command */
static void
set_isosamples()
{
    register int tsamp1, tsamp2;
    struct value a;

    c_token++;
    tsamp1 = (int)magnitude(const_express(&a));
    tsamp2 = tsamp1;
    if (!END_OF_COMMAND) {
	if (!equals(c_token,","))
	    int_error(c_token, "',' expected");
	c_token++;
	tsamp2 = (int)magnitude(const_express(&a));
    }
    if (tsamp1 < 2 || tsamp2 < 2)
	int_error(c_token, "sampling rate must be > 1; sampling unchanged");
    else {
	register struct curve_points *f_p = first_plot;
	register struct surface_points *f_3dp = first_3dplot;

	first_plot = NULL;
	first_3dplot = NULL;
	cp_free(f_p);
	sp_free(f_3dp);

	iso_samples_1 = tsamp1;
	iso_samples_2 = tsamp2;
    }
}


/* FIXME - old bug in this parser
 * to exercise: set key left hiho
 */

/* process 'set key' command */
static void
set_key()
{
    struct value a;

    c_token++;
    if (END_OF_COMMAND) {
	reset_key(); 		/* reset to defaults */
	key_title[0] = 0;
    } else {
	while (!END_OF_COMMAND) {
	    switch(lookup_table(&set_key_tbl[0],c_token)) {
	    case S_KEY_TOP:
		key_vpos = TTOP;
		key = KEY_AUTO_PLACEMENT;
		break;
	    case S_KEY_BOTTOM:
		key_vpos = TBOTTOM;
		key = KEY_AUTO_PLACEMENT;
		break;
	    case S_KEY_LEFT:
		key_hpos = TLEFT;
		/* key_just = TRIGHT; */
		key = KEY_AUTO_PLACEMENT;
		break;
	    case S_KEY_RIGHT:
		key_hpos = TRIGHT;
		key = KEY_AUTO_PLACEMENT;
		break;
	    case S_KEY_UNDER:
		key_vpos = TUNDER;
		if (key_hpos == TOUT)
		    key_hpos = TRIGHT;
		key = KEY_AUTO_PLACEMENT;
		break;
	    case S_KEY_OUTSIDE:
		key_hpos = TOUT;
		if (key_vpos == TUNDER)
		    key_vpos = TBOTTOM;
		key = KEY_AUTO_PLACEMENT;
		break;
	    case S_KEY_LLEFT:
		/* key_hpos = TLEFT; */
		key_just = JLEFT;
		break;
	    case S_KEY_RRIGHT:
		/* key_hpos = TLEFT; */
		key_just = JRIGHT;
		break;
	    case S_KEY_REVERSE:
		key_reverse = TRUE;
		break;
	    case S_KEY_NOREVERSE:
		key_reverse = FALSE;
		break;
	    case S_KEY_BOX:
		c_token++;
		if (END_OF_COMMAND)
		    key_box.l_type = -2;
		else {
		    int old_token = c_token;

		    lp_parse(&key_box,1,0,-2,0);
		    if (old_token == c_token)
			key_box.l_type = real(const_express(&a)) -1;
		}
		c_token--;  /* is incremented after loop */
		break;
	    case S_KEY_NOBOX:
		key_box.l_type = L_TYPE_NODRAW;
		break;
	    case S_KEY_SAMPLEN:
		c_token++;
		key_swidth = real(const_express(&a));
		c_token--; /* it is incremented after loop */
		break;
	    case S_KEY_SPACING:
		c_token++;
		key_vert_factor = real(const_express(&a));
		if (key_vert_factor < 0.0)
		    key_vert_factor = 0.0;
		c_token--; /* it is incremented after loop */
		break;
	    case S_KEY_WIDTH:
		c_token++;
		key_width_fix = real(const_express(&a));
		c_token--; /* it is incremented after loop */
		break;
	    case S_KEY_HEIGHT:
		c_token++;
		key_height_fix = real(const_express(&a));
		c_token--; /* it is incremented after loop */
		break;
	    case S_KEY_TITLE:
		if (isstring(c_token+1)) {
		    /* We have string specified - grab it. */
		    quote_str(key_title,++c_token, MAX_LINE_LEN);
		}
		else
		    key_title[0] = 0;
		break;
	    case S_KEY_INVALID:
	    default:
		get_position(&key_user_pos);
		key = KEY_USER_PLACEMENT;
		c_token--;  /* will be incremented again soon */
		break;
	    }
	    c_token++;
	}
    }
}


/* process 'set keytitle' command */
static void
set_keytitle()
{
    c_token++;
    if (END_OF_COMMAND) {	/* set to default */
	key_title[0] = NUL;
    } else {
	if (isstring(c_token)) {
	    /* We have string specified - grab it. */
	    quote_str(key_title,c_token, MAX_LINE_LEN);
	    c_token++;
	}
	/* c_token++; */
    }
}

/* process 'set label' command */
/* set label {tag} {label_text} {at x,y} {pos} {font name,size} {...} */
/* Entry font added by DJL */
/* allow any order of options - pm 10.11.2001 */
/* Changed again, to still allow every class of exclusive options only
 * *once* in the command line. */
static void
set_label()
{
    struct value a;
    struct text_label *this_label = NULL;
    struct text_label *new_label = NULL;
    struct text_label *prev_label = NULL;
    struct position pos;
    char *text = NULL, *font = NULL;
    enum JUSTIFY just = LEFT;
    int rotate = 0;
    int tag;
    TBOOLEAN set_text = FALSE, set_position = FALSE, set_just = FALSE,
	set_rot = FALSE, set_font = FALSE, set_offset = FALSE,
	set_layer = FALSE;
    int layer = 0;
    struct lp_style_type loc_lp = DEFAULT_LP_STYLE_TYPE;
    float hoff = 1.0;
    float voff = 1.0;

    loc_lp.pointflag = -2;	/* untouched */
    c_token++;
    /* get tag */
    if (!END_OF_COMMAND
	&& !isstring(c_token)
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
	&& !equals(c_token, "font")) {
	/* must be a tag expression! */
	tag = (int) real(const_express(&a));
	if (tag <= 0)
	    int_error(c_token, "tag must be > zero");
    } else
	tag = assign_label_tag();	/* default next tag */

    /* get text */
    if (!END_OF_COMMAND && isstring(c_token)) {
	/* get text */
	text = gp_alloc (token_len(c_token), "text_label->text");
	quote_str(text, c_token, token_len(c_token));
	c_token++;

	/* HBB 20001021: new functionality. If next token is a ','
	 * treat it as a numeric expression whose value is to be
	 * sprintf()ed into the label string (which contains an
	 * appropriate %f format string) */
	if (!END_OF_COMMAND && equals(c_token, ",")) 
	    text = fill_numbers_into_string(text);
	set_text = TRUE;
    }

    while (!END_OF_COMMAND) {
	/* get position */
	if (! set_position && equals(c_token, "at")) {
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
		rotate = TRUE;
		c_token++;
		set_rot = TRUE;
		continue;
	    } else if (almost_equals(c_token, "norot$ate")) {
		rotate = FALSE;
		c_token++;
		set_rot = TRUE;
		continue;
	    }
	}

	/* get font */
	/* Entry font added by DJL */
	if (! set_font && equals(c_token, "font")) {
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "font name and size expected");
	    if (isstring(c_token)) {
		font = gp_alloc (token_len(c_token), "text_label->font");
		quote_str(font, c_token, token_len(c_token));
		/* get 'name,size', no further check */
		set_font = TRUE;
		c_token++;
		continue;
	    } else
		int_error(c_token, "'fontname,fontsize' expected");
	}

	/* get front/back (added by JDP) */
	if (! set_layer) {
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
    
	if (loc_lp.pointflag == -2) {
	    /* read point type */
	    /* FIXME HBB 20011120: should probably have a keyword
	     * "point" required before the actual lp-style spec, for
	     * clarity and better parsing */
	    int stored_token = c_token;
	    struct lp_style_type tmp_lp;
	    
	    lp_parse(&tmp_lp, 0, 1, -2, -2);
	    if (stored_token != c_token) {
		loc_lp = tmp_lp;
		loc_lp.pointflag = 1;
		if (loc_lp.p_type < -1)
		    loc_lp.p_type = 0;
		continue;
	    } else if (almost_equals(c_token, "nopo$int")) {
		/* didn't look like a pointstyle... */
		loc_lp.pointflag = 0;
		c_token++;
		continue;
	    }
	}

	if (! set_offset && almost_equals(c_token, "of$fset")) {
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "Expected horizontal offset");
	    hoff = real(const_express(&a));

	    if (!equals(c_token, ","))
		int_error(c_token, "Expected comma");

	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "Expected vertical offset");
	    voff = real(const_express(&a));
	    set_offset = TRUE;
	    continue;
	}

	/* Coming here means that none of the previous 'if's struck
	 * its "continue" statement, i.e.  whatever is in the command
	 * line is forbidden by the command syntax. */
	int_error(c_token, "extraenous or contradicting arguments in set label");

    } /* while(!END_OF_COMMAND) */

    /* HBB 20011120: this chunk moved here, behind the while()
     * loop. Only after all options have been parsed it's safe to
     * overwrite the position if none has been specified. */
    if (!set_position) {
	pos.x = pos.y = pos.z = 0;
	pos.scalex = pos.scaley = pos.scalez = first_axes;
    }

    /* OK! add label */
    if (first_label != NULL) {	/* skip to last label */
	for (this_label = first_label; this_label != NULL;
	     prev_label = this_label, this_label = this_label->next)
	    /* is this the label we want? */
	    if (tag <= this_label->tag)
		break;
    }
    if (this_label != NULL && tag == this_label->tag) {
	/* changing the label */
	if (set_position) {
	    this_label->place = pos;
	}
	if (set_text)
	    this_label->text = text;
	if (set_just)
	    this_label->pos = just;
	if (set_rot)
	    this_label->rotate = rotate;
	if (set_layer)
	    this_label->layer = layer;
	if (set_font)
	    this_label->font = font;
	if (loc_lp.pointflag >= 0)
	    memcpy(&(this_label->lp_properties), &loc_lp, sizeof(loc_lp));
	if (set_offset) {
	    this_label->hoffset = hoff;
	    this_label->voffset = voff;
	}
    } else {
	/* adding the label */
	new_label = gp_alloc(sizeof(struct text_label), "label");
	if (prev_label != NULL)
	    prev_label->next = new_label;	/* add it to end of list */
	else
	    first_label = new_label;	/* make it start of list */
	new_label->tag = tag;
	new_label->next = this_label;
	new_label->place = pos;
	new_label->text = text;
	new_label->pos = just;
	new_label->rotate = rotate;
	new_label->layer = layer;
	new_label->font = font;
	if (loc_lp.pointflag == -2)
	    loc_lp.pointflag = 0;
	memcpy(&(new_label->lp_properties), &loc_lp, sizeof(loc_lp));
	new_label->hoffset = hoff;
	new_label->voffset = voff;
    }
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
	    int_error(c_token, "Expected string");
	}
    }
    if (collect) {
	set_var_loadpath(collect);
	free(collect);
    }
}


/* process 'set locale' command */
static void
set_locale()
{
    c_token++;
    if (END_OF_COMMAND) {
	init_locale();
    } else if (isstring(c_token)) {
	char *ss = gp_alloc (token_len(c_token), "tmp locale");
	quote_str(ss,c_token,token_len(c_token));
	set_var_locale(ss);
	free(ss);
	++c_token;
    } else {
	int_error(c_token, "Expected string");
    }
}


/* FIXME - check whether the next two functions can be merged */

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

	if ((axis = lookup_table(axisname_tbl, c_token)) >= 0) {
#ifdef PM3D
	    if (axis == COLOR_AXIS) 
		int_error(c_token,"cannot set independent log for cb-axis --- log setup of z-axis is used");
#endif
	    set_for_axis[axis] = TRUE;
	} else { /* must not see x when x2, etc */
	    if (chr_in_str(c_token, 'x'))
 		set_for_axis[FIRST_X_AXIS] = TRUE;
	    if (chr_in_str(c_token, 'y'))
 		set_for_axis[FIRST_Y_AXIS] = TRUE;
	    if (chr_in_str(c_token, 'z'))
 		set_for_axis[FIRST_Z_AXIS] = TRUE;
	}
	c_token++;

	if (!END_OF_COMMAND) {
	    struct value a;
	    newbase = magnitude(const_express(&a));
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
}


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


/* FIXME - merge set_*margin() functions */

#define PROCESS_MARGIN(MARGIN) \
 c_token++; \
 if (END_OF_COMMAND) \
  MARGIN = -1; \
 else { \
  struct value a; \
  MARGIN = real(const_express(&a)); \
 }

/* process 'set bmargin' command */
static void
set_bmargin()
{
    PROCESS_MARGIN(bmargin); 
}


/* process 'set lmargin' command */
static void
set_lmargin()
{
    PROCESS_MARGIN(lmargin);
}


/* process 'set rmargin' command */
static void
set_rmargin()
{
    PROCESS_MARGIN(rmargin);
}


/* process 'set tmargin' command */
static void
set_tmargin()
{
    PROCESS_MARGIN(tmargin);
}


/* process 'set missing' command */
static void
set_missing()
{
    c_token++;
    if (END_OF_COMMAND) {
	if (missing_val)
	    free(missing_val);
	missing_val = NULL;
    } else {
	if (!isstring(c_token))
	    int_error(c_token, "Expected missing-value string");
	m_quote_capture(&missing_val, c_token, c_token);
	c_token++;
    }
}

#ifdef USE_MOUSE
static void
set_mouse()
{
    c_token++;
    mouse_setting.on = 1;

    while (!END_OF_COMMAND) {
	if (almost_equals(c_token, "do$ubleclick")) {
	    struct value a;
	    ++c_token;
	    mouse_setting.doubleclick = real(const_express(&a));
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
	} else if (almost_equals(c_token, "po$lardistance")) {
	    mouse_setting.polardistance = 1;
	    ++c_token;
	} else if (almost_equals(c_token, "nopo$lardistance")) {
	    mouse_setting.polardistance = 0;
	    ++c_token;
	} else if (equals(c_token, "labels")) {
	    mouse_setting.label = 1;
	    ++c_token;
	    /* check if the optional argument "<label options>" is present. */
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
		struct value a;
		int itmp = (int) real(const_express(&a));
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
		struct value a;
		int itmp = (int) real(const_express(&a));
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
	} else {
	    /* discard unknown options (joze) */
	    break;
	}
    }
#if defined(USE_MOUSE) && defined(OS2)
    update_menu_items_PM_terminal();
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
    struct value a;

    c_token++;
    if (END_OF_COMMAND) {
	xoffset = 0.0;
	yoffset = 0.0;
    } else {
	xoffset = real(const_express(&a));
	if (!equals(c_token,","))
	    int_error(c_token, "',' expected");
	c_token++;
	yoffset = real(const_express(&a));
    } 
}


/* process 'set output' command */
static void
set_output()
{
    c_token++;
    if (multiplot)
	int_error(c_token, "you can't change the output in multiplot mode");

    if (END_OF_COMMAND) {	/* no file specified */
	term_set_output(NULL);
	if (outstr) {
	    free(outstr);
	    outstr = NULL; /* means STDOUT */
	}
    } else if (!isstring(c_token)) {
	int_error(c_token, "expecting filename");
    } else {
	/* on int_error, we'd like to remember that this is allocated */
	static char *testfile = NULL;
	m_quote_capture(&testfile,c_token, c_token); /* reallocs store */
	gp_expand_tilde(&testfile);
	/* Skip leading whitespace */
	while (isspace((unsigned char)*testfile))
	    testfile++;
	c_token++;
	term_set_output(testfile);
	/* if we get here then it worked, and outstr now = testfile */
	testfile = NULL;
    }
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

#ifdef PM3D
/* process 'set palette' command */
static void
set_palette()
{
    c_token++;

    if (END_OF_COMMAND) { /* assume default settings */
	sm_palette.colorMode = SMPAL_COLOR_MODE_RGB;
	sm_palette.formulaR = 7; sm_palette.formulaG = 5;
	sm_palette.formulaB = 15;
	sm_palette.positive = SMPAL_POSITIVE;
	sm_palette.ps_allcF = 0;
	sm_palette.use_maxcolors = 0;
    }
    else { /* go through all options of 'set palette' */
	for ( ; !END_OF_COMMAND && !equals(c_token,";"); c_token++ ) {
	    /* positive and negative picture */
	    if (almost_equals(c_token, "pos$itive")) {
		sm_palette.positive = SMPAL_POSITIVE;
		continue;
	    }
	    if (almost_equals(c_token, "neg$ative")) {
		sm_palette.positive = SMPAL_NEGATIVE;
		continue;
	    }
	    /* Now the options that determine the palette of smooth colours */
	    /* gray or rgb-coloured */
	    if (equals(c_token, "gray")) {
		sm_palette.colorMode = SMPAL_COLOR_MODE_GRAY;
		continue;
	    }
	    if (almost_equals(c_token, "col$or")) {
		sm_palette.colorMode = SMPAL_COLOR_MODE_RGB;
		continue;
	    }
	    /* rgb color mapping formulae: rgb$formulae r,g,b (three integers) */
	    if (almost_equals(c_token, "rgb$formulae")) {
		struct value a;
		int i;
		c_token++;
		i = (int)real(const_express(&a));
		if ( abs(i) >= sm_palette.colorFormulae )
		    int_error(c_token,"color formula out of range (use `show pm3d' to display the range)");
		sm_palette.formulaR = i;
		if (!equals(c_token,",")) { c_token--; continue; }
		c_token++;
		i = (int)real(const_express(&a));
		if ( abs(i) >= sm_palette.colorFormulae )
		    int_error(c_token,"color formula out of range (use `show pm3d' to display the range)");
		sm_palette.formulaG = i;
		if (!equals(c_token,",")) { c_token--; continue; }
		c_token++;
		i = (int)real(const_express(&a));
		if ( abs(i) >= sm_palette.colorFormulae )
		    int_error(c_token,"color formula out of range (`show pm3d' displays the range)");
		sm_palette.formulaB = i;
		c_token--;
		continue;
	    } /* rgbformulae */
	    /* ps_allcF: write all rgb formulae into PS file? */
	    if (equals(c_token, "nops_allcF")) {
		sm_palette.ps_allcF = 0;
		continue;
	    }
	    if (equals(c_token, "ps_allcF")) {
		sm_palette.ps_allcF = 1;
		continue;
	    }
	    /* max colors used */
	    if (almost_equals(c_token, "maxc$olors")) {
		struct value a;
		int i;
		c_token++;
		i = (int)real(const_express(&a));
		if (i<0) int_error(c_token,"non-negative number required");
		sm_palette.use_maxcolors = i;
		continue;
	    }
	    int_error(c_token,"invalid palette option");
	} /* end of while over palette options */
    }
}


/* process 'set colorobox' command */
static void
set_colorbox()
{
    c_token++;

    if (END_OF_COMMAND) { /* assume default settings */
	color_box.where = SMCOLOR_BOX_DEFAULT;
	color_box.rotation = 'v';
	color_box.border = 1;
	color_box.border_lt_tag = -1; /* use default border */
    }
    else { /* go through all options of 'set colorbox' */
	for ( ; !END_OF_COMMAND && !equals(c_token,";"); c_token++ ) {
	    /* vertical or horizontal color gradient */
	    if (almost_equals(c_token, "v$ertical")) {
		color_box.rotation = 'v';
		continue;
	    }
	    if (almost_equals(c_token, "h$orizontal")) {
		color_box.rotation = 'h';
		continue;
	    }
#if 0
	    /* obsolete -- use 'unset colorbox' instead */
	    /* color box where: no box */
	    if (equals(c_token, "no")) {
		color_box.where = SMCOLOR_BOX_NO;
		continue;
	    }
#endif
	    /* color box where: default position */
	    if (almost_equals(c_token, "def$ault")) {
		color_box.where = SMCOLOR_BOX_DEFAULT;
		continue;
	    }
	    /* color box where: position by user */
	    if (almost_equals(c_token, "u$ser")) {
		color_box.where = SMCOLOR_BOX_USER;
		continue;
	    }
	    /* border of the color box */
	    if (almost_equals(c_token, "bo$rder")) {

		color_box.border = 1;
		c_token++;

		if (!END_OF_COMMAND) {
		    /* expecting a border line type */
		    struct value a;
		    color_box.border_lt_tag = real(const_express(&a));
		    if (color_box.border_lt_tag <= 0) {
			color_box.border_lt_tag = 0;
			int_error(c_token, "tag must be strictly positive (see `help set style line')");
		    }
		    --c_token; /* why ? (joze) */
		}
		continue;
	    }
	    if (almost_equals(c_token, "bd$efault")) {
		color_box.border_lt_tag = -1; /* use default border */
		continue;
	    }
	    if (almost_equals(c_token, "nob$order")) {
		color_box.border = 0;
		continue;
	    }
	    if (almost_equals(c_token, "o$rigin")) {
		c_token++;
		if (END_OF_COMMAND) {
		    int_error(c_token, "expecting screen value [0 - 1]");
		} else {
		    struct value a;
		    color_box.xorigin = real(const_express(&a));
		    if (!equals(c_token,","))
			int_error(c_token, "',' expected");
		    c_token++;
		    color_box.yorigin = real(const_express(&a));
		    c_token--;
		} 
		continue;
	    }
	    if (almost_equals(c_token, "s$ize")) {
		c_token++;
		if (END_OF_COMMAND) {
		    int_error(c_token, "expecting screen value [0 - 1]");
		} else {
		    struct value a;
		    color_box.xsize = real(const_express(&a));
		    if (!equals(c_token,","))
			int_error(c_token, "',' expected");
		    c_token++;
		    color_box.ysize = real(const_express(&a));
		    c_token--;
		} 
		continue;
	    }
	    int_error(c_token,"invalid colorbox option");
	} /* end of while over colorbox options */
    if (color_box.where == SMCOLOR_BOX_NO) color_box.where = SMCOLOR_BOX_DEFAULT;
    }
}


/* process 'set pm3d' command */
static void
set_pm3d()
{
    c_token++;

    if (END_OF_COMMAND) { /* assume default settings */
	pm3d_reset(); /* sets pm3d.where to 0; setting 'at s' below */
    }
    else { /* go through all options of 'set pm3d' */
	for ( ; !END_OF_COMMAND && !equals(c_token,";"); c_token++ ) {
	    if (equals(c_token, "at")) {
		char* c;
		c_token++;
		if (token[c_token].length >= sizeof(pm3d.where)) {
		    int_error(c_token,"ignoring so many redrawings");
		    return /* (TRUE) */;
		}
		memcpy(pm3d.where, input_line + token[c_token].start_index, token[c_token].length);
		pm3d.where[ token[c_token].length ] = 0;
		for (c = pm3d.where; *c; c++) {
		    if (*c != 'C') /* !!!!! CONTOURS, UNDOCUMENTED, THIS LINE IS TEMPORARILY HERE !!!!! */
			if (*c != PM3D_AT_BASE && *c != PM3D_AT_TOP && *c != PM3D_AT_SURFACE) {
			    int_error(c_token,"parameter to pm3d requires combination of characters b,s,t\n\t(drawing at bottom, surface, top)");
			    return /* (TRUE) */;
			}
		}
		continue;
	    }  /* at */
	    /* forward and backward drawing direction */
	    if (almost_equals(c_token, "scansfor$ward")) {
		pm3d.direction = PM3D_SCANS_FORWARD;
		continue;
	    }
	    if (almost_equals(c_token, "scansback$ward")) {
		pm3d.direction = PM3D_SCANS_BACKWARD;
		continue;
	    }
	    if (almost_equals(c_token, "scansauto$matic")) {
		pm3d.direction = PM3D_SCANS_AUTOMATIC;
		continue;
	    }
	    /* flush scans: left, right or center */
	    if (almost_equals(c_token, "fl$ush")) {
		c_token++;
		if (almost_equals(c_token, "b$egin"))
		    pm3d.flush = PM3D_FLUSH_BEGIN;
		else if (almost_equals(c_token, "c$enter"))
		    pm3d.flush = PM3D_FLUSH_CENTER;
		else if (almost_equals(c_token, "e$nd"))
		    pm3d.flush = PM3D_FLUSH_END;
		else int_error(c_token,"expecting flush 'begin', 'center' or 'end'");
		continue;
	    }
	    /* clipping method */
	    if (almost_equals(c_token, "clip1$in")) {
		pm3d.clip = PM3D_CLIP_1IN;
		continue;
	    }
	    if (almost_equals(c_token, "clip4$in")) {
		pm3d.clip = PM3D_CLIP_4IN;
		continue;
	    }
	    /* setup everything for plotting a map */
	    if (equals(c_token, "map")) {
		pm3d.where[0] = 'b'; pm3d.where[1] = 0; /* set pm3d at b */
		draw_surface = FALSE;        /* set nosurface */
		draw_contour = CONTOUR_NONE; /* set nocontour */
		surface_rot_x = 180;         /* set view 180,0,1.3 */
		surface_rot_z = 0;
		surface_scale = 1.3;
		axis_array[FIRST_Y_AXIS].range_flags |= RANGE_REVERSE; /* set yrange reverse */
		pm3d.map = 1;  /* trick for rotating ylabel */
		continue;
	    }
	    if (almost_equals(c_token, "hi$dden3d")) {
		struct value a;
		c_token++;
		pm3d.hidden3d_tag = real(const_express(&a));
		if (pm3d.hidden3d_tag <= 0) {
		    pm3d.hidden3d_tag = 0;
		    int_error(c_token,"tag must be strictly positive (see `help set style line')");
		}
		--c_token; /* why ? (joze) */
		continue;
	    }
	    if (almost_equals(c_token, "nohi$dden3d")) {
		pm3d.hidden3d_tag = 0;
		continue;
	    }
	    if (almost_equals(c_token, "so$lid") || almost_equals(c_token, "notr$ansparent")) {
		pm3d.solid = 1;
		continue;
	    }
	    if (almost_equals(c_token, "noso$lid") || almost_equals(c_token, "tr$ansparent")) {
		pm3d.solid = 0;
		continue;
	    }
	    if (almost_equals(c_token, "i$mplicit") || almost_equals(c_token, "noe$xplicit")) {
		pm3d.implicit = PM3D_IMPLICIT;
		continue;
	    }
	    if (almost_equals(c_token, "noi$mplicit") || almost_equals(c_token, "e$xplicit")) {
		pm3d.implicit = PM3D_EXPLICIT;
		continue;
	    }
	    int_error(c_token,"invalid pm3d option");
	} /* end of while over pm3d options */
	if (PM3D_SCANS_AUTOMATIC == pm3d.direction
	    && PM3D_FLUSH_BEGIN != pm3d.flush) {
	    pm3d.direction = PM3D_SCANS_FORWARD;
	    /* Petr Mikulik said that he wants no warning message here (joze) */
#if 0
	    fprintf(stderr, "pm3d: `scansautomatic' and %s are incompatible\n",
		PM3D_FLUSH_END == pm3d.flush ? "`flush end'": "`flush center'");
	    fprintf(stderr, "      setting scansforward\n");
#endif
	}
    }
    if (!pm3d.where[0]) /* default: draw at surface */
	strcpy(pm3d.where,"s");
}
#endif

/* process 'set pointsize' command */
static void
set_pointsize()
{
    struct value a;

    c_token++;
    if (END_OF_COMMAND)
	pointsize = 1.0;
    else
	pointsize = real(const_express(&a));
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


/* process 'set samples' command */
static void
set_samples()
{
    register int tsamp1, tsamp2;
    struct value a;

    c_token++;
    tsamp1 = (int)magnitude(const_express(&a));
    tsamp2 = tsamp1;
    if (!END_OF_COMMAND) {
	if (!equals(c_token,","))
	    int_error(c_token, "',' expected");
	c_token++;
	tsamp2 = (int)magnitude(const_express(&a));
    }
    if (tsamp1 < 2 || tsamp2 < 2)
	int_error(c_token, "sampling rate must be > 1; sampling unchanged");
    else {
	register struct surface_points *f_3dp = first_3dplot;

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
    struct value s;

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
	    aspect_ratio = real(const_express(&s));
	} else if (almost_equals(c_token, "nora$tio") || almost_equals(c_token, "nosq$uare")) {
	    aspect_ratio = 0.0;
	    ++c_token;
	}

	if (!END_OF_COMMAND) {
	    xsize = real(const_express(&s));
	    if (equals(c_token,",")) {
		c_token++;
		ysize = real(const_express(&s));
	    } else {
		ysize = xsize;
	    }
	}
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
#ifdef PM3D
	if (data_style & FILLEDCURVES) {
	    get_filledcurves_style_options(&filledcurves_opts_data);
	    if (!filledcurves_opts_data.opt_given) /* default value */
		filledcurves_opts_data.closeto = FILLEDCURVES_CLOSED;
	}
#endif
	break;
    case SHOW_STYLE_FUNCTION:
	{
	    enum PLOT_STYLE temp_style = get_style();

	    if (temp_style & PLOT_STYLE_HAS_ERRORBAR)
		int_error(c_token, "style not usable for function plots, left unchanged");
	    else 
		func_style = temp_style;
#ifdef PM3D
	    if (func_style & FILLEDCURVES) {
		get_filledcurves_style_options(&filledcurves_opts_func);
		if (!filledcurves_opts_func.opt_given) /* default value */
		    filledcurves_opts_func.closeto = FILLEDCURVES_CLOSED;
	    }
#endif
	    break;
	}
    case SHOW_STYLE_LINE:
	set_linestyle();
	break;
#if USE_ULIG_FILLEDBOXES
    case SHOW_STYLE_FILLING:
	set_fillstyle();
	break;
#endif /* USE_ULIG_FILLEDBOXES */
    default:
	int_error(c_token,
#if USE_ULIG_FILLEDBOXES
		  "expecting 'data', 'function', 'line' or 'filling'"
#else
		  "expecting 'data', 'function' or 'line'"
#endif /* USE_ULIG_FILLEDBOXES */
		  );
    }
}


/* process 'set surface' command */
static void
set_surface()
{
    c_token++;
    draw_surface = TRUE;
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
    } else {
	static struct termentry *push_term = NULL;
	static char *push_term_opts = NULL;
	/* `set term push' ? */
	if (equals(c_token,"push")) {
	    if (term) {
		if (push_term == NULL) push_term = gp_alloc(sizeof(struct termentry),"push_term");
		if (push_term_opts != NULL) free(push_term_opts);
		if (push_term) {
		    memcpy((void*)&push_term[0], (void*)term, sizeof(struct termentry));
		    push_term_opts = gp_strdup(term_options);
		    fprintf(stderr, "\tpushed terminal %s %s\n", term->name, push_term_opts);
		}
	    } else
		fputs("\tcurrent terminal type is unknown\n", stderr);
	    c_token++;
	} /* set term push */
	else {
#ifdef EXTENDED_COLOR_SPECS
	/* each terminal is supposed to turn this on, probably
	 * somewhere when the graphics is initialized */
	supply_extended_color_specs = 0;
#endif
#ifdef USE_MOUSE
        event_reset((void *)1);   /* cancel zoombox etc. */
#endif
	term_reset();
	/* `set term pop' ? */
	if (equals(c_token,"pop")) {
	    if (push_term != NULL) {
		int it = interactive;
		interactive = 0;
		change_term(push_term->name, strlen(push_term->name));
		interactive = it;
		memcpy((void*)term, (void*)&push_term[0], sizeof(struct termentry));
		if (push_term_opts != NULL)
		    strcpy(term_options, push_term_opts);
		if (interactive)
		    fprintf(stderr,"\trestored terminal is %s %s\n", term->name, (*term_options) ? term_options : "");
	     } else
		 fprintf(stderr,"No terminal has been pushed yet\n");
	    c_token++;
	} /* set term pop */
	else { /* `set term <normal terminal>' */
	term = 0; /* in case set_term() fails */
	term = set_term(c_token);
	c_token++;
	/* FIXME
	 * handling of common terminal options before term specific options
	 * as per HBB's suggestion
	 * new `set termoptions' command
	 */
	/* get optional mode parameters
	 * not all drivers reset the option string before
	 * strcat-ing to it, so we reset it for them
	 */
	*term_options = 0;
	if (term)
	    (*term->options)();
	if (interactive && *term_options)
	    fprintf(stderr,"Options are '%s'\n",term_options);
		} /* set term <normal terminal> */
	    } /* if / else of 'set term push' */
    }
}

/* process 'set termoptions' command */
static void
set_termoptions()
{
    int_error(c_token,"Command not yet supported");

    /* if called from term.c:
     * scan input_line for common options
     * filter out common options
     * reset input_line (num_tokens = scanner(&input_line, &input_line_len);
     * c_token=0 (1? 2)
     */
}


/* process 'set tics' command */
static void
set_tics()
{
    c_token++;
    tic_in = TRUE;

    if (almost_equals(c_token,"i$n")) {
	tic_in = TRUE;
	c_token++;
    } else if (almost_equals(c_token,"o$ut")) {
	tic_in = FALSE;
	c_token++;
    }
}


/* process 'set ticscale' command */
static void
set_ticscale()
{
    struct value tscl;

    c_token++;
    if (END_OF_COMMAND) {
	ticscale = 1.0;
	miniticscale = 0.5;
    } else {
	ticscale = real(const_express(&tscl));
	if (END_OF_COMMAND) {
	    miniticscale = ticscale*0.5;
	} else {
	    miniticscale = real(const_express(&tscl));
	}
    }
}


/* process 'set ticslevel' command */
static void
set_ticslevel()
{
    struct value a;
    double tlvl;

    c_token++;
    /* is datatype 'time' relevant here ? */
    tlvl = real(const_express(&a));
    ticslevel = tlvl;
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
    c_token++;
    if (END_OF_COMMAND || !isstring(c_token))
	strcpy(timelabel.text, DEFAULT_TIMESTAMP_FORMAT);

    if (!END_OF_COMMAND) {
	struct value a;

	if (isstring(c_token)) {
	    /* we have a format string */
	    quote_str(timelabel.text, c_token, MAX_LINE_LEN);
	    ++c_token;
	} else {
	    strcpy(timelabel.text, DEFAULT_TIMESTAMP_FORMAT);
	}
	if (almost_equals(c_token,"t$op")) {
	    timelabel_bottom = FALSE;
	    ++c_token;
	} else if (almost_equals(c_token, "b$ottom")) {
	    timelabel_bottom = TRUE;
	    ++c_token;
	}
	if (almost_equals(c_token,"r$otate")) {
	    timelabel_rotate = TRUE;
	    ++c_token;
	} else if (almost_equals(c_token, "n$orotate")) {
	    timelabel_rotate = FALSE;
	    ++c_token;
	}
	/* We have x,y offsets specified */
	if (!END_OF_COMMAND && !equals(c_token,","))
	    timelabel.xoffset = real(const_express(&a));
	if (!END_OF_COMMAND && equals(c_token,",")) {
	    c_token++;
	    timelabel.yoffset = real(const_express(&a));
	}
	if (!END_OF_COMMAND && isstring(c_token)) {
	    quote_str(timelabel.font, c_token, MAX_LINE_LEN);
	    ++c_token;
	} else {
	    *timelabel.font = 0;
	}
    }
}


/* process 'set view' command */
static void
set_view()
{
    int i;
    TBOOLEAN was_comma = TRUE;
    char *errmsg1 = "rot_%c must be in [0:%d] degrees range; view unchanged";
    char *errmsg2 = "%sscale must be > 0; view unchanged";
    double local_vals[4];
    struct value a;

    local_vals[0] = surface_rot_x;
    local_vals[1] = surface_rot_z;
    local_vals[2] = surface_scale;
    local_vals[3] = surface_zscale;
    c_token++;
    for (i = 0; i < 4 && !(END_OF_COMMAND);) {
	if (equals(c_token,",")) {
	    if (was_comma) i++;
	    was_comma = TRUE;
	    c_token++;
	} else {
	    if (!was_comma)
		int_error(c_token, "',' expected");
	    local_vals[i] = real(const_express(&a));
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
set_timedata(axis)
    AXIS_INDEX axis;
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
set_range(axis)
    AXIS_INDEX axis;
{
    c_token++;

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
}

/* process 'set xzeroaxis' command */
static void
set_zeroaxis(axis)
    AXIS_INDEX axis;
{

    c_token++;
    if (END_OF_COMMAND)
	axis_array[axis].zeroaxis.l_type = -1;
    else {
	struct value a;
	int old_token = c_token;
	lp_parse(&axis_array[axis].zeroaxis,1,0,-1,0);
	if (old_token == c_token)
	    axis_array[axis].zeroaxis.l_type = real(const_express(&a)) - 1;
}

}

/* process 'set zeroaxis' command */
static void
set_allzeroaxis()
{
    c_token++;
    set_zeroaxis(FIRST_X_AXIS);
    axis_array[FIRST_Y_AXIS].zeroaxis = axis_array[FIRST_X_AXIS].zeroaxis;
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
set_tic_prop(axis)
    AXIS_INDEX axis;
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
	match = 1;
	if (almost_equals(++c_token, "ax$is")) {
	    axis_array[axis].ticmode &= ~TICS_ON_BORDER;
	    axis_array[axis].ticmode |= TICS_ON_AXIS;
	    ++c_token;
	}
	/* if tics are off, reset to default (border) */
	if (axis_array[axis].ticmode == NO_TICS) {
	    axis_array[axis].ticmode = TICS_ON_BORDER;
	    if ((axis == FIRST_X_AXIS) || (axis == FIRST_Y_AXIS)) {
		axis_array[axis].ticmode |= TICS_MIRROR;
	    }
	}
	if (almost_equals(c_token, "bo$rder")) {
	    axis_array[axis].ticmode &= ~TICS_ON_AXIS;
	    axis_array[axis].ticmode |= TICS_ON_BORDER;
	    ++c_token;
	}
	if (almost_equals(c_token, "mi$rror")) {
	    axis_array[axis].ticmode |= TICS_MIRROR;
	    ++c_token;
	} else if (almost_equals(c_token, "nomi$rror")) {
	    axis_array[axis].ticmode &= ~TICS_MIRROR;
	    ++c_token;
	}
	if (almost_equals(c_token, "ro$tate")) {
	    axis_array[axis].tic_rotate = TRUE;
	    ++c_token;
	} else if (almost_equals(c_token, "noro$tate")) {
	    axis_array[axis].tic_rotate = FALSE;
	    ++c_token;
	}
	if (almost_equals(c_token, "au$tofreq")) {	/* auto tic interval */
	    ++c_token;
	    if (axis_array[axis].ticdef.type == TIC_USER) {
		free_marklist(axis_array[axis].ticdef.def.user);
		axis_array[axis].ticdef.def.user = NULL;
	    }
	    axis_array[axis].ticdef.type = TIC_COMPUTED;
	}
	/* user spec. is last */ 
	else if (!END_OF_COMMAND) {
	    load_tics(axis);
	}
    }
    if (almost_equals(c_token, nocmd)) {	/* NOSTRING */
	axis_array[axis].ticmode = NO_TICS;
	c_token++;
	match = 1;
    }
/* other options */

    (void) strcpy(sfxptr, "m$tics");	/* MONTH */
    if (almost_equals(c_token, cmdptr)) {
	if (axis_array[axis].ticdef.type == TIC_USER) {
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
	if (axis_array[axis].ticdef.type == TIC_USER) {
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
	struct value freq;
	c_token++;
	match = 1;
	if (END_OF_COMMAND) {
	    axis_array[axis].minitics = MINI_AUTO;
	} else if (almost_equals(c_token, "def$ault")) {
	    axis_array[axis].minitics = MINI_DEFAULT;
	    ++c_token;
	} else {
	    axis_array[axis].mtic_freq = floor(real(const_express(&freq)));
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
/* set {x/y/z}label {label_text} {x}{,y} */
static void
set_xyzlabel(label)
label_struct *label;
{
    c_token++;
    if (END_OF_COMMAND) {	/* no label specified */
	*label->text = '\0';
	return;
    }
    if (isstring(c_token)) {
	/* We have string specified - grab it. */
	quote_str(label->text, c_token, MAX_LINE_LEN);
	c_token++;
    }
    if (END_OF_COMMAND)
	return;

    if (!almost_equals(c_token, "font") && !isstring(c_token)) {
	/* We have x,y offsets specified */
	struct value a;
	if (!equals(c_token, ","))
	    label->xoffset = real(const_express(&a));

	if (END_OF_COMMAND)
	    return;

	if (equals(c_token, ",")) {
	    c_token++;
	    label->yoffset = real(const_express(&a));
	}
    }
    if (END_OF_COMMAND)
	return;

    /* optional keyword 'font' can go here */

    if (almost_equals(c_token, "f$ont"))
	++c_token;		/* skip it */

    if (!isstring(c_token))
	int_error(c_token, "Expected font");

    quote_str(label->font, c_token, MAX_LINE_LEN);
    c_token++;
}



/* ======================================================== */
/* process a 'set linestyle' command */
/* set linestyle {tag} {linetype n} {linewidth x} {pointtype n} {pointsize x} */
static void
set_linestyle()
{
    struct value a;
    struct linestyle_def *this_linestyle = NULL;
    struct linestyle_def *new_linestyle = NULL;
    struct linestyle_def *prev_linestyle = NULL;
    struct lp_style_type loc_lp = DEFAULT_LP_STYLE_TYPE;
    int tag;

    c_token++;

    /* get tag */
    if (!END_OF_COMMAND) {
	/* must be a tag expression! */
	tag = (int) real(const_express(&a));
	if (tag <= 0)
	    int_error(c_token, "tag must be > zero");
    } else
	tag = assign_linestyle_tag();	/* default next tag */

    /* pick up a line spec : dont allow ls, do allow point type
     * default to same line type = point type = tag
     */
    lp_parse(&loc_lp, 0, 1, tag - 1, tag - 1);

    if (!END_OF_COMMAND)
	int_error(c_token, "extraneous or out-of-order arguments in set linestyle");

    /* OK! add linestyle */
    if (first_linestyle != NULL) {	/* skip to last linestyle */
	for (this_linestyle = first_linestyle; this_linestyle != NULL;
	     prev_linestyle = this_linestyle, this_linestyle = this_linestyle->next)
	    /* is this the linestyle we want? */
	    if (tag <= this_linestyle->tag)
		break;
    }
    if (this_linestyle != NULL && tag == this_linestyle->tag) {
	/* changing the linestyle */
	this_linestyle->lp_properties = loc_lp;
    } else {
	/* adding the linestyle */
	new_linestyle = (struct linestyle_def *)
	    gp_alloc(sizeof(struct linestyle_def), "linestyle");
	if (prev_linestyle != NULL)
	    prev_linestyle->next = new_linestyle;	/* add it to end of list */
	else
	    first_linestyle = new_linestyle;	/* make it start of list */
	new_linestyle->tag = tag;
	new_linestyle->next = this_linestyle;
	new_linestyle->lp_properties = loc_lp;
    }
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
delete_linestyle(prev, this)
struct linestyle_def *prev, *this;
{
    if (this != NULL) {		/* there really is something to delete */
	if (prev != NULL)	/* there is a previous linestyle */
	    prev->next = this->next;
	else			/* this = first_linestyle so change first_linestyle */
	    first_linestyle = this->next;
	free((char *) this);
    }
}

/* For set [xy]tics... command */
static void
load_tics(axis)
AXIS_INDEX axis;
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
 * where tic is ["string"] value
 * Left paren is already scanned off before entry.
 */
static void
load_tic_user(axis)
AXIS_INDEX axis;
{
    struct ticmark *list = NULL;	/* start of list */
    struct ticmark *last = NULL;	/* end of list */
    struct ticmark *tic = NULL;	/* new ticmark */
    char temp_string[MAX_LINE_LEN];

    while (!END_OF_COMMAND) {
	/* parse a new ticmark */
	tic = (struct ticmark *) gp_alloc(sizeof(struct ticmark), (char *) NULL);
	if (tic == (struct ticmark *) NULL) {
	    free_marklist(list);
	    int_error(c_token, "out of memory for tic mark");
	}
	/* syntax is  (  ['format'] value , ... )
	 * but for timedata, the value itself is a string, which
	 * complicates things somewhat
	 */

	/* has a string with it? */
	if (isstring(c_token) &&
	    (!axis_array[axis].is_timedata || isstring(c_token + 1))) {
	    quote_str(temp_string, c_token, MAX_LINE_LEN);
	    tic->label = gp_alloc(strlen(temp_string) + 1, "tic label");
	    (void) strcpy(tic->label, temp_string);
	    c_token++;
	} else
	    tic->label = NULL;

	/* in any case get the value */
	GET_NUM_OR_TIME(tic->position, axis);
	tic->next = NULL;

	/* append to list */
	if (list == NULL)
	    last = list = tic;	/* new list */
	else {			/* append to list */
	    last->next = tic;
	    last = tic;
	}

	/* expect "," or ")" here */
	if (!END_OF_COMMAND && equals(c_token, ","))
	    c_token++;		/* loop again */
	else
	    break;		/* hopefully ")" */
    }

    if (END_OF_COMMAND || !equals(c_token, ")")) {
	free_marklist(list);
	int_error(c_token, "expecting right parenthesis )");
    }
    c_token++;

    /* successful list */
    if (axis_array[axis].ticdef.type == TIC_USER) {
	/* remove old list */
	/* VAX Optimiser was stuffing up following line. Turn Optimiser OFF */
	free_marklist(axis_array[axis].ticdef.def.user);
	axis_array[axis].ticdef.def.user = NULL;
    }
    axis_array[axis].ticdef.type = TIC_USER;
    axis_array[axis].ticdef.def.user = list;
}

static void
free_marklist(list)
struct ticmark *list;
{
    register struct ticmark *freeable;

    while (list != NULL) {
	freeable = list;
	list = list->next;
	if (freeable->label != NULL)
	    free((char *) freeable->label);
	free((char *) freeable);
    }
}

/* load TIC_SERIES definition */
/* [start,]incr[,end] */
static void
load_tic_series(axis)
AXIS_INDEX axis;
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

	if (END_OF_COMMAND)
	    end = VERYLARGE;
	else {
	    if (!equals(c_token, ","))
		int_error(c_token, "expecting comma to separate incr,end");
	    c_token++;
	    GET_NUM_OR_TIME(end, axis);
	}
	if (!END_OF_COMMAND)
	    int_error(c_token, "tic series is defined by [start,]increment[,end]");

	if (start < end && incr <= 0)
	    int_error(incr_token, "increment must be positive");
	if (start > end && incr >= 0)
	    int_error(incr_token, "increment must be negative");
	if (start > end) {
	    /* put in order */
	    double numtics;
	    numtics = floor((end * (1 + SIGNIF) - start) / incr);
	    end = start;
	    start = end + numtics * incr;
	    incr = -incr;
/*
   double temp = start;
   start = end;
   end = temp;
   incr = -incr;
 */
	}
    }

    if (tdef->type == TIC_USER) {
	/* remove old list */
	/* VAX Optimiser was stuffing up following line. Turn Optimiser OFF */
	free_marklist(tdef->def.user);
	tdef->def.user = NULL;
    }
    tdef->type = TIC_SERIES;
    tdef->def.series.start = start;
    tdef->def.series.incr = incr;
    tdef->def.series.end = end;
}

static void
load_offsets(a, b, c, d)
double *a, *b, *c, *d;
{
    struct value t;

    *a = real(const_express(&t));	/* loff value */
    if (!equals(c_token, ","))
	return;

    c_token++;
    *b = real(const_express(&t));	/* roff value */
    if (!equals(c_token, ","))
	return;

    c_token++;
    *c = real(const_express(&t));	/* toff value */
    if (!equals(c_token, ","))
	return;

    c_token++;
    *d = real(const_express(&t));	/* boff value */
}

/* return 1 if format looks like a numeric format
 * ie more than one %{efg}, or %something-else
 */
/* FIXME HBB 20000430: as coded, this will only check the *first*
 * format string, not all of them. */
static int
looks_like_numeric(format)
char *format;
{
    if (!(format = strchr(format, '%')))
	return 0;

    while (++format && (isdigit((unsigned char)*format)
			|| *format == '.'))
	/* do nothing */;

    return (*format == 'f' || *format == 'g' || *format == 'e');
}


/* get_position_type - for use by get_position().
 * parses first/second/graph/screen keyword
 */

static void
get_position_type(type, axes)
enum position_type *type;
int *axes;
{
    if (almost_equals(c_token, "fir$st")) {
	++c_token;
	*type = first_axes;
    } else if (almost_equals(c_token, "sec$ond")) {
	++c_token;
	*type = second_axes;
    } else if (almost_equals(c_token, "gr$aph")) {
	++c_token;
	*type = graph;
    } else if (almost_equals(c_token, "sc$reen")) {
	++c_token;
	*type = screen;
    }
    switch (*type) {
    case first_axes:
	*axes = FIRST_AXES;
	return;
    case second_axes:
	*axes = SECOND_AXES;
	return;
    default:
	*axes = (-1);
	return;
    }
}

/* get_position() - reads a position for label,arrow,key,... */

static void
get_position(pos)
struct position *pos;
{
    int axes;
    enum position_type type = first_axes;

    get_position_type(&type, &axes);
    pos->scalex = type;
    GET_NUMBER_OR_TIME(pos->x, axes, FIRST_X_AXIS);
    if (!equals(c_token, ","))
	int_error(c_token, "Expected comma");
    ++c_token;
    get_position_type(&type, &axes);
    pos->scaley = type;
    GET_NUMBER_OR_TIME(pos->y, axes, FIRST_Y_AXIS);

    /* z is not really allowed for a screen co-ordinate, but keep it simple ! */
    if (equals(c_token, ",")) {
	++c_token;
	get_position_type(&type, &axes);
	pos->scalez = type;
	GET_NUMBER_OR_TIME(pos->z, axes, FIRST_Z_AXIS);
    } else {
	pos->z = 0;
	pos->scalez = type;	/* same as y */
    }
}

/*
 * Backwards compatibility ...
 */
static void set_nolinestyle()
{
    struct value a;
    struct linestyle_def *this, *prev;
    int tag;

    if (END_OF_COMMAND) {
        /* delete all linestyles */
        while (first_linestyle != NULL)
            delete_linestyle((struct linestyle_def *) NULL, first_linestyle);
    } else {
        /* get tag */
        tag = (int) real(const_express(&a));
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


/* HBB 20001021: new function: make label texts decoratable with numbers */
static char *
fill_numbers_into_string(pattern)
     char *pattern;
{
    size_t pattern_length = strlen(pattern) + 1;
    size_t newlen = pattern_length;
    char *output = gp_alloc(newlen, "fill_numbers output buffer");
    size_t output_end = 0;

    do {			/* loop over string/value pairs */
	struct value a;
	double value;
	
	if (isstring(++c_token)) {
	    free(output); 
	    free(pattern); 
	    int_error(c_token, "constant expression expected");
	}

	/* assume it's a numeric expression, concatenate it to output
	 * string: parse value, enlarge output buffer, and gprintf()
	 * it. */
	value = real(const_express(&a));
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
