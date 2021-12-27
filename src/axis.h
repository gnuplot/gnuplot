/*[
 * Copyright 2000, 2004   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_AXIS_H
#define GNUPLOT_AXIS_H

/* Aug 2017 - unconditional support for nonlinear axes */
# define nonlinear(axis) ((axis)->linked_to_primary != NULL && (axis)->link_udf->at != NULL)
# define invalid_coordinate(x,y) ((unsigned)(x)==intNaN || (unsigned)(y)==intNaN)

#include <stddef.h>		/* for offsetof() */
#include "gp_types.h"		/* for TBOOLEAN */

#include "gadgets.h"
#include "parse.h"		/* for const_*() */
#include "tables.h"		/* for the axis name parse table */
#include "term_api.h"		/* for lp_style_type */
#include "util.h"		/* for int_error() */

/* typedefs / #defines */

/* give some names to some array elements used in command.c and grap*.c
 * maybe one day the relevant items in setshow will also be stored
 * in arrays.
 *
 * Always keep the following conditions alive:
 * SECOND_X_AXIS = FIRST_X_AXIS + SECOND_AXES
 * FIRST_X_AXIS & SECOND_AXES == 0
 */

typedef enum AXIS_INDEX {
    NO_AXIS = -2,
    ALL_AXES = -1,
    FIRST_Z_AXIS = 0,
#define FIRST_AXES FIRST_Z_AXIS
    FIRST_Y_AXIS,
    FIRST_X_AXIS,
    COLOR_AXIS,			/* fill gap */
    SECOND_Z_AXIS,		/* not used, yet */
#define SECOND_AXES SECOND_Z_AXIS
    SAMPLE_AXIS=SECOND_Z_AXIS,
    SECOND_Y_AXIS,
    SECOND_X_AXIS,
    POLAR_AXIS,
#define NUMBER_OF_MAIN_VISIBLE_AXES (POLAR_AXIS + 1)
    T_AXIS,
    U_AXIS,
    V_AXIS,		/* Last index into axis_array[] */
    PARALLEL_AXES,	/* Parallel axis structures are allocated dynamically */
    THETA_index = 1234,	/* Used to identify THETA_AXIS */

    AXIS_ARRAY_SIZE = PARALLEL_AXES
} AXIS_INDEX;


/* sample axis doesn't need mtics, so use the slot to hold sample interval */
# define SAMPLE_INTERVAL mtic_freq

/* What kind of ticmarking is wanted? */
typedef enum en_ticseries_type {
    TIC_COMPUTED=1, 		/* default; gnuplot figures them */
    TIC_SERIES,			/* user-defined series */
    TIC_USER,			/* user-defined points */
    TIC_MONTH,   		/* print out month names ((mo-1)%12)+1 */
    TIC_DAY      		/* print out day of week */
} t_ticseries_type;

/* Defines one ticmark for TIC_USER style.
 * If label==NULL, the value is printed with the usual format string.
 * else, it is used as the format string (note that it may be a constant
 * string, like "high" or "low").
 */
typedef struct ticmark {
    double position;		/* where on axis is this */
    char *label;		/* optional (format) string label */
    int level;			/* 0=major tic, 1=minor tic */
    struct ticmark *next;	/* linked list */
} ticmark;

/* Tic-mark labelling definition; see set xtics */
typedef struct ticdef {
    t_ticseries_type type;
    char *font;
    struct t_colorspec textcolor;
    struct {
	   struct ticmark *user;	/* for TIC_USER */
	   struct {			/* for TIC_SERIES */
		  double start, incr;
		  double end;		/* ymax, if VERYLARGE */
	   } series;
	   TBOOLEAN mix;		/* TRUE to use both the above */
    } def;
    struct position offset;
    TBOOLEAN rangelimited;		/* Limit tics to data range */
    TBOOLEAN enhanced;			/* Use enhanced text mode or labels */
    TBOOLEAN logscaling;		/* place tics using old logscale algorithm */
} t_ticdef;

/* we want two auto modes for minitics - default where minitics are
 * auto for log/time and off for linear, and auto where auto for all
 * graphs I've done them in this order so that logscale-mode can
 * simply test bit 0 to see if it must do the minitics automatically.
 * similarly, conventional plot can test bit 1 to see if minitics are
 * required */
typedef enum en_minitics_status {
    MINI_OFF,
    MINI_DEFAULT,
    MINI_USER,
    MINI_AUTO
} t_minitics_status;

/* Values to put in the axis_tics[] variables that decides where the
 * ticmarks should be drawn: not at all, on one or both plot borders,
 * or the zeroaxes. These look like a series of values, but TICS_MASK
 * shows that they're actually bit masks --> don't turn into an enum
 * */
