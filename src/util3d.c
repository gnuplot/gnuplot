#ifndef lint
static char *RCSid() { return RCSid("$Id: util3d.c,v 1.8 2000/02/11 19:13:12 lhecking Exp $"); }
#endif

/* GNUPLOT - util3d.c */

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


/*
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * 3.6 - split graph3d.c into graph3d.c (graph),
 *                            util3d.c (intersections, etc)
 *                            hidden3d.c (hidden-line removal code)
 *
 */

#include "util3d.h"

#include "axis.h"
#include "hidden3d.h"
#include "setshow.h"
#include "util3d.h"

/* HBB 990826: all that stuff referenced from other modules is now
 * exported in graph3d.h, instead of being listed here */

/* Prototypes for local functions */
static void mat_unit __PROTO((transform_matrix mat));

static void
mat_unit(mat)
transform_matrix mat;
{
    int i, j;

    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
	    if (i == j)
		mat[i][j] = 1.0;
	    else
		mat[i][j] = 0.0;
}

#if 0 /* HBB 990829: unused --> commented out */
void
mat_trans(tx, ty, tz, mat)
double tx, ty, tz;
transform_matrix mat;
{
    mat_unit(mat);		/* Make it unit matrix. */
    mat[3][0] = tx;
    mat[3][1] = ty;
    mat[3][2] = tz;
}
#endif /* commented out */

void
mat_scale(sx, sy, sz, mat)
double sx, sy, sz;
transform_matrix mat;
{
    mat_unit(mat);		/* Make it unit matrix. */
    mat[0][0] = sx;
    mat[1][1] = sy;
    mat[2][2] = sz;
}

void
mat_rot_x(teta, mat)
double teta;
transform_matrix mat;
{
    double cos_teta, sin_teta;

    teta *= DEG2RAD;
    cos_teta = cos(teta);
    sin_teta = sin(teta);

    mat_unit(mat);		/* Make it unit matrix. */
    mat[1][1] = cos_teta;
    mat[1][2] = -sin_teta;
    mat[2][1] = sin_teta;
    mat[2][2] = cos_teta;
}

#if 0 /* HBB 990829: unused --> commented out */
void
mat_rot_y(teta, mat)
double teta;
transform_matrix mat;
{
    double cos_teta, sin_teta;

    teta *= DEG2RAD;
    cos_teta = cos(teta);
    sin_teta = sin(teta);

    mat_unit(mat);		/* Make it unit matrix. */
    mat[0][0] = cos_teta;
    mat[0][2] = -sin_teta;
    mat[2][0] = sin_teta;
    mat[2][2] = cos_teta;
}
#endif /* commented out */

void
mat_rot_z(teta, mat)
double teta;
transform_matrix mat;
{
    double cos_teta, sin_teta;

    teta *= DEG2RAD;
    cos_teta = cos(teta);
    sin_teta = sin(teta);

    mat_unit(mat);		/* Make it unit matrix. */
    mat[0][0] = cos_teta;
    mat[0][1] = -sin_teta;
    mat[1][0] = sin_teta;
    mat[1][1] = cos_teta;
}

/* Multiply two transform_matrix. Result can be one of two operands. */
void
mat_mult(mat_res, mat1, mat2)
transform_matrix mat_res, mat1, mat2;
{
    int i, j, k;
    transform_matrix mat_res_temp;

    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++) {
	    mat_res_temp[i][j] = 0;
	    for (k = 0; k < 4; k++)
		mat_res_temp[i][j] += mat1[i][k] * mat2[k][j];
	}
    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
	    mat_res[i][j] = mat_res_temp[i][j];
}


/* Test a single point to be within the xleft,xright,ybot,ytop bbox.
 * Sets the returned integers 4 l.s.b. as follows:
 * bit 0 if to the left of xleft.
 * bit 1 if to the right of xright.
 * bit 2 if above of ytop.
 * bit 3 if below of ybot.
 * 0 is returned if inside.
 */
int
clip_point(x, y)
unsigned int x, y;
{
    int ret_val = 0;

    if (x < xleft)
	ret_val |= 0x01;
    if (x > xright)
	ret_val |= 0x02;
    if (y < ybot)
	ret_val |= 0x04;
    if (y > ytop)
	ret_val |= 0x08;

    return ret_val;
}

/* Clip the given line to drawing coords defined as xleft,xright,ybot,ytop.
 *   This routine uses the cohen & sutherland bit mapping for fast clipping -
 * see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
 */
void
draw_clip_line(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
    int x, y, dx, dy, x_intr[4], y_intr[4], count, pos1, pos2;
    register struct termentry *t = term;

#if defined(ATARI) || defined(MTOS)
    if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0)
	return;			/* temp bug fix */
