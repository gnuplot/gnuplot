#ifndef lint
static char *RCSid() { return RCSid("$Id: unset.c,v 1.9.2.4 2000/06/24 21:57:33 mikulik Exp $"); }
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

#include "command.h"
#include "misc.h"
#include "parse.h"
#include "plot2d.h"
#include "plot3d.h"
#include "tables.h"
#include "term_api.h"
#include "util.h"
#ifdef USE_MOUSE
#   include "mouse.h"
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
static void unset_clabel __PROTO((void));
static void unset_clip __PROTO((void));
static void unset_cntrparam __PROTO((void));
static void unset_contour __PROTO((void));
static void unset_dgrid3d __PROTO((void));
static void unset_dummy __PROTO((void));
static void unset_encoding __PROTO((void));
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

static void unset_mxtics __PROTO((void));
static void unset_mx2tics __PROTO((void));
static void unset_mytics __PROTO((void));
static void unset_my2tics __PROTO((void));
static void unset_mztics __PROTO((void));

static void unset_xtics __PROTO((void));
static void unset_x2tics __PROTO((void));
static void unset_ytics __PROTO((void));
static void unset_y2tics __PROTO((void));
static void unset_ztics __PROTO((void));

static void unset_offsets __PROTO((void));
static void unset_origin __PROTO((void));
static void unset_output __PROTO((void));
static void unset_parametric __PROTO((void));
#ifdef PM3D
static void unset_palette __PROTO((void));
static void unset_pm3d __PROTO((void));
#endif
static void unset_pointsize __PROTO((void));
static void unset_polar __PROTO((void));
static void unset_samples __PROTO((void));
static void unset_size __PROTO((void));
static void unset_style __PROTO((void));
static void unset_surface __PROTO((void));
static void unset_terminal __PROTO((void));
static void unset_tics __PROTO((void));
static void unset_ticscale __PROTO((void));
static void unset_ticslevel __PROTO((void));
static void unset_timefmt __PROTO((void));
static void unset_timestamp __PROTO((void));
static void unset_view __PROTO((void));
static void unset_zero __PROTO((void));
static void unset_xdata __PROTO((void));
static void unset_ydata __PROTO((void));
static void unset_zdata __PROTO((void));
static void unset_x2data __PROTO((void));
static void unset_y2data __PROTO((void));
static void unset_xrange __PROTO((void));
static void unset_x2range __PROTO((void));
static void unset_yrange __PROTO((void));
static void unset_y2range __PROTO((void));
static void unset_zrange __PROTO((void));
static void unset_rrange __PROTO((void));
static void unset_trange __PROTO((void));
static void unset_urange __PROTO((void));
static void unset_vrange __PROTO((void));
static void unset_xzeroaxis __PROTO((void));
static void unset_yzeroaxis __PROTO((void));
static void unset_x2zeroaxis __PROTO((void));
static void unset_y2zeroaxis __PROTO((void));
static void unset_zeroaxis __PROTO((void));

static void unset_xyzlabel __PROTO((label_struct *));

