#ifndef lint
static char *RCSid() { return RCSid("$Id: mouse.c,v 1.2.2.2 2000/06/04 12:53:20 joze Exp $"); }
#endif

/* GNUPLOT - mouse.c */

/* driver independent mouse part. */

/*[
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

/*
 * AUTHORS
 * 
 *   Original Software (October 1999 - January 2000):
 *     Pieter-Tjerk de Boer <ptdeboer@cs.utwente.nl>
 *     Petr Mikulik <mikulik@physics.muni.cz>
 *     Johannes Zellner <johannes@zellner.org>
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define _MOUSE_C

#ifdef USE_MOUSE
# include <stdio.h>
# include <math.h>
# include <ctype.h>		/* for toupper */
# include <string.h>

# ifdef HAVE_LIBREADLINE
#  include <readline/readline.h>	/* for ding() */
# endif

# include "plot.h"
# include "graphics.h"
# include "graph3d.h"
# include "setshow.h"
# include "alloc.h"
# include "gp_time.h"
# include "command.h"
# include "plot3d.h"
# include "mouse.h"
# include "mousecmn.h"

/********************** variables *********************************************/


/* Structure for the ruler: on/off, position,...
 */
static struct {
    TBOOLEAN on;
    double x, y, x2, y2;	/* ruler position in real units of the graph */
    long px, py;		/* ruler position in the viewport units */
} ruler = {
    FALSE, 0, 0, 0, 0, 0, 0
};


/* the coordinates of the mouse cursor in gnuplot's internal coordinate system
 */
int mouse_x, mouse_y;


/* the "real" coordinates of the mouse cursor, i.e., in the user's coordinate
 * system(s)
 */
double real_x, real_y, real_x2, real_y2;


/* mouse_polar_distance is set to TRUE if user wants to see the distance between
 * the ruler and mouse pointer in polar coordinates too (otherwise, distance 
 * in cartesian coordinates only is shown)
 */
/* int mouse_polar_distance = FALSE; */
/* moved to the struct mouse_setting_t (joze) */


/* status of buttons; button i corresponds to bit (1<<i) of this variable
 */
int button = 0;


/* variables for setting the zoom region:
 */
/* flag, TRUE while user is outlining the zoom region */
TBOOLEAN setting_zoom_region = FALSE;
/* coordinates of the first corner of the zoom region, in the internal
 * coordinate system */
int setting_zoom_x, setting_zoom_y;


/* variables for changing the 3D view:
*/
TBOOLEAN allowmotion = TRUE;	/* do we allow motion to result in a replot right now? */
TBOOLEAN needreplot = FALSE;	/* did we already postpone a replot because allowmotion was FALSE ? */
int start_x, start_y;		/* mouse position when dragging started */
int motion = 0;			/* ButtonPress sets this to 0, ButtonMotion to 1 */
float zero_rot_x, zero_rot_z;	/* values for rot_x and rot_z corresponding to zero position of mouse */


/* bind related stuff */

typedef struct bind_t {
    struct bind_t *prev;
    int key;
    char modifier;
    char *command;
    char *(*builtin) (struct gp_event_t * ge);
    struct bind_t *next;
} bind_t;

bind_t *bindings = (bind_t *) 0;
static const int NO_KEY = -1;
static TBOOLEAN trap_release = FALSE;

/* forward declarations */
static void alert __PROTO((void));
static void MousePosToGraphPosReal __PROTO((int xx, int yy, double *x, double *y, double *x2, double *y2));
static char *xy_format __PROTO((void));
static char *zoombox_format __PROTO((void));
static char *xy1_format __PROTO((char *leader));
static char *GetAnnotateString __PROTO((char *s, double x, double y, int mode, char *fmt));
static char *xDateTimeFormat __PROTO((double x, char *b, int mode));
#ifdef OLD_STATUS_LINE
static char *GetCoordinateString __PROTO((char *s, double x, double y));
#endif
static void GetRulerString __PROTO((char *p, double x, double y));
static void apply_zoom __PROTO((struct t_zoom * z));
static void do_zoom __PROTO((double xmin, double ymin, double x2min,
			     double y2min, double xmax, double ymax, double x2max, double y2max));
static void ZoomNext __PROTO((void));
static void ZoomPrevious __PROTO((void));
static void ZoomUnzoom __PROTO((void));
static void incr_mousemode __PROTO((const int amount));
static void incr_clipboardmode __PROTO((const int amount));
static void UpdateStatuslineWithMouseSetting __PROTO((mouse_setting_t * ms));

static void event_keypress __PROTO((struct gp_event_t * ge));
static void ChangeView __PROTO((int x, int z));
static void event_buttonpress __PROTO((struct gp_event_t * ge));
static void event_buttonrelease __PROTO((struct gp_event_t * ge));
static void event_motion __PROTO((struct gp_event_t * ge));
static void event_modifier __PROTO((struct gp_event_t * ge));
static void event_print __PROTO((FILE * fp, char *s));
static void do_save_3dplot __PROTO((struct surface_points *, int, int));

/* builtins */
static char *builtin_autoscale __PROTO((struct gp_event_t * ge));
static char *builtin_toggle_border __PROTO((struct gp_event_t * ge));
static char *builtin_replot __PROTO((struct gp_event_t * ge));
static char *builtin_toggle_grid __PROTO((struct gp_event_t * ge));
static char *builtin_help __PROTO((struct gp_event_t * ge));
static char *builtin_toggle_log __PROTO((struct gp_event_t * ge));
static char *builtin_nearest_log __PROTO((struct gp_event_t * ge));
static char *builtin_toggle_mouse __PROTO((struct gp_event_t * ge));
static char *builtin_toggle_ruler __PROTO((struct gp_event_t * ge));
static char *builtin_decrement_mousemode __PROTO((struct gp_event_t * ge));
static char *builtin_increment_mousemode __PROTO((struct gp_event_t * ge));
static char *builtin_decrement_clipboardmode __PROTO((struct gp_event_t * ge));
static char *builtin_increment_clipboardmode __PROTO((struct gp_event_t * ge));
static char *builtin_toggle_polardistance __PROTO((struct gp_event_t * ge));
static char *builtin_toggle_verbose __PROTO((struct gp_event_t * ge));
static char *builtin_toggle_ratio __PROTO((struct gp_event_t * ge));
static char *builtin_zoom_next __PROTO((struct gp_event_t * ge));
static char *builtin_zoom_previous __PROTO((struct gp_event_t * ge));
static char *builtin_unzoom __PROTO((struct gp_event_t * ge));
static char *builtin_rotate_right __PROTO((struct gp_event_t * ge));
static char *builtin_rotate_up __PROTO((struct gp_event_t * ge));
static char *builtin_rotate_left __PROTO((struct gp_event_t * ge));
static char *builtin_rotate_down __PROTO((struct gp_event_t * ge));
static char *builtin_cancel_zoom __PROTO((struct gp_event_t * ge));

/* prototypes for bind stuff
 * which are used only here. */
static void bind_install_default_bindings __PROTO((void));
static void bind_clear __PROTO((bind_t * b));
static int lookup_key __PROTO((char *ptr, int *len));
static int bind_scan_lhs __PROTO((bind_t * out, const char *in));
static char *bind_fmt_lhs __PROTO((const bind_t * in));
static int bind_matches __PROTO((const bind_t * a, const bind_t * b));
static void bind_display_one __PROTO((bind_t * ptr));
static void bind_display __PROTO((char *lhs));
static void bind_remove __PROTO((bind_t * b));
static void bind_append __PROTO((char *lhs, char *rhs, char *(*builtin) (struct gp_event_t * ge)));
/* void bind_process __PROTO((char *lhs, char *rhs)); */
/* void bind_remove_all __PROTO((void)); */
static void recalc_ruler_pos __PROTO((void));
static void turn_ruler_off __PROTO((void));
static void remove_label __PROTO((int x, int y));
static void put_label __PROTO((char *label, double x, double y));
# ifdef OS2
void send_gpPMmenu __PROTO((FILE * PM_pipe));
# endif

/********* functions ********************************************/

/* produce a beep */
static void
alert(void)
{
# ifdef GNUPMDRV
    DosBeep(444, 111);
# else
#  ifdef HAVE_LIBREADLINE
    ding();
    fflush(rl_outstream);
#  else
    fprintf(stderr, "\a");
#  endif
# endif
}

