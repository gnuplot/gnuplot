#ifndef lint
static char *RCSid() { return RCSid("$Id: save.c,v 1.24 2001/06/11 16:47:59 broeker Exp $"); }
#endif

/* GNUPLOT - save.c */

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

#include "save.h"

#include "axis.h"
#include "command.h"
#include "contour.h"
#include "datafile.h"
#include "eval.h"
#include "fit.h"
#include "gp_time.h"
#include "graphics.h"
#include "hidden3d.h"
#include "plot2d.h"
#include "plot3d.h"
#include "setshow.h"
#include "term_api.h"
#include "util.h"
#include "variable.h"
#ifdef PM3D
# include "pm3d.h"
#endif

static void save_functions__sub __PROTO((FILE *));
static void save_variables__sub __PROTO((FILE *));
static void save_tics __PROTO((FILE *, AXIS_INDEX));
static void save_position __PROTO((FILE *, struct position *));
static void save_range __PROTO((FILE *, AXIS_INDEX));
static void save_zeroaxis __PROTO((FILE *,AXIS_INDEX));
static void save_set_all __PROTO((FILE *));

/*
 *  functions corresponding to the arguments of the GNUPLOT `save` command
 */
void
save_functions(fp)
FILE *fp;
{
    if (fp) {
	show_version(fp);	/* I _love_ information written */
	save_functions__sub(fp);	/* at the top and the end of an */
	fputs("#    EOF\n", fp);	/* human readable ASCII file.   */
	if (stdout != fp)
	    (void) fclose(fp);	/*                        (JFi) */
    } else
	os_error(c_token, "Cannot open save file");
}


void
save_variables(fp)
FILE *fp;
{
    if (fp) {
	show_version(fp);
	save_variables__sub(fp);
	fputs("#    EOF\n", fp);
	if (stdout != fp)
	    (void) fclose(fp);	/*                        (JFi) */
    } else
	os_error(c_token, "Cannot open save file");
}


void
save_set(fp)
FILE *fp;
{
    if (fp) {
	show_version(fp);
	save_set_all(fp);
	fputs("#    EOF\n", fp);
	if (stdout != fp)
	    (void) fclose(fp);	/*                        (JFi) */
    } else
	os_error(c_token, "Cannot open save file");
}


void
save_all(fp)
FILE *fp;
{
    if (fp) {
	show_version(fp);
	save_set_all(fp);
	save_functions__sub(fp);
	save_variables__sub(fp);
	fprintf(fp, "%s\n", replot_line);
	if (wri_to_fil_last_fit_cmd(NULL)) {
	    fputs("## ", fp);
	    wri_to_fil_last_fit_cmd(fp);
	    putc('\n', fp);
	}
	fputs("#    EOF\n", fp);
	if (stdout != fp)
	    (void) fclose(fp);	/*                        (JFi) */
    } else
	os_error(c_token, "Cannot open save file");
}

/*
 *  auxiliary functions
 */

static void
save_functions__sub(fp)
FILE *fp;
{
    register struct udft_entry *udf = first_udf;

    while (udf) {
	if (udf->definition) {
	    fprintf(fp, "%s\n", udf->definition);
	}
	udf = udf->next_udf;
    }
}

static void
save_variables__sub(fp)
FILE *fp;
{
    /* always skip pi */
    register struct udvt_entry *udv = first_udv->next_udv;

    while (udv) {
	if (!udv->udv_undef) {
	    fprintf(fp, "%s = ", udv->udv_name);
	    disp_value(fp, &(udv->udv_value));
	    (void) putc('\n', fp);
	}
	udv = udv->next_udv;
    }
}

/* HBB 19990823: new function 'save term'. This will be mainly useful
 * for the typical 'set term post ... plot ... set term <normal term>
 * sequence. It's the only 'save' function that will write the
 * current term setting to a file uncommentedly. */
