#ifndef lint
static char *RCSid() { return RCSid("$Id: save.c,v 1.11.2.6 2000/10/31 14:48:19 joze Exp $"); }
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

#include "command.h"
#include "eval.h"
#include "fit.h"
#include "gp_time.h"
#include "hidden3d.h"
#include "setshow.h"
#include "util.h"

/* HBB 990825 FIXME: how come these strings are only used for
 * _displaying_ encodings (-->no use in set.c) ? */
const char *encoding_names[] = {
    "default", "iso_8859_1", "cp437", "cp850", NULL };

static void save_functions__sub __PROTO((FILE *));
static void save_variables__sub __PROTO((FILE *));
static void save_tics __PROTO((FILE *, int, int, struct ticdef *, TBOOLEAN, const char *));
static void save_position __PROTO((FILE *, struct position *));
static void save_range __PROTO((FILE *, int, double, double, TBOOLEAN, const char *));
static void save_set_all __PROTO((FILE *));

#define SAVE_NUM_OR_TIME(fp, x, axis) \
do{if (datatype[axis]==TIME) { \
  char s[80]; char *p; \
  putc('"', fp);   \
  gstrftime(s,80,timefmt,(double)(x)); \
  for(p=s; *p; ++p) {\
   if ( *p == '\t' ) fputs("\\t",fp);\
   else if (*p == '\n') fputs("\\n",fp); \
   else if ( *p > 126 || *p < 32 ) fprintf(fp,"\\%03o",*p);\
   else putc(*p, fp);\
  }\
  putc('"', fp);\
 } else {\
  fprintf(fp,"%#g",x);\
}} while(0)


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
	    datatype[FIRST_X_AXIS] == TIME ? " time" : "",
	    datatype[FIRST_Y_AXIS] == TIME ? " time" : "",
	    datatype[FIRST_Z_AXIS] == TIME ? " time" : "",
	    datatype[SECOND_X_AXIS] == TIME ? " time" : "",
	    datatype[SECOND_Y_AXIS] == TIME ? " time" : "");

    if (boxwidth < 0.0)
	fputs("set boxwidth\n", fp);
    else
	fprintf(fp, "set boxwidth %g\n", boxwidth);
    if (dgrid3d)
	fprintf(fp, "set dgrid3d %d,%d, %d\n",
		dgrid3d_row_fineness,
		dgrid3d_col_fineness,
		dgrid3d_norm_value);

    fprintf(fp, "set dummy %s,%s\n", dummy_var[0], dummy_var[1]);
    fprintf(fp, "set format x \"%s\"\n", conv_text(xformat));
    fprintf(fp, "set format y \"%s\"\n", conv_text(yformat));
    fprintf(fp, "set format x2 \"%s\"\n", conv_text(x2format));
    fprintf(fp, "set format y2 \"%s\"\n", conv_text(y2format));
    fprintf(fp, "set format z \"%s\"\n", conv_text(zformat));
    fprintf(fp, "set angles %s\n",
	    (angles_format == ANGLES_RADIANS) ? "radians" : "degrees");

    if (work_grid.l_type == 0)
	fputs("unset grid\n", fp);
    else {
	/* FIXME */
	if (polar_grid_angle)	/* set angle already output */
	    fprintf(fp, "set grid polar %f\n", polar_grid_angle / ang2rad);
	else
	    fputs("unset grid polar\n", fp);
	fprintf(fp,
		"set grid %sxtics %sytics %sztics %sx2tics %sy2tics %smxtics %smytics %smztics %smx2tics %smy2tics lt %d lw %.3f, lt %d lw %.3f\n",
		work_grid.l_type & GRID_X ? "" : "no",
		work_grid.l_type & GRID_Y ? "" : "no",
		work_grid.l_type & GRID_Z ? "" : "no",
		work_grid.l_type & GRID_X2 ? "" : "no",
		work_grid.l_type & GRID_Y2 ? "" : "no",
		work_grid.l_type & GRID_MX ? "" : "no",
		work_grid.l_type & GRID_MY ? "" : "no",
		work_grid.l_type & GRID_MZ ? "" : "no",
		work_grid.l_type & GRID_MX2 ? "" : "no",
		work_grid.l_type & GRID_MY2 ? "" : "no",
		grid_lp.l_type + 1, grid_lp.l_width,
		mgrid_lp.l_type + 1, mgrid_lp.l_width);
    }
    fprintf(fp, "set key title \"%s\"\n", conv_text(key_title));
    switch (key) {
    case -1:{
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
	}
    case 0:
	fputs("unset key\n", fp);
	break;
    case 1:
	fputs("set key ", fp);
	save_position(fp, &key_user_pos);
	break;
    }
    if (key) {
	fprintf(fp, " %s %sreverse box linetype %d linewidth %.3f samplen %g spacing %g width %g\n",
		key_just == JLEFT ? "Left" : "Right",
		key_reverse ? "" : "no",
		key_box.l_type + 1, key_box.l_width, key_swidth, key_vert_factor, key_width_fix);
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
    if (is_log_x)
	fprintf(fp, "set logscale x %g\n", base_log_x);
    if (is_log_y)
	fprintf(fp, "set logscale y %g\n", base_log_y);
    if (is_log_z)
	fprintf(fp, "set logscale z %g\n", base_log_z);
    if (is_log_x2)
	fprintf(fp, "set logscale x2 %g\n", base_log_x2);
    if (is_log_y2)
	fprintf(fp, "set logscale y2 %g\n", base_log_y2);

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
    switch (levels_kind) {
    case LEVELS_AUTO:
	fprintf(fp, "auto %d\n", contour_levels);
	break;
    case LEVELS_INCREMENTAL:
	fprintf(fp, "incremental %g,%g,%g\n",
		levels_list[0], levels_list[1],
		levels_list[0] + levels_list[1] * contour_levels);
	break;
    case LEVELS_DISCRETE:
	{
	    int i;
	    fprintf(fp, "discrete %g", levels_list[0]);
	    for (i = 1; i < contour_levels; i++)
		fprintf(fp, ",%g ", levels_list[i]);
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

    fprintf(fp, "\
set xzeroaxis lt %d lw %.3f\n\
set x2zeroaxis lt %d lw %.3f\n\
set yzeroaxis lt %d lw %.3f\n\
set y2zeroaxis lt %d lw %.3f\n\
set tics %s\n\
set ticslevel %g\n\
set ticscale %g %g\n",
	    xzeroaxis.l_type + 1, xzeroaxis.l_width,
	    x2zeroaxis.l_type + 1, x2zeroaxis.l_width,
	    yzeroaxis.l_type + 1, yzeroaxis.l_width,
	    y2zeroaxis.l_type + 1, y2zeroaxis.l_width,
	    (tic_in) ? "in" : "out",
	    ticslevel,
	    ticscale, miniticscale);

#define SAVE_XYZLABEL(name,lab) { \
  fprintf(fp, "set %s \"%s\" %f,%f ", \
    name, conv_text(lab.text),lab.xoffset,lab.yoffset); \
  fprintf(fp, " \"%s\"\n", conv_text(lab.font)); \
}

#define SAVE_MINI(name,m,freq) switch(m&TICS_MASK) { \
 case 0: fprintf(fp, "set no%s\n", name); break; \
 case MINI_AUTO: fprintf(fp, "set %s\n",name); break; \
 case MINI_DEFAULT: fprintf(fp, "set %s default\n",name); break; \
 case MINI_USER: fprintf(fp, "set %s %f\n", name, freq); break; \
}
    SAVE_MINI("mxtics", mxtics, mxtfreq)
    SAVE_MINI("mytics", mytics, mytfreq)
    SAVE_MINI("mx2tics", mx2tics, mx2tfreq)
    SAVE_MINI("my2tics", my2tics, my2tfreq)

    save_tics(fp, xtics, FIRST_X_AXIS, &xticdef, rotate_xtics, "x");
    save_tics(fp, ytics, FIRST_Y_AXIS, &yticdef, rotate_ytics, "y");
    save_tics(fp, ztics, FIRST_Z_AXIS, &zticdef, rotate_ztics, "z");
    save_tics(fp, x2tics, SECOND_X_AXIS, &x2ticdef, rotate_x2tics, "x2");
    save_tics(fp, y2tics, SECOND_Y_AXIS, &y2ticdef, rotate_y2tics, "y2");

    SAVE_XYZLABEL("title", title);

    /* FIXME */
    fprintf(fp, "set %s \"%s\" %s %srotate %f,%f ",
	    "timestamp", conv_text(timelabel.text),
	    (timelabel_bottom ? "bottom" : "top"),
	    (timelabel_rotate ? "" : "no"),
	    timelabel.xoffset, timelabel.yoffset);
    fprintf(fp, " \"%s\"\n", conv_text(timelabel.font));

    save_range(fp, R_AXIS, rmin, rmax, autoscale_r, "r");
    save_range(fp, T_AXIS, tmin, tmax, autoscale_t, "t");
    save_range(fp, U_AXIS, umin, umax, autoscale_u, "u");
    save_range(fp, V_AXIS, vmin, vmax, autoscale_v, "v");

    SAVE_XYZLABEL("xlabel", xlabel);
    SAVE_XYZLABEL("x2label", x2label);

    if (strlen(timefmt)) {
	fprintf(fp, "set timefmt \"%s\"\n", conv_text(timefmt));
    }
    save_range(fp, FIRST_X_AXIS, xmin, xmax, autoscale_x, "x");
    save_range(fp, SECOND_X_AXIS, x2min, x2max, autoscale_x2, "x2");

    SAVE_XYZLABEL("ylabel", ylabel);
    SAVE_XYZLABEL("y2label", y2label);

    save_range(fp, FIRST_Y_AXIS, ymin, ymax, autoscale_y, "y");
    save_range(fp, SECOND_Y_AXIS, y2min, y2max, autoscale_y2, "y2");

    SAVE_XYZLABEL("zlabel", zlabel);
    save_range(fp, FIRST_Z_AXIS, zmin, zmax, autoscale_z, "z");

    fprintf(fp, "set zero %g\n", zero);
    fprintf(fp, "set lmargin %d\nset bmargin %d\nset rmargin %d\nset tmargin %d\n",
	    lmargin, bmargin, rmargin, tmargin);

    fprintf(fp, "set locale \"%s\"\n", get_locale());

    fputs("set loadpath ", fp);
    {
	char *s;
	while ((s = save_loadpath()) != NULL)
	    fprintf(fp, "\"%s\" ", s);
	fputc('\n', fp);
    }
}

