/*
 * $Id: matrix.h,v 1.4.2.1 2000/05/03 21:26:11 joze Exp $
 */

/* GNUPLOT - matrix.h */

/*  NOTICE: Change of Copyright Status
 *
 *  The author of this module, Carsten Grammes, has expressed in
 *  personal email that he has no more interest in this code, and
 *  doesn't claim any copyright. He has agreed to put this module
 *  into the public domain.
 *
 *  Lars Hecking  15-02-1999
 */

/*
 *	Header file: public functions in matrix.c
 *
 *
 *	Copyright of this module:   Carsten Grammes, 1993
 *      Experimental Physics, University of Saarbruecken, Germany
 *
 *	Internet address: cagr@rz.uni-sb.de
 *
 *	Permission to use, copy, and distribute this software and its
 *	documentation for any purpose with or without fee is hereby granted,
 *	provided that the above copyright notice appear in all copies and
 *	that both that copyright notice and this permission notice appear
 *	in supporting documentation.
 *
 *      This software is provided "as is" without express or implied warranty.
 */


#ifndef MATRIX_H
#define MATRIX_H

#include "plot.h"

#ifdef EXT
#undef EXT
#endif

#ifdef MATRIX_MAIN
#define EXT
#else
#define EXT extern
#endif


/******* public functions ******/

EXT double  *vec __PROTO((int n));
EXT int     *ivec __PROTO((int n));
EXT double  **matr __PROTO((int r, int c));
EXT void    free_matr __PROTO((double **m));
EXT double  *redim_vec __PROTO((double **v, int n));
EXT void    solve __PROTO((double **a, int n, double **b, int m));
EXT void    Givens __PROTO((double **C, double *d, double *x,
			double *r, int N, int n, int want_r)); 
EXT void    Invert_RtR __PROTO((double **R, double **I, int n));

#endif /* MATRIX_H */
