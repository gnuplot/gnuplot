/*
 * $Id: plot.h%v 3.50 1993/07/09 05:35:24 woo Exp $
 *
 */

/* GNUPLOT - plot.h */
/*
 * Copyright (C) 1986 - 1993   Thomas Williams, Colin Kelley
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
 * There is a mailing list for gnuplot users. Note, however, that the
 * newsgroup 
 *	comp.graphics.gnuplot 
 * is identical to the mailing list (they
 * both carry the same set of messages). We prefer that you read the
 * messages through that newsgroup, to subscribing to the mailing list.
 * (If you can read that newsgroup, and are already on the mailing list,
 * please send a message info-gnuplot-request@dartmouth.edu, asking to be
 * removed from the mailing list.)
 *
 * The address for mailing to list members is
 *	   info-gnuplot@dartmouth.edu
 * and for mailing administrative requests is 
 *	   info-gnuplot-request@dartmouth.edu
 * The mailing list for bug reports is 
 *	   bug-gnuplot@dartmouth.edu
 * The list of those interested in beta-test versions is
 *	   info-gnuplot-beta@dartmouth.edu
 */

#define PROGRAM "G N U P L O T"
#define PROMPT "gnuplot> "
#if defined(AMIGA_SC_6_1) || defined(AMIGA_AC_5)
#define SHELL "NewShell"
#else /* AMIGA */
#ifdef ATARI
#define SHELL "gulam.prg"
#else /* ATARI */
#ifdef OS2
#define SHELL "c:\\cmd.exe"
#else /*OS2 */
#define SHELL "/bin/sh"		/* used if SHELL env variable not set */
#endif /*OS2 */
#endif /* ATARI */
#endif /* AMIGA  */

#if defined(__unix__) && !defined(unix)
#define unix
#endif

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
#define MAX_TOKENS 400
#define MAX_ID_LEN 50		/* max length of an identifier */


#define MAX_AT_LEN 150		/* max number of entries in action table */
#define STACK_DEPTH 100
#define NO_CARET (-1)

#ifdef MSDOS
#define MAX_NUM_VAR	3	/* Ploting projection of func. of max. five vars. */
#else
#define MAX_NUM_VAR	5	/* Ploting projection of func. of max. five vars. */
#endif

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

#define LEVELS_AUTO			0		/* How contour levels are set */
#define LEVELS_INCREMENTAL	1		/* user specified start & incremnet */
#define LEVELS_DISCRETE		2		/* user specified discrete levels */
#define MAX_DISCRETE_LEVELS   30

#define ANGLES_RADIANS	0
#define ANGLES_DEGREES	1


#if defined(AMIGA_SC_6_1) || defined(AMIGA_AC_5)
#define OS "Amiga "
#endif

#ifdef OS2
#ifdef unix
#undef unix	/* GCC might declare this */
#define OS "OS/2"
#endif
#endif  /* OS2 */

#ifdef vms
#define OS "VMS "
#endif

#ifdef linux
#define OS "Linux "
#else
#ifdef unix
#define OS "unix "
#endif
#endif

#ifdef _WINDOWS
#define _Windows
#endif

#ifdef DOS386
#define OS "DOS 386 "
#endif
#ifdef _Windows
#define OS "MS-Windows "
#else
#ifdef MSDOS
#ifdef unix	/* __EMX__ and DJGPP may set this */
#undef OS
#undef unix
#endif
#define OS "MS-DOS "
#endif
#endif


#ifdef ATARI
#define OS "TOS "
#endif
 
#ifndef OS
#define OS ""
#endif


/* To access curves larger than 64k, MSDOS needs to use huge pointers */
#if (defined(__TURBOC__) && defined(MSDOS)) || (defined(_Windows) && !defined(WIN32))
#define GPHUGE huge
#define GPFAR far
#else
#define GPHUGE
#define GPFAR
#endif


/*
 * Note about VERYLARGE:  This is the upper bound double (or float, if PC)
 * numbers. This flag indicates very large numbers. It doesn't have to 
 * be the absolutely biggest number on the machine.  
 * If your machine doesn't have HUGE, or float.h,
 * define VERYLARGE here. 
 *
 * This is a mess.  If someone figures out how to clean this up, please
 *    diff -c  of your fixes
 *
 *
 * example:
#define VERYLARGE 1e37
 */

#ifdef ATARI
#include <stdlib.h>		/* Prototyping used !! 'size_t' */
#include <stdio.h>
#include <string.h>
#include <math.h>
#define VERYLARGE	HUGE_VAL
#else  /* not ATARI */
#if defined(MSDOS) || defined(_Windows)
#include <float.h>
#define VERYLARGE (FLT_MAX/2 -1)
#else  /* not MSDOS || _Windows */
#if defined( vms ) || defined( _CRAY ) || defined( NEXT ) || defined(__osf__) || defined( OS2 ) || defined(__EMX__) || defined( DOS386) || defined(KSR)
#include <float.h>
#if defined ( NEXT )  /* bug in NeXT OS 2.0 */
#if defined ( DBL_MAX)
#undef DBL_MAX
#endif
#define DBL_MAX 1.7976931348623157e+308 
#undef HUGE
#define HUGE	DBL_MAX
#undef HUGE_VAL
#define HUGE_VAL DBL_MAX
#endif /* not NEXT but CRAY, VMS or OSF */
#define VERYLARGE (DBL_MAX/2 -1)
#else  /* not vms, CRAY, NEXT, OS/2 or OSF */
#ifdef AMIGA_AC_5
#include <math.h>
#define VERYLARGE (HUGE/2 -1)
#else /* not AMIGA_AC_5 */
#ifdef AMIGA_SC_6_1
#include <float.h>
#ifndef HUGE
#define HUGE DBL_MAX
#endif
#define VERYLARGE (HUGE/2 -1)
#else /* !AMIGA_SC_6_1 */
/* #include <float.h> */
#ifdef ISC22
#include <float.h>
#ifndef HUGE
#define HUGE DBL_MAX
#endif
#endif /* ISC22 */
/* This is the default */
#ifndef HUGE
#define HUGE DBL_MAX
#endif
#define VERYLARGE (HUGE/2 -1)
/* default */
#endif /* !AMIGA_SC_6_1 */
#endif /* !AMIGA_AC_5 */
#endif /* !VMS !CRAY !NEXT */
#endif /* !MSDOS || !_Windows */
#endif /* !ATARI */