void
save_term(fp)
FILE *fp;
{
    if (fp) {
	show_version(fp);

	/* A possible gotcha: the default initialization often doesn't set
	 * term_options, but a 'set term <type>' without options doesn't
	 * reset the options to startup defaults. This may have to be
	 * changed on a per-terminal driver basis... */
	if (term)
	    fprintf(fp, "set terminal %s %s\n", term->name, term_options);
	else
	    fputs("set terminal unknown\n", fp);

	/* output will still be written in commented form.  Otherwise, the
	 * risk of overwriting files is just too high */
	if (outstr)
	    fprintf(fp, "# set output '%s'\n", outstr);
	else
	    fputs("# set output\n", fp);
		
	fputs("#    EOF\n", fp);
	if (stdout != fp)
	    (void) fclose(fp);	/*                        (JFi) */
    } else
	os_error(c_token, "Cannot open save file");
}


static void
save_set_all(fp)
FILE *fp;
{
    struct text_label *this_label;
    struct arrow_def *this_arrow;
    struct linestyle_def *this_linestyle;

    /* opinions are split as to whether we save term and outfile
     * as a compromise, we output them as comments !
     */
    if (term)
	fprintf(fp, "# set terminal %s %s\n", term->name, term_options);
    else
	fputs("# set terminal unknown\n", fp);

    if (outstr)
	fprintf(fp, "# set output '%s'\n", outstr);
    else
	fputs("# set output\n", fp);

    fprintf(fp, "\
%sset clip points\n\
%sset clip one\n\
%sset clip two\n\
set bar %f\n",
	    (clip_points) ? "" : "un",
	    (clip_lines1) ? "" : "un",
	    (clip_lines2) ? "" : "un",
	    bar_size);

    if (draw_border)
	/* HBB 980609: handle border linestyle, too */
	fprintf(fp, "set border %d lt %d lw %.3f\n",
		draw_border, border_lp.l_type + 1, border_lp.l_width);
    else
	fputs("unset border\n", fp);

    fprintf(fp, "\
set xdata%s\n\
set ydata%s\n\
set zdata%s\n\
set x2data%s\n\
set y2data%s\n",
	    axis_array[FIRST_X_AXIS].is_timedata ? " time" : "",
	    axis_array[FIRST_Y_AXIS].is_timedata ? " time" : "",
	    axis_array[FIRST_Z_AXIS].is_timedata ? " time" : "",
	    axis_array[SECOND_X_AXIS].is_timedata ? " time" : "",
	    axis_array[SECOND_Y_AXIS].is_timedata ? " time" : "");

    if (boxwidth < 0.0)
	fputs("set boxwidth\n", fp);
    else
	fprintf(fp, "set boxwidth %g\n", boxwidth);
    if (dgrid3d)
	fprintf(fp, "set dgrid3d %d,%d, %d\n",
		dgrid3d_row_fineness,
		dgrid3d_col_fineness,
		dgrid3d_norm_value);

    fprintf(fp, "set dummy %s,%s\n", set_dummy_var[0], set_dummy_var[1]);

#define SAVE_FORMAT(axis)						\
    fprintf(fp, "set format %s \"%s\"\n", axis_defaults[axis].name,	\
	    conv_text(axis_array[axis].formatstring));
    SAVE_FORMAT(FIRST_X_AXIS );
    SAVE_FORMAT(FIRST_Y_AXIS );
    SAVE_FORMAT(SECOND_X_AXIS);
    SAVE_FORMAT(SECOND_Y_AXIS);
    SAVE_FORMAT(FIRST_Z_AXIS );
#undef SAVE_FORMAT

    fprintf(fp, "set angles %s\n",
	    (ang2rad == 1.0) ? "radians" : "degrees");

    if (grid_selection == 0)
	fputs("unset grid\n", fp);
    else {
	/* FIXME */
	if (polar_grid_angle)	/* set angle already output */
	    fprintf(fp, "set grid polar %f\n", polar_grid_angle / ang2rad);
	else
	    fputs("set grid nopolar\n", fp);
	fprintf(fp,
#ifdef PM3D
		"set grid %sxtics %sytics %sztics %sx2tics %sy2tics %smxtics %smytics %smztics %smx2tics %smy2tics %scbtics %smcbtics lt %d lw %.3f, lt %d lw %.3f\n",
#else
		"set grid %sxtics %sytics %sztics %sx2tics %sy2tics %smxtics %smytics %smztics %smx2tics %smy2tics lt %d lw %.3f, lt %d lw %.3f\n",
#endif
		grid_selection & GRID_X ? "" : "no",
		grid_selection & GRID_Y ? "" : "no",
		grid_selection & GRID_Z ? "" : "no",
		grid_selection & GRID_X2 ? "" : "no",
		grid_selection & GRID_Y2 ? "" : "no",
		grid_selection & GRID_MX ? "" : "no",
		grid_selection & GRID_MY ? "" : "no",
		grid_selection & GRID_MZ ? "" : "no",
		grid_selection & GRID_MX2 ? "" : "no",
		grid_selection & GRID_MY2 ? "" : "no",
#ifdef PM3D
		grid_selection & GRID_CB ? "" : "no",
		grid_selection & GRID_MCB ? "" : "no",
#endif
		grid_lp.l_type + 1, grid_lp.l_width,
		mgrid_lp.l_type + 1, mgrid_lp.l_width);
    }
    fprintf(fp, "set key title \"%s\"\n", conv_text(key_title));
    switch (key) {
    case KEY_AUTO_PLACEMENT:
	    fputs("set key", fp);
	    switch (key_hpos) {
	    case TRIGHT:
		fputs(" right", fp);
		break;
	    case TLEFT:
		fputs(" left", fp);
		break;
	    case TOUT:
		fputs(" out", fp);
		break;
	    }
	    switch (key_vpos) {
	    case TTOP:
		fputs(" top", fp);
		break;
	    case TBOTTOM:
		fputs(" bottom", fp);
		break;
	    case TUNDER:
		fputs(" below", fp);
		break;
	    }
	    break;
    case KEY_NONE:
	fputs("unset key\n", fp);
	break;
    case KEY_USER_PLACEMENT:
	fputs("set key ", fp);
	save_position(fp, &key_user_pos);
	break;
    }
    if (key != KEY_NONE) {
	fprintf(fp, " %s %sreverse box linetype %d linewidth %.3f samplen %g spacing %g width %g\n",
		key_just == JLEFT ? "Left" : "Right",
		key_reverse ? "" : "no",
		key_box.l_type + 1, key_box.l_width,
		key_swidth, key_vert_factor, key_width_fix);
    }
    fputs("unset label\n", fp);
    for (this_label = first_label; this_label != NULL;
	 this_label = this_label->next) {
	fprintf(fp, "set label %d \"%s\" at ",
		this_label->tag,
		conv_text(this_label->text));
	save_position(fp, &this_label->place);

	switch (this_label->pos) {
	case LEFT:
	    fputs(" left", fp);
	    break;
	case CENTRE:
	    fputs(" centre", fp);
	    break;
	case RIGHT:
	    fputs(" right", fp);
	    break;
	}
	fprintf(fp, " %srotate", this_label->rotate ? "" : "no");
	if (this_label->font != NULL)
	    fprintf(fp, " font \"%s\"", this_label->font);
	if (-1 == this_label->pointstyle)
	    fprintf(fp, " nopointstyle");
	else {
	    fprintf(fp, " pointstyle %d offset %f,%f",
		this_label->pointstyle, this_label->hoffset, this_label->voffset);
	}
	/* Entry font added by DJL */
	fputc('\n', fp);
    }
    fputs("unset arrow\n", fp);
    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next) {
	fprintf(fp, "set arrow %d from ", this_arrow->tag);
	save_position(fp, &this_arrow->start);
	fputs(this_arrow->relative ? " rto " : " to ", fp);
	save_position(fp, &this_arrow->end);
	fprintf(fp, " %s linetype %d linewidth %.3f\n",
		this_arrow->head ? "" : " nohead",
		this_arrow->lp_properties.l_type + 1,
		this_arrow->lp_properties.l_width);
    }
    fputs("unset style line\n", fp);
    for (this_linestyle = first_linestyle; this_linestyle != NULL;
	 this_linestyle = this_linestyle->next) {
	fprintf(fp, "set style line %d ", this_linestyle->tag);
#ifdef PM3D
	if (this_linestyle->lp_properties.use_palette)
	    fprintf(fp, "linetype palette ");
	else
#endif
	    fprintf(fp, "linetype %d ", this_linestyle->lp_properties.l_type + 1);
	fprintf(fp, "linewidth %.3f pointtype %d pointsize %.3f\n",
		this_linestyle->lp_properties.l_width,
		this_linestyle->lp_properties.p_type + 1,
		this_linestyle->lp_properties.p_size);
    }
    fputs("unset logscale\n", fp);
