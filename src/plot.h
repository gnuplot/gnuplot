/*
 * $Id: plot.h,v 1.30 2000/05/02 18:01:03 lhecking Exp $
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

#include "syscfg.h"
#include "stdfn.h"
#include "gp_types.h"		/* FIXME HBB 20000416: only for testing */

#if 0				/* HBB 20000501: not *all* modules need it! */
#ifdef USE_MOUSE
# include "mouse.h"
#endif
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

/* minimum size of points[] in curve_points */
#define MIN_CRV_POINTS 100
/* minimum size of points[] in surface_points */
#define MIN_SRF_POINTS 1000

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

/* note that MAX_LINE_LEN, MAX_TOKENS and MAX_AT_LEN do no longer limit the
   size of the input. The values describe the steps in which the sizes are
   extended. */

#define MAX_LINE_LEN 1024	/* maximum number of chars allowed on line */
#define MAX_TOKENS 400
#define MAX_ID_LEN 50		/* max length of an identifier */


#define MAX_AT_LEN 150		/* max number of entries in action table */
#define NO_CARET (-1)

#define DATATYPE_ARRAY_SIZE 10

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

/* argument: char *fn */
#define VALID_FILENAME(fn) ((fn) != NULL && (*fn) != '\0')

#define END_OF_COMMAND (c_token >= num_tokens || equals(c_token,";"))
#define is_EOF(c) ((c) == 'e' || (c) == 'E')

#define is_jump(operator) ((operator) >=(int)JUMP && (operator) <(int)SF_START)

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

/* defines used for timeseries, seconds */
#define ZERO_YEAR	2000
#define JAN_FIRST_WDAY 6  /* 1st jan, 2000 is a Saturday (cal 1 2000 on unix) */
#define SEC_OFFS_SYS	946684800.0		/*  zero gnuplot (2000) - zero system (1970) */
#define YEAR_SEC	31557600.0	/* avg, incl. leap year */
#define MON_SEC		2629800.0	/* YEAR_SEC / 12 */
#define WEEK_SEC	604800.0
#define DAY_SEC		86400.0

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

#endif /* GNUPLOT_PLOT_H */
