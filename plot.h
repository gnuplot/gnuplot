/*
 * $Id: plot.h,v 3.26 92/03/24 22:34:13 woo Exp Locker: woo $
 *
 */

/* GNUPLOT - plot.h */
/*
 * Copyright (C) 1986, 1987, 1990, 1991, 1992   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *  
 * This software is provided "as is" without express or implied warranty.
 * 
 *
 * AUTHORS
 * 
 *   Original Software:
 *     Thomas Williams,  Colin Kelley.
 * 
 *   Gnuplot 2.0 additions:
 *       Russell Lang, Dave Kotz, John Campbell.
 *
 *   Gnuplot 3.0 additions:
 *       Gershon Elber and many others.
 * 
 * Send your comments or suggestions to 
 *  info-gnuplot@ames.arc.nasa.gov.
 * This is a mailing list; to join it send a note to 
 *  info-gnuplot-request@ames.arc.nasa.gov.  
 * Send bug reports to
 *  bug-gnuplot@ames.arc.nasa.gov.
 */

#define PROGRAM "G N U P L O T"
#define PROMPT "gnuplot> "
#if defined(AMIGA_LC_5_1) || defined(AMIGA_AC_5)
#define SHELL "NewShell"
#else /* AMIGA */
#define SHELL "/bin/sh"		/* used if SHELL env variable not set */
#endif /* AMIGA  */

#define SAMPLES 100		/* default number of samples for a plot */
#define ISO_SAMPLES 10		/* default number of isolines per splot */
#define ZERO	1e-8		/* default for 'zero' set option */

#ifndef TERM
/* default terminal is "unknown"; but see init_terminal */
#define TERM "unknown"
#endif

#define TRUE 1
#define FALSE 0


#define Pi 3.141592653589793
#define DEG2RAD (Pi / 180.0)


#define MIN_CRV_POINTS 100		/* minimum size of points[] in curve_points */
#define MIN_SRF_POINTS 1000		/* minimum size of points[] in surface_points */

#define MAX_LINE_LEN 1024	/* maximum number of chars allowed on line */
#define MAX_TOKENS 200
#define MAX_ID_LEN 50		/* max length of an identifier */


#define MAX_AT_LEN 150		/* max number of entries in action table */
#define STACK_DEPTH 100
#define NO_CARET (-1)


#define MAX_NUM_VAR	2	/* Ploting projection of func. of max. two vars. */

#define MAP3D_CARTESIAN		0	/* 3D Data mapping. */
#define MAP3D_SPHERICAL		1
#define MAP3D_CYLINDRICAL	2

#define CONTOUR_NONE	0	/* Where to place contour maps if at all. */
#define CONTOUR_BASE	1
#define CONTOUR_SRF	2
#define CONTOUR_BOTH	3

#define CONTOUR_KIND_LINEAR	0 /* See contour.h in contours subdirectory. */
#define CONTOUR_KIND_CUBIC_SPL	1
#define CONTOUR_KIND_BSPLINE	2

#define ANGLES_RADIANS	0
#define ANGLES_DEGREES	1


#if defined(AMIGA_LC_5_1) || defined(AMIGA_AC_5)
#define OS "Amiga "
#endif


#ifdef vms
#define OS "VMS "
#endif


#ifdef unix
#define OS "unix "
#endif


#ifdef MSDOS
#define OS "MS-DOS "
#endif


#ifndef OS
#define OS ""
#endif


/*
 * Note about VERYLARGE:  This is the upper bound double (or float, if PC)
 * numbers. This flag indicates very large numbers. It doesn't have to 
 * be the absolutely biggest number on the machine.  
 * If your machine doesn't have HUGE, or float.h,
 * define VERYLARGE here. 
 *
 * example:
#define VERYLARGE 1e38
 */

#ifdef PC
#include <float.h>
#define VERYLARGE FLT_MAX
#else
#if defined( vms ) || defined( _CRAY ) || defined( NEXT )
#include <float.h>
#define VERYLARGE DBL_MAX
#else
#if defined(AMIGA_AC_5) || defined(AMIGA_LC_5_1)
#include <math.h>
#define VERYLARGE HUGE
#else
#define VERYLARGE HUGE
#endif
#endif
#endif


#define END_OF_COMMAND (c_token >= num_tokens || equals(c_token,";"))

#ifdef vms


#define is_comment(c) ((c) == '#' || (c) == '!')
#define is_system(c) ((c) == '$')


#else /* vms */


#define is_comment(c) ((c) == '#')
#define is_system(c) ((c) == '!')


#endif /* vms */