#define SAVE_LOG(axis)							\
    if (axis_array[axis].log)						\
	fprintf(fp, "set logscale %s %g\n", axis_defaults[axis].name,	\
		axis_array[axis].base);
    SAVE_LOG(FIRST_X_AXIS );
    SAVE_LOG(FIRST_Y_AXIS );
    SAVE_LOG(SECOND_X_AXIS);
    SAVE_LOG(SECOND_Y_AXIS);
    SAVE_LOG(FIRST_Z_AXIS );
#undef SAVE_LOG

    /* FIXME */
    fprintf(fp, "\
set offsets %g, %g, %g, %g\n\
set pointsize %g\n\
set encoding %s\n\
%sset polar\n\
%sset parametric\n",
	    loff, roff, toff, boff,
	    pointsize,
	    encoding_names[encoding],
	    (polar) ? "" : "un",
	    (parametric) ? "" : "un");
    fprintf(fp, "\
set view %g, %g, %g, %g\n\
set samples %d, %d\n\
set isosamples %d, %d\n\
%sset surface\n\
%sset contour",
	    surface_rot_x, surface_rot_z, surface_scale, surface_zscale,
	    samples_1, samples_2,
	    iso_samples_1, iso_samples_2,
	    (draw_surface) ? "" : "un",
	    (draw_contour) ? "" : "un");

    switch (draw_contour) {
    case CONTOUR_NONE:
	fputc('\n', fp);
	break;
    case CONTOUR_BASE:
	fputs(" base\n", fp);
	break;
    case CONTOUR_SRF:
	fputs(" surface\n", fp);
	break;
    case CONTOUR_BOTH:
	fputs(" both\n", fp);
	break;
    }
    if (label_contours)
	fprintf(fp, "set clabel '%s'\n", contour_format);
    else
	fputs("unset clabel\n", fp);

    fputs("set mapping ", fp);
    switch (mapping3d) {
    case MAP3D_SPHERICAL:
	fputs("spherical\n", fp);
	break;
    case MAP3D_CYLINDRICAL:
	fputs("cylindrical\n", fp);
	break;
    case MAP3D_CARTESIAN:
    default:
	fputs("cartesian\n", fp);
	break;
    }

    if (missing_val != NULL)
	fprintf(fp, "set missing %s\n", missing_val);

    save_hidden3doptions(fp);
    fprintf(fp, "set cntrparam order %d\n", contour_order);
    fputs("set cntrparam ", fp);
    switch (contour_kind) {
    case CONTOUR_KIND_LINEAR:
	fputs("linear\n", fp);
	break;
    case CONTOUR_KIND_CUBIC_SPL:
	fputs("cubicspline\n", fp);
	break;
    case CONTOUR_KIND_BSPLINE:
	fputs("bspline\n", fp);
	break;
    }
    fputs("set cntrparam levels ", fp);
    switch (contour_levels_kind) {
    case LEVELS_AUTO:
	fprintf(fp, "auto %d\n", contour_levels);
	break;
    case LEVELS_INCREMENTAL:
	fprintf(fp, "incremental %g,%g,%g\n",
		contour_levels_list[0], contour_levels_list[1],
		contour_levels_list[0] + contour_levels_list[1] * contour_levels);
	break;
    case LEVELS_DISCRETE:
	{
	    int i;
	    fprintf(fp, "discrete %g", contour_levels_list[0]);
	    for (i = 1; i < contour_levels; i++)
		fprintf(fp, ",%g ", contour_levels_list[i]);
	    fputc('\n', fp);
	}
    }
    fprintf(fp, "\
set cntrparam points %d\n\
set size ratio %g %g,%g\n\
set origin %g,%g\n\
set style data ",
	    contour_pts,
	    aspect_ratio, xsize, ysize,
	    xoffset, yoffset);

    switch (data_style) {
    case LINES:
	fputs("lines\n", fp);
	break;
    case POINTSTYLE:
	fputs("points\n", fp);
	break;
    case IMPULSES:
	fputs("impulses\n", fp);
	break;
    case LINESPOINTS:
	fputs("linespoints\n", fp);
	break;
    case DOTS:
	fputs("dots\n", fp);
	break;
    case YERRORLINES:
	fputs("yerrorlines\n", fp);
	break;
    case XERRORLINES:
	fputs("xerrorlines\n", fp);
	break;
    case XYERRORLINES:
	fputs("xyerrorlines\n", fp);
	break;
    case YERRORBARS:
	fputs("yerrorbars\n", fp);
	break;
    case XERRORBARS:
	fputs("xerrorbars\n", fp);
	break;
    case XYERRORBARS:
	fputs("xyerrorbars\n", fp);
	break;
    case BOXES:
	fputs("boxes\n", fp);
	break;
    case BOXERROR:
	fputs("boxerrorbars\n", fp);
	break;
    case BOXXYERROR:
	fputs("boxxyerrorbars\n", fp);
	break;
    case STEPS:
	fputs("steps\n", fp);
	break;			/* JG */
    case FSTEPS:
	fputs("fsteps\n", fp);
	break;			/* HOE */
    case HISTEPS:
	fputs("histeps\n", fp);
	break;			/* CAC */
    case VECTOR:
	fputs("vector\n", fp);
	break;
    case FINANCEBARS:
	fputs("financebars\n", fp);
	break;
    case CANDLESTICKS:
	fputs("candlesticks\n", fp);
	break;
    }

    fputs("set style function ", fp);
    switch (func_style) {
    case LINES:
	fputs("lines\n", fp);
	break;
    case POINTSTYLE:
	fputs("points\n", fp);
	break;
    case IMPULSES:
	fputs("impulses\n", fp);
	break;
    case LINESPOINTS:
	fputs("linespoints\n", fp);
	break;
    case DOTS:
	fputs("dots\n", fp);
	break;
    case YERRORLINES:
	fputs("yerrorlines\n", fp);
	break;
    case XERRORLINES:
	fputs("xerrorlines\n", fp);
	break;
    case XYERRORLINES:
	fputs("xyerrorlines\n", fp);
	break;
    case YERRORBARS:
	fputs("yerrorbars\n", fp);
	break;
    case XERRORBARS:
	fputs("xerrorbars\n", fp);
	break;
    case XYERRORBARS:
	fputs("xyerrorbars\n", fp);
	break;
    case BOXXYERROR:
	fputs("boxxyerrorbars\n", fp);
	break;
    case BOXES:
	fputs("boxes\n", fp);
	break;
    case BOXERROR:
	fputs("boxerrorbars\n", fp);
	break;
    case STEPS:
	fputs("steps\n", fp);
	break;			/* JG */
    case FSTEPS:
	fputs("fsteps\n", fp);
	break;			/* HOE */
    case HISTEPS:
	fputs("histeps\n", fp);
	break;			/* CAC */
    case VECTOR:
	fputs("vector\n", fp);
	break;
    case FINANCEBARS:
	fputs("financebars\n", fp);
	break;
    case CANDLESTICKS:
	fputs("candlesticks\n", fp);
	break;
    default:
	fputs("---error!---\n", fp);
	break;
    }

    save_zeroaxis(fp, FIRST_X_AXIS);
    save_zeroaxis(fp, FIRST_Y_AXIS);
    save_zeroaxis(fp, SECOND_X_AXIS);
    save_zeroaxis(fp, SECOND_Y_AXIS);
    
    fprintf(fp, "\
set tics %s\n\
set ticslevel %g\n\
set ticscale %g %g\n",
	    (tic_in) ? "in" : "out",
	    ticslevel,
	    ticscale, miniticscale);

