/*
 * $Id: gp_types.h,v 1.4 2000/05/01 23:23:29 hbb Exp $
 */

/* GNUPLOT - gp_types.h */

/*[
 * Copyright 2000   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_GPTYPES_H
#define GNUPLOT_GPTYPES_H

#include "syscfg.h"

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

#define DEG2RAD (M_PI / 180.0)

/* These used to be #defines in plot.h. For easier debugging, I've
 * converted most of them into enum's. */
enum en_data_mapping {
    MAP3D_CARTESIAN,
    MAP3D_SPHERICAL,
    MAP3D_CYLINDRICAL,
};

/* FIXME: move these to contour.h! */
enum en_contour_placement {
    /* Where to place contour maps if at all. */
    CONTOUR_NONE,
    CONTOUR_BASE,
    CONTOUR_SRF,
    CONTOUR_BOTH,
};

enum en_contour_kind {
    /* Method of drawing the contour lines found */
    CONTOUR_KIND_LINEAR,
    CONTOUR_KIND_CUBIC_SPL,
    CONTOUR_KIND_BSPLINE,
};

enum en_contour_levels {
    /* How contour levels are set */
    LEVELS_AUTO,		/* automatically selected */
    LEVELS_INCREMENTAL,		/* user specified start & incremnet */
    LEVELS_DISCRETE,		/* user specified discrete levels */
};

enum en_angle_units {
    ANGLES_RADIANS,
    ANGLES_DEGREES,
};

/* These look like a series of values, but TICS_MASK shows that
 * they're actually bit masks --> don't turn into an enum */
#define NO_TICS        0
#define TICS_ON_BORDER 1
#define TICS_ON_AXIS   2
#define TICS_MASK      3
#define TICS_MIRROR    4

/* bit masks for range_flags[]: */
/* write auto-ed ranges back to variables for autoscale */
#define RANGE_WRITEBACK 1	
/* allow auto and reversed ranges */
#define RANGE_REVERSE   2	

/* we want two auto modes for minitics - default where minitics
 * are auto for log/time and off for linear, and auto where
 * auto for all graphs
 * I've done them in this order so that logscale-mode can simply
 * test bit 0 to see if it must do the minitics automatically.
 * similarly, conventional plot can test bit 1 to see if minitics are
 * required
 */
enum en_minitics_status {
    MINI_OFF,
    MINI_DEFAULT,
    MINI_USER,
    MINI_AUTO,
};

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
enum DATA_TYPES {
	INTGR, CMPLX
};

enum PLOT_TYPE {
	FUNC, DATA, FUNC3D, DATA3D
};

/* we explicitly assign values to the types, such that we can
 * perform bit tests to see if the style involves points and/or lines
 * bit 0 (val 1) = line, bit 1 (val 2) = point, bit 2 (val 4)= error
 * This allows rapid decisions about the sample drawn into the key,
 * for example.
 */
typedef enum PLOT_STYLE {
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
} PLOT_STYLE;

typedef enum PLOT_SMOOTH { 
    SMOOTH_NONE,
    SMOOTH_ACSPLINES,
    SMOOTH_BEZIER,
    SMOOTH_CSPLINES,
    SMOOTH_SBEZIER,
    SMOOTH_UNIQUE
} PLOT_SMOOTH;

/* this order means we can use  x-(just*strlen(text)*t->h_char)/2 if
 * term cannot justify
 */
typedef enum JUSTIFY {
    LEFT,
    CENTRE,
    RIGHT
} JUSTIFY;

/* we use a similar trick for vertical justification of multi-line labels */
typedef enum VERT_JUSTIFY {
    JUST_TOP,
    JUST_CENTRE,
    JUST_BOT,
} VERT_JUSTIFY;

#if !(defined(ATARI)&&defined(__GNUC__)&&defined(_MATH_H)) &&  !(defined(MTOS)&&defined(__GNUC__)&&defined(_MATH_H)) /* FF's math.h has the type already */
struct cmplx {
	double real, imag;
};
#endif

typedef struct value {
	enum DATA_TYPES type;
	union {
		int int_val;
		struct cmplx cmplx_val;
	} v;
} t_value;