/******** The 'unset' command ********/
void
unset_command()
{
    static char GPFAR unsetmess[] = 
    "valid unset options:  [] = choose one, {} means optional\n\n\
\t'angles',  'arrow',  'autoscale',  'bar',  'border', 'boxwidth',\n\
\t'clabel', 'clip', 'cntrparam', 'contour', 'dgrid3d',  'dummy',\n\
\t'encoding',  'format', 'grid', 'hidden3d',  'historysize',  'isosamples',\n\
\t'key',  'label', 'loadpath', 'locale', 'logscale', '[blrt]margin',\n\
\t'mapping',  'missing', 'mouse', 'multiplot', 'offsets', 'origin',\n\
\t'output', 'palette', 'parametric', 'pm3d', 'pointsize', 'polar',\n\
\t'[rtuv]range',  'samples', 'size', 'style', 'surface', 'terminal',\n\
\t'tics',  'ticscale', 'ticslevel', 'timestamp',  'timefmt', 'title',\n\
\t'view', '[xyz]{2}data', '[xyz]{2}label', '[xyz]{2}range',\n\
\t'{m}[xyz]{2}tics', '[xyz]{2}[md]tics', '{[xyz]{2}}zeroaxis', 'zero'";

    c_token++;

    switch(lookup_table(&set_tbl[0],c_token)) {
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
    case S_PALETTE:
	unset_palette();
	break;
    case S_PM3D:
	unset_pm3d();
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
	unset_tics();
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
	unset_xyzlabel(&title);
	break;
    case S_VIEW:
	unset_view();
	break;
    case S_ZERO:
	unset_zero();
	break;
/* FIXME - are the tics correct? */
    case S_MXTICS:
	unset_mxtics();
	break;
    case S_XTICS:
	unset_xtics();
	break;
    case S_XDTICS:
    case S_XMTICS: 
	c_token++;
	break;
    case S_MYTICS:
	unset_mytics();
	break;
    case S_YTICS:
	unset_ytics();
	break;
    case S_YDTICS:
    case S_YMTICS: 
	break;
    case S_MX2TICS:
	unset_mx2tics();
	break;
    case S_X2TICS:
	unset_x2tics();
	break;
    case S_X2DTICS:
    case S_X2MTICS: 
	break;
    case S_MY2TICS:
	unset_my2tics();
	break;
    case S_Y2TICS:
	unset_y2tics();
	break;
    case S_Y2DTICS:
    case S_Y2MTICS: 
	break;
    case S_MZTICS:
	unset_mztics();
	break;
    case S_ZTICS:
	unset_ztics();
	break;
    case S_ZDTICS:
    case S_ZMTICS: 
	break;
    case S_XDATA:
	unset_xdata();
	break;
    case S_YDATA:
	unset_ydata();
	break;
    case S_ZDATA:
	unset_zdata();
	break;
    case S_X2DATA:
	unset_x2data();
	break;
    case S_Y2DATA:
	unset_y2data();
	break;
    case S_XLABEL:
	unset_xyzlabel(&xlabel);
	break;
    case S_YLABEL:
	unset_xyzlabel(&ylabel);
	break;
    case S_ZLABEL:
	unset_xyzlabel(&zlabel);
	break;
    case S_X2LABEL:
	unset_xyzlabel(&x2label);
	break;
    case S_Y2LABEL:
	unset_xyzlabel(&y2label);
	break;
    case S_XRANGE:
	unset_xrange();
	break;
    case S_X2RANGE:
	unset_x2range();
	break;
    case S_YRANGE:
	unset_yrange();
	break;
    case S_Y2RANGE:
	unset_y2range();
	break;
    case S_ZRANGE:
	unset_zrange();
	break;
    case S_RRANGE:
	unset_rrange();
	break;
    case S_TRANGE:
	unset_trange();
	break;
    case S_URANGE:
	unset_urange();
	break;
    case S_VRANGE:
	unset_vrange();
	break;
    case S_XZEROAXIS:
	unset_xzeroaxis();
	break;
    case S_YZEROAXIS:
	unset_yzeroaxis();
	break;
    case S_X2ZEROAXIS:
	unset_x2zeroaxis();
	break;
    case S_Y2ZEROAXIS:
	unset_y2zeroaxis();
	break;
    case S_ZEROAXIS:
	unset_zeroaxis();
	break;
    default:
	int_error(c_token, unsetmess);
	break;
    }
}


/* process 'unset angles' command */
static void
unset_angles()
{
    c_token++;
    angles_format = ANGLES_RADIANS;
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

    c_token++;

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
	free((char *) this);
    }
}


/* process 'unset autoscale' command */
static void
unset_autoscale()
{
    c_token++;
    if (END_OF_COMMAND) {
	autoscale_r = autoscale_t = autoscale_x = autoscale_y = autoscale_z = FALSE;
    } else if (equals(c_token, "xy") || equals(c_token, "tyx")) {
	autoscale_x = autoscale_y = FALSE;
	c_token++;
    } else if (equals(c_token, "r")) {
	autoscale_r = FALSE;
	c_token++;
    } else if (equals(c_token, "t")) {
	autoscale_t = FALSE;
	c_token++;
    } else if (equals(c_token, "u")) {
	autoscale_u = FALSE;
	c_token++;
    } else if (equals(c_token, "v")) {
	autoscale_v = FALSE;
	c_token++;
    } else if (equals(c_token, "x")) {
	autoscale_x = FALSE;
	c_token++;
    } else if (equals(c_token, "y")) {
	autoscale_y = FALSE;
	c_token++;
    } else if (equals(c_token, "x2")) {
	autoscale_x2 = FALSE;
	c_token++;
    } else if (equals(c_token, "y2")) {
	autoscale_y2 = FALSE;
	c_token++;
    } else if (equals(c_token, "z")) {
	autoscale_z = FALSE;
	c_token++;
    }
}


