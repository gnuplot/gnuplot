/* -*- C -*-
 * FILE: "/home/joze/pub/gnuplot/src/ipc.h"
 * LAST MODIFICATION: "Wed, 22 Mar 2000 21:31:36 +0100 (joze)"
 * 1999 by Johannes Zellner, <johannes@zellner.org>
 * $Id: ipc.h,v 1.1 2000/02/11 18:41:55 lhecking Exp $
 */


#ifndef _IPC_H
#define _IPC_H

char* readline_ipc __PROTO((const char*));
/*
 * special readline_ipc routine for IPC communication, usual readline 
 * otherwise (OS/2)
 */

#ifndef OS2
/*
 * gnuplot's terminals communicate with gnuplot by shared memory + event
 * semaphores, thus the code below is not used (see also gpexecute.inc)
 */


enum { IPC_BACK_UNUSABLE = -2, IPC_BACK_CLOSED = -1 };

/*
 * currently only used for X11 && USE_MOUSE, but in principle
 * every term could use this file descriptor to write back
 * commands to gnuplot.  Note, that terminals using this fd
 * should set it to a negative value when closing. (joze)
 */
#ifdef _TERM_C
    int ipc_back_fd = IPC_BACK_CLOSED;
    int isatty_state = 1;
#else
    extern int ipc_back_fd;
    extern int isatty_state;
#endif

#endif /* OS/2 */

#endif /* _IPC_H */
