/* GNUPLOT - term_api.h */

/*[
 * Copyright 1999, 2004   Thomas Williams, Colin Kelley
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
#ifdef OS2
# include "mousecmn.h"
#endif

/* Constants that are interpreted by terminal driver routines */

/* Default line type is LT_BLACK; reset to this after changing colors */
#define LT_AXIS       (-1)
#define LT_BLACK      (-2)		/* Base line type */
#define LT_SOLID      (-2)		/* Synonym for base line type */
#define LT_NODRAW     (-3)
#define LT_BACKGROUND (-4)
#define LT_UNDEFINED  (-5)
#define LT_COLORFROMCOLUMN  (-6)	/* Used by hidden3d code */
#define LT_DEFAULT    (-7)

/* Pre-defined dash types */
#define DASHTYPE_NODRAW (-4)
#define DASHTYPE_CUSTOM (-3)
#define DASHTYPE_AXIS   (-2)
#define DASHTYPE_SOLID  (-1)
/* more...? */

/* magic point type that indicates a character rather than a predefined symbol */
#define PT_CHARACTER  (-9)
/* magic point type that indicates true point type comes from a data column */
#define PT_VARIABLE   (-8)
/* magic point type that indicates we really want a circle drawn by do_arc() */
#define PT_CIRCLE     (-7)

/* Constant value passed to (term->text_angle)(ang) to generate vertical
 * text corresponding to old keyword "rotate", which produced the equivalent
 * of "rotate by 90 right-justified".
 */
#define TEXT_VERTICAL (-270)


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

typedef enum t_linecap {
    BUTT = 0,
    ROUNDED,
    SQUARE
} t_linecap;

/* custom dash pattern definition modeled after SVG terminal,
 * but string specifications like "--.. " are also allowed and stored */
#define DASHPATTERN_LENGTH 8

typedef struct t_dashtype {
	float pattern[DASHPATTERN_LENGTH];
	char dstring[8];
} t_dashtype;

#define DEFAULT_DASHPATTERN {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, \
                             {0,0,0,0,0,0,0,0} }

typedef struct lp_style_type {	/* contains all Line and Point properties */
    int     flags;		/* e.g. LP_SHOW_POINTS */
    int     l_type;
    int	    p_type;
    int     d_type;		/* Dashtype */
    int     p_interval;		/* Every Nth point in style LINESPOINTS */
    int     p_number;		/* specify number of points in style LINESPOINTS */
    double  l_width;
    double  p_size;
    char    p_char[8];		/* string holding UTF-8 char used if p_type = PT_CHARACTER */
    struct t_colorspec pm3d_color;
    t_dashtype custom_dash_pattern;	/* per-line, user defined dashtype */
    /* ... more to come ? */
} lp_style_type;

#define DEFAULT_P_CHAR {0,0,0,0,0,0,0,0}
#define DEFAULT_LP_STYLE_TYPE {0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN}

/* Bit definitions for lp_style_type.flags */
#define LP_SHOW_POINTS     (0x1) /* if not set, ignore the point properties of this line style */
#define LP_NOT_INITIALIZED (0x2) /* internal flag used in set.c:parse_label_options */
#define LP_EXPLICIT_COLOR  (0x4) /* set by lp_parse if the user provided a color spec */
#define LP_ERRORBAR_SET    (0x8) /* set by "set errorbars <lineprops> */

#define DEFAULT_COLOR_SEQUENCE { 0x9400d3, 0x009e73, 0x56b4e9, 0xe69f00, \
                                 0xf0e442, 0x0072b2, 0xe51e10, 0x000000 }
#define PODO_COLOR_SEQUENCE { 0x000000, 0xe69f00, 0x56b4e9, 0x009e73, \
                              0xf0e442, 0x0072b2, 0xd55e00, 0xcc79a7 }

