#ifndef lint
static char *RCSid() { return RCSid("$Id: save.c,v 1.132.2.2 2006/11/12 23:53:37 sfeam Exp $"); }
#endif

/* GNUPLOT - save.c */

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

#include "save.h"

#include "command.h"
#include "contour.h"
#include "datafile.h"
#include "eval.h"
#include "fit.h"
#include "gp_time.h"
#include "graphics.h"
#include "hidden3d.h"
#include "misc.h"
#include "plot2d.h"
#include "plot3d.h"
#include "setshow.h"
#include "term_api.h"
#include "util.h"
#include "variable.h"
#include "pm3d.h"
#include "getcolor.h"

static void save_functions__sub __PROTO((FILE *));
static void save_variables__sub __PROTO((FILE *));
static void save_tics __PROTO((FILE *, AXIS_INDEX));
static void save_position __PROTO((FILE *, struct position *, TBOOLEAN offset));
static void save_zeroaxis __PROTO((FILE *,AXIS_INDEX));
static void save_set_all __PROTO((FILE *));

/*
 *  functions corresponding to the arguments of the GNUPLOT `save` command
 */
void
save_functions(FILE *fp)
{
    /* I _love_ information written at the top and the end
     * of a human readable ASCII file. */
    show_version(fp);
    save_functions__sub(fp);
    fputs("#    EOF\n", fp);
}


void
save_variables(FILE *fp)
{
	show_version(fp);
	save_variables__sub(fp);
	fputs("#    EOF\n", fp);
}


void
save_set(FILE *fp)
{
	show_version(fp);
	save_set_all(fp);
	fputs("#    EOF\n", fp);
}


void
save_all(FILE *fp)
{
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
}

/*
 *  auxiliary functions
 */

static void
save_functions__sub(FILE *fp)
{
    struct udft_entry *udf = first_udf;

    while (udf) {
	if (udf->definition) {
	    fprintf(fp, "%s\n", udf->definition);
	}
	udf = udf->next_udf;
    }
}

static void
save_variables__sub(FILE *fp)
{
    /* always skip pi */
    struct udvt_entry *udv = first_udv->next_udv;

    while (udv) {
	if (!udv->udv_undef) {
	    if (strncmp(udv->udv_name,"GPVAL_",6) 
	     && strncmp(udv->udv_name,"MOUSE_",6)
	     && strncmp(udv->udv_name,"NaN",4)) {
		fprintf(fp, "%s = ", udv->udv_name);
		disp_value(fp, &(udv->udv_value), TRUE);
		(void) putc('\n', fp);
	    }
	}
	udv = udv->next_udv;
    }
}

/* HBB 19990823: new function 'save term'. This will be mainly useful
 * for the typical 'set term post ... plot ... set term <normal term>
 * sequence. It's the only 'save' function that will write the
 * current term setting to a file uncommentedly. */
void
save_term(FILE *fp)
{
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
}


