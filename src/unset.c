/* GNUPLOT - unset.c */

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

#include "setshow.h"

#include "axis.h"
#include "command.h"
#include "contour.h"
#include "datafile.h"
#include "fit.h"
#include "gp_hist.h"
#include "hidden3d.h"
#include "jitter.h"
#include "misc.h"
#include "multiplot.h"
#include "parse.h"
#include "plot.h"
#include "plot2d.h"
#include "plot3d.h"
#include "tables.h"
#include "tabulate.h"
#include "term_api.h"
#include "util.h"
#include "variable.h"
#include "pm3d.h"
#ifdef USE_MOUSE
#include "mouse.h"
#endif
#include "voxelgrid.h"

static void unset_angles(void);
static void unset_arrow(void);
static void unset_arrowstyles(void);
static void free_arrowstyle(struct arrowstyle_def *);
static void delete_arrow(struct arrow_def *, struct arrow_def *);
static void unset_autoscale(void);
static void unset_bars(void);
static void unset_border(void);
static void unset_boxplot(void);
static void unset_boxdepth(void);
static void unset_boxwidth(void);
static void unset_fillstyle(void);
static void unset_clip(void);
static void unset_cntrparam(void);
static void unset_cntrlabel(void);
static void unset_contour(void);
static void unset_dashtype(void);
static void unset_dgrid3d(void);
static void unset_dummy(void);
static void unset_encoding(void);
static void unset_decimalsign(void);
static void unset_fit(void);
static void unset_grid(void);
static void unset_hidden3d(void);
static void unset_histogram(void);
static void unset_textbox_style(void);
static void unset_historysize(void);
static void unset_pixmaps(void);
static void unset_pixmap(int);
static void unset_isosamples(void);
static void unset_key(void);
static void unset_label(void);
static void delete_label(struct text_label * prev, struct text_label * this);
static void unset_linestyle(struct linestyle_def **head);
static void unset_linetype(void);
static void unset_object(void);
static void delete_object(struct object * prev, struct object * this);
static void unset_style_rectangle(void);
static void unset_style_circle(void);
static void unset_style_ellipse(void);
static void unset_style_parallel(void);
static void unset_style_spiderplot(void);
static void unset_wall(int which);
static void unset_loadpath(void);
static void unset_locale(void);
static void reset_logscale(struct axis *);
static void unset_logscale(void);
static void unset_mapping(void);
static void unset_margin(t_position *);
static void unset_missing(void);
static void unset_micro(void);
static void unset_minus_sign(void);
static void unset_mouse(void);

static void unset_month_day_tics(AXIS_INDEX);
static void unset_minitics(struct axis *);

static void unset_offsets(void);
static void unset_origin(void);
static void unset_output(void);
static void unset_parametric(void);
static void unset_pm3d(void);
static void unset_palette(void);
static void reset_colorbox(void);
static void unset_colorbox(void);
static void unset_pointsize(void);
static void unset_pointintervalbox(void);
static void unset_polar(void);
static void unset_print(void);
static void unset_psdir(void);
static void unset_samples(void);
static void unset_size(void);
static void unset_style(void);
static void unset_spiderplot(void);
static void unset_surface(void);
static void unset_table(void);
static void unset_terminal(void);
static void unset_tics(struct axis *);
static void unset_ticslevel(void);
static void unset_timefmt(void);
static void unset_timestamp(void);
static void unset_view(void);
static void unset_zero(void);
static void unset_timedata(AXIS_INDEX);
static void unset_range(AXIS_INDEX);
static void unset_zeroaxis(AXIS_INDEX);
static void unset_all_zeroaxes(void);

static void unset_axislabel_or_title(text_label *);
static void unset_axislabel(AXIS_INDEX);

static void reset_mouse(void);

