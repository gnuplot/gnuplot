#ifndef lint
static char *RCSid() { return RCSid("$Id: unset.c,v 1.30 2002/02/14 14:28:20 mikulik Exp $"); }
#endif

/* GNUPLOT - unset.c */

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

#include "setshow.h"

#include "axis.h"
#include "command.h"
#include "contour.h"
#include "datafile.h"
#include "misc.h"
#include "plot.h"
#include "plot2d.h"
#include "plot3d.h"
#include "tables.h"
#include "term_api.h"
#include "util.h"
#include "variable.h"
#ifdef USE_MOUSE
# include "mouse.h"
#endif

#ifdef PM3D
#include "pm3d.h"
#endif

static void unset_angles __PROTO((void));
static void unset_arrow __PROTO((void));
static void delete_arrow __PROTO((struct arrow_def *, struct arrow_def *));
static void unset_autoscale __PROTO((void));
static void unset_bars __PROTO((void));
static void unset_border __PROTO((void));

static void unset_boxwidth __PROTO((void));
#if USE_ULIG_FILLEDBOXES
static void unset_fillstyle __PROTO((void));
#endif /* USE_ULIG_FILLEDBOXES */
static void unset_clabel __PROTO((void));
static void unset_clip __PROTO((void));
static void unset_cntrparam __PROTO((void));
static void unset_contour __PROTO((void));
static void unset_dgrid3d __PROTO((void));
static void unset_dummy __PROTO((void));
static void unset_encoding __PROTO((void));
static void unset_decimalsign __PROTO((void));
static void unset_format __PROTO((void));
static void unset_grid __PROTO((void));
static void unset_hidden3d __PROTO((void));
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
static void unset_historysize __PROTO((void));
#endif
static void unset_isosamples __PROTO((void));
static void unset_key __PROTO((void));
static void unset_keytitle __PROTO((void));
static void unset_label __PROTO((void));
static void delete_label __PROTO((struct text_label * prev, struct text_label * this));
static void unset_loadpath __PROTO((void));
static void unset_locale __PROTO((void));
static void reset_logscale __PROTO((AXIS_INDEX));
static void unset_logscale __PROTO((void));
static void unset_mapping __PROTO((void));
static void unset_bmargin __PROTO((void));
static void unset_lmargin __PROTO((void));
static void unset_rmargin __PROTO((void));
static void unset_tmargin __PROTO((void));
static void unset_missing __PROTO((void));
#ifdef USE_MOUSE
static void unset_mouse __PROTO((void));
#endif
#if 0
static void unset_multiplot __PROTO((void));
#endif

static void unset_mtics __PROTO((AXIS_INDEX));
static void unset_tics_in __PROTO((void));

static void unset_offsets __PROTO((void));
static void unset_origin __PROTO((void));
static void unset_output __PROTO((void));
static void unset_parametric __PROTO((void));
#ifdef PM3D
static void unset_pm3d __PROTO((void));
static void unset_palette __PROTO((void));
static void unset_colorbox __PROTO((void));
#endif
static void unset_pointsize __PROTO((void));
static void unset_polar __PROTO((void));
static void unset_samples __PROTO((void));
static void unset_size __PROTO((void));
static void unset_style __PROTO((void));
static void unset_surface __PROTO((void));
static void unset_terminal __PROTO((void));
static void unset_tics __PROTO((AXIS_INDEX));
static void unset_ticscale __PROTO((void));
static void unset_ticslevel __PROTO((void));
static void unset_timefmt __PROTO((void));
static void unset_timestamp __PROTO((void));
static void unset_view __PROTO((void));
static void unset_zero __PROTO((void));
static void unset_timedata __PROTO((AXIS_INDEX));
static void unset_range __PROTO((AXIS_INDEX));
static void unset_zeroaxis __PROTO((AXIS_INDEX));
static void unset_all_zeroaxes __PROTO((void));

static void unset_axislabel_or_title __PROTO((label_struct *));
static void unset_axislabel __PROTO((AXIS_INDEX));

