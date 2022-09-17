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
#include "jitter.h"
#include "misc.h"
#include "plot2d.h"
#include "plot3d.h"
#include "setshow.h"
#include "term_api.h"
#include "util.h"
#include "variable.h"
#include "pm3d.h"
#include "getcolor.h"

static void save_functions__sub(FILE *);
static void save_variables__sub(FILE *);
static void save_tics(FILE *, struct axis *);
static void save_mtics(FILE *, struct axis *);
static void save_zeroaxis(FILE *,AXIS_INDEX);
static void save_set_all(FILE *);
static void save_justification(int just, FILE *fp);

const char *coord_msg[] = {"first ", "second ", "graph ", "screen ", "character ", "polar "};
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
	if (df_filename)
	    fprintf(fp, "## Last datafile plotted: \"%s\"\n", df_filename);
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
	if (udv->udv_value.type != NOTDEFINED) {
	    if ((udv->udv_value.type == ARRAY)
		&& strncmp(udv->udv_name,"ARGV",4)) {
		fprintf(fp,"array %s[%d] = ", udv->udv_name,
			(int)(udv->udv_value.v.value_array[0].v.int_val));
		save_array_content(fp, udv->udv_value.v.value_array);
	    } else if (strncmp(udv->udv_name,"GPVAL_",6)
		 && strncmp(udv->udv_name,"GPFUN_",6)
		 && strncmp(udv->udv_name,"MOUSE_",6)
		 && strncmp(udv->udv_name,"$",1)
		 && (strncmp(udv->udv_name,"ARG",3) || (strlen(udv->udv_name) != 4))
		 && strncmp(udv->udv_name,"NaN",4)) {
		    fprintf(fp, "%s = ", udv->udv_name);
		    disp_value(fp, &(udv->udv_value), TRUE);
		    (void) putc('\n', fp);
	    }
	}
	udv = udv->next_udv;
    }
}