static void
save_set_all(FILE *fp)
{
    struct text_label *this_label;
    struct arrow_def *this_arrow;
    struct linestyle_def *this_linestyle;
    struct arrowstyle_def *this_arrowstyle;
    legend_key *key = &keyT;

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

    if (draw_border) {
	fprintf(fp, "set border %d %s", draw_border, border_layer == 0 ? "back" : "front");
	save_linetype(fp, &border_lp, FALSE);
	fprintf(fp, "\n");
    } else
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

#define SAVE_TIMEFMT(axis)						\
    if (strlen(axis_array[axis].timefmt)) 				\
	fprintf(fp, "set timefmt %s \"%s\"\n", axis_defaults[axis].name,\
		conv_text(axis_array[axis].timefmt));
    SAVE_TIMEFMT(FIRST_X_AXIS);
    SAVE_TIMEFMT(FIRST_Y_AXIS);
    SAVE_TIMEFMT(FIRST_Z_AXIS);
    SAVE_TIMEFMT(SECOND_X_AXIS);
    SAVE_TIMEFMT(SECOND_Y_AXIS);
    SAVE_TIMEFMT(COLOR_AXIS);
#undef SAVE_TIMEFMT

    if (boxwidth < 0.0)
	fputs("set boxwidth\n", fp);
    else
	fprintf(fp, "set boxwidth %g %s\n", boxwidth,
		(boxwidth_is_absolute) ? "absolute" : "relative");

    fprintf(fp, "set style fill ");
    save_fillstyle(fp, &default_fillstyle);

#ifdef EAM_OBJECTS
    /* Default rectangle style */
    fprintf(fp, "set style rectangle %s fc ",
	    default_rectangle.layer > 0 ? "front" : 
	    default_rectangle.layer < 0 ? "behind" : "back");
    if (default_rectangle.lp_properties.use_palette)
	save_pm3dcolor(fp, &default_rectangle.lp_properties.pm3d_color);
    else
	fprintf(fp, "lt %d",default_rectangle.lp_properties.l_type+1);
    fprintf(fp, " fillstyle ");
    save_fillstyle(fp, &default_rectangle.fillstyle);
#endif

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
    SAVE_FORMAT(COLOR_AXIS );
#undef SAVE_FORMAT

    fprintf(fp, "set angles %s\n",
	    (ang2rad == 1.0) ? "radians" : "degrees");

    if (! some_grid_selected())
	fputs("unset grid\n", fp);
    else {
	if (polar_grid_angle) 	/* set angle already output */
	    fprintf(fp, "set grid polar %f\n", polar_grid_angle / ang2rad);
        else
	    fputs("set grid nopolar\n", fp);

#define SAVE_GRID(axis)					\
	fprintf(fp, " %s%stics %sm%stics",		\
		axis_array[axis].gridmajor ? "" : "no",	\
		axis_defaults[axis].name,		\
		axis_array[axis].gridminor ? "" : "no",	\
		axis_defaults[axis].name);
	fputs("set grid", fp);
	SAVE_GRID(FIRST_X_AXIS);
	SAVE_GRID(FIRST_Y_AXIS);
	SAVE_GRID(FIRST_Z_AXIS);
	fputs(" \\\n", fp);
	SAVE_GRID(SECOND_X_AXIS);
	SAVE_GRID(SECOND_Y_AXIS);
	SAVE_GRID(COLOR_AXIS);
	fputs("\n", fp);
#undef SAVE_GRID

	fprintf(fp, "set grid %s  ", (grid_layer==-1) ? "layerdefault" : ((grid_layer==0) ? "back" : "front"));
	save_linetype(fp, &grid_lp, FALSE);
	fprintf(fp, ", ");
	save_linetype(fp, &mgrid_lp, FALSE);
	fputc('\n', fp);
    }

    fprintf(fp, "set key title \"%s\"\n", conv_text(key->title));
    if (!(key->visible))
	fputs("unset key\n", fp);
    else {
	fputs("set key ", fp);
	switch (key->region) {
	case GPKEY_AUTO_INTERIOR_LRTBC:
	    fputs("inside", fp);
	    break;
	case GPKEY_AUTO_EXTERIOR_LRTBC:
	    fputs("outside", fp);
	    break;
	case GPKEY_AUTO_EXTERIOR_MARGIN:
	    switch (key->margin) {
	    case GPKEY_TMARGIN:
		fputs("tmargin", fp);
		break;
	    case GPKEY_BMARGIN:
		fputs("bmargin", fp);
		break;
	    case GPKEY_LMARGIN:
		fputs("lmargin", fp);
		break;
	    case GPKEY_RMARGIN:
		fputs("rmargin", fp);
		break;
	    }
	    break;
	case GPKEY_USER_PLACEMENT:
	    save_position(fp, &key->user_pos, FALSE);
	    break;
	}
	if (!(key->region == GPKEY_AUTO_EXTERIOR_MARGIN
	      && (key->margin == GPKEY_LMARGIN || key->margin == GPKEY_RMARGIN))) {
	    switch (key->hpos) {
	    case RIGHT:
		fputs(" right", fp);
		break;
	    case LEFT:
		fputs(" left", fp);
		break;
	    case CENTRE:
		fputs(" center", fp);
		break;
	    }
	}
	if (!(key->region == GPKEY_AUTO_EXTERIOR_MARGIN
	      && (key->margin == GPKEY_TMARGIN || key->margin == GPKEY_BMARGIN))) {
	    switch (key->vpos) {
	    case JUST_TOP:
		fputs(" top", fp);
		break;
	    case JUST_BOT:
		fputs(" bottom", fp);
		break;
	    case JUST_CENTRE:
		fputs(" center", fp);
		break;
	    }
	}
	fprintf(fp, " %s %s %sreverse %senhanced %s ",
		key->stack_dir == GPKEY_VERTICAL ? "vertical" : "horizontal",
		key->just == GPKEY_LEFT ? "Left" : "Right",
		key->reverse ? "" : "no",
		key->enhanced ? "" : "no",
		key->auto_titles == COLUMNHEAD_KEYTITLES ? "autotitles columnhead"
		: key->auto_titles == FILENAME_KEYTITLES ? "autotitles"
		: "noautotitles" );
	if (key->box.l_type > LT_NODRAW) {
	    fputs("box", fp);
	    save_linetype(fp, &(key->box), FALSE);
	} else
	    fputs("nobox", fp);
	/* Put less common options on a separate line*/
	fprintf(fp, "\nset key %sinvert samplen %g spacing %g width %g height %g ",
		key->invert ? "" : "no",
		key->swidth, key->vert_factor, key->width_fix, key->height_fix);
	fputc('\n', fp);
    }

    fputs("unset label\n", fp);
    for (this_label = first_label; this_label != NULL;
	 this_label = this_label->next) {
	fprintf(fp, "set label %d \"%s\" at ",
		this_label->tag,
		conv_text(this_label->text));
	save_position(fp, &this_label->place, FALSE);

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
	if (this_label->rotate)
	    fprintf(fp, " rotate by %d", this_label->rotate);
	else
	    fprintf(fp, " norotate");
	if (this_label->font != NULL)
	    fprintf(fp, " font \"%s\"", this_label->font);
	fprintf(fp, " %s", (this_label->layer==0) ? "back" : "front");
	if (this_label->noenhanced)
	    fprintf(fp, " noenhanced");
	save_textcolor(fp, &(this_label->textcolor));
	if (this_label->lp_properties.pointflag == 0)
	    fprintf(fp, " nopoint");
	else {
	    fprintf(fp, " point");
	    save_linetype(fp, &(this_label->lp_properties), TRUE);
	}
	save_position(fp, &this_label->offset, TRUE);
	fputc('\n', fp);
    }
    fputs("unset arrow\n", fp);
    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next) {
	fprintf(fp, "set arrow %d from ", this_arrow->tag);
	save_position(fp, &this_arrow->start, FALSE);
	fputs(this_arrow->relative ? " rto " : " to ", fp);
	save_position(fp, &this_arrow->end, FALSE);
	fprintf(fp, " %s %s %s",
		arrow_head_names[this_arrow->arrow_properties.head],
		(this_arrow->arrow_properties.layer==0) ? "back" : "front",
		( (this_arrow->arrow_properties.head_filled==2) ? "filled" :
		  ( (this_arrow->arrow_properties.head_filled==1) ? "empty" :
		    "nofilled" )) );
	save_linetype(fp, &(this_arrow->arrow_properties.lp_properties), FALSE);
	if (this_arrow->arrow_properties.head_length > 0) {
	    static char *msg[] = {"first", "second", "graph", "screen",
				  "character"};
	    fprintf(fp, " size %s %.3f,%.3f,%.3f",
		    msg[this_arrow->arrow_properties.head_lengthunit],
		    this_arrow->arrow_properties.head_length,
		    this_arrow->arrow_properties.head_angle,
		    this_arrow->arrow_properties.head_backangle);
	}
	fprintf(fp, "\n");
    }
    fprintf(fp, "set style increment %s\n", prefer_line_styles ? "userstyles" : "default");
    fputs("unset style line\n", fp);
    for (this_linestyle = first_linestyle; this_linestyle != NULL;
	 this_linestyle = this_linestyle->next) {
	fprintf(fp, "set style line %d ", this_linestyle->tag);
	save_linetype(fp, &(this_linestyle->lp_properties), TRUE);
	fprintf(fp, "\n");
    }
    fputs("unset style arrow\n", fp);
    for (this_arrowstyle = first_arrowstyle; this_arrowstyle != NULL;
	 this_arrowstyle = this_arrowstyle->next) {
	fprintf(fp, "set style arrow %d", this_arrowstyle->tag);
	fprintf(fp, " %s %s %s",
		arrow_head_names[this_arrowstyle->arrow_properties.head],
		(this_arrowstyle->arrow_properties.layer==0)?"back":"front",
		( (this_arrowstyle->arrow_properties.head_filled==2)?"filled":
		  ( (this_arrowstyle->arrow_properties.head_filled==1)?"empty":
		    "nofilled" )) );
	save_linetype(fp, &(this_arrowstyle->arrow_properties.lp_properties), FALSE);
	if (this_arrowstyle->arrow_properties.head_length > 0) {
	    static char *msg[] = {"first", "second", "graph", "screen",
				  "character"};
	    fprintf(fp, " size %s %.3f,%.3f,%.3f",
		    msg[this_arrowstyle->arrow_properties.head_lengthunit],
		    this_arrowstyle->arrow_properties.head_length,
		    this_arrowstyle->arrow_properties.head_angle,
		    this_arrowstyle->arrow_properties.head_backangle);
	}
	fprintf(fp, "\n");
    }

