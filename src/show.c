#ifndef lint
static char *RCSid() { return RCSid("$Id: show.c,v 1.36.2.7 2000/10/23 04:35:28 joze Exp $"); }
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

#include "setshow.h"

#include "alloc.h"
#include "command.h"
#include "eval.h"
#include "gp_time.h"
#include "hidden3d.h"
#include "parse.h"
#include "tables.h"
#include "util.h"
#ifdef USE_MOUSE
# include "mouse.h"
#endif

#ifdef PM3D
# include "pm3d.h"
#endif

/******** Local functions ********/

static void show_at __PROTO((void));
static void disp_at __PROTO((struct at_type *, int));
static void show_all __PROTO((void));
static void show_autoscale __PROTO((void));
static void show_bars __PROTO((void));
static void show_border __PROTO((void));
static void show_boxwidth __PROTO((void));
static void show_clip __PROTO((void));
static void show_contour __PROTO((void));
static void show_dgrid3d __PROTO((void));
static void show_label_contours __PROTO((void));
static void show_mapping __PROTO((void));
static void show_dummy __PROTO((void));
static void show_format __PROTO((void));
static void show_styles __PROTO((const char *name, enum PLOT_STYLE style));
static void show_style __PROTO((void));
static void show_grid __PROTO((void));
static void show_xzeroaxis __PROTO((void));
static void show_yzeroaxis __PROTO((void));
static void show_label __PROTO((int tag));
static void show_keytitle __PROTO((void));
static void show_key __PROTO((void));
static void show_logscale __PROTO((void));
static void show_offsets __PROTO((void));
static void show_margin __PROTO((void));
static void show_output __PROTO((void));
static void show_parametric __PROTO((void));
#ifdef PM3D
static void show_pm3d __PROTO((void));
static void show_palette __PROTO((void));
#endif
static void show_pointsize __PROTO((void));
static void show_encoding __PROTO((void));
static void show_polar __PROTO((void));
static void show_angles __PROTO((void));
static void show_samples __PROTO((void));
static void show_isosamples __PROTO((void));
static void show_view __PROTO((void));
static void show_surface __PROTO((void));
static void show_hidden3d __PROTO((void));
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
static void show_historysize __PROTO((void));
#endif
static void show_size __PROTO((void));
static void show_origin __PROTO((void));
static void show_term __PROTO((void));
static void show_tics __PROTO((TBOOLEAN showx, TBOOLEAN showy, TBOOLEAN showz, TBOOLEAN showx2, TBOOLEAN showy2));
static void show_mtics __PROTO((int mini, double freq, const char *name));
static void show_timestamp __PROTO((void));
static void show_range __PROTO((int axis, double min, double max, TBOOLEAN autosc, const char *text));
static void show_xyzlabel __PROTO((const char *name, label_struct * label));
static void show_title __PROTO((void));
static void show_xlabel __PROTO((void));
static void show_ylabel __PROTO((void));
static void show_zlabel __PROTO((void));
static void show_x2label __PROTO((void));
static void show_y2label __PROTO((void));
static void show_datatype __PROTO((const char *, int));
static void show_xdata __PROTO((void));
static void show_ydata __PROTO((void));
static void show_zdata __PROTO((void));
static void show_x2data __PROTO((void));
static void show_y2data __PROTO((void));
static void show_timefmt __PROTO((void));
static void show_locale __PROTO((void));
static void show_loadpath __PROTO((void));
static void show_zero __PROTO((void));
static void show_missing __PROTO((void));
#ifdef USE_MOUSE
static void show_mouse __PROTO((void));
#endif
static void show_plot __PROTO((void));
static void show_variables __PROTO((void));

static void show_linestyle __PROTO((int tag));
static void show_arrow __PROTO((int tag));

static void show_ticdef __PROTO((int tics, int axis, struct ticdef * tdef, const char *text, int rotate_tics, const char *ticfmt));
static void show_position __PROTO((struct position * pos));
static void show_functions __PROTO((void));

static int var_show_all = 0;


/* following code segment appears over and over again */

#define SHOW_NUM_OR_TIME(x, axis) \
 do { \
  if (datatype[axis]==TIME) { \
   char s[80]; char *p; \
   putc('"', stderr); \
   gstrftime(s,80,timefmt,(double)(x)); \
   for(p=s; *p; ++p) { \
    if ( *p == '\t' ) \
     fputs("\\t",stderr); \
    else if (*p == '\n') \
     fputs("\\n",stderr); \
    else if ( *p > 126 || *p < 32 ) \
     fprintf(stderr,"\\%03o",*p); \
    else \
     putc(*p, stderr); \
   } \
   putc('"', stderr); \
  } else \
   fprintf(stderr,"%#g",x); \
 } while(0)

#define SHOW_ALL_NL { if (!var_show_all) (void) putc('\n',stderr); }

#define CHECK_TAG_GT_ZERO \
 c_token++; \
 if (!END_OF_COMMAND) { \
  tag = (int) real(const_express(&a)); \
  if (tag <= 0) \
   int_error(c_token, "tag must be > zero"); \
 }