/******** The 'unset' command ********/
void
unset_command()
{
    int found_token;
    int save_token;
    int i;

    c_token++;
    save_token = c_token;

    set_iterator = check_for_iteration();
    if (empty_iteration(set_iterator)) {
	/* Skip iteration [i=start:end] where start > end */
	while (!END_OF_COMMAND) c_token++;
	set_iterator = cleanup_iteration(set_iterator);
	return;
    }
    if (forever_iteration(set_iterator)) {
	set_iterator = cleanup_iteration(set_iterator);
	int_error(save_token, "unbounded iteration not accepted here");
    }

    found_token = lookup_table(&set_tbl[0],c_token);

    /* HBB 20000506: rationalize occurrences of c_token++ ... */
    if (found_token != S_INVALID)
	c_token++;

    save_token = c_token;
    ITERATE:

    switch(found_token) {
    case S_ANGLES:
	unset_angles();
	break;
    case S_ARROW:
	unset_arrow();
	break;
    case S_AUTOSCALE:
	unset_autoscale();
	break;
    case S_BARS:
	unset_bars();
	break;
    case S_BORDER:
	unset_border();
	break;
    case S_BOXDEPTH:
	unset_boxdepth();
	break;
    case S_BOXWIDTH:
	unset_boxwidth();
	break;
    case S_CLIP:
	unset_clip();
	break;
    case S_CNTRPARAM:
	unset_cntrparam();
	break;
    case S_CNTRLABEL:
	unset_cntrlabel();
	break;
    case S_CLABEL:	/* deprecated command */
	clabel_onecolor = TRUE;
	break;
    case S_CONTOUR:
	unset_contour();
	break;
    case S_DASHTYPE:
	unset_dashtype();
	break;
    case S_DGRID3D:
	unset_dgrid3d();
	break;
    case S_DEBUG:
	debug = 0;
	break;
    case S_DUMMY:
	unset_dummy();
	break;
    case S_ENCODING:
	unset_encoding();
	break;
    case S_DECIMALSIGN:
	unset_decimalsign();
	break;
    case S_FIT:
	unset_fit();
	break;
    case S_FORMAT:
	c_token--;
	set_format();
	break;
    case S_GRID:
	unset_grid();
	break;
    case S_HIDDEN3D:
	unset_hidden3d();
	break;
    case S_HISTORY:
	break; /* FIXME: reset to default values? */
    case S_HISTORYSIZE:	/* Deprecated */
	unset_historysize();
	break;
    case S_PIXMAP:
	if (END_OF_COMMAND)
	    unset_pixmaps();
	else {
	    i = int_expression();
	    unset_pixmap(i);
	}
	break;
    case S_ISOSAMPLES:
	unset_isosamples();
	break;
    case S_JITTER:
	unset_jitter();
	break;
    case S_KEY:
	unset_key();
	break;
    case S_LABEL:
	unset_label();
	break;
    case S_LINETYPE:
	unset_linetype();
	break;
    case S_LINK:
    case S_NONLINEAR:
	c_token--;
	link_command();
	break;
    case S_LOADPATH:
	unset_loadpath();
	break;
    case S_LOCALE:
	unset_locale();
	break;
    case S_LOGSCALE:
	unset_logscale();
	break;
    case S_MACROS:
	/* Aug 2013 - macros are always enabled */
	break;
    case S_MAPPING:
	unset_mapping();
	break;
    case S_MARGIN:
	unset_margin(&lmargin);
	unset_margin(&rmargin);
	unset_margin(&tmargin);
	unset_margin(&bmargin);
	break;
    case S_BMARGIN:
	unset_margin(&bmargin);
	break;
    case S_LMARGIN:
	unset_margin(&lmargin);
	break;
    case S_RMARGIN:
	unset_margin(&rmargin);
	break;
    case S_TMARGIN:
	unset_margin(&tmargin);
	break;
    case S_DATAFILE:
	if (almost_equals(c_token,"fort$ran")) {
	    df_fortran_constants = FALSE;
	    c_token++;
	    break;
	} else if (almost_equals(c_token,"miss$ing")) {
	    unset_missing();
	    c_token++;
	    break;
	} else if (almost_equals(c_token,"sep$arators")) {
	    free(df_separators);
	    df_separators = NULL;
	    c_token++;
	    break;
	} else if (almost_equals(c_token,"com$mentschars")) {
	    free(df_commentschars);
	    df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
	    c_token++;
	    break;
	} else if (almost_equals(c_token,"bin$ary")) {
	    df_unset_datafile_binary();
	    c_token++;
	    break;
	} else if (almost_equals(c_token,"nofpe_trap")) {
	    df_nofpe_trap = FALSE;
	    c_token++;
	    break;
	} else if (almost_equals(c_token,"columnhead$ers")) {
	    df_columnheaders = FALSE;
	    c_token++;
	    break;
	}
	df_fortran_constants = FALSE;
	unset_missing();
	free(df_separators);
	df_separators = NULL;
	free(df_commentschars);
	df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
	df_unset_datafile_binary();
	df_columnheaders = FALSE;
	break;
    case S_MICRO:
	unset_micro();
	break;
    case S_MINUS_SIGN:
	unset_minus_sign();
	break;
    case S_MONOCHROME:
	unset_monochrome();
	break;
    case S_MOUSE:
	unset_mouse();
	break;
    case S_MULTIPLOT:
	term_end_multiplot();
	break;
    case S_OFFSETS:
	unset_offsets();
	break;
    case S_ORIGIN:
	unset_origin();
	break;
    case SET_OUTPUT:
	unset_output();
	break;
    case S_OVERFLOW:
	overflow_handling = INT64_OVERFLOW_IGNORE;
	break;
    case S_PARAMETRIC:
	unset_parametric();
	break;
    case S_PM3D:
	unset_pm3d();
	break;
    case S_PALETTE:
	unset_palette();
	break;
    case S_COLORBOX:
	unset_colorbox();
	break;
    case S_POINTINTERVALBOX:
	unset_pointintervalbox();
	break;
    case S_POINTSIZE:
	unset_pointsize();
	break;
    case S_POLAR:
	unset_polar();
	break;
    case S_PRINT:
	unset_print();
	break;
    case S_PSDIR:
	unset_psdir();
	break;
    case S_OBJECT:
	unset_object();
	break;
    case S_WALL:
	for (i=0; i<5; i++)
	    unset_wall(i);
	break;
    case S_RTICS:
	unset_tics(&axis_array[POLAR_AXIS]);
	break;
    case S_TTICS:
	unset_tics(&THETA_AXIS);
	break;
    case S_PAXIS:
	i = int_expression();
	if (almost_equals(c_token, "tic$s")) {
	    if (i > 0 && i <= num_parallel_axes)
		unset_tics(&parallel_axis_array[i-1]);
	    c_token++;
	}
	break;
    case S_RGBMAX:
	rgbmax = 255;
	break;
    case S_SAMPLES:
	unset_samples();
	break;
    case S_SIZE:
	unset_size();
	break;
    case S_SPIDERPLOT:
	unset_spiderplot();
	break;
    case S_STYLE:
	unset_style();
	break;
    case S_SURFACE:
	unset_surface();
	break;
    case S_TABLE:
	unset_table();
	break;
    case S_TERMINAL:
	unset_terminal();
	break;
    case S_TICS:
	unset_all_tics();
	break;
    case S_TICSCALE:
	int_warn(c_token, "Deprecated syntax - use 'set tics scale default'");
	break;
    case S_TICSLEVEL:
    case S_XYPLANE:
	unset_ticslevel();
	break;
    case S_TIMEFMT:
	unset_timefmt();
	break;
    case S_TIMESTAMP:
	unset_timestamp();
	break;
    case S_TITLE:
	unset_axislabel_or_title(&title);
	break;
    case S_VIEW:
	unset_view();
	break;
    case S_VGRID:
	unset_vgrid();
	break;
    case S_ZERO:
	unset_zero();
	break;
/* FIXME - are the tics correct? */
    case S_MXTICS:
	unset_minitics(&axis_array[FIRST_X_AXIS]);
	break;
    case S_XTICS:
	unset_tics(&axis_array[FIRST_X_AXIS]);
	break;
    case S_XDTICS:
    case S_XMTICS:
	unset_month_day_tics(FIRST_X_AXIS);
	break;
    case S_MYTICS:
	unset_minitics(&axis_array[FIRST_Y_AXIS]);
	break;
    case S_YTICS:
	unset_tics(&axis_array[FIRST_Y_AXIS]);
	break;
    case S_YDTICS:
    case S_YMTICS:
	unset_month_day_tics(FIRST_X_AXIS);
	break;
    case S_MX2TICS:
	unset_minitics(&axis_array[SECOND_X_AXIS]);
	break;
    case S_X2TICS:
	unset_tics(&axis_array[SECOND_X_AXIS]);
	break;
    case S_X2DTICS:
    case S_X2MTICS:
	unset_month_day_tics(FIRST_X_AXIS);
	break;
    case S_MY2TICS:
	unset_minitics(&axis_array[SECOND_Y_AXIS]);
	break;
    case S_Y2TICS:
	unset_tics(&axis_array[SECOND_Y_AXIS]);
	break;
    case S_Y2DTICS:
    case S_Y2MTICS:
	unset_month_day_tics(SECOND_Y_AXIS);
	break;
    case S_MZTICS:
	unset_minitics(&axis_array[FIRST_Z_AXIS]);
	break;
    case S_ZTICS:
	unset_tics(&axis_array[FIRST_Z_AXIS]);
	break;
    case S_ZDTICS:
    case S_ZMTICS:
	unset_month_day_tics(FIRST_X_AXIS);
	break;
    case S_MCBTICS:
	unset_minitics(&axis_array[COLOR_AXIS]);
	break;
    case S_CBTICS:
	unset_tics(&axis_array[COLOR_AXIS]);
	break;
    case S_CBDTICS:
    case S_CBMTICS:
	unset_month_day_tics(FIRST_X_AXIS);
	break;
    case S_MRTICS:
	unset_minitics(&axis_array[POLAR_AXIS]);
	break;
    case S_MTTICS:
	unset_minitics(&THETA_AXIS);
	break;
    case S_XDATA:
	unset_timedata(FIRST_X_AXIS);
	break;
    case S_YDATA:
	unset_timedata(FIRST_Y_AXIS);
	break;
    case S_ZDATA:
	unset_timedata(FIRST_Z_AXIS);
	break;
    case S_CBDATA:
	unset_timedata(COLOR_AXIS);
	break;
    case S_X2DATA:
	unset_timedata(SECOND_X_AXIS);
	break;
    case S_Y2DATA:
	unset_timedata(SECOND_Y_AXIS);
	break;
    case S_XLABEL:
	unset_axislabel(FIRST_X_AXIS);
	break;
    case S_YLABEL:
	unset_axislabel(FIRST_Y_AXIS);
	break;
    case S_ZLABEL:
	unset_axislabel(FIRST_Z_AXIS);
	break;
    case S_CBLABEL:
	unset_axislabel(COLOR_AXIS);
	break;
    case S_RLABEL:
	unset_axislabel(POLAR_AXIS);
	break;
    case S_X2LABEL:
	unset_axislabel(SECOND_X_AXIS);
	break;
    case S_Y2LABEL:
	unset_axislabel(SECOND_Y_AXIS);
	break;
    case S_XRANGE:
	unset_range(FIRST_X_AXIS);
	break;
    case S_X2RANGE:
	unset_range(SECOND_X_AXIS);
	break;
    case S_YRANGE:
	unset_range(FIRST_Y_AXIS);
	break;
    case S_Y2RANGE:
	unset_range(SECOND_Y_AXIS);
	break;
    case S_ZRANGE:
	unset_range(FIRST_Z_AXIS);
	break;
    case S_CBRANGE:
	unset_range(COLOR_AXIS);
	break;
    case S_RRANGE:
	unset_range(POLAR_AXIS);
	break;
    case S_TRANGE:
	unset_range(T_AXIS);
	break;
    case S_URANGE:
	unset_range(U_AXIS);
	break;
    case S_VRANGE:
	unset_range(V_AXIS);
	break;
    case S_RAXIS:
	raxis = FALSE;
	break;
    case S_XZEROAXIS:
	unset_zeroaxis(FIRST_X_AXIS);
	break;
    case S_YZEROAXIS:
	unset_zeroaxis(FIRST_Y_AXIS);
	break;
    case S_ZZEROAXIS:
	unset_zeroaxis(FIRST_Z_AXIS);
	break;
    case S_X2ZEROAXIS:
	unset_zeroaxis(SECOND_X_AXIS);
	break;
    case S_Y2ZEROAXIS:
	unset_zeroaxis(SECOND_Y_AXIS);
	break;
    case S_ZEROAXIS:
	unset_all_zeroaxes();
	break;
    case S_INVALID:
    default:
	int_error(c_token, "Unrecognized option.  See 'help unset'.");
	break;
    }

    if (next_iteration(set_iterator)) {
	c_token = save_token;
	goto ITERATE;
    }

    update_gpval_variables(0);

    set_iterator = cleanup_iteration(set_iterator);
}