#ifdef EAM_HISTOGRAMS
    fprintf(fp, "set style histogram ");
    switch (histogram_opts.type) {
	default:
	case HT_CLUSTERED:
	    fprintf(fp,"clustered gap %d ",histogram_opts.gap); break;
	case HT_ERRORBARS:
	    fprintf(fp,"errorbars gap %d lw %g",histogram_opts.gap,histogram_opts.bar_lw); break;
	case HT_STACKED_IN_LAYERS:
	    fprintf(fp,"rowstacked "); break;
	case HT_STACKED_IN_TOWERS:
	    fprintf(fp,"columnstacked "); break;
    }
    fprintf(fp,"title ");
    save_position(fp, &histogram_opts.title.offset, TRUE);
    fprintf(fp, "\n");
#endif

#ifdef EAM_OBJECTS
    save_rectangle(fp, 0);
#endif

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
    SAVE_LOG(COLOR_AXIS );
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
    if (decimalsign != NULL) {
#ifdef HAVE_LOCALE_H
	fprintf(fp, "set decimalsign locale \"%s\"\n", setlocale(LC_NUMERIC,NULL));
#endif
	fprintf(fp, "set decimalsign '%s'\n", decimalsign);
    } else
        fprintf(fp, "unset decimalsign\n");

    fputs("set view ", fp);
    if (splot_map == TRUE)
	fputs("map", fp);
    else
	fprintf(fp, "%g, %g, %g, %g",
	    surface_rot_x, surface_rot_z, surface_scale, surface_zscale);

    fprintf(fp, "\n\
set samples %d, %d\n\
set isosamples %d, %d\n\
%sset surface\n\
%sset contour",
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

#ifdef GP_MACROS
    if (expand_macros)
	fputs("set macros\n", fp);
#endif

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
	fprintf(fp, "set datafile missing '%s'\n", missing_val);
    if (df_separator != '\0')
	fprintf(fp, "set datafile separator \"%c\"\n",df_separator);
    else
	fprintf(fp, "set datafile separator whitespace\n");
    if (strcmp(df_commentschars, DEFAULT_COMMENTS_CHARS))
	fprintf(fp, "set datafile commentschars '%s'\n", df_commentschars);
    if (df_fortran_constants)
	fprintf(fp, "set datafile fortran\n");

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
set origin %g,%g\n",
	    contour_pts,
	    aspect_ratio, xsize, ysize,
	    xoffset, yoffset);

    fprintf(fp, "set style data ");
    save_data_func_style(fp,"data",data_style);
    fprintf(fp, "set style function ");
    save_data_func_style(fp,"function",func_style);

    save_zeroaxis(fp, FIRST_X_AXIS);
    save_zeroaxis(fp, FIRST_Y_AXIS);
    save_zeroaxis(fp, FIRST_Z_AXIS);
    save_zeroaxis(fp, SECOND_X_AXIS);
    save_zeroaxis(fp, SECOND_Y_AXIS);

    if (xyplane.absolute)
	fprintf(fp, "set xyplane at %g\n", xyplane.xyplane_z);
    else
	fprintf(fp, "set ticslevel %g\n", xyplane.ticslevel);

