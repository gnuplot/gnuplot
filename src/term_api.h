/*
 * $Id: term_api.h,v 1.23 2003/12/15 07:52:15 mikulik Exp $
 */

/* GNUPLOT - term_api.h */

/*[
 * Copyright 1999   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_TERM_API_H
# define GNUPLOT_TERM_API_H

/* #if... / #include / #define collection: */

#include "syscfg.h"
#include "stdfn.h"
#include "gp_types.h"

#include "color.h"
#include "tables.h"

/* Type definitions */

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
    JUST_BOT
} VERT_JUSTIFY;


typedef struct lp_style_type {	/* contains all Line and Point properties */
    int     pointflag;		/* 0 if points not used, otherwise 1 */
    int     l_type;
    int	    p_type;
    double  l_width;
    double  p_size;
#ifdef PM3D
    TBOOLEAN use_palette;
#endif
    /* ... more to come ? */
} lp_style_type;


typedef struct arrow_style_type {    /* contains all Arrow properties */
    int layer;	                     /* 0 = back, 1 = front */
    struct lp_style_type lp_properties;
    /* head options */
    int head;		/* arrow head: 0=no, 1=at end, 2=at start and end */
    /* struct position headsize; */	/* x = length, y = angle [deg] */
    double head_length;              /* length of head, 0 = default */
    int head_lengthunit;             /* unit (x1, x2, screen, graph) */
    double head_angle;               /* front angle / deg */
    double head_backangle;           /* back angle / deg */
    unsigned int head_filled;        /* filled heads: 0=not, 1=empty, 2=filled */
    /* ... more to come ? */
} arrow_style_type;


#define L_TYPE_NODRAW -3	/* use if line is not to be drawn */

#ifdef PM3D
# define DEFAULT_LP_STYLE_TYPE {0, 0, 0, 1.0, 1.0, FALSE}
#else
# define DEFAULT_LP_STYLE_TYPE {0, 0, 0, 1.0, 1.0}
#endif

/* EAM Sep 2002 - define fillstyle structure whether or not USE_ULIG_FILLEDBOXES */
typedef struct fill_style_type {
    int fillstyle;
    int filldensity;
    int fillpattern;
    int border_linetype;
} fill_style_type;

#ifdef USE_ULIG_FILLEDBOXES
typedef enum t_fillstyle { FS_EMPTY, FS_SOLID, FS_PATTERN }
	     t_fillstyle;
#else
typedef enum t_fillstyle { FS_EMPTY }
	     t_fillstyle;
#endif

/* values for the optional flags field - choose sensible defaults
 * these aren't really very sensible names - multiplot attributes
 * depend on whether stdout is redirected or not. Remember that
 * the default is 0. Thus most drivers can do multiplot only if
 * the output is redirected
 */
#define TERM_CAN_MULTIPLOT    1  /* tested if stdout not redirected */
#define TERM_CANNOT_MULTIPLOT 2  /* tested if stdout is redirected  */
#define TERM_BINARY           4  /* open output file with "b"       */
#define TERM_INIT_ON_REPLOT   8  /* call term->init() on replot     */
#define TERM_IS_POSTSCRIPT   16  /* post, next, pslatex, etc        */

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

/* Enhanced text mode driver call-backs */
    void (*enhanced_open) __PROTO((char * fontname, double fontsize,
		double base, TBOOLEAN widthflag, TBOOLEAN showflag,
		int overprint));
    void (*enhanced_flush) __PROTO((void));
    void (*enhanced_writec) __PROTO((char c));

} TERMENTRY;

#ifdef WIN16
# define termentry TERMENTRY far
#else
# define termentry TERMENTRY
#endif

enum set_encoding_id {
   S_ENC_DEFAULT, S_ENC_ISO8859_1, S_ENC_ISO8859_2, S_ENC_ISO8859_15,
   S_ENC_CP437, S_ENC_CP850, S_ENC_CP852, S_ENC_KOI8_R, S_ENC_INVALID
};

/* HBB 20020225: this stuff used to be in a separate header, ipc.h,
 * but I strongly disliked the way that was done */