/* process 'unset angles' command */
static void
unset_angles()
{
    ang2rad = 1.0;
}


/* process 'unset arrow' command */
static void
unset_arrow()
{
    struct arrow_def *this_arrow;
    struct arrow_def *prev_arrow;
    int tag;

    if (END_OF_COMMAND) {
	/* delete all arrows */
	while (first_arrow != NULL)
	    delete_arrow((struct arrow_def *) NULL, first_arrow);
    } else {
	/* get tag */
	tag = int_expression();
	if (!END_OF_COMMAND)
	    int_error(c_token, "extraneous arguments to unset arrow");
	for (this_arrow = first_arrow, prev_arrow = NULL;
	     this_arrow != NULL;
	     prev_arrow = this_arrow, this_arrow = this_arrow->next) {
	    if (this_arrow->tag == tag) {
		delete_arrow(prev_arrow, this_arrow);
		return;		/* exit, our job is done */
	    }
	}
    }
}


/* delete arrow from linked list started by first_arrow.
 * called with pointers to the previous arrow (prev) and the
 * arrow to delete (this).
 * If there is no previous arrow (the arrow to delete is
 * first_arrow) then call with prev = NULL.
 */
static void
delete_arrow(struct arrow_def *prev, struct arrow_def *this)
{
    if (this != NULL) {		/* there really is something to delete */
	if (prev != NULL)	/* there is a previous arrow */
	    prev->next = this->next;
	else			/* this = first_arrow so change first_arrow */
	    first_arrow = this->next;
	free(this);
    }
}

/* delete the whole list of arrow styles */
static void
unset_arrowstyles()
{
    free_arrowstyle(first_arrowstyle);
    first_arrowstyle = NULL;
}

static void
free_arrowstyle(struct arrowstyle_def *arrowstyle)
{
    if (arrowstyle) {
	free_arrowstyle(arrowstyle->next);
	free(arrowstyle);
    }
}

/*
 * Deletes all pixmaps.
 */
static void
unset_pixmaps(void)
{
    t_pixmap *pixmap, *next;
    for (pixmap = pixmap_listhead; pixmap; pixmap = next) {
	free(pixmap->filename);
	free(pixmap->image_data);
	next = pixmap->next;
	free(pixmap);
    }
    pixmap_listhead = NULL;
}
/*
 * Deletes a single pixmap
 */
static void
unset_pixmap(int i)
{
    t_pixmap *pixmap = pixmap_listhead;
    t_pixmap *prev = pixmap_listhead;
    while (pixmap) {
	if (pixmap->tag == i) {
	    if (pixmap == pixmap_listhead)
		prev = pixmap_listhead = pixmap->next;
	    else
		prev->next = pixmap->next;
	    free(pixmap->filename);
	    free(pixmap->image_data);
	    free(pixmap);
	    pixmap = prev ? prev->next : NULL;
	} else {
	    prev = pixmap;
	    pixmap = pixmap->next;
	}
    }
}


/* process 'unset autoscale' command */
static void
unset_autoscale()
{
    if (END_OF_COMMAND) {
	int axis;
	for (axis=0; axis<AXIS_ARRAY_SIZE; axis++)
	    axis_array[axis].set_autoscale = FALSE;
	for (axis=0; axis<num_parallel_axes; axis++)
	    parallel_axis_array[axis].set_autoscale = FALSE;
    } else if (equals(c_token, "xy") || equals(c_token, "tyx")) {
	axis_array[FIRST_X_AXIS].set_autoscale
	    = axis_array[FIRST_Y_AXIS].set_autoscale = AUTOSCALE_NONE;
	c_token++;
    } else {
	/* HBB 20000506: parse axis name, and unset the right element
	 * of the array: */
	AXIS_INDEX axis = lookup_table(axisname_tbl, c_token);
	if (axis >= 0) {
	    axis_array[axis].set_autoscale = AUTOSCALE_NONE;
	c_token++;
	}
    }
}


/* process 'unset bars' command */
static void
unset_bars()
{
    bar_size = 0.0;
}

/* reset is different from unset */
/* This gets called from 'set bars default' also */
void
reset_bars()
{
    struct lp_style_type def = DEFAULT_LP_STYLE_TYPE;
    bar_lp = def;
    bar_lp.l_type = LT_DEFAULT;
    bar_lp.pm3d_color.type = TC_VARIABLE;
    bar_size = 1.0;
    bar_layer = LAYER_FRONT;
}