#define SAVE_MINI(axis)							  \
    switch(axis_array[axis].minitics & TICS_MASK) {				  \
    case 0:								  \
	fprintf(fp, "set nom%stics\n", axis_defaults[axis].name);		  \
	break;								  \
    case MINI_AUTO:							  \
	fprintf(fp, "set m%stics\n", axis_defaults[axis].name);		  \
	break;								  \
    case MINI_DEFAULT:							  \
	fprintf(fp, "set m%stics default\n", axis_defaults[axis].name);	  \
	break;								  \
    case MINI_USER: fprintf(fp, "set m%stics %f\n", axis_defaults[axis].name, \
			    axis_array[axis].mtic_freq);			  \
	break;								  \
    }

    SAVE_MINI(FIRST_X_AXIS);
    SAVE_MINI(FIRST_Y_AXIS);
    SAVE_MINI(FIRST_Z_AXIS);	/* HBB 20000506: noticed mztics were not saved! */
    SAVE_MINI(SECOND_X_AXIS);
    SAVE_MINI(SECOND_Y_AXIS);
#ifdef PM3D
    SAVE_MINI(COLOR_AXIS);
#endif
#undef SAVE_MINI

    save_tics(fp, FIRST_X_AXIS);
    save_tics(fp, FIRST_Y_AXIS);
    save_tics(fp, FIRST_Z_AXIS);
    save_tics(fp, SECOND_X_AXIS);
    save_tics(fp, SECOND_Y_AXIS);