/******* The 'show' command *******/
void
show_command()
{
    /* show at is undocumented/hidden... */
    static char GPFAR showmess[] =
    "valid set options:  [] = choose one, {} means optional\n\n\
\t'all',  'angles',  'arrow',  'autoscale',  'bars', 'border',\n\
\t'boxwidth', 'clip', 'cntrparam', 'contour',  'dgrid3d', 'dummy',\n\
\t'encoding', 'format', 'functions',  'grid',  'hidden', 'isosamples',\n\
\t'key', 'label', 'loadpath', 'locale', 'logscale', 'mapping',\n\
\t'margin', 'missing', 'offsets', 'origin', 'output', 'plot',\n\
\t'palette', 'parametric', 'pm3d', 'pointsize', 'polar',\n\
\t'[rtuv]range', 'samples', 'size', 'style', 'terminal', 'tics',\n\
\t'timestamp', 'timefmt', 'title', 'variables', 'version', 'view',\n\
\t'[xyz]{2}label',   '[xyz]{2}range', '{m}[xyz]{2}tics',\n\
\t'[xyz]{2}[md]tics', '[xyz]{2}zeroaxis', '[xyz]data',\n\
\t'zero', 'zeroaxis'";


    struct value a;
    double tag =0.;
    int x_and_y_zeroax = 0;

    c_token++;

    switch(lookup_table(&set_tbl[0],c_token)) {
    case S_ACTIONTABLE:
	show_at();
	break;
    case S_ALL:
	show_all();
	break;
    case S_VERSION:
	show_version(stderr);
	break;
    case S_AUTOSCALE:
	show_autoscale();
	break;
    case S_BARS:
	show_bars();
	break;
    case S_BORDER:
	show_border();
	break;
    case S_BOXWIDTH:
	show_boxwidth();
	break;
    case S_CLIP:
	show_clip();
	break;
    case S_CONTOUR:
    case S_CNTRPARAM:
	show_contour();
	break;
    case S_DGRID3D:
	show_dgrid3d();
	break;
    case S_MAPPING:
	show_mapping();
	break;
    case S_DUMMY:
	show_dummy();
	break;
    case S_FORMAT:
	show_format();
	break;
    case S_FUNCTIONS:
	show_functions();
	break;
    case S_GRID:
	show_grid();
	break;
    /* FIXME */
    case S_ZEROAXIS:
	x_and_y_zeroax = 1;
    case S_XZEROAXIS:
	show_xzeroaxis();
	if (!x_and_y_zeroax)
	    break;
	c_token--;
    case S_YZEROAXIS:
	show_yzeroaxis();
	break;
    case S_LABEL:
	c_token++;
	if (!END_OF_COMMAND) {
	    tag = real(const_express(&a));
	    if ((int)tag <= 0)
		int_error(c_token, "tag must be > zero");
	}
	(void) putc('\n',stderr);
	show_label((int)tag);
	break;
    case S_ARROW:
	c_token++;
	if (!END_OF_COMMAND) {
	    tag = real(const_express(&a));
	    if ((int)tag <= 0)
		int_error(c_token, "tag must be > zero");
	}
	(void) putc('\n',stderr);
	show_arrow((int)tag);
	break;
/*
    case S_LINESTYLE:
	c_token++;
	if (!END_OF_COMMAND) {
	    tag = (int)real(const_express(&a));
	    if (tag <= 0)
		int_error(c_token, "tag must be > zero");
	}
	(void) putc('\n',stderr);
	show_linestyle(tag);
	break;
*/
    case S_KEYTITLE:
	show_keytitle();
	break;
    case S_KEY:
	show_key();
	break;
    case S_LOGSCALE:
	show_logscale();
	break;
    case S_OFFSETS:
	show_offsets();
	break;
    case S_MARGIN:
	show_margin();
	break;
    case S_OUTPUT:
	show_output();
	break;
    case S_PARAMETRIC:
	show_parametric();
	break;
#ifdef PM3D
    case S_PALETTE:
	show_palette();
	break;
    case S_PM3D:
	show_pm3d();
	break;
#endif
    case S_POINTSIZE:
	show_pointsize();
	break;
    case S_ENCODING:
	show_encoding();
	break;
    case S_POLAR:
	show_polar();
	break;
    case S_ANGLES:
	show_angles();
	break;
    case S_SAMPLES:
	show_samples();
	break;
    case S_ISOSAMPLES:
	show_isosamples();
	break;
    case S_VIEW:
	show_view();
	break;
    case S_STYLE:
	show_style();
	break;
    case S_SURFACE:
	show_surface();
	break;
    case S_HIDDEN3D:
	show_hidden3d();
	break;
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
    case S_HISTORYSIZE:
	show_historysize();
	break;
#endif
    case S_SIZE:
	show_size();
	break;
    case S_ORIGIN:
	show_origin();
	break;
    case S_TERMINAL:
	show_term();
	break;
    case S_TICS:
	show_tics(TRUE, TRUE, TRUE, TRUE, TRUE);
	break;
    case S_MXTICS:
	show_mtics(mxtics, mxtfreq, "x");
	break;
    case S_MYTICS:
	show_mtics(mytics, mytfreq, "y");
	break;
    case S_MZTICS:
	show_mtics(mztics, mztfreq, "z");
	break;
    case S_MX2TICS:
	show_mtics(mx2tics, mx2tfreq, "x2");
	break;
    case S_MY2TICS:
	show_mtics(my2tics, my2tfreq, "y2");
	break;
    case S_TIMESTAMP:
	show_timestamp();
	break;
    case S_RRANGE:
	show_range(R_AXIS, rmin, rmax, autoscale_r, "r");
	break;
    case S_TRANGE:
	show_range(T_AXIS, tmin, tmax, autoscale_t, "t");
	break;
    case S_URANGE:
	show_range(U_AXIS, umin, umax, autoscale_u, "u");
	break;
    case S_VRANGE:
	show_range(V_AXIS, vmin, vmax, autoscale_v, "v");
	break;
    case S_XRANGE:
	show_range(FIRST_X_AXIS, xmin, xmax, autoscale_x, "x");
	break;
    case S_YRANGE:
	show_range(FIRST_Y_AXIS, ymin, ymax, autoscale_y, "y");
	break;
    case S_X2RANGE:
	show_range(SECOND_X_AXIS, x2min, x2max, autoscale_x2, "x2");
	break;
    case S_Y2RANGE:
	show_range(SECOND_Y_AXIS, y2min, y2max, autoscale_y2, "y2");
	break;
    case S_ZRANGE:
	show_range(FIRST_Z_AXIS, zmin, zmax, autoscale_z, "z");
	break;
    case S_TITLE:
	show_title();
	break;
    case S_XLABEL:
	show_xlabel();
	break;
    case S_YLABEL:
	c_token++;
	show_ylabel();
	break;
    case S_ZLABEL:
	show_zlabel();
	break;
    case S_X2LABEL:
	show_x2label();
	break;
    case S_Y2LABEL:
	show_y2label();
	break;
    case S_XDATA:
	show_xdata();
	break;
    case S_YDATA:
	show_ydata();
	break;
    case S_X2DATA:
	show_x2data();
	break;
    case S_Y2DATA:
	show_y2data();
	break;
    case S_ZDATA:
	show_zdata();
	break;
    case S_TIMEFMT:
	show_timefmt();
	break;
    case S_LOCALE:
	show_locale();
	break;
    case S_LOADPATH:
	show_loadpath();
	break;
    case S_ZERO:
	show_zero();
	break;
    case S_MISSING:
	show_missing();
	break;
#ifdef USE_MOUSE
    case S_MOUSE:
	show_mouse();
	break;
#endif
    case S_PLOT:
	show_plot();
	break;
    case S_VARIABLES:
	show_variables();
	break;
    case S_XTICS:
	show_tics(TRUE, FALSE, FALSE, TRUE, FALSE);
	break;
    case S_YTICS:
	show_tics(FALSE, TRUE, FALSE, FALSE, TRUE);
	break;
    case S_ZTICS:
	show_tics(FALSE, FALSE, TRUE, FALSE, FALSE);
	break;
    case S_X2TICS:
	show_tics(FALSE, FALSE, FALSE, TRUE, FALSE);
	break;
    case S_Y2TICS:
	show_tics(FALSE, FALSE, FALSE, FALSE, TRUE);
	break;
    case S_XDTICS:
	show_tics(TRUE, FALSE, FALSE, TRUE, FALSE);
	break;
    case S_YDTICS:
	show_tics(FALSE, TRUE, FALSE, FALSE, TRUE);
	break;
    case S_ZDTICS:
	show_tics(FALSE, FALSE, TRUE, FALSE, FALSE);
	break;
    case S_X2DTICS:
	show_tics(FALSE, FALSE, FALSE, TRUE, FALSE);
	break;
    case S_Y2DTICS:
	show_tics(FALSE, FALSE, FALSE, FALSE, TRUE);
	break;
    case S_XMTICS:
	show_tics(TRUE, FALSE, FALSE, TRUE, FALSE);
	break;
    case S_YMTICS:
	show_tics(FALSE, TRUE, FALSE, FALSE, TRUE);
	break;
    case S_ZMTICS:
	show_tics(FALSE, FALSE, TRUE, FALSE, FALSE);
	break;
    case S_X2MTICS:
	show_tics(FALSE, FALSE, FALSE, TRUE, FALSE);
	break;
    case S_Y2MTICS:
	show_tics(FALSE, FALSE, FALSE, FALSE, TRUE);
	break;
    case S_INVALID:
	int_error(c_token, showmess);
    case S_CLABEL:
	/* contour labels are shown with 'show contour' */
    default:
	break;
    }

    screen_ok = FALSE;
    (void) putc('\n', stderr);

}


