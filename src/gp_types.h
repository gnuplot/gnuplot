/*
 * $Id: gp_types.h,v 1.2.2.3 2000/06/22 12:57:38 broeker Exp $
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

#define MAX_ID_LEN 50		/* max length of an identifier */
#define MAX_LINE_LEN 1024	/* maximum number of chars allowed on line */

#define DEG2RAD (M_PI / 180.0)

/* These used to be #defines in plot.h. For easier debugging, I've
 * converted most of them into enum's. */
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

/* FIXME HBB 20000521: 'struct value' and its part, 'cmplx', should go
 * into one of scanner/internal/standard/util .h, but I've yet to
 * decide which of them */

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

#if 0 /* HBB 20000521: move to command.h */
typedef struct lexical_unit {	/* produced by scanner */
	TBOOLEAN is_token;	/* true if token, false if a value */ 
	struct value l_val;
	int start_index;	/* index of first char in token */
	int length;			/* length of token in chars */
} lexical_unit;
#endif /* 0 */

#if 0 /* HBB 20000521: move to eval.h */
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
#endif /* 0 */


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

#endif /* GNUPLOT_GPTYPES_H */