#ifdef PM3D
    save_tics(fp, COLOR_AXIS);
#endif

#define SAVE_AXISLABEL_OR_TITLE(name,suffix,lab)	\
    {							\
	fprintf(fp, "set %s%s \"%s\" %f,%f ",		\
		name, suffix, conv_text(lab.text),	\
		lab.xoffset, lab.yoffset);		\
  fprintf(fp, " \"%s\"\n", conv_text(lab.font)); \
}

    SAVE_AXISLABEL_OR_TITLE("", "title", title);

    /* FIXME */
    fprintf(fp, "set %s \"%s\" %s %srotate %f,%f ",
	    "timestamp", conv_text(timelabel.text),
	    (timelabel_bottom ? "bottom" : "top"),
	    (timelabel_rotate ? "" : "no"),
	    timelabel.xoffset, timelabel.yoffset);
    fprintf(fp, " \"%s\"\n", conv_text(timelabel.font));

    save_range(fp, R_AXIS);
    save_range(fp, T_AXIS);
    save_range(fp, U_AXIS);
    save_range(fp, V_AXIS);

#define SAVE_TIMEFMT(axis)						\
    if (strlen(axis_array[axis].timefmt)) 						\
	fprintf(fp, "set timefmt %s \"%s\"\n", axis_defaults[axis].name,	\
		conv_text(axis_array[axis].timefmt));
    SAVE_TIMEFMT(FIRST_X_AXIS);
    SAVE_TIMEFMT(FIRST_Y_AXIS);
    SAVE_TIMEFMT(FIRST_Z_AXIS);
    SAVE_TIMEFMT(SECOND_X_AXIS);
    SAVE_TIMEFMT(SECOND_Y_AXIS);