/* process 'show actiontable|at' command
 * not documented
 */
static void
show_at()
{
    c_token++;
    (void) putc('\n', stderr);
    disp_at(temp_at(), 0);
    c_token++;
}


/* called by show_at() */
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


/* process 'show all' command */
static void
show_all()
{
    var_show_all = 1;

    c_token++;

    show_version(stderr);
    show_autoscale();
    show_bars();
    show_border();
    show_boxwidth();
    show_clip();
    show_label_contours();
    show_contour();
    show_dgrid3d();
    show_mapping();
    show_dummy();
    show_format();
    show_style();
/*
    show_styles("data",data_style);
    show_styles("functions",func_style);
*/
    show_grid();
    show_xzeroaxis();
    show_yzeroaxis();
    show_label(0);
    show_arrow(0);
/*
    show_linestyle(0);
*/
    show_keytitle();
    show_key();
    show_logscale();
    show_offsets();
    show_margin();
    show_output();
    show_parametric();
#ifdef PM3D
	show_palette();
	show_pm3d();
#endif
    show_pointsize();
    show_encoding();
    show_polar();
    show_angles();
    show_samples();
    show_isosamples();
    show_view();
    show_surface();
    show_hidden3d();
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
    show_historysize();
#endif
    show_size();
    show_origin();
    show_term();
    show_tics(TRUE,TRUE,TRUE,TRUE,TRUE);
    show_mtics(mxtics, mxtfreq, "x");
    show_mtics(mytics, mytfreq, "y");
    show_mtics(mztics, mztfreq, "z");
    show_mtics(mx2tics, mx2tfreq, "x2");
    show_mtics(my2tics, my2tfreq, "y2");
    show_xyzlabel("time", &timelabel);
    if (parametric || polar) {
	if (!is_3d_plot)
	    show_range(T_AXIS,tmin,tmax,autoscale_t, "t");
	else {
	    show_range(U_AXIS,umin,umax,autoscale_u, "u");
	    show_range(V_AXIS,vmin,vmax,autoscale_v, "v");
	}
    }
    show_range(FIRST_X_AXIS,xmin,xmax,autoscale_x, "x");
    show_range(FIRST_Y_AXIS,ymin,ymax,autoscale_y, "y");
    show_range(SECOND_X_AXIS,x2min,x2max,autoscale_x2, "x2");
    show_range(SECOND_Y_AXIS,y2min,y2max,autoscale_y2, "y2");
    show_range(FIRST_Z_AXIS,zmin,zmax,autoscale_z, "z");
    show_xyzlabel("title", &title);
    show_xyzlabel("xlabel", &xlabel);
    show_xyzlabel("ylabel", &ylabel);
    show_xyzlabel("zlabel", &zlabel);
    show_xyzlabel("x2label", &x2label);
    show_xyzlabel("y2label", &y2label);
    show_datatype("xdata", FIRST_X_AXIS);
    show_datatype("ydata", FIRST_Y_AXIS);
    show_datatype("x2data", SECOND_X_AXIS);
    show_datatype("y2data", SECOND_Y_AXIS);
    show_datatype("zdata", FIRST_Z_AXIS);
    show_timefmt();
    show_loadpath();
    show_locale();
    show_zero();
    show_missing();
#ifdef USE_MOUSE
    show_mouse();
#endif
    show_plot();
    show_variables();
    show_functions();

    var_show_all = 0;
}


