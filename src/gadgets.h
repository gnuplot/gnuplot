/*
 * gadgets.h,v 1.1.3.1 2000/05/03 21:47:15 hbb Exp
 */

/* GNUPLOT - gadgets.h */

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

#ifndef GNUPLOT_GADGETS_H
# define GNUPLOT_GADGETS_H

#include "syscfg.h"

#include "term_api.h"

/* Types and variables concerning graphical plot elements that are not
 * *terminal-specific, are used by both* 2D and 3D plots, and are not
 * *assignable to any particular * axis. I.e. they belong to neither
 * *term_api, graphics, graph3d, nor * axis .h files.
 */

/* #if... / #include / #define collection: */

/* Default line type is LT_BLACK; reset to this after changing colors */
/* LT_NODRAW must == L_TYPE_NODRAW; the latter should probably go away */
#define LT_AXIS       (-1)
#define LT_BLACK      (-2)
#define LT_NODRAW     (-3)
#define LT_BACKGROUND (-4)
#define LT_UNDEFINED  (-5)

/* Constant value passed to (term->text_angle)(ang) to generate vertical 
 * text. Current implementation has ang equal to rotation in degrees. 
 */
#define TEXT_VERTICAL (90)

/* Type definitions */

/* Coordinate system specifications: x1/y1, x2/y2, graph-box relative
 * or screen relative coordinate systems */
typedef enum position_type {
    first_axes,
    second_axes,
    graph,
    screen
} position_type; 

/* A full 3D position, with all 3 coordinates of different axes,
 * possibly. Used for 'set label' and 'set arrow' positions: */
typedef struct position {
    enum position_type scalex,scaley,scalez;
    double x,y,z;
} t_position;

/* Generalized pm3d-compatible color specifier,
 * currently used only for setting text color  */
typedef struct t_colorspec {
    int type;			/* TC_<type> definitions below */
    int lt;			/* holds lt    if type==1 */
    double value;		/* holds value if type>1  */
} t_colorspec;
#define	TC_DEFAULT	0
#define	TC_LT		1
#define	TC_CB		2
#define	TC_FRAC		3
#define	TC_Z		4

/* Linked list of structures storing 'set label' information */
typedef struct text_label {
    struct text_label *next;	/* pointer to next label in linked list */
    int tag;			/* identifies the label */
    t_position place;
    enum JUSTIFY pos;
    int rotate;
    int layer;
    char *text;
    char *font;			/* Entry font added by DJL */
    struct t_colorspec textcolor;
    struct lp_style_type lp_properties;
    double hoffset;
    double voffset;
} text_label;

/* Datastructure for implementing 'set arrow' */
typedef struct arrow_def {
    struct arrow_def *next;	/* pointer to next arrow in linked list */
    int tag;			/* identifies the arrow */
    t_position start;
    t_position end;
    TBOOLEAN relative;		/* second coordinate is relative to first */
    struct arrow_style_type arrow_properties;
} arrow_def;

/* Datastructure implementing 'set linestyle' */
struct linestyle_def {
    struct linestyle_def *next;	/* pointer to next linestyle in linked list */
    int tag;			/* identifies the linestyle */
    struct lp_style_type lp_properties;
};

/* Datastructure implementing 'set linestyle' */
struct arrowstyle_def {
    struct arrowstyle_def *next;/* pointer to next arrowstyle in linked list */
    int tag;			/* identifies the arrowstyle */
    struct arrow_style_type arrow_properties;
};

/* The vertical positioning of the key box: (top, bottom, under) */
typedef enum en_key_vertical_position {
    TTOP,
    TBOTTOM,
    TUNDER
} t_key_vertical_position;

/* Horizontal positions of the key box: (left, right, outside right */
typedef enum en_key_horizontal_position {
    TLEFT,
    TRIGHT,
    TOUT
} t_key_horizontal_position;

/* Key sample to the left or the right of the plot title? */
typedef enum en_key_sample_positioning {
    JLEFT,
    JRIGHT
} t_key_sample_positioning;

typedef enum key_type {
    KEY_USER_PLACEMENT,
    KEY_AUTO_PLACEMENT
} t_key_flag;

/* The data describing an axis label, or the plot title */
typedef struct {
    char text[MAX_LINE_LEN+1];
    char font[MAX_LINE_LEN+1];
    double xoffset, yoffset;
    struct t_colorspec textcolor;
} label_struct;
#define EMPTY_LABELSTRUCT {"", "", 0.0, 0.0,{0,0,0.0}}

#ifdef PM3D
typedef struct {
    int opt_given; /* option given / not given (otherwise default) */
    int closeto;   /* from list FILLEDCURVES_CLOSED, ... */
    double at;	   /* value for FILLEDCURVES_AT... */
    double aty;	   /* the other value for FILLEDCURVES_ATXY */
} filledcurves_opts;
#define EMPTY_FILLEDCURVES_OPTS { 0, 0, 0.0, 0.0 }
#endif


/***********************************************************/
/* Variables defined by gadgets.c needed by other modules. */
/***********************************************************/



/* EAM Feb 2003 - Move all global variables related to key into a */
/* single structure. Eventually this will allow multiple keys.    */


