#ifndef lint
static char *RCSid = "$Id: matrix.c,v 1.14 1997/04/10 02:33:05 drd Exp $";
#endif

/*
 *	Matrix algebra, part of
 *
 *	Nonlinear least squares fit according to the
 *	Marquardt-Levenberg-algorithm
 *
 *	added as Patch to Gnuplot (v3.2 and higher)
 *	by Carsten Grammes
 *	Experimental Physics, University of Saarbruecken, Germany
 *
 *	Internet address: cagr@rz.uni-sb.de
 *
 *	Copyright of this module:   Carsten Grammes, 1993
 *
 *	Permission to use, copy, and distribute this software and its
 *	documentation for any purpose with or without fee is hereby granted,
 *	provided that the above copyright notice appear in all copies and
 *	that both that copyright notice and this permission notice appear
 *	in supporting documentation.
 *
 *	This software is provided "as is" without express or implied warranty.
 */


#define MATRIX_MAIN

#include <math.h>

#include "type.h"
#include "fit.h"
#include "matrix.h"
#include "stdfn.h"
#include "alloc.h"

/*****************************************************************/

#define Swap(a,b)   {double temp=(a); (a)=(b); (b)=temp;}
#define WINZIG	      1e-30


/*****************************************************************
    internal prototypes
*****************************************************************/
static void lu_decomp __PROTO((double **a, int n, int *indx, double *d));
static void lu_backsubst __PROTO((double **a, int n, int *indx, double b[]));


/*****************************************************************
    first straightforward vector and matrix allocation functions
*****************************************************************/
double *vec (n)
int n;
{
    /* allocates a double vector with n elements */
    double *dp;
    if( n < 1 )
	return (double *) NULL;
    dp = (double *) gp_alloc ( n * sizeof(double), "vec");
    return dp;
}


int *ivec (n)
int n;
{
    /* allocates a int vector with n elements */
    int *ip;
    if( n < 1 )
	return (int *) NULL;
    ip = (int *) gp_alloc ( n * sizeof(int), "ivec");
    return ip;
}

double **matr (rows, cols)
int rows;
int cols;
{
    /* allocates a double matrix */

    register int i;
    register double **m;

    if ( rows < 1  ||  cols < 1 )
        return NULL;
    /* allocate pointers to rows */
    m = (double **) gp_alloc ( rows * sizeof(double *), "matrix pointers");
    /* allocate rows and set pointers to them */
    for ( i=0 ; i<rows ; i++ )
	m[i] = (double *) gp_alloc (cols * sizeof(double), "matrix row");
    return m;
}


void free_matr (m, rows)
double **m;
int rows;
{
    register int i;
    for ( i=0 ; i<rows ; i++ )
	free ( m[i] );
    free (m);
}


void redim_vec (v, n)
double **v;
int n;
{
    if ( n < 1 ) {
	*v = NULL;
	return;
    }
    *v = (double *) gp_realloc (*v, n * sizeof(double), "vec");
}

void redim_ivec (v, n)
int **v;
int n;
{
    if ( n < 1 ) {
	*v = NULL;
	return;
    }
    *v = (int *) gp_realloc (*v, n * sizeof(int), "ivec");
}



/*****************************************************************
    Linear equation solution by Gauss-Jordan elimination
*****************************************************************/
void solve (a, n, b, m)
double **a;
int n;
double **b;
int m;
{
    int     *c_ix, *r_ix, *pivot_ix, *ipj, *ipk,
	    i, ic=-1, ir=-1, j, k, l, s; /* HBB: added initial vals to shut up gcc -Wall */

    double  large, dummy, tmp, recpiv,
	    **ar, **rp,
	    *ac, *cp, *aic, *bic;

    c_ix	= ivec (n);
    r_ix	= ivec (n);
    pivot_ix	= ivec (n);
    memset (pivot_ix, 0, n*sizeof(int));

    for ( i=0 ; i<n ; i++ ) {
	large = 0.0;
	ipj = pivot_ix;
	ar = a;
	for ( j=0  ;  j<n  ;  j++, ipj++, ar++ )
	    if (*ipj != 1) {
		ipk = pivot_ix;
		ac = *ar;
		for ( k=0  ;  k<n  ;  k++, ipk++, ac++ )
		    if ( *ipk ) {
			if ( *ipk > 1 )
			    Eex ("Singular matrix")
		    }
		    else {
			if ( (tmp = fabs(*ac)) >= large ) {
			    large = tmp;
			    ir = j;
			    ic = k;
			}
		    }
	    }
	++(pivot_ix[ic]);

	if ( ic != ir ) {
	    ac = b[ir];
	    cp = b[ic];
	    for ( l=0  ;  l<m  ;  l++, ac++, cp++ )
		Swap(*ac, *cp)
	    ac = a[ir];
            cp = a[ic];
            for ( l=0  ;  l<n  ;  l++, ac++, cp++ )
                Swap(*ac, *cp)
        }

	c_ix[i] = ic;
        r_ix[i] = ir;
	if ( *(cp = &(a[ic][ic])) == 0.0 )
	    Eex ("Singular matrix")
	recpiv = 1/(*cp);
	*cp = 1;

	cp = b[ic];
        for ( l=0 ; l<m ; l++ )
            *cp++ *= recpiv;
        cp = a[ic];
	for ( l=0 ; l<n ; l++ )
	    *cp++ *= recpiv;

	ar = a;
	rp = b;
	for ( s=0 ; s<n ; s++, ar++, rp++ )
	    if ( s != ic ) {
		dummy = (*ar)[ic];
		(*ar)[ic] = 0;
		ac = *ar;
		aic = a[ic];
		for ( l=0 ; l<n ; l++ )
		    *ac++ -= *aic++ * dummy;
		cp = *rp;
		bic = b[ic];
		for ( l=0 ; l<m ; l++ )
		    *cp++ -= *bic++ * dummy;
	    }
    }

    for ( l=n-1 ; l>=0 ; l-- )
	if ( r_ix[l] != c_ix[l] )
	    for ( ar=a, k=0  ;	k<n  ;	k++, ar++ )
		Swap ((*ar)[r_ix[l]], (*ar)[c_ix[l]])

    free (pivot_ix);
    free (r_ix);
    free (c_ix);
}


