#ifndef lint
static char *RCSid = "$Id: gplt_x11.c,v 1.71 1998/04/14 00:15:22 drd Exp $";
#endif

/* GNUPLOT - gplt_x11.c */

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
 * There is a mailing list for gnuplot users. Note, however, that the
 * newsgroup 
 *	comp.graphics.apps.gnuplot 
 * is identical to the mailing list (they
 * both carry the same set of messages). We prefer that you read the
 * messages through that newsgroup, to subscribing to the mailing list.
 * (If you can read that newsgroup, and are already on the mailing list,
 * please send a message to majordomo@dartmouth.edu, asking to be
 * removed from the mailing list.)
 *
 * The address for mailing to list members is
 *	   info-gnuplot@dartmouth.edu
 * and for mailing administrative requests is 
 *	   majordomo@dartmouth.edu
 * The mailing list for bug reports is 
 *	   bug-gnuplot@dartmouth.edu
 * The list of those interested in beta-test versions is
 *	   info-gnuplot-beta@dartmouth.edu
 *---------------------------------------------------------------------------*/

/* drd : export the graph via ICCCM primary selection. well... not quite
 * ICCCM since we dont support full list of targets, but this
 * is a start.  define EXPORT_SELECTION if you want this feature
 */

/*lph: add a "feature" to undefine EXPORT_SELECTION
   The following makes EXPORT_SELECTION the default and 
   defining NOEXPORT over-rides the default
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

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

#include <signal.h>

#ifdef HAVE_SYS_BSDTYPES_H
# include <sys/bsdtypes.h>
#endif /* HAVE_SYS_BSDTYPES_H */

#ifdef __EMX__
/* for gethostname ... */
# include <netdb.h>
#endif

#if defined(HAVE_SYS_SELECT_H) && !defined(VMS)
# include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H && !VMS */

#ifndef FD_SET
# define FD_SET(n, p)    ((p)->fds_bits[0] |= (1 << ((n) % 32)))
# define FD_CLR(n, p)    ((p)->fds_bits[0] &= ~(1 << ((n) % 32)))
# define FD_ISSET(n, p)  ((p)->fds_bits[0] & (1 << ((n) % 32)))
# define FD_ZERO(p)      memset((char *)(p),'\0',sizeof(*(p)))
#endif /* not FD_SET */

#include "plot.h"

#if defined(HAVE_SYS_SYSTEMINFO_H) && defined(HAVE_SYSINFO)
# include <sys/systeminfo.h>
# define SYSINFO_METHOD "sysinfo"
# define GP_SYSTEMINFO(host) sysinfo (SI_HOSTNAME, (host), MAXHOSTNAMELEN)
#else
# define SYSINFO_METHOD "gethostname"
# define GP_SYSTEMINFO(host) gethostname ((host), MAXHOSTNAMELEN)
#endif /* HAVE_SYS_SYSTEMINFO_H && HAVE_SYSINFO */

#ifdef VMS
# ifdef __DECC
#  include <starlet.h>
# endif				/* __DECC */
# define EXIT(status) sys$delprc(0,0)	/* VMS does not drop itself */
#else
# define EXIT(status) exit(status)
#endif

#ifdef OSK
# define EINTR	E_ILLFNC
#endif

/* information about one window/plot */

typedef struct plot_struct {
    Window window;
    Pixmap pixmap;
    unsigned int posn_flags;
    int x, y;
    unsigned int width, height;	/* window size */
    unsigned int px, py;	/* pointsize */
    int ncommands, max_commands;
    char **commands;
} plot_struct;

void store_command __PROTO((char *line, plot_struct * plot));
void prepare_plot __PROTO((plot_struct * plot, int term_number));
void delete_plot __PROTO((plot_struct * plot));

int record __PROTO((void));
void process_event __PROTO((XEvent * event));	/* from Xserver */

void mainloop __PROTO((void));

void display __PROTO((plot_struct * plot));

void reset_cursor __PROTO((void));

void preset __PROTO((int argc, char *argv[]));
char *pr_GetR __PROTO((XrmDatabase db, char *resource));
void pr_color __PROTO((void));
void pr_dashes __PROTO((void));
void pr_font __PROTO((void));
void pr_geometry __PROTO((void));
void pr_pointsize __PROTO((void));
void pr_width __PROTO((void));
Window pr_window __PROTO((unsigned int flags, int x, int y, unsigned int width, unsigned height));
void pr_raise __PROTO((void));
void pr_persist __PROTO((void));

#ifdef EXPORT_SELECTION
void export_graph __PROTO((plot_struct * plot));
void handle_selection_event __PROTO((XEvent * event));
#endif

#define FallbackFont "fixed"

#define Ncolors 13
unsigned long colors[Ncolors];