/* process 'unset bars' command */
static void
unset_bars()
{
    c_token++;
    bar_size = 0.0;
}


/* process 'unset border' command */
static void
unset_border()
{
    c_token++;
    /* FIXME? reset_command() uses 31 */
    draw_border = 0;
}


/* process 'unset boxwidth' command */
static void
unset_boxwidth()
{
    c_token++;
    boxwidth = -1.0;
}


/* process 'unset clabel' command */
static void
unset_clabel()
{
    c_token++;
    /* FIXME? reset_command() uses TRUE */
    label_contours = FALSE;
}


/* process 'unset clip' command */
static void
unset_clip()
{
    c_token++;
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
    c_token++;

    contour_pts = 5;
    contour_kind = CONTOUR_KIND_LINEAR;
    contour_order = 4;
    contour_levels = 5;
    levels_kind = LEVELS_AUTO;
}


/* process 'unset contour' command */
static void
unset_contour()
{
    c_token++;
    draw_contour = CONTOUR_NONE;
}


/* process 'unset dgrid3d' command */
static void
unset_dgrid3d()
{
    c_token++;
    dgrid3d_row_fineness = 10;
    dgrid3d_col_fineness = 10;
    dgrid3d_norm_value = 1;
    dgrid3d = FALSE;
}


/* process 'unset dummy' command */
static void
unset_dummy()
{
    c_token++;
    strcpy(dummy_var[0], "x");
    strcpy(dummy_var[1], "y");
}


/* process 'unset encoding' command */
static void
unset_encoding()
{
    c_token++;

    encoding = S_ENC_DEFAULT;
}


/* process 'unset format' command */
/* FIXME: compare and merge with set.c::set_format */
static void
unset_format()
{
    TBOOLEAN setx = FALSE, sety = FALSE, setz = FALSE;
    TBOOLEAN setx2 = FALSE, sety2 = FALSE;

    c_token++;
    if (equals(c_token,"x")) {
        setx = TRUE;
        c_token++;
    } else if (equals(c_token,"y")) {
        sety = TRUE;
        c_token++;
    } else if (equals(c_token,"x2")) {
        setx2 = TRUE;
        c_token++;
    } else if (equals(c_token,"y2")) {
        sety2 = TRUE;
        c_token++;
    } else if (equals(c_token,"z")) {
        setz = TRUE;
        c_token++;
    } else if (equals(c_token,"xy") || equals(c_token,"yx")) {
        setx = sety = TRUE;
        c_token++;
    } else if (isstring(c_token) || END_OF_COMMAND) {
        /* Assume he wants all */
        setx = sety = setz = setx2 = sety2 = TRUE;
    }

    if (END_OF_COMMAND) {
        if (setx) {
            (void) strcpy(xformat,DEF_FORMAT);
            format_is_numeric[FIRST_X_AXIS] = 1;
        }
        if (sety) {
            (void) strcpy(yformat,DEF_FORMAT);
            format_is_numeric[FIRST_Y_AXIS] = 1;
        }
        if (setz) {
            (void) strcpy(zformat,DEF_FORMAT);
            format_is_numeric[FIRST_Z_AXIS] = 1;
        }
        if (setx2) {
            (void) strcpy(x2format,DEF_FORMAT);
            format_is_numeric[SECOND_X_AXIS] = 1;
        }
        if (sety2) {
            (void) strcpy(y2format,DEF_FORMAT);
            format_is_numeric[SECOND_Y_AXIS] = 1;
        }
    }
}


/* process 'unset grid' command */
static void
unset_grid()
{
    c_token++;
    work_grid.l_type = GRID_OFF;
}


