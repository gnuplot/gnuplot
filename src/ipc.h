/* -*- C -*-
 * FILE: "/home/joze/pub/gnuplot-3.8a-Nov-27-99/src/ipc.h"
 * LAST MODIFICATION: "Sun Nov 28 02:38:31 1999 (joze)"
 * 1999 by Johannes Zellner, <johannes@zellner.org>
 * $Id: ipc.h,v 1.2 2000/01/20 13:49:25 mikulik Exp $
 */

#ifndef OS2
/*
 * gnuplot's terminals communicate with gnuplot by shared memory + event
 * semaphores, thus this code is not used (see also gpexecute.inc)
 */


#ifndef _IPC_H
#define _IPC_H

/*
 * currently only used for X11 && USE_MOUSE, but in principle
 * every term could use this file descriptor to write back
 * commands to gnuplot.  Note, that terminals using this fd
 * should set it to a negative value when closing. (joze)
 */
#ifdef _TERM_C
    int ipc_back_fd = -1;
#else
    extern int ipc_back_fd;
#endif

char* readline_ipc __PROTO((const char*));

#endif /* _IPC_H */

#endif /* OS/2 */