#define Nwidths 10
unsigned int widths[Nwidths] = { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#define Ndashes 10
char dashes[Ndashes][5];

#define MAX_WINDOWS 16

#define XC_crosshair 34


struct plot_struct plot_array[MAX_WINDOWS];


Display *dpy;
int scr;
Window root;
Visual *vis;
GC gc = (GC) 0;
XFontStruct *font;
int do_raise = 1, persist = 0;
KeyCode q_keycode;
Cursor cursor;

int windows_open = 0;

int gX = 100, gY = 100;
unsigned int gW = 640, gH = 450;
unsigned int gFlags = PSize;

unsigned int BorderWidth = 2;
unsigned int D;			/* depth */

Bool Mono = 0, Gray = 0, Rv = 0, Clear = 0;
char Name[64] = "gnuplot";
char Class[64] = "Gnuplot";

int cx = 0, cy = 0, vchar;
double xscale, yscale, pointsize;
#define X(x) (int) ((x) * xscale)
#define Y(y) (int) ((4095-(y)) * yscale)

#define Nbuf 1024
char buf[Nbuf], **commands = (char **) 0;

FILE *X11_ipc;
char X11_ipcpath[32];

/* when using an ICCCM-compliant window manager, we can ask it
 * to send us an event when user chooses 'close window'. We do this
 * by setting WM_DELETE_WINDOW atom in property WM_PROTOCOLS
 */

Atom WM_PROTOCOLS, WM_DELETE_WINDOW;

XPoint Diamond[5], Triangle[4];
XSegment Plus[2], Cross[2], Star[4];

/*-----------------------------------------------------------------------------
 *   main program 
 *---------------------------------------------------------------------------*/

int main(argc, argv)
int argc;
char *argv[];
{


#ifdef OSK
    /* malloc large blocks, otherwise problems with fragmented mem */
    _mallocmin(102400);
#endif
#ifdef __EMX__
    /* close open file handles */
    fcloseall();
#endif

    FPRINTF((stderr, "gnuplot_X11 starting up\n"));

    preset(argc, argv);

/* set up the alternative cursor */
    cursor = XCreateFontCursor(dpy, XC_crosshair);

    mainloop();

    if (persist) {
	FPRINTF((stderr, "waiting for %d windows\n, windows_open"));
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
/*-----------------------------------------------------------------------------
 *    DEFAULT_X11 mainloop
 *---------------------------------------------------------------------------*/

void mainloop()
{
    fd_set_size_t nf, nfds, cn = ConnectionNumber(dpy), in;
    struct timeval *timer = (struct timeval *) 0;
#ifdef ISC22
    struct timeval timeout;
#endif
    fd_set rset, tset;

    X11_ipc = stdin;
    in = fileno(X11_ipc);

    FD_ZERO(&rset);
    FD_SET(cn, &rset);

    FD_SET(in, &rset);
    nfds = (cn > in) ? cn + 1 : in + 1;

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

	tset = rset;
	nf = select(nfds, SELECT_FD_SET_CAST &tset, 0, 0, timer);
	if (nf < 0) {
	    if (errno == EINTR)
		continue;
	    fprintf(stderr, "gnuplot: select failed. errno:%d\n", errno);
	    EXIT(1);
	}
	if (nf > 0)
	    XNoOp(dpy);

	if (FD_ISSET(cn, &tset)) {
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
	if (FD_ISSET(in, &tset)) {
	    if (!record())      /* end of input */
		return;
	}
    }
}

#endif


#ifdef CRIPPLED_SELECT
/*-----------------------------------------------------------------------------
 *    CRIPPLED_SELECT mainloop
 *---------------------------------------------------------------------------*/

void mainloop()
{
    fd_set_size_t nf, nfds, cn = ConnectionNumber(dpy);
    struct timeval timeout, *timer;
    fd_set rset, tset;
    unsigned long all = (unsigned long) (-1L);
    XEvent xe;

    FD_ZERO(&rset);
    FD_SET(cn, &rset);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    timer = &timeout;
    sprintf(X11_ipcpath, "/tmp/Gnuplot_%d", getppid());
    nfds = cn + 1;

    while (1) {
	XFlush(dpy);		/* see above */
	tset = rset;
	nf = select(nfds, SELECT_FD_SET_CAST &tset, 0, 0, timer);
	if (nf < 0) {
	    if (errno == EINTR)
		continue;
	    fprintf(stderr, "gnuplot: select failed. errno:%d\n", errno);
	    EXIT(1);
	}
	nf > 0 && XNoOp(dpy);
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
#endif /* CRIPPLED_SELECT */


#ifdef VMS
/*-----------------------------------------------------------------------------
 *    VMS mainloop - Yehavi Bourvine - YEHAVI@VMS.HUJI.AC.IL
 *---------------------------------------------------------------------------*/

/*  In VMS there is no decent Select(). hence, we have to loop inside
 *  XGetNextEvent for getting the next X window event. In order to get input
 *  from the master we assign a channel to SYS$INPUT and use AST's in order to
 *  receive data. In order to exit the mainloop, we need to somehow make XNextEvent
 *  return from within the ast. We do this with a XSendEvent() to ourselves !
 *  This needs a window to send the message to, so we create an unmapped window
 *  for this purpose. Event type XClientMessage is perfect for this, but it
 *  appears that such messages come from elsewhere (motif window manager, perhaps ?)
 *  So we need to check fairly carefully that it is the ast event that has been received.
 */

#include <iodef.h>
char STDIIN[] = "SYS$INPUT:";
short STDIINchannel, STDIINiosb[4];
struct { short size, type; char *address; } STDIINdesc;
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

void mainloop()
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
	    if (xe.xclient.message_type == None &&
		xe.xclient.format == 8 &&
		STREQ(xe.xclient.data.b, "die gnuplot die")) {
		FPRINTF((stderr, "quit message from ast\n"));
		return;
	    } else {
		FPRINTF((stderr, "Bogus XClientMessage event from window manager ?\n"));
	    }
	}
	process_event(&xe);
    }
}

#endif /* VMS */

/* delete a window / plot */

void delete_plot(plot)
plot_struct *plot;
{
    int i;

    FPRINTF((stderr, "Delete plot %d\n", plot - plot_array));

    for (i = 0; i < plot->ncommands; ++i)
	free(plot->commands[i]);
    plot->ncommands = 0;

    if (plot->window) {
	FPRINTF((stderr, "Destroy window 0x%x\n", plot->window));
	XDestroyWindow(dpy, plot->window);
	plot->window = None;
	--windows_open;
    }
    if (plot->pixmap) {
	XFreePixmap(dpy, plot->pixmap);
	plot->pixmap = None;
    }
    /* but preserve geometry */
}


/* prepare the plot structure */

void prepare_plot(plot, term_number)
plot_struct *plot;
int term_number;
{
    int i;
    char *term_name;

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
    }

    if (!plot->window) {
	plot->window = pr_window(plot->posn_flags, plot->x, plot->y, plot->width, plot->height);
	++windows_open;

	/* append the X11 terminal number (if greater than zero) */

	if (term_number) {
	    char new_name[60];
	    XFetchName(dpy, plot->window, &term_name);
	    FPRINTF((stderr, "Window title is %s\n", term_name));

	    sprintf(new_name, "%.55s%3d", term_name, term_number);
	    FPRINTF((stderr, "term_number  is %d\n", term_number));

	    XStoreName(dpy, plot->window, new_name);

	    sprintf(new_name, "gplt%3d", term_number);
	    XSetIconName(dpy, plot->window, new_name);
	}
    }
/* We don't know that it is the same window as before, so we reset the
 * cursors for all windows and then define the cursor for the active
 * window 
 */
    reset_cursor();
    XDefineCursor(dpy, plot->window, cursor);

}

/* store a command in a plot structure */

void store_command(buffer, plot)
char *buffer;
plot_struct *plot;
{
    char *p;

    FPRINTF((stderr, "Store in %d : %s", plot - plot_array, buffer));

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
/*-----------------------------------------------------------------------------
 *   record - record new plot from gnuplot inboard X11 driver (Unix)
 *---------------------------------------------------------------------------*/

int record()
{
    static plot_struct *plot = plot_array;

    while (fgets(buf, Nbuf, X11_ipc)) {
	switch (*buf) {
	case 'G':		/* enter graphics mode */
	    {
		int plot_number = atoi(buf + 1);	/* 0 if none specified */

		if (plot_number < 0 || plot_number >= MAX_WINDOWS)
		    plot_number = 0;

		FPRINTF((stderr, "plot for window number %d\n", plot_number));
		plot = plot_array + plot_number;
		prepare_plot(plot, plot_number);
		continue;
	    }
	case 'E':		/* leave graphics mode / suspend */
	    display(plot);
	    return 1;
	case 'R':		/* leave x11 mode */
	    reset_cursor();
	    return 0;
	default:
	    store_command(buf, plot);
	    continue;
	}
    }

    /* get here if fgets fails */

#ifdef OSK
    if (feof(X11_ipc))		/* On OS-9 sometimes while resizing the window,  */
	_cleareof(X11_ipc);	/* and plotting data, the eof or error flag of   */
    if (ferror(X11_ipc))	/* X11_ipc stream gets set, while there is       */
	clearerr(X11_ipc);	/* nothing wrong! Probably a bug in my select()? */
#else
    if (feof(X11_ipc) || ferror(X11_ipc))
	return 0;
#endif /* not OSK */

    return 1;
}

#else /* VMS */
/*-----------------------------------------------------------------------------
 *   record - record new plot from gnuplot inboard X11 driver (VMS)
 *---------------------------------------------------------------------------*/

record()
{
    static plot_struct *plot = plot_array;

    int status;

    if ((STDIINiosb[0] & 0x1) == 0)
	EXIT(STDIINiosb[0]);
    STDIINbuffer[STDIINiosb[1]] = '\0';
    strcpy(buf, STDIINbuffer);

    switch (*buf) {
    case 'G':			/* enter graphics mode */
	{
	    int plot_number = atoi(buf + 1);	/* 0 if none specified */
	    if (plot_number < 0 || plot_number >= MAX_WINDOWS)
		plot_number = 0;
	    FPRINTF((stderr, "plot for window number %d\n", plot_number));
	    plot = plot_array + plot_number;
	    prepare_plot(plot, plot_number);
	    break;
	}
    case 'E':			/* leave graphics mode */
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
	store_command(buf, plot);
	break;
    }
    ast();
}
#endif /* VMS */


/*-----------------------------------------------------------------------------
 *   display - display a stored plot
 *---------------------------------------------------------------------------*/

void display(plot)
plot_struct *plot;
{
    int n, x, y, sw, sl, lt = 0, width, type, point, px, py;
    int user_width = 1;		/* as specified by plot...linewidth */
    char *buffer, *str;
    enum JUSTIFY jmode;

    FPRINTF((stderr, "Display %d ; %d commands\n", plot - plot_array, plot->ncommands));

    if (plot->ncommands == 0)
	return;

    /* set scaling factor between internal driver & window geometry */
    xscale = plot->width / 4096.0;
    yscale = plot->height / 4096.0;

    /* initial point sizes, until overridden with P7xxxxyyyy */
    px = (int) (xscale * pointsize);
    py = (int) (yscale * pointsize);

    /* create new pixmap & GC */
    if (gc)
	XFreeGC(dpy, gc);

    if (!plot->pixmap) {
	FPRINTF((stderr, "Create pixmap %d : %dx%dx%d\n", plot - plot_array, plot->width, plot->height, D));
	plot->pixmap = XCreatePixmap(dpy, root, plot->width, plot->height, D);
    }

    gc = XCreateGC(dpy, plot->pixmap, 0, (XGCValues *) 0);

    XSetFont(dpy, gc, font->fid);

    /* set pixmap background */
    XSetForeground(dpy, gc, colors[0]);
    XFillRectangle(dpy, plot->pixmap, gc, 0, 0, plot->width, plot->height);
    XSetBackground(dpy, gc, colors[0]);

    if (!plot->window) {
	plot->window = pr_window(plot->posn_flags, plot->x, plot->y, plot->width, plot->height);
	++windows_open;
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
	buffer = plot->commands[n];

	/*   X11_vector(x,y) - draw vector  */
	if (*buffer == 'V') {
	    sscanf(buffer, "V%4d%4d", &x, &y);
	    XDrawLine(dpy, plot->pixmap, gc, X(cx), Y(cy), X(x), Y(y));
	    cx = x;
	    cy = y;
	}
	/*   X11_move(x,y) - move  */
	else if (*buffer == 'M')
	    sscanf(buffer, "M%4d%4d", &cx, &cy);

	/*   X11_put_text(x,y,str) - draw text   */
	else if (*buffer == 'T') {
	    sscanf(buffer, "T%4d%4d", &x, &y);
	    str = buffer + 9;
	    sl = strlen(str) - 1;
	    sw = XTextWidth(font, str, sl);

	    switch (jmode) {
	    case LEFT:
		sw = 0;
		break;
	    case CENTRE:
		sw = -sw / 2;
		break;
	    case RIGHT:
		sw = -sw;
		break;
	    }

	    XSetForeground(dpy, gc, colors[2]);
	    XDrawString(dpy, plot->pixmap, gc, X(x) + sw, Y(y) + vchar / 3, str, sl);
	    XSetForeground(dpy, gc, colors[lt + 3]);
	} else if (*buffer == 'F') {	/* fill box */
	    int style, xtmp, ytmp, w, h;

	    if (sscanf(buffer + 1, "%4d%4d%4d%4d%4d", &style, &xtmp, &ytmp, &w, &h) == 5) {
		/* gnuplot has origin at bottom left, but X uses top left
		 * There may be an off-by-one (or more) error here.
		 * style ignored here for the moment
		 */
		ytmp += h;		/* top left corner of rectangle to be filled */
		w *= xscale;
		h *= yscale;
		XSetForeground(dpy, gc, colors[0]);
		XFillRectangle(dpy, plot->pixmap, gc, X(xtmp), Y(ytmp), w, h);
		XSetForeground(dpy, gc, colors[lt + 3]);
	    }
	}
	/*   X11_justify_text(mode) - set text justification mode  */
	else if (*buffer == 'J')
	    sscanf(buffer, "J%4d", (int *) &jmode);

	/*  X11_linewidth(width) - set line width */
	else if (*buffer == 'W')
	    sscanf(buffer + 1, "%4d", &user_width);

	/*   X11_linetype(type) - set line type  */
	else if (*buffer == 'L') {
	    sscanf(buffer, "L%4d", &lt);
	    lt = (lt % 8) + 2;
	    /* default width is 0 {which X treats as 1} */
	    width = widths[lt] ? user_width * widths[lt] : user_width;
	    if (dashes[lt][0]) {
		type = LineOnOffDash;
		XSetDashes(dpy, gc, 0, dashes[lt], strlen(dashes[lt]));
	    } else {
		type = LineSolid;
	    }
	    XSetForeground(dpy, gc, colors[lt + 3]);
	    XSetLineAttributes(dpy, gc, width, type, CapButt, JoinBevel);
	}
	/*   X11_point(number) - draw a point */
	else if (*buffer == 'P') {
	    /* linux sscanf does not like %1d%4d%4d" with Oxxxxyyyy */
	    /* sscanf(buffer, "P%1d%4d%4d", &point, &x, &y); */
	    point = buffer[1] - '0';
	    sscanf(buffer + 2, "%4d%4d", &x, &y);
	    if (point == 7) {
		/* set point size */
		px = (int) (x * xscale * pointsize);
		py = (int) (y * yscale * pointsize);
	    } else {
		if (type != LineSolid || width != 0) {	/* select solid line */
		    XSetLineAttributes(dpy, gc, 0, LineSolid, CapButt, JoinBevel);
		}
		switch (point) {
		case 0:	/* dot */
		    XDrawPoint(dpy, plot->pixmap, gc, X(x), Y(y));
		    break;
		case 1:	/* do diamond */
		    Diamond[0].x = (short) X(x) - px;
		    Diamond[0].y = (short) Y(y);
		    Diamond[1].x = (short) px;
		    Diamond[1].y = (short) -py;
		    Diamond[2].x = (short) px;
		    Diamond[2].y = (short) py;
		    Diamond[3].x = (short) -px;
		    Diamond[3].y = (short) py;
		    Diamond[4].x = (short) -px;
		    Diamond[4].y = (short) -py;

		    /*
		     * Should really do a check with XMaxRequestSize()
		     */
		    XDrawLines(dpy, plot->pixmap, gc, Diamond, 5, CoordModePrevious);
		    XDrawPoint(dpy, plot->pixmap, gc, X(x), Y(y));
		    break;
		case 2:	/* do plus */
		    Plus[0].x1 = (short) X(x) - px;
		    Plus[0].y1 = (short) Y(y);
		    Plus[0].x2 = (short) X(x) + px;
		    Plus[0].y2 = (short) Y(y);
		    Plus[1].x1 = (short) X(x);
		    Plus[1].y1 = (short) Y(y) - py;
		    Plus[1].x2 = (short) X(x);
		    Plus[1].y2 = (short) Y(y) + py;

		    XDrawSegments(dpy, plot->pixmap, gc, Plus, 2);
		    break;
		case 3:	/* do box */
		    XDrawRectangle(dpy, plot->pixmap, gc, X(x) - px, Y(y) - py, (px + px), (py + py));
		    XDrawPoint(dpy, plot->pixmap, gc, X(x), Y(y));
		    break;
		case 4:	/* do X */
		    Cross[0].x1 = (short) X(x) - px;
		    Cross[0].y1 = (short) Y(y) - py;
		    Cross[0].x2 = (short) X(x) + px;
		    Cross[0].y2 = (short) Y(y) + py;
		    Cross[1].x1 = (short) X(x) - px;
		    Cross[1].y1 = (short) Y(y) + py;
		    Cross[1].x2 = (short) X(x) + px;
		    Cross[1].y2 = (short) Y(y) - py;

		    XDrawSegments(dpy, plot->pixmap, gc, Cross, 2);
		    break;
		case 5:	/* do triangle */
		    {
			short temp_x, temp_y;

			temp_x = (short) (1.33 * (double) px + 0.5);
			temp_y = (short) (1.33 * (double) py + 0.5);

			Triangle[0].x = (short) X(x);
			Triangle[0].y = (short) Y(y) - temp_y;
			Triangle[1].x = (short) temp_x;
			Triangle[1].y = (short) 2 *py;
			Triangle[2].x = (short) -(2 * temp_x);
			Triangle[2].y = (short) 0;
			Triangle[3].x = (short) temp_x;
			Triangle[3].y = (short) -(2 * py);

			XDrawLines(dpy, plot->pixmap, gc, Triangle, 4, CoordModePrevious);
			XDrawPoint(dpy, plot->pixmap, gc, X(x), Y(y));
		    }
		    break;
		case 6:	/* do star */
		    Star[0].x1 = (short) X(x) - px;
		    Star[0].y1 = (short) Y(y);
		    Star[0].x2 = (short) X(x) + px;
		    Star[0].y2 = (short) Y(y);
		    Star[1].x1 = (short) X(x);
		    Star[1].y1 = (short) Y(y) - py;
		    Star[1].x2 = (short) X(x);
		    Star[1].y2 = (short) Y(y) + py;
		    Star[2].x1 = (short) X(x) - px;
		    Star[2].y1 = (short) Y(y) - py;
		    Star[2].x2 = (short) X(x) + px;
		    Star[2].y2 = (short) Y(y) + py;
		    Star[3].x1 = (short) X(x) - px;
		    Star[3].y1 = (short) Y(y) + py;
		    Star[3].x2 = (short) X(x) + px;
		    Star[3].y2 = (short) Y(y) - py;

		    XDrawSegments(dpy, plot->pixmap, gc, Star, 4);
		    break;
		}
		if (type != LineSolid || width != 0) {	/* select solid line */
		    XSetLineAttributes(dpy, gc, width, type, CapButt, JoinBevel);
		}
	    }
	}
    }

    /* set new pixmap as window background */
    XSetWindowBackgroundPixmap(dpy, plot->window, plot->pixmap);

    /* trigger exposure of background pixmap */
    XClearWindow(dpy, plot->window);

#ifdef EXPORT_SELECTION
    export_graph(plot);
#endif

    XFlush(dpy);
}

/*---------------------------------------------------------------------------
 *  reset all cursors (since we dont have a record of the previous terminal #)
 *---------------------------------------------------------------------------*/

void reset_cursor()
{
    int plot_number;
    plot_struct *plot = plot_array;

    for (plot_number = 0, plot = plot_array;
	 plot_number < MAX_WINDOWS;
	 ++plot_number, ++plot) {
	if (plot->window) {
	    FPRINTF((stderr, "Window for plot %d exists\n", plot_number));
	    XUndefineCursor(dpy, plot->window);;
	}
    }

    FPRINTF((stderr, "Cursors reset\n"));
    return;
}

/*-----------------------------------------------------------------------------
 *   resize - rescale last plot if window resized
 *---------------------------------------------------------------------------*/

plot_struct *find_plot(window)
Window window;
{
    int plot_number;
    plot_struct *plot = plot_array;

    for (plot_number = 0, plot = plot_array;
	 plot_number < MAX_WINDOWS;
	 ++plot_number, ++plot) {
	if (plot->window == window) {
	    FPRINTF((stderr, "Event for plot %d\n", plot_number));
	    return plot;
	}
    }

    FPRINTF((stderr, "Bogus window 0x%x in event !\n", window));
    return NULL;
}

void process_event(event)
XEvent *event;
{
    FPRINTF((stderr, "Event 0x%x\n", event->type));

    switch (event->type) {
    case ConfigureNotify:
	{
	    plot_struct *plot = find_plot(event->xconfigure.window);
	    if (plot) {
		int w = event->xconfigure.width, h = event->xconfigure.height;

		/* store settings in case window is closed then recreated */
		plot->x = event->xconfigure.x;
		plot->y = event->xconfigure.y;
		plot->posn_flags = (plot->posn_flags & ~PPosition) | USPosition;

		if (w > 1 && h > 1 && (w != plot->width || h != plot->height)) {
		    plot->width = w;
		    plot->height = h;
		    plot->posn_flags = (plot->posn_flags & ~PSize) | USSize;
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
	}
    case KeyPress:
	if (event->xkey.keycode == q_keycode) {
	    plot_struct *plot = find_plot(event->xkey.window);
	    if (plot)
		delete_plot(plot);
	}
	break;
    case ClientMessage:
	if (event->xclient.message_type == WM_PROTOCOLS &&
	    event->xclient.format == 32 &&
	    event->xclient.data.l[0] == WM_DELETE_WINDOW) {
	    plot_struct *plot = find_plot(event->xclient.window);
	    if (plot)
		delete_plot(plot);
	}
	break;
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

#define On(v) ( !strcmp(v,"on") || !strcmp(v,"true") || \
                !strcmp(v,"On") || !strcmp(v,"True") || \
                !strcmp(v,"ON") || !strcmp(v,"TRUE") )

#define AppDefDir "/usr/lib/X11/app-defaults"
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

static XrmDatabase dbCmd, dbApp, dbDef, dbEnv, db = (XrmDatabase) 0;

char *pr_GetR(), *getenv(), *type[20];
XrmValue value;

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
    {"-persist", "*persist", XrmoptionNoArg, (caddr_t) "on"}
};

#define Nopt (sizeof(options) / sizeof(options[0]))

void preset(argc, argv)
int argc;
char *argv[];
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
	    safe_strncpy(Name, Argv[1], sizeof(Name));
	    safe_strncpy(Class, Argv[1], sizeof(Class));
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
	fprintf(stderr, "\n\
gnuplot: bad option: %s\n\
gnuplot: X11 aborted.\n", Argv[1]);
	EXIT(1);
    }
    if (pr_GetR(dbCmd, ".display"))
	ldisplay = (char *) value.addr;

/*---open display---------------------------------------------------------*/

    dpy = XOpenDisplay(ldisplay);
    if (!dpy) {
	fprintf(stderr, "\n\
gnuplot: unable to open display '%s'\n\
gnuplot: X11 aborted.\n", ldisplay);
	EXIT(1);
    }
    scr = DefaultScreen(dpy);
    vis = DefaultVisual(dpy, scr);
    D = DefaultDepth(dpy, scr);
    root = DefaultRootWindow(dpy);
    server_defaults = XResourceManagerString(dpy);

/*---get symcode for key q ---*/

    q_keycode = XKeysymToKeycode(dpy, XK_q);

/**** atoms we will need later ****/

    WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
    WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);


/*---get application defaults--(subset of Xt processing)------------------*/

#ifdef VMS
    strcpy(buffer, "DECW$USER_DEFAULTS:GNUPLOT_X11.INI");
#else
# ifdef OS2
/* for Xfree86 ... */
    {
	char *appdefdir = "XFree86/lib/X11/app-defaults";
	char *xroot = getenv("X11ROOT");
	sprintf(buffer, "%s/%s/%s", xroot, appdefdir, "Gnuplot");
    }
# else
    sprintf(buffer, "%s/%s", AppDefDir, "Gnuplot");
# endif /* !OS2 */
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
	sprintf(buffer, "%s/.Xdefaults", home);
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
	sprintf(buffer, "%s/.Xdefaults-%s", home, host);
	dbEnv = XrmGetFileDatabase(buffer);
    }
    XrmMergeDatabases(dbEnv, &db);
#endif /* not VMS */

/*---merge command line options-------------------------------------------*/

    XrmMergeDatabases(dbCmd, &db);

/*---set geometry, font, colors, line widths, dash styles, point size-----*/

    pr_geometry();
    pr_font();
    pr_color();
    pr_width();
    pr_dashes();
    pr_pointsize();
    pr_raise();
    pr_persist();
}

