/*
 * $Id: plot.h,v 1.120 1998/04/14 00:16:06 drd Exp $
 *
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

#ifndef PLOT_H		/* avoid multiple includes */
#define PLOT_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ansichek.h"
#include "stdfn.h"

#define PROGRAM "G N U P L O T"
#define PROMPT "gnuplot> "

/*
 * Define operating system dependent constants [default value]:
 *
 * OS:      [" "] Name of OS plus trailing space
 * HOME:    [HOME] Name of environment variable which points to
 *          the directory where gnuplot's config file is found.
 * PLOTRC:  [".gnuplot"] Name of the gnuplot startup file.
 * SHELL:   ["/bin/sh"] Name, and in some cases, full path to the shell
 *          that is used to run external commands.
 * DIRSEP1: ['/'] Primary character which separates path components.
 * DIRSEP2: ['\0'] Secondary character which separates path components.
 *        
 */

#if defined(AMIGA_SC_6_1) || defined(AMIGA_AC_5) || defined(__amigaos__)
# define OS "Amiga "
# ifndef __amigaos__
#  define HOME     "GNUPLOT"
#  define SHELL    "NewShell"
#  define DIRSEP2 ':'
# endif
# ifndef AMIGA
#  define AMIGA
# endif
#endif /* Amiga */

#ifdef ATARI
# define OS "TOS "
# define HOME  "GNUPLOT"
# define PLOTRC "gnuplot.ini"
# define SHELL "gulam.prg"
# define DIRSEP1 '\\'
# ifdef MTOS
#  define DIRSEP2 '/'
# endif
#endif /* Atari */

#ifdef DOS386
# define OS "DOS 386 "
# define HOME  "GNUPLOT"
# define PLOTRC "gnuplot.ini"
# define DIRSEP1 '\\'
#endif /* DOS386 */

#ifdef linux
# define OS "Linux "
#endif /* Linux */

#if defined(__NeXT__) && !defined(NEXT)
# define NEXT
#endif /* NeXT */

#ifdef OS2
# define OS "OS/2 "
# define HOME  "GNUPLOT"
# define PLOTRC "gnuplot.ini"
# define SHELL "c:\\cmd.exe"
# define DIRSEP1 '\\'
#endif /* OS/2 */

#ifdef OSK
# define OS "OS-9 "
# define SHELL "/dd/cmds/shell"
#endif /* OS-9 */

#if defined(vms) || defined(VMS)
# ifndef VMS
#  define VMS
# endif
# define OS "VMS "
# define HOME "sys$login:"
# define PLOTRC "gnuplot.ini"
# if !defined(VAXCRTL) && !defined(DECCRTL)
#  error Please /define either VAXCRTL or DECCRTL
# endif
/* avoid some IMPLICITFUNC warnings */
# ifdef __DECC
#  include <starlet.h>
# endif  /* __DECC */
#endif /* VMS */

#if defined(_WINDOWS) || defined(_Windows)
# ifndef _Windows
#  define _Windows
# endif
# ifdef WIN32
#  define OS "MS-Windows 32 bit "
# else
#  ifndef WIN16
#   define WIN16
#  endif
#  define OS "MS-Windows "
# endif /* WIN32 */
# define HOME  "GNUPLOT"
# define PLOTRC "gnuplot.ini"
# define DIRSEP1 '\\'
#endif /* _WINDOWS */

#if defined(MSDOS) && !defined(_Windows)
# if !defined(DOS32) && !defined(DOS16)
#  define DOS16
# endif
# ifdef MTOS
#  define OS "TOS & MiNT & MULTITOS & Magic - "
# endif /* MTOS */
# define HOME "GNUPLOT"
# define PLOTRC "gnuplot.ini"
# define OS "MS-DOS "
# define DIRSEP1 '\\'
# ifdef __DJGPP__
#  define DIRSEP2 '/'
# endif
#endif /* MSDOS */

#if defined(__unix__) || defined(unix)
# ifndef unix
#  define unix
# endif
# ifndef OS
#  define OS "Unix "
# endif
#endif /* Unix */

/* Note: may not catch all IBM AIX compilers */
#ifdef _AIX
# ifndef unix
#  define unix
# endif
# define OS "Unix "
#endif /* AIX */

