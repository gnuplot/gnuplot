#ifndef lint
static char *RCSid = "$Id: misc.c,v 1.79 1998/04/14 00:16:02 drd Exp $";
#endif

/* GNUPLOT - misc.c */

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

#ifdef AMIGA_AC_5
# include <exec/types.h>
#endif /* AMIGA_AC_5 */

#include "plot.h"
#include "setshow.h"

extern int key_vpos, key_hpos, key_just;
extern int datatype[];
extern char timefmt[];

static void save_range __PROTO((FILE * fp, int axis, double min, double max, int autosc, char *text));
static void save_tics __PROTO((FILE * fp, int where, int axis, struct ticdef * tdef, TBOOLEAN rotate, char *text));
static void save_position __PROTO((FILE * fp, struct position * pos));
static void save_functions__sub __PROTO((FILE * fp));
static void save_variables__sub __PROTO((FILE * fp));
static int lf_pop __PROTO((void));
static void lf_push __PROTO((FILE * fp));
static int find_maxl_cntr __PROTO((struct gnuplot_contours * contours, int *count));


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
  fprintf(fp,"%g",x);\
}} while(0)


/* State information for load_file(), to recover from errors
 * and properly handle recursive load_file calls
 */
typedef struct lf_state_struct LFS;
struct lf_state_struct {
    FILE *fp;			/* file pointer for load file */
    char *name;			/* name of file */
    TBOOLEAN interactive;	/* value of interactive flag on entry */
    TBOOLEAN do_load_arg_substitution;	/* likewise ... */
    int inline_num;		/* inline_num on entry */
    LFS *prev;			/* defines a stack */
    char *call_args[10];	/* args when file is 'call'ed instead of 'load'ed */
} *lf_head = NULL;		/* NULL if not in load_file */

/* these two could be in load_file, except for error recovery */
extern TBOOLEAN do_load_arg_substitution;
extern char *call_args[10];

/*
 * cp_alloc() allocates a curve_points structure that can hold 'num'
 * points.
 */
struct curve_points *
 cp_alloc(num)
int num;
{
    struct curve_points *cp;

    cp = (struct curve_points *) gp_alloc((unsigned long) sizeof(struct curve_points), "curve");
    cp->p_max = (num >= 0 ? num : 0);

    if (num > 0) {
	cp->points = (struct coordinate GPHUGE *)
	    gp_alloc((unsigned long) num * sizeof(struct coordinate), "curve points");
    } else
	cp->points = (struct coordinate GPHUGE *) NULL;
    cp->next_cp = NULL;
    cp->title = NULL;
    return (cp);
}


/*
 * cp_extend() reallocates a curve_points structure to hold "num"
 * points. This will either expand or shrink the storage.
 */
void cp_extend(cp, num)
struct curve_points *cp;
int num;
{

#if defined(DOS16) || defined(WIN16)
    /* Make sure we do not allocate more than 64k points in msdos since 
       * indexing is done with 16-bit int
       * Leave some bytes for malloc maintainance.
     */
    if (num > 32700)
	int_error("Array index must be less than 32k in msdos", NO_CARET);
#endif /* MSDOS */

    if (num == cp->p_max)
	return;

    if (num > 0) {
	if (cp->points == NULL) {
	    cp->points = (struct coordinate GPHUGE *)
		gp_alloc((unsigned long) num * sizeof(struct coordinate), "curve points");
	} else {
	    cp->points = (struct coordinate GPHUGE *)
		gp_realloc(cp->points, (unsigned long) num * sizeof(struct coordinate), "expanding curve points");
	}
	cp->p_max = num;
    } else {
	if (cp->points != (struct coordinate GPHUGE *) NULL)
	    free(cp->points);
	cp->points = (struct coordinate GPHUGE *) NULL;
	cp->p_max = 0;
    }
}

/*
 * cp_free() releases any memory which was previously malloc()'d to hold
 *   curve points (and recursively down the linked list).
 */
void cp_free(cp)
struct curve_points *cp;
{
    if (cp) {
	cp_free(cp->next_cp);
	if (cp->title)
	    free((char *) cp->title);
	if (cp->points)
	    free((char *) cp->points);
	free((char *) cp);
    }
}