/* process 'show version' command */
void
show_version(fp)
FILE *fp;
{
    /* If printed to a file, we prefix everything with
     * a hash mark to comment out the version information.
     */
    char prefix[6];		/* "#    " */
    char *p = prefix;

    c_token++;

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
%s\tVersion %s patchlevel %s\n\
%s\tlast modified %s\n\
%s\tSystem: %s %s\n\
%s\n\
%s\t%s\n\
%s\tThomas Williams, Colin Kelley and many others\n\
%s\n\
%s\tThis is a pre-version of gnuplot 4.0. Please refer to the documentation\n\
%s\tfor command syntax changes. The old syntax will be accepted throughout\n\
%s\tthe 4.0 series, but all save files use the new syntax.\n\
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
	    p, gnuplot_version, gnuplot_patchlevel,
	    p, gnuplot_date,
	    p, os_name, os_rel,
	    p,			/* empty line */
	    p, gnuplot_copyright,
	    p,			/* authors */
	    p,			/* empty line */
	    p,			/* 4.0 info */
	    p,			/* 4.0 info */
	    p,			/* 4.0 info */
	    p,			/* empty line */
	    p,			/* Type `help` */
	    p,			/* FAQ is at */
	    p, faq_location,
	    p,			/* empty line */
	    p, help_email,
	    p, bug_email,
	    p);			/* empty line */


    /* show version long */
    if (almost_equals(c_token, "l$ong")) {
	char *helpfile = NULL;

	c_token++;

	fputs("Compile options:\n", stderr);

	{
	    /* The following code could be a lot simpler if
	     * it wasn't for Borland's broken compiler ...
	     */
	    const char *rdline, *gnu_rdline, *libgd, *libpng, *linuxvga,
			*nocwdrc, *x11, *use_mouse, *unixplot, *gnugraph;

	    rdline =
#ifdef READLINE
		"+READLINE  "
#else
		"-READLINE  "
#endif
		, gnu_rdline =
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
		, libgd =
#ifdef HAVE_LIBGD
		"+LIBGD  "
#else
		"-LIBGD  "
#endif
		, libpng =
#ifdef HAVE_LIBPNG
		"+LIBPNG  "
#else
		"-LIBPNG  "
#endif
		, linuxvga =
#ifdef LINUXVGA
		"+LINUXVGA  "
#else
		""
#endif
		, nocwdrc =
#ifdef NOCWDRC
		"+NOCWDRC  "
#else
		"-NOCWDRC  "
#endif
		, x11 =

#ifdef X11
		"+X11  "
#else
		""
#endif
		, use_mouse =

#ifdef USE_MOUSE
		"+USE_MOUSE  "
#else
		""
#endif
		, unixplot =
#ifdef UNIXPLOT
		"+UNIXPLOT  "
#else
		""
#endif
		, gnugraph =
#ifdef GNUGRAPH
		"+GNUGRAPH  "
#else
		""
#endif
		;
	    fprintf(stderr, "%s%s%s%s%s%s%s%s%s%s\n\n", rdline, gnu_rdline,
		    libgd, libpng, linuxvga, nocwdrc, x11, use_mouse,
		    unixplot, gnugraph);
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

	fprintf(stderr, "\
HELPFILE     = \"%s\"\n\
CONTACT      = <%s>\n\
HELPMAIL     = <%s>\n", helpfile, bug_email, help_email);

    }
}


#define SHOW_AUTOSCALE(axis,scvar) \
  fprintf(stderr, "\t%s: %s%s%s, ", \
    axis, (scvar) ? "ON" : "OFF", \
    (scvar == 1) ? " (min)" : "", (scvar == 2) ? " (max)" : "");

/* process 'show autoscale' command */
static void
show_autoscale()
{
    c_token++;

    SHOW_ALL_NL;

    fputs("\tautoscaling is ", stderr);
    if (parametric) {
	if (is_3d_plot) {
	    SHOW_AUTOSCALE("t",autoscale_t);
	} else {
	    SHOW_AUTOSCALE("u",autoscale_u);
	    SHOW_AUTOSCALE("v",autoscale_u);
	}
    } else
	putc('\t', stderr);

    if (polar) {
	SHOW_AUTOSCALE("r",autoscale_r);
    }

    SHOW_AUTOSCALE("x",autoscale_x);
    SHOW_AUTOSCALE("y",autoscale_y);
    SHOW_AUTOSCALE("x2",autoscale_x2);
    SHOW_AUTOSCALE("y2",autoscale_y2);
    SHOW_AUTOSCALE("z",autoscale_x);

}


/* process 'show bars' command */
static void
show_bars()
{
    c_token++;
    SHOW_ALL_NL;

    /* I really like this: "terrorbars" ;-) */
    if (bar_size > 0.0)
	fprintf(stderr, "\terrorbars are plotted with bars of size %f\n",
		bar_size);
    else
	fputs("\terrors are plotted without bars\n", stderr);
}


/* process 'show border' command */
static void
show_border()
{
    c_token++;
    SHOW_ALL_NL;

    fprintf(stderr, "\tborder is %sdrawn %d\n", draw_border ? "" : "not ",
	    draw_border);
    /* HBB 980609: added facilities to specify border linestyle */
    fprintf(stderr, "\tBorder drawn with linetype %d, linewidth %.3f\n",
	    border_lp.l_type + 1, border_lp.l_width);
}


/* process 'show boxwidth' command */
static void
show_boxwidth()
{
    c_token++;
    SHOW_ALL_NL;

    if (boxwidth < 0.0)
	fputs("\tboxwidth is auto\n", stderr);
    else
	fprintf(stderr, "\tboxwidth is %g\n", boxwidth);
}


