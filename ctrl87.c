/*
 * ctrl87.c
 *
 */


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