/******** The 'unset' command ********/
void
unset_command()
{
    static char GPFAR unsetmess[] = 
    "valid unset options:  [] = choose one, {} means optional\n\n\
\t'angles', 'arrow', 'autoscale', 'bar', 'border', 'boxwidth', 'clabel',\n\
\t'clip', 'cntrparam', 'colorbox', 'contour', 'dgrid3d', 'decimalsign',\n\
\t'dummy', 'encoding', 'format', 'grid', 'hidden3d', 'historysize',\n\
\t'isosamples', 'key', 'label', 'loadpath', 'locale', 'logscale',\n\
\t'[blrt]margin', 'mapping', 'missing', 'mouse', 'multiplot', 'offsets',\n\
\t'origin', 'output', 'palette', 'parametric', 'pm3d', 'pointsize',\n\
\t'polar', '[rtuv]range', 'samples', 'size', 'style', 'surface',\n\
\t'terminal', 'tics', 'ticscale', 'ticslevel', 'timestamp', 'timefmt',\n\
\t'title', 'view', '[xyz,cb]{2}data', '[xyz,cb]{2}label',\n\
\t'[xyz,cb]{2}range', '{m}[xyz,cb]{2}tics', '[xyz,cb]{2}[md]tics',\n\
\t'{[xyz]{2}}zeroaxis', 'zero'";

    int found_token;

    c_token++;

    found_token = lookup_table(&set_tbl[0],c_token);

    /* HBB 20000506: rationalize occurences of c_token++ ... */
    if (found_token != S_INVALID)
	c_token++;

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
    case S_BOXWIDTH:
	unset_boxwidth();
	break;
    case S_CLABEL:
	unset_clabel();
	break;
    case S_CLIP:
	unset_clip();
	break;
    case S_CNTRPARAM:
	unset_cntrparam();
	break;
    case S_CONTOUR:
	unset_contour();
	break;
    case S_DGRID3D:
	unset_dgrid3d();
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
    case S_FORMAT:
	unset_format();
	break;
    case S_GRID:
	unset_grid();
	break;
    case S_HIDDEN3D:
	unset_hidden3d();
	break;
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
    case S_HISTORYSIZE:
	unset_historysize();
	break;
#endif
    case S_ISOSAMPLES:
	unset_isosamples();
	break;
    case S_KEY:
	unset_key();
	break;
    case S_KEYTITLE:
	unset_keytitle();
	break;
    case S_LABEL:
	unset_label();
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
    case S_MAPPING:
	unset_mapping();
	break;
    case S_BMARGIN:
	unset_bmargin();
	break;
    case S_LMARGIN:
	unset_lmargin();
	break;
    case S_RMARGIN:
	unset_rmargin();
	break;
    case S_TMARGIN:
	unset_tmargin();
	break;
    case S_MISSING:
	unset_missing();
	break;
#ifdef USE_MOUSE
    case S_MOUSE:
	unset_mouse();
	break;
#endif
    case S_MULTIPLOT:
/*	unset_multiplot(); */
	term_end_multiplot();
	break;
    case S_OFFSETS:
	unset_offsets();
	break;
    case S_ORIGIN:
	unset_origin();
	break;
    case S_OUTPUT:
	unset_output();
	break;
    case S_PARAMETRIC:
	unset_parametric();
	break;
#ifdef PM3D
    case S_PM3D:
	unset_pm3d();
	break;
    case S_PALETTE:
	unset_palette();
	break;
    case S_COLORBOX:
	unset_colorbox();
	break;
#endif
    case S_POINTSIZE:
	unset_pointsize();
	break;
    case S_POLAR:
	unset_polar();
	break;
    case S_SAMPLES:
	unset_samples();
	break;
    case S_SIZE:
	unset_size();
	break;
    case S_STYLE:
	unset_style();
	break;
    case S_SURFACE:
	unset_surface();
	break;
    case S_TERMINAL:
	unset_terminal();
	break;
    case S_TICS:
	unset_tics_in();
	break;
    case S_TICSCALE:
	unset_ticscale();
	break;
    case S_TICSLEVEL:
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
    case S_ZERO:
	unset_zero();
	break;
/* FIXME - are the tics correct? */
    case S_MXTICS:
	unset_mtics(FIRST_X_AXIS);
	break;
    case S_XTICS:
	unset_tics(FIRST_X_AXIS);
	break;
    case S_XDTICS:
    case S_XMTICS: 
	break;
    case S_MYTICS:
	unset_mtics(FIRST_Y_AXIS);
	break;
    case S_YTICS:
	unset_tics(FIRST_Y_AXIS);
	break;
    case S_YDTICS:
    case S_YMTICS: 
	break;
    case S_MX2TICS:
	unset_mtics(SECOND_X_AXIS);
	break;
    case S_X2TICS:
	unset_tics(SECOND_X_AXIS);
	break;
    case S_X2DTICS:
    case S_X2MTICS: 
	break;
    case S_MY2TICS:
	unset_mtics(SECOND_Y_AXIS);
	break;
    case S_Y2TICS:
	unset_tics(SECOND_Y_AXIS);
	break;
    case S_Y2DTICS:
    case S_Y2MTICS: 
	break;
    case S_MZTICS:
	unset_mtics(FIRST_Z_AXIS);
	break;
    case S_ZTICS:
	unset_tics(FIRST_Z_AXIS);
	break;
    case S_ZDTICS:
    case S_ZMTICS: 
	break;
#ifdef PM3D
    case S_MCBTICS:
	unset_mtics(COLOR_AXIS);
	break;
    case S_CBTICS:
	unset_tics(COLOR_AXIS);
	break;
    case S_CBDTICS:
    case S_CBMTICS: 
	break;
#endif
    case S_XDATA:
	unset_timedata(FIRST_X_AXIS);
	/* FIXME HBB 20000506: does unsetting these axes make *any*
	 * sense?  After all, their content is never displayed, so
	 * what would they need a corrected format for? */
	unset_timedata(T_AXIS);
	unset_timedata(U_AXIS);
	break;
    case S_YDATA:
	unset_timedata(FIRST_Y_AXIS);
	/* FIXME: see above */
	unset_timedata(V_AXIS);
	break;
    case S_ZDATA:
	unset_timedata(FIRST_Z_AXIS);
	break;
#ifdef PM3D
    case S_CBDATA:
	unset_timedata(COLOR_AXIS);
	break;
#endif
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
#ifdef PM3D
    case S_CBLABEL:
	unset_axislabel(COLOR_AXIS);
	break;
#endif
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
#ifdef PM3D
    case S_CBRANGE:
	unset_range(COLOR_AXIS);
	break;
#endif
    case S_RRANGE:
	unset_range(R_AXIS);
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
    case S_XZEROAXIS:
	unset_zeroaxis(FIRST_X_AXIS);
	break;
    case S_YZEROAXIS:
	unset_zeroaxis(FIRST_Y_AXIS);
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
	int_error(c_token, unsetmess);
	break;
    }
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
    struct value a;
    struct arrow_def *this_arrow;
    struct arrow_def *prev_arrow;
    int tag;

    if (END_OF_COMMAND) {
	/* delete all arrows */
	while (first_arrow != NULL)
	    delete_arrow((struct arrow_def *) NULL, first_arrow);
    } else {
	/* get tag */
	tag = (int) real(const_express(&a));
	if (!END_OF_COMMAND)
	    int_error(c_token, "extraneous arguments to set noarrow");
	for (this_arrow = first_arrow, prev_arrow = NULL;
	     this_arrow != NULL;
	     prev_arrow = this_arrow, this_arrow = this_arrow->next) {
	    if (this_arrow->tag == tag) {
		delete_arrow(prev_arrow, this_arrow);
		return;		/* exit, our job is done */
	    }
	}
	int_error(c_token, "arrow not found");
    }
}