void
save_array_content(FILE *fp, struct value *array)
{
    int i;
    int size = array[0].v.int_val;
    fprintf(fp, "[");
    for (i=1; i<=size; i++) {
	if (array[i].type != NOTDEFINED)
	    disp_value(fp, &(array[i]), TRUE);
	if (i < size)
	    fprintf(fp, ",");
    }
    fprintf(fp, "]\n");
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

/* helper function */
void
save_axis_label_or_title(FILE *fp, char *name, char *suffix,
			struct text_label *label, TBOOLEAN savejust)
{
    fprintf(fp, "set %s%s \"%s\" ",
	    name, suffix, label->text ? conv_text(label->text) : "");
    fprintf(fp, "\nset %s%s ", name, suffix);
    save_position(fp, &(label->offset), 3, TRUE);
    fprintf(fp, " font \"%s\"", label->font ? conv_text(label->font) : "");
    save_textcolor(fp, &(label->textcolor));
    if (savejust && (label->pos != CENTRE)) save_justification(label->pos,fp);
    if (label->tag == ROTATE_IN_3D_LABEL_TAG)
	fprintf(fp, " rotate parallel");
    else if (label->rotate == TEXT_VERTICAL)
	fprintf(fp, " rotate");
    else if (label->rotate)
	fprintf(fp, " rotate by %d", label->rotate);
    else
	fprintf(fp, " norotate");

    if (label == &title && label->boxed) {
	fprintf(fp," boxed ");
	if (label->boxed > 0)
	    fprintf(fp,"bs %d ",label->boxed);
    }
    fprintf(fp, "%s\n", (label->noenhanced) ? " noenhanced" : "");
}

static void
save_justification(int just, FILE *fp)
{
    switch (just) {
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

static void
save_set_all(FILE *fp)
{
    struct text_label *this_label;
    struct arrow_def *this_arrow;
    struct linestyle_def *this_linestyle;
    struct arrowstyle_def *this_arrowstyle;
    legend_key *key = &keyT;
    int axis;

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
%sset clip radial\n",
	    (clip_points) ? "" : "un",
	    (clip_lines1) ? "" : "un",
	    (clip_lines2) ? "" : "un",
	    (clip_radial) ? "" : "un"
	    );

    save_bars(fp);

    if (draw_border) {
	fprintf(fp, "set border %d %s", draw_border,
	    border_layer == LAYER_BEHIND ? "behind" : border_layer == LAYER_BACK ? "back" : "front");
	save_linetype(fp, &border_lp, FALSE);
	fprintf(fp, "\n");
    } else
	fputs("unset border\n", fp);

    for (axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
	if (axis == SAMPLE_AXIS) continue;
	if (axis == COLOR_AXIS) continue;
	if (axis == POLAR_AXIS) continue;
	fprintf(fp, "set %sdata %s\n", axis_name(axis),
		axis_array[axis].datatype == DT_TIMEDATE ? "time" :
		axis_array[axis].datatype == DT_DMS ? "geographic" :
		"");
    }

    if (boxwidth < 0.0)
	fputs("set boxwidth\n", fp);
    else
	fprintf(fp, "set boxwidth %g %s\n", boxwidth,
		(boxwidth_is_absolute) ? "absolute" : "relative");
    fprintf(fp, "set boxdepth %g\n", boxdepth > 0 ? boxdepth : 0.0);

    fprintf(fp, "set style fill ");
    save_fillstyle(fp, &default_fillstyle);

    /* Default rectangle style */
    fprintf(fp, "set style rectangle %s fc ",
	    default_rectangle.layer > 0 ? "front" :
	    default_rectangle.layer < 0 ? "behind" : "back");
    save_pm3dcolor(fp, &default_rectangle.lp_properties.pm3d_color);
    fprintf(fp, " fillstyle ");
    save_fillstyle(fp, &default_rectangle.fillstyle);

    /* Default circle properties */
    fprintf(fp, "set style circle radius ");
    save_position(fp, &default_circle.o.circle.extent, 1, FALSE);
    fputs(" \n", fp);

    /* Default ellipse properties */
    fprintf(fp, "set style ellipse size ");
    save_position(fp, &default_ellipse.o.ellipse.extent, 2, FALSE);
    fprintf(fp, " angle %g ", default_ellipse.o.ellipse.orientation);
    fputs("units ", fp);
    switch (default_ellipse.o.ellipse.type) {
	case ELLIPSEAXES_XY:
	    fputs("xy\n", fp);
	    break;
	case ELLIPSEAXES_XX:
	    fputs("xx\n", fp);
	    break;
	case ELLIPSEAXES_YY:
	    fputs("yy\n", fp);
	    break;
    }

    if (dgrid3d) {
      if (dgrid3d_mode == DGRID3D_QNORM) {
	fprintf(fp, "set dgrid3d %d,%d, %d\n",
	  	dgrid3d_row_fineness,
	  	dgrid3d_col_fineness,
	  	dgrid3d_norm_value);
      } else if (dgrid3d_mode == DGRID3D_SPLINES) {
	fprintf(fp, "set dgrid3d %d,%d splines\n",
	  	dgrid3d_row_fineness, dgrid3d_col_fineness );
      } else {
	fprintf(fp, "set dgrid3d %d,%d %s%s %f,%f\n",
	  	dgrid3d_row_fineness,
	  	dgrid3d_col_fineness,
		reverse_table_lookup(dgrid3d_mode_tbl, dgrid3d_mode),
		dgrid3d_kdensity ? " kdensity2d" : "",
	  	dgrid3d_x_scale,
	  	dgrid3d_y_scale );
      }
    }

    /* Dummy variable names */ 
    fprintf(fp, "set dummy %s", set_dummy_var[0]);
    for (axis=1; axis<MAX_NUM_VAR; axis++) {
	if (*set_dummy_var[axis] == '\0')
	    break;
	fprintf(fp, ", %s", set_dummy_var[axis]);
    }
    fprintf(fp, "\n");

    save_axis_format(fp, FIRST_X_AXIS );
    save_axis_format(fp, FIRST_Y_AXIS );
    save_axis_format(fp, SECOND_X_AXIS);
    save_axis_format(fp, SECOND_Y_AXIS);
    save_axis_format(fp, FIRST_Z_AXIS );
    save_axis_format(fp, COLOR_AXIS);
    save_axis_format(fp, POLAR_AXIS);
    fprintf(fp, "set ttics format \"%s\"\n", THETA_AXIS.formatstring);

    fprintf(fp, "set timefmt \"%s\"\n", timefmt);

    fprintf(fp, "set angles %s\n",
	    (ang2rad == 1.0) ? "radians" : "degrees");

    fprintf(fp,"set tics %s\n", grid_tics_in_front ? "front" : "back");

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
		axis_name(axis),		\
		axis_array[axis].gridminor ? "" : "no",	\
		axis_name(axis));
	fputs("set grid", fp);
	SAVE_GRID(FIRST_X_AXIS);
	SAVE_GRID(FIRST_Y_AXIS);
	SAVE_GRID(FIRST_Z_AXIS);
	SAVE_GRID(POLAR_AXIS);
	fputs(" \\\n", fp);
	SAVE_GRID(SECOND_X_AXIS);
	SAVE_GRID(SECOND_Y_AXIS);
	SAVE_GRID(COLOR_AXIS);
	fputs("\n", fp);
#undef SAVE_GRID

	fprintf(fp, "set grid %s%s  ",
		(grid_vertical_lines) ? "vertical " : "",
		(grid_layer==-1) ? "layerdefault" : ((grid_layer==0) ? "back" : "front"));
	save_linetype(fp, &grid_lp, FALSE);
	fprintf(fp, ", ");
	save_linetype(fp, &mgrid_lp, FALSE);
	fputc('\n', fp);
    }
    fprintf(fp, "%sset raxis\n", raxis ? "" : "un");

    /* Theta axis origin and direction */
    fprintf(fp, "set theta %s %s\n",
	theta_direction > 0 ? "counterclockwise" : "clockwise",
	theta_origin == 180 ? "left" : theta_origin ==  90 ? "top" :
	theta_origin == -90 ? "bottom" : "right");

    /* Save parallel axis state */
    save_style_parallel(fp);

    if (key->title.text == NULL)
	fprintf(fp, "set key notitle\n");
    else {
	fprintf(fp, "set key title \"%s\"", conv_text(key->title.text));
	if (key->title.font)
	    fprintf(fp, " font \"%s\" ", key->title.font);
	save_justification(key->title.pos, fp);
	fputs("\n", fp);
    }

    fputs("set key ", fp);
    switch (key->region) {
	case GPKEY_AUTO_INTERIOR_LRTBC:
	    fputs(key->fixed ? "fixed" : "inside", fp);
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
	    fputs("at ", fp);
	    save_position(fp, &key->user_pos, 2, FALSE);
	    break;
    }
    if (!(key->region == GPKEY_AUTO_EXTERIOR_MARGIN
	      && (key->margin == GPKEY_LMARGIN || key->margin == GPKEY_RMARGIN))) {
	save_justification(key->hpos, fp);
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
		key->auto_titles == COLUMNHEAD_KEYTITLES ? "autotitle columnhead"
		: key->auto_titles == FILENAME_KEYTITLES ? "autotitle"
		: "noautotitle" );
    if (key->box.l_type > LT_NODRAW) {
	fputs("box", fp);
	save_linetype(fp, &(key->box), FALSE);
    } else
	fputs("nobox", fp);

    /* These are for the key entries, not the key title */
    if (key->font)
	fprintf(fp, " font \"%s\"", key->font);
    if (key->textcolor.type != TC_LT || key->textcolor.lt != LT_BLACK)
	save_textcolor(fp, &key->textcolor);

    /* Put less common options on separate lines */
    fprintf(fp, "\nset key %sinvert samplen %g spacing %g width %g height %g ",
		key->invert ? "" : "no",
		key->swidth, key->vert_factor, key->width_fix, key->height_fix);
    fprintf(fp, "\nset key maxcolumns %d maxrows %d",key->maxcols,key->maxrows);
    save_position(fp, &key->offset, 2, TRUE);
    fputc('\n', fp);
    if (key->front) {
	fprintf(fp, "set key opaque");
	if (key->fillcolor.lt != LT_BACKGROUND) {
	    fprintf(fp, " fc ");
	    save_pm3dcolor(fp, &key->fillcolor);
	}
	fprintf(fp, "\n");
    } else {
	fprintf(fp, "set key noopaque\n");
    }

    if (!(key->visible))
	fputs("unset key\n", fp);

    fputs("unset label\n", fp);
    for (this_label = first_label; this_label != NULL;
	 this_label = this_label->next) {
	fprintf(fp, "set label %d \"%s\" at ",
		this_label->tag,
		conv_text(this_label->text));
	save_position(fp, &this_label->place, 3, FALSE);
	if (this_label->hypertext)
	    fprintf(fp, " hypertext");

	save_justification(this_label->pos, fp);
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
	if ((this_label->lp_properties.flags & LP_SHOW_POINTS) == 0)
	    fprintf(fp, " nopoint");
	else {
	    fprintf(fp, " point");
	    save_linetype(fp, &(this_label->lp_properties), TRUE);
	}
	save_position(fp, &this_label->offset, 3, TRUE);
	if (this_label->boxed) {
	    fprintf(fp," boxed ");
	    if (this_label->boxed > 0)
		fprintf(fp,"bs %d ",this_label->boxed);
	}
	fputc('\n', fp);
    }
    fputs("unset arrow\n", fp);
    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next) {
	fprintf(fp, "set arrow %d from ", this_arrow->tag);
	save_position(fp, &this_arrow->start, 3, FALSE);
	if (this_arrow->type == arrow_end_absolute) {
	    fputs(" to ", fp);
	    save_position(fp, &this_arrow->end, 3, FALSE);
	} else if (this_arrow->type == arrow_end_relative) {
	    fputs(" rto ", fp);
	    save_position(fp, &this_arrow->end, 3, FALSE);
	} else { /* type arrow_end_oriented */
	    struct position *e = &this_arrow->end;
	    fputs(" length ", fp);
	    fprintf(fp, "%s%g", e->scalex == first_axes ? "" : coord_msg[e->scalex], e->x);
	    fprintf(fp, " angle %g", this_arrow->angle);
	}
	fprintf(fp, " %s %s %s",
		arrow_head_names[this_arrow->arrow_properties.head],
		(this_arrow->arrow_properties.layer==0) ? "back" : "front",
		(this_arrow->arrow_properties.headfill==AS_FILLED) ? "filled" :
		(this_arrow->arrow_properties.headfill==AS_EMPTY) ? "empty" :
		(this_arrow->arrow_properties.headfill==AS_NOBORDER) ? "noborder" :
		    "nofilled");
	save_linetype(fp, &(this_arrow->arrow_properties.lp_properties), FALSE);
	if (this_arrow->arrow_properties.head_length > 0) {
	    fprintf(fp, " size %s %.3f,%.3f,%.3f",
		    coord_msg[this_arrow->arrow_properties.head_lengthunit],
		    this_arrow->arrow_properties.head_length,
		    this_arrow->arrow_properties.head_angle,
		    this_arrow->arrow_properties.head_backangle);
	}
	fprintf(fp, "\n");
    }

    /* Mostly for backwards compatibility */
    if (prefer_line_styles)
	fprintf(fp, "set style increment userstyles\n");
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
		(this_arrowstyle->arrow_properties.headfill==AS_FILLED)?"filled":
		(this_arrowstyle->arrow_properties.headfill==AS_EMPTY)?"empty":
		(this_arrowstyle->arrow_properties.headfill==AS_NOBORDER)?"noborder":
		   "nofilled");
	save_linetype(fp, &(this_arrowstyle->arrow_properties.lp_properties), FALSE);
	if (this_arrowstyle->arrow_properties.head_length > 0) {
	    fprintf(fp, " size %s %.3f,%.3f,%.3f",
		    coord_msg[this_arrowstyle->arrow_properties.head_lengthunit],
		    this_arrowstyle->arrow_properties.head_length,
		    this_arrowstyle->arrow_properties.head_angle,
		    this_arrowstyle->arrow_properties.head_backangle);
	    if (this_arrowstyle->arrow_properties.head_fixedsize) {
		fputs(" ", fp);
		fputs(" fixed", fp);
	    }
	}
	fprintf(fp, "\n");
    }

    fprintf(fp, "set style histogram ");
    save_histogram_opts(fp);

    save_pixmaps(fp);

    fprintf(fp, "unset object\n");
    save_object(fp, 0);
    fprintf(fp, "unset walls\n");
    save_walls(fp);

    save_style_textbox(fp);

    save_offsets(fp, "set offsets");

    fprintf(fp, "\
set pointsize %g\n\
set pointintervalbox %g\n\
set encoding %s\n\
%sset polar\n\
%sset parametric\n",
	    pointsize, pointintervalbox,
	    encoding_names[encoding],
	    (polar) ? "" : "un",
	    (parametric) ? "" : "un");

    if (spiderplot) {
	fprintf(fp, "set spiderplot\n");
	save_style_spider(fp);
    } else 
	fprintf(fp, "unset spiderplot\n");

    if (numeric_locale)
	fprintf(fp, "set decimalsign locale \"%s\"\n", numeric_locale);
    if (decimalsign != NULL)
	fprintf(fp, "set decimalsign '%s'\n", decimalsign);
    if (!numeric_locale && !decimalsign)
	fprintf(fp, "unset decimalsign\n");

    fprintf(fp, "%sset micro\n", use_micro ? "" : "un");
    fprintf(fp, "%sset minussign\n", use_minus_sign ? "" : "un");

    fputs("set view ", fp);
    if (splot_map == TRUE)
	fprintf(fp, "map scale %g", mapview_scale);
    else if (xz_projection)
	fprintf(fp, "projection xz");
    else if (yz_projection)
	fprintf(fp, "projection yz");
    else {
	fprintf(fp, "%g, %g, %g, %g",
	    surface_rot_x, surface_rot_z, surface_scale, surface_zscale);
	fprintf(fp, "\nset view azimuth %g", azimuth);
    }
    if (aspect_ratio_3D)
	fprintf(fp, "\nset view  %s", aspect_ratio_3D == 2 ? "equal xy" :
			aspect_ratio_3D == 3 ? "equal xyz": "");

    fprintf(fp, "\nset rgbmax %g", rgbmax);

    fprintf(fp, "\n\
set samples %d, %d\n\
set isosamples %d, %d",
	    samples_1, samples_2,
	    iso_samples_1, iso_samples_2);

    fprintf(fp, "\n\
set surface %s\n\
%sset surface",
	    (implicit_surface) ? "implicit" : "explicit",
	    (draw_surface) ? "" : "un");

    fprintf(fp, "\n\
%sset contour", (draw_contour) ? "" : "un");
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

    /* Contour label options */
    fprintf(fp, "set cntrlabel %s format '%s' font '%s' start %d interval %d\n", 
	clabel_onecolor ? "onecolor" : "", contour_format,
	clabel_font ? clabel_font : "",
	clabel_start, clabel_interval);

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
    if (df_separators)
	fprintf(fp, "set datafile separator \"%s\"\n",df_separators);
    else
	fprintf(fp, "set datafile separator whitespace\n");
    if (strcmp(df_commentschars, DEFAULT_COMMENTS_CHARS))
	fprintf(fp, "set datafile commentschars '%s'\n", df_commentschars);
    if (df_fortran_constants)
	fprintf(fp, "set datafile fortran\n");
    if (df_nofpe_trap)
	fprintf(fp, "set datafile nofpe_trap\n");
    fprintf(fp, "set datafile %scolumnheaders\n", df_columnheaders ? "" : "no");

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
    fprintf(fp, "set cntrparam levels %d\nset cntrparam levels ", contour_levels);
    switch (contour_levels_kind) {
    case LEVELS_AUTO:
	fprintf(fp, "auto");
	break;
    case LEVELS_INCREMENTAL:
	fprintf(fp, "incremental %g,%g",
		contour_levels_list[0], contour_levels_list[1]);
	break;
    case LEVELS_DISCRETE:
	{
	    int i;
	    fprintf(fp, "discrete %g", contour_levels_list[0]);
	    for (i = 1; i < contour_levels; i++)
		fprintf(fp, ",%g ", contour_levels_list[i]);
	}
    }
    fprintf(fp, "\nset cntrparam firstlinetype %d", contour_firstlinetype);
    fprintf(fp, " %ssorted\n", contour_sortlevels ? "" : "un");
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
	fprintf(fp, "set xyplane at %g\n", xyplane.z);
    else
	fprintf(fp, "set xyplane relative %g\n", xyplane.z);

    {
    int i;
    fprintf(fp, "set tics scale ");
    for (i=0; i<MAX_TICLEVEL; i++)
	fprintf(fp, " %g%c", ticscale[i], i<MAX_TICLEVEL-1 ? ',' : '\n');
    }

    save_mtics(fp, &axis_array[FIRST_X_AXIS]);
    save_mtics(fp, &axis_array[FIRST_Y_AXIS]);
    save_mtics(fp, &axis_array[FIRST_Z_AXIS]);
    save_mtics(fp, &axis_array[SECOND_X_AXIS]);
    save_mtics(fp, &axis_array[SECOND_Y_AXIS]);
    save_mtics(fp, &axis_array[COLOR_AXIS]);
    save_mtics(fp, &R_AXIS);
    save_mtics(fp, &THETA_AXIS);

    save_tics(fp, &axis_array[FIRST_X_AXIS]);
    save_tics(fp, &axis_array[FIRST_Y_AXIS]);
    save_tics(fp, &axis_array[FIRST_Z_AXIS]);
    save_tics(fp, &axis_array[SECOND_X_AXIS]);
    save_tics(fp, &axis_array[SECOND_Y_AXIS]);
    save_tics(fp, &axis_array[COLOR_AXIS]);
    save_tics(fp, &R_AXIS);
    save_tics(fp, &THETA_AXIS);
    for (axis=0; axis<num_parallel_axes; axis++)
	save_tics(fp, &parallel_axis_array[axis]);

    save_axis_label_or_title(fp, "", "title", &title, TRUE);

    fprintf(fp, "set timestamp %s \n", timelabel_bottom ? "bottom" : "top");
    save_axis_label_or_title(fp, "", "timestamp", &timelabel, FALSE);

    save_prange(fp, axis_array + T_AXIS);
    save_prange(fp, axis_array + U_AXIS);
    save_prange(fp, axis_array + V_AXIS);

