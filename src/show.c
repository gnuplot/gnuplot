#ifndef lint
static char *RCSid() { return RCSid("$Id: show.c,v 1.20 1999/08/07 17:21:32 lhecking Exp $"); }
#endif

/* GNUPLOT - show.c */

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


#include "plot.h"
#include "setshow.h"
#include "tables.h"

/* for show_version_long() */
#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h>
#endif

#define DEF_FORMAT   "%g"	/* default format for tic mark labels */
#define SIGNIF (0.01)		/* less than one hundredth of a tic mark */

/******** Local functions ********/

static void show_style __PROTO((const char *name, enum PLOT_STYLE style));
static void show_range __PROTO((int axis, double min, double max, int autosc, const char *text));
static void show_border __PROTO((void));
static void show_dgrid3d __PROTO((void));
static void show_offsets __PROTO((void));
static void show_output __PROTO((void));
static void show_size __PROTO((void));
static void show_xyzlabel __PROTO((const char *name, label_struct * label));
static void show_angles __PROTO((void));
static void show_boxwidth __PROTO((void));
static void show_bars __PROTO((void));
static void show_xzeroaxis __PROTO((void));
static void show_yzeroaxis __PROTO((void));
static void show_label __PROTO((int tag));
static void show_linestyle __PROTO((int tag));
static void show_arrow __PROTO((int tag));
static void show_grid __PROTO((void));
static void show_key __PROTO((void));
static void show_keytitle __PROTO((void));
static void show_mtics __PROTO((int mini, double freq, const char *name));
static void show_tics __PROTO((int showx, int showy, int showz, int showx2, int showy2));
static void show_ticdef __PROTO((int tics, int axis, struct ticdef * tdef, const char *text, int rotate_tics, const char *ticfmt));
static void show_term __PROTO((void));
static void show_autoscale __PROTO((void));
static void show_clip __PROTO((void));
static void show_contour __PROTO((void));
static void show_mapping __PROTO((void));
static void show_format __PROTO((void));
static void show_logscale __PROTO((void));
static void show_variables __PROTO((void));
static void show_hidden3d __PROTO((void));
static void show_label_contours __PROTO((void));
static void show_margin __PROTO((void));
static void show_position __PROTO((struct position * pos));

static void show_timefmt __PROTO((void));
static void show_missing __PROTO((void));
static void show_functions __PROTO((void));
static void show_at __PROTO((void));
static void disp_at __PROTO((struct at_type *, int));

/* Some defines for consistency */
#define show_datatype(name,a) \
	fprintf(stderr, "\t%s is set to %s\n", (name), \
		datatype[a] == TIME ? "time" : "numerical");
#define show_dummy() \
	fprintf(stderr, "\tdummy variables are \"%s\" and \"%s\"\n", \
		dummy_var[0], dummy_var[1])
#define show_encoding() \
	fprintf(stderr, "\tencoding is %s\n", encoding_names[encoding]);
#define show_isosamples() \
	fprintf(stderr, "\tiso sampling rate is %d, %d\n", \
		iso_samples_1, iso_samples_2);
#define show_offsets() \
	fprintf(stderr, "\toffsets are %g, %g, %g, %g\n", \
		loff, roff, toff, boff);
#define show_origin() \
	fprintf(stderr, "\torigin is set to %g,%g\n", xoffset, yoffset);
#define show_parametric() \
	fprintf(stderr, "\tparametric is %s\n", (parametric) ? "ON" : "OFF");
#define show_plot() \
	fprintf(stderr, "\tlast plot command was: %s\n", replot_line);
#define show_pointsize() \
	fprintf(stderr, "\tpointsize is %g\n", pointsize);
#define show_polar() \
	fprintf(stderr, "\tpolar is %s\n", (polar) ? "ON" : "OFF");
#define show_samples() \
	fprintf(stderr, "\tsampling rate is %d, %d\n", samples_1, samples_2);
#define show_surface() \
	fprintf(stderr, "\tsurface is %sdrawn\n", draw_surface ? "" : "not ");
#define show_view() \
	fprintf(stderr, "\tview is %g rot_x, %g rot_z, %g scale, %g scale_z\n",\
		surface_rot_x, surface_rot_z, surface_scale, surface_zscale);
#define show_zero() \
	fprintf(stderr, "\tzero is %g\n", zero)

/* following code segment appears over and over again */

#define SHOW_NUM_OR_TIME(x, axis) \
do{if (datatype[axis]==TIME) { \
  char s[80]; char *p; \
  putc('"', stderr);   \
  gstrftime(s,80,timefmt,(double)(x)); \
  for(p=s; *p; ++p) {\
   if ( *p == '\t' ) fputs("\\t",stderr);\
   else if (*p == '\n') fputs("\\n",stderr); \
   else if ( *p > 126 || *p < 32 ) fprintf(stderr,"\\%03o",*p);\
   else putc(*p, stderr);\
  }\
  putc('"', stderr);\
 } else {\
  fprintf(stderr,"%g",x);\
}} while(0)


#define BREAK if (!show_all) { c_token++; break; }
#define PUT_NEWLINE if (!show_all) (void) putc('\n',stderr)
#define CHECK_TAG_GT_ZERO \
  c_token++; \
  if (!END_OF_COMMAND) { \
    tag = (int) real(const_express(&a)); \
    if (tag <= 0) int_error(c_token, "tag must be > zero"); \
  }

