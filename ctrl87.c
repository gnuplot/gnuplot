/* $Id: ctrl87.c,v 1.2 1998/03/22 22:31:27 drd Exp $ */


/* GNUPLOT - ctrl87.c */

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


#define __CONTROL87_C__


#include "ctrl87.h"


/***** _control87 *****/
unsigned short _control87(unsigned short newcw, unsigned short mask)
{
  unsigned short cw;

  asm volatile("                                                    \n\
      wait                                                          \n\
      fstcw  %0                                                       "
      : /* outputs */   "=g" (cw)
      : /* inputs */
  );

  if(mask) { /* mask is not 0 */
    asm volatile("                                                  \n\
        mov    %2, %%ax                                             \n\
        mov    %3, %%bx                                             \n\
        and    %%bx, %%ax                                           \n\
        not    %%bx                                                 \n\
        nop                                                         \n\
        wait                                                        \n\
        mov    %1, %%dx                                             \n\
        and    %%bx, %%dx                                           \n\
        or     %%ax, %%dx                                           \n\
        mov    %%dx, %0                                             \n\
        wait                                                        \n\
        fldcw  %1                                                     "
        : /* outputs */   "=g" (cw)
        : /* inputs */    "g" (cw), "g" (newcw), "g" (mask)
        : /* registers */ "ax", "bx", "dx"
    );
  }

  return cw;
} /* _control87 */