#define SAVE_AXISLABEL(axis)					\
    save_axis_label_or_title(fp, axis_name(axis), "label", &axis_array[axis].label, TRUE)

    SAVE_AXISLABEL(FIRST_X_AXIS);
    SAVE_AXISLABEL(SECOND_X_AXIS);
    save_prange(fp, axis_array + FIRST_X_AXIS);
    save_prange(fp, axis_array + SECOND_X_AXIS);

    SAVE_AXISLABEL(FIRST_Y_AXIS);
    SAVE_AXISLABEL(SECOND_Y_AXIS);
    save_prange(fp, axis_array + FIRST_Y_AXIS);
    save_prange(fp, axis_array + SECOND_Y_AXIS);

    SAVE_AXISLABEL(FIRST_Z_AXIS);
    save_prange(fp, axis_array + FIRST_Z_AXIS);

    SAVE_AXISLABEL(COLOR_AXIS);
    save_prange(fp, axis_array + COLOR_AXIS);

    SAVE_AXISLABEL(POLAR_AXIS);
    save_prange(fp, axis_array + POLAR_AXIS);

    for (axis=0; axis<num_parallel_axes; axis++) {
	struct axis *paxis = &parallel_axis_array[axis];
	save_prange(fp, paxis);
	if (paxis->label.text)
	    save_axis_label_or_title(fp,
				axis_name(paxis->index),"label",
				&paxis->label, TRUE);
	if (paxis->zeroaxis) {
	    fprintf(fp, "set paxis %d", axis+1);
	    save_linetype(fp, paxis->zeroaxis, FALSE);
	    fprintf(fp, "\n");
	}
    }