/******* The 'show' command *******/
void
show_command()
{
    /* show at is undocumented/hidden... */
    static char GPFAR showmess[] = SETSHOWMSG;
    struct value a;
    int tag = 0;
    int show_all = 0;

    c_token++;

    switch(lookup_table(&set_tbl[0],c_token)) {
    case S_INVALID:
	int_error(c_token, showmess);
    case S_ACTIONTABLE:
	c_token++;
	show_at();
	c_token++;
	break;
    case S_XTICS:
	show_tics(TRUE, FALSE, FALSE, TRUE, FALSE);
	c_token++;
	break;
    case S_YTICS:
	show_tics(FALSE, TRUE, FALSE, FALSE, TRUE);
	c_token++;
	break;
    case S_ZTICS:
	show_tics(FALSE, FALSE, TRUE, FALSE, FALSE);
	c_token++;
	break;
    case S_X2TICS:
	show_tics(FALSE, FALSE, FALSE, TRUE, FALSE);
	c_token++;
	break;
    case S_Y2TICS:
	show_tics(FALSE, FALSE, FALSE, FALSE, TRUE);
	c_token++;
	break;
    case S_XDTICS:
	show_tics(TRUE, FALSE, FALSE, TRUE, FALSE);
	c_token++;
	break;
    case S_YDTICS:
	show_tics(FALSE, TRUE, FALSE, FALSE, TRUE);
	c_token++;
	break;
    case S_ZDTICS:
	show_tics(FALSE, FALSE, TRUE, FALSE, FALSE);
	c_token++;
	break;
    case S_X2DTICS:
	show_tics(FALSE, FALSE, FALSE, TRUE, FALSE);
	c_token++;
	break;
    case S_Y2DTICS:
	show_tics(FALSE, FALSE, FALSE, FALSE, TRUE);
	c_token++;
	break;
    case S_XMTICS:
	show_tics(TRUE, FALSE, FALSE, TRUE, FALSE);
	c_token++;
	break;
    case S_YMTICS:
	show_tics(FALSE, TRUE, FALSE, FALSE, TRUE);
	c_token++;
	break;
    case S_ZMTICS:
	show_tics(FALSE, FALSE, TRUE, FALSE, FALSE);
	c_token++;
	break;
    case S_X2MTICS:
	show_tics(FALSE, FALSE, FALSE, TRUE, FALSE);
	c_token++;
	break;
    case S_Y2MTICS:
	show_tics(FALSE, FALSE, FALSE, FALSE, TRUE);
	c_token++;
	break;
    /* The following cases are fall-through if `show all' is requested
     */
    case S_ALL:
	c_token++;
	show_all = 1;
    case S_VERSION:
	show_version(stderr);
	if (!show_all) {
	    c_token++;
	    if (almost_equals(c_token, "l$ong")) {
		show_version_long();
		c_token++;
	    }
	    break;
	}
    case S_AUTOSCALE:
	PUT_NEWLINE;
	show_autoscale();
	BREAK;
    case S_BARS:
	PUT_NEWLINE;
	show_bars();
	BREAK;
    case S_BORDER:
	PUT_NEWLINE;
	show_border();
	BREAK;
    case S_BOXWIDTH:
	PUT_NEWLINE;
	show_boxwidth();
	BREAK;
    case S_CLIP:
	PUT_NEWLINE;
	show_clip();
	BREAK;
    case S_CLABEL:
	PUT_NEWLINE;
	show_label_contours();
	BREAK;
    case S_CONTOUR:
    case S_CNTRPARAM:
	PUT_NEWLINE;
	show_contour();
	BREAK;
    case S_DGRID3D:
	PUT_NEWLINE;
	show_dgrid3d();
	BREAK;
    case S_MAPPING:
	PUT_NEWLINE;
	show_mapping();
	BREAK;
    case S_DUMMY:
	PUT_NEWLINE;
	show_dummy();
	BREAK;
    case S_FORMAT:
	PUT_NEWLINE;
	show_format();
	BREAK;
    case S_DATA:
	if (!show_all) {
	    c_token++;
	    if (!almost_equals(c_token, "s$tyle"))
		int_error(c_token, "expecting keyword 'style'");
	}
	PUT_NEWLINE;
	show_style("data", data_style);
	BREAK;
    case S_FUNCTIONS:
	if (!show_all) {
	    c_token++;
	    if (almost_equals(c_token, "s$tyle")) {
		(void) putc('\n', stderr);
		show_style("functions", func_style);
		c_token++;
	    } else
		show_functions();
	    break;
	}
	PUT_NEWLINE;
	show_style("functions", func_style);
	BREAK;
    case S_GRID:
	PUT_NEWLINE;
	show_grid();
	BREAK;
    case S_XZEROAXIS:
	PUT_NEWLINE;
	show_xzeroaxis();
	BREAK;
    case S_YZEROAXIS:
	PUT_NEWLINE;
	show_yzeroaxis();
	BREAK;
    case S_ZEROAXIS:
	if (!show_all) {
	    (void) putc('\n', stderr);
	    show_xzeroaxis();
	    show_yzeroaxis();
	    c_token++;
	    break;
	}
    case S_LABEL:
	tag = 0;
	if (!show_all) {
	    CHECK_TAG_GT_ZERO;
	}
	PUT_NEWLINE;
	show_label(tag);
	BREAK;
    case S_ARROW:
	tag = 0;
	if (!show_all) {
	    CHECK_TAG_GT_ZERO;
	}
	PUT_NEWLINE;
	show_arrow(tag);
	BREAK;
    case S_LINESTYLE:
	tag = 0;
	if (!show_all) {
	    CHECK_TAG_GT_ZERO;
	}
	PUT_NEWLINE;
	show_linestyle(tag);
	BREAK;
    case S_KEYTITLE:
	PUT_NEWLINE;
	show_keytitle();
	BREAK;
    case S_KEY:
	PUT_NEWLINE;
	show_key();
	BREAK;
    case S_LOGSCALE:
	PUT_NEWLINE;
	show_logscale();
	BREAK;
    case S_OFFSETS:
	PUT_NEWLINE;
	show_offsets();
	BREAK;
    case S_MARGIN:
	PUT_NEWLINE;
	show_margin();
	BREAK;
    case S_OUTPUT:
	PUT_NEWLINE;
	show_output();
	BREAK;
    case S_PARAMETRIC:
	PUT_NEWLINE;
	show_parametric();
	BREAK;
    case S_POINTSIZE:
	PUT_NEWLINE;
	show_pointsize();
	BREAK;
    case S_ENCODING:
	PUT_NEWLINE;
	show_encoding();
	BREAK;
    case S_POLAR:
	PUT_NEWLINE;
	show_polar();
	BREAK;
    case S_ANGLES:
	PUT_NEWLINE;
	show_angles();
	BREAK;
    case S_SAMPLES:
	PUT_NEWLINE;
	show_samples();
	BREAK;
    case S_ISOSAMPLES:
	PUT_NEWLINE;
	show_isosamples();
	BREAK;
    case S_VIEW:
	PUT_NEWLINE;
	show_view();
	BREAK;
    case S_SURFACE:
	PUT_NEWLINE;
	show_surface();
	BREAK;
#ifndef LITE
    case S_HIDDEN3D:
	PUT_NEWLINE;
	show_hidden3d();
	BREAK;
#endif
    case S_SIZE:
	PUT_NEWLINE;
	show_size();
	BREAK;
    case S_ORIGIN:
	PUT_NEWLINE;
	show_origin();
	BREAK;
    case S_TERMINAL:
	PUT_NEWLINE;
	show_term();
	BREAK;
    case S_TICS:
	PUT_NEWLINE;
	show_tics(TRUE, TRUE, TRUE, TRUE, TRUE);
	BREAK;
    case S_MXTICS:
	PUT_NEWLINE;
	show_mtics(mxtics, mxtfreq, "x");
	BREAK;
    case S_MYTICS:
	PUT_NEWLINE;
	show_mtics(mytics, mytfreq, "y");
	BREAK;
    case S_MZTICS:
	PUT_NEWLINE;
	show_mtics(mztics, mztfreq, "z");
	BREAK;
    case S_MX2TICS:
	PUT_NEWLINE;
	show_mtics(mx2tics, mx2tfreq, "x2");
	BREAK;
    case S_MY2TICS:
	PUT_NEWLINE;
	show_mtics(my2tics, my2tfreq, "y2");
	BREAK;
    case S_TIMESTAMP:
	PUT_NEWLINE;
	show_xyzlabel("time", &timelabel);
	if (!show_all) {
	    fprintf(stderr, "\twritten in %s corner\n",
		    (timelabel_bottom ? "bottom" : "top"));
	    if (timelabel_rotate)
		fputs("\trotated if the terminal allows it\n\t", stderr);
	    else
		fputs("\tnot rotated\n\t", stderr);
	}
	BREAK;
    case S_RRANGE:
	PUT_NEWLINE;
	if (!show_all)
	    show_range(R_AXIS, rmin, rmax, autoscale_r, "r");
	BREAK;
    case S_TRANGE:
	PUT_NEWLINE;
	if (!show_all)
	    show_range(T_AXIS, tmin, tmax, autoscale_t, "t");
	else if ((parametric || polar) && !is_3d_plot)
	    show_range(T_AXIS, tmin, tmax, autoscale_t, "t");
	BREAK;
    case S_URANGE:
	PUT_NEWLINE;
	if (!show_all)
	    show_range(U_AXIS, umin, umax, autoscale_u, "u");
	else if ((parametric || polar) && is_3d_plot)
	    show_range(V_AXIS, vmin, vmax, autoscale_v, "v");
	BREAK;
    case S_VRANGE:
	PUT_NEWLINE;
	if (!show_all)
	    show_range(V_AXIS, vmin, vmax, autoscale_v, "v");
	else if ((parametric || polar) && is_3d_plot)
	    show_range(V_AXIS, vmin, vmax, autoscale_v, "v");
	BREAK;
    case S_XRANGE:
	PUT_NEWLINE;
	show_range(FIRST_X_AXIS, xmin, xmax, autoscale_x, "x");
	BREAK;
    case S_YRANGE:
	PUT_NEWLINE;
	show_range(FIRST_Y_AXIS, ymin, ymax, autoscale_y, "y");
	BREAK;
    case S_X2RANGE:
	PUT_NEWLINE;
	show_range(SECOND_X_AXIS, x2min, x2max, autoscale_x2, "x2");
	BREAK;
    case S_Y2RANGE:
	PUT_NEWLINE;
	show_range(SECOND_Y_AXIS, y2min, y2max, autoscale_y2, "y2");
	BREAK;
    case S_ZRANGE:
	PUT_NEWLINE;
	show_range(FIRST_Z_AXIS, zmin, zmax, autoscale_z, "z");
	BREAK;
    case S_TITLE:
	PUT_NEWLINE;
	show_xyzlabel("title", &title);
	BREAK;
    case S_XLABEL:
	PUT_NEWLINE;
	show_xyzlabel("xlabel", &xlabel);
	BREAK;
    case S_YLABEL:
	PUT_NEWLINE;
	show_xyzlabel("ylabel", &ylabel);
	BREAK;
    case S_ZLABEL:
	PUT_NEWLINE;
	show_xyzlabel("zlabel", &zlabel);
	BREAK;
    case S_X2LABEL:
	PUT_NEWLINE;
	show_xyzlabel("x2label", &x2label);
	BREAK;
    case S_Y2LABEL:
	PUT_NEWLINE;
	show_xyzlabel("y2label", &y2label);
	BREAK;
    case S_XDATA:
	PUT_NEWLINE;
	show_datatype("xdata", FIRST_X_AXIS);
	BREAK;
    case S_YDATA:
	PUT_NEWLINE;
	show_datatype("ydata", FIRST_Y_AXIS);
	BREAK;
    case S_X2DATA:
	PUT_NEWLINE;
	show_datatype("x2data", SECOND_X_AXIS);
	BREAK;
    case S_Y2DATA:
	PUT_NEWLINE;
	show_datatype("y2data", SECOND_Y_AXIS);
	BREAK;
    case S_ZDATA:
	PUT_NEWLINE;
	show_datatype("zdata", FIRST_Z_AXIS);
	BREAK;
    case S_TIMEFMT:
	PUT_NEWLINE;
	show_timefmt();
	BREAK;
    case S_LOCALE:
	PUT_NEWLINE;
	show_locale();
	BREAK;
    case S_LOADPATH:
	PUT_NEWLINE;
	show_loadpath();
	BREAK;
    case S_ZERO:
	PUT_NEWLINE;
	show_zero();
	BREAK;
    case S_MISSING:
	PUT_NEWLINE;
	show_missing();
	BREAK;
    case S_PLOT:
	PUT_NEWLINE;
	show_plot();
	BREAK;
    case S_VARIABLES:
	PUT_NEWLINE;
	show_variables();
	BREAK;
    default:
	if (show_all) {
	    /* Urgh ... */
	    show_functions();
	    c_token++;
	}
	break;
    }

    screen_ok = FALSE;
    (void) putc('\n', stderr);

}


