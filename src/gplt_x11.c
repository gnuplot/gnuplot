#ifndef lint
static char *RCSid() { return RCSid("$Id: gplt_x11.c,v 1.93 2004/04/13 17:23:53 broeker Exp $"); }
#endif

#define X11_POLYLINE 1
#define MOUSE_ALL_WINDOWS 1

/* GNUPLOT - gplt_x11.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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


/* lph changes:
 * (a) make EXPORT_SELECTION the default and specify NOEXPORT to undefine
 * (b) append X11 terminal number to resource name
 * (c) change cursor for active terminal
 */

/*-----------------------------------------------------------------------------
 *   gnuplot_x11 - X11 outboard terminal driver for gnuplot 3.3
 *
 *   Requires installation of companion inboard x11 driver in gnuplot/term.c
 *
 *   Acknowledgements: 
 *      Chris Peterson (MIT)
 *      Dana Chee (Bellcore) 
 *      Arthur Smith (Cornell)
 *      Hendri Hondorp (University of Twente, The Netherlands)
 *      Bill Kucharski (Solbourne)
 *      Charlie Kline (University of Illinois)
 *      Yehavi Bourvine (Hebrew University of Jerusalem, Israel)
 *      Russell Lang (Monash University, Australia)
 *      O'Reilly & Associates: X Window System - Volumes 1 & 2
 *
 *   This code is provided as is and with no warranties of any kind.
 *
 * drd: change to allow multiple windows to be maintained independently
 *       
 *---------------------------------------------------------------------------*/

/* drd : export the graph via ICCCM primary selection. well... not quite
 * ICCCM since we dont support full list of targets, but this
 * is a start.  define EXPORT_SELECTION if you want this feature
 */

/*lph: add a "feature" to undefine EXPORT_SELECTION
   The following makes EXPORT_SELECTION the default and 
   defining NOEXPORT over-rides the default
 */

/* Petr Mikulik and Johannes Zellner: added mouse support (October 1999)
 * Implementation and functionality is based on os2/gclient.c; see mousing.c
 * Pieter-Tjerk de Boer <ptdeboer@cs.utwente.nl>: merged two versions
 * of mouse patches. (November 1999) (See also mouse.[ch]).
 */

/* X11 support for Petr Mikulik's pm3d 
 * by Johannes Zellner <johannes@zellner.org>
 * (November 1999 - January 2000, Oct. 2000)
 */

/* Polyline support May 2003 
 * Ethan Merritt <merritt@u.washington.edu>
 */

/* Dynamically created windows July 2003
 * Across-pipe title text and close command October 2003
 * Dan Sebald <daniel.sebald@ieee.org>
 */

#include "syscfg.h"
#include "stdfn.h"
#include "gp_types.h"
#include "term_api.h"

#ifdef EXPORT_SELECTION
# undef EXPORT_SELECTION
#endif /* EXPORT SELECTION */
#ifndef NOEXPORT
# define EXPORT_SELECTION XA_PRIMARY
#endif /* NOEXPORT */


#if !(defined(VMS) || defined(CRIPPLED_SELECT))
# define DEFAULT_X11
#endif

#if defined(VMS) && defined(CRIPPLED_SELECT)
Error. Incompatible options.
#endif

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef PM3D
# include <math.h>		/* pow() */
# include "getcolor.h"
#endif

#ifdef USE_MOUSE
# include <X11/cursorfont.h>
#else
# define XC_crosshair 34
#endif

#include <signal.h>

#ifdef HAVE_SYS_BSDTYPES_H
# include <sys/bsdtypes.h>
#endif

#if defined(HAVE_SYS_SELECT_H) && !defined(VMS)
# include <sys/select.h>
#endif

#ifndef FD_SET
# define FD_SET(n, p)    ((p)->fds_bits[0] |= (1 << ((n) % 32)))
# define FD_CLR(n, p)    ((p)->fds_bits[0] &= ~(1 << ((n) % 32)))
# define FD_ISSET(n, p)  ((p)->fds_bits[0] & (1 << ((n) % 32)))
# define FD_ZERO(p)      memset((char *)(p),'\0',sizeof(*(p)))
#endif /* not FD_SET */

#if defined(HAVE_SYS_SYSTEMINFO_H) && defined(HAVE_SYSINFO)
# include <sys/systeminfo.h>
# define SYSINFO_METHOD "sysinfo"
# define GP_SYSTEMINFO(host) sysinfo (SI_HOSTNAME, (host), MAXHOSTNAMELEN)
#else
# define SYSINFO_METHOD "gethostname"
# define GP_SYSTEMINFO(host) gethostname ((host), MAXHOSTNAMELEN)
#endif /* HAVE_SYS_SYSTEMINFO_H && HAVE_SYSINFO */

#ifdef USE_MOUSE
# ifdef OS2_IPC
#  define INCL_DOSPROCESS
#  define INCL_DOSSEMAPHORES
#  include <os2.h>
#  include "os2/dialogs.h"
# endif
# include "gpexecute.h"
# include "mouse.h"
# include <unistd.h>
# include <fcntl.h>
# include <errno.h>
static unsigned long gnuplotXID = 0; /* WINDOWID of gnuplot */

#ifdef MOUSE_ALL_WINDOWS
# include "axis.h" /* Just to pick up FIRST_X_AXIS enums */
typedef struct axis_scale_t {
    int term_lower;
    double term_scale;
    double min;
    double logbase;
} axis_scale_t;
#endif

#endif /* USE_MOUSE */

#ifdef __EMX__
/* for gethostname ... */
# include <netdb.h>
/* for __XOS2RedirRoot */
#include <X11/Xlibint.h>
#endif


#ifdef VMS
# ifdef __DECC
#  include <starlet.h>
# endif
# define EXIT(status) sys$delprc(0,0)	/* VMS does not drop itself */
#else /* !VMS */
# ifdef PIPE_IPC
#  define EXIT(status)                         \
    do {                                       \
	gp_exec_event(GE_pending, 0, 0, 0, 0); \
	close(1);                              \
	close(0);                              \
	exit(status);                          \
    } while (0)
# else
#  define EXIT(status) exit(status)
# endif	/* PIPE_IPC */
#endif /* !VMS */

#ifdef OSK
# define EINTR	E_ILLFNC
#endif


#define Ncolors 13

typedef struct cmap_t {
    Colormap colormap;
    unsigned long colors[Ncolors];	/* line colors */
#ifdef             PM3D
    unsigned long xorpixel;	/* line colors */
    int total;
    int allocated;
    unsigned long *pixels;	/* pm3d colors */
#endif
} cmap_t;

/* always allocate a default colormap (done in preset()) */
static cmap_t cmap;

/* information about one window/plot */
typedef struct plot_struct {
    Window window;
    Pixmap pixmap;
    unsigned int posn_flags;
    int x, y;
    unsigned int width, height;	/* window size */
    unsigned int gheight;	/* height of the part of the window that
				 * contains the graph (i.e., excluding the
				 * status line at the bottom if mouse is
				 * enabled) */
    unsigned int px, py;
    int ncommands, max_commands;
    char **commands;
    char *titlestring;
#ifdef USE_MOUSE
    int button;			/* buttons which are currently pressed */
    char str[0xff];		/* last displayed string */
    Time time;			/* time of last button press event */
#endif
    int lwidth;			/* this and the following 6 lines declare */
    int type;			/* variables used during drawing in exec_cmd() */
    int user_width;
    enum JUSTIFY jmode;
    int angle;			/* 0 = horizontal (default), 1 = vertical */
    int lt;
#ifdef USE_MOUSE
    TBOOLEAN mouse_on;		/* is mouse bar on? */
    TBOOLEAN ruler_on;		/* is ruler on? */
    int ruler_x, ruler_y;	/* coordinates of ruler */
    TBOOLEAN zoombox_on;	/* is zoombox on? */
    int zoombox_x1, zoombox_y1, zoombox_x2, zoombox_y2;	/* coordinates of zoombox as last drawn */
    char zoombox_str1a[64], zoombox_str1b[64], zoombox_str2a[64], zoombox_str2b[64];	/* strings to be drawn at corners of zoombox ; 1/2 indicate corner; a/b indicate above/below */
    TBOOLEAN resizing;		/* TRUE while waiting for an acknowledgement of resize */
#endif
    /* points to the cmap which is currently used for drawing.
     * This is always the default colormap, if not in pm3d.
     */
    cmap_t *cmap;
#ifdef PM3D
    TBOOLEAN release_cmap;
#endif
#if defined(USE_MOUSE) && defined(MOUSE_ALL_WINDOWS)
    /* This array holds per-axis scaling information sufficient to reconstruct
     * plot coordinates of a mouse click.  It is a snapshot of the contents of
     * gnuplot's axis_array structure at the time the plot was drawn.
     */
    int axis_mask;		/* Bits set to show which axes are active */
    axis_scale_t axis_scale[2*SECOND_AXES];
#endif
    /* Last text position  - used by enhanced text mode */
    int xLast, yLast;
    /* Saved text position  - used by enhanced text mode */
    int xSave, ySave;
    
    struct plot_struct *prev_plot;  /* Linked list pointers and number */
    struct plot_struct *next_plot;
    int plot_number;
} plot_struct;

static plot_struct *Add_Plot_To_Linked_List __PROTO((int));
static void Remove_Plot_From_Linked_List __PROTO((Window));
static plot_struct *Find_Plot_In_Linked_List_By_Number __PROTO((int));
static plot_struct *Find_Plot_In_Linked_List_By_Window __PROTO((Window));

static struct plot_struct *current_plot = NULL;
static struct plot_struct *list_start = NULL;

/* information about window/plot to be removed */
typedef struct plot_remove_struct {
    Window plot_window_to_remove;
    struct plot_remove_struct *next_remove;
    int processed;
} plot_remove_struct;

static void Add_Plot_To_Remove_FIFO_Queue __PROTO((Window));
static void Process_Remove_FIFO_Queue __PROTO((void));

static struct plot_remove_struct *remove_fifo_queue_start = NULL;
static int process_remove_fifo_queue = 0;

/* These might work better as fuctions, but defines will do for now. */
#define ERROR_NOTICE(str)         "\nGNUPLOT (gplt_x11):  " str
#define ERROR_NOTICE_NEWLINE(str) "\n                     " str

#ifdef USE_MOUSE
enum { NOT_AVAILABLE = -1 };

#define SEL_LEN 0xff
static char selection[SEL_LEN] = "";

#endif /* USE_MOUSE */

#ifdef USE_MOUSE
# define GRAPH_HEIGHT(plot)  ((plot)->gheight)
# define PIXMAP_HEIGHT(plot)  ((plot)->gheight + vchar)
  /* note: PIXMAP_HEIGHT is the height of the plot including the status line, 
     even if the latter is not enabled right now */
#else
# define GRAPH_HEIGHT(plot)  ((plot)->height)
# define PIXMAP_HEIGHT(plot)  ((plot)->height)
#endif

#ifdef PM3D
static void GetGCpm3d __PROTO((plot_struct *, GC *));
static void CmapClear __PROTO((cmap_t *));
static void RecolorWindow __PROTO((plot_struct *));
static void FreeColors __PROTO((plot_struct *));
static void ReleaseColormap __PROTO((plot_struct *));
static unsigned long *ReallocColors __PROTO((plot_struct *, int));
static void PaletteMake __PROTO((plot_struct *, t_sm_palette *));
static void PaletteSetColor __PROTO((plot_struct *, double));
static int GetVisual __PROTO((int, Visual **, int *));
static void scan_palette_from_buf __PROTO((plot_struct *));
#endif

static void store_command __PROTO((char *, plot_struct *));
static void prepare_plot __PROTO((plot_struct *, int));
static void delete_plot __PROTO((plot_struct *));

static int record __PROTO((void));
static void process_event __PROTO((XEvent *));	/* from Xserver */

static void mainloop __PROTO((void));

static void display __PROTO((plot_struct *));
static void UpdateWindow __PROTO((plot_struct *));
#ifdef USE_MOUSE
static int ErrorHandler __PROTO((Display *, XErrorEvent *));
static void DrawRuler __PROTO((plot_struct *));
static void EventuallyDrawMouseAddOns __PROTO((plot_struct *));
static void DrawBox __PROTO((plot_struct *));
static void AnnotatePoint __PROTO((plot_struct *, int, int, const char[], const char[]));
static long int SetTime __PROTO((plot_struct *, Time));
static unsigned long AllocateXorPixel __PROTO((cmap_t *));
static void GetGCXor __PROTO((plot_struct *, GC *));
static void GetGCXorDashed __PROTO((plot_struct *, GC *));
#if 0
static void GetGCBlackAndWhite __PROTO((plot_struct *, GC *, Pixmap, int));
static int SplitAt __PROTO((char **, int, char *, char));
static void xfree __PROTO((void *));
#endif
static void EraseCoords __PROTO((plot_struct *));
static void DrawCoords __PROTO((plot_struct *, const char *));
static void DisplayCoords __PROTO((plot_struct *, const char *));
#if 0
static int is_control __PROTO((KeySym));
static int is_meta __PROTO((KeySym));
static int is_shift __PROTO((KeySym));
#endif
static char* __PROTO((getMultiTabConsoleSwitchCommand(unsigned long *)));
#endif

static void DrawRotated __PROTO((plot_struct *, Display *, GC, 
				 int, int, const char *, int));
static int DrawRotatedErrorHandler __PROTO((Display *, XErrorEvent *));
static void exec_cmd __PROTO((plot_struct *, char *));

static void reset_cursor __PROTO((void));

static void preset __PROTO((int, char **));
static char *pr_GetR __PROTO((XrmDatabase, char *));
static void pr_color __PROTO((cmap_t *));
static void pr_dashes __PROTO((void));
static void pr_font __PROTO((char *));
static void pr_geometry __PROTO((void));
static void pr_pointsize __PROTO((void));
static void pr_width __PROTO((void));
static void pr_window __PROTO((plot_struct *));
static void ProcessEvents __PROTO((Window));
static void pr_raise __PROTO((void));
static void pr_persist __PROTO((void));
static void pr_feedback __PROTO((void));

#ifdef EXPORT_SELECTION
static void export_graph __PROTO((plot_struct *));
static void handle_selection_event __PROTO((XEvent *));
#endif

#if defined(USE_MOUSE) && defined(MOUSE_ALL_WINDOWS)
static void mouse_to_coords __PROTO((plot_struct *, XEvent *,
			double *, double *, double *, double *));
static double mouse_to_axis __PROTO((int, axis_scale_t *));
#endif

#define FallbackFont "fixed"
enum set_encoding_id encoding = S_ENC_DEFAULT; /* EAM - mirrored from core code by 'QE' */
static char default_font[64] = { '\0' };

#define Nwidths 10
static unsigned int widths[Nwidths] = { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#define Ndashes 10
static char dashes[Ndashes][5];

#ifdef PM3D
/* these must match the definitions in x11.trm */
/* FIXME HBB 20010919: don't say so, *force* it!  Include a common
 * header file into both x11.trm and this file. */
#define X11_GR_MAKE_PALETTE     'p'
#define X11_GR_RELEASE_PALETTE  'e'
#define X11_GR_SET_COLOR        'c'
#define X11_GR_FILLED_POLYGON   'f'
t_sm_palette sm_palette = {
    -1,				/* colorFormulae */
    SMPAL_COLOR_MODE_NONE,	/* colorMode */
    0, 0, 0,			/* formula[RGB] */
    0,				/* positive */
    0,				/* use_maxcolors */
    -1,				/* colors */
    (rgb_color *) 0,            /* color */
    0,				/* ps_allcF */
    0,                          /* gradient_num */
    (gradient_struct *) 0       /* gradient */
    /* Afunc, Bfunc and Cfunc can't be initialised here */
};
static GC gc_pm3d = (GC) 0;
static int have_pm3d = 1;
static int num_colormaps = 0;
static unsigned int maximal_possible_colors = 0x100;
static unsigned int minimal_possible_colors;

/* the following visual names must match the
 * definitions in X.h in this order ! I hope
 * this is standard (joze) */
static char *visual_name[] = {
    "StaticGray",
    "GrayScale",
    "StaticColor",
    "PseudoColor",
    "TrueColor",
    "DirectColor",
    (char *) 0
};
#endif /* PM3D */

static Display *dpy;
static int scr;
static Window root;
static Visual *vis = (Visual *) 0;
static GC gc = (GC) 0;
/* current_gc points to either the `line' GC `gc' or
 * to the pm3d smooth palette GC `gc_pm3d'. If pm3d
 * is disabled at compile time, current_gc always
 * points to the `line' GC `gc'. Thus the purpose of
 * current_gc is to switch between the `line' and the
 * pm3d GC's. */
static GC *current_gc = (GC *) 0;
static GC gc_xor = (GC) 0;
static GC gc_xor_dashed = (GC) 0;
static XFontStruct *font;
/* must match the definition in term/x11.trm: */
/* FIXME HBB 20020225: that really should be ensured by sharing a common
 * header file between this file and term/x11.trm! */
enum { UNSET = -1, no = 0, yes = 1 };
static int do_raise = yes, persist = no;
static int feedback = yes;
static Cursor cursor;
static Cursor cursor_default;
#ifdef USE_MOUSE
static Cursor cursor_exchange;
static Cursor cursor_sizing;
static Cursor cursor_zooming;
#ifndef TITLE_BAR_DRAWING_MSG
static Cursor cursor_waiting;
static Cursor cursor_save;
static int button_pressed = 0;
#endif
#endif

static int windows_open = 0;

static int gX = 100, gY = 100;
static unsigned int gW = 640, gH = 450;
static unsigned int gFlags = PSize;

static unsigned int BorderWidth = 2;
static unsigned int dep;		/* depth */

static Bool Mono = 0, Gray = 0, Rv = 0, Clear = 0;
static char Name[64] = "gnuplot";
static char Class[64] = "Gnuplot";

static int cx = 0, cy = 0;

/* Font characteristics held locally but sent back via pipe to x11.trm */
static int vchar,hchar;

/* Speficy negative values as indicator of uninitialized state */
static double xscale = -1.;
static double yscale = -1.;
double pointsize = -1.;
/* Avoid a crash upon using uninitialized variables from
   above and avoid unnecessary calls to display().
   Probably this is not the best fix ... */
#define Call_display(plot) if (xscale<0.) display(plot);
#define X(x) (int) ((x) * xscale)
#define Y(y) (int) ((4095-(y)) * yscale)
#define RevX(x) (((x)+0.5)/xscale)
#define RevY(y) (4095-((y)+0.5)/yscale)
/* note: the 0.5 term in RevX(x) and RevY(y) compensates for the round-off in X(x) and Y(y) */

#define Nbuf 1024
static char buf[Nbuf];
static int buffered_input_available = 0;

static FILE *X11_ipc;

/* when using an ICCCM-compliant window manager, we can ask it
 * to send us an event when user chooses 'close window'. We do this
 * by setting WM_DELETE_WINDOW atom in property WM_PROTOCOLS
 */

static Atom WM_PROTOCOLS, WM_DELETE_WINDOW;

static XPoint Diamond[5], Triangle[4];
static XSegment Plus[2], Cross[2], Star[4];

#if USE_ULIG_FILLEDBOXES
/* pixmaps used for filled boxes (ULIG) */
/* FIXME EAM - These data structures are a duplicate of the ones in bitmap.c */

/* halftone stipples for solid fillstyle */
#define stipple_halftone_width 8
#define stipple_halftone_height 8
#define stipple_halftone_num 5
static const char stipple_halftone_bits[stipple_halftone_num][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	/* no fill */
    { 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44 },	/* 25% pattern */
    { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa },	/* 50% pattern */
    { 0x77, 0xdd, 0x77, 0xdd, 0x77, 0xdd, 0x77, 0xdd },	/* 75% pattern */
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }	/* solid pattern */
};

/* pattern stipples for pattern fillstyle */
#define stipple_pattern_width 8
#define stipple_pattern_height 8
#define stipple_pattern_num 8
static const char stipple_pattern_bits[stipple_pattern_num][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } /* no fill */
   ,{ 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x41 } /* cross-hatch      (1) */
   ,{ 0x88, 0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55 } /* double crosshatch(2) */
   ,{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } /* solid fill       (3) */
   ,{ 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 } /* diagonal stripes (4) */
   ,{ 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 } /* diagonal stripes (5) */
   ,{ 0x11, 0x11, 0x22, 0x22, 0x44, 0x44, 0x88, 0x88 } /* diagonal stripes (6) */
   ,{ 0x88, 0x88, 0x44, 0x44, 0x22, 0x22, 0x11, 0x11 } /* diagonal stripes (7) */
#if (0)
   ,{ 0x03, 0x0C, 0x30, 0xC0, 0x03, 0x0C, 0x30, 0xC0 } /* diagonal stripes (8) */
   ,{ 0xC0, 0x30, 0x0C, 0x03, 0xC0, 0x30, 0x0C, 0x03 } /* diagonal stripes (9) */
#endif
};

static Pixmap stipple_halftone[stipple_halftone_num];
static Pixmap stipple_pattern[stipple_pattern_num];
static int stipple_initialized = 0;
#endif /* USE_ULIG_FILLEDBOXES */

#ifdef X11_POLYLINE
static XPoint *polyline = NULL;
static int polyline_space = 0;
static int polyline_size = 0;
#endif

/*
 * Main program
 */