/* process 'show boxwidth' command */
static void
show_clip()
{
    c_token++;
    SHOW_ALL_NL;

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


/* process 'show cntrparam|contour' commands */
static void
show_contour()
{
    c_token++;
    SHOW_ALL_NL;

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


/* called by show_contour() */
static void
show_label_contours()
{
    if (label_contours)
	fprintf(stderr, "\tcontour line types are varied & labeled with format '%s'\n", contour_format);
    else
	fputs("\tcontour line types are all the same\n", stderr);
}


/* process 'show dgrid3d' command */
static void
show_dgrid3d()
{
    c_token++;
    SHOW_ALL_NL;

    if (dgrid3d)
	fprintf(stderr, "\
\tdata grid3d is enabled for mesh of size %dx%d, norm=%d\n",
		dgrid3d_row_fineness,
		dgrid3d_col_fineness,
		dgrid3d_norm_value);
    else
	fputs("\tdata grid3d is disabled\n", stderr);
}


/* process 'show mapping' command */
static void
show_mapping()
{
    c_token++;
    SHOW_ALL_NL;

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


/* process 'show dummy' command */
static void
show_dummy()
{
    c_token++;
    SHOW_ALL_NL;

    fprintf(stderr, "\tdummy variables are \"%s\" and \"%s\"\n",
	    dummy_var[0], dummy_var[1]);
}


/* process 'show format' command */
static void
show_format()
{
    c_token++;
    SHOW_ALL_NL;

    fprintf(stderr, "\ttic format is x-axis: \"%s\"", conv_text(xformat));
    fprintf(stderr, ", y-axis: \"%s\"", conv_text(yformat));
    fprintf(stderr, ", z-axis: \"%s\"", conv_text(zformat));
    fprintf(stderr, ", x2-axis: \"%s\"", conv_text(x2format));
    fprintf(stderr, ", y2-axis: \"%s\"\n", conv_text(y2format));
}


/* process 'show style' sommand */
static void
show_style()
{
    struct value a;
    double tag = 0;

    c_token++;

    switch(lookup_table(&show_style_tbl[0],c_token)){
    case SHOW_STYLE_DATA:
	SHOW_ALL_NL;
	show_styles("data",data_style);
	c_token++;
	break;
    case SHOW_STYLE_FUNCTION:
	SHOW_ALL_NL;
	show_styles("functions", func_style);
	c_token++;
	break;
    case SHOW_STYLE_LINE:
	c_token++;
	if (!END_OF_COMMAND) {
	    tag = real(const_express(&a));
	    if ((int)tag <= 0)
		int_error(c_token, "tag must be > zero");
	}
	(void) putc('\n',stderr);
	show_linestyle((int)tag);
	break;
    default:
	/* show all styles */
	show_styles("data",data_style);
	show_styles("functions", func_style);
	show_linestyle((int)tag);
	break;
    }
}


/* called by show_data() and show_func() */
static void
show_styles(name, style)
const char *name;
enum PLOT_STYLE style;
{

    fprintf(stderr, "\t%s are plotted with ", name);
    /* perhaps these need to be put into a constant array ? */
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


/* called by show_func() */
static void
show_functions()
{
    register struct udft_entry *udf = first_udf;

    c_token++;

    fputs("\n\tUser-Defined Functions:\n", stderr);

    while (udf) {
	if (udf->definition)
	    fprintf(stderr, "\t%s\n", udf->definition);
	else
	    fprintf(stderr, "\t%s is undefined\n", udf->udf_name);
	udf = udf->next_udf;
    }
}


/* process 'show grid' command */
static void
show_grid()
{
    c_token++;
    SHOW_ALL_NL;

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

    fprintf(stderr, "\
\tMajor grid drawn with linetype %d, linewidth %.3f\n\
\tMinor grid drawn with linetype %d, linewidth %.3f\n",
	    grid_lp.l_type + 1, grid_lp.l_width,
	    mgrid_lp.l_type + 1, mgrid_lp.l_width);

    if (polar_grid_angle)
	fprintf(stderr, "\tGrid radii drawn every %f %s\n",
		polar_grid_angle / ang2rad,
		angles_format == ANGLES_DEGREES ? "degrees" : "radians");
}


/* process 'show xzeroaxis' command */
static void
show_xzeroaxis()
{
    c_token++;
    SHOW_ALL_NL;

    if (xzeroaxis.l_type > -3)
	fprintf(stderr, "\
\txzeroaxis is drawn with linestyle %d, linewidth %.3f\n",
		xzeroaxis.l_type + 1, xzeroaxis.l_width);
    else
	fputs("\txzeroaxis is OFF\n", stderr);

    if (x2zeroaxis.l_type > -3)
	fprintf(stderr, "\
\tx2zeroaxis is drawn with linestyle %d, linewidth %.3f\n",
		x2zeroaxis.l_type + 1, x2zeroaxis.l_width);
    else
	fputs("\tx2zeroaxis is OFF\n", stderr);
}

static void
show_yzeroaxis()
{
    c_token++;
    SHOW_ALL_NL;

    if (yzeroaxis.l_type > -3)
	fprintf(stderr, "\
\tyzeroaxis is drawn with linestyle %d, linewidth %.3f\n",
		yzeroaxis.l_type + 1, yzeroaxis.l_width);
    else
	fputs("\tyzeroaxis is OFF\n", stderr);

    if (y2zeroaxis.l_type > -3)
	fprintf(stderr, "\
\ty2zeroaxis is drawn with linestyle %d, linewidth %.3f\n",
		y2zeroaxis.l_type + 1, y2zeroaxis.l_width);
    else
	fputs("\ty2zeroaxis is OFF\n", stderr);
}


/* */
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
	    if (this_label->pointstyle < 0)
		fprintf(stderr, "nopointstyle");
	    else {
		fprintf(stderr, "pointstyle %d offset %f,%f",
		    this_label->pointstyle, this_label->hoffset, this_label->voffset);
	    }
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
		    this_arrow->head ? (this_arrow->head==2 ? " both heads " : "") : " (nohead)",
		    this_arrow->layer ? "front" : "back");
	    show_position(&this_arrow->start);
	    fputs(this_arrow->relative ? " rto " : " to ", stderr);
	    show_position(&this_arrow->end);
	    if (this_arrow->headsize.x > 0) {
		static char *msg[] =
		{"(first x axis) ", "(second x axis) ", "(graph units) ", "(screen units) "};
		fprintf(stderr,"\n\t  arrow head: length %s%g, angle %g deg", 
		   this_arrow->headsize.scalex == first_axes ? "" : msg[this_arrow->headsize.scalex],
		   this_arrow->headsize.x,
		   this_arrow->headsize.y);
	    }
	    putc('\n', stderr);
	}
    }
    if (tag > 0 && !showed)
	int_error(c_token, "arrow not found");
}


/* process 'show keytitle' command */
static void
show_keytitle()
{
    c_token++;
    SHOW_ALL_NL;

    fprintf(stderr, "\tkeytitle is \"%s\"\n", conv_text(key_title));
}


/* process 'show key' command */
static void
show_key()
{
    char *str = gp_alloc(30, "show_key");

    c_token++;
    SHOW_ALL_NL;

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
	free(str);
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
	    fprintf(stderr, "not boxed\n");
	fprintf(stderr, "\tsample length is %g characters\n\
\tvertical spacing is %g characters\n\
\twidth adjustment is %g characters\n\
\tkey title is \"%s\"\n",
		    key_swidth,
		    key_vert_factor,
		    key_width_fix,
		    key_title);
    }
}


/* */
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


#define SHOW_LOG(FLAG, BASE, TEXT) \
 if (FLAG) \
   fprintf(stderr, "%s %s (base %g)", !count++ ? "\tlogscaling" : " and", \
   TEXT,BASE)