#ifdef AMIGA_SC_6_1
/* Get function prototypes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#endif /* AMIGA_SC_6_1 */

#define END_OF_COMMAND (c_token >= num_tokens || equals(c_token,";"))

#ifdef vms


#define is_comment(c) ((c) == '#' || (c) == '!')
#define is_system(c) ((c) == '$')


#else /* vms */

#define is_comment(c) ((c) == '#')
#define is_system(c) ((c) == '!')

#endif /* vms */

/* If you don't have vfork, then undefine this */
#if defined(NOVFORK) || defined(MSDOS) || defined( OS2 ) || defined(_Windows) || defined(DOS386)
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
# if defined(MEMCPY) || defined(MSDOS) || defined (ATARI) || defined( OS2 ) || defined(_Windows) || defined(DOS386)
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
#if defined(MEMSET) || defined(MSDOS) || defined( OS2 ) || defined(_Windows) || defined(DOS386)
#define bzero(dest,len)  (void)(memset(dest, 0, len))
#endif /* MEMSET || MSDOS */

#define top_of_stack stack[s_p]

typedef int TBOOLEAN;

#ifdef __ZTC__
typedef int (*FUNC_PTR)(...);
#else
typedef int (*FUNC_PTR)();
#endif

enum operators {
	PUSH, PUSHC, PUSHD1, PUSHD2, PUSHD, CALL, CALLN, LNOT, BNOT, UMINUS,
	LOR, LAND, BOR, XOR, BAND, EQ, NE, GT, LT, GE, LE, PLUS, MINUS, MULT,
	DIV, MOD, POWER, FACTORIAL, BOOLE, JUMP, JUMPZ, JUMPNZ, JTERN, SF_START
};


#define is_jump(operator) ((operator) >=(int)JUMP && (operator) <(int)SF_START)


enum DATA_TYPES {
	INTGR, CMPLX
};


enum PLOT_TYPE {
	FUNC, DATA, FUNC3D, DATA3D
};

/*XXX - JG */
enum PLOT_STYLE {
	LINES, POINTSTYLE, IMPULSES, LINESPOINTS, DOTS, ERRORBARS, BOXES, BOXERROR, STEPS
};

enum JUSTIFY {
	LEFT, CENTRE, RIGHT
};

#if !(defined(ATARI)&&defined(__GNUC__)&&defined(_MATH_H)) /* FF's math.h has the type already */
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
	TBOOLEAN udv_undef;		/* true if not defined yet */
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
  
#if defined(MSDOS) || defined(_Windows) 
typedef float coordval;		/* memory is tight on PCs! */
#else
typedef double coordval;
#endif

struct coordinate {
	enum coord_type type;	/* see above */
	coordval x, y, z;
	coordval ylow, yhigh;	/* ignored in 3d */
#if (defined(_Windows) && !defined(WIN32)) || (defined(MSDOS) && defined(__TURBOC__))
	char pad[10];		/* pad to 32 byte boundary */
#endif
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
	struct coordinate GPHUGE *points;
};

struct gnuplot_contours {
	struct gnuplot_contours *next;
	struct coordinate GPHUGE *coords;
 	char isNewLevel;
 	char label[12];
	int num_pts;
};

struct iso_curve {
	struct iso_curve *next;
 	int p_max;					/* how many points are allocated */
	int p_count;					/* count of points in points */
	struct coordinate GPHUGE *points;
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

struct TERMENTRY {
	char *name;
#if defined(_Windows) && !defined(WIN32)
	char GPFAR description[80];	/* to make text go in FAR segment */
#else
	char *description;
#endif
	unsigned int xmax,ymax,v_char,h_char,v_tic,h_tic;
	FUNC_PTR options,init,reset,text,scale,graphics,move,vector,linetype,
		put_text,text_angle,justify_text,point,arrow;
};

#ifdef _Windows
#define termentry TERMENTRY far
#else
#define termentry TERMENTRY
#endif


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
	TBOOLEAN head;			/* arrow has a head or not */
};

/* Tic-mark labelling definition; see set xtics */
struct ticdef {
    int type;				/* one of three values below */
#define TIC_COMPUTED 1		/* default; gnuplot figures them */
#define TIC_SERIES 2		/* user-defined series */
#define TIC_USER 3			/* user-defined points */
#define TIC_MONTH 4		/* print out month names ((mo-1)%12)+1 */
#define TIC_DAY 5		/* print out day of week */
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
extern TBOOLEAN screen_ok;
extern TBOOLEAN term_init;
extern TBOOLEAN undefined;
extern struct termentry term_tbl[];

extern char *alloc();
extern char GPFAR *gpfaralloc();	/* far versions */
extern char GPFAR *gpfarrealloc();
extern void gpfarfree();
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

/* Windows needs to redefine stdin/stdout functions */
#ifdef _Windows
#include "win/wtext.h"
#endif
