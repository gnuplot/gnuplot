/* 
 * $Id: axis.h,v 1.5 2000/05/01 23:23:29 hbb Exp $
 *
 */

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

#ifndef GNUPLOT_AXIS_H
#define GNUPLOT_AXIS_H

#include "gp_types.h"		/* for TBOOLEAN */
#include "parse.h"		/* for const_express() */
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
#define FIRST_AXES 0
    FIRST_Z_AXIS, 
    FIRST_Y_AXIS,
    FIRST_X_AXIS,
    T_AXIS,			/* fill gap */
#define SECOND_AXES 4
    SECOND_Z_AXIS,		/* not used, yet */
    SECOND_Y_AXIS,
    SECOND_X_AXIS,
    R_AXIS,			/* never used ? */
    U_AXIS,			/* dito */
    V_AXIS,			/* dito */
#define AXIS_ARRAY_SIZE 10
} AXIS_INDEX;

typedef void (*tic_callback) __PROTO((AXIS_INDEX, double, char *, struct lp_style_type ));

/* global variables in axis.c */

extern double min_array[AXIS_ARRAY_SIZE];
extern double max_array[AXIS_ARRAY_SIZE];
extern int auto_array[AXIS_ARRAY_SIZE];
extern TBOOLEAN log_array[AXIS_ARRAY_SIZE];
extern double base_array[AXIS_ARRAY_SIZE];
extern double log_base_array[AXIS_ARRAY_SIZE];

/* scale factors for mapping for each axis */
extern double scale[AXIS_ARRAY_SIZE];

/* low and high end of the axis on output, in terminal coords: */
extern unsigned int axis_graphical_lower[AXIS_ARRAY_SIZE];
extern unsigned int axis_graphical_upper[AXIS_ARRAY_SIZE];
/* axis' zero positions in terminal coords */
extern unsigned int axis_zero[AXIS_ARRAY_SIZE];	

/* if user specifies [10:-10] we use [-10:10] internally, and swap at end */
extern int reverse_range[AXIS_ARRAY_SIZE];

/* does the format string look a numeric one, for sprintf()? */
extern int format_is_numeric[AXIS_ARRAY_SIZE];

/* is this axis in 'set {x|...}data time' mode? */
extern int axis_is_timedata[AXIS_ARRAY_SIZE];

/* 'set' options 'writeback' and 'reverse' */
extern int range_flags[AXIS_ARRAY_SIZE];

extern int tic_start, tic_direction, tic_text,
    rotate_tics, tic_hjust, tic_vjust, tic_mirror;

extern int xleft, xright, ybot, ytop;



/* -------- macros using these variables: */

/* Macros to map from user to terminal coordinates and back */
#define AXIS_MAP(axis, variable)				\
  (int) ((axis_graphical_lower[axis])				\
	 + ((variable) - min_array[axis])*scale[axis] + 0.5)
#define AXIS_MAPBACK(axis, pos)					\
  (((double)(pos)-axis_graphical_lower[axis])/scale[axis]	\
   + min_array[axis])

/* these are the old names for these: */
#define map_x(x) AXIS_MAP(x_axis, x)
#define map_y(y) AXIS_MAP(y_axis, y)

#define AXIS_SETSCALE(axis, out_low, out_high) \
    scale[axis] = ((out_high) - (out_low)) / \
                  (max_array[axis] - min_array[axis])

/* write current min/max_array contents into the set/show status
 * variables */
#define AXIS_WRITEBACK(axis,min,max)				\
    if (range_flags[axis]&RANGE_WRITEBACK) {		\
	if (auto_array[axis]&1) min = min_array[axis];	\
	if (auto_array[axis]&2) max = max_array[axis];	\
    }

/* HBB 20000430: New macros, logarithmize a value into a stored
 * coordinate*/ 
#define AXIS_DO_LOG(axis,value) (log(value) / log_base_array[axis])
#define AXIS_UNDO_LOG(axis,value) exp((value) * log_base_array[axis])