#endif

    pos1 = clip_point(x1, y1);
    pos2 = clip_point(x2, y2);
    if (pos1 || pos2) {
	if (pos1 & pos2)
	    return;		/* segment is totally out. */

	/* Here part of the segment MAY be inside. test the intersection
	 * of this segment with the 4 boundaries for hopefully 2 intersections
	 * in. If none are found segment is totaly out.
	 * Under rare circumstances there may be up to 4 intersections (e.g.
	 * when the line passes directly through at least one corner). In
	 * this case it is sufficient to take any 2 intersections (e.g. the
	 * first two found).
	 */
	count = 0;
	dx = x2 - x1;
	dy = y2 - y1;

	/* Find intersections with the x parallel bbox lines: */
	if (dy != 0) {
	    x = (ybot - y2) * dx / dy + x2;	/* Test for ybot boundary. */
	    if (x >= xleft && x <= xright) {
		x_intr[count] = x;
		y_intr[count++] = ybot;
	    }
	    x = (ytop - y2) * dx / dy + x2;	/* Test for ytop boundary. */
	    if (x >= xleft && x <= xright) {
		x_intr[count] = x;
		y_intr[count++] = ytop;
	    }
	}
	/* Find intersections with the y parallel bbox lines: */
	if (dx != 0) {
	    y = (xleft - x2) * dy / dx + y2;	/* Test for xleft boundary. */
	    if (y >= ybot && y <= ytop) {
		x_intr[count] = xleft;
		y_intr[count++] = y;
	    }
	    y = (xright - x2) * dy / dx + y2;	/* Test for xright boundary. */
	    if (y >= ybot && y <= ytop) {
		x_intr[count] = xright;
		y_intr[count++] = y;
	    }
	}
	if (count >= 2) {
	    int x_max, x_min, y_max, y_min;

	    x_min = GPMIN(x1, x2);
	    x_max = GPMAX(x1, x2);
	    y_min = GPMIN(y1, y2);
	    y_max = GPMAX(y1, y2);

	    if (pos1 && pos2) {	/* Both were out - update both */
		x1 = x_intr[0];
		y1 = y_intr[0];
		x2 = x_intr[1];
		y2 = y_intr[1];
	    } else if (pos1) {	/* Only x1/y1 was out - update only it */
		if (dx * (x2 - x_intr[0]) + dy * (y2 - y_intr[0]) > 0) {
		    x1 = x_intr[0];
		    y1 = y_intr[0];
		} else {
		    x1 = x_intr[1];
		    y1 = y_intr[1];
		}
	    } else {		/* Only x2/y2 was out - update only it */
		if (dx * (x_intr[0] - x1) + dy * (y_intr[0] - y1) > 0) {
		    x2 = x_intr[0];
		    y2 = y_intr[0];
		} else {
		    x2 = x_intr[1];
		    y2 = y_intr[1];
		}
	    }

	    if (x1 < x_min || x1 > x_max ||
		x2 < x_min || x2 > x_max ||
		y1 < y_min || y1 > y_max ||
		y2 < y_min || y2 > y_max)
		return;
	} else
	    return;
    }
#ifndef LITE
    if (hidden3d && hidden_active && draw_surface) {
	draw_line_hidden(x1, y1, x2, y2);
	return;
    };
#endif /* not LITE */
    if (!suppressMove)
	(*t->move) (x1, y1);
    (*t->vector) (x2, y2);
}



/* And text clipping routine. */
void
clip_put_text(x, y, str)
unsigned int x, y;
char *str;
{
    register struct termentry *t = term;

    if (clip_point(x, y))
	return;

    (*t->put_text) (x, y, str);
}

/* seems sensible to put the justification in here too..? */
void
clip_put_text_just(x, y, str, just)
unsigned int x, y;
char *str;
enum JUSTIFY just;
{
    register struct termentry *t = term;
    if (clip_point(x, y))
	return;
    if (!(*t->justify_text) (just)) {
	assert(CENTRE == 1 && RIGHT == 2);
	x -= (t->h_char * strlen(str) * just) / 2;
    }
    (*t->put_text) (x, y, str);
}



/* Clip the given line to drawing coords defined as xleft,xright,ybot,ytop.
 *   This routine uses the cohen & sutherland bit mapping for fast clipping -
 * see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
 */

int
clip_line(x1, y1, x2, y2)
int *x1, *y1, *x2, *y2;
{
    int x, y, dx, dy, x_intr[4], y_intr[4], count, pos1, pos2;
    int x_max, x_min, y_max, y_min;
    pos1 = clip_point(*x1, *y1);
    pos2 = clip_point(*x2, *y2);
    if (!pos1 && !pos2)
	return 1;		/* segment is totally in */
    if (pos1 & pos2)
	return 0;		/* segment is totally out. */
    /* Here part of the segment MAY be inside. test the intersection
     * of this segment with the 4 boundaries for hopefully 2 intersections
     * in. If non found segment is totaly out.
     */
    count = 0;
    dx = *x2 - *x1;
    dy = *y2 - *y1;
    /* Find intersections with the x parallel bbox lines: */
    if (dy != 0) {
	x = (ybot - *y2) * dx / dy + *x2;	/* Test for ybot boundary. */
	if (x >= xleft && x <= xright) {
	    x_intr[count] = x;
	    y_intr[count++] = ybot;
	}
	x = (ytop - *y2) * dx / dy + *x2;	/* Test for ytop boundary. */
	if (x >= xleft && x <= xright) {
	    x_intr[count] = x;
	    y_intr[count++] = ytop;
	}
    }
    /* Find intersections with the y parallel bbox lines: */
    if (dx != 0) {
	y = (xleft - *x2) * dy / dx + *y2;	/* Test for xleft boundary. */
	if (y >= ybot && y <= ytop) {
	    x_intr[count] = xleft;
	    y_intr[count++] = y;
	}
	y = (xright - *x2) * dy / dx + *y2;	/* Test for xright boundary. */
	if (y >= ybot && y <= ytop) {
	    x_intr[count] = xright;
	    y_intr[count++] = y;
	}
    }
    if (count < 2)
	return 0;
    if (*x1 < *x2)
	x_min = *x1, x_max = *x2;
    else
	x_min = *x2, x_max = *x1;
    if (*y1 < *y2)
	y_min = *y1, y_max = *y2;
    else
	y_min = *y2, y_max = *y1;
    if (pos1 && pos2) {		/* Both were out - update both */
	*x1 = x_intr[0];
	*y1 = y_intr[0];
	*x2 = x_intr[1];
	*y2 = y_intr[1];
    } else if (pos1) {		/* Only x1/y1 was out - update only it */
	if (dx * (*x2 - x_intr[0]) + dy * (*y2 - y_intr[0]) >= 0) {
	    *x1 = x_intr[0];
	    *y1 = y_intr[0];
	} else {
	    *x1 = x_intr[1];
	    *y1 = y_intr[1];
	}
    } else {			/* Only x2/y2 was out - update only it */
	if (dx * (x_intr[0] - *x1) + dy * (y_intr[0] - *y1) >= 0) {
	    *x2 = x_intr[0];
	    *y2 = y_intr[0];
	} else {
	    *x2 = x_intr[1];
	    *y2 = y_intr[1];
	}
    }

    if (*x1 < x_min || *x1 > x_max ||
	*x2 < x_min || *x2 > x_max ||
	*y1 < y_min || *y1 > y_max ||
	*y2 < y_min || *y2 > y_max)
	return 0;
    return 1;
}