/* always include the prototype. The prototype might even not be
 * declared if the system supports stpcpy(). E.g. on Linux I would
 * have to define __USE_GNU before including string.h to get the
 * prototype (joze) */
char *stpcpy __PROTO((char *s, char *p));

# ifndef HAVE_STPCPY
/* handy function for composing strings; note: some platforms may
 * already provide it, how should we handle that? autoconf? -- ptdb */
char *
stpcpy(char *s, char *p)
{
    strcpy(s, p);
    return s + strlen(p);
}
# endif


/* a macro to check whether 2D functionality is allowed: 
   either the plot is a 2D plot, or it is a suitably oriented 3D plot
*/
# define ALMOST2D      \
    ( !is_3d_plot ||  \
      ( fabs(fmod(surface_rot_z,90.0))<0.1  \
        && (surface_rot_x>179.9 || surface_rot_x<0.1) ) )


/* main job of transformation, which is not device dependent
*/
static void
MousePosToGraphPosReal(int xx, int yy, double *x, double *y, double *x2, double *y2)
{
    if (!is_3d_plot) {
# if 0
	printf("POS: xleft=%i, xright=%i, ybot=%i, ytop=%i\n",
	       xleft, xright, ybot, ytop);
# endif
	if (xright == xleft)
	    *x = *x2 = 1e38;	/* protection */
	else {
	    *x =
		min_array[FIRST_X_AXIS] + ((double) xx - xleft) / (xright - xleft) * (max_array[FIRST_X_AXIS] -
										      min_array[FIRST_X_AXIS]);
	    *x2 =
		min_array[SECOND_X_AXIS] + ((double) xx - xleft) / (xright - xleft) * (max_array[SECOND_X_AXIS] -
										       min_array[SECOND_X_AXIS]);
	}
	if (ytop == ybot)
	    *y = *y2 = 1e38;	/* protection */
	else {
	    *y =
		min_array[FIRST_Y_AXIS] + ((double) yy - ybot) / (ytop - ybot) * (max_array[FIRST_Y_AXIS] -
										  min_array[FIRST_Y_AXIS]);
	    *y2 =
		min_array[SECOND_Y_AXIS] + ((double) yy - ybot) / (ytop - ybot) * (max_array[SECOND_Y_AXIS] -
										   min_array[SECOND_Y_AXIS]);
	}
# if 0
	printf("POS: xx=%i, yy=%i  =>  x=%g  y=%g\n", xx, yy, *x, *y);
# endif
    } else {
	/* for 3D plots, we treat the mouse position as if it is
	 * in the bottom plane, i.e., the plane of the x and y axis */
	/* note: at present, this projection is only correct if
	 * surface_rot_z is a multiple of 90 degrees! */
	xx -= axis3d_o_x;
	yy -= axis3d_o_y;
	if (abs(axis3d_x_dx) > abs(axis3d_x_dy)) {
	    *x = min_array[FIRST_X_AXIS] + ((double) xx) / axis3d_x_dx * (max_array[FIRST_X_AXIS] - min_array[FIRST_X_AXIS]);
	} else {
	    *x = min_array[FIRST_X_AXIS] + ((double) yy) / axis3d_x_dy * (max_array[FIRST_X_AXIS] - min_array[FIRST_X_AXIS]);
	}
	if (abs(axis3d_y_dx) > abs(axis3d_y_dy)) {
	    *y = min_array[FIRST_Y_AXIS] + ((double) xx) / axis3d_y_dx * (max_array[FIRST_Y_AXIS] - min_array[FIRST_Y_AXIS]);
	} else {
	    *y = min_array[FIRST_Y_AXIS] + ((double) yy) / axis3d_y_dy * (max_array[FIRST_Y_AXIS] - min_array[FIRST_Y_AXIS]);
	}
	*x2 = *y2 = 1e38;	/* protection */
    }
    /*
       Note: there is xleft+0.5 in "#define map_x" in graphics.c, which
       makes no major impact here. It seems that the mistake of the real
       coordinate is at about 0.5%, which corresponds to the screen resolution.
       It would be better to round the distance to this resolution, and thus
       *x = xmin + rounded-to-screen-resolution (xdistance)
     */

    /* Now take into account possible log scales of x and y axes */
    if (is_log_x)
	*x = exp(*x * log_base_log_x);
    if (is_log_y)
	*y = exp(*y * log_base_log_y);
    if (is_log_x2)
	*x2 = exp(*x2 * log_base_log_x2);
    if (is_log_y2)
	*y2 = exp(*y2 * log_base_log_y2);
}

static char *
xy_format(void)
{
    static char format[0xff];
    format[0] = NUL;
    strcat(format, mouse_setting.fmt);
    strcat(format, ", ");
    strcat(format, mouse_setting.fmt);
    return format;
}

static char *
zoombox_format(void)
{
    static char format[0xff];
    format[0] = NUL;
    strcat(format, mouse_setting.fmt);
    strcat(format, "\r");
    strcat(format, mouse_setting.fmt);
    return format;
}

static char *
xy1_format(char *leader)
{
    static char format[0xff];
    format[0] = NUL;
    strcat(format, leader);
    strcat(format, mouse_setting.fmt);
    return format;
}

/* formats the information for an annotation (middle mouse button clicked)
 */
static char *
GetAnnotateString(char *s, double x, double y, int mode, char *fmt)
{
    if (mode == MOUSE_COORDINATES_XDATE || mode == MOUSE_COORDINATES_XTIME ||
	mode == MOUSE_COORDINATES_XDATETIME || mode == MOUSE_COORDINATES_TIMEFMT) {	/* time is on the x axis */
	char buf[0xff];
	char format[0xff] = "[%s, ";
	strcat(format, mouse_setting.fmt);
	strcat(format, "]");
	s += sprintf(s, format, xDateTimeFormat(x, buf, mode), y);
    } else if (mode == MOUSE_COORDINATES_FRACTIONAL) {
	double xrange = max_array[FIRST_X_AXIS] - min_array[FIRST_X_AXIS];
	double yrange = max_array[FIRST_Y_AXIS] - min_array[FIRST_Y_AXIS];
	/* calculate fractional coordinates.
	 * prevent division by zero */
	if (xrange) {
	    char format[0xff] = "/";
	    strcat(format, mouse_setting.fmt);
	    s += sprintf(s, format, (x - min_array[FIRST_X_AXIS]) / xrange);
	} else {
	    s += sprintf(s, "/(undefined)");
	}
	if (yrange) {
	    char format[0xff] = ", ";
	    strcat(format, mouse_setting.fmt);
	    strcat(format, "/");
	    s += sprintf(s, format, (y - min_array[FIRST_Y_AXIS]) / yrange);
	} else {
	    s += sprintf(s, ", (undefined)/");
	}
    } else if (mode == MOUSE_COORDINATES_REAL1) {
	s += sprintf(s, xy_format(), x, y);	/* w/o brackets */
    } else if (mode == MOUSE_COORDINATES_ALT && fmt) {
	s += sprintf(s, fmt, x, y);	/* user defined format */
    } else {
	s += sprintf(s, xy_format(), x, y);	/* usual x,y values */
    }
    return s;
}


/* Format x according to the date/time mouse mode. Uses and returns b as
   a buffer
 */
static char *
xDateTimeFormat(double x, char *b, int mode)
{
# ifndef SEC_OFFS_SYS
#  define SEC_OFFS_SYS 946684800
# endif
    time_t xtime_position = SEC_OFFS_SYS + x;
    struct tm *pxtime_position = gmtime(&xtime_position);
    switch (mode) {
    case MOUSE_COORDINATES_XDATE:
	sprintf(b, "%d. %d. %04d", pxtime_position->tm_mday, (pxtime_position->tm_mon) + 1,
# if 1
		(pxtime_position->tm_year) + ((pxtime_position->tm_year <= 68) ? 2000 : 1900)
# else
		((pxtime_position->tm_year) < 100) ? (pxtime_position->tm_year) : (pxtime_position->tm_year) - 100
		/*              (pxtime_position->tm_year)+1900 */
# endif
	    );
	break;
    case MOUSE_COORDINATES_XTIME:
	sprintf(b, "%d:%02d", pxtime_position->tm_hour, pxtime_position->tm_min);
	break;
    case MOUSE_COORDINATES_XDATETIME:
	sprintf(b, "%d. %d. %04d %d:%02d", pxtime_position->tm_mday, (pxtime_position->tm_mon) + 1,
# if 1
		(pxtime_position->tm_year) + ((pxtime_position->tm_year <= 68) ? 2000 : 1900),
# else
		((pxtime_position->tm_year) < 100) ? (pxtime_position->tm_year) : (pxtime_position->tm_year) - 100,
		/*              (pxtime_position->tm_year)+1900, */
# endif
		pxtime_position->tm_hour, pxtime_position->tm_min);
	break;
    case MOUSE_COORDINATES_TIMEFMT:
	gstrftime(b, 0xff, timefmt, x);
	break;
    default:
	sprintf(b, mouse_setting.fmt, x);
    }
    return b;
}




