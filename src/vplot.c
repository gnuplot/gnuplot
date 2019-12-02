/* GNUPLOT - vplot.c */

/*[
 * Copyright Ethan A Merritt 2019
 *
 * Gnuplot license:
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
 * Alternative license:
 *
 * As an alternative to distributing code in this file under the gnuplot license,
 * you may instead comply with the terms below. In this case, redistribution and
 * use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.  Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
]*/

/*
 * This file implements 3D plotting directly from a voxel grid
 * 	splot $voxelgrid with {dots|points} {above <iso_level>}
 *	splot $voxelgrid with isosurface level <iso_level>
 * For point or dot plots
 * 	points are only drawn if voxel value > iso_level (defaults to 0)
 * For isosurface plots
 *	the isosurface is generated for value == iso_level
 */

#include "gp_types.h"
#include "jitter.h"
#include "graph3d.h"
#include "pm3d.h"
#include "util.h"
#include "util3d.h"
#include "voxelgrid.h"
#include "vplot.h"

#ifdef VOXEL_GRID_SUPPORT

/*            Data structures for tessellation
 *            ===============================
 * We offer a choice of two tessellation tables.
 * One is Heller's table originally derived for use with marching cubes.
 * It contains triangles only.
 * The other contains a mix of quadrangles and triangles.
 * Near a complicated fold in the surface the quadrangles are an imperfect
 * approximation of the triangular tessellation.  However for most smooth
 * surfaces the reduced number of facets from using quadrangles makes the
 * pm3d rendering look cleaner.
 */
#include "marching_cubes.h"
#include "qt_table.h"

/* local copy of vertex offsets from voxel corner, scaled by downsampling */
static int scaled_offset[8][3];

/* the fractional index intersection along each of the cubes's 12 edges */
static double intersection[12][3];

/* working copy of the corner values for the current cube */
static t_voxel cornervalue[8];

/* local prototypes */
static void vertex_interp( int edge_no, int start, int end, t_voxel isolevel );
static void tessellate_one_cube( struct surface_points *plot,
					int ix, int iy, int iz );

/*
 * splot $vgrid with {dots|points} {above <value>}
 */
void
vplot_points (struct surface_points *plot, double level)
{
    int ix, iy, iz;
    double vx, vy, vz;
    struct vgrid *vgrid = plot->vgrid;
    struct termentry *t = term;
    int N = vgrid->size;
    t_voxel *voxel;
    int index;
    int x, y;
    int downsample = plot->lp_properties.p_interval;

    /* dots or points only */
    if (plot->lp_properties.p_type == PT_CHARACTER)
	plot->lp_properties.p_type = -1;
    if (plot->lp_properties.p_type == PT_VARIABLE)
	plot->lp_properties.p_type = -1;

    /* Set whatever we can that applies to every point in the loop */
    if (plot->lp_properties.pm3d_color.type == TC_RGB)
	set_rgbcolor_const( plot->lp_properties.pm3d_color.lt );

    for (ix = 0; ix < N; ix++) {
	for (iy = 0; iy < N; iy++) {
	    for (iz = 0; iz < N; iz++) {

		/* The pointinterval property can be used to downsample */
		if ((downsample > 0)
		&& (ix % downsample || iy % downsample || iz % downsample))
		    continue;

		index = ix + iy * N + iz * N*N;
		voxel = &vgrid->vdata[index];

		if (*voxel <= level)
		    continue;

		/* vx, vy, vz are the true coordinates of this voxel */
		vx = vgrid->vxmin + ix * vgrid->vxdelta;
		vy = vgrid->vymin + iy * vgrid->vydelta;
		vz = vgrid->vzmin + iz * vgrid->vzdelta;

		if (jitter.spread > 0) {
		    vx += jitter.spread * vgrid->vxdelta * ( (double)(rand()/(double)RAND_MAX ) - 0.5);
		    vy += jitter.spread * vgrid->vydelta * ( (double)(rand()/(double)RAND_MAX ) - 0.5);
		    vz += jitter.spread * vgrid->vzdelta * ( (double)(rand()/(double)RAND_MAX ) - 0.5);
		}

		map3d_xy(vx, vy, vz, &x, &y);

		/* the usual variable color array cannot be used for voxel data */
		/* but we can use the voxel value itself as a palette index     */
		if (plot->lp_properties.pm3d_color.type == TC_Z)
		    set_color(cb2gray(*voxel));

		/* This code is also used for "splot ... with dots" */
		if (plot->plot_style == DOTS)
		    (*t->point) (x, y, -1);

		/* The normal case */
		else if (plot->lp_properties.p_type >= 0)
		    (*t->point) (x, y, plot->lp_properties.p_type);

	    }
	}
    }

}