/*
 * There are the following types of interprocess communication from
 * (gnupmdrv, gnuplot_x11) => gnuplot:
 *	OS2_IPC  ... the OS/2 shared memory + event semaphores approach
 *	PIPE_IPC ... communication by using bidirectional pipe
 */


/*
 * OS2_IPC: gnuplot's terminals communicate with it by shared memory + event
 * semaphores => the code in gpexecute.c is used, and nothing more from here.
 */

#ifdef PIPE_IPC

enum { IPC_BACK_UNUSABLE = -2, IPC_BACK_CLOSED = -1 };

/*
 * Currently only used for PIPE_IPC, but in principle
 * every term could use this file descriptor to write back
 * commands to gnuplot.  Note, that terminals using this fd
 * should set it to a negative value when closing. (joze)
 */
/* HBB 20020225: currently not used anywhere outside term.c --> make
 * it static */
/* extern int ipc_back_fd; */
extern int isatty_state;

# endif /* PIPE_IPC */


/* Variables of term.c needed by other modules: */

/* the terminal info structure, being the heart of the whole module */
extern struct termentry *term;
/* Options string of the currently used terminal driver */
extern char term_options[];
/* access head length + angle without changing API */
extern int curr_arrow_headlength;
/* angle in degrees */
extern double curr_arrow_headangle;
extern double curr_arrow_headbackangle;
/* arrow head filled or not */
extern TBOOLEAN curr_arrow_headfilled;

/* Current 'output' file: name and open filehandle */
extern char *outstr;
extern FILE *gpoutfile;

#ifdef PM3D
/* Output file where the PostScript output goes to.
   In particular:
	postscript_gpoutfile == gpoutfile
		for 'set term': postscript, pstex
	postscript_gpoutfile == PSLATEX_auxfile
		for 'set term': pslatex
	postscript_gpoutfile == 0
		for all other terminals
   It is non-zero for for the family of postscript terminals, thus making 
   this a unique check for postscript output (pm3d has some code optimized
   for PS, for instance).
*/
extern FILE *postscript_gpoutfile;
#endif

extern TBOOLEAN multiplot;

/* 'set encoding' support: index of current encoding ... */
extern enum set_encoding_id encoding;
/* ... in table of encoding names: */
extern const char *encoding_names[];
/* parsing table for encodings */
extern const struct gen_table set_encoding_tbl[];

/* mouse module needs this */
extern TBOOLEAN term_initialised;

#ifdef OS2
extern int mouseGnupmdrv;
extern FILE *PM_pipe;
#endif 


/* Prototypes of functions exported by term.c */

void term_set_output __PROTO((char *));
void term_init __PROTO((void));
void term_start_plot __PROTO((void));
void term_end_plot __PROTO((void));
void term_start_multiplot __PROTO((void));
void term_end_multiplot __PROTO((void));
/* void term_suspend __PROTO((void)); */
void term_reset __PROTO((void));
void term_apply_lp_properties __PROTO((struct lp_style_type *lp));
void term_check_multiplot_okay __PROTO((TBOOLEAN));

void write_multiline __PROTO((unsigned int, unsigned int, char *, JUSTIFY, VERT_JUSTIFY, int, const char *));
GP_INLINE int term_count __PROTO((void));
void list_terms __PROTO((void));
struct termentry *set_term __PROTO((int));
struct termentry *change_term __PROTO((const char *name, int length));
void init_terminal __PROTO((void));
void test_term __PROTO((void));

/* flag: don't use enhanced output methods --- for output of
 * filenames, which usually looks bad using subscripts */
extern TBOOLEAN ignore_enhanced_text;

#ifdef LINUXVGA
void LINUX_setup __PROTO((void));
#endif

#ifdef VMS
void vms_reset();
#endif

#ifdef OS2
int PM_pause __PROTO((char *)); /* term/pm.trm */
#endif

/* in set.c (used in pm3d.c) */
void lp_use_properties __PROTO((struct lp_style_type *lp, int tag, int pointflag));

#endif /* GNUPLOT_TERM_API_H */
