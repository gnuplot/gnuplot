/*
 * $Id: binary.h,v 1.4.2.1 2000/05/03 21:26:10 joze Exp $
 */

/* GNUPLOT - binary.h */

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

#ifndef GNUPLOT_BINARY_H
# define GNUPLOT_BINARY_H

#include "plot.h"

/* Routines for interfacing with command.c */
float GPFAR *vector __PROTO(( int nl, int nh));
float GPFAR *extend_vector __PROTO((float GPFAR *vec, int old_nl, int old_nh, int new_nh));
float GPFAR *retract_vector __PROTO((float GPFAR *v, int old_nl, int old_nh, int new_nh));
float GPFAR * GPFAR *matrix __PROTO(( int nrl, int nrh, int ncl, int nch));
float GPFAR * GPFAR *extend_matrix __PROTO(( float GPFAR * GPFAR *a, int nrl, int nrh, int ncl, int nch, int srh, int sch));
float GPFAR * GPFAR *retract_matrix __PROTO(( float GPFAR * GPFAR *a, int nrl, int nrh, int ncl, int nch, int srh, int sch));
void free_matrix __PROTO((float GPFAR * GPFAR *m, unsigned nrl, unsigned nrh, unsigned ncl, unsigned nch));
void free_vector __PROTO((float GPFAR *vec, int nl, int nh));
int is_binary_file __PROTO((FILE *fp));
int fread_matrix __PROTO((FILE *fin, float GPFAR * GPFAR * GPFAR *ret_matrix, int *nr, int *nc, float GPFAR * GPFAR *row_title, float GPFAR * GPFAR *column_title));
int fwrite_matrix __PROTO((FILE *fout, float GPFAR * GPFAR *m, int nrl, int nrh, int ncl, int nch, float GPFAR *row_title, float GPFAR *column_title));
float GPFAR * GPFAR *convert_matrix __PROTO((float GPFAR *a, int nrl, int nrh, int ncl, int nch));
void free_convert_matrix __PROTO((float GPFAR* GPFAR *b, int nrl, int nrh, int ncl, int nch));

#endif /* GNUPLOT_BINARY_H */