/* Attempted fix for SCO */
#ifdef SCO
# ifndef unix
#  define unix
# endif
# define OS "Unix "
#endif /* SCO */

/* End OS dependent constants; fall-through defaults
 * for the constants defined above are following.
 */

#ifndef OS
# define OS " "
#endif

#ifndef HOME
# define HOME "HOME"
#endif

#ifndef PLOTRC
# define PLOTRC ".gnuplot"
#endif

#ifndef SHELL
# define SHELL "/bin/sh"    /* used if SHELL env variable not set */
#endif

#ifndef DIRSEP1
# define DIRSEP1 '/'
#endif

#ifndef DIRSEP2
# define DIRSEP2 NUL
#endif

/* Define helpfile location - overriden by Makefile */
#ifndef HELPFILE
# if defined( MSDOS ) || defined( OS2 ) || defined(DOS386)
#  define HELPFILE "gnuplot.gih"
# else
#  if defined(AMIGA_SC_6_1) || defined(AMIGA_AC_5)
#   define HELPFILE "S:gnuplot.gih"
#  else
#   define HELPFILE "docs/gnuplot.gih"  /* changed by makefile */
#  endif /* AMIGA_SC_6_1 || AMIGA_AC_5 */
# endif /* MSDOS || OS2 || DOS386 */
#endif /* !HELPFILE */

#ifndef CONTACT
# define CONTACT "bug-gnuplot@dartmouth.edu"
#endif

#ifndef HELPMAIL
# define HELPMAIL "info-gnuplot@dartmouth.edu"
#endif

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
# define TRUE 1
# define FALSE 0
#endif

#define DTRUE 3 /* double true, used in autoscale: 1=autoscale ?min, 2=autoscale ?max */

#define Pi 3.141592653589793
#define DEG2RAD (Pi / 180.0)


#define MIN_CRV_POINTS 100		/* minimum size of points[] in curve_points */
#define MIN_SRF_POINTS 1000		/* minimum size of points[] in surface_points */


/* note that MAX_LINE_LEN, MAX_TOKENS and MAX_AT_LEN do no longer limit the
   size of the input. The values describe the steps in which the sizes are
   extended. */

#define MAX_LINE_LEN 1024	/* maximum number of chars allowed on line */
#define MAX_TOKENS 400
#define MAX_ID_LEN 50		/* max length of an identifier */


#define MAX_AT_LEN 150		/* max number of entries in action table */
#define STACK_DEPTH 100
#define NO_CARET (-1)

#ifdef DOS16
# define MAX_NUM_VAR	3	/* Ploting projection of func. of max. five vars. */
#else /* not DOS16 */
# define MAX_NUM_VAR	5	/* Ploting projection of func. of max. five vars. */
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
#define MAX_DISCRETE_LEVELS   30

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
#define GRID_X      1
#define GRID_Y      2
#define GRID_Z      4
#define GRID_X2     8
#define GRID_Y2     16
#define GRID_MX     32
#define GRID_MY     64
#define GRID_MZ     128
#define GRID_MX2    256
#define GRID_MY2    512

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

/* introduced by Pedro Mendes, prm@aber.ac.uk */
#ifdef WIN32
#define far 
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
#  warning "using last resort 1e37 as VERYLARGE define, please check your headers"
# endif /* HUGE */
#endif /* VERYLARGE */

/* Some older platforms, namely SunOS 4.x, don't define this. */
#ifndef DBL_EPSILON
# define DBL_EPSILON     2.2204460492503131E-16
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

#define top_of_stack stack[s_p]

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

typedef int TBOOLEAN;

enum operators {
	/* keep this in line with table in plot.c */
	PUSH, PUSHC, PUSHD1, PUSHD2, PUSHD, CALL, CALLN, LNOT, BNOT, UMINUS,
	LOR, LAND, BOR, XOR, BAND, EQ, NE, GT, LT, GE, LE, PLUS, MINUS, MULT,
	DIV, MOD, POWER, FACTORIAL, BOOLE,
	DOLLARS, /* for using extension - div */
	/* only jump operators go between jump and sf_start */
   JUMP, JUMPZ, JUMPNZ, JTERN, SF_START
};


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
	LINES      = 0*8 + 1,
	POINTSTYLE = 1*8 + 2,
	IMPULSES   = 2*8 + 1,
	LINESPOINTS= 3*8 + 3,
	DOTS       = 4*8 + 0,
	XERRORBARS = 5*8 + 6,
	YERRORBARS = 6*8 + 6,
	XYERRORBARS= 7*8 + 6,
	BOXXYERROR = 8*8 + 1,
	BOXES      = 9*8 + 1,
	BOXERROR   =10*8 + 1,
	STEPS      =11*8 + 1,
	FSTEPS     =12*8 + 1,
	HISTEPS    =13*8 + 1,
	VECTOR     =14*8 + 1,
	CANDLESTICKS=15*8 + 4,
	FINANCEBARS=16*8 + 1
};