#ifdef PM3D
    SAVE_TIMEFMT(COLOR_AXIS);
#endif
#undef SAVE_TIMEFMT
    
#define SAVE_AXISLABEL(axis)					\
    SAVE_AXISLABEL_OR_TITLE(axis_defaults[axis].name,"label",	\
			    axis_array[axis].label)
	
    SAVE_AXISLABEL(FIRST_X_AXIS);
    SAVE_AXISLABEL(SECOND_X_AXIS);
    save_range(fp, FIRST_X_AXIS);
    save_range(fp, SECOND_X_AXIS);

    SAVE_AXISLABEL(FIRST_Y_AXIS);
    SAVE_AXISLABEL(SECOND_Y_AXIS);
    save_range(fp, FIRST_Y_AXIS);
    save_range(fp, SECOND_Y_AXIS);

    SAVE_AXISLABEL(FIRST_Z_AXIS);
    save_range(fp, FIRST_Z_AXIS);

#ifdef PM3D
    SAVE_AXISLABEL(COLOR_AXIS);
    save_range(fp, COLOR_AXIS);
#endif
#undef SAVE_AXISLABEL
#undef SAVE_AXISLABEL_OR_TITLE

    fprintf(fp, "set zero %g\n", zero);
    fprintf(fp, "set lmargin %d\nset bmargin %d\n"
	    "set rmargin %d\nset tmargin %d\n",
	    lmargin, bmargin, rmargin, tmargin);

    fprintf(fp, "set locale \"%s\"\n", get_locale());

