/*
 * $Id: wxt_plot.h,v 1.6 2006/09/28 13:54:55 tlecomte Exp $
 */

/* GNUPLOT - wxt_plot.h */

/*[
 * Copyright 2005,2006   Timothee Lecomte
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
 *
 *
 * Alternatively, the contents of this file may be used under the terms of the
 * GNU General Public License Version 2 or later (the "GPL"), in which case the
 * provisions of GPL are applicable instead of those above. If you wish to allow
 * use of your version of this file only under the terms of the GPL and not
 * to allow others to use your version of this file under the above gnuplot
 * license, indicate your decision by deleting the provisions above and replace
 * them with the notice and other provisions required by the GPL. If you do not
 * delete the provisions above, a recipient may use your version of this file
 * under either the GPL or the gnuplot license.
]*/

/* ------------------------------------------------------
 * Here are the declarations of the C++ functions
 * used in plot.c and defined in wxt.cpp,
 * where the wxWidgets terminal is mainly implemented.
 * ------------------------------------------------------*/

#ifndef GNUPLOT_WXT_PLOT_H
# define GNUPLOT_WXT_PLOT_H

/* for TBOOLEAN */
#include "syscfg.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/* cleanup at exit, and handle 'persist' setting */
void wxt_atexit(TBOOLEAN persist);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* GNUPLOT_WXT_PLOT_H */