/* delete arrow from linked list started by first_arrow.
 * called with pointers to the previous arrow (prev) and the 
 * arrow to delete (this).
 * If there is no previous arrow (the arrow to delete is
 * first_arrow) then call with prev = NULL.
 */
static void
delete_arrow(prev, this)
struct arrow_def *prev, *this;
{
    if (this != NULL) {		/* there really is something to delete */
	if (prev != NULL)	/* there is a previous arrow */
	    prev->next = this->next;
	else			/* this = first_arrow so change first_arrow */
	    first_arrow = this->next;
	free(this);
    }
}


/* process 'unset autoscale' command */
static void
unset_autoscale()
{
    if (END_OF_COMMAND) {
	INIT_AXIS_ARRAY(set_autoscale, FALSE);
    } else if (equals(c_token, "xy") || equals(c_token, "tyx")) {
	axis_array[FIRST_X_AXIS].set_autoscale
	    = axis_array[FIRST_Y_AXIS].set_autoscale = AUTOSCALE_NONE;
	c_token++;
    } else {
	/* HBB 20000506: parse axis name, and unset the right element
	 * of the array: */
	int axis = lookup_table(axisname_tbl, c_token);
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


/* process 'unset border' command */
static void
unset_border()
{
    /* this is not the effect as with reset, as the border is enabled,
     * by default */
    draw_border = 0;
}


/* process 'unset boxwidth' command */
static void
unset_boxwidth()
{
    boxwidth = -1.0;
#if USE_ULIG_RELATIVE_BOXWIDTH
    boxwidth_is_absolute = TRUE;
#endif
}


#if USE_ULIG_FILLEDBOXES
/* process 'unset fillstyle' command */
static void
unset_fillstyle()
{
    fillstyle = 1;
    filldensity = 100;
    fillpattern = 0;
}
#endif /* USE_ULIG_FILLEDBOXES */


/* process 'unset clabel' command */
static void
unset_clabel()
{
    /* FIXME? reset_command() uses TRUE */
    label_contours = FALSE;
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
    } else if (almost_equals(c_token, "p$oints"))
	clip_points = FALSE;
    else if (almost_equals(c_token, "o$ne"))
	clip_lines1 = FALSE;
    else if (almost_equals(c_token, "t$wo"))
	clip_lines2 = FALSE;
    else
	int_error(c_token, "expecting 'points', 'one', or 'two'");
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
}


/* process 'unset contour' command */
static void
unset_contour()
{
    draw_contour = CONTOUR_NONE;
}


/* process 'unset dgrid3d' command */
static void
unset_dgrid3d()
{
    dgrid3d_row_fineness = 10;
    dgrid3d_col_fineness = 10;
    dgrid3d_norm_value = 1;
    dgrid3d = FALSE;
}


/* process 'unset dummy' command */
static void
unset_dummy()
{
    strcpy(set_dummy_var[0], "x");
    strcpy(set_dummy_var[1], "y");
}


/* process 'unset encoding' command */
static void
unset_encoding()
{
    encoding = S_ENC_DEFAULT;
}


/* process 'unset encoding' command */
static void
unset_decimalsign()
{
    if (decimalsign != NULL)
        free(decimalsign);
    decimalsign = NULL;
}


/* process 'unset format' command */
/* FIXME: compare and merge with set.c::set_format */
static void
unset_format()
{
    TBOOLEAN set_for_axis[AXIS_ARRAY_SIZE] = AXIS_ARRAY_INITIALIZER(FALSE);
    int axis;

    if ((axis = lookup_table(axisname_tbl, c_token)) >= 0) {
	set_for_axis[axis] = TRUE;
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
    }
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
    for (; i < AXIS_ARRAY_SIZE; i++) {
	axis_array[i].gridmajor = FALSE;
	axis_array[i].gridminor = FALSE;
    }
}


/* process 'unset hidden3d' command */
static void
unset_hidden3d()
{
#ifdef LITE
    printf(" Hidden Line Removal Not Supported in LITE version\n");
#else
    hidden3d = FALSE;
#endif
}


#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
/* process 'unset historysize' command */
static void
unset_historysize()
{
    gnuplot_history_size = -1; /* don't ever truncate the history. */
}
#endif


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
reset_key(void)
{
    key = KEY_AUTO_PLACEMENT;
    key_hpos = TRIGHT;
    key_vpos = TTOP;
    key_just = JRIGHT;
    key_reverse = FALSE;
    key_enhanced = TRUE;
    key_box = default_keybox_lp;
    key_swidth = 4;
    key_vert_factor = 1;
    key_width_fix = 0;
    key_height_fix = 0;
}

/* process 'unset key' command */
static void
unset_key()
{
    key = KEY_NONE;
}


/* process 'unset keytitle' command */
static void
unset_keytitle()
{
    key_title[0] = '\0';	/* empty string */
}


/* process 'unset label' command */
static void
unset_label()
{
    struct value a;
    struct text_label *this_label;
    struct text_label *prev_label;
    int tag;

    if (END_OF_COMMAND) {
	/* delete all labels */
	while (first_label != NULL)
	    delete_label((struct text_label *) NULL, first_label);
    } else {
	/* get tag */
	tag = (int) real(const_express(&a));
	if (!END_OF_COMMAND)
	    int_error(c_token, "extraneous arguments to set nolabel");
	for (this_label = first_label, prev_label = NULL;
	     this_label != NULL;
	     prev_label = this_label, this_label = this_label->next) {
	    if (this_label->tag == tag) {
		delete_label(prev_label, this_label);
		return;		/* exit, our job is done */
	    }
	}
	int_error(c_token, "label not found");
    }
}


/* delete label from linked list started by first_label.
 * called with pointers to the previous label (prev) and the 
 * label to delete (this).
 * If there is no previous label (the label to delete is
 * first_label) then call with prev = NULL.
 */
static void
delete_label(prev, this)
struct text_label *prev, *this;
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
reset_logscale(axis)
    AXIS_INDEX axis;
{
    axis_array[axis].log = FALSE;
    axis_array[axis].base = 0.0;
}

/* process 'unset logscale' command */
static void
unset_logscale()
{
    int axis;

    if (END_OF_COMMAND) {
	/* clean all the islog flags. This will hit some currently
	 * unused ones, too, but that's actually a good thing, IMHO */
	for(axis = 0; axis < AXIS_ARRAY_SIZE; axis++)
	    reset_logscale(axis);
    } else {
	int i = 0;
	/* do reverse search because of "x", "x1", "x2" sequence in axisname_tbl */
	while (i < token[c_token].length) {
	    axis = lookup_table_nth_reverse( axisname_tbl, AXIS_ARRAY_SIZE, 
		       input_line+token[c_token].start_index+i );
	    if (axis < 0) {
		token[c_token].start_index += i;
		int_error(c_token, "unknown axis");
	    }
	    reset_logscale(axisname_tbl[axis].value);
	    i += strlen(axisname_tbl[axis].key);
	}
	++c_token;
    }
}


/* process 'unset mapping3d' command */
static void
unset_mapping()
{
    /* assuming same as points */
    mapping3d = MAP3D_CARTESIAN;
}


/* process 'unset bmargin' command */
static void
unset_bmargin()
{
    bmargin = -1;
}


/* process 'unset lmargin' command */
static void
unset_lmargin()
{
    lmargin = -1;
}


/* process 'unset rmargin' command */
static void
unset_rmargin()
{
    rmargin = -1;
}


/* process 'unset tmargin' command */
static void
unset_tmargin()
{
    tmargin = -1;
}


/* process 'unset missing' command */
static void
unset_missing()
{
    free(missing_val);
    missing_val = NULL;
}

#ifdef USE_MOUSE
/* process 'unset mouse' command */
static void
unset_mouse()
{
    mouse_setting.on = 0;
#ifdef OS2
    update_menu_items_PM_terminal();
#endif
    UpdateStatusline(); /* wipe status line */
}
#endif

/* process 'unset mxtics' command */
static void
unset_mtics(axis)
    AXIS_INDEX axis;
{
    axis_array[axis].minitics = MINI_DEFAULT;
    axis_array[axis].mtic_freq = 10.0;
}


/*process 'unset {x|y|x2|y2|z}tics' command */
static void
unset_tics(axis)
    AXIS_INDEX axis;
{
    /* FIXME HBB 20000506: shouldn't we remove tic series settings,
     * too? */
    axis_array[axis].ticmode = NO_TICS;
}


/* process 'unset offsets' command */
static void
unset_offsets()
{
    loff = roff = toff = boff = 0.0;
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
    if (multiplot)
	int_error(c_token, "you can't change the output in multiplot mode");

    term_set_output(NULL);
    free(outstr);
    outstr = NULL; /* means STDOUT */
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

#ifdef PM3D
/* process 'unset palette' command */
static void
unset_palette()
{
    c_token++;
    fprintf(stderr, "you can't unset the palette.\n");
}


/* process 'unset colorbox' command */
static void
unset_colorbox()
{
    c_token++;
    color_box.where = SMCOLOR_BOX_NO;
}


/* process 'unset pm3d' command */
static void
unset_pm3d()
{
    c_token++;
    if (pm3d.map) {
	axis_array[FIRST_Y_AXIS].range_flags &= ~RANGE_REVERSE; /* unset reversed y axis */
	draw_surface = TRUE;	/* set surface */
	surface_rot_x = 60.0;	/* set view 60,30,1.3 */
	surface_rot_z = 30;
	surface_scale = 1.0;
    }
    pm3d.where[0] = 0;
    pm3d.map = 0;  /* trick for rotating ylabel */
}
#endif

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
}


/* process 'unset samples' command */
static void
unset_samples()
{
    /* HBB 20000506: unlike unset_isosamples(), this one *has* to
     * clear 2D data structues! */
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
	    delete_linestyle((struct linestyle_def *) NULL, first_linestyle);
#if USE_ULIG_FILLEDBOXES
	unset_fillstyle();
#endif
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
	while (first_linestyle != NULL)
	    delete_linestyle((struct linestyle_def *) NULL, first_linestyle);
	c_token++;
	break;
#if USE_ULIG_FILLEDBOXES
    case SHOW_STYLE_FILLING:
	unset_fillstyle();
	c_token++;
	break;
#endif /* USE_ULIG_FILLEDBOXES */
    default:
#if USE_ULIG_FILLEDBOXES
        int_error(c_token, "expecting 'data', 'function', 'line' or 'filling'");
#else
        int_error(c_token, "expecting 'data', 'function', or 'line'");
#endif /* USE_ULIG_FILLEDBOXES */
    }
}