#ifdef PM3D
    if (pm3d.map) fputs("set pm3d map\n", fp);
	else if (pm3d.where[0]) fprintf(fp, "set pm3d at %s\n", pm3d.where);
    fputs("set pm3d ", fp);
    switch (pm3d.direction) {
    case PM3D_SCANS_AUTOMATIC: fputs("scansautomatic", fp); break;
    case PM3D_SCANS_FORWARD: fputs("scansforward", fp); break;
    case PM3D_SCANS_BACKWARD: fputs("scansbackward", fp); break;
    }
    fputs(" flush ", fp);
    switch (pm3d.flush) {
    case PM3D_FLUSH_CENTER: fputs("center", fp); break;
    case PM3D_FLUSH_BEGIN: fputs("begin", fp); break;
    case PM3D_FLUSH_END: fputs("end", fp); break;
    }
    if (pm3d.hidden3d_tag) fprintf(fp," hidden3d %d", pm3d.hidden3d_tag);
	else fputs(" nohidden3d", fp);
    fputs( (pm3d.solid ? " solid" : " transparent"), fp);
    if (!pm3d.where[0]) fputs("\nunset pm3d", fp);
    fputs("\n", fp);

    fprintf(fp, "set palette %s rgbformulae %d,%d,%d %stive %sps_allcF maxcolors %d\n",
	sm_palette.colorMode == SMPAL_COLOR_MODE_GRAY ? "gray" : "color",
	sm_palette.formulaR, sm_palette.formulaG, sm_palette.formulaB,
	sm_palette.positive ? "posi" : "nega",
	sm_palette.ps_allcF ? "" : "no",
	sm_palette.use_maxcolors);

    if (color_box.where != SMCOLOR_BOX_NO)
	fprintf(fp,"set colorbox %s\n", color_box.where==SMCOLOR_BOX_DEFAULT ? "default" : "user");
    fprintf(fp, "set colorbox %sal origin %g,%g size %g,%g ",
	color_box.rotation ==  'v' ? "vertic" : "horizont",
	color_box.xorigin, color_box.yorigin,
	color_box.xsize, color_box.ysize);
    if (color_box.border == 0) fputs("noborder", fp);
	else if (color_box.border_lt_tag < 0) fputs("bdefault", fp);
		 else fprintf(fp, "border %d", color_box.border_lt_tag);
    if (color_box.where == SMCOLOR_BOX_NO) fputs("\nunset colorbox\n", fp);
	else fputs("\n", fp);
#endif

    fputs("set loadpath ", fp);
    {
	char *s;
	while ((s = save_loadpath()) != NULL)
	    fprintf(fp, "\"%s\" ", s);
	fputc('\n', fp);
    }
}