#define NO_TICS        0
#define TICS_ON_BORDER 1
#define TICS_ON_AXIS   2
#define TICS_MASK      3
#define TICS_MIRROR    4

/* Tic levels 0 and 1 are maintained in the axis structure.
 * Tic levels 2 - MAX_TICLEVEL have only one property - scale.
 */
#define MAX_TICLEVEL 5
extern double ticscale[MAX_TICLEVEL];

/* HBB 20010610: new type for storing autoscale activity. Effectively
 * two booleans (bits) in a single variable, so I'm using an enum with
 * all 4 possible bit masks given readable names. */
typedef enum e_autoscale {
    AUTOSCALE_NONE = 0,
    AUTOSCALE_MIN = 1<<0,
    AUTOSCALE_MAX = 1<<1,
    AUTOSCALE_BOTH = (1<<0 | 1 << 1),
    AUTOSCALE_FIXMIN = 1<<2,
    AUTOSCALE_FIXMAX = 1<<3
} t_autoscale;

typedef enum e_constraint {
    CONSTRAINT_NONE  = 0,
    CONSTRAINT_LOWER = 1<<0,
    CONSTRAINT_UPPER = 1<<1,
    CONSTRAINT_BOTH  = (1<<0 | 1<<1)
} t_constraint;

/* The unit the tics of a given time/date axis are to interpreted in */
/* HBB 20040318: start at one, to avoid undershoot */
typedef enum e_timelevel {
    TIMELEVEL_SECONDS = 1, TIMELEVEL_MINUTES, TIMELEVEL_HOURS,
    TIMELEVEL_DAYS, TIMELEVEL_WEEKS, TIMELEVEL_MONTHS,
    TIMELEVEL_YEARS
} t_timelevel;

typedef struct axis {
/* range of this axis */
    t_autoscale autoscale;	/* Which end(s) are autoscaled? */
    t_autoscale set_autoscale;	/* what does 'set' think autoscale to be? */
    int range_flags;		/* flag bits about autoscale/writeback: */
    /* write auto-ed ranges back to variables for autoscale */
#define RANGE_WRITEBACK   1
#define RANGE_SAMPLED     2
#define RANGE_IS_REVERSED 4
    double min;			/* 'transient' axis extremal values */
    double max;
    double set_min;		/* set/show 'permanent' values */
    double set_max;
    double writeback_min;	/* ULIG's writeback implementation */
    double writeback_max;
    double data_min;		/* Not necessarily the same as axis min */
    double data_max;

/* range constraints */
    t_constraint min_constraint;
    t_constraint max_constraint;
    double min_lb, min_ub;     /* min lower- and upper-bound */
    double max_lb, max_ub;     /* min lower- and upper-bound */
    
/* output-related quantities */
    int term_lower;		/* low and high end of the axis on output, */
    int term_upper;		/* ... (in terminal coordinates)*/
    double term_scale;		/* scale factor: plot --> term coords */
    unsigned int term_zero;	/* position of zero axis */

/* log axis control */
    TBOOLEAN log;		/* log axis stuff: flag "islog?" */
    double base;		/* logarithm base value */
    double log_base;		/* ln(base), for easier computations */

/* linked axis information (used only by x2, y2)
 * If axes are linked, the primary axis info will be cloned into the
 * secondary axis only up to this point in the structure.
 */
    struct axis *linked_to_primary;	/* Set only in a secondary axis */
    struct axis *linked_to_secondary;	/* Set only in a primary axis */
    struct udft_entry *link_udf;

/* ticmark control variables */
    int ticmode;		/* tics on border/axis? mirrored? */
    struct ticdef ticdef;	/* tic series definition */
    int tic_rotate;		/* ticmarks rotated by this angle */
    enum JUSTIFY tic_pos;	/* left/center/right tic label justification */
    TBOOLEAN gridmajor;		/* Grid lines wanted on major tics? */
    TBOOLEAN gridminor;		/* Grid lines for minor tics? */
    t_minitics_status minitics;	/* minor tic mode (none/auto/user)? */
    double mtic_freq;		/* minitic stepsize */
    double ticscale;		/* scale factor for tic marks (was (0..1])*/
    double miniticscale;	/* and for minitics */
    double ticstep;		/* increment used to generate tic placement */
    TBOOLEAN tic_in;		/* tics to be drawn inward?  */

/* time/date axis control */
    td_type datatype;		/* {DT_NORMAL|DT_TIMEDATE} controls _input_ */
    td_type tictype;		/* {DT_NORMAL|DT_TIMEDATE|DT_DMS} controls _output_ */
    char *formatstring;		/* the format string for output */
    char *ticfmt;		/* autogenerated alternative to formatstring (needed??) */
    t_timelevel timelevel;	/* minimum time unit used to quantize ticks */

/* other miscellaneous fields */
    int index;			/* if this is a permanent axis, this indexes axis_array[] */
				/* (index >= PARALLEL_AXES) indexes parallel axes */
				/* (index < 0) indicates a dynamically allocated structure */
    text_label label;		/* label string and position offsets */
    TBOOLEAN manual_justify;	/* override automatic justification */
    lp_style_type *zeroaxis;	/* usually points to default_axis_zeroaxis */
    double paxis_x;		/* x coordinate of parallel axis */
} AXIS;