typedef struct lexical_unit {	/* produced by scanner */
	TBOOLEAN is_token;	/* true if token, false if a value */ 
	struct value l_val;
	int start_index;	/* index of first char in token */
	int length;			/* length of token in chars */
} lexical_unit;


typedef struct udft_entry {	/* user-defined function table entry */
  struct udft_entry *next_udf;	/* pointer to next udf in linked list */
  char *udf_name;		/* name of this function entry */
  struct at_type *at;		/* pointer to action table to execute */
  char *definition;		/* definition of function as typed */
  struct value dummy_values[MAX_NUM_VAR]; /* current value of dummy variables */
} udft_entry;


typedef struct udvt_entry {	/* user-defined value table entry */
	struct udvt_entry *next_udv; /* pointer to next value in linked list */
	char *udv_name;			/* name of this value entry */
	TBOOLEAN udv_undef;		/* true if not defined yet */
	struct value udv_value;	/* value it has */
} udvt_entry;


typedef union argument {	/* p-code argument */
	int j_arg;		/* offset for jump */
	struct value v_arg;	/* constant value */
	struct udvt_entry *udv_arg; /* pointer to dummy variable */
	struct udft_entry *udf_arg; /* pointer to udf to execute */
} argument;


/* This type definition has to come after union argument has been declared. */
#ifdef __ZTC__
typedef int (*FUNC_PTR)(...);
#else
typedef int (*FUNC_PTR) __PROTO((union argument *arg));
#endif

typedef struct ft_entry {	/* standard/internal function table entry */
	const char *f_name;	/* pointer to name of this function */
	FUNC_PTR func;		/* address of function to call */
} ft_entry;


/* Defines the type of a coordinate */
/* INRANGE and OUTRANGE points have an x,y point associated with them */
typedef enum coord_type {
    INRANGE,			/* inside plot boundary */
    OUTRANGE,			/* outside plot boundary, but defined */
    UNDEFINED			/* not defined at all */
} coord_type;


typedef struct coordinate {
    enum coord_type type;	/* see above */
    coordval x, y, z;
    coordval ylow, yhigh;	/* ignored in 3d */
    coordval xlow, xhigh;	/* also ignored in 3d */
#if defined(WIN16) || (defined(MSDOS) && defined(__TURBOC__))
    char pad[2];		/* pad to 32 byte boundary */
#endif
} coordinate;

typedef struct lp_style_type {          /* contains all Line and Point properties */
    int     pointflag;		/* 0 if points not used, otherwise 1 */
    int     l_type;
    int	    p_type;
    double  l_width;
    double  p_size;
    /* ... more to come ? */
} lp_style_type;

typedef struct curve_points {
    struct curve_points *next;	/* pointer to next plot in linked list */
    int token;			/* last token nr., for second pass */
    enum PLOT_TYPE plot_type;
    enum PLOT_STYLE plot_style;
    enum PLOT_SMOOTH plot_smooth;
    char *title;
    struct lp_style_type lp_properties;
    int p_max;			/* how many points are allocated */
    int p_count;		/* count of points in points */
    int x_axis;			/* FIRST_X_AXIS or SECOND_X_AXIS */
    int y_axis;			/* FIRST_Y_AXIS or SECOND_Y_AXIS */
    struct coordinate GPHUGE *points;
} curve_points;

typedef struct gnuplot_contours {
    struct gnuplot_contours *next;
    struct coordinate GPHUGE *coords;
    char isNewLevel;
    char label[32];
    int num_pts;
} gnuplot_contours;

typedef struct iso_curve {
    struct iso_curve *next;
    int p_max;			/* how many points are allocated */
    int p_count;			/* count of points in points */
    struct coordinate GPHUGE *points;
} iso_curve;

typedef struct surface_points {
    struct surface_points *next_sp; /* linked list */
    int token;			/* last token nr, for second pass */
    enum PLOT_TYPE plot_type;
    enum PLOT_STYLE plot_style;
    char *title;
    struct lp_style_type lp_properties;
    int has_grid_topology;
    
    /* Data files only - num of isolines read from file. For
     * functions, num_iso_read is the number of 'primary' isolines (in
     * x direction) */
    int num_iso_read;		
    struct gnuplot_contours *contours; /* NULL if not doing contours. */
    struct iso_curve *iso_crvs;	/* the actual data */
} surface_points;