/* process 'unset surface' command */
static void
unset_surface()
{
    draw_surface = FALSE;
}


/* process 'unset terminal' comamnd */
static void
unset_terminal()
{
    /* This is a problematic case */
/* FIXME */
    if (multiplot)
	int_error(c_token, "You can't change the terminal in multiplot mode");

    list_terms();
    screen_ok = FALSE;
}


/* process 'unset tics' command */
static void
unset_tics_in()
{
    tic_in = TRUE;
}


/* process 'unset ticscale' command */
static void
unset_ticscale()
{
    ticscale = 1.0;
    miniticscale = 0.5;
}


/* process 'unset ticslevel' command */
static void
unset_ticslevel()
{
    ticslevel = 0.5;
}


/* Process 'unset timefmt' command */
static void
unset_timefmt()
{
    int axis;

    if (END_OF_COMMAND)
	for (axis=0; axis < AXIS_ARRAY_SIZE; axis++)
	    strcpy(axis_array[axis].timefmt,TIMEFMT);
    else if ((axis=lookup_table(axisname_tbl, c_token)) >= 0) {
	strcpy(axis_array[axis].timefmt, TIMEFMT);
    c_token++;
    }
    else
	int_error(c_token, "expected optional axis name");
	
}


/* process 'unset notimestamp' command */
static void
unset_timestamp()
{
    unset_axislabel_or_title(&timelabel);
    timelabel_rotate = FALSE;
    timelabel_bottom = TRUE;
}