#define SAVE_MINI(axis)							\
    switch(axis_array[axis].minitics & TICS_MASK) {			\
    case 0:								\
	fprintf(fp, "set nom%stics\n", axis_defaults[axis].name);	\
	break;								\
    case MINI_AUTO:							\
	fprintf(fp, "set m%stics\n", axis_defaults[axis].name);		\
	break;								\
    case MINI_DEFAULT:							\
	fprintf(fp, "set m%stics default\n", axis_defaults[axis].name);	\
	break;								\
    case MINI_USER:							\
	fprintf(fp, "set m%stics %f\n", axis_defaults[axis].name,	\
		axis_array[axis].mtic_freq);				\
	break;								\
    }

    SAVE_MINI(FIRST_X_AXIS);
    SAVE_MINI(FIRST_Y_AXIS);
    SAVE_MINI(FIRST_Z_AXIS);	/* HBB 20000506: noticed mztics were not saved! */
    SAVE_MINI(SECOND_X_AXIS);
    SAVE_MINI(SECOND_Y_AXIS);
    SAVE_MINI(COLOR_AXIS);
#undef SAVE_MINI

    save_tics(fp, FIRST_X_AXIS);
    save_tics(fp, FIRST_Y_AXIS);
    save_tics(fp, FIRST_Z_AXIS);
    save_tics(fp, SECOND_X_AXIS);
    save_tics(fp, SECOND_Y_AXIS);
    save_tics(fp, COLOR_AXIS);