/* single edge intersection algorithm */
/* Given two points, one inside and one outside the plot, return
 * the point where an edge of the plot intersects the line segment defined 
 * by the two points.
 */
void
edge3d_intersect(points, i, ex, ey, ez)
struct coordinate GPHUGE *points;	/* the points array */
int i;				/* line segment from point i-1 to point i */
double *ex, *ey, *ez;		/* the point where it crosses an edge */
{
    int count;
    double ix = points[i - 1].x;
    double iy = points[i - 1].y;
    double iz = points[i - 1].z;
    double ox = points[i].x;
    double oy = points[i].y;
    double oz = points[i].z;
    double x, y, z;		/* possible intersection point */

    if (points[i].type == INRANGE) {
	/* swap points around so that ix/ix/iz are INRANGE and ox/oy/oz are OUTRANGE */
	x = ix;
	ix = ox;
	ox = x;
	y = iy;
	iy = oy;
	oy = y;
	z = iz;
	iz = oz;
	oz = z;
    }
    /* nasty degenerate cases, effectively drawing to an infinity point (?)
       cope with them here, so don't process them as a "real" OUTRANGE point 

       If more than one coord is -VERYLARGE, then can't ratio the "infinities"
       so drop out by returning the INRANGE point.

       Obviously, only need to test the OUTRANGE point (coordinates) */

    /* nasty degenerate cases, effectively drawing to an infinity point (?)
       cope with them here, so don't process them as a "real" OUTRANGE point 

       If more than one coord is -VERYLARGE, then can't ratio the "infinities"
       so drop out by returning FALSE */

    count = 0;
    if (ox == -VERYLARGE)
	count++;
    if (oy == -VERYLARGE)
	count++;
    if (oz == -VERYLARGE)
	count++;

    /* either doesn't pass through 3D volume *or* 
       can't ratio infinities to get a direction to draw line, so return the INRANGE point */
    if (count > 1) {
	*ex = ix;
	*ey = iy;
	*ez = iz;

	return;
    }
    if (count == 1) {
	*ex = ix;
	*ey = iy;
	*ez = iz;

	if (ox == -VERYLARGE) {
	    *ex = min_array[FIRST_X_AXIS];
	    return;
	}
	if (oy == -VERYLARGE) {
	    *ey = min_array[FIRST_Y_AXIS];
	    return;
	}
	/* obviously oz is -VERYLARGE and (ox != -VERYLARGE && oy != -VERYLARGE) */
	*ez = min_array[FIRST_Z_AXIS];
	return;
    }
    /*
     * Can't have case (ix == ox && iy == oy && iz == oz) as one point
     * is INRANGE and one point is OUTRANGE.
     */
    if (ix == ox) {
	if (iy == oy) {
	    /* line parallel to z axis */

	    /* assume inrange(iy, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS]) && inrange(ix, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) */
	    *ex = ix;		/* == ox */
	    *ey = iy;		/* == oy */

	    if (inrange(max_array[FIRST_Z_AXIS], iz, oz))
		*ez = max_array[FIRST_Z_AXIS];
	    else if (inrange(min_array[FIRST_Z_AXIS], iz, oz))
		*ez = min_array[FIRST_Z_AXIS];
	    else {
		graph_error("error in edge3d_intersect");
	    }

	    return;
	}
	if (iz == oz) {
	    /* line parallel to y axis */

	    /* assume inrange(iz, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS]) && inrange(ix, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) */
	    *ex = ix;		/* == ox */
	    *ez = iz;		/* == oz */

	    if (inrange(max_array[FIRST_Y_AXIS], iy, oy))
		*ey = max_array[FIRST_Y_AXIS];
	    else if (inrange(min_array[FIRST_Y_AXIS], iy, oy))
		*ey = min_array[FIRST_Y_AXIS];
	    else {
		graph_error("error in edge3d_intersect");
	    }

	    return;
	}
	/* nasty 2D slanted line in a yz plane */

	/* does it intersect min_array[FIRST_Y_AXIS] edge */
	if (inrange(min_array[FIRST_Y_AXIS], iy, oy) && min_array[FIRST_Y_AXIS] != iy && min_array[FIRST_Y_AXIS] != oy) {
	    z = iz + (min_array[FIRST_Y_AXIS] - iy) * ((oz - iz) / (oy - iy));
	    if (inrange(z, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
		*ex = ix;
		*ey = min_array[FIRST_Y_AXIS];
		*ez = z;
		return;
	    }
	}
	/* does it intersect max_array[FIRST_Y_AXIS] edge */
	if (inrange(max_array[FIRST_Y_AXIS], iy, oy) && max_array[FIRST_Y_AXIS] != iy && max_array[FIRST_Y_AXIS] != oy) {
	    z = iz + (max_array[FIRST_Y_AXIS] - iy) * ((oz - iz) / (oy - iy));
	    if (inrange(z, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
		*ex = ix;
		*ey = max_array[FIRST_Y_AXIS];
		*ez = z;
		return;
	    }
	}
	/* does it intersect min_array[FIRST_Z_AXIS] edge */
	if (inrange(min_array[FIRST_Z_AXIS], iz, oz) && min_array[FIRST_Z_AXIS] != iz && min_array[FIRST_Z_AXIS] != oz) {
	    y = iy + (min_array[FIRST_Z_AXIS] - iz) * ((oy - iy) / (oz - iz));
	    if (inrange(y, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS])) {
		*ex = ix;
		*ey = y;
		*ez = min_array[FIRST_Z_AXIS];
		return;
	    }
	}
	/* does it intersect max_array[FIRST_Z_AXIS] edge */
	if (inrange(max_array[FIRST_Z_AXIS], iz, oz) && max_array[FIRST_Z_AXIS] != iz && max_array[FIRST_Z_AXIS] != oz) {
	    y = iy + (max_array[FIRST_Z_AXIS] - iz) * ((oy - iy) / (oz - iz));
	    if (inrange(y, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS])) {
		*ex = ix;
		*ey = y;
		*ez = max_array[FIRST_Z_AXIS];
		return;
	    }
	}
    }
    if (iy == oy) {
	/* already checked case (ix == ox && iy == oy) */
	if (oz == iz) {
	    /* line parallel to x axis */

	    /* assume inrange(iz, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS]) && inrange(iy, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS]) */
	    *ey = iy;		/* == oy */
	    *ez = iz;		/* == oz */

	    if (inrange(max_array[FIRST_X_AXIS], ix, ox))
		*ex = max_array[FIRST_X_AXIS];
	    else if (inrange(min_array[FIRST_X_AXIS], ix, ox))
		*ex = min_array[FIRST_X_AXIS];
	    else {
		graph_error("error in edge3d_intersect");
	    }

	    return;
	}
	/* nasty 2D slanted line in an xz plane */

	/* does it intersect min_array[FIRST_X_AXIS] edge */
	if (inrange(min_array[FIRST_X_AXIS], ix, ox) && min_array[FIRST_X_AXIS] != ix && min_array[FIRST_X_AXIS] != ox) {
	    z = iz + (min_array[FIRST_X_AXIS] - ix) * ((oz - iz) / (ox - ix));
	    if (inrange(z, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
		*ex = min_array[FIRST_X_AXIS];
		*ey = iy;
		*ez = z;
		return;
	    }
	}
	/* does it intersect max_array[FIRST_X_AXIS] edge */
	if (inrange(max_array[FIRST_X_AXIS], ix, ox) && max_array[FIRST_X_AXIS] != ix && max_array[FIRST_X_AXIS] != ox) {
	    z = iz + (max_array[FIRST_X_AXIS] - ix) * ((oz - iz) / (ox - ix));
	    if (inrange(z, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
		*ex = max_array[FIRST_X_AXIS];
		*ey = iy;
		*ez = z;
		return;
	    }
	}
	/* does it intersect min_array[FIRST_Z_AXIS] edge */
	if (inrange(min_array[FIRST_Z_AXIS], iz, oz) && min_array[FIRST_Z_AXIS] != iz && min_array[FIRST_Z_AXIS] != oz) {
	    x = ix + (min_array[FIRST_Z_AXIS] - iz) * ((ox - ix) / (oz - iz));
	    if (inrange(x, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS])) {
		*ex = x;
		*ey = iy;
		*ez = min_array[FIRST_Z_AXIS];
		return;
	    }
	}
	/* does it intersect max_array[FIRST_Z_AXIS] edge */
	if (inrange(max_array[FIRST_Z_AXIS], iz, oz) && max_array[FIRST_Z_AXIS] != iz && max_array[FIRST_Z_AXIS] != oz) {
	    x = ix + (max_array[FIRST_Z_AXIS] - iz) * ((ox - ix) / (oz - iz));
	    if (inrange(x, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS])) {
		*ex = x;
		*ey = iy;
		*ez = max_array[FIRST_Z_AXIS];
		return;
	    }
	}
    }
    if (iz == oz) {
	/* already checked cases (ix == ox && iz == oz) and (iy == oy && iz == oz) */

	/* nasty 2D slanted line in an xy plane */

	/* assume inrange(oz, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS]) */

	/* does it intersect min_array[FIRST_X_AXIS] edge */
	if (inrange(min_array[FIRST_X_AXIS], ix, ox) && min_array[FIRST_X_AXIS] != ix && min_array[FIRST_X_AXIS] != ox) {
	    y = iy + (min_array[FIRST_X_AXIS] - ix) * ((oy - iy) / (ox - ix));
	    if (inrange(y, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS])) {
		*ex = min_array[FIRST_X_AXIS];
		*ey = y;
		*ez = iz;
		return;
	    }
	}
	/* does it intersect max_array[FIRST_X_AXIS] edge */
	if (inrange(max_array[FIRST_X_AXIS], ix, ox) && max_array[FIRST_X_AXIS] != ix && max_array[FIRST_X_AXIS] != ox) {
	    y = iy + (max_array[FIRST_X_AXIS] - ix) * ((oy - iy) / (ox - ix));
	    if (inrange(y, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS])) {
		*ex = max_array[FIRST_X_AXIS];
		*ey = y;
		*ez = iz;
		return;
	    }
	}
	/* does it intersect min_array[FIRST_Y_AXIS] edge */
	if (inrange(min_array[FIRST_Y_AXIS], iy, oy) && min_array[FIRST_Y_AXIS] != iy && min_array[FIRST_Y_AXIS] != oy) {
	    x = ix + (min_array[FIRST_Y_AXIS] - iy) * ((ox - ix) / (oy - iy));
	    if (inrange(x, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS])) {
		*ex = x;
		*ey = min_array[FIRST_Y_AXIS];
		*ez = iz;
		return;
	    }
	}
	/* does it intersect max_array[FIRST_Y_AXIS] edge */
	if (inrange(max_array[FIRST_Y_AXIS], iy, oy) && max_array[FIRST_Y_AXIS] != iy && max_array[FIRST_Y_AXIS] != oy) {
	    x = ix + (max_array[FIRST_Y_AXIS] - iy) * ((ox - ix) / (oy - iy));
	    if (inrange(x, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS])) {
		*ex = x;
		*ey = max_array[FIRST_Y_AXIS];
		*ez = iz;
		return;
	    }
	}
    }
    /* really nasty general slanted 3D case */

    /* does it intersect min_array[FIRST_X_AXIS] edge */
    if (inrange(min_array[FIRST_X_AXIS], ix, ox) && min_array[FIRST_X_AXIS] != ix && min_array[FIRST_X_AXIS] != ox) {
	y = iy + (min_array[FIRST_X_AXIS] - ix) * ((oy - iy) / (ox - ix));
	z = iz + (min_array[FIRST_X_AXIS] - ix) * ((oz - iz) / (ox - ix));
	if (inrange(y, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS]) && inrange(z, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
	    *ex = min_array[FIRST_X_AXIS];
	    *ey = y;
	    *ez = z;
	    return;
	}
    }
    /* does it intersect max_array[FIRST_X_AXIS] edge */
    if (inrange(max_array[FIRST_X_AXIS], ix, ox) && max_array[FIRST_X_AXIS] != ix && max_array[FIRST_X_AXIS] != ox) {
	y = iy + (max_array[FIRST_X_AXIS] - ix) * ((oy - iy) / (ox - ix));
	z = iz + (max_array[FIRST_X_AXIS] - ix) * ((oz - iz) / (ox - ix));
	if (inrange(y, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS]) && inrange(z, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
	    *ex = max_array[FIRST_X_AXIS];
	    *ey = y;
	    *ez = z;
	    return;
	}
    }
    /* does it intersect min_array[FIRST_Y_AXIS] edge */
    if (inrange(min_array[FIRST_Y_AXIS], iy, oy) && min_array[FIRST_Y_AXIS] != iy && min_array[FIRST_Y_AXIS] != oy) {
	x = ix + (min_array[FIRST_Y_AXIS] - iy) * ((ox - ix) / (oy - iy));
	z = iz + (min_array[FIRST_Y_AXIS] - iy) * ((oz - iz) / (oy - iy));
	if (inrange(x, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) && inrange(z, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
	    *ex = x;
	    *ey = min_array[FIRST_Y_AXIS];
	    *ez = z;
	    return;
	}
    }
    /* does it intersect max_array[FIRST_Y_AXIS] edge */
    if (inrange(max_array[FIRST_Y_AXIS], iy, oy) && max_array[FIRST_Y_AXIS] != iy && max_array[FIRST_Y_AXIS] != oy) {
	x = ix + (max_array[FIRST_Y_AXIS] - iy) * ((ox - ix) / (oy - iy));
	z = iz + (max_array[FIRST_Y_AXIS] - iy) * ((oz - iz) / (oy - iy));
	if (inrange(x, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) && inrange(z, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
	    *ex = x;
	    *ey = max_array[FIRST_Y_AXIS];
	    *ez = z;
	    return;
	}
    }
    /* does it intersect min_array[FIRST_Z_AXIS] edge */
    if (inrange(min_array[FIRST_Z_AXIS], iz, oz) && min_array[FIRST_Z_AXIS] != iz && min_array[FIRST_Z_AXIS] != oz) {
	x = ix + (min_array[FIRST_Z_AXIS] - iz) * ((ox - ix) / (oz - iz));
	y = iy + (min_array[FIRST_Z_AXIS] - iz) * ((oy - iy) / (oz - iz));
	if (inrange(x, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) && inrange(y, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS])) {
	    *ex = x;
	    *ey = y;
	    *ez = min_array[FIRST_Z_AXIS];
	    return;
	}
    }
    /* does it intersect max_array[FIRST_Z_AXIS] edge */
    if (inrange(max_array[FIRST_Z_AXIS], iz, oz) && max_array[FIRST_Z_AXIS] != iz && max_array[FIRST_Z_AXIS] != oz) {
	x = ix + (max_array[FIRST_Z_AXIS] - iz) * ((ox - ix) / (oz - iz));
	y = iy + (max_array[FIRST_Z_AXIS] - iz) * ((oy - iy) / (oz - iz));
	if (inrange(x, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) && inrange(y, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS])) {
	    *ex = x;
	    *ey = y;
	    *ez = max_array[FIRST_Z_AXIS];
	    return;
	}
    }
    /* If we reach here, the inrange point is on the edge, and
     * the line segment from the outrange point does not cross any 
     * other edges to get there. In this case, we return the inrange 
     * point as the 'edge' intersection point. This will basically draw
     * line.
     */
    *ex = ix;
    *ey = iy;
    *ez = iz;
    return;
}

