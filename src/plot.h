/*
 * $Id: plot.h,v 1.29.2.8 2000/10/23 18:57:54 joze Exp $
 */

/* GNUPLOT - plot.h */

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

#ifndef GNUPLOT_PLOT_H
# define GNUPLOT_PLOT_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "syscfg.h"
#include "stdfn.h"

#ifdef USE_MOUSE
# include "mouse.h"
#endif


#define PROGRAM "G N U P L O T"
#define PROMPT "gnuplot> "

/* OS dependent constants are now defined in syscfg.h */

#define SAMPLES 100		/* default number of samples for a plot */
#define ISO_SAMPLES 10		/* default number of isolines per splot */
#define ZERO	1e-8		/* default for 'zero' set option */

#ifndef TERM
/* default terminal is "unknown"; but see init_terminal */
# define TERM "unknown"
#endif

/* avoid precompiled header conflict with redefinition */
#ifdef NEXT
# include <mach/boolean.h>
#else
/* Sheer, raging paranoia */
# ifdef TRUE
#  undef TRUE
# endif
# ifdef FALSE
#  undef FALSE
# endif
# define TRUE 1
# define FALSE 0
#endif

#ifndef __cplusplus
#undef bool
typedef unsigned int bool;
#endif

/* TRUE or FALSE */
#define TBOOLEAN bool

/* double true, used in autoscale: 1=autoscale ?min, 2=autoscale ?max */
#define DTRUE 3

/* Normally in <math.h> */
#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
# define M_PI_2 1.57079632679489661923
#endif

#define DEG2RAD (M_PI / 180.0)


/* minimum size of points[] in curve_points */
#define MIN_CRV_POINTS 100
/* minimum size of points[] in surface_points */
#define MIN_SRF_POINTS 1000

/* _POSIX_PATH_MAX is too small for practical purposes */
#ifndef PATH_MAX
# ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
# endif
# ifndef MAXPATHLEN
#  define PATH_MAX 1024
# else
#  define PATH_MAX MAXPATHLEN
# endif
#endif

/* Minimum number of chars to represent an integer */
#define INT_STR_LEN (3*sizeof(int))

/* Concatenate a path name and a file name. The file name
 * may or may not end with a "directory separation" character.
 * Path must not be NULL, but can be empty
 */
#define PATH_CONCAT(path,file) \
 { char *p = path; \
   p += strlen(path); \
   if (p!=path) p--; \
   if (*p && (*p != DIRSEP1) && (*p != DIRSEP2)) { \
     if (*p) p++; *p++ = DIRSEP1; *p = NUL; \
   } \
   strcat (path, file); \
 }

/* Macros for string concatenation */
#ifdef HAVE_STRINGIZE
/* ANSI version */
# define CONCAT(x,y) x##y
# define CONCAT3(x,y,z) x##y##z
#else
/* K&R version */
# define CONCAT(x,y) x/**/y
# define CONCAT3(x,y,z) x/**/y/**/z
#endif

/* note that MAX_LINE_LEN, MAX_TOKENS and MAX_AT_LEN do no longer limit the
   size of the input. The values describe the steps in which the sizes are
   extended. */

#define MAX_LINE_LEN 1024	/* maximum number of chars allowed on line */
#define MAX_TOKENS 400
#define MAX_ID_LEN 50		/* max length of an identifier */


#define MAX_AT_LEN 150		/* max number of entries in action table */
#define NO_CARET (-1)

#ifdef DOS16
# define MAX_NUM_VAR	3	/* Plotting projection of func. of max. five vars. */
#else /* not DOS16 */
# define MAX_NUM_VAR	5	/* Plotting projection of func. of max. five vars. */
#endif /* not DOS16 */

#define MAP3D_CARTESIAN		0	/* 3D Data mapping. */
#define MAP3D_SPHERICAL		1
#define MAP3D_CYLINDRICAL	2

#define CONTOUR_NONE	0	/* Where to place contour maps if at all. */
#define CONTOUR_BASE	1
#define CONTOUR_SRF	2
#define CONTOUR_BOTH	3

#define CONTOUR_KIND_LINEAR	0
#define CONTOUR_KIND_CUBIC_SPL	1
#define CONTOUR_KIND_BSPLINE	2

#define LEVELS_AUTO		0		/* How contour levels are set */
#define LEVELS_INCREMENTAL	1		/* user specified start & incremnet */
#define LEVELS_DISCRETE		2		/* user specified discrete levels */