/*-----------------------------------------------------------------------------
 *   pr_GetR - get resource from database using "-name" option (if any)
 *---------------------------------------------------------------------------*/

char *
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
	? (char *) value.addr
	: (char *) 0;
    return (rc);
}

/*-----------------------------------------------------------------------------
 *   pr_color - determine color values
 *---------------------------------------------------------------------------*/

char color_keys[Ncolors][30] = {
    "background", "bordercolor", "text", "border", "axis",
    "line1", "line2", "line3", "line4",
    "line5", "line6", "line7", "line8"
};
char color_values[Ncolors][30] = {
    "white", "black", "black", "black", "black",
    "red", "green", "blue", "magenta",
    "cyan", "sienna", "orange", "coral"
};
char gray_values[Ncolors][30] = {
    "black", "white", "white", "gray50", "gray50",
    "gray100", "gray60", "gray80", "gray40",
    "gray90", "gray50", "gray70", "gray30"
};

void pr_color()
{
    unsigned long black = BlackPixel(dpy, scr), white = WhitePixel(dpy, scr);
    char option[20], color[30], *v, *ctype;
    XColor xcolor;
    Colormap cmap;
    double intensity = -1;
    int n;

    pr_GetR(db, ".mono") && On(value.addr) && Mono++;
    pr_GetR(db, ".gray") && On(value.addr) && Gray++;
    pr_GetR(db, ".reverseVideo") && On(value.addr) && Rv++;

    if (!Gray && (vis->class == GrayScale || vis->class == StaticGray))
	Mono++;

    if (!Mono) {
	cmap = DefaultColormap(dpy, scr);
	ctype = (Gray) ? "Gray" : "Color";

	for (n = 0; n < Ncolors; n++) {
	    strcpy(option, ".");
	    strcat(option, color_keys[n]);
	    (n > 1) && strcat(option, ctype);
	    v = pr_GetR(db, option)
		? (char *) value.addr
		: ((Gray) ? gray_values[n] : color_values[n]);

	    if (sscanf(v, "%30[^,],%lf", color, &intensity) == 2) {
		if (intensity < 0 || intensity > 1) {
		    fprintf(stderr, "\ngnuplot: invalid color intensity in '%s'\n",
			    color);
		    intensity = 1;
		}
	    } else {
		strcpy(color, v);
		intensity = 1;
	    }

	    if (!XParseColor(dpy, cmap, color, &xcolor)) {
		fprintf(stderr, "\ngnuplot: unable to parse '%s'. Using black.\n",
			color);
		colors[n] = black;
	    } else {
		xcolor.red *= intensity;
		xcolor.green *= intensity;
		xcolor.blue *= intensity;
		if (XAllocColor(dpy, cmap, &xcolor)) {
		    colors[n] = xcolor.pixel;
		} else {
		    fprintf(stderr, "\ngnuplot: can't allocate '%s'. Using black.\n",
			    v);
		    colors[n] = black;
		}
	    }
	}
    } else {
	colors[0] = (Rv) ? black : white;
	for (n = 1; n < Ncolors; n++)
	    colors[n] = (Rv) ? white : black;
    }
}