/* process 'unset hidden3d' command */
static void
unset_hidden3d()
{
    c_token++;
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
    c_token++;
    gnuplot_history_size = -1; /* don't ever truncate the history. */
}
#endif


/* process 'unset isosamples' command */
static void
unset_isosamples()
{
    register struct curve_points *f_p = first_plot;
    register struct surface_points *f_3dp = first_3dplot;

    c_token++;

    first_plot = NULL;
    first_3dplot = NULL;
    cp_free(f_p);
    sp_free(f_3dp);

    iso_samples_1 = ISO_SAMPLES;
    iso_samples_2 = ISO_SAMPLES;
}


/* process 'unset key' command */
static void
unset_key()
{
    c_token++;
    key = 0;
}


/* process 'unset keytitle' command */
static void
unset_keytitle()
{
    c_token++;
    *key_title = 0;
}


/* process 'unset label' command */
static void
unset_label()
{
    struct value a;
    struct text_label *this_label;
    struct text_label *prev_label;
    int tag;

    c_token++;

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
	free (this->text);
	free (this->font);
	free (this);
    }
}


/* process 'unset loadpath' command */
static void
unset_loadpath()
{
    c_token++;
    clear_loadpath();
}


/* process 'unset locale' command */
static void
unset_locale()
{
    c_token++;
    init_locale();
}


/* process 'unset logscale' command */
static void
unset_logscale()
{
    c_token++;
    if (END_OF_COMMAND) {
	is_log_x = is_log_y = is_log_z = is_log_x2 = is_log_y2 = FALSE;
    } else if (equals(c_token, "x2")) {
	is_log_x2 = FALSE; ++c_token;
    } else if (equals(c_token, "y2")) {
	is_log_y2 = FALSE; ++c_token;
    } else {
	if (chr_in_str(c_token, 'x')) {
	    is_log_x = FALSE;
	    base_log_x = 0.0;
	    log_base_log_x = 0.0;
	}
	if (chr_in_str(c_token, 'y')) {
	    is_log_y = FALSE;
	    base_log_y = 0.0;
	    log_base_log_y = 0.0;
	}
	if (chr_in_str(c_token, 'z')) {
	    is_log_z = FALSE;
	    base_log_z = 0.0;
	    log_base_log_z = 0.0;
	}
	c_token++;
    }
}


/* process 'unset mapping3d' command */
static void
unset_mapping()
{
    c_token++;
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
    c_token++;
    free(missing_val);
    missing_val = NULL;
}

#ifdef USE_MOUSE
/* process 'unset mouse' command */
static void
unset_mouse()
{
    c_token++;
    mouse_setting.on = 0;
#if defined(USE_MOUSE) && defined(OS2)
    update_menu_items_PM_terminal();
#endif
    UpdateStatusline(); /* wipe status line */
}
#endif

/* process 'unset mxtics' command */
static void
unset_mxtics()
{
    c_token++;
    mxtics = MINI_DEFAULT;
    mxtfreq = 10.0;
}


/* process 'unset mx2tics' command */
static void
unset_mx2tics()
{
    c_token++;
    mx2tics = MINI_DEFAULT;
    mx2tfreq = 10.0;
}


/* process 'unset mytics' command */
static void
unset_mytics()
{
    c_token++;
    mytics = MINI_DEFAULT;
    mytfreq = 10.0;
}


/* process 'unset my2tics' command */
static void
unset_my2tics()
{
    c_token++;
    my2tics = MINI_DEFAULT;
    my2tfreq = 10.0;
}


/* process 'unset mztics' command */
static void
unset_mztics()
{
    c_token++;
    mztics = MINI_DEFAULT;
    mztfreq = 10.0;
}


/*process 'unset xtics' command */
static void
unset_xtics()
{
    c_token++;
    xtics = NO_TICS;
}


/*process 'unset ytics' command */
static void
unset_ytics()
{
    c_token++;
    ytics = NO_TICS;
}


/*process 'unset x2tics' command */
static void
unset_x2tics()
{
    c_token++;
    x2tics = NO_TICS;
}


/*process 'unset y2tics' command */
static void
unset_y2tics()
{
    c_token++;
    y2tics = NO_TICS;
}