#define ANGLES_RADIANS	0
#define ANGLES_DEGREES	1

#define NO_TICS        0
#define TICS_ON_BORDER 1
#define TICS_ON_AXIS   2
#define TICS_MASK      3
#define TICS_MIRROR    4

/* give some names to some array elements used in command.c and grap*.c
 * maybe one day the relevant items in setshow will also be stored
 * in arrays
 */

/* SECOND_X_AXIS = FIRST_X_AXIS + SECOND_AXES
 * FIRST_X_AXIS & SECOND_AXES == 0
 */
#define FIRST_AXES 0
#define SECOND_AXES 4

#define FIRST_Z_AXIS 0
#define FIRST_Y_AXIS 1
#define FIRST_X_AXIS 2
#define SECOND_Z_AXIS 4 /* for future expansion ;-) */
#define SECOND_Y_AXIS 5
#define SECOND_X_AXIS 6
/* extend list for datatype[] for t,u,v,r though IMHO
 * they are not relevant to time data [being parametric dummies]
 */
#define T_AXIS 3  /* fill gap */
#define R_AXIS 7  /* never used ? */
#define U_AXIS 8
#define V_AXIS 9

#define AXIS_ARRAY_SIZE 10
#define DATATYPE_ARRAY_SIZE 10

/* values for range_flags[] */
#define RANGE_WRITEBACK 1    /* write auto-ed ranges back to variables for autoscale */
#define RANGE_REVERSE   2    /* allow auto and reversed ranges */

/* we want two auto modes for minitics - default where minitics
 * are auto for log/time and off for linear, and auto where
 * auto for all graphs
 * I've done them in this order so that logscale-mode can simply
 * test bit 0 to see if it must do the minitics automatically.
 * similarly, conventional plot can test bit 1 to see if minitics are
 * required
 */
#define MINI_OFF      0
#define MINI_DEFAULT  1
#define MINI_AUTO     3
#define MINI_USER     2


/* Need to allow user to choose grid at first and/or second axes tics.
 * Also want to let user choose circles at x or y tics for polar grid.
 * Also want to allow user rectangular grid for polar plot or polar
 * grid for parametric plot. So just go for full configurability.
 * These are bitmasks
 */
#define GRID_OFF    0
#define GRID_X      (1<<0)
#define GRID_Y      (1<<1)
#define GRID_Z      (1<<2)
#define GRID_X2     (1<<3)
#define GRID_Y2     (1<<4)
#define GRID_MX     (1<<5)
#define GRID_MY     (1<<6)
#define GRID_MZ     (1<<7)
#define GRID_MX2    (1<<8)
#define GRID_MY2    (1<<9)

/* To access curves larger than 64k, MSDOS needs to use huge pointers */
#if (defined(__TURBOC__) && defined(MSDOS)) || defined(WIN16)
# define GPHUGE huge
# define GPFAR far
#else /* not TurboC || WIN16 */
# define GPHUGE /* nothing */
# define GPFAR /* nothing */
#endif /* not TurboC || WIN16 */

#if defined(DOS16) || defined(WIN16)
typedef float coordval;		/* memory is tight on PCs! */
# define COORDVAL_FLOAT 1
#else
typedef double coordval;
#endif

/*
 * Note about VERYLARGE:  This is the upper bound double (or float, if PC)
 * numbers. This flag indicates very large numbers. It doesn't have to 
 * be the absolutely biggest number on the machine.  
 * If your machine doesn't have HUGE, or float.h,
 * define VERYLARGE here. 
 *
 * example:
#define VERYLARGE 1e37
 *
 * To get an appropriate value for VERYLARGE, we can use DBL_MAX (or
 * FLT_MAX on PCs), HUGE or HUGE_VAL. DBL_MAX is usually defined in
 * float.h and is the largest possible double value. HUGE and HUGE_VAL
 * are either DBL_MAX or +Inf (IEEE special number), depending on the
 * compiler. +Inf may cause problems with some buggy fp
 * implementations, so we better avoid that. The following should work
 * better than the previous setup (which used HUGE in preference to
 * DBL_MAX).
 */

/* both min/max and MIN/MAX are defined by some compilers.
 * we are now on GPMIN / GPMAX
 */

