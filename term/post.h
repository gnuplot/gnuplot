/*
 * $Id: post.h,v 1.2 1999/10/01 15:09:12 lhecking Exp $
 */

/* GNUPLOT - post.h */

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

#ifndef TERM_POST_H
# define TERM_POST_H

/* Needed by terminals which output postscript
 * (post.trm and pslatex.trm)
 */

/* HBB 990829: made these 'static' again. This is correct as long as
 * the terminal stuff is still compiled as a single large module, i.e.
 * the terminals aren't compiled one by one. */
static TBOOLEAN ps_color;
static TBOOLEAN ps_solid;

#define PS_POINT_TYPES 8

/* page offset in pts */
#define PS_XOFF 50
#define PS_YOFF 50

/* assumes landscape */
#define PS_XMAX 7200
#define PS_YMAX 5040

#define PS_XLAST (PS_XMAX - 1)
#define PS_YLAST (PS_YMAX - 1)

#define PS_VTIC (PS_YMAX/80)
#define PS_HTIC (PS_YMAX/80)

/* scale is 1pt = 10 units */
#define PS_SC (10)

/* linewidth = 0.5 pts */
#define PS_LW (0.5*PS_SC)

/* default is 14 point characters */
#define PS_VCHAR (14*PS_SC)
#define PS_HCHAR (14*PS_SC*6/10)

#endif /* TERM_POST_H */