#define DEFAULT_AXIS_TICDEF {TIC_COMPUTED, NULL, {TC_DEFAULT, 0, 0.0}, {NULL, {0.,0.,0.}, FALSE},  { character, character, character, 0., 0., 0. }, FALSE, TRUE, FALSE }
#define DEFAULT_AXIS_ZEROAXIS {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}

#define DEFAULT_AXIS_STRUCT {						    \
	AUTOSCALE_BOTH, AUTOSCALE_BOTH, /* auto, set_auto */		    \
	0, 			/* range_flags for autoscaling */	    \
	-10.0, 10.0,		/* 3 pairs of min/max for axis itself */    \
	-10.0, 10.0,							    \
	-10.0, 10.0,							    \
	  0.0,  0.0,		/* and another min/max for the data */	    \
	CONSTRAINT_NONE, CONSTRAINT_NONE,  /* min and max constraints */    \
	0., 0., 0., 0.,         /* lower and upper bound for min and max */ \
	0, 0,   		/* terminal lower and upper coords */	    \
	0.,        		/* terminal scale */			    \
	0,        		/* zero axis position */		    \
	FALSE, 10.0, 0.0,	/* log, base, log(base) */		    \
	NULL, NULL,		/* linked_to_primary, linked_to_secondary */\
	NULL,      		/* link function */                         \
	NO_TICS,		/* tic output positions (border, mirror) */ \
	DEFAULT_AXIS_TICDEF,	/* tic series definition */		    \
	0, CENTRE,	 	/* tic_rotate, horizontal justification */  \
	FALSE, FALSE,	 	/* grid{major,minor} */			    \
	MINI_DEFAULT, 10.,	/* minitics, mtic_freq */		    \
	1.0, 0.5, 0.0, TRUE,	/* ticscale, miniticscale, ticstep, tic_in */ \
	DT_NORMAL, DT_NORMAL,	/* datatype for input, output */	    \
	NULL, NULL,      	/* output format, another output format */  \
	0,			/* timelevel */                             \
	0,			/* index (e.g.FIRST_Y_AXIS) */		    \
	EMPTY_LABELSTRUCT,	/* axis label */			    \
	FALSE,			/* override automatic justification */	    \
	NULL			/* NULL means &default_axis_zeroaxis */	    \
}

/* This much of the axis structure is cloned by the "set x2range link" command */
#define AXIS_CLONE_SIZE offsetof(AXIS, linked_to_primary)

/* Table of default behaviours --- a subset of the struct above. Only
 * those fields are present that differ from axis to axis. */
typedef struct axis_defaults {
    double min;			/* default axis endpoints */
    double max;
    char name[4];		/* axis name, like in "x2" or "t" */
    int ticmode;		/* tics on border/axis? mirrored? */
} AXIS_DEFAULTS;

/* global variables in axis.c */

extern AXIS axis_array[AXIS_ARRAY_SIZE];
extern AXIS *shadow_axis_array;
extern const AXIS_DEFAULTS axis_defaults[AXIS_ARRAY_SIZE];
extern const AXIS default_axis_state;

/* Dynamic allocation of parallel axis structures */
extern AXIS *parallel_axis_array;
extern int num_parallel_axes;

/* A parsing table for mapping axis names into axis indices. For use
 * by the set/show machinery, mainly */
extern const struct gen_table axisname_tbl[];


extern const struct ticdef default_axis_ticdef;

/* default format for tic mark labels */
#define DEF_FORMAT "% h"
#define DEF_FORMAT_LATEX "$%h$"

/* default parse timedata string */
#define TIMEFMT "%d/%m/%y,%H:%M"
extern char *timefmt;

/* axis labels */
extern const text_label default_axis_label;

/* zeroaxis linetype (flag type==-3 if none wanted) */
extern const lp_style_type default_axis_zeroaxis;

