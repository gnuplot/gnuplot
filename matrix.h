/* $Id: matrix.h,v 1.3 1996/12/08 13:08:33 drd Exp $ */

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

#include "ansichek.h"

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
EXT void    free_matr __PROTO((double **m, int r));
EXT double  *redim_vec __PROTO((double **v, int n));
EXT void    redim_ivec __PROTO((int **v, int n));
EXT void    solve __PROTO((double **a, int n, double **b, int m));
EXT void    inverse __PROTO((double **src, double **dst, int n));

#endif