/*
 * iso_alloc() allocates a iso_curve structure that can hold 'num'
 * points.
 */
struct iso_curve *
 iso_alloc(num)
int num;
{
    struct iso_curve *ip;
    ip = (struct iso_curve *) gp_alloc((unsigned long) sizeof(struct iso_curve), "iso curve");
    ip->p_max = (num >= 0 ? num : 0);
    if (num > 0) {
	ip->points = (struct coordinate GPHUGE *)
	    gp_alloc((unsigned long) num * sizeof(struct coordinate), "iso curve points");
    } else
	ip->points = (struct coordinate GPHUGE *) NULL;
    ip->next = NULL;
    return (ip);
}

/*
 * iso_extend() reallocates a iso_curve structure to hold "num"
 * points. This will either expand or shrink the storage.
 */
void iso_extend(ip, num)
struct iso_curve *ip;
int num;
{
    if (num == ip->p_max)
	return;

#if defined(DOS16) || defined(WIN16)
    /* Make sure we do not allocate more than 64k points in msdos since 
       * indexing is done with 16-bit int
       * Leave some bytes for malloc maintainance.
     */
    if (num > 32700)
	int_error("Array index must be less than 32k in msdos", NO_CARET);
#endif /* 16bit (Win)Doze */

    if (num > 0) {
	if (ip->points == NULL) {
	    ip->points = (struct coordinate GPHUGE *)
		gp_alloc((unsigned long) num * sizeof(struct coordinate), "iso curve points");
	} else {
	    ip->points = (struct coordinate GPHUGE *)
		gp_realloc(ip->points, (unsigned long) num * sizeof(struct coordinate), "expanding curve points");
	}
	ip->p_max = num;
    } else {
	if (ip->points != (struct coordinate GPHUGE *) NULL)
	    free(ip->points);
	ip->points = (struct coordinate GPHUGE *) NULL;
	ip->p_max = 0;
    }
}

/*
 * iso_free() releases any memory which was previously malloc()'d to hold
 *   iso curve points.
 */
void iso_free(ip)
struct iso_curve *ip;
{
    if (ip) {
	if (ip->points)
	    free((char *) ip->points);
	free((char *) ip);
    }
}

/*
 * sp_alloc() allocates a surface_points structure that can hold 'num_iso_1'
 * iso-curves with 'num_samp_2' samples and 'num_iso_2' iso-curves with
 * 'num_samp_1' samples.
 * If, however num_iso_2 or num_samp_1 is zero no iso curves are allocated.
 */
struct surface_points *
 sp_alloc(num_samp_1, num_iso_1, num_samp_2, num_iso_2)