enum PLOT_SMOOTH { 
	NONE, UNIQUE, CSPLINES, ACSPLINES, BEZIER, SBEZIER

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


/* This type definition has to come after union argument has been declared. */
#ifdef __ZTC__
typedef int (*FUNC_PTR)(...);
#else
typedef int (*FUNC_PTR) __PROTO((union argument *arg));
#endif

struct ft_entry {		/* standard/internal function table entry */
	char *f_name;		/* pointer to name of this function */
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
};

/* default values for the structure 'lp_style_type' */
#define LP_DEFAULT {0,0,0,1.0,1.0}


struct curve_points {
	struct curve_points *next_cp;	/* pointer to next plot in linked list */
	int token;    /* last token in re/plot line, for second pass */
	enum PLOT_TYPE plot_type;
	enum PLOT_STYLE plot_style;
	enum PLOT_SMOOTH plot_smooth;
	char *title;
        struct lp_style_type lp_properties;
 	int p_max;					/* how many points are allocated */
	int p_count;					/* count of points in points */
	int x_axis;					/* FIRST_X_AXIS or SECOND_X_AXIS */
	int y_axis;					/* FIRST_Y_AXIS or SECOND_Y_AXIS */
	struct coordinate GPHUGE *points;
};

struct gnuplot_contours {
	struct gnuplot_contours *next;
	struct coordinate GPHUGE *coords;
 	char isNewLevel;
 	char label[32];
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
	int token;  /* last token in re/plot line, for second pass */
	enum PLOT_TYPE plot_type;
	enum PLOT_STYLE plot_style;
	char *title;
        struct lp_style_type lp_properties;
	int has_grid_topology;
	int num_iso_read;  /* Data files only - num of isolines read from file. */
	/* for functions, num_iso_read is the number of 'primary' isolines (in x dirn) */
	struct gnuplot_contours *contours;	/* Not NULL If have contours. */
	struct iso_curve *iso_crvs;
};

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
	char *name;
#ifdef WIN16
	char GPFAR description[80];	/* to make text go in FAR segment */
#else
	char *description;
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
	void (*put_text) __PROTO((unsigned int, unsigned int,char*));
/* the following are optional. set term ensures they are not NULL */
	int (*text_angle) __PROTO((int));
	int (*justify_text) __PROTO((enum JUSTIFY));
	void (*point) __PROTO((unsigned int, unsigned int,int));
	void (*arrow) __PROTO((unsigned int, unsigned int, unsigned int, unsigned int, int));
	int (*set_font) __PROTO((char *font));
	void (*pointsize) __PROTO((double)); /* change pointsize */
	int flags;
	void (*suspend) __PROTO((void)); /* called after one plot of multiplot */
	void (*resume)  __PROTO((void)); /* called before plots of multiplot */
	void (*fillbox) __PROTO((int, unsigned int, unsigned int, unsigned int, unsigned int)); /* clear in multiplot mode */
   void (*linewidth) __PROTO((double linewidth));
};

#ifdef WIN16
# define termentry TERMENTRY far
#else
# define termentry TERMENTRY
#endif


/* we might specify a co-ordinate in first/second/graph/screen coords */
/* allow x,y and z to be in separate co-ordinates ! */
enum position_type { first_axes, second_axes, graph, screen };

struct position {
	enum position_type scalex,scaley,scalez;
	double x,y,z;
};

struct text_label {
	struct text_label *next;	/* pointer to next label in linked list */
	int tag;			/* identifies the label */
	struct position place;
	enum JUSTIFY pos;
        int rotate;
	char text[MAX_LINE_LEN+1];
        char font[MAX_LINE_LEN+1];
}; /* Entry font added by DJL */