#define MKSTR(sp,x,idx,_format)  \
    if (datatype[idx]==TIME) {  \
	if (format_is_numeric[idx]) sp+=gstrftime(sp,40,timefmt,x);  \
	else sp+=gstrftime(sp,40,_format,x);  \
    } else sp+=sprintf(sp, mouse_setting.fmt ,x);


/* formats the information for an annotation (middle mouse button clicked)
 * returns pointer to the end of the string, for easy concatenation
*/
# ifdef OLD_STATUS_LINE
static char *
GetCoordinateString(char *s, double x, double y)
{
    char *sp;
    s[0] = '[';
    sp = s + 1;
    MKSTR(sp, x, FIRST_X_AXIS, xformat);
    *sp++ = ',';
    *sp++ = ' ';
    MKSTR(sp, y, FIRST_Y_AXIS, yformat);
    *sp++ = ']';
    *sp = 0;
    return sp;
}
# endif


# define DIST(x,rx,is_log)  \
    (is_log)  /* ratio for log, distance for linear */  \
    ? ( (rx==0) ? 99999 : x / rx )  \
    : (x - rx)


/* formats the ruler information (position, distance,...) into string p
	(it must be sufficiently long)
   x, y is the current mouse position in real coords (for the calculation 
	of distance)
*/
static void
GetRulerString(char *p, double x, double y)
{
    double dx, dy;

    char format[0xff] = "  ruler: [";
    strcat(format, mouse_setting.fmt);
    strcat(format, ", ");
    strcat(format, mouse_setting.fmt);
    strcat(format, "]  distance: ");
    strcat(format, mouse_setting.fmt);
    strcat(format, ", ");
    strcat(format, mouse_setting.fmt);

    dx = DIST(x, ruler.x, is_log_x);
    dy = DIST(y, ruler.y, is_log_y);
    sprintf(p, format, ruler.x, ruler.y, dx, dy);

    if (mouse_setting.polardistance && !is_log_x && !is_log_y) {
	/* polar coords of distance (axes cannot be logarithmic) */
	double rho = sqrt((x - ruler.x) * (x - ruler.x) + (y - ruler.y) * (y - ruler.y));
	double phi = (180 / M_PI) * atan2(y - ruler.y, x - ruler.x);
	char ptmp[69];

	format[0] = '\0';
	strcat(format, " (");
	strcat(format, mouse_setting.fmt);
# ifdef OS2
	strcat(format, ";% #.4gø)");
# else
	strcat(format, ", % #.4gdeg)");
# endif
	sprintf(ptmp, format, rho, phi);
	strcat(p, ptmp);
    }
}


struct t_zoom *zoom_head = NULL, *zoom_now = NULL;

/* Applies the zoom rectangle of  z  by sending the appropriate command
   to gnuplot
*/
static void
apply_zoom(struct t_zoom *z)
{
    char s[255];

    if (zoom_now != NULL) {	/* remember the current zoom */
	zoom_now->xmin = (!is_log_x) ? min_array[FIRST_X_AXIS] : exp(min_array[FIRST_X_AXIS] * log_base_log_x);
	zoom_now->ymin = (!is_log_y) ? min_array[FIRST_Y_AXIS] : exp(min_array[FIRST_Y_AXIS] * log_base_log_y);
	zoom_now->x2min = (!is_log_x2) ? min_array[SECOND_X_AXIS] : exp(min_array[SECOND_X_AXIS] * log_base_log_x2);
	zoom_now->y2min = (!is_log_y2) ? min_array[SECOND_Y_AXIS] : exp(min_array[SECOND_Y_AXIS] * log_base_log_y2);
	zoom_now->xmax = (!is_log_x) ? max_array[FIRST_X_AXIS] : exp(max_array[FIRST_X_AXIS] * log_base_log_x);
	zoom_now->ymax = (!is_log_y) ? max_array[FIRST_Y_AXIS] : exp(max_array[FIRST_Y_AXIS] * log_base_log_y);
	zoom_now->x2max = (!is_log_x2) ? max_array[SECOND_X_AXIS] : exp(max_array[SECOND_X_AXIS] * log_base_log_x2);
	zoom_now->y2max = (!is_log_y2) ? max_array[SECOND_Y_AXIS] : exp(max_array[SECOND_Y_AXIS] * log_base_log_y2);
    }
    zoom_now = z;
    if (zoom_now == NULL) {
	alert();
	return;
    }

    sprintf(s, "set xr[% #g:% #g]; set yr[% #g:% #g]", zoom_now->xmin, zoom_now->xmax, zoom_now->ymin, zoom_now->ymax);
    do_string(s);

    sprintf(s, "set x2r[% #g:% #g]; set y2r[% #g:% #g]",
	    zoom_now->x2min, zoom_now->x2max, zoom_now->y2min, zoom_now->y2max);
    do_string_replot(s);
}


/* makes a zoom: update zoom history, call gnuplot to set ranges + replot
*/

static void
do_zoom(double xmin, double ymin, double x2min, double y2min, double xmax, double ymax, double x2max, double y2max)
{
    struct t_zoom *z;
    if (zoom_head == NULL) {	/* queue not yet created, thus make its head */
	zoom_head = malloc(sizeof(struct t_zoom));
	zoom_head->prev = NULL;
	zoom_head->next = NULL;
    }
    if (zoom_now == NULL)
	zoom_now = zoom_head;
    if (zoom_now->next == NULL) {	/* allocate new item */
	z = malloc(sizeof(struct t_zoom));
	z->prev = zoom_now;
	z->next = NULL;
	zoom_now->next = z;
	z->prev = zoom_now;
    } else			/* overwrite next item */
	z = zoom_now->next;
    z->xmin = xmin;
    z->ymin = ymin;
    z->x2min = x2min;
    z->y2min = y2min;
    z->xmax = xmax;
    z->ymax = ymax;
    z->x2max = x2max;
    z->y2max = y2max;
    apply_zoom(z);
}


static void
ZoomNext(void)
{
    if (zoom_now == NULL || zoom_now->next == NULL)
	alert();
    else
	apply_zoom(zoom_now->next);
    if (display_ipc_commands()) {
	fprintf(stderr, "next zoom.\n");
    }
}

static void
ZoomPrevious(void)
{
    if (zoom_now == NULL || zoom_now->prev == NULL)
	alert();
    else
	apply_zoom(zoom_now->prev);
    if (display_ipc_commands()) {
	fprintf(stderr, "previous zoom.\n");
    }
}

static void
ZoomUnzoom(void)
{
    if (zoom_head == NULL || zoom_now == zoom_head)
	alert();
    else
	apply_zoom(zoom_head);
    if (display_ipc_commands()) {
	fprintf(stderr, "unzoom.\n");
    }
}

static void
incr_mousemode(const int amount)
{
    long int old = mouse_mode;
    mouse_mode += amount;
    if (MOUSE_COORDINATES_ALT == mouse_mode && !mouse_alt_string) {
	mouse_mode += amount;	/* stepping over */
    }
    if (mouse_mode > MOUSE_COORDINATES_ALT) {
	mouse_mode = MOUSE_COORDINATES_REAL;
    } else if (mouse_mode < MOUSE_COORDINATES_REAL) {
	mouse_mode = MOUSE_COORDINATES_ALT;
	if (MOUSE_COORDINATES_ALT == mouse_mode && !mouse_alt_string) {
	    mouse_mode--;	/* stepping over */
	}
    }
    UpdateStatusline();
    if (display_ipc_commands()) {
	fprintf(stderr, "switched mouse format from %ld to %ld\n", old, mouse_mode);
    }
}