/* process 'show logscale' command */
static void
show_logscale()
{
    int count = 0;

    c_token++;
    SHOW_ALL_NL;

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


/* process 'show offsets' command */
static void
show_offsets()
{
    c_token++;
    SHOW_ALL_NL;

    fprintf(stderr, "\toffsets are %g, %g, %g, %g\n", loff, roff, toff, boff);
}


/* process 'show margin' command */
static void
show_margin()
{
    c_token++;
    SHOW_ALL_NL;

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


/* process 'show output' command */
static void
show_output()
{
    c_token++;
    SHOW_ALL_NL;

    if (outstr)
	fprintf(stderr, "\toutput is sent to '%s'\n", outstr);
    else
	fputs("\toutput is sent to STDOUT\n", stderr);
}


/* process 'show parametric' command */
static void
show_parametric()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tparametric is %s\n", (parametric) ? "ON" : "OFF");
}

#ifdef PM3D
static void show_palette()
{
    c_token++;
    /* no option given, i.e. "show palette" */
    if (END_OF_COMMAND) {
	fprintf(stderr,"\tfigure is %s\n",
	    sm_palette.colorMode == SMPAL_COLOR_MODE_GRAY ? "GRAY" : "COLOR");
	fprintf(stderr,"\trgb color mapping formulae are %i,%i,%i\n",
	    sm_palette.formulaR, sm_palette.formulaG, sm_palette.formulaB);
	fprintf(stderr,"\t  * there are %i available rgb color mapping formulae:",
	    sm_palette.colorFormulae);
	/* take the description of the color formulae from the comment to their
	   PostScript definition */
	{
	    extern char *( PostScriptColorFormulae[] );
	    char *s;
	    int i = 0;
	    while ( *(PostScriptColorFormulae[i]) ) {
		if (i % 3 == 0) fprintf(stderr, "\n\t    ");
		s = strchr( PostScriptColorFormulae[ i ], '%' ) + 2;
		fprintf(stderr, "%2i: %-15s",i,s);
		i++;
	    }
	    fprintf(stderr, "\n");
	}
	fprintf(stderr,"\t  * negative numbers mean inverted=negative colour component\n");
	fprintf(stderr,"\t  * thus the ranges in `set pm3d rgbformulae' are -%i..%i\n",
	    sm_palette.colorFormulae-1,sm_palette.colorFormulae-1);
	fprintf(stderr,"\tfigure is %s\n",
	    sm_palette.positive == SMPAL_POSITIVE ? "POSITIVE" : "NEGATIVE");
	fprintf(stderr,"\tall color formulae ARE%s written into output postscript file\n",
	    sm_palette.ps_allcF == 0 ? "" : " NOT");
	fprintf(stderr,"\tallocating ");
	if (sm_palette.use_maxcolors) fprintf(stderr,"MAX %i",sm_palette.use_maxcolors);
	else fprintf(stderr,"ALL remaining");
	fprintf(stderr," color positions for discrete palette terminals\n");
	if (color_box.border) {
	    fprintf(stderr,"\tcolor box with border, ");
	    if (color_box.border_lt_tag >= 0)
		fprintf(stderr,"line type %d is ", color_box.border_lt_tag);
	    else
		fprintf(stderr,"DEFAULT line type is ");
	} else {
	    fprintf(stderr,"\tcolor box without border is ");
	}
	if (color_box.where == SMCOLOR_BOX_NO ) {
	    fprintf(stderr,"NOT drawn\n");
	} else if (color_box.where == SMCOLOR_BOX_DEFAULT ) {
	    fprintf(stderr,"drawn at DEFAULT position\n");
	} else if (color_box.where == SMCOLOR_BOX_USER ) {
	    fprintf(stderr,"drawn at USER position:\n");
	    fprintf(stderr,"\t\torigin: %f, %f\n", color_box.xorigin, color_box.yorigin);
	    fprintf(stderr,"\t\tsize  : %f, %f\n", color_box.xsize  , color_box.ysize  );
	} else {
	    /* should *never* happen */
	    fprintf(stderr, "%s:%d please report this bug to <johannes@zellner.org>\n", __FILE__, __LINE__);
	}
	fprintf(stderr,"\tcolor gradient is %s in the color box\n",
	    color_box.rotation == 'v' ? "VERTICAL" : "HORIZONTAL");
	return;
    }
    /* option: "show palette palette <n>" */
    if (almost_equals(c_token, "pal$ette")) {
	int colors, i;
	struct value a;
	double gray, r, g, b;
	extern double GetColorValueFromFormula (int formula, double x);
	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token,"palette size required");
	colors = (int) real(const_express(&a));
	if (colors<2) colors = 100;
	if (sm_palette.colorMode == SMPAL_COLOR_MODE_GRAY)
	    printf("Gray palette with %i discrete colors\n", colors);
	else
	    printf("Color palette with %i discrete colors, formulae R=%i, G=%i, B=%i\n",
		colors, sm_palette.formulaR, sm_palette.formulaG, sm_palette.formulaB);
	for (i = 0; i < colors; i++) {
	    gray = (double)i / (colors - 1); /* colours equidistantly from [0,1] */
	    if (sm_palette.positive == SMPAL_NEGATIVE)
		gray = 1 - gray; /* needed, since printing without call to set_color() */
	    if (sm_palette.colorMode == SMPAL_COLOR_MODE_GRAY) /* gray scale only */
		r = g = b = gray;
	    else { /* i.e. sm_palette.colorMode == SMPAL_COLOR_MODE_RGB */
		r = GetColorValueFromFormula(sm_palette.formulaR, gray);
		g = GetColorValueFromFormula(sm_palette.formulaG, gray);
		b = GetColorValueFromFormula(sm_palette.formulaB, gray);
	    }
	    printf("%3i. gray=%0.4f, (r,g,b)=(%0.4f,%0.4f,%0.4f), #%02x%02x%02x = %3i %3i %3i\n",
		i, gray, r,g,b,
		(int)(colors*r),(int)(colors*g),(int)(colors*b),
		(int)(colors*r),(int)(colors*g),(int)(colors*b));
	}
	return;
    }
    /* wrong option to "show palette" */
    int_error(c_token,"required 'show palette' or 'show palette palette <n>'");
}