static void
save_tics(fp, axis)
    FILE *fp;
    AXIS_INDEX axis;
{
    if (axis_array[axis].ticmode == NO_TICS) {
	fprintf(fp, "set no%stics\n", axis_defaults[axis].name);
	return;
    }
    fprintf(fp, "set %stics %s %smirror %srotate ", axis_defaults[axis].name,
	    ((axis_array[axis].ticmode & TICS_MASK) == TICS_ON_AXIS)
	    ? "axis" : "border",
	    (axis_array[axis].ticmode & TICS_MIRROR) ? "" : "no",
	    axis_array[axis].tic_rotate ? "" : "no");
    switch (axis_array[axis].ticdef.type) {
    case TIC_COMPUTED:{
	    fputs("autofreq ", fp);
	    break;
	}
    case TIC_MONTH:{
	    fprintf(fp, "\nset %smtics", axis_defaults[axis].name);
	    break;
	}
    case TIC_DAY:{
	    fprintf(fp, "\nset %sdtics", axis_defaults[axis].name);
	    break;
	}
    case TIC_SERIES:
	if (axis_array[axis].ticdef.def.series.start != -VERYLARGE) 
	    SAVE_NUM_OR_TIME(fp,
			     (double) axis_array[axis].ticdef.def.series.start,
			     axis);
	fprintf(fp, ",%g,", axis_array[axis].ticdef.def.series.incr);
	if (axis_array[axis].ticdef.def.series.end != VERYLARGE) 
	    SAVE_NUM_OR_TIME(fp,
			     (double) axis_array[axis].ticdef.def.series.end,
			     axis);
	break;

    case TIC_USER:{
	    register struct ticmark *t;
	    fputs(" (", fp);
	    for (t = axis_array[axis].ticdef.def.user;
		 t != NULL; t = t->next) {
		if (t->label)
		    fprintf(fp, "\"%s\" ", conv_text(t->label));
		SAVE_NUM_OR_TIME(fp, (double) t->position, axis);
		if (t->next) {
		    fputs(", ", fp);
		}
	    }
	    fputs(")", fp);
	    break;
	}
    }
    putc('\n', fp);
}

static void
save_position(fp, pos)
    FILE *fp;
    struct position *pos;
{
    static const char *msg[] = { "first ", "second ", "graph ", "screen " };

    assert(first_axes == 0 && second_axes == 1 && graph == 2 && screen == 3);

    fprintf(fp, "%s%g, %s%g, %s%g",
	    pos->scalex == first_axes ? "" : msg[pos->scalex], pos->x,
	    pos->scaley == pos->scalex ? "" : msg[pos->scaley], pos->y,
	    pos->scalez == pos->scaley ? "" : msg[pos->scalez], pos->z);
}


static void
save_range(fp, axis)
    FILE *fp;
    AXIS_INDEX axis;
{
#if 0 /* unused ?*/
    int i;

    i = axis;
#endif
    
    fprintf(fp, "set %srange [ ", axis_defaults[axis].name);
    if (axis_array[axis].set_autoscale & AUTOSCALE_MIN) {
	putc('*', fp);
    } else {
	SAVE_NUM_OR_TIME(fp, axis_array[axis].set_min, axis);
    }
    fputs(" : ", fp);
    if (axis_array[axis].set_autoscale & AUTOSCALE_MAX) {
	putc('*', fp);
    } else {
	SAVE_NUM_OR_TIME(fp, axis_array[axis].set_max, axis);
    }

    fprintf(fp, " ] %sreverse %swriteback",
	    axis_array[axis].range_flags & RANGE_REVERSE ? "" : "no",
	    axis_array[axis].range_flags & RANGE_WRITEBACK ? "" : "no");

    if (axis_array[axis].set_autoscale) {
	/* add current (hidden) range as comments */
	fputs("  # (currently [", fp);
	if (axis_array[axis].set_autoscale & AUTOSCALE_MIN) {
	    SAVE_NUM_OR_TIME(fp, axis_array[axis].set_min, axis);
	}
	putc(':', fp);
	if (axis_array[axis].set_autoscale & AUTOSCALE_MAX) {
	    SAVE_NUM_OR_TIME(fp, axis_array[axis].set_max, axis);
	}
	fputs("] )\n", fp);

	if (axis_array[axis].set_autoscale & (AUTOSCALE_FIXMIN)) 
	    fprintf(fp, "set autoscale %sfixmin\n", axis_defaults[axis].name);
	if (axis_array[axis].set_autoscale & AUTOSCALE_FIXMAX)
	    fprintf(fp, "set autoscale %sfixmax\n", axis_defaults[axis].name);
    } else
	putc('\n', fp);
}

static void
save_zeroaxis(fp, axis)
    FILE *fp;
    AXIS_INDEX axis;
{
    fprintf(fp, "set %szeroaxis lt %d lw %.3f\n", axis_defaults[axis].name,
	    axis_array[axis].zeroaxis.l_type + 1,
	    axis_array[axis].zeroaxis.l_width);

}