/* double edge intersection algorithm */
/* Given two points, both outside the plot, return
 * the points where an edge of the plot intersects the line segment defined 
 * by the two points. There may be zero, one, two, or an infinite number
 * of intersection points. (One means an intersection at a corner, infinite
 * means overlaying the edge itself). We return FALSE when there is nothing
 * to draw (zero intersections), and TRUE when there is something to 
 * draw (the one-point case is a degenerate of the two-point case and we do 
 * not distinguish it - we draw it anyway).
 */
TBOOLEAN			/* any intersection? */
two_edge3d_intersect(points, i, lx, ly, lz)
struct coordinate GPHUGE *points;	/* the points array */
int i;				/* line segment from point i-1 to point i */
double *lx, *ly, *lz;		/* lx[2], ly[2], lz[2]: points where it crosses edges */
{
    int count;
    /* global min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS], min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS], min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS] */
    double ix = points[i - 1].x;
    double iy = points[i - 1].y;
    double iz = points[i - 1].z;
    double ox = points[i].x;
    double oy = points[i].y;
    double oz = points[i].z;
    double t[6];
    double swap;
    double x, y, z;		/* possible intersection point */
    double t_min, t_max;

    /* nasty degenerate cases, effectively drawing to an infinity point (?)
       cope with them here, so don't process them as a "real" OUTRANGE point 

       If more than one coord is -VERYLARGE, then can't ratio the "infinities"
       so drop out by returning FALSE */

    count = 0;
    if (ix == -VERYLARGE)
	count++;
    if (ox == -VERYLARGE)
	count++;
    if (iy == -VERYLARGE)
	count++;
    if (oy == -VERYLARGE)
	count++;
    if (iz == -VERYLARGE)
	count++;
    if (oz == -VERYLARGE)
	count++;

    /* either doesn't pass through 3D volume *or* 
       can't ratio infinities to get a direction to draw line, so simply return(FALSE) */
    if (count > 1) {
	return (FALSE);
    }
    if (ox == -VERYLARGE || ix == -VERYLARGE) {
	if (ix == -VERYLARGE) {
	    /* swap points so ix/iy/iz don't have a -VERYLARGE component */
	    x = ix;
	    ix = ox;
	    ox = x;
	    y = iy;
	    iy = oy;
	    oy = y;
	    z = iz;
	    iz = oz;
	    oz = z;
	}
	/* check actually passes through the 3D graph volume */
	if (ix > max_array[FIRST_X_AXIS] && inrange(iy, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS]) && inrange(iz, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
	    lx[0] = min_array[FIRST_X_AXIS];
	    ly[0] = iy;
	    lz[0] = iz;

	    lx[1] = max_array[FIRST_X_AXIS];
	    ly[1] = iy;
	    lz[1] = iz;

	    return (TRUE);
	} else {
	    return (FALSE);
	}
    }
    if (oy == -VERYLARGE || iy == -VERYLARGE) {
	if (iy == -VERYLARGE) {
	    /* swap points so ix/iy/iz don't have a -VERYLARGE component */
	    x = ix;
	    ix = ox;
	    ox = x;
	    y = iy;
	    iy = oy;
	    oy = y;
	    z = iz;
	    iz = oz;
	    oz = z;
	}
	/* check actually passes through the 3D graph volume */
	if (iy > max_array[FIRST_Y_AXIS] && inrange(ix, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) && inrange(iz, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
	    lx[0] = ix;
	    ly[0] = min_array[FIRST_Y_AXIS];
	    lz[0] = iz;

	    lx[1] = ix;
	    ly[1] = max_array[FIRST_Y_AXIS];
	    lz[1] = iz;

	    return (TRUE);
	} else {
	    return (FALSE);
	}
    }
    if (oz == -VERYLARGE || iz == -VERYLARGE) {
	if (iz == -VERYLARGE) {
	    /* swap points so ix/iy/iz don't have a -VERYLARGE component */
	    x = ix;
	    ix = ox;
	    ox = x;
	    y = iy;
	    iy = oy;
	    oy = y;
	    z = iz;
	    iz = oz;
	    oz = z;
	}
	/* check actually passes through the 3D graph volume */
	if (iz > max_array[FIRST_Z_AXIS] && inrange(ix, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) && inrange(iy, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS])) {
	    lx[0] = ix;
	    ly[0] = iy;
	    lz[0] = min_array[FIRST_Z_AXIS];

	    lx[1] = ix;
	    ly[1] = iy;
	    lz[1] = max_array[FIRST_Z_AXIS];

	    return (TRUE);
	} else {
	    return (FALSE);
	}
    }
    /*
     * Quick outcode tests on the 3d graph volume
     */

    /* 
     * test z coord first --- most surface OUTRANGE points generated between
     * min_array[FIRST_Z_AXIS] and min_array[FIRST_Z_AXIS] (i.e. when ticslevel is non-zero)
     */
    if (GPMAX(iz, oz) < min_array[FIRST_Z_AXIS] || GPMIN(iz, oz) > max_array[FIRST_Z_AXIS])
	return (FALSE);

    if (GPMAX(ix, ox) < min_array[FIRST_X_AXIS] || GPMIN(ix, ox) > max_array[FIRST_X_AXIS])
	return (FALSE);

    if (GPMAX(iy, oy) < min_array[FIRST_Y_AXIS] || GPMIN(iy, oy) > max_array[FIRST_Y_AXIS])
	return (FALSE);

    /*
     * Special horizontal/vertical, etc. cases are checked and remaining
     * slant lines are checked separately.
     *
     * The slant line intersections are solved using the parametric form
     * of the equation for a line, since if we test x/y/z min/max planes explicitly
     * then e.g. a  line passing through a corner point (x_min,y_min,z_min) 
     * actually intersects all 3 planes and hence further tests would be required 
     * to anticipate this and similar situations.
     */

    /*
     * Can have case (ix == ox && iy == oy && iz == oz) as both points OUTRANGE
     */
    if (ix == ox && iy == oy && iz == oz) {
	/* but as only define single outrange point, can't intersect 3D graph volume */
	return (FALSE);
    }
    if (ix == ox) {
	if (iy == oy) {
	    /* line parallel to z axis */

	    /* x and y coords must be in range, and line must span both min_array[FIRST_Z_AXIS] and max_array[FIRST_Z_AXIS] */
	    /* note that spanning min_array[FIRST_Z_AXIS] implies spanning max_array[FIRST_Z_AXIS] as both points OUTRANGE */
	    if (!inrange(ix, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) || !inrange(iy, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS])) {
		return (FALSE);
	    }
	    if (inrange(min_array[FIRST_Z_AXIS], iz, oz)) {
		lx[0] = ix;
		ly[0] = iy;
		lz[0] = min_array[FIRST_Z_AXIS];

		lx[1] = ix;
		ly[1] = iy;
		lz[1] = max_array[FIRST_Z_AXIS];

		return (TRUE);
	    } else
		return (FALSE);
	}
	if (iz == oz) {
	    /* line parallel to y axis */

	    /* x and z coords must be in range, and line must span both min_array[FIRST_Y_AXIS] and max_array[FIRST_Y_AXIS] */
	    /* note that spanning min_array[FIRST_Y_AXIS] implies spanning max_array[FIRST_Y_AXIS], as both points OUTRANGE */
	    if (!inrange(ix, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) || !inrange(iz, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
		return (FALSE);
	    }
	    if (inrange(min_array[FIRST_Y_AXIS], iy, oy)) {
		lx[0] = ix;
		ly[0] = min_array[FIRST_Y_AXIS];
		lz[0] = iz;

		lx[1] = ix;
		ly[1] = max_array[FIRST_Y_AXIS];
		lz[1] = iz;

		return (TRUE);
	    } else
		return (FALSE);
	}
	/* nasty 2D slanted line in a yz plane */

	if (!inrange(ox, min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]))
	    return (FALSE);

	t[0] = (min_array[FIRST_Y_AXIS] - iy) / (oy - iy);
	t[1] = (max_array[FIRST_Y_AXIS] - iy) / (oy - iy);

	if (t[0] > t[1]) {
	    swap = t[0];
	    t[0] = t[1];
	    t[1] = swap;
	}
	t[2] = (min_array[FIRST_Z_AXIS] - iz) / (oz - iz);
	t[3] = (max_array[FIRST_Z_AXIS] - iz) / (oz - iz);

	if (t[2] > t[3]) {
	    swap = t[2];
	    t[2] = t[3];
	    t[3] = swap;
	}
	t_min = GPMAX(GPMAX(t[0], t[2]), 0.0);
	t_max = GPMIN(GPMIN(t[1], t[3]), 1.0);

	if (t_min > t_max)
	    return (FALSE);

	lx[0] = ix;
	ly[0] = iy + t_min * (oy - iy);
	lz[0] = iz + t_min * (oz - iz);

	lx[1] = ix;
	ly[1] = iy + t_max * (oy - iy);
	lz[1] = iz + t_max * (oz - iz);

	/*
	 * Can only have 0 or 2 intersection points -- only need test one coord
	 */
	if (inrange(ly[0], min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS]) &&
	    inrange(lz[0], min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
	    return (TRUE);
	}
	return (FALSE);
    }
    if (iy == oy) {
	/* already checked case (ix == ox && iy == oy) */
	if (oz == iz) {
	    /* line parallel to x axis */

	    /* y and z coords must be in range, and line must span both min_array[FIRST_X_AXIS] and max_array[FIRST_X_AXIS] */
	    /* note that spanning min_array[FIRST_X_AXIS] implies spanning max_array[FIRST_X_AXIS], as both points OUTRANGE */
	    if (!inrange(iy, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS]) || !inrange(iz, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
		return (FALSE);
	    }
	    if (inrange(min_array[FIRST_X_AXIS], ix, ox)) {
		lx[0] = min_array[FIRST_X_AXIS];
		ly[0] = iy;
		lz[0] = iz;

		lx[1] = max_array[FIRST_X_AXIS];
		ly[1] = iy;
		lz[1] = iz;

		return (TRUE);
	    } else
		return (FALSE);
	}
	/* nasty 2D slanted line in an xz plane */

	if (!inrange(oy, min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS]))
	    return (FALSE);

	t[0] = (min_array[FIRST_X_AXIS] - ix) / (ox - ix);
	t[1] = (max_array[FIRST_X_AXIS] - ix) / (ox - ix);

	if (t[0] > t[1]) {
	    swap = t[0];
	    t[0] = t[1];
	    t[1] = swap;
	}
	t[2] = (min_array[FIRST_Z_AXIS] - iz) / (oz - iz);
	t[3] = (max_array[FIRST_Z_AXIS] - iz) / (oz - iz);

	if (t[2] > t[3]) {
	    swap = t[2];
	    t[2] = t[3];
	    t[3] = swap;
	}
	t_min = GPMAX(GPMAX(t[0], t[2]), 0.0);
	t_max = GPMIN(GPMIN(t[1], t[3]), 1.0);

	if (t_min > t_max)
	    return (FALSE);

	lx[0] = ix + t_min * (ox - ix);
	ly[0] = iy;
	lz[0] = iz + t_min * (oz - iz);

	lx[1] = ix + t_max * (ox - ix);
	ly[1] = iy;
	lz[1] = iz + t_max * (oz - iz);

	/*
	 * Can only have 0 or 2 intersection points -- only need test one coord
	 */
	if (inrange(lx[0], min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) &&
	    inrange(lz[0], min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
	    return (TRUE);
	}
	return (FALSE);
    }
    if (iz == oz) {
	/* already checked cases (ix == ox && iz == oz) and (iy == oy && iz == oz) */

	/* nasty 2D slanted line in an xy plane */

	if (!inrange(oz, min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS]))
	    return (FALSE);

	t[0] = (min_array[FIRST_X_AXIS] - ix) / (ox - ix);
	t[1] = (max_array[FIRST_X_AXIS] - ix) / (ox - ix);

	if (t[0] > t[1]) {
	    swap = t[0];
	    t[0] = t[1];
	    t[1] = swap;
	}
	t[2] = (min_array[FIRST_Y_AXIS] - iy) / (oy - iy);
	t[3] = (max_array[FIRST_Y_AXIS] - iy) / (oy - iy);

	if (t[2] > t[3]) {
	    swap = t[2];
	    t[2] = t[3];
	    t[3] = swap;
	}
	t_min = GPMAX(GPMAX(t[0], t[2]), 0.0);
	t_max = GPMIN(GPMIN(t[1], t[3]), 1.0);

	if (t_min > t_max)
	    return (FALSE);

	lx[0] = ix + t_min * (ox - ix);
	ly[0] = iy + t_min * (oy - iy);
	lz[0] = iz;

	lx[1] = ix + t_max * (ox - ix);
	ly[1] = iy + t_max * (oy - iy);
	lz[1] = iz;

	/*
	 * Can only have 0 or 2 intersection points -- only need test one coord
	 */
	if (inrange(lx[0], min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) &&
	    inrange(ly[0], min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS])) {
	    return (TRUE);
	}
	return (FALSE);
    }
    /* really nasty general slanted 3D case */

    /*
       Solve parametric equation

       (ix, iy, iz) + t (diff_x, diff_y, diff_z)

       where 0.0 <= t <= 1.0 and

       diff_x = (ox - ix);
       diff_y = (oy - iy);
       diff_z = (oz - iz);
     */

    t[0] = (min_array[FIRST_X_AXIS] - ix) / (ox - ix);
    t[1] = (max_array[FIRST_X_AXIS] - ix) / (ox - ix);

    if (t[0] > t[1]) {
	swap = t[0];
	t[0] = t[1];
	t[1] = swap;
    }
    t[2] = (min_array[FIRST_Y_AXIS] - iy) / (oy - iy);
    t[3] = (max_array[FIRST_Y_AXIS] - iy) / (oy - iy);

    if (t[2] > t[3]) {
	swap = t[2];
	t[2] = t[3];
	t[3] = swap;
    }
    t[4] = (iz == oz) ? 0.0 : (min_array[FIRST_Z_AXIS] - iz) / (oz - iz);
    t[5] = (iz == oz) ? 1.0 : (max_array[FIRST_Z_AXIS] - iz) / (oz - iz);

    if (t[4] > t[5]) {
	swap = t[4];
	t[4] = t[5];
	t[5] = swap;
    }
    t_min = GPMAX(GPMAX(t[0], t[2]), GPMAX(t[4], 0.0));
    t_max = GPMIN(GPMIN(t[1], t[3]), GPMIN(t[5], 1.0));

    if (t_min > t_max)
	return (FALSE);

    lx[0] = ix + t_min * (ox - ix);
    ly[0] = iy + t_min * (oy - iy);
    lz[0] = iz + t_min * (oz - iz);

    lx[1] = ix + t_max * (ox - ix);
    ly[1] = iy + t_max * (oy - iy);
    lz[1] = iz + t_max * (oz - iz);

    /*
     * Can only have 0 or 2 intersection points -- only need test one coord
     */
    if (inrange(lx[0], min_array[FIRST_X_AXIS], max_array[FIRST_X_AXIS]) &&
	inrange(ly[0], min_array[FIRST_Y_AXIS], max_array[FIRST_Y_AXIS]) &&
	inrange(lz[0], min_array[FIRST_Z_AXIS], max_array[FIRST_Z_AXIS])) {
	return (TRUE);
    }
    return (FALSE);
}