int
main(int argc, char *argv[])
{

#ifdef PIPE_IPC
    int getfl;
#endif

#ifdef OSK
    /* malloc large blocks, otherwise problems with fragmented mem */
    _mallocmin(102400);
#endif
#ifdef __EMX__
    /* close open file handles */
    fcloseall();
#endif

    preset(argc, argv);

/* set up the alternative cursor */
    cursor_default = XCreateFontCursor(dpy, XC_crosshair);
    cursor = cursor_default;
#ifdef USE_MOUSE
    /* create cursors for the splot actions */
    cursor_exchange = XCreateFontCursor(dpy, XC_exchange);
    cursor_sizing = XCreateFontCursor(dpy, XC_sizing);
    /* arrow, top_left_arrow, left_ptr, sb_left_arrow, sb_right_arrow,
     * plus, pencil, draft_large, right_ptr, draft_small */
    cursor_zooming = XCreateFontCursor(dpy, XC_draft_small);
#ifndef TITLE_BAR_DRAWING_MSG
    cursor_waiting = XCreateFontCursor(dpy, XC_watch);
    cursor_save = (Cursor)0;
#endif
#endif
#ifdef PIPE_IPC
    if (!pipe_died) {
	/* set up nonblocking stdout */
	getfl = fcntl(1, F_GETFL);	/* get current flags */
	fcntl(1, F_SETFL, getfl | O_NONBLOCK);
	signal(SIGPIPE, pipe_died_handler);
    }
# endif

#ifdef X11_POLYLINE
    polyline_space = 100;
    polyline = calloc(polyline_space, sizeof(XPoint));
    if (!polyline) fprintf(stderr,"Panic: cannot allocate polyline\n");
#endif

    mainloop();

    if (persist) {
	FPRINTF((stderr, "waiting for %d windows\n", windows_open));

#ifndef DEBUG
	/* HBB 20030519: Some programs executing gnuplot -persist may
	 * be waiting for all default handles to be closed before they
	 * consider the sub-process finished.  Emacs, e.g., does.  So,
	 * unless this is a DEBUG build, drop our connection to stderr
	 * now.  Using Freopen() ensures that debug fprintf()s won't
	 * crash. */
	freopen("/dev/null", "w", stderr);
#endif

	/* read x events until all windows have been quit */
	while (windows_open > 0) {
	    XEvent event;
	    XNextEvent(dpy, &event);
	    process_event(&event);
	}
    }
    XCloseDisplay(dpy);

    FPRINTF((stderr, "exiting\n"));

    EXIT(0);
}

/*-----------------------------------------------------------------------------
 *   mainloop processing - process X events and input from gnuplot
 *
 *   Three different versions of main loop processing are provided to support
 *   three different platforms.
 * 
 *   DEFAULT_X11:     use select() for both X events and input on stdin 
 *                    from gnuplot inboard driver
 *
 *   CRIPPLED_SELECT: use select() to service X events and check during 
 *                    select timeout for temporary plot file created
 *                    by inboard driver
 *
 *   VMS:             use XNextEvent to service X events and AST to
 *                    service input from gnuplot inboard driver on stdin 
 *---------------------------------------------------------------------------*/


#ifdef DEFAULT_X11

/*
 * DEFAULT_X11 mainloop
 */
static void
mainloop()
{
    int nf, cn = ConnectionNumber(dpy), in;
    SELECT_TYPE_ARG1 nfds;
    struct timeval timeout, *timer = (struct timeval *) 0;
    fd_set tset;

#ifdef PIPE_IPC
    int out;
    out = fileno(stdout);
#endif

    X11_ipc = stdin;
    in = fileno(X11_ipc);

#ifdef PIPE_IPC
    if (out > in)
	nfds = ((cn > out) ? cn : out) + 1;
    else
#endif
	nfds = ((cn > in) ? cn : in) + 1;

#ifdef ISC22
/* Added by Robert Eckardt, RobertE@beta.TP2.Ruhr-Uni-Bochum.de */
    timeout.tv_sec = 0;		/* select() in ISC2.2 needs timeout */
    timeout.tv_usec = 300000;	/* otherwise input from gnuplot is */
    timer = &timeout;		/* suspended til next X event. */
#endif /* ISC22   (0.3s are short enough not to be noticed */

    while (1) {

	/* XNextEvent does an XFlush() before waiting. But here.
	 * we must ensure that the queue is flushed, since we
	 * dont call XNextEvent until an event arrives. (I have
	 * twice wasted quite some time over this issue, so now
	 * I am making sure of it !
	 */

	XFlush(dpy);

	FD_ZERO(&tset);
	FD_SET(cn, &tset);

	/* Don't wait for events if we know that input is
	 * already sitting in a buffer.  Also don't wait for
	 * input to become available.
	 */
	if (buffered_input_available) {
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 0;
	    timer = &timeout;
	} else {
	    timer = (struct timeval *) 0;
	    FD_SET(in, &tset);
	}

#ifdef PIPE_IPC
	if (buffered_output_pending && !pipe_died) {
	    /* check, if stdout becomes writable */
	    FD_SET(out, &tset);
	}
#endif

	nf = select(nfds, SELECT_TYPE_ARG234 &tset, 0, 0, SELECT_TYPE_ARG5 timer);

	if (nf < 0) {
	    if (errno == EINTR)
		continue;
	    perror("gnuplot_x11: select failed");
	    EXIT(1);
	}

	if (nf > 0)
	    XNoOp(dpy);

	if (XPending(dpy)) {
	    /* used to use CheckMaskEvent() but that cannot receive
	     * maskable events such as ClientMessage. So now we do
	     * one event, then return to the select.
	     * And that almost works, except that under some Xservers
	     * running without a window manager (e.g. Hummingbird Exceed under Win95)
	     * a bogus ConfigureNotify is sent followed by a valid ConfigureNotify
	     * when the window is maximized.  The two events are queued, apparently
	     * in a single I/O because select() above doesn't see the second, valid
	     * event.  This little loop fixes the problem by flushing the
	     * event queue completely.
	     */
	    XEvent xe;
	    do {
		XNextEvent(dpy, &xe);
		process_event(&xe);
	    } while (XPending(dpy));
	}

	if (FD_ISSET(in, &tset) || buffered_input_available) {
	    if (!record())	/* end of input */
		return;
	}
#ifdef PIPE_IPC
	if (!pipe_died && (FD_ISSET(out, &tset) || buffered_output_pending)) {
	    gp_exec_event(GE_pending, 0, 0, 0, 0);
	}
#endif
	/* A method in which the ErrorHandler can queue plots to be
	   removed from the linked list.  This prevents the situation
	   that could arise if the ErrorHandler directly removed
	   plots and some other part of the program were utilizing
	   a pointer to a plot that was destroyed. */
	if (process_remove_fifo_queue) {
	    Process_Remove_FIFO_Queue();
	}
    }
}

#elif defined(CRIPPLED_SELECT)

char X11_ipcpath[32];

/*
 * CRIPPLED_SELECT mainloop
 */
static void
mainloop()
{
    SELECT_TYPE_ARG1 nf, nfds, cn = ConnectionNumber(dpy);
    struct timeval timeout, *timer;
    fd_set tset;
    unsigned long all = (unsigned long) (-1L);
    XEvent xe;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    timer = &timeout;
    sprintf(X11_ipcpath, "/tmp/Gnuplot_%d", getppid());
    nfds = cn + 1;

    while (1) {
	XFlush(dpy);		/* see above */

	FD_ZERO(&tset);
	FD_SET(cn, &tset);

	/* Don't wait for events if we know that input is
	 * already sitting in a buffer.  Also don't wait for
	 * input to become available.
	 */
	if (buffered_input_available) {
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 0;
	    timer = &timeout;
	} else {
	    timer = (struct timeval *) 0;
	    FD_SET(in, &tset);
	}

	nfds = (cn > in) ? cn + 1 : in + 1;

	nf = select(nfds, SELECT_TYPE_ARG234 &tset, 0, 0, SELECT_TYPE_ARG5 timer);

	if (nf < 0) {
	    if (errno == EINTR)
		continue;
	    perror("gnuplot_x11: select failed");
	    EXIT(1);
	}

	if (nf > 0)
	    XNoOp(dpy);

	if (FD_ISSET(cn, &tset)) {
	    while (XCheckMaskEvent(dpy, all, &xe)) {
		process_event(&xe);
	    }
	}
	if ((X11_ipc = fopen(X11_ipcpath, "r"))) {
	    unlink(X11_ipcpath);
	    record();
	    fclose(X11_ipc);
	}
    }
}


#elif defined(VMS)
/*-----------------------------------------------------------------------------
 *    VMS mainloop - Yehavi Bourvine - YEHAVI@VMS.HUJI.AC.IL
 *---------------------------------------------------------------------------*/

/*  In VMS there is no decent Select(). hence, we have to loop inside
 *  XGetNextEvent for getting the next X window event. In order to get input
 *  from the master we assign a channel to SYS$INPUT and use AST's in order to
 *  receive data. In order to exit the mainloop, we need to somehow make
 *  XNextEvent return from within the ast. We do this with a XSendEvent() to
 *  ourselves !
 *  This needs a window to send the message to, so we create an unmapped window
 *  for this purpose. Event type XClientMessage is perfect for this, but it
 *  appears that such messages come from elsewhere (motif window manager,
 *  perhaps ?) So we need to check fairly carefully that it is the ast event
 *  that has been received.
 */

#include <iodef.h>
char STDIIN[] = "SYS$INPUT:";
short STDIINchannel, STDIINiosb[4];
struct {
    short size, type;
    char *address;
} STDIINdesc;
char STDIINbuffer[64];
int status;

ast()
{
    int status = sys$qio(0, STDIINchannel, IO$_READVBLK, STDIINiosb, record,
			 0, STDIINbuffer, sizeof(STDIINbuffer) - 1, 0, 0, 0, 0);
    if ((status & 0x1) == 0)
	EXIT(status);
}

Window message_window;

static void
mainloop()
{
    /* dummy unmapped window for receiving internally-generated terminate
     * messages
     */
    message_window = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 1, 0, 0);

    STDIINdesc.size = strlen(STDIIN);
    STDIINdesc.type = 0;
    STDIINdesc.address = STDIIN;
    status = sys$assign(&STDIINdesc, &STDIINchannel, 0, 0, 0);
    if ((status & 0x1) == 0)
	EXIT(status);
    ast();

    for (;;) {
	XEvent xe;
	XNextEvent(dpy, &xe);
	if (xe.type == ClientMessage && xe.xclient.window == message_window) {
	    if (xe.xclient.message_type == None && xe.xclient.format == 8 && strcmp(xe.xclient.data.b, "die gnuplot die") == 0) {
		FPRINTF((stderr, "quit message from ast\n"));
		return;
	    } else {
		FPRINTF((stderr, "Bogus XClientMessage event from window manager ?\n"));
	    }
	}
	process_event(&xe);
    }
}
#else /* !(DEFAULT_X11 || CRIPPLED_SELECT || VMS */
# error You lose. No mainloop.
#endif				/* !(DEFAULT_X11 || CRIPPLED_SELECT || VMS */

/* delete a window / plot */
static void
delete_plot(plot_struct *plot)
{
    int i;

    FPRINTF((stderr, "Delete plot %d\n", plot->plot_number));

    for (i = 0; i < plot->ncommands; ++i)
	free(plot->commands[i]);
    plot->ncommands = 0;

    /* Free up memory for window title. */
    if (plot->titlestring) {
	free(plot->titlestring);
	plot->titlestring = 0;
    }

    if (plot->window) {
	FPRINTF((stderr, "Destroy window 0x%x\n", plot->window));
	XDestroyWindow(dpy, plot->window);
	plot->window = None;
	--windows_open;
    }
#if USE_ULIG_FILLEDBOXES
    if (stipple_initialized) {	/* ULIG */
	int i;
	for (i = 0; i < stipple_halftone_num; i++)
	    XFreePixmap(dpy, stipple_halftone[i]);
	for (i = 0; i < stipple_pattern_num; i++)
	    XFreePixmap(dpy, stipple_pattern[i]);
	stipple_initialized = 0;
    }
#endif /* USE_ULIG_FILLEDBOXES */
    if (plot->pixmap) {
	XFreePixmap(dpy, plot->pixmap);
	plot->pixmap = None;
    }
#ifdef PM3D
    /* we release the colormap here to free color resources.
     * but how would be recreate the colormap ? -- need to
     * call PaletteMake() somewhere ... */
    ReleaseColormap(plot);
    sm_palette.colorMode = SMPAL_COLOR_MODE_NONE;
#endif
    /* but preserve geometry */
}


/* prepare the plot structure */
static void
prepare_plot(plot_struct *plot, int term_number)
{
    int i;

    for (i = 0; i < plot->ncommands; ++i)
	free(plot->commands[i]);
    plot->ncommands = 0;

    if (!plot->posn_flags) {
	/* first time this window has been used - use default or -geometry
	 * settings
	 */
	plot->posn_flags = gFlags;
	plot->x = gX;
	plot->y = gY;
	plot->width = gW;
	plot->height = gH;
	plot->pixmap = None;
#ifdef USE_MOUSE
	plot->gheight = gH;
	plot->resizing = FALSE;
	plot->str[0] = '\0';
	plot->zoombox_on = FALSE;
#endif
    }
    if (!plot->window) {
	plot->cmap = &cmap;	/* default color space */
	pr_window(plot);
#ifdef USE_MOUSE
	/*
	 * set all mouse parameters
	 * to a well-defined state.
	 */
	plot->button = 0;
	plot->mouse_on = TRUE;
	plot->x = NOT_AVAILABLE;
	plot->y = NOT_AVAILABLE;
	if (plot->str[0] != '\0') {
	    /* if string was non-empty last time, initialize it as almost empty: one space, to prevent window resize */
	    plot->str[0] = ' ';
	    plot->str[1] = '\0';
	}
	plot->time = 0;		/* XXX how should we initialize this ? XXX */
#endif
    }
/* We don't know that it is the same window as before, so we reset the
 * cursors for all windows and then define the cursor for the active
 * window 
 */
    /* allow releasing of the cmap at the first drawing
     * operation if no palette was allocated until then.
     */
    plot->angle = 0;		/* default is horizontal */
#ifdef PM3D
    plot->release_cmap = TRUE;
#endif
    reset_cursor();
    XDefineCursor(dpy, plot->window, cursor);
}

/* store a command in a plot structure */
static void
store_command(char *buffer, plot_struct *plot)
{
    char *p;

    FPRINTF((stderr, "Store in %d : %s", plot->plot_number, buffer));

    if (plot->ncommands >= plot->max_commands) {
	plot->max_commands = plot->max_commands * 2 + 1;
	plot->commands = (plot->commands)
	    ? (char **) realloc(plot->commands, plot->max_commands * sizeof(char *))
	    : (char **) malloc(sizeof(char *));
    }
    p = (char *) malloc((unsigned) strlen(buffer) + 1);
    if (!plot->commands || !p) {
	fputs("gnuplot: can't get memory. X11 aborted.\n", stderr);
	EXIT(1);
    }
    plot->commands[plot->ncommands++] = strcpy(p, buffer);
}

#ifndef VMS

static int read_input __PROTO((void));

/*
 * Handle input.  Use read instead of fgets because stdio buffering
 * causes trouble when combined with calls to select.
 */
static int
read_input()
{
    static int rdbuf_size = 10 * Nbuf;
    static char rdbuf[10 * Nbuf - 1];
    static int total_chars;
    static int rdbuf_offset;
    static int buf_offset;
    static int partial_read = 0;
    int fd = fileno(X11_ipc);

    if (!partial_read)
	buf_offset = 0;

    if (!buffered_input_available) {
	total_chars = read(fd, rdbuf, rdbuf_size);
	buffered_input_available = 1;
	partial_read = 0;
	rdbuf_offset = 0;
	if (total_chars == 0)
	    return -2;
	if (total_chars < 0)
	    return -1;
    }

    if (rdbuf_offset < total_chars) {
	while (rdbuf_offset < total_chars && buf_offset < Nbuf) {
	    char c = rdbuf[rdbuf_offset++];
	    buf[buf_offset++] = c;
	    if (c == '\n')
		break;
	}

	if (buf_offset == Nbuf) {
	    fputs("\
\n\
gnuplot: buffer overflow in read_input!\n\
gnuplot: X11 aborted.\n", stderr);
	    EXIT(1);
	} else
	    buf[buf_offset] = NUL;
    }

    if (rdbuf_offset == total_chars) {
	buffered_input_available = 0;
	if (buf[buf_offset - 1] != '\n')
	    partial_read = 1;
    }

    return partial_read;
}


#ifdef PM3D
/*  
 * This function builds back a palette from what x11.trm has written
 * into the pipe.  It cheats:  SMPAL_COLOR_MODE_FUNCTIONS for user defined
 * formulaes to transform gray into the three color components is not
 * implemented.  If this had to be done, one would have to include all
 * the code for evaluating functions here too, and, even worse: how to
 * transmit all function definition from gnuplot to here! 
 * To avoid this, the following is done:  For grayscale, and rgbformulae
 * everything is easy.  Gradients are more difficult: Each gradient point
 * is encoded in a 8-byte string (which does not include any '\n' and
 * transmitted here.  Here the gradient is rebuilt.
 * Form user defined formulae x11.trm builds a special gradient:  The gray
 * values are equally spaced and are not transmitted, only the 6 bytes for
 * the rgbcolor are sent.  These are assembled into a gradient which is
 * than used in the palette.
 *
 * This function belongs completely into record(), but is quiet large so it
 * became a function of its own.  
*/
static void
scan_palette_from_buf( plot_struct *plot )
{
    t_sm_palette tpal;
    char cm, pos, mod;
    if (4 != sscanf( buf+2, "%c %c %c %d", &cm, &pos, &mod,
		     &(tpal.use_maxcolors) ) ) {
        fprintf( stderr, "%s:%d error in setting palette.\n", 
		 __FILE__, __LINE__);
	
	return;
    } 
    
    tpal.colorMode = cm;
    tpal.positive = pos;
    tpal.cmodel = mod;

    /* function palettes are transmitted as approximated gradients: */
    if (tpal.colorMode == SMPAL_COLOR_MODE_FUNCTIONS)
      tpal.colorMode = SMPAL_COLOR_MODE_GRADIENT;

    switch( tpal.colorMode ) {
    case SMPAL_COLOR_MODE_GRAY:  
	read_input();  /*  FIXME: discarding status  */
	if (1 != sscanf( buf, "%lf", &(tpal.gamma) )) {
	    fprintf( stderr, "%s:%d error in setting palette.\n", 
		     __FILE__, __LINE__);
	    return;
	}
	break;
    case SMPAL_COLOR_MODE_RGB:
	read_input();  /*  FIXME: discarding status  */
	if (3 != sscanf( buf, "%d %d %d", &(tpal.formulaR),
			 &(tpal.formulaG), &(tpal.formulaB) )) {
	    fprintf( stderr, "%s:%d error in setting palette.\n", 
		     __FILE__, __LINE__);
	    return;
	}
	break;
    case SMPAL_COLOR_MODE_GRADIENT: {
	int i=0;
	read_input();  /*  FIXME: discarding status  */
	if (1 != sscanf( buf, "%d", &(tpal.gradient_num) )) {
	    fprintf( stderr, "%s:%d error in setting palette.\n", 
		     __FILE__, __LINE__);
	    return;
	}
	tpal.gradient = (gradient_struct*)
	  malloc( tpal.gradient_num * sizeof(gradient_struct) );
	for( i=0; i<tpal.gradient_num; i++ ) {
	    /*  this %50 *must* match the amount of gradient structs
		written to the pipe by x11.trm!  */
	    if (i%50 == 0) {  
	        read_input();  /*  FIXME: discarding status  */
	    }
	    str_to_gradient_entry( &(buf[8*(i%50)]), &(tpal.gradient[i]) );
	}
	break;
      }
    case SMPAL_COLOR_MODE_FUNCTIONS: 
        fprintf( stderr, "%s:%d ooops: No function palettes for x11!\n",
		 __FILE__, __LINE__ );
	break;
    default:
        fprintf( stderr, "%s:%d ooops: Unknown colorMode '%c'.\n",
		 __FILE__, __LINE__, (char)(tpal.colorMode) );
	tpal.colorMode = SMPAL_COLOR_MODE_GRAY;
	break;
    }
    PaletteMake(plot, &tpal);
}
#endif  /* PM3D */



/*
 * record - record new plot from gnuplot inboard X11 driver (Unix)
 */