/* process 'unset border' command */
static void
unset_border()
{
    /* this is not the effect as with reset, as the border is enabled,
     * by default */
    draw_border = 0;
}


/* process 'unset style boxplot' command */
static void
unset_boxplot()
{
    boxplot_style defstyle = DEFAULT_BOXPLOT_STYLE;
    boxplot_opts = defstyle;
}


/* process 'unset boxdepth' command */
static void
unset_boxdepth()
{
    boxdepth = 0.0;
}


/* process 'unset boxwidth' command */
static void
unset_boxwidth()
{
    boxwidth = -1.0;
    boxwidth_is_absolute = TRUE;
}


/* process 'unset fill' command */
static void
unset_fillstyle()
{
    default_fillstyle.fillstyle = FS_EMPTY;
    default_fillstyle.filldensity = 100;
    default_fillstyle.fillpattern = 0;
    default_fillstyle.border_color.type = TC_DEFAULT;
}


/* process 'unset clip' command */
static void
unset_clip()
{
    if (END_OF_COMMAND) {
	/* same as all three */
	clip_points = FALSE;
	clip_lines1 = FALSE;
	clip_lines2 = FALSE;
	clip_radial = FALSE;
    } else if (almost_equals(c_token, "r$adial") || equals(c_token, "polar"))
	clip_radial = FALSE;
    else if (almost_equals(c_token, "p$oints"))
	clip_points = FALSE;
    else if (almost_equals(c_token, "o$ne"))
	clip_lines1 = FALSE;
    else if (almost_equals(c_token, "t$wo"))
	clip_lines2 = FALSE;
    else
	int_error(c_token, "expecting 'points', 'one', 'two', or 'radial'");
    c_token++;
}


/* process 'unset cntrparam' command */
static void
unset_cntrparam()
{
    contour_pts = DEFAULT_NUM_APPROX_PTS;
    contour_kind = CONTOUR_KIND_LINEAR;
    contour_order = DEFAULT_CONTOUR_ORDER;
    contour_levels = DEFAULT_CONTOUR_LEVELS;
    contour_levels_kind = LEVELS_AUTO;
    contour_firstlinetype = 0;
    contour_sortlevels = FALSE;
}

/* process 'unset cntrlabel' command */
static void
unset_cntrlabel()
{
    clabel_onecolor = FALSE;
    clabel_start = 5;
    clabel_interval = 20;
    strcpy(contour_format, "%8.3g");
    free(clabel_font);
    clabel_font = NULL;
}


/* process 'unset contour' command */
static void
unset_contour()
{
    draw_contour = CONTOUR_NONE;
}


/* process 'unset dashtype' command */
static void
unset_dashtype()
{
    struct custom_dashtype_def *this, *prev;
    if (END_OF_COMMAND) {
	/* delete all */
	while (first_custom_dashtype != NULL)
	    delete_dashtype((struct custom_dashtype_def *) NULL, first_custom_dashtype);
	}
    else {		
	int tag = int_expression();
	for (this = first_custom_dashtype, prev = NULL; this != NULL;
	 prev = this, this = this->next) {
	    if (this->tag == tag) {
		delete_dashtype(prev, this);
		break;
	    }
	}
    }
}


/* process 'unset dgrid3d' command */
static void
unset_dgrid3d()
{
    dgrid3d_row_fineness = 10;
    dgrid3d_col_fineness = 10;
    dgrid3d_norm_value = 1;
    dgrid3d_mode = DGRID3D_QNORM;
    dgrid3d_x_scale = 1.0;
    dgrid3d_y_scale = 1.0;
    dgrid3d = FALSE;
}


/* process 'unset dummy' command */
static void
unset_dummy()
{
    int i;
    strcpy(set_dummy_var[0], "x");
    strcpy(set_dummy_var[1], "y");
    for (i=2; i<MAX_NUM_VAR; i++)
	*set_dummy_var[i] = '\0';
}


/* process 'unset encoding' command */
static void
unset_encoding()
{
    encoding = S_ENC_DEFAULT;
}


/* process 'unset decimalsign' command */
static void
unset_decimalsign()
{
    if (decimalsign != NULL)
	free(decimalsign);
    decimalsign = NULL;
    free(numeric_locale);
    numeric_locale = NULL;
}


/* process 'unset fit' command */
static void
unset_fit()
{
    free(fitlogfile);
    fitlogfile = NULL;
    fit_errorvariables = TRUE;
    fit_covarvariables = FALSE;
    fit_errorscaling = TRUE;
    fit_prescale = TRUE;
    fit_verbosity = BRIEF;
    del_udv_by_name((char *)FITLIMIT, FALSE);
    epsilon_abs = 0.;
    del_udv_by_name((char *)FITMAXITER, FALSE);
    del_udv_by_name((char *)FITSTARTLAMBDA, FALSE);
    del_udv_by_name((char *)FITLAMBDAFACTOR, FALSE);
    free(fit_script);
    fit_script = NULL;
    fit_wrap = 0;
    /* do not reset fit_v4compatible */
}

/* process 'unset grid' command */
static void
unset_grid()
{
    /* FIXME HBB 20000506: there is no command to explicitly reset the
     * linetypes for major and minor gridlines. This function should
     * do that, maybe... */
    AXIS_INDEX i = 0;

    /* grid_selection = GRID_OFF; */
    for (; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++) {
	axis_array[i].gridmajor = FALSE;
	axis_array[i].gridminor = FALSE;
    }
    polar_grid_angle = 0;
    grid_vertical_lines = FALSE;
    grid_spiderweb = FALSE;
}


/* process 'unset hidden3d' command */
static void
unset_hidden3d()
{
    hidden3d = FALSE;
}

static void
unset_histogram()
{
    histogram_style foo = DEFAULT_HISTOGRAM_STYLE;
    free(histogram_opts.title.font);
    free_histlist(&histogram_opts);
    histogram_opts = foo;
}

static void
unset_textbox_style()
{
    textbox_style foo = DEFAULT_TEXTBOX_STYLE;
    int i;

    for (i=0; i<NUM_TEXTBOX_STYLES; i++) {
	textbox_opts[i] = foo;
	if (i>0)
	    textbox_opts[i].linewidth = 0.;
    }
}

/* process 'unset historysize' command DEPRECATED */
static void
unset_historysize()
{
    gnuplot_history_size = -1; /* don't ever truncate the history. */
}


/* process 'unset isosamples' command */
static void
unset_isosamples()
{
    /* HBB 20000506: was freeing 2D data structures although
     * isosamples are only used by 3D plots. */

    sp_free(first_3dplot);
    first_3dplot = NULL;

    iso_samples_1 = ISO_SAMPLES;
    iso_samples_2 = ISO_SAMPLES;
}


void
reset_key()
{
    legend_key temp_key = DEFAULT_KEY_PROPS;
    free(keyT.font);
    free(keyT.title.text);
    free(keyT.title.font);
    memcpy(&keyT, &temp_key, sizeof(keyT));
}

/* process 'unset key' command */
static void
unset_key()
{
    legend_key *key = &keyT;
    key->visible = FALSE;
}