/* process 'unset view' command */
static void
unset_view()
{
    surface_rot_z = 30.0;
    surface_rot_x = 60.0;
    surface_scale = 1.0;
    surface_zscale = 1.0;
}


/* process 'unset zero' command */
static void
unset_zero()
{
    zero = ZERO;
}

/* process 'unset {x|y|z|x2|y2}data' command */
static void
unset_timedata(axis)
    AXIS_INDEX axis;
{
    axis_array[axis].is_timedata = FALSE;
}


/* process 'unset {x|y|z|x2|y2|t|u|v|r}range' command */
static void
unset_range(axis)
    AXIS_INDEX axis;
{
    /* FIXME HBB 20000506: do we want to reset the axis autoscale and
     * min/max, too?  */
    axis_array[axis].range_flags = 0;
}

/* process 'unset {x|y|z|x2|y2}zeroaxis' command */
static void
unset_zeroaxis(axis)
    AXIS_INDEX axis;
{
    axis_array[axis].zeroaxis = default_axis_zeroaxis;
}


/* process 'unset zeroaxis' command */
static void
unset_all_zeroaxes()
{
    AXIS_INDEX axis;

    for(axis = 0; axis < AXIS_ARRAY_SIZE; axis++)
	unset_zeroaxis(axis);
}