/*
 * splot $vgrid with isosurface level <value>
 */

void
vplot_isosurface (struct surface_points *plot, int downsample)
{
    int i, j, k;
    int N = plot->vgrid->size;

    /* Apply down-sampling, if any, to the vertex offsets */
    if (downsample > 1)
	downsample = ceil((double)N / 76.);
    if (downsample < 1)
	downsample = 1;

    for (i=0; i<8; i++) {
	for (j=0; j<3; j++)
	    scaled_offset[i][j] = downsample * vertex_offset[i][j];
    }

    /* These initializations are normally done in pm3d_plot()
     * isosurfaces do not use that code path.
     */
    if (pm3d_shade.strength > 0)
	pm3d_init_lighting_model();

    for (i = 0; i < N - downsample; i += downsample) {
	for (j = 0; j < N - downsample; j += downsample) {
	    for (k = 0; k < N - downsample; k += downsample) {
		tessellate_one_cube( plot, i, j, k );
	    }
	}
    }
}


/*
 * tessellation algorithm applied to a single voxel.
 * ix, iy, iz are the indices of the corner nearest [xmin, ymin, zmin].
 * We will work in index space and convert back to actual graph coordinates
 * when we have found the triangles (if any) that result from intersections
 * of the isosurface with this voxel.
 */