/*-----------------------------------------------------------------------------
 *   pr_dashes - determine line dash styles 
 *---------------------------------------------------------------------------*/

char dash_keys[Ndashes][10] = {
    "border", "axis",
    "line1", "line2", "line3", "line4", "line5", "line6", "line7", "line8"
};

char dash_mono[Ndashes][10] = {
    "0", "16",
    "0", "42", "13", "44", "15", "4441", "42", "13"
};

char dash_color[Ndashes][10] = {
    "0", "16",
    "0", "0", "0", "0", "0", "0", "0", "0"
};

void pr_dashes()
{
    int n, j, l, ok;
    char option[20], *v;

    for (n = 0; n < Ndashes; n++) {
	strcpy(option, ".");
	strcat(option, dash_keys[n]);
	strcat(option, "Dashes");
	v = pr_GetR(db, option)
	    ? (char *) value.addr
	    : ((Mono) ? dash_mono[n] : dash_color[n]);
	l = strlen(v);
	if (l == 1 && *v == '0') {
	    dashes[n][0] = (unsigned char) 0;
	    continue;
	}
	for (ok = 0, j = 0; j < l; j++) {
	    v[j] >= '1' && v[j] <= '9' && ok++;
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

void pr_font()
{
    char *fontname = pr_GetR(db, ".font");

    if (!fontname)
	fontname = FallbackFont;
    font = XLoadQueryFont(dpy, fontname);
    if (!font) {
	fprintf(stderr, "\ngnuplot: can't load font '%s'\n", fontname);
	fprintf(stderr, "gnuplot: using font '%s' instead.\n", FallbackFont);
	font = XLoadQueryFont(dpy, FallbackFont);
	if (!font) {
	    fprintf(stderr, "\
gnuplot: can't load font '%s'\n\
gnuplot: no useable font - X11 aborted.\n", FallbackFont);
	    EXIT(1);
	}
    }
    vchar = font->ascent + font->descent;
}

/*-----------------------------------------------------------------------------
 *   pr_geometry - determine window geometry      
 *---------------------------------------------------------------------------*/

void pr_geometry()
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

void pr_pointsize()
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

char width_keys[Nwidths][30] = {
    "border", "axis",
    "line1", "line2", "line3", "line4", "line5", "line6", "line7", "line8"
};

void pr_width()
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

Window pr_window(flags, x, y, width, height)
unsigned int flags;
int x, y;
unsigned int width, height;
{
    char *title = pr_GetR(db, ".title");
    static XSizeHints hints;
    int Tvtwm = 0;

    Window win = XCreateSimpleWindow(dpy, root, x, y, width, height, BorderWidth,
				     colors[1], colors[0]);

    /* ask ICCCM-compliant window manager to tell us when close window
     * has been chosen, rather than just killing us
     */

    XChangeProperty(dpy, win, WM_PROTOCOLS, XA_ATOM, 32, PropModeReplace,
		    (unsigned char *) &WM_DELETE_WINDOW, 1);

    pr_GetR(db, ".clear") && On(value.addr) && Clear++;
    pr_GetR(db, ".tvtwm") && On(value.addr) && Tvtwm++;

    if (!Tvtwm) {
	hints.flags = flags;
    } else {
	hints.flags = (flags & ~USPosition) | PPosition;	/* ? */
    }
    hints.x = gX;
    hints.y = gY;
    hints.width = width;
    hints.height = height;

    XSetNormalHints(dpy, win, &hints);

    if (pr_GetR(db, ".iconic") && On(value.addr)) {
	XWMHints wmh;

	wmh.flags = StateHint;
	wmh.initial_state = IconicState;
	XSetWMHints(dpy, win, &wmh);
    }
    XStoreName(dpy, win, ((title) ? title : Class));

    XSelectInput(dpy, win, KeyPressMask | StructureNotifyMask);
    XMapWindow(dpy, win);

    return win;
}


/***** pr_raise ***/
void pr_raise()
{
    if (pr_GetR(db, ".raise"))
	do_raise = (On(value.addr));
}


void pr_persist()
{
    if (pr_GetR(db, ".persist"))
	persist = (On(value.addr));
}

/************ code to handle selection export *********************/

#ifdef EXPORT_SELECTION

/* bit of a bodge, but ... */
static struct plot_struct *exported_plot;

void export_graph(plot)
struct plot_struct *plot;
{
    FPRINTF((stderr, "export_graph(0x%x)\n", plot));

    XSetSelectionOwner(dpy, EXPORT_SELECTION, plot->window, CurrentTime);
    /* to check we have selection, we would have to do a
     * GetSelectionOwner(), but if it failed, it failed - no big deal
     */
    exported_plot = plot;
}

void handle_selection_event(event)
XEvent *event;
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
		static Atom targets[] =	{XA_PIXMAP, XA_COLORMAP};

		FPRINTF((stderr, "Targets request from %d\n", reply.xselection.requestor));

		XChangeProperty(dpy, reply.xselection.requestor,
				reply.xselection.property, reply.xselection.target, 32, PropModeReplace,
				(unsigned char *) targets, 2);
	    } else if (reply.xselection.target == XA_COLORMAP) {
		Colormap cmap = DefaultColormap(dpy, 0);

		FPRINTF((stderr, "colormap request from %d\n", reply.xselection.requestor));

		XChangeProperty(dpy, reply.xselection.requestor,
				reply.xselection.property, reply.xselection.target, 32, PropModeReplace,
				(unsigned char *) &cmap, 1);
	    } else if (reply.xselection.target == XA_PIXMAP) {

		FPRINTF((stderr, "pixmap request from %d\n", reply.xselection.requestor));

		XChangeProperty(dpy, reply.xselection.requestor,
				reply.xselection.property, reply.xselection.target, 32, PropModeReplace,
			  (unsigned char *) &(exported_plot->pixmap), 1);
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
