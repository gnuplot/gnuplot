/*
 * $Id: mousecmn.h,v 1.1.2.1 2000/05/03 21:26:11 joze Exp $
 */

/* GNUPLOT - mousecnm.h */

/*[
 * Copyright 1999
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted
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

#ifndef MOUSECMN_H
# define MOUSECMN_H

/*
 * Definitions that are used by both gnuplot core and standalone terminals.
 */


/*
 * Structure for reporting mouse events to the main program
 */
struct gp_event_t {
    int type;		/* see below */
    int mx, my;		/* current mouse coordinates */
    int par1, par2;	/* other parameters, depending on the event type */
    char text[100];	/* literal command string for type=GE_cmd */
};


/* event types:
*/
enum {
    GE_motion,          /* mouse has moved */
    GE_buttonpress,     /* mouse button has been pressed; par1 = number of the button (1, 2, 3...) */
    GE_buttonrelease,   /* mouse button has been released; par1 = number of the button (1, 2, 3...); par2 = time (ms) since previous button release */
    GE_keypress,        /* keypress; par1 = keycode (either ASCII, or one of the GP_ enums defined below); par2 = ( |1 .. don't pass through bindings )*/
    GE_modifier,        /* shift/ctrl/alt key pressed or released; par1 = is new mask, see Mod_ enums below */
    GE_plotdone,        /* acknowledgement of plot completion (for synchronization) */
    GE_stdout,          /* print text to stdout */
    GE_stderr,          /* print text to stderr */
    GE_cmd,             /* text = literal command string */
    GE_reset            /* reset to a well-defined state
			   (e.g.  after an X11 error occured) */
#if defined(USE_NONBLOCKING_STDOUT) && !defined(OS2)
    , GE_pending        /* signal gp_exec_event() to send pending events */
#endif
};


/* the status of the shift, ctrl and alt keys
*/
enum { Mod_Shift = (1), Mod_Ctrl = (1 << 1), Mod_Alt = (1 << 2) };


/* this depends on the ascii character set lying in the
 * range from 0 to 255 (below 1000). */
enum {
    GP_FIRST_KEY = 1000,
    GP_BackSpace,
    GP_Tab,
    GP_Linefeed,
    GP_Clear,
    GP_Return,
    GP_Pause,
    GP_Scroll_Lock,
    GP_Sys_Req,
    GP_Escape,
    GP_Insert,
    GP_Delete,
    GP_Home,
    GP_Left,
    GP_Up,
    GP_Right,
    GP_Down,
    GP_PageUp,
    GP_PageDown,
    GP_End,
    GP_Begin,
    GP_KP_Space,
    GP_KP_Tab,
    GP_KP_Enter,
    GP_KP_F1,
    GP_KP_F2,
    GP_KP_F3,
    GP_KP_F4,

    GP_KP_Insert,    /* ~ KP_0 */
    GP_KP_End,       /* ~ KP_1 */
    GP_KP_Down,      /* ~ KP_2 */
    GP_KP_Page_Down, /* ~ KP_3 */
    GP_KP_Left,      /* ~ KP_4 */
    GP_KP_Begin,     /* ~ KP_5 */
    GP_KP_Right,     /* ~ KP_6 */
    GP_KP_Home,      /* ~ KP_7 */
    GP_KP_Up,        /* ~ KP_8 */
    GP_KP_Page_Up,   /* ~ KP_9 */

    GP_KP_Delete,
    GP_KP_Equal,
    GP_KP_Multiply,
    GP_KP_Add,
    GP_KP_Separator,
    GP_KP_Subtract,
    GP_KP_Decimal,
    GP_KP_Divide,
    GP_KP_0,
    GP_KP_1,
    GP_KP_2,
    GP_KP_3,
    GP_KP_4,
    GP_KP_5,
    GP_KP_6,
    GP_KP_7,
    GP_KP_8,
    GP_KP_9,
    GP_F1,
    GP_F2,
    GP_F3,
    GP_F4,
    GP_F5,
    GP_F6,
    GP_F7,
    GP_F8,
    GP_F9,
    GP_F10,
    GP_F11,
    GP_F12,
    GP_LAST_KEY
};


#ifdef OS2
/* Pass information necessary for (un)checking menu items in the
   Presentation Manager terminal.
   Thus this structure is required by pm.trm and gclient.c.
*/
struct t_gpPMmenu {
    int use_mouse;
    int where_zoom_queue;
	/* logical or: 1..unzoom, 2..unzoom back, 4..zoom next */
    int polar_distance;
};

#endif


#endif /* MOUSECMN_H */