/*********** support functions for 'show'  **********/
/* perhaps these need to be put into a constant array ? */
static void
show_style(name, style)
const char *name;
enum PLOT_STYLE style;
{
    fprintf(stderr, "\t%s are plotted with ", name);
    switch (style) {
    case LINES:
	fputs("lines\n", stderr);
	break;
    case POINTSTYLE:
	fputs("points\n", stderr);
	break;
    case IMPULSES:
	fputs("impulses\n", stderr);
	break;
    case LINESPOINTS:
	fputs("linespoints\n", stderr);
	break;
    case DOTS:
	fputs("dots\n", stderr);
	break;

    case YERRORLINES:
	fputs("yerrorlines\n", stderr);
	break;
    case XERRORLINES:
	fputs("xerrorlines\n", stderr);
	break;
    case XYERRORLINES:
	fputs("xyerrorlines\n", stderr);
	break;

    case YERRORBARS:
	fputs("yerrorbars\n", stderr);
	break;
    case XERRORBARS:
	fputs("xerrorbars\n", stderr);
	break;
    case XYERRORBARS:
	fputs("xyerrorbars\n", stderr);
	break;
    case BOXES:
	fputs("boxes\n", stderr);
	break;
    case BOXERROR:
	fputs("boxerrorbars\n", stderr);
	break;
    case BOXXYERROR:
	fputs("boxxyerrorbars\n", stderr);
	break;
    case STEPS:
	fputs("steps\n", stderr);
	break;
    case FSTEPS:
	fputs("fsteps\n", stderr);
	break;
    case HISTEPS:
	fputs("histeps\n", stderr);
	break;
    case VECTOR:
	fputs("vector\n", stderr);
	break;
    case FINANCEBARS:
	fputs("financebars\n", stderr);
	break;
    case CANDLESTICKS:
	fputs("candlesticsks\n", stderr);
	break;
    }
}

