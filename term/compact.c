/*
 * $Id: $
 *
 */

/* GNUPLOT - compact.c */

/* contains routines to compress a vector stream without modifying it */

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

#ifndef COMPACT

/* replaces runs of constant slope in the buffer with single vectors 
   returns the number of points eliminated */
int compact_slope(xp, yp, isa_move, sz, delta)
int xp[], yp[], isa_move[];
int *sz;
double delta;
{
    int dx, dy, old_size, new_index, i, start;
    float slope, old_slope;

    old_size = *sz;
    new_index = 0;
    start = 0;
    if (xp[1] != xp[0])
	old_slope = (float) (yp[1] - yp[0]) / (float) (xp[1] - xp[0]);
    else
	old_slope = (float) (yp[1] - yp[0]) / (float) (0.00001 + xp[1] - xp[0]);
    for (i = 2; i < old_size; i++) {
	dx = xp[i] - xp[i - 1];
	dy = yp[i] - yp[i - 1];
	if (dx != 0)
	    slope = (float) dy / (float) dx;
	else
	    slope = (float) dy / ((float) dx + 0.00001);
	if ((ABS(slope - old_slope) > delta) || (isa_move[i])) {
	    xp[new_index] = xp[start];
	    yp[new_index] = yp[start];
	    isa_move[new_index] = isa_move[start];
	    new_index++;
	    if (start != i - 1) {
		xp[new_index] = xp[i - 1];
		yp[new_index] = yp[i - 1];
		isa_move[new_index] = isa_move[i - 1];
		new_index++;
	    }
	    start = i;
	    /* this is the slope for the new run */
	    old_slope = slope;
	}
    }
    /* copy the last point into the new array */
    xp[new_index] = xp[old_size - 1];
    yp[new_index] = yp[old_size - 1];
    isa_move[new_index] = isa_move[old_size - 1];
    new_index++;
    *sz = new_index;
    return (old_size - *sz);
}

/* compacts the vector list by compressing runs of constant 
   dx&dy into one vector
   use this if floating point is too expensive!
   more naive than compact_slope; doesn't compact as much as possible
   returns the number of points eliminated */
int compact_int(xp, yp, isa_move, size)
int xp[], yp[], isa_move[], *size;
{
    int dx, dy, old_dx, old_dy, start, index, i, old_size;

    start = index = 0;
    old_dx = xp[1] - xp[0];
    old_dy = yp[1] - yp[0];
    for (i = 2; i < *size; i++) {
	dx = xp[i] - xp[i - 1];
	dy = yp[i] - yp[i - 1];
	if ((ABS(dx - old_dx) + ABS(dy - old_dy) != 0) || (isa_move[i])) {
	    /*  we've reached the end of a run */
	    xp[index] = xp[start];
	    yp[index] = yp[start];
	    isa_move[index] = isa_move[start];
	    index++;
	    if (start != i - 1) {
		xp[index] = xp[i - 1];
		yp[index] = yp[i - 1];
		isa_move[index] = isa_move[i - 1];
		index++;
	    }
	    start = i;
	    old_dx = dx;
	    old_dy = dy;
	}
    }				/* end for */
    /* include the last point */
    xp[index] = xp[*size - 1];
    yp[index] = yp[*size - 1];
    isa_move[index] = isa_move[*size - 1];
    index++;
    old_size = *size;
    *size = index;
    return (old_size - *size);
}
#endif

#define COMPACT