#define GPMAX(a,b) ( (a) > (b) ? (a) : (b) )
#define GPMIN(a,b) ( (a) < (b) ? (a) : (b) )

/* Moved from binary.h, command.c, graph3d.c, graphics.c, interpol.c,
 * plot2d.c, plot3d.c, util3d.c ...
 */
#ifndef inrange
# define inrange(z,min,max) \
   (((min)<(max)) ? (((z)>=(min)) && ((z)<=(max))) : \
	            (((z)>=(max)) && ((z)<=(min))))
#endif

/* There is a bug in the NEXT OS. This is a workaround. Lookout for
 * an OS correction to cancel the following dinosaur
 *
 * Hm, at least with my setup (compiler version 3.1, system 3.3p1),
 * DBL_MAX is defined correctly and HUGE and HUGE_VAL are both defined
 * as 1e999. I have no idea to which OS version the bugfix below
 * applies, at least wrt. HUGE, it is inconsistent with the current
 * version. Since we are using DBL_MAX anyway, most of this isn't
 * really needed anymore.
 */

#if defined ( NEXT ) && NX_CURRENT_COMPILER_RELEASE<310
# if defined ( DBL_MAX)
#  undef DBL_MAX
# endif
# define DBL_MAX 1.7976931348623157e+308
# undef HUGE
# define HUGE    DBL_MAX
# undef HUGE_VAL
# define HUGE_VAL DBL_MAX
#endif /* NEXT && NX_CURRENT_COMPILER_RELEASE<310 */

/* Now define VERYLARGE. This is usually DBL_MAX/2 - 1. On MS-DOS however
 * we use floats for memory considerations and thus use FLT_MAX.
 */

#ifndef COORDVAL_FLOAT
# ifdef DBL_MAX
#  define VERYLARGE (DBL_MAX/2-1)
# endif
#else /* COORDVAL_FLOAT */
# ifdef FLT_MAX
#  define VERYLARGE (FLT_MAX/2-1)
# endif
#endif /* COORDVAL_FLOAT */

#ifndef VERYLARGE
# ifdef HUGE
#  define VERYLARGE (HUGE/2-1)
# elif defined(HUGE_VAL)
#  define VERYLARGE (HUGE_VAL/2-1)
# else
/* as a last resort */
#  define VERYLARGE (1e37)
/* #  warning "using last resort 1e37 as VERYLARGE define, please check your headers" */
/* Maybe add a note somewhere in the install docs instead */
# endif /* HUGE */
#endif /* VERYLARGE */

/* Some older platforms, namely SunOS 4.x, don't define this. */
#ifndef DBL_EPSILON
# define DBL_EPSILON     2.2204460492503131E-16
#endif

/* The XOPEN ln(10) macro */
#ifndef M_LN10
#  define M_LN10    2.3025850929940456840e0 
#endif

/* argument: char *fn */
#define VALID_FILENAME(fn) ((fn) != NULL && (*fn) != '\0')

#define END_OF_COMMAND (c_token >= num_tokens || equals(c_token,";"))
#define is_EOF(c) ((c) == 'e' || (c) == 'E')

#ifdef VMS
# define is_comment(c) ((c) == '#' || (c) == '!')
# define is_system(c) ((c) == '$')
/* maybe configure could check this? */
# define BACKUP_FILESYSTEM 1
#else /* not VMS */
# define is_comment(c) ((c) == '#')
# define is_system(c) ((c) == '!')
#endif /* not VMS */

#ifndef RETSIGTYPE
/* assume ANSI definition by default */
# define RETSIGTYPE void
#endif

#ifndef SIGFUNC_NO_INT_ARG
typedef RETSIGTYPE (*sigfunc)__PROTO((int));
#else
typedef RETSIGTYPE (*sigfunc)__PROTO((void));
#endif

#ifndef SORTFUNC_ARGS
typedef int (*sortfunc) __PROTO((const generic *, const generic *));
#else
typedef int (*sortfunc) __PROTO((SORTFUNC_ARGS, SORTFUNC_ARGS));
#endif

#define is_jump(operator) ((operator) >=(int)JUMP && (operator) <(int)SF_START)


enum DATA_TYPES {
	INTGR, CMPLX
};

#define TIME 3

enum PLOT_TYPE {
	FUNC, DATA, FUNC3D, DATA3D
};

