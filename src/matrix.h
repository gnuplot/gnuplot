/*
 * $Id: matrix.h,v 1.7 2000/11/01 18:57:33 broeker Exp $
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

#include "syscfg.h"

/******* public functions ******/

double  *vec __PROTO((int n));
int     *ivec __PROTO((int n));
double  **matr __PROTO((int r, int c));
void    free_matr __PROTO((double **m));
double  *redim_vec __PROTO((double **v, int n));
void    solve __PROTO((double **a, int n, double **b, int m));
void    Givens __PROTO((double **C, double *d, double *x,
    		double *r, int N, int n, int want_r)); 
void    Invert_RtR __PROTO((double **R, double **I, int n));

/* Functions for use by THIN_PLATE_SPLINES_GRID method */
void	lu_decomp __PROTO((double **, int, int *, double *));
void	lu_backsubst __PROTO((double **, int n, int *, double *));


#endif /* MATRIX_H */