static struct plot_struct *plot = NULL;
static int
record()
{
    while (1) {
	int status = read_input();
	if (status == -2)
	    return 0;
	if (status != 0)
	    return status;

	switch (*buf) {
	case 'G':		/* enter graphics mode */
	    {
		int plot_number;
#ifndef USE_MOUSE
		sscanf(buf, "G%d", &plot_number);
#else
#ifdef OS2_IPC
		sscanf(buf, "G%d %lu %li", &plot_number, &gnuplotXID, &ppidGnu);
#else
		sscanf(buf, "G%d %lu", &plot_number, &gnuplotXID);
#endif
#endif
		FPRINTF((stderr, "plot for window number %d\n", plot_number));
		if (!(plot = Find_Plot_In_Linked_List_By_Number(plot_number)))
		    plot = Add_Plot_To_Linked_List(plot_number);
		if (plot)
		    prepare_plot(plot, plot_number);
		current_plot = plot;
#ifdef OS2_IPC
		if (!input_from_PM_Terminal) {	/* get shared memory */
		    sprintf(mouseShareMemName, "\\SHAREMEM\\GP%i_Mouse_Input", (int) ppidGnu);
		    if (DosGetNamedSharedMem(&input_from_PM_Terminal, mouseShareMemName, PAG_WRITE))
			DosBeep(1440L, 1000L);	/* indicates error */
		    semInputReady = 0;
		}
#endif
#ifdef USE_MOUSE
#ifdef TITLE_BAR_DRAWING_MSG
		/* show a message in the wm's title bar that the
		 * graph will be redrawn. This might be useful
		 * for slow redrawing (large plots). The title
		 * string is reset to the default at the end of
		 * display(). We should make this configurable!
		 */
		if (plot) {
		    if (plot->window) {
			char *msg;
			char *added_text = " drawing ...";
			int orig_len = (plot->titlestring ? strlen(plot->titlestring : 0);
			if (msg = (char *) malloc(orig_len + strlen(added_text) + 1) {
			    strcpy(msg, plot->titlestring);
			    strcat(msg, added_text);
			    XStoreName(dpy, plot->window, msg);
			    free(msg);
			} else
			    XStoreName(dpy, plot->window, added_text + 1);
		    }
		}
#else
		if (!button_pressed) {
		    cursor_save = cursor;
		    cursor = cursor_waiting;
		    if (plot)
			XDefineCursor(dpy, plot->window, cursor);
		}
#endif
#endif
		/* continue; */
		/* return 1; To do: Should a "return" be here?  Gnuplot usually sends 'G' followed by other commands.  So perhaps not needed. DJS */
	    }
	    break;
	case 'N':		/* just update the plot number */
	    {
		int itmp;
		if (strcspn(buf+1," \n") && sscanf(buf, "N%d", &itmp))
		    current_plot = Add_Plot_To_Linked_List(itmp);
		return 1;
	    }
	    break;
	case 'C':		/* close the plot with given number */
	    {
		int itmp;
		if (strcspn(buf+1," \n") && sscanf(buf, "C%d", &itmp)) {
		  plot_struct *psp;
		  if ((psp = Find_Plot_In_Linked_List_By_Number(itmp)))
		    Remove_Plot_From_Linked_List(psp->window);
		} else if (current_plot) {
		  Remove_Plot_From_Linked_List(current_plot->window);
		}
		return 1;
	    }
	    break;
	case 'n':		/* update the plot name (title) */
	    {
		if (!current_plot)
		    current_plot = Add_Plot_To_Linked_List(0);
		if (current_plot) {
		    char *cp;
		    if (current_plot->titlestring)
			free(current_plot->titlestring);
		    if ((current_plot->titlestring = (char *) malloc(strlen(buf+1) + 1) )) {
			strcpy(current_plot->titlestring, buf+1);
			cp = current_plot->titlestring;
		    } else
			cp = "<lost name>";
		    if (current_plot->window)
			XStoreName(dpy, current_plot->window, cp);
		}
		return 1;
	    }
	    break;
	case 'E':		/* leave graphics mode / suspend */
	    if (plot)
		display(plot);
#ifdef USE_MOUSE
	    if (plot == current_plot)
		gp_exec_event(GE_plotdone, 0, 0, 0, 0);	/* notify main program */
#endif
	    return 1;
	case 'R':		/* leave x11 mode */
	    reset_cursor();
	    return 0;

#ifdef PM3D
	case X11_GR_MAKE_PALETTE:
	    if (plot)
		scan_palette_from_buf( plot );
	    return 1;
#if 0
	case X11_GR_RELEASE_PALETTE:
	    /* turn pm3d off */
	    FPRINTF((stderr, "X11_GR_RELEASE_PALETTE\n"));
	    if (plot)
		ReleaseColormap(plot);
	    sm_palette.colorMode = SMPAL_COLOR_MODE_NONE;
	    free( sm_palette.gradient );
	    return 1;
#endif
#endif

	case 'X':		/* tell the driver about do_raise /  persist */
	    {
		int tmp_do_raise, tmp_persist;
		if (2 == sscanf(buf, "X%d%d", &tmp_do_raise, &tmp_persist)) {
		    if (UNSET != tmp_do_raise) {
			do_raise = tmp_do_raise;
		    }
		    if (UNSET != tmp_persist) {
			persist = tmp_persist;
		    }
		}
	    }
	    return 1;

#ifdef USE_MOUSE
	case 'u':
#ifdef PIPE_IPC
	    if (!pipe_died)
#endif
	    {
		/* `set cursor' */
		int c, x, y;
		sscanf(buf, "u%4d%4d%4d", &c, &x, &y);
		if (plot) {
		    switch (c) {
		    case -2:	/* warp pointer */
			XWarpPointer(dpy, None /* src_w */ ,
				     plot->window /* dest_w */ , 0, 0, 0, 0, X(x), Y(y));
		    case -1:	/* zoombox */
			plot->zoombox_x1 = plot->zoombox_x2 = X(x);
			plot->zoombox_y1 = plot->zoombox_y2 = Y(y);
			plot->zoombox_on = TRUE;
			DrawBox(plot);
			break;
		    case 0:	/* standard cross-hair cursor */
			cursor = cursor_default;
			XDefineCursor(dpy, plot->window, cursor);
			break;
		    case 1:	/* cursor during rotation */
			cursor = cursor_exchange;
			XDefineCursor(dpy, plot->window, cursor);
			break;
		    case 2:	/* cursor during scaling */
			cursor = cursor_sizing;
			XDefineCursor(dpy, plot->window, cursor);
			break;
		    case 3:	/* cursor during zooming */
			cursor = cursor_zooming;
			XDefineCursor(dpy, plot->window, cursor);
			break;
		    }
		    if (c >= 0 && plot->zoombox_on) {
			/* erase zoom box */
			DrawBox(plot);
			plot->zoombox_on = FALSE;
		    }
		}
	    }
	    return 1;

	case 't':
#ifdef PIPE_IPC
	    if (!pipe_died)
#endif
	    {
		int where;
		char *second;
		if (sscanf(buf, "t%4d", &where) != 1)
		    return 1;
		buf[strlen(buf) - 1] = 0;	/* remove trailing \n */
		if (plot) {
		    switch (where) {
		    case 0:
			DisplayCoords(plot, buf + 5);
			break;
		    case 1:
			second = strchr(buf + 5, '\r');
			if (second == NULL) {
			    *(plot->zoombox_str1a) = '\0';
			    *(plot->zoombox_str1b) = '\0';
			    break;
			}
			*second = 0;
			second++;
			if (plot->zoombox_on)
			    DrawBox(plot);
			strcpy(plot->zoombox_str1a, buf + 5);
			strcpy(plot->zoombox_str1b, second);
			if (plot->zoombox_on)
			    DrawBox(plot);
			break;
		    case 2:
			second = strchr(buf + 5, '\r');
			if (second == NULL) {
			    *(plot->zoombox_str2a) = '\0';
			    *(plot->zoombox_str2b) = '\0';
			    break;
			}
			*second = 0;
			second++;
			if (plot->zoombox_on)
			    DrawBox(plot);
			strcpy(plot->zoombox_str2a, buf + 5);
			strcpy(plot->zoombox_str2b, second);
			if (plot->zoombox_on)
			    DrawBox(plot);
			break;
		    }
		}
	    }
	    return 1;

	case 'r':
#ifdef PIPE_IPC
	    if (!pipe_died)
#endif
	    {
		if (plot) {
		    int x, y;
		    DrawRuler(plot);	/* erase previous ruler */
		    sscanf(buf, "r%4d%4d", &x, &y);
		    if (x < 0)
			plot->ruler_on = FALSE;
		    else {
			plot->ruler_on = TRUE;
			plot->ruler_x = x;
			plot->ruler_y = y;
		    }
		    DrawRuler(plot);	/* draw new one */
		}
	    }
	    return 1;

	case 'z':
#ifdef PIPE_IPC
	    if (!pipe_died)
#endif
	    {
		int len = strlen(buf + 1) - 1;	/* discard newline '\n' */
		memcpy(selection, buf + 1, len < SEL_LEN ? len : SEL_LEN);
		/* terminate */
		selection[len + 1 < SEL_LEN ? len + 1 : SEL_LEN - 1] = '\0';
		XStoreBytes(dpy, buf + 1, len);
		XFlush(dpy);
#ifdef EXPORT_SELECTION
		if (plot)
		    export_graph(plot);
#endif
	    }
	    return 1;
#endif
#ifdef USE_MOUSE
	case 'Q':		
	    /* Set default font immediately and return size info through pipe */
	    if (buf[1] == 'G') {
		int scaled_hchar, scaled_vchar;
		char *c = &(buf[strlen(buf)-1]);
		while (*c <= ' ') *c-- = '\0';
		strncpy(default_font,&buf[2],strlen(&buf[2])+1);
		pr_font(NULL);
		if (plot) {
		    /* EAM FIXME - this is all out of order; initialization doesnt */
		    /*             happen until term->graphics() is called.        */
		    xscale = (plot->width > 0) ? plot->width / 4096. : gW / 4096.;
		    yscale = (plot->height > 0) ? plot->height / 4096. : gH / 4096.;
		    scaled_hchar = (1.0/xscale) * hchar;
		    scaled_vchar = (1.0/yscale) * vchar;
		    FPRINTF((stderr,"gplt_x11: preset default font to %s hchar = %d vchar = %d \n", 
			     default_font, scaled_hchar, scaled_vchar));
		    gp_exec_event(GE_fontprops, plot->width, plot->height, 
				  scaled_hchar, scaled_vchar);
		}
		return 1;
	    }
	    /* fall through */
#endif
	default:
	    if (plot)
		store_command(buf, plot);
	    continue;
	}
    }
    if (feof(X11_ipc) || ferror(X11_ipc))
	return 0;
    else
	return 1;
}

#else /* VMS */

/*
 *   record - record new plot from gnuplot inboard X11 driver (VMS)
 */
static struct plot_struct *plot = NULL;
record()
{
    int status;

    if ((STDIINiosb[0] & 0x1) == 0)
	EXIT(STDIINiosb[0]);
    STDIINbuffer[STDIINiosb[1]] = '\0';
    strcpy(buf, STDIINbuffer);

    switch (*buf) {
    case 'G':			/* enter graphics mode */
	{
	    int plot_number = atoi(buf + 1);	/* 0 if none specified */
	    FPRINTF((stderr, "plot for window number %d\n", plot_number));
	    if (!(plot = Find_Plot_In_Linked_List_By_Number(plot_number)))
		plot = Add_Plot_To_Linked_List(plot_number);
	    if (plot)
		prepare_plot(plot, plot_number);
	    current_plot = plot;
	    break;
	}
    case 'E':			/* leave graphics mode */
	if (plot) 
	    display(plot);
	break;
    case 'R':			/* exit x11 mode */
	FPRINTF((stderr, "received R - sending ClientMessage\n"));
	reset_cursor();
	sys$cancel(STDIINchannel);
	/* this is ridiculous - cook up an event to ourselves,
	 * in order to get the mainloop() out of the XNextEvent() call
	 * it seems that window manager can also send clientmessages,
	 * so put a checksum into the message
	 */
	{
	    XClientMessageEvent event;
	    event.type = ClientMessage;
	    event.send_event = True;
	    event.display = dpy;
	    event.window = message_window;
	    event.message_type = None;
	    event.format = 8;
	    strcpy(event.data.b, "die gnuplot die");
	    XSendEvent(dpy, message_window, False, 0, (XEvent *) & event);
	    XFlush(dpy);
	}
	return;			/* no ast */
    default:
	if (plot)
	    store_command(buf, plot);
	break;
    }
    ast();
}
#endif /* VMS */

static int
DrawRotatedErrorHandler(Display * display, XErrorEvent * error_event)
{
  return 0;  /* do nothing */
}

static void
DrawRotated(plot_struct *plot, Display *dpy, GC gc, int xdest, int ydest,
       	const char *str, int len)
{
    Window w = plot->window;
    Drawable d = plot->pixmap;
    int angle = plot->angle;
    enum JUSTIFY just = plot->jmode;
    int x, y;
    double src_x, src_y;
    double dest_x, dest_y;
    int width = XTextWidth(font, str, len);
    int height = vchar;
    double src_cen_x = (double)width * 0.5;
    double src_cen_y = (double)height * 0.5;
    static const double deg2rad = .01745329251994329576; /* atan2(1, 1) / 45.0; */
    double sa = sin(angle * deg2rad);
    double ca = cos(angle * deg2rad);
    int dest_width = (double)height * fabs(sa) + (double)width * fabs(ca) + 2;
    int dest_height = (double)width * fabs(sa) + (double)height * fabs(ca) + 2;
    double dest_cen_x = (double)dest_width * 0.5;
    double dest_cen_y = (double)dest_height * 0.5;
    char* data = (char*) malloc(dest_width * dest_height * sizeof(char));
    Pixmap pixmap_src = XCreatePixmap(dpy, root, (unsigned int)width, (unsigned int)height, 1);
    XImage *image_src;
    XImage *image_dest;
    XImage *image_scr;
    unsigned long fgpixel = 0;
    XWindowAttributes win_attrib;
    int xscr, yscr, xoff, yoff;
    unsigned int scr_width, scr_height;
    XErrorHandler prevErrorHandler;

    unsigned long gcFunctionMask = GCFunction;
    XGCValues gcValues;
    int gcCurrentFunction = 0;
    Status s = XGetGCValues(dpy, gc, gcFunctionMask|GCForeground, &gcValues);

    /* bitmapGC is static, so that is has to be initialized only once */
    static GC bitmapGC = (GC) 0;

    if (s) {
	/* success */
	fgpixel = gcValues.foreground;
	gcCurrentFunction = gcValues.function; /* save current function */
	gcValues.function = GXcopy; /* merge image_scr with drawable */
	XChangeGC(dpy, gc, gcFunctionMask, &gcValues);
    }

    /* eventually initialize bitmapGC */
    if ((GC)0 == bitmapGC) {
	bitmapGC = XCreateGC(dpy, pixmap_src, 0, (XGCValues *) 0);
	XSetForeground(dpy, bitmapGC, 1);
	XSetBackground(dpy, bitmapGC, 0);
    }

    /* set font for the bitmap GC */
    XSetFont(dpy, bitmapGC, font->fid);

    /* draw string to the source bitmap */
    XDrawImageString(dpy, pixmap_src, bitmapGC, 0, font->ascent, str, len);

    /* create XImage's of depth 1 */
    /* source from pixmap */
    image_src = XGetImage(dpy, pixmap_src, 0, 0, (unsigned int)width, (unsigned int)height,
	    1, XYPixmap /* ZPixmap, XYBitmap */ );

    /* empty dest */
    assert(data);
    memset((void*)data, 0, (size_t)dest_width * dest_height);
    image_dest = XCreateImage(dpy, vis, 1, XYBitmap,
	    0, data, (unsigned int)dest_width, (unsigned int)dest_height, 8, 0);
#define RotateX(_x, _y) (( (_x) * ca + (_y) * sa + dest_cen_x))
#define RotateY(_x, _y) ((-(_x) * sa + (_y) * ca + dest_cen_y))
    /* copy & rotate from source --> dest */
    for (y = 0, src_y = -src_cen_y; y < height; y++, src_y++) {
	for (x = 0, src_x = -src_cen_x; x < width; x++, src_x++) {
	    /* TODO: move some operations outside the inner loop (joze) */
	    dest_x = rint(RotateX(src_x, src_y));
	    dest_y = rint(RotateY(src_x, src_y));
	    if (dest_x >= 0 && dest_x < dest_width && dest_y >= 0 && dest_y < dest_height)
		XPutPixel(image_dest, (int)dest_x, (int)dest_y, XGetPixel(image_src, x, y));
	}
    }

    src_cen_y = 0; /* EAM 29-Sep-2002 - vertical justification has already been done */

    switch (just) {
	case LEFT:
	default:
	    xdest -= RotateX(-src_cen_x, src_cen_y);
	    ydest -= RotateY(-src_cen_x, src_cen_y);
	    break;
	case CENTRE:
	    xdest -= RotateX(0, src_cen_y);
	    ydest -= RotateY(0, src_cen_y);
	    break;
	case RIGHT:
	    xdest -= RotateX(src_cen_x, src_cen_y);
	    ydest -= RotateY(src_cen_x, src_cen_y);
	    break;
    }

#undef RotateX
#undef RotateY

    /* grab the current screen area where the new text will go */
    s = XGetWindowAttributes(dpy, w, &win_attrib);
    /* compute screen coords that are within the current window */
    xscr = (xdest<0)? 0 : xdest; 
    yscr = (ydest<0)? 0 : ydest;;
    scr_width = dest_width; scr_height = dest_height;
    if (xscr + dest_width > win_attrib.width){
      scr_width = win_attrib.width - xscr;
    }
    if (yscr + dest_height > win_attrib.height){
      scr_height = win_attrib.height - yscr;
    }
    xoff = xscr - xdest;
    yoff = yscr - ydest;
    scr_width -= xoff;
    scr_height -= yoff;
    prevErrorHandler = XSetErrorHandler(DrawRotatedErrorHandler);

    image_scr = XGetImage(dpy, d, xscr, yscr, scr_width,
			  scr_height, AllPlanes, XYPixmap);
    if (image_scr != 0){
      /* copy from 1 bit bitmap image of text to the full depth image of screen*/
      for (y = 0; y < scr_height; y++){
	for (x = 0; x < scr_width; x++){
	  if (XGetPixel(image_dest, x + xoff, y + yoff)){
	    XPutPixel(image_scr, x, y, fgpixel);
	  }
	}
      }
      /* copy the rotated image to the drawable d */
      XPutImage(dpy, d, gc, image_scr, 0, 0, xscr, yscr, scr_width, scr_height);

      XDestroyImage(image_scr);
    }
    XSetErrorHandler(prevErrorHandler);

    /* free resources */
    XFreePixmap(dpy, pixmap_src);
    XDestroyImage(image_src);
    XDestroyImage(image_dest);

    if (s) {
	/* restore old function of gc */
	gcValues.function = gcCurrentFunction;
	XChangeGC(dpy, gc, gcFunctionMask, &gcValues);
    }
}

/*
 *   exec_cmd - execute drawing command from inboard driver
 */
static void
exec_cmd(plot_struct *plot, char *command)
{
    int x, y, sw, sl, sj;
    char *buffer, *str;

    buffer = command;
    FPRINTF((stderr, "(display) buffer = |%s|\n", buffer));

#ifdef X11_POLYLINE
    /*   X11_vector(x,y) - draw vector  */
    if (*buffer == 'V') {
	sscanf(buffer, "V%4d%4d", &x, &y);
	if (polyline_size == 0) {
	    polyline[polyline_size].x = X(cx);
	    polyline[polyline_size].y = Y(cy);
	}
	if (++polyline_size >= polyline_space) {
	    polyline_space += 100;
	    polyline = realloc(polyline, polyline_space * sizeof(XPoint));
	    if (!polyline) fprintf(stderr,"Panic: cannot realloc polyline\n");
	}
	polyline[polyline_size].x = X(x);
	polyline[polyline_size].y = Y(y);
	cx = x;
	cy = y;
	FPRINTF((stderr, "(display) loading polyline element %d\n",polyline_size));
	return;
    } else if (polyline_size > 0) {
	FPRINTF((stderr, "(display) dumping polyline size %d\n",polyline_size));
	XDrawLines(dpy, plot->pixmap, *current_gc, polyline, polyline_size+1, CoordModeOrigin);
	polyline_size = 0;
    }
#else
    /*   X11_vector(x,y) - draw vector  */
    if (*buffer == 'V') {
	sscanf(buffer, "V%4d%4d", &x, &y);
	XDrawLine(dpy, plot->pixmap, *current_gc, X(cx), Y(cy), X(x), Y(y));
	cx = x;
	cy = y;
    } else
#endif
    /*   X11_move(x,y) - move  */
    if (*buffer == 'M')
	sscanf(buffer, "M%4d%4d", &cx, &cy);

    /* change default font (QD) encoding (QE) or current font (QF)  */
    else if (*buffer == 'Q') {
	char *c;
	switch (buffer[1]) {
	case 'F':
		/* Strip out just the font name */
		c = &(buffer[strlen(buffer)-1]);
		while (*c <= ' ') *c-- = '\0';
	    	pr_font(&buffer[2]);
		XSetFont(dpy,gc,font->fid);
		break;
	case 'E':
		/* Save the requested font encoding */
		{
		    int tmp;
		    sscanf(buffer,"QE%d", &tmp);
		    encoding = (enum set_encoding_id)tmp;
		}
		FPRINTF((stderr,"gnuplot_x11: changing encoding to %d\n",encoding));
		break;
	case 'D':
		/* Save the request default font */
		c = &(buffer[strlen(buffer)-1]);
		while (*c <= ' ') *c-- = '\0';
		strncpy(default_font,&buffer[2],strlen(&buffer[2])+1);
		FPRINTF((stderr,"gnuplot_x11: exec_cmd() set default_font to \"%s\"\n",default_font));
		break;
	}
    }

    /*   X11_put_text(x,y,str) - draw text   */
    else if (*buffer == 'T') {
	/* Enhanced text mode added November 2003 - Ethan A Merritt */
	int x_offset=0, y_offset=0, v_offset=0;

	switch (buffer[1]) {

	case 'j':	/* Set start for right-justified enhanced text */
		    sscanf(buffer+2, "%4d%4d", &x_offset, &y_offset);
		    plot->xLast = x_offset - (plot->xLast - x_offset);
		    plot->yLast = y_offset - (plot->yLast - y_offset);
		    return;
	case 'k':	/* Set start for center-justified enhanced text */
		    sscanf(buffer+2, "%4d%4d", &x_offset, &y_offset);
		    plot->xLast = x_offset - 0.5*(plot->xLast - x_offset);
		    plot->yLast = y_offset - 0.5*(plot->yLast - y_offset);
		    return;
	case 'o':	/* Enhanced mode print with no update */
	case 'c':	/* Enhanced mode print with update to center */
	case 'u':	/* Enhanced mode print with update */
	case 's':	/* Enhanced mode update with no print */
		    sscanf(buffer+2, "%4d%4d", &x_offset, &y_offset);
		    /* EAM FIXME - This code has only been tested for x_offset == 0 */
		    if (plot->angle != 0) {
			int xtmp=0, ytmp=0;
			xtmp += x_offset * cos((double)(plot->angle) * 0.01745);
			xtmp -= y_offset * sin((double)(plot->angle) * 0.01745) * yscale/xscale;
			ytmp += x_offset * sin((double)(plot->angle) * 0.01745) * xscale/yscale;
			ytmp += y_offset * cos((double)(plot->angle) * 0.01745);
			x_offset = xtmp;
			y_offset = ytmp;
		    }
		    x = plot->xLast + x_offset;
		    y = plot->yLast + y_offset;
		    v_offset = 0;
		    str = buffer + 10;
		    break;
	case 'p':	/* Push (Save) position for later use */
		    plot->xSave = plot->xLast;
		    plot->ySave = plot->yLast;
		    return;
	case 'r':	/* Pop (Restore) saved position */
		    plot->xLast = plot->xSave;
		    plot->yLast = plot->ySave;
		    return;
	default:
		    sscanf(buffer, "T%4d%4d", &x, &y);
		    v_offset = vchar/3;		/* Why is this??? */
		    str = buffer + 9;
		    break;
	}

	sl = strlen(str) - 1;
	sw = XTextWidth(font, str, sl);

/*	EAM - May 2002 	Modify to allow colored text.
 *	1) do not force foreground of gc to be black
 *	2) write text to (*current_gc), rather than to gc, so that text color can be set
 *	   using pm3d mappings.
 */

	switch (plot->jmode) {
	    default:
	    case LEFT:
		sj = 0;
		break;
	    case CENTRE:
		sj = -sw / 2;
		break;
	    case RIGHT:
		sj = -sw;
		break;
	}

	if (sl == 0) /* Pointless to draw empty string */
	    ;
	else if (buffer[1] == 's') /* Enhanced text mode reserve space only */
	    ;
	else if (plot->angle != 0) {
	    /* rotated text */
	    DrawRotated(plot, dpy, *current_gc, X(x), Y(y), str, sl);
	} else {
	    /* horizontal text */
	    XDrawString(dpy, plot->pixmap, *current_gc,
		    X(x) + sj, Y(y) + v_offset, str, sl);
	}

	/* Update current text position */
	if (buffer[1] == 'c') {
	    plot->xLast = RevX(X(x) + sj + sw/2) - x_offset;
	    plot->yLast = y - y_offset;
	} else if (buffer[1] != 'o') {
	    plot->xLast = RevX(X(x) + sj + sw) - x_offset;
	    plot->yLast = y - y_offset;
	    if (plot->angle != 0) { /* This correction is not perfect */
		plot->yLast += RevX(sw) * sin((double)(plot->angle) * 0.01745) * xscale/yscale;
		plot->xLast -= RevX(sw) * (1.0 - cos((double)(plot->angle) * 0.01745));
	    }
	}

    } else if (*buffer == 'F') {	/* fill box */
	int style, xtmp, ytmp, w, h;

	if (sscanf(buffer + 1, "%4d%4d%4d%4d%4d", &style, &xtmp, &ytmp, &w, &h) == 5) {
#if USE_ULIG_FILLEDBOXES
	    int fillpar, idx;

	    /* upper 3 nibbles contain fillparameter (ULIG) */
	    fillpar = style >> 4;

	    /* lower nibble contains fillstyle */
	    switch (style & 0xf) {
	    case FS_SOLID:
	    /* use halftone fill pattern according to filldensity */
		/* filldensity is from 0..100 percent */
		idx = (int) (fillpar * (stipple_halftone_num - 1) / 100);
		if (idx < 0)
		    idx = 0;
		if (idx >= stipple_halftone_num)
		    idx = stipple_halftone_num - 1;
		XSetStipple(dpy, gc, stipple_halftone[idx]);
		XSetFillStyle(dpy, gc, FillOpaqueStippled);
		XSetForeground(dpy, gc, plot->cmap->colors[plot->lt + 3]);
		break;
	    case FS_PATTERN:
	    /* use fill pattern according to fillpattern */
		idx = (int) fillpar;	/* fillpattern is enumerated */
		if (idx < 0)
		    idx = 0;
		idx = idx % stipple_pattern_num;
		XSetStipple(dpy, gc, stipple_pattern[idx]);
		XSetFillStyle(dpy, gc, FillOpaqueStippled);
		XSetForeground(dpy, gc, plot->cmap->colors[plot->lt + 3]);
		break;
	    case FS_EMPTY:
	    default:
	    /* fill with background color */
		XSetFillStyle(dpy, gc, FillSolid);
		XSetForeground(dpy, gc, plot->cmap->colors[0]);
	    }
#endif /* USE_ULIG_FILLEDBOXES */

	    /* gnuplot has origin at bottom left, but X uses top left
	     * There may be an off-by-one (or more) error here.
	     */
	    ytmp += h;		/* top left corner of rectangle to be filled */
	    w *= xscale;
	    h *= yscale;
#if USE_ULIG_FILLEDBOXES
	    XFillRectangle(dpy, plot->pixmap, gc, X(xtmp), Y(ytmp), w + 1, h + 1);
	    /* reset everything */
	    XSetForeground(dpy, gc, plot->cmap->colors[plot->lt + 3]);
	    XSetStipple(dpy, gc, stipple_halftone[0]);
	    XSetFillStyle(dpy, gc, FillSolid);
#else /* ! USE_ULIG_FILLEDBOXES */
	    XSetForeground(dpy, gc, plot->cmap->colors[0]);
	    XFillRectangle(dpy, plot->pixmap, gc, X(xtmp), Y(ytmp), w, h);
	    XSetForeground(dpy, gc, plot->cmap->colors[plot->lt + 3]);
#endif /* USE_ULIG_FILLEDBOXES */
	}
    }
    /*   X11_justify_text(mode) - set text justification mode  */
    else if (*buffer == 'J')
	sscanf(buffer, "J%4d", (int *) &plot->jmode);

    else if (*buffer == 'A')
	sscanf(buffer + 1, "%d", (int *) &plot->angle);

    /*  X11_linewidth(plot->lwidth) - set line width */
    else if (*buffer == 'W')
	sscanf(buffer + 1, "%4d", &plot->user_width);

    /*   X11_linetype(plot->type) - set line type  */
    else if (*buffer == 'L') {
	sscanf(buffer, "L%4d", &plot->lt);
	plot->lt = (plot->lt % 8) + 2;
	if (plot->lt < 0) /* LT_NODRAW, LT_BACKGROUND, LT_UNDEFINED */
	    plot->lt = -3;
	
	/* default width is 0 {which X treats as 1} */
	plot->lwidth = widths[plot->lt] ? plot->user_width * widths[plot->lt] : plot->user_width;
	if (dashes[plot->lt][0]) {
	    plot->type = LineOnOffDash;
	    XSetDashes(dpy, gc, 0, dashes[plot->lt], strlen(dashes[plot->lt]));
	} else {
	    plot->type = LineSolid;
	}
	XSetForeground(dpy, gc, plot->cmap->colors[plot->lt + 3]);
	XSetLineAttributes(dpy, gc, plot->lwidth, plot->type, CapButt, JoinBevel);
	current_gc = &gc;
#ifdef PM3D
	/* Set line width for pm3d mode also, but not dashes */
	if (gc_pm3d)
	    XSetLineAttributes(dpy, gc_pm3d, plot->lwidth, LineSolid, CapButt, JoinBevel);
#endif
    }
    /*   X11_point(number) - draw a point */
    else if (*buffer == 'P') {
	int point;
	sscanf(buffer + 1, "%d %d %d", &point, &x, &y);
	if (point == -2) {
	    /* set point size */
	    plot->px = (int) (x * xscale * pointsize);
	    plot->py = (int) (y * yscale * pointsize);
	} else if (point == -1) {
	    /* dot */
	    XDrawPoint(dpy, plot->pixmap, *current_gc, X(x), Y(y));
	} else {
	    unsigned char fill = 0;
	    unsigned char upside_down_fill = 0;
	    short upside_down_sign = 1;
	    if (plot->type != LineSolid || plot->lwidth != 0) {	/* select solid line */
		XSetLineAttributes(dpy, *current_gc, 0, LineSolid, CapButt, JoinBevel);
	    }
	    switch (point % 13) {
	    case 0:		/* do plus */
		Plus[0].x1 = (short) X(x) - plot->px;
		Plus[0].y1 = (short) Y(y);
		Plus[0].x2 = (short) X(x) + plot->px;
		Plus[0].y2 = (short) Y(y);
		Plus[1].x1 = (short) X(x);
		Plus[1].y1 = (short) Y(y) - plot->py;
		Plus[1].x2 = (short) X(x);
		Plus[1].y2 = (short) Y(y) + plot->py;

		XDrawSegments(dpy, plot->pixmap, *current_gc, Plus, 2);
		break;
	    case 1:		/* do X */
		Cross[0].x1 = (short) X(x) - plot->px;
		Cross[0].y1 = (short) Y(y) - plot->py;
		Cross[0].x2 = (short) X(x) + plot->px;
		Cross[0].y2 = (short) Y(y) + plot->py;
		Cross[1].x1 = (short) X(x) - plot->px;
		Cross[1].y1 = (short) Y(y) + plot->py;
		Cross[1].x2 = (short) X(x) + plot->px;
		Cross[1].y2 = (short) Y(y) - plot->py;

		XDrawSegments(dpy, plot->pixmap, *current_gc, Cross, 2);
		break;
	    case 2:		/* do star */
		Star[0].x1 = (short) X(x) - plot->px;
		Star[0].y1 = (short) Y(y);
		Star[0].x2 = (short) X(x) + plot->px;
		Star[0].y2 = (short) Y(y);
		Star[1].x1 = (short) X(x);
		Star[1].y1 = (short) Y(y) - plot->py;
		Star[1].x2 = (short) X(x);
		Star[1].y2 = (short) Y(y) + plot->py;
		Star[2].x1 = (short) X(x) - plot->px;
		Star[2].y1 = (short) Y(y) - plot->py;
		Star[2].x2 = (short) X(x) + plot->px;
		Star[2].y2 = (short) Y(y) + plot->py;
		Star[3].x1 = (short) X(x) - plot->px;
		Star[3].y1 = (short) Y(y) + plot->py;
		Star[3].x2 = (short) X(x) + plot->px;
		Star[3].y2 = (short) Y(y) - plot->py;

		XDrawSegments(dpy, plot->pixmap, *current_gc, Star, 4);
		break;
	    case 3:		/* do box */
		XDrawRectangle(dpy, plot->pixmap, *current_gc, X(x) - plot->px, Y(y) - plot->py,
		       	(plot->px + plot->px), (plot->py + plot->py));
		XDrawPoint(dpy, plot->pixmap, *current_gc, X(x), Y(y));
		break;
	    case 4:		/* filled box */
		XFillRectangle(dpy, plot->pixmap, *current_gc, X(x) - plot->px, Y(y) - plot->py,
		       	(plot->px + plot->px), (plot->py + plot->py));
		break;
	    case 5:		/* circle */
		XDrawArc(dpy, plot->pixmap, *current_gc, X(x) - plot->px, Y(y) - plot->py,
		       	2 * plot->px, 2 * plot->py, 0, 23040 /* 360 * 64 */);
		XDrawPoint(dpy, plot->pixmap, *current_gc, X(x), Y(y));
		break;
	    case 6:		/* filled circle */
		XFillArc(dpy, plot->pixmap, *current_gc, X(x) - plot->px, Y(y) - plot->py,
		       	2 * plot->px, 2 * plot->py, 0, 23040 /* 360 * 64 */);
		break;
	    case 10:		/* filled upside-down triangle */
		upside_down_fill = 1;
		/* FALLTHRU */
	    case 9:		/* do upside-down triangle */
		upside_down_sign = (short)-1;
	    case 8:		/* filled triangle */
		fill = 1;
		/* FALLTHRU */
	    case 7:		/* do triangle */
		{
		    short temp_x, temp_y;

		    temp_x = (short) (1.33 * (double) plot->px + 0.5);
		    temp_y = (short) (1.33 * (double) plot->py + 0.5);

		    Triangle[0].x = (short) X(x);
		    Triangle[0].y = (short) Y(y) - upside_down_sign * temp_y;
		    Triangle[1].x = (short) temp_x;
		    Triangle[1].y = (short) upside_down_sign * 2 * plot->py;
		    Triangle[2].x = (short) -(2 * temp_x);
		    Triangle[2].y = (short) 0;
		    Triangle[3].x = (short) temp_x;
		    Triangle[3].y = (short) -(upside_down_sign * 2 * plot->py);

		    if ((upside_down_sign == 1 && fill) || upside_down_fill) {
			XFillPolygon(dpy, plot->pixmap, *current_gc,
				Triangle, 4, Convex, CoordModePrevious);
		    } else {
			XDrawLines(dpy, plot->pixmap, *current_gc, Triangle, 4, CoordModePrevious);
			XDrawPoint(dpy, plot->pixmap, *current_gc, X(x), Y(y));
		    }
		}
		break;
	    case 12:		/* filled diamond */
		fill = 1;
		/* FALLTHRU */
	    case 11:		/* do diamond */
		Diamond[0].x = (short) X(x) - plot->px;
		Diamond[0].y = (short) Y(y);
		Diamond[1].x = (short) plot->px;
		Diamond[1].y = (short) -plot->py;
		Diamond[2].x = (short) plot->px;
		Diamond[2].y = (short) plot->py;
		Diamond[3].x = (short) -plot->px;
		Diamond[3].y = (short) plot->py;
		Diamond[4].x = (short) -plot->px;
		Diamond[4].y = (short) -plot->py;

		/*
		 * Should really do a check with XMaxRequestSize()
		 */

		if (fill) {
		    XFillPolygon(dpy, plot->pixmap, *current_gc,
			    Diamond, 5, Convex, CoordModePrevious);
		} else {
		    XDrawLines(dpy, plot->pixmap, *current_gc, Diamond, 5, CoordModePrevious);
		    XDrawPoint(dpy, plot->pixmap, *current_gc, X(x), Y(y));
		}
		break;
	    }
	    if (plot->type != LineSolid || plot->lwidth != 0) {	/* select solid line */
		XSetLineAttributes(dpy, *current_gc, plot->lwidth, plot->type, CapButt, JoinBevel);
	    }
	}
    }
#ifdef PM3D
    else if (*buffer == X11_GR_SET_COLOR) {	/* set color */
	if (have_pm3d) {	/* ignore, if your X server is not supported */
	    double gray;
	    sscanf(buffer + 1, "%lf", &gray);
	    PaletteSetColor(plot, gray);
	    current_gc = &gc_pm3d;
	}
    } else if (*buffer == X11_GR_FILLED_POLYGON) {	/* filled polygon */
	if (have_pm3d) {	/* ignore, if your X server is not supported */
	    static XPoint *points = NULL;
	    static int st_npoints = 0;
	    static int saved_npoints = -1, saved_i = -1;	/* HBB 20010919 */
	    int i, npoints;
	    char *ptr = buffer + 1;

	    sscanf(ptr, "%4d", &npoints);

	    /* HBB 20010919: Implement buffer overflow protection by
	     * breaking up long lines */
	    if (npoints == -1) {
		/* This is a continuation line. */
		if (saved_npoints < 100) {
		    fprintf(stderr, "gnuplot_x11: filled_polygon() protocol error\n");
		    EXIT(1);
		}
		/* Continue filling at end of previous list: */
		i = saved_i;
		npoints = saved_npoints;
	    } else {
		saved_npoints = npoints;
		i = 0;
	    }

	    ptr += 4;
	    if (npoints > st_npoints) {
		XPoint *new_points = realloc(points, sizeof(XPoint) * npoints);
		st_npoints = npoints;
		if (!new_points) {
		    perror("gnuplot_x11: exec_cmd()->points");
		    EXIT(1);
		}
		points = new_points;
	    }

	    while (*ptr != 'x' && i < npoints) {	/* not end-of-line marker */
		sscanf(ptr, "%4d%4d", &x, &y);
		ptr += 8;
		points[i].x = X(x);
		points[i].y = Y(y);
		i++;
	    }

	    if (i >= npoints) {
		/* only do the call if list is complete by now */
		XFillPolygon(dpy, plot->pixmap, *current_gc, points, npoints,
			     Nonconvex, CoordModeOrigin);
		/* Flag this continuation line as closed */
		saved_npoints = saved_i = -1;
	    } else {
		/* Store how far we got: */
		saved_i = i;
	    }
	}
    }
#endif
#if defined(USE_MOUSE) && defined(MOUSE_ALL_WINDOWS)
    /*   Axis scaling information to save for later mouse clicks */
    else if (*buffer == 'S') {
	int axis, axis_mask;

	sscanf(&buffer[1], "%d %d", &axis, &axis_mask);
	if (axis < 0) {
	    plot->axis_mask = axis_mask;
	} else if (axis < 2*SECOND_AXES) {
	    sscanf(&buffer[1], "%d %lg %d %lg %lg", &axis,
		&(plot->axis_scale[axis].min), &(plot->axis_scale[axis].term_lower),
		&(plot->axis_scale[axis].term_scale), &(plot->axis_scale[axis].logbase));
	    FPRINTF((stderr,"gnuplot_x11: axis %d scaling %14.3lg %14d %14.3lg %14.3lg\n",
		axis, plot->axis_scale[axis].min, plot->axis_scale[axis].term_lower,
		plot->axis_scale[axis].term_scale, plot->axis_scale[axis].logbase));
	}
    }
#endif
    else {
	fprintf(stderr, "gnuplot_x11: unknown command <%s>\n", buffer);
    }
}

/*
 *   display - display a stored plot
 */
static void
display(plot_struct *plot)
{
    int n;

#ifdef PM3D
    if (TRUE == plot->release_cmap) {
	/* no pm3d palette was allocated, so
	 * switch back to the default cmap */
	plot->release_cmap = FALSE;
	ReleaseColormap(plot);
	sm_palette.colorMode = SMPAL_COLOR_MODE_NONE;
    }
#endif

    FPRINTF((stderr, "Display %d ; %d commands\n", plot->plot_number, plot->ncommands));

    if (plot->ncommands == 0)
	return;

    /* set scaling factor between internal driver & window geometry */
    xscale = plot->width / 4096.0;
    yscale = GRAPH_HEIGHT(plot) / 4096.0;

    /* initial point sizes, until overridden with P7xxxxyyyy */
    plot->px = (int) (xscale * pointsize);
    plot->py = (int) (yscale * pointsize);

    /* create new pixmap & GC */
    if (!plot->pixmap) {
	FPRINTF((stderr, "Create pixmap %d : %dx%dx%d\n", plot->plot_number, plot->width, PIXMAP_HEIGHT(plot), dep));
	plot->pixmap = XCreatePixmap(dpy, root, plot->width, PIXMAP_HEIGHT(plot), dep);
    }

    if (gc)
	XFreeGC(dpy, gc);

    gc = XCreateGC(dpy, plot->pixmap, 0, (XGCValues *) 0);

    XSetFont(dpy, gc, font->fid);
#if USE_ULIG_FILLEDBOXES
    XSetFillStyle(dpy, gc, FillSolid);

    /* initialize stipple for filled boxes (ULIG) */
    if (!stipple_initialized) {
	int i;
	for (i = 0; i < stipple_halftone_num; i++)
	    stipple_halftone[i] =
		XCreateBitmapFromData(dpy, plot->pixmap, stipple_halftone_bits[i], stipple_halftone_width, stipple_halftone_height);
	for (i = 0; i < stipple_pattern_num; i++)
	    stipple_pattern[i] =
		XCreateBitmapFromData(dpy, plot->pixmap, stipple_pattern_bits[i], stipple_pattern_width, stipple_pattern_height);
	stipple_initialized = 1;
    }
#endif /* USE_ULIG_FILLEDBOXES */

    /* set pixmap background */
    XSetForeground(dpy, gc, plot->cmap->colors[0]);
    XFillRectangle(dpy, plot->pixmap, gc, 0, 0, plot->width, PIXMAP_HEIGHT(plot) + vchar);
    XSetBackground(dpy, gc, plot->cmap->colors[0]);

    if (!plot->window) {
	pr_window(plot);
    }
    /* top the window but don't put keyboard or mouse focus into it. */
    if (do_raise)
	XMapRaised(dpy, plot->window);

    /* momentarily clear the window first if requested */
    if (Clear) {
	XClearWindow(dpy, plot->window);
	XFlush(dpy);
    }
    /* loop over accumulated commands from inboard driver */
    for (n = 0; n < plot->ncommands; n++) {
	exec_cmd(plot, plot->commands[n]);
    }

#ifdef EXPORT_SELECTION
    export_graph(plot);
#endif

    UpdateWindow(plot);
#ifdef USE_MOUSE
#ifdef TITLE_BAR_DRAWING_MSG
    if (plot->window) {
	/* restore default window title */
	char *cp = plot->titlestring;
	if (!cp) cp = "";
	XStoreName(dpy, plot->window, cp);
    }
#else
    if (!button_pressed) {
	cursor = cursor_save ? cursor_save : cursor_default;
	cursor_save = (Cursor)0;
	XDefineCursor(dpy, plot->window, cursor);
    }
#endif
#endif
}

static void
UpdateWindow(plot_struct * plot)
{
#ifdef USE_MOUSE
    XEvent event;
#endif
    if (None == plot->window) {
	return;
    }

    if (!plot->pixmap) {
	/* create a black background pixmap */
	FPRINTF((stderr, "Create pixmap %d : %dx%dx%d\n", plot->plot_number, plot->width, PIXMAP_HEIGHT(plot), dep));
	plot->pixmap = XCreatePixmap(dpy, root, plot->width, PIXMAP_HEIGHT(plot), dep);
	if (gc)
	    XFreeGC(dpy, gc);
	gc = XCreateGC(dpy, plot->pixmap, 0, (XGCValues *) 0);
	XSetFont(dpy, gc, font->fid);
	/* set pixmap background */
	XSetForeground(dpy, gc, plot->cmap->colors[0]);
	XFillRectangle(dpy, plot->pixmap, gc, 0, 0, plot->width, PIXMAP_HEIGHT(plot) + vchar);
	XSetBackground(dpy, gc, plot->cmap->colors[0]);
    }
    XSetWindowBackgroundPixmap(dpy, plot->window, plot->pixmap);
    XClearWindow(dpy, plot->window);

#ifdef USE_MOUSE
    EventuallyDrawMouseAddOns(plot);

    XFlush(dpy);

    /* XXX discard expose events. This is a kludge for
     * preventing the event dispatcher calling UpdateWindow()
     * and the latter again generating expose events, which
     * again would trigger the event dispatcher ... (joze) XXX */
    while (XCheckWindowEvent(dpy, plot->window, ExposureMask, &event))
	/* EMPTY */ ;
#endif
}

#ifdef PM3D

static void
CmapClear(cmap_t * cmap_ptr)
{
    cmap_ptr->total = (int) 0;
    cmap_ptr->allocated = (int) 0;
    cmap_ptr->pixels = (unsigned long *) 0;
}

static void
GetGCpm3d(plot_struct * plot, GC * ret)
{
    XGCValues values;
    unsigned long mask = 0;

    mask = GCForeground | GCBackground;
    values.foreground = WhitePixel(dpy, scr);
    values.background = BlackPixel(dpy, scr);

    *ret = XCreateGC(dpy, plot->window, mask, &values);
}

static void
RecolorWindow(plot_struct * plot)
{
    if (None != plot->window) {
	XSetWindowColormap(dpy, plot->window, plot->cmap->colormap);
	XSetWindowBackground(dpy, plot->window, plot->cmap->colors[0]);
	XSetWindowBorder(dpy, plot->window, plot->cmap->colors[1]);
#ifdef USE_MOUSE
	if (gc_xor) {
	    XFreeGC(dpy, gc_xor);
	    gc_xor = (GC) 0;
	    GetGCXor(plot, &gc_xor);	/* recreate gc_xor */
	}
#endif
    }
}

/*
 * free all *pm3d* colors (*not* the line colors cmap->colors)
 * of a plot_struct's colormap.  This could be either a private
 * or the default colormap.  Note, that the line colors are not
 * free'd nor even touched.
 */
static void
FreeColors(plot_struct * plot)
{
    if (plot->cmap->total && plot->cmap->pixels) {
	if (plot->cmap->allocated) {
	    FPRINTF((stderr, "freeing palette colors\n"));
	    XFreeColors(dpy, plot->cmap->colormap, plot->cmap->pixels,
			plot->cmap->allocated, 0 /* XXX ??? XXX */ );
	}
	free(plot->cmap->pixels);
    }
    CmapClear(plot->cmap);
}

/*
 * free pm3d colors and eventually a private colormap.
 * set the plot_struct's colormap to the default colormap
 * and the line `colors' to the line colors of the default
 * colormap.
 */
static void
ReleaseColormap(plot_struct * plot)
{
    FreeColors(plot);
    if (plot->cmap && plot->cmap != &cmap) {
	fprintf(stderr, "releasing private colormap\n");
	if (plot->cmap->colormap && plot->cmap->colormap != cmap.colormap) {
	    XFreeColormap(dpy, plot->cmap->colormap);
	}
	free((char *) plot->cmap);
	plot->cmap = &cmap;	/* switching to default colormap */
	RecolorWindow(plot);
    }
}

static unsigned long *
ReallocColors(plot_struct * plot, int n)
{
    FreeColors(plot);
    plot->cmap->total = n;
    plot->cmap->pixels = (unsigned long *)
	malloc(sizeof(unsigned long) * plot->cmap->total);
    return plot->cmap->pixels;
}

/*
 * check if the display supports the visual of type `class'.
 *
 * If multiple visuals of `class' are supported, try to get
 * the one with the highest depth.
 *
 * If visual class and depth are equal to the default visual
 * class and depth, the latter is preferred.
 *
 * modifies: best, depth
 *
 * returns 1 if a visual which matches the request
 * could be found else 0.
 */
static int
GetVisual(int class, Visual ** visual, int *depth)
{
    XVisualInfo *visualsavailable;
    int nvisuals = 0;
    long vinfo_mask = VisualClassMask;
    XVisualInfo vinfo;

    vinfo.class = class;
    *depth = 0;
    *visual = 0;

    visualsavailable = XGetVisualInfo(dpy, vinfo_mask, &vinfo, &nvisuals);

    if (visualsavailable && nvisuals > 0) {
	int i;
	for (i = 0; i < nvisuals; i++) {
	    if (visualsavailable[i].depth > *depth) {
		*visual = visualsavailable[i].visual;
		*depth = visualsavailable[i].depth;
	    }
	}
	XFree(visualsavailable);
	if (*visual && (*visual)->class == (DefaultVisual(dpy, scr))->class && *depth == DefaultDepth(dpy, scr)) {
	    /* prefer the default visual */
	    *visual = DefaultVisual(dpy, scr);
	}
    }
    return nvisuals > 0;
}

static void
PaletteMake(plot_struct * plot, t_sm_palette * tpal)
{
    static int virgin = yes;
    static int recursion = 0;
    int max_colors;
    int min_colors;
    char *save_title = (char *) 0;

#ifdef PM3D
    /* don't release this cmap at the first drawing operation */
    plot->release_cmap = FALSE;
#endif

    if (tpal && tpal->use_maxcolors > 0)
	max_colors = tpal->use_maxcolors;
    else
	max_colors = maximal_possible_colors;

    if (minimal_possible_colors < max_colors) 
        min_colors = minimal_possible_colors; 
    else
        min_colors = max_colors / (num_colormaps > 1 ? 2 : 8);

    if (tpal) {
	FPRINTF((stderr, "(PaletteMake) tpal->use_maxcolors = %d\n", 
		 tpal->use_maxcolors));
    } else {
	FPRINTF((stderr, "(PaletteMake) tpal=NULL\n"));
    }

    /* reallocate the palette
     * only if it has changed.
     */
    if (tpal) {
        /*  make sure they do not differ by unused members of palette struct */
	sm_palette.colorFormulae = tpal-> colorFormulae = 1;
    }

    if (no == virgin && tpal && !palettes_differ(tpal, &sm_palette)) {
	FPRINTF((stderr, "(PaletteMake) palette didn't change.\n"));
	return;
    } else if (tpal) {
        /*  free old gradient table  */
        if (sm_palette.gradient) {
	    free( sm_palette.gradient );
	    sm_palette.gradient = NULL;
	    sm_palette.gradient_num = 0;
	}

	sm_palette.colorMode = tpal->colorMode;
	sm_palette.positive = tpal->positive;
	sm_palette.use_maxcolors = tpal->use_maxcolors;
	sm_palette.cmodel = tpal->cmodel;
	
	virgin = no;
	switch( sm_palette.colorMode ) {
          case SMPAL_COLOR_MODE_GRAY:  
	    sm_palette.gamma = tpal->gamma;
	    break;
	  case SMPAL_COLOR_MODE_RGB:
	    sm_palette.formulaR = tpal->formulaR;
	    sm_palette.formulaG = tpal->formulaG;
	    sm_palette.formulaB = tpal->formulaB;
	    break;
        case SMPAL_COLOR_MODE_FUNCTIONS:
	  fprintf( stderr, "Ooops:  no SMPAL_COLOR_MODE_FUNCTIONS here!\n" );
	  break;
	case SMPAL_COLOR_MODE_GRADIENT: 
	  sm_palette.gradient_num = tpal->gradient_num;
	  sm_palette.gradient = tpal->gradient;
	  break;
	default:
	  fprintf(stderr,"%s:%d ooops: Unknown color mode '%c'.\n",
		  __FILE__, __LINE__, (char)(sm_palette.colorMode) );
	}
    }

    if (!have_pm3d) {
	return;
    }

    if (!plot->window) {
	fprintf(stderr, "(PaletteMake) calling pr_window()\n");
	pr_window(plot);
    }


    if (plot->window) {
	char *msg;
	char *added_text = " allocating colors ...";
	int orig_len = (plot->titlestring ? strlen(plot->titlestring) : 0);
	XFetchName(dpy, plot->window, &save_title);
	if ((msg = (char *) malloc(orig_len + strlen(added_text) + 1))) {
	    if (plot->titlestring)
		strcpy(msg, plot->titlestring);
	    else
		msg[0] = '\0';
	    strcat(msg, added_text);
	    XStoreName(dpy, plot->window, msg);
	    free(msg);
	}
    }

    if (!gc_pm3d) {
	GetGCpm3d(plot, &gc_pm3d);
    }

    if (!num_colormaps) {
	XFree(XListInstalledColormaps(dpy, plot->window, &num_colormaps));
	FPRINTF((stderr, "(PaletteMake) num_colormaps = %d\n", num_colormaps));
    }

    /* TODO */
    /* EventuallyChangeVisual(plot); */

    /*
     * start with trying to allocate max_colors. This should 
     * always succeed with TrueColor visuals >= 16bit. If it
     * fails (for example for a PseudoColor visual of depth 8),
     * try it with half of the colors. Proceed until min_colors
     * is reached. If this fails we should probably install a
     * private colormap.
     * Note that I make no difference for different
     * visual types here. (joze)
     */
    for ( /* EMPTY */ ; max_colors >= min_colors; max_colors /= 2) {

	XColor xcolor;
	double fact = 1.0 / (double)(max_colors-1);

	ReallocColors(plot, max_colors);

	for (plot->cmap->allocated = 0; plot->cmap->allocated < max_colors; plot->cmap->allocated++) {

	    double gray = (double) plot->cmap->allocated * fact;
	    rgb_color color;
	    rgb1_from_gray( gray, &color );
	    xcolor.red = 0xffff * color.r + 0.5;
	    xcolor.green = 0xffff * color.g + 0.5;
	    xcolor.blue = 0xffff * color.b + 0.5;
	    
	    if (XAllocColor(dpy, plot->cmap->colormap, &xcolor)) {
		plot->cmap->pixels[plot->cmap->allocated] = xcolor.pixel;
	    } else {
		FPRINTF((stderr, "failed at color %d\n", plot->cmap->allocated));
		break;
	    }
	}

	if (plot->cmap->allocated == max_colors) {
	    break;		/* success! */
	}

	/* reduce the number of max_colors to at
	 * least less than plot->cmap->allocated */
	while (max_colors > plot->cmap->allocated && max_colors >= min_colors) {
	    max_colors /= 2;
	}
    }

    FPRINTF((stderr, "(PaletteMake) allocated = %d\n", plot->cmap->allocated));
    FPRINTF((stderr, "(PaletteMake) max_colors = %d\n", max_colors));
    FPRINTF((stderr, "(PaletteMake) min_colors = %d\n", min_colors));

    if (plot->cmap->allocated < min_colors && !recursion) {
	ReleaseColormap(plot);
	/* create a private colormap. */
	fprintf(stderr, "switching to private colormap\n");
	plot->cmap = (cmap_t *) malloc(sizeof(cmap_t));
	assert(plot->cmap);
	CmapClear(plot->cmap);
	plot->cmap->colormap = XCreateColormap(dpy, root, vis, AllocNone);
	assert(plot->cmap->colormap);
	pr_color(plot->cmap);	/* set default colors for lines */
	RecolorWindow(plot);
	recursion = 1;
	PaletteMake(plot, (t_sm_palette *) 0);
    } else {
	/* this is just for calculating the number of unique colors */
	int i;
	unsigned long previous = plot->cmap->allocated ? plot->cmap->pixels[0] : 0;
	int unique_colors = 1;
	for (i = 0; i < plot->cmap->allocated; i++) {
	    if (plot->cmap->pixels[i] != previous) {
		previous = plot->cmap->pixels[i];
		unique_colors++;
	    }
	}
    }

    if (plot->window && save_title) {
	/* restore default window title */
	XStoreName(dpy, plot->window, save_title);
    }
    if (save_title) {
	XFree(save_title);
    }

    recursion = 0;
}

static void
PaletteSetColor(plot_struct * plot, double gray)
{
    if (plot->cmap->allocated) {
	int index = gray * (plot->cmap->allocated - 1);
	XSetForeground(dpy, gc_pm3d, plot->cmap->pixels[index]);
    }
}
#endif /* PM3D */

#ifdef USE_MOUSE

static int
ErrorHandler(Display * display, XErrorEvent * error_event)
{
    /* Don't remove directly.  Main program might be using the memory. */
    (void) display;		/* avoid -Wunused warnings */
    Add_Plot_To_Remove_FIFO_Queue((Window) error_event->resourceid);
    gp_exec_event(GE_reset, 0, 0, 0, 0);
    return 0;
}

static void
DrawRuler(plot_struct * plot)
{
    if (plot->ruler_on) {
	int x = X(plot->ruler_x);
	int y = Y(plot->ruler_y);
	if (!gc_xor) {
	    /* create a gc for `rubberbanding' (well ...) */
	    GetGCXor(plot, &gc_xor);
	}
	/* vertical line */
	XDrawLine(dpy, plot->window, gc_xor, x, 0, x, GRAPH_HEIGHT(plot));
	/* horizontal line */
	XDrawLine(dpy, plot->window, gc_xor, 0, y, plot->width, y);
    }
}

static void
EventuallyDrawMouseAddOns(plot_struct * plot)
{
    DrawRuler(plot);
    if (plot->zoombox_on)
	DrawBox(plot);
    DrawCoords(plot, plot->str);
    /*
       TODO more ...
     */
}



/*
 * draw a box using the gc with the GXxor function.
 * This can be used to turn on *and off* a box. The
 * corners of the box are annotated with the strings
 * stored in the plot structure.
 */
static void
DrawBox(plot_struct * plot)
{
    int width;
    int height;
    int X0 = plot->zoombox_x1;
    int Y0 = plot->zoombox_y1;
    int X1 = plot->zoombox_x2;
    int Y1 = plot->zoombox_y2;

    if (!gc_xor_dashed) {
	GetGCXorDashed(plot, &gc_xor_dashed);
    }

    if (X1 < X0) {
	int tmp = X1;
	X1 = X0;
	X0 = tmp;
    }

    if (Y1 < Y0) {
	int tmp = Y1;
	Y1 = Y0;
	Y0 = tmp;
    }

    width = X1 - X0;
    height = Y1 - Y0;

    XDrawRectangle(dpy, plot->window, gc_xor_dashed, X0, Y0, width, height);

    if (plot->zoombox_str1a[0] || plot->zoombox_str1b[0])
	AnnotatePoint(plot, plot->zoombox_x1, plot->zoombox_y1, plot->zoombox_str1a, plot->zoombox_str1b);
    if (plot->zoombox_str2a[0] || plot->zoombox_str2b[0])
	AnnotatePoint(plot, plot->zoombox_x2, plot->zoombox_y2, plot->zoombox_str2a, plot->zoombox_str2b);
}


/*
 * draw the strings xstr and ystr centered horizontally
 * and vertically at the point x, y. Use the GXxor
 * as usually, so that we can also remove the coords
 * later.
 */
static void
AnnotatePoint(plot_struct * plot, int x, int y, const char xstr[], const char ystr[])
{
    int ylen, xlen;
    int xwidth, ywidth;

    xlen = strlen(xstr);
    xwidth = XTextWidth(font, xstr, xlen);

    ylen = strlen(ystr);
    ywidth = XTextWidth(font, ystr, ylen);

    /* horizontal centering disabled (joze) */

    if (!gc_xor) {
	GetGCXor(plot, &gc_xor);
    }
    XDrawString(dpy, plot->window, gc_xor, x, y - 3, xstr, xlen);
    XDrawString(dpy, plot->window, gc_xor, x, y + vchar, ystr, ylen);
}

/* returns the time difference to the last click in milliseconds */
static long int
SetTime(plot_struct *plot, Time t)
{
    long int diff = t - plot->time;

    FPRINTF((stderr, "(SetTime) difftime = %ld\n", diff));

    plot->time = t;
    return diff > 0 ? diff : 0;
}

static unsigned long
AllocateXorPixel(cmap_t *cmap_ptr)
{
    unsigned long pixel;
    XColor xcolor;

    xcolor.pixel = cmap_ptr->colors[0];	/* background color */
    XQueryColor(dpy, cmap_ptr->colormap, &xcolor);

    if (xcolor.red + xcolor.green + xcolor.blue < 0xffff) {
	/* it is admittedly somehow arbitrary to call
	 * everything with a gray value < 0xffff a 
	 * dark background. Try to use the background's
	 * complement for drawing which will always
	 * result in white when using xor. */
	xcolor.red = ~xcolor.red;
	xcolor.green = ~xcolor.green;
	xcolor.blue = ~xcolor.blue;
	if (XAllocColor(dpy, cmap_ptr->colormap, &xcolor)) {
	    /* use white foreground for dark backgrounds */
	    pixel = xcolor.pixel;
	} else {
	    /* simple xor if we've run out of colors. */
	    pixel = WhitePixel(dpy, scr);
	}
    } else {
	/* use the background itself for drawing.
	 * xoring two same colors will always result
	 * in black. This color is already allocated. */
	pixel = xcolor.pixel;
    }
#ifdef PM3D
    cmap_ptr->xorpixel = pixel;
#endif
    return pixel;
}

static void
GetGCXor(plot_struct * plot, GC * ret)
{
    XGCValues values;
    unsigned long mask = 0;

    values.foreground = AllocateXorPixel(plot->cmap);

    mask = GCForeground | GCFunction | GCFont;
    values.function = GXxor;
    values.font = font->fid;

    *ret = XCreateGC(dpy, plot->window, mask, &values);
}

static void
GetGCXorDashed(plot_struct * plot, GC * gc)
{
    GetGCXor(plot, gc);
    XSetLineAttributes(dpy, *gc, 0,	/* line width, X11 treats 0 as a `thin' line */
		       LineOnOffDash,	/* also: LineDoubleDash */
		       CapNotLast,	/* also: CapButt, CapRound, CapProjecting */
		       JoinMiter /* also: JoinRound, JoinBevel */ );
}

#if 0
/*
 * returns the newly created gc
 * pixmap: where the gc will be used
 * mode == 0 --> black on white
 * mode == 1 --> white on black
 */
/* FIXME HBB 20020225: This function is not used anywhere ??? */
static void
GetGCBlackAndWhite(plot_struct * plot, GC * ret, Pixmap pixmap, int mode)
{
    XGCValues values;
    unsigned long mask = 0;

    mask = GCForeground | GCBackground | GCFont | GCFunction;
    if (!mode) {
#if 0
	values.foreground = BlackPixel(dpy, scr);
	values.background = WhitePixel(dpy, scr);
#else
	values.foreground = plot->cmap->colors[1];
	values.background = plot->cmap->colors[0];
#endif
    } else {
	/*
	 * swap colors
	 */
#if 0
	values.foreground = WhitePixel(dpy, scr);
	values.background = BlackPixel(dpy, scr);
#else
	values.foreground = plot->cmap->colors[0];
	values.background = plot->cmap->colors[1];
#endif
    }
    values.function = GXcopy;
    values.font = font->fid;

    *ret = XCreateGC(dpy, pixmap, mask, &values);
}

/*
 * split a string at `splitchar'.
 */
/* FIXME HBB 20020225: This function is not used anywhere ??? */
static int
SplitAt(char **args, int maxargs, char *buf, char splitchar)
{
    int argc = 0;

    while (*buf != '\0' && argc < maxargs) {

	if ((*buf == splitchar))
	    *buf++ = '\0';

	if (!(*buf))		/* don't count the terminating NULL */
	    break;

	/* Save the argument.  */
	*args++ = buf;
	argc++;

	/* Skip over the argument */
	while ((*buf != '\0') && (*buf != splitchar))
	    buf++;
    }

    *args = '\0';		/* terminate */
    return argc;
}

/* FIXME HBB 20020225: This function is not used anywhere ??? */
static void
xfree(void *fred)
{
    if (fred)
	free(fred);
}
#endif

/* erase the last displayed position string */
static void
EraseCoords(plot_struct * plot)
{
    DrawCoords(plot, plot->str);
}



static void
DrawCoords(plot_struct * plot, const char *str)
{
    if (!gc_xor) {
	GetGCXor(plot, &gc_xor);
    }

    if (str[0] != 0)
	XDrawString(dpy, plot->window, gc_xor, 1, plot->gheight + vchar - 1, str, strlen(str));
}


/* display text (e.g. mouse position) in the lower left corner of the window. */
static void
DisplayCoords(plot_struct * plot, const char *s)
{
    /* first, erase old text */
    EraseCoords(plot);

    if (s[0] == 0) {
	/* no new text? */
	if (plot->height > plot->gheight) {
	    /* and window has space for text? then make it smaller, unless we're already doing a resize: */
	    if (!plot->resizing) {
		XResizeWindow(dpy, plot->window, plot->width, plot->gheight);
		plot->resizing = TRUE;
	    }
	}
    } else {
	/* so we do have new text */
	if (plot->height == plot->gheight) {
	    /* window not large enough? then make it larger, unless we're already doing a resize: */
	    if (!plot->resizing) {
		XResizeWindow(dpy, plot->window, plot->width, plot->gheight + vchar);
		plot->resizing = TRUE;
	    }
	}
    }

    /* finally, draw the new text: */
    DrawCoords(plot, s);

    /* and save it, for later erasing: */
    strcpy(plot->str, s);
}


#if 0
/* FIXME HBB 20020225: This function is not used anywhere ??? */
static int
is_control(KeySym mod)
{
    return (XK_Control_R == mod || XK_Control_L == mod);
}

/* FIXME HBB 20020225: This function is not used anywhere ??? */
static int
is_meta(KeySym mod)
{
    /* we make no difference between alt and meta */
    return (XK_Meta_R == mod || XK_Meta_L == mod
	    || XK_Alt_R == mod || XK_Alt_L == mod);
}

/* FIXME HBB 20020225: This function is not used anywhere ??? */
static int
is_shift(KeySym mod)
{
    return (XK_Shift_R == mod || XK_Shift_L == mod);
}
#endif


/* It returns NULL if we are not running in any known (=implemented) multitab
 * console.
 * Otherwise it returns a command to be executed in order to switch to the
 * appropriate tab of a multitab console.
 * In addition, it may return non-zero newGnuplotXID in order to overwrite zero
 * value of gnuplotXID (because Konsole's in KDE <3.2 don't set WINDOWID contrary
 * to all other xterm's).
 * Currently implemented for:
 *	- KDE's Konsole.
 * Note: if the returned command is !NULL, then it must be free()'d by the caller.
 */
static char*
getMultiTabConsoleSwitchCommand(unsigned long *newGnuplotXID)
{
    char *cmd = NULL; /* result */
    char *ptr = getenv("KONSOLE_DCOP_SESSION"); /* Try KDE's Konsole first. */
    *newGnuplotXID = 0;
    if (ptr) {
	/* We are in KDE's Konsole, or in a terminal window detached from a Konsole.
	 * In order to active a tab:
	 * 1. get environmental variable KONSOLE_DCOP_SESSION: it includes konsole id and session name
	 * 2. if
	 *	$WINDOWID is defined and it equals
	 *	    `dcop konsole-3152 konsole-mainwindow#1 getWinID`
	 *	(KDE 3.2) or when $WINDOWID is undefined (KDE 3.1), then run commands
	 *    dcop konsole-3152 konsole activateSession session-2; \
	 *    dcop konsole-3152 konsole-mainwindow#1 raise
	 * Note: by $WINDOWID we mean gnuplot's text console WINDOWID.
	 * Missing: focus is not transferred unless $WINDOWID is defined (should be fixed in KDE 3.2).
	 *
	 * Implementation and tests on KDE 3.1.4: Petr Mikulik.
	 */
	char *konsole_name = NULL;
	*newGnuplotXID = 0; /* don't change gnuplotXID by default */
	/* use 'while' instead of 'if' to easily break out (aka catch exception) */
	while (1) {
	    char *konsole_tab;
	    unsigned long w;
	    FILE *p;
	    ptr = strchr(ptr, '(');
	    /* the string for tab nb 4 looks like 'DCOPRef(konsole-2866,session-4)' */
	    if (!ptr) return NULL;
	    konsole_name = strdup(ptr+1);
	    konsole_tab = strchr(konsole_name, ',');
	    if (!konsole_tab) break;
	    *konsole_tab++ = 0;
	    ptr = strchr(konsole_tab, ')');
	    if (ptr) *ptr = 0;
#if 0
	    fprintf(stderr, "konsole_name = |%s|\n", konsole_name);
	    fprintf(stderr, "konsole_tab  = |%s|\n", konsole_tab);
#endif
	    /* Not necessary to define DCOP_RAISE: returning newly known
	     * newGnuplotXID instead is sufficient.
	     */
/* #define DCOP_RAISE */
#ifdef DCOP_RAISE
	    cmd = malloc(2*strlen(konsole_name) + strlen(konsole_tab) + 128);
#else
	    cmd = malloc(strlen(konsole_name) + strlen(konsole_tab) + 64);
#endif
	    sprintf(cmd, "dcop %s konsole-mainwindow#1 getWinID 2>/dev/null", konsole_name);
      		/* is  2>/dev/null  portable among various shells? */
	    p = popen(cmd, "r");
	    if (p) {
		fscanf(p, "%lu", &w);
		pclose(p);
	    }
	    if (gnuplotXID) { /* $WINDOWID is known */
		if (w != gnuplotXID) break;
		    /* `dcop getWinID`==$WINDOWID thus we are running in a window detached from Konsole */
	    } else {
		*newGnuplotXID = w;
		    /* $WINDOWID has not been known (KDE 3.1), thus set it up */
	    }
#ifdef DCOP_RAISE
	    /* not necessary: returning newly known newGnuplotXID instead is sufficient */
	    sprintf(cmd, "dcop %s konsole-mainwindow#1 raise;", konsole_name);
	    sprintf(cmd+strlen(cmd), "dcop %s konsole activateSession %s", konsole_name, konsole_tab);
#else
	    sprintf(cmd, "dcop %s konsole activateSession %s", konsole_name, konsole_tab);
#endif
	    free(konsole_name);
	    return cmd;
	}
	free(konsole_name);
	free(cmd);
	return NULL;
    }
    /* now test for GNOME multitab console */
    /* ... if somebody bothers to implement it ... */
    /* we are not running in any known (implemented) multitab console */
    return NULL;
}

#endif


/*---------------------------------------------------------------------------
 *  reset all cursors (since we dont have a record of the previous terminal #)
 *---------------------------------------------------------------------------*/

static void
reset_cursor()
{
    plot_struct *plot = list_start;

    while (plot) {
	if (plot->window) {
	    FPRINTF((stderr, "Window for plot %d exists\n", plot->plot_number));
	    XUndefineCursor(dpy, plot->window);
	}
	plot = plot->next_plot;
    }

    FPRINTF((stderr, "Cursors reset\n"));
    return;
}

/*-----------------------------------------------------------------------------
 *   resize - rescale last plot if window resized
 *---------------------------------------------------------------------------*/

#ifdef USE_MOUSE

/* the status of the shift, ctrl and alt keys
*/
static int modifier_mask = 0;

static void update_modifiers __PROTO((unsigned int));

static void
update_modifiers(unsigned int state)
{
    int old_mod_mask;

    old_mod_mask = modifier_mask;
    modifier_mask = ((state & ShiftMask) ? Mod_Shift : 0)
	| ((state & ControlMask) ? Mod_Ctrl : 0)
	| ((state & Mod1Mask) ? Mod_Alt : 0);
    if (old_mod_mask != modifier_mask) {
	gp_exec_event(GE_modifier, 0, 0, modifier_mask, 0);
    }
}

#endif


static void
process_event(XEvent *event)
{
#if 0
    static char key_string[0xf];
#endif
    plot_struct *plot;
    KeySym keysym;
#ifdef USE_MOUSE
#if 0
    static int border = 0;
    static int aspect_ratio = 0;
    int old_mod_mask = 0;
    FPRINTF((stderr, "Event 0x%x\n", event->type));
#endif
#endif

    switch (event->type) {
    case ConfigureNotify:
	plot = Find_Plot_In_Linked_List_By_Window(event->xconfigure.window);
	if (plot) {
	    int w = event->xconfigure.width, h = event->xconfigure.height;

	    /* store settings in case window is closed then recreated */
	    /* but: don't do this if both x and y are 0, since some
	     * (all?) systems set these to zero when only resizing
	     * (not moving) the window. This does mean that a move to
	     * (0,0) won't be registered: can we solve that? */
	    if (event->xconfigure.x != 0 || event->xconfigure.y != 0) {
		plot->x = event->xconfigure.x;
		plot->y = event->xconfigure.y;
		plot->posn_flags = (plot->posn_flags & ~PPosition) | USPosition;
	    }
#ifdef USE_MOUSE
	    /* first, check whether we were waiting
	     * for completion of a resize */
	    if (plot->resizing) {
		/* it seems to be impossible to distinguish between a
		 * resize caused by our call to XResizeWindow(), and a
		 * resize started by the user/windowmanager; but we can
		 * make a good guess which can only fail if the user
		 * resizes the window while we're also resizing it
		 * ourselves: */
		if (w == plot->width && (h == plot->gheight || h == plot->gheight + vchar)) {
		    /* most likely, it's a resize for showing/hiding
		     * the status line: test whether the height is now
		     * correct; if not, start another resize: */
		    if (w == plot->width && h == plot->gheight + (plot->str[0] ? vchar : 0)) {
			plot->resizing = FALSE;
		    } else {
			XResizeWindow(dpy, plot->window, plot->width, plot->gheight + (plot->str[0] ? vchar : 0));
		    }
		    plot->height = h;
		    break;
		}
		plot->resizing = FALSE;
	    }
#endif

	    if (w > 1 && h > 1 && (w != plot->width || h != plot->height)) {

		plot->width = w;
		plot->height = h;
#ifdef USE_MOUSE
		if (plot->str[0])
		    /* Make sure that unsigned number doesn't underflow. */
		    plot->gheight = (vchar > plot->height) ? 0 : plot->height - vchar;
		else
		    plot->gheight = plot->height;
#endif
		plot->posn_flags = (plot->posn_flags & ~PSize) | USSize;
#if USE_ULIG_FILLEDBOXES
		if (stipple_initialized) {	/* ULIG */
		    int i;
		    for (i = 0; i < stipple_halftone_num; i++)
			XFreePixmap(dpy, stipple_halftone[i]);
		    for (i = 0; i < stipple_pattern_num; i++)
			XFreePixmap(dpy, stipple_pattern[i]);
		    stipple_initialized = 0;
		}
#endif /* USE_ULIG_FILLEDBOXES */
		if (plot->pixmap) {
		    /* it is the wrong size now */
		    FPRINTF((stderr, "Free pixmap %d\n", 0));
		    XFreePixmap(dpy, plot->pixmap);
		    plot->pixmap = None;
		}
		display(plot);
	    }
	}
	break;
    case KeyPress:
	plot = Find_Plot_In_Linked_List_By_Window(event->xkey.window);
#if 0
	if (0 == XLookupString(&(event->xkey), key_string, sizeof(key_string), (KeySym *) NULL, (XComposeStatus *) NULL))
	    key_string[0] = 0;
#endif

#ifdef USE_MOUSE

#endif
	keysym = XKeycodeToKeysym(dpy, event->xkey.keycode, 0);
#ifdef USE_MOUSE
	update_modifiers(event->xkey.state);

	if (!modifier_mask) {
#endif
	    switch (keysym) {
#ifdef USE_MOUSE
	    case ' ': {
		static int cmd_tried = 0;
		static char *cmd = NULL;
		static unsigned long newGnuplotXID = 0;
		if (!cmd_tried)
		    cmd = getMultiTabConsoleSwitchCommand(&newGnuplotXID);
		/* overwrite gnuplotXID (re)set after x11.trm:X11_options() */
	    	if (newGnuplotXID) gnuplotXID = newGnuplotXID;
    		if (cmd) system(cmd);
		}
		if (gnuplotXID) {
		    XMapRaised(dpy, gnuplotXID);
		    XSetInputFocus(dpy, gnuplotXID, 0 /*revert */ , CurrentTime);
		    XFlush(dpy);
		}
		return;
	    case 'm': /* Toggle mouse display, but only if we control the window here */
		if (plot != current_plot
#ifdef PIPE_IPC
		    || pipe_died
#endif
		   ) {
		    plot->mouse_on = !(plot->mouse_on);
		    DisplayCoords(plot,plot->mouse_on ? " " : "");
		}
		break;
#endif
	    case 'q':
		/* close X window */
		Remove_Plot_From_Linked_List(event->xkey.window);
		return;
	    default:
		break;
	    }			/* switch (keysym) */
#ifdef USE_MOUSE
	}
	/* if (!modifier_mask) */
	switch (keysym) {

#define KNOWN_KEYSYMS(gp_keysym)                                             \
		if (plot == current_plot) {                                  \
		    gp_exec_event(GE_keypress,                               \
			(int)RevX(event->xkey.x), (int)RevY(event->xkey.y),  \
			gp_keysym, 0);                                       \
		}                                                            \
		return;

	case XK_BackSpace:
	    KNOWN_KEYSYMS(GP_BackSpace);
	case XK_Tab:
	    KNOWN_KEYSYMS(GP_Tab);
	case XK_Linefeed:
	    KNOWN_KEYSYMS(GP_Linefeed);
	case XK_Clear:
	    KNOWN_KEYSYMS(GP_Clear);
	case XK_Return:
	    KNOWN_KEYSYMS(GP_Return);
	case XK_Pause:
	    KNOWN_KEYSYMS(GP_Pause);
	case XK_Scroll_Lock:
	    KNOWN_KEYSYMS(GP_Scroll_Lock);
#ifdef XK_Sys_Req
	case XK_Sys_Req:
	    KNOWN_KEYSYMS(GP_Sys_Req);
#endif
	case XK_Escape:
	    KNOWN_KEYSYMS(GP_Escape);
	case XK_Insert:
	    KNOWN_KEYSYMS(GP_Insert);
	case XK_Delete:
	    KNOWN_KEYSYMS(GP_Delete);
	case XK_Home:
	    KNOWN_KEYSYMS(GP_Home);
	case XK_Left:
	    KNOWN_KEYSYMS(GP_Left);
	case XK_Up:
	    KNOWN_KEYSYMS(GP_Up);
	case XK_Right:
	    KNOWN_KEYSYMS(GP_Right);
	case XK_Down:
	    KNOWN_KEYSYMS(GP_Down);
	case XK_Prior:		/* XXX */
	    KNOWN_KEYSYMS(GP_PageUp);
	case XK_Next:		/* XXX */
	    KNOWN_KEYSYMS(GP_PageDown);
	case XK_End:
	    KNOWN_KEYSYMS(GP_End);
	case XK_Begin:
	    KNOWN_KEYSYMS(GP_Begin);
	case XK_KP_Space:
	    KNOWN_KEYSYMS(GP_KP_Space);
	case XK_KP_Tab:
	    KNOWN_KEYSYMS(GP_KP_Tab);
	case XK_KP_Enter:
	    KNOWN_KEYSYMS(GP_KP_Enter);
	case XK_KP_F1:
	    KNOWN_KEYSYMS(GP_KP_F1);
	case XK_KP_F2:
	    KNOWN_KEYSYMS(GP_KP_F2);
	case XK_KP_F3:
	    KNOWN_KEYSYMS(GP_KP_F3);
	case XK_KP_F4:
	    KNOWN_KEYSYMS(GP_KP_F4);
#ifdef XK_KP_Home
	case XK_KP_Home:
	    KNOWN_KEYSYMS(GP_KP_Home);
#endif
#ifdef XK_KP_Left
	case XK_KP_Left:
	    KNOWN_KEYSYMS(GP_KP_Left);
#endif
#ifdef XK_KP_Up
	case XK_KP_Up:
	    KNOWN_KEYSYMS(GP_KP_Up);
#endif
#ifdef XK_KP_Right
	case XK_KP_Right:
	    KNOWN_KEYSYMS(GP_KP_Right);
#endif
#ifdef XK_KP_Down
	case XK_KP_Down:
	    KNOWN_KEYSYMS(GP_KP_Down);
#endif
#ifdef XK_KP_Page_Up
	case XK_KP_Page_Up:
	    KNOWN_KEYSYMS(GP_KP_Page_Up);
#endif
#ifdef XK_KP_Page_Down
	case XK_KP_Page_Down:
	    KNOWN_KEYSYMS(GP_KP_Page_Down);
#endif
#ifdef XK_KP_End
	case XK_KP_End:
	    KNOWN_KEYSYMS(GP_KP_End);
#endif
#ifdef XK_KP_Begin
	case XK_KP_Begin:
	    KNOWN_KEYSYMS(GP_KP_Begin);
#endif
#ifdef XK_KP_Insert
	case XK_KP_Insert:
	    KNOWN_KEYSYMS(GP_KP_Insert);
#endif
#ifdef XK_KP_Delete
	case XK_KP_Delete:
	    KNOWN_KEYSYMS(GP_KP_Delete);
#endif
	case XK_KP_Equal:
	    KNOWN_KEYSYMS(GP_KP_Equal);
	case XK_KP_Multiply:
	    KNOWN_KEYSYMS(GP_KP_Multiply);
	case XK_KP_Add:
	    KNOWN_KEYSYMS(GP_KP_Add);
	case XK_KP_Separator:
	    KNOWN_KEYSYMS(GP_KP_Separator);
	case XK_KP_Subtract:
	    KNOWN_KEYSYMS(GP_KP_Subtract);
	case XK_KP_Decimal:
	    KNOWN_KEYSYMS(GP_KP_Decimal);
	case XK_KP_Divide:
	    KNOWN_KEYSYMS(GP_KP_Divide);

	case XK_KP_0:
	    KNOWN_KEYSYMS(GP_KP_0);
	case XK_KP_1:
	    KNOWN_KEYSYMS(GP_KP_1);
	case XK_KP_2:
	    KNOWN_KEYSYMS(GP_KP_2);
	case XK_KP_3:
	    KNOWN_KEYSYMS(GP_KP_3);
	case XK_KP_4:
	    KNOWN_KEYSYMS(GP_KP_4);
	case XK_KP_5:
	    KNOWN_KEYSYMS(GP_KP_5);
	case XK_KP_6:
	    KNOWN_KEYSYMS(GP_KP_6);
	case XK_KP_7:
	    KNOWN_KEYSYMS(GP_KP_7);
	case XK_KP_8:
	    KNOWN_KEYSYMS(GP_KP_8);
	case XK_KP_9:
	    KNOWN_KEYSYMS(GP_KP_9);

	case XK_F1:
	    KNOWN_KEYSYMS(GP_F1);
	case XK_F2:
	    KNOWN_KEYSYMS(GP_F2);
	case XK_F3:
	    KNOWN_KEYSYMS(GP_F3);
	case XK_F4:
	    KNOWN_KEYSYMS(GP_F4);
	case XK_F5:
	    KNOWN_KEYSYMS(GP_F5);
	case XK_F6:
	    KNOWN_KEYSYMS(GP_F6);
	case XK_F7:
	    KNOWN_KEYSYMS(GP_F7);
	case XK_F8:
	    KNOWN_KEYSYMS(GP_F8);
	case XK_F9:
	    KNOWN_KEYSYMS(GP_F9);
	case XK_F10:
	    KNOWN_KEYSYMS(GP_F10);
	case XK_F11:
	    KNOWN_KEYSYMS(GP_F11);
	case XK_F12:
	    KNOWN_KEYSYMS(GP_F12);


	default:
	    if (plot == current_plot) {
		KNOWN_KEYSYMS((int) keysym);
	    }
	    break;
	}
#endif
	break;
    case KeyRelease:
#ifdef USE_MOUSE
	update_modifiers(event->xkey.state);
#endif
	keysym = XKeycodeToKeysym(dpy, event->xkey.keycode, 0);
	switch (keysym) {
	case XK_Shift_L:
	case XK_Shift_R:
	case XK_Control_L:
	case XK_Control_R:
	case XK_Meta_L:
	case XK_Meta_R:
	case XK_Alt_L:
	case XK_Alt_R:
	    plot = Find_Plot_In_Linked_List_By_Window(event->xkey.window);
	    cursor = cursor_default;
	    if (plot)
		XDefineCursor(dpy, plot->window, cursor);
	}
	break;
    case ClientMessage:
	if (event->xclient.message_type == WM_PROTOCOLS &&
	    event->xclient.format == 32 && event->xclient.data.l[0] == WM_DELETE_WINDOW) {
	    Remove_Plot_From_Linked_List(event->xclient.window);
	}
	break;
#ifdef USE_MOUSE
    case Expose:
	/*
	 * we need to handle expose events here, because
	 * there might stuff like rulers which has to
	 * be redrawn. Well. It's not really hard to handle
	 * this. Note that the x and y fields are not used
	 * to update the pointer pos because the pointer
	 * might be on another window which generates the
	 * expose events. (joze)
	 */
	plot = Find_Plot_In_Linked_List_By_Window(event->xexpose.window);
	if (!plot)
	    break;
	if (!event->xexpose.count) {
	    /* XXX jitters display while resizing */
	    UpdateWindow(plot);
	}
	break;
    case EnterNotify:
	plot = Find_Plot_In_Linked_List_By_Window(event->xcrossing.window);
	if (!plot)
	    break;
	if (plot == current_plot) {
	    Call_display(plot);
	    gp_exec_event(GE_motion, (int) RevX(event->xcrossing.x), (int) RevY(event->xcrossing.y), 0, 0);
	}
	if (plot->zoombox_on) {
	    DrawBox(plot);
	    plot->zoombox_x2 = event->xcrossing.x;
	    plot->zoombox_y2 = event->xcrossing.y;
	    DrawBox(plot);
	}
	break;
    case MotionNotify:
	update_modifiers(event->xmotion.state);
	plot = Find_Plot_In_Linked_List_By_Window(event->xmotion.window);
	if (!plot)
	    break;
	{
	    Window root, child;
	    int root_x, root_y, pos_x, pos_y;
	    unsigned int keys_buttons;
	    if (!XQueryPointer(dpy, event->xmotion.window, &root, &child, &root_x, &root_y, &pos_x, &pos_y, &keys_buttons))
		break;

	    if (plot == current_plot
#ifdef PIPE_IPC
		&& !pipe_died
#endif
	       ) {
		Call_display(plot);
		gp_exec_event(GE_motion, (int) RevX(pos_x), (int) RevY(pos_y), 0, 0);
#if defined(USE_MOUSE) && defined(MOUSE_ALL_WINDOWS)
	    } else if (plot->axis_mask && plot->mouse_on) {
		/* This is not the active plot window, but we can still update the mouse coords */
		char mouse_format[60];
		char *m = mouse_format;
		double x, y, x2, y2;

		mouse_to_coords(plot, event, &x, &y, &x2, &y2);
		if (plot->axis_mask & (1<<FIRST_X_AXIS)) {
		    sprintf(m,"x=  %10g %c",x,'\0');
		    m += 15; 
		}
		if (plot->axis_mask & (1<<SECOND_X_AXIS)) {
		    sprintf(m,"x2= %10g %c",x2,'\0');
		    m += 15; 
		}
		if (plot->axis_mask & (1<<FIRST_Y_AXIS)) {
		    sprintf(m,"y=  %10g %c",y,'\0');
		    m += 15; 
		}
		if (plot->axis_mask & (1<<SECOND_Y_AXIS)) {
		    sprintf(m,"y2= %10g %c",y2,'\0');
		    m += 15; 
		}
		DisplayCoords(plot, mouse_format);
	    } else if (!plot->mouse_on) {
		DisplayCoords(plot,"");
#endif
	    }

	    if (plot->zoombox_on) {
		DrawBox(plot);
		plot->zoombox_x2 = pos_x;
		plot->zoombox_y2 = pos_y;
		DrawBox(plot);
	    }
	}
	break;
    case ButtonPress:
	update_modifiers(event->xbutton.state);
#ifndef TITLE_BAR_DRAWING_MSG
	button_pressed |= (1 << event->xbutton.button);
#endif
	plot = Find_Plot_In_Linked_List_By_Window(event->xbutton.window);
	if (!plot)
	    break;
	{
	    if (plot == current_plot) {
		Call_display(plot);
		gp_exec_event(GE_buttonpress, (int) RevX(event->xbutton.x), (int) RevY(event->xbutton.y), event->xbutton.button, 0);
	    }
	}
	break;
    case ButtonRelease:
#ifndef TITLE_BAR_DRAWING_MSG
	button_pressed &= ~(1 << event->xbutton.button);
#endif
	plot = Find_Plot_In_Linked_List_By_Window(event->xbutton.window);
	if (!plot)
	    break;
	if (plot == current_plot) {

	    long int doubleclick = SetTime(plot, event->xbutton.time);

	    update_modifiers(event->xbutton.state);
	    Call_display(plot);
	    gp_exec_event(GE_buttonrelease,
			  (int) RevX(event->xbutton.x), (int) RevY(event->xbutton.y), event->xbutton.button, (int) doubleclick);
	}

#ifdef DEBUG
#if defined(USE_MOUSE) && defined(MOUSE_ALL_WINDOWS)
	/* This causes gnuplot_x11 to pass mouse clicks back from all plot windows,
	 * not just the current plot. But who should we notify that a click has 
	 * happened, and how?  The fprintf to stderr is just for debugging. */
	else if (plot->axis_mask) {
	    double x, y, x2, y2;
	    mouse_to_coords(plot, event, &x, &y, &x2, &y2);
	    fprintf(stderr, "gnuplot_x11 %d: mouse button %1d from window %d at %g %g\n",
		    __LINE__, event->xbutton.button, (plot ? plot->plot_number : 0), x, y);
	}
#endif
#endif

	break;
#endif /* USE_MOUSE */
#ifdef EXPORT_SELECTION
    case SelectionNotify:
    case SelectionRequest:
	handle_selection_event(event);
	break;
#endif
    }
}

/*-----------------------------------------------------------------------------
 *   preset - determine options, open display, create window
 *---------------------------------------------------------------------------*/
/*
#define On(v) ( !strcmp(v,"on") || !strcmp(v,"true") || \
                !strcmp(v,"On") || !strcmp(v,"True") || \
                !strcmp(v,"ON") || !strcmp(v,"TRUE") )
*/
#define On(v) ( !strncasecmp(v,"on",2) || !strncasecmp(v,"true",4) )

#define AppDefDir "/usr/lib/X11/app-defaults"
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

static XrmDatabase dbCmd, dbApp, dbDef, dbEnv, db = (XrmDatabase) 0;

static char *type[20];
static XrmValue value;

static XrmOptionDescRec options[] = {
    {"-mono", ".mono", XrmoptionNoArg, (caddr_t) "on"},
    {"-gray", ".gray", XrmoptionNoArg, (caddr_t) "on"},
    {"-clear", ".clear", XrmoptionNoArg, (caddr_t) "on"},
    {"-tvtwm", ".tvtwm", XrmoptionNoArg, (caddr_t) "on"},
    {"-pointsize", ".pointsize", XrmoptionSepArg, (caddr_t) NULL},
    {"-display", ".display", XrmoptionSepArg, (caddr_t) NULL},
    {"-name", ".name", XrmoptionSepArg, (caddr_t) NULL},
    {"-geometry", "*geometry", XrmoptionSepArg, (caddr_t) NULL},
    {"-background", "*background", XrmoptionSepArg, (caddr_t) NULL},
    {"-bg", "*background", XrmoptionSepArg, (caddr_t) NULL},
    {"-foreground", "*foreground", XrmoptionSepArg, (caddr_t) NULL},
    {"-fg", "*foreground", XrmoptionSepArg, (caddr_t) NULL},
    {"-bordercolor", "*bordercolor", XrmoptionSepArg, (caddr_t) NULL},
    {"-bd", "*bordercolor", XrmoptionSepArg, (caddr_t) NULL},
    {"-borderwidth", ".borderwidth", XrmoptionSepArg, (caddr_t) NULL},
    {"-bw", ".borderwidth", XrmoptionSepArg, (caddr_t) NULL},
    {"-font", "*font", XrmoptionSepArg, (caddr_t) NULL},
    {"-fn", "*font", XrmoptionSepArg, (caddr_t) NULL},
    {"-reverse", "*reverseVideo", XrmoptionNoArg, (caddr_t) "on"},
    {"-rv", "*reverseVideo", XrmoptionNoArg, (caddr_t) "on"},
    {"+rv", "*reverseVideo", XrmoptionNoArg, (caddr_t) "off"},
    {"-iconic", "*iconic", XrmoptionNoArg, (caddr_t) "on"},
    {"-synchronous", "*synchronous", XrmoptionNoArg, (caddr_t) "on"},
    {"-xnllanguage", "*xnllanguage", XrmoptionSepArg, (caddr_t) NULL},
    {"-selectionTimeout", "*selectionTimeout", XrmoptionSepArg, (caddr_t) NULL},
    {"-title", ".title", XrmoptionSepArg, (caddr_t) NULL},
    {"-xrm", NULL, XrmoptionResArg, (caddr_t) NULL},
    {"-raise", "*raise", XrmoptionNoArg, (caddr_t) "on"},
    {"-noraise", "*raise", XrmoptionNoArg, (caddr_t) "off"},
    {"-feedback", "*feedback", XrmoptionNoArg, (caddr_t) "on"},
    {"-nofeedback", "*feedback", XrmoptionNoArg, (caddr_t) "off"},
    {"-persist", "*persist", XrmoptionNoArg, (caddr_t) "on"}
};

#define Nopt (sizeof(options) / sizeof(options[0]))

static void
preset(int argc, char *argv[])
{
    int Argc = argc;
    char **Argv = argv;

#ifdef VMS
    char *ldisplay = (char *) 0;
#else
    char *ldisplay = getenv("DISPLAY");
#endif
    char *home = getenv("HOME");
    char *server_defaults, *env, buffer[256];
#ifdef PM3D
#if 0
    Visual *TrueColor_vis, *PseudoColor_vis, *StaticGray_vis, *GrayScale_vis;
    int TrueColor_depth, PseudoColor_depth, StaticGray_depth, GrayScale_depth;
#endif
    char *db_string;
    sm_palette.colorMode = SMPAL_COLOR_MODE_NONE;	/* color is off by default */
#endif

    FPRINTF((stderr, "(preset) \n"));

    /* avoid bus error when env vars are not set */
    if (ldisplay == NULL)
	ldisplay = "";
    if (home == NULL)
	home = "";

/*---set to ignore ^C and ^Z----------------------------------------------*/

    signal(SIGINT, SIG_IGN);
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);
#endif

/*---prescan arguments for "-name"----------------------------------------*/

    while (++Argv, --Argc > 0) {
	if (!strcmp(*Argv, "-name") && Argc > 1) {
	    strncpy(Name, Argv[1], sizeof(Name) - 1);
	    strncpy(Class, Argv[1], sizeof(Class) - 1);
	    /* just in case */
	    Name[sizeof(Name) - 1] = NUL;
	    Class[sizeof(Class) - 1] = NUL;
	    if (Class[0] >= 'a' && Class[0] <= 'z')
		Class[0] -= 0x20;
	}
    }
    Argc = argc;
    Argv = argv;

/*---parse command line---------------------------------------------------*/

    XrmInitialize();
    XrmParseCommand(&dbCmd, options, Nopt, Name, &Argc, Argv);
    if (Argc > 1) {
#ifdef PIPE_IPC
	if (!strcmp(Argv[1], "-noevents")) {
	    pipe_died = 1;
	} else {
#endif
	    fprintf(stderr, "\n\
gnuplot: bad option: %s\n\
gnuplot: X11 aborted.\n", Argv[1]);
	    EXIT(1);
#ifdef PIPE_IPC
	}
#endif
    }
    if (pr_GetR(dbCmd, ".display"))
	ldisplay = (char *) value.addr;

/*---open display---------------------------------------------------------*/

#ifdef USE_MOUSE
    XSetErrorHandler(ErrorHandler);
#endif
    dpy = XOpenDisplay(ldisplay);
    if (!dpy) {
	fprintf(stderr, "\n\
gnuplot: unable to open display '%s'\n\
gnuplot: X11 aborted.\n", ldisplay);
	EXIT(1);
    }
    scr = DefaultScreen(dpy);
    root = DefaultRootWindow(dpy);
    server_defaults = XResourceManagerString(dpy);
    vis = DefaultVisual(dpy, scr);
    dep = DefaultDepth(dpy, scr);
    cmap.colormap = DefaultColormap(dpy, scr);


/**** atoms we will need later ****/

    WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
    WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);


/*---get application defaults--(subset of Xt processing)------------------*/

#ifdef VMS
    strcpy(buffer, "DECW$USER_DEFAULTS:GNUPLOT_X11.INI");
#elif defined OS2
/* for XFree86 ... */
    {
	char appdefdir[MAXPATHLEN];
	strncpy(appdefdir, 
	        __XOS2RedirRoot("/XFree86/lib/X11/app-defaults"),
	        sizeof(appdefdir));
	sprintf(buffer, "%s/%s", appdefdir, "Gnuplot");
    }
# else /* !OS/2 */
    strcpy(buffer, AppDefDir);
    strcat(buffer, "/");
    strcat(buffer, "Gnuplot");
#endif /* !VMS */

