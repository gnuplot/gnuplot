/*
 * $Id: getcolor.h,v 1.2 2001/08/22 14:15:34 broeker Exp $
 */

/* GNUPLOT - getcolor.h */

/* Routines + constants for mapping interval [0,1] into another [0,1] to be 
 * used to get RGB triplets from gray (palette of smooth colours).
 *
 * Note: The code in getcolor.h,.c cannot be inside color.h,.c since gplt_x11.c
 * compiles with getcolor.o, so it cannot load the other huge staff.
 *
 */

/*[
 *
 * Petr Mikulik, since December 1998
 * Copyright: open source as much as possible
 *
]*/


#ifndef GETCOLOR_H
#define GETCOLOR_H

#ifdef PM3D

#include "syscfg.h"

double GetColorValueFromFormula __PROTO((int formula, double x));

extern const char *ps_math_color_formulae[];


#endif /* PM3D */

#endif /* GETCOLOR_H */

/* eof getcolor.h */