#define SAVE_AXISLABEL_OR_TITLE(name,suffix,lab)			 \
    {									 \
	fprintf(fp, "set %s%s \"%s\" ",					 \
		name, suffix, lab.text ? conv_text(lab.text) : "");	 \
	fprintf(fp, "\nset %s%s ", name, suffix);			 \
        save_position(fp, &(lab.offset), TRUE);				 \
	fprintf(fp, " font \"%s\"", lab.font ? conv_text(lab.font) : "");\
	save_textcolor(fp, &(lab.textcolor));				 \
	if (lab.rotate)							 \
	    fprintf(fp, " rotate by %d", lab.rotate);			 \
	else								 \
	    fprintf(fp, " norotate");					 \
	fprintf(fp, "%s\n", (lab.noenhanced) ? " noenhanced" : "");	 \
    }

    SAVE_AXISLABEL_OR_TITLE("", "title", title);

    /* FIXME */
    fprintf(fp, "set timestamp %s \n", timelabel_bottom ? "bottom" : "top");
    SAVE_AXISLABEL_OR_TITLE("", "timestamp", timelabel);

    save_range(fp, R_AXIS);
    save_range(fp, T_AXIS);
    save_range(fp, U_AXIS);
    save_range(fp, V_AXIS);

#define SAVE_AXISLABEL(axis)					\
    SAVE_AXISLABEL_OR_TITLE(axis_defaults[axis].name,"label",	\
			    axis_array[axis].label)

    SAVE_AXISLABEL(FIRST_X_AXIS);
    SAVE_AXISLABEL(SECOND_X_AXIS);
    save_range(fp, FIRST_X_AXIS);
    save_range(fp, SECOND_X_AXIS);

    SAVE_AXISLABEL(FIRST_Y_AXIS);
    SAVE_AXISLABEL(SECOND_Y_AXIS);
    if (splot_map == FALSE) {
	save_range(fp, FIRST_Y_AXIS);
	save_range(fp, SECOND_Y_AXIS);
    } else { /* 'set view map' uses flipped y-axes */
	splot_map_deactivate();
	save_range(fp, FIRST_Y_AXIS);
	save_range(fp, SECOND_Y_AXIS);
	splot_map_activate();
    }

    SAVE_AXISLABEL(FIRST_Z_AXIS);
    save_range(fp, FIRST_Z_AXIS);

    SAVE_AXISLABEL(COLOR_AXIS);
    save_range(fp, COLOR_AXIS);
#undef SAVE_AXISLABEL
#undef SAVE_AXISLABEL_OR_TITLE

    fprintf(fp, "set zero %g\n", zero);
    fprintf(fp, "set lmargin %g\nset bmargin %g\n"
	    "set rmargin %g\nset tmargin %g\n",
	    lmargin, bmargin, rmargin, tmargin);

    fprintf(fp, "set locale \"%s\"\n", get_locale());

    fputs("set pm3d ", fp);
    fputs((PM3D_IMPLICIT == pm3d.implicit ? "implicit" : "explicit"), fp);
    fprintf(fp, " at %s\n", pm3d.where);
    fputs("set pm3d ", fp);
    switch (pm3d.direction) {
    case PM3D_SCANS_AUTOMATIC: fputs("scansautomatic\n", fp); break;
    case PM3D_SCANS_FORWARD: fputs("scansforward\n", fp); break;
    case PM3D_SCANS_BACKWARD: fputs("scansbackward\n", fp); break;
    case PM3D_DEPTH: fputs("depthorder\n", fp); break;
    }
    fprintf(fp, "set pm3d interpolate %d,%d", pm3d.interp_i, pm3d.interp_j);
    fputs(" flush ", fp);
    switch (pm3d.flush) {
    case PM3D_FLUSH_CENTER: fputs("center", fp); break;
    case PM3D_FLUSH_BEGIN: fputs("begin", fp); break;
    case PM3D_FLUSH_END: fputs("end", fp); break;
    }
    fputs((pm3d.ftriangles ? " " : " no"), fp);
    fputs("ftriangles", fp);
    if (pm3d.hidden3d_tag) fprintf(fp," hidden3d %d", pm3d.hidden3d_tag);
	else fputs(" nohidden3d", fp);
#if PM3D_HAVE_SOLID
    fputs((pm3d.solid ? " solid" : " transparent"), fp);