static void
show_bars()
{
    /* I really like this: "terrorbars" ;-) */
    if (bar_size > 0.0)
	fprintf(stderr, "\terrorbars are plotted with bars of size %f\n",
		bar_size);
    else
	fputs("\terrors are plotted without bars\n", stderr);
}

static void
show_boxwidth()
{
    if (boxwidth < 0.0)
	fputs("\tboxwidth is auto\n", stderr);
    else
	fprintf(stderr, "\tboxwidth is %g\n", boxwidth);
}
static void
show_dgrid3d()
{
    if (dgrid3d)
	fprintf(stderr, "\
\tdata grid3d is enabled for mesh of size %dx%d, norm=%d\n",
		dgrid3d_row_fineness,
		dgrid3d_col_fineness,
		dgrid3d_norm_value);
    else
	fputs("\tdata grid3d is disabled\n", stderr);
}

static void
show_range(axis, min, max, autosc, text)
int axis;
double min, max;
TBOOLEAN autosc;
const char *text;
{
    /* this probably ought to just invoke save_range(stderr) from misc.c
     * since I think it is identical
     */

    if (datatype[axis] == TIME)
	fprintf(stderr, "\tset %sdata time\n", text);
    fprintf(stderr, "\tset %srange [", text);
    if (autosc & 1) {
	fputc('*', stderr);
    } else {
	SHOW_NUM_OR_TIME(min, axis);
    }
    fputs(" : ", stderr);
    if (autosc & 2) {
	fputc('*', stderr);
    } else {
	SHOW_NUM_OR_TIME(max, axis);
    }
    fprintf(stderr, "] %sreverse %swriteback",
	    (range_flags[axis] & RANGE_REVERSE) ? "" : "no",
	    (range_flags[axis] & RANGE_WRITEBACK) ? "" : "no");

    if (autosc) {
	/* add current (hidden) range as comments */
	fputs("  # (currently [", stderr);
	if (autosc & 1) {
	    SHOW_NUM_OR_TIME(min, axis);
	}
	putc(':', stderr);
	if (autosc & 2) {
	    SHOW_NUM_OR_TIME(max, axis);
	}
	fputs("] )\n", stderr);
    } else {
	putc('\n', stderr);
    }
}

static void
show_border()
{
    fprintf(stderr, "\tborder is %sdrawn %d\n", draw_border ? "" : "not ",
	    draw_border);
    /* HBB 980609: added facilities to specify border linestyle */
    fprintf(stderr, "\tBorder drawn with linetype %d, linewidth %.3f\n",
	    border_lp.l_type + 1, border_lp.l_width);
}

static void
show_output()
{
    if (outstr)
	fprintf(stderr, "\toutput is sent to '%s'\n", outstr);
    else
	fputs("\toutput is sent to STDOUT\n", stderr);
}

static void
show_hidden3d()
{
#ifdef LITE
    printf(" Hidden Line Removal Not Supported in LITE version\n");
#else
    fprintf(stderr, "\thidden surface is %s\n", hidden3d ? "removed" : "drawn");
    show_hidden3doptions();
#endif /* LITE */
}

static void
show_label_contours()
{
    if (label_contours)
	fprintf(stderr, "\tcontour line types are varied & labeled with format '%s'\n", contour_format);
    else
	fputs("\tcontour line types are all the same\n", stderr);
}

static void
show_size()
{
    fprintf(stderr, "\tsize is scaled by %g,%g\n", xsize, ysize);
    if (aspect_ratio > 0)
	fprintf(stderr, "\tTry to set aspect ratio to %g:1.0\n", aspect_ratio);
    else if (aspect_ratio == 0)
	fputs("\tNo attempt to control aspect ratio\n", stderr);
    else
	fprintf(stderr, "\tTry to set LOCKED aspect ratio to %g:1.0\n",
		-aspect_ratio);
}

static void
show_xyzlabel(name, label)
const char *name;
label_struct *label;
{
    fprintf(stderr, "\t%s is \"%s\", offset at %f, %f",
	    name, conv_text(label->text), label->xoffset, label->yoffset);

    if (*label->font)
	fprintf(stderr, ", using font \"%s\"", conv_text(label->font));

    putc('\n', stderr);
}

static void
show_keytitle()
{
    fprintf(stderr, "\tkeytitle is \"%s\"\n", conv_text(key_title));
}

static void
show_timefmt()
{
    fprintf(stderr, "\tread format for time is \"%s\"\n", conv_text(timefmt));
}


static void
show_xzeroaxis()
{
    if (xzeroaxis.l_type > -3)
	fprintf(stderr, "\txzeroaxis is drawn with linestyle %d, linewidth %.3f\n", xzeroaxis.l_type + 1, xzeroaxis.l_width);
    else
	fputs("\txzeroaxis is OFF\n", stderr);
    if (x2zeroaxis.l_type > -3)
	fprintf(stderr, "\tx2zeroaxis is drawn with linestyle %d, linewidth %.3f\n", x2zeroaxis.l_type + 1, x2zeroaxis.l_width);
    else
	fputs("\tx2zeroaxis is OFF\n", stderr);
}

static void
show_yzeroaxis()
{
    if (yzeroaxis.l_type > -3)
	fprintf(stderr, "\tyzeroaxis is drawn with linestyle %d, linewidth %.3f\n", yzeroaxis.l_type + 1, yzeroaxis.l_width);
    else
	fputs("\tyzeroaxis is OFF\n", stderr);
    if (y2zeroaxis.l_type > -3)
	fprintf(stderr, "\ty2zeroaxis is drawn with linestyle %d, linewidth %.3f\n", y2zeroaxis.l_type + 1, y2zeroaxis.l_width);
    else
	fputs("\ty2zeroaxis is OFF\n", stderr);
}

