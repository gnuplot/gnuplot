/*
 * $Id: protos.h,v 1.21 1999/10/01 14:54:35 lhecking Exp $
 *
 */

/* GNUPLOT - protos.h */

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


#ifndef GNUPLOT_PROTOS_H
# define GNUPLOT_PROTOS_H

#include "ansichek.h"

/* HBB 990828 FIXME: that comment below is somewhat illogical. Don't
 * require that header being included before this one, include it
 * *from* this one, and be done with it */
/* note that this before this file, some headers that define stuff like FILE,
   time_t and GPFAR must be already be included */

/* HBB 990828: I've moved all the prototypes to per-module headers, as
 * proper modularization should be done. In the future, this whole
 * 'protos.h' stuff should go away, being replaced by direct #include
 * of exactly those modules actually *used*, in every .h and .c
 * file. Following this route, separation of gnuplot into distinct
 * layers, as per David's original long term plan, should become quite
 * a bit easier.
 * */

#if 0 
/* HBB 991021: I'm starting to tear the 'one ring to bind them all' to
 * pieces. As a start, the 'include everything, everywhere' which happens by
 * every module including protos.h, is taken out */
#include "alloc.h"
#include "command.h"
#include "contour.h"
#include "datafile.h"
#include "eval.h"
#include "fit.h"
/* HBB 990828: note the name, to avoid collision with system headers */
#include "gp_time.h"
#include "graph3d.h"
#include "hidden3d.h"
#include "internal.h"
#include "interpol.h"
#include "misc.h"
#include "plot.h"
#include "plot2d.h"
#include "plot3d.h"
/* HBB 990828: NB: this *could* conflict with readline.h found elsewhere? */
#include "readline.h"
#include "save.h"
#include "scanner.h"
#include "specfun.h"
#include "standard.h"
/* HBB 990828: note the name, keeping our beloved 'term.h' as is :-) */
#include "term_api.h"

#endif /* 0/1 */

#endif /* GNUPLOT_PROTOS_H */