    dbApp = XrmGetFileDatabase(buffer);
    XrmMergeDatabases(dbApp, &db);

/*---get server or ~/.Xdefaults-------------------------------------------*/

    if (server_defaults)
	dbDef = XrmGetStringDatabase(server_defaults);
    else {
#ifdef VMS
	strcpy(buffer, "DECW$USER_DEFAULTS:DECW$XDEFAULTS.DAT");
#else
	strcpy(buffer, home);
	strcat(buffer, "/.Xdefaults");
#endif
	dbDef = XrmGetFileDatabase(buffer);
    }
    XrmMergeDatabases(dbDef, &db);

/*---get XENVIRONMENT or  ~/.Xdefaults-hostname---------------------------*/

#ifndef VMS
    if ((env = getenv("XENVIRONMENT")) != NULL)
	dbEnv = XrmGetFileDatabase(env);
    else {
	char *p = NULL, host[MAXHOSTNAMELEN];

	if (GP_SYSTEMINFO(host) < 0) {
	    fprintf(stderr, "gnuplot: %s failed. X11 aborted.\n", SYSINFO_METHOD);
	    EXIT(1);
	}
	if ((p = strchr(host, '.')) != NULL)
	    *p = '\0';
	strcpy(buffer, home);
	strcat(buffer, "/.Xdefaults-");
	strcat(buffer, host);
	dbEnv = XrmGetFileDatabase(buffer);
    }
    XrmMergeDatabases(dbEnv, &db);
#endif /* not VMS */

/*---merge command line options-------------------------------------------*/