/* process 'unset label' command */
static void
unset_label()
{
    struct text_label *this_label;
    struct text_label *prev_label;
    int tag;

    if (END_OF_COMMAND) {
	/* delete all labels */
	while (first_label != NULL)
	    delete_label((struct text_label *) NULL, first_label);
    } else {
	/* get tag */
	tag = int_expression();
	if (!END_OF_COMMAND)
	    int_error(c_token, "extraneous arguments to unset label");
	for (this_label = first_label, prev_label = NULL;
	     this_label != NULL;
	     prev_label = this_label, this_label = this_label->next) {
	    if (this_label->tag == tag) {
		delete_label(prev_label, this_label);
		return;		/* exit, our job is done */
	    }
	}
    }
}


/* delete label from linked list started by first_label.
 * called with pointers to the previous label (prev) and the
 * label to delete (this).
 * If there is no previous label (the label to delete is
 * first_label) then call with prev = NULL.
 */
static void
delete_label(struct text_label *prev, struct text_label *this)
{
    if (this != NULL) {		/* there really is something to delete */
	if (prev != NULL)	/* there is a previous label */
	    prev->next = this->next;
	else			/* this = first_label so change first_label */
	    first_label = this->next;
	if (this->text) free (this->text);
	if (this->font) free (this->font);
	free (this);
    }
}

static void
unset_linestyle(struct linestyle_def **head)
{
    int tag = int_expression();
    struct linestyle_def *this, *prev;
    for (this = *head, prev = NULL; this != NULL;
	 prev = this, this = this->next) {
	if (this->tag == tag) {
	    delete_linestyle(head, prev, this);
	    break;
	}
    }
}

static void
unset_linetype()
{
    if (equals(c_token,"cycle")) {
	linetype_recycle_count = 0;
	c_token++;
    }
    else if (!END_OF_COMMAND)
	unset_linestyle(&first_perm_linestyle);
}

static void
unset_object()
{
    struct object *this_object;
    struct object *prev_object;
    int tag;

    if (END_OF_COMMAND) {
	/* delete all objects */
	while (first_object != NULL)
	    delete_object((struct object *) NULL, first_object);
    } else {
	/* get tag */
	tag = int_expression();
	if (!END_OF_COMMAND)
	    int_error(c_token, "extraneous arguments to unset rectangle");
	for (this_object = first_object, prev_object = NULL;
	     this_object != NULL;
	     prev_object = this_object, this_object = this_object->next) {
	    if (this_object->tag == tag) {
		delete_object(prev_object, this_object);
		return;		/* exit, our job is done */
	    }
	}
    }
}


/* delete object from linked list started by first_object.
 * called with pointers to the previous object (prev) and the
 * object to delete (this).
 * If there is no previous object (the object to delete is
 * first_object) then call with prev = NULL.
 */
static void
delete_object(struct object *prev, struct object *this)
{
    if (this != NULL) {		/* there really is something to delete */
	if (prev != NULL)	/* there is a previous rectangle */
	    prev->next = this->next;
	else			/* this = first_object so change first_object */
	    first_object = this->next;
	/* NOTE:  Must free contents as well */
	if (this->object_type == OBJ_POLYGON)
	    free(this->o.polygon.vertex);
	free (this);
    }
}


/* process 'unset loadpath' command */
static void
unset_loadpath()
{
    clear_loadpath();
}


/* process 'unset locale' command */
static void
unset_locale()
{
    init_locale();
}

static void
reset_logscale(struct axis *this_axis)
{
    TBOOLEAN undo_rlog = (this_axis->index == POLAR_AXIS && this_axis->log);
    this_axis->log = FALSE;
    /* Do not zero the base because we can still use it for gprintf formats
     * %L and %l with linear axis scaling.
    this_axis->base = 0.0;
     */
    if (undo_rlog)
	rrange_to_xy();
}

/* process 'unset logscale' command */
static void
unset_logscale()
{
    TBOOLEAN set_for_axis[AXIS_ARRAY_SIZE] = AXIS_ARRAY_INITIALIZER(FALSE);
    int axis;

    if (END_OF_COMMAND) {
	for (axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++)
	    set_for_axis[axis] = TRUE;
    } else {
	/* do reverse search because of "x", "x1", "x2" sequence in axisname_tbl */
	int i = 0;
	while (i < token[c_token].length) {
	    axis = lookup_table_nth_reverse(axisname_tbl, NUMBER_OF_MAIN_VISIBLE_AXES,
		       gp_input_line + token[c_token].start_index + i);
	    if (axis < 0) {
		token[c_token].start_index += i;
		int_error(c_token, "invalid axis");
	    }
	    set_for_axis[axisname_tbl[axis].value] = TRUE;
	    i += strlen(axisname_tbl[axis].key);
	}
	c_token++;
    }

    for (axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
	if (set_for_axis[axis]) {
	    static char command[64];
	    if (!isalpha((unsigned char)axis_name(axis)[0]))
		continue;
	    if (axis_array[axis].log) {
		sprintf(command, "unset nonlinear %s", axis_name(axis));
		do_string(command); 
		axis_array[axis].log = FALSE;
	    }
	    axis_array[axis].ticdef.logscaling = FALSE;
	}
    }
}

/* process 'unset mapping3d' command */
static void
unset_mapping()
{
    /* assuming same as points */
    mapping3d = MAP3D_CARTESIAN;
}

/* process 'unset {blrt}margin' command */
static void
unset_margin(t_position *margin)
{
    margin->scalex = character;
    margin->x = -1;
}

/* process 'unset micro' command */
static void
unset_micro()
{
    use_micro = FALSE;
}

/* process 'unset minus_sign' command */
static void
unset_minus_sign()
{
    use_minus_sign = FALSE;
}

/* process 'unset datafile' command */
static void
unset_missing()
{
    free(missing_val);
    missing_val = NULL;
}

/* process 'unset mouse' command */
static void
unset_mouse()
{
#ifdef USE_MOUSE
    mouse_setting.on = 0;
#ifdef OS2
    PM_update_menu_items();
#endif
    UpdateStatusline(); /* wipe status line */
#endif
}

/* process 'unset mxtics' command */
static void
unset_minitics(struct axis *this_axis)
{
    this_axis->minitics = MINI_OFF;
    this_axis->mtic_freq = 10.0;
}

/*process 'unset {x|y|x2|y2|z}tics' command */
void
unset_all_tics()
{
    int i;
    for (i = 0; i < NUMBER_OF_MAIN_VISIBLE_AXES; i++)
	unset_tics(&axis_array[i]);
}