/* HBB 20000430: same, but these test if the axis is log, first: */
#define AXIS_LOG_VALUE(axis,value) \
    (log_array[axis] ? AXIS_DO_LOG(axis,value) : (value))
#define AXIS_DE_LOG_VALUE(axis,coordinate) \
    (log_array[axis] ? AXIS_UNDO_LOG(axis,coordinate): (coordinate))
 

/* copy scalar data to arrays. The difference between 3D and 2D
 * versions is: dont know we have to support ranges [10:-10] - lets
 * reverse it for now, then fix it at the end.  */
/* FIXME HBB 20000426: unknown if this distinction makes any sense... */
#define AXIS_INIT3D(axis, min, max, auto, is_log, base, infinite)	\
do {									\
    if ((auto_array[axis] = auto) == 0 && max<min) {			\
	min_array[axis] = max;						\
	max_array[axis] = min; /* we will fix later */			\
    } else {								\
	min_array[axis] = (infinite && (auto&1)) ? VERYLARGE : min;	\
	max_array[axis] = (infinite && (auto&2)) ? -VERYLARGE : max;	\
    }									\
    log_array[axis] = is_log;						\
    base_array[axis] = base;						\
    log_base_array[axis] = log(base);					\
} while(0)

#define AXIS_INIT2D(axis, min, max, auto, is_log, base, infinite)	\
do {									\
    auto_array[axis] = auto;						\
    min_array[axis] = (infinite && (auto&1)) ? VERYLARGE : min;		\
    max_array[axis] = (infinite && (auto&2)) ? -VERYLARGE : max;	\
    log_array[axis] = is_log;						\
    base_array[axis] = base;						\
    log_base_array[axis] = log(base);					\
} while(0)

/* handle reversed ranges */
#define CHECK_REVERSE(axis)						\
do {									\
    if ((auto_array[axis] == 0)						\
	&& (max_array[axis] < min_array[axis])				\
	) {								\
	double temp = min_array[axis];					\
	min_array[axis] = max_array[axis];				\
	max_array[axis] = temp;						\
	reverse_range[axis] = 1;					\
    } else								\
	reverse_range[axis] = (range_flags[axis]&RANGE_REVERSE);	\
} while(0)

/* get optional [min:max] */
#define PARSE_RANGE(axis)						\
do {									\
    if (equals(c_token, "[")) {						\
	c_token++;							\
	auto_array[axis] =						\
	    load_range(axis, &min_array[axis], &max_array[axis],	\
		       auto_array[axis]);				\
	if (!equals(c_token, "]"))					\
	    int_error(c_token, "']' expected");				\
	c_token++;							\
    }									\
} while (0)

/* HBB 20000430: new macro, like PARSE_RANGE, but for named ranges as
 * in 'plot [phi=3.5:7] sin(phi)' */
#define PARSE_NAMED_RANGE(axis, dummy_token)				\
do {									\
    if (equals(c_token, "[")) {						\
	c_token++;							\
	if (isletter(c_token)) {					\
	    if (equals(c_token + 1, "=")) {				\
		dummy_token = c_token;					\
		c_token += 2;						\
	    } 								\
		/* oops; probably an expression with a variable: act	\
		 * as if no variable name had been seen, by		\
		 * fallthrough */ 					\
	}								\
	auto_array[axis] = load_range(axis, &min_array[axis],		\
				      &max_array[axis],			\
				      auto_array[axis]);		\
	if (!equals(c_token, "]"))					\
	    int_error(c_token, "']' expected");				\
	c_token++;							\
    }				/* first '[' */				\
} while (0)

/* parse a position of the form
 *    [coords] x, [coords] y {,[coords] z}
 * where coords is one of first,second.graph,screen
 * if first or second, we need to take axis_is_timedata into account
 */