int num_samp_1, num_iso_1, num_samp_2, num_iso_2;
{
    struct surface_points *sp;

    sp = (struct surface_points *) gp_alloc((unsigned long) sizeof(struct surface_points), "surface");
    sp->next_sp = NULL;
    sp->title = NULL;
    sp->contours = NULL;
    sp->iso_crvs = NULL;
    sp->num_iso_read = 0;

    if (num_iso_2 > 0 && num_samp_1 > 0) {
	int i;
	struct iso_curve *icrv;

	for (i = 0; i < num_iso_1; i++) {
	    icrv = iso_alloc(num_samp_2);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
	for (i = 0; i < num_iso_2; i++) {
	    icrv = iso_alloc(num_samp_1);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
    } else
	sp->iso_crvs = (struct iso_curve *) NULL;

    return (sp);
}

/*
 * sp_replace() updates a surface_points structure so it can hold 'num_iso_1'
 * iso-curves with 'num_samp_2' samples and 'num_iso_2' iso-curves with
 * 'num_samp_1' samples.
 * If, however num_iso_2 or num_samp_1 is zero no iso curves are allocated.
 */
void sp_replace(sp, num_samp_1, num_iso_1, num_samp_2, num_iso_2)
struct surface_points *sp;
int num_samp_1, num_iso_1, num_samp_2, num_iso_2;
{
    int i;
    struct iso_curve *icrv, *icrvs = sp->iso_crvs;

    while (icrvs) {
	icrv = icrvs;
	icrvs = icrvs->next;
	iso_free(icrv);
    }
    sp->iso_crvs = NULL;

    if (num_iso_2 > 0 && num_samp_1 > 0) {
	for (i = 0; i < num_iso_1; i++) {
	    icrv = iso_alloc(num_samp_2);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
	for (i = 0; i < num_iso_2; i++) {
	    icrv = iso_alloc(num_samp_1);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
    } else
	sp->iso_crvs = (struct iso_curve *) NULL;
}

/*
 * sp_free() releases any memory which was previously malloc()'d to hold
 *   surface points.
 */
void sp_free(sp)
struct surface_points *sp;
{
    if (sp) {
	sp_free(sp->next_sp);
	if (sp->title)
	    free((char *) sp->title);
	if (sp->contours) {
	    struct gnuplot_contours *cntr, *cntrs = sp->contours;

	    while (cntrs) {
		cntr = cntrs;
		cntrs = cntrs->next;
		free(cntr->coords);
		free(cntr);
	    }
	}
	if (sp->iso_crvs) {
	    struct iso_curve *icrv, *icrvs = sp->iso_crvs;

	    while (icrvs) {
		icrv = icrvs;
		icrvs = icrvs->next;
		iso_free(icrv);
	    }
	}
	free((char *) sp);
    }
}


/*
 *  functions corresponding to the arguments of the GNUPLOT `save` command
 */
void save_functions(fp)
FILE *fp;
{
    if (fp) {
	show_version(fp);		/* I _love_ information written */
	save_functions__sub(fp);	/* at the top and the end of an */
	fputs("#    EOF\n", fp);	/* human readable ASCII file.   */
	(void) fclose(fp);	/*                        (JFi) */
    } else
	os_error("Cannot open save file", c_token);
}


void save_variables(fp)
FILE *fp;
{
    if (fp) {
	show_version(fp);
	save_variables__sub(fp);
	fputs("#    EOF\n", fp);
	(void) fclose(fp);
    } else
	os_error("Cannot open save file", c_token);
}


void save_set(fp)
FILE *fp;
{
    if (fp) {
	show_version(fp);
	save_set_all(fp);
	fputs("#    EOF\n", fp);
	(void) fclose(fp);
    } else
	os_error("Cannot open save file", c_token);
}


void save_all(fp)
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
	(void) fclose(fp);
    } else
	os_error("Cannot open save file", c_token);
}

/*
 *  auxiliary functions
 */

static void save_functions__sub(fp)
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

static void save_variables__sub(fp)
FILE *fp;
{
    register struct udvt_entry *udv = first_udv->next_udv;	/* always skip pi */

    while (udv) {
	if (!udv->udv_undef) {
	    fprintf(fp, "%s = ", udv->udv_name);
	    disp_value(fp, &(udv->udv_value));
	    (void) putc('\n', fp);
	}
	udv = udv->next_udv;
    }
}

void save_set_all(fp)
FILE *fp;
{
    struct text_label *this_label;
    struct arrow_def *this_arrow;
    struct linestyle_def *this_linestyle;
    char str[MAX_LINE_LEN+1];

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
set %sclip points\n\
set %sclip one\n\
set %sclip two\n\
set bar %f\n",
	    (clip_points) ? "" : "no",
	    (clip_lines1) ? "" : "no",
	    (clip_lines2) ? "" : "no",
	    bar_size);

    if (draw_border)
	/* HBB 980609: handle border linestyle, too */
	fprintf(fp, "set border %d lt %d lw %.3f\n", draw_border, border_lp.l_type + 1, border_lp.l_width);
    else
	fprintf(fp, "set noborder\n");

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

    fprintf(fp, "set dummy %s,%s\n",dummy_var[0], dummy_var[1]);
    fprintf(fp, "set format x \"%s\"\n", conv_text(str, xformat));
    fprintf(fp, "set format y \"%s\"\n", conv_text(str, yformat));
    fprintf(fp, "set format x2 \"%s\"\n", conv_text(str, x2format));
    fprintf(fp, "set format y2 \"%s\"\n", conv_text(str, y2format));
    fprintf(fp, "set format z \"%s\"\n", conv_text(str, zformat));
    fprintf(fp, "set angles %s\n",
	    (angles_format == ANGLES_RADIANS) ? "radians" : "degrees");

    if (work_grid.l_type == 0)
	fputs("set nogrid\n", fp);
    else {
	if (polar_grid_angle)	/* set angle already output */
	    fprintf(fp, "set grid polar %f\n", polar_grid_angle / ang2rad);
	else
	    fputs("set grid nopolar\n", fp);
	fprintf(fp, "set grid %sxtics %sytics %sztics %sx2tics %sy2tics %smxtics %smytics %smztics %smx2tics %smy2tics lt %d lw %.3f, lt %d lw %.3f\n",
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
    fprintf(fp, "set key title \"%s\"\n", conv_text(str, key_title));
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
	fputs("set nokey\n", fp);
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
    fputs("set nolabel\n", fp);
    for (this_label = first_label; this_label != NULL;
	 this_label = this_label->next) {
	fprintf(fp, "set label %d \"%s\" at ",
		this_label->tag,
		conv_text(str, this_label->text));
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
	/* Entry font added by DJL */
	fputc('\n', fp);
    }
    fputs("set noarrow\n", fp);
    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next) {
	fprintf(fp, "set arrow %d from ", this_arrow->tag);
	save_position(fp, &this_arrow->start);
	fputs(" to ", fp);
	save_position(fp, &this_arrow->end);
	fprintf(fp, " %s linetype %d linewidth %.3f\n",
		this_arrow->head ? "" : " nohead",
		this_arrow->lp_properties.l_type + 1,
		this_arrow->lp_properties.l_width);
    }
    fputs("set nolinestyle\n", fp);
    for (this_linestyle = first_linestyle; this_linestyle != NULL;
	 this_linestyle = this_linestyle->next) {
	fprintf(fp, "set linestyle %d ", this_linestyle->tag);
	fprintf(fp, "linetype %d linewidth %.3f pointtype %d pointsize %.3f\n",
		this_linestyle->lp_properties.l_type + 1,
		this_linestyle->lp_properties.l_width,
		this_linestyle->lp_properties.p_type + 1,
		this_linestyle->lp_properties.p_size);
    }
    fputs("set nologscale\n", fp);
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

    fprintf(fp, "\
set offsets %g, %g, %g, %g\n\
set pointsize %g\n\
set encoding %s\n\
set %spolar\n\
set %sparametric\n",
	    loff, roff, toff, boff,
	    pointsize,
	    encoding_names[encoding],
	    (polar) ? "" : "no",
	    (parametric) ? "" : "no");
    fprintf(fp, "\
set view %g, %g, %g, %g\n\
set samples %d, %d\n\
set isosamples %d, %d\n\
set %ssurface\n\
set %scontour",
	    surface_rot_x, surface_rot_z, surface_scale, surface_zscale,
	    samples_1, samples_2,
	    iso_samples_1, iso_samples_2,
	    (draw_surface) ? "" : "no",
	    (draw_contour) ? "" : "no");

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
	fputs("set noclabel\n", fp);

    fputs("set mapping ", fp);
    switch(mapping3d) {
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
set data style ",
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
    fputs("set function style ", fp);
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
        /* HBB: default case demanded by gcc, still needed ?? */
	fputs("---error!---\n", fp);
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
    name, conv_text(str,lab.text),lab.xoffset,lab.yoffset); \
  fprintf(fp, " \"%s\"\n", conv_text(str, lab.font)); \
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

    fprintf(fp, "set %s \"%s\" %s %srotate %f,%f ",
	    "timestamp", conv_text(str, timelabel.text),
	    (timelabel_bottom ? "bottom" : "top"),
	    (timelabel_rotate ? "" : "no"),
	    timelabel.xoffset, timelabel.yoffset);
    fprintf(fp, " \"%s\"\n", conv_text(str, timelabel.font));

    save_range(fp, R_AXIS, rmin, rmax, autoscale_r, "r");
    save_range(fp, T_AXIS, tmin, tmax, autoscale_t, "t");
    save_range(fp, U_AXIS, umin, umax, autoscale_u, "u");
    save_range(fp, V_AXIS, vmin, vmax, autoscale_v, "v");

    SAVE_XYZLABEL("xlabel", xlabel);
    SAVE_XYZLABEL("x2label", x2label);

    if (strlen(timefmt)) {
	fprintf(fp, "set timefmt \"%s\"\n", conv_text(str, timefmt));
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

    fprintf(fp, "set locale \"%s\"\n", cur_locale);

    fputs("set loadpath ",fp);
    {
	char *s;
	while ((s=save_loadpath()) != NULL)
	    fprintf(fp, "\"%s\" ",s);
	fputc('\n',fp);
    }
}

static void save_tics(fp, where, axis, tdef, rotate, text)
FILE *fp;
int where;
int axis;
struct ticdef *tdef;
TBOOLEAN rotate;
char *text;
{
    char str[MAX_LINE_LEN + 1];

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
		fprintf(fp, "\"%s\",", conv_text(str, fd));
	    }
	    fprintf(fp, "%g", tdef->def.series.incr);

	    if (tdef->def.series.end != VERYLARGE) {
		char td[26];
		gstrftime(td, 24, timefmt, (double) tdef->def.series.end);
		fprintf(fp, ",\"%s\"", conv_text(str, td));
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
		    fprintf(fp, "\"%s\" ", conv_text(str, t->label));
		if (flag_time) {
		    char td[26];
		    gstrftime(td, 24, timefmt, (double) t->position);
		    fprintf(fp, "\"%s\"", conv_text(str, td));
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

static void save_position(fp, pos)
FILE *fp;
struct position *pos;
{
    static char *msg[] =
    {"first_axes ", "second axes ", "graph ", "screen "};

    assert(first_axes == 0 && second_axes == 1 && graph == 2 && screen == 3);

    fprintf(fp, "%s%g, %s%g, %s%g",
	    pos->scalex == first_axes ? "" : msg[pos->scalex], pos->x,
	    pos->scaley == pos->scalex ? "" : msg[pos->scaley], pos->y,
	    pos->scalez == pos->scaley ? "" : msg[pos->scalez], pos->z);
}

void load_file(fp, name, can_do_args)
FILE *fp;
char *name;
TBOOLEAN can_do_args;
{
    register int len;

    int start, left;
    int more;
    int stop = FALSE;

    lf_push(fp);		/* save state for errors and recursion */
    do_load_arg_substitution = can_do_args;

    if (fp == (FILE *) NULL) {
	/* HBB 980311: alloc() it, to save valuable stack space: */
	char *errbuf = gp_alloc(BUFSIZ, "load_file errorstring");
	(void) sprintf(errbuf, "Cannot open %s file '%s'",
		       can_do_args ? "call" : "load", name);
	os_error(errbuf, c_token);
	free(errbuf);
    } else if (fp == stdin) {
	/* DBT 10-6-98  go interactive if "-" named as load file */
	interactive = TRUE; 
	while(!com_line())
	    ;
    } else {
	/* go into non-interactive mode during load */
	/* will be undone below, or in load_file_error */
	interactive = FALSE;
	inline_num = 0;
	infile_name = name;

	if (can_do_args) {
	    int aix = 0;
	    while (++c_token < num_tokens && aix <= 9) {
		if (isstring(c_token))
		    m_quote_capture(&call_args[aix++], c_token, c_token);
		else
		    m_capture(&call_args[aix++], c_token, c_token);
	    }

/*         A GNUPLOT "call" command can have up to _10_ arguments named "$0"
   to "$9".  After reading the 10th argument (i.e.: "$9") the variable
   'aix' contains the value '10' because of the 'aix++' construction
   in '&call_args[aix++]'.  So I think the following test of 'aix' 
   should be done against '10' instead of '9'.                (JFi) */

/*              if (c_token >= num_tokens && aix > 9) */
	    if (c_token >= num_tokens && aix > 10)
		int_error("too many arguments for CALL <file>", ++c_token);
	}
	while (!stop) {		/* read all commands in file */
	    /* read one command */
	    left = input_line_len;
	    start = 0;
	    more = TRUE;

	    while (more) {
		if (fgets(&(input_line[start]), left, fp) == (char *) NULL) {
		    stop = TRUE;	/* EOF in file */
		    input_line[start] = '\0';
		    more = FALSE;
		} else {
		    inline_num++;
		    len = strlen(input_line) - 1;
		    if (input_line[len] == '\n') {	/* remove any newline */
			input_line[len] = '\0';
			/* Look, len was 1-1 = 0 before, take care here! */
			if (len > 0)
			    --len;
			if (input_line[len] == '\r') {	/* remove any carriage return */
			    input_line[len] = NUL;
			    if (len > 0)
				--len;
			}
		    }
		     else if (len + 2 >= left) {
			extend_input_line();
			left = input_line_len - len - 1;
			start = len + 1;
			continue;	/* don't check for '\' */
		    }
		    if (input_line[len] == '\\') {
			/* line continuation */
			start = len;
			left = input_line_len - start;
		    } else
			more = FALSE;
		}
	    }

	    if (strlen(input_line) > 0) {
		if (can_do_args) {
		    register int il = 0;
		    register char *rl;
		    char *raw_line = rl = gp_alloc((unsigned long) strlen(input_line) + 1, "string");

		    strcpy(raw_line, input_line);
		    *input_line = '\0';
		    while (*rl) {
			register int aix;
			if (*rl == '$'
			    && ((aix = *(++rl)) != 0)	/* HBB 980308: quiet BCC warning */
			    &&aix >= '0' && aix <= '9') {
			    if (call_args[aix -= '0']) {
				len = strlen(call_args[aix]);
				while (input_line_len - il < len + 1) {
				    extend_input_line();
				}
				strcpy(input_line + il, call_args[aix]);
				il += len;
			    }
			} else {
			    /* substitute for $<n> here */
			    if (il + 1 > input_line_len) {
				extend_input_line();
			    }
			    input_line[il++] = *rl;
			}
			rl++;
		    }
		    if (il + 1 > input_line_len) {
			extend_input_line();
		    }
		    input_line[il] = '\0';
		    free(raw_line);
		}
		screen_ok = FALSE;	/* make sure command line is
					   echoed on error */
		if (do_line())
		    stop=TRUE;
	    }
	}
    }

    /* pop state */
    (void) lf_pop();		/* also closes file fp */
}

/* pop from load_file state stack */
static TBOOLEAN			/* FALSE if stack was empty */
 lf_pop()
{				/* called by load_file and load_file_error */
    LFS *lf;

    if (lf_head == NULL)
	return (FALSE);
    else {
	int argindex;
	lf = lf_head;
	if (lf->fp != (FILE *)NULL && lf->fp != stdin) {
	    /* DBT 10-6-98  do not close stdin in the case
	     * that "-" is named as a load file
	     */
	    (void) fclose(lf->fp);
	}
	for (argindex = 0; argindex < 10; argindex++) {
	    if (call_args[argindex]) {
		free(call_args[argindex]);
	    }
	    call_args[argindex] = lf->call_args[argindex];
	}
	do_load_arg_substitution = lf->do_load_arg_substitution;
	interactive = lf->interactive;
	inline_num = lf->inline_num;
	infile_name = lf->name;
	lf_head = lf->prev;
	free((char *) lf);
	return (TRUE);
    }
}

/* push onto load_file state stack */
/* essentially, we save information needed to undo the load_file changes */
static void lf_push(fp)		/* called by load_file */
FILE *fp;
{
    LFS *lf;
    int argindex;

    lf = (LFS *) gp_alloc((unsigned long) sizeof(LFS), (char *) NULL);
    if (lf == (LFS *) NULL) {
	if (fp != (FILE *) NULL)
	    (void) fclose(fp);	/* it won't be otherwise */
	int_error("not enough memory to load file", c_token);
    }
    lf->fp = fp;		/* save this file pointer */
    lf->name = infile_name;	/* save current name */
    lf->interactive = interactive;	/* save current state */
    lf->inline_num = inline_num;	/* save current line number */
    lf->do_load_arg_substitution = do_load_arg_substitution;
    for (argindex = 0; argindex < 10; argindex++) {
	lf->call_args[argindex] = call_args[argindex];
	call_args[argindex] = NULL;	/* initially no args */
    }
    lf->prev = lf_head;		/* link to stack */
    lf_head = lf;
}

/* used for reread  vsnyder@math.jpl.nasa.gov */
FILE *lf_top()
{
    if (lf_head == (LFS *) NULL)
	return ((FILE *) NULL);
    return (lf_head->fp);
}

/* called from main */
void load_file_error()
{
    /* clean up from error in load_file */
    /* pop off everything on stack */
    while (lf_pop());
}

/* find char c in string str; return p such that str[p]==c;
 * if c not in str then p=strlen(str)
 */
int instring(str, c)
char *str;
int c;
{
    int pos = 0;

    while (str != NULL && *str != NUL && c != *str) {
	str++;
	pos++;
    }
    return (pos);
}

void show_functions()
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


void show_at()
{
    (void) putc('\n', stderr);
    disp_at(temp_at(), 0);
}


void disp_at(curr_at, level)
struct at_type *curr_at;
int level;
{
    register int i, j;
    register union argument *arg;

    for (i = 0; i < curr_at->a_count; i++) {
	(void) putc('\t', stderr);
	for (j = 0; j < level; j++)
	    (void) putc(' ', stderr);	/* indent */

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
		    disp_at(arg->udf_arg->at, level + 2);	/* recurse! */
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
		    disp_at(arg->udf_arg->at, level + 2);	/* recurse! */
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

/* find max len of keys and count keys with len > 0 */

int find_maxl_keys(plots, count, kcnt)
struct curve_points *plots;
int count, *kcnt;
{
    int mlen, len, curve, cnt;
    register struct curve_points *this_plot;

    mlen = cnt = 0;
    this_plot = plots;
    for (curve = 0; curve < count; this_plot = this_plot->next_cp, curve++)
	if (this_plot->title
	    && ((len = /*assign */ strlen(this_plot->title)) != 0)	/* HBB 980308: quiet BCC warning */
	    ) {
	    cnt++;
	    if (len > mlen)
		mlen = strlen(this_plot->title);
	}
    if (kcnt != NULL)
	*kcnt = cnt;
    return (mlen);
}


/* calculate the number and max-width of the keys for an splot.
 * Note that a blank line is issued after each set of contours
 */
int find_maxl_keys3d(plots, count, kcnt)
struct surface_points *plots;
int count, *kcnt;
{
    int mlen, len, surf, cnt;
    struct surface_points *this_plot;

    mlen = cnt = 0;
    this_plot = plots;
    for (surf = 0; surf < count; this_plot = this_plot->next_sp, surf++) {

	/* we draw a main entry if there is one, and we are
	 * drawing either surface, or unlabelled contours
	 */
	if (this_plot->title && *this_plot->title &&
	    (draw_surface || (draw_contour && !label_contours))) {
	    ++cnt;
	    len = strlen(this_plot->title);
	    if (len > mlen)
		mlen = len;
	}
	if (draw_contour && label_contours && this_plot->contours != NULL) {
	    len = find_maxl_cntr(this_plot->contours, &cnt);
	    if (len > mlen)
		mlen = len;
	}
    }

    if (kcnt != NULL)
	*kcnt = cnt;
    return (mlen);
}

static int find_maxl_cntr(contours, count)
struct gnuplot_contours *contours;
int *count;
{
    register int cnt;
    register int mlen, len;
    register struct gnuplot_contours *cntrs = contours;

    mlen = cnt = 0;
    while (cntrs) {
	if (label_contours && cntrs->isNewLevel) {
	    len = strlen(cntrs->label);
	    if (len)
		cnt++;
	    if (len > mlen)
		mlen = len;
	}
	cntrs = cntrs->next;
    }
    *count += cnt;
    return (mlen);
}

static void save_range(fp, axis, min, max, autosc, text)
FILE *fp;
int axis;
double min, max;
TBOOLEAN autosc;
char *text;
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

/* check user defined format strings for valid double conversions */
TBOOLEAN valid_format(format)
const char *format;
{
    for (;;) {
	if (!(format = strchr(format, '%')))	/* look for format spec  */
	    return TRUE;	/* passed Test           */
	do {			/* scan format statement */
	    format++;
	} while (strchr("+-#0123456789.", *format));

	switch (*format) {	/* Now at format modifier */
	case '*':		/* Ignore '*' statements */
	case '%':		/* Char   '%' itself     */
	    format++;
	    continue;
	case 'l':		/* Now we found it !!! */
	    if (!strchr("fFeEgG", format[1]))	/* looking for a valid format */
		return FALSE;
	    format++;
	    break;
	default:
	    return FALSE;
	}
    }
}