/* we explicitly assign values to the types, such that we can
 * perform bit tests to see if the style involves points and/or lines
 * bit 0 (val 1) = line, bit 1 (val 2) = point, bit 2 (val 4)= error
 * This allows rapid decisions about the sample drawn into the key,
 * for example.
 */
enum PLOT_STYLE {
	LINES        =  0*8 + 1,
	POINTSTYLE   =  1*8 + 2,
	IMPULSES     =  2*8 + 1,
	LINESPOINTS  =  3*8 + 3,
	DOTS         =  4*8 + 0,
	XERRORBARS   =  5*8 + 6,
	YERRORBARS   =  6*8 + 6,
	XYERRORBARS  =  7*8 + 6,
	BOXXYERROR   =  8*8 + 1,
	BOXES        =  9*8 + 1,
	BOXERROR     = 10*8 + 1,
	STEPS        = 11*8 + 1,
	FSTEPS       = 12*8 + 1,
	HISTEPS      = 13*8 + 1,
	VECTOR       = 14*8 + 1,
	CANDLESTICKS = 15*8 + 4,
	FINANCEBARS  = 16*8 + 1,
	XERRORLINES  = 17*8 + 7,
	YERRORLINES  = 18*8 + 7,
	XYERRORLINES = 19*8 + 7
};

enum PLOT_SMOOTH { 
    SMOOTH_NONE,
    SMOOTH_ACSPLINES, SMOOTH_BEZIER, SMOOTH_CSPLINES, SMOOTH_SBEZIER,
    SMOOTH_UNIQUE
};

/* this order means we can use  x-(just*strlen(text)*t->h_char)/2 if
 * term cannot justify
 */
enum JUSTIFY {
	LEFT, CENTRE, RIGHT
};

/* we use a similar trick for vertical justification of multi-line labels */
#define JUST_TOP    0
#define JUST_CENTRE 1
#define JUST_BOT    2

#if !(defined(ATARI)&&defined(__GNUC__)&&defined(_MATH_H)) &&  !(defined(MTOS)&&defined(__GNUC__)&&defined(_MATH_H)) /* FF's math.h has the type already */
struct cmplx {
	double real, imag;
};
#endif


struct value {
	enum DATA_TYPES type;
	union {
		int int_val;
		struct cmplx cmplx_val;
	} v;
};


struct lexical_unit {	/* produced by scanner */
	TBOOLEAN is_token;	/* true if token, false if a value */ 
	struct value l_val;
	int start_index;	/* index of first char in token */
	int length;			/* length of token in chars */
};


struct udft_entry {				/* user-defined function table entry */
	struct udft_entry *next_udf; 		/* pointer to next udf in linked list */
	char *udf_name;				/* name of this function entry */
	struct at_type *at;			/* pointer to action table to execute */
	char *definition; 			/* definition of function as typed */
	struct value dummy_values[MAX_NUM_VAR];	/* current value of dummy variables */
};


struct udvt_entry {			/* user-defined value table entry */
	struct udvt_entry *next_udv; /* pointer to next value in linked list */
	char *udv_name;			/* name of this value entry */
	TBOOLEAN udv_undef;		/* true if not defined yet */
	struct value udv_value;	/* value it has */
};


union argument {			/* p-code argument */
	int j_arg;				/* offset for jump */
	struct value v_arg;		/* constant value */
	struct udvt_entry *udv_arg;	/* pointer to dummy variable */
	struct udft_entry *udf_arg; /* pointer to udf to execute */
};


/* This type definition has to come after union argument has been declared. */
#ifdef __ZTC__
typedef int (*FUNC_PTR)(...);
#else
typedef int (*FUNC_PTR) __PROTO((union argument *arg));
#endif

struct ft_entry {		/* standard/internal function table entry */
	const char *f_name;	/* pointer to name of this function */
	FUNC_PTR func;		/* address of function to call */
};


/* Defines the type of a coordinate */
/* INRANGE and OUTRANGE points have an x,y point associated with them */
enum coord_type {
    INRANGE,				/* inside plot boundary */
    OUTRANGE,				/* outside plot boundary, but defined */
    UNDEFINED				/* not defined at all */
};


struct coordinate {
	enum coord_type type;	/* see above */
	coordval x, y, z;
	coordval ylow, yhigh;	/* ignored in 3d */
	coordval xlow, xhigh;   /* also ignored in 3d */
#if defined(WIN16) || (defined(MSDOS) && defined(__TURBOC__))
	char pad[2];		/* pad to 32 byte boundary */
#endif
};