static void
incr_clipboardmode(const int amount)
{
    long int old = clipboard_mode;
    clipboard_mode += amount;
    if (MOUSE_COORDINATES_ALT == clipboard_mode && !clipboard_alt_string) {
	clipboard_mode += amount;	/* stepping over */
    }
    if (clipboard_mode > MOUSE_COORDINATES_ALT) {
	clipboard_mode = MOUSE_COORDINATES_REAL;
    } else if (clipboard_mode < MOUSE_COORDINATES_REAL) {
	clipboard_mode = MOUSE_COORDINATES_ALT;
	if (MOUSE_COORDINATES_ALT == clipboard_mode && !clipboard_alt_string) {
	    clipboard_mode--;	/* stepping over */
	}
    }
    if (display_ipc_commands()) {
	fprintf(stderr, "switched clipboard format from %ld to %ld\n", old, clipboard_mode);
    }
}

# define TICS_ON(ti) (((ti)&TICS_MASK)!=NO_TICS)

void
UpdateStatusline(void)
{
    UpdateStatuslineWithMouseSetting(&mouse_setting);
}

static void
UpdateStatuslineWithMouseSetting(mouse_setting_t * ms)
{
    char s0[256], *sp;
    extern TBOOLEAN term_initialised;	/* term.c */
    if (!term_initialised)
	return;
    if (!ms->on) {
	s0[0] = 0;
    } else if (!ALMOST2D) {
	char format[0xff];
	format[0] = '\0';
	strcat(format, "view: ");
	strcat(format, ms->fmt);
	strcat(format, ", ");
	strcat(format, ms->fmt);
	strcat(format, "   scale: ");
	strcat(format, ms->fmt);
	strcat(format, ", ");
	strcat(format, ms->fmt);
	sprintf(s0, format, surface_rot_x, surface_rot_z, surface_scale, surface_zscale);
    } else if (!TICS_ON(x2tics) && !TICS_ON(y2tics)) {
	/* only first X and Y axis are in use */
# ifdef OLD_STATUS_LINE
	sp = GetCoordinateString(s0, real_x, real_y);
# else
	sp = GetAnnotateString(s0, real_x, real_y, mouse_mode, mouse_alt_string);
# endif
	if (ruler.on) {
# if 0
	    MousePosToGraphPosReal(ruler.px, ruler.py, &ruler.x, &ruler.y, &ruler.x2, &ruler.y2);
# endif
	    GetRulerString(sp, real_x, real_y);
	}
    } else {
	/* X2 and/or Y2 are in use: use more verbose format */
	sp = s0;
	if (TICS_ON(xtics)) {
	    sp = stpcpy(sp, "x=");
	    MKSTR(sp, real_x, FIRST_X_AXIS, xformat);
	    *sp++ = ' ';
	}
	if (TICS_ON(ytics)) {
	    sp = stpcpy(sp, "y=");
	    MKSTR(sp, real_y, FIRST_Y_AXIS, yformat);
	    *sp++ = ' ';
	}
	if (TICS_ON(x2tics)) {
	    sp = stpcpy(sp, "x2=");
	    MKSTR(sp, real_x2, SECOND_X_AXIS, x2format);
	    *sp++ = ' ';
	}
	if (TICS_ON(y2tics)) {
	    sp = stpcpy(sp, "y2=");
	    MKSTR(sp, real_y2, SECOND_Y_AXIS, y2format);
	    *sp++ = ' ';
	}
	if (ruler.on) {
	    /* ruler on? then also print distances to ruler */
# if 0
	    MousePosToGraphPosReal(ruler.px, ruler.py, &ruler.x, &ruler.y, &ruler.x2, &ruler.y2);
# endif
	    if (TICS_ON(xtics))
		sp += sprintf(sp, xy1_format("dx="), DIST(real_x, ruler.x, is_log_x));
	    if (TICS_ON(ytics))
		sp += sprintf(sp, xy1_format("dy="), DIST(real_y, ruler.y, is_log_y));
	    if (TICS_ON(x2tics))
		sp += sprintf(sp, xy1_format("dx2="), DIST(real_x2, ruler.x2, is_log_x2));
	    if (TICS_ON(y2tics))
		sp += sprintf(sp, xy1_format("dy2="), DIST(real_y2, ruler.y2, is_log_y2));
	}
	*--sp = 0;		/* delete trailing space */
    }
    if (term->put_tmptext) {
	(term->put_tmptext) (0, s0);
    }
}


void
recalc_statusline(void)
{
    MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
    UpdateStatusline();
}

/****************** handlers for user's actions ******************/

static char *
builtin_autoscale(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-autoscale` (set autoscale; replot)";
    }
    do_string_replot("set autoscale");
    return (char *) 0;
}

static char *
builtin_toggle_border(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-toggle-border`";
    }
    if (is_3d_plot) {
	if (draw_border == 4095)
	    do_string_replot("set border");
	else
	    do_string_replot("set border 4095 lw 0.5");
    } else {
	if (draw_border == 15)
	    do_string_replot("set border");
	else
	    do_string_replot("set border 15 lw 0.5");
    }
    return (char *) 0;
}

static char *
builtin_replot(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-replot`";
    }
    do_string_replot("");
    return (char *) 0;
}

static char *
builtin_toggle_grid(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-toggle-grid`";
    }
    if (work_grid.l_type == GRID_OFF)
	do_string_replot("set grid");
    else
	do_string_replot("unset grid");
    return (char *) 0;
}

static char *
builtin_help(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-help`";
    }
    fprintf(stderr, "\n");
    bind_display((char *) 0);	/* display all bindings */
    restore_prompt();
    return (char *) 0;
}

static char *
builtin_toggle_log(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-toggle-log` y logscale for plots, z logscale for splots";
    }
    if (is_3d_plot) {
	if (is_log_z)
	    do_string_replot("unset log z");
	else
	    do_string_replot("set log z");
    } else {
	if (is_log_y)
	    do_string_replot("unset log y");
	else
	    do_string_replot("set log y");
    }
    return (char *) 0;
}

static char *
builtin_nearest_log(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-nearest-log` toggle logscale of axis nearest cursor";
    }
    if (is_3d_plot) {
	/* 3D-plot: toggle lin/log z axis */
	if (is_log_z)
	    do_string_replot("unset log z");
	else
	    do_string_replot("set log z");
    } else {
	/* 2D-plot: figure out which axis/axes is/are
	 * close to the mouse cursor, and toggle those lin/log */
	/* note: here it is assumed that the x axis is at
	 * the bottom, x2 at top, y left and y2 right; it
	 * would be better to derive that from the ..tics settings */
	TBOOLEAN change = FALSE;
	if (mouse_y < ybot + (ytop - ybot) / 4 && mouse_x > xleft && mouse_x < xright) {
	    do_string(is_log_x ? "unset log x" : "set log x");
	    change = TRUE;
	}
	if (mouse_y > ytop - (ytop - ybot) / 4 && mouse_x > xleft && mouse_x < xright) {
	    do_string(is_log_x2 ? "unset log x2" : "set log x2");
	    change = TRUE;
	}
	if (mouse_x < xleft + (xright - xleft) / 4 && mouse_y > ybot && mouse_y < ytop) {
	    do_string(is_log_y ? "unset log y" : "set log y");
	    change = TRUE;
	}
	if (mouse_x > xright - (xright - xleft) / 4 && mouse_y > ybot && mouse_y < ytop) {
	    do_string(is_log_y2 ? "unset log y2" : "set log y2");
	    change = TRUE;
	}
	if (change)
	    do_string_replot("");
    }
    return (char *) 0;
}

static char *
builtin_toggle_mouse(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-toggle-mouse`";
    }
    if (!mouse_setting.on) {
	mouse_setting.on = 1;
	if (display_ipc_commands()) {
	    fprintf(stderr, "turning mouse on.\n");
	}
    } else {
	mouse_setting.on = 0;
	if (display_ipc_commands()) {
	    fprintf(stderr, "turning mouse off.\n");
	}
# if 0
	if (ruler.on) {
	    ruler.on = FALSE;
	    (*term->set_ruler) (-1, -1);
	}
# endif
    }
# ifdef OS2
    update_menu_items_PM_terminal();
# endif
    UpdateStatusline();
    return (char *) 0;
}