static void
unset_tics(struct axis *this_axis)
{
    struct position tics_nooffset = { character, character, character, 0., 0., 0.};

    this_axis->ticmode = NO_TICS;

    if (this_axis->ticdef.font) {
	free(this_axis->ticdef.font);
	this_axis->ticdef.font = NULL;
    }
    this_axis->ticdef.textcolor.type = TC_DEFAULT;
    this_axis->ticdef.textcolor.lt = 0;
    this_axis->ticdef.textcolor.value = 0;
    this_axis->ticdef.offset = tics_nooffset;
    this_axis->ticdef.rangelimited = FALSE; 
    this_axis->ticdef.enhanced = TRUE;
    this_axis->tic_rotate = 0;
    this_axis->ticscale = 1.0;
    this_axis->miniticscale = 0.5;
    this_axis->tic_in = TRUE;
    this_axis->manual_justify = FALSE;

    free_marklist(this_axis->ticdef.def.user);
    this_axis->ticdef.def.user = NULL;

    if (this_axis->index >= PARALLEL_AXES)
	this_axis->ticdef.rangelimited = TRUE; 
}

static void
unset_month_day_tics(AXIS_INDEX axis)
{
    axis_array[axis].ticdef.type = TIC_COMPUTED;
}

void
unset_monochrome()
{
    monochrome = FALSE;
    if (equals(c_token,"lt") || almost_equals(c_token,"linet$ype")) {
	c_token++;
	if (!END_OF_COMMAND)
	    unset_linestyle(&first_mono_linestyle);
    }
    term->flags &= ~TERM_MONOCHROME;
}

/* process 'unset offsets' command */
static void
unset_offsets()
{
    loff.x = roff.x = 0.0;
    toff.y = boff.y = 0.0;
}


/* process 'unset origin' command */
static void
unset_origin()
{
    xoffset = 0.0;
    yoffset = 0.0;
}


/* process 'unset output' command */
static void
unset_output()
{
    if (multiplot) {
	int_error(c_token, "you can't change the output in multiplot mode");
	return;
    }

    term_set_output(NULL);
    if (outstr) {
	free(outstr);
	outstr = NULL; /* means STDOUT */
    }
}


/* process 'unset print' command */
static void
unset_print()
{
    print_set_output(NULL, FALSE, FALSE);
}

/* process 'unset psdir' command */
static void
unset_psdir()
{
    free(PS_psdir);
    PS_psdir = NULL;
}

/* process 'unset parametric' command */
static void
unset_parametric()
{
    if (parametric) {
	parametric = FALSE;
	if (!polar) { /* keep t for polar */
	    unset_dummy();
	    if (interactive)
		(void) fprintf(stderr,"\n\tdummy variable is x for curves, x/y for surfaces\n");
	}
    }
}

/* process 'unset palette' command */
static void
unset_palette()
{
    c_token++;
    fprintf(stderr, "you can't unset the palette.\n");
}


/* reset colorbox to default settings */
static void
reset_colorbox()
{
    color_box = default_color_box;
}


/* process 'unset colorbox' command: reset to default settings and then
 * switch it off */
static void
unset_colorbox()
{
    reset_colorbox();
    color_box.where = SMCOLOR_BOX_NO;
}


/* process 'unset pm3d' command */
static void
unset_pm3d()
{
    pm3d.implicit = PM3D_EXPLICIT;
    /* reset styles, required to 'plot something' after e.g. 'set pm3d map' */
    if (data_style == PM3DSURFACE) data_style = POINTSTYLE;
    if (func_style == PM3DSURFACE) func_style = LINES;
}


/* process 'unset pointintervalbox' command */
static void
unset_pointintervalbox()
{
    pointintervalbox = 1.0;
}

/* process 'unset pointsize' command */
static void
unset_pointsize()
{
    pointsize = 1.0;
}


/* process 'unset polar' command */
static void
unset_polar()
{
    if (polar) {
	polar = FALSE;
	if (parametric && axis_array[T_AXIS].set_autoscale) {
	    /* only if user has not set an explicit range */
	    axis_array[T_AXIS].set_min = axis_defaults[T_AXIS].min;
	    axis_array[T_AXIS].set_max = axis_defaults[T_AXIS].min;
	}
	if (!parametric) {
	    strcpy (set_dummy_var[0], "x");
	    if (interactive)
		(void) fprintf(stderr,"\n\tdummy variable is x for curves\n");
	}
    }
    raxis = FALSE;
    theta_origin = 0.0;
    theta_direction = 1.0;

    /* Clear and reinitialize THETA axis structure */
    unset_tics(&THETA_AXIS);
    unset_minitics(&THETA_AXIS);
    THETA_AXIS.min = 0.;
    THETA_AXIS.max = 360.;
    THETA_AXIS.ticdef = default_axis_ticdef;
    THETA_AXIS.index = THETA_index;
    free(THETA_AXIS.formatstring);
    THETA_AXIS.formatstring = gp_strdup(DEF_FORMAT);
    THETA_AXIS.ticscale = 1.0;
    THETA_AXIS.miniticscale = 0.5;
    THETA_AXIS.tic_in = TRUE;
    THETA_AXIS.tic_rotate = 0;
}


/* process 'unset samples' command */
static void
unset_samples()
{
    /* HBB 20000506: unlike unset_isosamples(), this one *has* to
     * clear 2D data structures! */
    cp_free(first_plot);
    first_plot = NULL;

    sp_free(first_3dplot);
    first_3dplot = NULL;

    samples_1 = SAMPLES;
    samples_2 = SAMPLES;
}


/* process 'unset size' command */
static void
unset_size()
{
    xsize = 1.0;
    ysize = 1.0;
    zsize = 1.0;
}


/* process 'unset style' command */
static void
unset_style()
{
    if (END_OF_COMMAND) {
	data_style = POINTSTYLE;
	func_style = LINES;
	while (first_linestyle != NULL)
	    delete_linestyle(&first_linestyle, NULL, first_linestyle);
	unset_fillstyle();
	unset_style_rectangle();
	unset_style_circle();
	unset_style_ellipse();
	unset_histogram();
	unset_boxplot();
	unset_textbox_style();
	c_token++;
	return;
    }

    switch(lookup_table(show_style_tbl, c_token)){
    case SHOW_STYLE_DATA:
	data_style = POINTSTYLE;
	c_token++;
	break;
    case SHOW_STYLE_FUNCTION:
	func_style = LINES;
	c_token++;
	break;
    case SHOW_STYLE_LINE:
	c_token++;
	if (END_OF_COMMAND) {
	    while (first_linestyle != NULL)
		delete_linestyle(&first_linestyle, NULL, first_linestyle);
	} else {
	    unset_linestyle(&first_linestyle);
	}
	break;
    case SHOW_STYLE_FILLING:
	unset_fillstyle();
	c_token++;
	break;
    case SHOW_STYLE_HISTOGRAM:
	unset_histogram();
	c_token++;
	break;
    case SHOW_STYLE_ARROW:
	unset_arrowstyles();
	c_token++;
	break;
    case SHOW_STYLE_RECTANGLE:
	unset_style_rectangle();
	c_token++;
	break;
    case SHOW_STYLE_CIRCLE:
	unset_style_circle();
	c_token++;
	break;
    case SHOW_STYLE_ELLIPSE:
	unset_style_ellipse();
	c_token++;
	break;
    case SHOW_STYLE_TEXTBOX:
	unset_textbox_style();
	c_token++;
	break;
    case SHOW_STYLE_BOXPLOT:
	unset_boxplot();
	c_token++;
	break;
    case SHOW_STYLE_PARALLEL:
	unset_style_parallel();
	c_token++;
	break;
    case SHOW_STYLE_SPIDERPLOT:
	unset_style_spiderplot();
	c_token++;
	break;
    default:
	int_error(c_token, "unrecognized style");
    }
}