/* default grid linetype, to be used by 'unset grid' and 'reset' */
extern const struct lp_style_type default_grid_lp;

/* grid layer: LAYER_BEHIND LAYER_BACK LAYER_FRONT */
extern int grid_layer;

/* Whether to draw the axis tic labels and tic marks in front of everything else */
extern TBOOLEAN grid_tics_in_front;

/* Whether to draw vertical grid lines in 3D */
extern TBOOLEAN grid_vertical_lines;

/* Whether to draw a grid in spiderplots */
extern TBOOLEAN grid_spiderweb;

/* Whether or not to draw a separate polar axis in polar mode */
extern TBOOLEAN raxis;

/* global variables for communication with the tic callback functions */
/* FIXME HBB 20010806: had better be collected into a struct that's
 * passed to the callback */
extern int tic_start, tic_direction, tic_mirror;
/* These are for passing on to write_multiline(): */
extern int tic_text, rotate_tics, tic_hjust, tic_vjust;
/* The remaining ones are for grid drawing; controlled by 'set grid': */
/* extern int grid_selection; --- comm'ed out, HBB 20010806 */
extern struct lp_style_type grid_lp; /* linestyle for major grid lines */
extern struct lp_style_type mgrid_lp; /* linestyle for minor grid lines */

extern double polar_grid_angle; /* angle step in polar grid in radians */
extern double theta_origin;	/* 0 = right side of plot */
extern double theta_direction;	/* 1 = counterclockwise -1 = clockwise */

/* Length of the longest tics label, set by widest_tic_callback(): */
extern int widest_tic_strlen;

/* flag to indicate that in-line axis ranges should be ignored */
extern TBOOLEAN inside_zoom;

/* axes being used by the current plot */
extern AXIS_INDEX x_axis, y_axis, z_axis;

extern struct axis THETA_AXIS;

/* macros to reduce code clutter caused by the array notation, mainly
 * in graphics.c and fit.c */
#define X_AXIS axis_array[x_axis]
#define Y_AXIS axis_array[y_axis]
#define Z_AXIS axis_array[z_axis]
#define R_AXIS axis_array[POLAR_AXIS]
#define CB_AXIS axis_array[COLOR_AXIS]

/* -------- macros using these variables: */

/* Macros to map from user to terminal coordinates and back */
#define AXIS_MAP(axis, variable)        axis_map(        &axis_array[axis], variable)
#define AXIS_MAP_DOUBLE(axis, variable) axis_map_double( &axis_array[axis], variable)
#define AXIS_MAPBACK(axis, pos)         axis_mapback(    &axis_array[axis], pos)

/* Same thing except that "axis" is a pointer, not an index */
#define axis_map_double(axis, variable)         \
    ((axis)->term_lower + ((variable) - (axis)->min) * (axis)->term_scale)
#define axis_map_toint(x) (int)( (x) + 0.5 )
#define axis_map(axis, variable) axis_map_toint( axis_map_double(axis, variable) )

#define axis_mapback(axis, pos) \
    (((double)(pos) - (axis)->term_lower)/(axis)->term_scale + (axis)->min)

/* parse a position of the form
 *    [coords] x, [coords] y {,[coords] z}
 * where coords is one of first,second.graph,screen,character
 * if first or second, we need to take axis.datatype into account
 * FIXME: Cannot handle parallel axes
 */
#define GET_NUMBER_OR_TIME(store,axes,axis)				\
do {									\
    AXIS *this_axis = (axes == NO_AXIS) ? NULL				\
			    : &(axis_array[(axes)+(axis)]);		\
    (store) = get_num_or_time(this_axis);				\
} while(0)


/*
 * Gradually replacing extremely complex macro ACTUAL_STORE_AND_UPDATE_RANGE
 * (called 50+ times) with a subroutine. The original logic was that in-line
 * code was faster than calls to a subroutine, but on current hardware it is
 * better to have one cached copy than to have 50 separate uncached copies.
 *
 * The difference between STORE_AND_UPDATE_RANGE and store_and_update_range
 * is that the former takes an axis index and the latter an axis pointer.
 */
#define STORE_AND_UPDATE_RANGE(STORE, VALUE, TYPE, AXIS, NOAUTOSCALE, UNDEF_ACTION)	 \
 if (AXIS != NO_AXIS) do { \
    if (store_and_update_range( &(STORE), VALUE, &(TYPE), (&axis_array[AXIS]), NOAUTOSCALE) \
        == UNDEFINED) { \
	UNDEF_ACTION; \
    } \
} while(0)

/* Use NOOP for UNDEF_ACTION if no action is wanted */
#define NOOP ((void)0)