/* process 'unset [xyz]{2}label command */
static void
unset_axislabel_or_title(label)
    label_struct *label;
{
    strcpy(label->text, "");
    strcpy(label->font, "");
    label->xoffset = 0;
    label->yoffset = 0;
}

static void
unset_axislabel(axis)
    AXIS_INDEX axis;
{
    axis_array[axis].label = default_axis_label;
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
    AXIS_INDEX axis;
    TBOOLEAN save_interactive = interactive;
    static const TBOOLEAN set_for_axis[AXIS_ARRAY_SIZE]
	= AXIS_ARRAY_INITIALIZER(TRUE);

    c_token++;

    /* Kludge alert, HBB 20000506: set to noninteractive mode, to
     * suppress some of the commentary output by the individual
     * unset_...() routines. */
    interactive = FALSE;

    unset_samples();
    unset_isosamples();

    /* delete arrows */
    while (first_arrow != NULL)
	delete_arrow((struct arrow_def *) NULL, first_arrow);
    /* delete labels */
    while (first_label != NULL)
	delete_label((struct text_label *) NULL, first_label);
    /* delete linestyles */
    while (first_linestyle != NULL)
	delete_linestyle((struct linestyle_def *) NULL, first_linestyle);

    /* 'polar', 'parametric' and 'dummy' are interdependent, so be
     * sure to keep the order intact */
    unset_polar();
    unset_parametric();
    unset_dummy();

    unset_axislabel_or_title(&title);

    reset_key();
    unset_keytitle();

    unset_timefmt();

    for (axis=0; axis<AXIS_ARRAY_SIZE; axis++) {
	SET_DEFFORMAT(axis, set_for_axis);
	unset_timedata(axis);
	unset_zeroaxis(axis);
	unset_range(axis);
	unset_axislabel(axis);

	axis_array[axis].set_autoscale = AUTOSCALE_BOTH;
	axis_array[axis].writeback_min = axis_array[axis].set_min
	    = axis_defaults[axis].min;
	axis_array[axis].writeback_max = axis_array[axis].set_max
	    = axis_defaults[axis].max;

	/* 'tics' default is on for some, off for the other axes: */
	axis_array[axis].ticmode = axis_defaults[axis].ticmode;
	unset_mtics(axis);
	axis_array[axis].ticdef = default_axis_ticdef;

	reset_logscale(axis);
}

    unset_boxwidth();

    clip_points = FALSE;
    clip_lines1 = TRUE;
    clip_lines2 = FALSE;

    border_lp = default_border_lp;
    draw_border = 31;

    draw_surface = 1.0;

    data_style = POINTSTYLE;
    func_style = LINES;

    bar_size = 1.0;

    unset_grid();
    grid_lp = default_grid_lp;
    mgrid_lp = default_grid_lp;
    polar_grid_angle = 0;
    grid_layer = -1;

    hidden3d = FALSE;

    label_contours = TRUE;
    strcpy(contour_format, "%8.3g");

    unset_angles();
    unset_mapping();

    unset_size();
    aspect_ratio = 0.0;		/* dont force it */

    unset_origin();
    unset_view();
    unset_timestamp();
    unset_offsets();
    unset_contour();
    unset_cntrparam();
    unset_zero();
    unset_dgrid3d();
    unset_ticscale();
    unset_ticslevel();
    unset_tics_in();
    unset_lmargin();
    unset_bmargin();
    unset_rmargin();
    unset_tmargin();
    unset_pointsize();
    unset_encoding();
    unset_decimalsign();
#ifdef PM3D
    pm3d_reset();
#endif
    unset_locale();
    unset_loadpath();

    /* HBB 20000506: set 'interactive' back to its real value: */
    interactive = save_interactive;
}