struct arrow_def {
	struct arrow_def *next;	/* pointer to next arrow in linked list */
	int tag;			/* identifies the arrow */
	struct position start;
	struct position end;
	TBOOLEAN head;			/* arrow has a head or not */
        struct lp_style_type lp_properties;
};

struct linestyle_def {
	struct linestyle_def *next;	/* pointer to next linestyle in linked list */
	int tag;			/* identifies the linestyle */
        struct lp_style_type lp_properties;
};

/* Tic-mark labelling definition; see set xtics */
struct ticdef {
    int type;				/* one of three values below */
#define TIC_COMPUTED 1		/* default; gnuplot figures them */
#define TIC_SERIES 2		/* user-defined series */
#define TIC_USER 3			/* user-defined points */
#define TIC_MONTH 4		/* print out month names ((mo-1[B)%12)+1 */
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
#ifdef VMS
# include		<ssdef.h>
# include		<stsdef.h>
# define	IO_SUCCESS	(SS$_NORMAL | STS$M_INHIB_MSG)
# define	IO_ERROR	SS$_ABORT
#endif /* VMS */


#ifndef	IO_SUCCESS	/* DECUS or VMS C will have defined these already */
# define	IO_SUCCESS	0
#endif

#ifndef	IO_ERROR
# define	IO_ERROR	1
#endif

/* Some key global variables */
extern TBOOLEAN screen_ok;		/* from util.c */
extern struct termentry *term;/* from term.c */
extern TBOOLEAN undefined;		/* from internal.c */
extern char *input_line;		/* from command.c */
extern int input_line_len;
extern char *replot_line;
extern struct lexical_unit *token;
extern int token_table_size;
extern int num_tokens, c_token;
extern int inline_num;
extern char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
extern struct ft_entry GPFAR ft[];	/* from plot.c */
extern struct udft_entry *first_udf;
extern struct udvt_entry *first_udv;
extern TBOOLEAN interactive;
extern char *infile_name;
extern struct udft_entry *dummy_func;
extern char dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];	/* from setshow.c */

/* Windows needs to redefine stdin/stdout functions */
#ifdef _Windows
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

#include "protos.h"


/* line/point parsing...
 *
 * allow_ls controls whether we are allowed to accept linestyle in
 * the current context [ie not when doing a  set linestyle command]
 * allow_point is whether we accept a point command
 * We assume compiler will optimise away if(0) or if(1)
 */
#if defined(ANSI_C) && defined(DEBUG_LP)
# define LP_DUMP(lp) \
 fprintf(stderr, \
  "lp_properties at %s:%d : lt: %d, lw: %.3f, pt: %d, ps: %.3f\n", \
  __FILE__, __LINE__, lp.l_type, lp.l_width, lp.p_type, lp.p_size)
#else
# define LP_DUMP(lp)
#endif

#define LP_PARSE(lp, allow_ls, allow_point, def_line, def_point) \
if (allow_ls && (almost_equals(c_token, "lines$tyle") || equals(c_token, "ls" ))) { \
 struct value t; ++c_token; \
 lp_use_properties(&(lp), (int) real(const_express(&t)), allow_point); \
} else { \
 	if (almost_equals(c_token, "linet$ype") || equals(c_token, "lt" )) { \
 		struct value t; ++c_token; \
      lp.l_type = (int) real(const_express(&t))-1; \
   } else lp.l_type = def_line; \
 	if (almost_equals(c_token, "linew$idth") || equals(c_token, "lw" )) { \
		struct value t; ++c_token; \
      lp.l_width = real(const_express(&t)); \
   } else lp.l_width = 1.0; \
   if ( (lp.pointflag = allow_point) != 0) { \
  	   if (almost_equals(c_token, "pointt$ype") || equals(c_token, "pt" )) { \
		   struct value t; ++c_token; \
         lp.p_type = (int) real(const_express(&t))-1; \
      } else lp.p_type = def_point; \
 	   if (almost_equals(c_token, "points$ize") || equals(c_token, "ps" )) { \
		   struct value t; ++c_token; \
         lp.p_size = real(const_express(&t)); \
      } else lp.p_size = pointsize; /* as in "set pointsize" */ \
   } else lp.p_size = pointsize; /* give it a value */ \
   LP_DUMP(lp); \
}
   

 
#endif /* not PLOT_H */