static void
show_label(tag)
int tag;			/* 0 means show all */
{
    struct text_label *this_label;
    TBOOLEAN showed = FALSE;

    for (this_label = first_label; this_label != NULL;
	 this_label = this_label->next) {
	if (tag == 0 || tag == this_label->tag) {
	    showed = TRUE;
	    fprintf(stderr, "\tlabel %d \"%s\" at ",
		    this_label->tag, conv_text(this_label->text));
	    show_position(&this_label->place);
	    switch (this_label->pos) {
	    case LEFT:{
		    fputs(" left", stderr);
		    break;
		}
	    case CENTRE:{
		    fputs(" centre", stderr);
		    break;
		}
	    case RIGHT:{
		    fputs(" right", stderr);
		    break;
		}
	    }
	    fprintf(stderr, " %s ", this_label->rotate ? "rotated (if possible)" : "not rotated");
	    fprintf(stderr, " %s ", this_label->layer ? "front" : "back");
	    if (this_label->font != NULL)
		fprintf(stderr, " font \"%s\"", this_label->font);
	    /* Entry font added by DJL */
	    fputc('\n', stderr);
	}
    }
    if (tag > 0 && !showed)
	int_error(c_token, "label not found");
}

static void
show_arrow(tag)
int tag;			/* 0 means show all */
{
    struct arrow_def *this_arrow;
    TBOOLEAN showed = FALSE;

    for (this_arrow = first_arrow; this_arrow != NULL;
	 this_arrow = this_arrow->next) {
	if (tag == 0 || tag == this_arrow->tag) {
	    showed = TRUE;
	    fprintf(stderr, "\tarrow %d, linetype %d, linewidth %.3f %s %s\n\t  from ",
		    this_arrow->tag,
		    this_arrow->lp_properties.l_type + 1,
		    this_arrow->lp_properties.l_width,
		    this_arrow->head ? "" : " (nohead)",
		    this_arrow->layer ? "front" : "back");
	    show_position(&this_arrow->start);
	    fputs(" to ", stderr);
	    show_position(&this_arrow->end);
	    putc('\n', stderr);
	}
    }
    if (tag > 0 && !showed)
	int_error(c_token, "arrow not found");
}

static void
show_linestyle(tag)
int tag;			/* 0 means show all */
{
    struct linestyle_def *this_linestyle;
    TBOOLEAN showed = FALSE;

    for (this_linestyle = first_linestyle; this_linestyle != NULL;
	 this_linestyle = this_linestyle->next) {
	if (tag == 0 || tag == this_linestyle->tag) {
	    showed = TRUE;
	    fprintf(stderr, "\tlinestyle %d, linetype %d, linewidth %.3f, pointtype %d, pointsize %.3f\n",
		    this_linestyle->tag,
		    this_linestyle->lp_properties.l_type + 1,
		    this_linestyle->lp_properties.l_width,
		    this_linestyle->lp_properties.p_type + 1,
		    this_linestyle->lp_properties.p_size);
	}
    }
    if (tag > 0 && !showed)
	int_error(c_token, "linestyle not found");
}

static void
show_grid()
{
    if (!work_grid.l_type) {
	fputs("\tgrid is OFF\n", stderr);
	return;
    }
    fprintf(stderr, "\t%s grid drawn at%s%s%s%s%s%s%s%s%s%s tics\n",
	    (polar_grid_angle != 0) ? "Polar" : "Rectangular",
	    work_grid.l_type & GRID_X ? " x" : "",
	    work_grid.l_type & GRID_Y ? " y" : "",
	    work_grid.l_type & GRID_Z ? " z" : "",
	    work_grid.l_type & GRID_X2 ? " x2" : "",
	    work_grid.l_type & GRID_Y2 ? " y2" : "",
	    work_grid.l_type & GRID_MX ? " mx" : "",
	    work_grid.l_type & GRID_MY ? " my" : "",
	    work_grid.l_type & GRID_MZ ? " mz" : "",
	    work_grid.l_type & GRID_MX2 ? " mx2" : "",
	    work_grid.l_type & GRID_MY2 ? " my2" : "");

    fprintf(stderr, "\tMajor grid drawn with linetype %d, linewidth %.3f\n",
	    grid_lp.l_type + 1, grid_lp.l_width);
    fprintf(stderr, "\tMinor grid drawn with linetype %d, linewidth %.3f\n",
	    mgrid_lp.l_type + 1, mgrid_lp.l_width);

    if (polar_grid_angle)
	fprintf(stderr, "\tGrid radii drawn every %f %s\n",
		polar_grid_angle / ang2rad,
		angles_format == ANGLES_DEGREES ? "degrees" : "radians");
}

static void
show_mtics(minitic, minifreq, name)
int minitic;
double minifreq;
const char *name;
{
    switch (minitic) {
    case MINI_OFF:
	fprintf(stderr, "\tminor %stics are off\n", name);
	break;
    case MINI_DEFAULT:
	fprintf(stderr, "\tminor %stics are computed automatically for log scales\n", name);
	break;
    case MINI_AUTO:
	fprintf(stderr, "\tminor %stics are computed automatically\n", name);
	break;
    case MINI_USER:
	fprintf(stderr, "\tminor %stics are drawn with %d subintervals between major xtic marks\n", name, (int) minifreq);
	break;
    default:
	int_error(NO_CARET, "Unknown minitic type in show_mtics()");
    }
}

static void
show_key()
{
    char str[80];
    *str = '\0';
    switch (key) {
    case -1:
	if (key_vpos == TUNDER) {
	    strcpy(str, "below");
	} else if (key_vpos == TTOP) {
	    strcpy(str, "top");
	} else {
	    strcpy(str, "bottom");
	}
	if (key_hpos == TOUT) {
	    strcpy(str, "outside (right)");
	} else if (key_hpos == TLEFT) {
	    strcat(str, " left");
	} else {
	    strcat(str, " right");
	}
	if (key_vpos != TUNDER && key_hpos != TOUT) {
	    strcat(str, " corner");
	}
	fprintf(stderr, "\tkey is ON, position: %s\n", str);
	break;
    case 0:
	fputs("\tkey is OFF\n", stderr);
	break;
    case 1:
	fputs("\tkey is at ", stderr);
	show_position(&key_user_pos);
	putc('\n', stderr);
	break;
    }
    if (key) {
	fprintf(stderr, "\tkey is %s justified, %s reversed and ",
		key_just == JLEFT ? "left" : "right",
		key_reverse ? "" : "not");
	if (key_box.l_type >= -2)
	    fprintf(stderr, "boxed\n\twith linetype %d, linewidth %.3f\n",
		    key_box.l_type + 1, key_box.l_width);
	else
	    fprintf(stderr, "not boxed\n\
\tsample length is %g characters\n\
\tvertical spacing is %g characters\n\
\twidth adjustment is %g characters\n\
\tkey title is \"%s\"\n",
		    key_swidth,
		    key_vert_factor,
		    key_width_fix,
		    key_title);
    }
}

static void
show_angles()
{
    fputs("\tAngles are in ", stderr);
    switch (angles_format) {
    case ANGLES_RADIANS:
	fputs("radians\n", stderr);
	break;
    case ANGLES_DEGREES:
	fputs("degrees\n", stderr);
	break;
    }
}