static void
save_tics(fp, where, axis, tdef, rotate, text)
FILE *fp;
int where;
int axis;
struct ticdef *tdef;
TBOOLEAN rotate;
const char *text;
{

    if (where == NO_TICS) {
	fprintf(fp, "set no%stics\n", text);
	return;
    }
    fprintf(fp, "set %stics %s %smirror %srotate ", text,
	    (where & TICS_MASK) == TICS_ON_AXIS ? "axis" : "border",
	    (where & TICS_MIRROR) ? "" : "no", rotate ? "" : "no");
    switch (tdef->type) {
    case TIC_COMPUTED:{
	    fputs("autofreq ", fp);
	    break;
	}
    case TIC_MONTH:{
	    fprintf(fp, "\nset %smtics", text);
	    break;
	}
    case TIC_DAY:{
	    fprintf(fp, "\nset %cdtics", axis);
	    break;
	}
    case TIC_SERIES:
	if (datatype[axis] == TIME) {
	    if (tdef->def.series.start != -VERYLARGE) {
		char fd[26];
		gstrftime(fd, 24, timefmt, (double) tdef->def.series.start);
		fprintf(fp, "\"%s\",", conv_text(fd));
	    }
	    fprintf(fp, "%g", tdef->def.series.incr);

	    if (tdef->def.series.end != VERYLARGE) {
		char td[26];
		gstrftime(td, 24, timefmt, (double) tdef->def.series.end);
		fprintf(fp, ",\"%s\"", conv_text(td));
	    }
	} else {		/* !TIME */

	    if (tdef->def.series.start != -VERYLARGE)
		fprintf(fp, "%g,", tdef->def.series.start);
	    fprintf(fp, "%g", tdef->def.series.incr);
	    if (tdef->def.series.end != VERYLARGE)
		fprintf(fp, ",%g", tdef->def.series.end);
	}

	break;

    case TIC_USER:{
	    register struct ticmark *t;
	    int flag_time;
	    flag_time = (datatype[axis] == TIME);
	    fputs(" (", fp);
	    for (t = tdef->def.user; t != NULL; t = t->next) {
		if (t->label)
		    fprintf(fp, "\"%s\" ", conv_text(t->label));
		if (flag_time) {
		    char td[26];
		    gstrftime(td, 24, timefmt, (double) t->position);
		    fprintf(fp, "\"%s\"", conv_text(td));
		} else {
		    fprintf(fp, "%g", t->position);
		}
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
save_range(fp, axis, min, max, autosc, text)
FILE *fp;
int axis;
double min, max;
TBOOLEAN autosc;
const char *text;
{
    int i;

    i = axis;
    fprintf(fp, "set %srange [ ", text);
    if (autosc & 1) {
	putc('*', fp);
    } else {
	SAVE_NUM_OR_TIME(fp, min, axis);
    }
    fputs(" : ", fp);
    if (autosc & 2) {
	putc('*', fp);
    } else {
	SAVE_NUM_OR_TIME(fp, max, axis);
    }

    fprintf(fp, " ] %sreverse %swriteback",
	    range_flags[axis] & RANGE_REVERSE ? "" : "no",
	    range_flags[axis] & RANGE_WRITEBACK ? "" : "no");

    if (autosc) {
	/* add current (hidden) range as comments */
	fputs("  # (currently [", fp);
	if (autosc & 1) {
	    SAVE_NUM_OR_TIME(fp, min, axis);
	}
	putc(':', fp);
	if (autosc & 2) {
	    SAVE_NUM_OR_TIME(fp, max, axis);
	}
	fputs("] )", fp);
    }
    putc('\n', fp);
}