/* If you don't have vfork, then undefine this */
#if defined(NOVFORK) || defined(MSDOS)
# undef VFORK
#else
# ifdef unix
#  define VFORK
# endif
#endif

/* 
 * memcpy() comes by many names. The default is now to assume bcopy, 
 * since it is the most common case. Define 
 *  MEMCPY to use memcpy(), 
 *  vms to use the vms function,
 *  NOCOPY to use a handwritten version in parse.c
 */
#ifdef vms
# define memcpy(dest,src,len) lib$movc3(&len,src,dest)
#else
# if defined(MEMCPY) || defined(MSDOS)
   /* use memcpy directly */
# else 
#  ifdef NOCOPY
    /* use the handwritten memcpy in parse.c */
#  else
    /* assume bcopy is in use */
#   define memcpy(dest,src,len) bcopy(src,dest,len)
#  endif /* NOCOPY */
# endif /* MEMCPY || MSDOS */
#endif /* vms */

/*
 * In case you have MEMSET instead of BZERO. If you have something 
 * else, define bzero to that something.
 */
#if defined(MEMSET) || defined(MSDOS)
#define bzero(dest,len)  (void)(memset(dest, (char)NULL, len))
#endif /* MEMSET || MSDOS */

/* Give the name of your gamma function, or undefine it if you have none.  */
#if defined(NOGAMMA) || defined(MSDOS)
# undef GAMMA
#else
# ifndef GAMMA
#  define GAMMA gamma
# endif /* GAMMA */
#endif /* NOGAMMA ||MSDOS */

#define top_of_stack stack[s_p]

typedef int BOOLEAN;

#ifdef __ZTC__
typedef int (*FUNC_PTR)(...);
#else
typedef int (*FUNC_PTR)();
#endif

#if defined(AMIGA_LC_5_1) || defined(AMIGA_AC_5)
enum operators {
	PUSH, PUSHC, PUSHD1, PUSHD2, CALL, CALL2, LNOT, BNOT, UMINUS, LOR, LAND,
	BOR, XOR, BAND, EQ, NE, GT, LT, GE, LE, PLUS, MINUS, MULT, DIV,
	MOD, POWER, FACTORIAL, ABOOL, JUMP, JUMPZ, JUMPNZ, JTERN, SF_START
};
#else
enum operators {
	PUSH, PUSHC, PUSHD1, PUSHD2, CALL, CALL2, LNOT, BNOT, UMINUS, LOR, LAND,
	BOR, XOR, BAND, EQ, NE, GT, LT, GE, LE, PLUS, MINUS, MULT, DIV,
	MOD, POWER, FACTORIAL, BOOLE, JUMP, JUMPZ, JUMPNZ, JTERN, SF_START
};
#endif


#define is_jump(operator) ((operator) >=(int)JUMP && (operator) <(int)SF_START)


enum DATA_TYPES {
	INT, CMPLX
};


enum PLOT_TYPE {
	FUNC, DATA, FUNC3D, DATA3D
};


enum PLOT_STYLE {
	LINES, POINTS, IMPULSES, LINESPOINTS, DOTS, ERRORBARS
};

enum JUSTIFY {
	LEFT, CENTRE, RIGHT
};

struct cmplx {
	double real, imag;
};


struct value {
	enum DATA_TYPES type;
	union {
		int int_val;
		struct cmplx cmplx_val;
	} v;
};


struct lexical_unit {	/* produced by scanner */
	BOOLEAN is_token;	/* true if token, false if a value */ 
	struct value l_val;
	int start_index;	/* index of first char in token */
	int length;			/* length of token in chars */
};


struct ft_entry {		/* standard/internal function table entry */
	char *f_name;		/* pointer to name of this function */
	FUNC_PTR func;		/* address of function to call */
};


struct udft_entry {				/* user-defined function table entry */
	struct udft_entry *next_udf; 		/* pointer to next udf in linked list */
	char udf_name[MAX_ID_LEN+1]; 		/* name of this function entry */
	struct at_type *at;			/* pointer to action table to execute */
	char *definition; 			/* definition of function as typed */
	struct value dummy_values[MAX_NUM_VAR];	/* current value of dummy variables */
};


struct udvt_entry {			/* user-defined value table entry */
	struct udvt_entry *next_udv; /* pointer to next value in linked list */
	char udv_name[MAX_ID_LEN+1]; /* name of this value entry */
	BOOLEAN udv_undef;		/* true if not defined yet */
	struct value udv_value;	/* value it has */
};


union argument {			/* p-code argument */
	int j_arg;				/* offset for jump */
	struct value v_arg;		/* constant value */
	struct udvt_entry *udv_arg;	/* pointer to dummy variable */
	struct udft_entry *udf_arg; /* pointer to udf to execute */
};