static void
show_tics(showx, showy, showz, showx2, showy2)
TBOOLEAN showx, showy, showz, showx2, showy2;
{
    fprintf(stderr, "\ttics are %s, \
\tticslevel is %g\n\
\tmajor ticscale is %g and minor ticscale is %g\n",
	    (tic_in ? "IN" : "OUT"),
	    ticslevel,
	    ticscale, miniticscale);

    if (showx)
	show_ticdef(xtics, FIRST_X_AXIS, &xticdef, "x", rotate_xtics,
		    conv_text(xformat));
    if (showx2)
	show_ticdef(x2tics, SECOND_X_AXIS, &x2ticdef, "x2", rotate_x2tics,
		    conv_text(x2format));
    if (showy)
	show_ticdef(ytics, FIRST_Y_AXIS, &yticdef, "y", rotate_ytics,
		    conv_text(yformat));
    if (showy2)
	show_ticdef(y2tics, SECOND_Y_AXIS, &y2ticdef, "y2", rotate_y2tics,
		    conv_text(y2format));
    if (showz)
	show_ticdef(ztics, FIRST_Z_AXIS, &zticdef, "z", rotate_ztics,
		    conv_text(zformat));
    screen_ok = FALSE;
}

/* called by show_tics */
static void
show_ticdef(tics, axis, tdef, text, rotate_tics, ticfmt)
int tics;			/* xtics ytics or ztics */
int axis;
struct ticdef *tdef;		/* xticdef yticdef or zticdef */
const char *text;		/* "x", ..., "x2", "y2" */
const char *ticfmt;
int rotate_tics;
{
    register struct ticmark *t;

    fprintf(stderr, "\t%s-axis tics:\t", text);
    switch (tics & TICS_MASK) {
    case NO_TICS:
	fputs("OFF\n", stderr);
	return;
    case TICS_ON_AXIS:
	fputs("on axis", stderr);
	if (tics & TICS_MIRROR)
	    fprintf(stderr, " and mirrored %s", (tic_in ? "OUT" : "IN"));
	break;
    case TICS_ON_BORDER:
	fputs("on border", stderr);
	if (tics & TICS_MIRROR)
	    fputs(" and mirrored on opposite border", stderr);
	break;
    }

    fprintf(stderr, "\n\t  labels are format \"%s\"", ticfmt);
    if (rotate_tics)
	fputs(", rotated in 2D mode, terminal permitting.\n\t", stderr);
    else
	fputs(" and are not rotated\n\t", stderr);

    switch (tdef->type) {
    case TIC_COMPUTED:{
	    fputs("  intervals computed automatically\n", stderr);
	    break;
	}
    case TIC_MONTH:{
	    fputs("  Months computed automatically\n", stderr);
	    break;
	}
    case TIC_DAY:{
	    fputs("  Days computed automatically\n", stderr);
	    break;
	}
    case TIC_SERIES:{
	    fputs("  series", stderr);
	    if (tdef->def.series.start != -VERYLARGE) {
		fputs(" from ", stderr);
		SHOW_NUM_OR_TIME(tdef->def.series.start, axis);
	    }
	    fprintf(stderr, " by %g%s", tdef->def.series.incr, datatype[axis] == TIME ? " secs" : "");
	    if (tdef->def.series.end != VERYLARGE) {
		fputs(" until ", stderr);
		SHOW_NUM_OR_TIME(tdef->def.series.end, axis);
	    }
	    putc('\n', stderr);
	    break;
	}
    case TIC_USER:{
/* this appears to be unused? lh
	    int time;
	    time = (datatype[axis] == TIME);
 */
	    fputs("  list (", stderr);
	    for (t = tdef->def.user; t != NULL; t = t->next) {
		if (t->label)
		    fprintf(stderr, "\"%s\" ", conv_text(t->label));
		SHOW_NUM_OR_TIME(t->position, axis);
		if (t->next)
		    fputs(", ", stderr);
	    }
	    fputs(")\n", stderr);
	    break;
	}
    default:{
	    int_error(NO_CARET, "unknown ticdef type in show_ticdef()");
	    /* NOTREACHED */
	}
    }
}


static void
show_margin()
{
    if (lmargin >= 0)
	fprintf(stderr, "\tlmargin is set to %d\n", lmargin);
    else
	fputs("\tlmargin is computed automatically\n", stderr);
    if (bmargin >= 0)
	fprintf(stderr, "\tbmargin is set to %d\n", bmargin);
    else
	fputs("\tbmargin is computed automatically\n", stderr);
    if (rmargin >= 0)
	fprintf(stderr, "\trmargin is set to %d\n", rmargin);
    else
	fputs("\trmargin is computed automatically\n", stderr);
    if (tmargin >= 0)
	fprintf(stderr, "\ttmargin is set to %d\n", tmargin);
    else
	fputs("\ttmargin is computed automatically\n", stderr);
}

static void
show_term()
{
    if (term)
	fprintf(stderr, "\tterminal type is %s %s\n",
		term->name, term_options);
    else
	fputs("\tterminal type is unknown\n", stderr);
}

static void
show_autoscale()
{
    fputs("\tautoscaling is ", stderr);
    if (parametric) {
	if (is_3d_plot) {
	    fprintf(stderr, "\tt: %s%s%s, ",
		    (autoscale_t) ? "ON" : "OFF",
		    (autoscale_t == 1) ? " (min)" : "",
		    (autoscale_t == 2) ? " (max)" : "");
	} else {
	    fprintf(stderr, "\tu: %s%s%s, ",
		    (autoscale_u) ? "ON" : "OFF",
		    (autoscale_u == 1) ? " (min)" : "",
		    (autoscale_u == 2) ? " (max)" : "");
	    fprintf(stderr, "v: %s%s%s, ",
		    (autoscale_v) ? "ON" : "OFF",
		    (autoscale_v == 1) ? " (min)" : "",
		    (autoscale_v == 2) ? " (max)" : "");
	}
    } else
	putc('\t', stderr);

    if (polar) {
	fprintf(stderr, "r: %s%s%s, ", (autoscale_r) ? "ON" : "OFF",
		(autoscale_r == 1) ? " (min)" : "",
		(autoscale_r == 2) ? " (max)" : "");
    }
    fprintf(stderr, "x: %s%s%s, ", (autoscale_x) ? "ON" : "OFF",
	    (autoscale_x == 1) ? " (min)" : "",
	    (autoscale_x == 2) ? " (max)" : "");
    fprintf(stderr, "y: %s%s%s, ", (autoscale_y) ? "ON" : "OFF",
	    (autoscale_y == 1) ? " (min)" : "",
	    (autoscale_y == 2) ? " (max)" : "");
    fprintf(stderr, "x2: %s%s%s, ", (autoscale_x2) ? "ON" : "OFF",
	    (autoscale_x2 == 1) ? " (min)" : "",
	    (autoscale_x2 == 2) ? " (max)" : "");
    fprintf(stderr, "y2: %s%s%s, ", (autoscale_y2) ? "ON" : "OFF",
	    (autoscale_y2 == 1) ? " (min)" : "",
	    (autoscale_y2 == 2) ? " (max)" : "");
    fprintf(stderr, "z: %s%s%s\n", (autoscale_z) ? "ON" : "OFF",
	    (autoscale_z == 1) ? " (min)" : "",
	    (autoscale_z == 2) ? " (max)" : "");
}