#define GET_NUMBER_OR_TIME(store,axes,axis)			\
do {								\
    if (((axes) >= 0) && (axis_is_timedata[(axes)+(axis)])	\
	&& isstring(c_token)) {					\
	char ss[80];						\
	struct tm tm;						\
	quote_str(ss,c_token, 80);				\
	++c_token;						\
	if (gstrptime(ss,timefmt,&tm))				\
	    (store) = (double) gtimegm(&tm);			\
    } else {							\
	struct value value;					\
	(store) = real(const_express(&value));			\
    }								\
} while(0)

/* This is one is very similar to GET_NUMBER_OR_TIME, but has slightly
 * different usage: it writes out '0' in case of inparsable time data,
 * and it's used when the target axis is fixed without a 'first' or
 * 'second' keyword in front of it. */
#define GET_NUM_OR_TIME(store,axis)			\
do {							\
    (store) = 0;					\
    GET_NUMBER_OR_TIME(store, FIRST_AXES, axis);	\
} while (0);

/* store VALUE or log(VALUE) in STORE, set TYPE as appropriate
 * Do OUT_ACTION or UNDEF_ACTION as appropriate
 * adjust range provided type is INRANGE (ie dont adjust y if x is outrange
 * VALUE must not be same as STORE
 */

#define STORE_WITH_LOG_AND_UPDATE_RANGE(STORE, VALUE, TYPE, AXIS,	\
				       OUT_ACTION, UNDEF_ACTION)	\
do {									\
    if (log_array[AXIS]) {						\
	if (VALUE<0.0) {						\
	    TYPE = UNDEFINED;						\
	    UNDEF_ACTION;						\
	    break;							\
	} else if (VALUE == 0.0) {					\
	    STORE = -VERYLARGE;						\
	    TYPE = OUTRANGE;						\
	    OUT_ACTION;							\
	    break;							\
	} else {							\
	    STORE = AXIS_DO_LOG(AXIS,VALUE);				\
	}								\
    } else								\
	STORE = VALUE;							\
    if (TYPE != INRANGE)						\
	break;  /* dont set y range if x is outrange, for example */	\
    if ( VALUE<min_array[AXIS] ) {					\
	if (auto_array[AXIS] & 1)					\
	    min_array[AXIS] = VALUE;					\
	else {								\
	    TYPE = OUTRANGE;						\
	    OUT_ACTION;							\
	    break;							\
	}								\
    }									\
    if ( VALUE>max_array[AXIS] ) {					\
	if (auto_array[AXIS] & 2)					\
	    max_array[AXIS] = VALUE;					\
	else {								\
	    TYPE = OUTRANGE;						\
	    OUT_ACTION;							\
	}								\
    }									\
} while(0)

/* use this instead empty macro arguments to work around NeXT cpp bug */
/* if this fails on any system, we might use ((void)0) */
#define NOOP			/* */

/* ------------ functions exported by axis.c */
extern TBOOLEAN load_range __PROTO((AXIS_INDEX, double *a, double *b, TBOOLEAN autosc));
extern void axis_unlog_interval __PROTO((AXIS_INDEX, double *, double *, TBOOLEAN checkrange));
extern void axis_revert_and_unlog_range __PROTO((AXIS_INDEX));
extern double axis_log_value_checked __PROTO((AXIS_INDEX, double, const char *));
extern void axis_checked_extend_empty_range __PROTO((AXIS_INDEX, const char *mesg));
extern double set_tic __PROTO((double, int));
extern void setup_tics __PROTO((AXIS_INDEX, struct ticdef *, char *, int));
extern void gen_tics __PROTO((AXIS_INDEX, struct ticdef *, int, int, double, tic_callback));
extern void axis_output_tics __PROTO((AXIS_INDEX, int, int, int, int, int *, AXIS_INDEX, struct ticdef *, int, int, double, tic_callback));
extern void axis_set_graphical_range __PROTO((AXIS_INDEX, unsigned int lower, unsigned int upper));
extern TBOOLEAN axis_position_zeroaxis __PROTO((AXIS_INDEX));
#endif /* GNUPLOT_AXIS_H */