static void show_pm3d()
{
    c_token++;
    if (!pm3d.where[0]) {
	fprintf(stderr, "\tpm3d is OFF\n");
	return;
    }
    fprintf(stderr,"\tpm3d plotted at ");
    { int i=0;
	for ( ; pm3d.where[i]; i++ ) {
	    if (i>0) fprintf(stderr,", then ");
	    switch (pm3d.where[i]) {
		case PM3D_AT_BASE: fprintf(stderr,"BOTTOM"); break;
		case PM3D_AT_SURFACE: fprintf(stderr,"SURFACE"); break;
		case PM3D_AT_TOP: fprintf(stderr,"TOP"); break;
	    }
	}
	fprintf(stderr,"\n");
    }
    if (pm3d.direction != PM3D_SCANS_AUTOMATIC) {
	fprintf(stderr,"\ttaking scans in %s direction\n",
	    pm3d.direction == PM3D_SCANS_FORWARD ? "FORWARD" : "BACKWARD");
    } else {
	fprintf(stderr,"\ttaking scans direction automatically\n");
    }
    fprintf(stderr,"\tsubsequent scans with different nb of pts are ");
    if (pm3d.flush == PM3D_FLUSH_CENTER) fprintf(stderr,"CENTERED\n");
    else fprintf(stderr,"flushed from %s\n",
	pm3d.flush == PM3D_FLUSH_BEGIN ? "BEGIN" : "END");
    fprintf(stderr,"\tclipping: ");
    if (pm3d.clip == PM3D_CLIP_1IN)
	fprintf(stderr,"at least 1 point of the quadrangle in x,y ranges\n");
    else
	fprintf(stderr, "all 4 points of the quadrangle in x,y ranges\n");
    fprintf(stderr,"\tz-range is ");
    if (pm3d.pm3d_zmin==0 && pm3d.pm3d_zmax==0)
	fprintf(stderr,"the same as `set zrange`\n");
    else {
	fprintf(stderr,"[");
	if (pm3d.pm3d_zmin) fprintf(stderr,"%g,",pm3d.zmin);
	else fprintf(stderr,"*,");
	if (pm3d.pm3d_zmax) fprintf(stderr,"%g]\n",pm3d.zmax);
	else fprintf(stderr,"*]\n");
    }
    if (pm3d.hidden3d_tag) {
	fprintf(stderr,"\tpm3d-hidden3d is on an will use linestyle %d\n",
	    pm3d.hidden3d_tag);
    } else {
	fprintf(stderr,"\tpm3d-hidden3d is off\n");
    }
    if (pm3d.solid) {
	fprintf(stderr,"\tborders, tics and labels may be hidden by the surface\n");
    } else {
	fprintf(stderr,"\tsurface is transparent for borders, tics and labels\n");
    }
}

#endif

/* process 'show pointsize' command */
static void
show_pointsize()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tpointsize is %g\n", pointsize);
}


/* process 'show encoding' command */
static void
show_encoding()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tencoding is %s\n", encoding_names[encoding]);
}


/* process 'show polar' command */
static void
show_polar()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tpolar is %s\n", (polar) ? "ON" : "OFF");
}


/* process 'show angles' command */
static void
show_angles()
{
    c_token++;
    SHOW_ALL_NL;

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


/* process 'show samples' command */
static void
show_samples()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tsampling rate is %d, %d\n", samples_1, samples_2);
}


/* process 'show isosamples' command */
static void
show_isosamples()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tiso sampling rate is %d, %d\n",
	    iso_samples_1, iso_samples_2);
}


/* process 'show view' command */
static void
show_view()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tview is %g rot_x, %g rot_z, %g scale, %g scale_z\n",
		surface_rot_x, surface_rot_z, surface_scale, surface_zscale);
}


/* process 'show surface' command */
static void
show_surface()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tsurface is %sdrawn\n", draw_surface ? "" : "not ");
}


/* process 'show hidden3d' command */
static void
show_hidden3d()
{
    c_token++;
    SHOW_ALL_NL;

#ifdef LITE
    printf(" Hidden Line Removal Not Supported in LITE version\n");
#else
    fprintf(stderr, "\thidden surface is %s\n",
	    hidden3d ? "removed" : "drawn");
    show_hidden3doptions();
#endif /* LITE */
}


#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
/* process 'show historysize' command */
static void
show_historysize()
{
    c_token++;
    if (gnuplot_history_size >= 0) {
	fprintf(stderr, "\thistory size: %ld\n", gnuplot_history_size);
    } else {
	fprintf(stderr, "\thistory will not be truncated.\n");
    }
}
#endif


/* process 'show size' command */
static void
show_size()
{
    c_token++;
    SHOW_ALL_NL;

    fprintf(stderr, "\tsize is scaled by %g,%g\n", xsize, ysize);
    if (aspect_ratio > 0)
	fprintf(stderr, "\tTry to set aspect ratio to %g:1.0\n", aspect_ratio);
    else if (aspect_ratio == 0)
	fputs("\tNo attempt to control aspect ratio\n", stderr);
    else
	fprintf(stderr, "\tTry to set LOCKED aspect ratio to %g:1.0\n",
		-aspect_ratio);
}


/* process 'show origin' command */
static void
show_origin()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\torigin is set to %g,%g\n", xoffset, yoffset);
}


/* process 'show term' command */
static void
show_term()
{
    c_token++;
    SHOW_ALL_NL;

    if (term)
	fprintf(stderr, "\tterminal type is %s %s\n",
		term->name, term_options);
    else
	fputs("\tterminal type is unknown\n", stderr);
}


/* process 'show tics|[xyzx2y2]tics' commands */
static void
show_tics(showx, showy, showz, showx2, showy2)
TBOOLEAN showx, showy, showz, showx2, showy2;
{
    c_token++;
    SHOW_ALL_NL;

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


/* process 'show m[xyzx2y2]tics' commands */
static void
show_mtics(minitic, minifreq, name)
int minitic;
double minifreq;
const char *name;
{
    c_token++;

    switch (minitic) {
    case MINI_OFF:
	fprintf(stderr, "\tminor %stics are off\n", name);
	break;
    case MINI_DEFAULT:
	fprintf(stderr, "\
\tminor %stics are off for linear scales\n\
\tminor %stics are computed automatically for log scales\n", name, name);
	break;
    case MINI_AUTO:
	fprintf(stderr, "\tminor %stics are computed automatically\n", name);
	break;
    case MINI_USER:
	fprintf(stderr, "\
\tminor %stics are drawn with %d subintervals between major xtic marks\n",
		name, (int) minifreq);
	break;
    default:
	int_error(NO_CARET, "Unknown minitic type in show_mtics()");
    }
}


/* process 'show timestamp' command */
static void
show_timestamp()
{
    c_token++;
    SHOW_ALL_NL;
    show_xyzlabel("time", &timelabel);
    fprintf(stderr, "\twritten in %s corner\n",
	    (timelabel_bottom ? "bottom" : "top"));
    if (timelabel_rotate)
	fputs("\trotated if the terminal allows it\n\t", stderr);
    else
	fputs("\tnot rotated\n\t", stderr);
}


/* process 'show [xyzx2y2rtuv]range' commands */
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
    c_token++;
    SHOW_ALL_NL;

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


/* called by the functions below */
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


/* process 'show title' command */
static void
show_title()
{
    c_token++;
    SHOW_ALL_NL;
    show_xyzlabel("title", &title);
}


/* process 'show xlabel' command */
static void
show_xlabel()
{
    c_token++;
    SHOW_ALL_NL;
    show_xyzlabel("xlabel", &xlabel);
}


/* process 'show ylabel' command */
static void
show_ylabel()
{
    c_token++;
    SHOW_ALL_NL;
    show_xyzlabel("ylabel", &ylabel);
}


/* process 'show zlabel' command */
static void
show_zlabel()
{
    c_token++;
    SHOW_ALL_NL;
    show_xyzlabel("zlabel", &zlabel);
}


/* process 'show x2label' command */
static void
show_x2label()
{
    c_token++;
    SHOW_ALL_NL;
    show_xyzlabel("x2label", &x2label);
}


/* process 'show y2label' command */
static void
show_y2label()
{
    c_token++;
    SHOW_ALL_NL;
    show_xyzlabel("y2label", &y2label);
}

/* process 'show [xyzx2y2]data' commands */
static void
show_datatype(name, axis)
const char *name;
int axis;
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\t%s is set to %s\n", name,
	    datatype[axis] == TIME ? "time" : "numerical");
}