static void
show_clip()
{
    fprintf(stderr, "\tpoint clip is %s\n", (clip_points) ? "ON" : "OFF");

    if (clip_lines1)
	fputs("\tdrawing and clipping lines between inrange and outrange points\n", stderr);
    else
	fputs("\tnot drawing lines between inrange and outrange points\n", stderr);

    if (clip_lines2)
	fputs("\tdrawing and clipping lines between two outrange points\n", stderr);
    else
	fputs("\tnot drawing lines between two outrange points\n", stderr);
}

static void
show_mapping()
{
    fputs("\tmapping for 3-d data is ", stderr);

    switch (mapping3d) {
    case MAP3D_CARTESIAN:
	fputs("cartesian\n", stderr);
	break;
    case MAP3D_SPHERICAL:
	fputs("spherical\n", stderr);
	break;
    case MAP3D_CYLINDRICAL:
	fputs("cylindrical\n", stderr);
	break;
    }
}

static void
show_contour()
{
    fprintf(stderr, "\tcontour for surfaces are %s",
	    (draw_contour) ? "drawn" : "not drawn\n");

    if (draw_contour) {
	fprintf(stderr, " in %d levels on ", contour_levels);
	switch (draw_contour) {
	case CONTOUR_BASE:
	    fputs("grid base\n", stderr);
	    break;
	case CONTOUR_SRF:
	    fputs("surface\n", stderr);
	    break;
	case CONTOUR_BOTH:
	    fputs("grid base and surface\n", stderr);
	    break;
	}
	switch (contour_kind) {
	case CONTOUR_KIND_LINEAR:
	    fputs("\t\tas linear segments\n", stderr);
	    break;
	case CONTOUR_KIND_CUBIC_SPL:
	    fprintf(stderr, "\t\tas cubic spline interpolation segments with %d pts\n", contour_pts);
	    break;
	case CONTOUR_KIND_BSPLINE:
	    fprintf(stderr, "\t\tas bspline approximation segments of order %d with %d pts\n", contour_order, contour_pts);
	    break;
	}
	switch (levels_kind) {
	case LEVELS_AUTO:
	    fprintf(stderr, "\t\tapprox. %d automatic levels\n", contour_levels);
	    break;
	case LEVELS_DISCRETE:
	    {
		int i;
		fprintf(stderr, "\t\t%d discrete levels at ", contour_levels);
		fprintf(stderr, "%g", levels_list[0]);
		for (i = 1; i < contour_levels; i++)
		    fprintf(stderr, ",%g ", levels_list[i]);
		putc('\n', stderr);
		break;
	    }
	case LEVELS_INCREMENTAL:
	    fprintf(stderr, "\t\t%d incremental levels starting at %g, step %g, end %g\n", contour_levels, levels_list[0],
		    levels_list[1],
		    levels_list[0] + (contour_levels - 1) * levels_list[1]);
	    /* contour-levels counts both ends */
	    break;
	}
	/* fprintf(stderr,"\t\tcontour line types are %s\n", label_contours ? "varied" : "all the same"); */
	show_label_contours();
    }
}

static void
show_format()
{
    fprintf(stderr, "\ttic format is x-axis: \"%s\"", conv_text(xformat));
    fprintf(stderr, ", y-axis: \"%s\"", conv_text(yformat));
    fprintf(stderr, ", z-axis: \"%s\"", conv_text(zformat));
    fprintf(stderr, ", x2-axis: \"%s\"", conv_text(x2format));
    fprintf(stderr, ", y2-axis: \"%s\"\n", conv_text(y2format));
}

#define SHOW_LOG(FLAG, BASE, TEXT) \
if (FLAG) fprintf(stderr, "%s %s (base %g)", !count++ ? "\tlogscaling" : " and", TEXT,BASE)

static void
show_logscale()
{
    int count = 0;

    SHOW_LOG(is_log_x, base_log_x, "x");
    SHOW_LOG(is_log_y, base_log_y, "y");
    SHOW_LOG(is_log_z, base_log_z, "z");
    SHOW_LOG(is_log_x2, base_log_x2, "x2");
    SHOW_LOG(is_log_y2, base_log_y2, "y2");

    if (count == 0)
	fputs("\tno logscaling\n", stderr);
    else if (count == 1)
	fputs(" only\n", stderr);
    else
	putc('\n', stderr);
}

static void
show_variables()
{
    register struct udvt_entry *udv = first_udv;
    int len;

    fputs("\n\tVariables:\n", stderr);
    while (udv) {
	len = strcspn(udv->udv_name, " ");
	fprintf(stderr, "\t%-*s ", len, udv->udv_name);
	if (udv->udv_undef)
	    fputs("is undefined\n", stderr);
	else {
	    fputs("= ", stderr);
	    disp_value(stderr, &(udv->udv_value));
	    (void) putc('\n', stderr);
	}
	udv = udv->next_udv;
    }
}

/*
 * Used in plot.c and misc.c
 * By adding FILE *fp, we can now use show_version()
 * to save version information to a file.
 */