struct lp_style_type {          /* contains all Line and Point properties */
	int     pointflag;      /* 0 if points not used, otherwise 1 */
	int     l_type,
	        p_type;
	double  l_width,
	        p_size;
	                        /* more to come ? */
#ifdef PM3D
	TBOOLEAN use_palette;
#endif
};

/* Now unused; replaced with set.c(reset_lp_properties) */
/* default values for the structure 'lp_style_type' */
/* #define LP_DEFAULT {0,0,0,1.0,1.0} */


struct curve_points {
	struct curve_points *next;	/* pointer to next plot in linked list */
	int token;    /* last token in re/plot line, for second pass */
	enum PLOT_TYPE plot_type;
	enum PLOT_STYLE plot_style;
	enum PLOT_SMOOTH plot_smooth;
	char *title;
	int title_no_enhanced;		/* don't typeset title in enhanced mode */
	struct lp_style_type lp_properties;
 	int p_max;			/* how many points are allocated */
	int p_count;			/* count of points in points */
	int x_axis;			/* FIRST_X_AXIS or SECOND_X_AXIS */
	int y_axis;			/* FIRST_Y_AXIS or SECOND_Y_AXIS */
	struct coordinate GPHUGE *points;
};

struct gnuplot_contours {
	struct gnuplot_contours *next;
	struct coordinate GPHUGE *coords;
 	char isNewLevel;
 	char label[32];
	int num_pts;
#ifdef PM3D
	double z;
#endif
};

struct iso_curve {
	struct iso_curve *next;
 	int p_max;			/* how many points are allocated */
	int p_count;			/* count of points in points */
	struct coordinate GPHUGE *points;
};

struct surface_points {
	struct surface_points *next_sp;	/* pointer to next plot in linked list */
	int token;  /* last token in re/plot line, for second pass */
	enum PLOT_TYPE plot_type;
	enum PLOT_STYLE plot_style;
	char *title;
	int title_no_enhanced;		/* don't typeset title in enhanced mode */
	struct lp_style_type lp_properties;
	int has_grid_topology;
	int num_iso_read;  /* Data files only - num of isolines read from file. */
	/* for functions, num_iso_read is the number of 'primary' isolines (in x dirn) */
	struct gnuplot_contours *contours;	/* Not NULL If have contours. */
	struct iso_curve *iso_crvs;
};

/* we might specify a co-ordinate in first/second/graph/screen coords */
/* allow x,y and z to be in separate co-ordinates ! */
enum position_type { first_axes, second_axes, graph, screen };

struct position {
	enum position_type scalex,scaley,scalez;
	double x,y,z;
};

#ifdef PM3D
/* color.h is included here, since it requires the definition of `coordinate' */
#include "color.h"
#endif

/* It should go without saying that additional entries may be made
 * only at the end of this structure. Any fields added must be
 * optional - a value of 0 (or NULL pointer) means an older driver
 * does not support that feature - gnuplot must still be able to
 * function without that terminal feature
 */

/* values for the optional flags field - choose sensible defaults
 * these aren't really very sensible names - multiplot attributes
 * depend on whether stdout is redirected or not. Remember that
 * the default is 0. Thus most drivers can do multiplot only if
 * the output is redirected
 */
 
#define TERM_CAN_MULTIPLOT    1  /* tested if stdout not redirected */
#define TERM_CANNOT_MULTIPLOT 2  /* tested if stdout is redirected  */
#define TERM_BINARY           4  /* open output file with "b"       */

struct TERMENTRY {
    const char *name;
#ifdef WIN16
    const char GPFAR description[80];  /* to make text go in FAR segment */
#else
    const char *description;
#endif
    unsigned int xmax,ymax,v_char,h_char,v_tic,h_tic;