/*****************************************************************
    LU-Decomposition of a quadratic matrix
*****************************************************************/
static void lu_decomp (a, n, indx, d)
double **a;
int n;
int *indx;
double *d;
{
    int     i, imax=-1, j, k; /* HBB: added initial value, to shut up gcc -Wall */

    double  large, dummy, temp,
	    **ar, **lim,
	    *limc, *ac, *dp, *vscal;

    dp = vscal = vec (n);
    *d = 1.0;
    for ( ar=a, lim = &(a[n]) ; ar<lim ; ar++ ) {
	large = 0.0;
	for ( ac = *ar, limc = &(ac[n]) ; ac<limc ; )
	    if ( (temp = fabs (*ac++)) > large )
		large = temp;
	if ( large == 0.0 )
	    Eex ("Singular matrix in LU-DECOMP")
	*dp++ = 1/large;
    }
    ar = a;
    for ( j=0 ; j<n ; j++, ar++ ) {
	for ( i=0 ; i<j ; i++ ) {
	    ac = &(a[i][j]);
	    for ( k=0 ; k<i ; k++ )
		*ac -= a[i][k] * a[k][j];
	}
	large = 0.0;
	dp = &(vscal[j]);
	for ( i=j ; i<n ; i++ ) {
	    ac = &(a[i][j]);
            for ( k=0 ; k<j ; k++ )
		*ac -= a[i][k] * a[k][j];
	    if ( (dummy = *dp++ * fabs(*ac)) >= large ) {
		large = dummy;
		imax = i;
	    }
	}
	if ( j != imax ) {
	    ac = a[imax];
	    dp = *ar;
	    for ( k=0 ; k<n ; k++, ac++, dp++ )
		Swap (*ac, *dp);
	    *d = -(*d);
	    vscal[imax] = vscal[j];
	}
	indx[j] = imax;
	if ( *(dp = &(*ar)[j]) == 0 )
	    *dp = WINZIG;

	if ( j != n-1 ) {
	    dummy = 1/(*ar)[j];
	    for ( i=j+1 ; i<n ; i++ )
		a[i][j] *= dummy;
	}
    }
    free (vscal);
}


/*****************************************************************
    Routine for backsubstitution
*****************************************************************/
static void lu_backsubst (a, n, indx, b)
double **a;
int n;
int *indx;
double b[];
{
    int     i, memi = -1, ip, j;

    double  sum, *bp, *bip, **ar, *ac;

    ar = a;
    for ( i=0 ; i<n ; i++, ar++ ) {
	ip = indx[i];
	sum = b[ip];
	b[ip] = b[i];
	if (memi >= 0) {
	    ac = &((*ar)[memi]);
	    bp = &(b[memi]);
	    for ( j=memi ; j<=i-1 ; j++ )
		sum -= *ac++ * *bp++;
	}
        else
	    if ( sum )
		memi = i;
	b[i] = sum;
    }
    ar--;
    for ( i=n-1 ; i>=0 ; i-- ) {
	ac = &(*ar)[i+1];
	bp = &(b[i+1]);
	bip = &(b[i]);
	for ( j=i+1 ; j<n ; j++ )
	    *bip -= *ac++ * *bp++;
	*bip /= (*ar--)[i];
    }
}


/*****************************************************************
    matrix inversion
*****************************************************************/
void inverse (src, dst, n)
double **src;
double **dst;
int n;
{
    int i,j;
    int *indx;
    double d, *col, **tmp;

    indx = ivec (n);
    col = vec (n);
    tmp = matr (n, n);
    for ( i=0 ; i<n ; i++ )
	memcpy (tmp[i], src[i], n*sizeof(double));

    lu_decomp (tmp, n, indx, &d);

    for ( j=0 ; j<n ; j++ ) {
	for ( i=0 ; i<n ; i++ )
	    col[i] = 0;
	col[j] = 1;
	lu_backsubst (tmp, n, indx, col);
	for ( i=0 ; i<n ; i++ )
	    dst[i][j] = col[i];
    }
    free (indx);
    free (col);
    free_matr (tmp, n);
}