    XrmMergeDatabases(dbCmd, &db);

/*---set geometry, font, colors, line widths, dash styles, point size-----*/

#ifdef PM3D
    /* a specific visual can be forced by the X resource visual */
    db_string = pr_GetR(db, ".visual") ? (char *) value.addr : (char *) 0;
    if (db_string) {
	Visual *visual = (Visual *) 0;
	int depth = (int) 0;
	char **ptr = visual_name;
	int i;
	for (i = 0; *ptr; i++, ptr++) {
	    if (!strcmp(db_string, *ptr)) {
#if 0
		if (DirectColor == i) {
		    fprintf(stderr, "DirectColor not supported by pm3d, using default.\n");
		} else
#endif
		if (GetVisual(i, &visual, &depth)) {
		    vis = visual;
		    dep = depth;
		    if (vis != DefaultVisual(dpy, scr)) {
			/* this will be the default colormap */
			cmap.colormap = XCreateColormap(dpy, root, vis, AllocNone);
		    }
		} else {
		    fprintf(stderr, "%s not supported by %s, using default.\n", *ptr, ldisplay);
		}
		break;
	    }
	}
    }
#if 0
    if (DirectColor == vis->class) {
	have_pm3d = 0;
    }
#endif
#if 0
    /* removed this message as it is annoying
     * when using gnuplot in a pipe (joze) */
    if (vis->class < (sizeof(visual_name) / sizeof(char **)) - 1) {
	fprintf(stderr, "Using %s at depth %d.\n", visual_name[vis->class], dep);
    }
#endif
    CmapClear(&cmap);