static char *
builtin_toggle_ruler(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-toggle-ruler`";
    }
    if (!term->set_ruler)
	return (char *) 0;
    if (ruler.on) {
	turn_ruler_off();
	if (display_ipc_commands()) {
	    fprintf(stderr, "turning ruler off.\n");
	}
    } else if (ALMOST2D) {
	/* only allow ruler, if the plot
	 * is 2d or a 3d `map' */
	ruler.on = TRUE;
	ruler.px = ge->mx;
	ruler.py = ge->my;
	MousePosToGraphPosReal(ruler.px, ruler.py, &ruler.x, &ruler.y, &ruler.x2, &ruler.y2);
	(*term->set_ruler) (ruler.px, ruler.py);
	if (display_ipc_commands()) {
	    fprintf(stderr, "turning ruler on.\n");
	}
    }
    UpdateStatusline();
    return (char *) 0;
}

static char *
builtin_decrement_mousemode(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-decrement-mousemode`";
    }
    incr_mousemode(-1);
    return (char *) 0;
}

static char *
builtin_increment_mousemode(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-increment-mousemode`";
    }
    incr_mousemode(1);
    return (char *) 0;
}

static char *
builtin_decrement_clipboardmode(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-decrement-clipboardmode`";
    }
    incr_clipboardmode(-1);
    return (char *) 0;
}

static char *
builtin_increment_clipboardmode(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-increment-clipboardmode`";
    }
    incr_clipboardmode(1);
    return (char *) 0;
}

static char *
builtin_toggle_polardistance(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-toggle-polardistance`";
    }
    mouse_setting.polardistance = !mouse_setting.polardistance;
# ifdef OS2
    update_menu_items_PM_terminal();
# endif
    UpdateStatusline();
    if (display_ipc_commands()) {
	fprintf(stderr, "distance to ruler will %s be shown in polar coordinates.\n", mouse_setting.polardistance ? "" : "not");
    }
    return (char *) 0;
}

static char *
builtin_toggle_verbose(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-toggle-verbose`";
    }
    /* this is tricky as the command itself modifies
     * the state of display_ipc_commands() */
    if (display_ipc_commands()) {
	fprintf(stderr, "echoing of communication commands is turned off.\n");
    }
    toggle_display_of_ipc_commands();
    if (display_ipc_commands()) {
	fprintf(stderr, "communication commands will be echoed.\n");
    }
    return (char *) 0;
}

static char *
builtin_toggle_ratio(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-toggle-ratio`";
    }
    if (aspect_ratio == 0)
	do_string_replot("set size ratio -1");
    else if (aspect_ratio == 1)
	do_string_replot("set size nosquare");
    else
	do_string_replot("set size square");
    return (char *) 0;
}

static char *
builtin_zoom_next(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-zoom-next` go to next zoom in the zoom stack";
    }
    ZoomNext();
    return (char *) 0;
}

static char *
builtin_zoom_previous(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-zoom-previous` go to previous zoom in the zoom stack";
    }
    ZoomPrevious();
    return (char *) 0;
}

static char *
builtin_unzoom(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-unzoom`";
    }
    ZoomUnzoom();
    return (char *) 0;
}

static char *
builtin_rotate_right(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-rotate-right` only for splots; <shift> increases amount";
    }
    if (is_3d_plot)
	ChangeView(0, -1);
    return (char *) 0;
}

static char *
builtin_rotate_left(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-rotate-left` only for splots; <shift> increases amount";
    }
    if (is_3d_plot)
	ChangeView(0, 1);
    return (char *) 0;
}

static char *
builtin_rotate_up(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-rotate-up` only for splots; <shift> increases amount";
    }
    if (is_3d_plot)
	ChangeView(1, 0);
    return (char *) 0;
}

static char *
builtin_rotate_down(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-rotate-down` only for splots; <shift> increases amount";
    }
    if (is_3d_plot)
	ChangeView(-1, 0);
    return (char *) 0;
}

static char *
builtin_cancel_zoom(struct gp_event_t *ge)
{
    if (!ge) {
	return "`builtin-cancel-zoom` cancel zoom region";
    }
    if (!setting_zoom_region)
	return (char *) 0;
    if (term->set_cursor)
	term->set_cursor(0, 0, 0);
    setting_zoom_region = FALSE;
    if (display_ipc_commands()) {
	fprintf(stderr, "zooming cancelled.\n");
    }
    return (char *) 0;
}

static void
event_keypress(struct gp_event_t *ge)
{
    int x, y;
    int c, par2;
    bind_t *ptr;
    bind_t keypress;

    c = ge->par1;
    par2 = ge->par2;
    x = ge->mx;
    y = ge->my;

    if (!bindings) {
	bind_install_default_bindings();
    }

    if (modifier_mask & Mod_Shift) {
	c = toupper(c);
    }

    bind_clear(&keypress);
    keypress.key = c;
    keypress.modifier = modifier_mask;

    for (ptr = bindings; ptr; ptr = ptr->next) {
	if (bind_matches(&keypress, ptr)) {
	    if ((par2 & 1) == 0 && ptr->command) {
		/* let user defined bindings overwrite the builtin bindings */
		do_string(ptr->command);
		return;
	    } else if (ptr->builtin) {
		ptr->builtin(ge);
	    } else {
		fprintf(stderr, "%s:%d protocol error\n", __FILE__, __LINE__);
	    }
	}
    }
}


static void
ChangeView(int x, int z)
{
    if (modifier_mask & Mod_Shift) {
	x *= 10;
	z *= 10;
    }

    if (x) {
	surface_rot_x += x;
	if (surface_rot_x < 0)
	    surface_rot_x = 0;
	if (surface_rot_x > 180)
	    surface_rot_x = 180;
    }
    if (z) {
	surface_rot_z += z;
	if (surface_rot_z < 0)
	    surface_rot_z += 360;
	if (surface_rot_z > 360)
	    surface_rot_z -= 360;
    }

    if (display_ipc_commands()) {
	fprintf(stderr, "changing view to %f, %f.\n", surface_rot_x, surface_rot_z);
    }

    do_save_3dplot(first_3dplot, plot3d_num, 0 /* not quick */ );

    if (ALMOST2D) {
	/* 2D plot, or suitably aligned 3D plot: update statusline */
	if (!term->put_tmptext)
	    return;
	recalc_statusline();
    }
}