/* process 'show xdata' command */
static void
show_xdata()
{
    c_token++;
    SHOW_ALL_NL;
    show_datatype("xdata", FIRST_X_AXIS);
}


/* process 'show ydata' command */
static void
show_ydata()
{
    c_token++;
    SHOW_ALL_NL;
    show_datatype("ydata", FIRST_X_AXIS);
}


/* process 'show zdata' command */
static void
show_zdata()
{
    c_token++;
    SHOW_ALL_NL;
    show_datatype("zdata", FIRST_X_AXIS);
}


/* process 'show x2data' command */
static void
show_x2data()
{
    c_token++;
    SHOW_ALL_NL;
    show_datatype("x2data", FIRST_X_AXIS);
}


/* process 'show y2data' command */
static void
show_y2data()
{
    c_token++;
    SHOW_ALL_NL;
    show_datatype("y2data", FIRST_X_AXIS);
}

/* process 'show timefmt' command */
static void
show_timefmt()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tread format for time is \"%s\"\n", conv_text(timefmt));
}


/* process 'show locale' command */
static void
show_locale()
{
    c_token++;
    SHOW_ALL_NL;
    locale_handler(ACTION_SHOW,NULL);
}


/* process 'show loadpath' command */
static void
show_loadpath()
{
    c_token++;
    SHOW_ALL_NL;
    loadpath_handler(ACTION_SHOW,NULL);
}


/* process 'show zero' command */
static void
show_zero()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tzero is %g\n", zero);
}


/* process 'show missing' command */
static void
show_missing()
{
    c_token++;
    SHOW_ALL_NL;

    if (missing_val == NULL)
	fputs("\tNo string is interpreted as missing data\n", stderr);
    else
	fprintf(stderr, "\t\"%s\" is interpreted as missing value\n",
		missing_val);
}

#ifdef USE_MOUSE
/* process 'show mouse' command */
static void
show_mouse()
{
    c_token++;
    SHOW_ALL_NL;
    if (mouse_setting.on) {
	fprintf(stderr, "\tmouse is on\n");
	if (mouse_setting.annotate_zoom_box) {
	    fprintf(stderr, "\tzoom coordinates will be drawn\n");
	} else {
	    fprintf(stderr, "\tno zoom coordinates will be drawn\n");
	}
	if (mouse_setting.polardistance) {
	    fprintf(stderr, "\tdistance to ruler will be show in polar coordinates\n");
	} else {
	    fprintf(stderr, "\tno polar distance to ruler will be shown\n");
	}
	if (mouse_setting.doubleclick > 0) {
	    fprintf(stderr, "\tdouble click resolution is %d ms\n",
		mouse_setting.doubleclick);
	} else {
	    fprintf(stderr, "\tdouble click resolution is off\n");
	}
	fprintf(stderr, "\tformatting numbers with \"%s\"\n",
	    mouse_setting.fmt);
	fprintf(stderr, "\tformat for Button 1 is %d\n", (int) clipboard_mode);
	if (clipboard_alt_string) {
	    fprintf(stderr, "\talternative format for Button 1 is '%s'\n",
		clipboard_alt_string);
	}
	fprintf(stderr, "\tformat for Button 2 is %d\n", (int) mouse_mode);
	if (mouse_alt_string) {
	    fprintf(stderr, "\talternative format for Button 2 is '%s'\n",
		mouse_alt_string);
	}
	if (mouse_setting.label) {
	    fprintf(stderr, "\tButton 2 draws labes with options \"%s\"\n",
		mouse_setting.labelopts);
	} else {
	    fprintf(stderr, "\tdrawing temporary annotation on Button 2\n");
	}
	fprintf(stderr, "\tzoomjump is %s\n",
	    mouse_setting.warp_pointer ? "on" : "off");
	fprintf(stderr, "\tcommunication commands will %sbe shown\n",
	    mouse_setting.verbose ? "" : "not ");
    } else {
	fprintf(stderr, "\tmouse is off\n");
    }
}
#endif

/* process 'show plot' command */
static void
show_plot()
{
    c_token++;
    SHOW_ALL_NL;
    fprintf(stderr, "\tlast plot command was: %s\n", replot_line);
}


/* process 'show variables' command */
static void
show_variables()
{
    register struct udvt_entry *udv = first_udv;
    int len;

    c_token++;
    SHOW_ALL_NL;

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
	    fprintf(stderr, "\tlinestyle %d, ", this_linestyle->tag);

#ifdef PM3D
	    if (this_linestyle->lp_properties.use_palette)
		fprintf(stderr, "linetype palette, ");
	    else
#endif
		fprintf(stderr, "linetype %d, ", this_linestyle->lp_properties.l_type + 1);

	    fprintf(stderr, "linewidth %.3f, pointtype %d, pointsize %.3f\n",
		    this_linestyle->lp_properties.l_width,
		    this_linestyle->lp_properties.p_type + 1,
		    this_linestyle->lp_properties.p_size);
	}
    }
    if (tag > 0 && !showed)
	int_error(c_token, "linestyle not found");
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
