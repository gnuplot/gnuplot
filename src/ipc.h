/*
 * $Id: ipc.h,v 1.2.2.1 2000/05/03 21:26:11 joze Exp $
 */

/* GNUPLOT - ipc.h */

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

/*
 * LAST MODIFICATION: "Wed, 22 Mar 2000 21:31:36 +0100 (joze)"
 * 1999 by Johannes Zellner, <johannes@zellner.org>
 */


#ifndef _IPC_H
# define _IPC_H

char* readline_ipc __PROTO((const char*));
/*
 * special readline_ipc routine for IPC communication, usual readline 
 * otherwise (OS/2)
 */

# ifndef OS2
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
#  ifdef _TERM_C
    int ipc_back_fd = IPC_BACK_CLOSED;
    int isatty_state = 1;
#  else
    extern int ipc_back_fd;
    extern int isatty_state;
#  endif

# endif /* OS/2 */

#endif /* _IPC_H */