#endif
    fputs(" corners2color ", fp);
    switch (pm3d.which_corner_color) {
	case PM3D_WHICHCORNER_MEAN:    fputs("mean", fp); break;
	case PM3D_WHICHCORNER_GEOMEAN: fputs("geomean", fp); break;
	case PM3D_WHICHCORNER_MEDIAN:  fputs("median", fp); break;
	case PM3D_WHICHCORNER_MIN:     fputs("min", fp); break;
	case PM3D_WHICHCORNER_MAX:     fputs("max", fp); break;
	default: /* PM3D_WHICHCORNER_C1 ... _C4 */
	     fprintf(fp, "c%i", pm3d.which_corner_color - PM3D_WHICHCORNER_C1 + 1);
    }
    fputs("\n", fp);

    /*
     *  Save palette information
     */
    fprintf( fp, "set palette %s %s maxcolors %d ",
	     sm_palette.positive ? "positive" : "negative",
	     sm_palette.ps_allcF ? "ps_allcF" : "nops_allcF",
	sm_palette.use_maxcolors);
    fprintf( fp, "gamma %g ", sm_palette.gamma );
    if (sm_palette.colorMode == SMPAL_COLOR_MODE_GRAY) {
      fputs( "gray\n", fp );
    }
    else {
      fputs( "color model ", fp );
      switch( sm_palette.cmodel ) {
        case C_MODEL_RGB: fputs( "RGB ", fp ); break;
        case C_MODEL_HSV: fputs( "HSV ", fp ); break;
        case C_MODEL_CMY: fputs( "CMY ", fp ); break;
        case C_MODEL_YIQ: fputs( "YIQ ", fp ); break;
        case C_MODEL_XYZ: fputs( "XYZ ", fp ); break;
        default:
	  fprintf( stderr, "%s:%d ooops: Unknown color model '%c'.\n",
		   __FILE__, __LINE__, (char)(sm_palette.cmodel) );
      }
      fputs( "\nset palette ", fp );
      switch( sm_palette.colorMode ) {
      case SMPAL_COLOR_MODE_RGB:
	fprintf( fp, "rgbformulae %d, %d, %d\n", sm_palette.formulaR,
		 sm_palette.formulaG, sm_palette.formulaB );
	break;
      case SMPAL_COLOR_MODE_GRADIENT: {
	int i=0;
	fprintf( fp, "defined (" );
	for( i=0; i<sm_palette.gradient_num; ++i ) {
	  fprintf( fp, " %.4g %.4g %.4g %.4g", sm_palette.gradient[i].pos,
		   sm_palette.gradient[i].col.r, sm_palette.gradient[i].col.g,
		   sm_palette.gradient[i].col.b );
	  if (i<sm_palette.gradient_num-1)  {
	      fputs( ",", fp);
	      if (i==2 || i%4==2)  fputs( "\\\n    ", fp );
	  }
	}
	fputs( " )\n", fp );
	break;
      }
      case SMPAL_COLOR_MODE_FUNCTIONS:
	fprintf( fp, "functions %s, %s, %s\n", sm_palette.Afunc.definition,
		 sm_palette.Bfunc.definition, sm_palette.Cfunc.definition );
	break;
      default:
	fprintf( stderr, "%s:%d ooops: Unknown color mode '%c'.\n",
		 __FILE__, __LINE__, (char)(sm_palette.colorMode) );
      }
    }

    /*
     *  Save colorbox info
     */
    if (color_box.where != SMCOLOR_BOX_NO)
	fprintf(fp,"set colorbox %s\n", color_box.where==SMCOLOR_BOX_DEFAULT ? "default" : "user");
    fprintf(fp, "set colorbox %sal origin ", color_box.rotation ==  'v' ? "vertic" : "horizont");
    save_position(fp, &color_box.origin, FALSE);
    fputs(" size ", fp);
    save_position(fp, &color_box.size, FALSE);
    fputs(" ", fp);
    if (color_box.border == 0) fputs("noborder", fp);
	else if (color_box.border_lt_tag < 0) fputs("bdefault", fp);
		 else fprintf(fp, "border %d", color_box.border_lt_tag);
    if (color_box.where == SMCOLOR_BOX_NO) fputs("\nunset colorbox\n", fp);
	else fputs("\n", fp);

    fputs("set loadpath ", fp);
    {
	char *s;
	while ((s = save_loadpath()) != NULL)
	    fprintf(fp, "\"%s\" ", s);
	fputc('\n', fp);
    }

    fputs("set fontpath ", fp);
    {
	char *s;
	while ((s = save_fontpath()) != NULL)
	    fprintf(fp, "\"%s\" ", s);
	fputc('\n', fp);
    }

    /* HBB NEW 20020927: fit logfile name option */
#if GP_FIT_ERRVARS
    fprintf(fp, "set fit %serrorvariables",
	    fit_errorvariables ? "" : "no");
    if (fitlogfile) {
	fprintf(fp, " logfile \'%s\'", fitlogfile);
    }
    fputc('\n', fp);
#else
    if (fitlogfile) {
	fprintf(fp, "set fit logfile \'%s\'\n", fitlogfile);
    }
#endif /* GP_FIT_ERRVARS */

}