    void (*options) __PROTO((void));
    void (*init) __PROTO((void));
    void (*reset) __PROTO((void));
    void (*text) __PROTO((void));
    int (*scale) __PROTO((double, double));
    void (*graphics) __PROTO((void));
    void (*move) __PROTO((unsigned int, unsigned int));
    void (*vector) __PROTO((unsigned int, unsigned int));
    void (*linetype) __PROTO((int));
    void (*put_text) __PROTO((unsigned int, unsigned int, const char*));
    /* the following are optional. set term ensures they are not NULL */
    int (*text_angle) __PROTO((int));
    int (*justify_text) __PROTO((enum JUSTIFY));
    void (*point) __PROTO((unsigned int, unsigned int,int));
    void (*arrow) __PROTO((unsigned int, unsigned int, unsigned int, unsigned int, TBOOLEAN));
    int (*set_font) __PROTO((const char *font));
    void (*pointsize) __PROTO((double)); /* change pointsize */
    int flags;
    void (*suspend) __PROTO((void)); /* called after one plot of multiplot */
    void (*resume)  __PROTO((void)); /* called before plots of multiplot */
    void (*fillbox) __PROTO((int, unsigned int, unsigned int, unsigned int, unsigned int)); /* clear in multiplot mode */
    void (*linewidth) __PROTO((double linewidth));
#ifdef USE_MOUSE
    int (*waitforinput) __PROTO((void));     /* used for mouse input */
    void (*put_tmptext) __PROTO((int, const char []));   /* draws temporary text; int determines where: 0=statusline, 1,2: at corners of zoom box, with \r separating text above and below the point */
    void (*set_ruler) __PROTO((int, int));    /* set ruler location; x<0 switches ruler off */
    void (*set_cursor) __PROTO((int, int, int));   /* set cursor style and corner of rubber band */
    void (*set_clipboard) __PROTO((const char[]));  /* write text into cut&paste buffer (clipboard) */
#endif
#ifdef PM3D
    int (*make_palette) __PROTO((t_sm_palette *palette));
    /* 1. if palette==NULL, then return nice/suitable
       maximal number of colours supported by this terminal.
       Returns 0 if it can make colours without palette (like 
       postscript).
       2. if palette!=NULL, then allocate its own palette
       return value is undefined
       3. available: some negative values of max_colors for whatever 
       can be useful
     */
    void (*previous_palette) __PROTO((void));	
    /* release the palette that the above routine allocated and get 
       back the palette that was active before.
       Some terminals, like displays, may draw parts of the figure
       using their own palette. Those terminals that possess only 
       one palette for the whole plot don't need this routine.
     */

    void (*set_color) __PROTO((double gray));
    /* gray is from [0;1], terminal uses its palette or another way
       to transform in into gray or r,g,b
       This routine (for each terminal separately) remembers or not
       this colour so that it can apply it for the subsequent drawings
     */
    void (*filled_polygon) __PROTO((int points, gpiPoint *corners));
#endif
};

#ifdef WIN16
# define termentry TERMENTRY far
#else
# define termentry TERMENTRY
#endif


struct text_label {
	struct text_label *next;	/* pointer to next label in linked list */
	int tag;			/* identifies the label */
	struct position place;
	enum JUSTIFY pos;
	int rotate;
	int layer;
	char *text;
	char *font;
	int pointstyle; /* joze */
	double hoffset;
	double voffset;
}; /* Entry font added by DJL */

struct arrow_def {
	struct arrow_def *next;	/* pointer to next arrow in linked list */
	int tag;			/* identifies the arrow */
	struct position start;
	struct position end;
	TBOOLEAN head;			/* arrow has a head or not */
	struct position headsize;	/* x = length, y = angle [deg] */
	int layer;			/* 0 = back, 1 = front */
	TBOOLEAN relative;		/* second coordinate is relative to first */
	struct lp_style_type lp_properties;
};

struct linestyle_def {
	struct linestyle_def *next;	/* pointer to next linestyle in linked list */
	int tag;			/* identifies the linestyle */
	struct lp_style_type lp_properties;
};

/* Tic-mark labelling definition; see set xtics */
struct ticdef {
    int type;				/* one of five values below */
#define TIC_COMPUTED 1			/* default; gnuplot figures them */
#define TIC_SERIES   2			/* user-defined series */
#define TIC_USER     3			/* user-defined points */
#define TIC_MONTH    4		/* print out month names ((mo-1[B)%12)+1 */
#define TIC_DAY      5			/* print out day of week */
    union {
	   struct ticmark *user;	/* for TIC_USER */
	   struct {			/* for TIC_SERIES */
		  double start, incr;
		  double end;		/* ymax, if VERYLARGE */
	   } series;
    } def;
};