static void
event_buttonpress(struct gp_event_t *ge)
{
    int b;

    motion = 0;

    b = ge->par1;
    mouse_x = ge->mx;
    mouse_y = ge->my;

    button |= (1 << b);

    FPRINTF((stderr, "(event_buttonpress) mouse_x = %d\n", mouse_x));
    FPRINTF((stderr, "(event_buttonpress) mouse_y = %d\n", mouse_y));

    MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);

    if (ALMOST2D) {
	if (!setting_zoom_region) {
	    if (1 == b) {
		/* not bound in 2d graphs */
	    } else if (2 == b) {
		/* not bound in 2d graphs */
	    } else if (3 == b) {
		/* start zoom */
		setting_zoom_x = mouse_x;
		setting_zoom_y = mouse_y;
		setting_zoom_region = TRUE;
		if (term->set_cursor) {
		    int mv_mouse_x, mv_mouse_y;
		    if (mouse_setting.annotate_zoom_box && term->put_tmptext) {
			double real_x, real_y, real_x2, real_y2;
			char s[64];
			/* tell driver annotations */
			MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
			sprintf(s, zoombox_format(), real_x, real_y);
			term->put_tmptext(1, s);
			term->put_tmptext(2, s);
		    }
		    /* displace mouse in order not to start with an empty zoom box */
		    mv_mouse_x = term->xmax / 20;
		    mv_mouse_y = (term->xmax == term->ymax) ? mv_mouse_x : (int) ((mv_mouse_x * (double) term->ymax) / term->xmax);
		    mv_mouse_x += mouse_x;
		    mv_mouse_y += mouse_y;

		    /* change cursor type */
		    term->set_cursor(3, 0, 0);

		    /* warp pointer */
		    if (mouse_setting.warp_pointer)
			term->set_cursor(-2, mv_mouse_x, mv_mouse_y);

		    /* turn on the zoom box */
		    term->set_cursor(-1, setting_zoom_x, setting_zoom_y);
		}
		if (display_ipc_commands()) {
		    fprintf(stderr, "starting zoom region.\n");
		}
	    }
	} else {

	    /* complete zoom (any button
	     * finishes zooming.) */

	    /* the following variables are used to check,
	     * if the box is big enough to be considered
	     * as zoom box. */
	    int dist_x = setting_zoom_x - mouse_x;
	    int dist_y = setting_zoom_y - mouse_y;
	    int dist = sqrt(dist_x * dist_x + dist_y * dist_y);

	    if (1 == b || 2 == b) {
		/* zoom region is finished by the `wrong' button.
		 * `trap' the next button-release event so that
		 * it won't trigger the actions which are bound
		 * to these events.
		 */
		trap_release = TRUE;
	    }

	    if (term->set_cursor) {
		term->set_cursor(0, 0, 0);
		if (mouse_setting.annotate_zoom_box && term->put_tmptext) {
		    term->put_tmptext(1, "");
		    term->put_tmptext(2, "");
		}
	    }

	    if (dist > 10 /* more ore less arbitrary */ ) {

		double xmin, ymin, x2min, y2min;
		double xmax, ymax, x2max, y2max;

		MousePosToGraphPosReal(setting_zoom_x, setting_zoom_y, &xmin, &ymin, &x2min, &y2min);
		xmax = real_x;
		x2max = real_x2;
		ymax = real_y;
		y2max = real_y2;
#define swap(x, y) do { double h = x; x = y; y = h; } while (0)
		if (xmin > xmax) {
		    swap(xmin, xmax);
		}
		if (ymin > ymax) {
		    swap(ymin, ymax);
		}
		if (x2min > x2max) {
		    swap(x2min, x2max);
		}
		if (y2min > y2max) {
		    swap(y2min, y2max);
		}
#undef swap
		do_zoom(xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
		if (display_ipc_commands()) {
		    fprintf(stderr, "zoom region finished.\n");
		}
	    } else {
		/* silently ignore a tiny zoom box. This might
		 * happen, if the user starts and finishes the
		 * zoom box at the same position. */
	    }
	    setting_zoom_region = FALSE;
	}
    } else {
	if (term->set_cursor) {
	    if (button & (1 << 1))
		term->set_cursor(1, 0, 0);
	    else if (button & (1 << 2))
		term->set_cursor(2, 0, 0);
	}
    }
    start_x = mouse_x;
    start_y = mouse_y;
    zero_rot_z = surface_rot_z + 360.0 * mouse_x / term->xmax;
    zero_rot_x = surface_rot_x - 180.0 * mouse_y / term->ymax;
}


static void
event_buttonrelease(struct gp_event_t *ge)
{
    int b, doubleclick;

    b = ge->par1;
    mouse_x = ge->mx;
    mouse_y = ge->my;
    doubleclick = ge->par2;

    button &= ~(1 << b);	/* remove button */

    if (setting_zoom_region) {
	return;
    }
    if (TRUE == trap_release) {
	trap_release = FALSE;
	return;
    }

    MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);

    FPRINTF(("MOUSE.C: doublclick=%i, set=%i, motion=%i, ALMOST2D=%i\n", (int) doubleclick, (int) mouse_setting.doubleclick, (int) motion, (int) ALMOST2D));

    if (ALMOST2D) {
	char s0[256];
	if (b == 1 && term->set_clipboard && ((doubleclick <= mouse_setting.doubleclick)
					      || !mouse_setting.doubleclick)) {

	    /* put coordinates to clipboard. For 3d plots this takes
	     * only place, if the user didn't drag (rotate) the plot */

	    if (!is_3d_plot || !motion) {
		GetAnnotateString(s0, real_x, real_y, clipboard_mode, clipboard_alt_string);
		term->set_clipboard(s0);
		if (display_ipc_commands()) {
		    fprintf(stderr, "put `%s' to clipboard.\n", s0);
		}
	    }
	}
	if (b == 2) {

	    /* draw temporary annotation or label. For 3d plots this takes
	     * only place, if the user didn't drag (scale) the plot */

	    if (!is_3d_plot || !motion) {

		GetAnnotateString(s0, real_x, real_y, mouse_mode, mouse_alt_string);
		if (mouse_setting.label) {
		    if (modifier_mask & Mod_Ctrl) {
			remove_label(mouse_x, mouse_y);
		    } else {
			put_label(s0, real_x, real_y);
		    }
		} else {
		    int dx, dy;
		    int x = mouse_x;
		    int y = mouse_y;
		    dx = term->h_tic;
		    dy = term->v_tic;
		    (term->linewidth) (border_lp.l_width);
		    (term->linetype) (border_lp.l_type);
		    (term->move) (x - dx, y);
		    (term->vector) (x + dx, y);
		    (term->move) (x, y - dy);
		    (term->vector) (x, y + dy);
		    (term->justify_text) (LEFT);
		    (term->put_text) (x + dx / 2, y + dy / 2 + term->v_char / 3, s0);
		    (term->text) ();
		}
	    }
	}
    }
    if (is_3d_plot && (b == 1 || b == 2)) {
	if (!!(modifier_mask & Mod_Ctrl) && !needreplot) {
	    /* redraw the 3d plot if its last redraw was 'quick'
	     * (only axes) because modifier key was pressed */
	    do_save_3dplot(first_3dplot, plot3d_num, 0);
	}
	if (term->set_cursor)
	    term->set_cursor(0, 0, 0);
    }

    MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
    UpdateStatusline();
}

static void
event_motion(struct gp_event_t *ge)
{
    motion = 1;

    mouse_x = ge->mx;
    mouse_y = ge->my;

    if (is_3d_plot) {
	TBOOLEAN redraw = FALSE;

	if (button & (1 << 1)) {

	    /* dragging with button 1 -> rotate */
	    surface_rot_x = rint(zero_rot_x + 180.0 * mouse_y / term->ymax);
	    if (surface_rot_x < 0)
		surface_rot_x = 0;
	    if (surface_rot_x > 180)
		surface_rot_x = 180;
	    surface_rot_z = rint(fmod(zero_rot_z - 360.0 * mouse_x / term->xmax, 360));
	    if (surface_rot_z < 0)
		surface_rot_z += 360;
	    redraw = TRUE;

	} else if (button & (1 << 2)) {

	    /* dragging with button 2 -> scale or changing ticslevel.
	     * we compare the movement in x and y direction, and
	     * change either scale or zscale */
	    double relx, rely;
	    relx = fabs(mouse_x - start_x) / term->h_tic;
	    rely = fabs(mouse_y - start_y) / term->v_tic;

# if 0
	    /* threshold: motion should be at least 3 pixels.
	     * We've to experiment with this. */
	    if (relx < 3 && rely < 3)
		return;
# endif
	    if (modifier_mask & Mod_Shift) {
		ticslevel += (1 + fabs(ticslevel))
		    * (mouse_y - start_y) * 2.0 / term->ymax;
	    } else {

		if (relx > rely) {
		    surface_scale += (mouse_x - start_x) * 2.0 / term->xmax;
		    if (surface_scale < 0)
			surface_scale = 0;
		} else {
		    surface_zscale += (mouse_y - start_y) * 2.0 / term->ymax;
		    if (surface_zscale < 0)
			surface_zscale = 0;
		}
	    }
	    /* reset the start values */
	    start_x = mouse_x;
	    start_y = mouse_y;
	    redraw = TRUE;
	}

	if (!ALMOST2D) {
	    turn_ruler_off();
	}

	if (redraw) {
	    if (allowmotion) {
		/* is processing of motions allowed right now?
		 * then replot while 
		 * disabling further replots until it completes */
		allowmotion = FALSE;
		do_save_3dplot(first_3dplot, plot3d_num, !!(modifier_mask & Mod_Ctrl));
	    } else {
		/* postpone the replotting */
		needreplot = TRUE;
	    }
	}
    }


    if (ALMOST2D) {
	/* 2D plot, or suitably aligned 3D plot: update
	 * statusline and possibly the zoombox annotation */
	if (!term->put_tmptext)
	    return;
	MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
	UpdateStatusline();

	if (setting_zoom_region && mouse_setting.annotate_zoom_box) {
	    double real_x, real_y, real_x2, real_y2;
	    char s[64];
	    MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
	    sprintf(s, zoombox_format(), real_x, real_y);
	    term->put_tmptext(2, s);
	}
    }
}