/* values for the optional flags field - choose sensible defaults
 * these aren't really very sensible names - multiplot attributes
 * depend on whether stdout is redirected or not. Remember that
 * the default is 0. Thus most drivers can do multiplot only if
 * the output is redirected
 */
#define TERM_CAN_MULTIPLOT    1  /* tested if stdout not redirected */
#define TERM_CANNOT_MULTIPLOT 2  /* tested if stdout is redirected  */
#define TERM_BINARY           4  /* open output file with "b"       */

/* The terminal interface structure --- heart of the terminal layer.
 *
 * It should go without saying that additional entries may be made
 * only at the end of this structure. Any fields added must be
 * optional - a value of 0 (or NULL pointer) means an older driver
 * does not support that feature - gnuplot must still be able to
 * function without that terminal feature
 */

typedef struct TERMENTRY {
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
} TERMENTRY;

#ifdef WIN16
# define termentry TERMENTRY far
#else
# define termentry TERMENTRY
#endif


/* we might specify a co-ordinate in first/second/graph/screen coords */
/* allow x,y and z to be in separate co-ordinates ! */
typedef enum position_type {
    first_axes,
    second_axes,
    graph,
    screen
} position_type;

typedef struct position {
	enum position_type scalex,scaley,scalez;
	double x,y,z;
} position;

typedef struct text_label {
	struct text_label *next; /* pointer to next label in linked list */
	int tag;		/* identifies the label */
	struct position place;
	enum JUSTIFY pos;
	int rotate;
	int layer;
	char *text;
	char *font;		/* Entry font added by DJL */
	int pointstyle;		/* joze */
	double hoffset;
	double voffset;
} text_label;

typedef struct arrow_def {
	struct arrow_def *next;	/* pointer to next arrow in linked list */
	int tag;			/* identifies the arrow */
	struct position start;
	struct position end;
	TBOOLEAN head;			/* arrow has a head or not */
	int layer;			/* 0 = back, 1 = front */
	struct lp_style_type lp_properties;
} arrow_def;

struct linestyle_def {
	struct linestyle_def *next;	/* pointer to next linestyle in linked list */
	int tag;			/* identifies the linestyle */
	struct lp_style_type lp_properties;
};

/* Tic-mark labelling definition; see set xtics */
typedef struct ticdef {
    int type;			/* one of five values below */
#define TIC_COMPUTED 1		/* default; gnuplot figures them */
#define TIC_SERIES   2		/* user-defined series */
#define TIC_USER     3		/* user-defined points */
#define TIC_MONTH    4		/* print out month names ((mo-1[B)%12)+1 */
#define TIC_DAY      5		/* print out day of week */
    union {
	   struct ticmark *user;	/* for TIC_USER */
	   struct {			/* for TIC_SERIES */
		  double start, incr;
		  double end;		/* ymax, if VERYLARGE */
	   } series;
    } def;
} t_ticdef;

/* Defines one ticmark for TIC_USER style.
 * If label==NULL, the value is printed with the usual format string.
 * else, it is used as the format string (note that it may be a constant
 * string, like "high" or "low").
 */
typedef struct ticmark {
    double position;		/* where on axis is this */
    char *label;			/* optional (format) string label */
    struct ticmark *next;	/* linked list */
} ticmark;

typedef enum en_key_vertical_position {
    TTOP,
    TBOTTOM,
    TUNDER,
} t_key_vertical_position;

typedef enum en_key_horizontal_position {
    TLEFT,
    TRIGHT,
    TOUT,
} t_key_horizontal_position;

typedef enum en_key_sample_positioning {
    JLEFT,
    JRIGHT,
} t_key_sample_positioning;
  
/* returns from DF_READLINE in datafile.c */
/* +ve is number of columns read */
#define DF_EOF          (-1)
#define DF_UNDEFINED    (-2)
#define DF_FIRST_BLANK  (-3)
#define DF_SECOND_BLANK (-4)

#endif /* GNUPLOT_GPTYPES_H */