#define DEFAULT_MONO_LINETYPES { \
	{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
	{0, LT_BLACK, 0, 1 /* dt 2 */, 0, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
	{0, LT_BLACK, 0, 2 /* dt 3 */, 0, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
	{0, LT_BLACK, 0, 3 /* dt 4 */, 0, 0, 1.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
	{0, LT_BLACK, 0, 0 /* dt 1 */, 0, 0, 2.0 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, \
	{0, LT_BLACK, 0, DASHTYPE_CUSTOM, 0, 0, 1.2 /*linewidth*/, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, {{16.,8.,2.,5.,2.,5.,2.,8.},{0,0,0,0,0,0,0,0}}} \
}

/* Note:  These are interpreted as bit flags, not ints */
typedef enum e_arrow_head {
	NOHEAD = 0,
	END_HEAD = 1,
	BACKHEAD = 2,
	BOTH_HEADS = 3,
	HEADS_ONLY = 4,
	SHAFT_ONLY = 8
} t_arrow_head;

extern const char *arrow_head_names[4];

typedef enum arrowheadfill {
	AS_NOFILL = 0,
	AS_EMPTY,
	AS_FILLED,
	AS_NOBORDER
} arrowheadfill;

typedef struct arrow_style_type {    /* contains all Arrow properties */
    int tag;                         /* -1 (local), AS_VARIABLE, or style index */
    int layer;	                     /* 0 = back, 1 = front */
    struct lp_style_type lp_properties;
    /* head options */
    t_arrow_head head;               /* arrow head choice */
    /* struct position headsize; */  /* x = length, y = angle [deg] */
    double head_length;              /* length of head, 0 = default */
    int head_lengthunit;             /* unit (x1, x2, screen, graph) */
    double head_angle;               /* front angle / deg */
    double head_backangle;           /* back angle / deg */
    arrowheadfill headfill;	     /* AS_FILLED etc */
    TBOOLEAN head_fixedsize;         /* Adapt the head size for short arrow shafts? */
    /* ... more to come ? */
} arrow_style_type;

/* Operations used by the terminal entry point term->layer(). */
typedef enum termlayer {
	TERM_LAYER_RESET,
	TERM_LAYER_BACKTEXT,
	TERM_LAYER_FRONTTEXT,
	TERM_LAYER_BEGIN_BORDER,
	TERM_LAYER_END_BORDER,
	TERM_LAYER_BEGIN_GRID,
	TERM_LAYER_END_GRID,
	TERM_LAYER_END_TEXT,
	TERM_LAYER_BEFORE_PLOT,
	TERM_LAYER_AFTER_PLOT,
	TERM_LAYER_KEYBOX,
	TERM_LAYER_BEGIN_KEYSAMPLE,
	TERM_LAYER_END_KEYSAMPLE,
	TERM_LAYER_RESET_PLOTNO,
	TERM_LAYER_BEFORE_ZOOM,
	TERM_LAYER_BEGIN_PM3D_MAP,
	TERM_LAYER_END_PM3D_MAP,
	TERM_LAYER_BEGIN_PM3D_FLUSH,
	TERM_LAYER_END_PM3D_FLUSH,
	TERM_LAYER_BEGIN_IMAGE,
	TERM_LAYER_END_IMAGE,
	TERM_LAYER_BEGIN_COLORBOX,
	TERM_LAYER_END_COLORBOX,
	TERM_LAYER_3DPLOT
} t_termlayer;

/* Options used by the terminal entry point term->waitforinput(). */
#define TERM_ONLY_CHECK_MOUSING	1
#define TERM_WAIT_FOR_FONTPROPS	2
#define TERM_EVENT_POLL_TIMEOUT 0	/* select() timeout in usec */

/* Options used by the terminal entry point term->hypertext(). */
#define TERM_HYPERTEXT_TOOLTIP 0
#define TERM_HYPERTEXT_TITLE   1
#define TERM_HYPERTEXT_FONT    2

typedef struct fill_style_type {
    int fillstyle;
    int filldensity;
    int fillpattern;
    t_colorspec border_color;
} fill_style_type;

/* Options used by the terminal entry point term->boxed_text() */
typedef enum t_textbox_options {
	TEXTBOX_INIT = 0,
	TEXTBOX_OUTLINE,
	TEXTBOX_BACKGROUNDFILL,
	TEXTBOX_MARGINS,
	TEXTBOX_FINISH,
	TEXTBOX_GREY
} t_textbox_options;

typedef enum t_fillstyle { FS_EMPTY, FS_SOLID, FS_PATTERN, FS_DEFAULT,
			   FS_TRANSPARENT_SOLID, FS_TRANSPARENT_PATTERN }
	     t_fillstyle;
#define FS_OPAQUE (FS_SOLID + (100<<4))

/* Color construction for an image, palette lookup or rgb components. */
typedef enum t_imagecolor { IC_PALETTE, IC_RGB, IC_RGBA }
	     t_imagecolor;

/* Operations possible with term->modify_plots() */
#define MODPLOTS_SET_VISIBLE         (1<<0)
#define MODPLOTS_SET_INVISIBLE       (1<<1)
#define MODPLOTS_INVERT_VISIBILITIES (MODPLOTS_SET_VISIBLE|MODPLOTS_SET_INVISIBLE)

/* Values for the flags field of TERMENTRY
 */
#define TERM_CAN_MULTIPLOT    (1<<0)	/* tested if stdout not redirected */
#define TERM_CANNOT_MULTIPLOT (1<<1)	/* tested if stdout is redirected  */
#define TERM_BINARY           (1<<2)	/* open output file with "b"       */
#define TERM_INIT_ON_REPLOT   (1<<3)	/* call term->init() on replot     */
#define TERM_IS_POSTSCRIPT    (1<<4)	/* post, next, pslatex, etc        */
#define TERM_ENHANCED_TEXT    (1<<5)	/* enhanced text mode is enabled   */
#define TERM_NO_OUTPUTFILE    (1<<6)	/* terminal doesn't write to a file */
#define TERM_CAN_CLIP         (1<<7)	/* terminal does its own clipping  */
#define TERM_CAN_DASH         (1<<8)	/* terminal supports dashed lines  */
#define TERM_ALPHA_CHANNEL    (1<<9)	/* alpha channel transparency      */
#define TERM_MONOCHROME      (1<<10)	/* term is running in mono mode    */
#define TERM_LINEWIDTH       (1<<11)	/* support for set term linewidth  */
#define TERM_FONTSCALE       (1<<12)	/* terminal supports fontscale     */
#define TERM_POINTSCALE      (1<<13)	/* terminal supports fontscale     */
#define TERM_IS_LATEX        (1<<14)	/* text uses TeX markup            */
#define TERM_EXTENDED_COLOR  (1<<15)	/* uses EXTENDED_COLOR_SPECS       */
#define TERM_NULL_SET_COLOR  (1<<16)	/* no support for RGB color        */
#define TERM_POLYGON_PIXELS  (1<<17)	/* filledpolygon rather than fillbox */

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
    const char *description;
    unsigned int xmax,ymax,v_char,h_char,v_tic,h_tic;

    void (*options)(void);
    void (*init)(void);
    void (*reset)(void);
    void (*text)(void);
    int (*scale)(double, double);
    void (*graphics)(void);
    void (*move)(unsigned int, unsigned int);
    void (*vector)(unsigned int, unsigned int);
    void (*linetype)(int);
    void (*put_text)(unsigned int, unsigned int, const char*);
    /* the following are optional. set term ensures they are not NULL */
    int (*text_angle)(int);
    int (*justify_text)(enum JUSTIFY);
    void (*point)(unsigned int, unsigned int, int);
    void (*arrow)(unsigned int, unsigned int, unsigned int, unsigned int, int headstyle);
    int (*set_font)(const char *font);
    void (*pointsize)(double); /* change pointsize */
    int flags;
    void (*suspend)(void); /* called after one plot of multiplot */
    void (*resume) (void); /* called before plots of multiplot */
    void (*fillbox)(int, unsigned int, unsigned int, unsigned int, unsigned int); /* clear in multiplot mode */
    void (*linewidth)(double linewidth);
#ifdef USE_MOUSE
    int (*waitforinput)(int);     /* used for mouse and hotkey input */
    void (*put_tmptext)(int, const char []);   /* draws temporary text; int determines where: 0=statusline, 1,2: at corners of zoom box, with \r separating text above and below the point */
    void (*set_ruler)(int, int);    /* set ruler location; x<0 switches ruler off */
    void (*set_cursor)(int, int, int);   /* set cursor style and corner of rubber band */
    void (*set_clipboard)(const char[]);  /* write text into cut&paste buffer (clipboard) */
#endif
    int (*make_palette)(t_sm_palette *palette);
    /* 1. if palette==NULL, then return nice/suitable
       maximal number of colours supported by this terminal.
       Returns 0 if it can make colours without palette (like
       postscript).
       2. if palette!=NULL, then allocate its own palette
       return value is undefined
       3. available: some negative values of max_colors for whatever
       can be useful
     */
    void (*previous_palette)(void);
    /* release the palette that the above routine allocated and get
       back the palette that was active before.
       Some terminals, like displays, may draw parts of the figure
       using their own palette. Those terminals that possess only
       one palette for the whole plot don't need this routine.
     */
    void (*set_color)(t_colorspec *);
    /* EAM November 2004 - revised to take a pointer to struct rgb_color,
       so that a palette gray value is not the only option for
       specifying color.
     */
    void (*filled_polygon)(int points, gpiPoint *corners);
    void (*image)(unsigned int, unsigned int, coordval *, gpiPoint *, t_imagecolor);

/* Enhanced text mode driver call-backs */
    void (*enhanced_open)(char * fontname, double fontsize,
		double base, TBOOLEAN widthflag, TBOOLEAN showflag,
		int overprint);
    void (*enhanced_flush)(void);
    void (*enhanced_writec)(int c);

/* Driver-specific synchronization or other layering commands.
 * Introduced as an alternative to the ugly sight of
 * driver-specific code strewn about in the core routines.
 * As of this point (July 2005) used only by pslatex.trm
 */
    void (*layer)(t_termlayer);

/* Begin/End path control.
 * Needed by PostScript-like devices in order to join the endpoints of
 * a polygon cleanly.
 */
    void (*path)(int p);

/* Scale factor for converting terminal coordinates to output
 * pixel coordinates.  Used to provide data for external mousing code.
 */
    double tscale;

/* Pass hypertext for inclusion in the output plot */
    void (*hypertext)(int type, const char *text);

    void (*boxed_text)(unsigned int, unsigned int, int);

    void (*modify_plots)(unsigned int operations, int plotno);

    void (*dashtype)(int type, t_dashtype *custom_dash_pattern);

} TERMENTRY;

# define termentry TERMENTRY

enum set_encoding_id {
   S_ENC_DEFAULT, S_ENC_ISO8859_1, S_ENC_ISO8859_2, S_ENC_ISO8859_9, S_ENC_ISO8859_15,
   S_ENC_CP437, S_ENC_CP850, S_ENC_CP852, S_ENC_CP950,
   S_ENC_CP1250, S_ENC_CP1251, S_ENC_CP1252, S_ENC_CP1254,
   S_ENC_KOI8_R, S_ENC_KOI8_U, S_ENC_SJIS,
   S_ENC_UTF8,
   S_ENC_INVALID
};

/* options handling */
enum { UNSET = -1, no = 0, yes = 1 };

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
extern arrowheadfill curr_arrow_headfilled;
extern TBOOLEAN curr_arrow_headfixedsize;

/* Recycle count for user-defined linetypes */
extern int linetype_recycle_count;
extern int mono_recycle_count;

/* Current 'output' file: name and open filehandle */
extern char *outstr;
extern FILE *gpoutfile;

/* Output file where postscript terminal output goes to.
   In particular:
	gppsfile == gpoutfile
		for 'set term': postscript, pstex
	gppsfile == PSLATEX_auxfile
		for 'set term': pslatex, cairolatex
	gppsfile == 0
		for all other terminals
*/
extern FILE *gppsfile;
extern char *PS_psdir;
extern char *PS_fontpath;	/* just a directory name */

extern TBOOLEAN monochrome;
extern TBOOLEAN multiplot;
extern int multiplot_count;

/* 'set encoding' support: index of current encoding ... */
extern enum set_encoding_id encoding;
/* ... in table of encoding names: */
extern const char *encoding_names[];
/* parsing table for encodings */
extern const struct gen_table set_encoding_tbl[];

/* mouse module needs this */
extern TBOOLEAN term_initialised;

/* The qt and wxt terminals cannot be used in the same session. */
/* Whichever one is used first to plot, this locks out the other. */
extern void *term_interlock;

/* Support for enhanced text mode. */
extern char  enhanced_text[];
extern char *enhanced_cur_text;
extern double enhanced_fontscale;
/* give array size to allow the use of sizeof */
extern char enhanced_escape_format[16];
extern TBOOLEAN ignore_enhanced_text;


/* Prototypes of functions exported by term.c */

void term_set_output(char *);
void term_initialise(void);
void term_start_plot(void);
void term_end_plot(void);
void term_start_multiplot(void);
void term_end_multiplot(void);
/* void term_suspend(void); */
void term_reset(void);
void term_apply_lp_properties(struct lp_style_type *lp);
void term_check_multiplot_okay(TBOOLEAN);
void init_monochrome(void);
struct termentry *change_term(const char *name, int length);

void write_multiline(int, int, char *, JUSTIFY, VERT_JUSTIFY, int, const char *);
int estimate_strlen(const char *length, double *estimated_fontheight);
char *estimate_plaintext(char *);
void list_terms(void);
char* get_terminals_names(void);
struct termentry *set_term(void);
void init_terminal(void);
void test_term(void);

/* Support for enhanced text mode. */
const char *enhanced_recursion(const char *p, TBOOLEAN brace,
                               char *fontname, double fontsize,
                               double base, TBOOLEAN widthflag,
                               TBOOLEAN showflag, int overprint);
void enh_err_check(const char *str);
/* note: c is char, but must be declared int due to K&R compatibility. */
void do_enh_writec(int c);
/* flag: don't use enhanced output methods --- for output of
 * filenames, which usually looks bad using subscripts */
void ignore_enhanced(TBOOLEAN flag);

/* Simple-minded test that point is with drawable area */
TBOOLEAN on_page(int x, int y);

/* Convert a fill style into a backwards compatible packed form */
int style_from_fill(struct fill_style_type *);

/* Terminal-independent routine to draw a circle or arc */
void do_arc( int cx, int cy, double radius,
             double arc_start, double arc_end,
	     int style, TBOOLEAN wedge);

#ifdef LINUXVGA
void LINUX_setup(void);
#endif

#ifdef OS2
int PM_pause(char *);
void PM_intc_cleanup(void);
# ifdef USE_MOUSE
void PM_update_menu_items(void);
void PM_set_gpPMmenu(struct t_gpPMmenu * gpPMmenu);
# endif
#endif

int load_dashtype(struct t_dashtype *dt, int tag);
void lp_use_properties(struct lp_style_type *lp, int tag);
void load_linetype(struct lp_style_type *lp, int tag);

/* Wrappers for term->path() */
void newpath(void);
void closepath(void);

/* Generic wrapper to check for mouse events or hotkeys during
 * non-interactive input (e.g. "load")
 */
void check_for_mouse_events(void);

/* shared routined to add backslash in front of reserved characters */
char *escape_reserved_chars(const char *str, const char *reserved);

#endif /* GNUPLOT_TERM_API_H */