static void
tessellate_one_cube( struct surface_points *plot, int ix, int iy, int iz )
{
    struct vgrid *vgrid = plot->vgrid;
    t_voxel isolevel = plot->iso_level;
    int N = vgrid->size;
    int ivertex, iedge, it;
    int corner_flags;			/* bit field */
    int edge_flags;			/* bit field */

    /* Make a local copy of the values at the cube corners */
    for (ivertex = 0; ivertex < 8; ivertex++) {
	int cx = ix + scaled_offset[ivertex][0];
	int cy = iy + scaled_offset[ivertex][1];
	int cz = iz + scaled_offset[ivertex][2];
	cornervalue[ivertex] = vgrid->vdata[cx + cy*N + cz*N*N];
    }

    /* Flag which vertices are inside the surface and which are outside */
    corner_flags = 0;
    if (cornervalue[0] < isolevel) corner_flags |= 1;
    if (cornervalue[1] < isolevel) corner_flags |= 2;
    if (cornervalue[2] < isolevel) corner_flags |= 4;
    if (cornervalue[3] < isolevel) corner_flags |= 8;
    if (cornervalue[4] < isolevel) corner_flags |= 16;
    if (cornervalue[5] < isolevel) corner_flags |= 32;
    if (cornervalue[6] < isolevel) corner_flags |= 64;
    if (cornervalue[7] < isolevel) corner_flags |= 128;

    /* Look up which edges are affected by this corner pattern */
    edge_flags = cube_edge_flags[corner_flags];

    /* If no edges are affected (surface does not intersect voxel) we're done */
    if (edge_flags == 0)
	return;

    /*
     * Find the intersection point on each affected edge.
     * Store in intersection[edge_no][i] as fractional (non-integral) indices.
     * vertex_interp( edge_no, start_corner, end_corner, isolevel )
     */
    if ((edge_flags &    1) != 0) vertex_interp(  0, 0, 1, isolevel);
    if ((edge_flags &    2) != 0) vertex_interp(  1, 1, 2, isolevel);
    if ((edge_flags &    4) != 0) vertex_interp(  2, 2, 3, isolevel);
    if ((edge_flags &    8) != 0) vertex_interp(  3, 3, 0, isolevel);
    if ((edge_flags &   16) != 0) vertex_interp(  4, 4, 5, isolevel);
    if ((edge_flags &   32) != 0) vertex_interp(  5, 5, 6, isolevel);
    if ((edge_flags &   64) != 0) vertex_interp(  6, 6, 7, isolevel);
    if ((edge_flags &  128) != 0) vertex_interp(  7, 7, 4, isolevel);
    if ((edge_flags &  256) != 0) vertex_interp(  8, 0, 4, isolevel);
    if ((edge_flags &  512) != 0) vertex_interp(  9, 1, 5, isolevel);
    if ((edge_flags & 1024) != 0) vertex_interp( 10, 2, 6, isolevel);
    if ((edge_flags & 2048) != 0) vertex_interp( 11, 3, 7, isolevel);

    /*
     * Convert the content of intersection[][] from fractional indices
     * to plot coordinates
     */
    for (iedge = 0; iedge < 12; iedge++) {
	intersection[iedge][0] =
	    vgrid->vxmin + ((double)ix + intersection[iedge][0]) * vgrid->vxdelta;
	intersection[iedge][1] =
	    vgrid->vymin + ((double)iy + intersection[iedge][1]) * vgrid->vydelta;
	intersection[iedge][2] =
	    vgrid->vzmin + ((double)iz + intersection[iedge][2]) * vgrid->vzdelta;
    }

    if (isosurface_options.tessellation == 0) {
	/*
	 * Draw a mixture of quadrangles and triangles
	 */
	for (it = 0; it < 3; it++) {
	    gpdPoint quad[4];	/* The structure expected by gnuplot's pm3d */

	    if (qt_table[corner_flags][4*it] < 0)
		break;

	    ivertex = qt_table[corner_flags][4*it]; /* first vertex */
		    quad[0].x = intersection[ivertex][0];
		    quad[0].y = intersection[ivertex][1];
		    quad[0].z = intersection[ivertex][2];
	    ivertex = qt_table[corner_flags][4*it+1]; /* second */
		    quad[1].x = intersection[ivertex][0];
		    quad[1].y = intersection[ivertex][1];
		    quad[1].z = intersection[ivertex][2];
	    ivertex = qt_table[corner_flags][4*it+2]; /* third */
		    quad[2].x = intersection[ivertex][0];
		    quad[2].y = intersection[ivertex][1];
		    quad[2].z = intersection[ivertex][2];
	    /* 4th vertex == -1 indicates a triangle */
	    /* repeat the 3rd vertex to treat it as a degenerate quadrangle */
	    if (qt_table[corner_flags][4*it+3] >= 0)
		ivertex = qt_table[corner_flags][4*it+3]; /* fourth */
		    quad[3].x = intersection[ivertex][0];
		    quad[3].y = intersection[ivertex][1];
		    quad[3].z = intersection[ivertex][2];

	    /* Color choice */
	    quad[0].c = plot->lp_properties.pm3d_color.lt;

	    /* Debugging aid: light up all facets of the same class */
	    if (debug > 0 && debug == corner_flags)
		quad[0].c = 6+it;

	    /* Hand off this facet to the pm3d code */
	    pm3d_add_quadrangle( plot, quad );
	}

    } else {
	/*
	 * Draw the triangles from a purely triangular tessellation.
	 * There can be up to four per voxel cube.
	 */
	for (it = 0; it < 4; it++) {
	    gpdPoint quad[4];	/* The structure expected by gnuplot's pm3d */

	    if (triangle_table[corner_flags][3*it] < 0)
		break;

	    ivertex = triangle_table[corner_flags][3*it]; /* first vertex */
		    quad[0].x = intersection[ivertex][0];
		    quad[0].y = intersection[ivertex][1];
		    quad[0].z = intersection[ivertex][2];
	    ivertex = triangle_table[corner_flags][3*it+1]; /* second */
		    quad[1].x = intersection[ivertex][0];
		    quad[1].y = intersection[ivertex][1];
		    quad[1].z = intersection[ivertex][2];
	    ivertex = triangle_table[corner_flags][3*it+2]; /* third */
		    quad[2].x = intersection[ivertex][0];
		    quad[2].y = intersection[ivertex][1];
		    quad[2].z = intersection[ivertex][2];
	    /* pm3d always wants a quadrangle, so repeat the 3rd vertex */
		    quad[3] = quad[2];

	    /* Color choice */
	    quad[0].c = plot->lp_properties.pm3d_color.lt;

	    /* Hand off this triangle to the pm3d code */
	    pm3d_add_quadrangle( plot, quad );
	}
    }

}

static void
vertex_interp( int edge_no, int start, int end, t_voxel isolevel )
{
    double fracindex;
    int i;

    for (i=0; i<3; i++) {
	if (vertex_offset[end][i] == vertex_offset[start][i])
	    fracindex = 0;
	else
	    fracindex = (scaled_offset[end][i] - scaled_offset[start][i])
		      * (isolevel - cornervalue[start])
		      / (cornervalue[end] - cornervalue[start]);
	intersection[edge_no][i] = scaled_offset[start][i] + fracindex;
    }
}

#endif /* VOXEL_GRID_SUPPORT */

#ifndef VOXEL_GRID_SUPPORT
void vplot_points (struct surface_points *plot, double level) {};
void vplot_isosurface (struct surface_points *plot, int downsample) {};
#endif