static void
event_modifier(struct gp_event_t *ge)
{
    modifier_mask = ge->par1;

    if (modifier_mask == 0 && is_3d_plot && (button & ((1 << 1) | (1 << 2))) && !needreplot) {
	/* redraw the 3d plot if modifier key released */
	do_save_3dplot(first_3dplot, plot3d_num, 0);
    }
}


void
event_plotdone()
{
    if (needreplot) {
	needreplot = FALSE;
	do_save_3dplot(first_3dplot, plot3d_num, !!(modifier_mask & Mod_Ctrl));
    } else {
	allowmotion = TRUE;
    }
}


static void
event_print(FILE * fp, char *s)
{
    fputs(s, fp);
    fflush(fp);
}

void
event_reset(struct gp_event_t *ge)
{
    modifier_mask = 0;
    button = 0;
    builtin_cancel_zoom(ge);
    if (term && term->set_cursor) {
	term->set_cursor(0, 0, 0);
	if (mouse_setting.annotate_zoom_box && term->put_tmptext) {
	    term->put_tmptext(1, "");
	    term->put_tmptext(2, "");
	}
    }
}

void
do_event(struct gp_event_t *ge)
{
    if (multiplot) {
	/* no event processing for multiplot */
	return;
    }

    /* disable `replot` when some data were sent through stdin */
    replot_disabled = plotted_data_from_stdin;

    if (ge->type) {
	FPRINTF((stderr, "(do_event) type       = %d\n", ge->type));
	FPRINTF((stderr, "           mx, my     = %d, %d\n", ge->mx, ge->my));
	FPRINTF((stderr, "           par1, par2 = %d, %d\n", ge->par1, ge->par2));
	FPRINTF((stderr, "           text       = %s\n", ge->text));
    }

    switch (ge->type) {
    case GE_stdout:
	event_print(stdout, ge->text);
	break;
    case GE_stderr:
	event_print(stderr, ge->text);
	break;
    case GE_plotdone:
	event_plotdone();
	break;
    case GE_keypress:
	event_keypress(ge);
	break;
    case GE_modifier:
	event_modifier(ge);
	break;
    case GE_motion:
	if (!mouse_setting.on)
	    break;
	event_motion(ge);
	break;
    case GE_buttonpress:
	if (!mouse_setting.on)
	    break;
	event_buttonpress(ge);
	break;
    case GE_buttonrelease:
	if (!mouse_setting.on)
	    break;
	event_buttonrelease(ge);
	break;
    case GE_cmd:
	do_string(ge->text);
	break;
    case GE_reset:
	event_reset(ge);
	break;
    default:
	fprintf(stderr, "%s:%d protocol error\n", __FILE__, __LINE__);
	break;
    }

    replot_disabled = 0; /* enable replot again */
}

static void
do_save_3dplot(struct surface_points *plots, int pcount, int quick)
{
    if (!plots) {
	/* this might happen after the `reset' command for example
	 * which was reported by Franz Bakan.  replotrequest()
	 * should set up again everything. */
	replotrequest();
    } else {
	do_3dplot(plots, pcount, quick);
    }
}


/*
 * bind related functions
 */

static void
bind_install_default_bindings(void)
{
    bind_remove_all();
    bind_append("a", (char *) 0, builtin_autoscale);
    bind_append("b", (char *) 0, builtin_toggle_border);
    bind_append("e", (char *) 0, builtin_replot);
    bind_append("g", (char *) 0, builtin_toggle_grid);
    bind_append("h", (char *) 0, builtin_help);
    bind_append("l", (char *) 0, builtin_toggle_log);
    bind_append("L", (char *) 0, builtin_nearest_log);
    bind_append("m", (char *) 0, builtin_toggle_mouse);
    bind_append("r", (char *) 0, builtin_toggle_ruler);
    bind_append("1", (char *) 0, builtin_decrement_mousemode);
    bind_append("2", (char *) 0, builtin_increment_mousemode);
    bind_append("3", (char *) 0, builtin_decrement_clipboardmode);
    bind_append("4", (char *) 0, builtin_increment_clipboardmode);
    bind_append("5", (char *) 0, builtin_toggle_polardistance);
    bind_append("6", (char *) 0, builtin_toggle_verbose);
    bind_append("7", (char *) 0, builtin_toggle_ratio);
    bind_append("n", (char *) 0, builtin_zoom_next);
    bind_append("p", (char *) 0, builtin_zoom_previous);
    bind_append("u", (char *) 0, builtin_unzoom);
    bind_append("Right", (char *) 0, builtin_rotate_right);
    bind_append("Up", (char *) 0, builtin_rotate_up);
    bind_append("Left", (char *) 0, builtin_rotate_left);
    bind_append("Down", (char *) 0, builtin_rotate_down);
    bind_append("Escape", (char *) 0, builtin_cancel_zoom);
}

static void
bind_clear(bind_t * b)
{
    b->key = NO_KEY;
    b->modifier = 0;
    b->command = (char *) 0;
    b->builtin = 0;
    b->prev = (struct bind_t *) 0;
    b->next = (struct bind_t *) 0;
}

/* returns the enum which corresponds to the
 * string (ptr) or NO_KEY if ptr matches not
 * any of special_keys. */
static int
lookup_key(char *ptr, int *len)
{
    char **keyptr;
    for (keyptr = special_keys; *keyptr; ++keyptr) {
	if (!strncasecmp(ptr, *keyptr, (*len = strlen(*keyptr)))) {
	    return keyptr - special_keys + GP_FIRST_KEY;
	}
    }
    return NO_KEY;
}

/* returns 1 on success, else 0. */
static int
bind_scan_lhs(bind_t * out, const char *in)
{
    static const char DELIM = '-';
    int itmp = NO_KEY;
    char *ptr;
    int len;
    bind_clear(out);
    if (!in) {
	return 0;
    }
    for (ptr = (char *) in; ptr && *ptr; /* EMPTY */ ) {
	if (!strncasecmp(ptr, "alt-", 4)) {
	    out->modifier |= Mod_Alt;
	    ptr += 4;
	} else if (!strncasecmp(ptr, "ctrl-", 5)) {
	    out->modifier |= Mod_Ctrl;
	    ptr += 5;
	} else if (NO_KEY != (itmp = lookup_key(ptr, &len))) {
	    out->key = itmp;
	    ptr += len;
	} else if ((out->key = *ptr++) && *ptr && *ptr != DELIM) {
	    fprintf(stderr, "bind: cannot parse %s\n", in);
	    return 0;
	}
    }
    if (NO_KEY == out->key)
	return 0;		/* failed */
    else
	return 1;		/* success */
}

/* note, that this returns a pointer
 * to the static char* `out' which is
 * modified on subsequent calls.
 */
static char *
bind_fmt_lhs(const bind_t * in)
{
    static char out[0x40];
    out[0] = '\0';		/* empty string */
    if (!in)
	return out;
    if (in->modifier & Mod_Ctrl) {
	sprintf(out, "Ctrl-");
    }
    if (in->modifier & Mod_Alt) {
	sprintf(out, "%sAlt-", out);
    }
    if (in->key > GP_FIRST_KEY && in->key < GP_LAST_KEY) {
	sprintf(out, "%s%s", out, special_keys[in->key - GP_FIRST_KEY]);
    } else {
	sprintf(out, "%s%c", out, in->key);
    }
    return out;
}

static int
bind_matches(const bind_t * a, const bind_t * b)
{
    /* discard Shift modifier */
    int a_mod = a->modifier & (Mod_Ctrl | Mod_Alt);
    int b_mod = b->modifier & (Mod_Ctrl | Mod_Alt);

    if (a->key == b->key && a_mod == b_mod)
	return 1;
    else
	return 0;
}

static void
bind_display_one(bind_t * ptr)
{
    fprintf(stderr, " %-12s  ", bind_fmt_lhs(ptr));
    if (ptr->command) {
	fprintf(stderr, "`%s'\n", ptr->command);
    } else if (ptr->builtin) {
	fprintf(stderr, "%s\n", ptr->builtin(0));
    } else {
	fprintf(stderr, "`%s:%d oops.'\n", __FILE__, __LINE__);
    }
}