static void
unset_spiderplot()
{
    if (spiderplot) {
	spiderplot = FALSE;
	data_style = POINTSTYLE;
	aspect_ratio = 0;
    }
}

static void
unset_style_spiderplot()
{
    struct spider_web spiderweb = DEFAULT_SPIDERPLOT_STYLE;
    spiderplot_style = spiderweb;
}

/* process 'unset surface' command */
static void
unset_surface()
{
    draw_surface = FALSE;
}


/* process 'unset table' command */
static void
unset_table()
{
    if (table_outfile)
	fclose(table_outfile);
    table_outfile = NULL;
    table_var = NULL;
    table_mode = FALSE;
}


/* process 'unset terminal' command */
/* Aug 2012:  restore original terminal type */
static void
unset_terminal()
{
    struct udvt_entry *original_terminal = get_udv_by_name("GNUTERM");

    if (multiplot)
	term_end_multiplot();

    term_reset();

    /* FIXME: change is correct but reported result is truncated */
    if (original_terminal && original_terminal->udv_value.type != NOTDEFINED) {
	char *termname = gp_strdup(original_terminal->udv_value.v.string_val);
	if (strchr(termname, ' '))
	    *strchr(termname, ' ') = '\0';
	*term_options = '\0';
	term = change_term(termname, strlen(termname));
	free(termname);
    }
    screen_ok = FALSE;
}


/* process 'unset ticslevel' command */
static void
unset_ticslevel()
{
    xyplane.z = 0.5;
    xyplane.absolute = FALSE;
}


/* Process 'unset timefmt' command */
static void
unset_timefmt()
{
    free(timefmt);
    timefmt = gp_strdup(TIMEFMT);
}


/* process 'unset timestamp' command */
static void
unset_timestamp()
{
    unset_axislabel_or_title(&timelabel);
    timelabel.rotate = 0;
    timelabel_bottom = TRUE;
}


/* process 'unset view' command */
static void
unset_view()
{
    splot_map = FALSE;
    xz_projection = FALSE;
    yz_projection = FALSE;
    aspect_ratio_3D = 0;
    surface_rot_z = 30.0;
    surface_rot_x = 60.0;
    surface_scale = 1.0;
    surface_lscale = 0.0;
    surface_zscale = 1.0;
    azimuth = 0.0;
}

/* process 'unset zero' command */
static void
unset_zero()
{
    zero = ZERO;
}

/* process 'unset {x|y|z|x2|y2}data' command */
static void
unset_timedata(AXIS_INDEX axis)
{
    axis_array[axis].datatype = DT_NORMAL;
    axis_array[axis].tictype = DT_NORMAL;
}


/* process 'unset {x|y|z|x2|y2|t|u|v|r}range' command */
static void
unset_range(AXIS_INDEX axis)
{
    struct axis *this_axis = &axis_array[axis];

    this_axis->writeback_min = this_axis->set_min = axis_defaults[axis].min;
    this_axis->writeback_max = this_axis->set_max = axis_defaults[axis].max;
    this_axis->set_autoscale = AUTOSCALE_BOTH;
    this_axis->min_constraint = CONSTRAINT_NONE;
    this_axis->max_constraint = CONSTRAINT_NONE;
    this_axis->range_flags = 0;
}

/* process 'unset {x|y|x2|y2|z}zeroaxis' command */
static void
unset_zeroaxis(AXIS_INDEX axis)
{
    if (axis_array[axis].zeroaxis != &default_axis_zeroaxis)
	free(axis_array[axis].zeroaxis);
    axis_array[axis].zeroaxis = NULL;
}


/* process 'unset zeroaxis' command */
static void
unset_all_zeroaxes()
{
    AXIS_INDEX axis;

    for (axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++)
	unset_zeroaxis(axis);
}


static void
unset_axislabel_or_title(text_label *label)
{
    if (label) {
	free(label->text);
	free(label->font);
	*label = default_axis_label;
    }
}

/* process 'unset [xyz]{2}label command */
static void
unset_axislabel(AXIS_INDEX axis)
{
    unset_axislabel_or_title(&axis_array[axis].label);
    axis_array[axis].label = default_axis_label;
    if (axis == FIRST_Y_AXIS || axis == SECOND_Y_AXIS || axis == COLOR_AXIS)
	axis_array[axis].label.rotate = TEXT_VERTICAL;
}

/*
 * Free dynamic fields in an axis structure so that it can be safely deleted
 * or reinitialized.  Doesn't free the axis structure itself.
 * SAMPLE_AXIS is an exception because its link pointers are only copies of
 * those in the real axis being sampled.
 */
static void
free_axis_struct(struct axis *this_axis)
{
	free(this_axis->formatstring);
	free(this_axis->ticfmt);
	if (this_axis->link_udf && this_axis->index != SAMPLE_AXIS) {
	    free(this_axis->link_udf->at);
	    free(this_axis->link_udf->definition);
	    free(this_axis->link_udf);
	}
	free_marklist(this_axis->ticdef.def.user);
	free(this_axis->ticdef.font);
	unset_axislabel_or_title(&this_axis->label);
	if (this_axis->zeroaxis != &default_axis_zeroaxis)
	    free(this_axis->zeroaxis);
}

/******** The 'reset' command ********/
/* HBB 20000506: I moved this here, from set.c, because 'reset' really
 * is more like a big lot of 'unset' commands, rather than a bunch of
 * 'set's. The benefit is that it can make use of many of the
 * unset_something() contained in this module, i.e. you now have one
 * place less to keep in sync if the semantics or defaults of any
 * option is changed. This is only true for options for which 'unset'
 * state is the default, however, e.g. not for 'surface', 'bars' and
 * some others. */