void
show_version(fp)
FILE *fp;
{
    /* If printed to a file, we prefix everything with
     * a hash mark to comment out the version information.
     */
    char prefix[6];		/* "#    " */
    char *p = prefix;

    prefix[0] = '#';
    prefix[1] = prefix[2] = prefix[3] = prefix[4] = ' ';
    prefix[5] = NUL;

    if (fp == stderr) {
	/* No hash mark - let p point to the trailing '\0' */
	p += sizeof(prefix) - 1;
    } else {
#ifdef BINDIR
# ifdef X11
	fprintf(fp, "#!%s/gnuplot -persist\n#\n", BINDIR);
#  else
	fprintf(fp, "#!%s/gnuplot\n#\n", BINDIR);
# endif				/* not X11 */
#endif /* BINDIR */
    }
    fprintf(fp, "%s\n\
%s\t%s\n\
%s\t%sversion %s\n\
%s\tpatchlevel %s\n\
%s\tlast modified %s\n\
%s\n\
%s\t%s\n\
%s\tThomas Williams, Colin Kelley and many others\n\
%s\n\
%s\tType `help` to access the on-line reference manual\n\
%s\tThe gnuplot FAQ is available from\n\
%s\t\t%s\n\
%s\n\
%s\tSend comments and requests for help to <%s>\n\
%s\tSend bugs, suggestions and mods to <%s>\n\
%s\n",
	    p,			/* empty line */
	    p, PROGRAM,
	    p, OS, gnuplot_version,
	    p, gnuplot_patchlevel,
	    p, gnuplot_date,
	    p,			/* empty line */
	    p, gnuplot_copyright,
	    p,			/* authors */
	    p,			/* empty line */
	    p,			/* Type help */
	    p,			/* FAQ is at */
	    p, faq_location,
	    p,			/* empty line */
	    p, help_email,
	    p, bug_email,
	    p);			/* empty line */
}

void
show_version_long()
{
    char *helpfile = NULL;
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname uts;

    /* something is fundamentally wrong if this fails ... */
    if (uname(&uts) > -1) {
# ifdef _AIX
	fprintf(stderr, "\nSystem: %s %s.%s", uts.sysname, uts.version, uts.release);
# elif defined (SCO)
	fprintf(stderr, "\nSystem: SCO %s", uts.release);
# else
	fprintf(stderr, "\nSystem: %s %s", uts.sysname, uts.release);
# endif
    } else {
	fprintf(stderr, "\n%s\n", OS);
    }

#else /* ! HAVE_SYS_UTSNAME_H */

    fprintf(stderr, "\n%s\n", OS);

#endif /* HAVE_SYS_UTSNAME_H */

    fputs("\nCompile options:\n", stderr);

    {
	/* The following code could be a lot simpler if
	 * it wasn't for Borland's broken compiler ...
	 */
	const char *rdline, *gnu_rdline, *libgd, *libpng, *linuxvga, *nocwdrc, *x11, *unixplot, *gnugraph;

	rdline =
#ifdef READLINE
	    "+READLINE  "
#else
	    "-READLINE  "
#endif
	    ,gnu_rdline =
#ifdef HAVE_LIBREADLINE
	    "+LIBREADLINE  "
# ifdef GNUPLOT_HISTORY
	    "+HISTORY  "
#else
	    "-HISTORY  "
# endif
#else
	    "-LIBREADLINE  "
#endif
	    ,libgd =
#ifdef HAVE_LIBGD
	    "+LIBGD  "
#else
	    "-LIBGD  "
#endif
	    ,libpng =
#ifdef HAVE_LIBPNG
	    "+LIBPNG  "
#else
	    "-LIBPNG  "
#endif
	    ,linuxvga =
#ifdef LINUXVGA
	    "+LINUXVGA  "
#else
	    ""
#endif
	    ,nocwdrc =
#ifdef NOCWDRC
	    "+NOCWDRC  "
#else
	    "-NOCWDRC  "
#endif
	    ,x11 =

#ifdef X11
	    "+X11  "
#else
	    ""
#endif
	    ,unixplot =
#ifdef UNIXPLOT
	    "+UNIXPLOT  "
#else
	    ""
#endif
	    ,gnugraph =
#ifdef GNUGRAPH
	    "+GNUGRAPH  "
#else
	    ""
#endif
	    ;
	fprintf(stderr, "%s%s%s%s%s%s%s%s%s\n\n", rdline, gnu_rdline,
		libgd, libpng, linuxvga, nocwdrc, x11, unixplot, gnugraph);
    }

    if ((helpfile = getenv("GNUHELP")) == NULL) {
#if defined(ATARI) || defined(MTOS)
	if ((helpfile = user_gnuplotpath) == NULL) {
	    helpfile = HELPFILE;
	}
#else
	helpfile = HELPFILE;
#endif
    }
    fprintf(stderr, "HELPFILE     = \"%s\"\n\
CONTACT      = <%s>\n\
HELPMAIL     = <%s>\n", helpfile, bug_email, help_email);
}


/* convert unprintable characters as \okt, tab as \t, newline \n .. */
char *
conv_text(t)
const char *t;
{
    static char *s = NULL, *r;

    if (s)
       s = r;

    /* is this enough? */
    s = gp_realloc(s, 4 * (strlen(t) + 1), "conv_text buffer");
    r = s;

    while (*t != NUL) {
	switch (*t) {
	case '\t':
	    *s++ = '\\';
	    *s++ = 't';
	    break;
	case '\n':
	    *s++ = '\\';
	    *s++ = 'n';
	    break;
#ifndef OSK
	case '\r':
	    *s++ = '\\';
	    *s++ = 'r';
	    break;
#endif
	case '"':
	case '\\':
	    *s++ = '\\';
	    *s++ = *t;
	    break;

	default:{
		if ((*t & 0177) > 31 && (*t & 0177) < 127)
		    *s++ = *t;
		else {
		    *s++ = '\\';
		    sprintf(s, "%o", *t);
		    while (*s != NUL)
			s++;
		}
	    }
	    break;
	}
	t++;
    }
    *s = NUL;
    return r;
}


static void
show_position(pos)
struct position *pos;
{
    static const char *msg[] = { "(first axes) ", "(second axes) ",
				 "(graph units) ", "(screen units) " };

    assert(first_axes == 0 && second_axes == 1 && graph == 2 && screen == 3);

    fprintf(stderr, "(%s%g, %s%g, %s%g)",
	    pos->scalex == first_axes ? "" : msg[pos->scalex], pos->x,
	    pos->scaley == pos->scalex ? "" : msg[pos->scaley], pos->y,
	    pos->scalez == pos->scaley ? "" : msg[pos->scalez], pos->z);

}


static void
show_at()
{
    (void) putc('\n', stderr);
    disp_at(temp_at(), 0);
}


static void
disp_at(curr_at, level)
struct at_type *curr_at;
int level;
{
    register int i, j;
    register union argument *arg;

    for (i = 0; i < curr_at->a_count; i++) {
	(void) putc('\t', stderr);
	for (j = 0; j < level; j++)
	    (void) putc(' ', stderr);   /* indent */

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
		    disp_at(arg->udf_arg->at, level + 2);       /* recurse! */
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
		    disp_at(arg->udf_arg->at, level + 2);       /* recurse! */
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


static void
show_functions()
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


static void
show_missing()
{
    if (missing_val == NULL)
	fputs("\tNo string is interpreted as missing data\n", stderr);
    else
	fprintf(stderr, "\t\"%s\" is interpreted as missing value\n", missing_val);
}