static void
bind_display(char *lhs)
{
    bind_t *ptr;
    bind_t lhs_scanned;

    if (!bindings) {
	bind_install_default_bindings();
    }

    if (!lhs) {
	/* display all bindings */
	char fmt[] = " %-17s  %s\n";
	fprintf(stderr, "\n");
	fprintf(stderr, fmt, "<B1>",
		"print coordinates to clipboard using `clipboardformat`\n                    (see keys '3', '4')");
	fprintf(stderr, fmt, "<B2>", "annotate the graph using `mouseformat` (see keys '1', '2')");
	fprintf(stderr, fmt, "", "or draw labels if `set mouse labels is on`");
	fprintf(stderr, fmt, "<Ctrl-B2>", "remove label close to pointer if `set mouse labels` is on");
	fprintf(stderr, fmt, "<B3>", "mark zoom region (only for 2d-plots).");
	fprintf(stderr, fmt, "<B1-Motion>", "change view (rotation). Use <ctrl> to rotate the axes only.");
	fprintf(stderr, fmt, "<B2-Motion>", "change view (scaling). Use <ctrl> to scale the axes only.");
	fprintf(stderr, fmt, "<Shift-B2-Motion>", "vertical motion -- change ticslevel");
	fprintf(stderr, "\n");
	fprintf(stderr, " %-12s  %s\n", "Space", "raise gnuplot console window");
	fprintf(stderr, " %-12s  %s\n", "q", "quit X11 terminal");
	fprintf(stderr, "\n");
	for (ptr = bindings; ptr; ptr = ptr->next) {
	    bind_display_one(ptr);
	}
	fprintf(stderr, "\n");
	return;
    }

    if (!bind_scan_lhs(&lhs_scanned, lhs)) {
	return;
    }
    for (ptr = bindings; ptr; ptr = ptr->next) {
	if (bind_matches(&lhs_scanned, ptr)) {
	    bind_display_one(ptr);
	    break;		/* only one match */
	}
    }
}

static void
bind_remove(bind_t * b)
{
    if (!b) {
	return;
    } else if (b->builtin) {
	/* don't remove builtins, just remove the overriding command */
	if (b->command) {
	    free(b->command);
	    b->command = (char *) 0;
	}
	return;
    }
    if (b->prev)
	b->prev->next = b->next;
    if (b->next)
	b->next->prev = b->prev;
    else
	bindings->prev = b->prev;
    if (b->command) {
	free(b->command);
	b->command = (char *) 0;
    }
    if (b == bindings) {
	bindings = b->next;
	if (bindings && bindings->prev) {
	    bindings->prev->next = (bind_t *) 0;
	}
    }
    free(b);
}

static void
bind_append(char *lhs, char *rhs, char *(*builtin) (struct gp_event_t * ge))
{
    bind_t *new = (bind_t *) gp_alloc(sizeof(bind_t), "bind_append->new");

    if (!bind_scan_lhs(new, lhs)) {
	free(new);
	return;
    }

    if (!bindings) {
	/* first binding */
	bindings = new;
    } else {

	bind_t *ptr;
	for (ptr = bindings; ptr; ptr = ptr->next) {
	    if (bind_matches(new, ptr)) {
		/* overwriting existing binding */
		if (!rhs) {
		    ptr->builtin = builtin;
		} else if (*rhs) {
		    if (ptr->command) {
			free(ptr->command);
			ptr->command = (char *) 0;
		    }
		    ptr->command = rhs;
		} else {	/* rhs is an empty string, so remove the binding */
		    bind_remove(ptr);
		}
		free(new);	/* don't need it any more */
		return;
	    }
	}
	/* if we're here, the binding does not exist yet */
	/* append binding ... */
	bindings->prev->next = new;
	new->prev = bindings->prev;
    }

    bindings->prev = new;
    new->next = (struct bind_t *) 0;
    if (!rhs) {
	new->builtin = builtin;
    } else if (*rhs) {
	new->command = rhs;	/* was allocated in command.c */
    } else {
	bind_remove(new);
    }
}

void
bind_process(char *lhs, char *rhs)
{
    if (!bindings) {
	bind_install_default_bindings();
    }
    if (!rhs) {
	bind_display(lhs);
    } else {
	bind_append(lhs, rhs, 0);
    }
    if (lhs)
	free(lhs);
}

void
bind_remove_all(void)
{
    bind_t *ptr;
    bind_t *safe;
    for (ptr = bindings; ptr; safe = ptr, ptr = ptr->next, free(safe)) {
	if (ptr->command) {
	    free(ptr->command);
	    ptr->command = (char *) 0;
	}
    }
    bindings = (bind_t *) 0;
}

/* Ruler is on, thus recalc its (px,py) from (x,y) for the current zoom and 
   log axes.
*/
static void
recalc_ruler_pos(void)
{
    double P, dummy;
    if (is_log_x && ruler.x < 0)
	ruler.px = -1;
    else {
	P = is_log_x ? log(ruler.x) / log_base_log_x : ruler.x;
	P = (P - min_array[FIRST_X_AXIS]) / (max_array[FIRST_X_AXIS] - min_array[FIRST_X_AXIS]);
	P *= xright - xleft;
	ruler.px = (long) (xleft + P);
    }
    if (is_log_y && ruler.y < 0)
	ruler.py = -1;
    else {
	P = is_log_y ? log(ruler.y) / log_base_log_y : ruler.y;
	P = (P - min_array[FIRST_Y_AXIS]) / (max_array[FIRST_Y_AXIS] - min_array[FIRST_Y_AXIS]);
	P *= ytop - ybot;
	ruler.py = (long) (ybot + P);
    }
    MousePosToGraphPosReal(ruler.px, ruler.py, &dummy, &dummy, &ruler.x2, &ruler.y2);
}


/* Recalculate and replot the ruler after a '(re)plot'. Called from term.c.
*/
void
update_ruler(void)
{
    if (!term->set_ruler || !ruler.on)
	return;
    (*term->set_ruler) (-1, -1);
    recalc_ruler_pos();
    (*term->set_ruler) (ruler.px, ruler.py);
}

/* for checking if we change from plot to splot (or vice versa) */
int
plot_mode(int set)
{
    static int mode = MODE_PLOT;
    if (MODE_PLOT == set || MODE_SPLOT == set) {
	if (mode != set) {
	    turn_ruler_off();
	}
	mode = set;
    }
    return mode;
}

static void
turn_ruler_off(void)
{
    if (ruler.on) {
	ruler.on = FALSE;
	if (term && *term->set_ruler) {
	    (*term->set_ruler) (-1, -1);
	}
    }
}

static void
remove_label(int x, int y)
{
    int tag = nearest_label_tag(x, y, term,
				is_3d_plot ? map3d_position : map_position);
    if (-1 != tag) {
	char cmd[0x40];
	sprintf(cmd, "unset label %d", tag);
	do_string_replot(cmd);
    }
}

static void
put_label(char *label, double x, double y)
{
    char cmd[0xff];
    sprintf(cmd, "set label \"%s\" at %f,%f %s", label, x, y, mouse_setting.labelopts);
    do_string_replot(cmd);
}

# ifdef OS2
/* routine required by pm.trm: fill & send information needed for (un)checking
   menu items in the Presentation Manager terminal
*/
void
send_gpPMmenu(FILE * PM_pipe)
{
    struct t_gpPMmenu gpPMmenu;
    extern int mouseGnupmdrv;
    /* not connected to mouseable gnupmdrv */
    if (!PM_pipe || !mouseGnupmdrv)
	return;
    gpPMmenu.use_mouse = mouse_setting.on;
    if (zoom_now == NULL)
	gpPMmenu.where_zoom_queue = 0;
    else {
	gpPMmenu.where_zoom_queue = (zoom_now == zoom_head) ? 0 : 1;
	if (zoom_now->prev != NULL)
	    gpPMmenu.where_zoom_queue |= 2;
	if (zoom_now->next != NULL)
	    gpPMmenu.where_zoom_queue |= 4;
    }
    gpPMmenu.polar_distance = mouse_setting.polardistance;
    putc('#', PM_pipe);
    fwrite(&gpPMmenu, sizeof(gpPMmenu), 1, PM_pipe);
}

/* update menu items in PM terminal */
void
update_menu_items_PM_terminal(void)
{
    extern FILE *PM_pipe;
    send_gpPMmenu(PM_pipe);
}
# endif

#endif /* USE_MOUSE */