typedef struct { 
    TBOOLEAN visible;		/* Do we show this key at all? */
    t_key_flag flag;		/* if so: where? */
    struct position user_pos;	/* if user specified position, this is it */
    t_key_vertical_position vpos; /* otherwise these guide auto-positioning */
    t_key_horizontal_position hpos;
    t_key_sample_positioning just;
    double swidth;		/* 'width' of the linestyle sample line in the key */
    double vert_factor;		/* user specified vertical spacing multiplier */
    double width_fix;		/* user specified additional (+/-) width of key titles */
    double height_fix;
    TBOOLEAN auto_titles;	/* auto title curves unless plotted 'with notitle' */
    TBOOLEAN reverse;		/* key back to front */ 
    TBOOLEAN enhanced;		/* enable/disable enhanced text of key titles */ 
    struct lp_style_type default_keybox_lp; /* linetype of box around key:  */
    struct lp_style_type box;		    /*    default and current state */
    char title[MAX_LINE_LEN+1];	/* title line for the key as a whole */
} legend_key;

extern legend_key keyT;

#ifdef PM3D
# define DEFAULT_KEYBOX_LP { 0, -3, 0, 1.0, 1.0, 0 }	/* -3 = no linetype */
#else
# define DEFAULT_KEYBOX_LP { 0, -3, 0, 1.0, 1.0 }	/* -3 = no linetype */
#endif

#define DEFAULT_KEY_POSITION { graph, graph, graph, 0.9, 0.9, 0. }

#define DEFAULT_KEY_PROPS \
		{ TRUE, \
		KEY_AUTO_PLACEMENT, \
		DEFAULT_KEY_POSITION, \
		TTOP, TRIGHT, JRIGHT,  \
		4.0, 1.0, 0.0, 0.0, \
		TRUE, FALSE, TRUE,  \
		DEFAULT_KEYBOX_LP, \
		DEFAULT_KEYBOX_LP, \
		"" }

/* bounding box position, in terminal coordinates */
extern int xleft, xright, ybot, ytop;

extern float xsize;		/* x scale factor for size */
extern float ysize;		/* y scale factor for size */
extern float zsize;		/* z scale factor for size */
extern float xoffset;		/* x origin setting */
extern float yoffset;		/* y origin setting */
extern float aspect_ratio;	/* 1.0 for square */

/* plot border autosizing overrides, in characters (-1: autosize) */
extern int lmargin, bmargin,rmargin,tmargin;

extern struct arrow_def *first_arrow;

extern struct text_label *first_label;

extern struct linestyle_def *first_linestyle;

extern struct arrowstyle_def *first_arrowstyle;

extern label_struct title;

extern label_struct timelabel;
#ifndef DEFAULT_TIMESTAMP_FORMAT
/* asctime() format */
# define DEFAULT_TIMESTAMP_FORMAT "%a %b %d %H:%M:%S %Y"
#endif
extern int timelabel_rotate;
extern int timelabel_bottom;

extern TBOOLEAN	polar;

#define ZERO 1e-8		/* default for 'zero' set option */
extern double zero;		/* zero threshold, not 0! */

extern double pointsize;

#define SOUTH		1 /* 0th bit */
#define WEST		2 /* 1th bit */
#define NORTH		4 /* 2th bit */
#define EAST		8 /* 3th bit */
#define border_east	(draw_border & EAST)
#define border_west	(draw_border & WEST)
#define border_south	(draw_border & SOUTH)
#define border_north	(draw_border & NORTH)
extern int draw_border;

extern struct lp_style_type border_lp;
extern const struct lp_style_type default_border_lp;

extern TBOOLEAN	clip_lines1;
extern TBOOLEAN	clip_lines2;
extern TBOOLEAN	clip_points;

#define SAMPLES 100		/* default number of samples for a plot */
extern int samples_1;
extern int samples_2;

extern double ang2rad; /* 1 or pi/180 */

extern enum PLOT_STYLE data_style;
extern enum PLOT_STYLE func_style;

extern TBOOLEAN parametric;

/* FIXME HBB 20010806: not modified anywhere, used only by
 * draw_clip_line() --- it's essentially a compile-time only
 * option. */
extern TBOOLEAN suppressMove;

/* Functions exported by gadgets.c */

/* moved here from util3d: */
void draw_clip_line __PROTO((int, int, int, int));
int clip_line __PROTO((int *, int *, int *, int *));
int clip_point __PROTO((unsigned int, unsigned int));
void clip_put_text __PROTO((unsigned int, unsigned int, char *));
void clip_put_text_just __PROTO((unsigned int, unsigned int, char *, JUSTIFY, VERT_JUSTIFY, char *));

/* moved here from graph3d: */
void clip_move __PROTO((unsigned int x, unsigned int y));
void clip_vector __PROTO((unsigned int x, unsigned int y));

/* Common routines for setting text color from t_colorspec */
void apply_textcolor __PROTO((const struct t_colorspec *tc, const struct termentry *t));
void reset_textcolor __PROTO((const struct t_colorspec *tc, const struct termentry *t));

extern fill_style_type default_fillstyle;

#ifdef PM3D
/* filledcurves style options set by 'set style [data|func] filledcurves opts' */
extern filledcurves_opts filledcurves_opts_data;
extern filledcurves_opts filledcurves_opts_func;
#endif

void default_arrow_style __PROTO((struct arrow_style_type *arrow));

#endif /* GNUPLOT_GADGETS_H */