static void
save_tics(FILE *fp, AXIS_INDEX axis)
{
    if ((axis_array[axis].ticmode & TICS_MASK) == NO_TICS) {
	fprintf(fp, "set no%stics\n", axis_defaults[axis].name);
	return;
    }
    fprintf(fp, "set %stics %s %s scale %g,%g %smirror %s ",
	    axis_defaults[axis].name,
	    ((axis_array[axis].ticmode & TICS_MASK) == TICS_ON_AXIS)
	    ? "axis" : "border",
	    (axis_array[axis].tic_in) ? "in" : "out",
	    axis_array[axis].ticscale, axis_array[axis].miniticscale,
	    (axis_array[axis].ticmode & TICS_MIRROR) ? "" : "no",
	    axis_array[axis].tic_rotate ? "rotate" : "norotate");
    if (axis_array[axis].tic_rotate)
    	fprintf(fp,"by %d ",axis_array[axis].tic_rotate);
    save_position(fp, &axis_array[axis].ticdef.offset, TRUE);
    fprintf(fp," ");
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
	if (axis_array[axis].ticdef.def.series.start != -VERYLARGE) {
	    SAVE_NUM_OR_TIME(fp,
			     (double) axis_array[axis].ticdef.def.series.start,
			     axis);
	    putc(',', fp);
	}
	fprintf(fp, "%g", axis_array[axis].ticdef.def.series.incr);
	if (axis_array[axis].ticdef.def.series.end != VERYLARGE) {
	    putc(',', fp);
	    SAVE_NUM_OR_TIME(fp,
			     (double) axis_array[axis].ticdef.def.series.end,
			     axis);
	}
	break;
    case TIC_USER:
	break;
    }

    if (axis_array[axis].ticdef.font && *axis_array[axis].ticdef.font)
        fprintf(fp, " font \"%s\"", axis_array[axis].ticdef.font);

    if (axis_array[axis].ticdef.textcolor.type != TC_DEFAULT)
        save_textcolor(fp, &axis_array[axis].ticdef.textcolor);

    putc('\n', fp);

    if (axis_array[axis].ticdef.def.user) { 
	struct ticmark *t;
	fprintf(fp, "set %stics %s ", axis_defaults[axis].name,
		(axis_array[axis].ticdef.type == TIC_USER) ? "" : "add");
	fputs(" (", fp);
	for (t = axis_array[axis].ticdef.def.user; t != NULL; t = t->next) {
	    if (t->label)
		fprintf(fp, "\"%s\" ", conv_text(t->label));
	    SAVE_NUM_OR_TIME(fp, (double) t->position, axis);
	    if (t->level)
		fprintf(fp, " %d", t->level);
	    if (t->next) {
		fputs(", ", fp);
	    }
	}
	fputs(")\n", fp);
    }

}

static void
save_position(FILE *fp, struct position *pos, TBOOLEAN offset)
{
    static const char *msg[] = { "first ", "second ", "graph ", "screen ",
				 "character "};
 
    assert(first_axes == 0 && second_axes == 1 && graph == 2 && screen == 3 &&
	   character == 4);

    if (offset)
	fprintf(fp, " offset ");

    fprintf(fp, "%s%g, %s%g, %s%g",
	    pos->scalex == first_axes ? "" : msg[pos->scalex], pos->x,
	    pos->scaley == pos->scalex ? "" : msg[pos->scaley], pos->y,
	    pos->scalez == pos->scaley ? "" : msg[pos->scalez], pos->z);
}