/* HBB 20000506: new macro to automatically build initializer lists
 * for arrays of AXIS_ARRAY_SIZE=11 equal elements */
#define AXIS_ARRAY_INITIALIZER(value) {			\
    value, value, value, value, value,			\
	value, value, value, value, value, value }

/* 'roundoff' check tolerance: less than one hundredth of a tic mark */
#define SIGNIF (0.01)
/* (DFK) Watch for cancellation error near zero on axes labels */
/* FIXME HBB 20000521: these seem not to be used much, anywhere... */
#define CheckZero(x,tic) (fabs(x) < ((tic) * SIGNIF) ? 0.0 : (x))

/* Function pointer type for callback functions to generate ticmarks */
typedef void (*tic_callback) (struct axis *, double, char *, int, 
			struct lp_style_type, struct ticmark *);

/* ------------ functions exported by axis.c */
coord_type store_and_update_range(double *store, double curval, coord_type *type,
				struct axis *axis, TBOOLEAN noautoscale);
void autoscale_one_point( struct axis *axis, double x );
t_autoscale load_range(struct axis *, double *, double *, t_autoscale);
void check_log_limits(struct axis *, double, double);
void axis_invert_if_requested(struct axis *);
void axis_check_range(AXIS_INDEX);
void axis_init(AXIS *this_axis, TBOOLEAN infinite);
void set_explicit_range(struct axis *axis, double newmin, double newmax);
double axis_log_value_checked(AXIS_INDEX, double, const char *);
void axis_checked_extend_empty_range(AXIS_INDEX, const char *mesg);
void axis_check_empty_nonlinear(struct axis *this_axis);
char * copy_or_invent_formatstring(struct axis *);
double quantize_normal_tics(double, int);
void setup_tics(struct axis *, int);
void gen_tics(struct axis *, tic_callback);
void axis_output_tics(AXIS_INDEX, int *, AXIS_INDEX, tic_callback);
void axis_set_scale_and_range(struct axis *axis, int lower, int upper);
void axis_draw_2d_zeroaxis(AXIS_INDEX, AXIS_INDEX);
TBOOLEAN some_grid_selected(void);
void add_tic_user(struct axis *, char *, double, int);
double get_num_or_time(struct axis *);
TBOOLEAN bad_axis_range(struct axis *axis);

void save_writeback_all_axes(void);
int  parse_range(AXIS_INDEX axis);
void parse_skip_range(void);
void check_axis_reversed(AXIS_INDEX axis);

/* set widest_tic_label: length of the longest tics label */
void widest_tic_callback(struct axis *, double place, char *text, int ticlevel,
			struct lp_style_type grid, struct ticmark *);

void get_position(struct position *pos);
void get_position_default(struct position *pos, enum position_type default_type, int ndim);

void gstrdms(char *label, char *format, double value);

void clone_linked_axes(AXIS *axis1, AXIS *axis2);
AXIS *get_shadow_axis(AXIS *axis);
void extend_primary_ticrange(AXIS *axis);
void update_primary_axis_range(struct axis *secondary);
void update_secondary_axis_range(struct axis *primary);
void reconcile_linked_axes(AXIS *primary, AXIS *secondary);
void extend_autoscaled_log_axis(AXIS *primary);

int map_x(double value);
int map_y(double value);

double map_x_double(double value);
double map_y_double(double value);

coord_type polar_to_xy( double theta, double r, double *x, double *y, TBOOLEAN update);
double polar_radius(double r);

void set_cbminmax(void);

void save_autoscaled_ranges(AXIS *, AXIS *);
void restore_autoscaled_ranges(AXIS *, AXIS *);

char * axis_name(AXIS_INDEX);
void init_sample_range(AXIS *axis, enum PLOT_TYPE plot_type);
void init_parallel_axis(AXIS *, AXIS_INDEX);
void extend_parallel_axis(int);

/* Evaluate the function linking a secondary axis to its primary axis */
double eval_link_function(AXIS *, double);

/* For debugging */
void dump_axis_range(AXIS *axis);

/* macro for tic scale, used in all tic_callback functions */
#define tic_scale(ticlevel, axis) \
    (ticlevel <= 0 ? axis->ticscale : \
     ticlevel == 1 ? axis->miniticscale : \
     ticlevel < MAX_TICLEVEL ? ticscale[ticlevel] : \
     0)

/* convenience macro to make sure min < max */
#define reorder_if_necessary( min, max ) \
do { if (max < min) { double temp = min; min = max; max = temp; } \
} while (0)

#endif /* GNUPLOT_AXIS_H */