/*process 'unset ztics' command */
static void
unset_ztics()
{
    c_token++;
    ztics = NO_TICS;
}


/* process 'unset offsets' command */
static void
unset_offsets()
{
    c_token++;
    loff = roff = toff = boff = 0.0;
}


/* process 'unset origin' command */
static void
unset_origin()
{
    c_token++;

    xoffset = 0.0;
    yoffset = 0.0;
}


/* process 'unset output' command */
static void
unset_output()
{
    c_token++;
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
    c_token++;

    if (parametric) {
	parametric = FALSE;
	if (!polar) { /* keep t for polar */
	    strcpy (dummy_var[0], "x");
	    strcpy (dummy_var[1], "y");
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


/* process 'unset pm3d' command */
static void
unset_pm3d()
{
    c_token++;
    if (pm3d.where[0]=='b' && !pm3d.where[1]) /* unset reversed y axis from 'set pm3d map' */
	range_flags[FIRST_Y_AXIS] &= ~RANGE_REVERSE;
    pm3d.where[0] = 0;
    pm3d_map_rotate_ylabel = 0;  /* trick for rotating ylabel */
#ifdef X11
    if (!strcmp(term->name, "x11")) {
	extern void X11_unset_pm3d __PROTO((void)); /* defined in x11.trm */
	X11_unset_pm3d();
    }
#endif
}
#endif

/* process 'unset pointsize' command */
static void
unset_pointsize()
{
    c_token++;
    pointsize = 1.0;
}


/* process 'unset polar' command */
static void
unset_polar()
{
    c_token++;

    if (polar) {
	polar = FALSE;
	if (parametric && autoscale_t) {
	    /* only if user has not set an explicit range */
	    tmin = -5.0;
	    tmax = 5.0;
	}
	if (!parametric) {
	    strcpy (dummy_var[0], "x");
	    if (interactive)
		(void) fprintf(stderr,"\n\tdummy variable is x for curves\n");
	}
    }
}


/* process 'unset samples' command */
static void
unset_samples()
{
    register struct surface_points *f_3dp = first_3dplot;

    c_token++;

    first_3dplot = NULL;
    sp_free(f_3dp);

    samples = SAMPLES;
    samples_1 = SAMPLES;
    samples_2 = SAMPLES;
}


/* process 'unset size' command */
static void
unset_size()
{
    c_token++;

    xsize = 1.0;
    ysize = 1.0;
    zsize = 1.0;
}


/* process 'unset style' command */
static void
unset_style()
{
    c_token++;

    if (END_OF_COMMAND) {
        data_style = POINTSTYLE;
        func_style = LINES;
	while (first_linestyle != NULL)
	    delete_linestyle((struct linestyle_def *) NULL, first_linestyle);
    } else if (almost_equals(c_token, "d$ata"))
        data_style = POINTSTYLE;
    else if (almost_equals(c_token, "f$unction"))
        func_style = LINES;
    else if (almost_equals(c_token, "l$ine")) {
	while (first_linestyle != NULL)
	    delete_linestyle((struct linestyle_def *) NULL, first_linestyle);
    } else 
        int_error(c_token, "expecting 'data', 'function', or 'line'");

    c_token++;

}


/* process 'unset surface' command */
static void
unset_surface()
{
    c_token++;
    draw_surface = FALSE;
}


/* process 'unset terminal' comamnd */
static void
unset_terminal()
{
    /* This is a problematic case */
    c_token++;
/* FIXME */
    if (multiplot)
	int_error(c_token, "You can't change the terminal in multiplot mode");

    list_terms();
    screen_ok = FALSE;
}


/* process 'unset tics' command */
static void
unset_tics()
{
    c_token++;
    tic_in = TRUE;
}


/* process 'unset ticscale' command */
static void
unset_ticscale()
{
    c_token++;

    ticscale = 1.0;
    miniticscale = 0.5;
}


/* process 'unset ticslevel' command */
static void
unset_ticslevel()
{
    c_token++;
    ticslevel = 0.5;
}


/* Process 'unset timefmt' command */
static void
unset_timefmt()
{
    c_token++;
    strcpy(timefmt,TIMEFMT);
}


/* process 'unset notimestamp' command */
static void
unset_timestamp()
{
    c_token++;

    *timelabel.text = 0;
    timelabel.xoffset = 0;
    timelabel.yoffset = 0;
    *timelabel.font = 0;
    timelabel_rotate = FALSE;
    timelabel_bottom = TRUE;
}


/* process 'unset view' command */
static void
unset_view()
{
    c_token++;
    surface_rot_z = 30.0;
    surface_rot_x = 60.0;
    surface_scale = 1.0;
    surface_zscale = 1.0;
}


/* process 'unset zero' command */
static void
unset_zero()
{
    c_token++;
    zero = ZERO;
}

/* FIXME - merge unset_*data() functions into one */

/* process 'unset xdata' command */
static void
unset_xdata()
{
    c_token++;
    datatype[FIRST_X_AXIS] = FALSE;
    /* eh ? - t and u have nothing to do with x */
    datatype[T_AXIS] = FALSE;
    datatype[U_AXIS] = FALSE;
}


/* process 'unset ydata' command */
static void
unset_ydata()
{
    c_token++;
    datatype[FIRST_Y_AXIS] = FALSE;
    datatype[V_AXIS] = FALSE;
}

#define PROCESS_AXIS_DATA(AXIS) \
    c_token++; \
    datatype[AXIS] = FALSE;

/* process 'unset zdata' command */
static void
unset_zdata()
{
    PROCESS_AXIS_DATA(FIRST_Z_AXIS);
}


/* process 'unset x2data' command */
static void
unset_x2data()
{
    PROCESS_AXIS_DATA(SECOND_X_AXIS);
}


/* process 'unset y2data' command */
static void
unset_y2data()
{
    PROCESS_AXIS_DATA(SECOND_Y_AXIS);
}


/* FIXME - merge set_*range() functions into one */

/* process 'unset xrange' command */
static void
unset_xrange()
{
    range_flags[FIRST_X_AXIS] = 0;
}


/* process 'unset x2range' command */
static void
unset_x2range()
{
    range_flags[SECOND_X_AXIS] = 0;
}


/* process 'unset yrange' command */
static void
unset_yrange()
{
    range_flags[FIRST_Y_AXIS] = 0;
}


/* process 'unset y2range' command */
static void
unset_y2range()
{
    range_flags[SECOND_Y_AXIS] = 0;
}


/* process 'unset zrange' command */
static void
unset_zrange()
{
    range_flags[FIRST_Z_AXIS] = 0;
}


/* process 'unset rrange' command */
static void
unset_rrange()
{
    range_flags[R_AXIS] = 0;
}


/* process 'unset trange' command */
static void
unset_trange()
{
    range_flags[T_AXIS] = 0;
}


/* process 'unset urange' command */
static void
unset_urange()
{
    range_flags[U_AXIS] = 0;
}


/* process 'unset vrange' command */
static void
unset_vrange()
{
    range_flags[V_AXIS] = 0;
}


/* FIXME - merge *zeroaxis() functions into one */

/* process 'unset xzeroaxis' command */
static void
unset_xzeroaxis()
{
    c_token++;
    xzeroaxis.l_type = -3;
}


/* process 'unset yzeroaxis' command */
static void
unset_yzeroaxis()
{
    c_token++;
    yzeroaxis.l_type = -3;
}


/* process 'unset x2zeroaxis' command */
static void
unset_x2zeroaxis()
{
    c_token++;
    x2zeroaxis.l_type = -3;
}


/* process 'unset y2zeroaxis' command */
static void
unset_y2zeroaxis()
{
    c_token++;
    y2zeroaxis.l_type = -3;
}


/* process 'unset zeroaxis' command */
static void
unset_zeroaxis()
{
    c_token++;
    xzeroaxis.l_type  = -3;
    yzeroaxis.l_type  = -3;
    x2zeroaxis.l_type = -3;
    y2zeroaxis.l_type = -3;
}


/* process 'unset [xyz]{2}label command */
static void
unset_xyzlabel(label)
label_struct *label;
{
    c_token++;

    strcpy(label->text, "");
    strcpy(label->font, "");
    label->xoffset = 0;
    label->yoffset = 0;
}