void
reset_command()
{
    int i;
    AXIS_INDEX axis;
    TBOOLEAN save_interactive = interactive;

    c_token++;

    /* Reset session state as well as internal graphics state */
    if (equals(c_token, "session")) {
	clear_udf_list();
	init_constants();
	init_session();
	reset_mouse();
	return;
    }

    /* Reset error state (only?) */
    update_gpval_variables(4);
    if (almost_equals(c_token,"err$orstate")) {
	c_token++;
	return;
    }

#ifdef USE_MOUSE
    /* Reset key bindings only */
    if (equals(c_token, "bind")) {
	bind_remove_all();
	c_token++;
	return;
    }
#endif

    if (!(END_OF_COMMAND)) {
	int_warn(c_token, "invalid option, expecting 'session', 'bind' or 'errorstate'");
	while (!(END_OF_COMMAND))
	    c_token++;
    }

#if (0)
    /* DEBUG - enable this code for testing
     * The *.dem unit tests all end with a "reset" command.
     * In order to test save/load from a wide variety of states we can intercept
     * this command and insert save/load before execution.
     */
    extern TBOOLEAN successful_initialization;
    if (successful_initialization) {
	FILE *fp = fopen("/tmp/gnuplot_debug.sav", "w+");
	replot_line[0] = '\0';
	save_all(fp);
	rewind(fp);
	load_file(fp, gp_strdup("/tmp/gnuplot_debug.sav"), 1);
	/* load_file closes fp */
    }
#endif

    /* Kludge alert, HBB 20000506: set to noninteractive mode, to
     * suppress some of the commentary output by the individual
     * unset_...() routines. */
    interactive = FALSE;

    unset_samples();
    unset_isosamples();

    unset_jitter();

    /* delete arrows */
    while (first_arrow != NULL)
	delete_arrow((struct arrow_def *) NULL, first_arrow);
    unset_arrowstyles();
    /* delete labels */
    while (first_label != NULL)
	delete_label((struct text_label *) NULL, first_label);
    /* delete linestyles */
    while (first_linestyle != NULL)
	delete_linestyle(&first_linestyle, NULL, first_linestyle);
    /* delete objects */
    while (first_object != NULL)
	delete_object((struct object *) NULL, first_object);
    unset_style_rectangle();
    unset_style_circle();
    unset_style_ellipse();
    /* delete pixmaps */
    unset_pixmaps();

    /* 'polar', 'parametric' and 'dummy' are interdependent, so be
     * sure to keep the order intact */
    unset_polar();
    unset_parametric();
    unset_dummy();

    unset_spiderplot();
    unset_style_spiderplot();

    unset_axislabel_or_title(&title);

    reset_key();

    unset_view();

    for (axis=0; axis<AXIS_ARRAY_SIZE; axis++) {

	struct axis *this_axis = &axis_array[axis];

	/* Free contents before overwriting with default values */
	free_axis_struct(this_axis);

	/* Fill with generic values, then customize */
	memcpy(this_axis, &default_axis_state, sizeof(AXIS));

	this_axis->formatstring = gp_strdup(DEF_FORMAT);
	this_axis->index = axis;
	unset_axislabel(axis);	/* sets vertical label for y/y2/cb */
	unset_range(axis);	/* copies min/max from axis_defaults */

	/* 'tics' default is on for some, off for the other axes: */
	unset_tics(this_axis);
	unset_minitics(this_axis);
	this_axis->ticdef = default_axis_ticdef;
	this_axis->minitics = MINI_DEFAULT;
	this_axis->ticmode = axis_defaults[axis].ticmode;

	reset_logscale(this_axis);
    }

    /* Free all previously allocated parallel axis structures */
    for (axis=0; axis<num_parallel_axes; axis++) {
	struct axis *this_axis = &parallel_axis_array[axis];
	free_axis_struct(this_axis);
    }
    free(parallel_axis_array);
    parallel_axis_array = NULL;
    num_parallel_axes = 0;
    unset_style_parallel();

    if (shadow_axis_array) {
	for (i=0; i<NUMBER_OF_MAIN_VISIBLE_AXES; i++)
	    free_axis_struct(&shadow_axis_array[i]);
	free(shadow_axis_array);
	shadow_axis_array = NULL;
    }

    raxis = FALSE;
    for (i=2; i<MAX_TICLEVEL; i++)
	ticscale[i] = 1;
    unset_timefmt();

    unset_boxplot();
    unset_boxdepth();
    unset_boxwidth();

    clip_points = FALSE;
    clip_lines1 = TRUE;
    clip_lines2 = FALSE;
    clip_radial = FALSE;

    border_lp = default_border_lp;
    border_layer = LAYER_FRONT;
    draw_border = 31;

    draw_surface = TRUE;
    implicit_surface = TRUE;

    data_style = POINTSTYLE;
    func_style = LINES;

    /* Reset individual plot style options to the default */
    filledcurves_opts_data.closeto = FILLEDCURVES_CLOSED;
    filledcurves_opts_func.closeto = FILLEDCURVES_CLOSED;

    unset_grid();
    grid_lp = default_grid_lp;
    mgrid_lp = default_grid_lp;
    polar_grid_angle = 0;
    grid_layer = LAYER_BEHIND;
    grid_tics_in_front = FALSE;
    for (i=0; i<5; i++)
	unset_wall(i);

    SET_REFRESH_OK(E_REFRESH_NOT_OK, 0);

    reset_hidden3doptions();
    hidden3d = FALSE;

    unset_angles();
    reset_bars();
    unset_mapping();

    unset_size();
    aspect_ratio = 0.0;		/* don't force it */
    rgbmax = 255;

    unset_origin();
    unset_timestamp();
    unset_offsets();
    unset_contour();
    unset_cntrparam();
    unset_cntrlabel();
    unset_zero();
    unset_dgrid3d();
    unset_ticslevel();
    unset_margin(&bmargin);
    unset_margin(&lmargin);
    unset_margin(&rmargin);
    unset_margin(&tmargin);
    unset_pointsize();
    unset_pointintervalbox();
    pm3d_reset();
    reset_colorbox();
    reset_palette();
    df_unset_datafile_binary();
    unset_fillstyle();
    unset_histogram();
    unset_textbox_style();
    prefer_line_styles = FALSE;

#ifdef USE_MOUSE
    mouse_setting = default_mouse_setting;
#endif

    /* restore previous multiplot offset and margins */
    if (multiplot)
	multiplot_reset();

    unset_missing();
    free(df_separators);
    df_separators = NULL;
    free(df_commentschars);
    df_commentschars = gp_strdup(DEFAULT_COMMENTS_CHARS);
    df_init();

    { /* Preserve some settings for `reset`, but not for `unset fit` */
	verbosity_level save_verbosity = fit_verbosity;
	TBOOLEAN save_errorscaling = fit_errorscaling;
	unset_fit();
	fit_verbosity = save_verbosity;
	fit_errorscaling = save_errorscaling;
    }

    update_gpval_variables(0); /* update GPVAL_ inner variables */

    /* HBB 20000506: set 'interactive' back to its real value: */
    interactive = save_interactive;
}

static void
unset_style_rectangle()
{
    struct object foo = DEFAULT_RECTANGLE_STYLE;
    default_rectangle = foo;
    return;
}
static void
unset_style_circle()
{
    struct object foo = DEFAULT_CIRCLE_STYLE;
    default_circle = foo;
    return;
}
static void
unset_style_ellipse()
{
    struct object foo = DEFAULT_ELLIPSE_STYLE;
    default_ellipse = foo;
    return;
}
static void
unset_style_parallel()
{
    struct pa_style parallel_axis_default = DEFAULT_PARALLEL_AXIS_STYLE;
    parallel_axis_style = parallel_axis_default;
}
static void
unset_wall(int which)
{
    grid_wall[which].layer = LAYER_BEHIND;
}

/* Invoked by "reset session".  There is no command line "reset mouse" */
static void
reset_mouse()
{
#ifdef USE_MOUSE
    free_at(mouse_readout_function.at);  /* sets to NULL */
    free(mouse_readout_function.definition);
    mouse_readout_function.definition = NULL;
    free(mouse_alt_string);
    mouse_alt_string = NULL;
    mouse_mode = MOUSE_COORDINATES_REAL;
    mouse_setting = default_mouse_setting;
#endif
}