    /* set default of maximal_possible_colors */
    if (dep > 12) {
	maximal_possible_colors = 0x200;
    } else if (dep > 8) {
	maximal_possible_colors = 0x100;
    } else {
	/* will be something like PseudoColor * 8 */
	maximal_possible_colors = 240;	/* leave 16 for line colors */
    }

    /* check database for maxcolors */
    db_string = pr_GetR(db, ".maxcolors") ? (char *) value.addr : (char *) 0;
    if (db_string) {
	int itmp;
	if (sscanf(db_string, "%d", &itmp)) {
	    if (itmp <= 0) {
		fprintf(stderr, "\nmaxcolors must be strictly positive.\n");
	    } else if (itmp > pow((double) 2, (double) dep)) {
		fprintf(stderr, "\noops, cannot use this many colors on a %d bit deep display.\n", dep);
	    } else {
		maximal_possible_colors = itmp;
	    }
	} else {
	    fprintf(stderr, "\nunable to parse '%s' as integer\n", db_string);
	}
    }

    /* setting a default for minimal_possible_colors */
    minimal_possible_colors = maximal_possible_colors / (num_colormaps > 1 ? 2 : 8);	/* 0x20 / 30 */
    /* check database for mincolors */
    db_string = pr_GetR(db, ".mincolors") ? (char *) value.addr : (char *) 0;
    if (db_string) {
	int itmp;
	if (sscanf(db_string, "%d", &itmp)) {
	    if (itmp <= 0) {
		fprintf(stderr, "\nmincolors must be strictly positive.\n");
	    } else if (itmp > pow((double) 2, (double) dep)) {
		fprintf(stderr, "\noops, cannot use this many colors on a %d bit deep display.\n", dep);
	    } else if (itmp > maximal_possible_colors) {
		fprintf(stderr, "\nmincolors must be <= %d\n", maximal_possible_colors);
	    } else {
		minimal_possible_colors = itmp;
	    }
	} else {
	    fprintf(stderr, "\nunable to parse '%s' as integer\n", db_string);
	}
    }
#endif /* PM3D */