void
save_range(FILE *fp, AXIS_INDEX axis)
{
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
save_zeroaxis(FILE *fp, AXIS_INDEX axis)
{
    fprintf(fp, "set %szeroaxis", axis_defaults[axis].name);
    save_linetype(fp, &(axis_array[axis].zeroaxis), FALSE);
    putc('\n', fp);
}

void
save_fillstyle(FILE *fp, const struct fill_style_type *fs)
{
    switch(fs->fillstyle) {
    case FS_SOLID:
	fprintf(fp, " solid %.2f ", fs->filldensity / 100.0);
	break;
    case FS_PATTERN:
	fprintf(fp, " pattern %d ", fs->fillpattern);
	break;
    case FS_DEFAULT:
	fprintf(fp, " default\n");
	return;
    default:
	fprintf(fp, " empty ");
	break;
    }
    if (fs->border_linetype == LT_NODRAW)
	fprintf(fp, "noborder\n");
    else if (fs->border_linetype == LT_UNDEFINED)
	fprintf(fp, "border\n");
    else
	fprintf(fp, "border %d\n",fs->border_linetype+1);
}

void
save_textcolor(FILE *fp, const struct t_colorspec *tc)
{
    if (tc->type) {
    	fprintf(fp, " textcolor");
	save_pm3dcolor(fp, tc);
    }
}


void
save_pm3dcolor(FILE *fp, const struct t_colorspec *tc)
{
    if (tc->type) {
	switch(tc->type) {
	case TC_LT:   fprintf(fp," lt %d", tc->lt+1);
		      break;
	case TC_LINESTYLE:   fprintf(fp," linestyle %d", tc->lt);
		      break;
	case TC_Z:    fprintf(fp," palette z");
		      break;
	case TC_CB:   fprintf(fp," palette cb %g", tc->value);
		      break;
	case TC_FRAC: fprintf(fp," palette fraction %4.2f", tc->value);
		      break;
	case TC_RGB:  {
		      const char *color = reverse_table_lookup(pm3d_color_names_tbl, tc->lt);
		      if (color)
	    		fprintf(fp," rgb \"%s\" ", color);
    		      else
	    		fprintf(fp," rgb \"#%6.6x\" ", tc->lt);
    		      break;
	    	      }
	default:      break;
	}
    }
}

void
save_data_func_style(FILE *fp, const char *which, enum PLOT_STYLE style)
{
    switch (style) {
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
#ifdef EAM_HISTOGRAMS
    case HISTOGRAMS:
	fputs("histograms\n", fp);
	break;
#endif
    case FILLEDCURVES:
	fputs("filledcurves ", fp);
	if (!strcmp(which, "data") || !strcmp(which, "Data"))
	    filledcurves_options_tofile(&filledcurves_opts_data, fp);
	else
	    filledcurves_options_tofile(&filledcurves_opts_func, fp);
	fputc('\n', fp);
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
    case PM3DSURFACE:
	fputs("pm3d\n", fp);
	break;
#ifdef EAM_DATASTRINGS
    case LABELPOINTS:
	fputs("labels\n", fp);
	break;
#endif
#ifdef WITH_IMAGE
    case IMAGE:
	fputs("image\n", stderr);
	break;
    case RGBIMAGE:
	fputs("rgbimage\n", stderr);
	break;
#endif
    default:
	fputs("---error!---\n", fp);
    }
}

void
save_linetype(FILE *fp, lp_style_type *lp, TBOOLEAN show_point)
{

    fprintf(fp, " linetype %d", lp->l_type + 1);
    if (lp->use_palette) {
	fprintf(fp, " linecolor");
	if (lp->pm3d_color.type == TC_LT)
    	    fprintf(fp, " %d", lp->pm3d_color.lt+1);
	else
    	    save_pm3dcolor(fp,&(lp->pm3d_color));
    }
    fprintf(fp, " linewidth %.3f", lp->l_width);

    if (show_point && lp->pointflag) {
	fprintf(fp, " pointtype %d", lp->p_type + 1);
	if (lp->p_size == PTSZ_VARIABLE)
	    fprintf(fp, " pointsize variable");
	else if (lp->p_size == PTSZ_DEFAULT)
	    fprintf(fp, " pointsize default");
	else
	    fprintf(fp, " pointsize %.3f", lp->p_size);
    }
	
}

#ifdef EAM_OBJECTS

/* Save/show rectangle <tag> (0 means show all) */
void
save_rectangle(FILE *fp, int tag)
{
    t_object *this_object;
    t_rectangle *this_rect;
    TBOOLEAN showed = FALSE;

    for (this_object = first_object; this_object != NULL; this_object = this_object->next) {
	if (this_object->object_type == OBJ_RECTANGLE)
	    this_rect = &this_object->o.rectangle;
	else
	    continue;
	if (tag == 0 || tag == this_object->tag) {
	    showed = TRUE;
	    fprintf(fp, "%sobject %2d rect ", (fp==stderr) ? "\t" : "set ",this_object->tag);

	    if (this_rect->type == 1) {
		fprintf(fp, "center ");
		save_position(fp, &this_rect->center, FALSE);
		fprintf(fp, " size ");
		save_position(fp, &this_rect->extent, FALSE);
	    } else {
		fprintf(fp, "from ");
		save_position(fp, &this_rect->bl, FALSE);
		fprintf(fp, " to ");
		save_position(fp, &this_rect->tr, FALSE);
	    }

	    fprintf(fp, " %s ", this_object->layer > 0 ? "front" : this_object->layer < 0 ? "behind" : "back");
	    if (this_object->lp_properties.l_width)
		fprintf(fp, "lw %.1f ",this_object->lp_properties.l_width);
	    fprintf(fp, "fc ");
	    if (this_object->lp_properties.l_type == LT_DEFAULT)
		fprintf(fp,"default");
	    else if (this_object->lp_properties.use_palette)
		save_pm3dcolor(fp, &this_object->lp_properties.pm3d_color);
	    else
		fprintf(fp, "lt %d",this_object->lp_properties.l_type+1);
	    fprintf(fp, " fillstyle ");
	    save_fillstyle(fp, &this_object->fillstyle);
	}
    }
    if (tag > 0 && !showed)
	int_error(c_token, "rect not found");
}

#endif