struct at_entry {			/* action table entry */
	enum operators index;	/* index of p-code function */
	union argument arg;
};


struct at_type {
	int a_count;				/* count of entries in .actions[] */
	struct at_entry actions[MAX_AT_LEN];
		/* will usually be less than MAX_AT_LEN is malloc()'d copy */
};


/* Defines the type of a coordinate */
/* INRANGE and OUTRANGE points have an x,y point associated with them */
enum coord_type {
    INRANGE,				/* inside plot boundary */
    OUTRANGE,				/* outside plot boundary, but defined */
    UNDEFINED				/* not defined at all */
};
  
#ifdef PC
typedef float coordval;		/* memory is tight on PCs! */
#else
typedef double coordval;
#endif

struct coordinate {
	enum coord_type type;	/* see above */
	coordval x, y, z;
	coordval ylow, yhigh;	/* ignored in 3d */
};

struct curve_points {
	struct curve_points *next_cp;	/* pointer to next plot in linked list */
	enum PLOT_TYPE plot_type;
	enum PLOT_STYLE plot_style;
	char *title;
	int line_type;
	int point_type;
 	int p_max;					/* how many points are allocated */
	int p_count;					/* count of points in points */
	struct coordinate *points;
};

struct gnuplot_contours {
	struct gnuplot_contours *next;
	struct coordinate *coords;
	int num_pts;
};

struct iso_curve {
	struct iso_curve *next;
 	int p_max;					/* how many points are allocated */
	int p_count;					/* count of points in points */
	struct coordinate *points;
};

struct surface_points {
	struct surface_points *next_sp;	/* pointer to next plot in linked list */
	enum PLOT_TYPE plot_type;
	enum PLOT_STYLE plot_style;
	char *title;
	int line_type;
	int point_type;
	int has_grid_topology;
	int num_iso_read;  /* Data files only - num of isolines read from file. */
	struct gnuplot_contours *contours;	/* Not NULL If have contours. */
	struct iso_curve *iso_crvs;
};

struct termentry {
	char *name;
	char *description;
	unsigned int xmax,ymax,v_char,h_char,v_tic,h_tic;
	FUNC_PTR options,init,reset,text,scale,graphics,move,vector,linetype,
		put_text,text_angle,justify_text,point,arrow;
};


struct text_label {
	struct text_label *next;	/* pointer to next label in linked list */
	int tag;			/* identifies the label */
	double x,y,z;
	enum JUSTIFY pos;
	char text[MAX_LINE_LEN+1];
};

struct arrow_def {
	struct arrow_def *next;	/* pointer to next arrow in linked list */
	int tag;			/* identifies the arrow */
	double sx,sy,sz;		/* start position */
	double ex,ey,ez;		/* end position */
	BOOLEAN head;			/* arrow has a head or not */
};

/* Tic-mark labelling definition; see set xtics */
struct ticdef {
    int type;				/* one of three values below */
#define TIC_COMPUTED 1		/* default; gnuplot figures them */
#define TIC_SERIES 2		/* user-defined series */
#define TIC_USER 3			/* user-defined points */
    union {
	   struct {			/* for TIC_SERIES */
		  double start, incr;
		  double end;		/* ymax, if VERYLARGE */
	   } series;
	   struct ticmark *user;	/* for TIC_USER */
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

/*
 * SS$_NORMAL is "normal completion", STS$M_INHIB_MSG supresses

 * printing a status message.
 * SS$_ABORT is the general abort status code.
 from:	Martin Minow
	decvax!minow
 */
#ifdef	vms
#include		<ssdef.h>
#include		<stsdef.h>
#define	IO_SUCCESS	(SS$_NORMAL | STS$M_INHIB_MSG)
#define	IO_ERROR	SS$_ABORT
#endif /* vms */


#ifndef	IO_SUCCESS	/* DECUS or VMS C will have defined these already */
#define	IO_SUCCESS	0
#endif
#ifndef	IO_ERROR
#define	IO_ERROR	1
#endif

/* Some key global variables */
extern BOOLEAN screen_ok;
extern BOOLEAN term_init;
extern BOOLEAN undefined;
extern struct termentry term_tbl[];

extern char *alloc();
/* allocating and managing curve_points structures */
extern struct curve_points *cp_alloc();
extern int cp_extend();
extern int cp_free();
/* allocating and managing surface_points structures */
extern struct surface_points *sp_alloc();
extern int sp_replace();
extern int sp_free();
/* allocating and managing is_curve structures */
extern struct iso_curve *iso_alloc();
extern int iso_extend();
extern int iso_free();