#undef SAVE_AXISLABEL

    fputs("unset logscale\n", fp);
    for (axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
	if (axis_array[axis].log)
	    fprintf(fp, "set logscale %s %g\n", axis_name(axis),
		axis_array[axis].base);
	else
	    save_nonlinear(fp, &axis_array[axis]);
    }

    /* These will only print something if the axis is, in fact, linked */    
    save_link(fp, axis_array + SECOND_X_AXIS);
    save_link(fp, axis_array + SECOND_Y_AXIS);

    save_jitter(fp);

    fprintf(fp, "set zero %g\n", zero);

    fprintf(fp, "set lmargin %s %g\n",
	    lmargin.scalex == screen ? "at screen" : "", lmargin.x);
    fprintf(fp, "set bmargin %s %g\n",
	    bmargin.scalex == screen ? "at screen" : "", bmargin.x);
    fprintf(fp, "set rmargin %s %g\n",
	    rmargin.scalex == screen ? "at screen" : "", rmargin.x);
    fprintf(fp, "set tmargin %s %g\n",
	    tmargin.scalex == screen ? "at screen" : "", tmargin.x);

    fprintf(fp, "set locale \"%s\"\n", get_time_locale());

    /* pm3d options */
    fputs("set pm3d ", fp);
    fputs((PM3D_IMPLICIT == pm3d.implicit ? "implicit" : "explicit"), fp);
    fprintf(fp, " at %s\n", pm3d.where);
    fputs("set pm3d ", fp);
    switch (pm3d.direction) {
    case PM3D_SCANS_AUTOMATIC: fputs("scansautomatic\n", fp); break;
    case PM3D_SCANS_FORWARD: fputs("scansforward\n", fp); break;
    case PM3D_SCANS_BACKWARD: fputs("scansbackward\n", fp); break;
    case PM3D_DEPTH: fprintf(fp, "depthorder %s\n", pm3d.base_sort ? "base" : ""); break;
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
    if (pm3d.border.l_type == LT_NODRAW) {
	fprintf(fp," noborder");
    } else {
	fprintf(fp," border");
	if (pm3d.border.l_type == LT_DEFAULT)
	    fprintf(fp," retrace");
	save_linetype(fp, &(pm3d.border), FALSE);
    }
    fputs(" corners2color ", fp);
    switch (pm3d.which_corner_color) {
	case PM3D_WHICHCORNER_MEAN:    fputs("mean", fp); break;
	case PM3D_WHICHCORNER_GEOMEAN: fputs("geomean", fp); break;
	case PM3D_WHICHCORNER_HARMEAN: fputs("harmean", fp); break;
	case PM3D_WHICHCORNER_MEDIAN:  fputs("median", fp); break;
	case PM3D_WHICHCORNER_MIN:     fputs("min", fp); break;
	case PM3D_WHICHCORNER_MAX:     fputs("max", fp); break;
	case PM3D_WHICHCORNER_RMS:     fputs("rms", fp); break;

	default: /* PM3D_WHICHCORNER_C1 ... _C4 */
	     fprintf(fp, "c%i", pm3d.which_corner_color - PM3D_WHICHCORNER_C1 + 1);
    }
    fputs("\n", fp);

    fprintf(fp, "set pm3d %s %s\n",
		pm3d.clip == PM3D_CLIP_1IN ? "clip1in" : 
		pm3d.clip == PM3D_CLIP_4IN ? "clip4in" : "clip z",
		pm3d.no_clipcb ? "noclipcb" : "");

    if (pm3d_shade.strength <= 0)
	fputs("set pm3d nolighting\n",fp);
    else
	fprintf(fp, "set pm3d lighting primary %g specular %g spec2 %g\n",
		pm3d_shade.strength, pm3d_shade.spec, pm3d_shade.spec2);

    /*
     *  Save palette information
     */

    fprintf( fp, "set palette %s %s maxcolors %d ",
	     sm_palette.positive==SMPAL_POSITIVE ? "positive" : "negative",
	     sm_palette.ps_allcF ? "ps_allcF" : "nops_allcF",
	sm_palette.use_maxcolors);
    fprintf( fp, "gamma %g ", sm_palette.gamma );
    if (sm_palette.colorMode == SMPAL_COLOR_MODE_GRAY) {
      fputs( "gray\n", fp );
    }
    else {
      fputs( "color model ", fp );
      switch( sm_palette.cmodel ) {
	default:
	case C_MODEL_RGB: fputs( "RGB ", fp ); break;
	case C_MODEL_HSV: fputs( "HSV ", fp ); break;
	case C_MODEL_CMY: fputs( "CMY ", fp ); break;
      }
      fputs( "\nset palette ", fp );
      switch( sm_palette.colorMode ) {
      default:
      case SMPAL_COLOR_MODE_RGB:
	fprintf( fp, "rgbformulae %d, %d, %d\n", sm_palette.formulaR,
		 sm_palette.formulaG, sm_palette.formulaB );
	break;
      case SMPAL_COLOR_MODE_GRADIENT: {
	int i=0;
	fprintf( fp, "defined (" );
	for (i=0; i<sm_palette.gradient_num; i++) {
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
      case SMPAL_COLOR_MODE_CUBEHELIX:
	fprintf( fp, "cubehelix start %.2g cycles %.2g saturation %.2g\n",
		sm_palette.cubehelix_start, sm_palette.cubehelix_cycles,
		sm_palette.cubehelix_saturation);
	break;
      }
    }

    /*
     *  Save colorbox info
     */
    if (color_box.where != SMCOLOR_BOX_NO)
	fprintf(fp,"set colorbox %s\n", color_box.where==SMCOLOR_BOX_DEFAULT ? "default" : "user");
    fprintf(fp, "set colorbox %sal origin ", color_box.rotation ==  'v' ? "vertic" : "horizont");
    save_position(fp, &color_box.origin, 2, FALSE);
    fputs(" size ", fp);
    save_position(fp, &color_box.size, 2, FALSE);
    fprintf(fp, " %s ", color_box.layer ==  LAYER_FRONT ? "front" : "back");
    fprintf(fp, " %sinvert ", color_box.invert ? "" : "no");
    if (color_box.border == 0) fputs("noborder", fp);
	else if (color_box.border_lt_tag < 0) fputs("bdefault", fp);
		 else fprintf(fp, "border %d", color_box.border_lt_tag);
    if (color_box.where == SMCOLOR_BOX_NO) fputs("\nunset colorbox\n", fp);
	else fputs("\n", fp);

    fprintf(fp, "set style boxplot %s %s %5.2f %soutliers pt %d separation %g labels %s %ssorted\n",
		boxplot_opts.plotstyle == FINANCEBARS ? "financebars" : "candles",
		boxplot_opts.limit_type == 1 ? "fraction" : "range",
		boxplot_opts.limit_value,
		boxplot_opts.outliers ? "" : "no",
		boxplot_opts.pointtype+1,
		boxplot_opts.separation,
		(boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X)  ? "x"  :
		(boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X2) ? "x2" :
		(boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_AUTO) ? "auto" :"off",
		boxplot_opts.sort_factors ? "" : "un");

    fputs("set loadpath ", fp);
    {
	char *s;
	while ((s = save_loadpath()) != NULL)
	    fprintf(fp, "\"%s\" ", s);
	fputc('\n', fp);
    }

    if (PS_fontpath)
	fprintf(fp, "set fontpath \"%s\"\n", PS_fontpath);
    else
	fprintf(fp, "set fontpath\n");

    if (PS_psdir)
	fprintf(fp, "set psdir \"%s\"\n", PS_psdir);
    else
	fprintf(fp, "set psdir\n");

    fprintf(fp, "set fit");
    if (fit_suppress_log)
	fprintf(fp, " nologfile");
    else if (fitlogfile)
	fprintf(fp, " logfile \'%s\'", fitlogfile);
    fprintf(fp, " %s", reverse_table_lookup(fit_verbosity_level, fit_verbosity));
    fprintf(fp, " %serrorvariables",
	fit_errorvariables ? "" : "no");
    fprintf(fp, " %scovariancevariables",
	fit_covarvariables ? "" : "no");
    fprintf(fp, " %serrorscaling",
	fit_errorscaling ? "" : "no");
    fprintf(fp, " %sprescale", fit_prescale ? "" : "no");
    {
	struct udvt_entry *v;
	double d;
	int i;

	v = get_udv_by_name((char *)FITLIMIT);
	d = ((v != NULL) && (v->udv_value.type != NOTDEFINED)) ? real(&(v->udv_value)) : -1.0;
	if ((d > 0.) && (d < 1.))
	    fprintf(fp, " limit %g", d);

	if (epsilon_abs > 0.)
	    fprintf(fp, " limit_abs %g", epsilon_abs);

	v = get_udv_by_name((char *)FITMAXITER);
	i = ((v != NULL) && (v->udv_value.type != NOTDEFINED)) ? real(&(v->udv_value)) : -1;
	if (i > 0)
	    fprintf(fp, " maxiter %i", i);

	v = get_udv_by_name((char *)FITSTARTLAMBDA);
	d = ((v != NULL) && (v->udv_value.type != NOTDEFINED)) ? real(&(v->udv_value)) : -1.0;
	if (d > 0.)
	    fprintf(fp, " start_lambda %g", d);

	v = get_udv_by_name((char *)FITLAMBDAFACTOR);
	d = ((v != NULL) && (v->udv_value.type != NOTDEFINED)) ? real(&(v->udv_value)) : -1.0;
	if (d > 0.)
	    fprintf(fp, " lambda_factor %g", d);
    }
    if (fit_script != NULL)
	fprintf(fp, " script \'%s\'", fit_script);
    if (fit_wrap != 0)
	fprintf(fp, " wrap %i", fit_wrap);
    else
	fprintf(fp, " nowrap");
    fprintf(fp, " v%i", fit_v4compatible ? 4 : 5);
    fputc('\n', fp);
}


static void
save_tics(FILE *fp, struct axis *this_axis)
{
    if ((this_axis->ticmode & TICS_MASK) == NO_TICS) {
	fprintf(fp, "unset %stics\n", axis_name(this_axis->index));
	return;
    }
    fprintf(fp, "set %stics %s %s scale %g,%g %smirror %s ",
	    axis_name(this_axis->index),
	    ((this_axis->ticmode & TICS_MASK) == TICS_ON_AXIS)
	    ? "axis" : "border",
	    (this_axis->tic_in) ? "in" : "out",
	    this_axis->ticscale, this_axis->miniticscale,
	    (this_axis->ticmode & TICS_MIRROR) ? "" : "no",
	    this_axis->tic_rotate ? "rotate" : "norotate");
    if (this_axis->tic_rotate)
	fprintf(fp,"by %d ",this_axis->tic_rotate);
    save_position(fp, &this_axis->ticdef.offset, 3, TRUE);
    if (this_axis->manual_justify)
	save_justification(this_axis->tic_pos, fp);
    else
	fputs(" autojustify", fp);
    fprintf(fp, "\nset %stics ", axis_name(this_axis->index));

    fprintf(fp, (this_axis->ticdef.rangelimited)?" rangelimit ":" norangelimit ");

    if (this_axis->ticdef.logscaling)
	fputs("logscale ", fp);

    switch (this_axis->ticdef.type) {
    case TIC_COMPUTED:{
	    fputs("autofreq ", fp);
	    break;
	}
    case TIC_MONTH:{
	    fprintf(fp, "\nset %smtics", axis_name(this_axis->index));
	    break;
	}
    case TIC_DAY:{
	    fprintf(fp, "\nset %sdtics", axis_name(this_axis->index));
	    break;
	}
    case TIC_SERIES:
	if (this_axis->ticdef.def.series.start != -VERYLARGE) {
	    save_num_or_time_input(fp,
			     (double) this_axis->ticdef.def.series.start,
			     this_axis);
	    putc(',', fp);
	}
	fprintf(fp, "%g", this_axis->ticdef.def.series.incr);
	if (this_axis->ticdef.def.series.end != VERYLARGE) {
	    putc(',', fp);
	    save_num_or_time_input(fp,
			     (double) this_axis->ticdef.def.series.end,
			     this_axis);
	}
	break;
    case TIC_USER:
	break;
    }

    if (this_axis->ticdef.font && *this_axis->ticdef.font)
	fprintf(fp, " font \"%s\"", this_axis->ticdef.font);

    if (this_axis->ticdef.enhanced == FALSE)
	fprintf(fp, " noenhanced");

    if (this_axis->ticdef.textcolor.type != TC_DEFAULT)
	save_textcolor(fp, &this_axis->ticdef.textcolor);

    putc('\n', fp);

    if (this_axis->ticdef.def.user) {
	struct ticmark *t;
	fprintf(fp, "set %stics %s ", axis_name(this_axis->index),
		(this_axis->ticdef.type == TIC_USER) ? "" : "add");
	fputs(" (", fp);
	for (t = this_axis->ticdef.def.user; t != NULL; t = t->next) {
	    if (t->level < 0)	/* Don't save ticlabels read from data file */
		continue;
	    if (t->label)
		fprintf(fp, "\"%s\" ", conv_text(t->label));
	    save_num_or_time_input(fp, (double) t->position, this_axis);
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
save_mtics(FILE *fp, struct axis *axis)
{
    char *name = axis_name(axis->index);

    switch(axis->minitics & TICS_MASK) {
    case 0:
	fprintf(fp, "set nom%stics\n", name);
	break;
    case MINI_AUTO:
	fprintf(fp, "set m%stics\n", name);
	break;
    case MINI_DEFAULT:
	fprintf(fp, "set m%stics default\n", name);
	break;
    case MINI_USER:
	fprintf(fp, "set m%stics %f\n", name, axis->mtic_freq);
	break;
    }
}

void
save_num_or_time_input(FILE *fp, double x, struct axis *this_axis)
{
    if (this_axis->datatype == DT_TIMEDATE) {
	char s[80];

	putc('"', fp);
	gstrftime(s,80,timefmt,(double)(x));
	fputs(conv_text(s), fp);
	putc('"', fp);
    } else {
	fprintf(fp,"%#g",x);
    }
}

void
save_axis_format(FILE *fp, AXIS_INDEX axis)
{
    fprintf(fp, 
	    (fp == stderr) ? "\t  %s-axis: \"%s\"%s\n" : "set format %s \"%s\" %s\n",
	     axis_name(axis), conv_text(axis_array[axis].formatstring),
	    axis_array[axis].tictype == DT_DMS ? "geographic" :
	    axis_array[axis].tictype == DT_TIMEDATE ? "timedate" :
	    "");
}

void
save_style_parallel(FILE *fp)
{
    if (fp == stderr)
	fputs("\t",fp);
    fprintf(fp, "set style parallel %s ",
	parallel_axis_style.layer == LAYER_BACK ? "back" : "front");
    save_linetype(fp, &(parallel_axis_style.lp_properties), FALSE);
    fprintf(fp, "\n");
}

void
save_style_spider(FILE *fp)
{
    fprintf(fp, "set style spiderplot ");
    save_linetype(fp, &(spiderplot_style.lp_properties), TRUE);
    fprintf(fp, "\nset style spiderplot fillstyle ");
    save_fillstyle(fp, &spiderplot_style.fillstyle);
}

void
save_style_textbox(FILE *fp)
{
    int bs;
    for (bs = 0; bs < NUM_TEXTBOX_STYLES; bs++) {
	textbox_style *textbox = &textbox_opts[bs];
	if (textbox->linewidth <= 0)
	    continue;
	fprintf(fp, "set style textbox ");
	if (bs > 0)
	    fprintf(fp,"%d ", bs);
	fprintf(fp, " %s margins %4.1f, %4.1f",
		textbox->opaque ? "opaque": "transparent",
		textbox->xmargin, textbox->ymargin);
	if (textbox->opaque) {
	    fprintf(fp, " fc ");
	    save_pm3dcolor(fp, &(textbox->fillcolor));
	}
	if (textbox->noborder) {
	    fprintf(fp, " noborder");
	} else {
	    fprintf(fp, " border ");
	    save_pm3dcolor(fp, &(textbox->border_color));
	}
	fprintf(fp, " linewidth %4.1f", textbox->linewidth);
	fputs("\n",fp);
    }
}

void
save_position(FILE *fp, struct position *pos, int ndim, TBOOLEAN offset)
{
    if (offset) {
	if (pos->x == 0 && pos->y == 0 && pos->z == 0)
	    return;
	fprintf(fp, " offset ");
    }

    /* Save in time coordinates if appropriate */
    if (pos->scalex == first_axes) {
	save_num_or_time_input(fp, pos->x, &axis_array[FIRST_X_AXIS]);
    } else {
	fprintf(fp, "%s%g", coord_msg[pos->scalex], pos->x);
    }

    if (ndim == 1)
	return;
    else
	fprintf(fp, ", ");

    if (pos->scaley == first_axes || pos->scalex == polar_axes) {
	if (pos->scaley != pos->scalex) fprintf(fp, "first ");
	save_num_or_time_input(fp, pos->y, &axis_array[FIRST_Y_AXIS]);
    } else {
	fprintf(fp, "%s%g", 
	    pos->scaley == pos->scalex ? "" : coord_msg[pos->scaley], pos->y);
    }

    if (ndim == 2)
	return;
    else
	fprintf(fp, ", ");

    if (pos->scalez == first_axes) {
	if (pos->scalez != pos->scaley) fprintf(fp, "first ");
	save_num_or_time_input(fp, pos->z, &axis_array[FIRST_Z_AXIS]);
    } else {
	fprintf(fp, "%s%g", 
	    pos->scalez == pos->scaley ? "" : coord_msg[pos->scalez], pos->z);
    }
}


void
save_prange(FILE *fp, struct axis *this_axis)
{
    TBOOLEAN noextend = FALSE;

    fprintf(fp, "set %srange [ ", axis_name(this_axis->index));
    if (this_axis->set_autoscale & AUTOSCALE_MIN) {
	if (this_axis->min_constraint & CONSTRAINT_LOWER ) {
	    save_num_or_time_input(fp, this_axis->min_lb, this_axis);
	    fputs(" < ", fp);
	}
	putc('*', fp);
	if (this_axis->min_constraint & CONSTRAINT_UPPER ) {
	    fputs(" < ", fp);
	    save_num_or_time_input(fp, this_axis->min_ub, this_axis);
	}
    } else {
	save_num_or_time_input(fp, this_axis->set_min, this_axis);
    }
    fputs(" : ", fp);
    if (this_axis->set_autoscale & AUTOSCALE_MAX) {
	if (this_axis->max_constraint & CONSTRAINT_LOWER ) {
	    save_num_or_time_input(fp, this_axis->max_lb, this_axis);
	    fputs(" < ", fp);
	}
	putc('*', fp);
	if (this_axis->max_constraint & CONSTRAINT_UPPER ) {
	    fputs(" < ", fp);
	    save_num_or_time_input(fp, this_axis->max_ub, this_axis);
	}
    } else {
	save_num_or_time_input(fp, this_axis->set_max, this_axis);
    }

    if (this_axis->index < PARALLEL_AXES)
	fprintf(fp, " ] %sreverse %swriteback",
	    ((this_axis->range_flags & RANGE_IS_REVERSED)) ? "" : "no",
	    this_axis->range_flags & RANGE_WRITEBACK ? "" : "no");
    else
	fprintf(fp, " ] ");

    if ((this_axis->set_autoscale & AUTOSCALE_FIXMIN)
    &&  (this_axis->set_autoscale & AUTOSCALE_FIXMAX)) {
	fprintf(fp, " noextend");
	noextend = TRUE;
    }

    if (this_axis->set_autoscale && fp == stderr) {
	/* add current (hidden) range as comments */
	fputs("  # (currently [", fp);
	save_num_or_time_input(fp, this_axis->min, this_axis);
	putc(':', fp);
	save_num_or_time_input(fp, this_axis->max, this_axis);
	fputs("] )\n", fp);
    } else
	putc('\n', fp);

    if (!noextend && (fp != stderr)) {
	if (this_axis->set_autoscale & (AUTOSCALE_FIXMIN))
	    fprintf(fp, "set autoscale %sfixmin\n", axis_name(this_axis->index));
	if (this_axis->set_autoscale & AUTOSCALE_FIXMAX)
	    fprintf(fp, "set autoscale %sfixmax\n", axis_name(this_axis->index));
    }
}

void
save_link(FILE *fp, AXIS *this_axis)
{
    if (this_axis->linked_to_primary
    &&  this_axis->index != -this_axis->linked_to_primary->index) {
	fprintf(fp, "set link %s ", axis_name(this_axis->index));
	if (this_axis->link_udf->at)
	    fprintf(fp, "via %s ", this_axis->link_udf->definition);
	if (this_axis->linked_to_primary->link_udf->at)
	    fprintf(fp, "inverse %s ", this_axis->linked_to_primary->link_udf->definition);
	fputs("\n", fp);
    }
}

void
save_nonlinear(FILE *fp, AXIS *this_axis)
{
    AXIS *primary = this_axis->linked_to_primary;

    if (primary &&  this_axis->index == -primary->index) {
	fprintf(fp, "set nonlinear %s ", axis_name(this_axis->index));
	if (primary->link_udf->at)
	    fprintf(fp, "via %s ", primary->link_udf->definition);
	else
	    fprintf(stderr, "[corrupt linkage] ");
	if (this_axis->link_udf->at)
	    fprintf(fp, "inverse %s ", this_axis->link_udf->definition);
	else
	    fprintf(stderr, "[corrupt linkage] ");
	fputs("\n", fp);
    }
}

static void
save_zeroaxis(FILE *fp, AXIS_INDEX axis)
{
    if (axis_array[axis].zeroaxis == NULL) {
	fprintf(fp, "unset %szeroaxis", axis_name(axis));
    } else {
	fprintf(fp, "set %szeroaxis", axis_name(axis));
	if (axis_array[axis].zeroaxis != &default_axis_zeroaxis)
	    save_linetype(fp, axis_array[axis].zeroaxis, FALSE);
    }
    putc('\n', fp);
}

void
save_fillstyle(FILE *fp, const struct fill_style_type *fs)
{
    switch(fs->fillstyle) {
    case FS_SOLID:
    case FS_TRANSPARENT_SOLID:
	fprintf(fp, " %s solid %.2f ",
		fs->fillstyle == FS_SOLID ? "" : "transparent",
		fs->filldensity / 100.0);
	break;
    case FS_PATTERN:
    case FS_TRANSPARENT_PATTERN:
	fprintf(fp, " %s pattern %d ",
		fs->fillstyle == FS_PATTERN ? "" : "transparent",
		fs->fillpattern);
	break;
    case FS_DEFAULT:
	fprintf(fp, " default\n");
	return;
    default:
	fprintf(fp, " empty ");
	break;
    }
    if (fs->border_color.type == TC_LT && fs->border_color.lt == LT_NODRAW) {
	fprintf(fp, "noborder\n");
    } else {
	fprintf(fp, "border");
	save_pm3dcolor(fp, &fs->border_color);
	fprintf(fp, "\n");
    }
}

void
save_textcolor(FILE *fp, const struct t_colorspec *tc)
{
    if (tc->type) {
	fprintf(fp, " textcolor");
	if (tc->type == TC_VARIABLE)
	    fprintf(fp, " variable");
	else
	    save_pm3dcolor(fp, tc);
    }
}


void
save_pm3dcolor(FILE *fp, const struct t_colorspec *tc)
{
    if (tc->type) {
	switch(tc->type) {
	case TC_LT:   if (tc->lt == LT_NODRAW)
			fprintf(fp," nodraw");
		      else if (tc->lt == LT_BACKGROUND)
			fprintf(fp," bgnd");
		      else
			fprintf(fp," lt %d", tc->lt+1);
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
		      if (tc->value < 0)
		  	fprintf(fp," rgb variable ");
		      else if (color)
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
    char *answer = strdup(reverse_table_lookup(plotstyle_tbl, style));
    char *idollar = strchr(answer, '$');
    if (idollar) {
	do {
	    *idollar = *(idollar+1);
	    idollar++;
	} while (*idollar);

    }
    fputs(answer, fp);
    free(answer);

    if (style == FILLEDCURVES) {
	fputs(" ", fp);
	if (!strcmp(which, "data") || !strcmp(which, "Data"))
	    filledcurves_options_tofile(&filledcurves_opts_data, fp);
	else
	    filledcurves_options_tofile(&filledcurves_opts_func, fp);
    }
    fputc('\n', fp);
}

void
save_dashtype(FILE *fp, int d_type, const t_dashtype *dt)
{
    /* this is indicated by LT_AXIS (lt 0) instead */
    if (d_type == DASHTYPE_AXIS)
	return;

    fprintf(fp, " dashtype");
    if (d_type == DASHTYPE_CUSTOM) {
	if (dt->dstring[0] != '\0')
	    fprintf(fp, " \"%s\"", dt->dstring);
	if (fp == stderr || dt->dstring[0] == '\0') {
	    int i;
	    fputs(" (", fp);
	    for (i = 0; i < DASHPATTERN_LENGTH && dt->pattern[i] > 0; i++)
		fprintf(fp, i ? ", %.2f" : "%.2f", dt->pattern[i]);
	    fputs(")", fp);
	}
    } else if (d_type == DASHTYPE_SOLID) {
	fprintf(fp, " solid");
    } else {
	fprintf(fp, " %d", d_type + 1);
    }
}

void
save_linetype(FILE *fp, lp_style_type *lp, TBOOLEAN show_point)
{
    if (lp->l_type == LT_NODRAW)
	fprintf(fp, " lt nodraw");
    else if (lp->l_type == LT_BACKGROUND)
	fprintf(fp, " lt bgnd");
    else if (lp->l_type == LT_DEFAULT)
	; /* Dont' print anything */
    else if (lp->l_type == LT_AXIS)
	fprintf(fp, " lt 0");

    if (lp->l_type == LT_BLACK && lp->pm3d_color.type == TC_LT)
	fprintf(fp, " lt black");
    else if (lp->pm3d_color.type != TC_DEFAULT) {
	fprintf(fp, " linecolor");
	if (lp->pm3d_color.type == TC_LT)
	    fprintf(fp, " %d", lp->pm3d_color.lt+1);
	else if (lp->pm3d_color.type == TC_LINESTYLE && lp->l_type == LT_COLORFROMCOLUMN)
	    fprintf(fp, " variable");
	else
	    save_pm3dcolor(fp,&(lp->pm3d_color));
    }
    fprintf(fp, " linewidth %.3f", lp->l_width);

    save_dashtype(fp, lp->d_type, &lp->custom_dash_pattern);

    if (show_point) {
	if (lp->p_type == PT_CHARACTER)
	    fprintf(fp, " pointtype \"%s\"", lp->p_char);
	else if (lp->p_type == PT_VARIABLE)
	    fprintf(fp, " pointtype variable");
	else
	    fprintf(fp, " pointtype %d", lp->p_type + 1);
	if (lp->p_size == PTSZ_VARIABLE)
	    fprintf(fp, " pointsize variable");
	else if (lp->p_size == PTSZ_DEFAULT)
	    fprintf(fp, " pointsize default");
	else
	    fprintf(fp, " pointsize %.3f", lp->p_size);
	if (lp->p_interval != 0)
	    fprintf(fp, " pointinterval %d", lp->p_interval);
	if (lp->p_number != 0)
	    fprintf(fp, " pointnumber %d", lp->p_number);
    }

}


void
save_offsets(FILE *fp, char *lead)
{
    fprintf(fp, "%s %s%g, %s%g, %s%g, %s%g\n", lead,
	loff.scalex == graph ? "graph " : "", loff.x,
	roff.scalex == graph ? "graph " : "", roff.x,
	toff.scaley == graph ? "graph " : "", toff.y,
	boff.scaley == graph ? "graph " : "", boff.y);
}

void
save_bars(FILE *fp)
{
    if (bar_size == 0.0) {
	fprintf(fp, "unset errorbars\n");
	return;
    }
    fprintf(fp, "set errorbars %s", (bar_layer == LAYER_BACK) ? "back" : "front");
    if (bar_size > 0.0)
	fprintf(fp, " %f ", bar_size);
    else
	fprintf(fp," fullwidth ");
    if ((bar_lp.flags & LP_ERRORBAR_SET) != 0)
	save_linetype(fp, &bar_lp, FALSE);
    fputs("\n",fp);
}

void 
save_histogram_opts (FILE *fp)
{
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
    if (fp == stderr)
	fprintf(fp,"\n\t\t");
    fprintf(fp,"title");
    save_textcolor(fp, &histogram_opts.title.textcolor);
    if (histogram_opts.title.font)
	fprintf(fp, " font \"%s\" ", histogram_opts.title.font);
    save_position(fp, &histogram_opts.title.offset, 2, TRUE);
    if (!histogram_opts.keyentry)
	fprintf(fp, " nokeyseparators");
    fprintf(fp, "\n");
}

void
save_pixmaps(FILE *fp)
{
    t_pixmap *pixmap;
    for (pixmap = pixmap_listhead; pixmap; pixmap = pixmap->next) {
	fprintf(fp, "set pixmap %d '%s' # (%d x %d pixmap)\n",
		pixmap->tag, pixmap->filename, pixmap->ncols, pixmap->nrows);
	fprintf(fp, "set pixmap %d at ", pixmap->tag);
	save_position(fp,&pixmap->pin, 3, FALSE);
	fprintf(fp, "  size ");
	save_position(fp,&pixmap->extent, 2, FALSE);
	fprintf(fp, " %s %s\n",
		pixmap->layer == LAYER_FRONT ? "front" : "behind",
		pixmap->center ? "center" : "");
    }
}

/* Save/show rectangle <tag> (0 means show all) */
void
save_object(FILE *fp, int tag)
{
    t_object *this_object;
    t_rectangle *this_rect;
    t_circle *this_circle;
    t_ellipse *this_ellipse;
    TBOOLEAN showed = FALSE;

    for (this_object = first_object; this_object != NULL; this_object = this_object->next) {
	if ((this_object->object_type == OBJ_RECTANGLE)
	    && (tag == 0 || tag == this_object->tag)) {
	    this_rect = &this_object->o.rectangle;
	    showed = TRUE;
	    fprintf(fp, "%sobject %2d rect ", (fp==stderr) ? "\t" : "set ",this_object->tag);

	    if (this_rect->type == 1) {
		fprintf(fp, "center ");
		save_position(fp, &this_rect->center, 2, FALSE);
		fprintf(fp, " size ");
		save_position(fp, &this_rect->extent, 2, FALSE);
	    } else {
		fprintf(fp, "from ");
		save_position(fp, &this_rect->bl, 2, FALSE);
		fprintf(fp, " to ");
		save_position(fp, &this_rect->tr, 2, FALSE);
	    }
	}

	else if ((this_object->object_type == OBJ_CIRCLE)
	    && (tag == 0 || tag == this_object->tag)) {
	    struct position *e = &this_object->o.circle.extent;
	    this_circle = &this_object->o.circle;
	    showed = TRUE;
	    fprintf(fp, "%sobject %2d circle ", (fp==stderr) ? "\t" : "set ",this_object->tag);

	    fprintf(fp, "center ");
	    save_position(fp, &this_circle->center, 3, FALSE);
	    fprintf(fp, " size ");
	    fprintf(fp, "%s%g", e->scalex == first_axes ? "" : coord_msg[e->scalex], e->x);
	    fprintf(fp, " arc [%g:%g] ", this_circle->arc_begin, this_circle->arc_end);
	    fprintf(fp, this_circle->wedge ? "wedge " : "nowedge");
	}

	else if ((this_object->object_type == OBJ_ELLIPSE)
	    && (tag == 0 || tag == this_object->tag)) {
	    struct position *e = &this_object->o.ellipse.extent;
	    this_ellipse = &this_object->o.ellipse;
	    showed = TRUE;
	    fprintf(fp, "%sobject %2d ellipse ", (fp==stderr) ? "\t" : "set ",this_object->tag);
	    fprintf(fp, "center ");
	    save_position(fp, &this_ellipse->center, 3, FALSE);
	    fprintf(fp, " size ");
	    fprintf(fp, "%s%g", e->scalex == first_axes ? "" : coord_msg[e->scalex], e->x);
	    fprintf(fp, ", %s%g", e->scaley == e->scalex ? "" : coord_msg[e->scaley], e->y);
	    fprintf(fp, "  angle %g", this_ellipse->orientation);
	    fputs(" units ", fp);
	    switch (this_ellipse->type) {
		case ELLIPSEAXES_XY:
		    fputs("xy", fp);
		    break;
		case ELLIPSEAXES_XX:
		    fputs("xx", fp);
		    break;
		case ELLIPSEAXES_YY:
		    fputs("yy", fp);
		    break;
	    }
	}

	else if ((this_object->object_type == OBJ_POLYGON)
	    && (tag == 0 || tag == this_object->tag)) {
	    t_polygon *this_polygon = &this_object->o.polygon;
	    int nv;
	    showed = TRUE;
	    fprintf(fp, "%sobject %2d polygon ", (fp==stderr) ? "\t" : "set ",this_object->tag);
	    if (this_polygon->vertex) {
		fprintf(fp, "from ");
		save_position(fp, &this_polygon->vertex[0], 3, FALSE);
	    }
	    for (nv=1; nv < this_polygon->type; nv++) {
		fprintf(fp, (fp==stderr) ? "\n\t\t\t    to " : " to ");
		save_position(fp, &this_polygon->vertex[nv], 3, FALSE);
	    }
	}

	/* Properties common to all objects */
	if (tag == 0 || tag == this_object->tag) {
	    fprintf(fp, "\n%sobject %2d ", (fp==stderr) ? "\t" : "set ",this_object->tag);
	    fprintf(fp, "%s ", this_object->layer == LAYER_FRONT ? "front" :
				this_object->layer == LAYER_DEPTHORDER ? "depthorder" :
				this_object->layer == LAYER_BEHIND ? "behind" :
				"back");
	    if (this_object->clip == OBJ_NOCLIP)
		fputs("noclip ", fp);
	    else 
		fputs("clip ", fp);

	    if (this_object->lp_properties.l_width)
		    fprintf(fp, "lw %.1f ",this_object->lp_properties.l_width);
	    if (this_object->lp_properties.d_type)
		    save_dashtype(fp, this_object->lp_properties.d_type,
					&this_object->lp_properties.custom_dash_pattern);
	    fprintf(fp, " fc ");
	    if (this_object->lp_properties.l_type == LT_DEFAULT)
		    fprintf(fp,"default");
	    else
		    save_pm3dcolor(fp, &this_object->lp_properties.pm3d_color);
	    fprintf(fp, " fillstyle ");
	    save_fillstyle(fp, &this_object->fillstyle);
	}

    }
    if (tag > 0 && !showed)
	int_error(c_token, "object not found");
}

/* Save/show special polygon objects created by "set wall" */
void
save_walls(FILE *fp)
{
    static const char* wall_name[5] = {"y0", "x0", "y1", "x1", "z0"};
    t_object *this_object;
    int i;

    for (i = 0; i < 5; i++) {
    	this_object = &grid_wall[i];
	if (this_object->layer == LAYER_FRONTBACK) {
	    fprintf(fp, "set wall %s ", wall_name[i]);
	    fprintf(fp, " fc ");
	    save_pm3dcolor(fp, &this_object->lp_properties.pm3d_color);
	    fprintf(fp, " fillstyle ");
	    save_fillstyle(fp, &this_object->fillstyle);
	}

    }
}