    pr_geometry();
    pr_font(NULL);		/* set current font to default font */
    pr_color(&cmap);		/* set colors for default colormap */
    pr_width();
    pr_dashes();
    pr_pointsize();
    pr_raise();
    pr_persist();
    pr_feedback();
}

/*-----------------------------------------------------------------------------
 *   pr_GetR - get resource from database using "-name" option (if any)
 *---------------------------------------------------------------------------*/

static char *
pr_GetR(xrdb, resource)
    XrmDatabase xrdb;
    char *resource;
{
    char name[128], class[128], *rc;

    strcpy(name, Name);
    strcat(name, resource);
    strcpy(class, Class);
    strcat(class, resource);
    rc = XrmGetResource(xrdb, name, class, type, &value)
	? (char *) value.addr : (char *) 0;
    return (rc);
}

/*-----------------------------------------------------------------------------
 *   pr_color - determine color values
 *---------------------------------------------------------------------------*/

static const char color_keys[Ncolors][30] = {
    "background", "bordercolor", "text", "border", "axis",
    "line1", "line2", "line3", "line4",
    "line5", "line6", "line7", "line8"
};
static char color_values[Ncolors][30] = {
    "white", "black", "black", "black", "black",
    "red", "green", "blue", "magenta",
    "cyan", "sienna", "orange", "coral"
};
static char color_values_rv[Ncolors][30] = {
    "black", "white", "white", "white", "white",
    "red", "green", "blue", "magenta",
    "cyan", "sienna", "orange", "coral"
};
static char gray_values[Ncolors][30] = {
    "black", "white", "white", "gray50", "gray50",
    "gray100", "gray60", "gray80", "gray40",
    "gray90", "gray50", "gray70", "gray30"
};

static void
pr_color(cmap_t * cmap_ptr)
{
    unsigned long black = BlackPixel(dpy, scr), white = WhitePixel(dpy, scr);
    char option[20], color[30], *v, *ctype;
    XColor xcolor;
    double intensity = -1;
    int n;

    if (pr_GetR(db, ".mono") && On(value.addr))
	Mono++;
    if (pr_GetR(db, ".gray") && On(value.addr))
	Gray++;
    if (pr_GetR(db, ".reverseVideo") && On(value.addr))
	Rv++;

    if (!Gray && (vis->class == GrayScale || vis->class == StaticGray))
	Mono++;

    if (!Mono) {

	ctype = (Gray) ? "Gray" : "Color";

#ifdef PM3D
	if (&cmap != cmap_ptr) {
	    /* for private colormaps: make sure
	     * that pixel 0 gets black (joze) */
	    xcolor.red = 0;
	    xcolor.green = 0;
	    xcolor.blue = 0;
	    XAllocColor(dpy, cmap_ptr->colormap, &xcolor);
	}
#endif

	for (n = 0; n < Ncolors; n++) {
	    strcpy(option, ".");
	    strcat(option, color_keys[n]);
	    if (n > 1)
		strcat(option, ctype);
	    v = pr_GetR(db, option)
		? (char *) value.addr : ((Gray) ? gray_values[n]
					 : (Rv ? color_values_rv[n] : color_values[n]));

	    if (sscanf(v, "%30[^,],%lf", color, &intensity) == 2) {
		if (intensity < 0 || intensity > 1) {
		    fprintf(stderr, "\ngnuplot: invalid color intensity in '%s'\n", color);
		    intensity = 1;
		}
	    } else {
		strcpy(color, v);
		intensity = 1;
	    }

	    if (!XParseColor(dpy, cmap_ptr->colormap, color, &xcolor)) {
		fprintf(stderr, "\ngnuplot: unable to parse '%s'. Using black.\n", color);
		cmap_ptr->colors[n] = black;
	    } else {
		xcolor.red *= intensity;
		xcolor.green *= intensity;
		xcolor.blue *= intensity;
		if (XAllocColor(dpy, cmap_ptr->colormap, &xcolor)) {
		    cmap_ptr->colors[n] = xcolor.pixel;
		} else {
		    fprintf(stderr, "\ngnuplot: can't allocate '%s'. Using black.\n", v);
		    cmap_ptr->colors[n] = black;
		}
	    }
	}
    } else {
	cmap_ptr->colors[0] = (Rv) ? black : white;
	for (n = 1; n < Ncolors; n++)
	    cmap_ptr->colors[n] = (Rv) ? white : black;
    }
#if defined(PM3D) && defined(USE_MOUSE)
    {
	/* create the xor GC just for allocating the xor value
	 * before a palette is created. This way the xor foreground
	 * will be available. */
	AllocateXorPixel(cmap_ptr);
    }
#endif
}

/*-----------------------------------------------------------------------------
 *   pr_dashes - determine line dash styles 
 *---------------------------------------------------------------------------*/

static const char dash_keys[Ndashes][10] = {
    "border", "axis",
    "line1", "line2", "line3", "line4", "line5", "line6", "line7", "line8"
};

static char dash_mono[Ndashes][10] = {
    "0", "16",
    "0", "42", "13", "44", "15", "4441", "42", "13"
};

static char dash_color[Ndashes][10] = {
    "0", "16",
    "0", "0", "0", "0", "0", "0", "0", "0"
};

static void
pr_dashes()
{
    int n, j, l, ok;
    char option[20], *v;

    for (n = 0; n < Ndashes; n++) {
	strcpy(option, ".");
	strcat(option, dash_keys[n]);
	strcat(option, "Dashes");
	v = pr_GetR(db, option)
	    ? (char *) value.addr : ((Mono) ? dash_mono[n] : dash_color[n]);
	l = strlen(v);
	if (l == 1 && *v == '0') {
	    dashes[n][0] = (unsigned char) 0;
	    continue;
	}
	for (ok = 0, j = 0; j < l; j++) {
	    if (v[j] >= '1' && v[j] <= '9')
		ok++;
	}
	if (ok != l || (ok != 2 && ok != 4)) {
	    fprintf(stderr, "gnuplot: illegal dashes value %s:%s\n", option, v);
	    dashes[n][0] = (unsigned char) 0;
	    continue;
	}
	for (j = 0; j < l; j++) {
	    dashes[n][j] = (unsigned char) (v[j] - '0');
	}
	dashes[n][l] = (unsigned char) 0;
    }
}

/*-----------------------------------------------------------------------------
 *   pr_font - determine font          
 *---------------------------------------------------------------------------*/

static void
pr_font( fontname )
char *fontname;
{
    XFontStruct *previous_font = font;
    static char previous_font_name[128];

    if (!fontname || !(*fontname))
	fontname = default_font;

    if (!fontname || !(*fontname)) {
	if ((fontname = pr_GetR(db, ".font")))
	    strncpy(default_font,fontname,sizeof(default_font)-1);
	    FPRINTF((stderr,"gnuplot_x11: setting default font %s from Xresource\n",
		    fontname));
    }

    if (!fontname)
	fontname = FallbackFont;
    font = XLoadQueryFont(dpy, fontname);

    if (!font) {
	/* EAM 19-Aug-2002 Try to construct a plausible X11 full font spec */
	/* We are passed "font<,size><,slant>"                             */
	char fontspec[128], shortname[64], *fontencoding, slant, *weight;
	int  sep;
	int  fontsize = 0;

	/* Enhanced font processing wants a method of requesting a new size  */
	/* for whatever the previously selected font was, so we have to save */
	/* and reuse the previous font name to construct the new spec.       */
	if (!strncmp(fontname,"DEFAULT",7)) {
	    sscanf(&fontname[8],"%d",&fontsize);
	    fontname = default_font;
	} else if (*fontname == ',') {
	    sscanf(&fontname[1],"%d",&fontsize);
	    fontname = previous_font_name;
	}

	sep = strcspn(fontname,",");
	if (sep >= sizeof(shortname))
	    sep = sizeof(shortname) - 1;
	strncpy(shortname,fontname,sep);
	shortname[sep] = '\0';
	if (!fontsize)
	    sscanf( &(fontname[sep+1]),"%d",&fontsize);
	if (fontsize > 99 || fontsize < 1)
	    fontsize = 12;
	   
	slant = strstr(&fontname[sep+1],"italic")  ? 'i' :
		strstr(&fontname[sep+1],"oblique") ? 'o' :
		                                     'r' ;

	weight = strstr(&fontname[sep+1],"bold")   ? "bold" :
		 strstr(&fontname[sep+1],"medium") ? "medium" :
		                                     "*" ;

	if (!strncmp("Symbol",shortname,6) || !strncmp("symbol",shortname,6))
	    fontencoding = "*-*";
	else
	    fontencoding = (
		encoding == S_ENC_CP437     ? "dosencoding-cp437" :
		encoding == S_ENC_CP850     ? "dosencoding-cp850" :
		encoding == S_ENC_ISO8859_1 ? "iso8859-1" :
		encoding == S_ENC_ISO8859_2 ? "iso8859-2" :
		encoding == S_ENC_ISO8859_15 ? "iso8859-15" :
		encoding == S_ENC_KOI8_R    ? "koi8-r" :
		"*-*" ) ;

	sprintf(fontspec, "-*-%s-%s-%c-*-*-%d-*-*-*-*-*-%s",
		shortname, weight, slant, fontsize, fontencoding
		);
	font = XLoadQueryFont(dpy, fontspec);

	if (!font) {
	    /* Try to decode some common PostScript font names */
	    if (!strcmp("Times-Bold",shortname) || !strcmp("times-bold",shortname)) {
		sprintf(fontspec,
			"-*-times-bold-r-*-*-%d-*-*-*-*-*-%s", fontsize, fontencoding);
	    } else if (!strcmp("Times-Roman",shortname) || !strcmp("times-roman",shortname)) {
		sprintf(fontspec,
			"-*-times-medium-r-*-*-%d-*-*-*-*-*-%s", fontsize, fontencoding);
	    } else if (!strcmp("Times-Italic",shortname) || !strcmp("times-italic",shortname)) {
		sprintf(fontspec,
			"-*-times-medium-i-*-*-%d-*-*-*-*-*-%s", fontsize, fontencoding);
	    } else if (!strcmp("Times-BoldItalic",shortname) || !strcmp("times-bolditalic",shortname)) {
		sprintf(fontspec,
			"-*-times-bold-i-*-*-%d-*-*-*-*-*-%s", fontsize, fontencoding);
	    } else if (!strcmp("Helvetica-Bold",shortname) || !strcmp("helvetica-bold",shortname)) {
		sprintf(fontspec,
			"-*-helvetica-bold-r-*-*-%d-*-*-*-*-*-%s", fontsize, fontencoding);
	    } else if (!strcmp("Helvetica-Oblique",shortname) || !strcmp("helvetica-oblique",shortname)) {
		sprintf(fontspec,
			"-*-helvetica-medium-o-*-*-%d-*-*-*-*-*-%s", fontsize, fontencoding);
	    } else if (!strcmp("Helvetica-BoldOblique",shortname) || !strcmp("helvetica-boldoblique",shortname)) {
		sprintf(fontspec,
			"-*-helvetica-bold-o-*-*-%d-*-*-*-*-*-%s", fontsize, fontencoding);
	    } else if (!strcmp("Helvetica-Narrow-Bold",shortname) || !strcmp("helvetica-narrow-bold",shortname)) {
		sprintf(fontspec,
			"-*-arial narrow-bold-r-*-*-%d-*-*-*-*-*-%s", fontsize, fontencoding);
	    }
	    font = XLoadQueryFont(dpy, fontspec);
	}

    }
    
    if (font) {
        strncpy(previous_font_name, fontname, sizeof(previous_font_name)-1);
        FPRINTF((stderr,"gnuplot_x11:saving current font name \"%s\"\n",previous_font_name));
    } else {
	if (!((font = XLoadQueryFont(dpy, default_font))))
	    font = previous_font;
    }
    if (!font) {
	fprintf(stderr, "gnuplot_x11: using font '%s' instead.\n", FallbackFont);
	font = XLoadQueryFont(dpy, FallbackFont);
	if (!font) {
	    fprintf(stderr, "\
gnuplot: can't load font '%s'\n\
gnuplot: no useable font - X11 aborted.\n", FallbackFont);
	    EXIT(1);
	} else
	    fontname = FallbackFont;
    }

    vchar = font->ascent + font->descent;
    hchar = XTextWidth(font, "0123456789", 10) / 10;

    FPRINTF((stderr,"gnuplot_x11: pr_font() set font %s, vchar %d hchar %d\n",
		fontname,vchar,hchar));

}