/* Defines one ticmark for TIC_USER style.
 * If label==NULL, the value is printed with the usual format string.
 * else, it is used as the format string (note that it may be a constant
 * string, like "high" or "low").
 */
struct ticmark {
    double position;		/* where on axis is this */
    char *label;			/* optional (format) string label */
    struct ticmark *next;	/* linked list */
};

/* Some key global variables */
/* command.c */
extern TBOOLEAN is_3d_plot;
extern int plot_token;

/* plot.c */
extern TBOOLEAN interactive;

extern struct termentry *term;/* from term.c */
extern TBOOLEAN undefined;		/* from internal.c */
extern char *input_line;		/* from command.c */
extern char *replot_line;
extern struct lexical_unit *token;
extern int token_table_size;
extern int inline_num;
extern const char *user_shell;
#if defined(ATARI) || defined(MTOS)
extern const char *user_gnuplotpath;
#endif

/* flags to disable `replot` when some data are sent through stdin; used by
   mouse/hotkey capable terminals */
extern int plotted_data_from_stdin;
extern int replot_disabled;

#ifdef GNUPLOT_HISTORY
extern long int gnuplot_history_size;
#endif

/* version.c */
extern const char gnuplot_version[];
extern const char gnuplot_patchlevel[];
extern const char gnuplot_date[];
extern const char gnuplot_copyright[];
extern const char faq_location[];
extern const char bug_email[];
extern const char help_email[];

extern char os_name[];
extern char os_rel[];

/* Windows needs to redefine stdin/stdout functions */
#if defined(_Windows) && !defined(WINDOWS_NO_GUI)
# include "win/wtext.h"
#endif

#define TTOP 0
#define TBOTTOM 1
#define TUNDER 2
#define TLEFT 0
#define TRIGHT 1
#define TOUT 2
#define JLEFT 0
#define JRIGHT 1
  
/* defines used for timeseries, seconds */
#define ZERO_YEAR	2000
#define JAN_FIRST_WDAY 6  /* 1st jan, 2000 is a Saturday (cal 1 2000 on unix) */
#define SEC_OFFS_SYS	946684800.0		/*  zero gnuplot (2000) - zero system (1970) */
#define YEAR_SEC	31557600.0	/* avg, incl. leap year */
#define MON_SEC		2629800.0	/* YEAR_SEC / 12 */
#define WEEK_SEC	604800.0
#define DAY_SEC		86400.0

/* returns from DF_READLINE in datafile.c */
/* +ve is number of columns read */
#define DF_EOF          (-1)
#define DF_UNDEFINED    (-2)
#define DF_FIRST_BLANK  (-3)
#define DF_SECOND_BLANK (-4)

/* HBB: changed to __inline__, which seems more usually
 * understood by tools like, e.g., lclint */
/* if GP_INLINE has not yet been defined, set to __inline__ for gcc,
 * nothing. I'd prefer that any other compilers have the defn in
 * the makefile, rather than having a huge list of compilers here.
 * But gcc is sufficiently ubiquitous that I'll allow it here !!!
 */
#ifndef GP_INLINE
# ifdef __GNUC__
#  define GP_INLINE __inline__
# else
#  define GP_INLINE /*nothing*/
# endif
#endif

/* line/point parsing...
 *
 * allow_ls controls whether we are allowed to accept linestyle in
 * the current context [ie not when doing a  set linestyle command]
 * allow_point is whether we accept a point command
 * We assume compiler will optimise away if(0) or if(1)
 */
#if defined(__FILE__) && defined(__LINE__) && defined(DEBUG_LP)
# define LP_DUMP(lp) \
 fprintf(stderr, \
  "lp_properties at %s:%d : lt: %d, lw: %.3f, pt: %d, ps: %.3f\n", \
  __FILE__, __LINE__, lp->l_type, lp->l_width, lp->p_type, lp->p_size)
#else
# define LP_DUMP(lp)
#endif

/* #if... / #include / #define collection: */

/* Type definitions */

/* Variables of plot.c needed by other modules: */

/* Prototypes of functions exported by plot.c */

void bail_to_command_line __PROTO((void));
void interrupt_setup __PROTO((void));
void gp_expand_tilde __PROTO((char **));

#ifdef LINUXVGA
void drop_privilege __PROTO((void));
void take_privilege __PROTO((void));
#endif /* LINUXVGA */

extern int ignore_enhanced_text;

#endif /* GNUPLOT_PLOT_H */