/*-----------------------------------------------------------------------------
 *   pr_geometry - determine window geometry      
 *---------------------------------------------------------------------------*/

static void
pr_geometry()
{
    char *geometry = pr_GetR(db, ".geometry");
    int x, y, flags;
    unsigned int w, h;

    if (geometry) {
	flags = XParseGeometry(geometry, &x, &y, &w, &h);
	if (flags & WidthValue)
	    gW = w;
	if (flags & HeightValue)
	    gH = h;
	if (flags & (WidthValue | HeightValue))
	    gFlags = (gFlags & ~PSize) | USSize;

	if (flags & XValue)
	    gX = (flags & XNegative) ? x + DisplayWidth(dpy, scr) - gW - BorderWidth * 2 : x;

	if (flags & YValue)
	    gY = (flags & YNegative) ? y + DisplayHeight(dpy, scr) - gH - BorderWidth * 2 : y;

	if (flags & (XValue | YValue))
	    gFlags = (gFlags & ~PPosition) | USPosition;
    }
}

/*-----------------------------------------------------------------------------
 *   pr_pointsize - determine size of points for 'points' plotting style
 *---------------------------------------------------------------------------*/

static void
pr_pointsize()
{
    if (pr_GetR(db, ".pointsize")) {
	if (sscanf((char *) value.addr, "%lf", &pointsize) == 1) {
	    if (pointsize <= 0 || pointsize > 10) {
		fprintf(stderr, "\ngnuplot: invalid pointsize '%s'\n", value.addr);
		pointsize = 1;
	    }
	} else {
	    fprintf(stderr, "\ngnuplot: invalid pointsize '%s'\n", value.addr);
	    pointsize = 1;
	}
    } else {
	pointsize = 1;
    }
}

/*-----------------------------------------------------------------------------
 *   pr_width - determine line width values
 *---------------------------------------------------------------------------*/

static const char width_keys[Nwidths][30] = {
    "border", "axis",
    "line1", "line2", "line3", "line4", "line5", "line6", "line7", "line8"
};

static void
pr_width()
{
    int n;
    char option[20], *v;

    for (n = 0; n < Nwidths; n++) {
	strcpy(option, ".");
	strcat(option, width_keys[n]);
	strcat(option, "Width");
	if ((v = pr_GetR(db, option)) != NULL) {
	    if (*v < '0' || *v > '4' || strlen(v) > 1)
		fprintf(stderr, "gnuplot: illegal width value %s:%s\n", option, v);
	    else
		widths[n] = (unsigned int) atoi(v);
	}
    }
}

/*-----------------------------------------------------------------------------
 *   pr_window - create window 
 *---------------------------------------------------------------------------*/
static void
ProcessEvents(Window win)
{
    XSelectInput(dpy, win, KeyPressMask | KeyReleaseMask
		 | StructureNotifyMask | PointerMotionMask | PointerMotionHintMask
		 | ButtonPressMask | ButtonReleaseMask | ExposureMask | EnterWindowMask);
    XSync(dpy, 0);
}

static void
pr_window(plot_struct *plot)
{
    char *title = pr_GetR(db, ".title");
    static XSizeHints hints;
    int Tvtwm = 0;

    FPRINTF((stderr, "(pr_window) \n"));

#ifdef PM3D
    if (have_pm3d) {
	XSetWindowAttributes attr;
	unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap;
	attr.background_pixel = plot->cmap->colors[0];
	attr.border_pixel = plot->cmap->colors[1];
	attr.colormap = plot->cmap->colormap;
	plot->window = XCreateWindow(dpy, root, plot->x, plot->y, plot->width,
				     plot->height, BorderWidth, dep, InputOutput, vis, mask, &attr);
	/* XXX need this ? yes. (joze) XXX */
	if (plot->window && SMPAL_COLOR_MODE_NONE != sm_palette.colorMode) {
	    PaletteMake(plot, (t_sm_palette *) 0);
	}
    } else
#endif
	plot->window = XCreateSimpleWindow(dpy, root, plot->x, plot->y,
					   plot->width, plot->height, BorderWidth, plot->cmap->colors[1], plot->cmap->colors[0]);

    /* Return if something wrong. */
    if (!plot->window)
	return;

    /* ask ICCCM-compliant window manager to tell us when close window
     * has been chosen, rather than just killing us
     */

    XChangeProperty(dpy, plot->window, WM_PROTOCOLS, XA_ATOM, 32, PropModeReplace, (unsigned char *) &WM_DELETE_WINDOW, 1);

    if (pr_GetR(db, ".clear") && On(value.addr))
	Clear++;
    if (pr_GetR(db, ".tvtwm") && On(value.addr))
	Tvtwm++;

    if (!Tvtwm) {
	hints.flags = plot->posn_flags;
    } else {
	hints.flags = (plot->posn_flags & ~USPosition) | PPosition;	/* ? */
    }
    hints.x = gX;
    hints.y = gY;
    hints.width = plot->width;
    hints.height = plot->height;

    XSetNormalHints(dpy, plot->window, &hints);

    if (pr_GetR(db, ".iconic") && On(value.addr)) {
	XWMHints wmh;

	wmh.flags = StateHint;
	wmh.initial_state = IconicState;
	XSetWMHints(dpy, plot->window, &wmh);
    }
#if 0 /* 1 clear, 0 do not clear */
    if (plot->titlestring) {
	free(plot->titlestring);
	plot->titlestring = 0;
    }
#endif

    ProcessEvents(plot->window);

    /* If title doesn't exist, create one. */
#if 1
#define ICON_TEXT "gplt"
#define TEMP_NUM_LEN 16
    {
    /* append the X11 terminal number (if greater than zero) */
    char numstr[sizeof(ICON_TEXT)+TEMP_NUM_LEN+1]; /* space for text, number and terminating \0 */
    if (plot->plot_number)
	sprintf(numstr, "%s%d%c", ICON_TEXT, plot->plot_number, '\0');
    else
	sprintf(numstr, "%s%c", ICON_TEXT, '\0');
    FPRINTF((stderr, "term_number is %d", plot->plot_number));
    XSetIconName(dpy, plot->window, numstr);
#undef TEMP_NUM_LEN
    if (!plot->titlestring) {
	int orig_len;
	if (!title) title = Class;
	orig_len = strlen(title);
	/* memory for text, white space, number and terminating \0 */
	if ((plot->titlestring = (char *) malloc(orig_len + ((orig_len && plot->plot_number) ? 1 : 0) + strlen(numstr) - strlen(ICON_TEXT) + 1))) {
	    strcpy(plot->titlestring, title);
	    if (orig_len && plot->plot_number)
		plot->titlestring[orig_len++] = ' ';
	    strcpy(plot->titlestring + orig_len, numstr + strlen(ICON_TEXT));
	    XStoreName(dpy, plot->window, plot->titlestring);
	} else
	    XStoreName(dpy, plot->window, title);
    } else {
	XStoreName(dpy, plot->window, plot->titlestring);
    }
    }
#undef ICON_TEXT
#endif

    XMapWindow(dpy, plot->window);

    windows_open++;
}


/***** pr_raise ***/
static void
pr_raise()
{
    if (pr_GetR(db, ".raise"))
	do_raise = (On(value.addr));
}


static void
pr_persist()
{
    if (pr_GetR(db, ".persist"))
	persist = (On(value.addr));
}

static void
pr_feedback()
{
    if (pr_GetR(db, ".feedback"))
	feedback = !(!strncasecmp(value.addr,"off",3) || !strncasecmp(value.addr,"false",5));
    FPRINTF((stderr,"gplt_x11: set feedback to %d (%s)\n",feedback,value.addr));
}

/************ code to handle selection export *********************/

#ifdef EXPORT_SELECTION

/* bit of a bodge, but ... */
static struct plot_struct *exported_plot;

static void
export_graph(struct plot_struct *plot)
{
    FPRINTF((stderr, "export_graph(0x%x)\n", plot));

    XSetSelectionOwner(dpy, EXPORT_SELECTION, plot->window, CurrentTime);
    /* to check we have selection, we would have to do a
     * GetSelectionOwner(), but if it failed, it failed - no big deal
     */
    exported_plot = plot;
}

static void
handle_selection_event(XEvent *event)
{
    switch (event->type) {
    case SelectionRequest:
	{
	    XEvent reply;

	    static Atom XA_TARGETS = (Atom) 0;
	    if (XA_TARGETS == 0)
		XA_TARGETS = XInternAtom(dpy, "TARGETS", False);

	    reply.type = SelectionNotify;
	    reply.xselection.send_event = True;
	    reply.xselection.display = event->xselectionrequest.display;
	    reply.xselection.requestor = event->xselectionrequest.requestor;
	    reply.xselection.selection = event->xselectionrequest.selection;
	    reply.xselection.target = event->xselectionrequest.target;
	    reply.xselection.property = event->xselectionrequest.property;
	    reply.xselection.time = event->xselectionrequest.time;

	    FPRINTF((stderr, "selection request\n"));

	    if (reply.xselection.target == XA_TARGETS) {
		static Atom targets[] = { XA_PIXMAP, XA_COLORMAP };

		FPRINTF((stderr, "Targets request from %d\n", reply.xselection.requestor));

		XChangeProperty(dpy, reply.xselection.requestor,
				reply.xselection.property, reply.xselection.target,
				32, PropModeReplace, (unsigned char *) targets, 2);
	    } else if (reply.xselection.target == XA_COLORMAP) {

		FPRINTF((stderr, "colormap request from %d\n", reply.xselection.requestor));

		XChangeProperty(dpy, reply.xselection.requestor,
				reply.xselection.property, reply.xselection.target,
				32, PropModeReplace, (unsigned char *) &(cmap.colormap), 1);
	    } else if (reply.xselection.target == XA_PIXMAP) {

		FPRINTF((stderr, "pixmap request from %d\n", reply.xselection.requestor));

		XChangeProperty(dpy, reply.xselection.requestor,
				reply.xselection.property, reply.xselection.target,
				32, PropModeReplace, (unsigned char *) &(exported_plot->pixmap), 1);
#ifdef PIPE_IPC
	    } else if (reply.xselection.target == XA_STRING) {
		FPRINTF((stderr, "XA_STRING request\n"));
		XChangeProperty(dpy, reply.xselection.requestor,
				reply.xselection.property, reply.xselection.target,
				8, PropModeReplace, (unsigned char *) selection, strlen(selection));
#endif
	    } else {
		reply.xselection.property = None;
	    }

	    XSendEvent(dpy, reply.xselection.requestor, False, 0L, &reply);
	    /* we never block on XNextEvent(), so must flush manually
	     * (took me *ages* to find this out !)
	     */

	    XFlush(dpy);
	}
	break;
    }
}

#endif /* EXPORT_SELECTION */

#if defined(USE_MOUSE) && defined(MOUSE_ALL_WINDOWS)
/* Convert X-window mouse coordinates to coordinate system of plot axes */
static void
mouse_to_coords(plot_struct *plot, XEvent *event,
		double *x, double *y, double *x2, double *y2)
{
    int xx =         4096. * (event->xbutton.x + 0.5)/ plot->width;
    int yy = 4095. - 4096. * (event->xbutton.y + 0.5)/ plot->gheight;

    FPRINTF((stderr,"gnuplot_x11 %d: mouse at %d %d\t", __LINE__, xx, yy));

    *x  = mouse_to_axis(xx, &(plot->axis_scale[FIRST_X_AXIS]));
    *y  = mouse_to_axis(yy, &(plot->axis_scale[FIRST_Y_AXIS]));
    *x2 = mouse_to_axis(xx, &(plot->axis_scale[SECOND_X_AXIS]));
    *y2 = mouse_to_axis(yy, &(plot->axis_scale[SECOND_Y_AXIS]));

    FPRINTF((stderr,"mouse x y %10g %10g x2 y2 %10g %10g\n", *x, *y, *x2, *y2 ));
}

static double
mouse_to_axis(int mouse_coord, axis_scale_t *axis)
{
    double axis_coord;

    if (axis->term_scale == 0.0)
	return 0.;

    axis_coord = ((double)(mouse_coord - axis->term_lower)) / axis->term_scale + axis->min;
    if (axis->logbase > 0.0)
	axis_coord = exp(axis_coord * axis->logbase);

    return axis_coord;
}
#endif


/*-----------------------------------------------------------------------------
 *   Add_Plot_To_Linked_List - Create space for plot and put in linked list.
 *---------------------------------------------------------------------------*/

static plot_struct *
Add_Plot_To_Linked_List(int plot_number)
{
    /* Make sure plot does not already exists in the list. */
    plot_struct *psp = Find_Plot_In_Linked_List_By_Number(plot_number);

    if (psp == NULL) {
	psp = (plot_struct *) malloc(sizeof(plot_struct));
	if (psp) {
	    /* Initialize structure variables. */
	    memset((void*)psp, 0, sizeof(plot_struct));
	    psp->plot_number = plot_number;
	    /* Add link to beginning of the list. */
	    psp->prev_plot = NULL;
	    if (list_start != NULL) {
		list_start->prev_plot = psp;
		psp->next_plot = list_start;
	    } else
		psp->next_plot = NULL;
	    list_start = psp;
	}
	else {
	    psp = NULL;
	    fprintf(stderr, ERROR_NOTICE("Could not allocate memory for plot.\n\n"));
	}
    }

    return psp;
}


/*-----------------------------------------------------------------------------
 *   Remove_Plot_From_Linked_List - Remove from linked list and free memory.
 *---------------------------------------------------------------------------*/

static void
Remove_Plot_From_Linked_List(Window plot_window)
{
    /* Make sure plot exists in the list. */
    plot_struct *psp = Find_Plot_In_Linked_List_By_Window(plot_window);

    if (psp != NULL) {
	/* Remove link from the list. */
	if (psp->next_plot != NULL)
	    psp->next_plot->prev_plot = psp->prev_plot;
	if (psp->prev_plot != NULL) {
	    psp->prev_plot->next_plot = psp->next_plot;
	} else {
	    list_start = psp->next_plot;
	}
	/* If global pointers point at this plot, reassign them. */
	if (current_plot == psp) {
#if 0 /* Make some other plot current. */
	    if (psp->prev_plot != NULL)
		current_plot = psp->prev_plot;
	    else
		current_plot = psp->next_plot;
#else /* No current plot. */
	    current_plot = NULL;
#endif
	}
	if (plot == psp)
	    plot = current_plot;
	/* Deallocate memory. */
	delete_plot(psp);
	free(psp);
    }
}


/*-----------------------------------------------------------------------------
 *   Find_Plot_In_Linked_List_By_Number - Search for the plot in the linked list.
 *---------------------------------------------------------------------------*/

static plot_struct *
Find_Plot_In_Linked_List_By_Number(int plot_number)
{
    plot_struct *psp = list_start;

    while (psp != NULL) {
	if (psp->plot_number == plot_number)
	    break;
	psp = psp->next_plot;
    }

    return psp;
}


/*-----------------------------------------------------------------------------
 *   Find_Plot_In_Linked_List_By_Window - Search for the plot in the linked list.
 *---------------------------------------------------------------------------*/

static plot_struct *
Find_Plot_In_Linked_List_By_Window(Window window)
{
    plot_struct *psp = list_start;

    while (psp != NULL) {
	if (psp->window == window)
	    break;
	psp = psp->next_plot;
    }

    return psp;
}


/* NOTE:  The removing of plots via the ErrorHandler routine is rather
   tricky.  The error events can happen at any time during execution of
   the program, very similar to an interrupt.  The consequence is that
   the error handling routine can't remove plots from the linked list
   directly.  Instead we use a queuing system in which the main code
   eventually removes the plots.

   Furthermore, to be safe, only the error handling routine should create
   and delete elements in the FIFO.  Otherwise, the possibility of bogus
   pointers can arise if error events happen at the exact wrong time.
   (Requires a lot of thought.)

   The scheme here is for the error handler to put elements in the
   queue marked as "processed = 0" and then indicate that the main
   code should process elements in the queue.  The main code then
   copies the information about the plot to remove and sets the value
   "processed = 1".  Afterward the main code removes the plot.
*/

/*-----------------------------------------------------------------------------
 *   Add_Plot_To_Remove_FIFO_Queue - Method for error handler to destroy plot.
 *---------------------------------------------------------------------------*/

static void
Add_Plot_To_Remove_FIFO_Queue(Window plot_window)
{
    /* Clean up any processed links. */
    plot_remove_struct *prsp = remove_fifo_queue_start;
    FPRINTF((stderr,"Add plot to remove FIFO queue called.\n"));
    while (prsp != NULL) {
	if (prsp->processed) {
	    remove_fifo_queue_start = prsp->next_remove;
	    free(prsp);
	    prsp = remove_fifo_queue_start;
	    FPRINTF((stderr,"  -> Removed a processed element from FIFO queue.\n"));
	} else {
	    break;
	}
    }

    /* Go to end of list while checking if this window is already in list. */
    while (prsp != NULL) {
	if (prsp->plot_window_to_remove == plot_window) {
	    /* Discard this request because the same window is yet to be processed.
	       X11 could be stuck sending the same error message again and again
	       while the main program is not responding for some reason.  This would
	       lead to the FIFO queue growing indefinitely. */
	    return;
	}
	if (prsp->next_remove == NULL)
	    break;
	else
	    prsp = prsp->next_remove;
    }

    /* Create link and add to end of queue. */
    {plot_remove_struct *prsp_new = (plot_remove_struct *) malloc(sizeof(plot_remove_struct));
    if (prsp_new) {
	/* Initialize structure variables. */
	prsp_new->next_remove = NULL;
	prsp_new->plot_window_to_remove = plot_window;
	prsp_new->processed = 0;
	if (remove_fifo_queue_start)
	    prsp->next_remove = prsp_new;
	else
	    remove_fifo_queue_start = prsp_new;
	process_remove_fifo_queue = 1; /* Indicate to main loop that there is a plot to remove. */
	FPRINTF((stderr,"  -> Added an element to FIFO queue.\n"));
    }
    else {
	fprintf(stderr, ERROR_NOTICE("Could not allocate memory for plot remove queue.\n\n"));
    }}
}


/*-----------------------------------------------------------------------------
 *   Process_Remove_FIFO_Queue - Remove plots queued by error handler.
 *---------------------------------------------------------------------------*/

static void
Process_Remove_FIFO_Queue(void)
{
    plot_remove_struct *prsp = remove_fifo_queue_start;

    /* Clear flag before processing so that if an ErrorHandler event
     * comes along while running the remainder of this routine, any new
     * error events that were missed because of timing issues will be
     * processed next time through the main loop.
     */
    process_remove_fifo_queue = 0;

    /* Go through the list and process any unprocessed queue request.
     * No clean up is done here because having two asynchronous routines
     * modifying the queue would be too dodgy.  The ErrorHandler creates
     * and removes links in the queue based upon the processed flag.
     */
    while (prsp != NULL) {

	/* Make a copy of the remove information structure before changing flag.
	 * Otherwise, there would be the possibility of the error handler routine
	 * removing the associated link upon seeing the "processed" flag set.  From
	 * this side of things, the pointer becomes invalid once that flag is set.
	 */
	plot_remove_struct prs;
	prs.plot_window_to_remove = prsp->plot_window_to_remove;
	prs.next_remove = prsp->next_remove;
	prs.processed = prsp->processed;

	/* Set processed flag before processing the event.  This is so
	 * that the FIFO queue does not have to repeat window entries.
	 * If the error handler were to break in right before the
	 * "processed" flag is set to 1 and not put another link in
	 * the FIFO because it sees the window in question is already
	 * in the FIFO, we're OK.  The reason is that the window in
	 * question is still to be processed.  On the other hand, if
	 * we were to process then set the flag, an entry in the queue
	 * could potentially be lost.
	 */
	prsp->processed = 1;
	FPRINTF((stderr, "Processed element in remove FIFO queue.\n"));

	/* NOW process the plot to remove. */
	if (!prs.processed)
	    Remove_Plot_From_Linked_List(prs.plot_window_to_remove);

	prsp = prs.next_remove;
    }

    /* Issue an X11 error so that error handler cleans up queue?
     * Really, this isn't super important.  Without issuing a bogus
     * error, the processed queue elements will be deleted the
     * next time there is an error.  Until another error comes
     * along it means there is maybe ten or so words of memory
     * reserved on the heap for the FIFO queue.  In some sense,
     * the extra code is hardly worth the effort, especially
     * when X11 documentation is so sparse on the matter of errors.
     */

}
